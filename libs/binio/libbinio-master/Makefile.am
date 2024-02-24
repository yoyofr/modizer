SUBDIRS = src doc

EXTRA_DIST = INSTALL.unix INSTALL.win32 INSTALL.dos Makefile.wat Makefile.bt \
	libbinio.qpg binio.spec binio_mdk.spec depcomp libbinio.pc.in

AUTOMAKE_OPTIONS = dist-bzip2

ACLOCAL_AMFLAGS=-I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbinio.pc
