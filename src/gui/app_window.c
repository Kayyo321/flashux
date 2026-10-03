#include "app_window.h"
#include <string.h>
static gboolean close_window(GtkWindow *window, gpointer data)
{
    struct flashux_gui *gui = data;
    (void)window;
    gui->closed = true;
    flashux_context_free(&gui->ctx);
    flashux_gui_progress_stop(gui);
    flashux_gui_device_monitor_stop(gui);
    return FALSE;
}
static GtkWidget *field(GtkBox *box, const char *title, GtkWidget *control)
{
    GtkWidget *label = gtk_label_new(title);
    gtk_label_set_xalign(GTK_LABEL(label), 0);
    gtk_box_append(box, label);
    gtk_box_append(box, control);
    return control;
}
static void paired_fields(GtkBox *box, const char *left_title, GtkWidget *left_control,
                           const char *right_title, GtkWidget *right_control)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_set_homogeneous(GTK_BOX(row), true);
    field(GTK_BOX(left), left_title, left_control);
    field(GTK_BOX(right), right_title, right_control);
    gtk_box_append(GTK_BOX(row), left);
    gtk_box_append(GTK_BOX(row), right);
    gtk_box_append(box, row);
}
void flashux_gui_update_start(struct flashux_gui *gui)
{
    bool raw = gtk_drop_down_get_selected(gui->mode) == FLASHUX_MODE_DD_RAW;
    gtk_widget_set_sensitive(GTK_WIDGET(gui->scheme), !raw && !gui->busy);
    gtk_widget_set_sensitive(GTK_WIDGET(gui->target), !raw && !gui->busy);
    gtk_widget_set_sensitive(GTK_WIDGET(gui->filesystem), !raw && !gui->busy);
    gtk_widget_set_sensitive(GTK_WIDGET(gui->cluster), !raw && !gui->busy);
    gtk_widget_set_sensitive(GTK_WIDGET(gui->label), !raw && !gui->busy);
    bool image = gtk_editable_get_text(GTK_EDITABLE(gui->image))[0] != '\0';
    guint position = gtk_drop_down_get_selected(gui->device_choice);
    flashux_device_t *device = gui->devices;
    for (guint i = 0; device != NULL && i < position; ++i) { device = device->next; }
    bool selected = device != NULL && flashux_device_is_safe_target(gui->ctx, device, NULL, 0);
    gtk_widget_set_sensitive(GTK_WIDGET(gui->start), image && selected && !gui->busy);
}
static void selection_changed(GObject *object, GParamSpec *parameter, gpointer data)
{
    (void)object;
    (void)parameter;
    flashux_gui_update_start(data);
}
static void image_changed(GtkEditable *editable, gpointer data)
{
    (void)editable;
    flashux_gui_update_start(data);
}
void flashux_gui_window_create(struct flashux_gui *gui)
{
    gui->window = GTK_WINDOW(adw_application_window_new(GTK_APPLICATION(gui->application)));
    g_object_ref(gui->window);
    gtk_window_set_title(gui->window, "Flashux");
    gtk_window_set_default_size(gui->window, 560, 760);
    g_object_set_data(G_OBJECT(gui->window), "flashux-state", gui);
    g_signal_connect(gui->window, "close-request", G_CALLBACK(close_window), gui);
    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(outer), adw_header_bar_new());
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(scroll, true);
    gtk_box_append(GTK_BOX(outer), scroll);
    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(body, 16);
    gtk_widget_set_margin_bottom(body, 20);
    gtk_widget_set_margin_start(body, 24);
    gtk_widget_set_margin_end(body, 24);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), body);
    GtkBox *box = GTK_BOX(body);
    gui->device_choice = GTK_DROP_DOWN(gtk_drop_down_new(NULL, NULL));
    field(box, "Device", GTK_WIDGET(gui->device_choice));
    gui->include_fixed = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("List fixed drives"));
    gtk_box_append(box, GTK_WIDGET(gui->include_fixed));
    GtkWidget *image_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gui->image = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_placeholder_text(gui->image, "Select an ISO or raw disk image");
    gtk_widget_set_hexpand(GTK_WIDGET(gui->image), true);
    gui->browse = GTK_BUTTON(gtk_button_new_with_label("Select…"));
    gtk_box_append(GTK_BOX(image_row), GTK_WIDGET(gui->image));
    gtk_box_append(GTK_BOX(image_row), GTK_WIDGET(gui->browse));
    field(box, "Boot selection", image_row);
    static const char *const modes[] = {"Raw image (DD)", "ISO extraction", "Windows To Go", "FreeDOS", NULL};
    gui->mode = GTK_DROP_DOWN(gtk_drop_down_new_from_strings(modes));
    gtk_box_append(box, GTK_WIDGET(gui->mode));
    GtkWidget *notice = gtk_label_new("Raw mode copies the image layout. ISO extraction and "
                                     "Windows setup features are planned for later releases.");
    gtk_label_set_wrap(GTK_LABEL(notice), true);
    gtk_label_set_xalign(GTK_LABEL(notice), 0);
    gtk_widget_add_css_class(notice, "dim-label");
    gtk_box_append(box, notice);
    static const char *const schemes[] = {"MBR", "GPT", "Hybrid MBR/GPT", NULL};
    static const char *const targets[] = {"BIOS / CSM", "UEFI", "UEFI:NTFS", NULL};
    static const char *const filesystems[] = {"FAT16", "FAT32", "NTFS", "exFAT", "ext4", "UDF", NULL};
    static const char *const clusters[] = {"Default", "512", "1024", "2048", "4096", "8192", "16384", "32768", "65536", NULL};
    gui->scheme = GTK_DROP_DOWN(gtk_drop_down_new_from_strings(schemes));
    gui->target = GTK_DROP_DOWN(gtk_drop_down_new_from_strings(targets));
    gui->filesystem = GTK_DROP_DOWN(gtk_drop_down_new_from_strings(filesystems));
    gui->cluster = GTK_DROP_DOWN(gtk_drop_down_new_from_strings(clusters));
    gtk_drop_down_set_selected(gui->scheme, FLASHUX_PARTITION_SCHEME_GPT);
    gtk_drop_down_set_selected(gui->target, FLASHUX_TARGET_UEFI_NON_CSM);
    gtk_drop_down_set_selected(gui->filesystem, FLASHUX_FS_FAT32);
    paired_fields(box, "Partition scheme", GTK_WIDGET(gui->scheme),
                  "Target system", GTK_WIDGET(gui->target));
    paired_fields(box, "File system", GTK_WIDGET(gui->filesystem),
                  "Cluster size (bytes)", GTK_WIDGET(gui->cluster));
    gui->label = GTK_ENTRY(gtk_entry_new());
    gtk_entry_set_max_length(gui->label, 127);
    field(box, "Volume label", GTK_WIDGET(gui->label));
    gui->customize = GTK_BUTTON(gtk_button_new_with_label("Windows user experience…"));
    gtk_box_append(box, GTK_WIDGET(gui->customize));
    gui->dry_run = GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Dry run (leave target unchanged)"));
    gtk_box_append(box, GTK_WIDGET(gui->dry_run));
    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(footer, 12);
    gtk_widget_set_margin_bottom(footer, 20);
    gtk_widget_set_margin_start(footer, 24);
    gtk_widget_set_margin_end(footer, 24);
    gtk_box_append(GTK_BOX(outer), footer);
    box = GTK_BOX(footer);
    gui->progress = GTK_PROGRESS_BAR(gtk_progress_bar_new());
    gtk_progress_bar_set_show_text(gui->progress, true);
    gtk_box_append(box, GTK_WIDGET(gui->progress));
    gui->status = GTK_LABEL(gtk_label_new("Select a device and image to begin"));
    gtk_label_set_wrap(gui->status, true);
    gtk_label_set_xalign(gui->status, 0);
    gtk_box_append(box, GTK_WIDGET(gui->status));
    GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gui->start = GTK_BUTTON(gtk_button_new_with_label("Start"));
    gui->cancel = GTK_BUTTON(gtk_button_new_with_label("Cancel"));
    gtk_widget_add_css_class(GTK_WIDGET(gui->start), "suggested-action");
    gtk_widget_set_sensitive(GTK_WIDGET(gui->cancel), false);
    gtk_box_append(GTK_BOX(buttons), GTK_WIDGET(gui->start));
    gtk_box_append(GTK_BOX(buttons), GTK_WIDGET(gui->cancel));
    gtk_box_append(box, buttons);
    g_signal_connect(gui->mode, "notify::selected", G_CALLBACK(selection_changed), gui);
    g_signal_connect(gui->device_choice, "notify::selected", G_CALLBACK(selection_changed), gui);
    g_signal_connect(gui->image, "changed", G_CALLBACK(image_changed), gui);
    g_signal_connect(gui->browse, "clicked", G_CALLBACK(flashux_gui_choose_image), gui);
    g_signal_connect(gui->customize, "clicked", G_CALLBACK(flashux_gui_customizations), gui);
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(gui->window), outer);
    flashux_gui_connect_actions(gui);
    flashux_gui_device_monitor_start(gui);
    flashux_gui_update_start(gui);
}
