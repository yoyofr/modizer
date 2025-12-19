AUTOMAKE_OPTIONS = gnu

noinst_LTLIBRARIES = libmacshared.la

libmacshared_la_SOURCES = \
	GlobalFunctions.cpp \
	StdLibFileIO.cpp \
	CharacterHelper.cpp \
	CircleBuffer.cpp \
	MACUtils.cpp

pkginclude_HEADERS = \
	config.h \
	All.h \
	GlobalFunctions.h \
	ID3Genres.h \
	APEIO.h \
	SmartPtr.h \
	StdLibFileIO.h \
	NoWindows.h \
	CharacterHelper.h \
	CircleBuffer.h \
	MACUtils.h

EXTRA_DIST = \
	APEInfoDialog.cpp \
	APEInfoDialog.h \
	RollBuffer.h \
	StdString.h \
	Unicows.cpp \
	WAVInfoDialog.cpp \
	WAVInfoDialog.h 

if OS_WIN
libmacshared_la_SOURCES += WinFileIO.cpp
pkginclude_HEADERS += WinFileIO.h
else
EXTRA_DIST += WinFileIO.cpp WinFileIO.h
endif

