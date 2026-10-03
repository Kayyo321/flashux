core_sources = files('core.c', 'logging.c', 'device.c', 'mount.c', 'checksum.c', 'partition.c', 'format.c')
libflashux = static_library('flashux', core_sources, include_directories: inc,
  dependencies: core_deps, install: true)
flashux_dep = declare_dependency(link_with: libflashux, include_directories: inc,
  dependencies: core_deps)
install_subdir('../../include/flashux', install_dir: get_option('includedir'))
