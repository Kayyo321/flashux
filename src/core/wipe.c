#include "wipe.h"
#include "flashux_config.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#if FLASHUX_LINUX
#include <sys/ioctl.h>
#include <linux/fs.h>
#endif
flashux_error_t flashux_zero_fd(int fd, bool (*cancel)(void *), void *cancel_data)
{
    struct stat target;
    uint64_t capacity = 0;
    if (fd < 0 || fstat(fd, &target) != 0) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    if (S_ISREG(target.st_mode) && target.st_size > 0) {
        capacity = (uint64_t)target.st_size;
    } else {
#if FLASHUX_LINUX
        if (!S_ISBLK(target.st_mode) || ioctl(fd, BLKGETSIZE64, &capacity) != 0) {
            return FLASHUX_ERR_INVALID_ARG;
        }
#else
        return FLASHUX_ERR_UNSUPPORTED;
#endif
    }
    if (capacity == 0 || capacity > (uint64_t)INT64_MAX) {
        return FLASHUX_ERR_INVALID_ARG;
    }
    const size_t block = 1024 * 1024;
    void *allocation = NULL;
    if (posix_memalign(&allocation, 4096, block) != 0) {
        return FLASHUX_ERR_NO_MEMORY;
    }
    memset(allocation, 0, block);
    unsigned char *buffer = allocation;
    flashux_error_t result = FLASHUX_SUCCESS;
    uint64_t offset = 0;
    while (offset < capacity) {
        if (cancel != NULL && cancel(cancel_data)) {
            result = FLASHUX_ERR_CANCELLED;
            break;
        }
        size_t count = capacity - offset < block ? (size_t)(capacity - offset) : block;
        size_t done = 0;
        while (done < count) {
            ssize_t written = pwrite(fd, buffer + done, count - done, (off_t)(offset + done));
            if (written < 0 && errno == EINTR) {
                continue;
            }
            if (written <= 0) {
                result = FLASHUX_ERR_IO;
                break;
            }
            done += (size_t)written;
        }
        if (result != FLASHUX_SUCCESS) {
            break;
        }
        offset += count;
    }
    if (fsync(fd) != 0 && result == FLASHUX_SUCCESS) {
        result = FLASHUX_ERR_IO;
    }
    free(allocation);
    allocation = NULL;
    buffer = NULL;
    return result;
}
