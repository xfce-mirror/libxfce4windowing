xfw_doc_ignore = [
  'libxfce4windowing-private.h',
  'xfw-marshal.h',
  'xfw-application-private.h',
  'xfw-application-wayland.h',
  'xfw-application-x11.h',
  'xfw-screen-wayland.h',
  'xfw-screen-x11.h',
  'xfw-window-private.h',
  'xfw-window-wayland.h',
  'xfw-window-x11.h',
  'xfw-wnck-icon.h',
  'xfw-workspace-dummy.h',
  'xfw-workspace-group-dummy.h',
  'xfw-workspace-group-wayland.h',
  'xfw-workspace-manager-dummy.h',
  'xfw-workspace-manager-wayland.h',
  'xfw-workspace-manager-x11.h',
  'xfw-workspace-wayland.h',
  'xfw-workspace-x11.',
]
xfw_name = fs.name(meson.current_source_dir())
libxfw_gtkdoc = gnome.gtkdoc(
  xfw_name,
  src_dir: meson.project_build_root() / xfw_name,
  content_files: xfw_src,
  ignore_headers: xfw_doc_ignore,
  main_xml: '@0@-docs.xml'.format(xfw_name),
  dependencies: [
    glib_dep,
    gobject_dep,
    gdk_dep,
    gdk_pixbuf_dep,
  ],
  install: true,
  scan_args: [
    '--deprecated-guards="XFCE_DISABLE_DEPRECATED',
  ],
)
