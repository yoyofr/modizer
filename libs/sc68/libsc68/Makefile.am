# ,----------------------------------------------------------------------.
# |                                                                      |
# | libsc68 -- Makefile.am -- Use with automake to produce configure.in  |
# |                                                                      |
# |               Copyright (c) 2007-2016 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

SUBDIRS          = io68 emu68 dial68
ACLOCAL_AMFLAGS  = -I m4 --install
AUTOMAKE_OPTIONS = subdir-objects
LIB_VER          = @LIB_CUR@:@LIB_REV@:@LIB_AGE@

MYLIBS_LA        = io68/libio68.la emu68/libemu68.la dial68/libdial68.la

# ,----------------------------------------------------------------------.
# | Libtool library                                                      |
# `----------------------------------------------------------------------'

lib_LTLIBRARIES     = libsc68.la

libsc68_la_SOURCES  = src/api68.c src/conf68.c src/libsc68.c		\
 src/mixer68.c sc68/conf68.h sc68/mixer68.h sc68/sc68.h sc68/trap68.h	\
 sc68/sc68_private.h
libsc68_la_CFLAGS   = $(file68_CFLAGS) $(gb_CFLAGS)
libsc68_la_CPPFLAGS = -I$(top_srcdir)/sc68 $(file68_CPPFLAGS)
libsc68_la_LDFLAGS  = -version-info $(LIB_VER) $(gb_LDFLAGS)
libsc68_la_LIBADD   = $(MYLIBS_LA) $(FILE68_LA) $(file68_LIBS)

if SOURCE_FILE68

FILE68_LA = $(file68_builddir)/libfile68.la

@SET_MAKE@
$(FILE68_LA):
	cd "$(file68_builddir)" && $(MAKE) $(AM_MAKEFLAGS)

file68_CPPFLAGS = -I$(top_builddir)/$(file68_builddir)

endif


# ,----------------------------------------------------------------------.
# | Installed headers                                                    |
# `----------------------------------------------------------------------'

pkginclude_HEADERS = sc68/sc68.h
pkgincludedir      = $(includedir)/sc68

# ,----------------------------------------------------------------------.
# | pkg-config                                                           |
# `----------------------------------------------------------------------'

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = sc68.pc

# ,----------------------------------------------------------------------.
# | Assemble and transform TOS trap emulator                             |
# `----------------------------------------------------------------------'

$(builddir)/src/libsc68_la-api68.lo: $(srcdir)/sc68/trap68.h
trap68: $(srcdir)/sc68/trap68.h

bin2h=$(hexdump) -ve '1/1 "%d,\n"'

$(srcdir)/sc68/trap68.h: $(srcdir)/asm/trapfunc.s
	$(as68) $< -o trapfunc.bin >/dev/null && \
	$(bin2h) trapfunc.bin >$@ || \
	{ rm -vf -- trapfunc.bin $@; \
	echo "$@: missing required tool (hexdump or as68)" >&2; \
	test a = b; }
	rm -v -- trapfunc.bin

BUILT_SOURCES = $(FILE68_LA) $(srcdir)/sc68/trap68.h 

EXTRA_DIST = asm/trapfunc.s
MAINTAINERCLEANFILES = $(srcdir)/sc68/trap68.h
