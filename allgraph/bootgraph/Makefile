all: xputpix bootgraph.o

xputpix: xputpix.c bootgraph.o
	cc xputpix.c bootgraph.o -lX11 -o $@

bootgraph.o: bootgraph.c
	cc -c $< -o $@

clean:
	rm -f xputpix bootgraph.o
