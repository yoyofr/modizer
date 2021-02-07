## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

EXTRA_DIST = scripts DEVELOPER libsidplay.spec sidplay.xpm

dist-hook:
		rm -rf $(distdir)/src/compconf.h $(distdir)/src/libcfg.h

