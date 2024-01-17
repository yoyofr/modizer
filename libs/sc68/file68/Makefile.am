# ,----------------------------------------------------------------------.
# |                                                                      |
# |  file68 -- Makefile.am -- Use with automake to produce configure.in  |
# |                                                                      |
# |               Copyright (c) 2007-2016 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

SUBDIRS          = data68
ACLOCAL_AMFLAGS  = -I m4 --install
AUTOMAKE_OPTIONS = subdir-objects
LIB_VER          = @LIB_CUR@:@LIB_REV@:@LIB_AGE@

# ,----------------------------------------------------------------------.
# | Source files                                                         |
# `----------------------------------------------------------------------'

mysources = src/file68_private.h src/error68.c src/file68.c		\
 src/gzip68.c src/ice68.c src/init68.c src/vfs68.c src/vfs68_ao.c	\
 src/vfs68_curl.c src/vfs68_fd.c src/vfs68_file.c src/vfs68_mem.c	\
 src/vfs68_null.c src/vfs68_z.c src/msg68.c src/option68.c		\
 src/registry68.c src/rsc68.c src/string68.c src/timedb68.c		\
 src/uri68.c
if REPLAY68
mysources += src/replay68.c
endif

apiheaders = sc68/file68_chk.h sc68/file68_ord.h sc68/file68_err.h      \
 sc68/file68.h sc68/file68_api.h sc68/file68_features.h                 \
 sc68/file68_vfs.h sc68/file68_vfs_def.h sc68/file68_msg.h              \
 sc68/file68_opt.h sc68/file68_reg.h sc68/file68_rsc.h                  \
 sc68/file68_str.h sc68/file68_tag.h sc68/file68_uri.h                  \
 sc68/file68_tdb.h sc68/file68_zip.h sc68/file68_ice.h

otherheaders = sc68/file68_vfs_ao.h sc68/file68_vfs_curl.h		\
  sc68/file68_vfs_fd.h sc68/file68_vfs_file.h sc68/file68_vfs_mem.h	\
  sc68/file68_vfs_null.h sc68/file68_vfs_z.h src/timedb.inc.h		\
  src/replay.inc.h

myheaders = $(apiheaders) $(otherheaders)

# ,----------------------------------------------------------------------.
# | Libtool library                                                      |
# `----------------------------------------------------------------------'

lib_LTLIBRARIES = libfile68.la

libfile68_la_SOURCES = $(mysources) $(myheaders)
libfile68_la_CFLAGS =\
 $(unice68_CFLAGS) $(z_CFLAGS) $(ao_CFLAGS) $(curl_CFLAGS) $(gb_CFLAGS)
libfile68_la_CPPFLAGS = -I$(top_srcdir)/sc68
libfile68_la_LDFLAGS = -version-info $(LIB_VER) $(gb_LDFLAGS)
libfile68_la_LIBADD =\
 $(UNICE68_LA) $(unice68_LIBS) $(curl_LIBS) $(ao_LIBS) $(z_LIBS)

if SOURCE_UNICE68

UNICE68_LA = $(unice68_builddir)/libunice68.la

@SET_MAKE@
$(UNICE68_LA):
	cd "$(unice68_builddir)" && $(MAKE) $(AM_MAKEFLAGS)

endif

BUILT_SOURCES = $(UNICE68_LA)

# ,----------------------------------------------------------------------.
# | Headers                                                              |
# `----------------------------------------------------------------------'

pkginclude_HEADERS = $(apiheaders)
pkgincludedir      = $(includedir)/sc68

# ,----------------------------------------------------------------------.
# | pkg-config                                                           |
# `----------------------------------------------------------------------'

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = file68.pc
