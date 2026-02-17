
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign subdir-objects

SUBDIRS = tests

#AM_DISTCHECK_CONFIGURE_FLAGS = --enable-tests


if MINGW32
  W32_CPPFLAGS = -DWIN32_LEAN_AND_MEAN
  W32_LDFLAGS = -no-undefined
endif

AM_CPPFLAGS = \
-I $(top_builddir)/src \
-I $(top_srcdir)/src \
${W32_CPPFLAGS} \
@debug_flags@

#=========================================================
EXTRA_DIST = \
docs/mainpage.dox

#=========================================================
# Libraries
lib_LTLIBRARIES = \
src/libresidfp.la

#=========================================================
# libresidfp

src_libresidfp_la_SOURCES = \
src/array.h \
src/Dac.cpp \
src/Dac.h \
src/EnvelopeGenerator.cpp \
src/EnvelopeGenerator.h \
src/ExternalFilter.cpp \
src/ExternalFilter.h \
src/Filter.cpp \
src/Filter.h \
src/Filter6581.cpp \
src/Filter6581.h \
src/Filter8580.cpp \
src/Filter8580.h \
src/FilterModelConfig.cpp \
src/FilterModelConfig.h \
src/FilterModelConfig6581.cpp \
src/FilterModelConfig6581.h \
src/FilterModelConfig8580.cpp \
src/FilterModelConfig8580.h \
src/Integrator.h \
src/Integrator6581.cpp \
src/Integrator6581.h \
src/Integrator8580.cpp \
src/Integrator8580.h \
src/OpAmp.cpp \
src/OpAmp.h \
src/SID.cpp \
src/SID.h \
src/Spline.cpp \
src/Spline.h \
src/Voice.h \
src/WaveformCalculator.cpp \
src/WaveformCalculator.h \
src/WaveformGenerator.cpp \
src/WaveformGenerator.h \
src/resample/Resampler.h \
src/resample/ZeroOrderResampler.h \
src/resample/SincResampler.cpp \
src/resample/SincResampler.h \
src/resample/TwoPassSincResampler.h \
src/version.cc \
src/residfp/residfp.cpp

src_libresidfp_la_LDFLAGS = -version-info $(LIBRESIDFPVERSION) $(W32_LDFLAGS)

src_libresidfp_ladir = $(includedir)/residfp

src_libresidfp_la_HEADERS = \
src/residfp/residfp.h \
src/residfp/residfp_defs.h

nodist_src_libresidfp_la_HEADERS = \
src/residfp/sidversion.h

src_libresidfp_la_CPPFLAGS = $(AM_CPPFLAGS) @RESIDFP_CXXFLAGS@

#=========================================================
# docs
if HAVE_DOXYGEN
doc:
	$(DOXYGEN) Doxyfile

clean-local:
	rm -fr $(builddir)/docs/html
endif

#=========================================================
# test
if ENABLE_TEST

noinst_PROGRAMS = \
src/resample/test

src_resample_test_SOURCES = \
src/resample/test.cpp \
src/resample/SincResampler.cpp

endif

#=========================================================

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libresidfp.pc
