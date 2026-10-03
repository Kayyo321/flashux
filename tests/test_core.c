#include "test.h"
#include "flashux/core.h"
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
struct log_state {
    flashux_context_t *ctx;
    atomic_uint count;
};
static void capture(flashux_log_level_t level, const char *message, void *data)
{
    struct log_state *state = data;
    CHECK(level == FLASHUX_LOG_INFO);
    CHECK(message != NULL);
    (void)atomic_fetch_add(&state->count, 1);
    flashux_context_set_logger(state->ctx, FLASHUX_LOG_INFO, capture, state);
}
static void *emit(void *data)
{
    struct log_state *state = data;
    for (unsigned i = 0; i < 1000; ++i) {
        flashux_log_info(state->ctx, "message %u", i);
    }
    return NULL;
}
int main(void)
{
    flashux_context_t *ctx = NULL;
    struct log_state state;
    pthread_t a;
    pthread_t b;
    CHECK(flashux_context_new(NULL) == FLASHUX_ERR_INVALID_ARG);
    CHECK(flashux_context_new(&ctx) == FLASHUX_SUCCESS);
    state.ctx = ctx;
    atomic_init(&state.count, 0);
    flashux_context_set_logger(ctx, FLASHUX_LOG_INFO, capture, &state);
    flashux_log_debug(ctx, "%s", "filtered");
    CHECK(pthread_create(&a, NULL, emit, &state) == 0);
    CHECK(pthread_create(&b, NULL, emit, &state) == 0);
    CHECK(pthread_join(a, NULL) == 0);
    CHECK(pthread_join(b, NULL) == 0);
    CHECK(atomic_load(&state.count) == 2000);
    CHECK(strcmp(flashux_strerror(FLASHUX_ERR_CANCELLED), "Operation cancelled") == 0);
    CHECK(strcmp(flashux_strerror((flashux_error_t)-200), "Unknown error") == 0);
    flashux_context_free(&ctx);
    CHECK(ctx == NULL);
    flashux_context_free(&ctx);
    flashux_context_free(NULL);
    return 0;
}
