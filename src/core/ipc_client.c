#include "ipc_client.h"
#include "flashux_config.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

struct flashux_ipc_client {
    int fd;
    uint32_t sequence;
};
void flashux_ipc_client_disconnect(flashux_ipc_client_t **client)
{
    if (client != NULL && *client != NULL) {
        (void)close((*client)->fd);
        free(*client);
        *client = NULL;
    }
}
int flashux_ipc_client_fd(const flashux_ipc_client_t *client)
{
    return client == NULL ? -1 : client->fd;
}
flashux_error_t flashux_ipc_client_connect(flashux_context_t *ctx, const char *socket_path,
                                          flashux_ipc_client_t **client_out)
{
    struct sockaddr_un address = {0};
    struct timeval timeout = {5, 0};
    struct stat st;
    if (ctx == NULL || socket_path == NULL || client_out == NULL) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    *client_out = NULL;
    if (strlen(socket_path) >= sizeof(address.sun_path) || lstat(socket_path, &st) != 0 ||
        !S_ISSOCK(st.st_mode)) {
        return FLASHUX_ERR_IPC;
    }
#if FLASHUX_LINUX
    if (st.st_uid != 0 || (st.st_mode & 0022) != 0) {
        return FLASHUX_ERR_PERMISSION_DENIED;
    }
#endif
    flashux_ipc_client_t *client = calloc(1, sizeof(*client));
    if (client == NULL) { return FLASHUX_ERR_NO_MEMORY; }
    client->fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client->fd < 0) {
        free(client);
        client = NULL;
        return FLASHUX_ERR_IPC;
    }
    (void)fcntl(client->fd, F_SETFD, FD_CLOEXEC);
    (void)setsockopt(client->fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    (void)setsockopt(client->fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    address.sun_family = AF_UNIX;
    memcpy(address.sun_path, socket_path, strlen(socket_path) + 1);
    if (connect(client->fd, (const struct sockaddr *)&address, sizeof(address)) != 0) {
        flashux_ipc_client_disconnect(&client);
        return FLASHUX_ERR_IPC;
    }
#if FLASHUX_LINUX
    struct ucred peer;
    socklen_t size = sizeof(peer);
    if (getsockopt(client->fd, SOL_SOCKET, SO_PEERCRED, &peer, &size) != 0 || peer.uid != 0) {
        flashux_ipc_client_disconnect(&client);
        return FLASHUX_ERR_PERMISSION_DENIED;
    }
#endif
    *client_out = client;
    return FLASHUX_SUCCESS;
}
flashux_error_t flashux_ipc_client_send_cmd(flashux_ipc_client_t *client,
                                          flashux_ipc_msg_type_t cmd,
                                          const void *payload, size_t payload_len)
{
    if (client == NULL || client->fd < 0 || client->sequence == UINT32_MAX ||
        cmd < FLASHUX_IPC_CMD_PING || cmd > FLASHUX_IPC_CMD_CANCEL_FLASH) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    return flashux_ipc_send(client->fd, cmd, ++client->sequence, payload, payload_len);
}
flashux_error_t flashux_ipc_client_recv_msg(flashux_ipc_client_t *client,
                                          flashux_ipc_header_t *header_out,
                                          void *payload_buffer, size_t max_payload_len)
{
    if (client == NULL || header_out == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    flashux_error_t result = flashux_ipc_receive(client->fd, header_out,
                                               payload_buffer, max_payload_len);
    if (result == FLASHUX_SUCCESS && (header_out->sequence_id == 0 ||
        header_out->sequence_id > client->sequence ||
        header_out->msg_type < FLASHUX_IPC_MSG_PROGRESS_UPDATE)) {
        result = FLASHUX_ERR_IPC;
    }
    if (result != FLASHUX_SUCCESS) {
        (void)close(client->fd);
        client->fd = -1;
    }
    return result;
}
