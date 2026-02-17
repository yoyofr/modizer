m4_define([lib_major], [0])
m4_define([lib_minor], [9])
m4_define([lib_level], [1])
m4_define([lib_version], [lib_major.lib_minor.lib_level])

AC_PREREQ([2.62])
AC_INIT([libresidfp],[lib_version],[],[],[https://github.com/libsidplayfp/libresidfp/])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([src/SID.cpp])
AM_INIT_AUTOMAKE

LIB_MAJOR=lib_major
LIB_MINOR=lib_minor
LIB_LEVEL=lib_level
AC_SUBST([LIB_MAJOR])
AC_SUBST([LIB_MINOR])
AC_SUBST([LIB_LEVEL])

AC_CANONICAL_HOST

AS_CASE([$host_os],
        [mingw*], [MINGW32=yes],
        [darwin*], [MACOS=yes],
        [MINGW32=no MACOS=no]
)
AM_CONDITIONAL([MINGW32], [test "x$MINGW32" = "xyes"])
AM_CONDITIONAL([MACOS], [test "x$MACOS" = "xyes"])

dnl Initialize libtool.
LT_INIT([win32-dll])

dnl Checks for programs.
AC_PROG_CXX

dnl Use C++ for tests.
AC_LANG([C++])


AX_CXX_COMPILE_STDCXX([23], [noext], [optional])
AS_IF([test $ax_cv_cxx_compile_cxx23__std_cpp23 != "yes"], [
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
])

dnl check for hidden visibility support
AX_APPEND_COMPILE_FLAGS([-fvisibility=hidden -fvisibility-inlines-hidden])

dnl enable fast math
AX_APPEND_COMPILE_FLAGS([-ffast-math -fno-unsafe-math-optimizations], [RESIDFP_CXXFLAGS])
AC_SUBST([RESIDFP_CXXFLAGS])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_BIGENDIAN

AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])

AS_IF([test $ac_cv_sizeof_short -ne 2],
  [AC_MSG_ERROR([size of short must be 16 bit])]
)

AS_IF([test $ac_cv_sizeof_int -lt 4],
  [AC_MSG_ERROR([only 32 bit or better CPUs are supported])]
)

AS_IF([test "x${ac_cv_header_stdint_h}" != "xyes"],
  [AC_MSG_ERROR([Required header stdint.h not found])]
)

dnl libtool-style version-info number
#
# https://autotools.io/libtool/version.html
#
# Always increase the revision value.
#
# Increase the current value whenever an interface has been added, removed or changed.
#
# Increase the age value only if the changes made to the ABI are backward compatible.

LIBRESIDFPCUR=0
LIBRESIDFPREV=0
LIBRESIDFPAGE=0
LIBRESIDFPVERSION=$LIBRESIDFPCUR:$LIBRESIDFPREV:$LIBRESIDFPAGE

AC_MSG_CHECKING([for debugging])
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [compile for debugging @<:@no/yes, default=no@:>@])],
 [], [enable_debug=no])

AS_IF([test "x$enable_debug" = "xno"],
  [AC_MSG_RESULT([Build for release])
  debug_flags=-DNDEBUG
  RESIDFP_INLINE=inline
  RESIDFP_INLINING=1],
  [AC_MSG_RESULT([Build for debugging])
  debug_flags=""
  RESIDFP_INLINE=""
  RESIDFP_INLINING=0
  AX_APPEND_COMPILE_FLAGS([-Wall -Werror -pedantic])
  ]
)

AC_SUBST([debug_flags])

dnl define compiler flag to enable SIMD instructions, not required if -march or -mcpu is defined
AC_MSG_CHECKING([for SIMD instructions])
AC_ARG_WITH(
    [simd],
    [AS_HELP_STRING([--with-simd=], [Build with SIMD optimizations @<:@runtime/mmx/sse2/sse4/avx2/avx512f/none, default=none@:>@])],
    [],
    [with_simd=none]
)

AS_IF([test x"$with_simd" != xnone],
    [AS_CASE([$with_simd],
        [runtime], [AC_DEFINE([RUNTIME_DISPATCH], 1, [Define to 1 to enable runtime SIMD dispatch.])],
        [mmx], [SID_X86_SIMD_SUPPORTS([mmx])],
        [sse2], [SID_X86_SIMD_SUPPORTS([sse2])],
        [sse4], [SID_X86_SIMD_SUPPORTS([sse4])],
        [avx2], [SID_X86_SIMD_SUPPORTS([avx2])],
        [avx512f], [SID_X86_SIMD_SUPPORTS([avx512f])],
        [AC_MSG_ERROR([Unrecognized SIMD specified])]
    )]
)
AC_MSG_RESULT([$with_simd])

dnl gcc needs this flag to enable vectorization
AX_APPEND_COMPILE_FLAGS([-ftree-loop-vectorize])

dnl Enable branch prediction hints.
AC_ARG_ENABLE([branch-hints],
  [AS_HELP_STRING([--enable-branch-hints],
    [enable static branch prediction hints @<:@default=yes@:>@])],
  [],
  [enable_branch_hints=yes]
)

AS_IF([test "$enable_branch_hints" != no],
  [RESIDFP_BRANCH_HINTS=1],
  [RESIDFP_BRANCH_HINTS=0]
)

AC_CACHE_CHECK([for __builtin_expect], [resid_cv_builtin_expect],
  [AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main() { __builtin_expect(0, 0); }])],
    [resid_cv_builtin_expect=yes], [resid_cv_builtin_expect=no])]
)

AS_IF([test "$resid_cv_builtin_expect" = no],
  [HAVE_BUILTIN_EXPECT=0],
  [HAVE_BUILTIN_EXPECT=1]
)

AC_ARG_ENABLE([lto],
  [AS_HELP_STRING([--enable-lto],
    [enable link time optimization @<:@default=no@:>@])]
)

AS_IF([test "x$enable_lto" = "xyes"],
  [AX_APPEND_COMPILE_FLAGS([-flto])]
)


AC_CHECK_PROGS([DOXYGEN], [doxygen])
AS_IF([test -z "$DOXYGEN"],
  [AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])],
  [AC_CHECK_PROG([DOT], [dot], [YES], [NO])]
)

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])

AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([Doxyfile])])

AC_ARG_ENABLE([tests],
  [AS_HELP_STRING([--disable-tests],
    [disable unit tests @<:@default=no@:>@]
  )],
  [],
  [enable_tests=yes]
)

AM_CONDITIONAL([ENABLE_TEST], [test x$enable_tests != xno])

AC_SUBST(HAVE_BUILTIN_EXPECT)
AC_SUBST(RESIDFP_INLINING)
AC_SUBST(RESIDFP_INLINE)
AC_SUBST(RESIDFP_BRANCH_HINTS)

AC_SUBST(LIBRESIDFPVERSION)


AC_CONFIG_FILES([
Makefile
libresidfp.pc
src/siddefs-fp.h
src/residfp/sidversion.h
tests/Makefile
])

AC_OUTPUT
