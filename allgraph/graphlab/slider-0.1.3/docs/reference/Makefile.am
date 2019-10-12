## Process this file with automake to produce Makefile.in
## This file is in Public Domain.

EXTRA_DIST = abbrevs.txt Doxyfile html

SRC = ../../src

if MAINTAINER_MODE
html: Doxyfile $(SRC)/*.c $(SRC)/*.h
	doxygen
endif

doxygen: Doxyfile $(SRC)/*.c $(SRC)/*.h
	doxygen

maintainer-clean:
	rm -R -f html
