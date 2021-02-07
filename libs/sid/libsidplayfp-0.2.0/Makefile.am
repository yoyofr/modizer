
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign subdir-objects

if HAVE_VISIBILITY
  AM_CXXFLAGS = -fvisibility=hidden -fvisibility-inlines-hidden
endif

AM_CPPFLAGS = -I $(top_builddir)/builders/residfp-builder \
-I $(top_builddir)/builders/residfp-builder/residfp \
-I $(top_builddir)/builders/resid-builder \
-I $(top_builddir)/builders/resid-builder/resid \
-I $(top_builddir)/sidplayfp \
@debug_flags@

#=========================================================
EXTRA_DIST = \
sidplayfp/psiddrv.bin \
sidplayfp/kernal.bin \
sidplayfp/poweron.bin \
sidplayfp/basic.bin \
sidplayfp/char.bin \
sidplayfp/c64/c64cia.h \
sidplayfp/c64/c64vic.h \
sidplayfp/c64/c64xsid.h \
sidplayfp/mos6510/cycle_based \
builders/residfp-builder/residfp/AUTHORS \
builders/residfp-builder/residfp/COPYING \
builders/residfp-builder/residfp/README \
builders/resid-builder/resid/AUTHORS \
builders/resid-builder/resid/ChangeLog \
builders/resid-builder/resid/COPYING \
builders/resid-builder/resid/configure.in \
builders/resid-builder/resid/INSTALL \
builders/resid-builder/resid/NEWS \
builders/resid-builder/resid/README \
builders/resid-builder/resid/README.VICE \
builders/resid-builder/resid/THANKS \
builders/resid-builder/resid/TODO \
$(noinst_DATA) \
$(noinst_SCRIPTS)

#=========================================================
# Libraries
lib_LTLIBRARIES = \
sidplayfp/libsidplayfp.la \
builders/residfp-builder/libsidplayfp-residfp.la \
builders/resid-builder/libsidplayfp-resid.la

if HARDSID
  lib_LTLIBRARIES += builders/hardsid-builder/libsidplayfp-hardsid.la
endif

noinst_LTLIBRARIES = \
builders/residfp-builder/residfp/libresidfp.la \
builders/resid-builder/resid/libresid.la

#=========================================================
# libsidplayfp
sidplayfp_libsidplayfp_la_SOURCES = \
sidplayfp/config.cpp \
sidplayfp/event.cpp \
sidplayfp/player.cpp \
sidplayfp/player.h \
sidplayfp/psiddrv.cpp \
sidplayfp/mixer.cpp \
sidplayfp/reloc65.c \
sidplayfp/sidplay2.cpp \
sidplayfp/nullsid.h \
sidplayfp/psiddrv.a65 \
sidplayfp/sidenv.h \
sidplayfp/mos656x/mos656x.cpp \
sidplayfp/mos656x/mos656x.h \
sidplayfp/mos6510/mos6510.cpp \
sidplayfp/mos6510/mos6510.h \
sidplayfp/mos6510/conf6510.h \
sidplayfp/mos6510/opcodes.h \
sidplayfp/mos6526/mos6526.cpp \
sidplayfp/mos6526/mos6526.h \
sidplayfp/sid6526/sid6526.cpp \
sidplayfp/sid6526/sid6526.h \
sidplayfp/sidtune/IconInfo.cpp \
sidplayfp/sidtune/InfoFile.cpp \
sidplayfp/sidtune/MUS.cpp \
sidplayfp/sidtune/p00.cpp \
sidplayfp/sidtune/PP20.cpp \
sidplayfp/sidtune/PP20.h \
sidplayfp/sidtune/PP20_Defs.h \
sidplayfp/sidtune/prg.cpp \
sidplayfp/sidtune/PSID.cpp \
sidplayfp/sidtune/SidTune.cpp \
sidplayfp/sidtune/SidTuneCfg.h \
sidplayfp/sidtune/SidTuneTools.cpp \
sidplayfp/sidtune/SidTuneTools.h \
sidplayfp/xsid/xsid.cpp \
sidplayfp/xsid/xsid.h \
utils/iniParser.cpp \
utils/iniParser.h \
utils/SidDatabase.cpp \
utils/SidFilter.cpp \
utils/SidTuneMod.cpp \
utils/SidUsage.cpp \
utils/smm.cpp \
utils/smm0.h \
utils/MD5/MD5.cpp \
utils/MD5/MD5.h \
utils/MD5/MD5_Defs.h

sidplayfp_libsidplayfp_la_LDFLAGS = -version-info $(LIBSIDPLAYVERSION)

sidplayfp_libsidplayfp_ladir = $(includedir)/sidplayfp

sidplayfp_libsidplayfp_la_HEADERS = \
sidplayfp/sidconfig.h \
sidplayfp/Buffer.h \
sidplayfp/c64env.h \
sidplayfp/component.h \
sidplayfp/event.h \
sidplayfp/sid2types.h \
sidplayfp/sidbuilder.h \
sidplayfp/sidendian.h \
sidplayfp/sidplay2.h \
sidplayfp/SidTune.h \
sidplayfp/sidtypes.h \
sidplayfp/sidusage.h \
sidplayfp/SmartPtr.h \
utils/SidDatabase.h \
utils/SidFilter.h \
utils/SidTuneMod.h \
utils/SidUsage.h

nodist_sidplayfp_libsidplayfp_la_HEADERS = \
sidplayfp/sidversion.h

#=========================================================
# residfp

builders_residfp_builder_residfp_libresidfp_la_SOURCES = \
builders/residfp-builder/residfp/array.h \
builders/residfp-builder/residfp/Dac.cpp \
builders/residfp-builder/residfp/Dac.h \
builders/residfp-builder/residfp/Integrator.cpp \
builders/residfp-builder/residfp/Integrator.h \
builders/residfp-builder/residfp/EnvelopeGenerator.cpp \
builders/residfp-builder/residfp/EnvelopeGenerator.h \
builders/residfp-builder/residfp/ExternalFilter.cpp \
builders/residfp-builder/residfp/ExternalFilter.h \
builders/residfp-builder/residfp/FilterModelConfig.cpp \
builders/residfp-builder/residfp/FilterModelConfig.h \
builders/residfp-builder/residfp/Filter.cpp \
builders/residfp-builder/residfp/Filter.h \
builders/residfp-builder/residfp/Filter8580.cpp \
builders/residfp-builder/residfp/Filter8580.h \
builders/residfp-builder/residfp/Filter6581.cpp \
builders/residfp-builder/residfp/Filter6581.h \
builders/residfp-builder/residfp/OpAmp.cpp \
builders/residfp-builder/residfp/OpAmp.h \
builders/residfp-builder/residfp/Potentiometer.h \
builders/residfp-builder/residfp/SID.cpp \
builders/residfp-builder/residfp/SID.h \
builders/residfp-builder/residfp/Spline.cpp \
builders/residfp-builder/residfp/Spline.h \
builders/residfp-builder/residfp/Voice.h \
builders/residfp-builder/residfp/WaveformCalculator.cpp \
builders/residfp-builder/residfp/WaveformCalculator.h \
builders/residfp-builder/residfp/WaveformGenerator.cpp \
builders/residfp-builder/residfp/WaveformGenerator.h \
builders/residfp-builder/residfp/resample/Resampler.h \
builders/residfp-builder/residfp/resample/ZeroOrderResampler.h \
builders/residfp-builder/residfp/resample/SincResampler.cpp \
builders/residfp-builder/residfp/resample/SincResampler.h \
builders/residfp-builder/residfp/resample/TwoPassSincResampler.h \
builders/residfp-builder/residfp/residfp-config.h \
builders/residfp-builder/residfp/version.cc

#=========================================================
# resid

BUILT_SOURCES = $(noinst_DATA:.dat=.h)

builders_resid_builder_resid_libresid_la_SOURCES = \
builders/resid-builder/resid/dac.h \
builders/resid-builder/resid/sid.h \
builders/resid-builder/resid/voice.h \
builders/resid-builder/resid/wave.h \
builders/resid-builder/resid/envelope.h \
builders/resid-builder/resid/filter.h \
builders/resid-builder/resid/extfilt.h \
builders/resid-builder/resid/pot.h \
builders/resid-builder/resid/spline.h \
builders/resid-builder/resid/dac.cc \
builders/resid-builder/resid/sid.cc \
builders/resid-builder/resid/voice.cc \
builders/resid-builder/resid/wave.cc \
builders/resid-builder/resid/envelope.cc \
builders/resid-builder/resid/filter.cc \
builders/resid-builder/resid/extfilt.cc \
builders/resid-builder/resid/pot.cc \
builders/resid-builder/resid/resid-config.h \
builders/resid-builder/resid/version.cc \
$(noinst_DATA:.dat=.h)

noinst_DATA = \
builders/resid-builder/resid/wave6581_PST.dat \
builders/resid-builder/resid/wave6581_PS_.dat \
builders/resid-builder/resid/wave6581_P_T.dat \
builders/resid-builder/resid/wave6581__ST.dat \
builders/resid-builder/resid/wave8580_PST.dat \
builders/resid-builder/resid/wave8580_PS_.dat \
builders/resid-builder/resid/wave8580_P_T.dat \
builders/resid-builder/resid/wave8580__ST.dat

noinst_SCRIPTS = builders/resid-builder/resid/samp2src.pl

#.dat.h:
#	$(PERL) $(srcdir)/samp2src.pl $* $< $(srcdir)/$@

#=========================================================
# builders
builders_residfp_builder_libsidplayfp_residfp_ladir = $(includedir)/sidplayfp/builders
builders_residfp_builder_libsidplayfp_residfp_la_HEADERS = \
builders/residfp-builder/residfp.h

builders_residfp_builder_libsidplayfp_residfp_la_SOURCES = \
builders/residfp-builder/residfp.cpp \
builders/residfp-builder/residfp-builder.cpp \
builders/residfp-builder/residfp-emu.h

builders_residfp_builder_libsidplayfp_residfp_la_LIBADD = \
builders/residfp-builder/residfp/libresidfp.la

builders_residfp_builder_libsidplayfp_residfp_la_LDFLAGS = -version-info $(LIBRESIDFPVERSION)


builders_resid_builder_libsidplayfp_resid_ladir = $(includedir)/sidplayfp/builders
builders_resid_builder_libsidplayfp_resid_la_HEADERS = \
builders/resid-builder/resid.h

builders_resid_builder_libsidplayfp_resid_la_SOURCES = \
builders/resid-builder/resid.cpp \
builders/resid-builder/resid-builder.cpp \
builders/resid-builder/resid-emu.h

builders_resid_builder_libsidplayfp_resid_la_LIBADD = \
builders/resid-builder/resid/libresid.la

builders_resid_builder_libsidplayfp_resid_la_LDFLAGS = -version-info $(LIBRESIDVERSION)

if HARDSID
builders_hardsid_builder_libsidplayfp_hardsid_ladir = $(includedir)/sidplayfp/builders
builders_hardsid_builder_libsidplayfp_hardsid_la_HEADERS = \
builders/hardsid-builder/hardsid.h

if MINGW32
  hardsid_src = builders/hardsid-builder/hardsid-win.cpp
else
  hardsid_src = builders/hardsid-builder/hardsid-unix.cpp
endif

builders_hardsid_builder_libsidplayfp_hardsid_la_SOURCES = \
$(hardsid_src) \
builders/hardsid-builder/hardsid-builder.cpp  \
builders/hardsid-builder/hardsid-emu.h

builders_hardsid_builder_libsidplayfp_hardsid_la_LDFLAGS = -version-info $(LIBHARDSIDVERSION)
endif

#=========================================================
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsidplayfp.pc

#=========================================================
# Remove SVN directories from any subdirs that get
# included completely (e.g. win, directx,...).

dist-hook:
	rm -rf `find $(distdir) -path \*/.svn`
