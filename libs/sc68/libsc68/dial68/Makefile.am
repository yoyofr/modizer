# ,----------------------------------------------------------------------.
# |                                                                      |
# |  dial68 -- Makefile.am -- Use with automake to produce Makefile.in   | 
# |                                                                      |
# |               Copyright (c) 2007-2016 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

SUBDIRS         = $(winui)
ACLOCAL_AMFLAGS = -I m4 --install

# ,----------------------------------------------------------------------.
# | Libtool library                                                      |
# `----------------------------------------------------------------------'

if use_winui
winui                = winui
dialsub              = winui/libwinui.la
else
winui=
dialsub=
endif

noinst_LTLIBRARIES    = libdial68.la
libdial68_la_SOURCES  = dial68.c dial_conf.c dial_tsel.c dial_finf.c
libdial68_la_LDFLAGS  = -avoid-version $(lib_LDFLAGS)
libdial68_la_LIBADD   = $(dialsub)

libdial68_la_CFLAGS   = $(file68_CFLAGS)
libdial68_la_CPPFLAGS = -I$(top_srcdir) $(file68_CPPFLAGS)
noinst_HEADERS        = dial68.h
