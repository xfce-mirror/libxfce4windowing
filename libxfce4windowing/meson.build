xfw_config_h = configure_file(
  output: 'libxfce4windowing-config.h',
  input:  'libxfce4windowing-config.h.in',
  configuration: xfw_config,
)
xfw_hdr = [
  'libxfce4windowing.h',
  'xfw-application.h',
  'xfw-screen.h',
  'xfw-util.h',
  'xfw-window.h',
  'xfw-workspace.h',
  'xfw-workspace-group.h',
  'xfw-workspace-manager.h',
  xfw_config_h,
]

xfw_hdr += xfw_config_h

libxfw_hdr_dir = join_paths('xfce4', fs.name(meson.current_source_dir()))
install_headers(
  xfw_hdr,
  subdir: libxfw_hdr_dir
)

xfw_private_src = [
  'libxfce4windowing-private.c',
  'libxfce4windowing-private.h',
  'xfw-application-private.h',
  'xfw-screen-private.h',
  'xfw-window-private.h',
  'xfw-workspace-private.h',
  'xfw-workspace-group-private.h',
  'xfw-workspace-manager-private.h',
]

xfw_src = xfw_hdr + [
  'libxfce4windowing-config.c',
  'xfw-application.c',
  'xfw-screen.c',
  'xfw-util.c',
  'xfw-window.c',
  'xfw-workspace.c',
  'xfw-workspace-dummy.c',
  'xfw-workspace-dummy.h',
  'xfw-workspace-group.c',
  'xfw-workspace-group-dummy.c',
  'xfw-workspace-group-dummy.h',
  'xfw-workspace-manager.c',
  'xfw-workspace-manager-dummy.c',
  'xfw-workspace-manager-dummy.h',
]

xfw_wl_src = [
  'xfw-application-wayland.c',
  'xfw-application-wayland.h',
  'xfw-screen-wayland.c',
  'xfw-screen-wayland.h',
  'xfw-window-wayland.c',
  'xfw-window-wayland.h',
  'xfw-workspace-group-wayland.c',
  'xfw-workspace-group-wayland.h',
  'xfw-workspace-manager-wayland.c',
  'xfw-workspace-manager-wayland.h',
  'xfw-workspace-wayland.c',
  'xfw-workspace-wayland.h',
]

xfw_x11_src = [
  'xfw-application-x11.c',
  'xfw-application-x11.h',
  'xfw-screen-x11.c',
  'xfw-screen-x11.h',
  'xfw-window-x11.c',
  'xfw-window-x11.h',
  'xfw-wnck-icon.c',
  'xfw-wnck-icon.h',
  'xfw-workspace-x11.c',
  'xfw-workspace-x11.h',
  'xfw-workspace-manager-x11.c',
  'xfw-workspace-manager-x11.h',
]

xfw_src += gnome.genmarshal(
  'xfw-marshal',
  sources: 'xfw-marshal.list',
  prefix: 'xfw_marshal',
  internal: true,
)

libxfw_deps = [
  glib_dep,
  gio_dep,
  gio_unix_dep,
  gobject_dep,
  gtk_dep,
  gdk_dep,
  gdk_pixbuf_dep,
]

if enable_wayland
  xfw_src += xfw_wl_src
  libxfw_deps += [
    xfw_wl_protocol_dep,
    gdk_wayland_dep,
    wl_client_dep,
  ]
endif
if enable_x11
  xfw_src += xfw_x11_src
  libxfw_deps += [
    libwnck_dep,
    gdk_x11_dep,
    libx11_dep,
  ]
endif

libxfw_link_args = [
  '-Wl,--version-script,@0@/@1@'.format(
    meson.project_source_root(),
    'libxfce4windowing.syms'
  ),
]

libxfw_apiversion = '0'
libxfw_apiname = 'libxfce4windowing-@0@'.format(libxfw_apiversion)
libxfw = shared_library(
  'xfce4windowing-@0@'.format(libxfw_apiversion),
  xfw_src + xfw_private_src,
  soversion:           0,
  dependencies:        libxfw_deps,
  include_directories: [ root_inc, include_directories('.') ],
  install:             true,
  link_args:           libxfw_link_args,
)
libxfw_dep = declare_dependency(
  dependencies: [
    gtk_dep
  ],
  link_with: libxfw,
  include_directories: include_directories('.')
)

if enable_gir
  libxfw_gir_extra_args = [
    # '--c-include=libxfce4windowing/libxfce4windowing.h',
    # '--add-include-path=@0@'.format(meson.current_source_dir()),
    '--warn-all',
  ]
  libxfw_gir = gnome.generate_gir(
    libxfw,
    sources:   xfw_src,
    nsversion: libxfw_apiversion,
    namespace: 'Libxfce4windowing',
    export_packages: libxfw_apiname,
    identifier_prefix: ['libxfce4windowing', 'Xfw'],
    link_with: libxfw,
    includes: [
      'GObject-2.0',
      'GLib-2.0',
      'Gio-2.0',
      'Gtk-3.0',
    ],
    header: 'libxfce4windowing/libxfce4windowing.h',
    # include_directories:
    install: true,
    install_dir_gir: xf_datadir / 'gir-1.0',
    install_dir_typelib: xf_libdir / 'girepository-1.0',
    extra_args: libxfw_gir_extra_args,
  )
endif

pc.generate(
  libraries: [libxfw],
  subdirs: libxfw_hdr_dir,
  version: meson.project_version(),
  name: libxfw_apiname,
  filebase: libxfw_apiname,
  description: 'X11/Wayland windowing utility library for Xfce',
  requires: 'gtk+-3.0',
  install_dir: xf_libdir / 'pkgconfig',
)
