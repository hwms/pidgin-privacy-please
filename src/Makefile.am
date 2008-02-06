pkgdir = \
	$(GAIM_LIBDIR)/gaim

GAIM_BLOCKY_SOURCES = \
	gaim-blocky.c

AM_CFLAGS = \
	$(st)

libgaim_blocky_la_LDFLAGS = \
	-module \
	-avoid-version \
	$(GAIM_LIBS)

pkg_LTLIBRARIES = \
	libgaim-blocky.la

libgaim_blocky_la_SOURCES = \
	$(GAIM_BLOCKY_SOURCES)

AM_CPPFLAGS = \
	$(DEBUG_CFLAGS) \
	$(GAIM_CFLAGS)
