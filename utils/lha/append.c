/* ------------------------------------------------------------------------ */
/* LHa for UNIX                                                             */
/*              append.c -- append to archive                               */
/*                                                                          */
/*      Modified                Nobutaka Watazaki                           */
/*                                                                          */
/*  Ver. 1.14   Source All chagned              1995.01.14  N.Watazaki      */
/* ------------------------------------------------------------------------ */
#include "lha.h"

int
encode_lzhuf(infp, outfp, size, original_size_var, packed_size_var,
         name, hdr_method)
    FILE           *infp;
    FILE           *outfp;
    size_t          size;
    size_t         *original_size_var;
    size_t         *packed_size_var;
    char           *name;
    char           *hdr_method;
{
    static int method = -1;
    unsigned int crc;
    struct interfacing interface;

    if (method < 0) {
        method = compress_method;
        if (method > 0)
            method = encode_alloc(method);
    }

    interface.method = method;

    if (interface.method > 0) {
        interface.infile = infp;
        interface.outfile = outfp;
        interface.original = size;
        start_indicator(name, size, "Freezing", 1 << dicbit);
        crc = encode(&interface);
        *packed_size_var = interface.packed;
        *original_size_var = interface.original;
    } else {
        *packed_size_var = *original_size_var =
            copyfile(infp, outfp, size, 0, &crc);
    }
    memcpy(hdr_method, "-lh -", 5);
    hdr_method[3] = interface.method + '0';

    finish_indicator2(name, "Frozen",
            (int) ((*packed_size_var * 100L) / *original_size_var));
    return crc;
}
