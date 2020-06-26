CFLAGS = -O3 -DNOPRINT

all: parseq serseq

parseq: parseq.c
	cc $(CFLAGS) -o $@ $<

serseq: serseq.c
	cc $(CFLAGS) -o $@ $<

bench:
	time ./parseq 5 3124999
	time ./serseq 100000000

clean:
	rm -f parseq serseq
