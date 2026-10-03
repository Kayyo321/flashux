# Flashux Development Workflow

This document establishes the official development workflow for **Flashux**. Every developer and automated agent contributing to this repository must follow this workflow without exception.

---

## 1. Core Workflow Overview

The Flashux workflow is founded on three immutable principles:

1. **Micro-Commits with Detailed Messages**: Every small change—whether a new struct, a helper function, a bug fix, or a test—must have its own dedicated commit with an in-depth explanatory message.
2. **Dedicated Feature Branches & Pull Requests**: Every feature lives on its own branch named `feature/<feature-name>` branched from `origin`, and is merged back into `origin` exclusively via a Pull Request.
3. **Mandatory Feature Folders & In-Depth Tracking**: Every feature branch contains a corresponding directory `feature/<feature-name>/` containing two living documents:
   - `IMPLEMENTATION.md`: An exhaustive architectural and technical design document.
   - `TRACKER.md`: A live progress tracker containing the feature name, overall percentage completion, an in-depth todo list with incremental tracking percentages for every item, and a complete commit log at the bottom recording what each commit added and removed.

```
                           PRIMARY BRANCH: origin
                                     |
                                     | (git checkout -b feature/<feature-name>)
                                     v
                        FEATURE BRANCH: feature/<feature-name>
                                     |
         +---------------------------+---------------------------+
         |                                                       |
         v                                                       v
+-------------------------------+               +---------------------------------+
| feature/<feature-name>/       |               | src/ & include/                 |
|   ├── IMPLEMENTATION.md       |               |   ├── Code & tests              |
|   └── TRACKER.md              |               |   └── Micro-commits with        |
|       - Name of feature       |               |       detailed commit messages  |
|       - % done                |               +---------------------------------+
|       - Todo list (with % ea) |                                |
|       - Commit log (bottom)   |                                |
+-------------------------------+                                |
         |                                                       |
         +---------------------------+---------------------------+
                                     |
                                     | (Tracker reaches 100%, tests pass)
                                     v
                          PULL REQUEST TO origin
```

---

## 2. Step-by-Step Feature Workflow

### Step 1: Create the Feature Branch
All features must branch from the latest state of `origin`.

```bash
# Ensure local origin branch is up to date
git checkout origin
git pull origin origin

# Create and switch to your feature branch
git checkout -b feature/<feature-name>
```

> **Naming Rule**: Feature branch names must begin with `feature/`, followed by lowercase alphanumeric words separated by hyphens (e.g., `feature/udev-device-discovery`, `feature/gpt-partition-table`, `feature/windows-bypass-tpm`).

---

### Step 2: Initialize the Feature Documentation Directory
Immediately after creating the branch, create the feature folder and the two required files:

```bash
# Create feature documentation directory
mkdir -p feature/<feature-name>

# Create the two required documents
touch feature/<feature-name>/IMPLEMENTATION.md
touch feature/<feature-name>/TRACKER.md
```

> **Path Rule**: The subfolder inside `feature/` **must exactly match** the branch name. For example, if the branch is `feature/device-discovery`, the directory must be `feature/device-discovery/`.

---

### Step 3: Author `IMPLEMENTATION.md`
Before writing production C code, write a comprehensive technical blueprint in `IMPLEMENTATION.md`. It must not be a trivial summary; it must thoroughly detail the architecture, data structures, and system interactions.

#### Required Sections in `IMPLEMENTATION.md`:
1. **Feature Overview & Purpose**: What problem this feature solves and its role in Rufus parity.
2. **Architecture & Module Boundaries**: Where this code lives (e.g., `libflashux/device`, `flashux-cli`, `flashux-gui`), header files exposed, and interaction with other components.
3. **Data Structures & Types**: Full C struct, enum, and typedef definitions with field descriptions.
4. **Function Prototypes & Interfaces**: Function signatures, parameters, return codes, and memory ownership rules.
5. **System Calls & External Libraries**: Specific Linux system calls (`ioctl`, `open`, `flock`, `mount`) and external libraries (`libudev`, `libblkid`, `libwim`) utilized.
6. **Control Flow & State Transitions**: Step-by-step algorithms, state machines, or sequence diagrams.
7. **Error Handling & Edge Cases**: Hardware unplug events, read-only media, disk full conditions, permission errors, and memory exhaustion.
8. **Testing & Verification**: Unit testing plan using loop devices, memory sanitizers, and mock structures.

---

### Step 4: Author `TRACKER.md`
The `TRACKER.md` file tracks progress incrementally and maintains an audit log of every commit.

#### Exact Required Format for `TRACKER.md`:

```markdown
# Feature: <Feature Name>

**Overall Progress: 0%**

## Overview
Brief 1-2 sentence description of the feature goal.

---

## In-Depth Todo List

### 1. Architectural Design & Scaffolding
- [ ] 1.1 Define public API header in `include/flashux/<module>.h` [0%]
- [ ] 1.2 Set up internal types and structs [0%]
- [ ] 1.3 Add module build target to `meson.build` [0%]

### 2. Core Implementation
- [ ] 2.1 Implement core initialization function [0%]
- [ ] 2.2 Implement primary processing logic [0%]
- [ ] 2.3 Implement error handling and cleanup routines [0%]

### 3. Testing & Verification
- [ ] 3.1 Write unit tests with mock structures [0%]
- [ ] 3.2 Verify clean execution under Valgrind (zero leaks) [0%]
- [ ] 3.3 Verify zero compiler warnings with `-Wall -Wextra -Werror` [0%]

---

## Commit History

| Commit | Summary | Added | Removed / Modified |
| :--- | :--- | :--- | :--- |
| `<hash>` | Initial documentation scaffolding | `IMPLEMENTATION.md`, `TRACKER.md` | None |
```

#### Guidelines for Progress Percentages:
- **Individual Todo %**: Every item has its own progress percentage (e.g., `[0%]`, `[25%]`, `[50%]`, `[75%]`, `[100%]`).
- **Overall Progress %**: Calculate the overall percentage as the arithmetic average or weighted sum of all individual items:
  $$\text{Overall } \% = \frac{\sum \text{Todo } \%}{\text{Total Number of Todos}}$$

---

### Step 5: Initial Documentation Commit
Commit the newly scaffolded documentation files before implementing code:

```bash
git add feature/<feature-name>/
git commit -m "docs(<feature-name>): initialize implementation spec and progress tracker

Create initial architecture document (IMPLEMENTATION.md) and task
progress tracker (TRACKER.md) for feature/<feature-name>.

Added:
- feature/<feature-name>/IMPLEMENTATION.md: detailed technical specification
- feature/<feature-name>/TRACKER.md: incremental todo tracker and commit log

Ref: feature/<feature-name>"
```

Update `TRACKER.md` with this first commit hash in the commit table.

---

### Step 6: Micro-Commit Implementation Cycle

Development proceeds in small, atomic, verifiable steps.

#### What Constitutes a Micro-Commit?
Do not bundle multiple changes into one commit. Create a commit whenever you:
- Add a new header file or data structure.
- Implement a single function or helper routine.
- Add input validation or error recovery to a routine.
- Write or update a unit test.
- Fix a compiler warning or memory leak.
- Refactor a routine for readability or performance.

#### The Micro-Commit Loop:
```text
                  +-----------------------------------+
                  | 1. Write small, focused C change  |
                  +-----------------+-----------------+
                                    |
                                    v
                  +-----------------------------------+
                  | 2. Compile & verify (`meson test`)|
                  +-----------------+-----------------+
                                    |
                                    v
                  +-----------------------------------+
                  | 3. Git commit with detailed body  |
                  +-----------------+-----------------+
                                    |
                                    v
                  +-----------------------------------+
                  | 4. Update TRACKER.md              |
                  |    - Recalculate % for item       |
                  |    - Recalculate overall %        |
                  |    - Append commit to history     |
                  +-----------------+-----------------+
                                    |
                                    v
                  +-----------------------------------+
                  | 5. Git commit updated TRACKER.md  |
                  +-----------------------------------+
```

#### Detailed Commit Message Format
Every commit message must follow this structure:

```text
<type>(<scope>): <concise imperative subject line (<= 72 chars)>

<Detailed paragraph explaining the context, the rationale for this change,
technical design choices, and how it interacts with the rest of the system.>

Added:
- <Itemized bullet of new functions, structs, parameters, or files>
- <Additional additions>

Removed / Modified:
- <Itemized bullet of deleted, replaced, or refactored code>
- <Changes to function behavior, signatures, or error returns>

Ref: feature/<feature-name>
```

---

### Step 7: Final Verification & Pull Request to Origin

When all tasks in `TRACKER.md` reach `[100%]` and the overall progress is `100%`:

1. **Perform Full Verification**:
   ```bash
   # 1. Clean build with strict warning flags
   meson setup build --wipe
   ninja -C build

   # 2. Run all unit and integration tests
   ninja -C build test

   # 3. Verify zero memory leaks
   valgrind --leak-check=full --show-leak-kinds=all ./build/tests/test_all

   # 4. Check git status to ensure working tree is clean
   git status
   ```

2. **Push the Feature Branch**:
   ```bash
   git push -u origin feature/<feature-name>
   ```

3. **Open Pull Request Targeting `origin`**:
   - Title: `feat(<feature-name>): <Feature Title>`
   - Description must contain:
     - High-level overview of the feature.
     - Link to `feature/<feature-name>/IMPLEMENTATION.md`.
     - Link to `feature/<feature-name>/TRACKER.md`.
     - Summary table of all commits included in the PR.
     - Checklist confirming all tests pass and Valgrind is clean.

---

## 3. Concrete Example Walkthrough

Below is a complete, real-world example illustrating this workflow in action for a feature named `feature/block-device-probing`.

### Example 1: `feature/block-device-probing/IMPLEMENTATION.md`

```markdown
# Implementation: Block Device Probing Subsystem

## 1. Overview
Implements block device enumeration and hardware probing using `libudev` and `/sys/block`. 
Identifies USB drives, SD cards, and external storage while safely isolating internal host disks.

## 2. Architecture & File Layout
- `include/flashux/device.h`: Public API exposing device structs and scan routines.
- `src/core/device.c`: Implementation interfacing with `udev_enumerate` and `libblkid`.
- `tests/test_device.c`: Unit tests with mock sysfs attributes and loop devices.

## 3. Data Structures
```c
typedef enum {
    FLASHUX_BUS_USB,
    FLASHUX_BUS_SD_MMC,
    FLASHUX_BUS_SCSI_SATA,
    FLASHUX_BUS_NVME,
    FLASHUX_BUS_VIRTUAL
} flashux_bus_type_t;

typedef struct flashux_device {
    char node_path[64];         // e.g., "/dev/sdb"
    char sys_path[256];         // /sys/devices/...
    char vendor[64];            // e.g., "SanDisk"
    char model[64];             // e.g., "Ultra USB 3.0"
    char serial[128];
    uint64_t size_bytes;        // Total capacity
    uint32_t logical_sector_sz; // 512 or 4096
    uint32_t phys_sector_sz;    // 512 or 4096
    bool is_removable;          // 1 if removable
    bool is_system_drive;       // 1 if contains mounted root or boot
    flashux_bus_type_t bus_type;
    struct flashux_device *next;
} flashux_device_t;
```

## 4. Function Prototypes
```c
int flashux_device_scan(flashux_device_t **device_list_out);
void flashux_device_list_free(flashux_device_t *device_list);
bool flashux_device_is_safe_target(const flashux_device_t *device);
```

## 5. Kernel & System Interfaces
- `udev_new()`, `udev_enumerate_new()` to scan block subsystem.
- `ioctl(fd, BLKGETSIZE64, &size)` to query sector-accurate device size.
- `ioctl(fd, BLKSSZGET, &sector_size)` to detect 4Kn vs 512e drives.
- Inspect `/proc/mounts` and `/proc/swaps` to mark `is_system_drive = true`.

## 6. Error Handling
- Return `-EACCES` when unable to access block nodes.
- Return `-ENODEV` if no candidate removable devices exist.
- Graceful `NULL` checks on all udev property queries.
```

---

### Example 2: `feature/block-device-probing/TRACKER.md`

```markdown
# Feature: Block Device Probing Subsystem

**Overall Progress: 100%**

## Overview
Implements hardware enumeration of removable storage media via `libudev` and safe system drive filtering.

---

## In-Depth Todo List

### 1. Specification & Interfaces
- [X] 1.1 Define `flashux_device_t` and `flashux_bus_type_t` in `device.h` [100%]
- [X] 1.2 Define `flashux_device_scan()` and `flashux_device_list_free()` [100%]
- [X] 1.3 Add `libudev` and `libblkid` dependencies to `meson.build` [100%]

### 2. Core Implementation
- [X] 2.1 Set up `udev_enumerate` subsystem scanner in `src/core/device.c` [100%]
- [X] 2.2 Query vendor, model, and serial string descriptors [100%]
- [X] 2.3 Query capacity and sector geometry via `ioctl(BLKGETSIZE64)` [100%]
- [X] 2.4 Parse `/proc/mounts` to identify and protect host system drives [100%]
- [X] 2.5 Implement memory cleanup in `flashux_device_list_free()` [100%]

### 3. Testing & Verification
- [X] 3.1 Create unit tests for parsing sysfs strings [100%]
- [X] 3.2 Verify clean memory execution under Valgrind (0 bytes lost) [100%]
- [X] 3.3 Verify compilation with zero warnings on GCC and Clang [100%]

---

## Commits in this Branch

| Commit | Summary | Added | Removed / Modified |
| :--- | :--- | :--- | :--- |
| `a1b2c3d` | docs(device): initialize feature spec and tracker | `IMPLEMENTATION.md`, `TRACKER.md` | None |
| `f4e5d6c` | feat(device): define device header and public types | `include/flashux/device.h` | Updated `meson.build` |
| `9a8b7c6` | feat(device): implement udev device scanner skeleton | `src/core/device.c` (udev scanner) | None |
| `3d2e1f0` | feat(device): add geometry and sector size queries | `ioctl(BLKGETSIZE64)` in `device.c` | Fixed sector calculation |
| `7b6c5d4` | feat(device): add system drive protection check | `/proc/mounts` parser in `device.c` | Protected root device |
| `4c5b6a7` | test(device): add unit tests and Valgrind clean pass | `tests/test_device.c` | Fixed memory leak in free |
| `1e2f3a4` | docs(device): finalize tracker to 100% completion | Marked all tasks 100% | Finalized tracker |
```

---

### Example 3: Micro-Commit Messages in this Flow

#### Commit 1: Header Definition
```text
feat(device): define device structs and public scanning API

Create include/flashux/device.h with definitions for flashux_device_t,
flashux_bus_type_t, and public function declarations for discovering
and freeing block devices.

Added:
- include/flashux/device.h: core device structure definitions
- flashux_device_scan() prototype
- flashux_device_list_free() prototype

Removed / Modified:
- meson.build: added include/ directory to include_directories

Ref: feature/block-device-probing
```

#### Commit 2: Sector Size and Geometry Query
```text
feat(device): query capacity and sector size via ioctl

Add low-level ioctl calls BLKGETSIZE64 and BLKSSZGET in src/core/device.c
to retrieve accurate byte capacity and distinguish 512-byte logical sectors
from 4Kn (4096-byte native Advanced Format) drives.

Added:
- query_device_geometry() helper in src/core/device.c
- ioctl BLKSSZGET logical sector query

Removed / Modified:
- src/core/device.c: replaced hardcoded 512 sector size with queried value

Ref: feature/block-device-probing
```

---

## 4. Enforcement & Review Checklist

Before any PR will be approved and merged into `origin`, reviewers must verify:
- [ ] Feature branch was created off `origin` and PR targets `origin`.
- [ ] Directory `feature/<branch-name>/` exists and contains both `IMPLEMENTATION.md` and `TRACKER.md`.
- [ ] `IMPLEMENTATION.md` provides an in-depth technical specification (not an empty placeholder).
- [ ] `TRACKER.md` is at 100% completion, with each todo displaying an individual progress percentage.
- [ ] The bottom commit history in `TRACKER.md` lists every commit in the branch and accurately describes what each commit added and removed.
- [ ] Every commit in the git history is a micro-commit with a detailed commit message.
- [ ] All code is 100% pure C (C11 standard).
- [ ] No compiler warnings (`-Wall -Wextra -Werror`).
- [ ] Valgrind reports zero memory leaks and zero invalid reads/writes.
