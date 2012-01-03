/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              huf.c -- new static Huffman                                 */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/*  Ver. 1.14i  Support LH7 & Bug Fixed         2000.10. 6  t.okamoto       */
/* ------------------------------------------------------------------------ */
#include "lha.h"

#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#if STDC_HEADERS
#include <stdlib.h>
#else
extern char *malloc ();
#endif

/* ------------------------------------------------------------------------ */
unsigned short left[2 * NC - 1], right[2 * NC - 1];

unsigned short c_code[NC];      /* encode */
unsigned short pt_code[NPT];    /* encode */

unsigned short c_table[4096];   /* decode */
unsigned short pt_table[256];   /* decode */

unsigned short c_freq[2 * NC - 1]; /* encode */
unsigned short p_freq[2 * NP - 1]; /* encode */
unsigned short t_freq[2 * NT - 1]; /* encode */

unsigned char  c_len[NC];
unsigned char  pt_len[NPT];

static unsigned char *buf;      /* encode */
static unsigned int bufsiz;     /* encode */
static unsigned short blocksize; /* decode */
static unsigned short output_pos, output_mask; /* encode */

static int pbit;
static int np;
/* ------------------------------------------------------------------------ */
/*                              Encording                                   */
/* ------------------------------------------------------------------------ */
static void
count_t_freq(/*void*/)
{
    short           i, k, n, count;

    for (i = 0; i < NT; i++)
        t_freq[i] = 0;
    n = NC;
    while (n > 0 && c_len[n - 1] == 0)
        n--;
    i = 0;
    while (i < n) {
        k = c_len[i++];
        if (k == 0) {
            count = 1;
            while (i < n && c_len[i] == 0) {
                i++;
                count++;
            }
            if (count <= 2)
                t_freq[0] += count;
            else if (count <= 18)
                t_freq[1]++;
            else if (count == 19) {
                t_freq[0]++;
                t_freq[1]++;
            }
            else
                t_freq[2]++;
        } else
            t_freq[k + 2]++;
    }
}

/* ------------------------------------------------------------------------ */
static void
write_pt_len(n, nbit, i_special)
    short           n;
    short           nbit;
    short           i_special;
{
    short           i, k;

    while (n > 0 && pt_len[n - 1] == 0)
        n--;
    putbits(nbit, n);
    i = 0;
    while (i < n) {
        k = pt_len[i++];
        if (k <= 6)
            putbits(3, k);
        else
            /* k=7 -> 1110  k=8 -> 11110  k=9 -> 111110 ... */
            putbits(k - 3, USHRT_MAX << 1);
        if (i == i_special) {
            while (i < 6 && pt_len[i] == 0)
                i++;
            putbits(2, i - 3);
        }
    }
}

/* ------------------------------------------------------------------------ */
static void
write_c_len(/*void*/)
{
    short           i, k, n, count;

    n = NC;
    while (n > 0 && c_len[n - 1] == 0)
        n--;
    putbits(CBIT, n);
    i = 0;
    while (i < n) {
        k = c_len[i++];
        if (k == 0) {
            count = 1;
            while (i < n && c_len[i] == 0) {
                i++;
                count++;
            }
            if (count <= 2) {
                for (k = 0; k < count; k++)
                    putcode(pt_len[0], pt_code[0]);
            }
            else if (count <= 18) {
                putcode(pt_len[1], pt_code[1]);
                putbits(4, count - 3);
            }
            else if (count == 19) {
                putcode(pt_len[0], pt_code[0]);
                putcode(pt_len[1], pt_code[1]);
                putbits(4, 15);
            }
            else {
                putcode(pt_len[2], pt_code[2]);
                putbits(CBIT, count - 20);
            }
        }
        else
            putcode(pt_len[k + 2], pt_code[k + 2]);
    }
}

/* ------------------------------------------------------------------------ */
static void
encode_c(c)
    short           c;
{
    putcode(c_len[c], c_code[c]);
}

/* ------------------------------------------------------------------------ */
static void
encode_p(p)
    unsigned short  p;
{
    unsigned short  c, q;

    c = 0;
    q = p;
    while (q) {
        q >>= 1;
        c++;
    }
    putcode(pt_len[c], pt_code[c]);
    if (c > 1)
        putbits(c - 1, p);
}

/* ------------------------------------------------------------------------ */
static void
send_block( /* void */ )
{
    unsigned char   flags;
    unsigned short  i, k, root, pos, size;

    root = make_tree(NC, c_freq, c_len, c_code);
    size = c_freq[root];
    putbits(16, size);
    if (root >= NC) {
        count_t_freq();
        root = make_tree(NT, t_freq, pt_len, pt_code);
        if (root >= NT) {
            write_pt_len(NT, TBIT, 3);
        } else {
            putbits(TBIT, 0);
            putbits(TBIT, root);
        }
        write_c_len();
    } else {
        putbits(TBIT, 0);
        putbits(TBIT, 0);
        putbits(CBIT, 0);
        putbits(CBIT, root);
    }
    root = make_tree(np, p_freq, pt_len, pt_code);
    if (root >= np) {
        write_pt_len(np, pbit, -1);
    }
    else {
        putbits(pbit, 0);
        putbits(pbit, root);
    }
    pos = 0;
    for (i = 0; i < size; i++) {
        if (i % CHAR_BIT == 0)
            flags = buf[pos++];
        else
            flags <<= 1;
        if (flags & (1 << (CHAR_BIT - 1))) {
            encode_c(buf[pos++] + (1 << CHAR_BIT));
            k = buf[pos++] << CHAR_BIT;
            k += buf[pos++];
            encode_p(k);
        } else
            encode_c(buf[pos++]);
        if (unpackable)
            return;
    }
    for (i = 0; i < NC; i++)
        c_freq[i] = 0;
    for (i = 0; i < np; i++)
        p_freq[i] = 0;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
void
output_st1(c, p)
    unsigned short  c;
    unsigned short  p;
{
    static unsigned short cpos;

    output_mask >>= 1;
    if (output_mask == 0) {
        output_mask = 1 << (CHAR_BIT - 1);
        if (output_pos >= bufsiz - 3 * CHAR_BIT) {
            send_block();
            if (unpackable)
                return;
            output_pos = 0;
        }
        cpos = output_pos++;
        buf[cpos] = 0;
    }
    buf[output_pos++] = (unsigned char) c;
    c_freq[c]++;
    if (c >= (1 << CHAR_BIT)) {
        buf[cpos] |= output_mask;
        buf[output_pos++] = (unsigned char) (p >> CHAR_BIT);
        buf[output_pos++] = (unsigned char) p;
        c = 0;
        while (p) {
            p >>= 1;
            c++;
        }
        p_freq[c]++;
    }
}

/* ------------------------------------------------------------------------ */
unsigned char  *
alloc_buf( /* void */ )
{
    bufsiz = 16 * 1024 *2;  /* 65408U; */ /* t.okamoto */
    while ((buf = (unsigned char *) malloc(bufsiz)) == NULL) {
        bufsiz = (bufsiz / 10) * 9;
        if (bufsiz < 4 * 1024)
            fatal_error("Not enough memory");
    }
    return buf;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
void
encode_start_st1( /* void */ )
{
    int             i;

    switch (dicbit) {
    case LZHUFF4_DICBIT:
    case LZHUFF5_DICBIT: pbit = 4; np = LZHUFF5_DICBIT + 1; break;
    case LZHUFF6_DICBIT: pbit = 5; np = LZHUFF6_DICBIT + 1; break;
    case LZHUFF7_DICBIT: pbit = 5; np = LZHUFF7_DICBIT + 1; break;
    default:
        fatal_error("Cannot use %d bytes dictionary", 1 << dicbit);
    }

    for (i = 0; i < NC; i++)
        c_freq[i] = 0;
    for (i = 0; i < np; i++)
        p_freq[i] = 0;
    output_pos = output_mask = 0;
    init_putbits();
    init_code_cache();
    buf[0] = 0;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
void
encode_end_st1( /* void */ )
{
    if (!unpackable) {
        send_block();
        putbits(CHAR_BIT - 1, 0);   /* flush remaining bits */
    }
}

/* ------------------------------------------------------------------------ */
/*                              decoding                                    */
/* ------------------------------------------------------------------------ */
static void
read_pt_len(nn, nbit, i_special)
    short           nn;
    short           nbit;
    short           i_special;
{
    int           i, c, n;

    n = getbits(nbit);
    if (n == 0) {
        c = getbits(nbit);
        for (i = 0; i < nn; i++)
            pt_len[i] = 0;
        for (i = 0; i < 256; i++)
            pt_table[i] = c;
    }
    else {
        i = 0;
        while (i < MIN(n, NPT)) {
            c = peekbits(3);
            if (c != 7)
                fillbuf(3);
            else {
                unsigned short  mask = 1 << (16 - 4);
                while (mask & bitbuf) {
                    mask >>= 1;
                    c++;
                }
                fillbuf(c - 3);
            }

            pt_len[i++] = c;
            if (i == i_special) {
                c = getbits(2);
                while (--c >= 0 && i < NPT)
                    pt_len[i++] = 0;
            }
        }
        while (i < nn)
            pt_len[i++] = 0;
        make_table(nn, pt_len, 8, pt_table);
    }
}

/* ------------------------------------------------------------------------ */
static void
read_c_len( /* void */ )
{
    short           i, c, n;

    n = getbits(CBIT);
    if (n == 0) {
        c = getbits(CBIT);
        for (i = 0; i < NC; i++)
            c_len[i] = 0;
        for (i = 0; i < 4096; i++)
            c_table[i] = c;
    } else {
        i = 0;
        while (i < MIN(n,NC)) {
            c = pt_table[peekbits(8)];
            if (c >= NT) {
                unsigned short  mask = 1 << (16 - 9);
                do {
                    if (bitbuf & mask)
                        c = right[c];
                    else
                        c = left[c];
                    mask >>= 1;
                } while (c >= NT && (mask || c != left[c])); /* CVE-2006-4338 */
            }
            fillbuf(pt_len[c]);
            if (c <= 2) {
                if (c == 0)
                    c = 1;
                else if (c == 1)
                    c = getbits(4) + 3;
                else
                    c = getbits(CBIT) + 20;
                while (--c >= 0)
                    c_len[i++] = 0;
            }
            else
                c_len[i++] = c - 2;
        }
        while (i < NC)
            c_len[i++] = 0;
        make_table(NC, c_len, 12, c_table);
    }
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
unsigned short
decode_c_st1( /*void*/ )
{
    unsigned short  j, mask;

    if (blocksize == 0) {
        blocksize = getbits(16);
        read_pt_len(NT, TBIT, 3);
        read_c_len();
        read_pt_len(np, pbit, -1);
    }
    blocksize--;
    j = c_table[peekbits(12)];
    if (j < NC)
        fillbuf(c_len[j]);
    else {
        fillbuf(12);
        mask = 1 << (16 - 1);
        do {
            if (bitbuf & mask)
                j = right[j];
            else
                j = left[j];
            mask >>= 1;
        } while (j >= NC && (mask || j != left[j])); /* CVE-2006-4338 */
        fillbuf(c_len[j] - 12);
    }
    return j;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
unsigned short
decode_p_st1( /* void */ )
{
    unsigned short  j, mask;

    j = pt_table[peekbits(8)];
    if (j < np)
        fillbuf(pt_len[j]);
    else {
        fillbuf(8);
        mask = 1 << (16 - 1);
        do {
            if (bitbuf & mask)
                j = right[j];
            else
                j = left[j];
            mask >>= 1;
        } while (j >= np && (mask || j != left[j])); /* CVE-2006-4338 */
        fillbuf(pt_len[j] - 8);
    }
    if (j != 0)
        j = (1 << (j - 1)) + getbits(j - 1);
    return j;
}

/* ------------------------------------------------------------------------ */
/* lh4, 5, 6, 7 */
void
decode_start_st1( /* void */ )
{
    switch (dicbit) {
    case LZHUFF4_DICBIT:
    case LZHUFF5_DICBIT: pbit = 4; np = LZHUFF5_DICBIT + 1; break;
    case LZHUFF6_DICBIT: pbit = 5; np = LZHUFF6_DICBIT + 1; break;
    case LZHUFF7_DICBIT: pbit = 5; np = LZHUFF7_DICBIT + 1; break;
    default:
        fatal_error("Cannot use %d bytes dictionary", 1 << dicbit);
    }

    init_getbits();
    init_code_cache();
    blocksize = 0;
}
