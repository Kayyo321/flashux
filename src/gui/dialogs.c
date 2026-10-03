#include "app_window.h"
#include <string.h>
struct reviewed_job {
    GtkWindow *parent;
    flashux_flash_options_t options;
};
static void free_review(gpointer data)
{
    struct reviewed_job *review = data;
    g_clear_object(&review->parent);
    g_free(review);
    review = NULL;
}
static void confirmation_response(AdwMessageDialog *dialog, const char *response, gpointer data)
{
    struct reviewed_job *review = data;
    struct flashux_gui *gui = g_object_get_data(G_OBJECT(review->parent), "flashux-state");
    (void)dialog;
    if (gui != NULL && !gui->closed && !gui->busy && strcmp(response, "erase") == 0) {
        flashux_gui_launch_options(gui, &review->options);
    }
}
static void cannot_start(struct flashux_gui *gui, const char *reason)
{
    GtkWidget *dialog = adw_message_dialog_new(gui->window, "Cannot start operation", reason);
    adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dialog), "close", "Close");
    adw_message_dialog_set_close_response(ADW_MESSAGE_DIALOG(dialog), "close");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), true);
    gtk_window_present(GTK_WINDOW(dialog));
    gtk_label_set_text(gui->status, reason);
    flashux_log_warn(gui->ctx, "%s", reason);
}
void flashux_gui_start(GtkButton *button, gpointer data)
{
    struct flashux_gui *gui = data;
    flashux_flash_options_t options;
    char reason[256] = {0};
    (void)button;
    if (gui->busy || gui->closed) { return; }
    flashux_error_t result = flashux_gui_collect(gui, &options);
    if (result == FLASHUX_SUCCESS) {
        result = flashux_flash_validate_options(gui->ctx, &options, reason, sizeof(reason));
    }
    if (result != FLASHUX_SUCCESS) {
        cannot_start(gui, reason[0] != '\0' ? reason : flashux_strerror(result));
        return;
    }
    if (options.dry_run) { flashux_gui_launch_options(gui, &options); return; }
    struct reviewed_job *review = g_try_new0(struct reviewed_job, 1);
    if (review == NULL) { cannot_start(gui, "Out of memory"); return; }
    review->parent = g_object_ref(gui->window);
    review->options = options;
    char body[1024];
    (void)g_snprintf(body, sizeof(body), "All data on %s will be destroyed.\n\nImage: %s\n\n"
        "Review the target before continuing.", options.target_device_path, options.source_image_path);
    GtkWidget *dialog = adw_message_dialog_new(gui->window, "Erase selected device?", body);
    adw_message_dialog_add_responses(ADW_MESSAGE_DIALOG(dialog), "cancel", "Cancel",
                                     "erase", "Erase and write", NULL);
    adw_message_dialog_set_response_appearance(ADW_MESSAGE_DIALOG(dialog), "erase", ADW_RESPONSE_DESTRUCTIVE);
    adw_message_dialog_set_default_response(ADW_MESSAGE_DIALOG(dialog), "cancel");
    adw_message_dialog_set_close_response(ADW_MESSAGE_DIALOG(dialog), "cancel");
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), true);
    g_object_set_data_full(G_OBJECT(dialog), "reviewed-job", review, free_review);
    g_signal_connect(dialog, "response", G_CALLBACK(confirmation_response), review);
    gtk_window_present(GTK_WINDOW(dialog));
}
static void cancel_job(GtkButton *button, gpointer data)
{
    struct flashux_gui *gui = data;
    (void)button;
    if (gui->busy) {
        (void)flashux_flash_cancel(gui->ctx);
        gtk_label_set_text(gui->status, "Cancelling…");
    }
}
void flashux_gui_connect_actions(struct flashux_gui *gui)
{
    g_signal_connect(gui->start, "clicked", G_CALLBACK(flashux_gui_start), gui);
    g_signal_connect(gui->cancel, "clicked", G_CALLBACK(cancel_job), gui);
}
