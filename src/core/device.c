#include "device_internal.h"
#include "flashux_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if FLASHUX_LINUX
#include <libudev.h>
#include <blkid.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/sysmacros.h>

static bool related_device(dev_t mounted, dev_t target)
{
    bool protected = true;
    (void)flashux_device_topology_contains("/sys", major(mounted), minor(mounted),
                                          major(target), minor(target), &protected);
    return protected;
}

static bool system_device(dev_t target)
{
    static const char *const paths[] = {"/", "/boot", "/boot/efi", "/home", "/usr", "/var"};
    struct stat st;
    FILE *swap;
    char line[1024];
    char source[512];
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        if (stat(paths[i], &st) == 0 && related_device(st.st_dev, target)) {
            return true;
        }
    }
    swap = fopen("/proc/swaps", "r");
    if (swap == NULL) {
        return true;
    }
    (void)fgets(line, sizeof(line), swap);
    while (fgets(line, sizeof(line), swap) != NULL) {
        if (sscanf(line, "%511s", source) != 1 || stat(source, &st) != 0) {
            (void)fclose(swap);
            return true;
        }
        if (related_device(S_ISBLK(st.st_mode) ? st.st_rdev : st.st_dev, target)) {
            (void)fclose(swap);
            return true;
        }
    }
    bool failed = ferror(swap) != 0;
    (void)fclose(swap);
    return failed;
}

static void copy_property(char *out, size_t size, const char *value)
{
    (void)snprintf(out, size, "%s", value == NULL ? "" : value);
}

static uint64_t read_number(struct udev_device *node, const char *key)
{
    const char *text = udev_device_get_sysattr_value(node, key);
    char *end;
    unsigned long long value;
    if (text == NULL || text[0] == '-') {
        return 0;
    }
    value = strtoull(text, &end, 10);
    return *end == '\0' ? (uint64_t)value : 0;
}

static flashux_error_t scan_partitions(struct udev *udev, struct udev_device *disk,
                                       flashux_device_t *out)
{
    struct udev_enumerate *scan = udev_enumerate_new(udev);
    struct udev_list_entry *entry;
    if (scan == NULL) {
        return FLASHUX_ERR_NO_MEMORY;
    }
    (void)udev_enumerate_add_match_parent(scan, disk);
    (void)udev_enumerate_add_match_subsystem(scan, "block");
    (void)udev_enumerate_scan_devices(scan);
    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(scan)) {
        struct udev_device *node = udev_device_new_from_syspath(udev,
            udev_list_entry_get_name(entry));
        const char *type = node == NULL ? NULL : udev_device_get_devtype(node);
        if (type != NULL && strcmp(type, "partition") == 0) {
            flashux_partition_t *part = calloc(1, sizeof(*part));
            if (part == NULL) {
                udev_device_unref(node);
                udev_enumerate_unref(scan);
                return FLASHUX_ERR_NO_MEMORY;
            }
            copy_property(part->node_path, sizeof(part->node_path),
                          udev_device_get_devnode(node));
            part->start_sector = read_number(node, "start");
            part->sector_count = read_number(node, "size");
            copy_property(part->fs_type, sizeof(part->fs_type),
                          udev_device_get_property_value(node, "ID_FS_TYPE"));
            copy_property(part->label, sizeof(part->label),
                          udev_device_get_property_value(node, "ID_FS_LABEL"));
            part->next = out->partitions;
            out->partitions = part;
        }
        if (node != NULL) {
            udev_device_unref(node);
        }
    }
    udev_enumerate_unref(scan);
    return FLASHUX_SUCCESS;
}
#endif

void flashux_device_list_free(flashux_device_t **list)
{
    if (list == NULL) {
        return;
    }
    while (*list != NULL) {
        flashux_device_t *next = (*list)->next;
        flashux_partition_t *part = (*list)->partitions;
        while (part != NULL) {
            flashux_partition_t *following = part->next;
            free(part);
            part = NULL;
            part = following;
        }
        free(*list);
        *list = NULL;
        *list = next;
    }
}

bool flashux_device_is_safe_target(flashux_context_t *ctx, const flashux_device_t *device,
                                  char *reason_buffer, size_t buffer_size)
{
    const char *reason = "";
    (void)ctx;
    if (device == NULL) {
        reason = "No device selected";
    } else if (device->is_system_drive) {
        reason = "System drive protected";
    } else if (device->is_read_only) {
        reason = "Device is read-only";
    } else if (device->size_bytes == 0 || device->logical_sector_sz == 0) {
        reason = "Device geometry unavailable";
    } else if (device->node_path[0] == '\0') {
        reason = "Device path unavailable";
    }
    if (reason_buffer != NULL && buffer_size > 0) {
        (void)snprintf(reason_buffer, buffer_size, "%s", reason);
    }
    return reason[0] == '\0';
}

flashux_error_t flashux_device_list_scan(flashux_context_t *ctx, bool include_fixed,
                                        flashux_device_t **list_out)
{
    if (ctx == NULL || list_out == NULL) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    *list_out = NULL;
#if FLASHUX_LINUX
    struct udev *udev = udev_new();
    struct udev_enumerate *scan;
    struct udev_list_entry *entry;
    flashux_error_t result = FLASHUX_SUCCESS;
    if (udev == NULL) {
        return FLASHUX_ERR_NO_MEMORY;
    }
    scan = udev_enumerate_new(udev);
    if (scan == NULL) {
        udev_unref(udev);
        return FLASHUX_ERR_NO_MEMORY;
    }
    (void)udev_enumerate_add_match_subsystem(scan, "block");
    if (udev_enumerate_scan_devices(scan) < 0) {
        result = FLASHUX_ERR_IO;
    } else {
        udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(scan)) {
            struct udev_device *node = udev_device_new_from_syspath(udev,
                udev_list_entry_get_name(entry));
            const char *type = node == NULL ? NULL : udev_device_get_devtype(node);
            if (type == NULL || strcmp(type, "disk") != 0) {
                if (node != NULL) {
                    udev_device_unref(node);
                }
                continue;
            }
            bool removable = read_number(node, "removable") != 0;
            const char *bus = udev_device_get_property_value(node, "ID_BUS");
            bool usb = bus != NULL && strcmp(bus, "usb") == 0;
            if (!include_fixed && !removable && !usb) {
                udev_device_unref(node);
                continue;
            }
            flashux_device_t *device = calloc(1, sizeof(*device));
            if (device == NULL) {
                udev_device_unref(node);
                result = FLASHUX_ERR_NO_MEMORY;
                break;
            }
            copy_property(device->node_path, sizeof(device->node_path),
                          udev_device_get_devnode(node));
            copy_property(device->sys_path, sizeof(device->sys_path),
                          udev_device_get_syspath(node));
            copy_property(device->vendor, sizeof(device->vendor),
                          udev_device_get_property_value(node, "ID_VENDOR"));
            copy_property(device->model, sizeof(device->model),
                          udev_device_get_property_value(node, "ID_MODEL"));
            copy_property(device->serial, sizeof(device->serial),
                          udev_device_get_property_value(node, "ID_SERIAL_SHORT"));
            (void)flashux_device_geometry_from_sysfs(udev_device_get_syspath(node), device);
            device->is_system_drive = system_device(udev_device_get_devnum(node));
            device->bus_type = usb ? FLASHUX_BUS_USB : FLASHUX_BUS_UNKNOWN;
            if (strncmp(udev_device_get_sysname(node), "loop", 4) == 0) {
                device->bus_type = FLASHUX_BUS_VIRTUAL;
            }
            device->next = *list_out;
            *list_out = device;
            result = scan_partitions(udev, node, device);
            udev_device_unref(node);
            if (result != FLASHUX_SUCCESS) {
                break;
            }
        }
    }
    udev_enumerate_unref(scan);
    udev_unref(udev);
    if (result != FLASHUX_SUCCESS) {
        flashux_device_list_free(list_out);
    }
    return result;
#else
    (void)include_fixed;
    flashux_log_warn(ctx, "%s", "Linux block device discovery is unavailable on this host");
    return FLASHUX_ERR_UNSUPPORTED;
#endif
}
