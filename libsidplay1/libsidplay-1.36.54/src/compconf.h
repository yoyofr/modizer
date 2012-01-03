/* compconf.h (template) */

/* @FOO@ : Define or undefine value FOO as appropriate. */

#ifndef SIDPLAY1_COMPCONF_H
#define SIDPLAY1_COMPCONF_H

#include "libcfg.h"

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* SID_WORDS_BIGENDIAN, SID_WORDS_LITTLEENDIAN  */
#define SID_WORDS_LITTLEENDIAN

/* Define if your C++ compiler implements exception-handling.  */
#define SID_HAVE_EXCEPTIONS

/* Define if your compiler supports type ``bool''.
   If not, a user-defined signed integral type will be used.  */
#define SID_HAVE_BOOL

/* Define if you support file names longer than 14 characters.  */
#define SID_HAVE_LONG_FILE_NAMES

/* Define if you have the <unistd.h> header file.  */
#define SID_HAVE_UNISTD_H

/* Define if you have the <sys/stat.h> header file.  */
#define SID_HAVE_SYS_STAT_H

/* Define if you have the <sys/types.h> header file.  */
#define SID_HAVE_SYS_TYPES_H

/* Define if you have the strncasecmp function.  */
#define SID_HAVE_STRNCASECMP

/* Define if you have the strcasecmp function.  */
#define SID_HAVE_STRCASECMP

/* Define if you have the strnicmp function.  */
#undef SID_HAVE_STRNICMP

/* Define if you have the stricmp function.  */
#undef SID_HAVE_STRICMP

/* Define if standard header <sstream> is available. */
#define SID_HAVE_SSTREAM

/* Define if standard member ``ios::binary'' is called ``ios::bin''. */
#undef SID_HAVE_IOS_BIN

/* Define if standard member function ``fstream::is_open()'' is not available.  */
#undef SID_DONT_HAVE_IS_OPEN

/* Define whether istream member function ``seekg(streamoff,seekdir).offset()''
   should be used instead of standard ``seekg(streamoff,seekdir); tellg()''.  */
#undef SID_HAVE_SEEKG_OFFSET

#endif  /* SIDPLAY1_COMPCONF_H */
