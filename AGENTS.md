# Agent Operational Guidelines for Flashux

This document outlines mandatory protocols, operational standards, coding practices, and commit workflows for all autonomous AI agents and automated coding systems contributing to the **Flashux** repository.

Every agent operating within this codebase **MUST** read, understand, and strictly follow every directive in this document, in addition to [PROJECT.md](file:///Users/jessicabruce/flashux/PROJECT.md) and [WORKFLOW.md](file:///Users/jessicabruce/flashux/WORKFLOW.md).

---

## 1. Prime Directives & Non-Negotiable Invariants

1. **Pure C Only (C11)**:
   - All application code (core library, CLI, GUI, and privileged helpers) **must be written exclusively in C** (C11 standard).
   - Under no circumstances may an agent introduce C++, Python, Rust, Go, or other scripting languages into the core application runtime. Shell scripts are strictly limited to build-time or packaging helpers if absolutely necessary.
2. **Strict Feature Branching**:
   - Direct commits to the primary branch (`origin`) are **strictly prohibited**.
   - Every feature must be developed in its own feature branch named `feature/<feature-name>`.
   - Every completed feature must be submitted as a pull request to `origin`.
3. **Mandatory Feature Documentation**:
   - Every feature branch must contain a dedicated directory: `feature/<feature-name>/`.
   - The directory must contain exactly two living documents:
     - `IMPLEMENTATION.md`: A comprehensive, in-depth architectural and technical description of what the feature implements.
     - `TRACKER.md`: A granular progress tracker containing the feature name, overall percentage completion, an itemized todo list with incremental percentage tracking per item, and an exhaustive commit history detailing additions and deletions.
4. **Micro-Commit Discipline**:
   - Every single atomic change must be accompanied by its own git commit.
   - Batch commits combining unrelated changes or multiple todo steps are forbidden.
   - Every commit must include a detailed commit message describing what was added, removed, or modified, and the technical rationale.
5. **Host Disk Safety**:
   - Flashux interacts directly with raw block devices. Agents must **never** run commands or tests that write to host physical disks (`/dev/sd*`, `/dev/nvme*`).
   - All destructive and block-level testing must be performed against loop devices (`losetup`), ramdisks, or sparse image files.

---

## 2. C Programming & Code Quality Standards

Flashux is a system-level utility performing low-level I/O. Agents must adhere to the highest standard of defensive C programming.

### 2.1. Language Standard & Compiler Directives
- **Standard**: ISO C11 (`-std=c11`).
- **Required Compiler Flags**:
  ```text
  -Wall -Wextra -Wpedantic -Werror -Wformat=2 -Wconversion -Wshadow 
  -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes 
  -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64
  ```
- **Large File Support**: Always define `_FILE_OFFSET_BITS=64` to ensure 64-bit `off_t` and `lseek64` compatibility on 32-bit and 64-bit systems.

### 2.2. Memory Safety & Resource Management
- **Explicit Allocations**: Every `malloc`, `calloc`, or `realloc` must check for `NULL` return values immediately.
- **Immediate Free & Nullify**: Every allocated pointer must have a clearly defined owner and lifecycle. Once freed, pointers must be set to `NULL` to prevent use-after-free bugs:
  ```c
  free(buffer);
  buffer = NULL;
  ```
- **Zero Memory Leaks**: All code must pass cleanly under Valgrind (`valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes`) and Clang/GCC AddressSanitizer (`-fsanitize=address,undefined`).
- **Unsafe String Functions Banned**:
  - `strcpy`, `strcat`, `sprintf`, `gets` are **strictly forbidden**.
  - Use `snprintf`, bounded `strlcpy`/`strncpy` with explicit null-termination, or custom safe string builders.

### 2.3. Error Handling & Return Codes
- Functions must return predictable, typed status codes.
- Use explicit error enums (e.g., `flashux_error_t`) or standard POSIX integer conventions (`0` on success, negative value or `-1` with `errno` on failure).
- Error messages must be structured and routed through the Flashux logging subsystem (`flashux_log_error()`, `flashux_log_warn()`).

### 2.4. Coding Style
- Follow Linux Kernel / GNU indentation:
  - 4 spaces indentation (or 8-space tabs conforming to `.editorconfig`).
  - Braces: Opening brace on same line for control statements (`if`, `while`, `for`); opening brace on new line for functions.
  - Column limit: 100 characters.
  - Header guards: `#ifndef FLASHUX_<MODULE>_H`, `#define FLASHUX_<MODULE>_H`, `#endif`.
  - Full Doxygen comments (`/** ... */`) for all public API functions in `include/flashux/*.h`.

---

## 3. Agent Lifecycle for Implementing Features

Whenever an agent is tasked with implementing a feature or bugfix, it **MUST** follow this exact execution sequence:

```
+--------------------------------------------------------------------+
| 1. Switch to branch: feature/<feature-name>                        |
+---------------------------------+----------------------------------+
                                  |
                                  v
+--------------------------------------------------------------------+
| 2. Create feature directory: feature/<feature-name>/               |
|    - Create IMPLEMENTATION.md (In-depth design & technical spec)   |
|    - Create TRACKER.md (Todos with incremental %, commit log)      |
+---------------------------------+----------------------------------+
                                  |
                                  v
+--------------------------------------------------------------------+
| 3. Initial Commit: Add feature documentation                       |
|    - Commit feature/<feature-name>/ files                          |
+---------------------------------+----------------------------------+
                                  |
                                  v
+--------------------------------------------------------------------+
| 4. Iterative Implementation Loop (Per Small Atomic Change):        |
|    a. Implement single code change / unit test                     |
|    b. Compile & verify (`meson compile -C build`, run tests)       |
|    c. Create atomic Git commit with detailed message               |
|    d. Update TRACKER.md (recalculate % done, append commit log)    |
|    e. Commit updated TRACKER.md                                    |
+---------------------------------+----------------------------------+
                                  |
                                  v
+--------------------------------------------------------------------+
| 5. Final Verification & Pull Request:                              |
|    - Verify full build, zero warnings, tests pass, memory clean    |
|    - Set TRACKER.md to 100% completed                              |
|    - Submit Pull Request to origin                                 |
+--------------------------------------------------------------------+
```

---

## 4. Feature Documentation Specifications

### 4.1. `IMPLEMENTATION.md` Requirements
The `IMPLEMENTATION.md` file must provide an exhaustive blueprint of the feature. It cannot be a vague summary. It must include:
1. **Feature Title & Purpose**: Clear statement of what problem the feature solves.
2. **Architecture & Design**: How the feature fits into `libflashux`, CLI, or GUI.
3. **Data Structures & Types**: Detailed definitions of `struct`, `enum`, and typedefs.
4. **API Function Signatures**: Public and internal function prototypes with input/output parameter documentation.
5. **System APIs & Algorithms**: Low-level Linux kernel system calls, `ioctl` flags, or library routines utilized (`libudev`, `libblkid`, `libwim`, etc.).
6. **Error Handling & Failure Modes**: Potential failure scenarios (device removal during write, permission denied, invalid superblock) and recovery strategies.
7. **Security & Boundary Considerations**: Prevention of buffer overflows, drive selection safeguards, and privilege constraints.

### 4.2. `TRACKER.md` Requirements
The `TRACKER.md` file is a living document that must be maintained with mathematical precision:
1. **Header**:
   - Feature Name.
   - Overall Progress: Exact percentage completed (`XX%`).
2. **In-Depth Todo List**:
   - Broken down into logical milestones or functional components.
   - **Every single todo item must have an incremental percentage** representing its current progress (e.g., `[45%]`, `[100%]`).
   - Use checkboxes (`[ ]` for incomplete, `[X]` for 100% complete).
3. **Commit Log**:
   - Located at the bottom of the document under `## Commits in this Branch`.
   - Updated **after every commit**.
   - Must list the short commit hash, commit title, and detailed itemized bullets of what was added and removed.

---

## 5. Commit Standards & Message Formatting

Every change—no matter how small—requires its own commit. Trivial or combined commits are unacceptable.

### Commit Message Structure
```text
<type>(<scope>): <short imperative summary>

<Detailed body paragraph explaining why this change was made, what problem
it solves, and the technical context.>

Added:
- <Specific struct, function, file, or capability added>
- <Additional item added>

Removed / Modified:
- <Specific item removed, refactored, or fixed>
- <Behavioral adjustments>

Ref: feature/<feature-name>
```

### Allowed Commit Types
- `feat`: A new user-facing or library feature.
- `fix`: A bug fix.
- `docs`: Documentation only changes (e.g., updating `TRACKER.md` or `IMPLEMENTATION.md`).
- `refactor`: A code change that neither fixes a bug nor adds a feature.
- `test`: Adding missing tests or correcting existing tests.
- `chore`: Changes to the build process, dependencies, or auxiliary tools.

---

## 6. Pull Request Protocol

When all tasks in `TRACKER.md` reach 100%:
1. Ensure the working tree is completely clean and all changes are committed.
2. Confirm the build compiles with zero warnings under `-Werror`.
3. Verify all unit and integration tests pass.
4. Open a pull request against the `origin` branch.
5. The pull request description must include:
   - Summary of the feature.
   - Link to `feature/<feature-name>/IMPLEMENTATION.md`.
   - Link to `feature/<feature-name>/TRACKER.md`.
   - Complete list of commits included in the PR.
