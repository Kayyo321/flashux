if gtk.found() and adwaita.found()
  gui_sources = files('main.c', 'app_window.c', 'device_selector.c', 'image_selector.c', 'win_dialog.c')
  gui = executable('flashux-gui', gui_sources, dependencies: [flashux_dep, gtk, adwaita],
    c_args: ['-DADW_VERSION_MIN_REQUIRED=ADW_VERSION_1_4',
             '-DADW_VERSION_MAX_ALLOWED=ADW_VERSION_1_4',
             '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_4_10',
             '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_4_10'], install: true)
endif
