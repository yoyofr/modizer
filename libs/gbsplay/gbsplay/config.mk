EXTRA_ALL := 
EXTRA_CFLAGS := -D_DARWIN_C_SOURCE=1 -I/opt/homebrew/include/SDL2  -std=c17 -Wdeclaration-after-statement -Wvla -Wimplicit-fallthrough -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fstack-clash-protection -Wall -fsigned-char -Os -pipe -fomit-frame-pointer
EXTRA_INSTALL := 
EXTRA_LDFLAGS := -lz -Wl,-pie -fstack-protector-strong -fstack-clash-protection
EXTRA_SRCS := 
EXTRA_UNINSTALL := 
VERSION := ish
prefix := /usr/local
exec_prefix := /usr/local
bindir := /usr/local/bin
libdir := /usr/local/lib
mandir := /usr/local/man
docdir := /usr/local/share/doc/gbsplay
localedir := /usr/local/share/locale
mimedir := /usr/local/share/mime
appdir := /usr/local/share/applications
includedir := /usr/local/include/libgbs
pkgconfigdir := /usr/local/share/pkgconfig
sysconfdir := /etc
CC := gcc
BUILDCC := gcc
HOSTCC := gcc
build_contrib := yes
build_test := yes
build_xgbsplay := no
have_doxygen := yes
have_xgettext := yes
use_i18n := no
use_sharedlibgbs := no
use_verbosebuild := no
windows_build := no
windows_libprefix := 
libaudio_flags := 
libpipewire_0_3_flags := 
libSDL2_flags := -L/opt/homebrew/lib  -lSDL2
plugout_alsa := no
plugout_devdsp := no
plugout_dsound := no
plugout_iodumper := yes
plugout_midi := yes
plugout_altmidi := yes
plugout_nas := no
plugout_pipewire :=
plugout_pulse := no
plugout_sdl := yes
plugout_stdout := yes
plugout_vgm := yes
plugout_wav := yes
configured := yes