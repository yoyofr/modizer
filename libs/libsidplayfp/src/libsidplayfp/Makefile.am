
ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign subdir-objects

SUBDIRS = tests

AM_DISTCHECK_CONFIGURE_FLAGS = --enable-tests


if MINGW32
  W32_CPPFLAGS = -DWIN32_LEAN_AND_MEAN
  W32_LDFLAGS = -no-undefined
endif

AM_CPPFLAGS = \
-I $(top_builddir)/src/builders/residfp-builder \
-I $(top_builddir)/src \
-I $(top_builddir)/src/sidtune \
-I $(top_srcdir)/src \
${W32_CPPFLAGS} \
@debug_flags@

#=========================================================
EXTRA_DIST = \
docs/mainpage.dox \
src/psiddrv.a65 \
src/sidtune/sidplayer1.a65 \
src/sidtune/sidplayer2.a65 \
src/builders/sidlite-builder/sidlite/README \
src/builders/exsid-builder/driver/docs \
src/builders/exsid-builder/driver/m4 \
$(noinst_DATA) \
$(noinst_SCRIPTS) \
test/testsuite.sh \
test/testlist \
src/libs

#=========================================================
# Libraries
lib_LTLIBRARIES = \
src/libsidplayfp.la \
src/libstilview.la

noinst_LTLIBRARIES = \
src/builders/sidlite-builder/sidlite/libsidlite.la \
src/builders/sidlite-builder/libsidplayfp-sidlite.la

if RESIDFP_SUPPORT
  noinst_LTLIBRARIES += src/builders/residfp-builder/libsidplayfp-residfp.la 
endif

if USBSID_SUPPORT
  noinst_LTLIBRARIES += src/builders/usbsid-builder/libsidplayfp-usbsid.la
endif

if EXSID_SUPPORT
  noinst_LTLIBRARIES += src/builders/exsid-builder/libsidplayfp-exsid.la
endif

#=========================================================
# libsidplayfp

src_libsidplayfp_la_SOURCES = \
src/Event.h \
src/EventCallback.h \
src/EventScheduler.cpp \
src/EventScheduler.h \
src/player.cpp \
src/player.h \
src/psiddrv.cpp \
src/psiddrv.h \
src/psiddrv.bin \
src/poweron.bin \
src/properties.h \
src/reloc65.cpp \
src/reloc65.h \
src/sidcxx11.h \
src/sidmd5.h \
src/sidmemory.h \
src/SidInfoImpl.h \
src/simpleMixer.cpp \
src/simpleMixer.h \
src/romCheck.h \
src/sidemu.cpp \
src/sidemu.h \
src/sidendian.h \
src/sidrandom.h \
src/stringutils.h \
src/c64/Banks/Bank.h \
src/c64/c64cpu.h \
src/c64/c64cia.h \
src/c64/c64env.h \
src/c64/c64sid.h \
src/c64/c64vic.h \
src/c64/c64.cpp \
src/c64/c64.h \
src/c64/mmu.cpp \
src/c64/mmu.h \
src/c64/Banks/ColorRAMBank.h \
src/c64/Banks/DisconnectedBusBank.h \
src/c64/Banks/ExtraSidBank.h \
src/c64/Banks/IOBank.h \
src/c64/Banks/NullSid.h \
src/c64/Banks/pla.h \
src/c64/Banks/SidBank.h \
src/c64/Banks/SystemRAMBank.h \
src/c64/Banks/SystemROMBanks.h \
src/c64/Banks/ZeroRAMBank.h \
src/c64/VIC_II/mos656x.cpp \
src/c64/VIC_II/mos656x.h \
src/c64/VIC_II/lightpen.h \
src/c64/VIC_II/sprites.h \
src/c64/CPU/flags.h \
src/c64/CPU/mos6510.cpp \
src/c64/CPU/mos6510.h \
src/c64/CPU/mos6510debug.cpp \
src/c64/CPU/mos6510debug.h \
src/c64/CPU/opcodes.h \
src/c64/CIA/interrupt.cpp \
src/c64/CIA/interrupt.h \
src/c64/CIA/mos652x.cpp \
src/c64/CIA/mos652x.h \
src/c64/CIA/SerialPort.h \
src/c64/CIA/SerialPort.cpp \
src/c64/CIA/timer.cpp \
src/c64/CIA/timer.h \
src/c64/CIA/tod.cpp \
src/c64/CIA/tod.h \
src/sidplayfp/sidplayfp.cpp \
src/sidplayfp/sidbuilder.cpp \
src/sidplayfp/SidConfig.cpp \
src/sidplayfp/SidInfo.cpp \
src/sidplayfp/SidTune.cpp \
src/sidplayfp/SidTuneInfo.cpp \
src/sidtune/MUS.cpp \
src/sidtune/MUS.h \
src/sidtune/p00.cpp \
src/sidtune/p00.h \
src/sidtune/prg.cpp \
src/sidtune/prg.h \
src/sidtune/PSID.cpp \
src/sidtune/PSID.h \
src/sidtune/sidplayer1.bin \
src/sidtune/sidplayer2.bin \
src/sidtune/SidTuneBase.cpp \
src/sidtune/SidTuneBase.h \
src/sidtune/SidTuneCfg.h \
src/sidtune/SidTuneInfoImpl.h \
src/sidtune/SidTuneTools.cpp \
src/sidtune/SidTuneTools.h \
src/sidtune/SmartPtr.h \
src/utils/iniParser.cpp \
src/utils/iniParser.h \
src/utils/SidDatabase.cpp

src_libsidplayfp_la_LDFLAGS = -version-info $(LIBSIDPLAYVERSION) $(W32_LDFLAGS)

src_libsidplayfp_ladir = $(includedir)/sidplayfp

src_libsidplayfp_la_HEADERS = \
src/sidplayfp/siddefs.h \
src/sidplayfp/SidConfig.h \
src/sidplayfp/SidInfo.h \
src/sidplayfp/SidTuneInfo.h \
src/sidplayfp/sidbuilder.h \
src/sidplayfp/sidplayfp.h \
src/sidplayfp/SidTune.h \
src/utils/SidDatabase.h

nodist_src_libsidplayfp_la_HEADERS = \
src/sidplayfp/sidversion.h

src_libsidplayfp_la_CPPFLAGS = \
$(LIBGCRYPT_CFLAGS) \
$(AM_CPPFLAGS)

src_libsidplayfp_la_LIBADD = \
src/builders/sidlite-builder/libsidplayfp-sidlite.la \
$(LIBGCRYPT_LIBS)

if RESIDFP_SUPPORT
  src_libsidplayfp_la_LIBADD += src/builders/residfp-builder/libsidplayfp-residfp.la 
endif

if USBSID_SUPPORT
  src_libsidplayfp_la_LIBADD += src/builders/usbsid-builder/libsidplayfp-usbsid.la
endif

if EXSID_SUPPORT
  src_libsidplayfp_la_LIBADD += src/builders/exsid-builder/libsidplayfp-exsid.la
endif

#=========================================================
# sidlite

src_builders_sidlite_builder_sidlite_libsidlite_la_SOURCES = \
src/builders/sidlite-builder/sidlite/cw_tables.h \
src/builders/sidlite-builder/sidlite/filt_tables.h \
src/builders/sidlite-builder/sidlite/sl_constants.h \
src/builders/sidlite-builder/sidlite/sl_settings.h \
src/builders/sidlite-builder/sidlite/ADSR.cpp \
src/builders/sidlite-builder/sidlite/ADSR.h \
src/builders/sidlite-builder/sidlite/Filter.cpp \
src/builders/sidlite-builder/sidlite/Filter.h \
src/builders/sidlite-builder/sidlite/SID.cpp \
src/builders/sidlite-builder/sidlite/SID.h \
src/builders/sidlite-builder/sidlite/WavGen.cpp \
src/builders/sidlite-builder/sidlite/WavGen.h

src_builders_sidlite_builder_sidlite_libsidlite_la_CPPFLAGS = \
-I $(top_builddir)/src/builders/sidlite-builder/sidlite

#=========================================================
# builders

# residfp
if RESIDFP_SUPPORT

src_builders_residfp_builder_libsidplayfp_residfp_ladir = $(includedir)/sidplayfp/builders
src_builders_residfp_builder_libsidplayfp_residfp_la_HEADERS = \
src/builders/residfp-builder/residfp.h

src_builders_residfp_builder_libsidplayfp_residfp_la_SOURCES = \
src/builders/residfp-builder/residfp-builder.cpp \
src/builders/residfp-builder/residfp-emu.cpp \
src/builders/residfp-builder/residfp-emu.h

src_builders_residfp_builder_libsidplayfp_residfp_la_CPPFLAGS = \
$(RESIDFP_CFLAGS) \
$(AM_CPPFLAGS)

src_builders_residfp_builder_libsidplayfp_residfp_la_LIBADD = \
$(RESIDFP_LIBS)

endif

# sidlite
src_builders_sidlite_builder_libsidplayfp_sidlite_ladir = $(includedir)/sidplayfp/builders
src_builders_sidlite_builder_libsidplayfp_sidlite_la_HEADERS = \
src/builders/sidlite-builder/sidlite.h

src_builders_sidlite_builder_libsidplayfp_sidlite_la_SOURCES = \
src/builders/sidlite-builder/sidlite-builder.cpp \
src/builders/sidlite-builder/sidlite-emu.cpp \
src/builders/sidlite-builder/sidlite-emu.h

src_builders_sidlite_builder_libsidplayfp_sidlite_la_LIBADD = \
src/builders/sidlite-builder/sidlite/libsidlite.la

# usbsid
if USBSID_SUPPORT

if USBSID_DRIVER
  usbsid_src = \
src/builders/usbsid-builder/driver/src/USBSID.cpp \
src/builders/usbsid-builder/driver/src/USBSID.h
endif

src_builders_usbsid_builder_libsidplayfp_usbsid_ladir = $(includedir)/sidplayfp/builders
src_builders_usbsid_builder_libsidplayfp_usbsid_la_HEADERS = \
src/builders/usbsid-builder/usbsid.h

src_builders_usbsid_builder_libsidplayfp_usbsid_la_SOURCES = \
src/builders/usbsid-builder/usbsid-builder.cpp  \
src/builders/usbsid-builder/usbsid-emu.cpp \
src/builders/usbsid-builder/usbsid-emu.h \
$(usbsid_src)

src_builders_usbsid_builder_libsidplayfp_usbsid_la_CPPFLAGS = \
$(USBSID_CFLAGS) \
$(PTHREAD_CFLAGS) \
$(AM_CPPFLAGS)

src_builders_usbsid_builder_libsidplayfp_usbsid_la_LIBADD = \
$(USBSID_LIBS) \
$(PTHREAD_LIBS)

endif

#exsid
if EXSID_SUPPORT

if EXSID_DRIVER
  exsid_src = \
src/builders/exsid-builder/driver/exSID.c \
src/builders/exsid-builder/driver/exSID.h \
src/builders/exsid-builder/driver/exSID_defs.h \
src/builders/exsid-builder/driver/exSID_ftdiwrap.c \
src/builders/exsid-builder/driver/exSID_ftdiwrap.h
endif

src_builders_exsid_builder_libsidplayfp_exsid_ladir = $(includedir)/sidplayfp/builders
src_builders_exsid_builder_libsidplayfp_exsid_la_HEADERS = \
src/builders/exsid-builder/exsid.h

src_builders_exsid_builder_libsidplayfp_exsid_la_SOURCES = \
src/builders/exsid-builder/exsid-builder.cpp  \
src/builders/exsid-builder/exsid-emu.cpp \
src/builders/exsid-builder/exsid-emu.h \
$(exsid_src)

src_builders_exsid_builder_libsidplayfp_exsid_la_CPPFLAGS = \
$(EXSID_CFLAGS) \
$(FTDI_CFLAGS) \
$(PTHREAD_CFLAGS) \
$(AM_CPPFLAGS)

src_builders_exsid_builder_libsidplayfp_exsid_la_LIBADD = \
$(EXSID_LIBS) \
$(FTDI_LIBS) \
$(PTHREAD_LIBS)

endif

#=========================================================
# libstilview
src_libstilview_la_SOURCES = \
src/utils/STILview/stil.cpp

src_libstilview_la_LDFLAGS = -version-info $(LIBSTILVIEWVERSION) $(W32_LDFLAGS)

src_libstilview_ladir = $(includedir)/stilview

src_libstilview_la_HEADERS = \
src/utils/STILview/stil.h \
src/utils/STILview/stildefs.h

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
if TESTSUITE

if MINGW32
DEMO_SRC =
else
DEMO_SRC = test/demo
endif

noinst_PROGRAMS = \
$(DEMO_SRC) \
test/test

test_demo_SOURCES = test/demo.cpp

test_demo_LDADD = src/libsidplayfp.la

test_test_SOURCES = test/test.cpp

test_test_LDADD = src/libsidplayfp.la

endif

#=========================================================

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libsidplayfp.pc libstilview.pc

#=========================================================

BUILT_SOURCES = \
$(noinst_DATA:.dat=.h) \
src/psiddrv.bin \
src/sidtune/sidplayer1.bin \
src/sidtune/sidplayer2.bin

#=========================================================
# Recreate psiddrv.bin, needs xa65

MAINTAINERCLEANFILES = $(BUILT_SOURCES)

.a65.bin:
	o65file=`echo $@ | sed 's/bin/o65/'`;\
	[ -n "$(OD)" ] || { echo "od not found"; false; } &&\
	[ -n "$(XA)" ] || { echo "xa not found"; false; } &&\
	${XA} -R -G $< -o $$o65file &&\
	${OD} -v -An -w8 -tx1 $$o65file | sed -re 's/[[:alnum:]]+/0x&,/g' > $@ &&\
	${RM} $$o65file
