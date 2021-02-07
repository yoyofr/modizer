/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              maketree.c -- make Huffman tree                             */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */
#include "lha.h"

static void
make_code(nchar, bitlen, code, leaf_num)
    int            nchar;
    unsigned char  *bitlen;
    unsigned short *code;       /* table */
    unsigned short *leaf_num;
{
    unsigned short  weight[17]; /* 0x10000ul >> bitlen */
    unsigned short  start[17];  /* start code */
    unsigned short  total;
    int i;
    int c;

    total = 0;
    for (i = 1; i <= 16; i++) {
        start[i] = total;
        weight[i] = 1 << (16 - i);
        total += weight[i] * leaf_num[i];
    }
    for (c = 0; c < nchar; c++) {
        i = bitlen[c];
        code[c] = start[i];
        start[i] += weight[i];
    }
}

static void
count_leaf(node, nchar, leaf_num, depth) /* call with node = root */
    int node;
    int nchar;
    unsigned short leaf_num[];
    int depth;
{
    if (node < nchar)
        leaf_num[depth < 16 ? depth : 16]++;
    else {
        count_leaf(left[node], nchar, leaf_num, depth + 1);
        count_leaf(right[node], nchar, leaf_num, depth + 1);
    }
}

static void
make_len(nchar, bitlen, sort, leaf_num)
    int nchar;
    unsigned char *bitlen;
    unsigned short *sort;       /* sorted characters */
    unsigned short *leaf_num;
{
    int i, k;
    unsigned int cum;

    cum = 0;
    for (i = 16; i > 0; i--) {
        cum += leaf_num[i] << (16 - i);
    }
#if (UINT_MAX != 0xffff)
    cum &= 0xffff;
#endif
    /* adjust len */
    if (cum) {
        leaf_num[16] -= cum; /* always leaf_num[16] > cum */
        do {
            for (i = 15; i > 0; i--) {
                if (leaf_num[i]) {
                    leaf_num[i]--;
                    leaf_num[i + 1] += 2;
                    break;
                }
            }
        } while (--cum);
    }
    /* make len */
    for (i = 16; i > 0; i--) {
        k = leaf_num[i];
        while (k > 0) {
            bitlen[*sort++] = i;
            k--;
        }
    }
}

/* priority queue; send i-th entry down heap */
static void
downheap(i, heap, heapsize, freq)
    int i;
    short *heap;
    size_t heapsize;
    unsigned short *freq;
{
    short j, k;

    k = heap[i];
    while ((j = 2 * i) <= heapsize) {
        if (j < heapsize && freq[heap[j]] > freq[heap[j + 1]])
            j++;
        if (freq[k] <= freq[heap[j]])
            break;
        heap[i] = heap[j];
        i = j;
    }
    heap[i] = k;
}

/* make tree, calculate bitlen[], return root */
short
make_tree(nchar, freq, bitlen, code)
    int             nchar;
    unsigned short  *freq;
    unsigned char   *bitlen;
    unsigned short  *code;
{
    short i, j, avail, root;
    unsigned short *sort;

    short heap[NC + 1];       /* NC >= nchar */
    size_t heapsize;

    avail = nchar;
    heapsize = 0;
    heap[1] = 0;
    for (i = 0; i < nchar; i++) {
        bitlen[i] = 0;
        if (freq[i])
            heap[++heapsize] = i;
    }
    if (heapsize < 2) {
        code[heap[1]] = 0;
        return heap[1];
    }

    /* make priority queue */
    for (i = heapsize / 2; i >= 1; i--)
        downheap(i, heap, heapsize, freq);

    /* make huffman tree */
    sort = code;
    do {            /* while queue has at least two entries */
        i = heap[1];    /* take out least-freq entry */
        if (i < nchar)
            *sort++ = i;
        heap[1] = heap[heapsize--];
        downheap(1, heap, heapsize, freq);
        j = heap[1];    /* next least-freq entry */
        if (j < nchar)
            *sort++ = j;
        root = avail++;    /* generate new node */
        freq[root] = freq[i] + freq[j];
        heap[1] = root;
        downheap(1, heap, heapsize, freq);    /* put into queue */
        left[root] = i;
        right[root] = j;
    } while (heapsize > 1);

    {
        unsigned short leaf_num[17];

        /* make leaf_num */
        memset(leaf_num, 0, sizeof(leaf_num));
        count_leaf(root, nchar, leaf_num, 0);

        /* make bitlen */
        make_len(nchar, bitlen, code, leaf_num);

        /* make code table */
        make_code(nchar, bitlen, code, leaf_num);
    }

    return root;
}
