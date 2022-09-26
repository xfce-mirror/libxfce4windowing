@SET_MAKE@

SUBDIRS = \
	libxfce4windowing \
	docs \
	po

EXTRA_DIST = \
	README.md \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache.lock

distuninstallcheck_listfiles = \
	find . -type f -print | grep -v ./share/icons/hicolor/icon-theme.cache

distclean-local:
	-rm -rf *.cache *~

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

compile_flags.txt: $(shell find . -name Makefile)
	rm -f $@
	for flag in -DLIBXFCE4WINDOWING_COMPILATION -DWNCK_I_KNOW_THIS_IS_UNSTABLE -I$(top_srcdir) -I$(top_srcdir)/libxfce4windowing -I$(top_builddir)/libxfce4windowing $(GOBJECT_CFLAGS) $(GDK_CFLAGS) $(LIBWNCK_CFLAGS) $(WAYLAND_CLIENT_CFLAGS); do \
		echo "$$flag" >> $@ || exit 1; \
	done
