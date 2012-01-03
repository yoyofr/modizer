## Process this file with automake to create Makefile.in

resid_headers=siddefs.h sid.h voice.h wave.h envelope.h filter.h extfilt.h pot.h spline.h
resid_sources=sid.cc voice.cc wave.cc envelope.cc filter.cc extfilt.cc pot.cc version.cc $(noinst_DATA:.dat=.cc)

if INSTALL_RESID
lib_LTLIBRARIES = libresid.la
pkginclude_HEADERS = $(resid_headers)
else
noinst_LTLIBRARIES = libresidc.la
noinst_HEADERS = $(resid_headers)

all-local:
	ln -sf libresidc.la libresid.la

endif

## Make sure these will be cleaned even when they're not built by
## default.
CLEANFILES = libresid.la libresidc.la

libresid_la_SOURCES = $(resid_sources)
libresidc_la_SOURCES = $(resid_sources)

BUILT_SOURCES = $(noinst_DATA:.dat=.cc)

noinst_DATA = wave6581_PST.dat wave6581_PS_.dat wave6581_P_T.dat wave6581__ST.dat wave8580_PST.dat wave8580_PS_.dat wave8580_P_T.dat wave8580__ST.dat

noinst_SCRIPTS = samp2src.pl

EXTRA_DIST = $(noinst_DATA) $(noinst_SCRIPTS)

SUFFIXES = .dat

.dat.cc:
	$(PERL) $(srcdir)/samp2src.pl $* $< $(srcdir)/$@
