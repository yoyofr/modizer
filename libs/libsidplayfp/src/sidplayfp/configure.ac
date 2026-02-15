dnl Process this file with autoconf to produce a configure script.
AC_INIT([sidplayfp], [3.0.0a], [], [], [https://github.com/libsidplayfp/sidplayfp/])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST
case "$host" in
    *mingw* )
        MINGW32=yes
        ;;
    *darwin* )
        MACOSX=yes
        ;;
esac
AM_CONDITIONAL([MINGW32], [test "x$MINGW32" = "xyes"])

AC_PROG_RANLIB

dnl Checks for programs.
AC_PROG_CXX

dnl Use C++ for tests.
AC_LANG([C++])

AX_CXX_COMPILE_STDCXX([20], [noext], [optional])
AS_IF([test $ax_cv_cxx_compile_cxx20__std_cpp20 != "yes"], [
    AX_CXX_COMPILE_STDCXX([17], [noext], [optional])
    AS_IF([test $ax_cv_cxx_compile_cxx17__std_cpp17 != "yes"], [
        AX_CXX_COMPILE_STDCXX([14], [noext], [optional])
        AS_IF([test $ax_cv_cxx_compile_cxx14__std_cpp14 != "yes"],
            [AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])]
        )
    ])
])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

AM_ICONV
AM_CONDITIONAL([USE_ICONV], [test "x$am_cv_func_iconv" = "xyes"])

PKG_CHECK_MODULES(SIDPLAYFP, [libsidplayfp >= 2.0])
PKG_CHECK_MODULES(STILVIEW, [libstilview >= 1.0])

# check for builders
PKG_CHECK_VAR([SIDPLAYPF_INCLUDE], [libsidplayfp], [includedir])

AC_MSG_CHECKING([for $SIDPLAYPF_INCLUDE/sidplayfp/builders/residfp.h])
AS_IF([test -f $SIDPLAYPF_INCLUDE/sidplayfp/builders/residfp.h],
    [AC_DEFINE([HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H], [1], [residfp builder])
    AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])]
)
AC_MSG_CHECKING([for $SIDPLAYPF_INCLUDE/sidplayfp/builders/sidlite.h])
AS_IF([test -f $SIDPLAYPF_INCLUDE/sidplayfp/builders/sidlite.h],
    [AC_DEFINE([HAVE_SIDPLAYFP_BUILDERS_SIDLITE_H], [1], [sidlite builder])
    AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])]
)
AC_MSG_CHECKING([for $SIDPLAYPF_INCLUDE/sidplayfp/builders/resid.h])
AS_IF([test -f $SIDPLAYPF_INCLUDE/sidplayfp/builders/resid.h],
    [AC_DEFINE([HAVE_SIDPLAYFP_BUILDERS_RESID_H], [1], [resid builder])
    AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])]
)
AC_MSG_CHECKING([for $SIDPLAYPF_INCLUDE/sidplayfp/builders/hardsid.h])
AS_IF([test -f $SIDPLAYPF_INCLUDE/sidplayfp/builders/hardsid.h],
    [AC_DEFINE([HAVE_SIDPLAYFP_BUILDERS_HARDSID_H], [1], [hardsid builder])
    AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])]
)
AC_MSG_CHECKING([for $SIDPLAYPF_INCLUDE/sidplayfp/builders/exsid.h])
AS_IF([test -f $SIDPLAYPF_INCLUDE/sidplayfp/builders/exsid.h],
    [AC_DEFINE([HAVE_SIDPLAYFP_BUILDERS_EXSID_H], [1], [exsid builder])
    AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])]
)
AC_MSG_CHECKING([for $SIDPLAYPF_INCLUDE/sidplayfp/builders/usbsid.h])
AS_IF([test -f $SIDPLAYPF_INCLUDE/sidplayfp/builders/usbsid.h],
    [AC_DEFINE([HAVE_SIDPLAYFP_BUILDERS_USBSID_H], [1], [usbsid builder])
    AC_MSG_RESULT([yes])],
    [AC_MSG_RESULT([no])]
)

AC_MSG_CHECKING([whether to build for debugging])
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [compile for debugging @<:@no/yes, default=no@:>@])],
 [], [enable_debug=no])

AS_IF([test "x$enable_debug" = "xno"],
  [AC_MSG_RESULT([no]); debug_flags=-DNDEBUG],
  [AC_MSG_RESULT([yes])],
)

AC_SUBST([debug_flags])

AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT
