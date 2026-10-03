#include "helper/ipc_protocol.h"
#include "common/wire.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>

static bool valid_type(uint16_t type)
{
    return (type >= FLASHUX_IPC_CMD_PING && type <= FLASHUX_IPC_CMD_CANCEL_FLASH) ||
        type == FLASHUX_IPC_MSG_PROGRESS_UPDATE || type == FLASHUX_IPC_MSG_LOG_EVENT ||
        type == FLASHUX_IPC_MSG_OPERATION_RESULT;
}
flashux_error_t flashux_ipc_encode_header(const flashux_ipc_header_t *header,
                                        unsigned char out[16])
{
    if (header == NULL || out == NULL || header->magic != FLASHUX_IPC_MAGIC ||
        header->version != 1 || !valid_type(header->msg_type) ||
        header->payload_len > FLASHUX_IPC_MAX_PAYLOAD) {
        return FLASHUX_ERR_IPC;
    }
    flashux_put_u32(out, header->magic);
    out[4] = 1; out[5] = 0;
    out[6] = (unsigned char)header->msg_type;
    out[7] = (unsigned char)(header->msg_type >> 8);
    flashux_put_u32(out + 8, header->payload_len);
    flashux_put_u32(out + 12, header->sequence_id);
    return FLASHUX_SUCCESS;
}
flashux_error_t flashux_ipc_decode_header(const unsigned char in[16],
                                        flashux_ipc_header_t *header)
{
    unsigned char check[16];
    if (in == NULL || header == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    header->magic = flashux_get_u32(in);
    header->version = (uint16_t)((uint16_t)in[4] | ((uint16_t)in[5] << 8));
    header->msg_type = (uint16_t)((uint16_t)in[6] | ((uint16_t)in[7] << 8));
    header->payload_len = flashux_get_u32(in + 8);
    header->sequence_id = flashux_get_u32(in + 12);
    return flashux_ipc_encode_header(header, check);
}
static flashux_error_t transfer(int fd, unsigned char *data, size_t size, bool sending)
{
    while (size > 0) {
        ssize_t count;
        if (sending) {
#ifdef MSG_NOSIGNAL
            count = send(fd, data, size, MSG_NOSIGNAL);
#else
            int enabled = 1;
            (void)setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
            count = send(fd, data, size, 0);
#endif
        } else {
            count = recv(fd, data, size, 0);
        }
        if (count < 0 && errno == EINTR) { continue; }
        if (count <= 0) { return FLASHUX_ERR_IPC; }
        data += (size_t)count;
        size -= (size_t)count;
    }
    return FLASHUX_SUCCESS;
}
flashux_error_t flashux_ipc_send(int fd, flashux_ipc_msg_type_t type, uint32_t sequence,
                               const void *payload, size_t size)
{
    unsigned char wire[16];
    unsigned char body[FLASHUX_IPC_MAX_PAYLOAD];
    if (size > FLASHUX_IPC_MAX_PAYLOAD || (size != 0 && payload == NULL)) {
        return FLASHUX_ERR_IPC;
    }
    flashux_ipc_header_t header = {FLASHUX_IPC_MAGIC, 1, (uint16_t)type,
                                 (uint32_t)size, sequence};
    flashux_error_t result = flashux_ipc_encode_header(&header, wire);
    if (result == FLASHUX_SUCCESS) { result = transfer(fd, wire, sizeof(wire), true); }
    if (result == FLASHUX_SUCCESS && size != 0) {
        memcpy(body, payload, size);
        result = transfer(fd, body, size, true);
    }
    return result;
}
flashux_error_t flashux_ipc_receive(int fd, flashux_ipc_header_t *header,
                                  void *payload, size_t capacity)
{
    unsigned char wire[16];
    flashux_error_t result = transfer(fd, wire, sizeof(wire), false);
    if (result == FLASHUX_SUCCESS) { result = flashux_ipc_decode_header(wire, header); }
    if (result != FLASHUX_SUCCESS) { return result; }
    if (header->payload_len > capacity || (header->payload_len != 0 && payload == NULL)) {
        return FLASHUX_ERR_IPC; /* Caller closes stream after a framing failure. */
    }
    return transfer(fd, payload, header->payload_len, false);
}
flashux_error_t flashux_ipc_encode_options(const flashux_flash_options_t *options,
                                         unsigned char out[FLASHUX_IPC_OPTIONS_SIZE])
{
    if (options == NULL || out == NULL ||
        memchr(options->target_device_path, 0, 64) == NULL ||
        memchr(options->source_image_path, 0, 512) == NULL ||
        memchr(options->volume_label, 0, 128) == NULL ||
        memchr(options->win_custom.local_username, 0, 64) == NULL) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    memset(out, 0, FLASHUX_IPC_OPTIONS_SIZE);
    memcpy(out, options->target_device_path, 64);
    memcpy(out + 64, options->source_image_path, 512);
    memcpy(out + 576, options->volume_label, 128);
    flashux_put_u32(out + 704, (uint32_t)options->mode);
    flashux_put_u32(out + 708, (uint32_t)options->part_scheme);
    flashux_put_u32(out + 712, (uint32_t)options->target_sys);
    flashux_put_u32(out + 716, (uint32_t)options->fs_type);
    flashux_put_u32(out + 720, options->cluster_size);
    flashux_put_u32(out + 724, options->bad_blocks_passes);
    flashux_put_u64(out + 728, options->persistence_size_bytes);
    out[736] = options->quick_format;
    out[737] = options->dry_run;
    out[738] = options->win_custom.bypass_tpm_secureboot_ram;
    out[739] = options->win_custom.bypass_online_account;
    out[740] = options->win_custom.create_local_account;
    out[741] = options->win_custom.match_regional_options;
    out[742] = options->win_custom.disable_telemetry;
    out[743] = options->win_custom.disable_bitlocker;
    memcpy(out + 744, options->win_custom.local_username, 64);
    flashux_put_u64(out + 808, options->expected_disk_sequence);
    return FLASHUX_SUCCESS;
}
flashux_error_t flashux_ipc_decode_options(const unsigned char *in, size_t size,
                                         flashux_flash_options_t *options)
{
    if (in == NULL || options == NULL || size != FLASHUX_IPC_OPTIONS_SIZE ||
        memchr(in, 0, 64) == NULL || memchr(in + 64, 0, 512) == NULL ||
        memchr(in + 576, 0, 128) == NULL || memchr(in + 744, 0, 64) == NULL) {
        return FLASHUX_ERR_IPC;
    }
    for (unsigned i = 736; i < 744; ++i) {
        if (in[i] > 1) { return FLASHUX_ERR_IPC; }
    }
    for (unsigned i = 816; i < FLASHUX_IPC_OPTIONS_SIZE; ++i) {
        if (in[i] != 0) { return FLASHUX_ERR_IPC; }
    }
    if (flashux_get_u32(in + 704) > FLASHUX_MODE_FREEDOS ||
        flashux_get_u32(in + 708) > FLASHUX_PARTITION_SCHEME_HYBRID ||
        flashux_get_u32(in + 712) > FLASHUX_TARGET_UEFI_NTFS ||
        flashux_get_u32(in + 716) > FLASHUX_FS_UDF) { return FLASHUX_ERR_IPC; }
    memset(options, 0, sizeof(*options));
    memcpy(options->target_device_path, in, 64);
    memcpy(options->source_image_path, in + 64, 512);
    memcpy(options->volume_label, in + 576, 128);
    options->mode = (flashux_flash_mode_t)flashux_get_u32(in + 704);
    options->part_scheme = (flashux_partition_scheme_t)flashux_get_u32(in + 708);
    options->target_sys = (flashux_target_system_t)flashux_get_u32(in + 712);
    options->fs_type = (flashux_fs_type_t)flashux_get_u32(in + 716);
    options->cluster_size = flashux_get_u32(in + 720);
    options->bad_blocks_passes = flashux_get_u32(in + 724);
    options->persistence_size_bytes = flashux_get_u64(in + 728);
    options->expected_disk_sequence = flashux_get_u64(in + 808);
    options->quick_format = in[736] != 0;
    options->dry_run = in[737] != 0;
    options->win_custom.bypass_tpm_secureboot_ram = in[738] != 0;
    options->win_custom.bypass_online_account = in[739] != 0;
    options->win_custom.create_local_account = in[740] != 0;
    options->win_custom.match_regional_options = in[741] != 0;
    options->win_custom.disable_telemetry = in[742] != 0;
    options->win_custom.disable_bitlocker = in[743] != 0;
    memcpy(options->win_custom.local_username, in + 744, 64);
    return FLASHUX_SUCCESS;
}
void flashux_ipc_encode_progress(const flashux_progress_t *progress,
                                unsigned char out[FLASHUX_IPC_PROGRESS_SIZE])
{
    memset(out, 0, FLASHUX_IPC_PROGRESS_SIZE);
    flashux_put_u32(out, (uint32_t)progress->stage);
    flashux_put_u64(out + 4, progress->bytes_processed);
    flashux_put_u64(out + 12, progress->total_bytes);
    flashux_put_u64(out + 20, progress->elapsed_nanos);
    flashux_put_u32(out + 28, progress->eta_seconds);
    /* Derive floating metrics at the receiver; no ABI-dependent doubles on wire. */
    memcpy(out + 40, progress->stage_description, 128);
}
flashux_error_t flashux_ipc_decode_progress(const unsigned char *in, size_t size,
                                          flashux_progress_t *progress)
{
    if (in == NULL || progress == NULL || size != FLASHUX_IPC_PROGRESS_SIZE ||
        flashux_get_u32(in) > FLASHUX_STAGE_FAILED || memchr(in + 40, 0, 128) == NULL) {
        return FLASHUX_ERR_IPC;
    }
    memset(progress, 0, sizeof(*progress));
    progress->stage = (flashux_stage_t)flashux_get_u32(in);
    progress->bytes_processed = flashux_get_u64(in + 4);
    progress->total_bytes = flashux_get_u64(in + 12);
    progress->elapsed_nanos = flashux_get_u64(in + 20);
    progress->eta_seconds = flashux_get_u32(in + 28);
    if (progress->bytes_processed > progress->total_bytes) { return FLASHUX_ERR_IPC; }
    progress->percent_complete = progress->total_bytes == 0 ? 0 :
        100.0 * (double)progress->bytes_processed / (double)progress->total_bytes;
    progress->average_speed_bps = progress->elapsed_nanos == 0 ? 0 :
        (double)progress->bytes_processed * 1e9 / (double)progress->elapsed_nanos;
    progress->current_speed_bps = progress->average_speed_bps;
    memcpy(progress->stage_description, in + 40, 128);
    return FLASHUX_SUCCESS;
}

flashux_error_t flashux_ipc_send_fd(int socket_fd, int file_fd)
{
    unsigned char marker = 0x46;
    struct iovec io = {&marker, 1};
    union { struct cmsghdr alignment; unsigned char bytes[CMSG_SPACE(sizeof(int))]; } control;
    struct msghdr message = {0};
    memset(&control, 0, sizeof(control));
    message.msg_iov = &io;
    message.msg_iovlen = 1;
    message.msg_control = control.bytes;
    message.msg_controllen = sizeof(control.bytes);
    struct cmsghdr *header = CMSG_FIRSTHDR(&message);
    header->cmsg_level = SOL_SOCKET;
    header->cmsg_type = SCM_RIGHTS;
    header->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(header), &file_fd, sizeof(file_fd));
    ssize_t sent;
#ifdef MSG_NOSIGNAL
    do { sent = sendmsg(socket_fd, &message, MSG_NOSIGNAL); } while (sent < 0 && errno == EINTR);
#else
    int enabled = 1;
    (void)setsockopt(socket_fd, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled));
    do { sent = sendmsg(socket_fd, &message, 0); } while (sent < 0 && errno == EINTR);
#endif
    return sent == 1 ? FLASHUX_SUCCESS : FLASHUX_ERR_IPC;
}
flashux_error_t flashux_ipc_receive_fd(int socket_fd, int *file_fd)
{
    unsigned char marker = 0;
    struct iovec io = {&marker, 1};
    union { struct cmsghdr alignment; unsigned char bytes[CMSG_SPACE(sizeof(int))]; } control;
    struct msghdr message = {0};
    if (file_fd == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    *file_fd = -1;
    memset(&control, 0, sizeof(control));
    message.msg_iov = &io;
    message.msg_iovlen = 1;
    message.msg_control = control.bytes;
    message.msg_controllen = sizeof(control.bytes);
    ssize_t got;
    do { got = recvmsg(socket_fd, &message, 0); } while (got < 0 && errno == EINTR);
    struct cmsghdr *header = CMSG_FIRSTHDR(&message);
    bool valid = got == 1 && marker == 0x46 && (message.msg_flags & (MSG_CTRUNC | MSG_TRUNC)) == 0;
    for (; header != NULL; header = CMSG_NXTHDR(&message, header)) {
        if (header->cmsg_level == SOL_SOCKET && header->cmsg_type == SCM_RIGHTS &&
            header->cmsg_len >= CMSG_LEN(sizeof(int))) {
            size_t count = (header->cmsg_len - CMSG_LEN(0)) / sizeof(int);
            for (size_t i = 0; i < count; ++i) {
                int received;
                memcpy(&received, CMSG_DATA(header) + i * sizeof(int), sizeof(received));
                if (*file_fd == -1 && count == 1 && valid) {
                    *file_fd = received;
                } else {
                    (void)close(received);
                    valid = false;
                }
            }
        } else { valid = false; }
    }
    if (!valid || *file_fd == -1) {
        if (*file_fd >= 0) { (void)close(*file_fd); *file_fd = -1; }
        return FLASHUX_ERR_IPC;
    }
    if (fcntl(*file_fd, F_SETFD, FD_CLOEXEC) != 0) {
        (void)close(*file_fd);
        *file_fd = -1;
        return FLASHUX_ERR_IPC;
    }
    return FLASHUX_SUCCESS;
}
