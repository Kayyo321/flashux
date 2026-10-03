#ifndef FLASHUX_IPC_PROTOCOL_H
#define FLASHUX_IPC_PROTOCOL_H

#include <stdint.h>
#include "flashux/flasher.h"

#define FLASHUX_IPC_MAGIC 0x464C5358 /* "FLSX" */
#define FLASHUX_IPC_SOCKET_PATH "/run/flashux/helper.sock"

typedef enum {
    FLASHUX_IPC_CMD_PING             = 0x01,
    FLASHUX_IPC_CMD_SCAN_DEVICES     = 0x02,
    FLASHUX_IPC_CMD_UNMOUNT_DEVICE   = 0x03,
    FLASHUX_IPC_CMD_START_FLASH      = 0x04,
    FLASHUX_IPC_CMD_CANCEL_FLASH     = 0x05,
    FLASHUX_IPC_MSG_PROGRESS_UPDATE  = 0x10,
    FLASHUX_IPC_MSG_LOG_EVENT        = 0x11,
    FLASHUX_IPC_MSG_OPERATION_RESULT = 0x20
} flashux_ipc_msg_type_t;

/* In-memory model; encode the 16-byte wire header explicitly. */
typedef struct {
    uint32_t magic;         /* FLASHUX_IPC_MAGIC */
    uint16_t version;       /* Protocol version (1) */
    uint16_t msg_type;      /* flashux_ipc_msg_type_t */
    uint32_t payload_len;   /* Length of trailing payload in bytes */
    uint32_t sequence_id;   /* Monotonic sequence identifier */
} flashux_ipc_header_t;


#define FLASHUX_IPC_MAX_PAYLOAD 65536U
#define FLASHUX_IPC_HEADER_SIZE 16U
#define FLASHUX_IPC_OPTIONS_SIZE 880U
#define FLASHUX_IPC_PROGRESS_SIZE 168U

flashux_error_t flashux_ipc_encode_header(const flashux_ipc_header_t *header,
                                        unsigned char out[16]);
flashux_error_t flashux_ipc_decode_header(const unsigned char in[16],
                                        flashux_ipc_header_t *header);
flashux_error_t flashux_ipc_send(int fd, flashux_ipc_msg_type_t type, uint32_t sequence,
                               const void *payload, size_t size);
flashux_error_t flashux_ipc_receive(int fd, flashux_ipc_header_t *header,
                                  void *payload, size_t capacity);
flashux_error_t flashux_ipc_encode_options(const flashux_flash_options_t *options,
                                         unsigned char out[FLASHUX_IPC_OPTIONS_SIZE]);
flashux_error_t flashux_ipc_decode_options(const unsigned char *in, size_t size,
                                         flashux_flash_options_t *options);
void flashux_ipc_encode_progress(const flashux_progress_t *progress,
                                unsigned char out[FLASHUX_IPC_PROGRESS_SIZE]);
flashux_error_t flashux_ipc_decode_progress(const unsigned char *in, size_t size,
                                          flashux_progress_t *progress);
flashux_error_t flashux_ipc_send_fd(int socket_fd, int file_fd);
flashux_error_t flashux_ipc_receive_fd(int socket_fd, int *file_fd);
#endif /* FLASHUX_IPC_PROTOCOL_H */
