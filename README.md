# Flashux

Flashux is a native C11 boot-media application for Linux, with a GTK4/libadwaita window,
an ncurses interface, a JSON batch CLI, a static core library, and a Polkit helper.
This branch implements the initial application foundation. See [PROJECT.md](PROJECT.md)
for the full roadmap and [the implementation tracker](feature/initial-implementation/TRACKER.md)
for verified progress and outstanding integration checks.

Raw/DD jobs copy an image's existing disk layout. They support ordinary images, gzip, xz,
bzip2, zstd, and single-file ZIP archives, with decoded-capacity preflight, cancellation,
sector-aligned writes, synchronization, and complete readback verification. Archives must contain
one unencrypted regular image. Preflight adds a complete decoding pass before writing; memory
usage stays bounded. Dry runs decode and validate the image without modifying the target.

ISO extraction, Windows To Go, FreeDOS creation, Windows setup modifications, persistence,
and bad-block scanning are future roadmap work. Requests for these operations return an
unsupported error before writes. Windows preferences can be saved in the GUI for the session;
they are not applied by the raw engine. Raw jobs preserve the image's partition and filesystem
layout, so partition and formatting selections are disabled in raw mode.

## Build on Linux

Ubuntu 24.04 or an equivalent development environment provides the tested dependencies:

```sh
sudo apt-get install build-essential meson ninja-build pkg-config libssl-dev libarchive-dev \
  libncurses-dev libglib2.0-dev libudev-dev libblkid-dev libmount-dev libpolkit-gobject-1-dev \
  libgtk-4-dev libadwaita-1-dev libpango1.0-dev libfontconfig1-dev xvfb valgrind \
  polkitd pkexec dosfstools ntfs-3g exfatprogs e2fsprogs
meson setup build -Dgui=enabled --prefix=/usr/local
meson compile -C build
meson test -C build --print-errorlogs
```

Meson 1.3+, GTK 4.10+, and libadwaita 1.4+ are required. Use `-Dgui=disabled` for a headless
build. All C targets compile with the strict flags specified in [AGENTS.md](AGENTS.md).
The formatter dispatcher uses the distribution's fixed `/usr/sbin/mkfs.*` executables with
argument arrays and an inherited validated descriptor, including a zero pass for full formats.

Install the helper and its generated Polkit policy together using the configured prefix:

```sh
sudo meson install -C build
flashux-gui
flashux-cli --help
```

Run frontends as the desktop user. Raw writes request authorization through `pkexec` and the
helper's independent peer check. The helper uses `/run/flashux/helper.sock` in a root-owned
runtime directory. Detected system and swap-backed disks remain protected, including stacked
storage ancestry; ambiguous geometry or topology fails closed. Fixed drives require explicit
listing. Each confirmed job retains its kernel disk sequence to reject a replaced target.
No system-drive override is provided. Real writes require Linux block devices and kernels
supporting `BLKGETDISKSEQ`; regular-file targets are accepted only for dry runs.

## CLI examples

```sh
flashux-cli --list-devices
flashux-cli --interactive
flashux-cli --batch --image image.img.xz --device /dev/sdX --dry-run
```

Batch execution emits one JSON object per line, including progress and a final result record.
Success exits with 0, invalid arguments with 2, cancelled jobs with 130, and other failures with 1.
A batch write additionally requires `--yes` after target review; interactive writes require a
separate typed confirmation. Dry runs against sparse image files are suitable for development.
The GUI also has a dry-run checkbox and keeps destructive confirmation separate from selection.

## Verification

Tests use temporary regular files and synthetic sysfs trees. They do not write to physical disks.
The native GUI suite runs under Xvfb when available and verifies layout state, modal behavior,
dry-run completion, and closing during a job. Missing Xvfb means that GUI runtime check is not
registered; a successful headless run does not prove GUI behavior.

```sh
meson setup build-asan -Dgui=enabled -Db_sanitize=address,undefined -Db_lundef=false
meson compile -C build-asan
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
  meson test -C build-asan --print-errorlogs
meson test -C build core partition checksum device cli-args format flasher ipc image cli-batch \
  --wrapper 'valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --errors-for-leak-kinds=all --error-exitcode=99' \
  --num-processes 2 --timeout-multiplier 4 --print-errorlogs
```

`build/tests/test_helper allow|deny` is a manual, nondestructive helper ping probe. It requires a
running helper and Polkit with the installed policy. It verifies transport and authorization,
not disk writes; use isolated Linux test environments for privileged integration. Native
mounted-loop-device writes, formatter tools on loop partitions, desktop authorization prompts,
and hotplug still need integration verification before this tracker can reach 100%.

## Portable development

macOS can build the portable library, CLI, GUI, and sparse-file tests. Native Linux device
operations and the privileged helper return unsupported there. With Homebrew dependencies:

```sh
brew install meson ninja pkg-config openssl@3 libarchive ncurses gtk4 libadwaita
meson setup build --pkg-config-path="$(brew --prefix libarchive)/lib/pkgconfig" -Dgui=enabled
meson compile -C build
meson test -C build --print-errorlogs
```

GTK availability on macOS does not provide Linux storage functionality. Linux builds and tests
remain necessary. The library installs public headers and `flashux.pc` for C consumers.

Licensed under [GPL-3.0](LICENSE). Contributions follow [WORKFLOW.md](WORKFLOW.md).
