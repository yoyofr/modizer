/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              header.c -- header manipulate functions                     */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Original                                                Y.Tagawa        */
/*  modified                                    1991.12.16  M.Oki           */
/*  Ver. 1.10  Symbolic Link added              1993.10.01  N.Watazaki      */
/*  Ver. 1.13b Symbolic Link Bug Fix            1994.08.22  N.Watazaki      */
/*  Ver. 1.14  Source All chagned               1995.01.14  N.Watazaki      */
/*  Ver. 1.14i bug fixed                        2000.10.06  t.okamoto       */
/*  Ver. 1.14i Contributed UTF-8 convertion for Mac OS X                    */
/*                                              2002.06.29  Hiroto Sakai    */
/*  Ver. 1.14i autoconfiscated & rewritten      2003.02.23  Koji Arai       */
/* ------------------------------------------------------------------------ */
#include "lha.h"

#define DUMP_HEADER 1           /* for debugging */

#if !STRCHR_8BIT_CLEAN
/* should use 8 bit clean version */
#undef strchr
#undef strrchr
#define strchr  xstrchr
#define strrchr  xstrrchr
#endif

static char    *get_ptr;
#define GET_BYTE()      (*get_ptr++ & 0xff)

#if DUMP_HEADER
static char    *start_ptr;
#define setup_get(PTR)  (start_ptr = get_ptr = (PTR))
#define get_byte()      dump_get_byte()
#define skip_bytes(len) dump_skip_bytes(len)
#else
#define setup_get(PTR)  (get_ptr = (PTR))
#define get_byte()      GET_BYTE()
#define skip_bytes(len) (get_ptr += (len))
#endif
#define put_ptr         get_ptr
#define setup_put(PTR)  (put_ptr = (PTR))
#define put_byte(c)     (*put_ptr++ = (char)(c))

int optional_archive_kanji_code = NONE;
int optional_system_kanji_code = NONE;
char *optional_archive_delim = NULL;
char *optional_system_delim = NULL;
int optional_filename_case = NONE;

#ifdef MULTIBYTE_FILENAME
int default_system_kanji_code = MULTIBYTE_FILENAME;
#else
int default_system_kanji_code = NONE;
#endif

int
calc_sum(p, len)
    char *p;
    int len;
{
    int sum = 0;

    while (len--) sum += *p++;

    return sum & 0xff;
}

#if DUMP_HEADER
static int
dump_get_byte()
{
    int c;

    if (verbose_listing && verbose > 1)
        printf("%02d %2d: ", get_ptr - start_ptr, 1);
    c = GET_BYTE();
    if (verbose_listing && verbose > 1) {
        if (isprint(c))
            printf("%d(0x%02x) '%c'\n", c, c, c);
        else
            printf("%d(0x%02x)\n", c, c);
    }
    return c;
}

static void
dump_skip_bytes(len)
    int len;
{
    if (len == 0) return;
    if (verbose_listing && verbose > 1) {
        printf("%02d %2d: ", get_ptr - start_ptr, len);
        while (len--)
            printf("0x%02x ", GET_BYTE());
        printf("... ignored\n");
    }
    else
        get_ptr += len;
}
#endif

static int
get_word()
{
    int b0, b1;
    int w;

#if DUMP_HEADER
    if (verbose_listing && verbose > 1)
        printf("%02d %2d: ", get_ptr - start_ptr, 2);
#endif
    b0 = GET_BYTE();
    b1 = GET_BYTE();
    w = (b1 << 8) + b0;
#if DUMP_HEADER
    if (verbose_listing && verbose > 1)
        printf("%d(0x%04x)\n", w, w);
#endif
    return w;
}

static void
put_word(v)
    unsigned int    v;
{
    put_byte(v);
    put_byte(v >> 8);
}

static long
get_longword()
{
    long b0, b1, b2, b3;
    long l;

#if DUMP_HEADER
    if (verbose_listing && verbose > 1)
        printf("%02d %2d: ", get_ptr - start_ptr, 4);
#endif
    b0 = GET_BYTE();
    b1 = GET_BYTE();
    b2 = GET_BYTE();
    b3 = GET_BYTE();
    l = (b3 << 24) + (b2 << 16) + (b1 << 8) + b0;
#if DUMP_HEADER
    if (verbose_listing && verbose > 1)
        printf("%ld(0x%08lx)\n", l, l);
#endif
    return l;
}

static void
put_longword(v)
    long v;
{
    put_byte(v);
    put_byte(v >> 8);
    put_byte(v >> 16);
    put_byte(v >> 24);
}

static int
get_bytes(buf, len, size)
    char *buf;
    int len, size;
{
    int i;

#if DUMP_HEADER
    if (verbose_listing && verbose > 1)
        printf("%02d %2d: \"", get_ptr - start_ptr, len);

    for (i = 0; i < len; i++) {
        if (i < size) buf[i] = get_ptr[i];

        if (verbose_listing && verbose > 1) {
            if (isprint(buf[i]))
                printf("%c", buf[i]);
            else
                printf("\\x%02x", (unsigned char)buf[i]);
        }
    }

    if (verbose_listing && verbose > 1)
        printf("\"\n");
#else
    for (i = 0; i < len && i < size; i++)
        buf[i] = get_ptr[i];
#endif

    get_ptr += len;
    return i;
}

static void
put_bytes(buf, len)
    char *buf;
    int len;
{
    int i;
    for (i = 0; i < len; i++)
        put_byte(buf[i]);
}

/* added by Koji Arai */
void
convert_filename(name, len, size,
                 from_code, to_code,
                 from_delim, to_delim,
                 case_to)
    char *name;
    int len;                    /* length of name */
    int size;                   /* size of name buffer */
    int from_code, to_code, case_to;
    char *from_delim, *to_delim;

{
    int i;
#ifdef MULTIBYTE_FILENAME
    char tmp[FILENAME_LENGTH];
    int to_code_save = NONE;

    if (from_code == CODE_CAP) {
        len = cap_to_sjis(tmp, name, sizeof(tmp));
        strncpy(name, tmp, size);
        name[size-1] = 0;
        len = strlen(name);
        from_code = CODE_SJIS;
    }

    if (to_code == CODE_CAP) {
        to_code_save = CODE_CAP;
        to_code = CODE_SJIS;
    }

    if (from_code == CODE_SJIS && to_code == CODE_UTF8) {
        for (i = 0; i < len; i++)
            /* FIXME: provisionally fix for the Mac OS CoreFoundation */
            if ((unsigned char)name[i] == LHA_PATHSEP)  name[i] = '/';
        sjis_to_utf8(tmp, name, sizeof(tmp));
        strncpy(name, tmp, size);
        name[size-1] = 0;
        len = strlen(name);
        for (i = 0; i < len; i++)
            if (name[i] == '/')  name[i] = LHA_PATHSEP;
        from_code = CODE_UTF8;
    }
    else if (from_code == CODE_UTF8 && to_code == CODE_SJIS) {
        for (i = 0; i < len; i++)
            /* FIXME: provisionally fix for the Mac OS CoreFoundation */
            if ((unsigned char)name[i] == LHA_PATHSEP)  name[i] = '/';
        utf8_to_sjis(tmp, name, sizeof(tmp));
        strncpy(name, tmp, size);
        name[size-1] = 0;
        len = strlen(name);
        for (i = 0; i < len; i++)
            if (name[i] == '/')  name[i] = LHA_PATHSEP;
        from_code = CODE_SJIS;
    }
#endif

    /* special case: if `name' has small lettter, not convert case. */
    if (from_code == CODE_SJIS && case_to == TO_LOWER) {
        for (i = 0; i < len; i++) {
#ifdef MULTIBYTE_FILENAME
            if (SJIS_FIRST_P(name[i]) && SJIS_SECOND_P(name[i+1]))
                i++;
            else
#endif
            if (islower(name[i])) {
                case_to = NONE;
                break;
            }
        }
    }

    for (i = 0; i < len; i ++) {
#ifdef MULTIBYTE_FILENAME
        if (from_code == CODE_EUC &&
            (unsigned char)name[i] == 0x8e) {
            if (to_code != CODE_SJIS) {
                i++;
                continue;
            }

            /* X0201 KANA */
            memmove(name + i, name + i + 1, len - i);
            len--;
            continue;
        }
        if (from_code == CODE_SJIS && X0201_KANA_P(name[i])) {
            if (to_code != CODE_EUC) {
                continue;
            }

            if (len == size - 1) /* check overflow */
                len--;
            memmove(name+i+1, name+i, len-i);
            name[i] = 0x8e;
            i++;
            len++;
            continue;
        }
        if (from_code == CODE_EUC && (name[i] & 0x80) && (name[i+1] & 0x80)) {
            int c1, c2;
            if (to_code != CODE_SJIS) {
                i++;
                continue;
            }

            c1 = (unsigned char)name[i];
            c2 = (unsigned char)name[i+1];
            euc2sjis(&c1, &c2);
            name[i] = c1;
            name[i+1] = c2;
            i++;
            continue;
        }
        if (from_code == CODE_SJIS &&
            SJIS_FIRST_P(name[i]) &&
            SJIS_SECOND_P(name[i+1])) {
            int c1, c2;

            if (to_code != CODE_EUC) {
                i++;
                continue;
            }

            c1 = (unsigned char)name[i];
            c2 = (unsigned char)name[i+1];
            sjis2euc(&c1, &c2);
            name[i] = c1;
            name[i+1] = c2;
            i++;
            continue;
        }
#endif /* MULTIBYTE_FILENAME */
        {
            char *ptr;

            /* transpose from_delim to to_delim */

            if ((ptr = strchr(from_delim, name[i])) != NULL) {
                name[i] = to_delim[ptr - from_delim];
                continue;
            }
        }

        if (case_to == TO_UPPER && islower(name[i])) {
            name[i] = toupper(name[i]);
            continue;
        }
        if (case_to == TO_LOWER && isupper(name[i])) {
            name[i] = tolower(name[i]);
            continue;
        }
    }

#ifdef MULTIBYTE_FILENAME
    if (to_code_save == CODE_CAP) {
        len = sjis_to_cap(tmp, name, sizeof(tmp));
        strncpy(name, tmp, size);
        name[size-1] = 0;
        len = strlen(name);
    }
#endif /* MULTIBYTE_FILENAME */
}

/*
 * Generic (MS-DOS style) time stamp format (localtime):
 *
 *  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16
 * |<---- year-1980 --->|<- month ->|<--- day ---->|
 *
 *  15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 * |<--- hour --->|<---- minute --->|<- second/2 ->|
 *
 */

static time_t
generic_to_unix_stamp(t)
    long t;
{
    struct tm tm;

#define subbits(n, off, len) (((n) >> (off)) & ((1 << (len))-1))

    tm.tm_sec  = subbits(t,  0, 5) * 2;
    tm.tm_min  = subbits(t,  5, 6);
    tm.tm_hour = subbits(t, 11, 5);
    tm.tm_mday = subbits(t, 16, 5);
    tm.tm_mon  = subbits(t, 21, 4) - 1;
    tm.tm_year = subbits(t, 25, 7) + 80;
    tm.tm_isdst = -1;

#if HAVE_MKTIME
    return mktime(&tm);
#else
    return timelocal(&tm);
#endif
}

static long
unix_to_generic_stamp(t)
    time_t t;
{
    struct tm *tm = localtime(&t);

    tm->tm_year -= 80;
    tm->tm_mon += 1;

    return ((long)(tm->tm_year << 25) +
            (tm->tm_mon  << 21) +
            (tm->tm_mday << 16) +
            (tm->tm_hour << 11) +
            (tm->tm_min  << 5) +
            (tm->tm_sec / 2));
}

static unsigned long
wintime_to_unix_stamp()
{
#if HAVE_UINT64_T
    uint64_t t;
    uint64_t epoch = ((uint64_t)0x019db1de << 32) + 0xd53e8000;
                     /* 0x019db1ded53e8000ULL: 1970-01-01 00:00:00 (UTC) */

    t = (unsigned long)get_longword();
    t |= (uint64_t)(unsigned long)get_longword() << 32;
    t = (t - epoch) / 10000000;
    return t;
#else
    int i, borrow;
    unsigned long t, q, x;
    unsigned long wintime[8];
    unsigned long epoch[8] = {0x01,0x9d,0xb1,0xde, 0xd5,0x3e,0x80,0x00};
                                /* 1970-01-01 00:00:00 (UTC) */
    /* wintime -= epoch */
    borrow = 0;
    for (i = 7; i >= 0; i--) {
        wintime[i] = (unsigned)get_byte() - epoch[i] - borrow;
        borrow = (wintime[i] > 0xff) ? 1 : 0;
        wintime[i] &= 0xff;
    }

    /* q = wintime / 10000000 */
    t = q = 0;
    x = 10000000;               /* x: 24bit */
    for (i = 0; i < 8; i++) {
        t = (t << 8) + wintime[i]; /* 24bit + 8bit. t must be 32bit variable */
        q <<= 8;                   /* q must be 32bit (time_t) */
        q += t / x;
        t %= x;     /* 24bit */
    }
    return q;
#endif
}

/*
 * extended header
 *
 *             size  field name
 *  --------------------------------
 *  base header:         :
 *           2 or 4  next-header size  [*1]
 *  --------------------------------------
 *  ext header:   1  ext-type            ^
 *                ?  contents            | [*1] next-header size
 *           2 or 4  next-header size    v
 *  --------------------------------------
 *
 *  on level 1, 2 header:
 *    size field is 2 bytes
 *  on level 3 header:
 *    size field is 4 bytes
 */

static ssize_t
get_extended_header(fp, hdr, header_size, hcrc)
    FILE *fp;
    LzHeader *hdr;
    size_t header_size;
    unsigned int *hcrc;
{
    char data[LZHEADER_STORAGE];
    int name_length;
    char dirname[FILENAME_LENGTH];
    int dir_length = 0;
    int i;
    ssize_t whole_size = header_size;
    int ext_type;
    int n = 1 + hdr->size_field_length; /* `ext-type' + `next-header size' */

    if (hdr->header_level == 0)
        return 0;

    name_length = strlen(hdr->name);

    while (header_size) {
        setup_get(data);
        if (sizeof(data) < header_size) {
            error("header size (%ld) too large.", header_size);
            exit(1);
        }

        if (fread(data, header_size, 1, fp) == 0) {
            error("Invalid header (LHa file ?)");
            return -1;
        }

        ext_type = get_byte();
        switch (ext_type) {
        case 0:
            /* header crc (CRC-16) */
            hdr->header_crc = get_word();
            /* clear buffer for CRC calculation. */
            data[1] = data[2] = 0;
            skip_bytes(header_size - n - 2);
            break;
        case 1:
            /* filename */
            name_length =
                get_bytes(hdr->name, header_size-n, sizeof(hdr->name)-1);
            hdr->name[name_length] = 0;
            break;
        case 2:
            /* directory */
            dir_length = get_bytes(dirname, header_size-n, sizeof(dirname)-1);
            dirname[dir_length] = 0;
            break;
        case 0x40:
            /* MS-DOS attribute */
            hdr->attribute = get_word();
            break;
        case 0x41:
            /* Windows time stamp (FILETIME structure) */
            /* it is time in 100 nano seconds since 1601-01-01 00:00:00 */

            skip_bytes(8); /* create time is ignored */

            /* set last modified time */
            if (hdr->header_level >= 2)
                skip_bytes(8);  /* time_t has been already set */
            else
                hdr->unix_last_modified_stamp = wintime_to_unix_stamp();

            skip_bytes(8); /* last access time is ignored */

            break;
        case 0x50:
            /* UNIX permission */
            hdr->unix_mode = get_word();
            break;
        case 0x51:
            /* UNIX gid and uid */
            hdr->unix_gid = get_word();
            hdr->unix_uid = get_word();
            break;
        case 0x52:
            /* UNIX group name */
            i = get_bytes(hdr->group, header_size-n, sizeof(hdr->group)-1);
            hdr->group[i] = '\0';
            break;
        case 0x53:
            /* UNIX user name */
            i = get_bytes(hdr->user, header_size-n, sizeof(hdr->user)-1);
            hdr->user[i] = '\0';
            break;
        case 0x54:
            /* UNIX last modified time */
            hdr->unix_last_modified_stamp = (time_t) get_longword();
            break;
        default:
            /* other headers */
            /* 0x39: multi-disk header
               0x3f: uncompressed comment
               0x42: 64bit large file size
               0x48-0x4f(?): reserved for authenticity verification
               0x7d: encapsulation
               0x7e: extended attribute - platform information
               0x7f: extended attribute - permission, owner-id and timestamp
                     (level 3 on OS/2)
               0xc4: compressed comment (dict size: 4096)
               0xc5: compressed comment (dict size: 8192)
               0xc6: compressed comment (dict size: 16384)
               0xc7: compressed comment (dict size: 32768)
               0xc8: compressed comment (dict size: 65536)
               0xd0-0xdf(?): operating systemm specific information
               0xfc: encapsulation (another opinion)
               0xfe: extended attribute - platform information(another opinion)
               0xff: extended attribute - permission, owner-id and timestamp
                     (level 3 on UNLHA32) */
            if (verbose)
                warning("unknown extended header 0x%02x", ext_type);
            skip_bytes(header_size - n);
            break;
        }

        if (hcrc)
            *hcrc = calccrc(*hcrc, data, header_size);

        if (hdr->size_field_length == 2)
            whole_size += header_size = get_word();
        else
            whole_size += header_size = get_longword();
    }

    /* concatenate dirname and filename */
    if (dir_length) {
        if (name_length + dir_length >= sizeof(hdr->name)) {
            warning("the length of pathname \"%s%s\" is too long.",
                    dirname, hdr->name);
            name_length = sizeof(hdr->name) - dir_length - 1;
            hdr->name[name_length] = 0;
        }
        strcat(dirname, hdr->name); /* ok */
        strcpy(hdr->name, dirname); /* ok */
        name_length += dir_length;
    }

    return whole_size;
}

#define I_HEADER_SIZE           0               /* level 0,1,2   */
#define I_HEADER_CHECKSUM       1               /* level 0,1     */
#define I_METHOD                2               /* level 0,1,2,3 */
#define I_PACKED_SIZE           7               /* level 0,1,2,3 */
#define I_ATTRIBUTE             19              /* level 0,1,2,3 */
#define I_HEADER_LEVEL          20              /* level 0,1,2,3 */

#define COMMON_HEADER_SIZE      21      /* size of common part */

#define I_GENERIC_HEADER_SIZE 24 /* + name_length */
#define I_LEVEL0_HEADER_SIZE  36 /* + name_length (unix extended) */
#define I_LEVEL1_HEADER_SIZE  27 /* + name_length */
#define I_LEVEL2_HEADER_SIZE  26 /* + padding */
#define I_LEVEL3_HEADER_SIZE  32

/*
 * level 0 header
 *
 *
 * offset  size  field name
 * ----------------------------------
 *     0      1  header size    [*1]
 *     1      1  header sum
 *            ---------------------------------------
 *     2      5  method ID                         ^
 *     7      4  packed size    [*2]               |
 *    11      4  original size                     |
 *    15      2  time                              |
 *    17      2  date                              |
 *    19      1  attribute                         | [*1] header size (X+Y+22)
 *    20      1  level (0x00 fixed)                |
 *    21      1  name length                       |
 *    22      X  pathname                          |
 * X +22      2  file crc (CRC-16)                 |
 * X +24      Y  ext-header(old style)             v
 * -------------------------------------------------
 * X+Y+24        data                              ^
 *                 :                               | [*2] packed size
 *                 :                               v
 * -------------------------------------------------
 *
 * ext-header(old style)
 *     0      1  ext-type ('U')
 *     1      1  minor version
 *     2      4  UNIX time
 *     6      2  mode
 *     8      2  uid
 *    10      2  gid
 *
 * attribute (MS-DOS)
 *    bit1  read only
 *    bit2  hidden
 *    bit3  system
 *    bit4  volume label
 *    bit5  directory
 *    bit6  archive bit (need to backup)
 *
 */
static int
get_header_level0(fp, hdr, data)
    FILE *fp;
    LzHeader *hdr;
    char *data;
{
    size_t header_size;
    ssize_t extend_size;
    int checksum;
    int name_length;
    int i;

    hdr->size_field_length = 2; /* in bytes */
    hdr->header_size = header_size = get_byte();
    checksum = get_byte();

    if (fread(data + COMMON_HEADER_SIZE,
              header_size + 2 - COMMON_HEADER_SIZE, 1, fp) == 0) {
        error("Invalid header (LHarc file ?)");
        return FALSE;   /* finish */
    }

    if (calc_sum(data + I_METHOD, header_size) != checksum) {
        error("Checksum error (LHarc file?)");
        return FALSE;
    }

    get_bytes(hdr->method, 5, sizeof(hdr->method));
    hdr->packed_size = get_longword();
    hdr->original_size = get_longword();
    hdr->unix_last_modified_stamp = generic_to_unix_stamp(get_longword());
    hdr->attribute = get_byte(); /* MS-DOS attribute */
    hdr->header_level = get_byte();
    name_length = get_byte();
    i = get_bytes(hdr->name, name_length, sizeof(hdr->name)-1);
    hdr->name[i] = '\0';

    /* defaults for other type */
    hdr->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
    hdr->unix_gid = 0;
    hdr->unix_uid = 0;

    extend_size = header_size+2 - name_length - 24;

    if (extend_size < 0) {
        if (extend_size == -2) {
            /* CRC field is not given */
            hdr->extend_type = EXTEND_GENERIC;
            hdr->has_crc = FALSE;

            return TRUE;
        } 

        error("Unkonwn header (lha file?)");
        exit(1);
    }

    hdr->has_crc = TRUE;
    hdr->crc = get_word();

    if (extend_size == 0)
        return TRUE;

    hdr->extend_type = get_byte();
    extend_size--;

    if (hdr->extend_type == EXTEND_UNIX) {
        if (extend_size >= 11) {
            hdr->minor_version = get_byte();
            hdr->unix_last_modified_stamp = (time_t) get_longword();
            hdr->unix_mode = get_word();
            hdr->unix_uid = get_word();
            hdr->unix_gid = get_word();
            extend_size -= 11;
        } else {
            hdr->extend_type = EXTEND_GENERIC;
        }
    }
    if (extend_size > 0)
        skip_bytes(extend_size);

    hdr->header_size += 2;
    return TRUE;
}

/*
 * level 1 header
 *
 *
 * offset   size  field name
 * -----------------------------------
 *     0       1  header size   [*1]
 *     1       1  header sum
 *             -------------------------------------
 *     2       5  method ID                        ^
 *     7       4  skip size     [*2]               |
 *    11       4  original size                    |
 *    15       2  time                             |
 *    17       2  date                             |
 *    19       1  attribute (0x20 fixed)           | [*1] header size (X+Y+25)
 *    20       1  level (0x01 fixed)               |
 *    21       1  name length                      |
 *    22       X  filename                         |
 * X+ 22       2  file crc (CRC-16)                |
 * X+ 24       1  OS ID                            |
 * X +25       Y  ???                              |
 * X+Y+25      2  next-header size                 v
 * -------------------------------------------------
 * X+Y+27      Z  ext-header                       ^
 *                 :                               |
 * -----------------------------------             | [*2] skip size
 * X+Y+Z+27       data                             |
 *                 :                               v
 * -------------------------------------------------
 *
 */
static int
get_header_level1(fp, hdr, data)
    FILE *fp;
    LzHeader *hdr;
    char *data;
{
    size_t header_size;
    ssize_t extend_size;
    int checksum;
    int name_length;
    int i, dummy;

    hdr->size_field_length = 2; /* in bytes */
    hdr->header_size = header_size = get_byte();
    checksum = get_byte();

    if (fread(data + COMMON_HEADER_SIZE,
              header_size + 2 - COMMON_HEADER_SIZE, 1, fp) == 0) {
        error("Invalid header (LHarc file ?)");
        return FALSE;   /* finish */
    }

    if (calc_sum(data + I_METHOD, header_size) != checksum) {
        error("Checksum error (LHarc file?)");
        return FALSE;
    }

    get_bytes(hdr->method, 5, sizeof(hdr->method));
    hdr->packed_size = get_longword(); /* skip size */
    hdr->original_size = get_longword();
    hdr->unix_last_modified_stamp = generic_to_unix_stamp(get_longword());
    hdr->attribute = get_byte(); /* 0x20 fixed */
    hdr->header_level = get_byte();

    name_length = get_byte();
    i = get_bytes(hdr->name, name_length, sizeof(hdr->name)-1);
    hdr->name[i] = '\0';

    /* defaults for other type */
    hdr->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
    hdr->unix_gid = 0;
    hdr->unix_uid = 0;

    hdr->has_crc = TRUE;
    hdr->crc = get_word();
    hdr->extend_type = get_byte();

    dummy = header_size+2 - name_length - I_LEVEL1_HEADER_SIZE;
    if (dummy > 0)
        skip_bytes(dummy); /* skip old style extend header */

    extend_size = get_word();
    extend_size = get_extended_header(fp, hdr, extend_size, 0);
    if (extend_size == -1)
        return FALSE;

    /* On level 1 header, size fields should be adjusted. */
    /* the `packed_size' field contains the extended header size. */
    /* the `header_size' field does not. */
    hdr->packed_size -= extend_size;
    hdr->header_size += extend_size + 2;

    return TRUE;
}

/*
 * level 2 header
 *
 *
 * offset   size  field name
 * --------------------------------------------------
 *     0       2  total header size [*1]           ^
 *             -----------------------             |
 *     2       5  method ID                        |
 *     7       4  packed size       [*2]           |
 *    11       4  original size                    |
 *    15       4  time                             |
 *    19       1  RESERVED (0x20 fixed)            | [*1] total header size
 *    20       1  level (0x02 fixed)               |      (X+26+(1))
 *    21       2  file crc (CRC-16)                |
 *    23       1  OS ID                            |
 *    24       2  next-header size                 |
 * -----------------------------------             |
 *    26       X  ext-header                       |
 *                 :                               |
 * -----------------------------------             |
 * X +26      (1) padding                          v
 * -------------------------------------------------
 * X +26+(1)      data                             ^
 *                 :                               | [*2] packed size
 *                 :                               v
 * -------------------------------------------------
 *
 */
static int
get_header_level2(fp, hdr, data)
    FILE *fp;
    LzHeader *hdr;
    char *data;
{
    size_t header_size;
    ssize_t extend_size;
    int padding;
    unsigned int hcrc;

    hdr->size_field_length = 2; /* in bytes */
    hdr->header_size = header_size = get_word();

    if (fread(data + COMMON_HEADER_SIZE,
              I_LEVEL2_HEADER_SIZE - COMMON_HEADER_SIZE, 1, fp) == 0) {
        error("Invalid header (LHarc file ?)");
        return FALSE;   /* finish */
    }

    get_bytes(hdr->method, 5, sizeof(hdr->method));
    hdr->packed_size = get_longword();
    hdr->original_size = get_longword();
    hdr->unix_last_modified_stamp = get_longword();
    hdr->attribute = get_byte(); /* reserved */
    hdr->header_level = get_byte();

    /* defaults for other type */
    hdr->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
    hdr->unix_gid = 0;
    hdr->unix_uid = 0;

    hdr->has_crc = TRUE;
    hdr->crc = get_word();
    hdr->extend_type = get_byte();
    extend_size = get_word();

    INITIALIZE_CRC(hcrc);
    hcrc = calccrc(hcrc, data, get_ptr - data);

    extend_size = get_extended_header(fp, hdr, extend_size, &hcrc);
    if (extend_size == -1)
        return FALSE;

    padding = header_size - I_LEVEL2_HEADER_SIZE - extend_size;
    while (padding--)           /* padding should be 0 or 1 */
        hcrc = UPDATE_CRC(hcrc, fgetc(fp));

    if (hdr->header_crc != hcrc)
        error("header CRC error");

    return TRUE;
}

/*
 * level 3 header
 *
 *
 * offset   size  field name
 * --------------------------------------------------
 *     0       2  size field length (4 fixed)      ^
 *     2       5  method ID                        |
 *     7       4  packed size       [*2]           |
 *    11       4  original size                    |
 *    15       4  time                             |
 *    19       1  RESERVED (0x20 fixed)            | [*1] total header size
 *    20       1  level (0x03 fixed)               |      (X+32)
 *    21       2  file crc (CRC-16)                |
 *    23       1  OS ID                            |
 *    24       4  total header size [*1]           |
 *    28       4  next-header size                 |
 * -----------------------------------             |
 *    32       X  ext-header                       |
 *                 :                               v
 * -------------------------------------------------
 * X +32          data                             ^
 *                 :                               | [*2] packed size
 *                 :                               v
 * -------------------------------------------------
 *
 */
static int
get_header_level3(fp, hdr, data)
    FILE *fp;
    LzHeader *hdr;
    char *data;
{
    size_t header_size;
    ssize_t extend_size;
    int padding;
    unsigned int hcrc;

    hdr->size_field_length = get_word();

    if (fread(data + COMMON_HEADER_SIZE,
              I_LEVEL3_HEADER_SIZE - COMMON_HEADER_SIZE, 1, fp) == 0) {
        error("Invalid header (LHarc file ?)");
        return FALSE;   /* finish */
    }

    get_bytes(hdr->method, 5, sizeof(hdr->method));
    hdr->packed_size = get_longword();
    hdr->original_size = get_longword();
    hdr->unix_last_modified_stamp = get_longword();
    hdr->attribute = get_byte(); /* reserved */
    hdr->header_level = get_byte();

    /* defaults for other type */
    hdr->unix_mode = UNIX_FILE_REGULAR | UNIX_RW_RW_RW;
    hdr->unix_gid = 0;
    hdr->unix_uid = 0;

    hdr->has_crc = TRUE;
    hdr->crc = get_word();
    hdr->extend_type = get_byte();
    hdr->header_size = header_size = get_longword();
    extend_size = get_longword();

    INITIALIZE_CRC(hcrc);
    hcrc = calccrc(hcrc, data, get_ptr - data);

    extend_size = get_extended_header(fp, hdr, extend_size, &hcrc);
    if (extend_size == -1)
        return FALSE;

    padding = header_size - I_LEVEL3_HEADER_SIZE - extend_size;
    while (padding--)           /* padding should be 0 */
        hcrc = UPDATE_CRC(hcrc, fgetc(fp));

    if (hdr->header_crc != hcrc)
        error("header CRC error");

    return TRUE;
}

boolean
get_header(fp, hdr)
    FILE *fp;
    LzHeader *hdr;
{
    char data[LZHEADER_STORAGE];

    int archive_kanji_code = CODE_SJIS;
    int system_kanji_code = default_system_kanji_code;
    char *archive_delim = "\377\\"; /* `\' is for level 0 header and
                                       broken archive. */
    char *system_delim = "//";
    int filename_case = NONE;
    int end_mark;

    memset(hdr, 0, sizeof(LzHeader));

    setup_get(data);

    if ((end_mark = getc(fp)) == EOF || end_mark == 0) {
        return FALSE;           /* finish */
    }
    data[0] = end_mark;

    if (fread(data + 1, COMMON_HEADER_SIZE - 1, 1, fp) == 0) {
        error("Invalid header (LHarc file ?)");
        return FALSE;           /* finish */
    }

    switch (data[I_HEADER_LEVEL]) {
    case 0:
        if (get_header_level0(fp, hdr, data) == FALSE)
            return FALSE;
        break;
    case 1:
        if (get_header_level1(fp, hdr, data) == FALSE)
            return FALSE;
        break;
    case 2:
        if (get_header_level2(fp, hdr, data) == FALSE)
            return FALSE;
        break;
    case 3:
        if (get_header_level3(fp, hdr, data) == FALSE)
            return FALSE;
        break;
    default:
        error("Unknown level header (level %d)", data[I_HEADER_LEVEL]);
        return FALSE;
    }

    /* filename conversion */
    switch (hdr->extend_type) {
    case EXTEND_MSDOS:
        filename_case = noconvertcase ? NONE : TO_LOWER;
        break;
    case EXTEND_HUMAN:
    case EXTEND_OS68K:
    case EXTEND_XOSK:
    case EXTEND_UNIX:
    case EXTEND_JAVA:
        filename_case = NONE;
        break;

    case EXTEND_MACOS:
        archive_delim = "\377/:\\";
                          /* `\' is for level 0 header and broken archive. */
        system_delim = "/://";
        filename_case = NONE;
        break;

    default:
        filename_case = noconvertcase ? NONE : TO_LOWER;
        break;
    }

    if (optional_archive_kanji_code)
        archive_kanji_code = optional_archive_kanji_code;
    if (optional_system_kanji_code)
        system_kanji_code = optional_system_kanji_code;
    if (optional_archive_delim)
        archive_delim = optional_archive_delim;
    if (optional_system_delim)
        system_delim = optional_system_delim;
    if (optional_filename_case)
        filename_case = optional_filename_case;

    /* kanji code and delimiter conversion */
    convert_filename(hdr->name, strlen(hdr->name), sizeof(hdr->name),
                     archive_kanji_code,
                     system_kanji_code,
                     archive_delim, system_delim, filename_case);

    if ((hdr->unix_mode & UNIX_FILE_SYMLINK) == UNIX_FILE_SYMLINK) {
        char *p;
        /* split symbolic link */
        p = strchr(hdr->name, '|');
        if (p) {
            /* hdr->name is symbolic link name */
            /* hdr->realname is real name */
            *p = 0;
            strcpy(hdr->realname, p+1); /* ok */
        }
        else
            error("unknown symlink name \"%s\"", hdr->name);
    }

    return TRUE;
}

/* skip SFX header */
int
seek_lha_header(fp)
    FILE *fp;
{
    unsigned char   buffer[64 * 1024]; /* max seek size */
    unsigned char  *p;
    int             n;

    n = fread(buffer, 1, sizeof(buffer), fp);

    for (p = buffer; p < buffer + n; p++) {
        if (! (p[I_METHOD]=='-' && p[I_METHOD+1]=='l' && p[I_METHOD+4]=='-'))
            continue;
        /* found "-l??-" keyword (as METHOD type string) */

        /* level 0 or 1 header */
        if ((p[I_HEADER_LEVEL] == 0 || p[I_HEADER_LEVEL] == 1)
            && p[I_HEADER_SIZE] > 20
            && p[I_HEADER_CHECKSUM] == calc_sum(p+2, p[I_HEADER_SIZE])) {
            if (fseeko(fp, (p - buffer) - n, SEEK_CUR) == -1)
                fatal_error("cannot seek header");
            return 0;
        }

        /* level 2 header */
        if (p[I_HEADER_LEVEL] == 2
            && p[I_HEADER_SIZE] >= 24
            && p[I_ATTRIBUTE] == 0x20) {
            if (fseeko(fp, (p - buffer) - n, SEEK_CUR) == -1)
                fatal_error("cannot seek header");
            return 0;
        }
    }

    if (fseeko(fp, -n, SEEK_CUR) == -1)
        fatal_error("cannot seek header");
    return -1;
}


/* remove leading `xxxx/..' */
static char *
remove_leading_dots(char *path)
{
    char *first = path;
    char *ptr = 0;

    if (strcmp(first, "..") == 0) {
        warning("Removing leading `..' from member name.");
        return first+1;         /* change to "." */
    }

    if (strstr(first, "..") == 0)
        return first;

    while (path && *path) {

        if (strcmp(path, "..") == 0)
            ptr = path = path+2;
        else if (strncmp(path, "../", 3) == 0)
            ptr = path = path+3;
        else
            path = strchr(path, '/');

        if (path && *path == '/') {
            path++;
        }
    }

    if (ptr) {
        warning("Removing leading `%.*s' from member name.", ptr-first, first);
        return ptr;
    }

    return first;
}

void
init_header(name, v_stat, hdr)
    char           *name;
    struct stat    *v_stat;
    LzHeader       *hdr;
{
    int             len;

    memset(hdr, 0, sizeof(LzHeader));

    /* the `method' member is rewrote by the encoding function.
       but need set for empty files */
    memcpy(hdr->method, LZHUFF0_METHOD, METHOD_TYPE_STORAGE);

    hdr->packed_size = 0;
    hdr->original_size = v_stat->st_size;
    hdr->attribute = GENERIC_ATTRIBUTE;
    hdr->header_level = header_level;
    len = str_safe_copy(hdr->name,
                        remove_leading_dots(name),
                        sizeof(hdr->name));
    hdr->crc = 0x0000;
    hdr->extend_type = EXTEND_UNIX;
    hdr->unix_last_modified_stamp = v_stat->st_mtime;
    /* since 00:00:00 JAN.1.1970 */
#ifdef NOT_COMPATIBLE_MODE
    /* Please need your modification in this space. */
#else
    hdr->unix_mode = v_stat->st_mode;
#endif

    hdr->unix_uid = v_stat->st_uid;
    hdr->unix_gid = v_stat->st_gid;

#if INCLUDE_OWNER_NAME_IN_HEADER
#if HAVE_GETPWUID
    {
        struct passwd *ent = getpwuid(hdr->unix_uid);

        if (ent) {
            strncpy(hdr->user, ent->pw_name, sizeof(hdr->user));
            if (hdr->user[sizeof(hdr->user)-1])
                hdr->user[sizeof(hdr->user)-1] = 0;
        }
    }
#endif
#if HAVE_GETGRGID
    {
        struct group *ent = getgrgid(hdr->unix_gid);

        if (ent) {
            strncpy(hdr->group, ent->gr_name, sizeof(hdr->group));
            if (hdr->group[sizeof(hdr->group)-1])
                hdr->group[sizeof(hdr->group)-1] = 0;
        }
    }
#endif
#endif /* INCLUDE_OWNER_NAME_IN_HEADER */
    if (is_directory(v_stat)) {
        memcpy(hdr->method, LZHDIRS_METHOD, METHOD_TYPE_STORAGE);
        hdr->attribute = GENERIC_DIRECTORY_ATTRIBUTE;
        hdr->original_size = 0;
        if (len > 0 && hdr->name[len - 1] != '/') {
            if (len < sizeof(hdr->name)-1)
                strcpy(&hdr->name[len++], "/"); /* ok */
            else
                warning("the length of dirname \"%s\" is too long.",
                        hdr->name);
        }
    }

#ifdef S_IFLNK
    if (is_symlink(v_stat)) {
        memcpy(hdr->method, LZHDIRS_METHOD, METHOD_TYPE_STORAGE);
        hdr->attribute = GENERIC_DIRECTORY_ATTRIBUTE;
        hdr->original_size = 0;
        readlink(name, hdr->realname, sizeof(hdr->realname));
    }
#endif
}

static void
write_unix_info(hdr)
    LzHeader *hdr;
{
    /* UNIX specific informations */

    put_word(5);            /* size */
    put_byte(0x50);         /* permission */
    put_word(hdr->unix_mode);

    put_word(7);            /* size */
    put_byte(0x51);         /* gid and uid */
    put_word(hdr->unix_gid);
    put_word(hdr->unix_uid);

    if (hdr->group[0]) {
        int len = strlen(hdr->group);
        put_word(len + 3);  /* size */
        put_byte(0x52);     /* group name */
        put_bytes(hdr->group, len);
    }

    if (hdr->user[0]) {
        int len = strlen(hdr->user);
        put_word(len + 3);  /* size */
        put_byte(0x53);     /* user name */
        put_bytes(hdr->user, len);
    }

    if (hdr->header_level == 1) {
        put_word(7);        /* size */
        put_byte(0x54);     /* time stamp */
        put_longword(hdr->unix_last_modified_stamp);
    }
}

static size_t
write_header_level0(data, hdr, pathname)
    LzHeader *hdr;
    char *data, *pathname;
{
    int limit;
    int name_length;
    size_t header_size;

    setup_put(data);
    memset(data, 0, LZHEADER_STORAGE);

    put_byte(0x00);             /* header size */
    put_byte(0x00);             /* check sum */
    put_bytes(hdr->method, 5);
    put_longword(hdr->packed_size);
    put_longword(hdr->original_size);
    put_longword(unix_to_generic_stamp(hdr->unix_last_modified_stamp));
    put_byte(hdr->attribute);
    put_byte(hdr->header_level); /* level 0 */

    /* write pathname (level 0 header contains the directory part) */
    name_length = strlen(pathname);
    if (generic_format)
        limit = 255 - I_GENERIC_HEADER_SIZE + 2;
    else
        limit = 255 - I_LEVEL0_HEADER_SIZE + 2;

    if (name_length > limit) {
        warning("the length of pathname \"%s\" is too long.", pathname);
        name_length = limit;
    }
    put_byte(name_length);
    put_bytes(pathname, name_length);
    put_word(hdr->crc);

    if (generic_format) {
        header_size = I_GENERIC_HEADER_SIZE + name_length - 2;
        data[I_HEADER_SIZE] = header_size;
        data[I_HEADER_CHECKSUM] = calc_sum(data + I_METHOD, header_size);
    } else {
        /* write old-style extend header */
        put_byte(EXTEND_UNIX);
        put_byte(CURRENT_UNIX_MINOR_VERSION);
        put_longword(hdr->unix_last_modified_stamp);
        put_word(hdr->unix_mode);
        put_word(hdr->unix_uid);
        put_word(hdr->unix_gid);

        /* size of extended header is 12 */
        header_size = I_LEVEL0_HEADER_SIZE + name_length - 2;
        data[I_HEADER_SIZE] = header_size;
        data[I_HEADER_CHECKSUM] = calc_sum(data + I_METHOD, header_size);
    }

    return header_size + 2;
}

static size_t
write_header_level1(data, hdr, pathname)
    LzHeader *hdr;
    char *data, *pathname;
{
    int name_length, dir_length, limit;
    char *basename, *dirname;
    size_t header_size;
    char *extend_header_top;
    size_t extend_header_size;

    basename = strrchr(pathname, LHA_PATHSEP);
    if (basename) {
        basename++;
        name_length = strlen(basename);
        dirname = pathname;
        dir_length = basename - dirname;
    }
    else {
        basename = pathname;
        name_length = strlen(basename);
        dirname = "";
        dir_length = 0;
    }

    setup_put(data);
    memset(data, 0, LZHEADER_STORAGE);

    put_byte(0x00);             /* header size */
    put_byte(0x00);             /* check sum */
    put_bytes(hdr->method, 5);
    put_longword(hdr->packed_size);
    put_longword(hdr->original_size);
    put_longword(unix_to_generic_stamp(hdr->unix_last_modified_stamp));
    put_byte(0x20);
    put_byte(hdr->header_level); /* level 1 */

    /* level 1 header: write filename (basename only) */
    limit = 255 - I_LEVEL1_HEADER_SIZE + 2;
    if (name_length > limit) {
        put_byte(0);            /* name length */
    }
    else {
        put_byte(name_length);
        put_bytes(basename, name_length);
    }

    put_word(hdr->crc);

    if (generic_format)
        put_byte(0x00);
    else
        put_byte(EXTEND_UNIX);

    /* write extend header from here. */

    extend_header_top = put_ptr+2; /* +2 for the field `next header size' */
    header_size = extend_header_top - data - 2;

    /* write filename and dirname */

    if (name_length > limit) {
        put_word(name_length + 3); /* size */
        put_byte(0x01);         /* filename */
        put_bytes(basename, name_length);
    }

    if (dir_length > 0) {
        put_word(dir_length + 3); /* size */
        put_byte(0x02);         /* dirname */
        put_bytes(dirname, dir_length);
    }

    if (!generic_format)
        write_unix_info(hdr);

    put_word(0x0000);           /* next header size */

    extend_header_size = put_ptr - extend_header_top;
    /* On level 1 header, the packed size field is contains the ext-header */
    hdr->packed_size += put_ptr - extend_header_top;

    /* put `skip size' */
    setup_put(data + I_PACKED_SIZE);
    put_longword(hdr->packed_size);

    data[I_HEADER_SIZE] = header_size;
    data[I_HEADER_CHECKSUM] = calc_sum(data + I_METHOD, header_size);

    return header_size + extend_header_size + 2;
}

static size_t
write_header_level2(data, hdr, pathname)
    LzHeader *hdr;
    char *data, *pathname;
{
    int name_length, dir_length;
    char *basename, *dirname;
    size_t header_size;
    char *extend_header_top;
    char *headercrc_ptr;
    unsigned int hcrc;

    basename = strrchr(pathname, LHA_PATHSEP);
    if (basename) {
        basename++;
        name_length = strlen(basename);
        dirname = pathname;
        dir_length = basename - dirname;
    }
    else {
        basename = pathname;
        name_length = strlen(basename);
        dirname = "";
        dir_length = 0;
    }

    setup_put(data);
    memset(data, 0, LZHEADER_STORAGE);

    put_word(0x0000);           /* header size */
    put_bytes(hdr->method, 5);
    put_longword(hdr->packed_size);
    put_longword(hdr->original_size);
    put_longword(hdr->unix_last_modified_stamp);
    put_byte(0x20);
    put_byte(hdr->header_level); /* level 2 */

    put_word(hdr->crc);

    if (generic_format)
        put_byte(0x00);
    else
        put_byte(EXTEND_UNIX);

    /* write extend header from here. */

    extend_header_top = put_ptr+2; /* +2 for the field `next header size' */

    /* write common header */
    put_word(5);
    put_byte(0x00);
    headercrc_ptr = put_ptr;
    put_word(0x0000);           /* header CRC */

    /* write filename and dirname */
    /* must have this header, even if the name_length is 0. */
    put_word(name_length + 3);  /* size */
    put_byte(0x01);             /* filename */
    put_bytes(basename, name_length);

    if (dir_length > 0) {
        put_word(dir_length + 3); /* size */
        put_byte(0x02);         /* dirname */
        put_bytes(dirname, dir_length);
    }

    if (!generic_format)
        write_unix_info(hdr);

    put_word(0x0000);           /* next header size */

    header_size = put_ptr - data;
    if ((header_size & 0xff) == 0) {
        /* cannot put zero at the first byte on level 2 header. */
        /* adjust header size. */
        put_byte(0);            /* padding */
        header_size++;
    }

    /* put header size */
    setup_put(data + I_HEADER_SIZE);
    put_word(header_size);

    /* put header CRC in extended header */
    INITIALIZE_CRC(hcrc);
    hcrc = calccrc(hcrc, data, (unsigned int) header_size);
    setup_put(headercrc_ptr);
    put_word(hcrc);

    return header_size;
}

void
write_header(fp, hdr)
    FILE           *fp;
    LzHeader       *hdr;
{
    size_t header_size;
    char data[LZHEADER_STORAGE];

    int archive_kanji_code = CODE_SJIS;
    int system_kanji_code = default_system_kanji_code;
    char *archive_delim = "\377";
    char *system_delim = "/";
    int filename_case = NONE;
    char pathname[FILENAME_LENGTH];

    if (optional_archive_kanji_code)
        archive_kanji_code = optional_archive_kanji_code;
    if (optional_system_kanji_code)
        system_kanji_code = optional_system_kanji_code;

    if (generic_format)
        filename_case = TO_UPPER;

    if (hdr->header_level == 0) {
        archive_delim = "\\";
    }

    if ((hdr->unix_mode & UNIX_FILE_SYMLINK) == UNIX_FILE_SYMLINK) {
        char *p;
        p = strchr(hdr->name, '|');
        if (p) {
            error("symlink name \"%s\" contains '|' char. change it into '_'",
                  hdr->name);
            *p = '_';
        }
        if (xsnprintf(pathname, sizeof(pathname),
                      "%s|%s", hdr->name, hdr->realname) == -1)
            error("file name is too long (%s -> %s)", hdr->name, hdr->realname);
    }
    else {
        strncpy(pathname, hdr->name, sizeof(pathname));
        pathname[sizeof(pathname)-1] = 0;
    }

    convert_filename(pathname, strlen(pathname), sizeof(pathname),
                     system_kanji_code,
                     archive_kanji_code,
                     system_delim, archive_delim, filename_case);

    switch (hdr->header_level) {
    case 0:
        header_size = write_header_level0(data, hdr, pathname);
        break;
    case 1:
        header_size = write_header_level1(data, hdr, pathname);
        break;
    case 2:
        header_size = write_header_level2(data, hdr, pathname);
        break;
    default:
        error("Unknown level header (level %d)", hdr->header_level);
        exit(1);
    }

    if (fwrite(data, header_size, 1, fp) == 0)
        fatal_error("Cannot write to temporary file");
}

#if MULTIBYTE_FILENAME

#if defined(__APPLE__)  /* Added by Hiroto Sakai */

#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFStringEncodingExt.h>

/* this is not need for Mac OS X v 10.2 later */
enum {
  kCFStringEncodingAllowLossyConversion = 1,
  kCFStringEncodingBasicDirectionLeftToRight = (1 << 1),
  kCFStringEncodingBasicDirectionRightToLeft = (1 << 2),
  kCFStringEncodingSubstituteCombinings = (1 << 3),
  kCFStringEncodingComposeCombinings = (1 << 4),
  kCFStringEncodingIgnoreCombinings = (1 << 5),
  kCFStringEncodingUseCanonical = (1 << 6),
  kCFStringEncodingUseHFSPlusCanonical = (1 << 7),
  kCFStringEncodingPrependBOM = (1 << 8),
  kCFStringEncodingDisableCorporateArea = (1 << 9),
  kCFStringEncodingASCIICompatibleConversion = (1 << 10),
};

static int
ConvertEncodingToUTF8(const char* inCStr,
                      char* outUTF8Buffer,
                      int outUTF8BufferLength,
                      unsigned long scriptEncoding,
                      unsigned long flags)
{
    unsigned long unicodeChars;
    unsigned long srcCharsUsed;
    unsigned long usedByteLen = 0;
    UniChar uniStr[512];
    unsigned long cfResult;

    cfResult = CFStringEncodingBytesToUnicode(scriptEncoding,
                                              flags,
                                              (char *)inCStr,
                                              strlen(inCStr),
                                              &srcCharsUsed,
                                              uniStr,
                                              512,
                                              &unicodeChars);
    if (cfResult == 0) {
        cfResult = CFStringEncodingUnicodeToBytes(kCFStringEncodingUTF8,
                                                  flags,
                                                  uniStr,
                                                  unicodeChars,
                                                  &srcCharsUsed,
                                                  (char*)outUTF8Buffer,
                                                  outUTF8BufferLength - 1,
                                                  &usedByteLen);
        outUTF8Buffer[usedByteLen] = '\0';
    }

    return cfResult;
}

static int
ConvertUTF8ToEncoding(const char* inUTF8Buf,
                      int inUTF8BufLength,
                      char* outCStrBuffer,
                      int outCStrBufferLength,
                      unsigned long scriptEncoding,
                      unsigned long flags)
{
    unsigned long unicodeChars;
    unsigned long srcCharsUsed;
    unsigned long usedByteLen = 0;
    UniChar uniStr[256];
    unsigned long cfResult;

    cfResult = CFStringEncodingBytesToUnicode(kCFStringEncodingUTF8,
                                              flags,
                                              (char*)inUTF8Buf,
                                              inUTF8BufLength,
                                              &srcCharsUsed,
                                              uniStr,
                                              255,
                                              &unicodeChars);
    if (cfResult == 0) {
        cfResult = CFStringEncodingUnicodeToBytes(scriptEncoding,
                                                  flags,
                                                  uniStr,
                                                  unicodeChars,
                                                  &srcCharsUsed,
                                                  (char*)outCStrBuffer,
                                                  outCStrBufferLength - 1,
                                                  &usedByteLen);
        outCStrBuffer[usedByteLen] = '\0';
    }

    return cfResult;
}

#elif HAVE_ICONV
#include <iconv.h>

static int
ConvertEncodingByIconv(const char *src, char *dst, int dstsize,
                       const char *srcEnc, const char *dstEnc)
{
    iconv_t ic;
    static char szTmpBuf[2048];
    char *src_p;
    char *dst_p;
    size_t sLen;
    size_t iLen;

    dst_p = &szTmpBuf[0];
    iLen = (size_t)sizeof(szTmpBuf)-1;
    src_p = (char *)src;
    sLen = (size_t)strlen(src);
    memset(szTmpBuf, 0, sizeof(szTmpBuf));
    memset(dst, 0, dstsize);

    ic = iconv_open(dstEnc, srcEnc);
    if (ic == (iconv_t)-1) {
        error("iconv_open() failure: %s", strerror(errno));
        return -1;
    }

    if (iconv(ic, &src_p, &sLen, &dst_p, &iLen) == (size_t)-1) {
        error("iconv() failure: %s", strerror(errno));
        iconv_close(ic);
        return -1;
    }

    strncpy(dst, szTmpBuf, dstsize);

    iconv_close(ic);

    return 0;
}
#endif /* defined(__APPLE__) */

char *
sjis_to_utf8(char *dst, const char *src, size_t dstsize)
{
#if defined(__APPLE__)
  dst[0] = '\0';
  if (ConvertEncodingToUTF8(src, dst, dstsize,
                            kCFStringEncodingDOSJapanese,
                            kCFStringEncodingUseHFSPlusCanonical) == 0)
      return dst;
#elif HAVE_ICONV
  if (ConvertEncodingByIconv(src, dst, dstsize, "SJIS", "UTF-8") != -1)
      return dst;
#else
  error("not support utf-8 conversion");
#endif

  if (dstsize < 1) return dst;
  dst[dstsize-1] = 0;
  return strncpy(dst, src, dstsize-1);
}

char *
utf8_to_sjis(char *dst, const char *src, size_t dstsize)
{
#if defined(__APPLE__)
  int srclen;

  dst[0] = '\0';
  srclen = strlen(src);
  if (ConvertUTF8ToEncoding(src, srclen, dst, dstsize,
                            kCFStringEncodingDOSJapanese,
                            kCFStringEncodingUseHFSPlusCanonical) == 0)
      return dst;
#elif HAVE_ICONV
  if (ConvertEncodingByIconv(src, dst, dstsize, "UTF-8", "SJIS") != -1)
      return dst;
#else
  error("not support utf-8 conversion");
#endif

  if (dstsize < 1) return dst;
  dst[dstsize-1] = 0;
  return strncpy(dst, src, dstsize-1);
}

/*
 * SJIS <-> EUC 変換関数
 * 「日本語情報処理」   ソフトバンク(株)
 *  より抜粋(by Koji Arai)
 */
void
euc2sjis(int *p1, int *p2)
{
    unsigned char c1 = *p1 & 0x7f;
    unsigned char c2 = *p2 & 0x7f;
    int rowoff = c1 < 0x5f ? 0x70 : 0xb0;
    int celoff = c1 % 2 ? (c2 > 0x5f ? 0x20 : 0x1f) : 0x7e;
    *p1 = ((c1 + 1) >> 1) + rowoff;
    *p2 += celoff - 0x80;
}

void
sjis2euc(int *p1, int *p2)
{
    unsigned char c1 = *p1;
    unsigned char c2 = *p2;
    int adjust = c2 < 0x9f;
    int rowoff = c1 < 0xa0 ? 0x70 : 0xb0;
    int celoff = adjust ? (c2 > 0x7f ? 0x20 : 0x1f) : 0x7e;
    *p1 = ((c1 - rowoff) << 1) - adjust;
    *p2 -= celoff;

    *p1 |= 0x80;
    *p2 |= 0x80;
}

static int
hex2int(int c)
{
    switch (c) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return c - '0';

    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        return c - 'a' + 10;

    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        return c - 'A' + 10;
    default:
        return -1;
    }
}

static int
int2hex(int c)
{
    switch (c) {
    case 0: case 1: case 2: case 3: case 4:
    case 5: case 6: case 7: case 8: case 9:
        return c + '0';

    case 10: case 11: case 12: case 13: case 14: case 15:
        return c + 'a' - 10;

    default:
        return -1;
    }
}

int
cap_to_sjis(char *dst, const char *src, size_t dstsize)
{
    int i, j;
    size_t len = strlen(src);
    int a, b;

    for (i = j = 0; i < len && i < dstsize; i++) {
        if (src[i] != ':') {
            dst[j++] = src[i];
            continue;
        }

        i++;
        a = hex2int((unsigned char)src[i]);
        b = hex2int((unsigned char)src[i+1]);

        if (a == -1 || b == -1) {
            /* leave as it */
            dst[j++] = ':';
            strncpy(dst+j, src+i, dstsize-j);
            dst[dstsize-1] = 0;
            return strlen(dst);
        }

        i++;

        dst[j++] = a * 16 + b;
    }
    dst[j] = 0;
    return j;
}

int
sjis_to_cap(char *dst, const char *src, size_t dstsize)
{
    int i, j;
    size_t len = strlen(src);
    int a, b;

    for (i = j = 0; i < len && i < dstsize; i++) {
        if (src[i] == ':') {
            strncpy(dst+j, ":3a", dstsize-j);
            dst[dstsize-1] = 0;
            j = strlen(dst);
            continue;
        }
        if (isprint(src[i])) {
            dst[j++] = src[i];
            continue;
        }

        if (j + 3 >= dstsize) {
            dst[j] = 0;
            return j;
        }

        a = int2hex((unsigned char)src[i] / 16);
        b = int2hex((unsigned char)src[i] % 16);

        dst[j++] = ':';
        dst[j++] = a;
        dst[j++] = b;
    }
    dst[j] = 0;
    return j;
}
#endif /* MULTIBYTE_FILENAME */
