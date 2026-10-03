#include "app_window.h"
#include <stdlib.h>
static void activate(GApplication *application, gpointer data)
{
    struct flashux_gui *gui = data;
    (void)application;
    if (gui->window == NULL) { flashux_gui_window_create(gui); }
    gtk_window_present(gui->window);
}
int flashux_gui_app_run(int argc, char **argv)
{
    struct flashux_gui gui = {0};
    if (pthread_mutex_init(&gui.mutex, NULL) != 0) { return 1; }
    if (flashux_context_new(&gui.ctx) != FLASHUX_SUCCESS) {
        (void)pthread_mutex_destroy(&gui.mutex);
        return 1;
    }
    gui.application = adw_application_new("org.flashux.Flashux", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(gui.application, "activate", G_CALLBACK(activate), &gui);
    int result = g_application_run(G_APPLICATION(gui.application), argc, argv);
    gui.closed = true;
    flashux_context_free(&gui.ctx);
    flashux_gui_progress_stop(&gui);
    flashux_gui_device_monitor_stop(&gui);
    if (gui.window != NULL) { g_object_set_data(G_OBJECT(gui.window), "flashux-state", NULL); }
    g_clear_object(&gui.window);
    g_clear_object(&gui.application);
    (void)pthread_mutex_destroy(&gui.mutex);
    return result;
}
int main(int argc, char **argv)
{
    return flashux_gui_app_run(argc, argv);
}
