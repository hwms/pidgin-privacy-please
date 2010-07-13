EXTRA_DIST = \
	Makefile.mingw

pkgdir = \
	$(PIDGIN_LIBDIR)/pidgin

PIDGIN_PP_SOURCES = \
	auto-reply.c auto-reply.h \
	botcheck.h botcheck.c \
	pp-prefs.h pp-prefs.c \
	pidgin-pp.c

AM_CFLAGS = \
	$(st)

libpidgin_pp_la_LDFLAGS = \
	-module \
	-avoid-version \
	$(PIDGIN_LIBS)

pkg_LTLIBRARIES = \
	libpidgin_pp.la

libpidgin_pp_la_SOURCES = \
	$(PIDGIN_PP_SOURCES)

AM_CPPFLAGS = \
	$(DEBUG_CFLAGS) \
	$(PIDGIN_CFLAGS) \
	-DLOCALEDIR=\"$(LIBPURPLE_DATADIR)/locale\"
