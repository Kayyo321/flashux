#include "test.h"
#include "core/format_internal.h"
#include <string.h>
#include "core/wipe.h"
#include <unistd.h>
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
    CHECK(flashux_format_validate(&options, label) == FLASHUX_SUCCESS);
    char path[] = "/tmp/flashux-format-XXXXXX";
    int fd = mkstemp(path);
    CHECK(fd >= 0 && unlink(path) == 0 && ftruncate(fd, 1048613) == 0);
    unsigned char mark = 0xad;
    CHECK(pwrite(fd, &mark, 1, 0) == 1 && pwrite(fd, &mark, 1, 1048612) == 1);
    CHECK(flashux_zero_fd(fd, NULL, NULL) == FLASHUX_SUCCESS);
    unsigned char buffer[65536];
    off_t bytes = 0;
    while (bytes < 1048613) {
        ssize_t count = pread(fd, buffer, sizeof(buffer), bytes);
        CHECK(count > 0);
        for (ssize_t i = 0; i < count; ++i) { CHECK(buffer[i] == 0); }
        bytes += count;
    }
    CHECK(close(fd) == 0);
    return 0;
}
