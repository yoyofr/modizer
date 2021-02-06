#include "meta.h"
#include "../coding/coding.h"

typedef enum { PCM16, MSADPCM, DSP_HEAD, DSP_BODY, AT9, MSF } kwb_codec;

typedef struct {
    int big_endian;
    int total_subsongs;
    int target_subsong;
    int found;
    kwb_codec codec;

    int channels;
    int sample_rate;
    int32_t num_samples;
    int32_t loop_start;
    int32_t loop_end;
    int loop_flag;
    int block_size;

    off_t stream_offset;
    size_t stream_size;

    off_t dsp_offset;
    //off_t name_offset;
} kwb_header;

static int parse_kwb(kwb_header* kwb, STREAMFILE* sf_h, STREAMFILE* sf_b);
static int parse_xws(kwb_header* kwb, STREAMFILE* sf);
static VGMSTREAM* init_vgmstream_koei_wavebank(kwb_header* kwb, STREAMFILE* sf_h, STREAMFILE* sf_b);


/* KWB - WaveBank from Koei games */
VGMSTREAM* init_vgmstream_kwb(STREAMFILE* sf) {
    VGMSTREAM* vgmstream = NULL;
    STREAMFILE *sf_h = NULL, *sf_b = NULL;
    kwb_header kwb = {0};
    int target_subsong = sf->stream_index;


    /* checks */
    /* .wbd+wbh: common [Bladestorm Nightmare (PC)]
     * .wb2+wh2: newer [Nights of Azure 2 (PC)]
     * .sed: mixed header+data [Dissidia NT (PC)] */
    if (!check_extensions(sf, "wbd,wb2,sed"))
        goto fail;


    /* open companion header */
    if (check_extensions(sf, "wbd")) {
        sf_h = open_streamfile_by_ext(sf, "wbh");
        sf_b = sf;
    }
    else if (check_extensions(sf, "wb2")) {
        sf_h = open_streamfile_by_ext(sf, "wh2");
        sf_b = sf;
    }
    else if (check_extensions(sf, "sed")) {
        sf_h = sf;
        sf_b = sf;
    }
    else {
        goto fail;
    }

    if (sf_h == NULL || sf_b == NULL)
        goto fail;

    if (target_subsong == 0) target_subsong = 1;
    kwb.target_subsong = target_subsong;

    if (!parse_kwb(&kwb, sf_h, sf_b))
        goto fail;

    vgmstream = init_vgmstream_koei_wavebank(&kwb, sf_h, sf_b);
    if (!vgmstream) goto fail;

    if (sf_h != sf) close_streamfile(sf_h);
    return vgmstream;

fail:
    if (sf_h != sf) close_streamfile(sf_h);
    close_vgmstream(vgmstream);
    return NULL;
}

/* XWS - WaveStream? from Koei games */
VGMSTREAM* init_vgmstream_xws(STREAMFILE* sf) {
    VGMSTREAM* vgmstream = NULL;
    kwb_header kwb = {0};
    int target_subsong = sf->stream_index;


    /* checks */
    if (!check_extensions(sf, "xws"))
        goto fail;

    if (target_subsong == 0) target_subsong = 1;
    kwb.target_subsong = target_subsong;

    if (!parse_xws(&kwb, sf))
        goto fail;

    vgmstream = init_vgmstream_koei_wavebank(&kwb, sf, sf);
    if (!vgmstream) goto fail;

    return vgmstream;
fail:
    close_vgmstream(vgmstream);
    return NULL;
}


static VGMSTREAM* init_vgmstream_koei_wavebank(kwb_header* kwb, STREAMFILE* sf_h, STREAMFILE* sf_b) {
    VGMSTREAM* vgmstream = NULL;
    uint32_t (*read_u32)(off_t,STREAMFILE*) = NULL;
    int32_t  (*read_s32)(off_t,STREAMFILE*) = NULL;


    read_u32 = kwb->big_endian ? read_u32be : read_u32le;
    read_s32 = kwb->big_endian ? read_s32be : read_s32le;

    /* container */
    if (kwb->codec == MSF) {
        if (kwb->stream_offset == 0) {
            vgmstream = init_vgmstream_silence(0,0,0); /* dummy, whatevs */
            if (!vgmstream) goto fail;
        }
        else {
            STREAMFILE* temp_sf = NULL;

            kwb->stream_size = read_u32(kwb->stream_offset + 0x0c, sf_h) + 0x40;

            temp_sf = setup_subfile_streamfile(sf_h, kwb->stream_offset, kwb->stream_size, "msf");
            if (!temp_sf) goto fail;

            vgmstream = init_vgmstream_msf(temp_sf);
            close_streamfile(temp_sf);
            if (!vgmstream) goto fail;
        }

        vgmstream->num_streams = kwb->total_subsongs;
        return vgmstream;
    }


    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(kwb->channels, kwb->loop_flag);
    if (!vgmstream) goto fail;

    vgmstream->meta_type = meta_KWB;
    vgmstream->sample_rate = kwb->sample_rate;
    vgmstream->num_samples = kwb->num_samples;
    vgmstream->stream_size = kwb->stream_size;
    vgmstream->num_streams = kwb->total_subsongs;

    switch(kwb->codec) {
        case PCM16: /* PCM */
            vgmstream->coding_type = coding_PCM16LE;
            vgmstream->layout_type = layout_interleave;
            vgmstream->interleave_block_size = 0x02;
            break;

        case MSADPCM:
            vgmstream->coding_type = coding_MSADPCM;
            vgmstream->layout_type = layout_none;
            vgmstream->frame_size = kwb->block_size;
            break;

        case DSP_HEAD:
        case DSP_BODY:
            if (kwb->channels > 1) goto fail;
            vgmstream->coding_type = coding_NGC_DSP; /* subinterleave? */
            vgmstream->layout_type = layout_interleave;
            vgmstream->interleave_block_size = 0x08;
            if (kwb->codec == DSP_HEAD) {
                dsp_read_coefs(vgmstream, sf_h, kwb->dsp_offset + 0x1c, 0x60, kwb->big_endian);
                dsp_read_hist (vgmstream, sf_h, kwb->dsp_offset + 0x40, 0x60, kwb->big_endian);
            }
            else {
                /* typical DSP header + data */
                vgmstream->num_samples = read_s32(kwb->stream_offset + 0x00, sf_b);
                dsp_read_coefs(vgmstream, sf_b, kwb->stream_offset + 0x1c, 0x60, kwb->big_endian);
                dsp_read_hist (vgmstream, sf_b, kwb->stream_offset + 0x40, 0x60, kwb->big_endian);
                kwb->stream_offset += 0x60;
            }

            break;

#ifdef VGM_USE_ATRAC9
        case AT9: {
            atrac9_config cfg = {0};

            {
                size_t extra_size = read_u32le(kwb->stream_offset + 0x00, sf_b);
                uint32_t config_data = read_u32be(kwb->stream_offset + 0x04, sf_b);
                /* 0x0c: encoder delay? */
                /* 0x0e: encoder padding? */
                /* 0x10: samples per frame */
                /* 0x12: frame size */

                cfg.channels = vgmstream->channels;
                cfg.config_data = config_data;

                kwb->stream_offset += extra_size;
                kwb->stream_size -= extra_size;
            }

            vgmstream->codec_data = init_atrac9(&cfg);
            if (!vgmstream->codec_data) goto fail;
            vgmstream->coding_type = coding_ATRAC9;
            vgmstream->layout_type = layout_none;

            //TODO: check encoder delay
            vgmstream->num_samples = atrac9_bytes_to_samples_cfg(kwb->stream_size, cfg.config_data);
            break;
        }
#endif
        default:
            goto fail;
    }


    if (!vgmstream_open_stream(vgmstream, sf_b, kwb->stream_offset))
        goto fail;
    return vgmstream;

fail:
    close_vgmstream(vgmstream);
    return NULL;
}

/* ************************************************************************* */

static int parse_type_kwb2(kwb_header* kwb, off_t offset, off_t body_offset, STREAMFILE* sf_h) {
    int i, j, sounds;

    /* 00: KWB2/KWBN id */
    /* 04: always 0x3200? */
    sounds = read_u16le(offset + 0x06, sf_h);
    /* 08: ? */
    /* 0c: 1.0? */
    /* 10: null or 1 */
    /* 14: offset to HDDB table (from type), can be null */

    //;VGM_LOG("KWB2: sounds %i, o=%lx\n", sounds, offset);

    /* offset table to entries */
    for (i = 0; i < sounds; i++) {
        off_t sound_offset = read_u32le(offset + 0x18 + i*0x04, sf_h);
        int subsounds, subsound_start, subsound_size;
        uint16_t version;

        //;VGM_LOG("KWB2: entry %i, o=%lx, so=%lx\n", i, offset + 0x18 + i*0x04, sound_offset);

        if (sound_offset == 0) /* common... */
            continue;
        sound_offset += offset;

        /* sound entry */
        version = read_u16le(sound_offset + 0x00, sf_h);
        /* 00: version? */
        /* 02: 0x2b or 0x32 */
        subsounds = read_u8(sound_offset + 0x03, sf_h);
        /* 03: subsounds? */
        /* others: unknown or null */

        /* unsure but seems to work, maybe upper byte only */
        if (version < 0xc000) {
            subsound_start = 0x2c;
            subsound_size  = 0x48;
        }
        else {
            subsound_start = read_u16le(sound_offset + 0x2c, sf_h);
            subsound_size  = read_u16le(sound_offset + 0x2e, sf_h);
        }

        subsound_start = sound_offset + subsound_start;

        for (j = 0; j < subsounds; j++) {
            off_t subsound_offset;
            uint8_t codec;

            kwb->total_subsongs++;
            if (kwb->total_subsongs != kwb->target_subsong)
                continue;
            kwb->found = 1;

            subsound_offset = subsound_start + j*subsound_size;

            kwb->sample_rate    = read_u16le(subsound_offset + 0x00, sf_h);
            codec               = read_u8   (subsound_offset + 0x02, sf_h);
            kwb->channels       = read_u8   (subsound_offset + 0x03, sf_h);
            kwb->block_size     = read_u16le(subsound_offset + 0x04, sf_h);
            /* 0x06: samples per frame in MSADPCM? */
            /* 0x08: some id? (not always) */
            kwb->num_samples    = read_u32le(subsound_offset + 0x0c, sf_h);
            kwb->stream_offset  = read_u32le(subsound_offset + 0x10, sf_h);
            kwb->stream_size    = read_u32le(subsound_offset + 0x14, sf_h);
            /* when size > 0x48 */
            /* 0x48: subsound entry size */
            /* rest: reserved per codec? (usually null) */
            
            kwb->stream_offset += body_offset;

            switch(codec) {
                case 0x00:
                    kwb->codec = PCM16;
                    break;
                case 0x10:
                    kwb->codec = MSADPCM;
                    break;
                case 0x90:
                    kwb->codec = DSP_HEAD;
                    kwb->dsp_offset = subsound_offset + 0x4c;
                    break;
                default:
                    VGM_LOG("KWB2: unknown codec\n");
                    goto fail;
            }
        }
    }

    //TODO: read names
    /* HDDB table (optional and not too common)
    00 HDDB id
    04 1?
    08: 20? start?
    0c: 14? start?
    10: size
    14: name table start
    20: name offsets?
    then some subtable
    then name table (null terminated and one after other)
    */

    return 1;
fail:
    return 0;
}

static int parse_type_k4hd(kwb_header* kwb, off_t offset, off_t body_offset, STREAMFILE* sf_h) {
    off_t ppva_offset, header_offset;
    int entries, current_subsongs, relative_subsong;
    size_t entry_size;


    /* a format mimicking PSVita's hd4+bd4 format */
    /* 00: K4HD id */
    /* 04: chunk size */
    /* 08: ? */
    /* 0c: ? */
    /* 10: PPPG offset ('program'? cues?) */
    /* 14: PPTN offset ('tone'? sounds?) */
    /* 18: PPVA offset ('VAG'? waves) */
    ppva_offset = read_u16le(offset + 0x18, sf_h);
    ppva_offset += offset;

    /* PPVA table: */
    if (read_u32be(ppva_offset + 0x00, sf_h) != 0x50505641) /* "PPVA" */
        goto fail;

    entry_size = read_u32le(ppva_offset + 0x08, sf_h);
    /* 0x0c: -1? */
    /* 0x10: 0? */
    entries = read_u32le(ppva_offset + 0x14, sf_h) + 1;
    /* 0x18: -1? */
    /* 0x1c: -1? */

    if (entry_size != 0x1c) {
        VGM_LOG("K4HD: unknown entry size\n");
        goto fail;
    }

    current_subsongs = kwb->total_subsongs;
    kwb->total_subsongs += entries;
    if (kwb->target_subsong - 1 < current_subsongs || kwb->target_subsong > kwb->total_subsongs)
        return 1;
    kwb->found = 1;

    relative_subsong = kwb->target_subsong - current_subsongs;
    header_offset = ppva_offset + 0x20 + (relative_subsong-1) * entry_size;

    kwb->stream_offset  = read_u32le(header_offset + 0x00, sf_h);
    kwb->sample_rate    = read_u32le(header_offset + 0x04, sf_h);
    kwb->stream_size    = read_u32le(header_offset + 0x08, sf_h);
    /* 0x0c: -1? loop? */
    if (read_u32le(header_offset + 0x10, sf_h) != 2) { /* codec? */
        VGM_LOG("K4HD: unknown codec\n");
        goto fail;
    }
    /* 0x14: loop start? */
    /* 0x18: loop end? */

    kwb->codec = AT9;
    kwb->channels = 1; /* always, devs use dual subsongs to fake stereo (like as hd3+bd3) */

    kwb->stream_offset += body_offset;

    return 1;
fail:
    return 0;
}

static int parse_type_sdsd(kwb_header* kwb, off_t offset, off_t body_offset, STREAMFILE* sf_h) {
    /* has Vers, Head, Prog, Smpl sections (like Sony VABs)
    unknown codec, blocked with some common start, variable sized */
    return 0;
}

static int parse_type_sdwi(kwb_header* kwb, off_t offset, off_t body_offset, STREAMFILE* sf_h) {
    off_t smpl_offset, header_offset;
    int entries, current_subsongs, relative_subsong;
    size_t entry_size;


    /* variation of SDsd */
    /* 00: SDWiVers */
    /* 08: chunk size */
    /* 0c: null */
    /* 10: SDsdHead */
    /* 18: chunk size */
    /* 1c: WBH_ size */
    /* 20: WBD_ size */
    /* 24: SDsdProg offset ('program'? cues?) */
    /* 28: SDsdSmpl offset ('samples'? waves?) */
    /* rest: ? */
    smpl_offset = read_u32be(offset + 0x28, sf_h);
    smpl_offset += offset;

    /* Smpl table: */
    if (read_u32be(smpl_offset + 0x00, sf_h) != 0x53447364 &&   /* "SDsd" */
        read_u32be(smpl_offset + 0x04, sf_h) != 0x536D706C)     /* "Smpl" */
        goto fail;

    /* 0x08: ? */
    entries = read_u32le(smpl_offset + 0x0c, sf_h); /* LE! */
    entry_size = 0x40;

    current_subsongs = kwb->total_subsongs;
    kwb->total_subsongs += entries;
    if (kwb->target_subsong - 1 < current_subsongs || kwb->target_subsong > kwb->total_subsongs)
        return 1;
    kwb->found = 1;

    relative_subsong = kwb->target_subsong - current_subsongs;
    header_offset = smpl_offset + 0x10 + (relative_subsong-1) * entry_size;

    /* 00: "SS" + ID (0..N) */
    kwb->stream_offset  = read_u32be(header_offset + 0x04, sf_h);
    /* 08: flag? */
    /* 0c: ? + channels? */
    kwb->sample_rate    = read_u32be(header_offset + 0x10, sf_h);
    /* 14: bitrate */
    /* 18: codec? + bps */
    /* 1c: null? */
    /* 20: null? */
    kwb->stream_size    = read_u32be(header_offset + 0x24, sf_h);
    /* 28: full stream size (with padding) */
    /* 2c: related to samples? */
    /* 30: ID */
    /* 34-38: null */

    kwb->codec = DSP_BODY;
    kwb->channels = 1;

    kwb->stream_offset += body_offset;

    return 1;
fail:
    return 0;
}


static int parse_kwb(kwb_header* kwb, STREAMFILE* sf_h, STREAMFILE* sf_b) {
    off_t head_offset, body_offset, start;
    uint32_t type;
    uint32_t (*read_u32)(off_t,STREAMFILE*) = NULL;


    if (read_u32be(0x00, sf_h) == 0x57484431) { /* "WHD1" */
        /* container of fused .wbh+wbd */
        /* 0x04: fixed value? */
        kwb->big_endian = read_u8(0x08, sf_h) == 0xFF;
        /* 0x0a: version? */

        read_u32 = kwb->big_endian ? read_u32be : read_u32le;

        start = read_u32(0x0c, sf_h);
        /* 0x10: file size */
        /* 0x14: subfiles? */
        /* 0x18: subfiles? */
        /* 0x1c: null */
        /* 0x20: some size? */
        /* 0x24: some size? */

        head_offset = read_u32(start + 0x00, sf_h);
        body_offset = read_u32(start + 0x04, sf_h);
        /* 0x10: head size */
        /* 0x14: body size */
    }
    else {
        /* dual file */
        head_offset = 0x00;
        body_offset = 0x00;

        kwb->big_endian = guess_endianness32bit(head_offset + 0x08, sf_h);

        read_u32 = kwb->big_endian ? read_u32be : read_u32le;
    }

    if (read_u32(head_offset + 0x00, sf_h) != 0x5742485F ||   /* "WBH_" */
        read_u32(head_offset + 0x04, sf_h) != 0x30303030)     /* "0000" */
        goto fail;
    if (read_u32(body_offset + 0x00, sf_b) != 0x5742445F ||   /* "WBD_" */
        read_u32(body_offset + 0x04, sf_b) != 0x30303030)     /* "0000" */
        goto fail;
    /* 0x08: head/body size */

    head_offset += 0x0c;
    body_offset += 0x0c;

    /* format has multiple bank subtypes that are quite different from each other */
    type = read_u32be(head_offset + 0x00, sf_h);
    switch(type) {
        case 0x4B574232: /* "KWB2" [Bladestorm Nightmare (PC), Dissidia NT (PC)] */
        case 0x4B57424E: /* "KWBN" [Fire Emblem Warriors (Switch)] */
            if (!parse_type_kwb2(kwb, head_offset, body_offset, sf_h))
                goto fail;
            break;

        case 0x4B344844: /* "K4HD" [Dissidia NT (PS4), (Vita) */
            if (!parse_type_k4hd(kwb, head_offset, body_offset, sf_h))
                goto fail;
            break;

        case 0x53447364: /* "SDsd" (PS3? leftover files) */
            if (!parse_type_sdsd(kwb, head_offset, body_offset, sf_h))
                goto fail;
            break;

        case 0x53445769: /* "SDWi" [Fatal Frame 5 (WiiU)] */
            if (!parse_type_sdwi(kwb, head_offset, body_offset, sf_h))
                goto fail;
            break;

        default:
            goto fail;
    }

    if (!kwb->found)
        goto fail;

    return 1;
fail:
    return 0;
}

static int parse_type_msfbank(kwb_header* kwb, off_t offset, STREAMFILE* sf) {
    /* this is just like XWSF, abridged: */
    int entries, current_subsongs, relative_subsong;
    off_t header_offset;
    
    entries = read_u32be(offset + 0x14, sf);

    current_subsongs = kwb->total_subsongs;
    kwb->total_subsongs += entries;
    if (kwb->target_subsong - 1 < current_subsongs || kwb->target_subsong > kwb->total_subsongs)
        return 1;
    kwb->found = 1;

    relative_subsong = kwb->target_subsong - current_subsongs;
    header_offset = offset + 0x30 + (relative_subsong-1) * 0x04;

    /* just a dumb table pointing to MSF, entries can be dummy */
    kwb->stream_offset = read_u32be(header_offset, sf);
    kwb->codec = MSF;

    kwb->stream_offset += offset;

    return 1;
//fail:
//    return 0;
}

static int parse_type_xwsfile(kwb_header* kwb, off_t offset, STREAMFILE* sf) {
    off_t table1_offset, table2_offset;
    int i, chunks, chunks2;
    uint32_t (*read_u32)(off_t,STREAMFILE*) = NULL;


    if (!(is_id32be(offset + 0x00, sf, "XWSF") && is_id32be(offset + 0x04, sf, "ILE\0")) &&
        !(is_id32be(offset + 0x00, sf, "tdpa") && is_id32be(offset + 0x04, sf, "ck\0\0")))
        goto fail;

    kwb->big_endian = read_u8(offset + 0x08, sf) == 0xFF;
    /* 0x0a: version? (0100: NG2/NG3 PS3, 0101: DoA LR PC) */

    read_u32 = kwb->big_endian ? read_u32be : read_u32le;

    /* 0x0c: tables start  */
    /* 0x10: file size */
    chunks  = read_u32(offset + 0x14, sf);
    chunks2 = read_u32(offset + 0x18, sf);
    /* 0x1c: null */
    if (chunks != chunks2)
        goto fail;

    table1_offset = read_u32(offset + 0x20, sf); /* offsets */
    table2_offset = read_u32(offset + 0x24, sf); /* sizes */
    /* 0x28: null */
    /* 0x2c: null */


    i = 0;
    while (i < chunks) {
        uint32_t entry_type, head_offset, body_offset, head_size;
        //;VGM_LOG("XWS: entry %i/%i\n", i, chunks);

        /* NG2/NG3 PS3 have table1+2, DoA LR PC removes table2 and includes body offset in entries */
        if (table2_offset) {
            head_offset = read_u32(offset + table1_offset + i * 0x04 + 0x00, sf);
            head_size   = read_u32(offset + table2_offset + i * 0x04 + 0x00, sf);
            body_offset = head_offset;
            i += 1;

            /* sometimes has file end offset as entry with no size*/
            if (!head_size)
                continue;
        }
        else {
            head_offset = read_u32(offset + table1_offset + i * 0x04 + 0x00, sf);
            body_offset = read_u32(offset + table1_offset + i * 0x04 + 0x04, sf);
            i += 2;
        }

        if (!head_offset) /* just in case */
            continue;


        head_offset += offset;
        body_offset += offset;
        entry_type = read_u32be(head_offset + 0x00, sf);
        //;VGM_LOG("XWS: head=%x, body=%x\n", head_offset, body_offset);

        if (entry_type == get_id32be("XWSF")) { /* + "ILE\0" */
            if (!parse_type_xwsfile(kwb, head_offset, sf))
                goto fail;
        }
        else if (entry_type == get_id32be("CUEB") || entry_type < 0x100) {
            ; /* CUE-like info (may start with 0 or a low number instead) */
        }
        else if (entry_type == get_id32be("MSFB")) { /* + "ANK\0" */
            if (!parse_type_msfbank(kwb, head_offset, sf))
                goto fail;
        }
        else if (entry_type == get_id32be("KWB2")) {
            if (!parse_type_kwb2(kwb, head_offset, body_offset, sf))
                goto fail;
        }
        else {
            VGM_LOG("XWS: unknown type %x at head=%x, body=%x\n", entry_type, head_offset, body_offset);
            goto fail;
        }
    }

    return 1;
fail:
    return 0;
}


static int parse_xws(kwb_header* kwb, STREAMFILE* sf) {

    /* Format is similar to WHD1 with some annoyances of its own. Variations:
     * - XWSFILE w/ N chunks: CUE offsets + 1 MSFBANK offset
     *   [Ninja Gaiden Sigma 2 (PS3), Ninja Gaiden 3 Razor's Edge (PS3)]
     * - XWSFILE w/ 2*N chunks: KWB2 offset + data offset * N (ex. 3 pairs = 6 chunks)
     *   [Dead or Alive 5 Last Round (PC)]
     * - tdpack: same but points to N XWSFILE
     *   [Ninja Gaiden 3 Razor's Edge (PS3)]
     *
     * Needs to call sub-parts multiple times to fill total subsongs when parsing xwsfile.
     */
    if (!parse_type_xwsfile(kwb, 0x00, sf))
        goto fail;

    if (!kwb->found)
        goto fail;

    return 1;
fail:
    return 0;
}
