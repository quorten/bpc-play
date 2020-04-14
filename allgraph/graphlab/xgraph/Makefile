all: xgraph

ivecmath.o: ivecmath.c ivecmath.h
	cc -c $<

xgraph: xgraph.c ivecmath.o
	cc $^ -lX11 -o $@

clean:
	rm -f ivecmath.o xgraph
