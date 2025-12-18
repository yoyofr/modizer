AUTOMAKE_OPTIONS = gnu

noinst_LTLIBRARIES = libmacasm.la

COMMON_SOURCES = \
	Assembly.h \
	common.cpp

SOURCES_X86 = \
	x86_mmx.asm

SOURCES_X86_64 = \
	x86_64_mmx.asm 

LINK = $(LIBTOOL) --tag=CXX --mode=link $(CXX) -o $@

YASM = @YASM@

if HAS_YASM

if ARCH_X86
EXTRA_SOURCES = $(SOURCES_X86)
endif

if ARCH_X86_64
EXTRA_SOURCES = $(SOURCES_X86_64)
endif

STRIP_FPIC = sh $(top_srcdir)/strip_fPIC.sh
SUFFIXES = .asm

.asm.lo:
	$(LIBTOOL) --mode=compile --tag=ASM $(STRIP_FPIC) $(YASM) $(YASM_FORMAT) $(YASM_ARCH) -o $@ $< 

endif

EXTRA_DISTS = \
	Assembly.nas \
	Tools.inc

libmacasm_la_SOURCES = $(COMMON_SOURCES) $(EXTRA_SOURCES)

