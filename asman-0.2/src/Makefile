# Copyright (C) 2013, 2017, 2018, 2019 Andrew Makousky
#
# See the file "COPYING" in the top level directory for details.

# X = .exe
CC = cc
# TESTING NOTES: Running `valgrind ./vshell <vsh_cov.sh' is great when
# compiled with code coverage testing.  Test with code coverage and
# check for memory errors, both together work very well.
GCOV_CFLAGS = -fprofile-arcs -ftest-coverage
CFLAGS =

bin_PROGRAMS = delempty$(X) dirtrans$(X) lsrize$(X) fmsimp$(X) asops$(X) \
	asup$(X) asrebase$(X) simsum$(X) sumxfr$(X) dedup$(X) lnmux$(X) \
	nltrans$(X) permsave$(X) timsave$(X) szsave$(X) asfsmon$(X) \
	treegen$(X) vshell$(X) lsf$(X) duhcs$(X) catsync$(X) blkfix$(X)

lib_LIBRARIES = libcmdline.a

all: $(bin_PROGRAMS) $(lib_LIBRARIES)

# Note: `asown' has to be installed as a setuid program like this:
#   sudo chown root asown
#   sudo chmod u+s,o-x asown
asown: asown.c

# Useful programs on Windows
dategather$(X): dategather.c
dateapply$(X): dateapply.c
# TODO usncollect

xmalloc.o: xmalloc.c
cmdline.o: cmdline.c
shenv.o: shenv.c
shglob.o: shglob.c
noglob.o: noglob.c

libcmdline.a: cmdline.o shenv.o noglob.o
	ar rc $@ $^
	ranlib $@

misc.o: misc.c
strheap.o: strheap.c
dirwalk.o: dirwalk.c
vfs.o: vfs.c
vwrap.o: vwrap.c
coboot.o: coboot.c
	$(CC) $(CFLAGS) -fno-stack-protector -o $@ -c $<
bpipe.o: bpipe.c

test_vfs$(X): test_vfs.c vfs.o misc.o xmalloc.o
test_coboot$(X): test_coboot.c coboot.o coboot.h
	$(CC) $(CFLAGS) -fno-stack-protector -o $@ $^
test_bpipe$(X): test_bpipe.c bpipe.o

vdirwalk.o: dirwalk.c
	$(CC) $(CFLAGS) -Ivwrap -o $@ -c $<
vshglob.o: shglob.c
	$(CC) $(CFLAGS) -Ivwrap -o $@ -c $<
vlsf.o: lsf.c
	$(CC) $(CFLAGS) -Ivwrap -Dmain=proc_lsf -o $@ -c $<
vduhcs.o: duhcs.c
	$(CC) $(CFLAGS) -Ivwrap -Dmain=proc_duhcs -o $@ -c $<
vtreegen.o: treegen.c
	$(CC) $(CFLAGS) -Ivwrap -Dmain=proc_treegen -o $@ -c $<
vdelempty.o: delempty.c
	$(CC) $(CFLAGS) -Ivwrap -Dmain=proc_delempty -o $@ -c $<
vszsave.o: szsave.c
	$(CC) $(CFLAGS) -Ivwrap -Dmain=proc_szsave -o $@ -c $<
vtimsave.o: timsave.c
	$(CC) $(CFLAGS) -Ivwrap -Dmain=proc_timsave -o $@ -c $<

delempty$(X): delempty.c dirwalk.o libcmdline.a xmalloc.o
dirtrans$(X): dirtrans.c xmalloc.o
lsrize$(X): lsrize.c xmalloc.o misc.o
fmprim$(X): fmprim.c xmalloc.o # TODO
fmsimp$(X): fmsimp.c misc.o vfs.o libcmdline.a xmalloc.o
asops$(X): asops.c libcmdline.a xmalloc.o
asup$(X): asup.c libcmdline.a xmalloc.o
asrebase$(X): asrebase.c libcmdline.a xmalloc.o
simsum$(X): simsum.c misc.o libcmdline.a xmalloc.o
sumxfr$(X): sumxfr.c misc.o libcmdline.a xmalloc.o
dedup$(X): dedup.c misc.o libcmdline.a xmalloc.o
lnmux$(X): lnmux.c
nltrans$(X): nltrans.c
permsave$(X): permsave.c dirwalk.o libcmdline.a xmalloc.o
timsave$(X): timsave.c dirwalk.o libcmdline.a xmalloc.o
szsave$(X): szsave.c dirwalk.o libcmdline.a xmalloc.o
treegen$(X): treegen.c dirwalk.o libcmdline.a xmalloc.o
duhcs$(X): duhcs.c dirwalk.o libcmdline.a xmalloc.o
catsync$(X): catsync.c
blkfix$(X): blkfix.c
lsf$(X): lsf.c xmalloc.o
vshell$(X): vshell.c xmalloc.o cmdline.o shenv.o vshglob.o misc.o vfs.o \
	vwrap.o vdirwalk.o vlsf.o vduhcs.o vtreegen.o vdelempty.o \
	vszsave.o vtimsave.o
# TODO lfnsave
# TODO or more simply, metasave
# TODO mergemv
# TODO unmv -- swap source and destination on move commands
# TODO rsprep -- Coalesce rsync commands
asfsmon$(X): asfsmon.c

# TODO need support for nano-second modified times.  Not really, it
# turns out that is non-standard.  Optional support.

# GNU touch --date "YYYYMMDD HH:MM:SS.UUUUUU UTC"

# touch -s (checksum), touch -x (attribute change time), -z (size),
# -i (inode)

# Wait, there is one secure file time: the attribute modification
# time.  To verify that a file has not changed, this time can be
# checked, when the operating system supports it.

# TODO Add filesystem context verification to dedup.

# TODO File alteration monitor should provide a "hints" mechanism for
# user applications too.  No, maybe just the straight application
# integration API should be used.

# NOTE: Use md5 and sha256 rather than md5sum and sha256sum on *BSD.
# Also note that the programs on *BSD are not newline-escaping safe.

# TODO winshell supplements: sort, find, sha256sum, xargs.  *sum
# programs need to be provided in general since they are nominally
# part of GNU coreutils.

# TODO Check strcmp consistency

# TODO permsave and timsave need more flexibility

# TODO application integration helper library

# TODO security of asman, and of user data in the pipeline.

# TODO asops should use regex

# TODO always use prototypes

# TODO docs say read from standard input, write to standard output

# TODO hooks on chdir in dirwalk

# TODO fix oversized handling in cmdline.c

# TODO better error handling for dirwalk programs.

# TODO improve error messages

# TODO testing for sub-components, strheap, cmdline

# TODO MBCS and WCHAR safety.  MBCS is supported by default: no
# special code needs to be added for UTF-8.  UCS-2 must be pre-byte
# swapped to the current machine.  Double-check for UTF-8 safety.

# TODO no $(L), use PATH

# TODO formal test suite

# TODO const safety

# TODO exparray debugging

# TODO config reader, process monolith

# TODO rsync efficiency in regard to mv, cp, touch.  Dependency chains
# simplifier algorithm.

# TODO special command shell on each side of link

# TODO unlimited undo and redo in the shell, purge-delete style.

# TODO randomized test cases

# TODO Verify that there are no memory leaks.

# TODO fmsimp needs to have argument for Windows case-sensitivity
# clobber check.

# TODO test cases for prefix handling code.

# TODO the sum programs need to check for a leading \ for \n escapes

# TODO the programs need to deal with / root and separators safely.

cleancov:
	rm -f *.gcno *.gcda *.gcov

mostlyclean: cleancov
	rm -f xmalloc.o misc.o strheap.o cmdline.o \
	  shenv.o shglob.o noglob.o dirwalk.o vfs.o vwrap.o \
	  vdirwalk.o vshglob.o vlsf.o vduhcs.o vtreegen.o vdelempty.o \
	  vszsave.o vtimsave.o bpipe.o coboot.o \
	  test_vfs$(X) test_bpipe$(X) test_coboot$(X)

clean: mostlyclean
	rm -f $(bin_PROGRAMS) asown $(lib_LIBRARIES)

install: $(bin_PROGRAMS)
	install $(bin_PROGRAMS) ../bin

uninstall:
	cd ../bin && rm -f $(bin_PROGRAMS)
