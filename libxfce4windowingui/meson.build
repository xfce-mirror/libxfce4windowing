xfwu_config_h = configure_file(
  output: 'libxfce4windowingui-config.h',
  input:  'libxfce4windowingui-config.h.in',
  configuration: xfw_config,
)
xfwu_hdr = [
  'libxfce4windowingui.h',
  'xfw-window-action-menu.h',
  xfwu_config_h,
]

libxfwu_hdr_dir = join_paths('xfce4', fs.name(meson.current_source_dir()))
install_headers(
  xfwu_hdr,
  subdir: libxfwu_hdr_dir
)

xfwu_src = xfwu_hdr + [
  'libxfce4windowingui-config.c',
  'xfw-window-action-menu.c',
]

libxfwu_deps = [
  libxfw_dep,
  gobject_dep,
  gtk_dep,
]

libxfwu_link_args = [
  '-Wl,--version-script,@0@/@1@'.format(
    meson.project_source_root(),
    'libxfce4windowing.syms'
  ),
]

libxfwu_apiversion = '0'
libxfwu_apiname = 'libxfce4windowingui-@0@'.format(libxfwu_apiversion)
libxfwu = shared_library(
  'xfce4windowingui-@0@'.format(libxfwu_apiversion),
  xfwu_src,
  soversion:           0,
  dependencies:        libxfwu_deps,
  include_directories: [ root_inc, include_directories('.') ],
  install:             true,
  link_args:           libxfwu_link_args,
)

if enable_gir
  libxfwu_gir_extra_args = [
    # '--c-include=libxfce4windowingui/libxfce4windowingui.h',
    # '--add-include-path=@0@'.format(meson.current_source_dir()),
    '--warn-all',
  ]
  libxfwu_gir = gnome.generate_gir(
    libxfwu,
    sources:   xfwu_src,
    nsversion: libxfwu_apiversion,
    namespace: 'Libxfce4windowingui',
    export_packages: libxfwu_apiname,
    identifier_prefix: ['libxfce4windowingui', 'Xfw'],
    link_with: libxfwu,
    includes: [
      libxfw_gir[0],
      'GObject-2.0',
      'GLib-2.0',
      'Gio-2.0',
      'Gtk-3.0',
    ],
    header: 'libxfce4windowingui/libxfce4windowingui.h',
    install: true,
    install_dir_gir: xf_datadir / 'gir-1.0',
    install_dir_typelib: xf_libdir / 'girepository-1.0',
    extra_args: libxfwu_gir_extra_args,
  )
endif

pc.generate(
  libraries: [libxfwu],
  subdirs: libxfwu_hdr_dir,
  version: meson.project_version(),
  name: libxfwu_apiname,
  filebase: libxfwu_apiname,
  description: 'X11/Wayland windowing utility library for Xfce - extra widgets',
  requires: [
    libxfw_apiname,
    'gtk+-3.0'
  ],
  install_dir: xf_libdir / 'pkgconfig',
)
