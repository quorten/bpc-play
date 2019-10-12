# Copyright (C) 2013 Andrew Makousky
#
# See the file "COPYING" for details.

all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean

install:
	mkdir -p bin lib
	$(MAKE) -C src install
	$(MAKE) -C scripts install

uninstall:
	rm -rf bin lib

dist: clean uninstall
	cd .. && tar -zcf asman-0.2.tar.gz asman-0.2
