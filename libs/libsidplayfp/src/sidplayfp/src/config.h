/* src/config.h.  Generated from config.h.in by configure.  */
/* src/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* define if the compiler supports basic C++11 syntax */
/* #undef HAVE_CXX11 */

/* define if the compiler supports basic C++14 syntax */
/* #undef HAVE_CXX14 */

/* define if the compiler supports basic C++17 syntax */
/* #undef HAVE_CXX17 */

/* define if the compiler supports basic C++20 syntax */
#define HAVE_CXX20 1

/* Define if you have the iconv() function and it works. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* exsid builder */
/* #undef HAVE_SIDPLAYFP_BUILDERS_EXSID_H */

/* hardsid builder */
/* #undef HAVE_SIDPLAYFP_BUILDERS_HARDSID_H */

/* residfp builder */
#define HAVE_SIDPLAYFP_BUILDERS_RESIDFP_H 1

/* resid builder */
#define HAVE_SIDPLAYFP_BUILDERS_RESID_H 1

/* sidlite builder */
/* #undef HAVE_SIDPLAYFP_BUILDERS_SIDLITE_H */

/* usbsid builder */
/* #undef HAVE_SIDPLAYFP_BUILDERS_USBSID_H */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST 

/* Name of package */
#define PACKAGE "sidplayfp"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "sidplayfp"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "sidplayfp 3.0.0a"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "sidplayfp"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://github.com/libsidplayfp/sidplayfp/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "3.0.0a"

/* Define to 1 if all of the C89 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "3.0.0a"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif
