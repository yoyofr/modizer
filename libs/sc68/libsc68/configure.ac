dnl# -*- mode:sh; sh-basic-offset:2; indent-tabs-mode:nil -*-
dnl#
dnl# the "configure" script is made from this by running GNU "autoconf"
dnl#
dnl# Configuration script for libsc68
dnl#
dnl# by Benjamin Gerard <http://sourceforge.net/users/benjihan>
dnl#

# ,----------------------------------------------------------------------.
# | Autotools and Package                                                |
# `----------------------------------------------------------------------'

dnl # This library uses interface versioning
m4_define([LIBCUR],3)dnl #
m4_define([LIBREV],0)dnl #
m4_define([LIBAGE],0)dnl #

m4_define([VERSION_MAJOR],3)
m4_define([VERSION_MINOR],0)
m4_define([VERSION_PATCH],0)
m4_define([VERSION_TWEAK],esyscmd([sh vcversion.sh]))dnl

AC_INIT(
  [libsc68],
  VERSION_MAJOR[.]VERSION_MINOR[.]VERSION_PATCH[.]VERSION_TWEAK,
  [http://sourceforge.net/projects/sc68/],
  [libsc68],
  [http://sc68.atari.org])
AC_CONFIG_SRCDIR([src/api68.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign 1.11 silent-rules dist-xz subdir-objects
                  no-installman no-installinfo])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS
LT_INIT([win32-dll])
LT_LANG([Windows Resource])

AC_SUBST([VER_MAJOR],[VERSION_MAJOR])
AC_SUBST([VER_MINOR],[VERSION_MINOR])
AC_SUBST([VER_PATCH],[VERSION_PATCH])
AC_SUBST([VER_TWEAK],[VERSION_TWEAK])

# ,----------------------------------------------------------------------.
# | sc68                                                                 |
# `----------------------------------------------------------------------'

SC68_PACKAGE(
  [

    sc68 engine library.

    libsc68 is the core of sc68 simulation and sc68 main API.

  ])

SC68_CC
AC_PROG_INSTALL

SC68_WITH_PACKAGE([file68],[file68],[file68],[../file68],
                  [sc68/file68.h],[file68_version],[req])

# ,----------------------------------------------------------------------.
# | Needed programs                                                      |
# `----------------------------------------------------------------------'

AC_CHECK_PROG([hexdump],[hexdump],[hexdump],[false hexdump])
AC_CHECK_PROG([as68],[as68],[as68],[false as68])

# ,----------------------------------------------------------------------.
# | Optional features                                                    |
# `----------------------------------------------------------------------'

AC_ARG_ENABLE([dialog],
  [AS_HELP_STRING([--enable-dialog],
      [compile dialog helpers for this sub-system @<:@yes*|win32@:>@])],
  [],[enable_dialog=check])

AS_IF([test "X$enable_dialog" = Xcheck],
      [AS_CASE(["$host_os"],
               [mingw32|mingw64|win32|win64],
               [enable_dialog=winui
                LT_LANG([Windows Resource])
                dnl #LT_PROG_RC
               ],
               [enable_dialog=no])])

AS_IF([test "X$enable_dialog" != Xno],
      [AC_DEFINE_UNQUOTED([USE_DIALOG],["[$]enable_dialog"],
                          [Using sc68 dialog helpers])])

AM_CONDITIONAL([use_winui],[test "X$enable_dialog" = Xwinui])
dnl #AM_CONDITIONAL([dll_winui],[test A = A])

AC_ARG_ENABLE(
  [emu68-monolitic],
  [AS_HELP_STRING([--enable-emu68-monolitic],
      [compile 68k instructions as a single huge file @<:@default=no@:>@])],
  [],[enable_emu68_monolitic='no'])
AM_CONDITIONAL([emu68_monolitic],[test "X${enable_emu68_monolitic}" = 'Xyes'])

AC_ARG_WITH(
  [ym-engine],
  [AS_HELP_STRING([--with-ym-engine],
      [set default YM-2149 engine (pulse|blep) @<:@default=auto-select@:>@])],
  [],[with_ym_engine=''])

AS_CASE(
  ["X$with_ym_engine"],
  [Xblep],
  [YM_ENGINE='YM_ENGINE_BLEP'; with_ym_engine='blep synthesis'],

  [Xdump],
  [YM_ENGINE='YM_ENGINE_DUMP'; with_ym_engine='dump registers'
   AC_MSG_WARN([The ym dump engine do not produce sound.])],

  [Xpulse],
  [YM_ENGINE='YM_ENGINE_PULS'; with_ym_engine='pulse (legacy engine)'],

  [X|Xyes|Xno],
  [YM_ENGINE=''; with_ym_engine='default (should be blep)'],

  [YM_ENGINE="YM_ENGINE_"[]AS_TR_CPP($with_ym_engine)
   with_ym_engine="unknown (3rd party?) $with_ym_engine engine"
   AC_MSG_WARN([Using an unknown (may be third party) YM engine])])

AS_IF(
  [test "X$YM_ENGINE" != X],
  [AC_DEFINE_UNQUOTED([YM_ENGINE],[$YM_ENGINE],[set default YM engine])])

# ,----------------------------------------------------------------------.
# | Sized size integers                                                  |
# `----------------------------------------------------------------------'

AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INTMAX_T
AC_TYPE_LONG_LONG_INT
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINTMAX_T
AC_TYPE_UNSIGNED_LONG_LONG_INT

# ,----------------------------------------------------------------------.
# | Functions and Headers                                                |
# `----------------------------------------------------------------------'

AC_CHECK_HEADERS([limits.h assert.h])
AC_HEADER_ASSERT
AC_CHECK_HEADERS([stdarg.h stdint.h stdio.h stdlib.h string.h])
AC_CHECK_HEADERS([ctype.h errno.h libgen.h])

AC_CHECK_FUNCS(
  [malloc free vsprintf vsnprintf getenv strtol strtoul stpcpy basename])

# ,----------------------------------------------------------------------.
# | Output                                                               |
# `----------------------------------------------------------------------'
AC_CONFIG_FILES(
  [Makefile
   asm/version.s
   emu68/Makefile
   io68/Makefile
   dial68/Makefile
   dial68/winui/Makefile
   sc68.pc])
SC68_OUTPUT

# ,----------------------------------------------------------------------.
# | Summary                                                              |
# `----------------------------------------------------------------------'

AC_MSG_NOTICE([])
AC_MSG_NOTICE([,-----------------------])
AC_MSG_NOTICE([| $PACKAGE_NAME ($PACKAGE_VERSION) for $host])
AC_MSG_NOTICE([|-----------------------])
AC_MSG_NOTICE([|   file68              : $has_file68 ($file68_VERSION)])
AC_MSG_NOTICE([|   default YM engine   : $with_ym_engine])
AC_MSG_NOTICE([|   dialog helpers      : $enable_dialog])
AC_MSG_NOTICE([+-----------------------])
