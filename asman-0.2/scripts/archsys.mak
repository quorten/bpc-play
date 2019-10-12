# Archive system management master include -*- makefile -*-.

# Copyright (C) 2013, 2018 Andrew Makousky

# See the file "COPYING" in the top level directory for details.

# Whenever a user-input pathname needs to be used in a script, double
# quotes will be used around the pathname.  For safety and robustness,
# we will preprocess such pathnames with `dq-escape' to escape any
# special characters that might be in the path names.

# Sometimes I am confined to using DOS style commands.  These are DOS
# aliases (assuming that the above pathnames are Cygwin or MSYS
# style), in case you want to generate "update.bat".
CONV_DRV = -e 's.^/\(cygdrive/\)\{0,1\}\([a-z]\).\2:.g'
CONV_SLASH = -e 's./.\\\\.g'
PATHCONV := $(CONV_DRV) $(CONV_SLASH)
D_SRC_ROOT := `echo "$(SRC_ROOT)" | sed $(PATHCONV)`
D_DEST_ROOT := `echo "$(DEST_ROOT)" | sed $(PATHCONV)`
BUILD_DIR := `pwd`
SE_DEST_ASMAN := `echo "$(DEST_ASMAN)" | sed -f $(L)/sed-escape.sed`

# File Management Gathering

modfiles.txt: sums-new.txt # sums.txt # Must already exist
	$(B)/sumxfr sums.txt <$< | asrebase ./ "$(ARCHIVE_ROOT)/" | >$@

merge-mod: modfiles.txt
	cat $< >>$(G)/fm-cmd.txt

find newfiles.txt:
	find "$(ARCHIVE_ROOT)" -not -type d -newer $(G)/fm-cmd.txt.hist \
	  -print0 | $(B)/dq-escape | $(B)/nltrans | \
	  sed -e 's/^/"/g' -e 's/$$/"/g' >newfiles.txt

merge-find: newfiles.txt
	sed -e 's/^/touch /g' $< | \
	  asrebase ./ "$(ARCHIVE_ROOT)/" >>$(G)/fm-cmd.txt

merge-emacs: dired-log.txt
	sed -e '/^sh-cmd: /p' $< | sed -e 's/^sh-cmd: //g' >>$(G)/fm-cmd.txt

merge-push: push.txt
	cat $< >>$(G)/fm-cmd.txt

$(G)/fm-simp.txt: $(G)/fm-cmd.txt
	$(B)/fmsimp <$< >$@

# Update Pipeline Processing

blacklist.txt: blacklist.txt.in
	echo "$(ARCHIVE_ROOT)/" | $(B)/dq-escape | \
	  sed -e 's/^/prefix "/g' -e 's/$$/"/g' | cat $< - >$@

asfm.txt: $(G)/fm-simp.txt blacklist.txt
	$(B)/asops blacklist.txt <$< >$@

upcmds.txt: asfm.txt
	$(B)/asup "$(ARCHIVE_ROOT)/" "$(UPDATECMD)" <$< >$@

cross-fm-log.txt: asfm.txt
	asrebase "$(ARCHIVE_ROOT)/" "$(DEST_ROOT)/" <$< >cross-fm-log.txt

update.sh: upcmds.txt cross-fm-log.txt
	echo '\#! /bin/sh' >$@
	echo SRC=\""$(SRC_ROOT)"\" >>$@
	echo DEST=\""$(DEST_ROOT)"\" >>$@
	cat $(L)/hupdate.sh $< >>$@
	echo '"$(UPDATECMD)" cross-fm-log.txt "$(DEST_ASMAN)"' >>$@
	# echo 'cat cross-fm-log.txt >>"$(DEST_ASMAN)"' >>$@
	chmod +x $@

update.bat: upcmds.txt
	echo '@echo off' >$@
	echo "set SRC=$(D_SRC_ROOT)" >>$@
	echo "set DEST=$(D_DEST_ROOT)" >>$@
	sed -n $< -e 's/\$${SRC}/%SRC%/g' -e 's/\$${DEST}/%DEST%/g' | \
	  sed -e 's./.\\.g' | \
	  sed -e 's.^cp -pR.ECHO F | XCOPY /E /I /Y.g' -e 's.^mv.MOVE /Y.g' \
	  -e 's.^rm -rf\{0,1\}.DELTREE /Y.g' -e 's.^rm .DEL .g' >>$@
	echo '"$(UPDATECMD)" cross-fm-log.txt "$(DEST_ASMAN)"' >>$@
	# echo 'TYPE cross-fm-log.txt >>"$(DEST_ASMAN)"' >>$@

update.cmd: upcmds.txt
	echo '@echo off' >$@
	echo "set SRC=$(D_SRC_ROOT)" >>$@
	echo "set DEST=$(D_DEST_ROOT)" >>$@
	sed -n $< -e 's/\$${SRC}/%SRC%/g' -e 's/\$${DEST}/%DEST%/g' | \
	  sed -e 's./.\\.g' | \
	  sed -e 's.^cp -pR.echo F | xcopy /E /I /Y.g' -e 's.^mv.move /Y.g' \
	  -e 's.^rm -rf\{0,1\}.rmdir /S /Q.g' -e 's.^rm .del .g' >>$@
	echo '"$(UPDATECMD)" cross-fm-log.txt "$(DEST_ASMAN)"' >>$@
	# echo 'type cross-fm-log.txt >>"$(DEST_ASMAN)"' >>$@

# A progressive update script

prog-update.sh: cross-fm-log.txt upcmds.txt
	echo '\#! /bin/sh' >$@
	echo SRC=\""$(SRC_ROOT)"\" >>$@
	echo DEST=\""$(DEST_ROOT)"\" >>$@
	$(B)/dq-escape $< | \
	  sed -e 's/^/echo "/g' -e 's/$$/" >>'"$(SE_DEST_ASMAN)"'/g' | \
	  $(B)/lnmux upcmds.txt - | cat $(L)/hupdate.sh - >>$@
	chmod +x $@

# Checksum Maintenance

check:
	cd "$(ARCHIVE_ROOT)" && \
	  "$(SUMFUNC)" -c "$(BUILD_DIR)/sums.txt" 2>&1 | \
	  sed -ne '/: FAILED/p' -e '/WARNING: /p'

sums.txt:
	cd "$(ARCHIVE_ROOT)" && \
	  find . -not -type d -print0 | xargs -r0 "$(SUMFUNC)" | \
	  sort >"$(BUILD_DIR)/$@"

# TODO sums-fm-updated

merge-new-sums: sums-new.txt
	mv sums-new.txt sums.txt

# Cleanup

mostlyclean-as:
	rm -f $(G)/fm-simp.txt asfm.txt upcmds.txt

clean-as: mostlyclean-as
	rm -f update.sh update.bat cross-fm-log.txt
	rm -f sums-new.txt newfiles.txt

distclean-as: clean-as
	rm -f blacklist.txt sums.txt

# This target creates backups of the logs to prevent accidental loss
# of journaling information.  Also, the update script is deleted by
# this target to prevent accidental clears of the current logs, and
# any cross logs are also deleted.
clear-logs: update.sh
	echo '# sync point' >> $(G)/fm-cmd.txt.hist
	cat $(G)/fm-cmd.txt >> $(G)/fm-cmd.txt.hist
	$(MAKE) clean

shrink-hist:
	mv $(G)/fm-cmd.txt.hist $(G)/fm-cmd.txt.hist.bad
	tail -n1000 $(G)/fm-cmd.txt.hist.bad > $(G)/fm-cmd.txt.hist
	rm $(G)/fm-cmd.txt.hist.bad
