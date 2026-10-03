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
static void topology(void)
{
    char root[] = "/tmp/flashux-topology-XXXXXX";
    char path[512];
    bool protected = false;
    static const char *const directories[] = {"dev", "dev/block", "devices", "devices/disk",
        "devices/disk/partition", "devices/mapped", "devices/mapped/slaves"};
    CHECK(mkdtemp(root) != NULL);
    for (size_t i = 0; i < sizeof(directories) / sizeof(directories[0]); ++i) {
        (void)snprintf(path, sizeof(path), "%s/%s", root, directories[i]);
        CHECK(mkdir(path, 0700) == 0);
    }
    fixture(root, "devices/disk/dev", "8:0\n");
    fixture(root, "devices/disk/partition/dev", "8:1\n");
    fixture(root, "devices/mapped/dev", "253:0\n");
    (void)snprintf(path, sizeof(path), "%s/dev/block/8:1", root);
    CHECK(symlink("../../devices/disk/partition", path) == 0);
    (void)snprintf(path, sizeof(path), "%s/dev/block/253:0", root);
    CHECK(symlink("../../devices/mapped", path) == 0);
    (void)snprintf(path, sizeof(path), "%s/devices/mapped/slaves/partition", root);
    CHECK(symlink("../../disk/partition", path) == 0);
    CHECK(flashux_device_topology_contains(root, 8, 1, 8, 0, &protected) == FLASHUX_SUCCESS);
    CHECK(protected);
    CHECK(flashux_device_topology_contains(root, 8, 1, 8, 16, &protected) == FLASHUX_SUCCESS);
    CHECK(!protected);
    CHECK(flashux_device_topology_contains(root, 253, 0, 8, 0, &protected) == FLASHUX_SUCCESS);
    CHECK(protected);
    CHECK(unlink(path) == 0);
    CHECK(symlink("..", path) == 0);
    CHECK(flashux_device_topology_contains(root, 253, 0, 8, 0, &protected) == FLASHUX_ERR_IO);
    CHECK(protected);
    CHECK(unlink(path) == 0);
    CHECK(flashux_device_topology_contains(root, 0, 42, 8, 0, &protected) == FLASHUX_ERR_IO);
    CHECK(protected);
    static const char *const files[] = {"dev/block/8:1", "dev/block/253:0",
        "devices/disk/dev", "devices/disk/partition/dev", "devices/mapped/dev"};
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {
        (void)snprintf(path, sizeof(path), "%s/%s", root, files[i]);
        CHECK(unlink(path) == 0);
    }
    for (size_t i = sizeof(directories) / sizeof(directories[0]); i > 0; --i) {
        (void)snprintf(path, sizeof(path), "%s/%s", root, directories[i - 1]);
        CHECK(rmdir(path) == 0);
    }
    CHECK(rmdir(root) == 0);
}
int main(void)
{
    topology();
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
