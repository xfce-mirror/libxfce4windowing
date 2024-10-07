libxfce4windowingui_apiname = 'xfce4windowingui-@0@'.format(lib_so_version)
libxfce4windowingui_pkgname = 'lib@0@'.format(libxfce4windowingui_apiname)

windowingui_headers = [
  'libxfce4windowingui.h',
  'xfw-window-action-menu.h',
  'xfw-window-list-menu.h',
]

windowingui_sources = [
  'libxfce4windowingui-config.c',
  'xfw-window-action-menu.c',
  'xfw-window-list-menu.c',
]

windowingui_config_data = configuration_data({
  'LIBXFCE4WINDOWINGUI_VERSION_MAJOR': version_parts[0],
  'LIBXFCE4WINDOWINGUI_VERSION_MINOR': version_parts[1],
  'LIBXFCE4WINDOWINGUI_VERSION_MICRO': version_parts[2],
})

windowingui_sources += configure_file(
  configuration: windowingui_config_data,
  input: 'libxfce4windowingui-config.h.in',
  output: 'libxfce4windowingui-config.h',
  install: true,
  install_dir: get_option('prefix') / get_option('includedir') / 'xfce4' / libxfce4windowing_pkgname / 'libxfce4windowingui',
)

windowingui_link_depends = []
windowingui_link_depends += custom_target(
  'libxfce4windowingui-visibility.h',
  input: 'libxfce4windowingui.symbols',
  output: 'libxfce4windowingui-visibility.h',
  command: [xdt_gen_visibility, '--kind=header', '@INPUT@', '@OUTPUT@'],
)
windowingui_link_depends += custom_target(
  'libxfce4windowingui-visibility.c',
  input: 'libxfce4windowingui.symbols',
  output: 'libxfce4windowingui-visibility.c',
  command: [xdt_gen_visibility, '--kind=source', '@INPUT@', '@OUTPUT@'],
)

windowingui_deps = [
  gtk
]

libxfce4windowingui = library(
  libxfce4windowingui_apiname,
  sources: windowingui_sources,
  version: lib_version,
  soversion: lib_so_version,
  c_args: [
    '-DLIBXFCE4WINDOWINGUI_COMPILATION=1',
  ],
  include_directories: [
    include_directories('..'),
  ],
  gnu_symbol_visibility: gnu_symbol_visibility,
  dependencies: windowingui_deps,
  link_with: [
    libxfce4windowing,
  ],
  link_depends: windowingui_link_depends,
  install: true,
)

install_headers(
  windowingui_headers,
  subdir: 'xfce4' / libxfce4windowing_pkgname / 'libxfce4windowingui',
)

pkgconfig.generate(
  libraries: [libxfce4windowingui],
  version: meson.project_version(),
  name: libxfce4windowingui_pkgname,
  filebase: libxfce4windowingui_pkgname,
  description: 'X11/Wayland windowing utility library for Xfce - extra widgets',
  requires: ['gtk+-3.0'],
  subdirs: ['xfce4' / libxfce4windowing_pkgname],
  install_dir: get_option('prefix') / get_option('libdir') / 'pkgconfig',
)

if get_option('introspection')
  windowingui_gir_sources = windowingui_sources + windowingui_headers

  gnome.generate_gir(
    libxfce4windowingui,
    sources: windowingui_gir_sources,
    namespace: 'Libxfce4windowingui',
    nsversion: '0.0',
    identifier_prefix: [
      'libxfce4windowingui',
      'Xfw',
    ],
    symbol_prefix: [
      'libxfce4windowingui',
      'xfw',
    ],
    export_packages: libxfce4windowingui_pkgname,
    includes: [
      'GLib-2.0',
      'GObject-2.0',
      'Gio-2.0',
      'Gtk-3.0',
      libxfce4windowing_gir[0],
    ],
    header: 'libxfce4windowingui/libxfce4windowingui.h',
    dependencies: libxfce4windowing_gir_dep,
    install: true,
    extra_args: [
      '-DLIBXFCE4WINDOWINGUI_COMPILATION=1',
    ],
  )
endif

libxfce4windowingui_dep = declare_dependency(
  dependencies: windowingui_deps,
  link_with: libxfce4windowingui,
)
