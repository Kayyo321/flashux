# Feature: Initial GUI/CLI Application Foundation

**Overall Progress: 18.18%**

## Overview
Establish the foundational build system (Meson + Ninja), the core shared library (`libflashux`), the command-line interface (`flashux-cli`), the GTK 4 / libadwaita graphical interface (`flashux-gui`), and the privileged backend helper daemon (`flashux-helper`) with IPC protocol in pure C (C11).

*Note: Since execution is hosted on macOS where native Linux kernel interfaces (`libudev`, `libblkid`, `/sys/block`, `Polkit`) are unavailable, implementation will focus on modular architecture and mock-safe verification. Per project directives, the tracker will not reach 100% until integration testing on native Linux hardware is completed.*

---

## In-Depth Todo List

### 1. Build System & Scaffolding
- [X] 1.1 Create top-level `meson.build` with C11 standard flags (`-std=c11`, `-Wall`, `-Wextra`, `-Wpedantic`, `-Werror`, `-D_GNU_SOURCE`, `-D_FILE_OFFSET_BITS=64`) [100%]
- [X] 1.2 Configure project dependency probing (`threads`, `glib-2.0`, `gtk4`, `libadwaita-1`, `ncurses`, `udev`, `blkid`, `mount`) [100%]
- [ ] 1.3 Configure directory tree targets for `libflashux`, `flashux-cli`, `flashux-gui`, `flashux-helper`, and `tests/` [0%]

### 2. Public API Headers & Data Models
- [X] 2.1 Define core context, logging, and error codes in `include/flashux/core.h` [100%]
- [X] 2.2 Define storage device, partition, bus enumeration, and locking structures in `include/flashux/device.h` [100%]
- [X] 2.3 Define flashing options, stages, performance metrics, and progress callbacks in `include/flashux/flasher.h` [100%]
- [X] 2.4 Define filesystem formatting interfaces and cluster size definitions in `include/flashux/format.h` [100%]
- [X] 2.5 Define MBR and GPT partition table generation abstractions in `include/flashux/partition.h` [100%]
- [X] 2.6 Define multi-threaded cryptographic checksum interfaces in `include/flashux/checksum.h` [100%]

### 3. Core Engine Implementation (`libflashux`)
- [ ] 3.1 Implement context lifecycle (`flashux_context_new`, `flashux_context_free`) in `src/core/core.c` [0%]
- [ ] 3.2 Implement thread-safe logging and error string translation in `src/core/logging.c` [0%]
- [ ] 3.3 Implement block device probing and sysfs parsing in `src/core/device.c` [0%]
- [ ] 3.4 Implement host system drive protection and mount inspection algorithm in `src/core/device.c` [0%]
- [ ] 3.5 Implement exclusive device locking (`flock`/`O_EXCL`) and partition unmounting in `src/core/mount.c` [0%]
- [ ] 3.6 Implement filesystem formatting dispatchers (FAT, NTFS, exFAT, ext4) in `src/core/format.c` [0%]
- [ ] 3.7 Implement MBR and GPT partition table binary layout synthesis in `src/core/partition.c` [0%]
- [ ] 3.8 Implement cryptographic checksum hashing engine in `src/core/checksum.c` [0%]
- [ ] 3.9 Implement flashing options validation and job state machine in `src/core/flasher.c` [0%]
- [ ] 3.10 Implement progress math, ETA computation, and moving average speed tracking in `src/core/flasher.c` [0%]

### 4. Command-Line Interface (`flashux-cli`)
- [ ] 4.1 Implement POSIX command-line option parser (`getopt_long`) in `src/cli/cli_args.c` [0%]
- [ ] 4.2 Implement non-interactive batch mode and structured JSON output in `src/cli/cli_batch.c` [0%]
- [ ] 4.3 Implement interactive curses/ncurses terminal dashboard in `src/cli/cli_interactive.c` [0%]
- [ ] 4.4 Implement CLI main entry point, signal handlers (`SIGINT`, `SIGTERM`), and exit routines in `src/cli/main.c` [0%]

### 5. Graphical User Interface (`flashux-gui`)
- [ ] 5.1 Implement GtkApplication lifecycle and application initialization in `src/gui/main.c` [0%]
- [ ] 5.2 Build Rufus single-window layout (Device, Image, Partition, Target, File System) in `src/gui/app_window.c` [0%]
- [ ] 5.3 Implement dynamic device dropdown population and hotplug signal listeners in `src/gui/device_selector.c` [0%]
- [ ] 5.4 Implement file chooser integration for ISO and raw disk images in `src/gui/app_window.c` [0%]
- [ ] 5.5 Implement Rufus Windows user experience customization modal in `src/gui/win_dialog.c` [0%]
- [ ] 5.6 Implement thread-safe progress bar and transfer speed updates via `g_idle_add` in `src/gui/progress_view.c` [0%]
- [ ] 5.7 Implement destructive operation confirmation modals and warning alerts in `src/gui/dialogs.c` [0%]

### 6. Privileged Backend Helper & IPC Subsystem (`flashux-helper`)
- [ ] 6.1 Define binary wire protocol, message headers, and command opcodes in `src/helper/ipc_protocol.h` [0%]
- [ ] 6.2 Implement UNIX domain socket server and event loop in `src/helper/helper_main.c` [0%]
- [ ] 6.3 Implement Polkit peer credential verification (`SO_PEERCRED`) in `src/helper/polkit_auth.c` [0%]
- [ ] 6.4 Implement client-side IPC transport and command dispatch in `src/core/ipc_client.c` [0%]
- [ ] 6.5 Implement privileged write engine and device operations in `src/helper/privileged_ops.c` [0%]

### 7. Unit Testing & Verification Harness
- [ ] 7.1 Implement core context and logging unit tests in `tests/test_core.c` [0%]
- [ ] 7.2 Implement mock device enumeration and system disk protection tests in `tests/test_device.c` [0%]
- [ ] 7.3 Implement CLI argument parsing and validation tests in `tests/test_cli_args.c` [0%]
- [ ] 7.4 Implement progress calculation and cancellation tests in `tests/test_flasher.c` [0%]
- [ ] 7.5 Implement filesystem formatting parameter and cluster tests in `tests/test_format.c` [0%]
- [ ] 7.6 Implement MBR/GPT partition binary layout synthesis tests in `tests/test_partition.c` [0%]
- [ ] 7.7 Implement binary IPC header serialization/framing tests in `tests/test_ipc.c` [0%]
- [ ] 7.8 Verify compiler diagnostics with zero warnings under `-Wall -Wextra -Wpedantic -Werror` [0%]
- [ ] 7.9 Verify zero memory leaks via Valgrind and AddressSanitizer [0%]

---

## Commits in this Branch

The final `HEAD` entry denotes this tracker-only commit. Its own hash cannot be embedded in its
contents; the next tracker update replaces it with the resolved short hash. No recursive audit
commits are created. Entries retain the additions and modifications from each detailed commit.

| Commit | Summary | Added | Removed / Modified |
| :--- | :--- | :--- | :--- |
| `0e2f289` | docs(initial-implementation): initialize architecture spec and progress tracker | feature/initial-implementation/IMPLEMENTATION.md: comprehensive technical and architectural specification; feature/initial-implementation/TRACKER.md: incremental todo tracker and commit log; ; Ref: feature/initial-implementation | See commit body |
| `bc312a8` | docs(tracker): record initial documentation commit in progress tracker | Hash 0e2f289 in commit history table | Replaced <pending> placeholder with exact commit hash 0e2f289 |
| `0b09e33` | docs(spec): fix typo in architecture diagram header | None | feature/initial-implementation/IMPLEMENTATION.md: corrected PRIVILEATED to PRIVILEGED |
| `b9e7f9b` | docs(tracker): update commit history with spec typo fix | Commit row for 0b09e33 | None |
| `2d48875` | docs(spec): add missing architecture sections, prototypes, and testing strategy | Section 6: Control Flow & State Transitions with stage diagram and IPC flow; Section 9: Testing & Verification Plan with loop device and sanitizer rules; Data structures: flashux_format_options_t, flashux_partition_entry_t, flashux_partition_table_t, flashux_checksum_result_t, flashux_cli_options_t; Function prototypes: flashux_log/flashux_log_* helpers, flashux_device_lock/unlock, flashux_unmount_device_partitions, flashux_format_partition, flashux_partition_table_create, flashux_checksum_file, flashux_cli_parse_args/run_*, flashux_gui_app_run, flashux_ipc_client_* | feature/initial-implementation/IMPLEMENTATION.md: expanded from 601 to 1058 lines with comprehensive C11 specifications |
| `f19ac36` | docs(tracker): expand todo milestones and update branch commit history | Todos 3.5, 3.6, 3.7, 3.8, 5.5, 6.5, 7.5, 7.6, 7.7; Commit rows for bc312a8, b9e7f9b, and 2d48875 in commit history table | feature/initial-implementation/TRACKER.md: updated todo numbering and commit descriptions |
| `cca7fb5` | docs(spec): define defensive foundation boundaries | Section 10 with IPC encoding, platform policy, lifecycle and verification rules | Clarified system disk protection and tracker self-hash bookkeeping |
| `3d01112` | docs(tracker): record define defensive foundation boundaries | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `81fbfc2` | chore(build): establish strict C11 Meson configuration | meson.build with C11 and all required compiler flags | No production targets yet; empty scaffold configuration verified |
| `8dd6bae` | docs(tracker): record establish strict C11 Meson configuration | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `4d50a3c` | chore(build): exclude generated Meson artifacts | Ignore rule for build directories | .gitignore excludes generated build artifacts |
| `2964785` | docs(tracker): record exclude generated Meson artifacts | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `e4a2168` | chore(build): probe native component dependencies | Thread, crypto, curses, GTK and Linux dependency probes; generated platform config | Meson configuration now exposes gui feature option |
| `aa68390` | docs(tracker): record probe native component dependencies | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `7fbbe6e` | fix(build): defer source includes until headers exist | Successful platform dependency configuration | Removed premature include_directories declaration |
| `17e2fdf` | docs(tracker): record defer source includes until headers exist | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `7cfbe2b` | feat(api): define core public interface | core types and documented public function declarations | No runtime behavior changed; header compilation verified |
| `9d5dd61` | docs(tracker): record define core public interface | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `171e532` | feat(api): define device public interface | device types and documented public function declarations | No runtime behavior changed; header compilation verified |
| `dcadfef` | docs(tracker): record define device public interface | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `a92461d` | feat(api): define flasher public interface | flasher types and documented public function declarations | No runtime behavior changed; header compilation verified |
| `8f664d5` | docs(tracker): record define flasher public interface | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `e4daf36` | feat(api): define format public interface | format types and documented public function declarations | No runtime behavior changed; header compilation verified |
| `6b1ed4e` | docs(tracker): record define format public interface | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `540a66b` | feat(api): define partition public interface | partition types and documented public function declarations | No runtime behavior changed; header compilation verified |
| `fa6800c` | docs(tracker): record define partition public interface | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `d325258` | feat(api): define checksum public interface | checksum types and documented public function declarations | No runtime behavior changed; header compilation verified |
| `HEAD` | docs(tracker): record define checksum public interface | Audit entry for preceding implementation commit | Incremental progress and exhaustive branch history |
