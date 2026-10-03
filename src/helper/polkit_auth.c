#include "polkit_auth.h"
#include "flashux_config.h"
#if FLASHUX_LINUX
#include <polkit/polkit.h>
#include <sys/socket.h>
#include <unistd.h>
#endif
flashux_error_t flashux_helper_authorize(int client_fd)
{
#if FLASHUX_LINUX
    struct ucred peer;
    socklen_t size = sizeof(peer);
    GError *error = NULL;
    if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &peer, &size) != 0 ||
        size != sizeof(peer) || peer.pid <= 0) { return FLASHUX_ERR_PERMISSION_DENIED; }
    PolkitAuthority *authority = polkit_authority_get_sync(NULL, &error);
    if (authority == NULL) {
        g_clear_error(&error);
        return FLASHUX_ERR_PERMISSION_DENIED;
    }
    PolkitSubject *subject = polkit_unix_process_new_for_owner(peer.pid, 0, (gint)peer.uid);
    if (subject == NULL) {
        g_object_unref(authority);
        return FLASHUX_ERR_PERMISSION_DENIED;
    }
    PolkitAuthorizationResult *authorization = polkit_authority_check_authorization_sync(
        authority, subject, "org.flashux.write-device", NULL,
        POLKIT_CHECK_AUTHORIZATION_FLAGS_ALLOW_USER_INTERACTION, NULL, &error);
    bool allowed = authorization != NULL &&
        polkit_authorization_result_get_is_authorized(authorization);
    if (authorization != NULL) { g_object_unref(authorization); }
    g_object_unref(subject);
    g_object_unref(authority);
    g_clear_error(&error);
    return allowed ? FLASHUX_SUCCESS : FLASHUX_ERR_PERMISSION_DENIED;
#else
    (void)client_fd;
    return FLASHUX_ERR_UNSUPPORTED;
#endif
}
