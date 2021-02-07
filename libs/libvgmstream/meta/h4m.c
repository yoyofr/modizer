#include "meta.h"
#include "../layout/layout.h"
#include "../coding/coding.h"

/* H4M - from Hudson HVQM4 videos [Resident Evil 0 (GC), Tales of Symphonia (GC)]
 * (info from hcs/Nisto's h4m_audio_decode) */
VGMSTREAM * init_vgmstream_h4m(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    off_t start_offset;
    int loop_flag, channel_count;
    int format, extra_tracks, sample_rate;
    int total_subsongs, target_subsong = streamFile->stream_index;


    /* checks */
    if (!check_extensions(streamFile, "h4m"))
        goto fail;

    if (read_32bitBE(0x00,streamFile) != 0x4856514D &&  /* "HVQM" */
        read_32bitBE(0x04,streamFile) != 0x3420312E)    /* "4 1." */
        goto fail;
    if (read_32bitBE(0x08,streamFile) != 0x33000000 &&  /* "3\0\0\0" */
        read_32bitBE(0x08,streamFile) != 0x35000000)    /* "5\0\0\0" */
        goto fail;

    /* header */
    start_offset = read_32bitBE(0x10, streamFile); /* header_size */
    if (start_offset != 0x44) /* known size */
        goto fail;
    if (read_32bitBE(0x14, streamFile) != get_streamfile_size(streamFile) - start_offset) /* body_size */
        goto fail;
    if (read_32bitBE(0x18, streamFile) == 0) /* blocks */
        goto fail;
    /* 0x1c: video_frames */
    if (read_32bitBE(0x20, streamFile) == 0) /* audio_frames */
        goto fail;
    /* 0x24: frame interval */
    /* 0x28: max_video_frame_size */
    /* 0x2c: unk2C (0) */
    if (read_32bitBE(0x30, streamFile) == 0) /* max_audio_frame_size */
        goto fail;
    /* 0x34: hres */
    /* 0x36: vres */
    /* 0x38: h_srate */
    /* 0x39: v_srate */
    /* 0x3a: unk3A (0 or 0x12) */
    /* 0x3b: unk3B (0) */
    channel_count = read_8bit(0x3c,streamFile);
    if (read_8bit(0x3d,streamFile) != 16) /* bitdepth */
        goto fail; //todo Pikmin (GC) is using some kind of variable blocks
    format = (uint8_t)read_8bit(0x3e,streamFile); /* flags? */
    extra_tracks = read_8bit(0x3f,streamFile);
    sample_rate = read_32bitBE(0x40,streamFile);

    loop_flag  = 0;

    /* tracks for languages [Pokemon Channel], or sometimes used to fake multichannel [Tales of Symphonia] */
    total_subsongs = extra_tracks + 1;
    if (target_subsong == 0) target_subsong = 1;
    if (target_subsong < 0 || target_subsong > total_subsongs || total_subsongs < 1) goto fail;


    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count, loop_flag);
    if (!vgmstream) goto fail;

    vgmstream->sample_rate = sample_rate;
    vgmstream->num_streams = total_subsongs;
    vgmstream->stream_size = get_streamfile_size(streamFile) / total_subsongs; /* approx... */
    vgmstream->codec_config = format; /* for blocks */
    vgmstream->meta_type = meta_H4M;
    vgmstream->layout_type = layout_blocked_h4m;

    switch(format & 0x7F) {
        case 0x00:
            vgmstream->coding_type = coding_H4M_IMA;
            break;

        /* no games known to use these, h4m_audio_decode may decode them */
        case 0x01: /* Uncompressed PCM */
        case 0x04: /* 8-bit (A)DPCM */
        default:
            VGM_LOG("H4M: unknown codec %x\n", format);
            goto fail;
    }

    if (!vgmstream_open_stream(vgmstream,streamFile,start_offset))
        goto fail;

    /* calc num_samples manually */
    {
        vgmstream->stream_index = target_subsong; /* extra setup for H4M */
        vgmstream->full_block_size = 0; /* extra setup for H4M */
        vgmstream->next_block_offset = start_offset;
        do {
            block_update(vgmstream->next_block_offset,vgmstream);
            vgmstream->num_samples += vgmstream->current_block_samples;
        }
        while (vgmstream->next_block_offset < get_streamfile_size(streamFile));
        vgmstream->full_block_size = 0; /* extra cleanup for H4M */
        block_update(start_offset, vgmstream);
    }

    return vgmstream;

fail:
    close_vgmstream(vgmstream);
    return NULL;
}
