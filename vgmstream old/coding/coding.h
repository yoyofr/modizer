#ifndef _CODING_H
#define _CODING_H

#include "../vgmstream.h"

void decode_adx(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_adx_enc(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void adx_next_key(VGMSTREAMCHANNEL * stream);

void decode_g721(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void g72x_init_state(struct g72x_state *state_ptr);

void decode_nds_ima(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_dat4_ima(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_xbox_ima(VGMSTREAM * vgmstream,VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do,int channel);
void decode_int_xbox_ima(VGMSTREAM * vgmstream,VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do,int channel);
void decode_dvi_ima(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_eacs_ima(VGMSTREAM * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int channel);

void decode_snds_ima(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int channel);

void decode_ima(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_ms_ima(VGMSTREAM * vgmstream,VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do,int channel);

void decode_rad_ima(VGMSTREAM * vgmstream,VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do,int channel);

void decode_rad_ima_mono(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_apple_ima4(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_ms_ima(VGMSTREAM * vgmstream,VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do,int channel);
void decode_ngc_afc(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_ngc_dsp(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_ngc_dsp_mem(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, uint8_t * mem);

int32_t dsp_nibbles_to_samples(int32_t nibbles);

void decode_ngc_dtk(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int channel);

void decode_pcm16LE(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_pcm16LE_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_pcm16LE_XOR_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_pcm16BE(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_pcm8(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_pcm8_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_pcm8_sb_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_pcm8_unsigned_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_pcm8_unsigned(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_psx(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_invert_psx(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_psx_badflags(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_ffxi_adpcm(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_baf_adpcm(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_xa(VGMSTREAM * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int channel);
void init_get_high_nibble(VGMSTREAM * vgmstream);

void decode_eaxa(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do,int channel);
void decode_ea_adpcm(VGMSTREAM * vgmstream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int channel);
void decode_maxis_adpcm(VGMSTREAM * vgmstream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int channel);

#ifdef VGM_USE_VORBIS
void decode_ogg_vorbis(ogg_vorbis_codec_data * data, sample * outbuf, int32_t samples_to_do, int channels);
#endif

void decode_sdx2(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_sdx2_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_cbd2(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);
void decode_cbd2_int(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_ws(VGMSTREAM * vgmstream, int channel, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

#ifdef VGM_USE_MPEG
void decode_fake_mpeg2_l2(VGMSTREAMCHANNEL * stream,
        mpeg_codec_data * data,
        sample * outbuf, int32_t samples_to_do);
mpeg_codec_data *init_mpeg_codec_data(STREAMFILE *streamfile, off_t start_offset, long given_sample_rate, int given_channels, coding_t *coding_type, int * actual_sample_rate, int * actual_channels);
long mpeg_bytes_to_samples(long bytes, const struct mpg123_frameinfo *mi);
void decode_mpeg(VGMSTREAMCHANNEL * stream,
        mpeg_codec_data * data,
        sample * outbuf, int32_t samples_to_do, int channels);
#endif

#ifdef VGM_USE_G7221
void decode_g7221(VGMSTREAM *vgmstream,
        sample * outbuf, int channelspacing, int32_t samples_to_do, int channel);
#endif

void decode_acm(ACMStream * acm, sample * outbuf,
        int32_t samples_to_do, int channelspacing);

void decode_nwa(NWAData *nwa, sample *outbuf, int32_t samples_to_do);

long msadpcm_bytes_to_samples(long bytes, int block_size, int channels);
void decode_msadpcm_stereo(VGMSTREAM * vgmstream, sample * outbuf, int32_t first_sample, int32_t samples_to_do);

void decode_msadpcm_mono(VGMSTREAM * vgmstream, sample * outbuf, int32_t first_sample, int32_t samples_to_do);

void decode_aica(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_nds_procyon(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_l5_555(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_SASSC(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_lsf(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do);

void decode_mtaf(VGMSTREAMCHANNEL * stream, sample * outbuf, int channelspacing, int32_t first_sample, int32_t samples_to_do, int channel, int channels);

#endif
