#include "internal.h"
#include <stdarg.h>
#include <stdio.h>

void flashux_context_set_logger(flashux_context_t *ctx, flashux_log_level_t min_level,
                                flashux_log_fn callback, void *user_data)
{
    if (ctx == NULL || min_level < FLASHUX_LOG_DEBUG || min_level > FLASHUX_LOG_ERROR) {
        return;
    }
    (void)pthread_mutex_lock(&ctx->logger_mutex);
    ctx->min_level = min_level;
    ctx->logger = callback;
    ctx->logger_data = user_data;
    (void)pthread_mutex_unlock(&ctx->logger_mutex);
}

static void log_message(flashux_context_t *ctx, flashux_log_level_t level, const char *format,
                        va_list args) FLASHUX_PRINTF(3, 0);
static void log_message(flashux_context_t *ctx, flashux_log_level_t level, const char *format,
                        va_list args)
{
    char message[2048];
    flashux_log_fn callback = NULL;
    void *data = NULL;
    if (format == NULL || level < FLASHUX_LOG_DEBUG || level > FLASHUX_LOG_ERROR) {
        return;
    }
    if (ctx != NULL) {
        (void)pthread_mutex_lock(&ctx->logger_mutex);
        if (level < ctx->min_level) {
            (void)pthread_mutex_unlock(&ctx->logger_mutex);
            return;
        }
        callback = ctx->logger;
        data = ctx->logger_data;
        (void)pthread_mutex_unlock(&ctx->logger_mutex);
    }
    if (vsnprintf(message, sizeof(message), format, args) < 0) {
        return;
    }
    if (callback != NULL) {
        callback(level, message, data);
    } else {
        (void)fprintf(stderr, "flashux[%d]: %s\n", (int)level, message);
    }
}

void flashux_log(flashux_context_t *ctx, flashux_log_level_t level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_message(ctx, level, format, args);
    va_end(args);
}

#define DEFINE_LOGGER(name, level)                                                                 \
    void name(flashux_context_t *ctx, const char *format, ...)                                     \
    {                                                                                              \
        va_list args;                                                                              \
        va_start(args, format);                                                                    \
        log_message(ctx, level, format, args);                                                     \
        va_end(args);                                                                              \
    }
DEFINE_LOGGER(flashux_log_error, FLASHUX_LOG_ERROR)
DEFINE_LOGGER(flashux_log_warn, FLASHUX_LOG_WARN)
DEFINE_LOGGER(flashux_log_info, FLASHUX_LOG_INFO)
DEFINE_LOGGER(flashux_log_debug, FLASHUX_LOG_DEBUG)

const char *flashux_strerror(flashux_error_t err)
{
    switch (err) {
    case FLASHUX_SUCCESS:
        return "Success";
    case FLASHUX_ERR_INVALID_ARG:
        return "Invalid argument";
    case FLASHUX_ERR_NO_MEMORY:
        return "Out of memory";
    case FLASHUX_ERR_DEVICE_BUSY:
        return "Device busy";
    case FLASHUX_ERR_SYSTEM_DISK_PROTECT:
        return "System drive protected";
    case FLASHUX_ERR_PERMISSION_DENIED:
        return "Permission denied";
    case FLASHUX_ERR_IO:
        return "Input/output error";
    case FLASHUX_ERR_IPC:
        return "IPC protocol error";
    case FLASHUX_ERR_NOT_FOUND:
        return "Not found";
    case FLASHUX_ERR_UNSUPPORTED:
        return "Unsupported operation on this platform";
    case FLASHUX_ERR_CANCELLED:
        return "Operation cancelled";
    case FLASHUX_ERR_CORRUPT_IMAGE:
        return "Corrupt image";
    case FLASHUX_ERR_UNKNOWN:
    default:
        return "Unknown error";
    }
}
