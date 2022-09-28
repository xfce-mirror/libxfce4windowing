# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:

lib_LTLIBRARIES = \
	libxfce4windowingui-0.la

libxfce4windowingui_0_includedir = $(includedir)/xfce4/libxfce4windowingui
libxfce4windowingui_0_headers = \
	libxfce4windowingui.h \
	xfw-window-actions-menu.h

libxfce4windowingui_0_include_HEADERS = \
	$(libxfce4windowingui_0_headers)

libxfce4windowingui_0_la_SOURCES = \
	$(libxfce4windowingui_0_headers) \
	xfw-window-actions-menu.c

libxfce4windowingui_0_la_CFLAGS = \
	-I$(top_srcdir) \
	-DLIBXFCE4WINDOWINGUI_COMPILATION \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	$(GOBJECT_CFLAGS) \
	$(GTK_CFLAGS)

libxfce4windowingui_0_la_LDFLAGS = \
	-version-info $(LIBXFCE4WINDOWINGUI_VERINFO) \
	-export-symbols-regex "^[^_].*" \
	-Wl,-no-undefined

libxfce4windowingui_0_la_LIBADD = \
	$(top_builddir)/libxfce4windowing/libxfce4windowing-0.la \
	$(GOBJECT_LIBS) \
	$(GTK_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	libxfce4windowingui-$(LIBXFCE4WINDOWING_VERSION_API).pc
