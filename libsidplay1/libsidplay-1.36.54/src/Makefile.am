## Process this file with automake to produce Makefile.in

EXTRA_DIST = libcfg.h.in compconf.h.in \
fastforward.txt format.txt mixing.txt mpu.txt panning.txt

DISTCLEANFILES = compconf.h libcfg.h

sidplayincludedir = $(includedir)/sidplay

sidplayinclude_HEADERS = compconf.h libcfg.h emucfg.h fformat.h \
fixpoint.h myendian.h mytypes.h player.h sidtune.h version.h

lib_LTLIBRARIES = libsidplay.la

## We substitute the libtool-specific library version in configure.in.

libsidplay_la_LDFLAGS = -version-info @LIBVERSION@

libsidplay_la_SOURCES = sidtune.cpp eeconfig.cpp eeconfig.h \
player.cpp 6510_.cpp 6510_.h 6581_.cpp enve_dl.h envelope.cpp \
envelope.h mixing.cpp opstruct.h samples.cpp samples.h wave6581.h \
wave8580.h fformat_.cpp info_.cpp info_.h mus_.cpp mus_.h pp_.cpp \
pp_.h psid_.cpp psid_.h sid_.cpp sid_.h compconf.h libcfg.h emucfg.h \
fformat.h fixpoint.h myendian.h mytypes.h player.h pp.h sidtune.h \
smart.h version.h

INCLUDES = -I$(top_srcdir)
LIBS = @LIBS@
