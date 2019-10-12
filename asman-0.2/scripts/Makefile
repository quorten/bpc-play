# Copyright (C) 2013 Andrew Makousky
#
# See the file "COPYING" in the top level directory for details.

dist_bin_SCRIPTS = dq-escape du-accum nfop prepsum timsave
dist_pkgdata_DATA = archsys.mak hupdate.sh comments.sed sed-escape.sed	\
	as-config.mak

all:

install:
	install $(dist_bin_SCRIPTS) ../bin
	cp -p $(dist_pkgdata_DATA) ../lib

uninstall:
	cd ../bin && rm -f $(dist_bin_SCRIPTS)
	cd ../lib && rm -f $(dist_pkgdata_DATA)
