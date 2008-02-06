#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([gaim-blocky],[0.2.0],[stefan@desire.ch])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE([1.9.5])

REQUIRED_PKG_CONFIG="0.15.0"
AC_SUBST(REQUIRED_PKG_CONFIG)

REQUIRED_GAIM="gaim >= 1.2 gaim < 3.0"
AC_SUBST(REQUIRED_GAIM)

AC_CONFIG_SRCDIR([src/gaim-blocky.c])
AC_CONFIG_HEADER([config.h])

AC_DEFINE(GAIM_PLUGINS, 1, [Define if plugins are enabled.])

# Checks for programs.
AC_PROG_CC
AC_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
LIBTOOL="$LIBTOOL --silent"
AC_PROG_INSTALL
AC_PATH_PROG(PKG_CONFIG, [pkg-config], [no])

if test x$PKG_CONFIG = xno ; then
  AC_MSG_ERROR([*** pkg-config not found])
fi

# Checks for libraries.
PKG_PROG_PKG_CONFIG([$REQUIRED_PKG_CONFIG])
PKG_CHECK_MODULES([GAIM], [$REQUIRED_GAIM],
	[
	AC_SUBST(GAIM_CFLAGS)
	AC_SUBST(GAIM_LIBS)
	]
)
AC_ARG_VAR([GAIM_DATADIR], [datadir of GAIM, overriding pkg-config])dnl
AC_ARG_VAR([GAIM_LIBDIR], [libdir of GAIM, overriding pkg-config])dnl
AC_CACHE_CHECK([for GAIM][_DATADIR], [pkg_cv_][GAIM][_DATADIR],
	[_PKG_CONFIG([GAIM][_DATADIR], [variable=datadir], [gaim])])
AC_CACHE_CHECK([for GAIM][_LIBDIR], [pkg_cv_][GAIM][_LIBDIR],
	[_PKG_CONFIG([GAIM][_LIBDIR], [variable=libdir], [gaim])])
GAIM[]_DATADIR=$pkg_cv_[]GAIM[]_DATADIR
GAIM[]_LIBDIR=$pkg_cv_[]GAIM[]_LIBDIR
AC_SUBST(GAIM_DATADIR)
AC_SUBST(GAIM_LIBDIR)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

AM_CONFIG_HEADER

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
                 src/Makefile])
AC_OUTPUT

echo
echo $PACKAGE $VERSION
echo
echo Installation directory........... : $GAIM_LIBDIR/gaim
echo
echo configure complete, now type \'make\'
echo
