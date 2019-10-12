# Build some test programs against both the system `libc' and our
# miniature `libc'.

# See the COPYING file for license details.

TARGET =
CC = $(TARGET)gcc
LD = $(TARGET)ld

ADD_CFLAGS = -g # -m32 OR -mx32
CFLAGS = -I ../include $(ADD_CFLAGS)
# NOTE: These CFLAGS and LDFLAGS are mostly setup to disable system
# default compiler flags and libraries that tend to add a lot of bloat
# to simple executables.
CFLAGS += -fno-stack-protector -fno-asynchronous-unwind-tables
ADD_LDFLAGS = # -m elf_i386 OR elf32_x86_64
LDFLAGS = --exclude-libs ALL $(ADD_LDFLAGS)
PROGS = arg modarg ctype atoi sbrk m_arg m_modarg m_ctype m_atoi m_sbrk

all: $(PROGS)

../crt0.o ../libc.a:
	$(MAKE) -C ..

arg: arg.c putnum.c
	$(CC) -o $@ $^

modarg: modarg.c
	$(CC) -o $@ $^

ctype: ctype.c putnum.c
	$(CC) -o $@ $^

atoi: atoi.c putnum.c
	$(CC) -o $@ $^

sbrk: sbrk.c putnum.c
	$(CC) -o $@ $^

putnum.o: putnum.c
	$(CC) $(CFLAGS) -c $<

arg.o: arg.c
	$(CC) $(CFLAGS) -c $<

modarg.o: modarg.c
	$(CC) $(CFLAGS) -c $<

ctype.o: ctype.c
	$(CC) $(CFLAGS) -c $<

atoi.o: atoi.c
	$(CC) $(CFLAGS) -c $<

sbrk.o: sbrk.c
	$(CC) $(CFLAGS) -c $<

m_arg: ../crt0.o arg.o putnum.o ../libc.a
	$(LD) $(LDFLAGS) -o $@ $^

m_modarg: ../crt0.o modarg.o ../libc.a
	$(LD) $(LDFLAGS) -o $@ $^

m_ctype: ../crt0.o ctype.o putnum.o ../libc.a
	$(LD) $(LDFLAGS) -o $@ $^

m_atoi: ../crt0.o atoi.o putnum.o ../libc.a
	$(LD) $(LDFLAGS) -o $@ $^

m_sbrk: ../crt0.o sbrk.o putnum.o ../libc.a
	$(LD) $(LDFLAGS) -o $@ $^

mostlyclean:
	rm -f putnum.o arg.o modarg.o ctype.o atoi.o sbrk.o

clean: mostlyclean
	rm -f $(PROGS)
