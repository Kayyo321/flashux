#include "internal.h"
#include "progress.h"
#include "image_stream.h"
#include "ipc_client.h"
#include "helper_launcher.h"
#include "device_internal.h"
#include "common/wire.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <poll.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static flashux_error_t invalid(char *message, size_t size, flashux_error_t code,
                               const char *reason)
{
    if (message != NULL && size > 0) { (void)snprintf(message, size, "%s", reason); }
    return code;
}
flashux_error_t flashux_flash_validate_options(flashux_context_t *ctx,
                                              const flashux_flash_options_t *options,
                                              char *error_message_buf, size_t buf_size)
{
    struct stat image;
    struct stat target;
    if (error_message_buf != NULL && buf_size > 0) { error_message_buf[0] = '\0'; }
    if (ctx == NULL || options == NULL ||
        memchr(options->source_image_path, 0, sizeof(options->source_image_path)) == NULL ||
        memchr(options->target_device_path, 0, sizeof(options->target_device_path)) == NULL ||
        memchr(options->volume_label, 0, sizeof(options->volume_label)) == NULL ||
        memchr(options->win_custom.local_username, 0, sizeof(options->win_custom.local_username))
            == NULL || options->mode < FLASHUX_MODE_DD_RAW || options->mode > FLASHUX_MODE_FREEDOS ||
        options->part_scheme < FLASHUX_PARTITION_SCHEME_MBR ||
        options->part_scheme > FLASHUX_PARTITION_SCHEME_HYBRID ||
        options->target_sys < FLASHUX_TARGET_BIOS_CSM || options->target_sys > FLASHUX_TARGET_UEFI_NTFS ||
        options->fs_type < FLASHUX_FS_FAT16 || options->fs_type > FLASHUX_FS_UDF) {
        return invalid(error_message_buf, buf_size, FLASHUX_ERR_INVALID_ARG, "Invalid flash options");
    }
    const flashux_windows_customizations_t *win = &options->win_custom;
    if (options->mode != FLASHUX_MODE_DD_RAW || options->bad_blocks_passes != 0 ||
        options->persistence_size_bytes != 0 || win->bypass_tpm_secureboot_ram ||
        win->bypass_online_account || win->create_local_account || win->match_regional_options ||
        win->disable_telemetry || win->disable_bitlocker) {
        return invalid(error_message_buf, buf_size, FLASHUX_ERR_UNSUPPORTED,
                       "This foundation supports raw image jobs without setup customizations");
    }
    int source = open(options->source_image_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (source < 0) {
        return invalid(error_message_buf, buf_size, FLASHUX_ERR_NOT_FOUND, "Image cannot be opened");
    }
    unsigned char magic[8] = {0};
    ssize_t count = pread(source, magic, sizeof(magic), 0);
    bool regular = fstat(source, &image) == 0 && S_ISREG(image.st_mode) && image.st_size > 0;
    (void)close(source);
    if (!regular || count < 0) {
        return invalid(error_message_buf, buf_size, FLASHUX_ERR_INVALID_ARG,
                       "Source must be a nonempty regular image file");
    }
    bool compressed = (magic[0] == 0x1f && magic[1] == 0x8b) ||
        memcmp(magic, "\xfd" "7zXZ", 5) == 0 || memcmp(magic, "BZh", 3) == 0 ||
        memcmp(magic, "PK\003\004", 4) == 0 || memcmp(magic, "\x28\xb5\x2f\xfd", 4) == 0;
    if (stat(options->target_device_path, &target) != 0 ||
        (target.st_dev == image.st_dev && target.st_ino == image.st_ino)) {
        return invalid(error_message_buf, buf_size, FLASHUX_ERR_INVALID_ARG, "Invalid target identity");
    }
    if (options->dry_run && S_ISREG(target.st_mode)) {
        return target.st_size > 0 && (compressed || target.st_size >= image.st_size) ? FLASHUX_SUCCESS :
            invalid(error_message_buf, buf_size, FLASHUX_ERR_INVALID_ARG, "Target capacity too small");
    }
    if (strncmp(options->target_device_path, "/dev/", 5) != 0 ||
        strstr(options->target_device_path, "..") != NULL || !S_ISBLK(target.st_mode)) {
        return invalid(error_message_buf, buf_size, FLASHUX_ERR_INVALID_ARG,
                       "Writes require a canonical Linux block device");
    }
    flashux_device_t *list = NULL;
    flashux_error_t result = flashux_device_list_scan(ctx, true, &list);
    if (result == FLASHUX_SUCCESS) {
        result = FLASHUX_ERR_NOT_FOUND;
        for (flashux_device_t *device = list; device != NULL; device = device->next) {
            struct stat node;
            if (stat(device->node_path, &node) == 0 && node.st_rdev == target.st_rdev) {
                if (!flashux_device_is_safe_target(ctx, device, error_message_buf, buf_size)) {
                    result = FLASHUX_ERR_SYSTEM_DISK_PROTECT;
                } else if (!compressed && (uint64_t)image.st_size > device->size_bytes) {
                    result = invalid(error_message_buf, buf_size, FLASHUX_ERR_INVALID_ARG,
                                     "Target capacity too small");
                } else { result = FLASHUX_SUCCESS; }
                break;
            }
        }
    }
    flashux_device_list_free(&list);
    return result;
}
static void report(flashux_context_t *ctx, flashux_progress_t *progress,
                    flashux_stage_t stage, const char *text)
{
    progress->stage = stage;
    (void)snprintf(progress->stage_description, sizeof(progress->stage_description), "%s", text);
    if (ctx->progress_cb != NULL) { ctx->progress_cb(progress, ctx->progress_data); }
}
static bool job_cancelled(void *data)
{
    flashux_context_t *ctx = data;
    return atomic_load(&ctx->cancel);
}
static flashux_error_t dry_run(flashux_context_t *ctx, flashux_progress_t *progress)
{
    struct stat source;
    struct stat target;
    unsigned char buffer[65536];
    int fd = open(ctx->options.source_image_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) { return FLASHUX_ERR_NOT_FOUND; }
    if (fstat(fd, &source) != 0 || !S_ISREG(source.st_mode) || source.st_size <= 0 ||
        stat(ctx->options.target_device_path, &target) != 0) {
        (void)close(fd);
        return FLASHUX_ERR_INVALID_ARG;
    }
    uint64_t capacity = 0;
    if (S_ISREG(target.st_mode) && target.st_size > 0) { capacity = (uint64_t)target.st_size; }
    else {
        flashux_device_t *list = NULL;
        flashux_error_t scanned = flashux_device_list_scan(ctx, true, &list);
        if (scanned == FLASHUX_SUCCESS) {
            for (flashux_device_t *item = list; item != NULL; item = item->next) {
                struct stat node;
                if (stat(item->node_path, &node) == 0 && node.st_rdev == target.st_rdev &&
                    flashux_device_is_safe_target(ctx, item, NULL, 0)) {
                    capacity = item->size_bytes;
                    break;
                }
            }
        }
        flashux_device_list_free(&list);
    }
    uint64_t total = 0;
    flashux_error_t result = capacity == 0 ? FLASHUX_ERR_INVALID_ARG :
        flashux_image_decoded_size(fd, capacity, &total, job_cancelled, ctx,
                                  ctx->progress_cb, ctx->progress_data);
    flashux_image_stream_t *stream = NULL;
    if (result == FLASHUX_SUCCESS) { result = flashux_image_stream_open(fd, &stream); }
    uint64_t start = flashux_monotonic_nanos();
    uint64_t bytes = 0;
    memset(progress, 0, sizeof(*progress));
    while (result == FLASHUX_SUCCESS) {
        if (atomic_load(&ctx->cancel)) { result = FLASHUX_ERR_CANCELLED; break; }
        size_t count = 0;
        result = flashux_image_stream_read(stream, buffer, sizeof(buffer), &count);
        if (result != FLASHUX_SUCCESS || count == 0) { break; }
        if ((uint64_t)count > total - bytes) { result = FLASHUX_ERR_CORRUPT_IMAGE; break; }
        uint64_t previous = bytes;
        bytes += count;
        (void)flashux_progress_update(progress, bytes, total,
            flashux_monotonic_nanos() - start, previous, progress->elapsed_nanos);
        report(ctx, progress, FLASHUX_STAGE_WRITING, "Dry run: decoding source; target untouched");
    }
    if (result == FLASHUX_SUCCESS && bytes != total) { result = FLASHUX_ERR_CORRUPT_IMAGE; }
    flashux_image_stream_close(&stream);
    (void)close(fd);
    return result;
}
static flashux_error_t remote_job(flashux_context_t *ctx, flashux_progress_t *progress)
{
    flashux_ipc_client_t *client = NULL;
    unsigned char options[FLASHUX_IPC_OPTIONS_SIZE];
    unsigned char payload[FLASHUX_IPC_MAX_PAYLOAD];
    flashux_error_t result = flashux_ipc_encode_options(&ctx->options, options);
    if (result == FLASHUX_SUCCESS) {
        report(ctx, progress, FLASHUX_STAGE_VERIFYING_LOCK, "Waiting for helper authorization");
        result = flashux_helper_connect_or_launch(ctx, &client);
    }
    if (result == FLASHUX_SUCCESS) {
        result = flashux_ipc_client_send_cmd(client, FLASHUX_IPC_CMD_START_FLASH,
                                            options, sizeof(options));
    }
    if (result == FLASHUX_SUCCESS) {
        int source_fd = open(ctx->options.source_image_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        if (source_fd < 0) { result = FLASHUX_ERR_NOT_FOUND; }
        else {
            result = flashux_ipc_send_fd(flashux_ipc_client_fd(client), source_fd);
            (void)close(source_fd);
        }
    }
    uint64_t last_message = flashux_monotonic_nanos();
    bool sent_cancel = false;
    while (result == FLASHUX_SUCCESS) {
        if (atomic_load(&ctx->cancel) && !sent_cancel) {
            result = flashux_ipc_client_send_cmd(client, FLASHUX_IPC_CMD_CANCEL_FLASH, NULL, 0);
            sent_cancel = true;
            if (result != FLASHUX_SUCCESS) { break; }
        }
        struct pollfd wait_fd = {flashux_ipc_client_fd(client), POLLIN, 0};
        int ready = poll(&wait_fd, 1, 100);
        if (ready < 0 && errno == EINTR) { continue; }
        if (ready < 0) { result = FLASHUX_ERR_IPC; break; }
        if (ready == 0) {
            if (flashux_monotonic_nanos() - last_message > UINT64_C(30000000000)) {
                result = FLASHUX_ERR_IPC;
                break;
            }
            continue;
        }
        last_message = flashux_monotonic_nanos();
        flashux_ipc_header_t header;
        result = flashux_ipc_client_recv_msg(client, &header, payload, sizeof(payload));
        if (result != FLASHUX_SUCCESS) { break; }
        if (header.msg_type == FLASHUX_IPC_MSG_PROGRESS_UPDATE) {
            result = flashux_ipc_decode_progress(payload, header.payload_len, progress);
            if (result == FLASHUX_SUCCESS && ctx->progress_cb != NULL) {
                ctx->progress_cb(progress, ctx->progress_data);
            }
        } else if (header.msg_type == FLASHUX_IPC_MSG_OPERATION_RESULT && header.payload_len == 4) {
            uint32_t code = flashux_get_u32(payload);
            if (code <= 11 || code == 99) { result = (flashux_error_t)-(int)code; }
            else { result = FLASHUX_ERR_IPC; }
            break;
        } else { result = FLASHUX_ERR_IPC; break; }
    }
    flashux_ipc_client_disconnect(&client);
    return result;
}
static void *flash_worker(void *data)
{
    flashux_context_t *ctx = data;
    flashux_progress_t progress = {0};
    report(ctx, &progress, FLASHUX_STAGE_VERIFYING_LOCK, "Validating job");
    flashux_error_t result = atomic_load(&ctx->cancel) ? FLASHUX_ERR_CANCELLED :
        ctx->options.dry_run ? dry_run(ctx, &progress) : remote_job(ctx, &progress);
    report(ctx, &progress, result == FLASHUX_SUCCESS ? FLASHUX_STAGE_COMPLETED : FLASHUX_STAGE_FAILED,
           flashux_strerror(result));
    (void)pthread_mutex_lock(&ctx->job_mutex);
    ctx->result = result;
    ctx->running = false;
    (void)pthread_mutex_unlock(&ctx->job_mutex);
    return NULL;
}
flashux_error_t flashux_flash_start_async(flashux_context_t *ctx,
                                         const flashux_flash_options_t *options,
                                         flashux_progress_cb progress_callback, void *user_data)
{
    flashux_error_t result = flashux_flash_validate_options(ctx, options, NULL, 0);
    if (result != FLASHUX_SUCCESS) { return result; }
    (void)pthread_mutex_lock(&ctx->job_mutex);
    if (ctx->running || ctx->joinable || ctx->joining) {
        (void)pthread_mutex_unlock(&ctx->job_mutex);
        return FLASHUX_ERR_DEVICE_BUSY;
    }
    ctx->options = *options;
    if (!options->dry_run) {
        uint64_t current_sequence;
        result = flashux_device_capture_sequence(options->target_device_path, &current_sequence);
        if (result == FLASHUX_SUCCESS && options->expected_disk_sequence != 0 &&
            options->expected_disk_sequence != current_sequence) { result = FLASHUX_ERR_NOT_FOUND; }
        if (result != FLASHUX_SUCCESS) {
            (void)pthread_mutex_unlock(&ctx->job_mutex);
            return result;
        }
        ctx->options.expected_disk_sequence = current_sequence;
    }
    ctx->progress_cb = progress_callback;
    ctx->progress_data = user_data;
    ctx->running = true;
    atomic_store(&ctx->cancel, false);
    if (pthread_create(&ctx->worker, NULL, flash_worker, ctx) != 0) {
        ctx->running = false;
        result = FLASHUX_ERR_UNKNOWN;
    } else { ctx->joinable = true; }
    (void)pthread_mutex_unlock(&ctx->job_mutex);
    return result;
}
flashux_error_t flashux_flash_cancel(flashux_context_t *ctx)
{
    if (ctx == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    atomic_store(&ctx->cancel, true);
    return FLASHUX_SUCCESS;
}
bool flashux_flash_is_running(flashux_context_t *ctx)
{
    if (ctx == NULL) { return false; }
    (void)pthread_mutex_lock(&ctx->job_mutex);
    bool running = ctx->running;
    (void)pthread_mutex_unlock(&ctx->job_mutex);
    return running;
}
flashux_error_t flashux_flash_wait(flashux_context_t *ctx)
{
    if (ctx == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    (void)pthread_mutex_lock(&ctx->job_mutex);
    if (!ctx->joinable || ctx->joining || pthread_equal(pthread_self(), ctx->worker)) {
        (void)pthread_mutex_unlock(&ctx->job_mutex);
        return FLASHUX_ERR_INVALID_ARG;
    }
    ctx->joining = true;
    (void)pthread_mutex_unlock(&ctx->job_mutex);
    int error = pthread_join(ctx->worker, NULL);
    (void)pthread_mutex_lock(&ctx->job_mutex);
    ctx->joinable = false;
    ctx->joining = false;
    flashux_error_t result = error == 0 ? ctx->result : FLASHUX_ERR_UNKNOWN;
    (void)pthread_mutex_unlock(&ctx->job_mutex);
    return result;
}
