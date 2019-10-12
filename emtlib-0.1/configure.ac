# Process this file with autoconf to produce a configure script.

AC_INIT(emtlib, 0.1)
AC_CONFIG_AUX_DIR(build-aux)
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE(src/ansi2knr)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_HEADER_STDC
AM_C_PROTOTYPES
AC_PROG_RANLIB

win32=no
case "${host_os}" in
  *mingw* | pw32* | cygwin*)
	win32="yes"
    PACKAGE_CFLAGS="$PACKAGE_CFLAGS -mms-bitfields"
esac
AM_CONDITIONAL(WITH_WIN32, test x"$win32" = "xyes")
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AC_CONFIG_FILES([
Makefile
src/Makefile
src/glib/Makefile
tests/Makefile
])
AC_OUTPUT
