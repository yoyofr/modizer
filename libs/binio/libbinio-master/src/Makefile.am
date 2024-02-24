lib_LTLIBRARIES = libbinio.la

libbinio_la_SOURCES = binio.cpp binfile.cpp binwrap.cpp binstr.cpp

libbinio_la_LDFLAGS = -version-info 1:0:0
libbinio_la_LDFLAGS += -no-undefined # mingw requires this when making shared DLL files

pkginclude_HEADERS = binio.h binfile.h binwrap.h binstr.h

EXTRA_DIST = Makefile.bt libbinio.dsp vc6inst.bat
