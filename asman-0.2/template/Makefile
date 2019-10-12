# Archive system management configuration makefile.

# This is a template, so copy it, rename it to `Makefile', edit it as
# necessary, and remove this tag!

SHELL=sh
# "G" is the location of the global logs, if necessary for this
# archive root.  Otherwise, just leave this variable empty.
G=..
# "B" is the location of the `bin' directory.
B=../bin
# "L" is the location of the `lib' directory.
L=../lib

# Warning: You should escape all characters that would be special when
# appearing in double quotes within the following variables.  The
# following variables will always be used within double quotes in the
# master makefile scripts.

# Briefly, replace the top characters with the bottom character
# sequences:
#	\	"	$$	`
#	\\	\"	\$$	\`
# Note that you must type a dollar sign as a double dollar sign in
# Makefiles to avoid special interpretation by `make'.

# This is the archive root prefix on the local system.
ARCHIVE_ROOT = c:/hafm
# SRC_ROOT is the directory pathname to use in the update script.
# This might be different from ARCHIVE_ROOT because you are running
# the script on a machine different from the one that you generated
# the script on.
SRC_ROOT = /c/hafm
DEST_ROOT = /h/hafm
# DEST_ASMAN is the remote file name where file management commands
# should be written after the update.
DEST_ASMAN = /h/asman/home-dell-2300/push.txt

all: update.sh

include ../as-config.mak
include $(L)/archsys.mak

mostlyclean: mostlyclean-as

clean: mostlyclean clean-as
