
AUTOMAKE_OPTIONS = foreign subdir-objects

EXTRA_DIST = utpp

if ENABLE_TEST

AM_CPPFLAGS = \
-I$(top_srcdir)/src \
-I$(top_builddir)/src/builders/residfp-builder/residfp

TESTS = \
TestPSID \
TestMUS \
TestMos6510 \
TestMD5

check_PROGRAMS = $(TESTS)

TestPSID_SOURCES = \
Main.cpp \
TestPSID.cpp
TestPSID_LDADD = $(top_builddir)/src/libsidplayfp.la

TestMUS_SOURCES = \
Main.cpp \
TestMUS.cpp
TestMUS_LDADD = $(top_builddir)/src/libsidplayfp.la

TestMos6510_SOURCES = \
Main.cpp \
TestMos6510.cpp

TestMD5_SOURCES = \
Main.cpp \
TestMD5.cpp

endif
