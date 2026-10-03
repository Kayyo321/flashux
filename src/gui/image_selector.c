#include "app_window.h"
#include <string.h>
static void image_selected(GObject *object, GAsyncResult *result, gpointer data)
{
    GtkWindow *window = data;
    struct flashux_gui *gui = g_object_get_data(G_OBJECT(window), "flashux-state");
    GError *error = NULL;
    GFile *file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(object), result, &error);
    if (file != NULL) {
        char *path = g_file_get_path(file);
        if (gui != NULL && !gui->closed && path != NULL) {
            if (strlen(path) < sizeof(((flashux_flash_options_t *)0)->source_image_path)) {
                gtk_editable_set_text(GTK_EDITABLE(gui->image), path);
            } else {
                gtk_label_set_text(gui->status, "The image path is too long");
            }
        }
        g_free(path);
        path = NULL;
        g_object_unref(file);
    } else if (gui != NULL && !gui->closed && error != NULL &&
               !g_error_matches(error, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED)) {
        gtk_label_set_text(gui->status, error->message);
    }
    g_clear_error(&error);
    g_object_unref(window);
}
void flashux_gui_choose_image(GtkButton *button, gpointer data)
{
    struct flashux_gui *gui = data;
    (void)button;
    GtkFileDialog *dialog = gtk_file_dialog_new();
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Disk images and compressed raw images");
    gtk_file_filter_add_pattern(filter, "*.iso");
    gtk_file_filter_add_pattern(filter, "*.ISO");
    gtk_file_filter_add_pattern(filter, "*.img");
    gtk_file_filter_add_pattern(filter, "*.raw");
    gtk_file_filter_add_pattern(filter, "*.gz");
    gtk_file_filter_add_pattern(filter, "*.xz");
    gtk_file_filter_add_pattern(filter, "*.bz2");
    gtk_file_filter_add_pattern(filter, "*.zst");
    gtk_file_filter_add_pattern(filter, "*.zip");
    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    gtk_file_dialog_set_default_filter(dialog, filter);
    gtk_file_dialog_set_title(dialog, "Select boot image");
    gtk_file_dialog_open(dialog, gui->window, NULL, image_selected, g_object_ref(gui->window));
    g_object_unref(filter);
    g_object_unref(filters);
    g_object_unref(dialog);
}
