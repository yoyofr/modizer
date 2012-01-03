VERSION = 2.1.2

# gcc for most ports
ASAP_GCC = mingw32-gcc -s -O2 -Wall -o $@ -I. -I..
ASAP_GPP = mingw32-g++ -static -s -O2 -Wall -o $@ -I. -I..
WINDRES = windres -o $@
LIB_GCC = mingw32-gcc -O2 -Wall -I..
AR = ar rc

# Microsoft compiler for Windows Media Player and foobar2000
DSHOW_BASECLASSES_DIR = "C:/Program Files/Microsoft SDKs/Windows/v6.1/Samples/Multimedia/DirectShow/BaseClasses"
FOOBAR2000_SDK_DIR = ../../foobar2000_SDK
CL = cl -nologo -O2 -GL -GR- -GS- -W3 -wd4996 -DNDEBUG
LINKOPT = -link -release
MKLIB = lib -nologo -ltcg -out:$@
ASAP_CL = $(CL) -Fe$@ -I. -I..

# old Microsoft compiler for XBMC plugin
ASAP_CL71 = cl -nologo -O2 -GL -GR- -W3 -DNDEBUG -Fe$@ -I. -I..

# gcc for Windows Mobile
WINCE_GCC = arm-mingw32ce-gcc -s -O2 -Wall -o $@ -I. -I..
WINCE_WINDRES = arm-mingw32ce-windres -o $@ -D_WIN32_WCE
CABWIZ = "C:/Program Files/Microsoft Visual Studio 9.0/SmartDevices/SDK/SDKTools/cabwiz.exe"

# common tools
PERL = perl
RM = rm -f
CP = cp -f
MV = mv
UNIX2DOS = unix2dos
ASCIIDOC_START = asciidoc -o - -a localtime -a doctime
ASCIIDOC_END = | sed -e "s/527bbd;/c02020;/" | xmllint --valid --nonet -o $@ -
GIT = git
TAR = tar
SEVENZIP = 7z a -mx=9
GREP = grep
CANDLE = candle -nologo -o $@
LIGHT = light -nologo -o $@

COMMON_C = ../asap.c ../acpu.c ../apokeysnd.c
COMMON_H = ../asap.h ../asap_internal.h ../anylang.h ../players.h

all: mingw dsf foo asap.lib asapconv-static-lame.exe asap-sdl.exe

win32: mingw dsf foo xbmc COPYING.txt

mingw: asapconv.exe asapconv-no-lame.exe wasap.exe in_asap.dll gspasap.dll ASAP_Apollo.dll xmp-asap.dll bass_asap.dll apokeysnd.dll asapscan.exe ASAPShellEx.dll libasap.a

wince: wince/wasap.exe wince/gspasap.dll

include ../players/Makefile

# asapconv

asapconv.exe: ../asapconv.c $(COMMON_C) $(COMMON_H)
	$(ASAP_GCC) -DHAVE_LIBMP3LAME -DHAVE_LIBMP3LAME_DLL ../asapconv.c $(COMMON_C)

asapconv-static-lame.exe: ../asapconv.c $(COMMON_C) $(COMMON_H)
	$(ASAP_GCC) -DHAVE_LIBMP3LAME ../asapconv.c $(COMMON_C) -lmp3lame

asapconv-no-lame.exe: ../asapconv.c $(COMMON_C) $(COMMON_H)
	$(ASAP_GCC) ../asapconv.c $(COMMON_C)

# WASAP

wasap.exe: wasap/wasap.c $(COMMON_C) info_dlg.c wasap/wasap-res.o $(COMMON_H) info_dlg.h wasap/wasap.h
	$(ASAP_GCC) -Wl,-subsystem,windows -DWASAP wasap/wasap.c $(COMMON_C) info_dlg.c wasap/wasap-res.o -lcomdlg32 -lwinmm

wasap/wasap-res.o: gui.rc ../asap.h info_dlg.h wasap/wasap.h wasap/wasap.ico wasap/play.ico wasap/stop.ico
	$(WINDRES) -DWASAP gui.rc

wince/wasap.exe: wasap/wasap.c $(COMMON_C) info_dlg.c wince/wasap-res.o $(COMMON_H) info_dlg.h wasap/wasap.h
	$(WINCE_GCC) -DWASAP wasap/wasap.c $(COMMON_C) info_dlg.c wince/wasap-res.o

wince/wasap-res.o: gui.rc ../asap.h info_dlg.h wasap/wasap.h wasap/wasap.ico wasap/play.ico wasap/stop.ico
	$(WINCE_WINDRES) -DWASAP gui.rc

# Winamp

in_asap.dll: winamp/in_asap.c $(COMMON_C) info_dlg.c settings_dlg.c winamp/in_asap-res.o $(COMMON_H) info_dlg.h settings_dlg.h winamp/in2.h winamp/out.h winamp/ipc_pe.h winamp/wa_ipc.h
	$(ASAP_GCC) -shared -Wl,-subsystem,windows -DWINAMP winamp/in_asap.c $(COMMON_C) info_dlg.c settings_dlg.c winamp/in_asap-res.o -lcomdlg32

winamp/in_asap-res.o: gui.rc ../asap.h info_dlg.h settings_dlg.h
	$(WINDRES) -DWINAMP gui.rc

# foobar2000

foo: foo_asap.dll

FOOBAR2000_RUNTIME = $(FOOBAR2000_SDK_DIR)/foobar2000/foobar2000_component_client/component_client.cpp foobar2000/foobar2000_SDK.lib foobar2000/pfc.lib $(FOOBAR2000_SDK_DIR)/foobar2000/shared/shared.lib

foo_asap.dll: foobar2000/foo_asap.cpp $(COMMON_C) settings_dlg.c foobar2000/foo_asap.res $(COMMON_H) settings_dlg.h $(FOOBAR2000_RUNTIME)
	$(ASAP_CL) -Fofoobar2000/ -DFOOBAR2000 -LD -DWIN32 -DUNICODE -EHsc -I$(FOOBAR2000_SDK_DIR) foobar2000/foo_asap.cpp $(COMMON_C) settings_dlg.c foobar2000/foo_asap.res $(FOOBAR2000_RUNTIME) user32.lib $(LINKOPT)

foobar2000/foobar2000_SDK.lib: foobar2000/abort_callback.obj foobar2000/audio_chunk.obj foobar2000/audio_chunk_channel_config.obj foobar2000/console.obj foobar2000/file_info.obj foobar2000/filesystem.obj foobar2000/guids.obj foobar2000/mem_block_container.obj foobar2000/playable_location.obj foobar2000/preferences_page.obj foobar2000/replaygain_info.obj foobar2000/service.obj
	$(MKLIB) $^

foobar2000/%.obj: $(FOOBAR2000_SDK_DIR)/foobar2000/SDK/%.cpp
	$(CL) -c -Fo$@ -DWIN32 -DUNICODE -EHsc -I$(FOOBAR2000_SDK_DIR) $<

foobar2000/pfc.lib: foobar2000/cfg_var.obj foobar2000/guid.obj foobar2000/other.obj foobar2000/string.obj foobar2000/string_conv.obj foobar2000/utf8.obj
	$(MKLIB) $^

foobar2000/%.obj: $(FOOBAR2000_SDK_DIR)/pfc/%.cpp
	$(CL) -c -Fo$@ -DWIN32 -DUNICODE -EHsc $<

foobar2000/foo_asap.res: gui.rc ../asap.h settings_dlg.h
	$(WINDRES) -DFOOBAR2000 gui.rc

# GSPlayer

gspasap.dll: gsplayer/gspasap.c $(COMMON_C) settings_dlg.c gspasap-res.o $(COMMON_H) settings_dlg.h
	$(ASAP_GCC) -shared -Wl,-subsystem,windows -Wl,--kill-at -DGSPLAYER gsplayer/gspasap.c $(COMMON_C) settings_dlg.c gspasap-res.o

gspasap-res.o: gui.rc ../asap.h settings_dlg.h
	$(WINDRES) -DGSPLAYER gui.rc

wince/gspasap.dll: gsplayer/gspasap.c $(COMMON_C) settings_dlg.c wince/gspasap-res.o $(COMMON_H) settings_dlg.h
	$(WINCE_GCC) -shared -DGSPLAYER gsplayer/gspasap.c $(COMMON_C) settings_dlg.c wince/gspasap-res.o

wince/gspasap-res.o: gui.rc ../asap.h settings_dlg.h
	$(WINCE_WINDRES) -DGSPLAYER gui.rc

# DirectShow

dsf: asap_dsf.dll

DSHOW_BASECLASSES = $(DSHOW_BASECLASSES_DIR)/amfilter.cpp $(DSHOW_BASECLASSES_DIR)/combase.cpp $(DSHOW_BASECLASSES_DIR)/dllentry.cpp $(DSHOW_BASECLASSES_DIR)/dllsetup.cpp $(DSHOW_BASECLASSES_DIR)/mtype.cpp $(DSHOW_BASECLASSES_DIR)/source.cpp $(DSHOW_BASECLASSES_DIR)/wxlist.cpp $(DSHOW_BASECLASSES_DIR)/wxutil.cpp

asap_dsf.dll: dshow/asap_dsf.cpp $(COMMON_C) dshow/asap_dsf.res dshow/asap_dsf.def $(COMMON_H)
	$(ASAP_CL) -Fodshow/ -DDSHOW -LD -I$(DSHOW_BASECLASSES_DIR) dshow/asap_dsf.cpp $(COMMON_C) $(DSHOW_BASECLASSES) dshow/asap_dsf.res dshow/asap_dsf.def advapi32.lib ole32.lib oleaut32.lib strmiids.lib user32.lib winmm.lib $(LINKOPT)

dshow/asap_dsf.res: gui.rc ../asap.h
	$(WINDRES) -DDSHOW gui.rc

# Apollo

ASAP_Apollo.dll: apollo/ASAP_Apollo.cpp $(COMMON_C) info_dlg.c settings_dlg.c ASAP_Apollo-res.o $(COMMON_H) info_dlg.h settings_dlg.h apollo/InputPlugin.h
	$(ASAP_GPP) -shared -Wl,-subsystem,windows -DAPOLLO apollo/ASAP_Apollo.cpp $(COMMON_C) info_dlg.c settings_dlg.c ASAP_Apollo-res.o -lcomdlg32

ASAP_Apollo-res.o: gui.rc ../asap.h info_dlg.h settings_dlg.h
	$(WINDRES) -DAPOLLO gui.rc

# XBMC

xbmc: xbmc_asap.dll

xbmc_asap.dll: ../xbmc/xbmc_asap.c $(COMMON_C) ../xbmc/xbmc_asap.res $(COMMON_H)
	$(ASAP_CL71) -Fo../xbmc/ -LD -MD ../xbmc/xbmc_asap.c $(COMMON_C) ../xbmc/xbmc_asap.res $(LINKOPT)

../xbmc/xbmc_asap.res: gui.rc ../asap.h
	$(WINDRES) -DXBMC gui.rc

# XMPlay

xmp-asap.dll: xmplay/xmp-asap.c $(COMMON_C) info_dlg.c settings_dlg.c xmp-asap-res.o $(COMMON_H) settings_dlg.h xmplay/xmpin.h xmplay/xmpfunc.h
	$(ASAP_GCC) -shared -Wl,-subsystem,windows -Wl,--kill-at -DXMPLAY xmplay/xmp-asap.c $(COMMON_C) info_dlg.c settings_dlg.c xmp-asap-res.o -lcomdlg32

xmp-asap-res.o: gui.rc ../asap.h info_dlg.h settings_dlg.h
	$(WINDRES) -DXMPLAY gui.rc

# BASS

bass_asap.dll: bass/bass_asap.c $(COMMON_C) bass_asap-res.o $(COMMON_H) bass/bass-addon.h bass/bass.h bass/bass.lib
	$(ASAP_GCC) -shared -Wl,-subsystem,windows -Wl,--kill-at -DBASS bass/bass_asap.c $(COMMON_C) bass_asap-res.o bass/bass.lib

bass_asap-res.o: gui.rc ../asap.h
	$(WINDRES) -DBASS gui.rc

# APokeySound

apokeysnd.dll: ../apokeysnd.c apokeysnd-res.o ../asap.h ../asap_internal.h ../anylang.h
	$(ASAP_GCC) -shared -Wl,-subsystem,windows -DAPOKEYSND ../apokeysnd.c apokeysnd-res.o

apokeysnd-res.o: gui.rc ../asap.h
	$(WINDRES) -DAPOKEYSND gui.rc

# asapscan

asapscan.exe: ../asapscan.c $(COMMON_C) $(COMMON_H)
	$(ASAP_GCC) -DASAPSCAN ../asapscan.c $(COMMON_C)

# ASAPShellEx

ASAPShellEx.dll: shellex/ASAPShellEx.cpp ../asap.c ASAPShellEx-res.o $(COMMON_H)
	$(ASAP_GPP) -shared -Wl,-subsystem,windows -Wl,--kill-at -DASAP_ONLY_INFO shellex/ASAPShellEx.cpp ../asap.c ASAPShellEx-res.o -lole32 -loleaut32 -lshlwapi -luuid

ASAPShellEx-res.o: gui.rc ../asap.h
	$(WINDRES) -DSHELLEX gui.rc

# library

asap.lib: $(COMMON_C) $(COMMON_H)
	$(CL) -c $(COMMON_C)
	$(MKLIB) asap.obj acpu.obj apokeysnd.obj

libasap.a: $(COMMON_C) $(COMMON_H)
	$(LIB_GCC) -c $(COMMON_C)
	$(AR) $@ asap.o acpu.o apokeysnd.o

# Java

java:
	$(MAKE) -C ../java

# C#

csharp:
	$(MAKE) -C ../csharp

# JavaScript

javascript:
	$(MAKE) -C ../javascript

# Flash

flash:
	$(MAKE) -C ../flash

# SDL

asap-sdl.exe: ../asap-sdl.c $(COMMON_C) $(COMMON_H)
	$(ASAP_GCC) ../asap-sdl.c $(COMMON_C) -lmingw32 -lSDLmain -lSDL

# delete generated files

clean:
	$(RM) *.txt *.html *.dll *.exe *.obj *.exp *.lib *.res *.o
	$(RM) setup/asap.wixobj
	$(RM) libasap.a
	$(RM) wasap/wasap-res.o wasap.exe
	$(RM) winamp/in_asap-res.o in_asap.dll
	$(RM) foobar2000/*.obj foobar2000/*.lib foobar2000/foo_asap.res
	$(RM) dshow/*.obj dshow/asap_dsf.res
	$(RM) ../xbmc/*.obj ../xbmc/xbmc_asap.res
	$(RM) wince/wasap-res.o wince/wasap.exe wince/gspasap-res.o wince/gspasap.dll
	$(RM) install_dsf.bat uninstall_dsf.bat
	$(RM) ../players/*.obx ../players.h ../README.html
	$(MAKE) -C ../csharp clean
	$(MAKE) -C ../flash clean
	$(MAKE) -C ../java clean
	$(MAKE) -C ../javascript clean

# prepare files for release

COPYING.txt: ../COPYING
	$(UNIX2DOS) <$< >$@

../README.html: ../README ../NEWS ../CREDITS ../INSTALL
	$(ASCIIDOC_START) -a asapsrc -a asapports ../README $(ASCIIDOC_END)

README_Flash.html: ../README ../NEWS ../CREDITS ../flash/USAGE
	$(ASCIIDOC_START) -a asapflash="(included in this binary package)" ../README $(ASCIIDOC_END)

README_Java.html: ../README ../NEWS ../CREDITS ../java/USAGE
	$(ASCIIDOC_START) -a asapjava="(included in this binary package)" ../README $(ASCIIDOC_END)

README_JavaScript.html: ../README ../NEWS ../CREDITS ../javascript/USAGE
	$(ASCIIDOC_START) -a asapjavascript="(included in this package)" ../README $(ASCIIDOC_END)

README_Windows.html: ../README ../NEWS ../CREDITS USAGE
	$(ASCIIDOC_START) -a asapwin="(included in this binary package)" ../README $(ASCIIDOC_END)

README_WindowsSetup.html: ../README ../NEWS ../CREDITS USAGE
	$(ASCIIDOC_START) -a asapwin="(included in this setup)" -a asapsetup ../README $(ASCIIDOC_END)

README_WindowsCE.html: ../README ../NEWS ../CREDITS wince/USAGE
	$(ASCIIDOC_START) -a asapwince="(included in this binary package)" ../README $(ASCIIDOC_END)

install_dsf.bat:
	echo regsvr32 asap_dsf.dll>$@

uninstall_dsf.bat:
	echo regsvr32 /u asap_dsf.dll>$@

setup: ../../asap-$(VERSION)-win32.msi

../../asap-$(VERSION)-win32.msi: setup/asap.wixobj wasap/wasap.ico setup/license.rtf setup/asap-banner.jpg setup/asap-dialog.jpg setup/Website.url README_WindowsSetup.html shellex/ASAPShellEx.propdesc \
	asapconv.exe wasap.exe in_asap.dll gspasap.dll ASAP_Apollo.dll xmp-asap.dll bass_asap.dll apokeysnd.dll ASAPShellEx.dll asap_dsf.dll foo_asap.dll xbmc_asap.dll
	$(LIGHT) -ext WixUIExtension -sice:ICE69 $<

setup/asap.wixobj: setup/asap.wxs
	$(CANDLE) -dVERSION=$(VERSION) $<

wince-setup: ../../asap-$(VERSION)-wince-arm.cab

../../asap-$(VERSION)-wince-arm.cab: wince/asap.inf wince/wasap.exe wince/gspasap.dll
	cd wince && $(CABWIZ) asap.inf /cpu ARM
	$(MV) wince/asap.ARM.CAB $@

../MANIFEST:
	if test -e ../.git; then \
		($(GIT) ls-tree -r --name-only --full-tree master | grep -vF .gitignore \
			&& echo MANIFEST && echo README.html && for plr in $(PLAYERS); do echo players/$$plr.obx; done) | sort >$@; \
	fi

srcdist: ../MANIFEST ../README.html $(PLAYERS_OBX)
	chmod 644 $(PLAYERS_OBX)
	$(RM) ../../asap-$(VERSION).tar.gz
	cd .. && $(TAR) -c --numeric-owner --owner=0 --group=0 -T MANIFEST --transform=s,,asap-$(VERSION)/, | $(SEVENZIP) -tgzip -si ../asap-$(VERSION).tar.gz

dist: srcdist win32 wince java csharp javascript flash setup wince-setup \
	COPYING.txt README_Flash.html README_Java.html README_Windows.html README_WindowsCE.html \
	install_dsf.bat uninstall_dsf.bat
	$(RM) ../../asap-$(VERSION)-flash.zip
	$(RM) ../../asap-$(VERSION)-java.zip
	$(RM) ../../asap-$(VERSION)-win32.zip
	$(RM) ../../asap-$(VERSION)-wince-arm.zip
	$(SEVENZIP) -tzip ../../asap-$(VERSION)-flash.zip \
		COPYING.txt README_Flash.html ../flash/asap.swf
	$(SEVENZIP) -tzip ../../asap-$(VERSION)-java.zip \
		COPYING.txt README_Java.html \
		../java/asap2wav.jar ../java/asap_applet.jar ../java/j2me/asap_midlet.jad ../java/j2me/asap_midlet.jar
	$(SEVENZIP) -tzip ../../asap-$(VERSION)-win32.zip \
		COPYING.txt README_Windows.html \
		install_dsf.bat uninstall_dsf.bat \
		asapconv.exe wasap.exe in_asap.dll foo_asap.dll gspasap.dll asap_dsf.dll ASAP_Apollo.dll apokeysnd.dll xbmc_asap.dll xmp-asap.dll bass_asap.dll ASAPShellEx.dll
	$(SEVENZIP) -tzip ../../asap-$(VERSION)-wince-arm.zip \
		COPYING.txt README_WindowsCE.html ./wince/wasap.exe ./wince/gspasap.dll
	$(MAKE) -C ../java/android ANDROID_RELEASE=../../../asap-$(VERSION)-android.apk release
	$(TAR) -tvzf ../../asap-$(VERSION).tar.gz | $(PERL) -ne "m'^-rwxr-xr-x .+?/(chksap|files2anylang)\.pl$$|^-rw-r--r-- ' or die \$$_"
	$(GREP) -m 1 ^ASAP ../NEWS
	$(GREP) ASAP_VERSION ../asap.h
	$(GREP) " Version =" ../csharp/ASAP.ppcs
	$(GREP) " VERSION =" ../java/ASAP.ppjava
	$(GREP) android:versionName ../java/android/AndroidManifest.xml
	$(GREP) MIDlet-Version ../java/j2me/MANIFEST.MF
	$(GREP) ASAP_VERSION ../javascript/asap.ppjs
	$(GREP) VERSION ../flash/ASAP.ppas
	$(GREP) "VERSION =" ../chksap.pl
	$(GREP) Version: ../asap.spec
	$(GREP) "<since>" ../PORTS.xml | sort -ru | head -1

www: ../../www/index.html ../../www/apokeysnd.dll ../../www/asap.swf ../../www/asap_applet.jar ../../www/SilverASAP.xap ../../www/favicon.ico ../../www/PORTS.xml ../../www/PORTS.xsl

../../www/index.html: ../README ../NEWS ../CREDITS
	$(ASCIIDOC_START) -a asapwww -a asapports ../README $(ASCIIDOC_END)

../../www/apokeysnd.dll: apokeysnd.dll
	$(CP) $< $@

../../www/asap.swf: ../flash/asap.swf
	$(CP) $< $@

../../www/asap_applet.jar: ../java/asap_applet.jar
	$(CP) $< $@

../../www/SilverASAP.xap: ../csharp/SilverASAP.xap
	$(CP) $< $@

../../www/favicon.ico: wasap/wasap.ico
	$(CP) $< $@

../../www/PORTS.xml: ../PORTS.xml
	$(CP) $< $@

../../www/PORTS.xsl: ../PORTS.xsl
	$(CP) $< $@

.PHONY: all win32 mingw foo wince dsf xbmc java csharp javascript flash clean setup ../MANIFEST srcdist dist www

.DELETE_ON_ERROR:
