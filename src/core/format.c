#include "format_internal.h"
#include "flashux/device.h"
#include "flashux_config.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#if FLASHUX_LINUX
#include <libudev.h>
#include <fcntl.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#endif

flashux_error_t flashux_format_validate(const flashux_format_options_t *options,
                                       char label[128])
{
    if (options == NULL || label == NULL ||
        memchr(options->partition_path, '\0', sizeof(options->partition_path)) == NULL ||
        memchr(options->volume_label, '\0', sizeof(options->volume_label)) == NULL ||
        strncmp(options->partition_path, "/dev/", 5) != 0 ||
        strstr(options->partition_path, "..") != NULL ||
        options->fs_type < FLASHUX_FS_FAT16 || options->fs_type > FLASHUX_FS_EXT4) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    uint32_t cluster = options->cluster_size;
    if (cluster != 0 && (cluster < 512 || (cluster & (cluster - 1)) != 0)) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    size_t limit = 127;
    uint32_t max_cluster = 65536;
    switch (options->fs_type) {
    case FLASHUX_FS_FAT16: case FLASHUX_FS_FAT32: limit = 11; break;
    case FLASHUX_FS_EXFAT: limit = 15; max_cluster = 32 * 1024 * 1024; break;
    case FLASHUX_FS_EXT4: limit = 16; max_cluster = 4096; break;
    default: break;
    }
    if (strlen(options->volume_label) > limit || cluster > max_cluster ||
        (options->fs_type == FLASHUX_FS_EXT4 && cluster != 0 && cluster < 1024)) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i <= strlen(options->volume_label); ++i) {
        unsigned char c = (unsigned char)options->volume_label[i];
        if (c != 0 && (c < 32 || c == 127)) {
            return FLASHUX_ERR_INVALID_ARG;
        }
        if (options->fs_type <= FLASHUX_FS_FAT32) {
            if (c >= 128 || (c != 0 && strchr("\"*+,./:;<=>?[\\]|", c) != NULL)) {
                return FLASHUX_ERR_INVALID_ARG;
            }
            if (c >= 'a' && c <= 'z') {
                c = (unsigned char)(c - 'a' + 'A');
            }
        }
        label[i] = (char)c;
    }
    return options->quick_format ? FLASHUX_SUCCESS : FLASHUX_ERR_UNSUPPORTED;
}

flashux_error_t flashux_format_partition(flashux_context_t *ctx,
                                        const flashux_format_options_t *options)
{
    char label[128] = {0};
    flashux_error_t result = flashux_format_validate(options, label);
    if (ctx == NULL) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    if (result != FLASHUX_SUCCESS) {
        return result;
    }
#if FLASHUX_LINUX
    if (geteuid() != 0) {
        return FLASHUX_ERR_PERMISSION_DENIED;
    }
    struct udev *udev = udev_new();
    struct stat st;
    struct udev_device *node = NULL;
    flashux_device_t disk = {0};
    int lock_fd = -1;
    int part_fd = -1;
    if (udev == NULL) {
        return FLASHUX_ERR_NO_MEMORY;
    }
    if (stat(options->partition_path, &st) != 0 || !S_ISBLK(st.st_mode)) {
        result = FLASHUX_ERR_INVALID_ARG;
        goto cleanup;
    }
    node = udev_device_new_from_devnum(udev, 'b', st.st_rdev);
    struct udev_device *parent = node == NULL ? NULL :
        udev_device_get_parent_with_subsystem_devtype(node, "block", "disk");
    if (parent == NULL || udev_device_get_devnode(parent) == NULL) {
        result = FLASHUX_ERR_INVALID_ARG;
        goto cleanup;
    }
    (void)snprintf(disk.node_path, sizeof(disk.node_path), "%s",
                   udev_device_get_devnode(parent));
    result = flashux_device_lock(ctx, &disk, &lock_fd);
    if (result != FLASHUX_SUCCESS) {
        goto cleanup;
    }
    part_fd = open(options->partition_path, O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    struct stat opened;
    if (part_fd < 0 || fstat(part_fd, &opened) != 0 || opened.st_rdev != st.st_rdev) {
        result = FLASHUX_ERR_DEVICE_BUSY;
        goto cleanup;
    }
    char size_arg[16];
    char *args[12];
    size_t argc = 0;
    const char *tool;
    if (options->fs_type <= FLASHUX_FS_FAT32) {
        tool = "/usr/sbin/mkfs.fat";
    } else if (options->fs_type == FLASHUX_FS_NTFS) {
        tool = "/usr/sbin/mkfs.ntfs";
    } else if (options->fs_type == FLASHUX_FS_EXFAT) {
        tool = "/usr/sbin/mkfs.exfat";
    } else {
        tool = "/usr/sbin/mkfs.ext4";
    }
    /* argv storage is mutable to meet POSIX exec signatures without const casts. */
    char executable[64];
    (void)snprintf(executable, sizeof(executable), "%s", tool);
    args[argc++] = executable;
    char flag_f[] = "-F";
    char fat16[] = "16";
    char fat32[] = "32";
    char flag_fast[] = "-f";
    char flag_label[] = "-L";
    char flag_fatlabel[] = "-n";
    char flag_exlabel[] = "-L";
    char flag_cluster[] = "-c";
    char flag_sectors[] = "-s";
    char flag_block[] = "-b";
    if (options->fs_type <= FLASHUX_FS_FAT32) {
        args[argc++] = flag_f;
        args[argc++] = options->fs_type == FLASHUX_FS_FAT16 ? fat16 : fat32;
    } else if (options->fs_type == FLASHUX_FS_NTFS) {
        args[argc++] = flag_fast;
    }
    args[argc++] = options->fs_type <= FLASHUX_FS_FAT32 ? flag_fatlabel :
        options->fs_type == FLASHUX_FS_EXFAT ? flag_exlabel : flag_label;
    args[argc++] = label;
    if (options->cluster_size != 0) {
        uint32_t size = options->cluster_size;
        if (options->fs_type <= FLASHUX_FS_FAT32) {
            uint32_t sector = (uint32_t)strtoul(
                udev_device_get_sysattr_value(parent, "queue/logical_block_size"), NULL, 10);
            if (sector == 0 || size < sector || size / sector > 128) {
                result = FLASHUX_ERR_INVALID_ARG;
                goto cleanup;
            }
            size /= sector;
        }
        (void)snprintf(size_arg, sizeof(size_arg), "%u", size);
        args[argc++] = options->fs_type <= FLASHUX_FS_FAT32 ? flag_sectors :
            options->fs_type == FLASHUX_FS_EXT4 ? flag_block : flag_cluster;
        args[argc++] = size_arg;
    }
    char target[] = "/proc/self/fd/3";
    args[argc++] = target;
    args[argc] = NULL;
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0) {
        result = FLASHUX_ERR_UNKNOWN;
        goto cleanup;
    }
    int error = posix_spawn_file_actions_adddup2(&actions, part_fd, 3);
    pid_t child;
    char path_env[] = "PATH=/usr/sbin:/usr/bin:/sbin:/bin";
    char locale_env[] = "LC_ALL=C";
    char *environment[] = {path_env, locale_env, NULL};
    if (error == 0) {
        error = posix_spawn(&child, executable, &actions, NULL, args, environment);
    }
    (void)posix_spawn_file_actions_destroy(&actions);
    if (error != 0) {
        result = error == ENOENT ? FLASHUX_ERR_NOT_FOUND : FLASHUX_ERR_IO;
    } else {
        int status = 0;
        pid_t waited;
        do { waited = waitpid(child, &status, 0); } while (waited < 0 && errno == EINTR);
        result = waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0 ?
            FLASHUX_SUCCESS : FLASHUX_ERR_IO;
    }
cleanup:
    if (part_fd >= 0) { (void)close(part_fd); }
    flashux_device_unlock(&lock_fd);
    if (node != NULL) { udev_device_unref(node); }
    udev_unref(udev);
    if (result != FLASHUX_SUCCESS) {
        flashux_log_error(ctx, "Formatting failed: %s", flashux_strerror(result));
    }
    return result;
#else
    return FLASHUX_ERR_UNSUPPORTED;
#endif
}
