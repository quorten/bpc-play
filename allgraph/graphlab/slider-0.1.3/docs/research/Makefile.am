## Process this file with automake to produce Makefile.in
## This file is in Public Domain.

EXTRA_DIST = wave.tex

if MAINTAINER_MODE
noinst_DATA = wave.pdf
endif

%.dvi: %.tex
	latex $<

%.pdf: %.dvi
	dvipdfm $<

mostlyclean:
	rm -f *.aux *.toc *.log

clean:
	rm -f *.aux *.toc *.log *.dvi *.pdf
