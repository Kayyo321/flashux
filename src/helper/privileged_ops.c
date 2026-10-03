#include "privileged_ops.h"
#include "core/progress.h"
#include "core/device_internal.h"
#include "flashux/device.h"
#include "flashux_config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#if FLASHUX_LINUX
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif

static flashux_error_t io_at(int fd, unsigned char *buffer, size_t size, off_t offset,
                             bool writing)
{
    while (size != 0) {
        ssize_t count = writing ? pwrite(fd, buffer, size, offset) : pread(fd, buffer, size, offset);
        if (count < 0 && errno == EINTR) { continue; }
        if (count <= 0) { return FLASHUX_ERR_IO; }
        buffer += (size_t)count;
        size -= (size_t)count;
        offset += count;
    }
    return FLASHUX_SUCCESS;
}
static void emit(flashux_progress_t *progress, flashux_stage_t stage, const char *text,
                  flashux_progress_cb callback, void *data)
{
    progress->stage = stage;
    (void)snprintf(progress->stage_description, sizeof(progress->stage_description), "%s", text);
    if (callback != NULL) { callback(progress, data); }
}
flashux_error_t flashux_raw_copy(int source_fd, int target_fd, uint32_t sector,
                                flashux_progress_cb progress_cb, void *progress_data,
                                flashux_cancel_fn cancel, void *cancel_data)
{
    struct stat source;
    struct stat target;
    flashux_progress_t progress = {0};
    void *allocation = NULL;
    void *verify_allocation = NULL;
    const size_t block = 1024 * 1024;
    flashux_error_t result = FLASHUX_SUCCESS;
    if ((sector != 512 && sector != 4096) || fstat(source_fd, &source) != 0 ||
        fstat(target_fd, &target) != 0 || !S_ISREG(source.st_mode) || source.st_size <= 0 ||
        (source.st_dev == target.st_dev && source.st_ino == target.st_ino) ||
        (uint64_t)source.st_size > (uint64_t)INT64_MAX - sector) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    uint64_t total = (uint64_t)source.st_size;
    uint64_t padded = (total + sector - 1) / sector * sector;
    if (S_ISREG(target.st_mode)) {
        if (target.st_size < 0 || (uint64_t)target.st_size < padded) {
            return FLASHUX_ERR_INVALID_ARG;
        }
    } else {
#if FLASHUX_LINUX
        uint64_t capacity;
        int actual_sector;
        if (!S_ISBLK(target.st_mode) || ioctl(target_fd, BLKGETSIZE64, &capacity) != 0 ||
            ioctl(target_fd, BLKSSZGET, &actual_sector) != 0 ||
            actual_sector != (int)sector || padded > capacity) {
            return FLASHUX_ERR_INVALID_ARG;
        }
        int flags = fcntl(target_fd, F_GETFL);
        if (flags < 0 || fcntl(target_fd, F_SETFL, flags | O_DIRECT) != 0) {
            return FLASHUX_ERR_IO;
        }
#else
        return FLASHUX_ERR_UNSUPPORTED;
#endif
    }
    if (posix_memalign(&allocation, 4096, block) != 0) { return FLASHUX_ERR_NO_MEMORY; }
    if (posix_memalign(&verify_allocation, 4096, block) != 0) {
        free(allocation);
        allocation = NULL;
        return FLASHUX_ERR_NO_MEMORY;
    }
    unsigned char *buffer = allocation;
    unsigned char *verify = verify_allocation;
    uint64_t start = flashux_monotonic_nanos();
    uint64_t bytes = 0;
    while (bytes < total) {
        if (cancel != NULL && cancel(cancel_data)) { result = FLASHUX_ERR_CANCELLED; break; }
        size_t count = total - bytes < block ? (size_t)(total - bytes) : block;
        size_t aligned = (count + sector - 1) / sector * sector;
        memset(buffer, 0, aligned);
        result = io_at(source_fd, buffer, count, (off_t)bytes, false);
        if (result == FLASHUX_SUCCESS) {
            result = io_at(target_fd, buffer, aligned, (off_t)bytes, true);
        }
        if (result != FLASHUX_SUCCESS) { break; }
        uint64_t previous = bytes;
        bytes += count;
        (void)flashux_progress_update(&progress, bytes, total,
            flashux_monotonic_nanos() - start, previous, progress.elapsed_nanos);
        emit(&progress, FLASHUX_STAGE_WRITING, "Writing raw image", progress_cb, progress_data);
    }
    if (result == FLASHUX_SUCCESS) {
        emit(&progress, FLASHUX_STAGE_SYNCING, "Synchronizing target", progress_cb, progress_data);
        if (fsync(target_fd) != 0) { result = FLASHUX_ERR_IO; }
#if FLASHUX_LINUX
        if (S_ISBLK(target.st_mode) && ioctl(target_fd, BLKFLSBUF) != 0) {
            result = FLASHUX_ERR_IO;
        }
#endif
    }
    /* Verify only source bytes; final physical sector padding is deterministic zero. */
    bytes = 0;
    while (result == FLASHUX_SUCCESS && bytes < total) {
        if (cancel != NULL && cancel(cancel_data)) { result = FLASHUX_ERR_CANCELLED; break; }
        size_t count = total - bytes < block ? (size_t)(total - bytes) : block;
        size_t aligned = (count + sector - 1) / sector * sector;
        result = io_at(source_fd, buffer, count, (off_t)bytes, false);
        if (result == FLASHUX_SUCCESS) {
            result = io_at(target_fd, verify, aligned, (off_t)bytes, false);
        }
        if (result == FLASHUX_SUCCESS && memcmp(buffer, verify, count) != 0) {
            result = FLASHUX_ERR_CORRUPT_IMAGE;
        }
        bytes += count;
        emit(&progress, FLASHUX_STAGE_VERIFYING_READ, "Verifying target readback",
             progress_cb, progress_data);
    }
    struct stat after;
    if (result == FLASHUX_SUCCESS && (fstat(source_fd, &after) != 0 ||
        source.st_size != after.st_size || source.st_mtime != after.st_mtime ||
        source.st_ctime != after.st_ctime)) { result = FLASHUX_ERR_IO; }
    if (result != FLASHUX_SUCCESS) { (void)fsync(target_fd); }
    free(allocation);
    allocation = NULL;
    buffer = NULL;
    free(verify_allocation);
    verify_allocation = NULL;
    verify = NULL;
    return result;
}
flashux_error_t flashux_helper_flash(flashux_context_t *ctx,
                                    const flashux_flash_options_t *options, int source_fd,
                                    flashux_progress_cb progress_cb, void *progress_data,
                                    flashux_cancel_fn cancel, void *cancel_data)
{
    char reason[256];
    flashux_device_t *list = NULL;
    flashux_device_t *device = NULL;
    int target_fd = -1;
    struct stat source;
    struct stat selected;
    flashux_error_t result = flashux_flash_validate_options(ctx, options, reason, sizeof(reason));
    if (result != FLASHUX_SUCCESS) { return result; }
    if (options->dry_run || fstat(source_fd, &source) != 0 ||
        stat(options->source_image_path, &selected) != 0 ||
        source.st_dev != selected.st_dev || source.st_ino != selected.st_ino) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    result = flashux_device_list_scan(ctx, true, &list);
    if (result != FLASHUX_SUCCESS) { return result; }
    char canonical[4096];
    if (realpath(options->target_device_path, canonical) == NULL) {
        result = FLASHUX_ERR_NOT_FOUND;
    } else {
        for (flashux_device_t *item = list; item != NULL; item = item->next) {
            if (strcmp(item->node_path, canonical) == 0) { device = item; break; }
        }
        if (!flashux_device_is_safe_target(ctx, device, NULL, 0)) {
            result = FLASHUX_ERR_SYSTEM_DISK_PROTECT;
        } else {
            uint64_t sequence = 0;
            result = flashux_device_capture_sequence(canonical, &sequence);
            if (result == FLASHUX_SUCCESS &&
                (sequence != options->expected_disk_sequence || sequence == 0)) {
                result = FLASHUX_ERR_NOT_FOUND;
            }
            if (result == FLASHUX_SUCCESS) {
                result = flashux_unmount_device_partitions(ctx, device);
            }
            if (result == FLASHUX_SUCCESS) { result = flashux_device_lock(ctx, device, &target_fd); }
            if (result == FLASHUX_SUCCESS) {
#if FLASHUX_LINUX
                uint64_t locked_sequence = 0;
                if (options->expected_disk_sequence == 0 ||
                    ioctl(target_fd, BLKGETDISKSEQ, &locked_sequence) != 0 ||
                    locked_sequence != options->expected_disk_sequence) { result = FLASHUX_ERR_NOT_FOUND; }
#endif
            }
            if (result == FLASHUX_SUCCESS) {
                result = flashux_raw_copy(source_fd, target_fd, device->logical_sector_sz,
                                          progress_cb, progress_data, cancel, cancel_data);
            }
        }
    }
    flashux_device_unlock(&target_fd);
    flashux_device_list_free(&list);
    return result;
}
