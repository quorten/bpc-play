# Process this file with autoconf to produce a configure script.

# Copyright (C) 2011, 2012, 2013 Andrew Makousky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#   * Redistributions in binary form must reproduce the above
#     copyright notice, this list of conditions and the following
#     disclaimer in the documentation and/or other materials provided
#     with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

AC_INIT(slider, 0.1.3)
AC_CONFIG_AUX_DIR(build-aux)
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_HEADER_STDC

# Configure GTK+.
pkg_modules="gtk+-2.0 >= 2.0.0"
PKG_CHECK_MODULES(GTK, [$pkg_modules])
PACKAGE_CFLAGS="$PACKAGE_CFLAGS "'$(GTK_CFLAGS)'
PACKAGE_LIBS="$PACKAGE_LIBS "'$(GTK_LIBS)'

# Check for Microsoft Windows.
win32=no
case "${host_os}" in
  *mingw* | pw32* | cygwin*)
	win32="yes"
    AC_CHECK_TOOL(RC, windres, :)
    if test "${RC}" = ":"; then
      AC_MSG_FAILURE([Error: you need windres to compile this program.])
    fi
    PACKAGE_CFLAGS="$PACKAGE_CFLAGS -mms-bitfields"
esac
AM_CONDITIONAL(WITH_WIN32, test "x$win32" = "xyes")
WIN_VERSION=`echo $VERSION | $AWK '
BEGIN { FS = "." }
{
  for (i = 1; i <= 4; i++)
  {
    if (i > NF)
      printf "0"
    else
      printf "%s", $(i)
    if (i < 4)
      printf ","
    else
      printf "\n"
  }
}'`
AC_SUBST(WIN_VERSION)
AC_DEFINE_UNQUOTED(WIN_VERSION, $WIN_VERSION,
  [Version string for the Windows resource script.])

# Configure BinReloc.
AM_BINRELOC

# Configure Gettext.
GETTEXT_PACKAGE=slider
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

# Add the languages which your application supports here.
ALL_LINGUAS="es"

# Add maintainer information for the POT file here.
POT_COPYRIGHT_HOLDER="'Andrew Makousky'"
AC_SUBST(POT_COPYRIGHT_HOLDER)
POT_BUGS_ADDRESS="'andrew@home-dell-2300'"
AC_SUBST(POT_BUGS_ADDRESS)

AM_GLIB_GNU_GETTEXT

AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

AC_CONFIG_FILES([
Makefile
devcpp/config.h
docs/Makefile
docs/reference/Makefile
docs/research/Makefile
msvc/config.h
packaging/Makefile
packaging/Property.idt
po/Makefile.in
src/Makefile
])
AC_OUTPUT
