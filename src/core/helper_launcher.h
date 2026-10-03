#ifndef FLASHUX_HELPER_LAUNCHER_H
#define FLASHUX_HELPER_LAUNCHER_H
#include "internal.h"
#include "ipc_client.h"
flashux_error_t flashux_helper_connect_or_launch(flashux_context_t *ctx,
                                                flashux_ipc_client_t **client);
#endif
