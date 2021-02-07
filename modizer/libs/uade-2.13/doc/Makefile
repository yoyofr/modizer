
all:	design.ps

design.pdf:	design.dvi
	dvipdf $<

design.ps:	design.dvi
	dvips -o $@ $<

design.dvi:	design.tex
	texi2dvi -c $<
