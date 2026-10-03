project('flashux', 'c', version: '0.1.0', meson_version: '>=1.3.0',
  default_options: ['c_std=c11', 'warning_level=0', 'werror=true'])
cc = meson.get_compiler('c')
add_project_arguments('-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Wformat=2',
  '-Wconversion', '-Wshadow', '-Wpointer-arith', '-Wcast-qual', '-Wstrict-prototypes',
  '-Wmissing-prototypes', '-D_GNU_SOURCE', '-D_FILE_OFFSET_BITS=64', language: 'c')
threads = dependency('threads')
crypto = dependency('libcrypto')
curses = dependency('ncurses')
is_linux = host_machine.system() == 'linux'
udev = dependency('libudev', required: is_linux)
blkid = dependency('blkid', required: is_linux)
mount = dependency('mount', required: is_linux)
polkit = dependency('polkit-gobject-1', required: is_linux)
glib = dependency('glib-2.0')
gtk = dependency('gtk4', version: '>=4.10', required: get_option('gui'))
adwaita = dependency('libadwaita-1', version: '>=1.4', required: get_option('gui'))
config = configuration_data()
config.set10('FLASHUX_LINUX', is_linux)
configure_file(output: 'flashux_config.h', configuration: config)
core_deps = [threads, crypto]
if is_linux
  core_deps += [udev, blkid, mount]
endif
inc = include_directories('include', 'src', '.')
subdir('src/core')
subdir('src/helper')
subdir('src/cli')
subdir('tests')
