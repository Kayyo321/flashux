#include "test.h"
#include "core/device_internal.h"
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
static void fixture(const char *root, const char *name, const char *value)
{
    char path[512];
    (void)snprintf(path, sizeof(path), "%s/%s", root, name);
    FILE *file = fopen(path, "w");
    CHECK(file != NULL);
    CHECK(fputs(value, file) >= 0);
    CHECK(fclose(file) == 0);
}
int main(void)
{
    char root[] = "/tmp/flashux-sysfs-XXXXXX";
    char path[512];
    flashux_device_t device = {0};
    CHECK(mkdtemp(root) != NULL);
    (void)snprintf(path, sizeof(path), "%s/queue", root);
    CHECK(mkdir(path, 0700) == 0);
    fixture(root, "size", "16384\n");
    fixture(root, "queue/logical_block_size", "4096\n");
    fixture(root, "queue/physical_block_size", "4096\n");
    fixture(root, "ro", "0\n");
    fixture(root, "removable", "1\n");
    CHECK(flashux_device_geometry_from_sysfs(root, &device) == FLASHUX_SUCCESS);
    CHECK(device.size_bytes == 8 * 1024 * 1024 && device.logical_sector_sz == 4096);
    (void)snprintf(device.node_path, sizeof(device.node_path), "/dev/loop-test");
    CHECK(flashux_device_is_safe_target(NULL, &device, NULL, 0));
    device.is_system_drive = true;
    CHECK(!flashux_device_is_safe_target(NULL, &device, NULL, 0));
    device.is_system_drive = false;
    fixture(root, "ro", "garbage\n");
    CHECK(flashux_device_geometry_from_sysfs(root, &device) == FLASHUX_ERR_IO);
    CHECK(device.is_read_only && device.size_bytes == 0);
    fixture(root, "ro", "0\n");
    fixture(root, "size", "18446744073709551615\n");
    CHECK(flashux_device_geometry_from_sysfs(root, &device) == FLASHUX_ERR_IO);
    static const char *const attributes[] = {"size", "queue/logical_block_size",
        "queue/physical_block_size", "ro", "removable"};
    for (size_t i = 0; i < sizeof(attributes) / sizeof(attributes[0]); ++i) {
        (void)snprintf(path, sizeof(path), "%s/%s", root, attributes[i]);
        CHECK(unlink(path) == 0);
    }
    (void)snprintf(path, sizeof(path), "%s/queue", root);
    CHECK(rmdir(path) == 0 && rmdir(root) == 0);
    flashux_device_t *list = calloc(1, sizeof(*list));
    CHECK(list != NULL);
    list->partitions = calloc(1, sizeof(*list->partitions));
    CHECK(list->partitions != NULL);
    flashux_device_list_free(&list);
    CHECK(list == NULL);
    return 0;
}
