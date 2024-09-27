project(
  'libxfce4windowing',
  'c',
  version: run_command(['./version.sh', 'print-version'], capture: true, check: true).stdout().strip(),
  license: 'GPL-2',
  meson_version: '>= 0.56.0',
  default_options: [
    'c_std=gnu99',
    'buildtype=debugoptimized',
    'warning_level=2',
  ]
)
meson.add_dist_script('./version.sh', 'rewrite-version', meson.project_version())

project_namespace = 'xfce'
pkgdatadir = get_option('datadir') / meson.project_name()

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')
gnome = import('gnome')
i18n = import('i18n')

dependency_versions = {
  'display-info': '>= 0.1.1',
  'glib': '>= 2.68.0',
  'gdk-pixbuf': '>= 2.40.0',
  'gtk': '>= 3.24.10',
  'libx11': '>= 1.6.7',
  'wayland': '>= 1.15',
  'wayland-protocols': '>= 1.18',
  'wlr-protocols': '>= 0',
  'wnck': '>= 3.14',
  'xrandr': '>= 1.5.0',
}

glib = dependency('glib-2.0', version: dependency_versions['glib'])
gobject = dependency('gobject-2.0', version: dependency_versions['glib'])
gio_unix = dependency('gio-unix-2.0', version: dependency_versions['glib'])
gdk = dependency('gdk-3.0', version: dependency_versions['gtk'])
gdk_pixbuf = dependency('gdk-pixbuf-2.0', version: dependency_versions['gdk-pixbuf'])
gtk = dependency('gtk+-3.0', version: dependency_versions['gtk'])

# Feature: 'x11'
display_info = dependency('libdisplay-info', version: dependency_versions['display-info'], required: get_option('x11'))
libx11 = dependency('x11', version: dependency_versions['libx11'], required: get_option('x11'))
gdk_x11 = dependency('gdk-x11-3.0', version: dependency_versions['gtk'], required: get_option('x11'))
wnck = dependency('libwnck-3.0', version: dependency_versions['wnck'], required: get_option('x11'))
xrandr = dependency('xrandr', version: dependency_versions['xrandr'], required: get_option('x11'))

# Feature: 'wayland'
gdk_wayland = dependency('gdk-wayland-3.0', version: dependency_versions['gtk'], required: get_option('wayland'))
wayland_client = dependency('wayland-client', version: dependency_versions['wayland'], required: get_option('wayland'))
wayland_scanner = dependency('wayland-scanner', version: dependency_versions['wayland'], required: get_option('wayland'))
wayland_protocols = dependency('wayland-protocols', version: dependency_versions['wayland-protocols'], required: get_option('wayland'))
wlr_protocols = dependency('wlr-protocols', version: dependency_versions['wlr-protocols'], required: false)

feature_cflags = []

enable_x11 = not get_option('x11').disabled() and \
  libx11.found() and \
  gdk_x11.found() and \
  wnck.found() and \
  display_info.found() and \
  xrandr.found()
x11_deps = []
if enable_x11
  feature_cflags += '-DENABLE_X11=1'
  x11_deps += [
    display_info,
    libx11,
    gdk_x11,
    wnck,
    xrandr,
  ]
endif

enable_wayland = not get_option('wayland').disabled() and \
  gdk_wayland.found() and \
  wayland_client.found() and \
  wayland_scanner.found() and
  wayland_protocols.found()
wayland_deps = []
if enable_wayland
  feature_cflags += '-DENABLE_WAYLAND=1'

  wayland_scanner_path = wayland_scanner.get_variable(pkgconfig: 'wayland_scanner')
  wayland_scanner_bin = find_program(wayland_scanner_path, required: true)

  wl_protocols_pkgdatadir = wayland_protocols.get_variable(pkgconfig: 'pkgdatadir')

  if wlr_protocols.found()
    wlr_protocols_pkgdatadir = wlr_protocols.get_variable(pkgconfig: 'pkgdatadir')
  else
    wlr_protocols_pkgdatadir = meson.project_source_root() / 'protocols' / 'wlr-protocols'
  endif

  wayland_deps += [
    gdk_wayland,
    wayland_client,
  ]
endif

if not enable_x11 and not enable_wayland
  error('At least one of the X11 and Wayland backends must be enabled')
endif

extra_cflags = []
extra_cflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wnested-externs',
  '-Wno-unused-parameter',
  '-Wno-declaration-after-statement',
  '-Werror=implicit-function-declaration',
]

buildtype = get_option('buildtype')
if buildtype == 'debug' or buildtype == 'debugoptimized'
  extra_cflags_check += '-fstack-protector-strong'
  extra_cflags += '-DDEBUG=1'

  if buildtype == 'debug'
    extra_cflags += [
      '-DG_ENABLE_DEBUG',
      '-DDEBUG_TRACE=1',
    ]
  endif
elif buildtype == 'release' or buildtype == 'minsize' or buildtype == 'plain'
  extra_cflags += '-DNDEBUG'

  if buildtype == 'minsize'
    extra_cflags += [
      '-DG_DISABLE_CAST_CHECKS',
      '-DG_DISABLE_ASSERT',
    ]
  endif
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

version_parts = meson.project_version().split('git')[0].split('.')

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DSHORT_VERSION="@0@.@1@"'.format(version_parts[0], version_parts[1]),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
]

git_rev = run_command('git', 'rev-parse', '--short', 'HEAD', capture: true, check: false).stdout().strip()
if git_rev == ''
  git_rev = 'UNKNOWN'
endif
extra_cflags += '-DREVISION="@0@"'.format(git_rev)

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(extra_cflags, language: 'c')
add_project_arguments(feature_cflags, language: 'c')

lib_so_version = '0'
lib_version = '@0@.0.0'.format(lib_so_version)

subdir('protocols')
subdir('libxfce4windowing')
subdir('libxfce4windowingui')
subdir('docs' / 'reference' / 'libxfce4windowing')
subdir('docs' / 'reference' / 'libxfce4windowingui')
subdir('po')
subdir('tests')
