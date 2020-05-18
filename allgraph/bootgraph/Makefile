all: xputpix bootgraph.o

xputpix: xputpix.c
	cc $< -lX11 -o $@

bootgraph.o: bootgraph.c
	cc -c $< -o $@

clean:
	rm -f xputpix bootgraph.o
