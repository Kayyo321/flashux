# Feature: Initial GUI/CLI Application Foundation

**Overall Progress: 0%**

## Overview
Establish the foundational build system (Meson + Ninja), the core shared library (`libflashux`), the command-line interface (`flashux-cli`), the GTK 4 / libadwaita graphical interface (`flashux-gui`), and the privileged backend helper daemon (`flashux-helper`) with IPC protocol in pure C (C11).

*Note: Since execution is hosted on macOS where native Linux kernel interfaces (`libudev`, `libblkid`, `/sys/block`, `Polkit`) are unavailable, implementation will focus on modular architecture and mock-safe verification. The tracker will not reach 100% until integration testing on native Linux hardware is completed.*

---

## In-Depth Todo List

### 1. Build System & Scaffolding
- [ ] 1.1 Create top-level `meson.build` with C11 standard flags (`-std=c11`, `-Wall`, `-Wextra`, `-Wpedantic`, `-Werror`, `-D_GNU_SOURCE`, `-D_FILE_OFFSET_BITS=64`) [0%]
- [ ] 1.2 Configure project dependency probing (`threads`, `glib-2.0`, `gtk4`, `libadwaita-1`, `ncurses`, `udev`, `blkid`, `mount`) [0%]
- [ ] 1.3 Configure directory tree targets for `libflashux`, `flashux-cli`, `flashux-gui`, `flashux-helper`, and `tests/` [0%]

### 2. Public API Headers & Data Models
- [ ] 2.1 Define core context, logging, and error codes in `include/flashux/core.h` [0%]
- [ ] 2.2 Define storage device, partition, and bus enumeration structures in `include/flashux/device.h` [0%]
- [ ] 2.3 Define flashing options, stages, performance metrics, and progress callbacks in `include/flashux/flasher.h` [0%]
- [ ] 2.4 Define filesystem formatting interfaces and cluster size definitions in `include/flashux/format.h` [0%]
- [ ] 2.5 Define MBR and GPT partition table generation abstractions in `include/flashux/partition.h` [0%]
- [ ] 2.6 Define multi-threaded cryptographic checksum interfaces in `include/flashux/checksum.h` [0%]

### 3. Core Engine Implementation (`libflashux`)
- [ ] 3.1 Implement context lifecycle (`flashux_context_new`, `flashux_context_free`) in `src/core/core.c` [0%]
- [ ] 3.2 Implement thread-safe logging and error string translation in `src/core/logging.c` [0%]
- [ ] 3.3 Implement block device probing and sysfs parsing in `src/core/device.c` [0%]
- [ ] 3.4 Implement host system drive protection and mount inspection algorithm in `src/core/device.c` [0%]
- [ ] 3.5 Implement flashing options validation and job dispatching in `src/core/flasher.c` [0%]
- [ ] 3.6 Implement progress math, ETA computation, and moving average speed tracking in `src/core/flasher.c` [0%]

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
- [ ] 5.5 Implement thread-safe progress bar and transfer speed updates via `g_idle_add` in `src/gui/progress_view.c` [0%]
- [ ] 5.6 Implement destructive operation confirmation modals and warning alerts in `src/gui/dialogs.c` [0%]

### 6. Privileged Backend Helper & IPC Subsystem (`flashux-helper`)
- [ ] 6.1 Define binary wire protocol, message headers, and command opcodes in `src/helper/ipc_protocol.h` [0%]
- [ ] 6.2 Implement UNIX domain socket server and event loop in `src/helper/helper_main.c` [0%]
- [ ] 6.3 Implement Polkit peer credential verification (`SO_PEERCRED`) in `src/helper/polkit_auth.c` [0%]
- [ ] 6.4 Implement client-side IPC transport and command dispatch in `src/core/ipc_client.c` [0%]

### 7. Unit Testing & Verification Harness
- [ ] 7.1 Implement core context and logging unit tests in `tests/test_core.c` [0%]
- [ ] 7.2 Implement mock device enumeration and system disk protection tests in `tests/test_device.c` [0%]
- [ ] 7.3 Implement CLI argument parsing and validation tests in `tests/test_cli_args.c` [0%]
- [ ] 7.4 Implement progress calculation and cancellation tests in `tests/test_flasher.c` [0%]
- [ ] 7.5 Verify compiler diagnostics with zero warnings under `-Wall -Wextra -Wpedantic -Werror` [0%]
- [ ] 7.6 Verify zero memory leaks via Valgrind and AddressSanitizer [0%]

---

## Commits in this Branch

| Commit | Summary | Added | Removed / Modified |
| :--- | :--- | :--- | :--- |
| `0e2f289` | docs(initial-implementation): initialize architecture spec and progress tracker | `IMPLEMENTATION.md`, `TRACKER.md` | None |
| `0b09e33` | docs(spec): fix typo in architecture diagram header | None | Corrected diagram header typo in `IMPLEMENTATION.md` |
