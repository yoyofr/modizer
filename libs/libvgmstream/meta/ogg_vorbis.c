#ifdef VGM_USE_VORBIS
#include <stdio.h>
#include <string.h>
#include "meta.h"
#include "../coding/coding.h"
#include "ogg_vorbis_streamfile.h"


static void um3_ogg_decryption_callback(void* ptr, size_t size, size_t nmemb, void* datasource) {
    uint8_t *ptr8 = ptr;
    size_t bytes_read = size * nmemb;
    ogg_vorbis_io *io = datasource;
    int i;

    /* first 0x800 bytes are xor'd */
    if (io->offset < 0x800) {
        int num_crypt = 0x800 - io->offset;
        if (num_crypt > bytes_read)
            num_crypt = bytes_read;

        for (i = 0; i < num_crypt; i++)
            ptr8[i] ^= 0xff;
    }
}

static void kovs_ogg_decryption_callback(void* ptr, size_t size, size_t nmemb, void* datasource) {
    uint8_t *ptr8 = ptr;
    size_t bytes_read = size * nmemb;
    ogg_vorbis_io *io = datasource;
    int i;

    /* first 0x100 bytes are xor'd */
    if (io->offset < 0x100) {
        int max_offset = io->offset + bytes_read;
        if (max_offset > 0x100)
            max_offset = 0x100;

        for (i = io->offset; i < max_offset; i++) {
            ptr8[i-io->offset] ^= i;
        }
    }
}

static void psychic_ogg_decryption_callback(void* ptr, size_t size, size_t nmemb, void* datasource) {
    static const uint8_t key[6] = {
            0x23,0x31,0x20,0x2e,0x2e,0x28
    };
    uint8_t *ptr8 = ptr;
    size_t bytes_read = size * nmemb;
    ogg_vorbis_io *io = datasource;
    int i;

    //todo incorrect, picked value changes (fixed order for all files), or key is bigger
    /* bytes add key that changes every 0x64 bytes */
    for (i = 0; i < bytes_read; i++) {
        int pos = (io->offset + i) / 0x64;
        ptr8[i] += key[pos % sizeof(key)];
    }
}

static void rpgmvo_ogg_decryption_callback(void* ptr, size_t size, size_t nmemb, void* datasource) {
    static const uint8_t header[16] = { /* OggS, packet type, granule, stream id(empty) */
            0x4F,0x67,0x67,0x53,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };
    uint8_t *ptr8 = ptr;
    size_t bytes_read = size*nmemb;
    ogg_vorbis_io *io = datasource;
    int i;

    /* first 0x10 are xor'd, but header can be easily reconstructed
     * (key is also in (game)/www/data/System.json "encryptionKey") */
    for (i = 0; i < bytes_read; i++) {
        if (io->offset+i < 0x10) {
            ptr8[i] = header[(io->offset + i) % 16];

            /* last two bytes are the stream id, get from next OggS */
            if (io->offset+i == 0x0e)
                ptr8[i] = read_8bit(0x58, io->streamfile);
            if (io->offset+i == 0x0f)
                ptr8[i] = read_8bit(0x59, io->streamfile);
        }
    }
}


static const uint32_t xiph_mappings[] = {
        0,
        mapping_MONO,
        mapping_STEREO,
        mapping_2POINT1_xiph,
        mapping_QUAD,
        mapping_5POINT0_xiph,
        mapping_5POINT1,
        mapping_7POINT0,
        mapping_7POINT1,
};


/* Ogg Vorbis,  may contain loop comments */
VGMSTREAM* init_vgmstream_ogg_vorbis(STREAMFILE* sf) {
    VGMSTREAM* vgmstream = NULL;
    STREAMFILE* temp_sf = NULL;
    ogg_vorbis_io_config_data cfg = {0};
    ogg_vorbis_meta_info_t ovmi = {0};
    off_t start_offset = 0;

    int is_ogg = 0;
    int is_um3 = 0;
    int is_kovs = 0;
    int is_sngw = 0;
    int is_isd = 0;
    int is_rpgmvo = 0;
    int is_eno = 0;
    int is_gwm = 0;
    int is_mus = 0;
    int is_lse = 0;
    int is_bgm = 0;


    /* check extension */
    /* .ogg: standard/various, .logg: renamed for plugins
     * .adx: KID games [Remember11 (PC)]
     * .rof: The Rhythm of Fighters (Mobile)
     * .acm: Planescape Torment Enhanced Edition (PC)
     * .sod: Zone 4 (PC)
     * .aif/laif/aif-Loop: Psychonauts (PC) raw extractions (named) */
    if (check_extensions(sf,"ogg,logg,adx,rof,acm,sod,aif,laif,aif-Loop")) {
        is_ogg = 1;
    } else if (check_extensions(sf,"um3")) {
        is_um3 = 1;
    } else if (check_extensions(sf,"kvs,kovs")) {
        /* .kvs: Atelier Sophie (PC), kovs: header id only? */
        is_kovs = 1;
    } else if (check_extensions(sf,"sngw")) { /* .sngw: Capcom [Devil May Cry 4 SE (PC), Biohazard 6 (PC)] */
        is_sngw = 1;
    } else if (check_extensions(sf,"isd")) { /* .isd: Inti Creates PC games */
        is_isd = 1;
    } else if (check_extensions(sf,"rpgmvo")) { /* .rpgmvo: RPG Maker MV games (PC) */
        is_rpgmvo = 1;
    } else if (check_extensions(sf,"eno")) { /* .eno: Metronomicon (PC) */
        is_eno = 1;
    } else if (check_extensions(sf,"gwm")) { /* .gwm: Adagio: Cloudburst (PC) */
        is_gwm = 1;
    } else if (check_extensions(sf,"mus")) { /* .mus: Redux - Dark Matters (PC) */
        is_mus = 1;
    } else if (check_extensions(sf,"lse")) { /* .lse: Labyrinth of Refrain: Coven of Dusk (PC) */
        is_lse = 1;
    } else if (check_extensions(sf,"bgm")) { /* .bgm: Fortissimo (PC) */
        is_bgm = 1;
    } else {
        goto fail;
    }

    if (is_ogg) {
        if (read_32bitBE(0x00,sf) == 0x2c444430) { /* Psychic Software [Darkwind: War on Wheels (PC)] */
            ovmi.decryption_callback = psychic_ogg_decryption_callback;
        }
        else if (read_32bitBE(0x00,sf) == 0x4C325344) { /* "L2SD" instead of "OggS" [Lineage II Chronicle 4 (PC)] */
            cfg.is_header_swap = 1;
            cfg.is_encrypted = 1;
        }
        else if (read_32bitBE(0x00,sf) == 0x048686C5) { /* "OggS" XOR'ed + bitswapped [Ys VIII (PC)] */
            cfg.key[0] = 0xF0;
            cfg.key_len = 1;
            cfg.is_nibble_swap = 1;
            cfg.is_encrypted = 1;

        }
        else if (read_32bitBE(0x00,sf) == 0x00000000 && /* null instead of "OggS" [Yuppie Psycho (PC)] */
                 read_32bitBE(0x3a,sf) == 0x4F676753) { /* "OggS" in next page */
            cfg.is_header_swap = 1;
            cfg.is_encrypted = 1;
        }
        else if (read_32bitBE(0x00,sf) != 0x4F676753 && /* random(?) swap instead of "OggS" [Tobi Tsukihime (PC)] */
                 read_32bitBE(0x3a,sf) == 0x4F676753) { /* "OggS" in next page */
            cfg.is_header_swap = 1;
            cfg.is_encrypted = 1;
        }
        else if (read_32bitBE(0x00,sf) == 0x4F676753) { /* "OggS" (standard) */
            ;
        }
        else {
            goto fail; /* unknown/not Ogg Vorbis (ex. Wwise) */
        }
    }

    if (is_um3) { /* ["Ultramarine3" (???)] */
        if (read_32bitBE(0x00,sf) != 0x4f676753) { /* "OggS" (optionally encrypted) */
            ovmi.decryption_callback = um3_ogg_decryption_callback;
        }
    }

    if (is_kovs) { /* Koei Tecmo PC games */
        if (read_32bitBE(0x00,sf) != 0x4b4f5653) { /* "KOVS" */
            goto fail;
        }
        ovmi.loop_start = read_32bitLE(0x08,sf);
        ovmi.loop_flag = (ovmi.loop_start != 0);
        ovmi.decryption_callback = kovs_ogg_decryption_callback;
        ovmi.meta_type = meta_OGG_KOVS;

        start_offset = 0x20;
    }

    if (is_sngw) { /* [Capcom's MT Framework PC games] */
        if (read_32bitBE(0x00,sf) != 0x4f676753) { /* "OggS" (optionally encrypted) */
            cfg.key_len = read_streamfile(cfg.key, 0x00, 0x04, sf);
            cfg.is_header_swap = 1;
            cfg.is_nibble_swap = 1;
            cfg.is_encrypted = 1;
        }

        ovmi.disable_reordering = 1; /* must be an MT Framework thing */
    }

    if (is_isd) { /* Inti Creates PC games */
        const char *isl_name = NULL;

        /* check various encrypted "OggS" values */
        if (read_32bitBE(0x00,sf) == 0xAF678753) { /* Azure Striker Gunvolt (PC) */
            static const uint8_t isd_gv_key[16] = {
                    0xe0,0x00,0xe0,0x00,0xa0,0x00,0x00,0x00,0xe0,0x00,0xe0,0x80,0x40,0x40,0x40,0x00
            };
            cfg.key_len = sizeof(isd_gv_key);
            memcpy(cfg.key, isd_gv_key, cfg.key_len);
            isl_name = "GV_steam.isl";
        }
        else if (read_32bitBE(0x00,sf) == 0x0FE787D3) { /* Mighty Gunvolt (PC) */
            static const uint8_t isd_mgv_key[120] = {
                    0x40,0x80,0xE0,0x80,0x40,0x40,0xA0,0x00,0xA0,0x40,0x00,0x80,0x00,0x40,0xA0,0x00,
                    0xC0,0x40,0xE0,0x00,0x60,0x40,0x80,0x00,0xA0,0x00,0xE0,0x00,0x60,0x40,0xC0,0x00,
                    0xA0,0x40,0xC0,0x80,0xE0,0x00,0x60,0x00,0x00,0x40,0x00,0x80,0xE0,0x80,0x40,0x00,
                    0xA0,0x80,0xA0,0x80,0x80,0xC0,0x60,0x00,0xA0,0x00,0xA0,0x80,0x40,0x80,0x60,0x00,
                    0x40,0xC0,0x20,0x00,0x20,0xC0,0x00,0x00,0x00,0xC0,0x20,0x00,0xC0,0xC0,0x60,0x00,
                    0xE0,0xC0,0x80,0x80,0x20,0x00,0x60,0x00,0xE0,0xC0,0xC0,0x00,0x20,0xC0,0xC0,0x00,
                    0x60,0x00,0xE0,0x80,0x00,0xC0,0x00,0x00,0x60,0x80,0x40,0x80,0x20,0x80,0x20,0x00,
                    0x80,0x40,0xE0,0x00,0x20,0x00,0x20,0x00,
            };
            cfg.key_len = sizeof(isd_mgv_key);
            memcpy(cfg.key, isd_mgv_key, cfg.key_len);
            isl_name = "MGV_steam.isl";
        }
        else if (read_32bitBE(0x00,sf) == 0x0FA74753) { /* Blaster Master Zero (PC) */
            static const uint8_t isd_bmz_key[120] = {
                    0x40,0xC0,0x20,0x00,0x40,0xC0,0xC0,0x00,0x00,0x80,0xE0,0x80,0x80,0x40,0x20,0x00,
                    0x60,0xC0,0xC0,0x00,0xA0,0x80,0x60,0x00,0x40,0x40,0x20,0x00,0x60,0x40,0xC0,0x00,
                    0x60,0x80,0xC0,0x80,0x40,0xC0,0x00,0x00,0xA0,0xC0,0x80,0x80,0x60,0x80,0xA0,0x00,
                    0x40,0x80,0x60,0x00,0x20,0x00,0xC0,0x00,0x60,0x00,0xA0,0x80,0x40,0x40,0xA0,0x00,
                    0x40,0x40,0xC0,0x80,0x00,0x80,0x60,0x00,0x80,0xC0,0xA0,0x00,0xE0,0x40,0xC0,0x00,
                    0x20,0x80,0xE0,0x00,0x40,0xC0,0xA0,0x00,0xE0,0xC0,0xC0,0x80,0xE0,0x80,0xC0,0x00,
                    0x40,0x40,0x00,0x00,0x20,0x40,0x80,0x00,0xE0,0x80,0x20,0x80,0x40,0x80,0xE0,0x00,
                    0xA0,0x00,0xC0,0x80,0xE0,0x00,0x20,0x00
            };
            cfg.key_len = sizeof(isd_bmz_key);
            memcpy(cfg.key, isd_bmz_key, cfg.key_len);
            isl_name = "output.isl";
        }
        else {
            goto fail;
        }

        cfg.is_encrypted = 1;

        /* .isd have companion files in the prev folder:
         * - .ish: constant id/names (not always)
         * - .isf: format table, ordered like file id/numbers, 0x18 header with:
         *   0x00(2): ?, 0x02(2): channels, 0x04: sample rate, 0x08: skip samples (in PCM bytes), always 32000
         *   0x0c(2): PCM block size, 0x0e(2): PCM bps, 0x10: null, 0x18: samples (in PCM bytes)
         * - .isl: looping table (encrypted like the files) */
        if (isl_name) {
            STREAMFILE* islFile = NULL;

            islFile = open_streamfile_by_filename(sf, isl_name);

            if (!islFile) {
                /* try in ../(file) too since that's how the .isl is stored on disc */
                char isl_path[PATH_LIMIT];
                snprintf(isl_path, sizeof(isl_path), "../%s", isl_name);
                islFile = open_streamfile_by_filename(sf, isl_path);
            }

            if (islFile) {
                STREAMFILE* dec_sf = NULL;

                dec_sf = setup_ogg_vorbis_streamfile(islFile, cfg);
                if (dec_sf) {
                    off_t loop_offset;
                    char basename[PATH_LIMIT];

                    /* has a bunch of tables then a list with file names without extension and loops */
                    loop_offset = read_32bitLE(0x18, dec_sf);
                    get_streamfile_basename(sf, basename, sizeof(basename));

                    while (loop_offset < get_streamfile_size(dec_sf)) {
                        char testname[0x20];

                        read_string(testname, sizeof(testname), loop_offset+0x2c, dec_sf);
                        if (strcmp(basename, testname) == 0) {
                            ovmi.loop_start = read_32bitLE(loop_offset+0x1c, dec_sf);
                            ovmi.loop_end   = read_32bitLE(loop_offset+0x20, dec_sf);
                            ovmi.loop_end_found = 1;
                            ovmi.loop_flag = (ovmi.loop_end != 0);
                            break;
                        }

                        loop_offset += 0x50;
                    }

                    close_streamfile(dec_sf);
                }

                close_streamfile(islFile);
            }
        }
    }

    if (is_rpgmvo) { /* [RPG Maker MV (PC)] */
        if (read_32bitBE(0x00,sf) != 0x5250474D &&  /* "RPGM" */
            read_32bitBE(0x00,sf) != 0x56000000) {  /* "V\0\0\0" */
            goto fail;
        }
        ovmi.decryption_callback = rpgmvo_ogg_decryption_callback;

        start_offset = 0x10;
    }

    if (is_eno) { /* [Metronomicon (PC)] */
        /* first byte probably derives into key, but this works too */
        cfg.key[0] = (uint8_t)read_8bit(0x05,sf); /* regular ogg have a zero at this offset = easy key */
        cfg.key_len = 1;
        cfg.is_encrypted = 1;
        start_offset = 0x01; /* "OggS" starts after key-thing */
    }

    if (is_gwm) { /* [Adagio: Cloudburst (PC)] */
        cfg.key[0] = 0x5D;
        cfg.key_len = 1;
        cfg.is_encrypted = 1;
    }

    if (is_mus) { /* [Redux: Dark Matters (PC)] */
        static const uint8_t mus_key[16] = {
                0x21,0x4D,0x6F,0x01,0x20,0x4C,0x6E,0x02,0x1F,0x4B,0x6D,0x03,0x20,0x4C,0x6E,0x02
        };
        cfg.key_len = sizeof(mus_key);
        memcpy(cfg.key, mus_key, cfg.key_len);
        cfg.is_header_swap = 1; /* decrypted header gives "Mus " */
        cfg.is_encrypted = 1;
    }

    if (is_lse) { /* [Nippon Ichi PC games] */
        if (read_32bitBE(0x00,sf) == 0xFFFFFFFF) { /* [Operation Abyss: New Tokyo Legacy (PC)] */
            cfg.key[0] = 0xFF;
            cfg.key_len = 1;
            cfg.is_header_swap = 1;
            cfg.is_encrypted = 1;
        }
        else { /* [Operation Babel: New Tokyo Legacy (PC), Labyrinth of Refrain: Coven of Dusk (PC)] */
            int i;
            /* found at file_size-1 but this works too (same key for most files but can vary) */
            uint8_t base_key = (uint8_t)read_8bit(0x04,sf) - 0x04;

            cfg.key_len = 256;
            for (i = 0; i < cfg.key_len; i++) {
                cfg.key[i] = (uint8_t)(base_key + i);
            }
            cfg.is_encrypted = 1;
        }
    }

    if (is_bgm) { /* [Fortissimo (PC)] */
        size_t file_size = get_streamfile_size(sf);
        uint8_t key[0x04];
        uint32_t xor_be;

        put_32bitLE(key, (uint32_t)file_size);
        xor_be = (uint32_t)get_32bitBE(key);
        if ((read_32bitBE(0x00,sf) ^ xor_be) == 0x4F676753) { /* "OggS" */
            int i;
            cfg.key_len = 4;
            for (i = 0; i < cfg.key_len; i++) {
                cfg.key[i] = key[i];
            }
            cfg.is_encrypted = 1;
        }
    }


    if (cfg.is_encrypted) {
        temp_sf = setup_ogg_vorbis_streamfile(sf, cfg);
        if (!temp_sf) goto fail;
    }

    if (ovmi.meta_type == 0) {
        if (cfg.is_encrypted || ovmi.decryption_callback != NULL)
            ovmi.meta_type = meta_OGG_encrypted;
        else
            ovmi.meta_type = meta_OGG_VORBIS;
    }

    vgmstream = init_vgmstream_ogg_vorbis_callbacks(temp_sf != NULL ? temp_sf : sf, NULL, start_offset, &ovmi);

    close_streamfile(temp_sf);
    return vgmstream;

fail:
    close_streamfile(temp_sf);
    return NULL;
}

VGMSTREAM* init_vgmstream_ogg_vorbis_callbacks(STREAMFILE* sf, ov_callbacks* callbacks, off_t start, const ogg_vorbis_meta_info_t *ovmi) {
    VGMSTREAM* vgmstream = NULL;
    ogg_vorbis_codec_data* data = NULL;
    ogg_vorbis_io io = {0};
    char name[STREAM_NAME_SIZE] = {0};
    int channels, sample_rate, num_samples;

    int loop_flag = ovmi->loop_flag;
    int32_t loop_start = ovmi->loop_start;
    int loop_length_found = ovmi->loop_length_found;
    int32_t loop_length = ovmi->loop_length;
    int loop_end_found = ovmi->loop_end_found;
    int32_t loop_end = ovmi->loop_end;
    size_t stream_size = ovmi->stream_size ?
            ovmi->stream_size :
            get_streamfile_size(sf) - start;
    int disable_reordering = ovmi->disable_reordering;


    //todo improve how to pass config
    io.decryption_callback = ovmi->decryption_callback;
    io.scd_xor = ovmi->scd_xor;
    io.scd_xor_length = ovmi->scd_xor_length;
    io.xor_value = ovmi->xor_value;

    data = init_ogg_vorbis(sf, start, stream_size, &io);
    if (!data) goto fail;

    ogg_vorbis_get_info(data, &channels, &sample_rate);
    ogg_vorbis_get_samples(data, &num_samples); /* let libvorbisfile find total samples */

    /* search for loop comments */
    {//todo ignore if loop flag already set?
        const char* comment = NULL;

        while (ogg_vorbis_get_comment(data, &comment)) {

            if (strstr(comment,"loop_start=") == comment || /* PSO4 */
                strstr(comment,"LOOP_START=") == comment || /* PSO4 */
                strstr(comment,"LOOPPOINT=") == comment || /* Sonic Robo Blast 2 */
                strstr(comment,"COMMENT=LOOPPOINT=") == comment ||
                strstr(comment,"LOOPSTART=") == comment ||
                strstr(comment,"um3.stream.looppoint.start=") == comment ||
                strstr(comment,"LOOP_BEGIN=") == comment || /* Hatsune Miku: Project Diva F (PS3) */
                strstr(comment,"LoopStart=") == comment ||  /* Devil May Cry 4 (PC) */
                strstr(comment,"LOOP=") == comment || /* Duke Nukem 3D: 20th Anniversary World Tour */
                strstr(comment,"XIPH_CUE_LOOPSTART=") == comment) {  /* Super Mario Run (Android) */
                loop_start = atol(strrchr(comment,'=')+1);
                loop_flag = (loop_start >= 0);
            }
            else if (strstr(comment,"LOOPLENGTH=") == comment) {/* (LOOPSTART pair) */
                loop_length = atol(strrchr(comment,'=')+1);
                loop_length_found = 1;
            }
            else if (strstr(comment,"title=-lps") == comment) { /* KID [Memories Off #5 (PC), Remember11 (PC)] */
                loop_start = atol(comment+10);
                loop_flag = (loop_start >= 0);
            }
            else if (strstr(comment,"album=-lpe") == comment) { /* (title=-lps pair) */
                loop_end = atol(comment+10);
                loop_flag = 1;
                loop_end_found = 1;
            }
            else if (strstr(comment,"LoopEnd=") == comment) { /* (LoopStart pair) */
                if(loop_flag) {
                    loop_length = atol(strrchr(comment,'=')+1)-loop_start;
                    loop_length_found = 1;
                }
            }
            else if (strstr(comment,"LOOP_END=") == comment) { /* (LOOP_BEGIN pair) */
                if(loop_flag) {
                    loop_length = atol(strrchr(comment,'=')+1)-loop_start;
                    loop_length_found = 1;
                }
            }
            else if (strstr(comment,"lp=") == comment) {
                sscanf(strrchr(comment,'=')+1,"%d,%d", &loop_start,&loop_end);
                loop_flag = 1;
                loop_end_found = 1;
            }
            else if (strstr(comment,"LOOPDEFS=") == comment) { /* Fairy Fencer F: Advent Dark Force */
                sscanf(strrchr(comment,'=')+1,"%d,%d", &loop_start,&loop_end);
                loop_flag = 1;
                loop_end_found = 1;
            }
            else if (strstr(comment,"COMMENT=loop(") == comment) { /* Zero Time Dilemma (PC) */
                sscanf(strrchr(comment,'(')+1,"%d,%d", &loop_start,&loop_end);
                loop_flag = 1;
                loop_end_found = 1;
            }
            else if (strstr(comment, "XIPH_CUE_LOOPEND=") == comment) { /* (XIPH_CUE_LOOPSTART pair) */
                if (loop_flag) {
                    loop_length = atol(strrchr(comment, '=') + 1) - loop_start;
                    loop_length_found = 1;
                }
            }
            else if (strstr(comment, "omment=") == comment) { /* Air (Android) */
                sscanf(strstr(comment, "=LOOPSTART=") + 11, "%d,LOOPEND=%d", &loop_start, &loop_end);
                loop_flag = 1;
                loop_end_found = 1;
            }
            else if (strstr(comment,"MarkerNum=0002") == comment) { /* Megaman X Legacy Collection: MMX1/2/3 (PC) flag */
                /* uses LoopStart=-1 LoopEnd=-1, then 3 secuential comments: "MarkerNum" + "M=7F(start)" + "M=7F(end)" */
                loop_flag = 1;
            }
            else if (strstr(comment,"M=7F") == comment) { /* Megaman X Legacy Collection: MMX1/2/3 (PC) start/end */
                if (loop_flag && loop_start < 0) { /* LoopStart should set as -1 before */
                    sscanf(comment,"M=7F%x", &loop_start);
                }
                else if (loop_flag && loop_start >= 0) {
                    sscanf(comment,"M=7F%x", &loop_end);
                    loop_end_found = 1;
                }
            }
            else if (strstr(comment,"LOOPMS=") == comment) { /* Sonic Robo Blast 2 */
                /* Convert from milliseconds to samples. */
                /* (x ms) * (y samples/s) / (1000 ms/s) */
                loop_start = atol(strrchr(comment,'=')+1) * sample_rate / 1000;
                loop_flag = (loop_start >= 0);
            }

            /* Hatsune Miku Project DIVA games, though only 'Arcade Future Tone' has >4ch files
             * ENCODER tag is common but ogg_vorbis_encode looks unique enough
             * (arcade ends with "2010-11-26" while consoles have "2011-02-07" */
            if (strstr(comment, "ENCODER=ogg_vorbis_encode/") == comment) {
                disable_reordering = 1;
            }

            if (strstr(comment, "TITLE=") == comment) {
                strncpy(name, comment + 6, sizeof(name) - 1);
            }

            ;VGM_LOG("OGG: user_comment=%s\n", comment);
        }
    }

    ogg_vorbis_set_disable_reordering(data, disable_reordering);


    /* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channels,loop_flag);
    if (!vgmstream) goto fail;

    vgmstream->codec_data = data;
    vgmstream->coding_type = coding_OGG_VORBIS;
    vgmstream->layout_type = layout_none;
    vgmstream->meta_type = ovmi->meta_type;

    vgmstream->sample_rate = sample_rate;
    vgmstream->stream_size = stream_size;

    if (ovmi->total_subsongs) /* not setting it has some effect when showing stream names */
        vgmstream->num_streams = ovmi->total_subsongs;

    if (name[0] != '\0')
        strcpy(vgmstream->stream_name, name);

    vgmstream->num_samples = num_samples;
    if (loop_flag) {
        vgmstream->loop_start_sample = loop_start;
        if (loop_length_found)
            vgmstream->loop_end_sample = loop_start + loop_length;
        else if (loop_end_found)
            vgmstream->loop_end_sample = loop_end;
        else
            vgmstream->loop_end_sample = vgmstream->num_samples;

        if (vgmstream->loop_end_sample > vgmstream->num_samples)
            vgmstream->loop_end_sample = vgmstream->num_samples;
    }

    if (vgmstream->channels <= 8) {
        vgmstream->channel_layout = xiph_mappings[vgmstream->channels];
    }

    return vgmstream;

fail:
    close_vgmstream(vgmstream);
    return NULL;
}

#endif
