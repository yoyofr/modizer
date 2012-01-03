# Checks if compiler supports hidden visibility
AC_DEFUN([CHECK_VISIBILITY],
[
  AC_MSG_CHECKING([whether the compiler supports hidden visibility])
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
      [[
      #if !defined(__GNUC__) || (__GNUC__ < 4) || defined(_WIN32)
      #  error
      #endif
      ]],
      [[]]
    )],
    [has_visibility=yes],
    [has_visibility=no]
  )
  AC_MSG_RESULT([$has_visibility])
  AM_CONDITIONAL([HAVE_VISIBILITY], [test "x$has_visibility" = xyes])
])
