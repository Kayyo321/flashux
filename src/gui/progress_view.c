#include "app_window.h"
#include <stdio.h>
#include <string.h>
static gboolean paint_progress(gpointer data)
{
    struct flashux_gui *gui = data;
    flashux_progress_t progress;
    (void)pthread_mutex_lock(&gui->mutex);
    gui->progress_source = 0;
    progress = gui->snapshot;
    (void)pthread_mutex_unlock(&gui->mutex);
    if (!gui->closed) {
        char text[128];
        (void)snprintf(text, sizeof(text), "%.1f%% · %.1f MiB/s · ETA %u s",
            progress.percent_complete, progress.current_speed_bps / 1048576.0, progress.eta_seconds);
        gtk_progress_bar_set_fraction(gui->progress, progress.percent_complete / 100.0);
        gtk_progress_bar_set_text(gui->progress, text);
        gtk_label_set_text(gui->status, progress.stage_description);
    }
    return G_SOURCE_REMOVE;
}
void flashux_gui_progress(const flashux_progress_t *progress, void *data)
{
    struct flashux_gui *gui = data;
    (void)pthread_mutex_lock(&gui->mutex);
    if (!gui->closed) {
        gui->snapshot = *progress;
        if (gui->progress_source == 0) {
            gui->progress_source = g_idle_add(paint_progress, gui);
        }
    }
    (void)pthread_mutex_unlock(&gui->mutex);
}
void flashux_gui_progress_stop(struct flashux_gui *gui)
{
    if (gui->job_source != 0) { (void)g_source_remove(gui->job_source); gui->job_source = 0; }
    (void)pthread_mutex_lock(&gui->mutex);
    if (gui->progress_source != 0) {
        (void)g_source_remove(gui->progress_source);
        gui->progress_source = 0;
    }
    (void)pthread_mutex_unlock(&gui->mutex);
}
static void set_busy(struct flashux_gui *gui, bool busy)
{
    gui->busy = busy;
    GtkWidget *controls[] = {GTK_WIDGET(gui->device_choice), GTK_WIDGET(gui->image),
        GTK_WIDGET(gui->browse), GTK_WIDGET(gui->mode), GTK_WIDGET(gui->dry_run),
        GTK_WIDGET(gui->customize), GTK_WIDGET(gui->include_fixed)};
    for (size_t i = 0; i < sizeof(controls) / sizeof(controls[0]); ++i) {
        gtk_widget_set_sensitive(controls[i], !busy);
    }
    gtk_widget_set_sensitive(GTK_WIDGET(gui->cancel), busy);
    flashux_gui_update_start(gui);
}
static gboolean job_finished(gpointer data)
{
    struct flashux_gui *gui = data;
    if (flashux_flash_is_running(gui->ctx)) { return G_SOURCE_CONTINUE; }
    flashux_error_t result = flashux_flash_wait(gui->ctx);
    gui->job_source = 0;
    set_busy(gui, false);
    gtk_label_set_text(gui->status, flashux_strerror(result));
    return G_SOURCE_REMOVE;
}
flashux_error_t flashux_gui_collect(struct flashux_gui *gui, flashux_flash_options_t *options)
{
    guint selected = gtk_drop_down_get_selected(gui->device_choice);
    flashux_device_t *device = gui->devices;
    for (guint i = 0; device != NULL && i < selected; ++i) { device = device->next; }
    const char *image = gtk_editable_get_text(GTK_EDITABLE(gui->image));
    if (device == NULL || !flashux_device_is_safe_target(gui->ctx, device, NULL, 0) ||
        strlen(image) >= sizeof(options->source_image_path)) { return FLASHUX_ERR_INVALID_ARG; }
    memset(options, 0, sizeof(*options));
    (void)snprintf(options->source_image_path, sizeof(options->source_image_path), "%s", image);
    (void)snprintf(options->target_device_path, sizeof(options->target_device_path), "%s", device->node_path);
    (void)snprintf(options->volume_label, sizeof(options->volume_label), "%s",
        gtk_editable_get_text(GTK_EDITABLE(gui->label)));
    options->mode = (flashux_flash_mode_t)gtk_drop_down_get_selected(gui->mode);
    options->part_scheme = (flashux_partition_scheme_t)gtk_drop_down_get_selected(gui->scheme);
    options->target_sys = (flashux_target_system_t)gtk_drop_down_get_selected(gui->target);
    options->fs_type = (flashux_fs_type_t)gtk_drop_down_get_selected(gui->filesystem);
    static const uint32_t clusters[] = {0,512,1024,2048,4096,8192,16384,32768,65536};
    guint cluster = gtk_drop_down_get_selected(gui->cluster);
    if (cluster >= sizeof(clusters) / sizeof(clusters[0])) { return FLASHUX_ERR_INVALID_ARG; }
    options->cluster_size = clusters[cluster];
    options->dry_run = gtk_check_button_get_active(gui->dry_run);
    options->quick_format = true;
    options->win_custom = gui->win_custom;
    return FLASHUX_SUCCESS;
}
void flashux_gui_launch_options(struct flashux_gui *gui, const flashux_flash_options_t *options)
{
    set_busy(gui, true);
    gtk_progress_bar_set_fraction(gui->progress, 0);
    gtk_label_set_text(gui->status, "Starting operation…");
    flashux_error_t result = flashux_flash_start_async(gui->ctx, options, flashux_gui_progress, gui);
    if (result != FLASHUX_SUCCESS) {
        set_busy(gui, false);
        gtk_label_set_text(gui->status, flashux_strerror(result));
    } else { gui->job_source = g_timeout_add(50, job_finished, gui); }
}
