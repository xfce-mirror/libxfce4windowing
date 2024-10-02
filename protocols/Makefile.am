if ENABLE_WAYLAND

noinst_LTLIBRARIES = \
	libwayland-protocols.la

libwayland_protocols_built_sources = \
	ext-workspace-v1-20230427.c \
	ext-workspace-v1-20230427-client.h \
	wlr-foreign-toplevel-management-unstable-v1.c \
	wlr-foreign-toplevel-management-unstable-v1-client.h \
	xdg-output-unstable-v1.c \
	xdg-output-unstable-v1-client.h

nodist_libwayland_protocols_la_SOURCES = \
	$(libwayland_protocols_built_sources)

libwayland_protocols_la_CFLAGS = \
	$(WAYLAND_CLIENT_CFLAGS)

libwayland_protocols_la_LDFLAGS = \
	-Wl,-no-undefined

libwayland_protocols_la_LIBADD = \
	$(WAYLAND_CLIENT_LIBS)

xdg-output-unstable-v1.c: $(WL_PROTOCOLS_PKGDATADIR)/unstable/xdg-output/xdg-output-unstable-v1.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) private-code $< $@

xdg-output-unstable-v1-client.h: $(WL_PROTOCOLS_PKGDATADIR)/unstable/xdg-output/xdg-output-unstable-v1.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) client-header $< $@

%.c: $(WLR_PROTOCOLS_PKGDATADIR)/unstable/%.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) private-code $< $@

%-client.h: $(WLR_PROTOCOLS_PKGDATADIR)/unstable/%.xml
	$(AM_V_GEN) $(WAYLAND_SCANNER) client-header $< $@

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
	ext-workspace-v1-20230427.xml \
	meson.build \
	wlr-protocols/unstable/wlr-foreign-toplevel-management-unstable-v1.xml

# required for make distcheck (gtk-doc)
dist-hook: all
