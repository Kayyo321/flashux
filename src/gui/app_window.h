#ifndef FLASHUX_APP_WINDOW_H
#define FLASHUX_APP_WINDOW_H
#include "flashux/flasher.h"
#include <adwaita.h>
#include <pthread.h>
#include <stdatomic.h>
struct flashux_gui {
    AdwApplication *application;
    GtkWindow *window;
    flashux_context_t *ctx;
    flashux_device_t *devices;
    GtkDropDown *device_choice;
    GtkDropDown *mode;
    GtkDropDown *scheme;
    GtkDropDown *target;
    GtkDropDown *filesystem;
    GtkDropDown *cluster;
    GtkEntry *image;
    GtkEntry *label;
    GtkCheckButton *dry_run;
    GtkCheckButton *include_fixed;
    GtkButton *start;
    GtkButton *cancel;
    GtkButton *browse;
    GtkButton *customize;
    GtkProgressBar *progress;
    GtkLabel *status;
    pthread_mutex_t mutex;
    flashux_progress_t snapshot;
    guint progress_source;
    guint job_source;
    guint hotplug_source;
    void *udev;
    void *monitor;
    atomic_bool closed;
    bool busy;
    flashux_windows_customizations_t win_custom;
};
int flashux_gui_app_run(int argc, char **argv);
void flashux_gui_window_create(struct flashux_gui *gui);
void flashux_gui_refresh_devices(struct flashux_gui *gui);
void flashux_gui_device_monitor_start(struct flashux_gui *gui);
void flashux_gui_device_monitor_stop(struct flashux_gui *gui);
void flashux_gui_update_start(struct flashux_gui *gui);
void flashux_gui_choose_image(GtkButton *button, gpointer data);
void flashux_gui_customizations(GtkButton *button, gpointer data);
void flashux_gui_connect_actions(struct flashux_gui *gui);
void flashux_gui_start(GtkButton *button, gpointer data);
flashux_error_t flashux_gui_collect(struct flashux_gui *gui, flashux_flash_options_t *options);
void flashux_gui_launch_options(struct flashux_gui *gui,
                               const flashux_flash_options_t *options);
void flashux_gui_progress(const flashux_progress_t *progress, void *data);
void flashux_gui_progress_stop(struct flashux_gui *gui);
#endif
