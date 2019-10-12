# Test suite for the Archive System Management Tools.
# This is a MANUAL verification test suite!

# Copyright (C) 2013 Andrew Makousky
#
# See the file "COPYING" in the top level directory for details.

B = ../src
S = ../scripts

all:

bogus:
	./bogusgen.sh

battle:
	./dupfiles.sh

delempty: bogus
	cd bogus && ../$(B)/delempty

dirtrans: battle
	cd battle && ls -R | ../$(B)/dirtrans

fmsimp: fmtest1 fmtest2
	echo '***TEST ONE***'
	$(B)/fmsimp < fmtest1
	echo '***TEST TWO***'
	$(B)/fmsimp < fmtest2

asops: optest
	$(B)/fmsimp < optest | $(B)/asops filt-script.txt

sumxfr: sums.txt new-sums.txt
	$(B)/sumxfr sums.txt < new-sums.txt | $(B)/fmsimp

asrebase: uptest
	$(B)/asrebase ./ /home/user/ < uptest

asup: uptest
	$(B)/asup ./ < uptest

lnmux: mux1 mux2 mux3 mux4 even1 even2 even3
	echo '***TEST ONE***'
	$(B)/lnmux mux1
	echo '***TEST TWO***'
	$(B)/lnmux mux1 mux2
	echo '***TEST THREE***'
	$(B)/lnmux mux1 mux2 mux3
	echo '***TEST FOUR***'
	$(B)/lnmux mux1 mux2 mux3 mux4
	echo '***TEST FIVE***'
	$(B)/lnmux even1 even2 even3

nltrans: battle
	find $< -not -type d -print0 | \
	  $(S)/dq-escape.sed | $(B)/nltrans | sed -e 's/^/"/g' -e 's/$$/"/g'

dedup: battle
	find $< -not -type d -print0 | xargs -r0 md5sum | sort | \
	  $(B)/dedup

sed-escape: crack-test.txt
	sed -e "s/`sed -f $(S)/comments.sed -f $(S)/sed-escape.sed crack-test.txt`/Defeated!/g" \
	  crack-test.txt

clean:
	rm -rf bogus battle
