/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              lhadd.c -- LHarc Add Command                                */
/*                                                                          */
/*      Copyright (C) MCMLXXXIX Yooichi.Tagawa                              */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */
#include "lha.h"
/* ------------------------------------------------------------------------ */
static void     remove_files();

static char     new_archive_name_buffer[FILENAME_LENGTH];
static char    *new_archive_name;
/* ------------------------------------------------------------------------ */
static void
add_one(fp, nafp, hdr)
    FILE           *fp, *nafp;
    LzHeader       *hdr;
{
    off_t header_pos, next_pos, org_pos, data_pos;
    size_t v_original_size, v_packed_size;

    reading_filename = hdr->name;
    writing_filename = temporary_name;

    if (!fp && generic_format)  /* [generic] doesn't need directory info. */
        return;
    header_pos = ftello(nafp);
    write_header(nafp, hdr);/* DUMMY */

    if ((hdr->unix_mode & UNIX_FILE_SYMLINK) == UNIX_FILE_SYMLINK) {
        if (!quiet)
            printf("%s -> %s\t- Symbolic Link\n", hdr->name, hdr->realname);
    }

    if (hdr->original_size == 0) {  /* empty file, symlink or directory */
        finish_indicator2(hdr->name, "Frozen", 0);
        return;     /* previous write_header is not DUMMY. (^_^) */
    }
    org_pos = ftello(fp);
    data_pos = ftello(nafp);

    hdr->crc = encode_lzhuf(fp, nafp, hdr->original_size,
          &v_original_size, &v_packed_size, hdr->name, hdr->method);

    if (v_packed_size < v_original_size) {
        next_pos = ftello(nafp);
    }
    else {          /* retry by stored method */
        fseeko(fp, org_pos, SEEK_SET);
        fseeko(nafp, data_pos, SEEK_SET);
        hdr->crc = encode_stored_crc(fp, nafp, hdr->original_size,
                      &v_original_size, &v_packed_size);
        fflush(nafp);
        next_pos = ftello(nafp);
#if HAVE_FTRUNCATE
        if (ftruncate(fileno(nafp), next_pos) == -1)
            error("cannot truncate archive");
#elif HAVE_CHSIZE
        if (chsize(fileno(nafp), next_pos) == -1)
            error("cannot truncate archive");
#else
        CAUSE COMPILE ERROR
#endif
        memcpy(hdr->method, LZHUFF0_METHOD, METHOD_TYPE_STORAGE);
    }
    hdr->original_size = v_original_size;
    hdr->packed_size = v_packed_size;
    fseeko(nafp, header_pos, SEEK_SET);
    write_header(nafp, hdr);
    fseeko(nafp, next_pos, SEEK_SET);
}

FILE           *
append_it(name, oafp, nafp)
    char           *name;
    FILE           *oafp, *nafp;
{
    LzHeader        ahdr, hdr;
    FILE           *fp;
    long            old_header;
    int             cmp;
    int             filec;
    char          **filev;
    int             i;
    struct stat     stbuf;

    boolean         directory, symlink;

    if (GETSTAT(name, &stbuf) < 0) {
        error("Cannot access file \"%s\"", name);   /* See cleaning_files, Why? */
        return oafp;
    }

    directory = is_directory(&stbuf);
#ifdef S_IFLNK
    symlink = is_symlink(&stbuf);
#else
    symlink = 0;
#endif

    fp = NULL;
    if (!directory && !symlink && !noexec) {
        fp = fopen(name, READ_BINARY);
        if (!fp) {
            error("Cannot open file \"%s\": %s", name, strerror(errno));
            return oafp;
        }
    }

    init_header(name, &stbuf, &hdr);

    cmp = 0;                    /* avoid compiler warnings `uninitialized' */
    while (oafp) {
        old_header = ftello(oafp);
        if (!get_header(oafp, &ahdr)) {
            /* end of archive or error occurred */
            fclose(oafp);
            oafp = NULL;
            break;
        }

        cmp = strcmp(ahdr.name, hdr.name);
        if (cmp < 0) {          /* SKIP */
            /* copy old to new */
            if (!noexec) {
                fseeko(oafp, old_header, SEEK_SET);
                copy_old_one(oafp, nafp, &ahdr);
            }
            else
                fseeko(oafp, ahdr.packed_size, SEEK_CUR);
        } else if (cmp == 0) {  /* REPLACE */
            /* drop old archive's */
            fseeko(oafp, ahdr.packed_size, SEEK_CUR);
            break;
        } else {                /* cmp > 0, INSERT */
            fseeko(oafp, old_header, SEEK_SET);
            break;
        }
    }

    if (!oafp || cmp > 0) { /* not in archive */
        if (noexec)
            printf("ADD %s\n", name);
        else
            add_one(fp, nafp, &hdr);
    }
    else {      /* cmp == 0 */
        if (!update_if_newer ||
            ahdr.unix_last_modified_stamp < hdr.unix_last_modified_stamp) {
                                /* newer than archive's */
            if (noexec)
                printf("REPLACE %s\n", name);
            else
                add_one(fp, nafp, &hdr);
        }
        else {                  /* copy old to new */
            if (!noexec) {
                fseeko(oafp, old_header, SEEK_SET);
                copy_old_one(oafp, nafp, &ahdr);
            }
        }
    }

    if (fp) fclose(fp);

    if (directory) {            /* recursive call */
        if (find_files(name, &filec, &filev)) {
            for (i = 0; i < filec; i++)
                oafp = append_it(filev[i], oafp, nafp);
            free_files(filec, filev);
        }
    }
    return oafp;
}

/* ------------------------------------------------------------------------ */
static void
find_update_files(oafp)
    FILE           *oafp;   /* old archive */
{
    char            name[FILENAME_LENGTH];
    struct string_pool sp;
    LzHeader        hdr;
    off_t           pos;
    struct stat     stbuf;
    int             len;

    pos = ftello(oafp);

    init_sp(&sp);
    while (get_header(oafp, &hdr)) {
        if ((hdr.unix_mode & UNIX_FILE_TYPEMASK) == UNIX_FILE_REGULAR) {
            if (stat(hdr.name, &stbuf) >= 0)    /* exist ? */
                add_sp(&sp, hdr.name, strlen(hdr.name) + 1);
        }
        else if ((hdr.unix_mode & UNIX_FILE_TYPEMASK) == UNIX_FILE_DIRECTORY) {
            strcpy(name, hdr.name); /* ok */
            len = strlen(name);
            if (len > 0 && name[len - 1] == '/')
                name[--len] = '\0'; /* strip tail '/' */
            if (stat(name, &stbuf) >= 0)    /* exist ? */
                add_sp(&sp, name, len + 1);
        }
        fseeko(oafp, hdr.packed_size, SEEK_CUR);
    }

    fseeko(oafp, pos, SEEK_SET);

    finish_sp(&sp, &cmd_filec, &cmd_filev);
}

/* ------------------------------------------------------------------------ */
static void
delete(oafp, nafp)
    FILE           *oafp, *nafp;
{
    LzHeader ahdr;
    off_t old_header_pos;

    old_header_pos = ftello(oafp);
    while (get_header(oafp, &ahdr)) {
        if (need_file(ahdr.name)) { /* skip */
            fseeko(oafp, ahdr.packed_size, SEEK_CUR);
            if (noexec || !quiet) {
                if ((ahdr.unix_mode & UNIX_FILE_TYPEMASK) == UNIX_FILE_SYMLINK)
                    message("delete %s -> %s", ahdr.name, ahdr.realname);
                else
                    message("delete %s", ahdr.name);
            }
        }
        else {      /* copy */
            if (noexec) {
                fseeko(oafp, ahdr.packed_size, SEEK_CUR);
            }
            else {
                fseeko(oafp, old_header_pos, SEEK_SET);
                copy_old_one(oafp, nafp, &ahdr);
            }
        }
        old_header_pos = ftello(oafp);
    }
    return;
}

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* ------------------------------------------------------------------------ */
static FILE    *
build_temporary_file()
{
    FILE *afp;

    signal(SIGINT, interrupt);
#ifdef SIGHUP
    signal(SIGHUP, interrupt);
#endif

    temporary_fd = build_temporary_name();
    if (temporary_fd == -1)
        fatal_error("Cannot open temporary file \"%s\"", temporary_name);

    afp = fdopen(temporary_fd, WRITE_BINARY);
    if (afp == NULL)
        fatal_error("Cannot open temporary file \"%s\"", temporary_name);

    return afp;
}

/* ------------------------------------------------------------------------ */
static void
build_backup_file()
{

    build_backup_name(backup_archive_name, archive_name,
                      sizeof(backup_archive_name));
    if (!noexec) {
        signal(SIGINT, SIG_IGN);
#ifdef SIGHUP
        signal(SIGHUP, SIG_IGN);
#endif
        if (rename(archive_name, backup_archive_name) < 0) {
#if __MINGW32__
            /* On MinGW, cannot rename when
               newfile (backup_archive_name) already exists */
            if (unlink(backup_archive_name) < 0 ||
                rename(archive_name, backup_archive_name) < 0)
#endif
            fatal_error("Cannot make backup file \"%s\"", archive_name);
        }
        recover_archive_when_interrupt = TRUE;
        signal(SIGINT, interrupt);
#ifdef SIGHUP
        signal(SIGHUP, interrupt);
#endif
    }
}

/* ------------------------------------------------------------------------ */
static void
report_archive_name_if_different()
{
    if (!quiet && new_archive_name == new_archive_name_buffer) {
        /* warning at old archive is SFX */
        message("New archive file is \"%s\"", new_archive_name);
    }
}

/* ------------------------------------------------------------------------ */
void
temporary_to_new_archive_file(new_archive_size)
    size_t new_archive_size;
{
    FILE *oafp, *nafp;

    if (!strcmp(new_archive_name, "-")) {
        nafp = stdout;
        writing_filename = "starndard output";
#if __MINGW32__
        setmode(fileno(stdout), O_BINARY);
#endif
    }
    else {
        unlink(new_archive_name);
        if (rename(temporary_name, new_archive_name) == 0)
            return;
        nafp = xfopen(new_archive_name, WRITE_BINARY);
        writing_filename = archive_name;
    }

    oafp = xfopen(temporary_name, READ_BINARY);
    reading_filename = temporary_name;
    copyfile(oafp, nafp, new_archive_size, 0, 0);
    if (nafp != stdout)
        fclose(nafp);
    fclose(oafp);

    recover_archive_when_interrupt = FALSE;
    unlink(temporary_name);
}

/* ------------------------------------------------------------------------ */
static void
set_archive_file_mode()
{
    int             umask_value;
    struct stat     stbuf;

    if (archive_file_gid < 0) {
        umask(umask_value = umask(0));
        archive_file_mode = (~umask_value) & 0666;  /* rw-rw-rw- */
        if (stat(".", &stbuf) >= 0)
            archive_file_gid = stbuf.st_gid;
    }
    if (archive_file_gid >= 0)
        chown(new_archive_name, getuid(), archive_file_gid);

    chmod(new_archive_name, archive_file_mode);
}

/* ------------------------------------------------------------------------ */
/*                          REMOVE FILE/DIRECTORY                           */
/* ------------------------------------------------------------------------ */
static void
remove_one(name)
    char           *name;
{
    struct stat     stbuf;
    int             filec;
    char          **filev;

    if (GETSTAT(name, &stbuf) < 0) {
        warning("Cannot access \"%s\": %s", name, strerror(errno));
    }
    else if (is_directory(&stbuf)) {
        if (find_files(name, &filec, &filev)) {
            remove_files(filec, filev);
            free_files(filec, filev);
        }
        else
            warning("Cannot open directory \"%s\"", name);

        if (noexec)
            message("REMOVE DIRECTORY %s", name);
        else if (rmdir(name) < 0)
            warning("Cannot remove directory \"%s\"", name);
        else if (verbose)
            message("Removed %s.", name);
    }
    else if (is_regularfile(&stbuf)) {
        if (noexec)
            message("REMOVE FILE %s.", name);
        else if (unlink(name) < 0)
            warning("Cannot remove \"%s\"", name);
        else if (verbose)
            message("Removed %s.", name);
    }
#ifdef S_IFLNK
    else if (is_symlink(&stbuf)) {
        if (noexec)
            printf("REMOVE SYMBOLIC LINK %s.\n", name);
        else if (unlink(name) < 0)
            warning("Cannot remove", name);
        else if (verbose)
            message("Removed %s.", name);
    }
#endif
    else {
        error("Cannot remove file \"%s\" (not a file or directory)", name);
    }
}

static void
remove_files(filec, filev)
    int             filec;
    char          **filev;
{
    int             i;

    for (i = 0; i < filec; i++)
        remove_one(filev[i]);
}

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* ------------------------------------------------------------------------ */
void
cmd_add()
{
    LzHeader        ahdr;
    FILE           *oafp, *nafp;
    int             i;
    long            old_header;
    boolean         old_archive_exist;
    size_t          new_archive_size;

    /* exit if no operation */
    if (!update_if_newer && cmd_filec == 0) {
        error("No files given in argument, do nothing.");
        exit(1);
    }

    /* open old archive if exist */
    if ((oafp = open_old_archive()) == NULL)
        old_archive_exist = FALSE;
    else
        old_archive_exist = TRUE;

    if (update_if_newer && cmd_filec == 0) {
        warning("No files given in argument");
        if (!oafp) {
            error("archive file \"%s\" does not exists.",
                  archive_name);
            exit(1);
        }
    }

    if (new_archive && old_archive_exist) {
        fclose(oafp);
        oafp = NULL;
    }

    if (oafp && archive_is_msdos_sfx1(archive_name)) {
        seek_lha_header(oafp);
        build_standard_archive_name(new_archive_name_buffer,
                                    archive_name,
                                    sizeof(new_archive_name_buffer));
        new_archive_name = new_archive_name_buffer;
    }
    else {
        new_archive_name = archive_name;
    }

    /* build temporary file */
    nafp = NULL;                /* avoid compiler warnings `uninitialized' */
    if (!noexec)
        nafp = build_temporary_file();

    /* find needed files when automatic update */
    if (update_if_newer && cmd_filec == 0)
        find_update_files(oafp);

    /* build new archive file */
    /* cleaning arguments */
    cleaning_files(&cmd_filec, &cmd_filev);
    if (cmd_filec == 0) {
        if (oafp)
            fclose(oafp);
        if (!noexec)
            fclose(nafp);
        return;
    }

    for (i = 0; i < cmd_filec; i++) {
        int j;

        if (strcmp(cmd_filev[i], archive_name) == 0) {
            /* exclude target archive */
            warning("specified file \"%s\" is the generating archive. skip",
                    cmd_filev[i]);
            for (j = i; j < cmd_filec-1; j++)
                cmd_filev[j] = cmd_filev[j+1];
            cmd_filec--;
            i--;
            continue;
        }

        /* exclude files specified by -x option */
        for (j = 0; exclude_files && exclude_files[j]; j++) {
            if (fnmatch(exclude_files[j], basename(cmd_filev[i]),
                        FNM_PATHNAME|FNM_NOESCAPE|FNM_PERIOD) == 0)
                goto next;
        }

        oafp = append_it(cmd_filev[i], oafp, nafp);
    next:
        ;
    }

    if (oafp) {
        old_header = ftello(oafp);
        while (get_header(oafp, &ahdr)) {
            if (noexec)
                fseeko(oafp, ahdr.packed_size, SEEK_CUR);
            else {
                fseeko(oafp, old_header, SEEK_SET);
                copy_old_one(oafp, nafp, &ahdr);
            }
            old_header = ftello(oafp);
        }
        fclose(oafp);
    }

    new_archive_size = 0;       /* avoid compiler warnings `uninitialized' */
    if (!noexec) {
        off_t tmp;

        write_archive_tail(nafp);
        tmp = ftello(nafp);
        if (tmp == -1) {
            warning("ftello(): %s", strerror(errno));
            new_archive_size = 0;
        }
        else
            new_archive_size = tmp;

        fclose(nafp);
    }

    /* build backup archive file */
    if (old_archive_exist && backup_old_archive)
        build_backup_file();

    report_archive_name_if_different();

    /* copy temporary file to new archive file */
    if (!noexec) {
        if (strcmp(new_archive_name, "-") == 0 ||
            rename(temporary_name, new_archive_name) < 0) {

            temporary_to_new_archive_file(new_archive_size);
        }
    }

    /* set new archive file mode/group */
    set_archive_file_mode();

    /* remove archived files */
    if (delete_after_append)
        remove_files(cmd_filec, cmd_filev);

    return;
}

/* ------------------------------------------------------------------------ */
void
cmd_delete()
{
    FILE *oafp, *nafp;
    size_t new_archive_size;

    /* open old archive if exist */
    if ((oafp = open_old_archive()) == NULL)
        fatal_error("Cannot open archive file \"%s\"", archive_name);

    /* exit if no operation */
    if (cmd_filec == 0) {
        fclose(oafp);
        warning("No files given in argument, do nothing.");
        return;
    }

    if (archive_is_msdos_sfx1(archive_name)) {
        seek_lha_header(oafp);
        build_standard_archive_name(new_archive_name_buffer,
                                    archive_name,
                                    sizeof(new_archive_name_buffer));
        new_archive_name = new_archive_name_buffer;
    }
    else {
        new_archive_name = archive_name;
    }

    /* build temporary file */
    nafp = NULL;                /* avoid compiler warnings `uninitialized' */
    if (!noexec)
        nafp = build_temporary_file();

    /* build new archive file */
    delete(oafp, nafp);
    fclose(oafp);

    new_archive_size = 0;       /* avoid compiler warnings `uninitialized' */
    if (!noexec) {
        off_t tmp;

        write_archive_tail(nafp);
        tmp = ftello(nafp);
        if (tmp == -1) {
            warning("ftello(): %s", strerror(errno));
            new_archive_size = 0;
        }
        else
            new_archive_size = tmp;

        fclose(nafp);
    }

    /* build backup archive file */
    if (backup_old_archive)
        build_backup_file();

    /* 1999.5.24 t.oka */
    if(!noexec && new_archive_size <= 1){
        unlink(temporary_name);
        if (!backup_old_archive)
            unlink(archive_name);
        warning("The archive file \"%s\" was removed because it would be empty.", new_archive_name);
        return;
    }

    report_archive_name_if_different();

    /* copy temporary file to new archive file */
    if (!noexec) {
        if (rename(temporary_name, new_archive_name) < 0)
            temporary_to_new_archive_file(new_archive_size);
    }

    /* set new archive file mode/group */
    set_archive_file_mode();

    return;
}
