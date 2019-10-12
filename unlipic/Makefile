bin_PROGRAMS = fprec gate-asctobin gate-sim strip-comments mac-build
all: $(bin_PROGRAMS)

fprec: fprec.c
	cc -O3 -o $@ $< -lm

gate-asctobin: gate-asctobin.c
	cc -o $@ $<

gate-sim: gate-sim.c xmalloc.c
	cc -O3 -o $@ $^

strip-comments: strip-comments.c
	cc -o $@ $<

mac-build: mac-build.c xmalloc.c
	cc -o $@ $^

uart.bin: uart.asc
	./strip-comments <$< | ./gate-asctobin >$@

counter.bin: counter.asc
	./strip-comments <$< | ./gate-asctobin >$@

flipflop.bin: flipflop.asc
	./strip-comments <$< | ./gate-asctobin >$@

advcount.asc: strip-comments mac-build advcount.mac
	./strip-comments <advcount.mac | ./mac-build >advcount.asc

advcount.nand.bin advcount.or.bin: gate-asctobin advcount.asc
	./gate-asctobin NAND <advcount.asc >advcount.nand.bin
	./gate-asctobin OR <advcount.asc >advcount.or.bin

test: gate-sim advcount.nand.bin advcount.or.bin
	./gate-sim advcount.nand.bin advcount.or.bin

clean:
	rm -f $(bin_PROGRAMS)
	rm -f uart.bin counter.bin flipflop.bin
	rm -f advcount.asc advcount.nand.bin advcount.or.bin
