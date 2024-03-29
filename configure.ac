#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([pidgin-privacy-please],[0.7.1],[stefan@ott.net])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE([1.9.5])

REQUIRED_PKG_CONFIG="0.15.0"
AC_SUBST(REQUIRED_PKG_CONFIG)

REQUIRED_PIDGIN="pidgin >= 2.5 pidgin < 3.0"
AC_SUBST(REQUIRED_PIDGIN)

AC_CONFIG_SRCDIR([src/pidgin-pp.c])
AC_CONFIG_HEADER([config.h])

AC_DEFINE(PIDGIN_PLUGINS, 1, [Define if plugins are enabled.])

# Checks for programs.
AC_PROG_CC
AC_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"
AC_PROG_INSTALL
AC_PATH_PROG(PKG_CONFIG, [pkg-config], [no])

LIBPURPLE_DATADIR=`pkg-config --variable=datadir purple`
AC_SUBST(LIBPURPLE_DATADIR)

#
# Setup gettext
#
AC_PROG_INTLTOOL
GETTEXT_PACKAGE=pidgin-privacy-please
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, ["$GETTEXT_PACKAGE"], [Define the gettext package to be used])
ALL_LINGUAS="de ru pl"
AM_GLIB_GNU_GETTEXT

if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found])
fi

# Checks for libraries.
PKG_PROG_PKG_CONFIG([$REQUIRED_PKG_CONFIG])
PKG_CHECK_MODULES([PIDGIN], [$REQUIRED_PIDGIN],
	[
	AC_SUBST(PIDGIN_CFLAGS)
	AC_SUBST(PIDGIN_LIBS)
	]
)
AC_ARG_VAR([PIDGIN_DATADIR], [datadir of PIDGIN, overriding pkg-config])dnl
AC_ARG_VAR([PIDGIN_LIBDIR], [libdir of PIDGIN, overriding pkg-config])dnl
AC_CACHE_CHECK([for PIDGIN][_DATADIR], [pkg_cv_][PIDGIN][_DATADIR],
	[_PKG_CONFIG([PIDGIN][_DATADIR], [variable=datadir], [pidgin])])
AC_CACHE_CHECK([for PIDGIN][_LIBDIR], [pkg_cv_][PIDGIN][_LIBDIR],
	[_PKG_CONFIG([PIDGIN][_LIBDIR], [variable=libdir], [pidgin])])
PIDGIN[]_DATADIR=$pkg_cv_[]PIDGIN[]_DATADIR
PIDGIN[]_LIBDIR=$pkg_cv_[]PIDGIN[]_LIBDIR
AC_SUBST(PIDGIN_DATADIR)
AC_SUBST(PIDGIN_LIBDIR)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

# AM_CONFIG_HEADER

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([gethostname inet_ntoa memmove strcasecmp strchr strdup strerror strpbrk strrchr strstr strtoul])

case "$host" in
	*-*-mingw*)
	CFLAGS="$CFLAGS -O2 -Wall -mno-cygwin -mms-bitfields"
	LIBS="$LIBS -no-undefined -Wl,--enable-auto-image-base -lws2_32"
	;;
	*)
	;;
esac

if test "x$GCC" = "xyes"; then
	CFLAGS="$CFLAGS -Wall -g3"
fi
AC_SUBST(CFLAGS)


AC_CONFIG_FILES([Makefile
		src/Makefile
		po/Makefile.in])
AC_OUTPUT

echo
echo $PACKAGE $VERSION
echo
echo Installation directory........... : $PIDGIN_LIBDIR/pidgin
echo
echo configure complete, now type \'make\'
echo
