core_sources = files('core.c', 'logging.c', 'device.c', 'device_geometry.c', 'device_topology.c', 'mount.c', 'checksum.c', 'partition.c', 'format.c', 'ipc_wire.c', 'ipc_client.c', 'progress.c', 'flasher.c', 'helper_launcher.c', 'image_stream.c', 'wipe.c')
libflashux = static_library('flashux', core_sources, include_directories: inc,
  dependencies: core_deps, install: true)
flashux_dep = declare_dependency(link_with: libflashux, include_directories: inc,
  dependencies: core_deps)
install_subdir('../../include/flashux', install_dir: get_option('includedir'))
raw_engine = static_library('flashux-raw', '../helper/privileged_ops.c',
  include_directories: inc, dependencies: flashux_dep)
raw_dep = declare_dependency(link_with: raw_engine, dependencies: flashux_dep)
pkgconfig = import('pkgconfig')
pkgconfig.generate(libflashux, name: 'Flashux', description: 'Native C11 boot media foundation')
