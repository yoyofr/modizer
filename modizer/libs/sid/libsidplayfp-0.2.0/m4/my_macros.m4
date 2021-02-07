dnl -------------------------------------------------------------------------
dnl Check whether C++ environment provides the "nothrow allocator".
dnl Will substitute @HAVE_EXCEPTIONS@ if test code compiles.
dnl -------------------------------------------------------------------------

AC_DEFUN([MY_CHECK_EXCEPTIONS],
[
    AC_MSG_CHECKING([whether exceptions are available])
    AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[#include <new.h>]],
            [[char* buf = new(nothrow) char[1024];]])],
        [have_exceptions=yes],
        [have_exceptions=no]
    )
    AC_MSG_RESULT([$have_exceptions])
    if test "x$have_exceptions" = xyes; then
        AC_DEFINE([HAVE_EXCEPTIONS],,
            [Define if your C++ compiler implements exception-handling.]
        )
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ library has member ios::bin instead of ios::binary.
dnl Will substitute @HAVE_IOS_BIN@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN([MY_CHECK_IOS_BIN],
[
    AC_MSG_CHECKING([whether standard member ios::binary is available])
    AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[#include <fstream>]],
            [[std::ifstream myTest(std::ios::in|std::ios::binary);]])],
        [have_ios_binary=yes],
        [have_ios_binary=no]
    )
    AC_MSG_RESULT([$have_ios_binary])
    if test "x$have_ios_binary" = xyes; then
        AC_DEFINE([HAVE_IOS_BIN],,
            [Define if standard member ``ios::binary'' is called ``ios::bin''.]
        )
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ library has member ios::bin instead of ios::binary.
dnl Will substitute @HAVE_IOS_OPENMODE@ with either a def or undef line.
dnl -------------------------------------------------------------------------

AC_DEFUN([MY_CHECK_IOS_OPENMODE],
[
    AC_MSG_CHECKING([whether standard member ios::openmode is available])
    AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM([[#include <fstream>
             #include <iomanip>]],
            [[std::ios::openmode myTest = std::ios::in;]])],
        [have_ios_openmode=yes],
        [have_ios_openmode=no]
    )
    AC_MSG_RESULT([$have_ios_openmode])
    if test "x$have_ios_openmode" = xyes; then
        AC_DEFINE([HAVE_IOS_OPENMODE],,
            [Define if ``ios::openmode'' is supported.]
        )
    fi
])
