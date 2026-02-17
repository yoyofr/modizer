
AUTOMAKE_OPTIONS = foreign subdir-objects

EXTRA_DIST = utpp

if ENABLE_TEST

AM_CPPFLAGS = \
-I$(top_srcdir)/src \
-I$(top_builddir)/src

TESTS = \
TestEnvelopeGenerator \
TestWaveformGenerator \
TestSpline \
TestDac \
TestResampler

check_PROGRAMS = $(TESTS)

TestEnvelopeGenerator_SOURCES = \
Main.cpp \
TestEnvelopeGenerator.cpp

TestWaveformGenerator_SOURCES = \
Main.cpp \
TestWaveformGenerator.cpp

TestSpline_SOURCES = \
Main.cpp \
TestSpline.cpp

TestDac_SOURCES = \
Main.cpp \
TestDac.cpp

TestResampler_SOURCES = \
Main.cpp \
TestResampler.cpp

endif
