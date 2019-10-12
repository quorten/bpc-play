## Process this file with automake to produce Makefile.in

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

SUBDIRS = docs packaging po src

EXTRA_DIST = \
	BUGS \
	HACKING \
	TRANSLATORS \
	autogen.sh \
	design.glade \
	design.gladep \
	devcpp/config.h \
	devcpp/config.h.in \
	devcpp/slider.dev \
	devcpp/slider_private.h \
	devcpp/slider_private.rc \
	m4macros/binreloc.m4 \
	msvc/config.h \
	msvc/config.h.in \
	msvc/slider.dsp \
	msvc/slider.dsw \
	msvc/slider.exe.config \
	msvc/slider.manifest \
	msvc/slider.sln \
	msvc/slider.vcproj \
	old-plans/Readme.txt \
	old-plans/layout.svg \
	po/README.translators \
	svgs/README \
	svgs/install-header.bmp \
	svgs/install-header.svg \
	svgs/app.ico \
	svgs/app_legacy.ico \
	svgs/app.svg \
	svgs/icons.svg

docdir = $(datadir)/doc/@PACKAGE@
doc_DATA = BUGS NEWS COPYING README THANKS TODO

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

