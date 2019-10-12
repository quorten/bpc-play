TARGET =
CC = $(TARGET)gcc
LD = $(TARGET)ld
OBJCOPY = $(TARGET)objcopy
ADD_CFLAGS = -g # -m32 OR -mx32
CFLAGS = -I libc/include $(ADD_CFLAGS)
# NOTE: These CFLAGS and LDFLAGS are mostly setup to disable system
# default compiler flags and libraries that tend to add a lot of bloat
# to simple executables.
CFLAGS += -fno-stack-protector -fno-asynchronous-unwind-tables
ADD_LDFLAGS = # -m elf_i386 OR elf32_x86_64
LDFLAGS = --exclude-libs ALL $(ADD_LDFLAGS)

all: example2 example2.debug

example2.o: example2.c
	$(CC) $(CFLAGS) -c $<

libc/crt0.o libc/libc.a:
	$(MAKE) -C libc

example2: example2.o libc/libc.a
	$(LD) $(LDFLAGS) -e main -o $@ $^

example2.debug: example2
	$(OBJCOPY) --only-keep-debug $< $@
	$(OBJCOPY) --strip-debug $<
	$(OBJCOPY) --add-gnu-debuglink $@ $<
	chmod -x $@
	touch $@

example2.txt: example2
	objdump $< -x -s -S >$@

randsym.o: randsym.c
	$(CC) $(CFLAGS) -c $<

randsym: libc/crt0.o randsym.o libc/libc.a
	$(LD) $(LDFLAGS) -o $@ $^
	$(OBJCOPY) --strip-debug $@

bmon.o: bmon.c
	$(CC) $(CFLAGS) -c $<

bmonmem.o: bmonmem.s
	$(CC) $(CFLAGS) -c $<

bmonend.o: bmonend.s
	$(CC) $(CFLAGS) -c $<

bmon: bmonmem.o bmon.o libc/libc.a
	# gcc ... -z execstack
	$(LD) $(LDFLAGS) -N -e main -o $@ $^

mostlyclean:
	rm -f example2.o randsym.o randsym bmon.o bmonmem.o
	$(MAKE) -C libc mostlyclean

clean: mostlyclean
	rm -f example2 example2.debug example2.txt bmon
	$(MAKE) -C libc clean

dist: clean
	mkdir example2/
	cp -pR Makefile example2.c randsym.c libc example2/
	tar -zcf example2.tar.gz example2/
	rm -rf example2/
