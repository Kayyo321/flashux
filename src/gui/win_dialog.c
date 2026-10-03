#include "app_window.h"
#include <string.h>
struct customization_dialog {
    GtkWindow *parent;
    GtkCheckButton *checks[6];
    GtkCheckButton *local;
    GtkEntry *username;
};
static void free_customization(gpointer data)
{
    struct customization_dialog *dialog = data;
    g_clear_object(&dialog->parent);
    g_free(dialog);
    dialog = NULL;
}
static void save_customizations(GtkButton *button, gpointer data)
{
    GtkWindow *window = data;
    struct customization_dialog *dialog = g_object_get_data(G_OBJECT(window), "customization");
    struct flashux_gui *gui = g_object_get_data(G_OBJECT(dialog->parent), "flashux-state");
    (void)button;
    if (gui != NULL && !gui->closed) {
        const char *username = gtk_editable_get_text(GTK_EDITABLE(dialog->username));
        if (strlen(username) >= sizeof(gui->win_custom.local_username)) { return; }
        gui->win_custom.bypass_tpm_secureboot_ram = gtk_check_button_get_active(dialog->checks[0]);
        gui->win_custom.bypass_online_account = gtk_check_button_get_active(dialog->checks[1]);
        gui->win_custom.match_regional_options = gtk_check_button_get_active(dialog->checks[2]);
        gui->win_custom.disable_telemetry = gtk_check_button_get_active(dialog->checks[3]);
        gui->win_custom.disable_bitlocker = gtk_check_button_get_active(dialog->checks[4]);
        gui->win_custom.create_local_account = gtk_check_button_get_active(dialog->local);
        (void)g_strlcpy(gui->win_custom.local_username, username, sizeof(gui->win_custom.local_username));
        gtk_label_set_text(gui->status, "Windows setup options are saved for this session. "
                           "Windows media creation is not yet supported.");
    }
    gtk_window_destroy(window);
}
void flashux_gui_customizations(GtkButton *button, gpointer data)
{
    struct flashux_gui *gui = data;
    (void)button;
    struct customization_dialog *dialog = g_try_new0(struct customization_dialog, 1);
    if (dialog == NULL) { return; }
    dialog->parent = g_object_ref(gui->window);
    GtkWindow *window = GTK_WINDOW(gtk_window_new());
    gtk_window_set_title(window, "Windows user experience");
    gtk_window_set_transient_for(window, gui->window);
    gtk_window_set_modal(window, true);
    gtk_window_set_destroy_with_parent(window, true);
    gtk_window_set_default_size(window, 520, 420);
    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_top(body, 20);
    gtk_widget_set_margin_bottom(body, 20);
    gtk_widget_set_margin_start(body, 20);
    gtk_widget_set_margin_end(body, 20);
    GtkWidget *notice = gtk_label_new("These options require the planned Windows installer engine. "
        "They cannot be applied by the initial raw-image foundation.");
    gtk_label_set_wrap(GTK_LABEL(notice), true);
    gtk_box_append(GTK_BOX(body), notice);
    static const char *const labels[] = {
        "Remove TPM, Secure Boot and RAM requirements", "Remove online account requirement",
        "Match regional options", "Skip privacy questions and telemetry", "Disable automatic BitLocker"
    };
    bool values[] = {gui->win_custom.bypass_tpm_secureboot_ram, gui->win_custom.bypass_online_account,
        gui->win_custom.match_regional_options, gui->win_custom.disable_telemetry,
        gui->win_custom.disable_bitlocker};
    for (unsigned i = 0; i < 5; ++i) {
        dialog->checks[i] = GTK_CHECK_BUTTON(gtk_check_button_new_with_label(labels[i]));
        gtk_check_button_set_active(dialog->checks[i], values[i]);
        gtk_box_append(GTK_BOX(body), GTK_WIDGET(dialog->checks[i]));
    }
    dialog->local = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Create a local account"));
    gtk_check_button_set_active(dialog->local, gui->win_custom.create_local_account);
    gtk_box_append(GTK_BOX(body), GTK_WIDGET(dialog->local));
    dialog->username = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_placeholder_text(dialog->username, "Local username");
    gtk_entry_set_max_length(dialog->username, 63);
    gtk_editable_set_text(GTK_EDITABLE(dialog->username), gui->win_custom.local_username);
    gtk_box_append(GTK_BOX(body), GTK_WIDGET(dialog->username));
    GtkWidget *save = gtk_button_new_with_label("Save options");
    gtk_box_append(GTK_BOX(body), save);
    g_object_set_data_full(G_OBJECT(window), "customization", dialog, free_customization);
    g_signal_connect(save, "clicked", G_CALLBACK(save_customizations), window);
    gtk_window_set_child(window, body);
    gtk_window_present(window);
}
