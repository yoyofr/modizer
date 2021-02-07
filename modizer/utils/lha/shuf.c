/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              shuf.c -- extract static Huffman coding                     */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */
#include "lha.h"

/* ------------------------------------------------------------------------ */
#undef NP
#undef NP2

#define NP          (8 * 1024 / 64)
#define NP2         (NP * 2 - 1)
/* ------------------------------------------------------------------------ */
static unsigned int np;
int             fixed[2][16] = {
    {3, 0x01, 0x04, 0x0c, 0x18, 0x30, 0},   /* old compatible */
    {2, 0x01, 0x01, 0x03, 0x06, 0x0D, 0x1F, 0x4E, 0}    /* 8K buf */
};
/* ------------------------------------------------------------------------ */
/* lh3 */
void
decode_start_st0( /*void*/ )
{
    n_max = 286;
    maxmatch = MAXMATCH;
    init_getbits();
    init_code_cache();
    np = 1 << (LZHUFF3_DICBIT - 6);
}

/* ------------------------------------------------------------------------ */
void
encode_p_st0(j)
    unsigned short  j;
{
    unsigned short  i;

    i = j >> 6;
    putcode(pt_len[i], pt_code[i]);
    putbits(6, j & 0x3f);
}

/* ------------------------------------------------------------------------ */
static void
ready_made(method)
    int             method;
{
    int             i, j;
    unsigned int    code, weight;
    int            *tbl;

    tbl = fixed[method];
    j = *tbl++;
    weight = 1 << (16 - j);
    code = 0;
    for (i = 0; i < np; i++) {
        while (*tbl == i) {
            j++;
            tbl++;
            weight >>= 1;
        }
        pt_len[i] = j;
        pt_code[i] = code;
        code += weight;
    }
}

/* ------------------------------------------------------------------------ */
/* lh1 */
void
encode_start_fix( /*void*/ )
{
    n_max = 314;
    maxmatch = 60;
    np = 1 << (12 - 6);
    init_putbits();
    init_code_cache();
    start_c_dyn();
    ready_made(0);
}

/* ------------------------------------------------------------------------ */
static void
read_tree_c( /*void*/ )
{               /* read tree from file */
    int             i, c;

    i = 0;
    while (i < N1) {
        if (getbits(1))
            c_len[i] = getbits(LENFIELD) + 1;
        else
            c_len[i] = 0;
        if (++i == 3 && c_len[0] == 1 && c_len[1] == 1 && c_len[2] == 1) {
            c = getbits(CBIT);
            for (i = 0; i < N1; i++)
                c_len[i] = 0;
            for (i = 0; i < 4096; i++)
                c_table[i] = c;
            return;
        }
    }
    make_table(N1, c_len, 12, c_table);
}

/* ------------------------------------------------------------------------ */
static void
read_tree_p(/*void*/)
{               /* read tree from file */
    int             i, c;

    i = 0;
    while (i < NP) {
        pt_len[i] = getbits(LENFIELD);
        if (++i == 3 && pt_len[0] == 1 && pt_len[1] == 1 && pt_len[2] == 1) {
            c = getbits(LZHUFF3_DICBIT - 6);
            for (i = 0; i < NP; i++)
                pt_len[i] = 0;
            for (i = 0; i < 256; i++)
                pt_table[i] = c;
            return;
        }
    }
}

/* ------------------------------------------------------------------------ */
/* lh1 */
void
decode_start_fix(/*void*/)
{
    n_max = 314;
    maxmatch = 60;
    init_getbits();
    init_code_cache();
    np = 1 << (LZHUFF1_DICBIT - 6);
    start_c_dyn();
    ready_made(0);
    make_table(np, pt_len, 8, pt_table);
}

/* ------------------------------------------------------------------------ */
/* lh3 */
unsigned short
decode_c_st0(/*void*/)
{
    int             i, j;
    static unsigned short blocksize = 0;

    if (blocksize == 0) {   /* read block head */
        blocksize = getbits(BUFBITS);   /* read block blocksize */
        read_tree_c();
        if (getbits(1)) {
            read_tree_p();
        }
        else {
            ready_made(1);
        }
        make_table(NP, pt_len, 8, pt_table);
    }
    blocksize--;
    j = c_table[peekbits(12)];
    if (j < N1)
        fillbuf(c_len[j]);
    else {
        fillbuf(12);
        i = bitbuf;
        do {
            if ((short) i < 0)
                j = right[j];
            else
                j = left[j];
            i <<= 1;
        } while (j >= N1);
        fillbuf(c_len[j] - 12);
    }
    if (j == N1 - 1)
        j += getbits(EXTRABITS);
    return j;
}

/* ------------------------------------------------------------------------ */
/* lh1, 3 */
unsigned short
decode_p_st0(/*void*/)
{
    int             i, j;

    j = pt_table[peekbits(8)];
    if (j < np) {
        fillbuf(pt_len[j]);
    }
    else {
        fillbuf(8);
        i = bitbuf;
        do {
            if ((short) i < 0)
                j = right[j];
            else
                j = left[j];
            i <<= 1;
        } while (j >= np);
        fillbuf(pt_len[j] - 8);
    }
    return (j << 6) + getbits(6);
}
