policy_config = configuration_data()
policy_config.set('HELPER_PATH', get_option('prefix') / get_option('libexecdir') / 'flashux-helper')
configure_file(input: 'org.flashux.policy.in', output: 'org.flashux.policy',
  configuration: policy_config, install: is_linux,
  install_dir: get_option('datadir') / 'polkit-1/actions')
if gtk.found() and adwaita.found()
  install_data('org.flashux.Flashux.desktop', install_dir: get_option('datadir') / 'applications')
endif
