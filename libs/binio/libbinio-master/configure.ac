AC_INIT([binio],[1.5],[dn.tlp@gmx.net],[libbinio])
AC_CONFIG_SRCDIR(src/binio.cpp)
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile src/binio.h libbinio.pc])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE([enable])
#AC_DISABLE_SHARED([])
LT_INIT
AC_LANG(C++)

# Check for a sane C/C++ build environment.
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_CXX

# Check if 'long long' or 'long double' are available and use them instead.
AC_CHECK_TYPE(long long, AC_SUBST(TYPE_INT,"long long"),
		   AC_SUBST(TYPE_INT,long))
AC_CHECK_TYPE(long double, AC_SUBST(TYPE_FLOAT,"long double"),
		   AC_SUBST(TYPE_FLOAT,double))

# Conditionally build iostream wrappers
AC_ARG_ENABLE([iostream], AS_HELP_STRING([--disable-iostream],
	[Do not build iostream wrapper streams]))

if test ${enable_iostream:=yes} = yes; then
   AC_CHECK_LIB(stdc++,main, AC_SUBST(ENABLE_IOSTREAM,1),
      AC_SUBST(ENABLE_IOSTREAM,0))
else
	AC_SUBST(ENABLE_IOSTREAM,0)
fi

# Conditionally build std::string methods
AC_ARG_ENABLE([string], AS_HELP_STRING([--disable-string],
	[Do not build std::string methods]))

if test ${enable_string:=yes} = yes; then
   AC_CHECK_LIB(stdc++, main, AC_SUBST(ENABLE_STRING,1),
			      AC_SUBST(ENABLE_STRING,0))
else
	AC_SUBST(ENABLE_STRING,0)
fi

# Check if we are using the ISO standard C++ library
AC_ARG_WITH([iso],AS_HELP_STRING([--with-iso],
	[Force ISO C++ standard library compliance]))

if test x${with_iso} = xyes; then
   AC_SUBST(ISO_STDLIB,1)
elif test x${with_iso} = xno; then
   AC_SUBST(ISO_STDLIB,0)
else
	AC_CHECK_TYPE([std::basic_istream<char>],
		AC_SUBST(ISO_STDLIB,1),
		AC_SUBST(ISO_STDLIB,0),
		[#include <iostream>])
fi

# Conditionally build with 'math.h' (and maybe library) dependencies
AC_ARG_WITH([math],AS_HELP_STRING([--without-math],
	[Do not build routines depending on 'math.h']))

if test ${with_math:=yes} = yes; then
   AC_CHECK_FUNC(ldexp, AC_SUBST(WITH_MATH,1), AC_SUBST(WITH_MATH,0))
else
	AC_SUBST(WITH_MATH,0)
fi

AC_OUTPUT
