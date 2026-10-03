#ifndef FLASHUX_INTERNAL_H
#define FLASHUX_INTERNAL_H
#include "flashux/flasher.h"
#include <pthread.h>
#include <stdatomic.h>
struct flashux_context {
    pthread_mutex_t logger_mutex;
    pthread_mutex_t job_mutex;
    flashux_log_level_t min_level;
    flashux_log_fn logger;
    void *logger_data;
    pthread_t worker;
    bool joinable;
    bool running;
    bool joining;
    atomic_bool cancel;
    flashux_error_t result;
    flashux_flash_options_t options;
    flashux_progress_cb progress_cb;
    void *progress_data;
};
#endif
