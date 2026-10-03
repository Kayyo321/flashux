#ifndef FLASHUX_IPC_CLIENT_H
#define FLASHUX_IPC_CLIENT_H
#include "helper/ipc_protocol.h"
typedef struct flashux_ipc_client flashux_ipc_client_t;

/**
 * @brief Connect to the privileged helper daemon over UNIX domain socket.
 *
 * @param[in] ctx Flashux context.
 * @param[in] socket_path Path to UNIX domain socket.
 * @param[out] client_out Allocated client connection instance.
 * @return FLASHUX_SUCCESS on success, FLASHUX_ERR_IPC on failure.
 */
flashux_error_t flashux_ipc_client_connect(flashux_context_t *ctx, const char *socket_path,
                                           flashux_ipc_client_t **client_out);

/**
 * @brief Disconnect and free IPC client resources.
 *
 * @param[in,out] client Pointer to client instance. Set to NULL on release.
 */
void flashux_ipc_client_disconnect(flashux_ipc_client_t **client);

/**
 * @brief Send an IPC command packet to the helper daemon.
 *
 * @param[in] client Connected client instance.
 * @param[in] cmd Command message type opcode.
 * @param[in] payload Data buffer to send.
 * @param[in] payload_len Length of payload in bytes.
 * @return FLASHUX_SUCCESS on success, negative error code on socket error.
 */
flashux_error_t flashux_ipc_client_send_cmd(flashux_ipc_client_t *client,
                                            flashux_ipc_msg_type_t cmd, const void *payload,
                                            size_t payload_len);

/**
 * @brief Receive next message packet from helper daemon.
 *
 * @param[in] client Connected client instance.
 * @param[out] header_out Fixed header buffer.
 * @param[out] payload_buffer Buffer to receive payload.
 * @param[in] max_payload_len Capacity of payload_buffer.
 * @return FLASHUX_SUCCESS on success, negative error code on socket error.
 */
flashux_error_t flashux_ipc_client_recv_msg(flashux_ipc_client_t *client,
                                            flashux_ipc_header_t *header_out, void *payload_buffer,
                                            size_t max_payload_len);

int flashux_ipc_client_fd(const flashux_ipc_client_t *client);
#endif
