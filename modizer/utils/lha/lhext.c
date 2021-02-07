/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              lhext.c -- LHarc extract                                    */
/*                                                                          */
/*      Copyright (C) MCMLXXXIX Yooichi.Tagawa                              */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 0.00  Original                             1988.05.23  Y.Tagawa    */
/*  Ver. 1.00  Fixed                                1989.09.22  Y.Tagawa    */
/*  Ver. 0.03  LHa for UNIX                         1991.12.17  M.Oki       */
/*  Ver. 1.12  LHa for UNIX                         1993.10.01  N.Watazaki  */
/*  Ver. 1.13b Symbolic Link Update Bug Fix         1994.06.21  N.Watazaki  */
/*  Ver. 1.14  Source All chagned                   1995.01.14  N.Watazaki  */
/*  Ver. 1.14e bugfix                               1999.04.30  T.Okamoto   */
/* ------------------------------------------------------------------------ */
#include "lha.h"
/* ------------------------------------------------------------------------ */
static int      skip_flg = FALSE;   /* FALSE..No Skip , TRUE..Skip */
static char    *methods[] =
{
    LZHUFF0_METHOD, LZHUFF1_METHOD, LZHUFF2_METHOD, LZHUFF3_METHOD,
    LZHUFF4_METHOD, LZHUFF5_METHOD, LZHUFF6_METHOD, LZHUFF7_METHOD,
    LARC_METHOD, LARC5_METHOD, LARC4_METHOD,
    LZHDIRS_METHOD,
    NULL
};

static void add_dirinfo(char* name, LzHeader* hdr);
static void adjust_dirinfo();

/* ------------------------------------------------------------------------ */
static          boolean
inquire_extract(name)
    char           *name;
{
    struct stat     stbuf;

    skip_flg = FALSE;
    if (stat(name, &stbuf) >= 0) {
        if (!is_regularfile(&stbuf)) {
            error("\"%s\" already exists (not a file)", name);
            return FALSE;
        }

        if (noexec) {
            printf("EXTRACT %s but file is exist.\n", name);
            return FALSE;
        }
        else if (!force) {
            if (!isatty(0)) {
                warning("skip to extract %s.", name);
                return FALSE;
            }

            switch (inquire("OverWrite ?(Yes/[No]/All/Skip)", name, "YyNnAaSs\n")) {
            case 0:
            case 1:/* Y/y */
                break;
            case 2:
            case 3:/* N/n */
            case 8:/* Return */
                return FALSE;
            case 4:
            case 5:/* A/a */
                force = TRUE;
                break;
            case 6:
            case 7:/* S/s */
                skip_flg = TRUE;
                break;
            }
        }
    }

    if (noexec)
        printf("EXTRACT %s\n", name);

    return TRUE;
}

/* ------------------------------------------------------------------------ */
static          boolean
make_parent_path(name)
    char           *name;
{
    char            path[FILENAME_LENGTH];
    struct stat     stbuf;
    register char  *p;

    /* make parent directory name into PATH for recursive call */
    str_safe_copy(path, name, sizeof(path));
    for (p = path + strlen(path); p > path; p--)
        if (p[-1] == '/') {
            *--p = '\0';
            break;
        }

    if (p == path) {
        message("invalid path name \"%s\"", name);
        return FALSE;   /* no more parent. */
    }

    if (GETSTAT(path, &stbuf) >= 0) {
        if (is_directory(&stbuf))
            return TRUE;
    }

    if (verbose)
        message("Making directory \"%s\".", path);

#if defined __MINGW32__
    if (mkdir(path) >= 0)
        return TRUE;
#else
    if (mkdir(path, 0777) >= 0) /* try */
        return TRUE;    /* successful done. */
#endif

    if (!make_parent_path(path))
        return FALSE;

#if defined __MINGW32__
    if (mkdir(path) < 0) {      /* try again */
        error("Cannot make directory \"%s\"", path);
        return FALSE;
    }
#else
    if (mkdir(path, 0777) < 0) {    /* try again */
        error("Cannot make directory \"%s\"", path);
        return FALSE;
    }
#endif

    return TRUE;
}

/* ------------------------------------------------------------------------ */
static FILE    *
open_with_make_path(name)
    char           *name;
{
    FILE           *fp;

    if ((fp = fopen(name, WRITE_BINARY)) == NULL) {
        if (!make_parent_path(name) ||
            (fp = fopen(name, WRITE_BINARY)) == NULL)
            error("Cannot extract a file \"%s\"", name);
    }
    return fp;
}

/* ------------------------------------------------------------------------ */
static void
adjust_info(name, hdr)
    char           *name;
    LzHeader       *hdr;
{
    struct utimbuf utimebuf;

    /* adjust file stamp */
    utimebuf.actime = utimebuf.modtime = hdr->unix_last_modified_stamp;

    if ((hdr->unix_mode & UNIX_FILE_TYPEMASK) != UNIX_FILE_SYMLINK)
        utime(name, &utimebuf);

    if (hdr->extend_type == EXTEND_UNIX
        || hdr->extend_type == EXTEND_OS68K
        || hdr->extend_type == EXTEND_XOSK) {
#ifdef NOT_COMPATIBLE_MODE
        Please need your modification in this space.
#else
        if ((hdr->unix_mode & UNIX_FILE_TYPEMASK) != UNIX_FILE_SYMLINK)
            chmod(name, hdr->unix_mode);
#endif
        if (!getuid()){
            uid_t uid = hdr->unix_uid;
            gid_t gid = hdr->unix_gid;

#if HAVE_GETPWNAM && HAVE_GETGRNAM
            if (hdr->user[0]) {
                struct passwd *ent = getpwnam(hdr->user);
                if (ent) uid = ent->pw_uid;
            }
            if (hdr->group[0]) {
                struct group *ent = getgrnam(hdr->group);
                if (ent) gid = ent->gr_gid;
            }
#endif

#if HAVE_LCHOWN
            if ((hdr->unix_mode & UNIX_FILE_TYPEMASK) == UNIX_FILE_SYMLINK)
                lchown(name, uid, gid);
            else
#endif /* HAVE_LCHWON */
                chown(name, uid, gid);
        }
    }
#if __CYGWIN__
    else {
        /* On Cygwin, execute permission should be set for .exe or .dll. */
        mode_t m;

        umask(m = umask(0));    /* get current umask */
        chmod(name, 0777 & ~m);
    }
#endif
}

/* ------------------------------------------------------------------------ */
static size_t
extract_one(afp, hdr)
    FILE           *afp;    /* archive file */
    LzHeader       *hdr;
{
    FILE           *fp; /* output file */
    struct stat     stbuf;
    char            name[FILENAME_LENGTH];
    unsigned int crc;
    int             method;
    boolean         save_quiet, save_verbose, up_flag;
    char           *q = hdr->name, c;
    size_t read_size = 0;

    if (ignore_directory && strrchr(hdr->name, '/')) {
        q = (char *) strrchr(hdr->name, '/') + 1;
    }
    else {
        if (is_directory_traversal(q)) {
            fprintf(stderr, "Possible directory traversal hack attempt in %s\n", q);
            exit(111);
        }

        if (*q == '/') {
            while (*q == '/') { q++; }

            /*
             * if OSK then strip device name
             */
            if (hdr->extend_type == EXTEND_OS68K
                || hdr->extend_type == EXTEND_XOSK) {
                do
                    c = (*q++);
                while (c && c != '/');
                if (!c || !*q)
                    q = ".";    /* if device name only */
            }
        }
    }

    if (extract_directory)
        xsnprintf(name, sizeof(name), "%s/%s", extract_directory, q);
    else
        str_safe_copy(name, q, sizeof(name));

    /* LZHDIRS_METHODを持つヘッダをチェックする */
    /* 1999.4.30 t.okamoto */
    for (method = 0;; method++) {
        if (methods[method] == NULL) {
            error("Unknown method \"%.*s\"; \"%s\" will be skiped ...",
                  5, hdr->method, name);
            return read_size;
        }
        if (memcmp(hdr->method, methods[method], 5) == 0)
            break;
    }

    if ((hdr->unix_mode & UNIX_FILE_TYPEMASK) == UNIX_FILE_REGULAR
        && method != LZHDIRS_METHOD_NUM) {
    extract_regular:
#if 0
        for (method = 0;; method++) {
            if (methods[method] == NULL) {
                error("Unknown method \"%.*s\"; \"%s\" will be skiped ...",
                      5, hdr->method, name);
                return read_size;
            }
            if (memcmp(hdr->method, methods[method], 5) == 0)
                break;
        }
#endif

        reading_filename = archive_name;
        writing_filename = name;
        if (output_to_stdout || verify_mode) {
            if (noexec) {
                printf("%s %s\n", verify_mode ? "VERIFY" : "EXTRACT", name);
                return read_size;
            }

            save_quiet = quiet;
            save_verbose = verbose;
            if (!quiet && output_to_stdout) {
                printf("::::::::\n%s\n::::::::\n", name);
                quiet = TRUE;
                verbose = FALSE;
            }
            else if (verify_mode) {
                quiet = FALSE;
                verbose = TRUE;
            }

#if __MINGW32__
            {
                int old_mode;
                fflush(stdout);
                old_mode = setmode(fileno(stdout), O_BINARY);
#endif

            crc = decode_lzhuf(afp, stdout,
                               hdr->original_size, hdr->packed_size,
                               name, method, &read_size);
#if __MINGW32__
                fflush(stdout);
                setmode(fileno(stdout), old_mode);
            }
#endif
            quiet = save_quiet;
            verbose = save_verbose;
        }
        else {
            if (skip_flg == FALSE)  {
                up_flag = inquire_extract(name);
                if (up_flag == FALSE && force == FALSE) {
                    return read_size;
                }
            }

            if (skip_flg == TRUE) { /* if skip_flg */
                if (stat(name, &stbuf) == 0 && force != TRUE) {
                    if (stbuf.st_mtime >= hdr->unix_last_modified_stamp) {
                        if (quiet != TRUE)
                            printf("%s : Skipped...\n", name);
                        return read_size;
                    }
                }
            }
            if (noexec) {
                return read_size;
            }

            signal(SIGINT, interrupt);
#ifdef SIGHUP
            signal(SIGHUP, interrupt);
#endif

            unlink(name);
            remove_extracting_file_when_interrupt = TRUE;

            if ((fp = open_with_make_path(name)) != NULL) {
                crc = decode_lzhuf(afp, fp,
                                   hdr->original_size, hdr->packed_size,
                                   name, method, &read_size);
                fclose(fp);
            }
            remove_extracting_file_when_interrupt = FALSE;
            signal(SIGINT, SIG_DFL);
#ifdef SIGHUP
            signal(SIGHUP, SIG_DFL);
#endif
            if (!fp)
                return read_size;
        }

        if (hdr->has_crc && crc != hdr->crc)
            error("CRC error: \"%s\"", name);
    }
    else if ((hdr->unix_mode & UNIX_FILE_TYPEMASK) == UNIX_FILE_DIRECTORY
             || (hdr->unix_mode & UNIX_FILE_TYPEMASK) == UNIX_FILE_SYMLINK
             || method == LZHDIRS_METHOD_NUM) {
        /* ↑これで、Symbolic Link は、大丈夫か？ */
        if (!ignore_directory && !verify_mode) {
            if (noexec) {
                if (quiet != TRUE)
                    printf("EXTRACT %s (directory)\n", name);
                return read_size;
            }
            /* NAME has trailing SLASH '/', (^_^) */
            if ((hdr->unix_mode & UNIX_FILE_TYPEMASK) == UNIX_FILE_SYMLINK) {
                int             l_code;

#ifdef S_IFLNK
                if (skip_flg == FALSE)  {
                    up_flag = inquire_extract(name);
                    if (up_flag == FALSE && force == FALSE) {
                        return read_size;
                    }
                } else {
                    if (GETSTAT(name, &stbuf) == 0 && force != TRUE) {
                        if (stbuf.st_mtime >= hdr->unix_last_modified_stamp) {
                            if (quiet != TRUE)
                                printf("%s : Skipped...\n", name);
                            return read_size;
                        }
                    }
                }

                unlink(name);
                make_parent_path(name);
                l_code = symlink(hdr->realname, name);
                if (l_code < 0) {
                    if (quiet != TRUE)
                        warning("Can't make Symbolic Link \"%s\" -> \"%s\"",
                                name, hdr->realname);
                }
                if (quiet != TRUE) {
                    message("Symbolic Link %s -> %s",
                            name, hdr->realname);
                }
#else
                warning("Can't make Symbolic Link %s -> %s",
                        name, hdr->realname);
                return read_size;
#endif
            } else { /* make directory */
                if (!output_to_stdout && !make_parent_path(name))
                    return read_size;
                /* save directory information */
                add_dirinfo(name, hdr);
            }
        }
    }
    else {
        if (force)              /* force extract */
            goto extract_regular;
        else
            error("Unknown file type: \"%s\". use `f' option to force extract.", name);
    }

    if (!output_to_stdout)
        adjust_info(name, hdr);

    return read_size;
}

/* ------------------------------------------------------------------------ */
/* EXTRACT COMMAND MAIN                                                     */
/* ------------------------------------------------------------------------ */
void
cmd_extract()
{
    LzHeader        hdr;
    off_t           pos;
    FILE           *afp;
    size_t read_size;

    /* open archive file */
    if ((afp = open_old_archive()) == NULL)
        fatal_error("Cannot open archive file \"%s\"", archive_name);

    if (archive_is_msdos_sfx1(archive_name))
        seek_lha_header(afp);

    /* extract each files */
    while (get_header(afp, &hdr)) {
        if (need_file(hdr.name)) {
            pos = ftello(afp);
            read_size = extract_one(afp, &hdr);
            if (read_size != hdr.packed_size) {
                /* when error occurred in extract_one(), should adjust
                   point of file stream */
                if (pos != -1 && afp != stdin)
                    fseeko(afp, pos + hdr.packed_size - read_size, SEEK_SET);
                else {
                    size_t i = hdr.packed_size - read_size;
                    while (i--) fgetc(afp);
                }
            }
        } else {
            if (afp != stdin)
                fseeko(afp, hdr.packed_size, SEEK_CUR);
            else {
                size_t i = hdr.packed_size;
                while (i--) fgetc(afp);
            }
        }
    }

    /* close archive file */
    fclose(afp);

    /* adjust directory information */
    adjust_dirinfo();

    return;
}

int
is_directory_traversal(char *path)
{
    int state = 0;

    for (; *path; path++) {
        switch (state) {
        case 0:
            if (*path == '.') state = 1;
            else state = 3;
            break;
        case 1:
            if (*path == '.') state = 2;
            else if (*path == '/') state = 0;
            else state = 3;
            break;
        case 2:
            if (*path == '/') return 1;
            else state = 3;
            break;
        case 3:
            if (*path == '/') state = 0;
            break;
        }
    }

    return state == 2;
}

/*
 * restore directory information (time stamp).
 * added by A.Iriyama  2003.12.12
 */

typedef struct lhdDirectoryInfo_t {
    struct lhdDirectoryInfo_t *next;
    LzHeader hdr;
} LzHeaderList;

static LzHeaderList *dirinfo;

static void add_dirinfo(char *name, LzHeader *hdr)
{
    LzHeaderList *p;

    if (memcmp(hdr->method, LZHDIRS_METHOD, 5) != 0)
        return;

    p = xmalloc(sizeof(LzHeaderList));

    memcpy(&p->hdr, hdr, sizeof(LzHeader));
    strncpy(p->hdr.name, name, sizeof(p->hdr.name));
    p->hdr.name[sizeof(p->hdr.name)-1] = 0;

    {
        LzHeaderList *tmp = dirinfo;
        dirinfo = p;
        dirinfo->next = tmp;
    }
}

static void adjust_dirinfo()
{
    while (dirinfo) {
        adjust_info(dirinfo->hdr.name, &dirinfo->hdr);

        {
            LzHeaderList *tmp = dirinfo;
            dirinfo = dirinfo->next;
            free(tmp);
        }
    }
}
