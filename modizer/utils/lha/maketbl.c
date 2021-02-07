/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              maketbl.c -- makes decoding table                           */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */
#include "lha.h"

void
make_table(nchar, bitlen, tablebits, table)
    short           nchar;
    unsigned char   bitlen[];
    short           tablebits;
    unsigned short  table[];
{
    unsigned short  count[17];  /* count of bitlen */
    unsigned short  weight[17]; /* 0x10000ul >> bitlen */
    unsigned short  start[17];  /* first code of bitlen */
    unsigned short  total;
    unsigned int    i, l;
    int             j, k, m, n, avail;
    unsigned short *p;

    avail = nchar;

    /* initialize */
    for (i = 1; i <= 16; i++) {
        count[i] = 0;
        weight[i] = 1 << (16 - i);
    }

    /* count */
    for (i = 0; i < nchar; i++) {
        if (bitlen[i] > 16) {
            /* CVE-2006-4335 */
            error("Bad table (case a)");
            exit(1);
        }
        else
            count[bitlen[i]]++;
    }

    /* calculate first code */
    total = 0;
    for (i = 1; i <= 16; i++) {
        start[i] = total;
        total += weight[i] * count[i];
    }
    if ((total & 0xffff) != 0 || tablebits > 16) { /* 16 for weight below */
        error("make_table(): Bad table (case b)");
        exit(1);
    }

    /* shift data for make table. */
    m = 16 - tablebits;
    for (i = 1; i <= tablebits; i++) {
        start[i] >>= m;
        weight[i] >>= m;
    }

    /* initialize */
    j = start[tablebits + 1] >> m;
    k = MIN(1 << tablebits, 4096);
    if (j != 0)
        for (i = j; i < k; i++)
            table[i] = 0;

    /* create table and tree */
    for (j = 0; j < nchar; j++) {
        k = bitlen[j];
        if (k == 0)
            continue;
        l = start[k] + weight[k];
        if (k <= tablebits) {
            /* code in table */
            l = MIN(l, 4096);
            for (i = start[k]; i < l; i++)
                table[i] = j;
        }
        else {
            /* code not in table */
            i = start[k];
            if ((i >> m) > 4096) {
                /* CVE-2006-4337 */
                error("Bad table (case c)");
                exit(1);
            }
            p = &table[i >> m];
            i <<= tablebits;
            n = k - tablebits;
            /* make tree (n length) */
            while (--n >= 0) {
                if (*p == 0) {
                    right[avail] = left[avail] = 0;
                    *p = avail++;
                }
                if (i & 0x8000)
                    p = &right[*p];
                else
                    p = &left[*p];
                i <<= 1;
            }
            *p = j;
        }
        start[k] = l;
    }
}
