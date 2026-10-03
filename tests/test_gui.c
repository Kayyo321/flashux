#include "test.h"
#include "gui/app_window.h"
#include <string.h>
#include <unistd.h>
static void pump(unsigned milliseconds)
{
    gint64 until = g_get_monotonic_time() + (gint64)milliseconds * 1000;
    while (g_get_monotonic_time() < until) {
        while (g_main_context_iteration(NULL, false)) { }
        g_usleep(1000);
    }
}
static void screenshot(GtkWidget *widget)
{
    const char *path = g_getenv("FLASHUX_GUI_SCREENSHOT");
    if (path == NULL) { return; }
    GdkPaintable *paintable = gtk_widget_paintable_new(widget);
    GtkSnapshot *snapshot = gtk_snapshot_new();
    gdk_paintable_snapshot(paintable, GDK_SNAPSHOT(snapshot),
        (double)gtk_widget_get_width(widget), (double)gtk_widget_get_height(widget));
    GskRenderNode *node = gtk_snapshot_free_to_node(snapshot);
    CHECK(node != NULL);
    GskRenderer *renderer = gtk_native_get_renderer(GTK_NATIVE(widget));
    GdkTexture *texture = gsk_renderer_render_texture(renderer, node, NULL);
    CHECK(texture != NULL && gdk_texture_save_to_png(texture, path));
    g_object_unref(texture);
    gsk_render_node_unref(node);
    g_object_unref(paintable);
}
int main(void)
{
    gtk_init();
    adw_init();
    struct flashux_gui gui = {0};
    CHECK(pthread_mutex_init(&gui.mutex, NULL) == 0);
    CHECK(flashux_context_new(&gui.ctx) == FLASHUX_SUCCESS);
    gui.application = adw_application_new("org.flashux.Tests", G_APPLICATION_NON_UNIQUE);
    CHECK(g_application_register(G_APPLICATION(gui.application), NULL, NULL));
    flashux_gui_window_create(&gui);
    gtk_window_present(gui.window);
    pump(250);
    CHECK(!gtk_widget_get_sensitive(GTK_WIDGET(gui.start)));
    CHECK(!gtk_widget_get_sensitive(GTK_WIDGET(gui.scheme)));
    gtk_drop_down_set_selected(gui.mode, FLASHUX_MODE_ISO_EXTRACT);
    CHECK(gtk_widget_get_sensitive(GTK_WIDGET(gui.scheme)));
    gtk_drop_down_set_selected(gui.mode, FLASHUX_MODE_DD_RAW);
    flashux_gui_device_monitor_stop(&gui);
    char image_path[] = "/tmp/flashux-gui-image-XXXXXX";
    char target_path[] = "/tmp/flashux-gui-target-XXXXXX";
    int image = mkstemp(image_path);
    int target = mkstemp(target_path);
    CHECK(image >= 0 && target >= 0);
    CHECK(ftruncate(image, 1024 * 1024) == 0 && ftruncate(target, 2 * 1024 * 1024) == 0);
    gui.devices = calloc(1, sizeof(*gui.devices));
    CHECK(gui.devices != NULL);
    gui.devices->size_bytes = 2 * 1024 * 1024;
    gui.devices->logical_sector_sz = 512;
    (void)snprintf(gui.devices->node_path, sizeof(gui.devices->node_path), "%s", target_path);
    const char *labels[] = {"Sparse test target — 2 MiB", NULL};
    GtkStringList *model = gtk_string_list_new(labels);
    gtk_drop_down_set_model(gui.device_choice, G_LIST_MODEL(model));
    g_object_unref(model);
    gtk_check_button_set_active(gui.dry_run, true);
    gtk_editable_set_text(GTK_EDITABLE(gui.image), image_path);
    flashux_gui_update_start(&gui);
    CHECK(gtk_widget_get_sensitive(GTK_WIDGET(gui.start)));
    pump(100);
    screenshot(GTK_WIDGET(gui.window));
    flashux_gui_start(NULL, &gui);
    CHECK(gui.busy);
    for (unsigned i = 0; i < 100 && gui.busy; ++i) { pump(10); }
    CHECK(!gui.busy && gtk_progress_bar_get_fraction(gui.progress) == 1.0);
    CHECK(strcmp(gtk_label_get_text(gui.status), "Success") == 0);
    CHECK(!gtk_widget_get_sensitive(GTK_WIDGET(gui.cancel)));
    gui.devices->is_system_drive = true;
    flashux_gui_update_start(&gui);
    CHECK(!gtk_widget_get_sensitive(GTK_WIDGET(gui.start)));
    gui.devices->is_system_drive = false;
    flashux_gui_start(NULL, &gui);
    gtk_window_close(gui.window);
    pump(50);
    CHECK(gui.closed && gui.ctx == NULL && gui.progress_source == 0 && gui.job_source == 0);
    g_object_set_data(G_OBJECT(gui.window), "flashux-state", NULL);
    g_clear_object(&gui.window);
    g_clear_object(&gui.application);
    (void)pthread_mutex_destroy(&gui.mutex);
    CHECK(close(image) == 0 && close(target) == 0);
    CHECK(unlink(image_path) == 0 && unlink(target_path) == 0);
    return 0;
}
