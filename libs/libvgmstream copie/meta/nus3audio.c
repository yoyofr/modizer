#include "meta.h"
#include "../coding/coding.h"

typedef enum { IDSP, OPUS, RIFF, } nus3audio_codec;

/* .nus3audio - Namco's newest newest audio container [Super Smash Bros. Ultimate (Switch)] */
VGMSTREAM* init_vgmstream_nus3audio(STREAMFILE* sf) {
    VGMSTREAM* vgmstream = NULL;
    STREAMFILE* temp_sf = NULL;
    off_t subfile_offset = 0, name_offset = 0;
    size_t subfile_size = 0;
    nus3audio_codec codec;
    const char* fake_ext = NULL;
    int total_subsongs, target_subsong = sf->stream_index;


    /* checks */
    if (!check_extensions(sf, "nus3audio"))
        goto fail;
    if (read_u32be(0x00,sf) != 0x4E555333) /* "NUS3" */
        goto fail;
    if (read_u32le(0x04,sf) + 0x08 != get_streamfile_size(sf))
        goto fail;
    if (read_u32be(0x08,sf) != 0x41554449) /* "AUDI" */
        goto fail;


    /* parse existing chunks */
    {
        off_t offset = 0x0c;
        size_t file_size = get_streamfile_size(sf);
        uint32_t codec_id = 0;

        total_subsongs = 0;

        while (offset < file_size) {
            uint32_t chunk_id  = read_u32be(offset+0x00, sf);
            size_t chunk_size  = read_u32le(offset+0x04, sf);

            switch(chunk_id) {
                case 0x494E4458: /* "INDX": audio index */
                    total_subsongs = read_u32le(offset+0x08 + 0x00,sf);
                    if (target_subsong == 0) target_subsong = 1;
                    if (target_subsong < 0 || target_subsong > total_subsongs || total_subsongs < 1) goto fail;
                    break;

                case 0x4E4D4F46: /* "NMOF": name offsets (absolute, inside TNNM) */
                    name_offset = read_u32le(offset+0x08 + 0x04*(target_subsong-1),sf);
                    break;

                case 0x41444F46: /* "ADOF": audio offsets (absolute, inside PACK) */
                    subfile_offset = read_u32le(offset+0x08 + 0x08*(target_subsong-1) + 0x00,sf);
                    subfile_size   = read_u32le(offset+0x08 + 0x08*(target_subsong-1) + 0x04,sf);
                    break;

                case 0x544E4944: /* "TNID": tone ids? */
                case 0x544E4E4D: /* "TNNM": tone names */
                case 0x4A554E4B: /* "JUNK": padding */
                case 0x5041434B: /* "PACK": main data */
                default:
                    break;
            }

            offset += 0x08 + chunk_size;
        }

        if (total_subsongs == 0 || subfile_offset == 0 || subfile_size == 0) {
            VGM_LOG("NUS3AUDIO: subfile not found\n");
            goto fail;
        }

        codec_id = read_u32be(subfile_offset, sf);
        switch(codec_id) {
            case 0x49445350: /* "IDSP" */
                codec = IDSP;
                fake_ext = "idsp";
                break;
            case 0x4F505553: /* "OPUS" */
                codec = OPUS;
                fake_ext = "opus";
                break;
            case 0x52494646: /* "RIFF" [Gundam Versus (PS4)] */
                codec = RIFF;
                fake_ext = "wav";
                break;
            default:
                VGM_LOG("NUS3AUDIO: unknown codec %x\n", codec_id);
                goto fail;
        }
    }


    temp_sf = setup_subfile_streamfile(sf, subfile_offset, subfile_size, fake_ext);
    if (!temp_sf) goto fail;

    /* init the VGMSTREAM */
    switch(codec) {
        case IDSP:
            vgmstream = init_vgmstream_idsp_namco(temp_sf);
            if (!vgmstream) goto fail;
            break;
        case OPUS:
            vgmstream = init_vgmstream_opus_nus3(temp_sf);
            if (!vgmstream) goto fail;
            break;
        case RIFF:
            vgmstream = init_vgmstream_riff(temp_sf);
            if (!vgmstream) goto fail;
            break;
        default:
            goto fail;
    }

    vgmstream->num_streams = total_subsongs;
    if (name_offset) /* null-terminated */
        read_string(vgmstream->stream_name,STREAM_NAME_SIZE, name_offset,sf);

    close_streamfile(temp_sf);
    return vgmstream;

fail:
    close_streamfile(temp_sf);
    close_vgmstream(vgmstream);
    return NULL;
}


