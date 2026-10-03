#include "test.h"
#include "core/format_internal.h"
#include <string.h>
int main(void)
{
    flashux_format_options_t options = {0};
    char label[128];
    (void)snprintf(options.partition_path, sizeof(options.partition_path), "/dev/loop-test1");
    (void)snprintf(options.volume_label, sizeof(options.volume_label), "boot disk");
    options.quick_format = true;
    options.fs_type = FLASHUX_FS_FAT32;
    options.cluster_size = 4096;
    CHECK(flashux_format_validate(&options, label) == FLASHUX_SUCCESS);
    CHECK(strcmp(label, "BOOT DISK") == 0);
    options.cluster_size = 4097;
    CHECK(flashux_format_validate(&options, label) == FLASHUX_ERR_INVALID_ARG);
    options.cluster_size = 4096;
    (void)snprintf(options.volume_label, sizeof(options.volume_label), "../../bad");
    CHECK(flashux_format_validate(&options, label) == FLASHUX_ERR_INVALID_ARG);
    (void)snprintf(options.volume_label, sizeof(options.volume_label), "abcdefghijkl");
    CHECK(flashux_format_validate(&options, label) == FLASHUX_ERR_INVALID_ARG);
    options.fs_type = FLASHUX_FS_EXT4;
    CHECK(flashux_format_validate(&options, label) == FLASHUX_SUCCESS);
    options.cluster_size = 512;
    CHECK(flashux_format_validate(&options, label) == FLASHUX_ERR_INVALID_ARG);
    options.fs_type = FLASHUX_FS_EXFAT;
    options.cluster_size = 32 * 1024 * 1024;
    CHECK(flashux_format_validate(&options, label) == FLASHUX_SUCCESS);
    options.quick_format = false;
    CHECK(flashux_format_validate(&options, label) == FLASHUX_ERR_UNSUPPORTED);
    return 0;
}
