/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              slide.c -- sliding dictionary with percolating update       */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14d  Exchanging a search algorithm  1997.01.11    T.Okamoto      */
/* ------------------------------------------------------------------------ */

#if 0
#define DEBUG 1
#endif

#include "lha.h"

#ifdef DEBUG
FILE *fout = NULL;
static int noslide = 1;
#endif

/* variables for hash */
struct hash {
    unsigned int pos;
    int too_flag;               /* if 1, matching candidate is too many */
} *hash;
static unsigned int *prev;      /* previous posiion associated with hash */

/* hash function: it represents 3 letters from `pos' on `text' */
#define INIT_HASH(pos) \
        ((( (text[(pos)] << 5) \
           ^ text[(pos) + 1]  ) << 5) \
           ^ text[(pos) + 2]         ) & (unsigned)(HSHSIZ - 1);
#define NEXT_HASH(hash,pos) \
        (((hash) << 5) \
           ^ text[(pos) + 2]         ) & (unsigned)(HSHSIZ - 1);

static struct encode_option encode_define[2] = {
#if defined(__STDC__) || defined(AIX)
    /* lh1 */
    {(void (*) ()) output_dyn,
     (void (*) ()) encode_start_fix,
     (void (*) ()) encode_end_dyn},
    /* lh4, 5, 6, 7 */
    {(void (*) ()) output_st1,
     (void (*) ()) encode_start_st1,
     (void (*) ()) encode_end_st1}
#else
    /* lh1 */
    {(int (*) ()) output_dyn,
     (int (*) ()) encode_start_fix,
     (int (*) ()) encode_end_dyn},
    /* lh4, 5, 6, 7 */
    {(int (*) ()) output_st1,
     (int (*) ()) encode_start_st1,
     (int (*) ()) encode_end_st1}
#endif
};

static struct decode_option decode_define[] = {
    /* lh1 */
    {decode_c_dyn, decode_p_st0, decode_start_fix},
    /* lh2 */
    {decode_c_dyn, decode_p_dyn, decode_start_dyn},
    /* lh3 */
    {decode_c_st0, decode_p_st0, decode_start_st0},
    /* lh4 */
    {decode_c_st1, decode_p_st1, decode_start_st1},
    /* lh5 */
    {decode_c_st1, decode_p_st1, decode_start_st1},
    /* lh6 */
    {decode_c_st1, decode_p_st1, decode_start_st1},
    /* lh7 */
    {decode_c_st1, decode_p_st1, decode_start_st1},
    /* lzs */
    {decode_c_lzs, decode_p_lzs, decode_start_lzs},
    /* lz5 */
    {decode_c_lz5, decode_p_lz5, decode_start_lz5}
};

static struct encode_option encode_set;
static struct decode_option decode_set;

#define TXTSIZ (MAX_DICSIZ * 2L + MAXMATCH)
#define HSHSIZ (((unsigned long)1) <<15)
#define NIL 0
#define LIMIT 0x100             /* limit of hash chain */

static unsigned int txtsiz;
static unsigned long dicsiz;
static unsigned int remainder;

struct matchdata {
    int len;
    unsigned int off;
};

int
encode_alloc(method)
    int method;
{
    switch (method) {
    case LZHUFF1_METHOD_NUM:
        encode_set = encode_define[0];
        maxmatch = 60;
        dicbit = LZHUFF1_DICBIT;    /* 12 bits  Changed N.Watazaki */
        break;
    case LZHUFF5_METHOD_NUM:
        encode_set = encode_define[1];
        maxmatch = MAXMATCH;
        dicbit = LZHUFF5_DICBIT;    /* 13 bits */
        break;
    case LZHUFF6_METHOD_NUM:
        encode_set = encode_define[1];
        maxmatch = MAXMATCH;
        dicbit = LZHUFF6_DICBIT;    /* 15 bits */
        break;
    case LZHUFF7_METHOD_NUM:
        encode_set = encode_define[1];
        maxmatch = MAXMATCH;
        dicbit = LZHUFF7_DICBIT;    /* 16 bits */
        break;
    default:
        error("unknown method %d", method);
        exit(1);
    }

    dicsiz = (((unsigned long)1) << dicbit);
    txtsiz = dicsiz*2+maxmatch;

    if (hash) return method;

    alloc_buf();

    hash = (struct hash*)xmalloc(HSHSIZ * sizeof(struct hash));
    prev = (unsigned int*)xmalloc(MAX_DICSIZ * sizeof(unsigned int));
    text = (unsigned char*)xmalloc(TXTSIZ);

    return method;
}

static void
init_slide()
{
    unsigned int i;

    for (i = 0; i < HSHSIZ; i++) {
        hash[i].pos = NIL;
        hash[i].too_flag = 0;
    }
}

/* update dictionary */
static void
update_dict(pos, crc)
    unsigned int *pos;
    unsigned int *crc;
{
    unsigned int i, j;
    long n;

    memmove(&text[0], &text[dicsiz], txtsiz - dicsiz);

    n = fread_crc(crc, &text[txtsiz - dicsiz], dicsiz, infile);

    remainder += n;

    *pos -= dicsiz;
    for (i = 0; i < HSHSIZ; i++) {
        j = hash[i].pos;
        hash[i].pos = (j > dicsiz) ? j - dicsiz : NIL;
        hash[i].too_flag = 0;
    }
    for (i = 0; i < dicsiz; i++) {
        j = prev[i];
        prev[i] = (j > dicsiz) ? j - dicsiz : NIL;
    }
}

/* associate position with token */
static void
insert_hash(token, pos)
    unsigned int token;
    unsigned int pos;
{
    prev[pos & (dicsiz - 1)] = hash[token].pos; /* chain the previous pos. */
    hash[token].pos = pos;
}

static void
search_dict_1(token, pos, off, max, m)
    unsigned int token;
    unsigned int pos;
    unsigned int off;
    unsigned int max;           /* max. length of matching string */
    struct matchdata *m;
{
    unsigned int chain = 0;
    unsigned int scan_pos = hash[token].pos;
    int scan_beg = scan_pos - off;
    int scan_end = pos - dicsiz;
    unsigned int len;

    while (scan_beg > scan_end) {
        chain++;

        if (text[scan_beg + m->len] == text[pos + m->len]) {
            {
                /* collate token */
                unsigned char *a = &text[scan_beg];
                unsigned char *b = &text[pos];

                for (len = 0; len < max && *a++ == *b++; len++);
            }

            if (len > m->len) {
                m->off = pos - scan_beg;
                m->len = len;
                if (m->len == max)
                    break;

#ifdef DEBUG
                if (noslide) {
                    if (pos - m->off < dicsiz) {
                        printf("matchpos=%u scan_pos=%u dicsiz=%u\n",
                               pos - m->off, scan_pos, dicsiz);
                    }
                }
#endif
            }
        }
        scan_pos = prev[scan_pos & (dicsiz - 1)];
        scan_beg = scan_pos - off;
    }

    if (chain >= LIMIT)
        hash[token].too_flag = 1;
}

/* search the longest token matching to current token */
static void
search_dict(token, pos, min, m)
    unsigned int token;         /* search token */
    unsigned int pos;           /* position of token */
    int min;                    /* min. length of matching string */
    struct matchdata *m;
{
    unsigned int off, tok, max;

    if (min < THRESHOLD - 1) min = THRESHOLD - 1;

    max = maxmatch;
    m->off = 0;
    m->len = min;

    off = 0;
    for (tok = token; hash[tok].too_flag && off < maxmatch - THRESHOLD; ) {
        /* If matching position is too many, The search key is
           changed into following token from `off' (for speed). */
        ++off;
        tok = NEXT_HASH(tok, pos+off);
    }
    if (off == maxmatch - THRESHOLD) {
        off = 0;
        tok = token;
    }

    search_dict_1(tok, pos, off, max, m);

    if (off > 0 && m->len < off + 3)
        /* re-search */
        search_dict_1(token, pos, 0, off+2, m);

    if (m->len > remainder) m->len = remainder;
}

/* slide dictionary */
static void
next_token(token, pos, crc)
    unsigned int *token;
    unsigned int *pos;
    unsigned int *crc;
{
    remainder--;
    if (++*pos >= txtsiz - maxmatch) {
        update_dict(pos, crc);
#ifdef DEBUG
        noslide = 0;
#endif
    }
    *token = NEXT_HASH(*token, *pos);
}

unsigned int
encode(interface)
    struct interfacing *interface;
{
    unsigned int token, pos, crc;
    size_t count;
    struct matchdata match, last;

#ifdef DEBUG
    if (!fout)
        fout = xfopen("en", "wt");
    fprintf(fout, "[filename: %s]\n", reading_filename);
#endif
    infile = interface->infile;
    outfile = interface->outfile;
    origsize = interface->original;
    compsize = count = 0L;
    unpackable = 0;

    INITIALIZE_CRC(crc);

    init_slide();

    encode_set.encode_start();
    memset(text, ' ', TXTSIZ);

    remainder = fread_crc(&crc, &text[dicsiz], txtsiz-dicsiz, infile);

    match.len = THRESHOLD - 1;
    match.off = 0;
    if (match.len > remainder) match.len = remainder;

    pos = dicsiz;
    token = INIT_HASH(pos);
    insert_hash(token, pos);     /* associate token and pos */

    while (remainder > 0 && ! unpackable) {
        last = match;

        next_token(&token, &pos, &crc);
        search_dict(token, pos, last.len-1, &match);
        insert_hash(token, pos);

        if (match.len > last.len || last.len < THRESHOLD) {
            /* output a letter */
            encode_set.output(text[pos - 1], 0);
#ifdef DEBUG
            fprintf(fout, "%u C %02X\n", count, text[pos-1]);
#endif
            count++;
        } else {
            /* output length and offset */
            encode_set.output(last.len + (256 - THRESHOLD),
                              (last.off-1) & (dicsiz-1) );

#ifdef DEBUG
            {
                int i;
                unsigned char *ptr;
                unsigned int offset = (last.off & (dicsiz-1));

                fprintf(fout, "%u M <%u %u> ",
                        count, last.len, count - offset);

                ptr = &text[pos-1 - offset];
                for (i=0; i < last.len; i++)
                    fprintf(fout, "%02X ", ptr[i]);
                fprintf(fout, "\n");
            }
#endif
            count += last.len;

            --last.len;
            while (--last.len > 0) {
                next_token(&token, &pos, &crc);
                insert_hash(token, pos);
            }
            next_token(&token, &pos, &crc);
            search_dict(token, pos, THRESHOLD - 1, &match);
            insert_hash(token, pos);
        }
    }
    encode_set.encode_end();

    interface->packed = compsize;
    interface->original = count;

    return crc;
}

unsigned int
decode(interface)
    struct interfacing *interface;
{
    unsigned int i, c;
    unsigned int dicsiz1, adjust;
    unsigned char *dtext;
    unsigned int crc;

#ifdef DEBUG
    if (!fout)
        fout = xfopen("de", "wt");
    fprintf(fout, "[filename: %s]\n", writing_filename);
#endif

    infile = interface->infile;
    outfile = interface->outfile;
    dicbit = interface->dicbit;
    origsize = interface->original;
    compsize = interface->packed;
    decode_set = decode_define[interface->method - 1];

    INITIALIZE_CRC(crc);
    dicsiz = 1L << dicbit;
    dtext = (unsigned char *)xmalloc(dicsiz);

    if (extract_broken_archive)

        /* LHa for UNIX (autoconf) had a fatal bug since version
           1.14i-ac20030713 (slide.c revision 1.20).

           This bug is possible to make a broken archive, proper LHA
           cannot extract it (probably it report CRC error).

           If the option "--extract-broken-archive" specified, extract
           the broken archive made by old LHa for UNIX. */
        memset(dtext, 0, dicsiz);
    else
        memset(dtext, ' ', dicsiz);
    decode_set.decode_start();
    dicsiz1 = dicsiz - 1;
    adjust = 256 - THRESHOLD;
    if (interface->method == LARC_METHOD_NUM)
        adjust = 256 - 2;

    decode_count = 0;
    loc = 0;
    while (decode_count < origsize) {
        c = decode_set.decode_c();
        if (c < 256) {
#ifdef DEBUG
          fprintf(fout, "%u C %02X\n", decode_count, c);
#endif
            dtext[loc++] = c;
            if (loc == dicsiz) {
                fwrite_crc(&crc, dtext, dicsiz, outfile);
                loc = 0;
            }
            decode_count++;
        }
        else {
            struct matchdata match;
            unsigned int matchpos;

            match.len = c - adjust;
            match.off = decode_set.decode_p() + 1;
            matchpos = (loc - match.off) & dicsiz1;
#ifdef DEBUG
            fprintf(fout, "%u M <%u %u> ",
                    decode_count, match.len, decode_count-match.off);
#endif
            decode_count += match.len;
            for (i = 0; i < match.len; i++) {
                c = dtext[(matchpos + i) & dicsiz1];
#ifdef DEBUG
                fprintf(fout, "%02X ", c & 0xff);
#endif
                dtext[loc++] = c;
                if (loc == dicsiz) {
                    fwrite_crc(&crc, dtext, dicsiz, outfile);
                    loc = 0;
                }
            }
#ifdef DEBUG
            fprintf(fout, "\n");
#endif
        }
    }
    if (loc != 0) {
        fwrite_crc(&crc, dtext, loc, outfile);
    }

    free(dtext);

    /* usually read size is interface->packed */
    interface->read_size = interface->packed - compsize;

    return crc;
}
