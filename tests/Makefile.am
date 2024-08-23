noinst_PROGRAMS = \
	xfw-enum-monitors \
	xfw-enum-windows

tests_cflags = \
	-I$(top_srcdir) \
	$(GTK_CFLAGS)

tests_ldadd = \
	$(GTK_LIBS) \
	$(top_builddir)/libxfce4windowing/libxfce4windowing-0.la

xfw_enum_monitors_SOURCES = xfw-enum-monitors.c
xfw_enum_monitors_CFLAGS = $(test_cflags)
xfw_enum_monitors_LDADD = $(tests_ldadd)

xfw_enum_windows_SOURCES = xfw-enum-windows.c
xfw_enum_windows_CFLAGS = $(test_cflags)
xfw_enum_windows_LDADD = $(tests_ldadd)
