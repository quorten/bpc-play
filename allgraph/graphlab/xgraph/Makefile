all: xgraph

ivecmath.o: ivecmath.c ivecmath.h
	cc -c $<

xgraph.o: xgraph.c ivecmath.h
	cc -c $<

xgraph: xgraph.o ivecmath.o
	cc $^ -lX11 -o $@

clean:
	rm -f ivecmath.o xgraph.o xgraph
