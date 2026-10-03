#include "helper_launcher.h"
#include "flashux_config.h"
#include "progress.h"
#include <sys/stat.h>
#if FLASHUX_LINUX
#include <spawn.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

static void *reap_helper(void *data)
{
    pid_t child = *(pid_t *)data;
    free(data);
    data = NULL;
    int status;
    while (waitpid(child, &status, 0) < 0 && errno == EINTR) { }
    return NULL;
}
#endif
flashux_error_t flashux_helper_connect_or_launch(flashux_context_t *ctx,
                                                flashux_ipc_client_t **client)
{
#if FLASHUX_LINUX
    flashux_error_t result = flashux_ipc_client_connect(ctx, FLASHUX_IPC_SOCKET_PATH, client);
    if (result == FLASHUX_SUCCESS) { return result; }
    struct stat existing;
    if (lstat(FLASHUX_IPC_SOCKET_PATH, &existing) == 0) { return result; }
    char executable[] = "/usr/bin/pkexec";
    char helper_path[] = FLASHUX_HELPER_PATH;
    char once[] = "--once";
    char *arguments[] = {executable, helper_path, once, NULL};
    /* pkexec uses the invoking desktop session for its authorization agent. */
    extern char **environ;
    pid_t *child = malloc(sizeof(*child));
    if (child == NULL) { return FLASHUX_ERR_NO_MEMORY; }
    int error = posix_spawn(child, executable, NULL, NULL, arguments, environ);
    if (error != 0) {
        free(child);
        child = NULL;
        return error == ENOENT ? FLASHUX_ERR_NOT_FOUND : FLASHUX_ERR_PERMISSION_DENIED;
    }
    pid_t pid = *child;
    pthread_t reaper;
    pthread_attr_t attributes;
    int initialized = pthread_attr_init(&attributes);
    if (initialized == 0) {
        error = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
        if (error == 0) { error = pthread_create(&reaper, &attributes, reap_helper, child); }
        (void)pthread_attr_destroy(&attributes);
    } else { error = initialized; }
    if (error != 0) {
        (void)kill(pid, SIGTERM);
        (void)reap_helper(child);
        return FLASHUX_ERR_UNKNOWN;
    }
    uint64_t began = flashux_monotonic_nanos();
    while (flashux_monotonic_nanos() - began < UINT64_C(60000000000)) {
        if (atomic_load(&ctx->cancel)) { return FLASHUX_ERR_CANCELLED; }
        result = flashux_ipc_client_connect(ctx, FLASHUX_IPC_SOCKET_PATH, client);
        if (result == FLASHUX_SUCCESS) { return result; }
        if (kill(pid, 0) != 0 && errno == ESRCH) { return FLASHUX_ERR_PERMISSION_DENIED; }
        struct timespec pause = {0, 100000000};
        (void)nanosleep(&pause, NULL);
    }
    return FLASHUX_ERR_PERMISSION_DENIED;
#else
    (void)ctx;
    (void)client;
    return FLASHUX_ERR_UNSUPPORTED;
#endif
}
