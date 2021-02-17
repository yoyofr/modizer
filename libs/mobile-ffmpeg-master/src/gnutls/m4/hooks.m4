# Copyright (C) 2000-2012 Free Software Foundation, Inc.
#
# Author: Nikos Mavrogiannopoulos, Simon Josefsson
#
# This file is part of GnuTLS.
#
# The GnuTLS is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# as published by the Free Software Foundation; either version 2.1 of
# the License, or (at your option) any later version.
#
# The GnuTLS is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>

AC_DEFUN([LIBGNUTLS_EXTRA_HOOKS],
[
  AC_MSG_CHECKING([whether to build OpenSSL compatibility layer])
  AC_ARG_ENABLE(openssl-compatibility,
    AS_HELP_STRING([--enable-openssl-compatibility],
                   [enable the OpenSSL compatibility library]),
    enable_openssl=$enableval, enable_openssl=no)
  AC_MSG_RESULT($enable_openssl)
  AM_CONDITIONAL(ENABLE_OPENSSL, test "$enable_openssl" = "yes")

  # We link to ../lib's gnulib, which needs -lws2_32 via LIBSOCKET in Makefile.am.
  gl_SOCKETS
])

AC_DEFUN([LIBGNUTLS_HOOKS],
[
  # Library code modified:                              REVISION++
  # Interfaces changed/added/removed:   CURRENT++       REVISION=0
  # Interfaces added:                             AGE++
  #   + add new version symbol in libgnutls.map, see Symbol and library versioning
  #     in CONTRIBUTION.md for more info.
  #
  # Interfaces removed:                           AGE=0 (+bump all symbol versions in .map)
  AC_SUBST(LT_CURRENT, 57)
  AC_SUBST(LT_REVISION, 0)
  AC_SUBST(LT_AGE, 27)

  AC_SUBST(LT_SSL_CURRENT, 27)
  AC_SUBST(LT_SSL_REVISION, 2)
  AC_SUBST(LT_SSL_AGE, 0)

  AC_SUBST(LT_DANE_CURRENT, 4)
  AC_SUBST(LT_DANE_REVISION, 1)
  AC_SUBST(LT_DANE_AGE, 4)

  AC_SUBST(LT_XSSL_CURRENT, 0)
  AC_SUBST(LT_XSSL_REVISION, 0)
  AC_SUBST(LT_XSSL_AGE, 0)

  AC_SUBST(CXX_LT_CURRENT, 29)
  AC_SUBST(CXX_LT_REVISION, 0)
  AC_SUBST(CXX_LT_AGE, 1)

  AC_SUBST(CRYWRAP_PATCHLEVEL, 3)

  # Used when creating the Windows libgnutls-XX.def files.
  DLL_VERSION=`expr ${LT_CURRENT} - ${LT_AGE}`
  AC_SUBST(DLL_VERSION)
  DLL_SSL_VERSION=`expr ${LT_SSL_CURRENT} - ${LT_SSL_AGE}`
  AC_SUBST(DLL_SSL_VERSION)

NETTLE_MINIMUM=3.4.1
  PKG_CHECK_MODULES(NETTLE, [nettle >= $NETTLE_MINIMUM], [cryptolib="nettle"], [
AC_MSG_ERROR([[
  ***
  *** Libnettle $NETTLE_MINIMUM was not found.
]])
  ])
  PKG_CHECK_MODULES(HOGWEED, [hogweed >= $NETTLE_MINIMUM ], [], [
AC_MSG_ERROR([[
  ***
  *** Libhogweed (nettle's companion library) $NETTLE_MINIMUM was not found. Note that you must compile nettle with gmp support.
]])
  ])
  AM_CONDITIONAL(ENABLE_NETTLE, test "$cryptolib" = "nettle")
  AC_DEFINE([HAVE_LIBNETTLE], 1, [nettle is enabled])

  GNUTLS_REQUIRES_PRIVATE="Requires.private: nettle, hogweed"

  AC_ARG_WITH(nettle-mini,
    AS_HELP_STRING([--with-nettle-mini], [Link against a mini-nettle (that includes mini-gmp)]),
      mini_nettle=$withval,
      mini_nettle=no)

  AC_ARG_VAR(GMP_CFLAGS, [C compiler flags for gmp])
  AC_ARG_VAR(GMP_LIBS, [linker flags for gmp])
  if test "$mini_nettle" != no;then
    GMP_CFLAGS=""
    GMP_LIBS=""
  else
    if test x$GMP_LIBS = x; then
	AC_CHECK_LIB(gmp, __gmpz_cmp, [GMP_LIBS="-lgmp"], [AC_MSG_ERROR([[
***
*** gmp was not found.
]])])
    fi
  fi
  AC_SUBST(GMP_CFLAGS)
  AC_SUBST(GMP_LIBS)

LIBTASN1_MINIMUM=4.9
  AC_ARG_WITH(included-libtasn1,
    AS_HELP_STRING([--with-included-libtasn1], [use the included libtasn1]),
      included_libtasn1=$withval,
      included_libtasn1=no)
  if test "$included_libtasn1" = "no"; then
    PKG_CHECK_MODULES(LIBTASN1, [libtasn1 >= $LIBTASN1_MINIMUM], [], [included_libtasn1=yes])
    if test "$included_libtasn1" = yes; then
      AC_MSG_ERROR([[
  ***
  *** Libtasn1 $LIBTASN1_MINIMUM was not found. To use the included one, use --with-included-libtasn1
  ]])
    fi
  fi
  AC_MSG_CHECKING([whether to use the included minitasn1])
  AC_MSG_RESULT($included_libtasn1)
  AM_CONDITIONAL(ENABLE_MINITASN1, test "$included_libtasn1" = "yes")

  if test "$included_libtasn1" = "no"; then
    GNUTLS_REQUIRES_PRIVATE="${GNUTLS_REQUIRES_PRIVATE}, libtasn1"
  fi

  AC_MSG_CHECKING([whether C99 macros are supported])
  AC_TRY_COMPILE(,
  [
    #define test_mac(...)
    int z,y,x;
    test_mac(x,y,z);
    return 0;
  ], [
    AC_DEFINE([C99_MACROS], 1, [C99 macros are supported])
    AC_MSG_RESULT(yes)
  ], [
    AC_MSG_RESULT(no)
    AC_MSG_WARN([C99 macros not supported. This may affect compiling.])
  ])

  ac_strict_der_time=yes
  AC_MSG_CHECKING([whether to disable strict DER time encodings for backwards compatibility])
  AC_ARG_ENABLE(strict-der-time,
    AS_HELP_STRING([--disable-strict-der-time],
                   [allow non compliant DER time values]),
    ac_strict_der_time=$enableval)
  if test x$ac_strict_der_time != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([STRICT_DER_TIME], 1, [force strict DER time constraints])
  else
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(STRICT_DER_TIME, test "$ac_strict_der_time" != "no")

  ac_allow_sha1=no
  AC_MSG_CHECKING([whether to allow SHA1 as an acceptable hash for cert digital signatures])
  AC_ARG_ENABLE(sha1-support,
    AS_HELP_STRING([--enable-sha1-support],
                   [allow SHA1 as an acceptable hash for cert digital signatures]),
    ac_allow_sha1=$enableval)
  if test x$ac_allow_sha1 != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ALLOW_SHA1], 1, [allow SHA1 as an acceptable hash for digital signatures])
  else
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ALLOW_SHA1, test "$ac_allow_sha1" != "no")

  ac_enable_ssl3=no
  AC_MSG_CHECKING([whether to disable the SSL 3.0 protocol])
  AC_ARG_ENABLE(ssl3-support,
    AS_HELP_STRING([--enable-ssl3-support],
                   [enable support for the SSL 3.0 protocol]),
    ac_enable_ssl3=$enableval)
  if test x$ac_enable_ssl3 != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_SSL3], 1, [enable SSL3.0 support])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi

  AM_CONDITIONAL(ENABLE_SSL3, test "$ac_enable_ssl3" != "no")

  ac_enable_ssl2=yes
  AC_MSG_CHECKING([whether to disable the SSL 2.0 client hello])
  AC_ARG_ENABLE(ssl2-support,
    AS_HELP_STRING([--disable-ssl2-support],
                   [disable support for the SSL 2.0 client hello]),
    ac_enable_ssl2=$enableval)
  if test x$ac_enable_ssl2 != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_SSL2], 1, [enable SSL2.0 support for client hello])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ENABLE_SSL2, test "$ac_enable_ssl2" != "no")

  ac_enable_srtp=yes
  AC_MSG_CHECKING([whether to disable DTLS-SRTP extension])
  AC_ARG_ENABLE(dtls-srtp-support,
    AS_HELP_STRING([--disable-dtls-srtp-support],
                   [disable support for the DTLS-SRTP extension]),
    ac_enable_srtp=$enableval)
  if test x$ac_enable_srtp != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_DTLS_SRTP], 1, [enable DTLS-SRTP support])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ENABLE_DTLS_SRTP, test "$ac_enable_srtp" != "no")

  AC_MSG_CHECKING([whether to disable ALPN extension])
  AC_ARG_ENABLE(alpn-support,
    AS_HELP_STRING([--disable-alpn-support],
                   [disable support for the Application Layer Protocol Negotiation (ALPN) extension]),
    ac_enable_alpn=$enableval,ac_enable_alpn=yes)
  if test x$ac_enable_alpn != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_ALPN], 1, [enable ALPN support])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ENABLE_ALPN, test "$ac_enable_alpn" != "no")

  ac_enable_heartbeat=yes
  AC_MSG_CHECKING([whether to enable TLS heartbeat support])
  AC_ARG_ENABLE(heartbeat-support,
    AS_HELP_STRING([--disable-heartbeat-support],
                   [disable support for the heartbeat extension]),
    ac_enable_heartbeat=$enableval)
  if test x$ac_enable_heartbeat != xno; then
   AC_MSG_RESULT(yes)
   AC_DEFINE([ENABLE_HEARTBEAT], 1, [enable heartbeat support])
  else
   AC_MSG_RESULT(no)
  fi
  AM_CONDITIONAL(ENABLE_HEARTBEAT, test "$ac_enable_heartbeat" != "no")

  ac_enable_srp=yes
  AC_MSG_CHECKING([whether to disable SRP authentication support])
  AC_ARG_ENABLE(srp-authentication,
    AS_HELP_STRING([--disable-srp-authentication],
                   [disable the SRP authentication support]),
    ac_enable_srp=$enableval)
  if test x$ac_enable_srp != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_SRP], 1, [enable SRP authentication])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ENABLE_SRP, test "$ac_enable_srp" != "no")

  ac_enable_psk=yes
  AC_MSG_CHECKING([whether to disable PSK authentication support])
  AC_ARG_ENABLE(psk-authentication,
    AS_HELP_STRING([--disable-psk-authentication],
                   [disable the PSK authentication support]),
    ac_enable_psk=$enableval)
  if test x$ac_enable_psk != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_PSK], 1, [enable PSK authentication])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ENABLE_PSK, test "$ac_enable_psk" != "no")

  ac_enable_anon=yes
  AC_MSG_CHECKING([whether to disable anonymous authentication support])
  AC_ARG_ENABLE(anon-authentication,
    AS_HELP_STRING([--disable-anon-authentication],
                   [disable the anonymous authentication support]),
    ac_enable_anon=$enableval)
  if test x$ac_enable_anon != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_ANON], 1, [enable anonymous authentication])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ENABLE_ANON, test "$ac_enable_anon" != "no")

  AC_MSG_CHECKING([whether to disable DHE support])
  AC_ARG_ENABLE(dhe,
    AS_HELP_STRING([--disable-dhe],
                   [disable the DHE support]),
    ac_enable_dhe=$enableval, ac_enable_dhe=yes)
  if test x$ac_enable_dhe != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_DHE], 1, [enable DHE])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ENABLE_DHE, test "$ac_enable_dhe" != "no")

  AC_MSG_CHECKING([whether to disable ECDHE support])
  AC_ARG_ENABLE(ecdhe,
    AS_HELP_STRING([--disable-ecdhe],
                   [disable the ECDHE support]),
    ac_enable_ecdhe=$enableval, ac_enable_ecdhe=yes)
  if test x$ac_enable_ecdhe != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_ECDHE], 1, [enable DHE])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ENABLE_ECDHE, test "$ac_enable_ecdhe" != "no")

  AC_MSG_CHECKING([whether to disable GOST support])
  AC_ARG_ENABLE(gost,
    AS_HELP_STRING([--disable-gost],
                   [disable the GOST support]),
    ac_enable_gost=$enableval, ac_enable_gost=yes)
  if test x$ac_enable_gost != xno; then
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_GOST], 1, [enable GOST])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ENABLE_GOST, test "$ac_enable_gost" != "no")

  # For cryptodev
  AC_MSG_CHECKING([whether to add cryptodev support])
  AC_ARG_ENABLE(cryptodev,
    AS_HELP_STRING([--enable-cryptodev], [enable cryptodev support]),
  enable_cryptodev=$enableval,enable_cryptodev=no)
  AC_MSG_RESULT($enable_cryptodev)

  if test "$enable_cryptodev" = "yes"; then
    AC_DEFINE([ENABLE_CRYPTODEV], 1, [Enable cryptodev support])
  fi

  AC_MSG_CHECKING([whether to disable OCSP support])
  AC_ARG_ENABLE(ocsp,
    AS_HELP_STRING([--disable-ocsp],
                   [disable OCSP support]),
    ac_enable_ocsp=$enableval,ac_enable_ocsp=yes)
  if test x$ac_enable_ocsp != xno; then
   ac_enable_ocsp=yes
   AC_MSG_RESULT(no)
   AC_DEFINE([ENABLE_OCSP], 1, [enable OCSP support])
  else
   ac_full=0
   AC_MSG_RESULT(yes)
  fi
  AM_CONDITIONAL(ENABLE_OCSP, test "$ac_enable_ocsp" != "no")

  # For storing integers in pointers without warnings
  # https://developer.gnome.org/doc/API/2.0/glib/glib-Type-Conversion-Macros.html#desc
  AC_CHECK_SIZEOF(void *)
  AC_CHECK_SIZEOF(long long)
  AC_CHECK_SIZEOF(long)
  AC_CHECK_SIZEOF(int)
  if test x$ac_cv_sizeof_void_p = x$ac_cv_sizeof_long;then
      AC_DEFINE([GNUTLS_POINTER_TO_INT_CAST], [(long)],
                [Additional cast to bring void* to a type castable to int.])
  elif test x$ac_cv_sizeof_void_p = x$ac_cv_sizeof_long_long;then
      AC_DEFINE([GNUTLS_POINTER_TO_INT_CAST], [(long long)],
                [Additional cast to bring void* to a type castable to int.])
   else
      AC_DEFINE([GNUTLS_POINTER_TO_INT_CAST], [])
   fi

dnl this is called from somewhere else
dnl #AM_ICONV
dnl m4_ifdef([gl_ICONV_MODULE_INDICATOR],
dnl  [gl_ICONV_MODULE_INDICATOR([iconv])])
])
