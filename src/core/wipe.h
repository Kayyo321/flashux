#ifndef FLASHUX_WIPE_H
#define FLASHUX_WIPE_H
#include "flashux/core.h"
/* Caller owns the validated, exclusive descriptor. Never resolves or opens a path. */
flashux_error_t flashux_zero_fd(int fd, bool (*cancel)(void *), void *cancel_data);
#endif
