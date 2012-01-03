## Process this file with automake to produce Makefile.in
bin_PROGRAMS = lha
lha_SOURCES = append.c bitio.c crcio.c dhuf.c extract.c header.c huf.c \
	indicator.c larc.c lha.h lha_macro.h prototypes.h lhadd.c lharc.c \
	lhext.c lhlist.c maketbl.c maketree.c patmatch.c shuf.c slide.c \
	util.c getopt_long.c getopt_long.h
lha_LDADD = @LIBOBJS@
EXTRA_DIST = lhdir.h fnmatch.h
AM_CPPFLAGS=$(DEF_KCODE) $(SUPPORT_LZHUFF_METHOD)
