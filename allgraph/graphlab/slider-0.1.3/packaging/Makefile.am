## Process this file with automake to produce Makefile.in

# Copyright (C) 2013 Andrew Makousky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#
#   * Redistributions in binary form must reproduce the above
#     copyright notice, this list of conditions and the following
#     disclaimer in the documentation and/or other materials provided
#     with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

EXTRA_DIST = 2.12.10-dist.txt \
	installer.nsi english.nsh spanish.nsh license.rtf \
	bannrbmp.bmp features.txt uuids.txt Property.idt.in \
	Directory.idt.cat Icon.idt Shortcut.idt.in

if WITH_WIN32

if MAINTAINER_MODE
noinst_DATA = slider-$(VERSION)-setup.exe
endif

MOSTLYCLEANFILES = \
	ls-r.txt ls-r2.txt File.idt Directory.idt Component.idt \
	Feature.idt FeatureComponents.idt Media.idt Directory.idt.old \
	Shortcut.idt archive.cab

CLEANFILES = slider-$(VERSION)-setup.exe slider-$(VERSION).msi

nsis: slider-$(VERSION)-setup.exe
msi: slider-$(VERSION).msi

2.12.10-dist: gtk+-bundle-2.12.10.zip
	mkdir $@
	-cd $@ && unzip -q ../gtk+-bundle-2.12.10.zip \
	  `cat ../2.12.10-dist.txt`
	echo 'gtk-theme-name = "MS-Windows"' > \
	  2.12.10-dist/etc/gtk-2.0/gtkrc
	if [ -f libportaudio-2.dll ]; then \
	  cp -p libportaudio-2.dll $@/bin/; \
	elif [ -f portaudio_x64.dll ]; then \
	  cp -p portaudio_x64.dll $@/bin/; \
	elif [ -f portaudio_x86.dll ]; then \
	  cp -p portaudio_x86.dll $@/bin/; \
	fi

instdir:
	tmpinst=`pwd`/instdir && \
	  cd $(top_builddir) && $(MAKE) install prefix=$$tmpinst
	if [ -d instdir/lib ]; then \
	  mv instdir/lib/locale instdir/share; \
	  rmdir instdir/lib; \
	fi

slider-$(VERSION)-setup.exe: installer.nsi english.nsh spanish.nsh \
  license.rtf instdir 2.12.10-dist
	if [ -n "$(TEST_INST)" ]; then \
	  makensis -DVERSION=$(VERSION) -DTEST_INST installer.nsi; \
	else \
	  makensis -DVERSION=$(VERSION) installer.nsi; \
	fi

ls-r.txt: 2.12.10-dist
	ls -RF $< | \
	  sed -e '/^.*\/$$/d' -e 's/\(^.*\)\*/\1/g' > $@

ls-r2.txt: instdir
	ls -RF $< | \
	  sed -e '/^.*\/$$/d' -e 's/\(^.*\)\*/\1/g' > $@

uuids.txt:
	uuidgen -c -n1000 > uuids.txt

File.idt Directory.idt Component.idt Feature.idt \
FeatureComponents.idt Media.idt: features.txt uuids.txt ls-r.txt ls-r2.txt
	msi-tool -r -d"slider|Slider Wave Editor" ls-r.txt ls-r2.txt

archive.cab: File.idt
	if [ -n "$(TEST_INST)" ]; then \
	  cd 2.12.10-dist && cabarc n archive.cab @cablist.txt; \
	else \
	  cd 2.12.10-dist && cabarc -m LZX:21 n archive.cab @cablist.txt; \
	fi
	mv 2.12.10-dist/archive.cab .
	rm -rf 2.12.10-dist instdir

slider-$(VERSION).msi: Property.idt File.idt Directory.idt Component.idt \
  Feature.idt FeatureComponents.idt Media.idt archive.cab Schema.msi
# Prepare tables that the installer will use to create shortcuts.
	mv Directory.idt Directory.idt.old
	cat Directory.idt.old Directory.idt.cat > Directory.idt
	mkdir Icon
	cp ../svgs/app.ico Icon/slider_icon.exe.ibd
	SSCOMP=`grep -F 'slider.exe' File.idt | cut -f2`; \
	  sed -e s/SliderComp/$${SSCOMP}/g -e s/SliderFeat/ft1/g \
	    Shortcut.idt.in > Shortcut.idt
# Build the installer.
	cp Schema.msi $@
	msiinfo $@ -t "Installation Database" -j "Slider Wave Editor" \
	  -a "Andrew Makousky" -k "Installer, MSI, Database" \
	  -o "This installer database contains the logic and data required to install Slider Wave Editor." \
	  -p "Intel;1033" -v {`uuidgen -c`} -g 110 -w 2 -u 0
	msidb -d$@ -f`pwd | sed -e 's.^\(\|/cygdrive\)/\([[:alpha:]]\)/.\2:/.g' -e 's./.\\\\.g'` \
	  -i Property.idt Component.idt Directory.idt \
	  Feature.idt FeatureComponents.idt File.idt Media.idt \
	  Icon.idt Shortcut.idt
	msidb -d$@ -aarchive.cab
	! msival2 $@ "C:\\Program Files\\MsiVal2\\darice.cub" | grep -e 'ERROR'

mostlyclean: mostlyclean-am
	rm -rf instdir 2.12.10-dist Icon

clean: clean-am mostlyclean

endif
