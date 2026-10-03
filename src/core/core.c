#include "internal.h"
#include <stdlib.h>

flashux_error_t flashux_context_new(flashux_context_t **ctx_out)
{
    flashux_context_t *ctx;
    if (ctx_out == NULL) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    *ctx_out = NULL;
    ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return FLASHUX_ERR_NO_MEMORY;
    }
    if (pthread_mutex_init(&ctx->logger_mutex, NULL) != 0) {
        free(ctx);
        ctx = NULL;
        return FLASHUX_ERR_UNKNOWN;
    }
    if (pthread_mutex_init(&ctx->job_mutex, NULL) != 0) {
        (void)pthread_mutex_destroy(&ctx->logger_mutex);
        free(ctx);
        ctx = NULL;
        return FLASHUX_ERR_UNKNOWN;
    }
    atomic_init(&ctx->cancel, false);
    ctx->min_level = FLASHUX_LOG_INFO;
    *ctx_out = ctx;
    return FLASHUX_SUCCESS;
}

void flashux_context_free(flashux_context_t **ctx)
{
    if (ctx == NULL || *ctx == NULL) {
        return;
    }
    atomic_store(&(*ctx)->cancel, true);
    if ((*ctx)->joinable) {
        (void)pthread_join((*ctx)->worker, NULL);
    }
    (void)pthread_mutex_destroy(&(*ctx)->job_mutex);
    (void)pthread_mutex_destroy(&(*ctx)->logger_mutex);
    free(*ctx);
    *ctx = NULL;
}
