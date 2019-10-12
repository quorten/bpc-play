# Archive system management common configuration -*- makefile -*-.

# This is a template, so copy it, edit it as necessary, and remove
# this tag!

# This is the command that is used to replace `touch' commands in the
# update script.
UPDATECMD=rsync -rtu
# Above is useful for basic setups on Microsoft Windows.  Setups that
# take place only on Unix machines might use the below command instead.
# UPDATECMD=rsync -au
# -Rau and rsync command coalescing should be used.  The problem is that
# command lines can get too long.
# rsync -Rtu ./file1 ./dir1/file1 /destdir
# Even better:
# rsync -tu0 --files-from=- . /destdir
# --iconv --protect-args
# This is the command to use for computing checksums.
SUMFUNC=sha256sum
