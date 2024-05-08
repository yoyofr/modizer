#ifndef MYON_FMDRIVER_PMD_H_INCLUDED
#define MYON_FMDRIVER_PMD_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "fmdriver.h"
#include <stddef.h>
enum {
  PMD_FILENAMELEN = 8+1+3
};

enum {
  PMD_PART_FM_1,
  PMD_PART_FM_2,
  PMD_PART_FM_3,
  PMD_PART_FM_4,
  PMD_PART_FM_5,
  PMD_PART_FM_6,
  PMD_PART_SSG_1,
  PMD_PART_SSG_2,
  PMD_PART_SSG_3,
  PMD_PART_ADPCM,
  PMD_PART_RHYTHM,
  PMD_PART_FM_3B,
  PMD_PART_FM_3C,
  PMD_PART_FM_3D,
  PMD_PART_PPZ_1,
  PMD_PART_PPZ_2,
  PMD_PART_PPZ_3,
  PMD_PART_PPZ_4,
  PMD_PART_PPZ_5,
  PMD_PART_PPZ_6,
  PMD_PART_PPZ_7,
  PMD_PART_PPZ_8,
  PMD_PART_FM_EFF,
  PMD_PART_NUM,
};

struct pmd_lfo {
  uint8_t delay;
  uint8_t speed;
  uint8_t step;
  uint8_t times;
};

struct pmd_part {
  // 0000
  uint16_t ptr;
  // 0002
  uint16_t loop_ptr;
  // 0004
  uint8_t len_cnt;
  // 0005
  uint8_t gate;
  // 0006
  uint16_t actual_freq;
  // 0008
  int16_t detune;
  // 000a
  int16_t lfo_diff;
  // 000c
  int16_t portamento_diff;
  // 000e
  int16_t portamento_add;
  // 0010
  int16_t portamento_rem;
  // 0012
  uint8_t vol;
  // 0013
  uint8_t transpose;
  // 0014
  struct pmd_lfo lfo;
  // 0018
  struct pmd_lfo lfo_set;
  // 001c
  struct pmd_part_lfo_flags {
    // bit 0
    bool freq;
    // bit 1
    bool vol;
    // bit 2
    bool sync;
    // bit 3
    bool portamento;
  } lfof, lfof_b;
  // 001d
  uint8_t volume_save;
  // 001e
  uint8_t md_depth;
  // 001f
  uint8_t md_speed;
  // 0020
  uint8_t md_speed_set;
  // 0021
  uint8_t ssg_env_state_old;
  // 0022
  uint8_t ssg_env_state_new;
  // 0023
  uint8_t ssg_env_param_set[4];
  // 0027
  uint8_t ssg_env_param_sl;
  // 0028
  uint8_t ssg_env_param_al;
  // 0029
  uint8_t ssg_env_param[4];
  // 002d
  uint8_t ssg_env_vol;
  // 002e
  struct pmd_part_flagext {
    // bit 0
    bool detune;
    // bit 1
    bool lfo;
    // bit 2
    bool env;
  } flagext, flagext_b;
  // 002f
  uint8_t pan;
  // 0030
  uint8_t ssg_mix;
  // 0031
  uint8_t tonenum;
  // 0032
  struct {
    // bit 0
    bool looped;
    // bit 1
    bool ended;
    // bit 2
    bool env;
  } loop;
  // 0033
  // bit 7-4: slot 4,3,2,1
  uint8_t fm_slotout;
  // 0034
  // 1,3,2,4
  uint8_t fm_tone_tl[4];
  // 0038
  // 4,3,2,1,0,0,0,0
  // when 1, enabled
  uint8_t fm_slotmask;
  // 0039
  // 1,3,2,4,1,3,2,4
  uint8_t fm_tone_slotmask;
  // 003a
  uint8_t lfo_waveform;
  // 003b
  struct {
    // bit 0
    bool ext;
    // bit 1
    bool effect;
    // bit 2
    bool disabled;
    // bit 5
    // true when all slots masked (part->fm_slotmask == 0)
    bool slot;
    // bit 6
    bool mml;
    // bit 7
    bool ff;
  } mask;
  // 003c
  struct {
    // bit 0, 2-7
    bool off;
    // bit 1
    bool off_mask;
  } keystatus;
  // 003d
  // default: same as part->fm_slotout
  // bit 7-4: mask slot 4,3,2,1
  // bit 3-0: if 1, overridden by user
  uint8_t vol_lfo_slotmask;
  // 003e
  // q
  uint8_t gate_abs;
  // 003f
  // Q
  uint8_t gate_rel;
  // 0040
  uint8_t hlfo_delay_set;
  // 0041
  uint8_t hlfo_delay;
  // 0042
  int16_t lfo_diff_b;
  // 0044
  struct pmd_lfo lfo_b;
  // 0048
  struct pmd_lfo lfo_set_b;
  // 004c
  uint8_t md_depth_b;
  // 004d
  uint8_t md_speed_b;
  // 004e
  uint8_t md_speed_set_b;
  // 004f
  uint8_t lfo_waveform_b;
  // 0050
  uint8_t vol_lfo_slotmask_b;
  // 0051
  uint8_t md_cnt;
  // 0052
  uint8_t md_cnt_set;
  // 0053
  uint8_t md_cnt_b;
  // 0054
  uint8_t md_cnt_set_b;
  // 0055
  uint8_t actual_note;
  // 0056
  uint8_t slot_delay;
  // 0057
  uint8_t slot_delay_cnt;
  // 0058
  uint8_t slot_delay_mask;
  // 0059
  uint8_t fb_alg;
  // 005a
  // increment when new note/rest processed
  uint8_t note_proc;
  // 005b
  uint8_t gate_min;
  // 005c
  // for ppz8
  uint16_t actual_freq_upper;
  // 005e
  uint8_t curr_note;
  // 005f
  uint8_t transpose_master;
  // 0060
  uint8_t gate_rand_range;
  
  // original
  // used from pmd_part_proc_ssg, pmd_cmdc0, pmd_cmdcf_slotmask
  bool proc_masked;
  // for display
  uint8_t len;
  uint32_t output_freq;
};

struct pmd_ssgeff_data {
  // ticks to wait, or 0xff to end
  uint8_t wait;
  uint16_t tone_freq;
  uint8_t noise_freq;
  bool tone_mix;
  bool noise_mix;
  uint8_t env_level;
  uint16_t env_freq;
  uint8_t env_waveform;
  int8_t tonefreq_add;
  int8_t noisefreq_add;
  uint8_t noisefreq_wait;
};

struct driver_pmd {
  // pmd->data = data+1;
  // pmd->datalen = datalen-1;
  // data[-1] is valid (OPM flag)
  uint8_t *data;
  uint16_t datalen;
  // 0ee1
  const struct pmd_ssgeff_data *ssgeff_ptr;
  // 0ee3
  uint16_t ssgeff_tonefreq;
  // 0ee5
  int16_t ssgeff_tonefreq_add;
  // 0ee7
  uint8_t ssgeff_wait;
  // 0ee8
  uint8_t ssgeff_noisefreq;
  // 0ee9
  int8_t ssgeff_noisefreq_add;
  uint8_t ssgeff_noisefreq_cnt_set;
  // 0eea
  uint8_t ssgeff_noisefreq_cnt;
  // 0eeb
  uint8_t ssgeff_priority;
  // 0eec
  uint8_t ssgeff_num;
  // 0eed
  bool ssgeff_internal;
  // 0eee
  // 2fc2
  uint16_t rand;
  // 42ae
  uint8_t proc_ch;
  // 42af
  // set with 0xfb
  // when next command is 0xfb, disable gate (q/Q)
  // used to implement tie
  bool no_keyoff;
  // 42b0
  // set when volume saved for soft echo (W)
  bool volume_saved;
  // 42b2
  bool opna_a1;
  // 42b3
  // same format as opna 0x28
  // 0b4321____
  uint8_t fm_slotkey[6];
  // 42b9
  // compare with part->loop
  struct {
    // bit 0
    bool looped;
    // bit 1
    bool ended;
    // bit 2
    bool env;
  } loop;
  // 42ba
  bool ppsdrv_enabled;
  // 42bd
  uint16_t adpcm_start_loop;
  // 42bf
  uint16_t adpcm_stop_loop;
  // 42c1
  uint16_t adpcm_release;
  // 42c3
  // timera_cnt sampled at timerb
  uint8_t timera_cnt_b;
  // 42c5
  // needed when fm3ex_det valid
  bool fm3ex_force;
  // 42c6
  // 1: ch3
  // 2: ch3a
  // 4: ch3b
  // 8: ch3c
  // when 0, not needed to extend fm3
  uint8_t fm3ex_needed;
  // 42d2
  uint8_t fm3ex_fb_alg;
  // 42d3
  bool tonemask_fb_alg;
  // 42d5
  struct pmd_part_lfo_flags lfoprocf, lfoprocf_b;
  // 42d6
  // 430e
  uint16_t tone_ptr;
  // 4310
  uint16_t r_offset;
  // 4312
  uint16_t r_ptr;
  // 4314
  uint8_t opnarhythm_mask;
  // 4317
  uint8_t fm_voldown;
  // 4318
  uint8_t ssg_voldown;
  // 4319
  uint8_t adpcm_voldown;
  // 431a
  uint8_t opnarhythm_voldown;
  // 431b
  bool tone_included;
  // 431c
  bool opm_flag;
  // 431d
  uint8_t status1;
  // 431e
  uint8_t status2;
  // 431f
  uint8_t timerb;
  // 4320
  uint8_t fadeout_speed;
  // 4321
  uint8_t fadeout_vol;
  // 4322
  uint8_t timerb_bak;
  // 4323
  uint8_t meas_len;
  // 4324
  uint8_t tick_cnt;
  // 4325
  uint8_t timera_cnt;
  // 4326
  bool ssgeff_disabled;
  // 4327
  uint8_t ssg_noise_freq;
  // 4328
  uint8_t ssg_noise_freq_wrote;
  // 432a
  bool fmeff_playing;
  // 432c
  bool adpcmeff_playing;
  // 432d
  uint16_t adpcm_start;
  // 432f
  uint16_t adpcm_stop;
  // 433a
  uint8_t opnarhythm;
  // 433b
  uint8_t opnarhythm_ilpan[6];
  // 4341
  uint8_t opnarhythm_tl;
  // 4342
  uint16_t ssgrhythm;
  // 4348
  bool playing;
  // 4349
  bool paused;
  // 434a
  //bool fadeout_mstop;
  // 434b
  bool ssgrhythm_opna;
  // 4350
  bool adpcm_disabled;
  // 4354
  int16_t fm3ex_det[4];
  // 4362
  uint8_t timerb_wrote;
  // 4363
  bool faded_out;
  // 4366
  bool pcm86_vol_spb;
  // 4367
  uint16_t meas_cnt;
  // 436a
  uint8_t opna_last22;
  // 436b
  uint8_t tempo;
  // 436c
  uint8_t tempo_bak;
  // 4370
  uint8_t fm_voldown_orig;
  // 4371
  uint8_t ssg_voldown_orig;
  // 4372
  uint8_t adpcm_voldown_orig;
  // 4373
  uint8_t opnarhythm_voldown_orig;
  // 4374
  bool pcm86_vol_spb_orig;
  // 4386
  uint8_t opnarhythm_shot_inc[6];
  // 438c
  uint8_t opnarhythm_dump_inc[6];
  // 4392
  // 0b0e111111
  // output to 0x27
  uint8_t fm3ex_mode;
  // 4394
  uint8_t ppz8_voldown;
  // 4395
  uint8_t ppz8_voldown_orig;
  // 43ce
  struct pmd_part parts[PMD_PART_NUM];

  // 4c9e
  uint16_t adpcm_addr[256][2];

  bool ssgeff_tone_mix;
  bool ssgeff_noise_mix;
  bool ssgeff_on;
  char ppzfile[PMD_FILENAMELEN+1];
  char ppzfile2[PMD_FILENAMELEN+1];
  char ppsfile[PMD_FILENAMELEN+1];
  char ppcfile[PMD_FILENAMELEN+1];
};

bool pmd_load(struct driver_pmd *pmd, uint8_t *data, uint16_t datalen);
void pmd_init(struct fmdriver_work *work, struct driver_pmd *pmd);
bool pmd_ppc_load(struct fmdriver_work *work, uint8_t *data, size_t datalen);
#ifdef __cplusplus
}
#endif

#endif // MYON_FMDRIVER_PMD_H_INCLUDED
