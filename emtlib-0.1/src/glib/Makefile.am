## Process this file with automake to produce Makefile.in

INCLUDES = \
	@PACKAGE_CFLAGS@ -I..

lib_LIBRARIES = libglib-2.0.a

glibincludedir = $(includedir)/glib-2.0
glibinclude_HEADERS = glib.h
glibsubincludedir = $(includedir)/glib-2.0/glib
glibsubinclude_HEADERS = \
	garray.h gmem.h gqsort.h gslice.h \
	gstring.h gtypes.h gmacros.h

libglib_2_0_a_SOURCES = \
	garray.c garray.h \
	gmem.c gmem.h \
	gqsort.c gqsort.h \
	gslice.c gslice.h \
	gstring.c gstring.h \
	gtypes.h gmacros.h
