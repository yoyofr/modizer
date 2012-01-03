/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              extract.c -- extrcat from archive                           */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */
#include "lha.h"

int
decode_lzhuf(infp, outfp, original_size, packed_size, name, method, read_sizep)
    FILE           *infp;
    FILE           *outfp;
    size_t          original_size;
    size_t          packed_size;
    char           *name;
    int             method;
    size_t         *read_sizep;
{
    unsigned int crc;
    struct interfacing interface;

    interface.method = method;
    interface.infile = infp;
    interface.outfile = outfp;
    interface.original = original_size;
    interface.packed = packed_size;
    interface.read_size = 0;

    switch (method) {
    case LZHUFF0_METHOD_NUM:    /* -lh0- */
        interface.dicbit = LZHUFF0_DICBIT;
        break;
    case LZHUFF1_METHOD_NUM:    /* -lh1- */
        interface.dicbit = LZHUFF1_DICBIT;
        break;
    case LZHUFF2_METHOD_NUM:    /* -lh2- */
        interface.dicbit = LZHUFF2_DICBIT;
        break;
    case LZHUFF3_METHOD_NUM:    /* -lh2- */
        interface.dicbit = LZHUFF3_DICBIT;
        break;
    case LZHUFF4_METHOD_NUM:    /* -lh4- */
        interface.dicbit = LZHUFF4_DICBIT;
        break;
    case LZHUFF5_METHOD_NUM:    /* -lh5- */
        interface.dicbit = LZHUFF5_DICBIT;
        break;
    case LZHUFF6_METHOD_NUM:    /* -lh6- */
        interface.dicbit = LZHUFF6_DICBIT;
        break;
    case LZHUFF7_METHOD_NUM:    /* -lh7- */
        interface.dicbit = LZHUFF7_DICBIT;
        break;
    case LARC_METHOD_NUM:       /* -lzs- */
        interface.dicbit = LARC_DICBIT;
        break;
    case LARC5_METHOD_NUM:      /* -lz5- */
        interface.dicbit = LARC5_DICBIT;
        break;
    case LARC4_METHOD_NUM:      /* -lz4- */
        interface.dicbit = LARC4_DICBIT;
        break;
    default:
        warning("unknown method %d", method);
        interface.dicbit = LZHUFF5_DICBIT; /* for backward compatibility */
        break;
    }

    if (interface.dicbit == 0) { /* LZHUFF0_DICBIT or LARC4_DICBIT */
        start_indicator(name,
                        original_size,
                        verify_mode ? "Testing " : "Melting ",
                        2048);
        *read_sizep = copyfile(infp, (verify_mode ? NULL : outfp),
                               original_size, 2, &crc);
    }
    else {
        start_indicator(name,
                        original_size,
                        verify_mode ? "Testing " : "Melting ",
                        1 << interface.dicbit);
        crc = decode(&interface);
        *read_sizep = interface.read_size;
    }

    finish_indicator(name, verify_mode ? "Tested  " : "Melted  ");

    return crc;
}
