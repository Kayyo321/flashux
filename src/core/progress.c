#include "progress.h"
#include <time.h>
#include <math.h>

uint64_t flashux_monotonic_nanos(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0 || now.tv_sec < 0) {
        return 0;
    }
    return (uint64_t)now.tv_sec * UINT64_C(1000000000) + (uint64_t)now.tv_nsec;
}
flashux_error_t flashux_progress_update(flashux_progress_t *progress, uint64_t bytes,
                                        uint64_t total, uint64_t elapsed, uint64_t previous_bytes,
                                        uint64_t previous_elapsed)
{
    if (progress == NULL || bytes > total || previous_bytes > bytes || previous_elapsed > elapsed) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    progress->bytes_processed = bytes;
    progress->total_bytes = total;
    progress->elapsed_nanos = elapsed;
    progress->percent_complete = total == 0 ? 0 : 100.0 * (double)bytes / (double)total;
    progress->average_speed_bps = elapsed == 0 ? 0 : (double)bytes * 1e9 / (double)elapsed;
    if (elapsed > previous_elapsed) {
        double instant =
            (double)(bytes - previous_bytes) * 1e9 / (double)(elapsed - previous_elapsed);
        double prior = progress->current_speed_bps;
        progress->current_speed_bps =
            prior > 0 && isfinite(prior) ? 0.25 * instant + 0.75 * prior : instant;
    }
    double seconds =
        progress->current_speed_bps > 0 ? (double)(total - bytes) / progress->current_speed_bps : 0;
    if (!isfinite(seconds) || seconds >= (double)UINT32_MAX) {
        progress->eta_seconds = UINT32_MAX;
    } else {
        progress->eta_seconds = (uint32_t)seconds;
        if ((double)progress->eta_seconds < seconds) {
            ++progress->eta_seconds;
        }
    }
    return FLASHUX_SUCCESS;
}
