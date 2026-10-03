#ifndef FLASHUX_CORE_H
#define FLASHUX_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(__GNUC__) || defined(__clang__)
#define FLASHUX_PRINTF(a, b) __attribute__((format(printf, a, b)))
#else
#define FLASHUX_PRINTF(a, b)
#endif

/**
 * @brief Standard Flashux error return codes.
 */
typedef enum {
    FLASHUX_SUCCESS                 = 0,
    FLASHUX_ERR_INVALID_ARG         = -1,
    FLASHUX_ERR_NO_MEMORY           = -2,
    FLASHUX_ERR_DEVICE_BUSY         = -3,
    FLASHUX_ERR_SYSTEM_DISK_PROTECT = -4,
    FLASHUX_ERR_PERMISSION_DENIED   = -5,
    FLASHUX_ERR_IO                  = -6,
    FLASHUX_ERR_IPC                 = -7,
    FLASHUX_ERR_NOT_FOUND           = -8,
    FLASHUX_ERR_UNSUPPORTED         = -9,
    FLASHUX_ERR_CANCELLED           = -10,
    FLASHUX_ERR_CORRUPT_IMAGE       = -11,
    FLASHUX_ERR_UNKNOWN             = -99
} flashux_error_t;

/**
 * @brief Log severity levels.
 */
typedef enum {
    FLASHUX_LOG_DEBUG = 0,
    FLASHUX_LOG_INFO  = 1,
    FLASHUX_LOG_WARN  = 2,
    FLASHUX_LOG_ERROR = 3
} flashux_log_level_t;

/**
 * @brief Custom log callback signature.
 */
typedef void (*flashux_log_fn)(
    flashux_log_level_t level,
    const char *message,
    void *user_data
);

/**
 * @brief Forward declaration of core context.
 */
typedef struct flashux_context flashux_context_t;

/**
 * @brief Allocate and initialize a new Flashux context.
 *
 * @param[out] ctx_out Pointer to receive the allocated context pointer.
 * @return FLASHUX_SUCCESS on success, negative flashux_error_t on failure.
 *
 * Ownership: Caller owns the context and must release it with flashux_context_free.
 */
flashux_error_t flashux_context_new(flashux_context_t **ctx_out);

/**
 * @brief Free a Flashux context and release all associated resources.
 *
 * @param[in,out] ctx Context to free; safely handles NULL. Pointer is cleared to NULL.
 */
void flashux_context_free(flashux_context_t **ctx);

/**
 * @brief Configure logging callback and minimum reporting level.
 *
 * @param[in] ctx Flashux context.
 * @param[in] min_level Minimum severity to report.
 * @param[in] callback Callback function pointer.
 * @param[in] user_data Opaque pointer passed to callback.
 */
void flashux_context_set_logger(
    flashux_context_t *ctx,
    flashux_log_level_t min_level,
    flashux_log_fn callback,
    void *user_data
);

/**
 * @brief Convert a Flashux error code into a human-readable English string.
 *
 * @param[in] err Error code.
 * @return Static null-terminated string description.
 */
const char *flashux_strerror(flashux_error_t err);

/**
 * @brief Structured logging helper for Flashux subsystems.
 *
 * @param[in] ctx Flashux context.
 * @param[in] level Severity level.
 * @param[in] format Printf-style format string.
 */
void flashux_log(
    flashux_context_t *ctx,
    flashux_log_level_t level,
    const char *format,
    ...
) FLASHUX_PRINTF(3, 4);

/**
 * @brief Convenience logging wrappers conforming to AGENTS.md §2.3.
 */
/**
 * @brief Emit a error message through the context logger.
 * @param[in] ctx Context, or NULL for stderr.
 * @param[in] format Printf-style format string, followed by its arguments.
 */
void flashux_log_error(flashux_context_t *ctx, const char *format, ...)
    FLASHUX_PRINTF(2, 3);
/**
 * @brief Emit a warn message through the context logger.
 * @param[in] ctx Context, or NULL for stderr.
 * @param[in] format Printf-style format string, followed by its arguments.
 */
void flashux_log_warn(flashux_context_t *ctx, const char *format, ...)
    FLASHUX_PRINTF(2, 3);
/**
 * @brief Emit a info message through the context logger.
 * @param[in] ctx Context, or NULL for stderr.
 * @param[in] format Printf-style format string, followed by its arguments.
 */
void flashux_log_info(flashux_context_t *ctx, const char *format, ...)
    FLASHUX_PRINTF(2, 3);
/**
 * @brief Emit a debug message through the context logger.
 * @param[in] ctx Context, or NULL for stderr.
 * @param[in] format Printf-style format string, followed by its arguments.
 */
void flashux_log_debug(flashux_context_t *ctx, const char *format, ...)
    FLASHUX_PRINTF(2, 3);

#endif /* FLASHUX_CORE_H */
