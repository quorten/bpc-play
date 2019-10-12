## Process this file with automake to produce Makefile.in

SUBDIRS = glib

EXTRA_DIST = exparray.gdb fixme old \
	main.c winmain.c timain.c

INCLUDES = \
	@PACKAGE_CFLAGS@

lib_LIBRARIES = libemt.a

nobase_include_HEADERS = bool.h null.h \
	xmalloc.h \
	exparray.h gawrapper.h \
	lnlist.h hooks.h

libemt_a_SOURCES = \
	core.h \
	null.h bool.h \
	exparray.h gawrapper.h \
	hooks.c hooks.h \
	lnlist.c lnlist.h \
	xmalloc.c xmalloc.h
