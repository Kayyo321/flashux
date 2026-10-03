#ifndef FLASHUX_PRIVILEGED_OPS_H
#define FLASHUX_PRIVILEGED_OPS_H
#include "flashux/flasher.h"
typedef bool (*flashux_cancel_fn)(void *data);
flashux_error_t flashux_raw_copy(int source_fd, int target_fd, uint32_t sector,
                                flashux_progress_cb progress_cb, void *progress_data,
                                flashux_cancel_fn cancel, void *cancel_data);
flashux_error_t flashux_helper_flash(flashux_context_t *ctx,
                                    const flashux_flash_options_t *options, int source_fd,
                                    flashux_progress_cb progress_cb, void *progress_data,
                                    flashux_cancel_fn cancel, void *cancel_data);
#endif
