/* ------------------------------------------------------------------------ */
/* LHa for UNIX    Archiver Driver                                          */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Soruce All chagned              1995.01.14  N.Watazaki      */
/*  Ver. 1.14i  Modified and bug fixed          2000.10.06  t.okamoto       */
/* ------------------------------------------------------------------------ */
/*
    Included...
        lharc.h     interface.h     slidehuf.h
*/

//#ifdef HAVE_CONFIG_H
#include "config.h"
//#endif

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <signal.h>

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcmp(s1, s2, n) bcmp ((s1), (s2), (n))
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#ifndef NULL
#define NULL ((char *)0)
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if STDC_HEADERS
# include <stdarg.h>
# define va_init(a,b) va_start(a,b)
#else
# include <varargs.h>
# define va_init(a,b) va_start(a)
#endif

#if HAVE_PWD_H
# include <pwd.h>
#endif
#if HAVE_GRP_H
# include <grp.h>
#endif

#if !HAVE_UID_T
typedef int uid_t;
#endif
#if !HAVE_GID_T
typedef int gid_t;
#endif

#if !HAVE_UINT64_T
# define HAVE_UINT64_T 1
# if SIZEOF_LONG == 8
    typedef unsigned long uint64_t;
# elif HAVE_LONG_LONG
    typedef unsigned long long uint64_t;
# else
#  undef HAVE_UINT64_T
# endif
#endif

#if !HAVE_SSIZE_T
typedef long ssize_t;
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_UTIME_H
#include <utime.h>
#else
struct utimbuf {
    time_t actime;
    time_t modtime;
};
int utime(const char *, struct utimbuf *);
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
# ifdef NONSYSTEM_DIR_LIBRARY           /* no use ?? */
#  include "lhdir.h"
# endif
#endif

#if HAVE_FNMATCH_H
# include <fnmatch.h>
#else
int fnmatch(const char *pattern, const char *string, int flags);
# define FNM_PATHNAME 1
# define FNM_NOESCAPE 2
# define FNM_PERIOD   4
#endif

#ifndef SEEK_SET
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2
#endif  /* SEEK_SET */

#if HAVE_LIMITS_H
#include <limits.h>
#else

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#ifndef UCHAR_MAX
#define UCHAR_MAX ((1<<(sizeof(unsigned char)*8))-1)
#endif

#ifndef USHRT_MAX
#define USHRT_MAX ((1<<(sizeof(unsigned short)*8))-1)
#endif

#ifndef SHRT_MAX
#define SHRT_MAX ((1<<(sizeof(short)*8-1))-1)
#endif

#ifndef SHRT_MIN
#define SHRT_MIN (SHRT_MAX-USHRT_MAX)
#endif

#ifndef ULONG_MAX
#define ULONG_MAX ((1<<(sizeof(unsigned long)*8))-1)
#endif

#ifndef LONG_MAX
#define LONG_MAX ((1<<(sizeof(long)*8-1))-1)
#endif

#ifndef LONG_MIN
#define LONG_MIN (LONG_MAX-ULONG_MAX)
#endif

#endif /* HAVE_LIMITS_H */

#if !HAVE_FSEEKO
# define fseeko  fseek
#endif
#if !HAVE_FTELLO
# define ftello  ftell
#endif

#include "lha_macro.h"

#define exit(n) lha_exit(n)

struct encode_option {
#if defined(__STDC__) || defined(AIX)
    void            (*output) ();
    void            (*encode_start) ();
    void            (*encode_end) ();
#else
    int             (*output) ();
    int             (*encode_start) ();
    int             (*encode_end) ();
#endif
};

struct decode_option {
    unsigned short  (*decode_c) ();
    unsigned short  (*decode_p) ();
#if defined(__STDC__) || defined(AIX)
    void            (*decode_start) ();
#else
    int             (*decode_start) ();
#endif
};

/* ------------------------------------------------------------------------ */
/*  LHa File Type Definition                                                */
/* ------------------------------------------------------------------------ */
typedef int boolean;            /* TRUE or FALSE */

struct string_pool {
    int             used;
    int             size;
    int             n;
    char           *buffer;
};

typedef struct LzHeader {
    size_t          header_size;
    int             size_field_length;
    char            method[METHOD_TYPE_STORAGE];
    size_t          packed_size;
    size_t          original_size;
    unsigned char   attribute;
    unsigned char   header_level;
    char            name[FILENAME_LENGTH];
    char            realname[FILENAME_LENGTH];/* real name for symbolic link */
    unsigned int    crc;      /* file CRC */
    boolean         has_crc;  /* file CRC */
    unsigned int    header_crc; /* header CRC */
    unsigned char   extend_type;
    unsigned char   minor_version;

    /* extend_type == EXTEND_UNIX  and convert from other type. */
    time_t          unix_last_modified_stamp;
    unsigned short  unix_mode;
    unsigned short  unix_uid;
    unsigned short  unix_gid;
    char            user[256];
    char            group[256];
}  LzHeader;

struct interfacing {
    FILE            *infile;
    FILE            *outfile;
    size_t          original;
    size_t          packed;
    size_t          read_size;
    int             dicbit;
    int             method;
};


/* ------------------------------------------------------------------------ */
/*  Option switch variable                                                  */
/* ------------------------------------------------------------------------ */
#ifdef LHA_MAIN_SRC
#define EXTERN
#else
#define EXTERN extern
#endif

/* command line options (common options) */
EXTERN boolean  quiet;
EXTERN boolean  text_mode;
EXTERN boolean  verbose;
EXTERN boolean  noexec;     /* debugging option */
EXTERN boolean  force;
EXTERN boolean  delete_after_append;
EXTERN int      compress_method;
EXTERN int      header_level;
/* EXTERN int       quiet_mode; */   /* 1996.8.13 t.okamoto */
#ifdef EUC
EXTERN boolean  euc_mode;
#endif

/* list command flags */
EXTERN boolean  verbose_listing;

/* extract/print command flags */
EXTERN boolean  output_to_stdout;

/* add/update/delete command flags */
EXTERN boolean  new_archive;
EXTERN boolean  update_if_newer;
EXTERN boolean  generic_format;

EXTERN boolean  remove_temporary_at_error;
EXTERN boolean  recover_archive_when_interrupt;
EXTERN boolean  remove_extracting_file_when_interrupt;
EXTERN boolean  get_filename_from_stdin;
EXTERN boolean  ignore_directory;
EXTERN char   **exclude_files;
EXTERN boolean  verify_mode;

/* Indicator flag */
EXTERN int      quiet_mode;

EXTERN boolean backup_old_archive;
EXTERN boolean  extract_broken_archive;

/* ------------------------------------------------------------------------ */
/*  Globale Variable                                                        */
/* ------------------------------------------------------------------------ */
EXTERN char     **cmd_filev;
EXTERN int      cmd_filec;

EXTERN char     *archive_name;
EXTERN char     temporary_name[FILENAME_LENGTH];
EXTERN char     backup_archive_name[FILENAME_LENGTH];

EXTERN char     *extract_directory;
EXTERN char     *reading_filename, *writing_filename;

EXTERN int      archive_file_mode;
EXTERN int      archive_file_gid;

EXTERN int      noconvertcase; /* 2000.10.6 */

/* slide.c */
EXTERN int      unpackable;
EXTERN size_t origsize, compsize;
EXTERN unsigned short dicbit;
EXTERN unsigned short maxmatch;
EXTERN size_t decode_count;
EXTERN unsigned long loc;           /* short -> long .. Changed N.Watazaki */
EXTERN unsigned char *text;

/* huf.c */
#ifndef LHA_MAIN_SRC  /* t.okamoto 96/2/20 */
EXTERN unsigned short left[], right[];
EXTERN unsigned char c_len[], pt_len[];
EXTERN unsigned short c_freq[], c_table[], c_code[];
EXTERN unsigned short p_freq[], pt_table[], pt_code[], t_freq[];
#endif

/* bitio.c */
EXTERN FILE     *infile, *outfile;
EXTERN unsigned short bitbuf;

/* crcio.c */
EXTERN unsigned int crctable[UCHAR_MAX + 1];
EXTERN int      dispflg;

/* from dhuf.c */
EXTERN unsigned int n_max;

/* lhadd.c */
EXTERN int temporary_fd;

/* ------------------------------------------------------------------------ */
/*  Functions                                                               */
/* ------------------------------------------------------------------------ */
#include "prototypes.h"
