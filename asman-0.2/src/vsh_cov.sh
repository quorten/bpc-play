# Shell script to run against `vshell' to provide as much code
# coverage as possible.

# Copyright (C) 2017, 2018 Andrew Makousky

# See the file "COPYING" in the top level directory for details.

VAR1=hello VAR2=cool
VAR3=another echo "Quoted string" arg2 $VAR1 "$VAR2" ${VAR3} 'arg4'
  echo "Even more \"strings\", some with quotes."
echo "strings wi\
th newlines
and quotes"
# Error:
# echo ?$
echo Escape \' some \" characters\ in \` other ways.
echo Split across \
  new\
lines in multiple ways.
VAR1=
echo Variable \"${VAR1}\" unset?
NL='
'
echo "This variable${NL}is a multiline one."
echo Set variables:
set
echo Return value: $?
bad command
echo Return value: $?
echo $BAD_VAR "$BAD_VAR2"

######################################################################
echo Done with shell checks, moving to basic VFS checks.
pwd
touch hello one two three
mkdir usr /boot ///home ///../../././var ../../tmp
mkdir /usr/bin
mkdir usr/share
mkdir usr/lib/
ls
ls .
ls /
cd usr////bin///
pwd
cd ../..
cd usr/bin
pwd
touch sh
echo $?
ls
touch ../../../vmlinuz
cd ..
pwd
cd /
mv hello bye
cd /boot
mv ../bye start
cd ../home
mkdir www
mkdir www/tftpboot
mv ../boot/start www/tftpboot/image
cd www
pwd
ls
ls tftpboot
stat tftpboot tftpboot/image
rm tftpboot/image
rmdir tftpboot
cd ..
rmdir www/
cd ..
stat usr usr/ usr/////
mkdir dir1
mv dir1/ dir2/
mvt two /three dir2
cd dir2
ls
mvt two three ..
cd ..
rmdir dir2
pwd

######################################################################
echo Performing VFS glob checks.
touch a b c d f1 f2 f3 f4 f1n f2n f3n f4n
touch tmp/junk
echo *
echo ***
echo /*
echo /////*
echo f*
echo f*n
echo usr*
echo *usr
echo usr/*
echo *n*
echo */*
cd usr
echo ./*
echo ../*
echo /././../*
cd ..
echo ?
echo ??
echo ?.
echo .?
echo *.
echo .*
echo ? f* tmp/junk
rm ? f* tmp/junk

######################################################################
echo Performing erroneous VFS checks.
cd /
stat one/ one///
# Yes, I know, Unix does support touching existing files, but for the
# time being, we have a more spartan interpretation here that you are
# trying to recreate an existing file.
touch /usr/bin/sh
mkdir /usr/bin/sh
touch /usr/bin/sh/false
cd /usr/bin/sh/false
touch does/not/exist
touch fails/
# Yes, I know, Unix supports `cd' with no argument as a change to the
# home directory, but we're keeping this as a lightweight wrapper to
# the system call, which doesn't support a NULL string.
cd
cd does/not/exist
ls does/not/exist
stat does/not/exist
rm fail fails/ does/not/exist
rmdir fail does/not/exist
rmdir vmlinuz
rm /usr
rmdir /usr
mv
mv does/not/exist /boot
mv does/not/exist/ boot/
mv vmlinuz does/not/exist
mv vmlinuz one
# Yes, I know, Unix allows you to overwrite existing files during
# rename, but we don't.
mv vmlinuz /usr/bin/sh
# Messing with "." and ".." is forbidden, even though Unix sometimes
# supports it.
mv vmlinuz/ /usr/bin/vmlinuz/
mv vmlinuz /usr/bin/
touch . ..
rm . ..
mkdir . ..
rmdir . ..
mv vmlinuz .
mv vmlinuz ..
mv . terrible
mv .. terrible
# Moving a directory into any subdirectory of itself is forbidden.
mv /usr/bin /usr/bin/bin
mv /usr /usr/bin/usr
mvt three does/not/exist
mvt two three
mvt two bad usr/ three boot
ls boot

######################################################################
echo Performing VFS metadata checks.
# truncate
# touch -t ...
# chmod
# chown
# xattr ...
# list forks
# read forks
# versions...

######################################################################
echo Performing extended VFS feature checks.
vfs_fastimport 1
vfs_endimport
mkdirp nest1/one/two/three
# This one is weird, but it's technically the same behavior as
# mainstream toolsets.
mkdirp nest2/one/two/three/../../../four/five/six/../../../seven/eight/nine

######################################################################
# TODO FIXME FATAL ERROR:
# "backslash"
# newline
# corruption detected
