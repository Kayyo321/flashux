# Feature: Initial GUI/CLI Application Foundation

**Overall Progress: 97.27%**

## Overview
Establish the foundational build system (Meson + Ninja), the core shared library (`libflashux`), the command-line interface (`flashux-cli`), the GTK 4 / libadwaita graphical interface (`flashux-gui`), and the privileged backend helper daemon (`flashux-helper`) with IPC protocol in pure C (C11).

*Note: Since execution is hosted on macOS where native Linux kernel interfaces (`libudev`, `libblkid`, `/sys/block`, `Polkit`) are unavailable, implementation will focus on modular architecture and mock-safe verification. Per project directives, the tracker will not reach 100% until integration testing on native Linux hardware is completed.*

---

## In-Depth Todo List

### 1. Build System & Scaffolding
- [X] 1.1 Create top-level `meson.build` with C11 standard flags (`-std=c11`, `-Wall`, `-Wextra`, `-Wpedantic`, `-Werror`, `-D_GNU_SOURCE`, `-D_FILE_OFFSET_BITS=64`) [100%]
- [X] 1.2 Configure project dependency probing (`threads`, `glib-2.0`, `gtk4`, `libadwaita-1`, `ncurses`, `udev`, `blkid`, `mount`) [100%]
- [X] 1.3 Configure directory tree targets for `libflashux`, `flashux-cli`, `flashux-gui`, `flashux-helper`, and `tests/` [100%]

### 2. Public API Headers & Data Models
- [X] 2.1 Define core context, logging, and error codes in `include/flashux/core.h` [100%]
- [X] 2.2 Define storage device, partition, bus enumeration, and locking structures in `include/flashux/device.h` [100%]
- [X] 2.3 Define flashing options, stages, performance metrics, and progress callbacks in `include/flashux/flasher.h` [100%]
- [X] 2.4 Define filesystem formatting interfaces and cluster size definitions in `include/flashux/format.h` [100%]
- [X] 2.5 Define MBR and GPT partition table generation abstractions in `include/flashux/partition.h` [100%]
- [X] 2.6 Define multi-threaded cryptographic checksum interfaces in `include/flashux/checksum.h` [100%]

### 3. Core Engine Implementation (`libflashux`)
- [X] 3.1 Implement context lifecycle (`flashux_context_new`, `flashux_context_free`) in `src/core/core.c` [100%]
- [X] 3.2 Implement thread-safe logging and error string translation in `src/core/logging.c` [100%]
- [ ] 3.3 Implement block device probing and sysfs parsing in `src/core/device.c` [95%]
- [ ] 3.4 Implement host system drive protection and mount inspection algorithm in `src/core/device.c` [95%]
- [ ] 3.5 Implement exclusive device locking (`flock`/`O_EXCL`) and partition unmounting in `src/core/mount.c` [90%]
- [ ] 3.6 Implement filesystem formatting dispatchers (FAT, NTFS, exFAT, ext4) in `src/core/format.c` [90%]
- [ ] 3.7 Implement MBR and GPT partition table binary layout synthesis in `src/core/partition.c` [95%]
- [X] 3.8 Implement cryptographic checksum hashing engine in `src/core/checksum.c` [100%]
- [ ] 3.9 Implement flashing options validation and job state machine in `src/core/flasher.c` [95%]
- [X] 3.10 Implement progress math, ETA computation, and moving average speed tracking in `src/core/flasher.c` [100%]

### 4. Command-Line Interface (`flashux-cli`)
- [X] 4.1 Implement POSIX command-line option parser (`getopt_long`) in `src/cli/cli_args.c` [100%]
- [X] 4.2 Implement non-interactive batch mode and structured JSON output in `src/cli/cli_batch.c` [100%]
- [ ] 4.3 Implement interactive curses/ncurses terminal dashboard in `src/cli/cli_interactive.c` [85%]
- [X] 4.4 Implement CLI main entry point, signal handlers (`SIGINT`, `SIGTERM`), and exit routines in `src/cli/main.c` [100%]

### 5. Graphical User Interface (`flashux-gui`)
- [X] 5.1 Implement GtkApplication lifecycle and application initialization in `src/gui/main.c` [100%]
- [X] 5.2 Build Rufus single-window layout (Device, Image, Partition, Target, File System) in `src/gui/app_window.c` [100%]
- [ ] 5.3 Implement dynamic device dropdown population and hotplug signal listeners in `src/gui/device_selector.c` [80%]
- [ ] 5.4 Implement file chooser integration for ISO and raw disk images in `src/gui/app_window.c` [90%]
- [X] 5.5 Implement Rufus Windows user experience customization modal in `src/gui/win_dialog.c` [100%]
- [X] 5.6 Implement thread-safe progress bar and transfer speed updates via `g_idle_add` in `src/gui/progress_view.c` [100%]
- [ ] 5.7 Implement destructive operation confirmation modals and warning alerts in `src/gui/dialogs.c` [95%]

### 6. Privileged Backend Helper & IPC Subsystem (`flashux-helper`)
- [X] 6.1 Define binary wire protocol, message headers, and command opcodes in `src/helper/ipc_protocol.h` [100%]
- [ ] 6.2 Implement UNIX domain socket server and event loop in `src/helper/helper_main.c` [95%]
- [ ] 6.3 Implement Polkit peer credential verification (`SO_PEERCRED`) in `src/helper/polkit_auth.c` [90%]
- [X] 6.4 Implement client-side IPC transport and command dispatch in `src/core/ipc_client.c` [100%]
- [ ] 6.5 Implement privileged write engine and device operations in `src/helper/privileged_ops.c` [90%]

### 7. Unit Testing & Verification Harness
- [X] 7.1 Implement core context and logging unit tests in `tests/test_core.c` [100%]
- [X] 7.2 Implement mock device enumeration and system disk protection tests in `tests/test_device.c` [100%]
- [X] 7.3 Implement CLI argument parsing and validation tests in `tests/test_cli_args.c` [100%]
- [X] 7.4 Implement progress calculation and cancellation tests in `tests/test_flasher.c` [100%]
- [X] 7.5 Implement filesystem formatting parameter and cluster tests in `tests/test_format.c` [100%]
- [X] 7.6 Implement MBR/GPT partition binary layout synthesis tests in `tests/test_partition.c` [100%]
- [X] 7.7 Implement binary IPC header serialization/framing tests in `tests/test_ipc.c` [100%]
- [X] 7.8 Verify compiler diagnostics with zero warnings under `-Wall -Wextra -Wpedantic -Werror` [100%]
- [ ] 7.9 Verify zero memory leaks via Valgrind and AddressSanitizer [95%]

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
| `6a10bfd` | docs(tracker): record define checksum public interface | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `5b51560` | feat(core): implement owned context lifecycle | Context lifecycle, private synchronization state and libflashux target | Meson now builds and installs the core library and headers |
| `300042f` | docs(tracker): record implement owned context lifecycle | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `08f970c` | feat(logging): validate printf arguments at call sites | Printf format attributes and complete logger wrapper Doxygen | Logging declarations now validate format and argument compatibility |
| `986cd5d` | docs(tracker): record validate printf arguments at call sites | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `8d40a3e` | feat(logging): dispatch structured messages outside locks | Severity filtering, bounded logging callbacks and error descriptions | Core build includes the structured logging subsystem |
| `691ae5a` | docs(tracker): record dispatch structured messages outside locks | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `e3eacdd` | feat(device): enumerate Linux disk and partition metadata | udev enumeration, partition metadata ownership and conservative safety filtering | Core target includes device discovery; native Linux paths await integration verification |
| `d764aed` | docs(tracker): record enumerate Linux disk and partition metadata | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `c444428` | fix(device): protect stacked storage slave ancestry | Recursive sysfs slave topology inspection | Replaced descendant enumeration with actual backing-device ancestry traversal |
| `bc61e10` | docs(tracker): record protect stacked storage slave ancestry | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `af33877` | feat(device): lock validated devices and unmount partitions | Exclusive device descriptors, identity validation and libmount unmount dispatcher | Unsupported hosts reject block mutation; Linux runtime verification pending |
| `e34f502` | docs(tracker): record lock validated devices and unmount partitions | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `a3bb9be` | test(core): verify concurrent reentrant logging and ownership | Always-active test assertions and concurrent core regression executable | Meson now runs the core suite on every verification cycle |
| `725d1ab` | docs(tracker): record verify concurrent reentrant logging and ownership | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `0165640` | feat(checksum): stream requested digests on independent workers | Concurrent hashing engine and deterministic hex outputs | Core target links the checksum implementation |
| `b6a37ad` | docs(tracker): record stream requested digests on independent workers | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `1ba6b93` | feat(partition): synthesize aligned MBR and redundant GPT layouts | MBR, GPT, hybrid layouts, CRC32 and bounded descriptor writes | Public partition writes acquire a validated device lock; Linux reread remains unverified |
| `250633f` | docs(tracker): record synthesize aligned MBR and redundant GPT layouts | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `3d10e8e` | test(partition): verify sector layouts and reject overlapping entries | 512-byte and 4Kn file-backed partition regression coverage | Test suite includes independent binary-layout assertions |
| `6a3d03b` | docs(tracker): record verify sector layouts and reject overlapping entries | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `6cbb131` | test(checksum): check standard digests and empty error outputs | Known-answer multihash tests and error-output assertions | Core checksum milestone verified on portable host |
| `7c397c8` | docs(tracker): record check standard digests and empty error outputs | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `18e7259` | feat(format): validate labels and dispatch native filesystem tools | FAT16/FAT32, NTFS, exFAT and ext4 quick-format dispatch and validation | Full zeroing remains unsupported; native formatter verification pending |
| `06143cf` | docs(tracker): record validate labels and dispatch native filesystem tools | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `6aa72b8` | feat(ipc): encode bounded portable wire messages | Versioned header, flash-options and progress codecs with bounded socket transport | IPC structs remain in-memory models; progress doubles are derived at receiver |
| `7289ca0` | docs(tracker): record encode bounded portable wire messages | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `b887083` | feat(ipc): connect authenticated helper clients with timeouts | IPC client lifecycle, command transport and bounded response handling | Core build includes client transport |
| `63a61cf` | docs(tracker): record connect authenticated helper clients with timeouts | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `abba1ee` | feat(progress): calculate bounded throughput and smoothed ETA | Progress metric calculation and monotonic clock abstraction | Metrics use checked counters and bounded ETA |
| `4aebc9d` | docs(tracker): record calculate bounded throughput and smoothed ETA | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `a2bda15` | feat(flasher): define observable asynchronous completion ownership | Doxygen-documented wait and running-query API; join ownership state | Context model now tracks in-progress joins |
| `a342d23` | docs(tracker): record define observable asynchronous completion ownership | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `7c4bfde` | feat(flasher): validate and run cancellable raw image jobs | Flash validation, asynchronous state machine, cancellation and dry-run source reader | Core now orchestrates raw jobs while rejecting unimplemented setup features |
| `a16cbd4` | docs(tracker): record validate and run cancellable raw image jobs | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `c9157b1` | feat(helper): copy aligned raw sectors and verify readback | Aligned raw engine with sparse-file seam, capacity checks and helper device orchestration | Privileged engine built separately from libflashux; Linux block execution pending |
| `3623c7e` | docs(tracker): record copy aligned raw sectors and verify readback | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `f4dfabd` | feat(ipc): pass frontend-opened image descriptors securely | Image descriptor transport with leak-safe ancillary validation | Raw dispatch sends a source descriptor; root socket permissions permit Polkit-gated clients |
| `a1ea742` | docs(tracker): record pass frontend-opened image descriptors securely | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `73db555` | feat(helper): authorize socket peers through Polkit | Linux SO_PEERCRED and Polkit authorization gate | Portable hosts reject privilege authorization explicitly |
| `ce91411` | docs(tracker): record authorize socket peers through Polkit | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `0375315` | feat(helper): serve authorized jobs and cooperative cancellation | Helper executable, command loop, progress streaming and shutdown handling | Meson now installs the privileged helper in libexec |
| `9f31857` | docs(tracker): record serve authorized jobs and cooperative cancellation | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `dfea08a` | feat(cli): define parsed options and explicit confirmation flags | CLI options header with customization and confirmation state | No frontend execution yet |
| `4868846` | docs(tracker): record define parsed options and explicit confirmation flags | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `4302edd` | feat(cli): parse bounded command options and mode constraints | POSIX CLI parser with safe size parsing and shared-option conversion | Meson builds a reusable CLI parser target |
| `722f9a1` | docs(tracker): record parse bounded command options and mode constraints | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `20d4c81` | feat(cli): emit batch progress and device records as JSON | Batch job runner, device listing and JSON output helpers | CLI support targets now include batch execution |
| `57ce8fb` | docs(tracker): record emit batch progress and device records as JSON | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `3fc4246` | feat(cli): present a cancellable ncurses device dashboard | Interactive device chooser, confirmation prompt and progress dashboard | Terminal-only mode fails clearly without a TTY |
| `e05f982` | docs(tracker): record present a cancellable ncurses device dashboard | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `959abdf` | fix(device): use the pkg-config blkid include layout | Verified Linux device translation-unit compilation | Corrected blkid include path |
| `2db4a03` | docs(tracker): record use the pkg-config blkid include layout | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `bf15bf0` | fix(format): check missing Linux sector attributes safely | Linux formatter translation-unit and suite verification | Added stdlib declaration and null-safe sector parsing |
| `0d0205d` | docs(tracker): record check missing Linux sector attributes safely | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `d370b46` | feat(cli): route signals and frontend execution modes | CLI executable and SIGINT/SIGTERM lifecycle handling | Meson installs flashux-cli; help smoke check passes |
| `cadbb54` | docs(tracker): record route signals and frontend execution modes | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `416e23c` | fix(device): fail closed on malformed sysfs geometry | Strict sysfs geometry parser and safe defaults | Enumeration uses checked geometry including 512-byte sysfs unit conversion |
| `5352eb0` | docs(tracker): record fail closed on malformed sysfs geometry | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `0a5225a` | test(device): exercise malformed sysfs and protected snapshots | Mock sysfs geometry and synthetic disk safety regressions | Device tests run without opening real block nodes |
| `6a3970f` | docs(tracker): record exercise malformed sysfs and protected snapshots | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `27ea904` | test(cli): reject truncation overflow and conflicting modes | CLI parser regression tests for boundary and mode handling | Argument parser milestone validated on portable host |
| `4f11d07` | docs(tracker): record reject truncation overflow and conflicting modes | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `fadff34` | test(format): check labels cluster limits and unsupported modes | Filesystem-option boundary regression tests | Validation suite runs independently of external mkfs tools |
| `782a114` | docs(tracker): record check labels cluster limits and unsupported modes | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `394762a` | test(flasher): verify sparse raw copies dry runs and cancellation | Sparse-file raw engine and asynchronous job regression tests | Progress calculations verified without physical disk access |
| `a2df7cf` | docs(tracker): record verify sparse raw copies dry runs and cancellation | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `3a73743` | test(ipc): exercise fragmented framing and descriptor ownership | Socketpair IPC framing and SCM_RIGHTS regression tests | Wire protocol tests require no privileged helper process |
| `de9969b` | docs(tracker): record exercise fragmented framing and descriptor ownership | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `990a533` | feat(gui): establish native application and window lifecycle | GtkApplication entry point, window ownership and GUI build target | All four specified components now have Meson targets |
| `718a873` | docs(tracker): record establish native application and window lifecycle | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `4354497` | feat(gui): build the single-window boot media workflow | Scrollable native boot-media controls with mode-aware sensitivity | Window retains a safe owning reference through application shutdown |
| `39ef568` | docs(tracker): record build the single-window boot media workflow | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `8baa7ee` | feat(gui): populate protected devices and monitor hotplug events | Device dropdown, fixed-drive warning and udev listener | GUI releases monitoring resources and enforces minimum supported toolkit APIs |
| `7da160f` | docs(tracker): record populate protected devices and monitor hotplug events | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `c404966` | feat(gui): select local images through asynchronous GTK dialogs | Async local image chooser and bounded image-entry updates | Select button now populates the boot image control safely |
| `cffc30e` | docs(tracker): record select local images through asynchronous GTK dialogs | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `ece4262` | feat(gui): present session-owned Windows setup preferences | Windows customization modal and owned preference state | GUI exposes saved settings without claiming unsupported media modifications |
| `821cc93` | docs(tracker): record present session-owned Windows setup preferences | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `c496ee6` | feat(gui): coalesce worker progress on the GTK main loop | Idle-dispatched progress, speed and ETA display plus async job completion | Close-time cleanup removes UI sources after joining the worker |
| `1181306` | docs(tracker): record coalesce worker progress on the GTK main loop | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `45fce38` | feat(gui): confirm an immutable reviewed job before erasing | Reviewed-job confirmation, validation errors and cancellation controls | GUI start actions now use the shared asynchronous engine; modal allocations fail safely |
| `c203bbd` | docs(tracker): record confirm an immutable reviewed job before erasing | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `c112bfa` | chore(integration): install Polkit policy and desktop entry | Polkit action policy and native desktop launcher | Meson installation includes Linux authorization assets and GUI desktop entry |
| `84199e6` | docs(tracker): record install Polkit policy and desktop entry | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `f5e22d0` | feat(helper): launch the installed backend through pkexec | Polkit helper launcher and child process reaping | Actual frontend jobs can request privilege authorization automatically |
| `daf655f` | docs(tracker): record launch the installed backend through pkexec | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `afef781` | fix(safety): retain kernel disk identity across confirmation and IPC | Expected disk sequence token and helper descriptor identity validation | Raw jobs reject replacement devices before any writes |
| `f11edbf` | docs(tracker): record retain kernel disk identity across confirmation and IPC | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `b96f9f6` | fix(build): isolate toolkit headers from application diagnostics | System include treatment for toolkit dependencies | Linux GCC GUI compilation now respects application warning policy |
| `e0ca5f3` | docs(tracker): record isolate toolkit headers from application diagnostics | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `3b23027` | refactor(safety): make backing-disk topology independently testable | Portable sysfs ancestry traversal with bounded recursion | System-drive checks use explicit device-number topology rather than opaque udev traversal |
| `1c6c5c4` | docs(tracker): record make backing-disk topology independently testable | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `acfd0a7` | test(safety): cover partition mapper and cyclic backing graphs | Mock system-drive ancestry tests including stacked devices and cycles | Device safety suite now verifies the actual topology algorithm |
| `ad1417c` | docs(tracker): record cover partition mapper and cyclic backing graphs | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `7fc5360` | refactor(gui): share window components with runtime verification | GUI component library and declared toolkit baseline dependency | GUI executable retains the same application entry point |
| `0a9b5d3` | docs(tracker): record share window components with runtime verification | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `cf20b6a` | test(gui): verify dry-run completion protected selection and close | Linux GUI runtime and shutdown regression executable | Virtual-display test confirms native window lifecycle without physical disk writes |
| `7d8ae4b` | docs(tracker): record verify dry-run completion protected selection and close | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `378d90f` | fix(gui): keep progress and action controls visible at normal sizes | Compact paired option rows and persistent action footer | Default window now keeps operational controls visible |
| `ba8d69f` | docs(tracker): record keep progress and action controls visible at normal sizes | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `dc27230` | fix(safety): revalidate protection and identity before unmounting | Fresh unmount safety validation and bounded device-path checks | Replacement devices are rejected before partition unmounting |
| `88547cf` | docs(tracker): record revalidate protection and identity before unmounting | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `cb5582e` | fix(helper): distinguish locked and reviewed disk sequence variables | Verified Linux sequence validation compilation | Renamed the inner locked disk sequence variable |
| `01d6c3e` | docs(tracker): record distinguish locked and reviewed disk sequence variables | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `801e6f6` | test(gui): release font backend caches for leak verification | Explicit test backend teardown and font dependency declarations | All nine Linux ASan/UBSan suites pass with leak detection enabled |
| `79a3ddb` | docs(tracker): record release font backend caches for leak verification | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `e906d82` | chore(workspace): ignore local editor and Finder metadata | IDE and Finder ignore patterns | Local metadata is excluded from source control |
| `41e095b` | docs(tracker): record ignore local editor and Finder metadata | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `0f90e2b` | feat(image): decode bounded compressed image streams | Native bounded image decoder and cancellable decoded-capacity preflight | Core build now requires native libarchive with built-in compression filters |
| `7472610` | docs(tracker): record decode bounded compressed image streams | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `ecdc0d7` | feat(helper): write and verify decoded image bytes | Decoded-capacity preflight and streaming compressed raw writes with full readback | Raw engine progress now reflects decoded bytes and resets metrics during verification |
| `d7fec17` | docs(tracker): record write and verify decoded image bytes | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `c82a62d` | feat(flasher): preflight and decode compressed dry runs | Cancellable compressed-image dry runs with decoded progress | Raw option validation delegates decoded capacity to the worker and helper |
| `5f2bbbc` | docs(tracker): record preflight and decode compressed dry runs | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `5765a69` | test(image): verify compression preflight and decoded readback | Cross-platform image stream and raw-copy regression suite | Test matrix now exercises five compressed image formats and pre-write failure safety |
| `2402e53` | docs(tracker): record verify compression preflight and decoded readback | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `9d81830` | feat(format): zero validated targets before full formatting | Cancellable full-format zero pass and sparse-file zeroing regression | Formatting accepts quick or full requests while retaining parent-disk safety and fixed argv execution |
| `52a6cd3` | docs(tracker): record zero validated targets before full formatting | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `97b78fe` | fix(partition): validate block geometry and GPT UUID encoding | Opened-descriptor capacity and sector validation with UUID regressions | Corrected GPT UUID byte order and filesystem-aware default partition type |
| `d6ad53f` | docs(tracker): record validate block geometry and GPT UUID encoding | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `95b33e5` | feat(device): inspect mounted partitions and classify storage buses | libmount partition metadata and complete storage bus classification | System-path inspection fails closed and sysfs numbers reject overflow |
| `8e5513f` | docs(tracker): record inspect mounted partitions and classify storage buses | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `b5fd714` | fix(mount): match unmount targets by kernel device identity | Kernel identity comparison for every target and child partition mount | Removed reliance on mount source path equality |
| `e51da1c` | docs(tracker): record match unmount targets by kernel device identity | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `d5ef093` | fix(image): detect source changes with nanosecond precision | Portable source identity helper for macOS and Linux timestamps | Replaced second-resolution source-change checks and added pre-write stability verification |
| `d86fab4` | docs(tracker): record detect source changes with nanosecond precision | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `f3a436a` | feat(frontend): expose supported compressed image inputs | Compressed-image chooser patterns and CLI usage guidance | Removed obsolete claim that all compressed images are unsupported |
| `cb1b0e2` | docs(tracker): record expose supported compressed image inputs | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `2c05c94` | fix(gui): retain fixed-drive warning parent through response | Owned parent reference and parent-destruction behavior for fixed-drive warnings | Fixed-drive dialog callbacks no longer rely on an unowned window pointer |
| `e907f9d` | docs(tracker): record retain fixed-drive warning parent through response | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `8986d66` | test(gui): exercise native modal state and warning cancellation | Native modal cancellation and customization regressions | Expanded GUI runtime suite covers modal ownership and session preference persistence |
| `0b7670d` | docs(tracker): record exercise native modal state and warning cancellation | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `fb91c62` | fix(ipc): preserve smoothed transfer speed in progress frames | Endian-independent speed field and malformed floating-point regression | Remote frontend progress retains the engine moving average instead of substituting average speed |
| `89c2cfb` | docs(tracker): record preserve smoothed transfer speed in progress frames | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `1d66922` | fix(cli): preserve valid Unicode in batch JSON records | Bounded UTF-8 validation for structured record strings | JSON output no longer reinterprets each UTF-8 byte as a separate codepoint |
| `88631a5` | docs(tracker): record preserve valid Unicode in batch JSON records | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `c3dce27` | test(cli): verify batch records and dry-run target preservation | Batch runner regression suite for JSON and confirmation safeguards | Batch implementation is now exercised independently of argument parsing |
| `328412d` | docs(tracker): record verify batch records and dry-run target preservation | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `131480d` | test(ipc): reject incomplete frames and missing descriptors safely | Malformed transport and descriptor lifecycle regressions | Framing verification now includes disconnect, capacity and descriptor boundary cases |
| `6e0923d` | docs(tracker): record reject incomplete frames and missing descriptors safely | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `eb27746` | chore(style): codify repository C indentation and column limits | clang-format and EditorConfig settings matching agent guidelines | Source formatting can now be applied consistently across all C modules |
| `6b288a9` | docs(tracker): record codify repository C indentation and column limits | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `8679c27` | refactor(core): apply mandated C layout to core sources | Consistent repository C layout in src/core | Normalized braces, indentation, declaration wrapping and comments without changing behavior |
| `3b640f1` | docs(tracker): record apply mandated C layout to core sources | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `4bff4bf` | refactor(cli): apply mandated C layout to cli sources | Consistent repository C layout in src/cli | Normalized braces, indentation, declaration wrapping and comments without changing behavior |
| `618291d` | docs(tracker): record apply mandated C layout to cli sources | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `b879dd0` | refactor(gui): apply mandated C layout to gui sources | Consistent repository C layout in src/gui | Normalized braces, indentation, declaration wrapping and comments without changing behavior |
| `6704729` | docs(tracker): record apply mandated C layout to gui sources | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `9f1e756` | refactor(helper): apply mandated C layout to helper sources | Consistent repository C layout in src/helper | Normalized braces, indentation, declaration wrapping and comments without changing behavior |
| `d4488b6` | docs(tracker): record apply mandated C layout to helper sources | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `1e6d411` | refactor(api): apply mandated C layout to api sources | Consistent repository C layout in include | Normalized braces, indentation, declaration wrapping and comments without changing behavior |
| `61352aa` | docs(tracker): record apply mandated C layout to api sources | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `7b5e0b6` | refactor(wire): apply mandated C layout to wire sources | Consistent repository C layout in src/common | Normalized braces, indentation, declaration wrapping and comments without changing behavior |
| `c977f6e` | docs(tracker): record apply mandated C layout to wire sources | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `e568916` | refactor(tests): apply mandated C layout to tests sources | Consistent repository C layout in tests | Normalized braces, indentation, declaration wrapping and comments without changing behavior |
| `06e9473` | docs(tracker): record apply mandated C layout to tests sources | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `bb09eaf` | chore(build): retain strict diagnostics through Meson warning options | Meson-native strict diagnostic configuration | Removed duplicate warning switches from add_project_arguments |
| `714891b` | docs(tracker): record retain strict diagnostics through Meson warning options | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `74cf461` | chore(build): install core library pkg-config metadata | Installable pkg-config metadata for libflashux | Core library installs now expose reproducible consumer compilation flags |
| `4e93468` | docs(tracker): record install core library pkg-config metadata | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `9942985` | test(helper): probe real Polkit peer authorization without writes | Nondestructive helper authorization smoke executable | Verified real helper allow and deny paths; active desktop authentication remains pending |
| `67eb4e9` | docs(tracker): record probe real Polkit peer authorization without writes | Detailed audit entries for every preceding branch commit | Incremental milestone percentages and overall completion |
| `7f7e54f` | chore(tests): build the manual Linux authorization probe | Linux test_helper build target | Default unit tests remain independent of root credentials and a running Polkit daemon |
| `HEAD` | docs(tracker): record build the manual Linux authorization probe | Audit entry for preceding implementation commit | Incremental progress and exhaustive branch history |
