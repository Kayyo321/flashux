#ifndef FLASHUX_PROGRESS_H
#define FLASHUX_PROGRESS_H
#include "flashux/flasher.h"
flashux_error_t flashux_progress_update(flashux_progress_t *progress, uint64_t bytes,
                                        uint64_t total, uint64_t elapsed, uint64_t previous_bytes,
                                        uint64_t previous_elapsed);
uint64_t flashux_monotonic_nanos(void);
#endif
