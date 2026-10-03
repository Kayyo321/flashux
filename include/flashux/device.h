#ifndef FLASHUX_DEVICE_H
#define FLASHUX_DEVICE_H

#include "flashux/core.h"

/**
 * @brief Hardware interconnect bus classification.
 */
typedef enum {
    FLASHUX_BUS_UNKNOWN   = 0,
    FLASHUX_BUS_USB       = 1,
    FLASHUX_BUS_SD_MMC    = 2,
    FLASHUX_BUS_SCSI_SATA = 3,
    FLASHUX_BUS_NVME      = 4,
    FLASHUX_BUS_VIRTUAL   = 5
} flashux_bus_type_t;

/**
 * @brief Partition metadata linked to a physical drive.
 */
typedef struct flashux_partition {
    char node_path[64];             /* e.g., "/dev/sdb1" */
    char mount_point[256];          /* e.g., "/media/user/DRIVE" or empty */
    char fs_type[32];               /* e.g., "vfat", "ntfs", "ext4" */
    char label[128];                /* Volume label */
    uint64_t start_sector;
    uint64_t sector_count;
    bool is_mounted;
    struct flashux_partition *next;
} flashux_partition_t;

/**
 * @brief Represents an enumerated physical or virtual block device.
 */
typedef struct flashux_device {
    char node_path[64];             /* e.g., "/dev/sdb" */
    char sys_path[256];             /* Sysfs path: /sys/class/block/sdb */
    char vendor[64];                /* Vendor string e.g. "SanDisk" */
    char model[64];                 /* Model string e.g. "Ultra USB 3.0" */
    char serial[128];               /* Hardware serial number */
    uint64_t size_bytes;            /* Total device capacity in bytes */
    uint32_t logical_sector_sz;     /* 512 or 4096 */
    uint32_t phys_sector_sz;        /* 512 or 4096 */
    bool is_removable;              /* 1 if removable flag is set in sysfs */
    bool is_read_only;              /* 1 if write-protected */
    bool is_system_drive;           /* 1 if contains mounted root '/', '/boot', etc. */
    flashux_bus_type_t bus_type;    /* Hardware interconnect type */
    flashux_partition_t *partitions;/* Linked list of detected child partitions */
    struct flashux_device *next;    /* Next device in enumerated linked list */
} flashux_device_t;

/**
 * @brief Enumerate all physical and virtual block devices currently present.
 *
 * @param[in] ctx Flashux context.
 * @param[in] include_fixed If false, only removable media are returned.
 *                          If true, non-removable fixed disks are included.
 * @param[out] list_out Pointer to receive head of linked list of devices.
 * @return FLASHUX_SUCCESS on success, negative error code on failure.
 *
 * Ownership: Caller takes ownership of the list and must free it with
 * flashux_device_list_free().
 */
flashux_error_t flashux_device_list_scan(
    flashux_context_t *ctx,
    bool include_fixed,
    flashux_device_t **list_out
);

/**
 * @brief Free an enumerated device linked list.
 *
 * @param[in,out] list Pointer to head of device list. Safely set to NULL.
 */
void flashux_device_list_free(flashux_device_t **list);

/**
 * @brief Verify whether a given block device is safe to overwrite.
 *
 * Checks if the target device contains the root filesystem ('/'), boot
 * partition ('/boot', '/boot/efi'), swap, or other active OS mounts.
 *
 * @param[in] ctx Flashux context.
 * @param[in] device Device structure to test.
 * @param[out] reason_buffer Optional buffer to receive human-readable safety reason.
 * @param[in] buffer_size Size of reason_buffer in bytes.
 * @return true if device is completely safe to overwrite; false otherwise.
 */
bool flashux_device_is_safe_target(
    flashux_context_t *ctx,
    const flashux_device_t *device,
    char *reason_buffer,
    size_t buffer_size
);

/**
 * @brief Acquire exclusive advisory / kernel lock on target block device.
 *
 * Opens device node with O_EXCL and acquires flock() lock.
 *
 * @param[in] ctx Flashux context.
 * @param[in] device Device to lock.
 * @param[out] lock_fd_out Pointer to receive file descriptor holding the lock.
 * @return FLASHUX_SUCCESS on success, negative error code if busy or locked.
 */
flashux_error_t flashux_device_lock(
    flashux_context_t *ctx,
    const flashux_device_t *device,
    int *lock_fd_out
);

/**
 * @brief Release block device lock and close descriptor.
 *
 * @param[in,out] lock_fd Pointer to open lock file descriptor. Safely sets to -1.
 */
void flashux_device_unlock(int *lock_fd);

/**
 * @brief Automatically unmount all mounted child partitions of a target drive.
 *
 * Queries mount tables and safely invokes umount2() via libmount.
 *
 * @param[in] ctx Flashux context.
 * @param[in] device Target device whose partitions must be unmounted.
 * @return FLASHUX_SUCCESS on success, negative error code if any partition remains busy.
 */
flashux_error_t flashux_unmount_device_partitions(
    flashux_context_t *ctx,
    const flashux_device_t *device
);

#endif /* FLASHUX_DEVICE_H */
