gui_available = gtk.found() and adwaita.found()
if gui_available
  gui_sources = files('app_window.c', 'device_selector.c', 'image_selector.c',
    'win_dialog.c', 'progress_view.c', 'dialogs.c')
  gui_api_flags = ['-DADW_VERSION_MIN_REQUIRED=ADW_VERSION_1_4',
    '-DADW_VERSION_MAX_ALLOWED=ADW_VERSION_1_4',
    '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_4_10',
    '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_4_10']
  gui_core = static_library('flashux-gui-core', gui_sources,
    dependencies: [flashux_dep, gtk, adwaita], c_args: gui_api_flags)
  gui_dep = declare_dependency(link_with: gui_core, dependencies: [flashux_dep, gtk, adwaita],
    compile_args: gui_api_flags)
  gui = executable('flashux-gui', 'main.c', dependencies: gui_dep, install: true)
endif
