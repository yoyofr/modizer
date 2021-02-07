#include "meta.h"
#include "../coding/coding.h"

/* NWAV - from Chunsoft games [Fuurai no Shiren Gaiden: Onnakenshi Asuka Kenzan! (PC)] */
VGMSTREAM * init_vgmstream_nwav(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    off_t start_offset;


    /* checks */
    /* .nwav: header id (no filenames in bigfiles) */
    if ( !check_extensions(streamFile,"nwav") )
        goto fail;
    if (read_32bitBE(0x00,streamFile) != 0x4E574156) /* "NWAV" */
        goto fail;


#ifdef VGM_USE_VORBIS
    {
        ogg_vorbis_meta_info_t ovmi = {0};
        int channels;

        /* 0x04: version? */
        /* 0x08: crc? */
        ovmi.stream_size = read_32bitLE(0x0c, streamFile);
        ovmi.loop_end = read_32bitLE(0x10, streamFile); /* num_samples, actually */
        /* 0x14: sample rate */
        /* 0x18: bps? (16) */
        channels = read_8bit(0x19, streamFile);
        start_offset = read_16bitLE(0x1a, streamFile);

        ovmi.loop_flag = read_16bitLE(0x1c, streamFile) != 0; /* loop count? -1 = loops */
        /* 0x1e: always 2? */
        /* 0x20: always 1? */
        ovmi.loop_start = read_32bitLE(0x24, streamFile);
        /* 0x28: always 1? */
        /* 0x2a: always 1? */
        /* 0x2c: always null? */

        ovmi.meta_type = meta_NWAV;

        /* values are in resulting bytes */
        ovmi.loop_start = ovmi.loop_start / sizeof(int16_t) / channels;
        ovmi.loop_end = ovmi.loop_end / sizeof(int16_t) / channels;

        vgmstream = init_vgmstream_ogg_vorbis_callbacks(streamFile, NULL, start_offset, &ovmi);
    }
#else
    goto fail;
#endif

    return vgmstream;

fail:
    close_vgmstream(vgmstream);
    return NULL;
}
