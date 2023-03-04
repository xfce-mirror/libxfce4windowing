project(
  'libxfce4windowing', ['c'],
  version:       '4.19.1',
  license:       'LGPL-2.1-or-later',
  meson_version: '>= 0.59.0',
  default_options: [
    'buildtype=debugoptimized',
    'warning_level=2',
  ]
)

xf_name = meson.project_name().to_lower()
xf_version = meson.project_version()

_xf_version = xf_version.split('.')
xf_version_major = _xf_version[0]
xf_version_minor = _xf_version[1]
xf_version_micro = _xf_version[2]
xf_version_nano = ''
if _xf_version.length() >= 4
  xf_version_nano  = _xf_version[3]
endif

xf_prefix = get_option('prefix')
xf_bindir = join_paths(xf_prefix, get_option('bindir'))
xf_datadir = join_paths(xf_prefix, get_option('datadir'))
xf_includedir = join_paths(xf_prefix, get_option('includedir'))
xf_libdir = join_paths(xf_prefix, get_option('libdir'))
xf_libexecdir = join_paths(xf_prefix, get_option('libexecdir'))
xf_localedir = join_paths(xf_prefix, get_option('localedir'))
xf_sysconfdir = join_paths(xf_prefix, get_option('sysconfdir'))
xf_mandir = join_paths(xf_prefix, get_option('mandir'))
xf_pkgdatadir = join_paths(xf_datadir, xf_name)

cc = meson.get_compiler('c')

xfw_config = configuration_data()
xfw_config.set('LIBXFCE4WINDOWING_VERSION_MAJOR', xf_version_major)
xfw_config.set('LIBXFCE4WINDOWING_VERSION_MINOR', xf_version_minor)
xfw_config.set('LIBXFCE4WINDOWING_VERSION_MICRO', xf_version_micro)
xfw_config.set('LIBXFCE4WINDOWINGUI_VERSION_MAJOR', xf_version_major)
xfw_config.set('LIBXFCE4WINDOWINGUI_VERSION_MINOR', xf_version_minor)
xfw_config.set('LIBXFCE4WINDOWINGUI_VERSION_MICRO', xf_version_micro)

config_h = configuration_data()

# information
config_h.set_quoted('PACKAGE_VERSION', xf_version)
config_h.set_quoted('VERSION', xf_version)
config_h.set_quoted('DATADIR', xf_datadir)
config_h.set_quoted('SYSCONFDIR', xf_sysconfdir)

# i18n
config_h.set_quoted('GETTEXT_PACKAGE', xf_name)
config_h.set('ENABLE_NLS', 1)
config_h.set_quoted('LOCALEDIR', xf_localedir)

# compiler and linker flags
cflags_ = [
  '-DHAVE_CONFIG_H',
  '-DLIBXFCE4WINDOWING_COMPILATION',
  '-DLIBXFCE4WINDOWINGUI_COMPILATION',
  '-DWNCK_I_KNOW_THIS_IS_UNSTABLE',
  '-I' + meson.project_build_root()
]
ldflags_ = []

cflags_ += cc.get_supported_arguments([
  '-Wno-missing-field-initializers',
  '-Wno-unused-parameter',
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Wformat', '-Wformat-security', '-Wformat-y2k',
  '-Winit-self', '-Wmissing-include-dirs',
  '-Wundef', '-Wredundant-decls',
  '-Wdeclaration-after-statement',
  '-Wnested-externs',
  '-Wold-style-definition',
  '-fstack-protector-strong',
])

add_project_arguments(cflags_, language: 'c')

srcdir = meson.current_source_dir()
root_inc = include_directories('.')

xf_debug = get_option('xf_debug')
if xf_debug == 'no'
  config_h.set('NDEBUG', 1)
  cflags_ += ['-DG_DISABLE_CAST_CHECKS', '-DG_DISABLE_ASSERT']
elif xf_debug == 'minimum'
  config_h.set('NDEBUG', 1)
elif xf_debug == 'yes'
  config_h.set('DEBUG', 1)
  cflags_ += ['-g', '-Wshadow']
elif xf_debug == 'full'
  config_h.set('DEBUG', 1)
  config_h.set('DEBUG_TRACE', 1)
  config_h.set('G_ENABLE_DEBUG', 1)
  cflags_ += ['-O0', '-g']
endif

# required headers
cc.check_header('ctype.h')
cc.check_header('errno.h')
cc.check_header('fcntl.h')
cc.check_header('math.h')
cc.check_header('pwd.h')
cc.check_header('signal.h')
cc.check_header('string.h')
cc.check_header('sys/param.h')
cc.check_header('sys/stat.h')
cc.check_header('sys/types.h')
cc.check_header('time.h')
cc.check_header('unistd.h')

if cc.has_function('bind_textdomain_codeset')
  config_h.set('HAVE_BIND_TEXTDOMAIN_CODESET', 1)
endif

fs = import('fs')
gnome = import('gnome')
i18n = import('i18n')
pc = import('pkgconfig')

glib_req = '>= 2.68.0'
gtk_req = '>= 3.24.0'
gdk_pixbuf_req = '>= 2.40.0'
libwnck_req = '>= 3.14'
wl_req = '>= 1.15'

glib_dep = dependency('glib-2.0', version: glib_req)
gobject_dep = dependency('gobject-2.0', version: glib_req)
gio_dep = dependency('gio-2.0', version: glib_req)
gio_unix_dep = dependency('gio-unix-2.0', version: glib_req)
gtk_dep = dependency('gtk+-3.0', version: gtk_req)
gdk_dep = dependency('gdk-3.0', version: gtk_req)
gdk_pixbuf_dep = dependency('gdk-pixbuf-2.0', version: gdk_pixbuf_req)

enable_x11 = false
gdk_x11_dep = dependency(
  'gdk-x11-3.0',
  version: gtk_req,
  required: get_option('x11')
)
libx11_dep = dependency(
  'x11',
  required: get_option('x11')
)
libwnck_dep = dependency(
  'libwnck-3.0',
  version: libwnck_req,
  required: get_option('x11')
)
if gdk_x11_dep.found() and libwnck_dep.found() and libx11_dep.found()
  enable_x11 = true
  config_h.set('ENABLE_X11', 1)
endif

enable_wayland = false
gdk_wayland_dep = dependency(
  'gdk-wayland-3.0',
  version: gtk_req,
  required: get_option('wayland'),
)
wl_scanner_dep = dependency(
  'wayland-scanner',
  version: wl_req,
  native: true,
  required: get_option('wayland'),
)
wl_client_dep = dependency(
  'wayland-client',
  version: wl_req,
  required: get_option('wayland'),
)
if gdk_wayland_dep.found() and wl_scanner_dep.found() and wl_client_dep.found()
  enable_wayland = true
  config_h.set('ENABLE_WAYLAND', 1)
endif

if not (enable_x11 or enable_wayland)
  error('At least one of windowing backend must be enabled')
endif

enable_gir = get_option('introspection')
enable_gtk_doc = get_option('gtk_doc')

subdir('po')
subdir('protocols')
subdir('libxfce4windowing')
subdir('libxfce4windowingui')
subdir('docs')

configure_file(
  output: 'config.h',
  configuration: config_h,
)

summary({
  'includedir': xf_includedir,
  'libdir': xf_libdir,
  'sysconfdir': xf_sysconfdir,
  },
  section: 'General',
)
summary({
  'X11 support': enable_x11,
  'Wayland support': enable_wayland,
  'Debug support': xf_debug,
  'GObject Introspection support': enable_gir,
  'Documentation': enable_gtk_doc,
  },
  section: 'Configuration',
)
