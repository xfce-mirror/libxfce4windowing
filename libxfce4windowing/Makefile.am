# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:

SUBDIRS = \
	protocols

lib_LTLIBRARIES = libxfce4windowing.la

libxfce4windowing_includedir = $(includedir)/xfce4/libxfce4windowing
libxfce4windowing_headers = \
	libxfce4windowing.h \
	xfw-screen.h \
	xfw-util.h \
	xfw-window.h \
	xfw-workspace-group.h \
	xfw-workspace-manager.h \
	xfw-workspace.h

libxfce4windowing_include_HEADERS = \
	$(libxfce4windowing_headers)

libxfce4windowing_built_sources = \
	xfw-marshal.c \
	xfw-marshal.h

libxfce4windowing_x11_sources = \
	xfw-screen-x11.c \
	xfw-screen-x11.h \
	xfw-window-x11.c \
	xfw-window-x11.h \
	xfw-workspace-manager-x11.c \
	xfw-workspace-manager-x11.h \
	xfw-workspace-x11.c \
	xfw-workspace-x11.h

libxfce4windowing_wayland_sources = \
	xfw-screen-wayland.c \
	xfw-screen-wayland.h \
	xfw-window-wayland.c \
	xfw-window-wayland.h \
	xfw-workspace-group-wayland.c \
	xfw-workspace-group-wayland.h \
	xfw-workspace-manager-wayland.c \
	xfw-workspace-manager-wayland.h \
	xfw-workspace-wayland.c \
	xfw-workspace-wayland.h

libxfce4windowing_la_SOURCES = \
	$(libxfce4windowing_headers) \
	$(libxfce4windowing_built_sources) \
	libxfce4windowing-private.h \
	xfw-screen.c \
	xfw-util.c \
	xfw-window.c \
	xfw-workspace-dummy.c \
	xfw-workspace-dummy.h \
	xfw-workspace-group-dummy.c \
	xfw-workspace-group-dummy.h \
	xfw-workspace-group.c \
	xfw-workspace-manager-dummy.c \
	xfw-workspace-manager-dummy.h \
	xfw-workspace-manager.c \
	xfw-workspace.c

if X11_ENABLED
libxfce4windowing_la_SOURCES += \
	$(libxfce4windowing_x11_sources)
endif

if WAYLAND_ENABLED
libxfce4windowing_la_SOURCES += \
	$(libxfce4windowing_wayland_sources)
endif

libxfce4windowing_la_CFLAGS = \
	-I$(top_srcdir) \
	-DLIBXFCE4WINDOWING_COMPILATION \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	$(GOBJECT_CFLAGS) \
	$(GDK_CFLAGS) \
	$(LIBWNCK_CFLAGS) \
	$(WAYLAND_CLIENT_CFLAGS)

libxfce4windowing_la_LDFLAGS = \
	-version-info $(LIBXFCE4WINDOWING_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-Wl,-no-undefined

libxfce4windowing_la_LIBADD = \
	$(GOBJECT_LIBS) \
	$(GDK_LIBS) \
	$(LIBWNCK_LIBS) \
	$(WAYLAND_CLIENT_LIBS)

if WAYLAND_ENABLED
libxfce4windowing_la_LIBADD += \
	protocols/libwayland-protocols.la
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	libxfce4windowing.pc

EXTRA_DIST = \
	xfw-marshal.c \
	xfw-marshal.h \
	xfw-marshal.list

if MAINTAINER_MODE

xfw-marshal.h: $(top_srcdir)/libxfce4windowing/xfw-marshal.list Makefile
	$(AM_V_GEN) glib-genmarshal --prefix=xfw_marshal --internal --header $< > $@

xfw-marshal.c: $(top_srcdir)/libxfce4windowing/xfw-marshal.list Makefile
	$(AM_V_GEN) echo '#include "xfw-marshal.h"' > $@
	$(AM_V_GET) glib-genmarshal --prefix=xfw_marshal --body $< >> $@

CLEANFILES = \
	$(libxfce4windowing_built_sources)

BUILT_SOURCES = \
	$(libxfce4windowing_built_sources)

endif
