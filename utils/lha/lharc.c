/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              lharc.c -- append to archive                                */
/*                                                                          */
/*      Copyright (C) MCMLXXXIX Yooichi.Tagawa                              */
/*      Modified                Nobutaka Watazaki                           */
/*                          Thanks to H.Yoshizaki. (MS-DOS LHarc)           */
/*                                                                          */
/*  Ver. 0.00  Original                         1988.05.23  Y.Tagawa        */
/*  Ver. 0.01  Alpha Version (for 4.2BSD)       1989.05.28  Y.Tagawa        */
/*  Ver. 0.02  Alpha Version Rel.2              1989.05.29  Y.Tagawa        */
/*  Ver. 0.03  Release #3  Beta Version         1989.07.02  Y.Tagawa        */
/*  Ver. 0.03a Debug                            1989.07.03  Y.Tagawa        */
/*  Ver. 0.03b Modified                         1989.07.13  Y.Tagawa        */
/*  Ver. 0.03c Debug (Thanks to void@rena.dit.junet)                        */
/*                                              1989.08.09  Y.Tagawa        */
/*  Ver. 0.03d Modified (quiet and verbose)     1989.09.14  Y.Tagawa        */
/*  V1.00  Fixed                                1989.09.22  Y.Tagawa        */
/*  V1.01  Bug Fixed                            1989.12.25  Y.Tagawa        */
/*                                                                          */
/*  DOS-Version Original LHx V C2.01        (C) H.Yohizaki                  */
/*                                                                          */
/*  V2.00  UNIX Lharc + DOS LHx -> OSK LHx      1990.11.01  Momozou         */
/*  V2.01  Minor Modified                       1990.11.24  Momozou         */
/*                                                                          */
/*  Ver. 0.02  LHx for UNIX                     1991.11.18  M.Oki           */
/*  Ver. 0.03  LHa for UNIX                     1991.12.17  M.Oki           */
/*  Ver. 0.04  LHa for UNIX beta version        1992.01.20  M.Oki           */
/*  Ver. 1.00  LHa for UNIX Fixed               1992.03.19  M.Oki           */
/*                                                                          */
/*  Ver. 1.10  for Symbolic Link                1993.06.25  N.Watazaki      */
/*  Ver. 1.11  for Symbolic Link Bug Fixed      1993.08.18  N.Watazaki      */
/*  Ver. 1.12  for File Date Check              1993.10.28  N.Watazaki      */
/*  Ver. 1.13  Bug Fixed (Idicator calcurate)   1994.02.21  N.Watazaki      */
/*  Ver. 1.13a Bug Fixed (Sym. Link delete)     1994.03.11  N.Watazaki      */
/*  Ver. 1.13b Bug Fixed (Sym. Link delete)     1994.07.29  N.Watazaki      */
/*  Ver. 1.14  Source All chagned               1995.01.14  N.Watazaki      */
/*  Ver. 1.14b,c  Bug Fixed                     1996.03.07  t.okamoto       */
/*  Ver. 1.14d Version up                       1997.01.12  t.okamoto       */
/*  Ver. 1.14g Bug Fixed                        2000.05.06  t.okamoto       */
/*  Ver. 1.14i Modified                         2000.10.06  t.okamoto       */
/* ------------------------------------------------------------------------ */
#define LHA_MAIN_SRC

#include "lha.h"

static int      cmd = CMD_UNKNOWN;
static int error_occurred;

/* static functions */
static void     sort_files();
static void     print_version();

extern int optional_archive_kanji_code;
extern int optional_system_kanji_code;

/* ------------------------------------------------------------------------ */
static void
init_variable()     /* Added N.Watazaki */
{
/* options */
    quiet           = FALSE;
    text_mode       = FALSE;
    verbose         = 0;
    noexec          = FALSE;    /* debugging option */
    force           = FALSE;

    compress_method = DEFAULT_LZHUFF_METHOD; /* defined in config.h */

    header_level    = 2;        /* level 2 */
    quiet_mode      = 0;

#ifdef EUC
    euc_mode        = FALSE;
#endif

/* view command flags */
    verbose_listing = FALSE;

/* extract command flags */
    output_to_stdout = FALSE;

/* append command flags */
    new_archive         = FALSE;
    update_if_newer     = FALSE;
    delete_after_append = FALSE;
    generic_format      = FALSE;

    recover_archive_when_interrupt          = FALSE;
    remove_extracting_file_when_interrupt   = FALSE;
    get_filename_from_stdin                 = FALSE;
    ignore_directory                        = FALSE;
    exclude_files                           = NULL;
    verify_mode                             = FALSE;

    noconvertcase                           = FALSE;

    extract_directory = NULL;
    temporary_fd = -1;

#if BACKUP_OLD_ARCHIVE
    backup_old_archive = TRUE;
#else
    backup_old_archive = FALSE;
#endif

    extract_broken_archive = FALSE;
}

/* ------------------------------------------------------------------------ */
/* NOTES :          Text File Format                                        */
/* GENERATOR        NewLine                                                 */
/* [generic]        0D 0A                                                   */
/* [MS-DOS]         0D 0A                                                   */
/* [OS9][MacOS]     0D                                                      */
/* [UNIX]           0A                                                      */
/* ------------------------------------------------------------------------ */
static void
print_tiny_usage()
{
    fprintf(stderr, "\
usage: lha [-]<commands>[<options>] [-<options> ...] archive_file [file...]\n\
  commands:  [axelvudmcpt]\n\
  options:   [q[012]vnfto[567]dizg012e[w=<dir>|x=<pattern>]]\n\
  long options: --system-kanji-code={euc,sjis,utf8,cap}\n\
                --archive-kanji-code={euc,sjis,utf8,cap}\n\
                --extract-broken-archive\n\
                --help\n\
                --version\n");
}

static void
print_usage()
{
    fprintf(stderr, "\
LHarc    for UNIX  V 1.02  Copyright(C) 1989  Y.Tagawa\n\
LHx      for MSDOS V C2.01 Copyright(C) 1990  H.Yoshizaki\n\
LHx(arc) for OSK   V 2.01  Modified     1990  Momozou\n\
LHa      for UNIX  V 1.00  Copyright(C) 1992  Masaru Oki\n\
LHa      for UNIX  V 1.14  Modified     1995  Nobutaka Watazaki\n\
LHa      for UNIX  V 1.14i Modified     2000  Tsugio Okamoto\n\
                   Autoconfiscated 2001-2005  Koji Arai\n\
");

    print_tiny_usage();

    fprintf(stderr, "\
commands:                           options:\n\
 a   Add(or replace) to archive      q{num} quiet (num:quiet mode)\n\
 x,e EXtract from archive            v  verbose\n\
 l,v List / Verbose List             n  not execute\n\
 u   Update newer files to archive   f  force (over write at extract)\n\
 d   Delete from archive             t  FILES are TEXT file\n");
#ifdef SUPPORT_LH7
    fprintf(stderr, "\
 m   Move to archive (means 'ad')    o[567] compression method (a/u/c)\n\
");
#endif
#ifndef SUPPORT_LH7
    fprintf(stderr, "\
 m   Move to archive (means 'ad')    o  use LHarc compatible method (a/u/c)\n\
");
#endif
    fprintf(stderr, "\
 c   re-Construct new archive        d  delete FILES after (a/u/c)\n\
 p   Print to STDOUT from archive    i  ignore directory path (x/e)\n\
 t   Test file CRC in archive        z  files not compress (a/u/c)\n\
                                     g  Generic format (for compatibility)\n\
                                        or not convert case when extracting\n\
                                     0/1/2 header level (a/u/c)\n\
");
#ifdef EUC
    fprintf(stderr, "\
                                     e  TEXT code convert from/to EUC\n\
");
#endif
    fprintf(stderr, "\
                                     w=<dir> specify extract directory (x/e)\n\
                                     x=<pattern>  eXclude files (a/u/c)\n\
");
#if IGNORE_DOT_FILES            /* experimental feature */
    fprintf(stderr, "\
                                     X ignore dot files (a/u/c)\n\
");
#endif
}

#include "getopt_long.h"

/*
  Parse LHA options
*/
static int
parse_suboption(int argc, char **argv)
{
    char *short_options = "q[012]vnfto[567]dizg012ew:x:";
    /* "[...]" means optional 1 byte argument (original extention) */
    enum {
        HELP_OPTION = 256,
        VERSION_OPTION,
        SYSTEM_KANJI_CODE_OPTION,
        ARCHIVE_KANJI_CODE_OPTION,
    };

    struct option long_options[] = {
        /* These options set a flag. */
        {"help",    no_argument,       0, HELP_OPTION},
        {"version", no_argument,       0, VERSION_OPTION},

        {"system-kanji-code", required_argument, 0, SYSTEM_KANJI_CODE_OPTION},
        {"archive-kanji-code", required_argument, 0, ARCHIVE_KANJI_CODE_OPTION},
        {"extract-broken-archive", no_argument, &extract_broken_archive, 1},
        {0, 0, 0, 0}
    };
    int i;

    /* parse option */
    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv,
                            short_options, long_options,
                            &option_index);

        if (c == -1) break;     /* end of options */

        switch (c) {
        case 0:
            /* Already set a flag variable by the definition of the
               long_options. */
            break;
        case '?':
            /* Invalid option */
            print_tiny_usage();
            exit(2);
        case HELP_OPTION:
            print_usage();
            exit(0);
        case VERSION_OPTION:
            print_version();
            exit(0);
        case 'q':
            if (!optarg) {
                /* In quiet mode, no confirm to overwrite */
                force = TRUE;
                quiet = TRUE;
                break;
            }

            switch (*optarg) {
            case '0':           /* no quiet */
            case '1':           /* no use the incremental indicator */
                quiet_mode = *optarg - '0';
                break;
            case '2':           /* no output */
                /* fall through */
            default:
                force = TRUE;
                quiet = TRUE;
                break;
            }
            break;
        case 'f':
            force = TRUE;
            break;
        case 'v':
            verbose++;
            break;
        case 't':
            text_mode = TRUE;
            break;
#ifdef EUC
        case 'e':
            text_mode = TRUE;
            euc_mode = TRUE;
            break;
#endif
        case 'n':
            noexec = TRUE;
            break;
        case 'g':
            generic_format = TRUE;
            noconvertcase = TRUE;
            header_level = 0;
            break;
        case 'd':
            delete_after_append = TRUE;
            break;
        case 'o':
            if (!optarg) {
                compress_method = LZHUFF1_METHOD_NUM;
                header_level = 0;
                break;
            }
            switch (*optarg) {
            case '5':
                compress_method = LZHUFF5_METHOD_NUM;
                break;
#ifdef SUPPORT_LH7
            case '6':
                compress_method = LZHUFF6_METHOD_NUM;
                break;
            case '7':
                compress_method = LZHUFF7_METHOD_NUM;
                break;
#endif
            default:
                error("invalid compression method 'o%c'", *optarg);
                return -1;
            }
            break;
        case 'z':
            compress_method = LZHUFF0_METHOD_NUM;   /* Changed N.Watazaki */
            break;
        case 'i':
            ignore_directory = TRUE;
            break;
        case 'x':
            if (!optarg) {
                error("exclude files does not specified for `-x'");
                exit(2);
            }

            for (i = 0; exclude_files && exclude_files[i]; i++)
                ;
            exclude_files = (char**)xrealloc(exclude_files,
                                             sizeof(char*) * (i+2));

            if (*optarg == '=')
                optarg++;
            exclude_files[i] = optarg;
            exclude_files[i+1] = 0;

            break;
#if IGNORE_DOT_FILES            /* experimental feature */
        case 'X':
            for (i = 0; exclude_files && exclude_files[i]; i++)
                ;
            exclude_files = (char**)xrealloc(exclude_files,
                                             sizeof(char*) * (i+2));

            exclude_files[i] = xstrdup(".*");
            exclude_files[i+1] = 0;
            break;
#endif
        case 'w':
            if (!optarg) {
                error("working directory does not specified for `-w'");
                exit(2);
            }
            if (*optarg == '=')
                optarg++;

            extract_directory = optarg;
            break;
        case '0':
            header_level = 0;
            break;
        case '1':
            header_level = 1;
            break;
        case '2':
            header_level = 2;
            break;
        case SYSTEM_KANJI_CODE_OPTION:
            if (!optarg) {
                error("kanji code not specified for --%s",
                      long_options[option_index].name);
                return -1;
            }
            if (strcmp(optarg, "euc") == 0) {
                optional_system_kanji_code = CODE_EUC;
            }
            else if (strcmp(optarg, "sjis") == 0) {
                optional_system_kanji_code = CODE_SJIS;
            }
            else if (strcmp(optarg, "utf8") == 0) {
                optional_system_kanji_code = CODE_UTF8;
            }
            else if (strcmp(optarg, "cap") == 0) {
                optional_system_kanji_code = CODE_CAP;
            }
            else {
                error("unknown kanji code \"%s\"", optarg);
                return -1;
            }
            break;

        case ARCHIVE_KANJI_CODE_OPTION:
            if (!optarg) {
                error("kanji code not specified for --%s",
                      long_options[option_index].name);
                return -1;
            }
            if (strcmp(optarg, "euc") == 0) {
                optional_archive_kanji_code = CODE_EUC;
            }
            else if (strcmp(optarg, "sjis") == 0) {
                optional_archive_kanji_code = CODE_SJIS;
            }
            else if (strcmp(optarg, "utf8") == 0) {
                optional_archive_kanji_code = CODE_UTF8;
            }
            else if (strcmp(optarg, "cap") == 0) {
                optional_archive_kanji_code = CODE_CAP;
            }
            else {
                error("unknown kanji code \"%s\"", optarg);
                return -1;
            }
            break;

        default:
            error("unknown option `-%c'.", c);
            return -1;
        }
    }

    argc -= optind;
    argv += optind;

    if (!archive_name) {
        archive_name = *argv++;
        argc--;
    }

    cmd_filec = argc;
    cmd_filev = argv;

    return 0;
}

/*
  Parse LHA command and options.
*/
static int
parse_option(int argc, char **argv)
{
    char *cmd_char;

    if (argv[1] == NULL || strcmp(argv[1], "--help") == 0) {
        print_usage();
        exit(0);
    }

    if (strcmp(argv[1], "--version") == 0) {
        print_version();
        exit(0);
    }

    if (argc == 2 && *argv[1] != '-') {
        archive_name = argv[1];
        cmd = CMD_LIST;
        cmd_filec = 0;
        cmd_filev = 0;
        return 0;
    }

    cmd_char = argv[1];

    if (cmd_char[0] == '-')
        cmd_char++;

    /* parse commands */
    switch (*cmd_char) {
    case 'x':
    case 'e':
        cmd = CMD_EXTRACT;
        break;

    case 'p':
        output_to_stdout = TRUE;
        cmd = CMD_EXTRACT;
        break;

    case 'c':
        new_archive = TRUE;
        cmd = CMD_ADD;
        break;

    case 'a':
        cmd = CMD_ADD;
        break;

    case 'd':
        cmd = CMD_DELETE;
        break;

    case 'u':
        update_if_newer = TRUE;
        cmd = CMD_ADD;
        break;

    case 'm':
        delete_after_append = TRUE;
        cmd = CMD_ADD;
        break;

    case 'v':
        verbose_listing = TRUE;
        cmd = CMD_LIST;
        break;

    case 'l':
        cmd = CMD_LIST;
        break;

    case 't':
        cmd = CMD_EXTRACT;
        verify_mode = TRUE;
        break;

    default:
        error("unknown command `-%c'", *cmd_char);
        return -1;
    }

    if (cmd_char[1] == '\0') {
        /* argv[1] is command name */
        argv[1] = argv[0];
        argv++;        
        argc--;
        /*for (int i=1;i<argc-1;i++) {
            argv[i]=argv[i+1];
        }
        argc--;*/
    }
    else {
        /* Eliminate command character
           e.g.) lha  cv foo.lzh -> lha -v foo.lzh
                 lha -cv foo.lzh -> lha -v foo.lzh
        */
        cmd_char[0] = '-';
        argv[1] = cmd_char;
    }

    return parse_suboption(argc, argv);
}

/* ------------------------------------------------------------------------ */
int lha_main(argc, argv)
    int             argc;
    char           *argv[];
{
    char           *p;

    int i;
	archive_name=NULL;
	extract_directory=NULL;
	optind=0;

    init_variable();        /* Added N.Watazaki */

    if (parse_option(argc, argv) == -1) {
        fputs("\n", stderr);
        print_tiny_usage();
        exit(2);
    }

    if (!archive_name) {
        error("archive file does not specified");
        fputs("\n", stderr);
        print_tiny_usage();
        exit(2);
    }

    if (!strcmp(archive_name, "-")) {
        if (!isatty(1) && cmd == CMD_ADD)
            quiet = TRUE;
    }
#if 0 /* Comment out; IMHO, this feature is useless. by Koji Arai */
    else {
        if (argc == 3 && !isatty(0)) { /* 1999.7.18 */
            /* Bug(?) on MinGW, isatty() return 0 on Cygwin console.
               mingw-runtime-1.3-2 and Cygwin 1.3.10(0.51/3/2) on Win2000 */
            get_filename_from_stdin = TRUE;
        }
    }
#endif

    /* target file name */
    if (get_filename_from_stdin) {
        char inpbuf[4096];
        char **xfilev;
        int xfilec = 257;

        cmd_filec = 0;
        xfilev = (char **)xmalloc(sizeof(char *) * xfilec);
        while (fgets(inpbuf, sizeof(inpbuf), stdin)) {
            /* delete \n if it exist */
            i=0; p=inpbuf;
            while (i < sizeof(inpbuf) && p != 0) {
                if (*p == '\n') {
                    *p = 0;
                    break;
                }
                p++; i++;
            }

            if (cmd_filec >= xfilec) {
                xfilec += 256;
                xfilev = (char **) xrealloc(xfilev,
                           sizeof(char *) * xfilec);
            }
            if (strlen(inpbuf) < 1)
                continue;
            xfilev[cmd_filec++] = xstrdup(inpbuf);
        }
        xfilev[cmd_filec] = NULL;
        cmd_filev = xfilev;
    }

    sort_files();

    /* make crc table */
    make_crctable();

    switch (cmd) {
    case CMD_EXTRACT:
        cmd_extract();
        break;
    case CMD_ADD:
        cmd_add();
        break;
    case CMD_LIST:
        cmd_list();
        break;
    case CMD_DELETE:
        cmd_delete();
        break;
    }

    if (error_occurred)
        return 1;
    return 0;
}


/* ------------------------------------------------------------------------ */
static void
print_version()
{
    /* macro PACKAGE_NAME, PACKAGE_VERSION and PLATFORM are
       defined in config.h by configure script */
    fprintf(stderr, "%s version %s (%s)\n",
            PACKAGE_NAME, PACKAGE_VERSION, PLATFORM);
}

void
#if STDC_HEADERS
message(char *fmt, ...)
#else
message(fmt, va_alist)
    char *fmt;
    va_dcl
#endif
{
    int errno_sv = errno;
    va_list v;

    fprintf(stderr, "LHa: ");

    va_init(v, fmt);
    vfprintf(stderr, fmt, v);
    va_end(v);

    fputs("\n", stderr);

    errno =  errno_sv;
}

/* ------------------------------------------------------------------------ */
void
#if STDC_HEADERS
warning(char *fmt, ...)
#else
warning(fmt, va_alist)
    char *fmt;
    va_dcl
#endif
{
    int errno_sv = errno;
    va_list v;

    fprintf(stderr, "LHa: Warning: ");

    va_init(v, fmt);
    vfprintf(stderr, fmt, v);
    va_end(v);

    fputs("\n", stderr);

    errno =  errno_sv;
}

/* ------------------------------------------------------------------------ */
void
#if STDC_HEADERS
error(char *fmt, ...)
#else
error(fmt, va_alist)
    char *fmt;
    va_dcl
#endif
{
    int errno_sv = errno;
    va_list v;

    fprintf(stderr, "LHa: Error: ");

    va_init(v, fmt);
    vfprintf(stderr, fmt, v);
    va_end(v);

    fputs("\n", stderr);

    error_occurred = 1;

    errno =  errno_sv;
}

void
#if STDC_HEADERS
fatal_error(char *fmt, ...)
#else
fatal_error(fmt, va_alist)
    char *fmt;
    va_dcl
#endif
{
    int errno_sv = errno;
    va_list v;

    fprintf(stderr, "LHa: Fatal error: ");

    va_init(v, fmt);
    vfprintf(stderr, fmt, v);
    va_end(v);

    if (errno)
        fprintf(stderr, ": %s\n", strerror(errno_sv));
    else
        fputs("\n", stderr);

    exit(1);
}

void
cleanup()
{
    if (temporary_fd != -1) {
        close(temporary_fd);
        temporary_fd = -1;
        unlink(temporary_name);
    }

    if (recover_archive_when_interrupt) {
        rename(backup_archive_name, archive_name);
        recover_archive_when_interrupt = FALSE;
    }
    if (remove_extracting_file_when_interrupt) {
        message("Removing: %s", writing_filename);
        unlink(writing_filename);
        remove_extracting_file_when_interrupt = FALSE;
    }
}

RETSIGTYPE
interrupt(signo)
    int signo;
{
    message("Interrupted");

    cleanup();

    signal(SIGINT, SIG_DFL);
#ifdef SIGHUP
    signal(SIGHUP, SIG_DFL);
#endif
    kill(getpid(), signo);
}

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* ------------------------------------------------------------------------ */
static int
sort_by_ascii(a, b)
    char          **a, **b;
{
    register char  *p, *q;
    register int    c1, c2;

    p = *a, q = *b;
    if (generic_format) {
        do {
            c1 = *(unsigned char *) p++;
            c2 = *(unsigned char *) q++;
            if (!c1 || !c2)
                break;
            if (islower(c1))
                c1 = toupper(c1);
            if (islower(c2))
                c2 = toupper(c2);
        }
        while (c1 == c2);
        return c1 - c2;
    }
    else {
        while (*p == *q && *p != '\0')
            p++, q++;
        return *(unsigned char *) p - *(unsigned char *) q;
    }
}

/* ------------------------------------------------------------------------ */
static void
sort_files()
{
    if (cmd_filec > 1)
        qsort(cmd_filev, cmd_filec, sizeof(char *), sort_by_ascii);
}

/* ------------------------------------------------------------------------ */
/*void *
xmalloc(size)
    size_t size;
{
    void *p = malloc(size);
    if (!p)
        fatal_error("Not enough memory");
    return p;
}*/

/* ------------------------------------------------------------------------ */
void *
xrealloc(old, size)
    void *old;
    size_t size;
{
    void *p = (char *) realloc(old, size);
    if (!p)
        fatal_error("Not enough memory");
    return p;
}

char *
xstrdup(str)
    char *str;
{
    int len = strlen(str);
    char *p = (char *)xmalloc(len + 1);
    strcpy(p, str);             /* ok */
    return p;
}

/* ------------------------------------------------------------------------ */
/*                              STRING POOL                                 */
/* ------------------------------------------------------------------------ */
/*
  string pool :
    +-------------+-------------+------+-------------+----------+
    | N A M E 1 \0| N A M E 2 \0| .... | N A M E n \0|          |
    +-------------+-------------+------+-------------+----------+
      ^ ^        ^ buffer+0 buffer+used buffer+size

  vector :
    +---------------+---------------+------------- -----------------+
    | pointer to    | pointer to    | pointer to   ...  pointer to  |
    |  stringpool   |  N A M E 1    |  N A M E 2   ...   N A M E n  |
    +---------------+---------------+-------------     -------------+
    ^ malloc base      returned
*/

/* ------------------------------------------------------------------------ */
void
init_sp(sp)
    struct string_pool *sp;
{
    sp->size = 1024 - 8;    /* any ( >=0 ) */
    sp->used = 0;
    sp->n = 0;
    sp->buffer = (char *) xmalloc(sp->size * sizeof(char));
}

/* ------------------------------------------------------------------------ */
void
add_sp(sp, name, len)
    struct string_pool *sp;
    char           *name;   /* stored '\0' at tail */
    int             len;    /* include '\0' */
{
    while (sp->used + len > sp->size) {
        sp->size *= 2;
        sp->buffer = (char *) xrealloc(sp->buffer, sp->size * sizeof(char));
    }
    memmove(sp->buffer + sp->used, name, len);
    sp->used += len;
    sp->n++;
}

/* ------------------------------------------------------------------------ */
void
finish_sp(sp, v_count, v_vector)
    register struct string_pool *sp;
    int            *v_count;
    char         ***v_vector;
{
    int             i;
    register char  *p;
    char          **v;

    v = (char **) xmalloc((sp->n + 1) * sizeof(char *));
    *v++ = sp->buffer;
    *v_vector = v;
    *v_count = sp->n;
    p = sp->buffer;
    for (i = sp->n; i; i--) {
        *v++ = p;
        if (i - 1)
            p += strlen(p) + 1;
    }
}

/* ------------------------------------------------------------------------ */
void
free_sp(vector)
    char          **vector;
{
    vector--;
    free(*vector);      /* free string pool */
    free(vector);
}


/* ------------------------------------------------------------------------ */
/*                          READ DIRECTORY FILES                            */
/* ------------------------------------------------------------------------ */
static          boolean
include_path_p(path, name)
    char           *path, *name;
{
    char           *n = name;
    while (*path)
        if (*path++ != *n++)
            return (path[-1] == '/' && *n == '\0');
    return (*n == '/' || (n != name && path[-1] == '/' && n[-1] == '/'));
}

/* ------------------------------------------------------------------------ */
void
cleaning_files(v_filec, v_filev)
    int            *v_filec;
    char         ***v_filev;
{
    char           *flags;
    struct stat     stbuf;

    register char **filev = *v_filev;
    register int    filec = *v_filec;
    register char  *p;
    register int    i, j;

    if (filec == 0)
        return;

    flags = xmalloc(filec * sizeof(char));

    /* flags & 0x01 :   1: ignore */
    /* flags & 0x02 :   1: directory, 0 : regular file */
    /* flags & 0x04 :   1: need delete */

    for (i = 0; i < filec; i++)
        if (GETSTAT(filev[i], &stbuf) < 0) {
            flags[i] = 0x04;
            warning("Cannot access \"%s\" : %s; ignored.", filev[i],
                    strerror(errno));
        }
        else {
            if (is_regularfile(&stbuf))
                flags[i] = 0x00;
            else if (is_directory(&stbuf))
                flags[i] = 0x02;
#ifdef S_IFLNK
            else if (is_symlink(&stbuf)) /* t.okamoto */
                flags[i] = 0x00;
#endif
            else {
                flags[i] = 0x04;
                warning("Cannot archive \"%s\", ignored.", filev[i]);
            }
        }

    for (i = 0; i < filec; i++) {
        p = filev[i];
        if ((flags[i] & 0x07) == 0x00) {    /* regular file, not
                             * deleted/ignored */
            for (j = i + 1; j < filec; j++) {
                if ((flags[j] & 0x07) == 0x00) {    /* regular file, not
                                     * deleted/ignored */
                    if (STREQU(p, filev[j]))
                        flags[j] = 0x04;    /* delete */
                }
            }
        }
        else if ((flags[i] & 0x07) == 0x02) {   /* directory, not
                             * deleted/ignored */
            for (j = i + 1; j < filec; j++) {
                if ((flags[j] & 0x07) == 0x00) {    /* regular file, not
                                     * deleted/ignored */
                    if (include_path_p(p, filev[j]))
                        flags[j] = 0x04;    /* delete */
                }
                else if ((flags[j] & 0x07) == 0x02) {   /* directory, not
                                     * deleted/ignored */
                    if (include_path_p(p, filev[j]))
                        flags[j] = 0x04;    /* delete */
                }
            }
        }
    }

    for (i = j = 0; i < filec; i++) {
        if ((flags[i] & 0x04) == 0) {
            if (i != j)
                filev[j] = filev[i];
            j++;
        }
    }
    *v_filec = j;

    free(flags);
}

/* ------------------------------------------------------------------------ */
boolean
find_files(name, v_filec, v_filev)
    char           *name;
    int            *v_filec;
    char         ***v_filev;
{
    struct string_pool sp;
    char            newname[FILENAME_LENGTH];
    int             len, n, i;
    DIR            *dirp;
    struct dirent  *dp;
    struct stat     tmp_stbuf, arc_stbuf, fil_stbuf;
    int exist_tmp = 1, exist_arc = 1;

    len = str_safe_copy(newname, name, sizeof(newname));
    if (len > 0 && newname[len - 1] != '/') {
        if (len < sizeof(newname)-1)
            strcpy(&newname[len++], "/"); /* ok */
        else
            warning("the length of pathname \"%s\" is too long.", name);
    }

    dirp = opendir(name);
    if (!dirp)
        return FALSE;

    init_sp(&sp);

    if (GETSTAT(temporary_name, &tmp_stbuf) == -1)
        exist_tmp = 0;
    if (GETSTAT(archive_name, &arc_stbuf) == -1)
        exist_arc = 0;

    while ((dp = readdir(dirp)) != NULL) {
        n = NAMLEN(dp);

        /* exclude '.' and '..' */
        if (strncmp(dp->d_name, ".", n) == 0
            || strncmp(dp->d_name, "..", n) == 0)
            continue;

        /* exclude exclude_files supplied by user */
        for (i = 0; exclude_files && exclude_files[i]; i++) {
            if (fnmatch(exclude_files[i], dp->d_name,
                        FNM_PATHNAME|FNM_NOESCAPE|FNM_PERIOD) == 0)
                goto next;
        }

        if (len + n >= sizeof(newname)) {
            warning("filename is too long");
            continue;
        }

        strncpy(newname + len, dp->d_name, n);
        newname[len + n] = '\0';
        if (GETSTAT(newname, &fil_stbuf) < 0)
            continue;

#if defined(HAVE_STRUCT_STAT_ST_INO) && !__MINGW32__
        /* MinGW has meaningless st_ino */

        /* exclude temporary file, archive file and these links */
        if (exist_tmp &&
            tmp_stbuf.st_dev == fil_stbuf.st_dev &&
            tmp_stbuf.st_ino == fil_stbuf.st_ino)
            continue;

        if (exist_arc &&
            arc_stbuf.st_dev == fil_stbuf.st_dev &&
            arc_stbuf.st_ino == fil_stbuf.st_ino)
            continue;
#endif
        add_sp(&sp, newname, len+n+1);
    next:
        ;
    }
    closedir(dirp);
    finish_sp(&sp, v_filec, v_filev);
    if (*v_filec > 1)
        qsort(*v_filev, *v_filec, sizeof(char *), sort_by_ascii);
    cleaning_files(v_filec, v_filev);

    return TRUE;
}

/* ------------------------------------------------------------------------ */
void
free_files(filec, filev)
    int             filec;
    char          **filev;
{
    free_sp(filev);
}

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* ------------------------------------------------------------------------ */
/* Build temporary file name and store to TEMPORARY_NAME */
int
build_temporary_name()
{
#ifdef TMP_FILENAME_TEMPLATE
    /* "/tmp/lhXXXXXX" etc. */
    if (extract_directory == NULL) {
        str_safe_copy(temporary_name, TMP_FILENAME_TEMPLATE,
                      sizeof(temporary_name));
    }
    else {
        xsnprintf(temporary_name, sizeof(temporary_name),
                  "%s/lhXXXXXX", extract_directory);
    }
#else
    char *s;

    str_safe_copy(temporary_name, archive_name, sizeof(temporary_name));
    s = strrchr(temporary_name, '/');
    if (s) {
        int len;
        len = s - temporary_name;
        if (len + strlen("lhXXXXXX") < sizeof(temporary_name))
            /* use directory at archive file */
            strcpy(s, "lhXXXXXX"); /* ok */
        else
            /* use current directory */
            str_safe_copy(temporary_name, "lhXXXXXX", sizeof(temporary_name));
    }
    else
        /* use current directory */
        str_safe_copy(temporary_name, "lhXXXXXX", sizeof(temporary_name));
#endif
#ifdef HAVE_MKSTEMP
    {
        int old_umask, fd;

        old_umask = umask(077);
        fd = mkstemp(temporary_name);
        umask(old_umask);
        return fd;
    }
#else
    {
        int flags;

        mktemp(temporary_name);
        flags = O_CREAT|O_EXCL|O_RDWR;
#ifdef O_BINARY
        flags |= O_BINARY;
#endif
        return open(temporary_name, flags, 0600);
    }
#endif
}

/* ------------------------------------------------------------------------ */
static void
modify_filename_extention(buffer, ext, size)
    char           *buffer;
    char           *ext;
    size_t size;
{
    register char  *p, *dot;

    for (p = buffer, dot = (char *) 0; *p; p++) {
        if (*p == '.')
            dot = p;
        else if (*p == '/')
            dot = (char *) 0;
    }

    if (dot)
        p = dot;

    str_safe_copy(p, ext, size - (p - buffer));
}

/* ------------------------------------------------------------------------ */
/* build backup file name */
void
build_backup_name(buffer, original, size)
    char           *buffer;
    char           *original;
    size_t size;
{
    str_safe_copy(buffer, original, size);
    modify_filename_extention(buffer, BACKUPNAME_EXTENTION, size);    /* ".bak" */
}

/* ------------------------------------------------------------------------ */
void
build_standard_archive_name(buffer, original, size)
    char           *buffer;
    char           *original;
    size_t size;
{
    str_safe_copy(buffer, original, size);
    modify_filename_extention(buffer, ARCHIVENAME_EXTENTION, size);   /* ".lzh" */
}

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* ------------------------------------------------------------------------ */
boolean
need_file(name)
    char           *name;
{
    int             i;

    if (cmd_filec == 0)
        return TRUE;

    for (i = 0; i < cmd_filec; i++) {
        if (patmatch(cmd_filev[i], name, 0))
            return TRUE;
    }

    return FALSE;
}

FILE           *
xfopen(name, mode)
    char           *name, *mode;
{
    FILE           *fp;

    if ((fp = fopen(name, mode)) == NULL)
        fatal_error("Cannot open file \"%s\"", name);

    return fp;
}

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* ------------------------------------------------------------------------ */
static          boolean
open_old_archive_1(name, v_fp)
    char           *name;
    FILE          **v_fp;
{
    FILE           *fp;
    struct stat     stbuf;

    if (stat(name, &stbuf) >= 0 &&
        is_regularfile(&stbuf) &&
        (fp = fopen(name, READ_BINARY)) != NULL) {
        *v_fp = fp;
        archive_file_gid = stbuf.st_gid;
        archive_file_mode = stbuf.st_mode;
        return TRUE;
    }

    *v_fp = NULL;
    archive_file_gid = -1;
    return FALSE;
}

/* ------------------------------------------------------------------------ */
FILE           *
open_old_archive()
{
    FILE           *fp;
    char           *p;
    static char expanded_archive_name[FILENAME_LENGTH];

    if (!strcmp(archive_name, "-")) {
        if (cmd == CMD_EXTRACT || cmd == CMD_LIST) {
#if __MINGW32__
            setmode(fileno(stdin), O_BINARY);
#endif
            return stdin;
        }
        else
            return NULL;
    }
    p = strrchr(archive_name, '.');
    if (p) {
        if (strcasecmp(".LZH", p) == 0
            || strcasecmp(".LZS", p) == 0
            || strcasecmp(".COM", p) == 0  /* DOS SFX */
            || strcasecmp(".EXE", p) == 0
            || strcasecmp(".X", p) == 0    /* HUMAN SFX */
            || strcasecmp(".BAK", p) == 0) {   /* for BackUp */
            open_old_archive_1(archive_name, &fp);
            return fp;
        }
    }

    if (open_old_archive_1(archive_name, &fp))
        return fp;
    xsnprintf(expanded_archive_name, sizeof(expanded_archive_name),
              "%s.lzh", archive_name);
    if (open_old_archive_1(expanded_archive_name, &fp)) {
        archive_name = expanded_archive_name;
        return fp;
    }
    /*
     * if ( (errno&0xffff)!=E_PNNF ) { archive_name =
     * expanded_archive_name; return NULL; }
     */
    xsnprintf(expanded_archive_name, sizeof(expanded_archive_name),
              "%s.lzs", archive_name);
    if (open_old_archive_1(expanded_archive_name, &fp)) {
        archive_name = expanded_archive_name;
        return fp;
    }
    /*
     * if ( (errno&0xffff)!=E_PNNF ) { archive_name =
     * expanded_archive_name; return NULL; }
     */
    /*
     * sprintf( expanded_archive_name , "%s.lzh",archive_name);
     * archive_name = expanded_archive_name;
     */
    return NULL;
}

/* ------------------------------------------------------------------------ */
int
inquire(msg, name, selective)
    char           *msg, *name, *selective;
{
    char            buffer[1024];
    char           *p;

    for (;;) {
        fprintf(stderr, "%s %s ", name, msg);
        fflush(stderr);

        fgets(buffer, 1024, stdin);

        for (p = selective; *p; p++)
            if (buffer[0] == *p)
                return p - selective;
    }
    /* NOTREACHED */
}

/* ------------------------------------------------------------------------ */
void
write_archive_tail(nafp)
    FILE           *nafp;
{
    putc(0x00, nafp);
}

/* ------------------------------------------------------------------------ */
void
copy_old_one(oafp, nafp, hdr)
    FILE           *oafp, *nafp;
    LzHeader       *hdr;
{
    if (noexec) {
        fseeko(oafp, hdr->header_size + hdr->packed_size, SEEK_CUR);
    }
    else {
        reading_filename = archive_name;
        writing_filename = temporary_name;
        copyfile(oafp, nafp, hdr->header_size + hdr->packed_size, 0, 0);
    }
}

#undef exit

void
lha_exit(status)
    int status;
{
    cleanup();
    exit(status);
}
