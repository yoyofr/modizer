dnl -------------------------------------------------------------------------
dnl Try to find a file (or one of more files in a list of dirs).
dnl -------------------------------------------------------------------------

AC_DEFUN(SID_FIND_FILE,
    [
    $3=NO
    for i in $2;
    do
        for j in $1;
        do
	        if test -r "$i/$j"; then
		        $3=$i
                break 2
            fi
        done
    done
    ]
)

dnl -------------------------------------------------------------------------

AC_DEFUN(SID_SUBST,
[
    eval "$1=$2"
    AC_SUBST($1)
])

AC_DEFUN(SID_SUBST_DEF,
[
    eval "$1=\"#define $1\""
    AC_SUBST($1)
])

AC_DEFUN(SID_SUBST_UNDEF,
[
    eval "$1=\"#undef $1\""
    AC_SUBST($1)
])

dnl -------------------------------------------------------------------------
dnl Check whether compiler has a working ``bool'' type.
dnl Will substitute @SID_HAVE_BOOL@ with either 1 (TRUE) or 0 (FALSE).
dnl -------------------------------------------------------------------------

AC_DEFUN(SID_CHECK_BOOL,
[
    AC_MSG_CHECKING([for bool])
    AC_CACHE_VAL(sid_cv_have_bool,
    [
        AC_TRY_COMPILE(
            [],
            [bool aBool = true;],
            [sid_cv_have_bool=yes],
            [sid_cv_have_bool=no]
        )
    ])
    AC_MSG_RESULT($sid_cv_have_bool)
    if test "$sid_cv_have_bool" = yes; then
        SID_SUBST_DEF(SID_HAVE_BOOL)
    else
        SID_SUBST_UNDEF(SID_HAVE_BOOL)
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ library has member ios::bin instead of ios::binary.
dnl Will substitute @SID_HAVE_IOS_BIN@ with either 1 (TRUE) or 0 (FALSE).
dnl -------------------------------------------------------------------------

AC_DEFUN(SID_CHECK_IOS_BIN,
[
    AC_MSG_CHECKING(whether standard member ios::binary is available)
    AC_CACHE_VAL(sid_cv_have_ios_binary,
    [
        AC_TRY_COMPILE(
            [
#include <iostream.h>
#include <fstream.h>
            ],
		    [
ifstream myTest("test",ios::in|ios::binary);
            ],
		    [sid_cv_have_ios_binary=yes],
		    [sid_cv_have_ios_binary=no]
	    )
    ])
    AC_MSG_RESULT($sid_cv_have_ios_binary)
    if test "$sid_cv_have_ios_binary" = no; then
        SID_SUBST_DEF(SID_HAVE_IOS_BIN)
    else
        SID_SUBST_UNDEF(SID_HAVE_IOS_BIN)
    fi
])

dnl -------------------------------------------------------------------------
dnl Check whether C++ compiler supports exception-handling
dnl and in particular the "nothrow allocator".
dnl Will substitute @SID_HAVE_EXCEPTIONS@ if test code compiles.
dnl -------------------------------------------------------------------------

AC_DEFUN(SID_CHECK_EXCEPTIONS,
[
    AC_MSG_CHECKING(whether exception-handling is supported)
    AC_CACHE_VAL(sid_cv_have_exceptions,
    [
        AC_TRY_COMPILE(
            [#include <new>],
		    [char* buf = new(std::nothrow) char[1024];],
		    [sid_cv_have_exceptions=yes],
		    [sid_cv_have_exceptions=no]
	    )
    ])
    AC_MSG_RESULT($sid_cv_have_exceptions)
    if test "$sid_cv_have_exceptions" = yes; then
        SID_SUBST_DEF(SID_HAVE_EXCEPTIONS)
    else
        SID_SUBST_UNDEF(SID_HAVE_EXCEPTIONS)
    fi
])

dnl -------------------------------------------------------------------------
dnl Pass C++ compiler options to libtool which supports C only.
dnl -------------------------------------------------------------------------

AC_DEFUN(SID_PROG_LIBTOOL,
[
    sid_save_cc=$CC
    sid_save_cflags=$CFLAGS
    CC=$CXX
    CFLAGS=$CXXFLAGS
    AM_PROG_LIBTOOL
    CC=$sid_save_cc
    CFLAGS=$sid_save_cflags
])

