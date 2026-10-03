#include "core/ipc_client.h"
#include "helper/ipc_protocol.h"
#include "common/wire.h"
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv)
{
    flashux_context_t *ctx = NULL;
    flashux_ipc_client_t *client = NULL;
    flashux_ipc_header_t header;
    unsigned char status[4];
    if (argc != 2 || (strcmp(argv[1], "allow") != 0 && strcmp(argv[1], "deny") != 0) ||
        flashux_context_new(&ctx) != FLASHUX_SUCCESS) {
        return 2;
    }
    flashux_error_t result = flashux_ipc_client_connect(ctx, FLASHUX_IPC_SOCKET_PATH, &client);
    if (result == FLASHUX_SUCCESS) {
        result = flashux_ipc_client_send_cmd(client, FLASHUX_IPC_CMD_PING, NULL, 0);
    }
    if (result == FLASHUX_SUCCESS) {
        result = flashux_ipc_client_recv_msg(client, &header, status, sizeof(status));
    }
    bool passed = strcmp(argv[1], "allow") == 0
                      ? result == FLASHUX_SUCCESS &&
                            header.msg_type == FLASHUX_IPC_MSG_OPERATION_RESULT &&
                            header.payload_len == 4 && flashux_get_u32(status) == 0
                      : result == FLASHUX_ERR_IPC;
    flashux_ipc_client_disconnect(&client);
    flashux_context_free(&ctx);
    (void)printf("Helper %s probe: %s (transport result %d)\n", argv[1], passed ? "PASS" : "FAIL",
                 (int)result);
    return passed ? 0 : 1;
}
