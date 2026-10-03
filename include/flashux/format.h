#ifndef FLASHUX_FORMAT_H
#define FLASHUX_FORMAT_H

#include "flashux/core.h"
#include "flashux/flasher.h"

/**
 * @brief Configuration parameters for formatting a target partition.
 */
typedef struct {
    char partition_path[64];   /* e.g., "/dev/sdb1" */
    flashux_fs_type_t fs_type; /* FAT32, NTFS, exFAT, ext4 */
    char volume_label[128];    /* Filesystem label */
    uint32_t cluster_size;     /* Cluster size in bytes (0 for default) */
    bool quick_format;         /* Fast format vs full zeroing */
} flashux_format_options_t;

/**
 * @brief Formats a target partition with the specified filesystem.
 *
 * @param[in] ctx Flashux context.
 * @param[in] options Formatting parameters (filesystem type, label, cluster size).
 * @return FLASHUX_SUCCESS on success, negative error code on failure.
 */
flashux_error_t flashux_format_partition(flashux_context_t *ctx,
                                         const flashux_format_options_t *options);

#endif /* FLASHUX_FORMAT_H */
