#ifndef FLASHUX_PARTITION_H
#define FLASHUX_PARTITION_H

#include "flashux/core.h"
#include "flashux/flasher.h"

/**
 * @brief Single partition entry layout.
 */
typedef struct {
    uint8_t partition_number;       /* 1-based partition index */
    uint64_t start_lba;             /* Starting sector (LBA) */
    uint64_t sector_count;          /* Size in sectors */
    flashux_fs_type_t fs_type;      /* Type for partition code / GUID mapping */
    bool is_bootable;               /* Active flag for MBR / EFI flag for GPT */
    char label[64];                 /* Partition name / label */
    char type_guid[37];             /* GPT partition type GUID string */
} flashux_partition_entry_t;

/**
 * @brief Full partition layout definition for a target drive.
 */
typedef struct {
    char device_path[64];           /* e.g., "/dev/sdb" */
    flashux_partition_scheme_t scheme; /* MBR, GPT, or Hybrid */
    uint32_t sector_size;           /* 512 or 4096 */
    uint64_t total_sectors;         /* Disk capacity in sectors */
    size_t entry_count;             /* Number of partitions */
    flashux_partition_entry_t entries[16]; /* Array of partition definitions */
} flashux_partition_table_t;

/**
 * @brief Writes an MBR or GPT partition table layout to target block device.
 *
 * @param[in] ctx Flashux context.
 * @param[in] table Partition table configuration and partition entries.
 * @return FLASHUX_SUCCESS on success, negative error code on failure.
 */
flashux_error_t flashux_partition_table_create(
    flashux_context_t *ctx,
    const flashux_partition_table_t *table
);

#endif /* FLASHUX_PARTITION_H */
