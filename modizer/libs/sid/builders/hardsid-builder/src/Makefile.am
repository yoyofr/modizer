# The library.

EXTRA_DIST = hardsid-win.cpp

lib_LTLIBRARIES = libhardsid-builder.la

libhardsid_builder_la_SOURCES = hardsid-unix.cpp hardsid-builder.cpp \
hardsid-emu.h
libhardsid_builder_la_LIBADD  =

# We substitute the libtool-specific library version in configure.in.
libhardsid_builder_la_LDFLAGS = -version-info $(LIBVERSION)

# Remove bad default includes
DEFAULT_INCLUDES=
