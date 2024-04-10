if get_option('gtk-doc')
  libxfce4windowing_doc_name = 'libxfce4windowing'

  conf_data = configuration_data()
  conf_data.set_quoted('PACKAGE_NAME', libxfce4windowing_doc_name)
  conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
  conf_data.set_quoted('PACKAGE_STRING', '@0@-@1@'.format(libxfce4windowing_doc_name, meson.project_version()))

  configure_file(
    input: 'gtkdocentities.ent.in',
    output: 'gtkdocentities.ent',
    configuration: conf_data,
  )

  libxfce4windowing_doc_private_headers = [
    'libxfce4windowing-private.h',
    'xfw-application-private.h',
    'xfw-application-wayland.h',
    'xfw-application-x11.h',
    'xfw-marshal.h',
    'xfw-screen-private.h',
    'xfw-screen-wayland.h',
    'xfw-screen-x11.h',
    'xfw-window-private.h',
    'xfw-window-wayland.h',
    'xfw-window-x11.h',
    'xfw-wnck-icon.h',
    'xfw-workspace-dummy.h',
    'xfw-workspace-group-dummy.h',
    'xfw-workspace-group-private.h',
    'xfw-workspace-group-wayland.h',
    'xfw-workspace-manager-dummy.h',
    'xfw-workspace-manager-private.h',
    'xfw-workspace-manager-wayland.h',
    'xfw-workspace-manager-x11.h',
    'xfw-workspace-private.h',
    'xfw-workspace-wayland.h',
    'xfw-workspace-x11.h',
  ]

  libxfce4windowing_doc_fixxref_args = [
    '--extra-dir=@0@/share/gtk-doc/html/glib'.format(glib.get_variable(pkgconfig: 'prefix')),
    '--extra-dir=@0@/share/gtk-doc/html/gobject'.format(gobject.get_variable(pkgconfig: 'prefix')),
    '--extra-dir=@0@/share/gtk-doc/html/gdk3'.format(gdk.get_variable(pkgconfig: 'prefix')),
    '--extra-dir=@0@/share/gtk-doc/html/gdk-pixbuf'.format(gdk_pixbuf.get_variable(pkgconfig: 'prefix')),
    # Weird non-standard Debian location?
    '--extra-dir=@0@/share/doc/libgdk-pixbuf2.0-doc/html/gdk-pixbuf'.format(gdk_pixbuf.get_variable(pkgconfig: 'prefix')),
  ]

  gnome.gtkdoc(
    libxfce4windowing_doc_name,
    module_version: lib_so_version,
    src_dir: meson.project_source_root() / 'libxfce4windowing',
    ignore_headers: libxfce4windowing_doc_private_headers,
    main_xml: 'libxfce4windowing-docs.xml',
    dependencies: libxfce4windowing_dep,
    gobject_typesfile: meson.current_source_dir() / 'libxfce4windowing.types',
    fixxref_args: libxfce4windowing_doc_fixxref_args,
    install: true,
    check: true,
  )
endif
