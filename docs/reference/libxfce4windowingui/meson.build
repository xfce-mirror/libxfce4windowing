xfwu_doc_ignore = [
]
xfwu_name = fs.name(meson.current_source_dir())
gnome.gtkdoc(
  xfwu_name,
  src_dir: meson.project_build_root() / xfw_name,
  content_files: xfwu_src,
  ignore_headers: xfwu_doc_ignore,
  main_xml: '@0@-docs.xml'.format(xfwu_name),
  dependencies: [
    libxfw_dep,
    glib_dep,
    gobject_dep,
    gtk_dep,
    gdk_dep,
    gdk_pixbuf_dep,
  ],
  install: true,
  scan_args: [
    '--deprecated-guards="XFCE_DISABLE_DEPRECATED',
  ],
)
