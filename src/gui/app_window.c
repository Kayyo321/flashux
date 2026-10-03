#include "app_window.h"
static gboolean close_window(GtkWindow *window, gpointer data)
{
    struct flashux_gui *gui = data;
    (void)window;
    gui->closed = true;
    flashux_context_free(&gui->ctx);
    return FALSE;
}
void flashux_gui_window_create(struct flashux_gui *gui)
{
    gui->window = GTK_WINDOW(adw_application_window_new(GTK_APPLICATION(gui->application)));
    gtk_window_set_title(gui->window, "Flashux");
    gtk_window_set_default_size(gui->window, 560, 720);
    g_object_set_data(G_OBJECT(gui->window), "flashux-state", gui);
    g_signal_connect(gui->window, "close-request", G_CALLBACK(close_window), gui);
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(gui->window),
        gtk_label_new("Flashux — native boot media creator"));
}
