# Implementation Specification: Initial GUI/CLI Application Architecture

## 1. Feature Title & Purpose

### 1.1. Title
**Initial GUI/CLI Application Foundation and Core Subsystem Architecture** (`feature/initial-implementation`)

### 1.2. Purpose & Rufus Parity Role
The primary objective of this feature is to establish the complete foundational architecture for **Flashux**—the high-performance, native Linux equivalent of Rufus built strictly in **pure C (ISO C11)**. 

Prior to this feature, Linux lacked a unified, native tool offering full parity with Rufus: users relied either on simple raw block imagers (`dd`, Balena Etcher) that cannot repartition, format, or configure bootloaders, or fragmented distribution-specific utilities. Flashux resolves this by providing:
1. **`libflashux`**: A core, modular static library encapsulating device enumeration, drive safety classification, partition management, filesystem creation, image flashing, and verification.
2. **`flashux-cli`**: A versatile command-line and terminal UI executable supporting both an interactive curses/ncurses dashboard and a scriptable non-interactive batch mode emitting structured JSON.
3. **`flashux-gui`**: A modern, native graphical interface built with GTK 4 and `libadwaita`, faithfully replicating the ergonomic, single-window workflow of Rufus (target drive selection, boot selection, partition schemes, filesystem choice, cluster sizing, advanced bypasses, real-time progress, and bad sector checks).
4. **`flashux-helper`**: A privileged backend daemon executed via Polkit (`pkexec`) communicating over a local UNIX domain socket, enforcing strict privilege separation so that the GUI and CLI run without root privileges.
5. **Meson Build System**: A fast, deterministic build definition compiling with strict C11 compiler flags (`-Wall -Wextra -Wpedantic -Werror -std=c11`) and zero external language dependencies.

---

## 2. Architecture & Design

### 2.1. System Architecture Diagram

```
+-----------------------------------------------------------------------------------+
|                                 UNPRIVILEGED LAYER                                |
|                                                                                   |
|  +-------------------------------------+   +------------------------------------+ |
|  |             flashux-gui             |   |            flashux-cli             | |
|  |     (GTK 4 / libadwaita UI)         |   |    (ncurses TUI & Batch JSON)      | |
|  +------------------+------------------+   +-----------------+------------------+ |
|                     |                                        |                    |
|                     +--------------------+-------------------+                    |
|                                          |                                        |
|                                          v                                        |
|                     +----------------------------------------+                    |
|                     |               libflashux               |                    |
|                     |  - Core Context & Logging Subsystem    |                    |
|                     |  - Device Enumeration & Safety Filter  |                    |
|                     |  - Options Validation & Job State Mach |                    |
|                     |  - Progress Tracking & Metrics Engine  |                    |
|                     |  - IPC Client Transport Module         |                    |
|                     +--------------------+-------------------+                    |
+------------------------------------------|----------------------------------------+
                                           | UNIX Domain Socket (AF_UNIX)
                                           | IPC Protocol + Polkit Authorization
                                           v
+-----------------------------------------------------------------------------------+
|                                 PRIVILEGED LAYER                                  |
|                                                                                   |
|  +------------------------------------------------------------------------------+ |
|  |                          flashux-helper (Root Daemon)                        | |
|  |  - UNIX Domain Socket Server & Client Polkit Peer Credential Verification    | |
|  |  - Exclusive Block Device Locking (flock / O_EXCL)                           | |
|  |  - Partition Unmounting Engine (libmount / umount2)                          | |
|  |  - Partition Table Synthesis (MBR / GPT)                                     | |
|  |  - Direct I/O Block Flasher Engine (O_DIRECT, posix_memalign, streaming)     | |
|  |  - Filesystem Formatting Wrappers (FAT12/16/32, NTFS, exFAT, ext4)           | |
|  |  - Post-write Cache Flush (BLKFLSBUF / fdatasync) & Verification             | |
|  +------------------------------------------------------------------------------+ |
|                                          |                                        |
|                                          v                                        |
|                              Linux Kernel /dev/sd*, /dev/nvme*                    |
+-----------------------------------------------------------------------------------+
```

### 2.2. Directory & File Organization

The project follows a clean, modular structure conforming to `PROJECT.md`:

```
flashux/
├── include/
│   └── flashux/
│       ├── core.h              # Context, logging, error codes, versioning
│       ├── device.h            # Device discovery, geometry, safety validation
│       ├── flasher.h           # Flash engine configuration, progress, state machine
│       ├── format.h            # Filesystem formatting types and interfaces
│       ├── partition.h         # MBR and GPT partition table generation
│       ├── checksum.h          # Multi-threaded cryptographic checksum engine
│       ├── iso.h               # ISO 9660, UDF, and El Torito boot descriptors
│       ├── windows.h           # Windows bypasses, AutoUnattend.xml, wimlib
│       └── persistence.h       # Linux live persistence layout definitions
├── src/
│   ├── core/
│   │   ├── core.c              # flashux_context lifecycle
│   │   ├── logging.c           # Thread-safe logging subsystem
│   │   ├── device.c            # Device discovery and safety filter implementation
│   │   ├── flasher.c           # Flash validation and progress orchestrator
│   │   └── ipc_client.c        # Client-side UNIX domain socket IPC transport
│   ├── cli/
│   │   ├── main.c              # CLI entry point and signal routing
│   │   ├── cli_args.c          # getopt_long option parser
│   │   ├── cli_batch.c         # Headless batch mode with JSON output
│   │   └── cli_interactive.c   # ncurses interactive terminal dashboard
│   ├── gui/
│   │   ├── main.c              # GUI entry point (GtkApplication lifecycle)
│   │   ├── app_window.c        # Main application window (Rufus UI layout)
│   │   ├── device_selector.c   # Device dropdown and udev hotplug update listener
│   │   ├── progress_view.c     # Thread-safe progress bar and speed display
│   │   └── dialogs.c           # Confirmation modals, warnings, and error dialogs
│   ├── helper/
│   │   ├── helper_main.c       # Privileged daemon entry point and socket listener
│   │   ├── ipc_protocol.h      # Binary IPC message wire format and opcodes
│   │   └── polkit_auth.c       # Peer credential verification (SO_PEERCRED)
│   └── common/
│       └── string_compat.h     # Bounded safe string copy and utility functions
├── tests/
│   ├── test_core.c             # Context and logging unit tests
│   ├── test_device.c           # Mock device parsing and system disk safety tests
│   ├── test_cli_args.c         # Command-line parser regression tests
│   └── test_flasher.c          # Options validation and progress math tests
└── meson.build                 # Top-level Meson build definition
```

---

## 3. Data Structures & Types

All definitions follow strict ISO C11 syntax and defensive coding standards.

### 3.1. Error Reporting & Logging Types (`include/flashux/core.h`)

```c
#ifndef FLASHUX_CORE_H
#define FLASHUX_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Standard Flashux error return codes.
 */
typedef enum {
    FLASHUX_SUCCESS                 = 0,
    FLASHUX_ERR_INVALID_ARG         = -1,
    FLASHUX_ERR_NO_MEMORY           = -2,
    FLASHUX_ERR_DEVICE_BUSY         = -3,
    FLASHUX_ERR_SYSTEM_DISK_PROTECT = -4,
    FLASHUX_ERR_PERMISSION_DENIED   = -5,
    FLASHUX_ERR_IO                  = -6,
    FLASHUX_ERR_IPC                 = -7,
    FLASHUX_ERR_NOT_FOUND           = -8,
    FLASHUX_ERR_UNSUPPORTED         = -9,
    FLASHUX_ERR_CANCELLED           = -10,
    FLASHUX_ERR_CORRUPT_IMAGE       = -11,
    FLASHUX_ERR_UNKNOWN             = -99
} flashux_error_t;

/**
 * @brief Log severity levels.
 */
typedef enum {
    FLASHUX_LOG_DEBUG = 0,
    FLASHUX_LOG_INFO  = 1,
    FLASHUX_LOG_WARN  = 2,
    FLASHUX_LOG_ERROR = 3
} flashux_log_level_t;

/**
 * @brief Custom log callback signature.
 */
typedef void (*flashux_log_fn)(
    flashux_log_level_t level,
    const char *message,
    void *user_data
);

/**
 * @brief Forward declaration of core context.
 */
typedef struct flashux_context flashux_context_t;

#endif /* FLASHUX_CORE_H */
```

### 3.2. Target Storage Device Types (`include/flashux/device.h`)

```c
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

#endif /* FLASHUX_DEVICE_H */
```

### 3.3. Flashing Options & Progress State (`include/flashux/flasher.h`)

```c
#ifndef FLASHUX_FLASHER_H
#define FLASHUX_FLASHER_H

#include "flashux/core.h"
#include "flashux/device.h"

/**
 * @brief Partitioning scheme requested.
 */
typedef enum {
    FLASHUX_PARTITION_SCHEME_MBR    = 0,
    FLASHUX_PARTITION_SCHEME_GPT    = 1,
    FLASHUX_PARTITION_SCHEME_HYBRID = 2
} flashux_partition_scheme_t;

/**
 * @brief Target boot firmware architecture.
 */
typedef enum {
    FLASHUX_TARGET_BIOS_CSM         = 0,
    FLASHUX_TARGET_UEFI_NON_CSM     = 1,
    FLASHUX_TARGET_UEFI_NTFS        = 2
} flashux_target_system_t;

/**
 * @brief Output filesystem type.
 */
typedef enum {
    FLASHUX_FS_FAT16 = 0,
    FLASHUX_FS_FAT32 = 1,
    FLASHUX_FS_NTFS  = 2,
    FLASHUX_FS_EXFAT = 3,
    FLASHUX_FS_EXT4  = 4,
    FLASHUX_FS_UDF   = 5
} flashux_fs_type_t;

/**
 * @brief Flashing operational mode.
 */
typedef enum {
    FLASHUX_MODE_DD_RAW          = 0,
    FLASHUX_MODE_ISO_EXTRACT     = 1,
    FLASHUX_MODE_WINDOWS_TO_GO   = 2,
    FLASHUX_MODE_FREEDOS         = 3
} flashux_flash_mode_t;

/**
 * @brief Current execution phase during an active operation.
 */
typedef enum {
    FLASHUX_STAGE_IDLE           = 0,
    FLASHUX_STAGE_VERIFYING_LOCK = 1,
    FLASHUX_STAGE_UNMOUNTING     = 2,
    FLASHUX_STAGE_ZEROING_HEADER = 3,
    FLASHUX_STAGE_BAD_BLOCKS     = 4,
    FLASHUX_STAGE_PARTITIONING   = 5,
    FLASHUX_STAGE_FORMATTING     = 6,
    FLASHUX_STAGE_WRITING        = 7,
    FLASHUX_STAGE_SYNCING        = 8,
    FLASHUX_STAGE_VERIFYING_READ = 9,
    FLASHUX_STAGE_COMPLETED      = 10,
    FLASHUX_STAGE_FAILED         = 11
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
    char target_device_path[64];    /* e.g., "/dev/sdb" */
    char source_image_path[512];    /* e.g., "/home/user/ubuntu-24.04.iso" */
    char volume_label[128];         /* Target filesystem label */
    flashux_flash_mode_t mode;      /* Raw DD vs ISO Extraction */
    flashux_partition_scheme_t part_scheme;
    flashux_target_system_t target_sys;
    flashux_fs_type_t fs_type;
    uint32_t cluster_size;          /* 0 for default, or explicit cluster size */
    bool quick_format;              /* Quick vs full block zeroing */
    uint32_t bad_blocks_passes;     /* 0 to skip, 1..4 passes */
    uint64_t persistence_size_bytes;/* 0 to disable, or size in bytes */
    flashux_windows_customizations_t win_custom;
    bool dry_run;                   /* Simulate writes without modifying disk */
} flashux_flash_options_t;

/**
 * @brief Real-time performance metrics delivered via callbacks.
 */
typedef struct {
    flashux_stage_t stage;
    uint64_t bytes_processed;
    uint64_t total_bytes;
    double current_speed_bps;       /* Bytes per second */
    double average_speed_bps;       /* Bytes per second over whole operation */
    uint64_t elapsed_nanos;
    uint32_t eta_seconds;
    double percent_complete;        /* 0.0 to 100.0 */
    char stage_description[128];    /* Human-readable status message */
} flashux_progress_t;

/**
 * @brief Progress update callback function pointer.
 */
typedef void (*flashux_progress_cb)(
    const flashux_progress_t *progress,
    void *user_data
);

#endif /* FLASHUX_FLASHER_H */
```

### 3.4. Privileged IPC Wire Protocol (`src/helper/ipc_protocol.h`)

Communication between unprivileged client (`flashux-cli` or `flashux-gui`) and privileged daemon (`flashux-helper`) is conducted over a UNIX domain socket (`/run/flashux/helper.sock`) using fixed-header binary messages.

```c
#ifndef FLASHUX_IPC_PROTOCOL_H
#define FLASHUX_IPC_PROTOCOL_H

#include <stdint.h>
#include "flashux/flasher.h"

#define FLASHUX_IPC_MAGIC 0x464C5358 /* "FLSX" */
#define FLASHUX_IPC_SOCKET_PATH "/run/flashux/helper.sock"

typedef enum {
    FLASHUX_IPC_CMD_PING             = 0x01,
    FLASHUX_IPC_CMD_SCAN_DEVICES     = 0x02,
    FLASHUX_IPC_CMD_START_FLASH      = 0x03,
    FLASHUX_IPC_CMD_CANCEL_FLASH     = 0x04,
    FLASHUX_IPC_MSG_PROGRESS_UPDATE  = 0x10,
    FLASHUX_IPC_MSG_LOG_EVENT        = 0x11,
    FLASHUX_IPC_MSG_OPERATION_RESULT = 0x20
} flashux_ipc_msg_type_t;

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;         /* FLASHUX_IPC_MAGIC */
    uint16_t version;       /* Protocol version (1) */
    uint16_t msg_type;      /* flashux_ipc_msg_type_t */
    uint32_t payload_len;   /* Length of trailing payload in bytes */
    uint32_t sequence_id;   /* Monotonic sequence identifier */
} flashux_ipc_header_t;
#pragma pack(pop)

#endif /* FLASHUX_IPC_PROTOCOL_H */
```

---

## 4. API Function Signatures & Interfaces

### 4.1. Core Context Lifecycle (`include/flashux/core.h`)

```c
/**
 * @brief Allocate and initialize a new Flashux context.
 *
 * @param[out] ctx_out Pointer to receive the allocated context pointer.
 * @return FLASHUX_SUCCESS on success, negative flashux_error_t on failure.
 *
 * Ownership: Caller owns the context and must release it with flashux_context_free.
 */
flashux_error_t flashux_context_new(flashux_context_t **ctx_out);

/**
 * @brief Free a Flashux context and release all associated resources.
 *
 * @param[in,out] ctx Context to free; safely handles NULL. Pointer is cleared.
 */
void flashux_context_free(flashux_context_t **ctx);

/**
 * @brief Configure logging callback and minimum reporting level.
 *
 * @param[in] ctx Flashux context.
 * @param[in] min_level Minimum severity to report.
 * @param[in] callback Callback function pointer.
 * @param[in] user_data Opaque pointer passed to callback.
 */
void flashux_context_set_logger(
    flashux_context_t *ctx,
    flashux_log_level_t min_level,
    flashux_log_fn callback,
    void *user_data
);

/**
 * @brief Convert a Flashux error code into a human-readable English string.
 *
 * @param[in] err Error code.
 * @return Static null-terminated string description.
 */
const char *flashux_strerror(flashux_error_t err);
```

### 4.2. Storage Device Probing & Safety Validation (`include/flashux/device.h`)

```c
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
 * @param[in,out] list Pointer to head of device list.
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
```

### 4.3. Flashing Operations & Orchestration (`include/flashux/flasher.h`)

```c
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
flashux_error_t flashux_flash_validate_options(
    flashux_context_t *ctx,
    const flashux_flash_options_t *options,
    char *error_message_buf,
    size_t buf_size
);

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
flashux_error_t flashux_flash_start_async(
    flashux_context_t *ctx,
    const flashux_flash_options_t *options,
    flashux_progress_cb progress_callback,
    void *user_data
);

/**
 * @brief Request cooperative cancellation of an in-progress flash operation.
 *
 * @param[in] ctx Flashux context.
 * @return FLASHUX_SUCCESS if cancellation requested, error code otherwise.
 */
flashux_error_t flashux_flash_cancel(flashux_context_t *ctx);
```

---

## 5. System APIs, Kernel Interfaces & Algorithms

### 5.1. Linux Storage Subsystem (`libudev` & `/sys/block`)
- **Device Enumeration**:
  `udev_new()` initializes the udev context. `udev_enumerate_new()` scans the `block` subsystem. Devices with `DEVTYPE="disk"` are inspected. Partitions (`DEVTYPE="partition"`) are grouped under parent disks.
- **Hardware Bus Determination**:
  Walks the sysfs device hierarchy using `udev_device_get_parent_with_subsystem_devtype()`. Subsystems like `usb`, `mmc`, `scsi`, and `nvme` map directly to `flashux_bus_type_t`.
- **Hotplug Event Monitoring**:
  `udev_monitor_new_from_netlink(udev, "udev")` listens for `add`, `remove`, and `change` uevents, notifying the GUI or CLI interactively without polling.

### 5.2. Drive Safety & Mount Protection Algorithm
To ensure host OS survival, the safety algorithm executes the following sequence before permitting any write:
1. **Mount Table Inspection**: Reads `/proc/mounts` and `/proc/self/mountinfo`.
2. **Mount Resolution**: Translates canonical device paths (resolving symlinks such as `/dev/mapper/*` or `/dev/disk/by-id/*` via `realpath()`).
3. **Partition & Parent Mapping**: Any device node sharing a major/minor or sysfs ancestor with a filesystem mounted at `/`, `/boot`, `/boot/efi`, `/home`, `/usr`, `/var`, or active swap partitions in `/proc/swaps` is flagged:
   `device->is_system_drive = true;`
4. **Safety Interlock**: In `flashux_device_is_safe_target()`, devices with `is_system_drive == true` are completely rejected unless an explicit override flag is supplied, accompanied by critical confirmation prompts.

### 5.3. High-Throughput Direct I/O Engine (`flasher_dd`)
For raw disk flashing:
1. **Direct Alignment**: Allocates 1 MiB or 4 MiB transfer buffers aligned to 4096-byte boundaries using `posix_memalign()`.
2. **Page Cache Bypass**: Target block devices are opened with `O_WRONLY | O_DIRECT | O_SYNC | O_EXCL`.
3. **Concurrent Uncompress**: Compressed images (`.gz`, `.xz`, `.zst`, `.bz2`) stream through respective decoder pipelines directly into the direct I/O buffer ring.
4. **Cache Flushing**: Upon completion, forces hardware write buffers to commit to physical flash using `sync_file_range()`, `fdatasync()`, and `ioctl(fd, BLKFLSBUF)`.

### 5.4. Unprivileged Client & Privileged Helper Separation
- **Polkit Authorization**: The client spawns `pkexec flashux-helper` or connects to a running systemd daemon `/run/flashux/helper.sock`.
- **Peer Verification**: The helper queries caller credentials via `getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &ucred, ...)` and verifies Polkit authorization via `polkit_authority_check_authorization()`.

---

## 6. Error Handling & Failure Modes

| Failure Scenario | Detection Mechanism | Remediation & Recovery Strategy |
| :--- | :--- | :--- |
| **Sudden Device Removal** | `write()` returns `EIO` or `ENODEV`; udev monitor detects `remove`. | Abort write loop immediately; clear partial buffers; transition state machine to `FLASHUX_STAGE_FAILED`; notify UI with device disconnect alert. |
| **Host System Drive Selected** | `flashux_device_is_safe_target()` checks mount hierarchy. | Operation hard-blocked with `FLASHUX_ERR_SYSTEM_DISK_PROTECT`; UI disables "START" button; error logged. |
| **Device In Use / Locked** | `open(..., O_EXCL)` fails with `EBUSY` or `flock()` fails. | Inspect mounted partitions; invoke `flashux_unmount_device_partitions()`; if still busy, fail with clear process lock explanation. |
| **Corrupted Image / Bad Sector** | ISO parsing fails checksum or read error occurs during surface scan. | Report bad sector LBA offset; prompt user whether to abort or mark sector list; clean up context. |
| **Out of Memory** | `malloc`/`posix_memalign` returns `NULL`. | Immediate unwinding of call stack; free all partial allocations; return `FLASHUX_ERR_NO_MEMORY`. |
| **Privilege Escalation Rejected** | User cancels Polkit password prompt or `pkexec` fails. | Gracefully return `FLASHUX_ERR_PERMISSION_DENIED`; inform user that root privileges are required to write block devices. |

---

## 7. Security & Boundary Considerations

1. **Pure C11 Defensive Standards**:
   - Zero tolerance for `strcpy`, `strcat`, `sprintf`, `gets`.
   - All string formatting uses `snprintf` with target buffer sizeof constraints and explicit termination guarantees.
   - Pointers set to `NULL` immediately after `free()`.
2. **Path Sanitization**:
   - Device paths restricted strictly to `/dev/` prefixes; directory traversal sequences (`..`) rejected.
   - Image file paths verified with `access(R_OK)` before passing to write engine.
3. **Privilege Demarcation**:
   - GUI and CLI processes run under the invoking user's unprivileged desktop session.
   - All destructive disk access is confined to `flashux-helper`.
   - IPC messages are validated against strict payload bounds before decoding.
