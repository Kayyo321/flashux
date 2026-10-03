#ifndef FLASHUX_FLASHER_H
#define FLASHUX_FLASHER_H

#include "flashux/core.h"
#include "flashux/device.h"

/**
 * @brief Partitioning scheme requested.
 */
typedef enum {
    FLASHUX_PARTITION_SCHEME_MBR = 0,
    FLASHUX_PARTITION_SCHEME_GPT = 1,
    FLASHUX_PARTITION_SCHEME_HYBRID = 2
} flashux_partition_scheme_t;

/**
 * @brief Target boot firmware architecture.
 */
typedef enum {
    FLASHUX_TARGET_BIOS_CSM = 0,
    FLASHUX_TARGET_UEFI_NON_CSM = 1,
    FLASHUX_TARGET_UEFI_NTFS = 2
} flashux_target_system_t;

/**
 * @brief Output filesystem type.
 */
typedef enum {
    FLASHUX_FS_FAT16 = 0,
    FLASHUX_FS_FAT32 = 1,
    FLASHUX_FS_NTFS = 2,
    FLASHUX_FS_EXFAT = 3,
    FLASHUX_FS_EXT4 = 4,
    FLASHUX_FS_UDF = 5
} flashux_fs_type_t;

/**
 * @brief Flashing operational mode.
 */
typedef enum {
    FLASHUX_MODE_DD_RAW = 0,
    FLASHUX_MODE_ISO_EXTRACT = 1,
    FLASHUX_MODE_WINDOWS_TO_GO = 2,
    FLASHUX_MODE_FREEDOS = 3
} flashux_flash_mode_t;

/**
 * @brief Current execution phase during an active operation.
 */
typedef enum {
    FLASHUX_STAGE_IDLE = 0,
    FLASHUX_STAGE_VERIFYING_LOCK = 1,
    FLASHUX_STAGE_UNMOUNTING = 2,
    FLASHUX_STAGE_ZEROING_HEADER = 3,
    FLASHUX_STAGE_BAD_BLOCKS = 4,
    FLASHUX_STAGE_PARTITIONING = 5,
    FLASHUX_STAGE_FORMATTING = 6,
    FLASHUX_STAGE_WRITING = 7,
    FLASHUX_STAGE_SYNCING = 8,
    FLASHUX_STAGE_VERIFYING_READ = 9,
    FLASHUX_STAGE_COMPLETED = 10,
    FLASHUX_STAGE_FAILED = 11
} flashux_stage_t;

/**
 * @brief Windows 10/11 unattended customization options.
 */
typedef struct {
    bool bypass_tpm_secureboot_ram; /* Remove TPM 2.0 / Secure Boot / RAM checks */
    bool bypass_online_account;     /* Skip Microsoft Account requirement */
    bool create_local_account;      /* Create offline local account */
    char local_username[64];        /* Username for offline local account */
    bool match_regional_options;    /* Sync timezone and locale with host */
    bool disable_telemetry;         /* Skip privacy and diagnostic questions */
    bool disable_bitlocker;         /* Prevent automatic device encryption */
} flashux_windows_customizations_t;

/**
 * @brief Full specification for a flash job.
 */
typedef struct {
    char target_device_path[64]; /* e.g., "/dev/sdb" */
    char source_image_path[512]; /* e.g., "/home/user/ubuntu-24.04.iso" */
    char volume_label[128];      /* Target filesystem label */
    flashux_flash_mode_t mode;   /* Raw DD vs ISO Extraction */
    flashux_partition_scheme_t part_scheme;
    flashux_target_system_t target_sys;
    flashux_fs_type_t fs_type;
    uint32_t cluster_size;           /* 0 for default, or explicit cluster size */
    bool quick_format;               /* Quick vs full block zeroing */
    uint32_t bad_blocks_passes;      /* 0 to skip, 1..4 passes */
    uint64_t persistence_size_bytes; /* 0 to disable, or size in bytes */
    flashux_windows_customizations_t win_custom;
    bool dry_run;                    /* Simulate writes without modifying disk */
    uint64_t expected_disk_sequence; /* Linux kernel identity captured before confirmation */
} flashux_flash_options_t;

/**
 * @brief Real-time performance metrics delivered via callbacks.
 */
typedef struct {
    flashux_stage_t stage;
    uint64_t bytes_processed;
    uint64_t total_bytes;
    double current_speed_bps; /* Bytes per second */
    double average_speed_bps; /* Bytes per second over whole operation */
    uint64_t elapsed_nanos;
    uint32_t eta_seconds;
    double percent_complete;     /* 0.0 to 100.0 */
    char stage_description[128]; /* Human-readable status message */
} flashux_progress_t;

/**
 * @brief Progress update callback function pointer.
 */
typedef void (*flashux_progress_cb)(const flashux_progress_t *progress, void *user_data);

/**
 * @brief Validate flashing options for consistency and target safety.
 *
 * Checks image path accessibility, target device validity, filesystem
 * compatibility, and minimum capacity requirements.
 *
 * @param[in] ctx Flashux context.
 * @param[in] options Flash options to validate.
 * @param[out] error_message_buf Optional buffer to receive error details.
 * @param[in] buf_size Size of error_message_buf.
 * @return FLASHUX_SUCCESS if options are valid, error code otherwise.
 */
flashux_error_t flashux_flash_validate_options(flashux_context_t *ctx,
                                               const flashux_flash_options_t *options,
                                               char *error_message_buf, size_t buf_size);

/**
 * @brief Start flashing operation asynchronously.
 *
 * Dispatches the operation through the privileged helper or local worker thread.
 *
 * @param[in] ctx Flashux context.
 * @param[in] options Verified flashing options.
 * @param[in] progress_callback Real-time progress callback.
 * @param[in] user_data Opaque pointer passed to callback.
 * @return FLASHUX_SUCCESS if operation launched, error code otherwise.
 */
flashux_error_t flashux_flash_start_async(flashux_context_t *ctx,
                                          const flashux_flash_options_t *options,
                                          flashux_progress_cb progress_callback, void *user_data);

/**
 * @brief Request cooperative cancellation of an in-progress flash operation.
 *
 * @param[in] ctx Flashux context.
 * @return FLASHUX_SUCCESS if cancellation requested, error code otherwise.
 */
flashux_error_t flashux_flash_cancel(flashux_context_t *ctx);

/**
 * @brief Join the active operation and return its terminal result.
 * @param[in] ctx Context whose worker should be joined.
 * @return Job result, or FLASHUX_ERR_INVALID_ARG when no worker exists.
 * Call from the owning thread; concurrent wait/free calls and callback destruction
 * are unsupported. A subsequent operation may begin after this function returns.
 */
flashux_error_t flashux_flash_wait(flashux_context_t *ctx);

/**
 * @brief Query whether an asynchronous operation is still running.
 * @param[in] ctx Context to inspect.
 * @return true while the worker is active, false otherwise.
 */
bool flashux_flash_is_running(flashux_context_t *ctx);
#endif /* FLASHUX_FLASHER_H */
