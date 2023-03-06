if ENABLE_WAYLAND

noinst_LTLIBRARIES = \
	libwayland-protocols.la

libwayland_protocols_built_sources = \
	ext-workspace-v1-20220919.c \
	ext-workspace-v1-20220919-client.h \
	wlr-foreign-toplevel-management-unstable-v1.c \
	wlr-foreign-toplevel-management-unstable-v1-client.h

nodist_libwayland_protocols_la_SOURCES = \
	$(libwayland_protocols_built_sources)

libwayland_protocols_la_CFLAGS = \
	$(WAYLAND_CLIENT_CFLAGS)

libwayland_protocols_la_LDFLAGS = \
	-Wl,-no-undefined

libwayland_protocols_la_LIBADD = \
	$(WAYLAND_CLIENT_LIBS)


%.c: %.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) private-code $< $@

%-client.h: %.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) client-header $< $@


CLEANFILES = \
	$(libwayland_protocols_built_sources)

BUILT_SOURCES = \
	$(libwayland_protocols_built_sources)

endif

EXTRA_DIST = \
	ext-workspace-v1-20220919.xml \
	wlr-foreign-toplevel-management-unstable-v1.xml

# required for make distcheck (gtk-doc)
dist-hook: all
