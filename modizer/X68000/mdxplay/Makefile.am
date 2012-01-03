EXTRA_DIST = version.h.in Makefile.zaurus sin_tab.dat tl_tab.dat ym2151_noglobal.c
bin_PROGRAMS = mdxplay
mdxplay_SOURCES = ioaccess.c mdxfile.c mdxopl3.c pdxfile.c main.c \
                  mdx2151.c mdxmml_opl3.c mdxmml_ym2151.c pcm8.c ym2151.c \
                  ioaccess.h mdx.h mdxopl3.h pcm8.h version.h ym2151.h \
                  mdx2151.h gettext_wrapper.h \
                  getopt.h getopt.c getopt1.c \
                  sin_tab.dat tl_tab.dat
LDADD           = $(INTLLIBS) @LIBS@ $(ESD_LIBS) $(JCONVLIBS) ../freeverb/libfreeverb.a -lstdc++
localedir       = $(datadir)/locale
INCLUDES        = $(ESD_CFLAGS) -I$(top_srcdir)/intl -I$(top_builddir)/intl -DLOCALEDIR=\"$(localedir)\"
