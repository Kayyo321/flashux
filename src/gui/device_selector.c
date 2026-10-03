#include "app_window.h"
#include "flashux_config.h"
#include <stdio.h>
#include <string.h>
#if FLASHUX_LINUX
#include <libudev.h>
#include <glib-unix.h>
#endif
void flashux_gui_refresh_devices(struct flashux_gui *gui)
{
    char previous[64] = {0};
    guint selected = gtk_drop_down_get_selected(gui->device_choice);
    flashux_device_t *old = gui->devices;
    for (guint i = 0; old != NULL && i < selected; ++i) { old = old->next; }
    if (old != NULL) { (void)snprintf(previous, sizeof(previous), "%s", old->node_path); }
    flashux_device_t *fresh = NULL;
    flashux_error_t result = flashux_device_list_scan(gui->ctx,
        gtk_check_button_get_active(gui->include_fixed), &fresh);
    GtkStringList *model = gtk_string_list_new(NULL);
    guint index = 0;
    guint restore = GTK_INVALID_LIST_POSITION;
    for (flashux_device_t *device = fresh; device != NULL; device = device->next) {
        char text[256];
        bool safe = flashux_device_is_safe_target(gui->ctx, device, NULL, 0);
        (void)snprintf(text, sizeof(text), "%s — %s (%.1f GiB)%s", device->node_path,
            device->model, (double)device->size_bytes / 1073741824.0, safe ? "" : " [protected]");
        gtk_string_list_append(model, text);
        if (strcmp(device->node_path, previous) == 0) { restore = index; }
        ++index;
    }
    flashux_device_list_free(&gui->devices);
    gui->devices = fresh;
    gtk_drop_down_set_model(gui->device_choice, G_LIST_MODEL(model));
    g_object_unref(model);
    if (restore != GTK_INVALID_LIST_POSITION) { gtk_drop_down_set_selected(gui->device_choice, restore); }
    gtk_label_set_text(gui->status, result != FLASHUX_SUCCESS ? flashux_strerror(result) :
        fresh == NULL ? "No suitable devices found" : "Select an image to begin");
    flashux_gui_update_start(gui);
}
static void fixed_response(AdwMessageDialog *dialog, const char *response, gpointer data)
{
    GtkWindow *window = data;
    struct flashux_gui *gui = g_object_get_data(G_OBJECT(window), "flashux-state");
    (void)dialog;
    if (gui != NULL && !gui->closed) {
        if (strcmp(response, "list") == 0) { flashux_gui_refresh_devices(gui); }
        else { gtk_check_button_set_active(gui->include_fixed, false); }
    }
}
static void fixed_toggled(GtkCheckButton *button, gpointer data)
{
    struct flashux_gui *gui = data;
    if (gtk_check_button_get_active(button)) {
        GtkWidget *dialog = adw_message_dialog_new(gui->window, "List fixed drives?",
            "Fixed drives may contain operating systems and personal data. "
            "Flashux will continue to protect detected system drives.");
        adw_message_dialog_add_responses(ADW_MESSAGE_DIALOG(dialog), "cancel", "Cancel",
                                         "list", "List fixed drives", NULL);
        adw_message_dialog_set_response_appearance(ADW_MESSAGE_DIALOG(dialog), "list",
                                                   ADW_RESPONSE_DESTRUCTIVE);
        adw_message_dialog_set_close_response(ADW_MESSAGE_DIALOG(dialog), "cancel");
        gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), true);
        g_object_set_data_full(G_OBJECT(dialog), "parent-window", g_object_ref(gui->window),
                               g_object_unref);
        g_signal_connect(dialog, "response", G_CALLBACK(fixed_response), gui->window);
        gtk_window_present(GTK_WINDOW(dialog));
    } else { flashux_gui_refresh_devices(gui); }
}
#if FLASHUX_LINUX
static gboolean hotplug(gint fd, GIOCondition condition, gpointer data)
{
    struct flashux_gui *gui = data;
    (void)fd;
    if ((condition & (G_IO_HUP | G_IO_ERR)) != 0) {
        gui->hotplug_source = 0;
        return G_SOURCE_REMOVE;
    }
    struct udev_device *event = udev_monitor_receive_device(gui->monitor);
    if (event != NULL) {
        const char *action = udev_device_get_action(event);
        const char *path = udev_device_get_devnode(event);
        if (gui->busy && action != NULL && strcmp(action, "remove") == 0 && path != NULL) {
            guint selected = gtk_drop_down_get_selected(gui->device_choice);
            flashux_device_t *device = gui->devices;
            for (guint i = 0; device != NULL && i < selected; ++i) { device = device->next; }
            if (device != NULL && strcmp(path, device->node_path) == 0) {
                (void)flashux_flash_cancel(gui->ctx);
            }
        } else if (!gui->busy) { flashux_gui_refresh_devices(gui); }
        udev_device_unref(event);
    }
    return G_SOURCE_CONTINUE;
}
#endif
void flashux_gui_device_monitor_start(struct flashux_gui *gui)
{
    g_signal_connect(gui->include_fixed, "toggled", G_CALLBACK(fixed_toggled), gui);
    flashux_gui_refresh_devices(gui);
#if FLASHUX_LINUX
    gui->udev = udev_new();
    if (gui->udev == NULL) { return; }
    gui->monitor = udev_monitor_new_from_netlink(gui->udev, "udev");
    if (gui->monitor == NULL) { return; }
    if (udev_monitor_filter_add_match_subsystem_devtype(gui->monitor, "block", NULL) < 0 ||
        udev_monitor_enable_receiving(gui->monitor) < 0) { return; }
    gui->hotplug_source = g_unix_fd_add(udev_monitor_get_fd(gui->monitor),
        G_IO_IN | G_IO_ERR | G_IO_HUP, hotplug, gui);
#endif
}
void flashux_gui_device_monitor_stop(struct flashux_gui *gui)
{
    if (gui->hotplug_source != 0) { (void)g_source_remove(gui->hotplug_source); gui->hotplug_source = 0; }
#if FLASHUX_LINUX
    if (gui->monitor != NULL) { udev_monitor_unref(gui->monitor); gui->monitor = NULL; }
    if (gui->udev != NULL) { udev_unref(gui->udev); gui->udev = NULL; }
#endif
    flashux_device_list_free(&gui->devices);
}
