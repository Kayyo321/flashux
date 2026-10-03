project('flashux', 'c', version: '0.1.0', meson_version: '>=1.3.0',
  default_options: ['c_std=c11', 'warning_level=0', 'werror=true'])
cc = meson.get_compiler('c')
add_project_arguments('-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Wformat=2',
  '-Wconversion', '-Wshadow', '-Wpointer-arith', '-Wcast-qual', '-Wstrict-prototypes',
  '-Wmissing-prototypes', '-D_GNU_SOURCE', '-D_FILE_OFFSET_BITS=64', language: 'c')
