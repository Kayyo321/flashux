#include "device_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
static bool attribute(const char *path, const char *key, uint64_t *value)
{
    char filename[4096];
    char text[128];
    char *end;
    int length = snprintf(filename, sizeof(filename), "%s/%s", path, key);
    if (length < 0 || (size_t)length >= sizeof(filename)) { return false; }
    FILE *file = fopen(filename, "r");
    if (file == NULL) { return false; }
    bool valid = fgets(text, sizeof(text), file) != NULL && !ferror(file);
    (void)fclose(file);
    if (!valid || text[0] < '0' || text[0] > '9') { return false; }
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (errno != 0) { return false; }
    while (isspace((unsigned char)*end)) { ++end; }
    if (*end != '\0') { return false; }
    *value = (uint64_t)parsed;
    return true;
}
flashux_error_t flashux_device_geometry_from_sysfs(const char *path, flashux_device_t *device)
{
    uint64_t sectors;
    uint64_t logical;
    uint64_t physical;
    uint64_t read_only;
    uint64_t removable;
    if (path == NULL || device == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    device->size_bytes = 0;
    device->logical_sector_sz = 0;
    device->phys_sector_sz = 0;
    device->is_read_only = true;
    device->is_removable = false;
    if (!attribute(path, "size", &sectors) || sectors == 0 || sectors > UINT64_MAX / 512 ||
        !attribute(path, "queue/logical_block_size", &logical) ||
        !attribute(path, "queue/physical_block_size", &physical) ||
        (logical != 512 && logical != 4096) || physical < logical || physical > UINT32_MAX ||
        (physical & (physical - 1)) != 0 || !attribute(path, "ro", &read_only) || read_only > 1 ||
        !attribute(path, "removable", &removable) || removable > 1) {
        return FLASHUX_ERR_IO;
    }
    device->size_bytes = sectors * 512; /* sysfs size always uses 512-byte units. */
    device->logical_sector_sz = (uint32_t)logical;
    device->phys_sector_sz = (uint32_t)physical;
    device->is_read_only = read_only != 0;
    device->is_removable = removable != 0;
    return FLASHUX_SUCCESS;
}
