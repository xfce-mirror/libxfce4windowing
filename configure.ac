dnl
dnl Copyright (c) 2022
dnl         The Xfce development team. All rights reserved.
dnl
dnl libxfce4windowing -- utilities for handling different windowing environments
dnl

dnl version info
m4_define([libxfce4windowing_verinfo], [0:0:0])
m4_define([libxfce4windowing_version_api], [0])
m4_define([libxfce4windowingui_verinfo], [0:0:0])
m4_define([libxfce4windowingui_version_api], [0])
XDT_VERSION_INIT([4.19.1], [git])

dnl minimum required versions
m4_define([glib_minimum_version], [2.68.0])
m4_define([gtk_minimum_version], [3.24.0])
m4_define([gdk_pixbuf_minimum_version], [2.40.0])
m4_define([wnck_minimum_version], [3.14])

dnl init autoconf
AC_INIT([libxfce4windowing], [xdt_version], [https://gitlab.xfce.org/xfce/libxfce4windowing])
AC_PREREQ([2.69])
AC_CONFIG_MACRO_DIRS([m4])

dnl init automake
AM_INIT_AUTOMAKE([1.11 dist-bzip2 tar-ustar no-dist-gzip foreign])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE()
AM_SILENT_RULES([yes])

dnl check for UNIX variants
AC_USE_SYSTEM_EXTENSIONS

dnl check for basic programs
AC_PROG_CC()
LT_PATH_LD([])
AM_PROG_CC_C_O()
AC_PROG_INSTALL()

dnl prepare libtool
LT_PREREQ([2.4])
LT_INIT([disable-static])

dnl version info
LIBXFCE4WINDOWING_VERINFO=libxfce4windowing_verinfo()
LIBXFCE4WINDOWING_VERSION_API=libxfce4windowing_version_api()
LIBXFCE4WINDOWING_VERSION_MAJOR=xdt_version_major()
LIBXFCE4WINDOWING_VERSION_MINOR=xdt_version_minor()
LIBXFCE4WINDOWING_VERSION_MICRO=xdt_version_micro()
LIBXFCE4WINDOWINGUI_VERINFO=libxfce4windowingui_verinfo()
LIBXFCE4WINDOWINGUI_VERSION_API=libxfce4windowingui_version_api()
LIBXFCE4WINDOWINGUI_VERSION_MAJOR=xdt_version_major()
LIBXFCE4WINDOWINGUI_VERSION_MINOR=xdt_version_minor()
LIBXFCE4WINDOWINGUI_VERSION_MICRO=xdt_version_micro()
AC_SUBST([LIBXFCE4WINDOWING_VERINFO])
AC_SUBST([LIBXFCE4WINDOWING_VERSION_API])
AC_SUBST([LIBXFCE4WINDOWING_VERSION_MAJOR])
AC_SUBST([LIBXFCE4WINDOWING_VERSION_MINOR])
AC_SUBST([LIBXFCE4WINDOWING_VERSION_MICRO])
AC_SUBST([LIBXFCE4WINDOWINGUI_VERINFO])
AC_SUBST([LIBXFCE4WINDOWINGUI_VERSION_API])
AC_SUBST([LIBXFCE4WINDOWINGUI_VERSION_MAJOR])
AC_SUBST([LIBXFCE4WINDOWINGUI_VERSION_MINOR])
AC_SUBST([LIBXFCE4WINDOWINGUI_VERSION_MICRO])

dnl check for standard header files
AC_CHECK_HEADERS([ctype.h errno.h fcntl.h math.h pwd.h signal.h \
                  string.h sys/param.h sys/stat.h  \
                  sys/types.h time.h \
                  unistd.h])
AC_CHECK_FUNCS([bind_textdomain_codeset])

GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl required
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GOBJECT], [gobject-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GIO_UNIX], [gio-unix-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GDK], [gdk-3.0], [gtk_minimum_version])
XDT_CHECK_PACKAGE([GDK_PIXBUF], [gdk-pixbuf-2.0], [gdk_pixbuf_minimum_version])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [gtk_minimum_version])

GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
GOBJECT_PREFIX="`$PKG_CONFIG --variable=prefix gobject-2.0`"
GDK_PREFIX="`$PKG_CONFIG --variable=prefix gdk-3.0`"
GDK_PIXBUF_PREFIX="`$PKG_CONFIG --variable=prefix gdk-pixbuf-2.0`"
GTK_PREFIX="`$PKG_CONFIG --variable=prefix gtk+-3.0`"

AC_SUBST([GLIB_PREFIX])
AC_SUBST([GOBJECT_PREFIX])
AC_SUBST([GDK_PREFIX])
AC_SUBST([GDK_PIXBUF_PREFIX])
AC_SUBST([GTK_PREFIX])

AC_ARG_ENABLE([x11],
              AS_HELP_STRING([--disable-x11], [Whether to build support for X11 (default=auto)]),
              [enable_x11=$enableval],
              [enable_x11=auto])
XDT_CHECK_LIBX11()
XDT_CHECK_OPTIONAL_PACKAGE([GDK_X11], [gdk-x11-3.0], [gtk_minimum_version],
                           [gdk-x11], [X11 backend for GDK])
XDT_CHECK_OPTIONAL_PACKAGE([LIBWNCK], [libwnck-3.0], [wnck_minimum_version],
                           [libwnck], [X11 WNCK library])

if test x"$enable_x11" != x"no"; then
    if test x"$GDK_X11_FOUND" = x"yes" -a x"$LIBWNCK_FOUND" = x"yes"; then
        enable_x11="yes"
        AC_DEFINE([ENABLE_X11], [1], [Set if X11 support is enabled])
    elif test x"$enable_x11" = x"yes"; then
        AC_MSG_ERROR([X11 support was requested, but required dependencies were not found])
    else
        enable_x11="no"
    fi
fi
AM_CONDITIONAL([X11_ENABLED], [test x"$enable_x11" = x"yes"])

AC_ARG_ENABLE([wayland],
              AS_HELP_STRING([--disable-wayland], [Whether to build support for Wayland (default=auto)]),
              [enable_wayland=$enableval],
              [enable_wayland=auto])
XDT_CHECK_OPTIONAL_PACKAGE([GDK_WAYLAND], [gdk-wayland-3.0], [gtk_minimum_version],
                           [gdk-wayland], [Wayland backend for GDK])
XDT_CHECK_OPTIONAL_PACKAGE([WAYLAND_SCANNER], [wayland-scanner], [1.15],
                           [wayland-scanner], [Wayland protocol code generator])
XDT_CHECK_OPTIONAL_PACKAGE([WAYLAND_CLIENT], [wayland-client], [1.15],
                           [wayland-client], [Wayland client library])

if test x"$enable_wayland" != x"no"; then
    if test x"$GDK_WAYLAND_FOUND" = x"yes" -a x"$WAYLAND_SCANNER_FOUND" = x"yes" -a x"$WAYLAND_CLIENT_FOUND" = x"yes"; then
        enable_wayland="yes"
        AC_DEFINE([ENABLE_WAYLAND], [1], [Set if Wayland support is enabled])
    elif test x"$enable_wayland" = x"yes"; then
        AC_MSG_ERROR([Wayland support was requested, but required dependencies were not found])
    else
        enable_wayland="no"
    fi
fi
AM_CONDITIONAL([WAYLAND_ENABLED], [test x"$enable_wayland" = x"yes"])

if test x"$enable_x11" = x"no" -a x"$enable_wayland" = x"no"; then
    AC_MSG_ERROR([At least one windowing backend must be enabled])
fi

GTK_DOC_CHECK([1.30])

GOBJECT_INTROSPECTION_CHECK([1.66.0])

dnl check for debugging support
XDT_FEATURE_DEBUG()
XDT_FEATURE_LINKER_OPTS()

dnl ****************************************
dnl *** Check for ELF visibility support ***
dnl ****************************************
AC_ARG_ENABLE([visibility],
              AS_HELP_STRING([--disable-visibility],
                             [Do not use ELF visibility attributes]),
              [enable_visibility=$enableval], [enable_visibility=yes])
have_gnuc_visibility=no
if test "x$enable_visibility" != "xno"; then
  XDT_SUPPORTED_FLAGS([xdt_vis_test_cflags], [-Wall -Werror -Wno-unused-parameter])
  saved_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $xdt_vis_test_cflags"
  AC_MSG_CHECKING([whether $CC supports the GNUC visibility attribute])
  AC_COMPILE_IFELSE([AC_LANG_SOURCE(
  [
    void test_default (void);
    void test_hidden (void);
    void __attribute__ ((visibility("default"))) test_default (void) {}
    void __attribute__ ((visibility("hidden"))) test_hidden (void) {}
    int main (int argc, char **argv) {
      test_default ();
      test_hidden ();
      return 0;
    }
  ])],
  [
    have_gnuc_visibility=yes
    AC_MSG_RESULT([yes])
  ],
  [
    AC_MSG_RESULT([no])
  ])
  CFLAGS="$saved_CFLAGS"
fi
if test "x$have_gnuc_visibility" = "xyes"; then
  CPPFLAGS="$CPPFLAGS -DHAVE_GNUC_VISIBILITY"
fi
AM_CONDITIONAL([HAVE_GNUC_VISIBILITY], [test "x$have_gnuc_visibility" = "xyes"])

AC_CONFIG_FILES([
Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/libxfce4windowing/Makefile
docs/reference/libxfce4windowingui/Makefile
libxfce4windowing/Makefile
libxfce4windowing/libxfce4windowing-0.pc
libxfce4windowing/libxfce4windowing-config.h
libxfce4windowingui/Makefile
libxfce4windowingui/libxfce4windowingui-0.pc
libxfce4windowingui/libxfce4windowingui-config.h
po/Makefile.in
protocols/Makefile
])
AC_OUTPUT

echo
echo "Build Configuration:"
echo "* X11 support:                   $enable_x11"
echo "* Wayland support:               $enable_wayland"
echo "* Debug support:                 $enable_debug"
echo "* GObject Introspection support: $enable_introspection"
echo "* Documentation support:         $enable_gtk_doc"
echo