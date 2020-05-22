all: xputpix bootgraph.o dmath

xputpix: xputpix.c bootgraph.h bootgraph.o
	cc xputpix.c bootgraph.o -lX11 -o $@

bootgraph.o: bootgraph.c bootgraph.h
	cc -Wmissing-prototypes -Wmissing-declarations -c $< -o $@

dmath: dmath.c
	cc $< -o $@

clean:
	rm -f xputpix bootgraph.o dmath
