dnl# -*- mode:sh; sh-basic-offset:2; indent-tabs-mode:nil -*-
dnl#
dnl# the "configure" script is made from this by running GNU "autoconf"
dnl#
dnl# Configuration script for file68
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
  [file68],
  VERSION_MAJOR[.]VERSION_MINOR[.]VERSION_PATCH[.]VERSION_TWEAK,
  [http://sourceforge.net/projects/sc68/],
  [libfile68],
  [http://sc68.atari.org])
AC_CONFIG_SRCDIR([src/file68.c])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign 1.11 silent-rules dist-xz subdir-objects
                  no-installman no-installinfo])
AC_CONFIG_HEADERS([config.h sc68/file68_features.h])
AC_USE_SYSTEM_EXTENSIONS
LT_INIT([win32-dll])

# ,----------------------------------------------------------------------.
# | sc68                                                                 |
# `----------------------------------------------------------------------'

SC68_PACKAGE(
  [

    sc68 file and utility library.

    The main goal of this library was to manipulated various resource
    needed by sc68. Mainly but not limited to music file loading. Over
    the time it became more of a utility library handling a more
    things such as options, virtual filesystem and  message logging.

  ])

SC68_CC
AC_PROG_INSTALL

# ,----------------------------------------------------------------------.
# | Optional Features                                                    |
# `----------------------------------------------------------------------'

AC_ARG_ENABLE(
  [file68-data],
  [AS_HELP_STRING([--enable-file68-data],
      [file68 shared data to be installed @<:@default=check@:>@])],
  [],[enable_file68_data=check])

AC_ARG_ENABLE(
  [file],
  [AS_HELP_STRING([--enable-file],
      [FILE stream support @<:@default=check@:>@])],
  [],[enable_file=check])

AC_ARG_ENABLE(
  [fd],
  [AS_HELP_STRING([--enable-fd],
      [file descriptor stream support @<:@default=check@:>@])],
  [],[enable_fd=check])

AC_ARG_ENABLE(
  [mem],
  [AS_HELP_STRING([--enable-mem],
      [memory stream support @<:@default=yes@:>@])],
  [],[enable_mem=yes])

AC_ARG_ENABLE(
  [registry],
  [AS_HELP_STRING([--enable-registry],
      [windows registry support @<:@default=check@:>@])],
  [],[enable_reg=check])

AC_ARG_ENABLE(
  [replay-rom],
  [AS_HELP_STRING([--enable-replay-rom],
      [add built-in replay (~130kB) @<:@default=no@:>@])],
  [],[enable_replay_rom=no])

# ,----------------------------------------------------------------------.
# | External libraries                                                   |
# `----------------------------------------------------------------------'

AS_UNSET([PAC_REQUIRES])

# ------------------------------------------------------------------------
# unice68

SC68_WITH_PACKAGE([unice68],[unice68],[unice68],
                  [../unice68],[unice68.h],[unice68_version])

# ------------------------------------------------------------------------
# zlib

SC68_WITH_MODULE([z],[zlib],[zlib.h],[gzopen])

# ------------------------------------------------------------------------
# libao

SC68_WITH_MODULE([ao],[ao],[ao/ao.h],[ao_initialize])
AS_IF([test X"$has_ao" = Xyes],
      [SC68_CHECK_FUNCS([ao],[ao_file_extension])])

# ------------------------------------------------------------------------
# cURL

SC68_WITH_MODULE([curl],[libcurl],[curl/curl.h],[curl_global_init])
AS_IF(
  [test "X${curl_SCHEMES+set}" != Xset],
  [
    curl_relevant_schemes='FILE HTTP HTTPS FTP FTPS SFTP SMB SMBS'
    AS_IF(
      [test "$has_curl/$curl_PKG_EXISTS" = 'yes/yes'],
      [SC68_PKG_CONFIG([curl],[libcurl],
                       [PROTOCOLS],[--variable=supported_protocols])
       AS_IF(
         [test "X${curl_PROTOCOLS+set}" = Xset],
         [set -- $curl_PROTOCOLS
          while test [$]# -ne 0; do
            for curl_scheme in $curl_relevant_schemes; do
              AS_IF([test "X$curl_scheme" = "X$1"],
                    [curl_SCHEMES="${curl_SCHEMES-}${curl_SCHEMES+ }$1"
                     break])
            done; shift
          done],
         [curl_SCHEMES="$curl_relevant_schemes"])
      ],
      [curl_SCHEMES="$curl_relevant_schemes"])
  ])

AS_IF(
  [test "x$has_curl" = xno],
  [AS_UNSET([curl_SCHEMES])],
  [
    AC_DEFINE_UNQUOTED(
      [CURL_SCHEMES],
      [`AS_ECHO_N(["$curl_SCHEMES"])|sed -e 's/ \{1,\}/,/g; s/\(@<:@_a-zA-Z0-9@:>@\{1,\}\)/"\1"/g'`],
      [Supported curl schemes])
   ])

AC_ARG_VAR([curl_SCHEMES],
           [List of space separated relevant schemes (protocols)
            supported by the cURL library.])

# Windows registry support
# ------------------------
AS_IF(
  [test "X$enable_reg" = Xcheck],
  [AS_CASE(["X$host_os"],[Xmingw*],
           [AC_CHECK_HEADERS([windows.h])
            AC_CHECK_HEADERS(
              [winreg.h],
              [
                AC_MSG_CHECKING([for Windows registry functions])
                AC_LINK_IFELSE(
                  [AC_LANG_SOURCE(
                      [#include <windows.h>]
                      [#include <winreg.h>]
                      [int main(int na, char ** a) {
                          return (int)RegCloseKey((HKEY)0);
                        }])
                  ],[enable_reg=yes],[enable_reg=no])
                AC_MSG_RESULT([$enable_reg])
              ],
              [],
              [#include <windows.h>])
           ])
  ])
AS_IF([test "X$enable_reg" = Xyes],
      [AC_DEFINE([USE_REGISTRY68],[1],[Using Windows registry])],
      [enable_reg=no])

# ,----------------------------------------------------------------------.
# | Functions and Headers                                                |
# `----------------------------------------------------------------------'

AC_CHECK_HEADERS([limits.h assert.h])
AC_HEADER_ASSERT
AC_CHECK_HEADERS([stdarg.h stdint.h stdio.h stdlib.h string.h])
AC_CHECK_HEADERS([unistd.h ctype.h errno.h fcntl.h])
AC_CHECK_HEADERS([sys/stat.h sys/types.h])

AC_CHECK_FUNCS(
  [malloc free getenv sleep usleep vsprintf vsnprintf fsync fdatasync])

# ,----------------------------------------------------------------------.
# | VFS to support                                                       |
# `----------------------------------------------------------------------'

# FILE stream support
# -------------------
AS_IF([test "X$enable_file" = Xcheck],
      [enable_file="$ac_cv_header_stdio_h"])
AS_IF([test "X$enable_file" = Xno],
      [AC_DEFINE([ISTREAM68_NO_FILE],[1],[Disable FILE stream support])])

# FD stream support
# -----------------
AS_IF([test "X$enable_fd" = Xcheck],
      [enable="$ac_cv_header_unistd_h"])
AS_IF([test "X$enable_fd" = Xno],
      [AC_DEFINE([ISTREAM68_NO_FD],[1],
                 [Disable file decriptor stream support])])

# MEM stream support
# ------------------
AS_IF([test "X$enable_mem" = Xno],
      [AC_DEFINE([ISTREAM68_NO_MEM],[1],[Disable memory stream support])])

# DATA for file68
# ---------------
sc68datadir='${datadir}/sc68'
AC_SUBST([sc68datadir])

AC_MSG_CHECKING([whether file68 data will be installed])
AS_CASE(["X$enable_file68_data/$host_os"],
        [Xno/* | Xcheck/mingw*],
        [enable_file68_data=no
         AC_MSG_RESULT([$enable_file68_data])],
        [enable_file68_data=yes
         AC_MSG_RESULT([$enable_file68_data @<:@$sc68datadir@:>@])])
AM_CONDITIONAL([FILE68_DATA],[test "X$enable_file68_data" = Xyes])

dnl # This can not be in config.h because we need the variable
dnl # substitutions to happen at compile time.

AS_CASE(["X$host_os"],
        [Xmingw*],[file68_datadir=0],
        [file68_datadir='\"$(datadir)/sc68\"'])
CPPFLAGS="-DFILE68_SHARED_PATH=${file68_datadir}${CPPFLAGS:+ }${CPPFLAGS}"

# Built-in replay rom
# -------------------
AM_CONDITIONAL([REPLAY68],[test "X$enable_replay_rom" = Xyes])
AC_MSG_CHECKING([whether external replays are built-in])
AS_IF([test "X$enable_replay_rom" = Xyes],
      [AC_DEFINE([USE_REPLAY68],[1],[Built-in replays])],
      [enable_replay_rom=no])
AC_MSG_RESULT([$enable_replay_rom])

# ,----------------------------------------------------------------------.
# | File68 features
# `----------------------------------------------------------------------'

AS_UNSET([file68_features])
AS_IF(
  [test "X$has_unice68" = Xyes],
  [file68_features="${file68_features-}${file68_features+ }ice"])
AS_IF(
  [test "X$has_z" = Xyes],
  [file68_features="${file68_features-}${file68_features+ }z"])
AS_IF(
  [test "X$has_curl" = Xyes],
  [file68_features="${file68_features-}${file68_features+ }curl"])
AS_IF(
  [test "X$has_ao" = Xyes],
  [file68_features="${file68_features-}${file68_features+ }ao"])
AC_SUBST([file68_features])

# ,----------------------------------------------------------------------.
# | Output                                                               |
# `----------------------------------------------------------------------'

AC_CONFIG_FILES([file68.pc Makefile data68/Makefile])
SC68_OUTPUT

# ,----------------------------------------------------------------------.
# | Summary                                                              |
# `----------------------------------------------------------------------'

AS_UNSET([vfs])
AS_IF([test "X$enable_file" = Xyes],[vfs="${vfs-}${vfs+,}file"])
AS_IF([test "X$enable_fd"   = Xyes],[vfs="${vfs-}${vfs+,}fd"])
AS_IF([test "X$enable_mem"  = Xyes],[vfs="${vfs-}${vfs+,}memory"])

AC_MSG_NOTICE([])
AC_MSG_NOTICE([,----------------------])
AC_MSG_NOTICE([| $PACKAGE_NAME ($PACKAGE_VERSION) for $host])
AC_MSG_NOTICE([|----------------------])
AC_MSG_NOTICE([|   supported VFS       : ${vfs-<none>}])
AC_MSG_NOTICE([|   windows registry    : $enable_reg])
AC_MSG_NOTICE([|   built-in replays    : $enable_replay_rom])
AC_MSG_NOTICE([|   ICE! support        : $has_unice68 ($unice68_VERSION)])
AC_MSG_NOTICE([|   ao support          : $has_ao ($ao_VERSION)])
AC_MSG_NOTICE([|   zlib support        : $has_z ($z_VERSION)])
AC_MSG_NOTICE([|   curl support        : $has_curl ($curl_VERSION)])
AC_MSG_NOTICE([|   curl schemes        : ${curl_SCHEMES-<unsupported>}])
AC_MSG_NOTICE([|   sc68 data dir       : $sc68datadir])
AC_MSG_NOTICE([+----------------------])
