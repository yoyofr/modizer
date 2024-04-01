MAKEFLAGS += --warn-undefined-variables
.DELETE_ON_ERROR:
SHELL := bash
.SHELLFLAGS := -eu -o pipefail -c

.PHONY: all default distclean clean install dist clean-apidoc

all: default

noincludes  := $(patsubst distclean,yes,$(MAKECMDGOALS))

# Defaults, overridden by config.mk below once configure has run
prefix      := /usr/local
exec_prefix := $(prefix)

bindir       := $(exec_prefix)/bin
libdir       := $(exec_prefix)/lib
mandir       := $(prefix)/man
docdir       := $(prefix)/share/doc/gbsplay
localedir    := $(prefix)/share/locale
mimedir      := $(prefix)/share/mime
appdir       := $(prefix)/share/applications
includedir   := $(prefix)/include/libgbs
pkgconfigdir := $(prefix)/share/pkgconfig

configured := no
ifneq ($(noincludes),yes)
-include config.mk
endif

ifeq ($(use_verbosebuild),yes)
  VERBOSE = 1
endif
ifeq ("$(origin V)", "command line")
  VERBOSE = $(V)
endif
ifndef VERBOSE
  VERBOSE = 0
endif
ifeq ($(VERBOSE),1)
  Q =
else
  Q = @
endif

generatedeps := no
ifneq ($(noincludes),yes)
ifeq ($(configured),yes)
generatedeps := yes
endif
endif

TEST_WRAPPER =
DESTDIR     :=

# Update paths with user-provided DESTDIR
prefix       := $(DESTDIR)$(prefix)
exec_prefix  := $(DESTDIR)$(exec_prefix)
bindir       := $(DESTDIR)$(bindir)
mandir       := $(DESTDIR)$(mandir)
docdir       := $(DESTDIR)$(docdir)
localedir    := $(DESTDIR)$(localedir)
mimedir      := $(DESTDIR)$(mimedir)
appdir       := $(DESTDIR)$(appdir)
includedir   := $(DESTDIR)$(includedir)
pkgconfigdir := $(DESTDIR)$(pkgconfigdir)

man1dir     := $(mandir)/man1
man3dir     := $(mandir)/man3
man5dir     := $(mandir)/man5
contribdir  := $(docdir)/contrib
exampledir  := $(docdir)/examples

DISTDIR := gbsplay-$(VERSION)

GBSCFLAGS  := -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=200809L $(EXTRA_CFLAGS)
GBSLDFLAGS := $(EXTRA_LDFLAGS)
comma := ,
GBSLIBLDFLAGS := -lm $(subst -pie,,$(subst -Wl$(comma)-pie,,$(EXTRA_LDFLAGS)))
# Additional ldflags for the gbsplay executable
GBSPLAYLDFLAGS :=
# Additional ldflags for the xgbsplay executable
XGBSPLAYLDFLAGS := -lxcb -lxcb-icccm -lcairo

EXTRA_CLEAN :=

export Q VERBOSE CC HOSTCC BUILDCC GBSCFLAGS GBSLDFLAGS

docs               := README.md HISTORY COPYRIGHT
docs-dist          := INSTALL.md CODINGSTYLE gbsformat.txt
contribs           := contrib/gbs2ogg.sh contrib/gbsplay.bashcompletion
examples           := examples/nightmode.gbs examples/gbsplayrc_sample

mans               := man/gbsplay.1    man/gbsinfo.1    man/gbsplayrc.5
mans_src           := man/gbsplay.in.1 man/gbsinfo.in.1 man/gbsplayrc.in.5

apimans_list       := libgbs.h gbs gbs_channel_status gbs_output_buffer gbs_status
apidocdir          := apidoc
apimans            := $(patsubst %,$(apidocdir)/man/man3/%.3,$(apimans_list))

apiheaders         := libgbs.h

objs_libgbspic     := gbcpu.lo gbhw.lo gblfsr.lo mapper.lo gbs.lo crc32.lo
objs_libgbs        := gbcpu.o  gbhw.o  gblfsr.o  mapper.o  gbs.o  crc32.o
objs_gbs2gb        := gbs2gb.o
objs_gbsinfo       := gbsinfo.o
objs_gbsplay       := gbsplay.o  util.o plugout.o player.o cfgparser.o
objs_xgbsplay      := xgbsplay.o util.o plugout.o player.o cfgparser.o
objs_test_gbs      := test_gbs.o
objs_gen_impulse_h := gen_impulse_h.ho impulsegen.ho

tests              := util.test impulsegen.test gblfsr.test

# terminal handling
ifeq ($(windows_libprefix),lib)
# when on MINGW
objs_gbsplay += terminal_windows.o
else
objs_gbsplay += terminal_posix.o
endif

# gbsplay output plugins
plugout_objs    :=
plugout_ldflags :=

ifeq ($(plugout_devdsp),yes)
plugout_objs += plugout_devdsp.o
endif
ifeq ($(plugout_alsa),yes)
plugout_objs += plugout_alsa.o
plugout_ldflags += -lasound
endif
ifeq ($(plugout_nas),yes)
plugout_objs += plugout_nas.o
plugout_ldflags += -laudio $(libaudio_flags)
endif
ifeq ($(plugout_sdl),yes)
plugout_objs += plugout_sdl.o
plugout_ldflags += $(libSDL2_flags)
endif
ifeq ($(plugout_stdout),yes)
plugout_objs += plugout_stdout.o
endif
ifeq ($(plugout_midi),yes)
plugout_objs += plugout_midi.o midifile.o filewriter.o
endif
ifeq ($(plugout_altmidi),yes)
plugout_objs += plugout_altmidi.o midifile.o filewriter.o
endif
ifeq ($(plugout_pipewire),yes)
plugout_objs += plugout_pipewire.o
plugout_ldflags += $(libpipewire_0_3_flags)
endif
ifeq ($(plugout_pulse),yes)
plugout_objs += plugout_pulse.o
plugout_ldflags += -lpulse-simple -lpulse
endif
ifeq ($(plugout_dsound),yes)
plugout_objs += plugout_dsound.o
plugout_ldflags += -ldsound $(libdsound_flags)
endif
ifeq ($(plugout_iodumper),yes)
plugout_objs += plugout_iodumper.o
endif
ifeq ($(plugout_wav),yes)
plugout_objs +=plugout_wav.o filewriter.o
endif
ifeq ($(plugout_vgm),yes)
plugout_objs += plugout_vgm.o filewriter.o
endif

# dedupe (and finalize) plugout_objs; order is irrelevant, sorting is ok
plugout_objs := $(sort $(plugout_objs))

objs_gbsplay += $(plugout_objs)
objs_xgbsplay += $(plugout_objs)
GBSPLAYLDFLAGS += $(plugout_ldflags)
XGBSPLAYLDFLAGS += $(plugout_ldflags)

# install contrib files?
ifeq ($(build_contrib),yes)
EXTRA_INSTALL += install-contrib
EXTRA_UNINSTALL += uninstall-contrib
endif

# test built binary?
ifeq ($(build_test),yes)
TEST_TARGETS += test
endif

# Cygwin automatically adds .exe to binaries.
# We should notice that or we can't rm the files later!
ifeq ($(windows_build),yes)
binsuffix         := .exe
else
binsuffix         :=
endif
gbsplaybin        := gbsplay$(binsuffix)
xgbsplaybin       := xgbsplay$(binsuffix)
gbs2gbbin         := gbs2gb$(binsuffix)
gbsinfobin        := gbsinfo$(binsuffix)
test_gbsbin       := test_gbs$(binsuffix)
gen_impulse_h_bin := gen_impulse_h$(binsuffix)

ifeq ($(use_sharedlibgbs),yes)

EXTRA_INSTALL += install-headers install-pkg-config
EXTRA_UNINSTALL += uninstall-headers uninstall-pkg-config

ifeq ($(have_doxygen),yes)
EXTRA_INSTALL += install-apidoc
EXTRA_UNINSTALL += uninstall-apidoc
endif

GBSLDFLAGS += -L. -lgbs
objs += $(objs_libgbspic)

libgbs.so.1.ver: libgbs_whitelist.txt gen_linkercfg.sh
	./gen_linkercfg.sh libgbs_whitelist.txt $@ ver

libgbs.def: libgbs_whitelist.txt gen_linkercfg.sh
	./gen_linkercfg.sh libgbs_whitelist.txt $@ def

ifeq ($(windows_build),yes)
EXTRA_INSTALL += install-$(windows_libprefix)gbs-1.dll
EXTRA_UNINSTALL += uninstall-$(windows_libprefix)gbs-1.dll

install-$(windows_libprefix)gbs-1.dll:
	install -d $(bindir)
	install -d $(libdir)
	install -m 644 $(windows_libprefix)gbs-1.dll $(bindir)/$(windows_libprefix)gbs-1.dll
	install -m 644 libgbs.dll.a $(libdir)/libgbs.dll.a

uninstall-$(windows_libprefix)gbs-1.dll:
	rm -f $(bindir)/$(windows_libprefix)gbs-1.dll
	rm -f $(libdir)/libgbs.dll.a
	-rmdir -p $(libdir)

$(windows_libprefix)gbs-1.dll: $(objs_libgbspic) libgbs.so.1.ver
	$(CC) -fpic -shared -Wl,-soname=$@ -Wl,--version-script,libgbs.so.1.ver -o $@ $(objs_libgbspic) $(EXTRA_LDFLAGS)

libgbs.dll.a: libgbs.def
	dlltool --input-def libgbs.def --dllname $(windows_libprefix)gbs-1.dll --output-lib libgbs.dll.a -k

libgbs: $(windows_libprefix)gbs-1.dll libgbs.dll.a
	touch libgbs

libgbspic: $(windows_libprefix)gbs-1.dll libgbs.dll.a
	touch libgbspic
else
EXTRA_INSTALL += install-libgbs.so.1
EXTRA_UNINSTALL += uninstall-libgbs.so.1


install-libgbs.so.1:
	install -d $(libdir)
	install -m 644 libgbs.so.1 $(libdir)/libgbs.so.1

uninstall-libgbs.so.1:
	rm -f $(libdir)/libgbs.so.1
	-rmdir -p $(libdir)


libgbs.so.1: $(objs_libgbspic) libgbs.so.1.ver
	$(BUILDCC) -fpic -shared -Wl,-soname=$@ -Wl,--version-script,$@.ver -o $@ $(objs_libgbspic) $(GBSLIBLDFLAGS)
	ln -fs $@ libgbs.so

libgbs: libgbs.so.1
	touch libgbs

libgbspic: libgbs.so.1
	touch libgbspic
endif
else
GBSLDFLAGS += -lm
objs += $(objs_libgbs)
objs_gbsplay += libgbs.a
objs_gbs2gb += libgbs.a
objs_gbsinfo += libgbs.a
objs_test_gbs += libgbs.a
objs_xgbsplay += libgbs.a

libgbs: libgbs.a
	touch libgbs

libgbspic: libgbspic.a
	touch libgbspic
endif # use_sharedlibs

objs += $(objs_gbsplay) $(ojbs_gbs2gb) $(objs_gbsinfo)
dsts += gbsplay gbs2gb gbsinfo

ifeq ($(build_xgbsplay),yes)
objs += $(objs_xgbsplay)
dsts += xgbsplay
mans += man/xgbsplay.1
mans_src += man/xgbsplay.in.1
EXTRA_INSTALL += install-xgbsplay
EXTRA_UNINSTALL += uninstall-xgbsplay
EXTRA_CLEAN += clean-xgbsplay
endif

# include the rules for each subdir
include $(shell find . -type f -name "subdir.mk")

ifeq ($(generatedeps),yes)
# Ready to build deps and everything else
default: config.mk $(objs) $(dsts) $(mans) $(EXTRA_ALL) $(TEST_TARGETS)

# Generate & include the dependency files
deps := $(patsubst %.o,%.d,$(filter %.o,$(objs)))
deps += $(patsubst %.lo,%.d,$(filter %.lo,$(objs)))
-include $(deps)
else
# Configure still needs to run
default: config.mk
endif

distclean: clean
	find . -regex ".*\.d" -exec rm -f "{}" \;
	rm -f ./config.mk ./config.h ./config.err ./config.sed ./libgbs.pc

clean: clean-default $(EXTRA_CLEAN) clean-apidoc

clean-default:
	find . -regex ".*\.\([aos]\|ho\|lo\|mo\|pot\|test\(\.exe\)?\|so\(\.[0-9]\)?\|gcda\|gcno\|gcov\)" -exec rm -f "{}" \;
	find . -name "*~" -exec rm -f "{}" \;
	rm -f libgbs libgbspic libgbs.def libgbs.so.1.ver
	rm -f $(mans)
	rm -f $(gbsplaybin) $(gbs2gbbin) $(gbsinfobin)
	rm -f $(test_gbsbin)
	rm -f $(gen_impulse_h_bin) impulse.h

clean-apidoc:
	rm -rf $(apidocdir)/

clean-xgbsplay:
	rm -f $(xgbsplaybin)

install: all install-default $(EXTRA_INSTALL)

install-default:
	install -d $(bindir)
	install -d $(man1dir)
	install -d $(man5dir)
	install -d $(docdir)
	install -d $(exampledir)
	install -d $(mimedir)/packages
	install -d $(appdir)
	install -m 755 $(gbsplaybin) $(gbs2gbbin) $(gbsinfobin) $(bindir)
	install -m 644 man/gbsplay.1 man/gbsinfo.1 $(man1dir)
	install -m 644 man/gbsplayrc.5 $(man5dir)
	install -m 644 mime/gbsplay.xml $(mimedir)/packages
	-update-mime-database $(mimedir)
	install -m 644 desktop/gbsplay.desktop $(appdir)
	-update-desktop-database $(appdir)
	install -m 644 $(docs) $(docdir)
	install -m 644 $(examples) $(exampledir)
	for i in $(mos); do \
		base=`basename $$i`; \
		install -d $(localedir)/$${base%.mo}/LC_MESSAGES; \
		install -m 644 $$i $(localedir)/$${base%.mo}/LC_MESSAGES/gbsplay.mo; \
	done

install-contrib:
	install -d $(contribdir)
	install -m 644 $(contribs) $(contribdir)

install-xgbsplay:
	install -d $(bindir)
	install -d $(man1dir)
	install -m 755 $(xgbsplaybin) $(bindir)
	install -m 644 man/xgbsplay.1 $(man1dir)
	install -m 644 desktop/xgbsplay.desktop $(appdir)
	-update-desktop-database $(appdir)

install-apidoc:	generate-apidoc
	install -d $(man3dir)
	install -m 644 $(apimans) $(man3dir)

install-headers:
	install -d $(includedir)
	install -m 644 $(apiheaders) $(includedir)

install-pkg-config:
	install -d $(pkgconfigdir)
	install -m 644 libgbs.pc $(pkgconfigdir)

uninstall: uninstall-default $(EXTRA_UNINSTALL)

uninstall-default:
	rm -f $(bindir)/$(gbsplaybin) $(bindir)/$(gbs2gbbin) $(bindir)/$(gbsinfobin)
	-rmdir -p $(bindir)
	rm -f $(man1dir)/gbsplay.1 $(man1dir)/gbsinfo.1
	-rmdir -p $(man1dir)
	rm -f $(man5dir)/gbsplayrc.5
	-rmdir -p $(man5dir)
	rm -f $(mimedir)/packages/gbsplay.xml
	-update-mime-database $(mimedir)
	-rmdir -p $(mimedir)/packages
	rm -f $(appdir)/gbsplay.desktop
	-update-desktop-database $(appdir)
	-rmdir -p $(appdir)
	rm -rf $(exampledir)
	-rmdir -p $(exampledir)
	rm -rf $(docdir)
	-mkdir -p $(docdir)
	-rmdir -p $(docdir)
	-for i in $(mos); do \
		base=`basename $$i`; \
		rm -f $(localedir)/$${base%.mo}/LC_MESSAGES/gbsplay.mo; \
		rmdir -p $(localedir)/$${base%.mo}/LC_MESSAGES; \
	done

uninstall-contrib:
	rm -rf $(contribdir)
	-rmdir -p $(contribdir)

uninstall-xgbsplay:
	rm -f $(bindir)/$(xgbsplaybin)
	rm -f $(man1dir)/xgbsplay.1
	rm -f $(appdir)/xgbsplay.desktop
	-update-desktop-database $(appdir)
	-rmdir -p $(bindir)
	-rmdir -p $(man1dir)
	-rmdir -p $(appdir)

uninstall-apidoc:
	rm -f $(patsubst %, $(man3dir)/%.3,$(apimans_list))
	-rmdir -p $(man3dir)

uninstall-headers:
	rm -f $(addprefix $(includedir)/,$(apiheaders))
	-rmdir -p $(includedir)

uninstall-pkg-config:
	rm -f $(pkgconfigdir)/libgbs.pc
	-rmdir -p $(pkgconfigdir)

dist:	distclean
	install -d ./$(DISTDIR)
	sed 's/^VERSION=.*/VERSION=$(VERSION)/' < configure > ./$(DISTDIR)/configure
	chmod 755 ./$(DISTDIR)/configure
	install -m 755 depend.sh ./$(DISTDIR)/
	install -m 644 Makefile ./$(DISTDIR)/
	install -m 644 *.c ./$(DISTDIR)/
	install -m 644 *.h ./$(DISTDIR)/
	install -d ./$(DISTDIR)/man
	install -m 644 $(mans_src) ./$(DISTDIR)/man
	install -m 644 $(docs) $(docs-dist) ./$(DISTDIR)/
	install -d ./$(DISTDIR)/examples
	install -m 644 $(examples) ./$(DISTDIR)/examples
	install -d ./$(DISTDIR)/contrib
	install -m 644 $(contribs) ./$(DISTDIR)/contrib
	install -d ./$(DISTDIR)/po
	install -m 644 po/*.po ./$(DISTDIR)/po
	install -m 644 po/subdir.mk ./$(DISTDIR)/po
	install -m 644 .gitignore ./$(DISTDIR)/
	install -d ./$(DISTDIR)/mime
	install -m 644 mime/* ./$(DISTDIR)/mime
	install -d ./$(DISTDIR)/desktop
	install -m 644 desktop/* ./$(DISTDIR)/desktop
	tar -cvzf ../$(DISTDIR).tar.gz $(DISTDIR)/ 
	rm -rf ./$(DISTDIR)

generate-apidoc: clean-apidoc
	doxygen Doxyfile

TESTOPTS := -r 44100 -t 30 -f 0 -g 0 -T 0 -H off

test: gbsplay $(tests) test_gbs
	@echo Verifying output correctness for examples/nightmode.gbs:
	$(Q)MD5=`LD_LIBRARY_PATH=.:$${LD_LIBRARY_PATH-} $(TEST_WRAPPER) ./gbsplay -c examples/gbsplayrc_sample -o iodumper $(TESTOPTS) examples/nightmode.gbs 1 < /dev/null | (md5sum || md5 -r) | cut -f1 -d\ `; \
	EXPECT="9e7595c3cd5c37a6a7793d1adb1c0741"; \
	if [ "$$MD5" = "$$EXPECT" ]; then \
		echo "iodumper output ok"; \
	else \
		echo "iodumper output failed"; \
		echo "  Expected: $$EXPECT"; \
		echo "  Got:      $$MD5" ; \
		exit 1; \
	fi
	$(Q)MD5=`LD_LIBRARY_PATH=.:$${LD_LIBRARY_PATH-} $(TEST_WRAPPER) ./gbsplay -c examples/gbsplayrc_sample -E b -o stdout $(TESTOPTS) examples/nightmode.gbs 1 < /dev/null | (md5sum || md5 -r) | cut -f1 -d\ `; \
	EXPECT="b2b83c42d5e250a54a90699a1faefd1a"; \
	if [ "$$MD5" = "$$EXPECT" ]; then \
		echo "Bigendian output ok"; \
	else \
		echo "Bigendian output failed"; \
		echo "  Expected: $$EXPECT"; \
		echo "  Got:      $$MD5" ; \
		exit 1; \
	fi
	$(Q)MD5=`LD_LIBRARY_PATH=.:$${LD_LIBRARY_PATH-} $(TEST_WRAPPER) ./gbsplay -c examples/gbsplayrc_sample -E l -o stdout $(TESTOPTS) examples/nightmode.gbs 1 < /dev/null | (md5sum || md5 -r) | cut -f1 -d\ `; \
	EXPECT="a49498eff6bd5a227c007dfef9050852"; \
	if [ "$$MD5" = "$$EXPECT" ]; then \
		echo "Littleendian output ok"; \
	else \
		echo "Littleendian output failed"; \
		echo "  Expected: $$EXPECT"; \
		echo "  Got:      $$MD5" ; \
		exit 1; \
	fi
	$(Q)MD5=`LD_LIBRARY_PATH=.:$${LD_LIBRARY_PATH-} $(TEST_WRAPPER) ./gbsplay -c examples/gbsplayrc_sample -E l -o wav $(TESTOPTS) examples/nightmode.gbs 1 < /dev/null; cat gbsplay-1.wav | (md5sum || md5 -r) | cut -f1 -d\ `; \
	EXPECT="2cb6c3a16bbf49657feaebfb5742b4df"; \
	if [ "$$MD5" = "$$EXPECT" ]; then \
		echo "WAV output ok"; \
	else \
		echo "WAV output failed"; \
		echo "  Expected: $$EXPECT"; \
		echo "  Got:      $$MD5" ; \
		exit 1; \
	fi
	$(Q)rm gbsplay-1.wav
	$(Q)MD5=`LD_LIBRARY_PATH=.:$${LD_LIBRARY_PATH-} $(TEST_WRAPPER) ./gbsplay -c examples/gbsplayrc_sample -E l -o vgm $(TESTOPTS) examples/nightmode.gbs 1 < /dev/null; cat gbsplay-1.vgm | (md5sum || md5 -r) | cut -f1 -d\ `; \
	EXPECT="d00e08025048d6831caffafa3d5b88c6"; \
	if [ "$$MD5" = "$$EXPECT" ]; then \
		echo "VGM output ok"; \
	else \
		echo "VGM output failed"; \
		echo "  Expected: $$EXPECT"; \
		echo "  Got:      $$MD5" ; \
		exit 1; \
	fi
	$(Q)rm gbsplay-1.vgm
	$(Q)MD5=`LD_LIBRARY_PATH=.:$${LD_LIBRARY_PATH-} $(TEST_WRAPPER) ./gbsplay -c examples/gbsplayrc_sample -E l -o midi $(TESTOPTS) examples/nightmode.gbs 1 < /dev/null; cat gbsplay-1.mid | (md5sum || md5 -r) | cut -f1 -d\ `; \
	EXPECT="196960b3048d50a748dcd0d26c9ea2de"; \
	if [ "$$MD5" = "$$EXPECT" ]; then \
		echo "MIDI output ok"; \
	else \
		echo "MIDI output failed"; \
		echo "  Expected: $$EXPECT"; \
		echo "  Got:      $$MD5" ; \
		exit 1; \
	fi
	$(Q)MD5=`LD_LIBRARY_PATH=.:$${LD_LIBRARY_PATH-} $(TEST_WRAPPER) ./gbsplay -c examples/gbsplayrc_sample -E l -o altmidi $(TESTOPTS) examples/nightmode.gbs 1 < /dev/null; cat gbsplay-1.mid | (md5sum || md5 -r) | cut -f1 -d\ `; \
	EXPECT="972105e5d0644d65cc557d2904117d13"; \
	if [ "$$MD5" = "$$EXPECT" ]; then \
		echo "alternate MIDI output ok"; \
	else \
		echo "alternate MIDI output failed"; \
		echo "  Expected: $$EXPECT"; \
		echo "  Got:      $$MD5" ; \
		exit 1; \
	fi
	$(Q)rm gbsplay-1.mid

$(gen_impulse_h_bin): $(objs_gen_impulse_h)
	$(HOSTCC) -o $(gen_impulse_h_bin) $(objs_gen_impulse_h) -lm
impulse.h: $(gen_impulse_h_bin)
	$(Q)./$(gen_impulse_h_bin) > $@
gbhw.d: impulse.h

libgbspic.a: $(objs_libgbspic)
	$(AR) r $@ $+
libgbs.a: $(objs_libgbs)
	$(AR) r $@ $+
gbs2gb: $(objs_gbs2gb) libgbs
	$(BUILDCC) -o $(gbs2gbbin) $(objs_gbs2gb) $(GBSLDFLAGS)
gbsinfo: $(objs_gbsinfo) libgbs
	$(BUILDCC) -o $(gbsinfobin) $(objs_gbsinfo) $(GBSLDFLAGS)
gbsplay: $(objs_gbsplay) libgbs
	$(BUILDCC) -o $(gbsplaybin) $(objs_gbsplay) $(GBSLDFLAGS) $(GBSPLAYLDFLAGS) -lm
test_gbs: $(objs_test_gbs) libgbs
	$(BUILDCC) -o $(test_gbsbin) $(objs_test_gbs) $(GBSLDFLAGS)

xgbsplay: $(objs_xgbsplay) libgbs
	$(BUILDCC) -o $(xgbsplaybin) $(objs_xgbsplay) $(GBSLDFLAGS) $(XGBSPLAYLDFLAGS) -lm

# rules for suffixes

.SUFFIXES: .i .s .lo .ho

.c.lo:
	@echo CC $< -o $@
	$(Q)$(BUILDCC) $(GBSCFLAGS) -fpic -c -o $@ $<
.c.o:
	@echo CC $< -o $@
	$(Q)$(BUILDCC) $(GBSCFLAGS) -fpie -c -o $@ $<
.c.ho:
	@echo HOSTCC $< -o $@
	$(Q)$(HOSTCC) $(GBSCFLAGS) -fpie -c -o $@ $<

.c.i:
	$(BUILDCC) -E $(GBSCFLAGS) -o $@ $<
.c.s:
	$(BUILDCC) -S $(GBSCFLAGS) -fverbose-asm -o $@ $<

# rules for generated files

config.mk: configure
	./configure

%.test: %.c
	@echo TEST $<
	$(Q)$(HOSTCC) -DENABLE_TEST=1 -o $@$(binsuffix) $< -lm
	$(Q)./$@$(binsuffix)
	$(Q)rm ./$@$(binsuffix)

%.d: %.c config.mk
	@echo DEP $< -o $@
	$(Q)CC=$(BUILDCC) ./depend.sh $< config.mk > $@ || rm -f $@

%.1: %.in.1 config.sed
	sed -f config.sed $< > $@

%.5: %.in.5 config.sed
	sed -f config.sed $< > $@
