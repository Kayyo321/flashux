#include "test.h"
#include "helper/ipc_protocol.h"
#include "common/wire.h"
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
struct fragmented {
    int fd;
    unsigned char bytes[20];
};
static void *send_fragmented(void *data)
{
    struct fragmented *fragment = data;
    for (size_t i = 0; i < sizeof(fragment->bytes); ++i) {
        CHECK(write(fragment->fd, fragment->bytes + i, 1) == 1);
    }
    return NULL;
}
int main(void)
{
    int sockets[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    flashux_ipc_header_t header = {FLASHUX_IPC_MAGIC, 1, FLASHUX_IPC_MSG_OPERATION_RESULT, 4, 7};
    struct fragmented fragmented = {.fd = sockets[0]};
    CHECK(flashux_ipc_encode_header(&header, fragmented.bytes) == FLASHUX_SUCCESS);
    flashux_put_u32(fragmented.bytes + 16, 10);
    pthread_t sender;
    CHECK(pthread_create(&sender, NULL, send_fragmented, &fragmented) == 0);
    unsigned char payload[4];
    CHECK(flashux_ipc_receive(sockets[1], &header, payload, sizeof(payload)) == FLASHUX_SUCCESS);
    CHECK(header.sequence_id == 7 && flashux_get_u32(payload) == 10);
    CHECK(pthread_join(sender, NULL) == 0);
    char path[] = "/tmp/flashux-fd-XXXXXX";
    int fd = mkstemp(path);
    CHECK(fd >= 0 && unlink(path) == 0);
    CHECK(flashux_ipc_send_fd(sockets[0], fd) == FLASHUX_SUCCESS);
    int received = -1;
    CHECK(flashux_ipc_receive_fd(sockets[1], &received) == FLASHUX_SUCCESS);
    struct stat original;
    struct stat duplicate;
    CHECK(fstat(fd, &original) == 0 && fstat(received, &duplicate) == 0);
    CHECK(original.st_ino == duplicate.st_ino && original.st_dev == duplicate.st_dev);
    CHECK(close(fd) == 0 && close(received) == 0);
    flashux_flash_options_t options = {0};
    flashux_flash_options_t decoded;
    unsigned char option_bytes[FLASHUX_IPC_OPTIONS_SIZE];
    options.dry_run = true;
    CHECK(flashux_ipc_encode_options(&options, option_bytes) == FLASHUX_SUCCESS);
    CHECK(flashux_ipc_decode_options(option_bytes, sizeof(option_bytes), &decoded) == FLASHUX_SUCCESS);
    CHECK(decoded.dry_run);
    option_bytes[737] = 2;
    CHECK(flashux_ipc_decode_options(option_bytes, sizeof(option_bytes), &decoded) == FLASHUX_ERR_IPC);
    option_bytes[737] = 1;
    memset(option_bytes, 'x', 64);
    CHECK(flashux_ipc_decode_options(option_bytes, sizeof(option_bytes), &decoded) == FLASHUX_ERR_IPC);
    CHECK(flashux_ipc_send(sockets[0], FLASHUX_IPC_CMD_PING, 1, payload, 65537) == FLASHUX_ERR_IPC);
    fragmented.bytes[0] = 0;
    CHECK(write(sockets[0], fragmented.bytes, 16) == 16);
    CHECK(flashux_ipc_receive(sockets[1], &header, payload, sizeof(payload)) == FLASHUX_ERR_IPC);
    flashux_progress_t sent_progress = {.stage = FLASHUX_STAGE_WRITING,
        .bytes_processed = 300, .total_bytes = 1000, .elapsed_nanos = UINT64_C(2000000000),
        .current_speed_bps = 125.25};
    flashux_progress_t got_progress;
    unsigned char progress_bytes[FLASHUX_IPC_PROGRESS_SIZE];
    flashux_ipc_encode_progress(&sent_progress, progress_bytes);
    CHECK(flashux_ipc_decode_progress(progress_bytes, sizeof(progress_bytes), &got_progress)
          == FLASHUX_SUCCESS);
    CHECK(got_progress.current_speed_bps == 125.25 && got_progress.average_speed_bps == 150.0);
    flashux_put_u64(progress_bytes + 32, UINT64_C(0x7ff0000000000000));
    CHECK(flashux_ipc_decode_progress(progress_bytes, sizeof(progress_bytes), &got_progress)
          == FLASHUX_ERR_IPC);
    CHECK(close(sockets[0]) == 0);
    CHECK(flashux_ipc_receive(sockets[1], &header, payload, sizeof(payload)) == FLASHUX_ERR_IPC);
    CHECK(close(sockets[1]) == 0);
    return 0;
}
