# See the COPYING file for license details.
OS = linux
ARCH = i386
TARGET =
CC = $(TARGET)gcc
ADD_CFLAGS = -g # -m32
CFLAGS = -DOS_LINUX $(ADD_CFLAGS)
AR = $(TARGET)ar
RANLIB = $(TARGET)ranlib
O = .o
A = .a
LOBJS = errno$(O) _exit$(O) read$(O) write$(O) brk$(O) sbrk$(O) \
	time$(O) \
	exit$(O) write_all$(O) getchar$(O) putchar$(O) abort$(O) \
	x_puts$(O) puts$(O) atoi$(O) rand$(O)

# NOTE: These flags are mostly setup to disable system default
# compiler flags and libraries that tend to add a lot of bloat to
# simple executables.
CFLAGS += -fno-stack-protector -fno-asynchronous-unwind-tables

all: crt0$(O) libc$(A)

crt0$(O): sysc/$(OS)/$(ARCH)/crt0.c include/stdlib.h
	$(CC) $(CFLAGS) -o $@ -c $<

errno$(O): sysc/errno.c
	$(CC) $(CFLAGS) -o $@ -c $<

_exit$(O): sysc/$(OS)/$(ARCH)/_exit.c
	$(CC) $(CFLAGS) -o $@ -c $<

read$(O): sysc/$(OS)/$(ARCH)/read.c
	$(CC) $(CFLAGS) -o $@ -c $<

write$(O): sysc/$(OS)/$(ARCH)/write.c
	$(CC) $(CFLAGS) -o $@ -c $<

brk$(O): sysc/$(OS)/$(ARCH)/brk.c include/errno.h
	$(CC) $(CFLAGS) -o $@ -c $<

sbrk$(O): sysc/$(OS)/$(ARCH)/sbrk.c include/errno.h
	$(CC) $(CFLAGS) -o $@ -c $<

time$(O): sysc/$(OS)/$(ARCH)/time.c include/time.h
	$(CC) $(CFLAGS) -o $@ -c $<

exit$(O): stdlib/exit.c include/stdio.h include/unistd.h
	$(CC) $(CFLAGS) -o $@ -c $<

write_all$(O): stdio/write_all.c \
	stdio/private.h \
	include/stdio.h include/stdlib.h include/unistd.h include/errno.h
	$(CC) $(CFLAGS) -o $@ -c $<

getchar$(O): stdio/getchar.c \
	stdio/private.h \
	include/stdio.h include/stdlib.h include/unistd.h include/errno.h
	$(CC) $(CFLAGS) -o $@ -c $<

putchar$(O): stdio/putchar.c \
	stdio/private.h \
	include/stdio.h include/stdlib.h include/unistd.h include/errno.h
	$(CC) $(CFLAGS) -o $@ -c $<

abort$(O): stdlib/abort.c include/stdio.h include/unistd.h
	$(CC) $(CFLAGS) -o $@ -c $<

x_puts$(O): stdio/x_puts.c \
	stdio/private.h \
	include/stdio.h include/stdlib.h include/unistd.h include/errno.h
	$(CC) $(CFLAGS) -o $@ -c $<

puts$(O): stdio/puts.c \
	stdio/private.h \
	include/stdio.h include/stdlib.h include/unistd.h include/errno.h
	$(CC) $(CFLAGS) -o $@ -c $<

atoi$(O): stdlib/atoi.c
	$(CC) $(CFLAGS) -o $@ -c $<

rand$(O): stdlib/rand.c include/stdlib.h
	$(CC) $(CFLAGS) -o $@ -c $<

libc$(A): $(LOBJS)
	$(AR) rc $@ $^
	$(RANLIB) $@

mostlyclean:
	rm -f $(LOBJS)
	$(MAKE) -C test mostlyclean

clean: mostlyclean
	rm -f crt0$(O) libc$(A)
	$(MAKE) -C test clean
