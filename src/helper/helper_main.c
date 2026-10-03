#include "flashux/core.h"
#include "flashux_config.h"
#include "ipc_protocol.h"
#include "polkit_auth.h"
#include "privileged_ops.h"
#include "common/wire.h"
#include <stdio.h>
#include <string.h>
#if FLASHUX_LINUX
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>

static volatile sig_atomic_t stopping = 0;
struct connection {
    int fd;
    uint32_t sequence;
    uint32_t last_command;
    bool cancelled;
    bool disconnected;
};
static void stop_helper(int signal_number)
{
    (void)signal_number;
    stopping = 1;
}
static bool should_cancel(void *data)
{
    struct connection *connection = data;
    struct pollfd descriptor = {connection->fd, POLLIN, 0};
    if (stopping || connection->cancelled || connection->disconnected) {
        return true;
    }
    int ready = poll(&descriptor, 1, 0);
    if (ready < 0 && errno == EINTR) {
        return stopping != 0;
    }
    if (ready < 0 || (descriptor.revents & (POLLHUP | POLLERR | POLLNVAL)) != 0) {
        connection->disconnected = true;
        return true;
    }
    if (ready > 0) {
        flashux_ipc_header_t header;
        unsigned char payload[1];
        if (flashux_ipc_receive(connection->fd, &header, payload, sizeof(payload)) !=
                FLASHUX_SUCCESS ||
            header.msg_type != FLASHUX_IPC_CMD_CANCEL_FLASH || header.payload_len != 0 ||
            header.sequence_id <= connection->last_command) {
            connection->disconnected = true;
        } else {
            connection->last_command = header.sequence_id;
            connection->cancelled = true;
        }
    }
    return connection->cancelled || connection->disconnected;
}
static void send_progress(const flashux_progress_t *progress, void *data)
{
    struct connection *connection = data;
    unsigned char payload[FLASHUX_IPC_PROGRESS_SIZE];
    flashux_ipc_encode_progress(progress, payload);
    if (flashux_ipc_send(connection->fd, FLASHUX_IPC_MSG_PROGRESS_UPDATE, connection->sequence,
                         payload, sizeof(payload)) != FLASHUX_SUCCESS) {
        connection->disconnected = true;
    }
}
static void serve(flashux_context_t *ctx, int fd)
{
    unsigned char payload[FLASHUX_IPC_MAX_PAYLOAD];
    struct connection connection = {fd, 0, 0, false, false};
    struct timeval timeout = {5, 0};
    (void)setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    (void)setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    flashux_error_t result = flashux_helper_authorize(fd);
    if (result != FLASHUX_SUCCESS) {
        return;
    }
    while (!stopping) {
        flashux_ipc_header_t header;
        result = flashux_ipc_receive(fd, &header, payload, sizeof(payload));
        if (result != FLASHUX_SUCCESS || header.sequence_id <= connection.last_command) {
            break;
        }
        connection.sequence = header.sequence_id;
        connection.last_command = header.sequence_id;
        connection.cancelled = false;
        if (header.msg_type == FLASHUX_IPC_CMD_PING && header.payload_len == 0) {
            result = FLASHUX_SUCCESS;
        } else if (header.msg_type == FLASHUX_IPC_CMD_START_FLASH) {
            flashux_flash_options_t options;
            int source_fd = -1;
            result = flashux_ipc_decode_options(payload, header.payload_len, &options);
            if (result == FLASHUX_SUCCESS) {
                result = flashux_ipc_receive_fd(fd, &source_fd);
            }
            if (result == FLASHUX_SUCCESS) {
                result = flashux_helper_flash(ctx, &options, source_fd, send_progress, &connection,
                                              should_cancel, &connection);
            }
            if (source_fd >= 0) {
                (void)close(source_fd);
            }
        } else {
            result = FLASHUX_ERR_UNSUPPORTED;
        }
        unsigned char status[4];
        flashux_put_u32(status, (uint32_t)-(int)result);
        if (connection.disconnected ||
            flashux_ipc_send(fd, FLASHUX_IPC_MSG_OPERATION_RESULT, connection.sequence, status,
                             sizeof(status)) != FLASHUX_SUCCESS ||
            result == FLASHUX_ERR_IPC) {
            break;
        }
    }
}
#endif

int main(int argc, char **argv)
{
    bool once = argc == 2 && strcmp(argv[1], "--once") == 0;
    if (argc > 1 && !once) {
        (void)fprintf(stderr, "Usage: flashux-helper [--once]\n");
        return 2;
    }
#if FLASHUX_LINUX
    struct stat directory;
    struct sockaddr_un address = {0};
    flashux_context_t *ctx = NULL;
    int server = -1;
    int result = 1;
    if (geteuid() != 0 || (mkdir("/run/flashux", 0755) != 0 && errno != EEXIST) ||
        lstat("/run/flashux", &directory) != 0 || !S_ISDIR(directory.st_mode) ||
        directory.st_uid != 0 || (directory.st_mode & 0022) != 0) {
        flashux_log_error(NULL, "%s", "Helper requires a secure root-owned /run/flashux");
        return 1;
    }
    if (flashux_context_new(&ctx) != FLASHUX_SUCCESS) {
        return 1;
    }
    server = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (server < 0) {
        goto cleanup;
    }
    address.sun_family = AF_UNIX;
    (void)snprintf(address.sun_path, sizeof(address.sun_path), "%s", FLASHUX_IPC_SOCKET_PATH);
    /* Bind never removes an existing pathname, avoiding competing helper races. */
    if (bind(server, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        goto cleanup;
    }
    if (chmod(FLASHUX_IPC_SOCKET_PATH, 0666) != 0 || listen(server, 4) != 0) {
        (void)unlink(FLASHUX_IPC_SOCKET_PATH);
        goto cleanup;
    }
    struct sigaction action = {0};
    action.sa_handler = stop_helper;
    (void)sigemptyset(&action.sa_mask);
    (void)sigaction(SIGTERM, &action, NULL);
    (void)sigaction(SIGINT, &action, NULL);
    while (!stopping) {
        struct pollfd wait_fd = {server, POLLIN, 0};
        int ready = poll(&wait_fd, 1, once ? 60000 : 1000);
        if (ready < 0 && errno == EINTR) {
            continue;
        }
        if (ready < 0 || (once && ready == 0)) {
            break;
        }
        if (ready == 0) {
            continue;
        }
        int client = accept4(server, NULL, NULL, SOCK_CLOEXEC);
        if (client < 0) {
            continue;
        }
        serve(ctx, client);
        (void)close(client);
        if (once) {
            break;
        }
    }
    (void)unlink(FLASHUX_IPC_SOCKET_PATH);
    result = 0;
cleanup:
    if (server >= 0) {
        (void)close(server);
    }
    flashux_context_free(&ctx);
    return result;
#else
    (void)once;
    flashux_log_error(NULL, "%s", "Privileged block operations require Linux");
    return 1;
#endif
}
