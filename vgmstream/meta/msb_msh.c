#include "meta.h"
#include "../coding/coding.h"

/* MSB+MSH - SCEE MultiStream flat bank [namCollection: Ace Combat 2 (PS2) sfx, EyeToy Play (PS2)] */
VGMSTREAM* init_vgmstream_msb_msh(STREAMFILE* sf) {
    VGMSTREAM* vgmstream = NULL;
    STREAMFILE* sh = NULL;
    off_t start_offset, header_offset = 0;
    size_t stream_size;
    int loop_flag, channels, sample_rate;
    int total_subsongs, target_subsong = sf->stream_index;


    /* checks */
    if (!check_extensions(sf, "msb"))
        goto fail;

    sh = open_streamfile_by_ext(sf, "msh");
    if (!sh) goto fail;

    if (read_u32le(0x00,sh) != get_streamfile_size(sh))
        goto fail;
    /* 0x04: unknown */

    /* parse entries */
    {
        int i;
        int entries = read_s32le(0x08,sh);

        total_subsongs = 0;
        if (target_subsong == 0) target_subsong = 1;

        for (i = 0; i < entries; i++) {
            if (read_u32le(0x0c + 0x10*i, sh) == 0) /* size 0 = empty entry */
                continue;

            total_subsongs++;
            if (total_subsongs == target_subsong && !header_offset) {
                header_offset = 0x0c + 0x10*i;
            }
        }

        if (!header_offset) goto fail;
        if (target_subsong < 0 || target_subsong > total_subsongs || total_subsongs < 1) goto fail;
    }


    loop_flag = 0;
    channels = 1;

    stream_size  = read_u32le(header_offset+0x00, sh);
    if (read_u32le(header_offset+0x04, sh) != 0) /* stereo flag? */
        goto fail;
    start_offset = read_u32le(header_offset+0x08, sh);
    sample_rate  = read_u32le(header_offset+0x0c, sh); /* Ace Combat 2 seems to set wrong values but probably their bug */


    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channels, loop_flag);
    if (!vgmstream) goto fail;

    vgmstream->meta_type = meta_MSB_MSH;
    vgmstream->sample_rate = sample_rate;
    vgmstream->num_samples = ps_bytes_to_samples(stream_size, channels);

    vgmstream->num_streams = total_subsongs;
    vgmstream->stream_size = stream_size;

    vgmstream->coding_type = coding_PSX;
    vgmstream->layout_type = layout_interleave;
    vgmstream->interleave_block_size = 0x10;

    if (!vgmstream_open_stream(vgmstream, sf, start_offset))
        goto fail;
    close_streamfile(sh);
    return vgmstream;

fail:
    close_streamfile(sh);
    close_vgmstream(vgmstream);
    return NULL;
}
