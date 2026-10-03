if gtk.found() and adwaita.found()
  gui_sources = files('main.c', 'app_window.c')
  gui = executable('flashux-gui', gui_sources, dependencies: [flashux_dep, gtk, adwaita],
    install: true)
endif
