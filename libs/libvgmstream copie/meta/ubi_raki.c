#include "meta.h"
#include "../coding/coding.h"


/* RAKI - Ubisoft audio format [Rayman Legends, Just Dance 2017 (multi)] */
VGMSTREAM * init_vgmstream_ubi_raki(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    off_t start_offset, offset, fmt_offset;
    size_t header_size, data_size;
    int big_endian;
    int loop_flag, channel_count, block_align, bits_per_sample;
    uint32_t platform, type;

    int32_t (*read_32bit)(off_t,STREAMFILE*) = NULL;
    int16_t (*read_16bit)(off_t,STREAMFILE*) = NULL;


    /* checks */
    /* .rak: Just Dance 2017
     * .ckd: Rayman Legends (technically .wav.ckd/rak) */
    if (!check_extensions(streamFile,"rak,ckd"))
        goto fail;

    /* some games (ex. Rayman Legends PS3) have a 32b file type before the RAKI data. However
     * offsets are absolute and expect the type exists, so it's part of the file and not an extraction defect. */
    if ((read_32bitBE(0x00,streamFile) == 0x52414B49))  /* "RAKI" */
        offset = 0x00;
    else if ((read_32bitBE(0x04,streamFile) == 0x52414B49)) /* type varies between platforms (0x09, 0x0b) so ignore */
        offset = 0x04;
    else
        goto fail;

    /* 0x04: version? (0x00, 0x07, 0x0a, etc); */
    platform = read_32bitBE(offset+0x08,streamFile); /* string */
    type     = read_32bitBE(offset+0x0c,streamFile); /* string */

    switch(platform) {
        case 0x57696920: /* "Wii " */
        case 0x43616665: /* "Cafe" */
        case 0x50533320: /* "PS3 " */
        case 0x58333630: /* "X360" */
            big_endian = 1;
            read_32bit = read_32bitBE;
            read_16bit = read_16bitBE;
            break;
        default:
            big_endian = 0;
            read_32bit = read_32bitLE;
            read_16bit = read_16bitLE;
            break;
    }

    header_size  = read_32bit(offset+0x10,streamFile);
    start_offset = read_32bit(offset+0x14,streamFile);
    /* 0x18: number of chunks */
    /* 0x1c: unk */

    /* the format has a chunk offset table, and the first one always "fmt" and points
     * to a RIFF "fmt"-style chunk (even for WiiU or PS3) */
    if (read_32bitBE(offset+0x20,streamFile) != 0x666D7420) goto fail; /* "fmt " */
    fmt_offset = read_32bit(offset+0x24,streamFile);
    //fmt_size = read_32bit(off+0x28,streamFile);

    loop_flag = 0; /* not seen */
    channel_count = read_16bit(fmt_offset+0x2,streamFile);
    block_align = read_16bit(fmt_offset+0xc,streamFile);
    bits_per_sample = read_16bit(fmt_offset+0xe,streamFile);

    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

    vgmstream->sample_rate = read_32bit(fmt_offset+0x4,streamFile);
    vgmstream->meta_type = meta_UBI_RAKI;

    /* codecs have a "data" or equivalent chunk with the size/start_offset, but always agree with this */
    data_size = get_streamfile_size(streamFile) - start_offset;

    /* parse compound codec to simplify */
    switch(((uint64_t)platform << 32) | type) {

        case 0x57696E2070636D20:    /* "Win pcm " */
        case 0x4F72626970636D20:    /* "Orbipcm " (Orbis = PS4) */
        case 0x4E78202070636D20:    /* "Nx  pcm " (Nx = Switch) */
            /* chunks: "data" */
            vgmstream->coding_type = coding_PCM16LE;
            vgmstream->layout_type = layout_interleave;
            vgmstream->interleave_block_size = 0x2;

            vgmstream->num_samples = pcm_bytes_to_samples(data_size, channel_count, bits_per_sample);
            break;

        case 0x57696E2061647063:    /* "Win adpc" */
            /* chunks: "data" */
            vgmstream->coding_type = coding_MSADPCM;
            vgmstream->layout_type = layout_none;
            vgmstream->frame_size = block_align;

            vgmstream->num_samples = msadpcm_bytes_to_samples(data_size, vgmstream->frame_size, channel_count);

            if (!msadpcm_check_coefs(streamFile, fmt_offset + 0x14))
                goto fail;
            break;

        case 0x5769692061647063:    /* "Wii adpc" */
        case 0x4361666561647063:    /* "Cafeadpc" (Cafe = WiiU) */
            /* chunks: "datS" (stereo), "datL" (mono or full interleave), "datR" (full interleave), "data" equivalents */
            vgmstream->coding_type = coding_NGC_DSP;
            vgmstream->layout_type = layout_interleave;
            vgmstream->interleave_block_size = 0x8;

            /* we need to know if the file uses "datL" and is full-interleave */
            if (channel_count > 1) {
                off_t chunk_offset = offset+ 0x20 + 0xc; /* after "fmt" */
                while (chunk_offset < header_size) {
                    if (read_32bitBE(chunk_offset,streamFile) == 0x6461744C) { /* "datL" found */
                        size_t chunk_size = read_32bit(chunk_offset+0x8,streamFile);
                        data_size = chunk_size * channel_count; /* to avoid counting the "datR" chunk */
                        vgmstream->interleave_block_size = (4+4) + chunk_size; /* don't forget to skip the "datR"+size chunk */
                        break;
                    }
                    chunk_offset += 0xc;
                }

                /* not found? probably "datS" (regular stereo interleave) */
            }

            {
                /* get coef offsets; could check "dspL" and "dspR" chunks after "fmt " better but whatevs (only "dspL" if mono) */
                off_t dsp_coefs = read_32bitBE(offset+0x30,streamFile); /* after "dspL"; spacing is consistent but could vary */
                dsp_read_coefs(vgmstream,streamFile, dsp_coefs+0x1c, 0x60, big_endian);
                /* dsp_coefs + 0x00-0x1c: ? (special coefs or adpcm history?) */
            }

            vgmstream->num_samples = dsp_bytes_to_samples(data_size, channel_count);
            break;

        case 0x4354520061647063:    /* "CTR\0adpc" (Citrus = 3DS) */
            /* chunks: "dspL" (CWAV-L header), "dspR" (CWAV-R header), "cwav" ("data" equivalent) */
            vgmstream->coding_type = coding_NGC_DSP;
            vgmstream->layout_type = layout_interleave;
            vgmstream->interleave_block_size = 0x8;

            /* reading could be improved but should work with some luck since most values are semi-fixed */
            if (channel_count > 1) {
                /* find "dspL" pointing to "CWAV" header and read coefs (separate from data at start_offset) */
                off_t chunk_offset = offset+ 0x20 + 0xc; /* after "fmt" */
                while (chunk_offset < header_size) {
                    if (read_32bitBE(chunk_offset,streamFile) == 0x6473704C) { /* "dspL" found */
                        off_t cwav_offset = read_32bit(chunk_offset+0x4,streamFile);
                        size_t cwav_size  = read_32bit(chunk_offset+0x8,streamFile);

                        dsp_read_coefs(vgmstream,streamFile, cwav_offset + 0x7c, cwav_size, big_endian);
                        break;
                    }
                    chunk_offset += 0xc;
                }
            }
            else {
                /* CWAV at start (a full CWAV, unlike the above) */
                dsp_read_coefs(vgmstream,streamFile, start_offset + 0x7c, 0x00, big_endian);
                start_offset += 0xE0;
                data_size = get_streamfile_size(streamFile) - start_offset;
            }

            vgmstream->num_samples = dsp_bytes_to_samples(data_size, channel_count);
            break;


#ifdef VGM_USE_MPEG
        case 0x505333206D703320: {  /* "PS3 mp3 " */
            /* chunks: "MARK" (optional seek table), "STRG" (optional description), "Msf " ("data" equivalent) */
            vgmstream->codec_data = init_mpeg(streamFile, start_offset, &vgmstream->coding_type, vgmstream->channels);
            if (!vgmstream->codec_data) goto fail;
            vgmstream->layout_type = layout_none;

            vgmstream->num_samples = mpeg_bytes_to_samples(data_size, vgmstream->codec_data);
            break;
        }
#endif

#ifdef VGM_USE_FFMPEG
        case 0x58333630786D6132: {  /* "X360xma2" */
            /* chunks: "seek" (XMA2 seek table), "data" */
            uint8_t buf[100];
            int bytes, block_count;

            block_count = data_size / block_align + (data_size % block_align ? 1 : 0);

            bytes = ffmpeg_make_riff_xma2(buf, 100, vgmstream->num_samples, data_size, vgmstream->channels, vgmstream->sample_rate, block_count, block_align);
            vgmstream->codec_data = init_ffmpeg_header_offset(streamFile, buf,bytes, start_offset,data_size);
            if ( !vgmstream->codec_data ) goto fail;
            vgmstream->coding_type = coding_FFmpeg;
            vgmstream->layout_type = layout_none;

            vgmstream->num_samples = read_32bit(fmt_offset+0x18,streamFile);

            xma_fix_raw_samples(vgmstream, streamFile, start_offset,data_size, 0, 0,0); /* should apply to num_samples? */
            break;
        }
#endif

#ifdef VGM_USE_ATRAC9
        case 0x5649544161743920: {  /* "VITAat9 " */
            /* chunks: "fact" (equivalent to a RIFF "fact", num_samples + skip_samples), "data" */
            atrac9_config cfg = {0};

            cfg.channels = vgmstream->channels;
            cfg.config_data = read_32bitBE(fmt_offset+0x2c,streamFile);
            cfg.encoder_delay = read_32bit(fmt_offset+0x3c,streamFile);

            vgmstream->codec_data = init_atrac9(&cfg);
            if (!vgmstream->codec_data) goto fail;
            vgmstream->coding_type = coding_ATRAC9;
            vgmstream->layout_type = layout_none;

            /* could get the "fact" offset but seems it always follows "fmt " */
            vgmstream->num_samples = read_32bit(fmt_offset+0x34,streamFile);
            break;
        }
#endif

#ifdef VGM_USE_FFMPEG
        case 0x4E7820204E782020: {  /* "Nx  Nx  " */
            /* chunks: "MARK" (optional seek table), "STRG" (optional description) */
            size_t skip, opus_size;

            /* a standard Switch Opus header */
            skip = read_32bitLE(start_offset + 0x1c, streamFile);
            opus_size = read_32bitLE(start_offset + 0x10, streamFile) + 0x08;
            start_offset += opus_size;
            data_size -= opus_size;

            vgmstream->codec_data = init_ffmpeg_switch_opus(streamFile, start_offset,data_size, vgmstream->channels, skip, vgmstream->sample_rate);
            if (!vgmstream->codec_data) goto fail;
            vgmstream->coding_type = coding_FFmpeg;
            vgmstream->layout_type = layout_none;

            {
                off_t chunk_offset = offset + 0x20 + 0xc; /* after "fmt" */
                while (chunk_offset < header_size) {
                    if (read_32bitBE(chunk_offset,streamFile) == 0x4164496E) { /*"AdIn" additional info */
                        off_t adin_offset = read_32bitLE(chunk_offset+0x04,streamFile);
                        vgmstream->num_samples = read_32bitLE(adin_offset,streamFile);
                        break;
                    }
                    chunk_offset += 0xc;
                }
            }

            break;
        }
#endif

        default:
            VGM_LOG("RAKI: unknown platform %x and type %x\n", platform, type);
            goto fail;
    }


    if (!vgmstream_open_stream(vgmstream,streamFile,start_offset))
        goto fail;
    return vgmstream;

fail:
    close_vgmstream(vgmstream);
    return NULL;
}
