#include "cli_runtime.h"
#include "flashux/device.h"
#include <ncurses.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
struct dashboard {
    pthread_mutex_t mutex;
    flashux_progress_t progress;
};
static void tui_progress(const flashux_progress_t *progress, void *data)
{
    struct dashboard *dashboard = data;
    (void)pthread_mutex_lock(&dashboard->mutex);
    dashboard->progress = *progress;
    (void)pthread_mutex_unlock(&dashboard->mutex);
}
static void tui_log(flashux_log_level_t level, const char *message, void *data)
{
    (void)level;
    struct dashboard *dashboard = data;
    (void)pthread_mutex_lock(&dashboard->mutex);
    (void)snprintf(dashboard->progress.stage_description,
                   sizeof(dashboard->progress.stage_description), "%s", message);
    (void)pthread_mutex_unlock(&dashboard->mutex);
}
static int select_device(flashux_context_t *ctx, char path[64], bool fixed)
{
    flashux_device_t *list = NULL;
    flashux_error_t result = flashux_device_list_scan(ctx, fixed, &list);
    if (result != FLASHUX_SUCCESS || list == NULL) {
        flashux_device_list_free(&list);
        return -1;
    }
    size_t selected = 0;
    int status = -1;
    while (!flashux_cli_interrupted) {
        size_t count = 0;
        flashux_device_t *choice = NULL;
        (void)erase();
        (void)mvprintw(0, 0, "Flashux - Select target (arrows, Enter, q to cancel)");
        for (flashux_device_t *device = list; device != NULL; device = device->next) {
            bool safe = flashux_device_is_safe_target(ctx, device, NULL, 0);
            if (count == selected) { choice = device; (void)attron(A_REVERSE); }
            if (count < (size_t)(LINES > 4 ? LINES - 4 : 0)) {
                (void)mvprintw((int)count + 2, 0, "%s %s %.1f GiB %s", device->node_path,
                    device->model, (double)device->size_bytes / 1073741824.0,
                    safe ? "" : "[PROTECTED]");
            }
            if (count == selected) { (void)attroff(A_REVERSE); }
            ++count;
        }
        (void)refresh();
        int key = getch();
        if (key == 'q' || key == 27) { break; }
        if (key == KEY_DOWN && selected + 1 < count) { ++selected; }
        if (key == KEY_UP && selected > 0) { --selected; }
        if (key == '\n' && choice != NULL && flashux_device_is_safe_target(ctx, choice, NULL, 0)) {
            (void)snprintf(path, 64, "%s", choice->node_path);
            status = 0;
            break;
        }
    }
    flashux_device_list_free(&list);
    return status;
}
int flashux_cli_run_interactive(flashux_context_t *ctx, const flashux_cli_options_t *opts)
{
    flashux_cli_options_t selected = *opts;
    struct dashboard dashboard = {0};
    int exit_code = 1;
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO) ||
        pthread_mutex_init(&dashboard.mutex, NULL) != 0) {
        flashux_log_error(ctx, "%s", "Interactive mode requires a terminal");
        return 1;
    }
    if (initscr() == NULL) { (void)pthread_mutex_destroy(&dashboard.mutex); return 1; }
    (void)cbreak();
    (void)noecho();
    (void)keypad(stdscr, true);
    (void)curs_set(0);
    flashux_context_set_logger(ctx, FLASHUX_LOG_WARN, tui_log, &dashboard);
    if (selected.device_path[0] == '\0' &&
        select_device(ctx, selected.device_path, selected.include_fixed) != 0) { goto cleanup; }
    if (selected.image_path[0] == '\0') {
        (void)erase();
        (void)mvprintw(0, 0, "Image path: ");
        (void)echo();
        (void)curs_set(1);
        int status = getnstr(selected.image_path, (int)sizeof(selected.image_path) - 1);
        (void)noecho();
        (void)curs_set(0);
        if (status == ERR) { goto cleanup; }
    }
    if (!selected.dry_run) {
        char confirmation[16] = {0};
        (void)erase();
        (void)mvprintw(0, 0, "All data on %s will be destroyed.", selected.device_path);
        (void)mvprintw(2, 0, "Type ERASE to continue: ");
        (void)echo();
        int status = getnstr(confirmation, (int)sizeof(confirmation) - 1);
        (void)noecho();
        if (status == ERR || strcmp(confirmation, "ERASE") != 0) { goto cleanup; }
    }
    flashux_flash_options_t options;
    flashux_cli_to_flash(&selected, &options);
    flashux_error_t result = flashux_flash_start_async(ctx, &options, tui_progress, &dashboard);
    if (result != FLASHUX_SUCCESS) { goto show_result; }
    (void)timeout(100);
    while (flashux_flash_is_running(ctx)) {
        flashux_progress_t progress;
        (void)pthread_mutex_lock(&dashboard.mutex);
        progress = dashboard.progress;
        (void)pthread_mutex_unlock(&dashboard.mutex);
        (void)erase();
        (void)mvprintw(0, 0, "Flashux - %s", selected.device_path);
        (void)mvprintw(2, 0, "%s", progress.stage_description);
        (void)mvprintw(4, 0, "%.1f%%  %.1f MiB/s  ETA %u seconds", progress.percent_complete,
                      progress.current_speed_bps / 1048576.0, progress.eta_seconds);
        (void)mvprintw(6, 0, "Press q or Escape to cancel");
        (void)refresh();
        int key = getch();
        if (key == 'q' || key == 27 || flashux_cli_interrupted) {
            (void)flashux_flash_cancel(ctx);
        }
    }
    result = flashux_flash_wait(ctx);
show_result:
    (void)erase();
    (void)mvprintw(0, 0, "%s", flashux_strerror(result));
    (void)mvprintw(2, 0, "Press any key to exit");
    (void)timeout(-1);
    if (!flashux_cli_interrupted) { (void)getch(); }
    exit_code = result == FLASHUX_SUCCESS ? 0 : result == FLASHUX_ERR_CANCELLED ? 130 : 1;
cleanup:
    flashux_context_set_logger(ctx, FLASHUX_LOG_INFO, NULL, NULL);
    (void)endwin();
    (void)pthread_mutex_destroy(&dashboard.mutex);
    return exit_code;
}
