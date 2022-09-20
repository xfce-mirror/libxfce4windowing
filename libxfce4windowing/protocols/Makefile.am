if WAYLAND_ENABLED

noinst_LTLIBRARIES = \
	libwayland-protocols.la

libwayland_protocols_built_sources = \
	ext-workspace-v1-20220919.c \
	ext-workspace-v1-20220919-client.h

libwayland_protocols_la_SOURCES = \
	$(libwayland_protocols_built_sources)

libwayland_protocols_la_CFLAGS = \
	$(WAYLAND_CLIENT_CFLAGS)

libwayland_protocols_la_LDFLAGS = \
	-no-undefined

libwayland_protocols_la_LIBADD = \
	$(WAYLAND_CLIENT_LIBS)


%.c: %.xml
	$(AM_V_GEN) wayland-scanner private-code $< $@

%-client.h: %.xml
	$(AM_V_GEN) wayland-scanner client-header $< $@


CLEANFILES = \
	$(libwayland_protocols_built_sources)

BUILT_SOURCES = \
	$(libwayland_protocols_built_sources)

endif

EXTRA_DIST = \
	ext-workspace-v1-20220919.xml
