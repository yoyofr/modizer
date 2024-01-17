dnl# -*- mode:sh; sh-basic-offset:2; indent-tabs-mode:nil -*-
dnl# 
dnl# the "configure" script is made from this by running GNU "autoconf"
dnl#
dnl# Configuration script for sc68-ng
dnl#
dnl# by Benjamin Gerard <http://sourceforge.net/users/benjihan>
dnl#

# ,----------------------------------------------------------------------.
# | Autotools and Package                                                |
# `----------------------------------------------------------------------'

m4_define([VERSION_MAJOR],3)
m4_define([VERSION_MINOR],0)
m4_define([VERSION_PATCH],0)
m4_define([VERSION_TWEAK],esyscmd([sh vcversion.sh]))dnl

AC_INIT(
  [sc68-ng],
  VERSION_MAJOR[.]VERSION_MINOR[.]VERSION_PATCH[.]VERSION_TWEAK,
  [http://sourceforge.net/projects/sc68/],
  [sc68-ng],
  [http://sc68.atari.org])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign 1.11 silent-rules dist-xz
                  no-installman no-installinfo])
AC_USE_SYSTEM_EXTENSIONS
LT_INIT([win32-dll])

# ,----------------------------------------------------------------------.
# | sc68                                                                 |
# `----------------------------------------------------------------------'

SC68_PACKAGE(
  [
    Atari ST and Amiga music player.

    This is sc68 next generation meta package. Each sub package can be
    distributed or built as a sperate package.

  ])

# ,----------------------------------------------------------------------.
# | Packages                                                             |
# `----------------------------------------------------------------------'

AC_ARG_ENABLE(
  [sc68-tools],
  [AS_HELP_STRING(
      [--enable-sc68-tools],
      [enable optionnal developer tools])],
  [],[enable_sc68_tools=tbd])

AC_ARG_ENABLE(
  [sc68-bins],
  [AS_HELP_STRING(
      [--disable-sc68-bins],
      [disable recommanded programs])],
  [],[enable_sc68_bins=yes])

AC_ARG_ENABLE(
  [sc68-libs],
  [AS_HELP_STRING(
      [--disable-sc68-libs],
      [disable required libraries])],
  [],[enable_sc68_libs=yes])

# Dev Tools
SC68_ENABLE_PACKAGE([as68],[68k macro assembler],[$enable_sc68_tools])
AS_IF([test X"$enable_sc68_tools" = Xtbd],[enable_sc68_tools=no])

SC68_ENABLE_PACKAGE([sourcer68],[68k sourcer],[$enable_sc68_tools])
SC68_ENABLE_PACKAGE([mksc68],[sc68 toolbox],[$enable_sc68_tools])
SC68_ENABLE_PACKAGE([desa68],[68k disassembler library],[$enable_sc68_tools])

# API Libraries
SC68_ENABLE_PACKAGE([unice68],[ice depacker support],[$enable_sc68_libs])
SC68_ENABLE_PACKAGE([file68],[file68 library],[$enable_sc68_libs])
SC68_ENABLE_PACKAGE([libsc68],[sc68 engine],[$enable_sc68_libs])

# CLI programs
SC68_ENABLE_PACKAGE([info68],[sc68 files info program],[$enable_sc68_bins])
SC68_ENABLE_PACKAGE([sc68],[sc68 command line player],[$enable_sc68_bins])

# DOC (disable for now)
SC68_ENABLE_PACKAGE([sc68-doc],[sc68 documentation],[no])

build_as68='$(builddir)/../as68/as68$(EXEEXT)'
AS_IF(
  [test X"$enable_as68" = Xtbd],
  [
    AS_IF(
      [test "$enable_libsc68/$cross_compiling" != yes/no],
      [enable_as68=no],
      [AC_MSG_CHECKING([for trap68.h i][n libsc68])
       AS_IF(
         [test -e "$srcdir/libsc68/sc68/trap68.h"],
         [enable_as68=no;
          AC_MSG_RESULT([yes])],
         [AC_MSG_RESULT([no])
          AS_IF([test X"$as68" = X"$build_as68"],
                [AS_UNSET([as68])
                 AS_UNSET([ac_cv_prog_as68])])
          AC_CHECK_PROG([as68],[as68],[],[false])
          AS_IF([test X"$as68" = Xfalse],
                [AC_MSG_NOTICE([will build as68 to bootstrap libsc68.])
                 AS_UNSET([as68])
                 AS_UNSET([ac_cv_prog_as68])
                 enable_as68=yes],
                [enable_as68=no])])])
  ])

# ,----------------------------------------------------------------------.
# | Package Dependencies                                                 |
# `----------------------------------------------------------------------'

# Do not check dependencies on the source tree. Some packages may not
# be present here but available by others means (may be installed or
# out of tree sources).

# ,----------------------------------------------------------------------.
# | Subdirs                                                              |
# `----------------------------------------------------------------------'

AS_IF([test Xyes = X"$enable_as68"],
      [AC_CONFIG_SUBDIRS([as68])
       AS_IF(
         [test yes = "$enable_libsc68${as68+no}"],
         [ac_configure_args="$ac_configure_args 'as68=$build_as68'"])
      ])

AS_IF([test Xyes = X"$enable_desa68"],
      [AC_CONFIG_SUBDIRS([desa68])])

AS_IF([test Xyes = X"$enable_unice68"],
      [AC_CONFIG_SUBDIRS([unice68])])

AS_IF([test Xyes = X"$enable_file68"],
      [AC_CONFIG_SUBDIRS([file68])])

AS_IF([test Xyes = X"$enable_info68"],
      [AC_CONFIG_SUBDIRS([info68])])

AS_IF([test Xyes = X"$enable_sourcer68"],
      [AC_CONFIG_SUBDIRS([sourcer68])])

AS_IF([test Xyes = X"$enable_libsc68"],
      [AC_CONFIG_SUBDIRS([libsc68])])

AS_IF([test Xyes = X"$enable_sc68"],
      [AC_CONFIG_SUBDIRS([sc68])])

AS_IF([test Xyes = X"$enable_mksc68"],
      [AC_CONFIG_SUBDIRS([mksc68])])

# AS_IF([test Xyes = X"$enable_sc68_doc"],
#       [AC_CONFIG_SUBDIRS([sc68-doc])])

AC_SUBST([subdirs])

# ,----------------------------------------------------------------------.
# | Output                                                               |
# `----------------------------------------------------------------------'

AC_CONFIG_FILES([Makefile])
SC68_OUTPUT

# ,----------------------------------------------------------------------.
# | Summary                                                              |
# `----------------------------------------------------------------------'

[subdirs=`echo $subdirs`]
AC_MSG_NOTICE([])
AC_MSG_NOTICE([,-----------------------])
AC_MSG_NOTICE([| $PACKAGE_NAME ($PACKAGE_VERSION)])
AC_MSG_NOTICE([|-----------------------])
AC_MSG_NOTICE([|   sub-dirs            : $subdirs])
AC_MSG_NOTICE([+-----------------------])
AC_MSG_NOTICE([])
