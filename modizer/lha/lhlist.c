/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              lhlist.c -- LHarc list                                      */
/*                                                                          */
/*      Copyright (C) MCMLXXXIX Yooichi.Tagawa                              */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 0.00   Original                        1988.05.23  Y.Tagawa        */
/*  Ver. 1.00   Fixed                           1989.09.22  Y.Tagawa        */
/*  Ver. 1.01   Bug Fix for month name          1989.12.25  Y.Tagawa        */
/*  Ver. 1.10   Changed list format             1993.10.01  N.Watazaki      */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/*  Ver. 1.14e  Bug Fix for many problems       1999.05.25  T.Okamoto       */
/* ------------------------------------------------------------------------ */
#include "lha.h"

/* ------------------------------------------------------------------------ */
/* Print Stuff                                                              */
/* ------------------------------------------------------------------------ */
/* need 14 or 22 (when verbose_listing is TRUE) column spaces */
static void
print_size(packed_size, original_size)
    long packed_size, original_size;
{
    if (verbose_listing)
        printf("%7lu ", packed_size);

    printf("%7lu ", original_size);

    if (original_size == 0L)
        printf("******");
    else    /* Changed N.Watazaki */
        printf("%5.1f%%", packed_size * 100.0 / original_size);
}

/* ------------------------------------------------------------------------ */
/* need 12 or 17 (when verbose_listing is TRUE) column spaces */
static void
print_stamp(t)
    time_t t;
{
    static unsigned int threshold;
    static char     t_month[12 * 3 + 1] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    struct tm      *p;

    if (t == 0) {
        if (verbose_listing && verbose)
            printf("                   "); /* 19 spaces */
        else
            printf("            "); /* 12 spaces */
        return;
    }

    if (!threshold) {
        time_t now = time(0);
        p = localtime(&now);
        threshold = p->tm_year * 12 + p->tm_mon - 6;
    }

    p = localtime(&t);

    if (verbose_listing && verbose)
        printf("%04d-%02d-%02d %02d:%02d:%02d",
               p->tm_year+1900, p->tm_mon+1, p->tm_mday,
               p->tm_hour, p->tm_min, p->tm_sec);
    else if (p->tm_year * 12 + p->tm_mon > threshold)
        printf("%.3s %2d %02d:%02d",
        &t_month[p->tm_mon * 3], p->tm_mday, p->tm_hour, p->tm_min);
    else
        printf("%.3s %2d  %04d",
            &t_month[p->tm_mon * 3], p->tm_mday, p->tm_year + 1900);
}

/* ------------------------------------------------------------------------ */
static void
print_bar()
{
    if (verbose_listing) {
        if (verbose)
            /*      PERMISSION  UID  GID    PACKED    SIZE  RATIO METHOD CRC     STAMP            LV */
            printf("---------- ----------- ------- ------- ------ ---------- ------------------- ---\n");
        else
            /*      PERMISSION  UID  GID    PACKED    SIZE  RATIO METHOD CRC     STAMP     NAME */
            printf("---------- ----------- ------- ------- ------ ---------- ------------ ----------\n");
    }
    else {
        if (verbose)
            /*      PERMISSION  UID  GID      SIZE  RATIO     STAMP     LV */
            printf("---------- ----------- ------- ------ ------------ ---\n");
        else
            /*      PERMISSION  UID  GID      SIZE  RATIO     STAMP           NAME */
            printf("---------- ----------- ------- ------ ------------ --------------------\n");
    }
}

/* ------------------------------------------------------------------------ */
/*                                                                          */
/* ------------------------------------------------------------------------ */
static void
list_header()
{
    if (verbose_listing) {
        if (verbose)
            printf("PERMISSION  UID  GID    PACKED    SIZE  RATIO METHOD CRC     STAMP            LV\n");
        else
            printf("PERMISSION  UID  GID    PACKED    SIZE  RATIO METHOD CRC     STAMP     NAME\n");
    }
    else {
        if (verbose)
            printf("PERMISSION  UID  GID      SIZE  RATIO     STAMP     LV\n");
        else
            printf("PERMISSION  UID  GID      SIZE  RATIO     STAMP           NAME\n");
    }
    print_bar();
}

/* ------------------------------------------------------------------------ */
static void
list_one(hdr)
    register LzHeader *hdr;
{
    register int    mode = 0;
    register char  *p;
    char            method[6];
    char modebits[11];

    if (verbose) {
        if ((hdr->unix_mode & UNIX_FILE_SYMLINK) != UNIX_FILE_SYMLINK)
            printf("%s\n", hdr->name);
        else
            printf("%s -> %s\n", hdr->name, hdr->realname);
    }

    strncpy(method, hdr->method, 5);
    method[5] = '\0';

    switch (hdr->extend_type) {
    case EXTEND_UNIX:
        mode = hdr->unix_mode;

        if (mode & UNIX_FILE_DIRECTORY)
            modebits[0] = 'd';
        else if ((mode & UNIX_FILE_SYMLINK) == UNIX_FILE_SYMLINK)
            modebits[0] = 'l';
        else
            modebits[0] = '-';
        modebits[1] = ((mode & UNIX_OWNER_READ_PERM) ? 'r' : '-');
        modebits[2] = ((mode & UNIX_OWNER_WRITE_PERM) ? 'w' : '-');
        modebits[3] = (mode & UNIX_SETUID) ? 's' :
            ((mode & UNIX_OWNER_EXEC_PERM) ? 'x' : '-');
        modebits[4] = ((mode & UNIX_GROUP_READ_PERM) ? 'r' : '-');
        modebits[5] = ((mode & UNIX_GROUP_WRITE_PERM) ? 'w' : '-');
        modebits[6] = (mode & UNIX_SETGID) ? 's' :
            ((mode & UNIX_GROUP_EXEC_PERM) ? 'x' : '-');
        modebits[7] = ((mode & UNIX_OTHER_READ_PERM) ? 'r' : '-');
        modebits[8] = ((mode & UNIX_OTHER_WRITE_PERM) ? 'w' : '-');
        modebits[9] = (mode & UNIX_STICKYBIT) ? 't' :
            ((mode & UNIX_OTHER_EXEC_PERM) ? 'x' : '-');
        modebits[10] = 0;

        printf("%s ", modebits);
        break;
    case EXTEND_OS68K:
     /**/ case EXTEND_XOSK:/**/
        mode = hdr->unix_mode;
        printf("%c%c%c%c%c%c%c%c   ",
               ((mode & OSK_DIRECTORY_PERM) ? 'd' : '-'),
               ((mode & OSK_SHARED_PERM) ? 's' : '-'),
               ((mode & OSK_OTHER_EXEC_PERM) ? 'e' : '-'),
               ((mode & OSK_OTHER_WRITE_PERM) ? 'w' : '-'),
               ((mode & OSK_OTHER_READ_PERM) ? 'r' : '-'),
               ((mode & OSK_OWNER_EXEC_PERM) ? 'e' : '-'),
               ((mode & OSK_OWNER_WRITE_PERM) ? 'w' : '-'),
               ((mode & OSK_OWNER_READ_PERM) ? 'r' : '-'));

        break;
    default:
        switch (hdr->extend_type) { /* max 18 characters */
        case EXTEND_GENERIC:
            p = "[generic]";
            break;
        case EXTEND_CPM:
            p = "[CP/M]";
            break;
        case EXTEND_FLEX:
            p = "[FLEX]";
            break;
        case EXTEND_OS9:
            p = "[OS-9]";
            break;
        case EXTEND_OS68K:
            p = "[OS-9/68K]";
            break;
        case EXTEND_MSDOS:
            p = "[MS-DOS]";
            break;
        case EXTEND_MACOS:
            p = "[Mac OS]";
            break;
        case EXTEND_OS2:
            p = "[OS/2]";
            break;
        case EXTEND_HUMAN:
            p = "[Human68K]";
            break;
        case EXTEND_OS386:
            p = "[OS-386]";
            break;
        case EXTEND_RUNSER:
            p = "[Runser]";
            break;
#ifdef EXTEND_TOWNSOS
            /* This ID isn't fixed */
        case EXTEND_TOWNSOS:
            p = "[TownsOS]";
            break;
#endif
        case EXTEND_JAVA:
            p = "[JAVA]";
            break;
            /* Ouch!  Please customize it's ID.  */
        default:
            p = "[unknown]";
            break;
        }
        printf("%-11.11s", p);
        break;
    }

    switch (hdr->extend_type) {
    case EXTEND_UNIX:
    case EXTEND_OS68K:
    case EXTEND_XOSK:
        if (hdr->user[0])
            printf("%5.5s/", hdr->user);
        else
            printf("%5d/", hdr->unix_uid);

        if (hdr->group[0])
            printf("%-5.5s ", hdr->group);
        else
            printf("%-5d ", hdr->unix_gid);
        break;
    default:
        printf("%12s", "");
        break;
    }

    print_size(hdr->packed_size, hdr->original_size);

    if (verbose_listing) {
        if (hdr->has_crc)
            printf(" %s %04x", method, hdr->crc);
        else
            printf(" %s ****", method);
    }

    printf(" ");
    print_stamp(hdr->unix_last_modified_stamp);

    if (!verbose) {
        if ((hdr->unix_mode & UNIX_FILE_SYMLINK) != UNIX_FILE_SYMLINK)
            printf(" %s", hdr->name);
        else {
            printf(" %s -> %s", hdr->name, hdr->realname);
        }
    }
    if (verbose)
        printf(" [%d]", hdr->header_level);
    printf("\n");

}

/* ------------------------------------------------------------------------ */
static void
list_tailer(list_files, packed_size_total, original_size_total)
    int list_files;
    unsigned long packed_size_total, original_size_total;
{
    struct stat     stbuf;

    print_bar();

    printf(" Total %9d file%c ",
           list_files, (list_files == 1) ? ' ' : 's');
    print_size(packed_size_total, original_size_total);
    printf(" ");

    if (verbose_listing)
        printf("           ");

    if (stat(archive_name, &stbuf) < 0)
        print_stamp((time_t) 0);
    else
        print_stamp(stbuf.st_mtime);

    printf("\n");
}

/* ------------------------------------------------------------------------ */
/* LIST COMMAND MAIN                                                        */
/* ------------------------------------------------------------------------ */
void
cmd_list()
{
    FILE           *afp;
    LzHeader        hdr;
    int             i;

    unsigned long packed_size_total;
    unsigned long original_size_total;
    int list_files;

    /* initialize total count */
    packed_size_total = 0L;
    original_size_total = 0L;
    list_files = 0;

    /* open archive file */
    if ((afp = open_old_archive()) == NULL) {
        error("Cannot open archive \"%s\"", archive_name);
        exit(1);
    }
    if (archive_is_msdos_sfx1(archive_name))
        seek_lha_header(afp);

    /* print header message */
    if (!quiet)
        list_header();

    /* print each file information */
    while (get_header(afp, &hdr)) {
        if (need_file(hdr.name)) {
            list_one(&hdr);
            list_files++;
            packed_size_total += hdr.packed_size;
            original_size_total += hdr.original_size;
        }

        if (afp != stdin)
            fseeko(afp, hdr.packed_size, SEEK_CUR);
        else {
            i = hdr.packed_size;
            while (i--)
                fgetc(afp);
        }
    }

    /* close archive file */
    fclose(afp);

    /* print tailer message */
    if (!quiet)
        list_tailer(list_files, packed_size_total, original_size_total);

    return;
}
