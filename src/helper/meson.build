helper_deps = [raw_dep]
if is_linux
  helper_deps += polkit
endif
helper = executable('flashux-helper', 'helper_main.c', 'polkit_auth.c',
  dependencies: helper_deps, install: true, install_dir: get_option('libexecdir'))
