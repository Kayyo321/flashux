# Flashux Project Specification

## 1. Project Overview & Vision

**Flashux** is an easy-to-use, high-performance Linux command-line (CLI) and graphical (GUI) application engineered specifically for creating bootable media and flashing disk images.

While Windows users have long relied on **Rufus** as the gold standard for creating bootable USB media, Linux users have historically faced fragmented alternatives: raw block copy tools (`dd`, Balena Etcher) that lack partition and bootloader configuration capabilities, or distribution-specific tools that lack broad operating system support and advanced setup customizations.

Flashux brings **complete feature parity with Rufus to native Linux**, written strictly in **pure C** (C11 standard). It provides both an accessible, responsive graphical interface and an automated, scriptable command-line interface. Flashux handles everything from raw hybrid ISO block writing to advanced multi-partition UEFI:NTFS layouts, Windows 11 installation bypasses, Linux live persistence, Windows To Go (WTG) provisioning, and low-level bad sector verification.

---

## 2. Core Architectural Principles & Language Constraint

- **Implementation Language**: **Pure C only** (C11 standard, compiled with strict flags `-Wall -Wextra -Werror -pedantic -std=c11`). No C++, Python, Rust, or other runtimes are used in the core engine, CLI, or GUI.
- **Safety First**: Destructive block operations are gated by multi-stage validation, safety interlocks, explicit user confirmations, and drive classification checks to prevent catastrophic accidental overwriting of host operating system drives.
- **Privilege Separation**: A lightweight unprivileged frontend (CLI or GUI) communicates with a minimal, privileged backend helper process via a secure local IPC channel (UNIX domain socket with Polkit / `pkexec` authorization). Block-level raw disk access is strictly isolated.
- **Zero Heavy Runtimes**: Flashux avoids web runtimes (Electron), heavy interpreters, or multi-gigabyte dependency trees. It links directly against native Linux system libraries (`libudev`, `libblkid`, `libmount`, `libwim`, etc.).
- **Dual Interfaces**: 
  - `flashux-cli`: Fully interactive terminal UI (curses/ncurses) or non-interactive scriptable batch mode with JSON output and real-time progress callbacks.
  - `flashux-gui`: Fast, native Linux GUI built with GTK (GTK4 / libadwaita or GTK3) respecting system dark/light themes and modern desktop standards.

---

## 3. In-Depth Rufus Parity: Comprehensive Feature Specifications

Flashux is designed to support every single feature, capability, and edge-case handled by Rufus on Windows, mapped directly to Linux kernel mechanisms and standards.

### 3.1. Target Media & Storage Device Management
- **Device Enumeration & Hotplug Detection**:
  - Monitors storage subsystem events dynamically via `libudev` and Linux netlink sockets (`udev_monitor`).
  - Detects USB flash drives, USB external HDDs/SSDs, SD/microSD cards, eMMC media, and virtual disk loop devices (`/dev/loop*`).
  - Displays friendly device names (Vendor, Model, Serial, Bus Type, Connection Speed, Capacity).
- **Drive Safety & Protection Guardrails**:
  - Automatically filters out internal system drives (NVMe, SATA SSDs/HDDs containing root `/`, `/boot`, `/home`, or swap partitions).
  - Explicit "List Non-Removable / Fixed Drives" toggle with prominent warning dialogs for power users flashing to external USB hard drives or target SSDs.
  - Drive locking: prevents concurrent access using `flock()` / `O_EXCL` on block devices.
  - Unmounting: automatically unmounts all active mounted partitions (`libmount` or `umount2()`) prior to re-partitioning or formatting.
- **Physical Sector Architecture**:
  - Native support for standard 512-byte logical sectors, 512e (512 emulation), and 4Kn (4096-byte native Advanced Format) drives.
  - Proper partition alignment to 1 MiB (2048 sectors) boundaries for flash endurance and optimal read/write throughput.
- **Bad Block Verification**:
  - Pattern-based surface testing before formatting (1-pass, 2-pass, or 4-pass bad blocks check using alternating bit patterns).
  - Detection of fake/counterfeit USB flash drives by testing high-boundary write/read consistency.

### 3.2. Target System Architectures & Partition Schemes
- **Partition Schemes**:
  - **MBR (Master Boot Record / DOS partition table)**:
    - Target: BIOS or UEFI computers (Compatibility Support Module - CSM).
    - Writes custom MBR boot code (Syslinux MBR, Windows MBR, GRUB MBR).
    - Sets active (bootable) partition flags.
    - Preserves or injects disk signatures and partition IDs (`0x07` NTFS/exFAT, `0x0c` FAT32 LBA, `0x83` Linux native, `0xEF` EFI System).
  - **GPT (GUID Partition Table)**:
    - Target: UEFI computers (non-CSM).
    - Creates Protective MBR (Type `0xEE`) spanning entire drive.
    - Writes Primary GPT Header & Partition Array at LBA 1–33 and Backup GPT at disk end.
    - Sets correct Partition Type GUIDs (e.g., EFI System Partition: `c12a7328-f81f-11d2-ba4b-00a0c93ec93b`, Microsoft Basic Data: `ebd0a0a2-b9e5-4433-87c0-68b6b72699c7`).
  - **Dual BIOS / UEFI Hybrid Mode**:
    - Creates hybrid MBR/GPT layouts enabling a single USB drive to boot both on legacy BIOS systems and modern UEFI systems.
- **Target System Architectures**:
  - x86 (32-bit IA-32)
  - x86_64 (64-bit AMD64)
  - ARM (32-bit ARMv7)
  - ARM64 (AArch64)

### 3.3. File Systems & Formatting Capabilities
- **Supported File Systems**:
  - **FAT12 / FAT16**: For small boot partitions, FreeDOS, and legacy utilities.
  - **FAT32**: Universal UEFI compatibility. Includes support for formatting volumes larger than 32 GiB (bypassing Windows GUI artificial format limits).
  - **NTFS**: Full read/write format support with custom cluster sizes (512B, 1KB, 2KB, 4KB, 8KB, 16KB, 32KB, 64KB). Handles large files (>4 GiB).
  - **exFAT**: Modern cross-platform filesystem supporting huge files and cluster sizes up to 32 MiB.
  - **UDF (Universal Disk Format)**: UDF 1.02, 1.50, 2.01 for optical media ISO replication.
  - **Ext2 / Ext3 / Ext4**: Native Linux filesystems for Linux Live persistence and Linux OS installations.
  - **ReFS**: Detection and metadata compatibility where applicable.
- **Formatting Options**:
  - Quick Format (wipes file tables, metadata, and partition superblocks).
  - Full Format / Secure Erase (zeros or discards all sectors via `BLKDISCARD` ioctl / TRIM).
  - Volume label customization (automatic uppercase sanitization for FAT, full UTF-8 for NTFS/exFAT/Ext4).

### 3.4. Flashing Modes & Image Handling

Flashux provides dual flashing modes matching Rufus's intelligence:

```
                  +--------------------------------+
                  | Input Image (.iso, .img, etc.) |
                  +---------------+----------------+
                                  |
               +------------------+------------------+
               |                                     |
       [ Hybrid / Raw Image ]               [ Bootable Installer ISO ]
               |                                     |
               v                                     v
       +---------------+                     +---------------+
       | DD Image Mode |                     |ISO Image Mode |
       +---------------+                     +---------------+
       | Sector-copy   |                     | 1. Partition  |
       | Direct I/O    |                     | 2. Format     |
       | O_DIRECT      |                     | 3. Extract    |
       | Uncompress    |                     | 4. Bootloader |
       +---------------+                     +---------------+
```

#### 3.4.1. Mode 1: DD Image Mode (Raw Block Copy)
- Used for raw disk images (`.img`, `.raw`), virtual disks (`.vhd`, `.vhdx`), and hybrid ISOs (Debian, Ubuntu, Arch Linux, Fedora, openSUSE, Proxmox, TrueNAS, Clonezilla).
- Sector-accurate streaming copy using `O_DIRECT` and `posix_memalign` to bypass OS page cache and achieve maximum bus saturation.
- On-the-fly decompression for compressed images:
  - Gzip (`.gz`), XZ / LZMA (`.xz`), Bzip2 (`.bz2`), Zip (`.zip`), and Zstandard (`.zst`).
- Dynamic progress reporting: current transfer speed (MB/s), elapsed time, estimated time of arrival (ETA), and bytes written.
- Post-write sync (`sync_file_range`, `fdatasync`, `ioctl(BLKFLSBUF)`) and optional readback verification pass.

#### 3.4.2. Mode 2: ISO Image Mode (File Extraction & Bootloader Setup)
- Used when file-level extraction is required (standard Windows ISOs, tailored Linux installers).
- Mounts or unpacks ISO contents (ISO 9660 with Rock Ridge and Joliet extensions, or UDF).
- Partitions the target device according to user specifications (MBR or GPT).
- Formats the target partition with chosen filesystem (FAT32, NTFS, exFAT).
- Copies files while preserving directory hierarchy and attributes.
- Installs and configures target bootloader:
  - Windows Boot Manager (`bootmgr`, `bootmgr.efi`, `\EFI\BOOT\BOOTX64.EFI`).
  - Syslinux / Isolinux (v4, v5, v6) with automated MBR boot code injection.
  - GRUB 2 (both `i386-pc` for BIOS and `x86_64-efi` / `ia32-efi` / `arm64-efi` for UEFI).

#### 3.4.3. Mode 3: UEFI:NTFS Bootloader Support
- **Problem**: Modern Windows 10/11 ISOs contain `install.wim` files exceeding 4 GiB, which cannot reside on standard FAT32 partitions. However, many UEFI firmwares only include FAT drivers and cannot boot NTFS partitions directly.
- **Solution**: Flashux implements the open-source **UEFI:NTFS** mechanism created by Pete Batard (Rufus):
  1. Creates a primary NTFS partition containing all extracted Windows files (including oversized `install.wim`).
  2. Creates a tiny secondary FAT12/FAT16 partition (1–2 MiB) at the end of the drive.
  3. Copies the signed UEFI:NTFS driver and bootloader (`bootx64.efi`, `bootaa64.efi`) to the secondary FAT partition.
  4. At system boot, the UEFI firmware boots the FAT partition, UEFI:NTFS loads a GPLv3 NTFS driver into memory, and chains execution directly to the Windows Boot Manager on the NTFS partition.
- **Alternative (FAT32 WIM Split)**:
  - Supports splitting large `install.wim` files into multiple `.swm` chunks (under 4 GiB each) using `libwim` (`wimlib-imagex split`), allowing full FAT32 creation without requiring NTFS drivers.

#### 3.4.4. Mode 4: Windows To Go (WTG)
- Creates a fully bootable, live portable Windows installation on a USB drive from any official Windows ISO (`install.wim` or `install.esd`).
- Automatically creates required partition structure:
  - EFI System Partition (FAT32, 260 MiB)
  - Microsoft Reserved (MSR) partition (16 MiB)
  - Windows OS partition (NTFS, remaining space)
- Applies WIM image directly to the target partition using `libwim`.
- Generates BCD (Boot Configuration Data) store and populates `\EFI\Microsoft\Boot\` files.
- Injects `SANPolicy` configuration to keep internal disks offline in WTG mode to prevent host interference.

#### 3.4.5. Mode 5: FreeDOS & DOS Bootable Media
- Includes embedded FreeDOS kernel (`kernel.sys`, `command.com`) and boot records.
- Formats USB in FAT16/FAT32 and writes DOS boot sector.
- Essential for technicians updating motherboard BIOS/UEFI firmware, GPU VBIOS flashing, and legacy industrial utilities.

### 3.5. Windows Extended Setup Customizations (User Experience Options)
When a Windows 11 or Windows 10 ISO is detected, Flashux provides an interactive modal or CLI flags enabling deep OS setup customizations:

```
+--------------------------------------------------------------------------+
| Flashux: Windows User Experience Customization                           |
+--------------------------------------------------------------------------+
|  [X] Remove requirement for 4GB+ RAM, Secure Boot and TPM 2.0            |
|  [X] Remove requirement for an online Microsoft account                  |
|  [X] Create a local account with username: [ user                      ] |
|  [X] Set regional options to match current system values                 |
|  [X] Disable data collection / telemetry (Skip privacy questions)        |
|  [X] Disable BitLocker automatic device encryption                       |
+--------------------------------------------------------------------------+
|                               [ Apply & Continue ]     [ Cancel ]         |
+--------------------------------------------------------------------------+
```

- **Mechanism**:
  - Generates and injects a tailored `AutoUnattend.xml` response file into the root of the installation media.
  - Injects registry keys into `boot.wim` and `winpe` registry hives:
    - `HKLM\SYSTEM\Setup\LabConfig\BypassTPMCheck=1`
    - `HKLM\SYSTEM\Setup\LabConfig\BypassSecureBootCheck=1`
    - `HKLM\SYSTEM\Setup\LabConfig\BypassRAMCheck=1`
    - `HKLM\SYSTEM\Setup\LabConfig\BypassCPUCheck=1`
    - `HKLM\SYSTEM\Setup\LabConfig\BypassStorageCheck=1`
  - Bypasses Network / Microsoft Account requirement (`OOBE\BYPASSNRO`).
  - Disables BitLocker automatic provisioning (`PreventDeviceEncryption=1`).

### 3.6. Linux Live USB Persistence
- Creates a dedicated persistence partition alongside the main live filesystem.
- Supports Ubuntu and derivatives (`casper-rw` label), Debian/Kali (`persistence` label and `persistence.conf`), Linux Mint, and Fedora (`live-rw`).
- Persistence partition formatted in `ext4` or `ext3`.
- Interactive slider or CLI flag (`--persistence-size`) allowing users to choose exact storage allocation (from 512 MiB up to drive capacity).
- Patches live bootloader configuration files (`grub.cfg`, `isolinux.cfg`, `syslinux.cfg`) to append persistence boot parameters (`persistent`, `persistence`, `persistence-path`).

### 3.7. Bootloader & Dependency Management
- **Syslinux / Isolinux Handling**:
  - Automatically identifies Syslinux version of target ISO (v4, v5, v6).
  - Injects matching `ldlinux.sys` and `ldlinux.bss`.
  - Resolves version mismatch errors by maintaining a local cache of official Syslinux binaries.
- **GRUB 2 Installation**:
  - Capable of installing GRUB 2 directly to MBR or EFI System Partition.
- **Checksum Calculation & Verification**:
  - Fast, multi-threaded hashing engine for MD5, SHA-1, SHA-256, and SHA-512.
  - One-click copy/compare against known distribution hashes.
- **ISO Downloader Integration**:
  - Built-in capability to fetch official Windows 10, Windows 11, and popular Linux distribution ISOs directly, utilizing verified mirrors and SHA-256 validation.

---

## 4. Software Architecture & Design

Flashux is structured as a modular C11 codebase consisting of a core static library (`libflashux`), a headless daemon/privileged helper, a terminal client, and a native graphical client.

```
flashux/
├── include/              # Public C API headers
│   ├── flashux/
│   │   ├── core.h        # Context, logging, error codes
│   │   ├── device.h      # Device discovery, probing, udev
│   │   ├── format.h      # Filesystem creation, mkfs wrappers
│   │   ├── partition.h   # MBR, GPT, partition tables
│   │   ├── flasher.h     # DD and ISO extraction engines
│   │   ├── iso.h         # ISO 9660, UDF, El Torito parser
│   │   ├── windows.h     # Windows bypasses, AutoUnattend, wimlib
│   │   ├── persistence.h # Casper / Debian persistence config
│   │   └── checksum.h    # MD5, SHA1, SHA256, SHA512
├── src/
│   ├── core/             # Library implementation
│   ├── cli/              # flashux-cli (ncurses / args parser)
│   ├── gui/              # flashux-gui (GTK4 / libadwaita)
│   └── helper/           # flashux-helper (privileged Polkit backend)
├── data/                 # Icons, desktop files, Polkit policy files
├── tests/                # Unit tests and mock block device tests
└── meson.build           # Meson build system configuration
```

### 4.1. Core Modules (libflashux)
1. **`device`**: Queries `libudev` and `/sys/block` to enumerate physical devices, partition topologies, vendor strings, read-only status, and sector sizes.
2. **`partition`**: Programmatically manipulates MBR and GPT partition tables using low-level disk I/O, generating standard partition headers without external tool dependencies.
3. **`format`**: Dispatches filesystem creation for FAT12/16/32, NTFS, exFAT, and ext4, invoking kernel-tuned direct formatting functions or system utilities with fine-grained parameter control.
4. **`iso`**: Parses El Torito boot catalogs, detects EFI boot images, verifies hybrid ISO headers, inspects Windows `install.wim` / `install.esd` metadata.
5. **`flasher`**:
   - `flasher_dd`: High-throughput direct I/O raw copier with streaming decompression and cancellation support.
   - `flasher_iso`: Multi-threaded extraction and bootloader installer.
6. **`windows`**: Unattended XML generator, registry patcher, `wimlib` integration for WTG and WIM splitting.
7. **`persistence`**: Partition sizing, ext4 formatting, label injection (`casper-rw`, `persistence`), and `grub.cfg` configuration patching.
8. **`checksum`**: SIMD/hardware-accelerated (AVX2/NEON) hashing for ISO integrity verification.

---

## 5. Technology Stack & Dependencies

| Component | Technology | Rationale |
| :--- | :--- | :--- |
| **Language** | C (C11 standard) | Strict project requirement; predictable low-level performance, minimal memory footprint. |
| **Build System** | Meson + Ninja | Modern, ultra-fast C build framework with clean dependency management. |
| **GUI Toolkit** | GTK 4 / libadwaita (or GTK 3) | Native Linux desktop look and feel, Wayland & X11 support, accessibility. |
| **TUI / CLI** | Curses / ncurses + POSIX CLI | Rich terminal interface for headless servers and terminal enthusiasts. |
| **Device Subsystem** | `libudev` & `libblkid` | Standard Linux kernel device management and filesystem probing. |
| **Mount Subsystem** | `libmount` | Safe, programmatic partition unmounting and mounting. |
| **WIM Management**| `wimlib` (`libwim`) | High-performance open-source WIM extraction, splitting, and inspection. |
| **Cryptography** | OpenSSL `libcrypto` or embedded C hashes | Fast SHA-256, SHA-512, MD5 calculation. |
| **Compression** | `zlib`, `liblzma`, `libzstd`, `libbz2` | High-speed decompression for `.gz`, `.xz`, `.zst`, `.bz2` disk images. |
| **Privilege Auth** | Polkit (`polkit-1`) / `pkexec` | Secure, standard Linux privilege escalation for block device writing. |

---

## 6. Project Roadmap & Milestones

1. **Milestone 1: Project Foundation & Core Library (`libflashux`)**
   - Logging, memory tracking, device enumeration via `libudev`.
   - MBR and GPT partitioning engine.
   - Filesystem formatting engine.
2. **Milestone 2: DD Flashing Engine & CLI Prototype**
   - Direct I/O block writing with real-time throughput and ETA metrics.
   - Image format validation and streaming decompression (`.gz`, `.xz`, `.zst`).
   - `flashux-cli` interactive and batch modes.
3. **Milestone 3: ISO Extraction & Bootloader Setup**
   - ISO9660/UDF parsing and extraction.
   - Syslinux, GRUB 2, and UEFI bootloader installation.
   - Multi-pass bad block detection.
4. **Milestone 4: Windows Support & Parity**
   - Windows 11 hardware requirement bypass (`AutoUnattend.xml` and registry patch).
   - UEFI:NTFS bootloader integration and WIM splitting (`libwim`).
   - Windows To Go (WTG) installation mode.
5. **Milestone 5: Linux Persistence & Advanced Tweaks**
   - Casper / Debian / Fedora persistence partition creation.
   - Boot config patching (`grub.cfg`).
6. **Milestone 6: Native GTK GUI & Privilege Daemon**
   - GTK4 / libadwaita graphical interface.
   - Polkit helper daemon with secure IPC.
7. **Milestone 7: Verification, Hardening & Packaging**
   - Valgrind / ASan leak-free verification.
   - AppImage, Flatpak, Debian/RPM, and Arch PKGBUILD packaging.
