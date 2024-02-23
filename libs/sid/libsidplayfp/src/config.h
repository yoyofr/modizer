/* src/config.h.  Generated from config.h.in by configure.  */
/* src/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define for threaded driver */
/* #undef EXSID_THREADED */

/* Define to 1 if you have the <arm_neon.h> header file. */
/* #undef HAVE_ARM_NEON_H */

/* define if the compiler supports basic C++11 syntax */
/* #undef HAVE_CXX11 */

/* define if the compiler supports basic C++14 syntax */
/* #undef HAVE_CXX14 */

/* define if the compiler supports basic C++17 syntax */
#define HAVE_CXX17 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <emmintrin.h> header file. */
/* #undef HAVE_EMMINTRIN_H */

/* Define to 1 if you have libexsid (-lexsid). */
/* #undef HAVE_EXSID */

/* Define to 1 if you have ftd2xx.h */
/* #undef HAVE_FTD2XX */

/* Define to 1 if you have the <ftd2xx.h> header file. */
/* #undef HAVE_FTD2XX_H */

/* Define to 1 if you have ftdi.h */
/* #undef HAVE_FTDI */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have libgcrypt (-lgcrypt). */
/* #undef HAVE_LIBGCRYPT */

/* Define to 1 if you have the <mmintrin.h> header file. */
/* #undef HAVE_MMINTRIN_H */

/* Define to 1 if you have pthread.h */
/* #undef HAVE_PTHREAD_H */

/* Have PTHREAD_PRIO_INHERIT. */
/* #undef HAVE_PTHREAD_PRIO_INHERIT */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "libsidplayfp"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "libsidplayfp"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libsidplayfp 2.6.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libsidplayfp"

/* Define to the home page for this package. */
#define PACKAGE_URL "https://github.com/libsidplayfp/libsidplayfp/"

/* Define to the version of this package. */
#define PACKAGE_VERSION "2.6.0"

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Shared library extension */
#define SHLIBEXT ".dylib"

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "2.6.0"

/* Path to VICE testsuite. */
/* #undef VICE_TESTSUITE */

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
