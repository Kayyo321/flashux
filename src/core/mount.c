#include "device_internal.h"
#include "flashux_config.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#if FLASHUX_LINUX
#include <libmount/libmount.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif

flashux_error_t flashux_device_lock(flashux_context_t *ctx, const flashux_device_t *device,
                                   int *lock_fd_out)
{
    if (ctx == NULL || device == NULL || lock_fd_out == NULL ||
        memchr(device->node_path, 0, sizeof(device->node_path)) == NULL) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    *lock_fd_out = -1;
#if FLASHUX_LINUX
    flashux_device_t *list = NULL;
    flashux_device_t *found = NULL;
    char canonical[4096];
    struct stat before;
    struct stat opened;
    flashux_error_t result;
    if (realpath(device->node_path, canonical) == NULL ||
        strncmp(canonical, "/dev/", 5) != 0 || stat(canonical, &before) != 0 ||
        !S_ISBLK(before.st_mode)) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    result = flashux_device_list_scan(ctx, true, &list);
    if (result != FLASHUX_SUCCESS) {
        return result;
    }
    for (flashux_device_t *item = list; item != NULL; item = item->next) {
        if (strcmp(item->node_path, canonical) == 0) {
            found = item;
            break;
        }
    }
    if (!flashux_device_is_safe_target(ctx, found, NULL, 0)) {
        flashux_device_list_free(&list);
        return FLASHUX_ERR_SYSTEM_DISK_PROTECT;
    }
    flashux_device_list_free(&list);
    int fd = open(canonical, O_RDWR | O_EXCL | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        return FLASHUX_ERR_DEVICE_BUSY;
    }
    if (fstat(fd, &opened) != 0 || !S_ISBLK(opened.st_mode) ||
        opened.st_rdev != before.st_rdev || flock(fd, LOCK_EX | LOCK_NB) != 0) {
        (void)close(fd);
        return FLASHUX_ERR_DEVICE_BUSY;
    }
    *lock_fd_out = fd;
    return FLASHUX_SUCCESS;
#else
    return FLASHUX_ERR_UNSUPPORTED;
#endif
}

void flashux_device_unlock(int *lock_fd)
{
    if (lock_fd != NULL && *lock_fd >= 0) {
        (void)flock(*lock_fd, LOCK_UN);
        (void)close(*lock_fd);
        *lock_fd = -1;
    }
}

flashux_error_t flashux_unmount_device_partitions(flashux_context_t *ctx,
                                                 const flashux_device_t *device)
{
    if (ctx == NULL || device == NULL || device->is_system_drive ||
        memchr(device->node_path, 0, sizeof(device->node_path)) == NULL) {
        return FLASHUX_ERR_INVALID_ARG;
    }
#if FLASHUX_LINUX
    flashux_device_t *fresh = NULL;
    char canonical[4096];
    flashux_error_t scan_result = flashux_device_list_scan(ctx, true, &fresh);
    if (scan_result != FLASHUX_SUCCESS) { return scan_result; }
    const flashux_device_t *validated = NULL;
    if (realpath(device->node_path, canonical) != NULL) {
        for (flashux_device_t *item = fresh; item != NULL; item = item->next) {
            if (strcmp(item->node_path, canonical) == 0) { validated = item; break; }
        }
    }
    if (!flashux_device_is_safe_target(ctx, validated, NULL, 0)) {
        flashux_device_list_free(&fresh);
        return FLASHUX_ERR_SYSTEM_DISK_PROTECT;
    }
    device = validated;
    struct stat disk_node;
    if (stat(device->node_path, &disk_node) != 0 || !S_ISBLK(disk_node.st_mode)) {
        flashux_device_list_free(&fresh);
        return FLASHUX_ERR_NOT_FOUND;
    }
    struct libmnt_table *table = mnt_new_table_from_file("/proc/self/mountinfo");
    struct libmnt_iter *iter = mnt_new_iter(MNT_ITER_BACKWARD);
    struct libmnt_fs *fs;
    flashux_error_t result = FLASHUX_SUCCESS;
    if (table == NULL || iter == NULL) {
        if (table != NULL) {
            mnt_free_table(table);
        }
        if (iter != NULL) {
            mnt_free_iter(iter);
        }
        flashux_device_list_free(&fresh);
        return FLASHUX_ERR_IO;
    }
    while (mnt_table_next_fs(table, iter, &fs) == 0) {
        const char *source = mnt_fs_get_source(fs);
        const char *target = mnt_fs_get_target(fs);
        if (source == NULL || target == NULL) {
            result = FLASHUX_ERR_IO;
            break;
        }
        dev_t mounted = mnt_fs_get_devno(fs);
        bool matched = mounted == disk_node.st_rdev;
        for (flashux_partition_t *part = device->partitions; part != NULL; part = part->next) {
            struct stat part_node;
            if (stat(part->node_path, &part_node) != 0 || !S_ISBLK(part_node.st_mode)) {
                result = FLASHUX_ERR_NOT_FOUND;
                break;
            }
            matched = matched || mounted == part_node.st_rdev;
        }
        if (result != FLASHUX_SUCCESS) { break; }
        if (matched && umount2(target, 0) != 0) {
            flashux_log_error(ctx, "Unable to unmount %s", target);
            result = FLASHUX_ERR_DEVICE_BUSY;
            break;
        }
    }
    mnt_free_iter(iter);
    mnt_free_table(table);
    flashux_device_list_free(&fresh);
    return result;
#else
    return FLASHUX_ERR_UNSUPPORTED;
#endif
}

flashux_error_t flashux_device_capture_sequence(const char *path, uint64_t *sequence)
{
    if (path == NULL || sequence == NULL) { return FLASHUX_ERR_INVALID_ARG; }
    *sequence = 0;
#if FLASHUX_LINUX
    struct stat st;
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) { return FLASHUX_ERR_NOT_FOUND; }
    flashux_error_t result = fstat(fd, &st) == 0 && S_ISBLK(st.st_mode) &&
        ioctl(fd, BLKGETDISKSEQ, sequence) == 0 && *sequence != 0 ?
        FLASHUX_SUCCESS : FLASHUX_ERR_UNSUPPORTED;
    (void)close(fd);
    return result;
#else
    return FLASHUX_ERR_UNSUPPORTED;
#endif
}
