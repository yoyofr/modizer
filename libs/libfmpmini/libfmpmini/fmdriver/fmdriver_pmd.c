#include "fmdriver_pmd.h"
#include "fmdriver_common.h"
#include <stddef.h>
#include <string.h>

enum {
  SSG_ENV_STATE_OLD_AL,
  SSG_ENV_STATE_OLD_SR,
  SSG_ENV_STATE_OLD_RR,
  SSG_ENV_STATE_OLD_OFF,
  SSG_ENV_STATE_OLD_NEW = 0xff
};

enum {
  SSG_ENV_STATE_NEW_OFF,
  SSG_ENV_STATE_NEW_AR,
  SSG_ENV_STATE_NEW_DR,
  SSG_ENV_STATE_NEW_SR,
  SSG_ENV_STATE_NEW_RR
};

enum {
  SSG_ENV_PARAM_NEW_AR,
  SSG_ENV_PARAM_NEW_DR,
  SSG_ENV_PARAM_NEW_SR,
  SSG_ENV_PARAM_NEW_RR
};

enum {
  SSG_ENV_PARAM_OLD_AL,
  SSG_ENV_PARAM_OLD_AD,
  SSG_ENV_PARAM_OLD_SR,
  SSG_ENV_PARAM_OLD_RR
};

enum {
  LFO_WF_TRIANGLE1,
  LFO_WF_SAWTOOTH,
  LFO_WF_SQUARE,
  LFO_WF_RANDOM,
  LFO_WF_TRIANGLE2,
  LFO_WF_TRIANGLE3,
  LFO_WF_ONESHOT
};

// 0790: PPZ8


static uint8_t pmd_adpcm_freq2key(uint16_t freq) {
  if (!freq) return 0x00;
  static const uint16_t freqtab[12] = {
    0x8738, // e upper limit
    0x8f42,
    0x97c7,
    0xa0cd,
    0xaa5d,
    0xb47e,
    0xbf3a,
    0xca99,
    0xd6a5,
    0xe369,
    0xf0ee,
    0xff42,
  };
  int octave = 5;
  while (!(freq & 0x8000u)) {
    freq <<= 1;
    octave -= 1;
  }
  int key = 0;
  for (; key < 12; key++) {
    if (freq < freqtab[key]) break;
  }
  key += 5;
  if (key >= 12) {
    key -= 12;
    octave++;
  }
  if (octave < 0) return 0x00;
  if (octave > 8) return 0x8b;
  return (octave << 4) | key;
}

// 33f7: write standard
// 341f: write extended
// 3447
static void pmd_reg_write(struct fmdriver_work *work,
                          struct driver_pmd *pmd,
                          uint8_t addr, uint8_t data
                         ) {
  work->opna_writereg(work, addr | (pmd->opna_a1 ? 0x100:0), data);
}

// 132b
static void pmd_opna_a1_on(struct driver_pmd *pmd) {
  pmd->opna_a1 = true;
}

// 133d
static void pmd_opna_a1_off(struct driver_pmd *pmd) {
  pmd->opna_a1 = false;
}

// 346f
static uint8_t pmd_opna_read_ssgout(struct fmdriver_work *work) {
  return work->opna_readreg(work, 0x07);
}

/*
// 331f
static void pmd_stop_sound(struct fmdriver_work *work,
                     struct driver_pmd *pmd) {
  // stop all FM sound except fm effect (CH6)

  // set release rate to 0xff
  bool fmeff = pmd->fmeff_playing;
  for (int a1 = 0; a1 < 2; a1++) {
    a1 ? pmd_opna_a1_on(pmd) : pmd_opna_a1_off(pmd);
    for (int s = 0; s < 4; s++) {
      for (int c = 0; c < 3; c++) {
        if (a1 && fmeff && (c == 2)) continue;
        pmd_reg_write(work, pmd, 0x80|c|(s<<2), 0xff);
      }
    }
  }
  // 3353
  // keyoff
  for (int c = 0; c < 3; c++) {
    work->opna_writereg(work, 0x28, 0x00|c);
  }
  for (int c = 0; c < 3; c++) {
    if (fmeff && (c == 2)) continue;
    work->opna_writereg(work, 0x28, 0x04|c);
  }
  // 3375
  // stop SSG sound
  if (!pmd->ssgeff_num) {
    if (pmd->ppsdrv_enabled) {
      // TODO: PPSDRV
    }
    work->opna_writereg(work, 0x07, 0xbf);
  } else {
    // 338f
    // don't stop ssg effect when playing
    uint8_t ssgout = pmd_opna_read_ssgout(work);
    ssgout &= 0x3f;
    ssgout |= 0x9b;
    work->opna_writereg(work, 0x07, ssgout);
  }
  // 33a2
  // stop ADPCM
  if (!pmd->adpcmeff_playing && !pmd->adpcm_disabled) {
    work->opna_writereg(work, 0x101, 0x02);
    work->opna_writereg(work, 0x100, 0x01);
    work->opna_writereg(work, 0x110, 0x80);
    work->opna_writereg(work, 0x110, 0x18);
  }
  // 33c8
  // stop PPZ8
  if (work->ppz8_functbl) {
    for (int c = 0; c < 8; c++) {
      work->ppz8_functbl->channel_stop(work->ppz8, c);
    }
  }
}
*/

/*
// 11df
static void pmd_mstop(struct fmdriver_work *work,
                      struct driver_pmd *pmd) {
  pmd->playing = false;
  pmd->paused = false;
  pmd->fadeout_speed = 0;
  pmd->status2 = 0xff;
  pmd->fadeout_vol = 0xff;
  pmd_stop_sound(work, pmd);
}
*/

// 1058
static void pmd_reset_state(struct driver_pmd *pmd) {
  pmd->fadeout_vol = 0;
  pmd->fadeout_speed = 0;
  pmd->faded_out = false;
  for (struct pmd_part *p = pmd->parts; p < (&pmd->parts[PMD_PART_NUM]); p++) {
    /*
    bool ext = p->mask.ext;
    bool effect = p->mask.effect;
    bool mml = p->mask.mml;
    bool ff = p->mask.ff;
    uint8_t note_proc = p->note_proc;
      zero reset part structure
    */
    p->actual_note = 0xff;
    p->curr_note = 0xff;
  }
  for (struct pmd_part *p = &pmd->parts[PMD_PART_PPZ_1]; p <= (&pmd->parts[PMD_PART_PPZ_8]); p++) {
    p->loop.ended = true;
  }
  for (struct pmd_part *p = &pmd->parts[PMD_PART_FM_3B]; p <= (&pmd->parts[PMD_PART_FM_3D]); p++) {
    p->loop.ended = true;
  }
  // 1090
  pmd->no_keyoff = false;
  pmd->status1 = 0;
  pmd->status2 = 0;
  pmd->meas_cnt = 0;
  pmd->tick_cnt = 0;
  pmd->timera_cnt = 0;
  pmd->timera_cnt_b = 0;
  for (int i = 0; i < 6; i++) pmd->fm_slotkey[i] = 0;
  pmd->fm3ex_fb_alg = 0;
  pmd->tonemask_fb_alg = false;
  pmd->adpcm_start = 0;
  pmd->adpcm_stop = 0;
  pmd->adpcm_release = 0x8000;
  pmd->ssgrhythm = 0;
  pmd->opnarhythm = 0;
  pmd->rand = 0;
  pmd->fm3ex_force = false;
  for (int s = 0; s < 4; s++) {
    pmd->fm3ex_det[s] = 0;
  }
  pmd->fm3ex_needed = 0;
  pmd->fm3ex_mode = 0x3f;
  pmd->opna_a1 = false;
  pmd->meas_len = 96;
  pmd->fm_voldown = pmd->fm_voldown_orig;
  pmd->ssg_voldown = pmd->ssg_voldown_orig;
  pmd->adpcm_voldown = pmd->adpcm_voldown_orig;
  pmd->ppz8_voldown = pmd->ppz8_voldown_orig;
  pmd->opnarhythm_voldown = pmd->opnarhythm_voldown_orig;
  pmd->pcm86_vol_spb = pmd->pcm86_vol_spb_orig;
}

// 0fa9
static bool pmd_data_init(struct driver_pmd *pmd) {
  if (pmd->data[-1] > 1) return false;
  pmd->opm_flag = pmd->data[-1];
  if (pmd->datalen < 0x18) return false;
  if (pmd->data[0] != 0x18) {
    if (pmd->datalen < (0x18+2)) return false;
    pmd->tone_ptr = read16le(&pmd->data[0x18]);
    pmd->tone_included = true;
  } else {
    pmd->tone_included = false;
  }
  // 0fcd
  for (int pi = 0; pi <= PMD_PART_RHYTHM; pi++) {
    struct pmd_part *p = &pmd->parts[pi];
    p->ptr = read16le(&pmd->data[pi*2]);
    if (pmd->datalen < (p->ptr + 1)) return false;
    p->len_cnt = 1;
    p->keystatus.off = true;
    p->keystatus.off_mask = true;
    p->md_cnt = 0xff;
    p->md_cnt_set = 0xff;
    p->md_cnt_b = 0xff;
    p->md_cnt_set_b = 0xff;
    p->actual_note = 0xff;
    p->curr_note = 0xff;
    if (pi <= PMD_PART_FM_6) {
      p->vol = 108;
      p->pan = 0xc0;
      p->fm_slotmask = 0xf0;
      p->fm_tone_slotmask = 0xff;
    } else if (pi <= PMD_PART_SSG_3) {
      p->vol = 8;
      p->ssg_mix = 0x07;
      p->ssg_env_state_old = SSG_ENV_STATE_OLD_OFF;
    } else if (pi == PMD_PART_ADPCM) {
      p->vol = 128;
      p->pan = 0xc0;
    } else if (pi == PMD_PART_RHYTHM) {
      p->vol = 15;
    }
  }
  // 1049
  // part R
  pmd->r_offset = read16le(&pmd->data[0x16]);
  pmd->r_ptr = 0;
  return true;
}

// 111c
static void pmd_reset_opna(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  // enable FM 4-6ch and set IRQ mask
  work->opna_writereg(work, 0x29, 0x83);
  // set ssg noise freq to 0 and write
  pmd->ssg_noise_freq = 0;
  if (!pmd->ssgeff_priority) {
    work->opna_writereg(work, 0x06, 0x00);
    pmd->ssg_noise_freq_wrote = 0;
  }
  // 1139
  // reset pan/ams/pms
  for (int a = 0; a < 2; a++) {
    a ? pmd_opna_a1_on(pmd) : pmd_opna_a1_off(pmd);
    for (int c = 0; c < 3; c++) {
      pmd_reg_write(work, pmd, 0xb4+c, 0xc0);
    }
  }
  pmd_opna_a1_off(pmd);
  // 1166
  // reset HLFO
  pmd->opna_last22 = 0x00;
  work->opna_writereg(work, 0x22, 0x00);
  // 1170
  // reset opnarhythm
  for (int i = 0; i < 6; i++) {
    pmd->opnarhythm_ilpan[i] = 0xcf;
  }
  work->opna_writereg(work, 0x10, 0xff);
  if (pmd->opnarhythm_voldown) {
    pmd->opnarhythm_tl = (0xc0 * (0x100-pmd->opnarhythm_voldown)) >> 10;
  } else {
    pmd->opnarhythm_tl = 0x30;
  }
  work->opna_writereg(work, 0x11, pmd->opnarhythm_tl);
  // 11a0
  // adpcm: set limit address to 0xffff
  if (!pmd->adpcm_disabled) {
    work->opna_writereg(work, 0x10c, 0xff);
    work->opna_writereg(work, 0x10d, 0xff);
  }
  // 11b3
  // reset ppz8 pan
  // dx=5 ah=0x13 al=i-1 call 0790
  if (work->ppz8_functbl) {
    for (int c = 0; c < 8; c++) {
      work->ppz8_functbl->channel_pan(work->ppz8, c, 5);
    }
  }
}

// 2329
static void pmd_calc_tempo(
  struct driver_pmd *pmd
) {
  uint8_t timerb = 0x100 - pmd->timerb;
  int tempo = 0xff;
  if (timerb) {
    tempo = 0x112c / timerb;
    if ((0x112c % timerb) & 0x80) tempo++;
    if (tempo > 0xff) tempo = 0xff;
  }
  pmd->tempo = tempo;
  pmd->tempo_bak = tempo;
}

// 2348
static void pmd_calc_tempo_rev(
  struct driver_pmd *pmd
) {
  uint8_t tempo = pmd->tempo;
  int timerb = 0;
  if (tempo) {
    timerb = 0x112c / tempo;
    timerb = 0x100 - timerb;
    if ((0x112c % tempo) & 0x80) timerb--;
    if (timerb < 0) timerb = 0;
  }
  pmd->timerb = timerb;
  pmd->timerb_bak = timerb;
}

// 3e43
static void pmd_timerb_write(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  // fast forward not implemented
  pmd->timerb_wrote = pmd->timerb;
  work->opna_writereg(work, 0x26, pmd->timerb);
}

// 32eb
static void pmd_reset_timer(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  pmd->timerb = 200;
  work->timerb = pmd->timerb;
  pmd->timerb_bak = pmd->timerb;
  pmd_calc_tempo(pmd);
  pmd_timerb_write(work, pmd);
  // set timera to 0x000
  // enable both timers
  work->opna_writereg(work, 0x25, 0x00);
  work->opna_writereg(work, 0x24, 0x00);
  work->opna_writereg(work, 0x27, 0x3f);
  pmd->tick_cnt = 0;
  pmd->meas_cnt = 0;
  pmd->meas_len = 96;
}

// 2cf6
static void pmd_part_off_ssg(
  struct pmd_part *part
) {
  if (part->actual_note == 0xff) return;
  if (part->ssg_env_state_old == SSG_ENV_STATE_OLD_NEW) {
    part->ssg_env_state_new = SSG_ENV_STATE_NEW_RR;
  } else {
    part->ssg_env_state_old = SSG_ENV_STATE_OLD_RR;
  }
}

// 3102
static void pmd_lfo_reset(
  struct pmd_part *part
) {
  part->lfo_diff = 0;
  part->lfo = part->lfo_set;
  part->md_cnt = part->md_cnt_set;
  if (part->lfo_waveform == LFO_WF_SQUARE ||
      part->lfo_waveform == LFO_WF_RANDOM) {
    part->lfo.speed = 1;
  } else {
    part->lfo.speed++;
  }
}

// 26ec
static void pmd_opnarhythm_tl_write(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  uint8_t vol
) {
  uint8_t outvol = pmd->fadeout_vol;
  if (outvol) {
    outvol = ((uint16_t)vol * (0x100-outvol)) >> 8;
  }
  work->opna_writereg(work, 0x11, outvol);
}

static uint8_t pmd_part_cmdload(
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (!part->ptr || (pmd->datalen < (part->ptr + 1))) {
    part->ptr = 0;
    return 0x80;
  }
  return pmd->data[part->ptr++];
}

// 2f5a
static void pmd_part_md_tick(
  struct pmd_part *part
) {
  if (--part->md_speed) return;
  part->md_speed = part->md_speed_set;
  if (!part->md_cnt) return;
  if (!(part->md_cnt & 0x80)) part->md_cnt--;
  // 2f73
  uint8_t step = part->lfo.step;
  if (step & 0x80) {
    // 2f7a
    step = -step;
    step += part->md_depth;
    if (step & 0x80) {
      // 2f87
      step = 0;
      if (part->md_depth & 0x80) step = 0x81;
    } else {
      // 2f81
      step = -step;
    }
  } else {
    // 2f93
    step += part->md_depth;
    if (step & 0x80) {
      step = 0;
      if (part->md_depth & 0x80) step = 0x7f;
    }
  }
  part->lfo.step = step;
}

// 2fa8
static uint16_t pmd_rand(
  struct driver_pmd *pmd,
  uint16_t range
) {
  uint16_t rand = pmd->rand * 0x103u;
  rand += 3;
  rand &= 0x7fffu;
  pmd->rand = rand;
  uint32_t mul = (uint32_t)rand * range;
  return mul / 0x7fffu;
}

// 2e7f
static void pmd_lfo_tick_waveform(
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->lfo.speed != 1) {
    if (part->lfo.speed != 0xff) {
      part->lfo.speed--;
    }
    return;
  }
  part->lfo.speed = part->lfo_set.speed;
  switch (part->lfo_waveform) {
  case LFO_WF_TRIANGLE1:
  case LFO_WF_TRIANGLE2:
  case LFO_WF_TRIANGLE3:
    {
      int16_t deptha = u8s8(part->lfo.step);
      if (part->lfo_waveform == LFO_WF_TRIANGLE3) {
        deptha *= (deptha > 0) ? deptha : -deptha;
      }
      // 2ec6
      part->lfo_diff = u16s16((uint16_t)part->lfo_diff + (uint16_t)deptha);
      if (!part->lfo_diff) pmd_part_md_tick(part);
      if (part->lfo.times != 0xff) {
        if (!--part->lfo.times) {
          uint8_t times = part->lfo_set.times;
          if (part->lfo_waveform != LFO_WF_TRIANGLE2) times *= 2;
          part->lfo.times = times;
          part->lfo.step = -part->lfo.step;
        }
      }
    }
    break;
  case LFO_WF_SAWTOOTH:
    {
      int16_t deptha = u8s8(part->lfo.step);
      part->lfo_diff = u16s16((uint16_t)part->lfo_diff + (uint16_t)deptha);
      if (part->lfo.times != 0xff) {
        if (!--part->lfo.times) {
          // 2f09
          part->lfo_diff = -part->lfo_diff;
          pmd_part_md_tick(part);
          part->lfo.times = part->lfo_set.times * 2;
        }
      }
    }
    break;
  case LFO_WF_SQUARE:
    part->lfo_diff = u8s8(part->lfo.step) * u8s8(part->lfo.times);
    pmd_part_md_tick(part);
    part->lfo.step = -part->lfo.step;
    break;
  case LFO_WF_RANDOM:
    {
      // 2f40
      int deptha = u8s8(part->lfo.step);
      if (deptha < 0) deptha = -deptha;
      uint16_t range = (unsigned)deptha * part->lfo.times;
      uint16_t rand = pmd_rand(pmd, range*2);
      rand -= range;
      part->lfo_diff = rand;
      pmd_part_md_tick(part);
    }
    break;
  case LFO_WF_ONESHOT:
    // 2f18
    if (part->lfo.times) {
      if (part->lfo.times != 0xff) {
        part->lfo.times--;
      }
      part->lfo_diff = u16s16((uint16_t)part->lfo_diff + (uint16_t)u8s8(part->lfo.step));
    }
    break;
  }
}

// 2e48
// returns true if lfo_diff updated
static bool pmd_lfo_tick(
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->lfo.delay) {
    part->lfo.delay--;
    return false;
  }
  int16_t lfo_diff_prev = part->lfo_diff;
  if (part->flagext.lfo) {
    // 2e58
    uint8_t cnt = pmd->timera_cnt - pmd->timera_cnt_b;
    for (int i = 0; i < cnt; i++) {
      pmd_lfo_tick_waveform(pmd, part);
    }
  } else {
    // 2e6f
    pmd_lfo_tick_waveform(pmd, part);
  }
  // 2e76
  return part->lfo_diff != lfo_diff_prev;
}

// 2493
static void pmd_part_lfo_flip(struct pmd_part *part) {
  int16_t tmpi16;
  uint8_t tmpu8;
  struct pmd_lfo tmplfo;
  struct pmd_part_lfo_flags tmplfof;
  struct pmd_part_flagext tmpflagext;

  tmpi16 = part->lfo_diff_b;
  part->lfo_diff_b = part->lfo_diff;
  part->lfo_diff = tmpi16;
  tmplfof = part->lfof_b;
  part->lfof_b = part->lfof;
  part->lfof = tmplfof;
  tmpflagext = part->flagext_b;
  part->flagext_b = part->flagext;
  part->flagext = tmpflagext;
  tmplfo = part->lfo_b;
  part->lfo_b = part->lfo;
  part->lfo = tmplfo;
  tmplfo = part->lfo_set_b;
  part->lfo_set_b = part->lfo_set;
  part->lfo_set = tmplfo;
  tmpu8 = part->md_depth_b;
  part->md_depth_b = part->md_depth;
  part->md_depth = tmpu8;
  tmpu8 = part->md_speed_b;
  part->md_speed_b = part->md_speed;
  part->md_speed = tmpu8;
  tmpu8 = part->md_speed_set_b;
  part->md_speed_set_b = part->md_speed_set;
  part->md_speed_set = tmpu8;
  tmpu8 = part->lfo_waveform_b;
  part->lfo_waveform_b = part->lfo_waveform;
  part->lfo_waveform = tmpu8;
  tmpu8 = part->md_cnt_b;
  part->md_cnt_b = part->md_cnt;
  part->md_cnt = tmpu8;
  tmpu8 = part->md_cnt_set_b;
  part->md_cnt_set_b = part->md_cnt_set;
  part->md_cnt_set = tmpu8;
}

// 30a6
static void pmd_lfo_tick_if_needed_hlfo(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  part->hlfo_delay = part->hlfo_delay_set;
  if (part->hlfo_delay) {
    pmd_reg_write(work, pmd, 0xb3+pmd->proc_ch, part->pan & 0xc0);
  }
  // 30c0
  part->slot_delay_cnt = part->slot_delay;
  if (part->lfof.freq || part->lfof.vol) {
    if (!part->lfof.sync) {
      pmd_lfo_reset(part);
    }
    pmd_lfo_tick(pmd, part);
  }
  // 30db
  if (part->lfof_b.freq || part->lfof_b.vol) {
    if (!part->lfof_b.sync) {
      pmd_part_lfo_flip(part);
      pmd_lfo_reset(part);
      pmd_part_lfo_flip(part);
    }
    pmd_part_lfo_flip(part);
    pmd_lfo_tick(pmd, part);
    pmd_part_lfo_flip(part);
  }
}

// 3086
static void pmd_lfo_tick_if_needed(
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->lfof.freq || part->lfof.vol) {
    pmd_lfo_tick(pmd, part);
  }
  // 3091
  if (part->lfof_b.freq || part->lfof_b.vol) {
    pmd_part_lfo_flip(part);
    pmd_lfo_tick(pmd, part);
    pmd_part_lfo_flip(part);
  }
}

// 31ce
static void pmd_ssg_env_tick_new(
  struct pmd_part *part
) {
  switch (part->ssg_env_state_new) {
  case SSG_ENV_STATE_NEW_AR:
    // 31d2
    {
      uint8_t ar = part->ssg_env_param[SSG_ENV_PARAM_NEW_AR];
      if (((unsigned)(ar-1)) & 0x80) {
        // 31ff
        if (part->ssg_env_param_set[SSG_ENV_PARAM_NEW_AR]) {
          part->ssg_env_param[SSG_ENV_PARAM_NEW_AR]++;
        }
      } else {
        // 31d9
        part->ssg_env_vol = part->ssg_env_vol + ar;
        if (part->ssg_env_vol < 0xf) {
          // 31e4
          part->ssg_env_param[SSG_ENV_PARAM_NEW_AR] =
            part->ssg_env_param_set[SSG_ENV_PARAM_NEW_AR] - 0x10;
        } else {
          // 31ee
          part->ssg_env_vol = 0xf;
          part->ssg_env_state_new = SSG_ENV_STATE_NEW_DR;
          if (part->ssg_env_param_sl == 0xf) {
            part->ssg_env_state_new = SSG_ENV_STATE_NEW_SR;
          }
        }
      }
    }
    break;
  case SSG_ENV_STATE_NEW_DR:
    // 320d
    {
      uint8_t dr = part->ssg_env_param[SSG_ENV_PARAM_NEW_DR];
      if (((unsigned)(dr-1)) & 0x80) {
        // 3238
        if (part->ssg_env_param_set[SSG_ENV_PARAM_NEW_DR]) {
          part->ssg_env_param[SSG_ENV_PARAM_NEW_DR]++;
        }
      } else {
        // 3214
        bool ssg_env_vol_carry;
        {
          unsigned next_ssg_env_vol = part->ssg_env_vol - dr;
          ssg_env_vol_carry = next_ssg_env_vol & 0x100;
          part->ssg_env_vol = next_ssg_env_vol;
        }
        uint8_t sl = part->ssg_env_param_sl;
        FMDRIVER_DEBUG("DR: %d\n", part->ssg_env_vol);
        if (!ssg_env_vol_carry && (part->ssg_env_vol >= sl)) {
          // 3223
          uint8_t newdr = part->ssg_env_param_set[SSG_ENV_PARAM_NEW_DR] - 0x10;
          if (newdr & 0x80) newdr += newdr;
          part->ssg_env_param[SSG_ENV_PARAM_NEW_DR] = newdr;
        } else {
          // 3231
          part->ssg_env_vol = sl;
          FMDRIVER_DEBUG("DRnext: %d\n", part->ssg_env_vol);
          part->ssg_env_state_new = SSG_ENV_STATE_NEW_SR;
        }
      }
    }
    break;
  case SSG_ENV_STATE_NEW_SR:
    // 3246
    {
      uint8_t sr = part->ssg_env_param[SSG_ENV_PARAM_NEW_SR];
      if (((unsigned)(sr-1)) & 0x80) {
        // 3266
        if (part->ssg_env_param_set[SSG_ENV_PARAM_NEW_SR]) {
          part->ssg_env_param[SSG_ENV_PARAM_NEW_SR]++;
        }
      } else {
        // 324d
        bool ssg_env_vol_carry;
        {
          unsigned next_ssg_env_vol = part->ssg_env_vol - sr;
          ssg_env_vol_carry = next_ssg_env_vol & 0x100;
          part->ssg_env_vol = next_ssg_env_vol;
        }
        if (ssg_env_vol_carry) {
          part->ssg_env_vol = 0;
        }
        // 3258
        uint8_t newsr = part->ssg_env_param_set[SSG_ENV_PARAM_NEW_SR] - 0x10;
        if (newsr & 0x80) newsr += newsr;
        part->ssg_env_param[SSG_ENV_PARAM_NEW_SR] = newsr;
      }
    }
    break;
  default: // SSG_ENV_STATE_NEW_RR
    // 3273
    {
      uint8_t rr = part->ssg_env_param[SSG_ENV_PARAM_NEW_RR];
      if (((unsigned)(rr-1)) & 0x80) {
        // 3291
        if (part->ssg_env_param_set[SSG_ENV_PARAM_NEW_RR]) {
          part->ssg_env_param[SSG_ENV_PARAM_NEW_RR]++;
        }
      } else {
        // 327a
        bool ssg_env_vol_carry;
        {
          unsigned next_ssg_env_vol = part->ssg_env_vol - rr;
          ssg_env_vol_carry = next_ssg_env_vol & 0x100;
          part->ssg_env_vol = next_ssg_env_vol;
        }
        if (ssg_env_vol_carry) {
          part->ssg_env_vol = 0;
        }
        uint8_t newrr = part->ssg_env_param_set[SSG_ENV_PARAM_NEW_RR];
        newrr += newrr;
        newrr -= 0x10;
        part->ssg_env_param[SSG_ENV_PARAM_NEW_RR] = newrr;
      }
    }
    break;
  }
}

// 31b9
// returns true if volume update needed
static bool pmd_ssg_env_tick_new_check(
  struct pmd_part *part
) {
  if (part->ssg_env_state_new == SSG_ENV_STATE_NEW_OFF) return false;
  uint8_t prev_vol = part->ssg_env_vol;
  pmd_ssg_env_tick_new(part);
  FMDRIVER_DEBUG("ssgnewenv %d %d\n", part->ssg_env_state_new, part->ssg_env_vol);
  return part->ssg_env_vol != prev_vol;
}

// 3060
static uint8_t pmd_part_lfo_init_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t note
) {
  uint8_t n = note & 0xf;
  if (n == 0xc) {
    note = part->curr_note;
    n = note & 0xf;
  }
  // 3072
  part->curr_note = note;
  
  if (n == 0xf) {
    pmd_lfo_tick_if_needed(pmd, part);
    return note;
  }
  part->portamento_diff = 0;

  if (pmd->no_keyoff) {
    pmd_lfo_tick_if_needed(pmd, part);
  } else {
    pmd_lfo_tick_if_needed_hlfo(work, pmd, part);
  }
  return note;
}

// 2fc4
static uint8_t pmd_part_lfo_init_ssg(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t note
) {
  uint8_t n = note & 0xf;
  if (n == 0xc) {
    note = part->curr_note;
    n = note & 0xf;
  }
  // 2fd6
  part->curr_note = note;
  if (n == 0xf) {
    pmd_lfo_tick_if_needed(pmd, part);
    return note;
  }
  // 2fe1
  part->portamento_diff = 0;
  if (pmd->no_keyoff) {
    pmd_lfo_tick_if_needed(pmd, part);
    return note;
  }
  if (part->ssg_env_state_old != SSG_ENV_STATE_OLD_NEW) {
    // 2ff6
    part->ssg_env_state_old = SSG_ENV_STATE_OLD_AL;
    part->ssg_env_vol = 0;
    // isn't this reversed?
    uint8_t al = part->ssg_env_param_set[SSG_ENV_PARAM_OLD_AL] =
      part->ssg_env_param[SSG_ENV_PARAM_OLD_AL];
    if (!al) {
      // 3008
      part->ssg_env_state_old = SSG_ENV_STATE_OLD_SR;
      part->ssg_env_vol = part->ssg_env_param_set[SSG_ENV_PARAM_OLD_AD];
    }
    // 3012
    part->ssg_env_param_set[SSG_ENV_PARAM_OLD_SR] =
      part->ssg_env_param[SSG_ENV_PARAM_OLD_SR];
    part->ssg_env_param_set[SSG_ENV_PARAM_OLD_RR] =
      part->ssg_env_param[SSG_ENV_PARAM_OLD_RR];
    pmd_lfo_tick_if_needed_hlfo(work, pmd, part);
  } else {
    // 3021
    part->ssg_env_param[SSG_ENV_PARAM_NEW_AR] =
      part->ssg_env_param_set[SSG_ENV_PARAM_NEW_AR] - 0x10;
    int dr = u8s8(part->ssg_env_param_set[SSG_ENV_PARAM_NEW_DR] - 0x10);
    if (dr < 0) dr += dr;
    part->ssg_env_param[SSG_ENV_PARAM_NEW_DR] = dr;
    int sr = u8s8(part->ssg_env_param_set[SSG_ENV_PARAM_NEW_SR] - 0x10);
    if (sr < 0) sr += sr;
    part->ssg_env_param[SSG_ENV_PARAM_NEW_SR] = sr;
    uint8_t rr = part->ssg_env_param_set[SSG_ENV_PARAM_NEW_RR];
    rr += rr;
    rr -= 0x10;
    part->ssg_env_param[SSG_ENV_PARAM_NEW_RR] = rr;
    // 304f
    part->ssg_env_vol = part->ssg_env_param_al;
    part->ssg_env_state_new = SSG_ENV_STATE_NEW_AR;
    // 31b9
    pmd_ssg_env_tick_new_check(part);
    FMDRIVER_DEBUG("envinit\n");
    pmd_lfo_tick_if_needed_hlfo(work, pmd, part);
  }
  return note;
}

// 0e5e
static void pmd_ssgeff_stop(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  // TODO: PPSDRV
  work->opna_writereg(work, 0x0a, 0x00);
  uint8_t ssgout = pmd_opna_read_ssgout(work);
  ssgout |= (1<<2) | (1<<5);
  work->opna_writereg(work, 0x07, ssgout);
  pmd->ssgeff_priority = 0;
  pmd->ssgeff_num = 0xff;
  pmd->ssgeff_on = false;
}

// 0def
static void pmd_ssgeff_advance(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  const struct pmd_ssgeff_data *ssgeff = pmd->ssgeff_ptr;
  if (ssgeff->wait == 0xff) {
    pmd_ssgeff_stop(work, pmd);
    return;
  }
  pmd->ssgeff_wait = ssgeff->wait;
  pmd->ssgeff_tonefreq = ssgeff->tone_freq;
  work->opna_writereg(work, 0x04, pmd->ssgeff_tonefreq);
  work->opna_writereg(work, 0x05, pmd->ssgeff_tonefreq>>8);
  pmd->ssgeff_noisefreq = ssgeff->noise_freq;
  work->opna_writereg(work, 0x06, pmd->ssgeff_noisefreq);
  pmd->ssg_noise_freq_wrote = pmd->ssgeff_noisefreq;
  uint8_t ssgout = pmd_opna_read_ssgout(work);
  ssgout &= ~((1<<2) | (1<<5));
  ssgout |= ((!ssgeff->tone_mix)<<2) | ((!ssgeff->noise_mix)<<5);
  pmd->ssgeff_tone_mix = ssgeff->tone_mix;
  pmd->ssgeff_noise_mix = ssgeff->noise_mix;
  work->opna_writereg(work, 0x07, ssgout);
  work->opna_writereg(work, 0x0a, ssgeff->env_level);
  work->opna_writereg(work, 0x0b, ssgeff->env_freq);
  work->opna_writereg(work, 0x0c, ssgeff->env_freq>>8);
  work->opna_writereg(work, 0x0d, ssgeff->env_waveform);
  pmd->ssgeff_tonefreq_add = ssgeff->tonefreq_add;
  pmd->ssgeff_noisefreq_add = ssgeff->noisefreq_add;
  pmd->ssgeff_noisefreq_cnt_set = ssgeff->noisefreq_wait;
  pmd->ssgeff_noisefreq_cnt = pmd->ssgeff_noisefreq_cnt_set;
  pmd->ssgeff_ptr++;
}

// 0e8d
static void pmd_ssgeff_tick_idle(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  pmd->ssgeff_tonefreq += pmd->ssgeff_tonefreq_add;
  work->opna_writereg(work, 0x04, pmd->ssgeff_tonefreq);
  work->opna_writereg(work, 0x05, pmd->ssgeff_tonefreq>>8);
  // pmd_opna_read_ssgout(work);
  if (!pmd->ssgeff_noisefreq_add && !pmd->ssgeff_noisefreq_cnt_set) return;
  if (--pmd->ssgeff_noisefreq_cnt) return;
  pmd->ssgeff_noisefreq_cnt = pmd->ssgeff_noisefreq_cnt_set;
  pmd->ssgeff_noisefreq += pmd->ssgeff_noisefreq_add;
  work->opna_writereg(work, 0x06, pmd->ssgeff_noisefreq);
  pmd->ssg_noise_freq_wrote = pmd->ssgeff_noisefreq;
}

// 0dde
static void pmd_ssgeff_tick(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  if (--pmd->ssgeff_wait) {
    pmd_ssgeff_tick_idle(work, pmd);
  } else {
    pmd_ssgeff_advance(work, pmd);
  }
}

// 2dda
static uint8_t *pmd_get_toneptr(
  struct driver_pmd *pmd,
  uint8_t tonenum
) {
  if (pmd->tone_included) {
    for (uint16_t toneptr = pmd->tone_ptr; ; toneptr += 0x1a) {
      if (pmd->datalen < (toneptr+0x1a)) return 0;
      if (pmd->data[toneptr] == tonenum) {
        return &pmd->data[toneptr+1];
      }
    }
  }
  // TODO: external tone
  return 0;
}

// 2cbc
static void pmd_part_do_keyoff_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint8_t out = pmd->proc_ch-1;
  int ch_ind = pmd->proc_ch-1;
  if (pmd->opna_a1) {
    ch_ind += 3;
    out |= 4;
  }
  pmd->fm_slotkey[ch_ind] &= ~part->fm_slotmask;
  out |= pmd->fm_slotkey[ch_ind];
  work->opna_writereg(work, 0x28, out);
}

// 2e15
static bool pmd_part_tone_slotmask_stop(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint8_t mask = part->fm_tone_slotmask;
  if (!mask) return false;
  uint8_t addr = 0x3f + pmd->proc_ch;
  for (int s = 0; s < 4; s++, addr += 4) {
    if (!(mask & (1<<(7-s)))) continue;
    // TL
    pmd_reg_write(work, pmd, addr, 0x7f);
    // SL/RR (why 0x7f instead of 0xff?)
    pmd_reg_write(work, pmd, addr+0x40, 0x7f);
  }
  pmd_part_do_keyoff_fm(work, pmd, part);
  return true;
}

// 3ee7
static const uint8_t pmd_alg_output_slot_table[8] = {
  0x80, 0x80, 0x80, 0x80, 0xa0, 0xe0, 0xe0, 0xf0
};
// 3eef
static const uint8_t pmd_alg_tone_slot_table[8] = {
  0xee, 0xee, 0xee, 0xee, 0xcc, 0x88, 0x88, 0x00
};

// 2dce
static void pmd_part_tone_tl_update(
  struct pmd_part *part,
  uint8_t const *toneptr
) {
  for (int s = 0; s < 4; s++) {
    part->fm_tone_tl[s] = toneptr[0x04+s];
  }
}

// 2d0d
static void pmd_part_tone_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint8_t const * const toneptr = pmd_get_toneptr(pmd, part->tonenum);
  if (!toneptr) return;
  if (!pmd_part_tone_slotmask_stop(work, pmd, part)) {
    // 2d15
    //toneptr += 4;
  } else {
    // 2d1b
    uint8_t fb_alg = toneptr[0x18];
    if (pmd->tonemask_fb_alg) fb_alg = part->fb_alg;
    if (!pmd->opna_a1 && pmd->proc_ch == 3) {
      if (pmd->tonemask_fb_alg) {
        // 2d43
        fb_alg = pmd->fm3ex_fb_alg;
      } else {
        // 2d49
        if (!(part->fm_slotmask & 0x10)) {
          fb_alg &= 0x7;
          fb_alg |= pmd->fm3ex_fb_alg & 0x38;
        }
        // 2d59
        pmd->fm3ex_fb_alg = fb_alg;
      }
    }
    // 2d5d
    pmd_reg_write(work, pmd, 0xaf+pmd->proc_ch, fb_alg);
    part->fb_alg = fb_alg;
    uint8_t outslot = pmd_alg_output_slot_table[fb_alg&7];
    if (!(part->vol_lfo_slotmask & 0xf)) {
      part->vol_lfo_slotmask = outslot;
    }
    if (!(part->vol_lfo_slotmask_b & 0xf)) {
      part->vol_lfo_slotmask_b = outslot;
    }
    // 2d83
    part->fm_slotout = outslot;
    uint8_t tone_slotmask = part->fm_tone_slotmask;
    uint8_t tl_slotmask = pmd_alg_tone_slot_table[fb_alg&7] & tone_slotmask;
    for (int s = 0; s < 4; s++) {
      if (tone_slotmask & (1<<(7-s))) {
        pmd_reg_write(work, pmd, 0x2f+pmd->proc_ch+s*4, toneptr[0x00+s]);
      }
    }
    for (int s = 0; s < 4; s++) {
      if (tl_slotmask & (1<<(7-s))) {
        pmd_reg_write(work, pmd, 0x3f+pmd->proc_ch+s*4, toneptr[0x04+s]);
      }
    }
    for (int s = 0; s < 4; s++) {
      if (tone_slotmask & (1<<(3-s))) {
        pmd_reg_write(work, pmd, 0x4f+pmd->proc_ch+s*4, toneptr[0x08+s]);
        pmd_reg_write(work, pmd, 0x6f+pmd->proc_ch+s*4, toneptr[0x10+s]);
      }
      if (tone_slotmask & (1<<(7-s))) {
        pmd_reg_write(work, pmd, 0x5f+pmd->proc_ch+s*4, toneptr[0x0c+s]);
        pmd_reg_write(work, pmd, 0x7f+pmd->proc_ch+s*4, toneptr[0x14+s]);
      }
    }
  }
  // 2dce
  pmd_part_tone_tl_update(part, toneptr);
}

#include "pmd_ssgeff.h"

// 0d28, 0d09
static void pmd_ssg_effect(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  uint8_t effnum
) {
  if (effnum > PMD_SSGEFF_CNT) return;
  pmd->ssgeff_internal = true;
  if (pmd->ssgeff_disabled) return;
  // TODO: PPSDRV
  // 0da4
  pmd->ssgeff_num = effnum;
  // 3ef7
  if (pmd->ssgeff_priority > pmd_ssgeff_table[effnum].priority) return;
  // TODO: PPSDRV
  // 0dc5
  pmd->ssgeff_ptr = pmd_ssgeff_table[effnum].data;
  pmd->parts[PMD_PART_SSG_3].mask.effect = true;
  pmd_ssgeff_advance(work, pmd);
  pmd->ssgeff_priority = pmd_ssgeff_table[effnum].priority;
  pmd->ssgeff_on = true;
}

// 1a58
static void pmd_cmd_null_16(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)work;
  for (int i = 0; i < 16; i++) {
    pmd_part_cmdload(pmd, part);
  }
}

// 1a5b
static void pmd_cmd_null_6(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)work;
  for (int i = 0; i < 6; i++) {
    pmd_part_cmdload(pmd, part);
  }
}

// 1a5c
static void pmd_cmd_null_5(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)work;
  for (int i = 0; i < 5; i++) {
    pmd_part_cmdload(pmd, part);
  }
}

// 1a5d
static void pmd_cmd_null_4(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)work;
  for (int i = 0; i < 4; i++) {
    pmd_part_cmdload(pmd, part);
  }
}

// 1a5e
static void pmd_cmd_null_3(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)work;
  for (int i = 0; i < 3; i++) {
    pmd_part_cmdload(pmd, part);
  }
}

// 1a5f
static void pmd_cmd_null_2(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)work;
  for (int i = 0; i < 2; i++) {
    pmd_part_cmdload(pmd, part);
  }
}

// 1a60
static void pmd_cmd_null_1(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)work;
  pmd_part_cmdload(pmd, part);
}

// 1a61
static void pmd_cmd_null_0(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)work;
  (void)pmd;
  (void)part;
}

// 271a
static uint8_t pmd_part_note_transpose(
  struct pmd_part *part,
  uint8_t note
) {
  if (note == 0x0f) return note;
  int8_t transpose = u8s8((uint8_t)(part->transpose + part->transpose_master));
  if (!transpose) return note;
  uint8_t octave = note >> 4;
  note &= 0x0f;
  if (transpose < 0) {
    int newnote = note + transpose;
    while (newnote < 0) {
      octave--;
      newnote += 0xc;
    }
    note = newnote;
  } else {
    int newnote = note + transpose;
    while (newnote >= 0xc) {
      octave++;
      newnote -= 0xc;
    }
    note = newnote;
  }
  return octave<<4 | note;
}

// 2771
static void pmd_note_freq_fm(
  struct pmd_part *part,
  uint8_t note
) {
  if ((note & 0xf) == 0xf) {
    // 2798
    part->actual_note = 0xff;
    if (!part->lfof.freq && !part->lfof_b.freq) {
      part->actual_freq = 0;
    }
    return;
  }
  // 277b
  part->actual_note = note;
  static const uint16_t fm_tonetable[0x10] = {
    // 3eb7
    // round(144*440*(2**((-9+i)/12))*(1<<17)/7987200)
    0x026a,
    0x028f,
    0x02b6,
    0x02df,
    0x030b,
    0x0339,
    0x036a,
    0x039e,
    0x03d5,
    0x0410,
    0x044e,
    0x048f,
    0, 0, 0, 0
  };
  uint16_t freq = fm_tonetable[note & 0x0f];
  freq |= (note & 0x70) << 7;
  part->actual_freq = freq;
}

// 27a8
static void pmd_note_freq_ssg(
  struct pmd_part *part,
  uint8_t note
) {
  if ((note & 0xf) == 0xf) {
    part->actual_note = 0xff;
    if (!part->lfof.freq && !part->lfof_b.freq) {
      part->actual_freq = 0;
    }
    return;
  }
  part->actual_note = note;
  static const uint16_t ssg_tonetable[0x10] = {
    // 3ecf
    // round(7987200/(64*440*(2**-4)*(2.0**((3+i)/12))))
    0x0ee8,
    0x0e12,
    0x0d48,
    0x0c89,
    0x0bd5,
    0x0b2b,
    0x0a8a,
    0x09f3,
    0x0964,
    0x08dd,
    0x085e,
    0x07e6,
    0, 0, 0, 0
  };
  uint8_t octave = note >> 4;
  uint16_t tonefreq = ssg_tonetable[note & 0x0f];
  if (octave) {
    tonefreq >>= (octave-1);
    bool inc = tonefreq & 1;
    tonefreq >>= 1;
    tonefreq += inc;
  }
  part->actual_freq = tonefreq;
}

// 0671
static void pmd_note_freq_adpcm(
  struct pmd_part *part,
  uint8_t note
) {
  if ((note & 0xf) == 0xf) {
    // 2798
    part->actual_note = 0xff;
    if (!part->lfof.freq && !part->lfof_b.freq) {
      part->actual_freq = 0;
    }
    return;
  }
  part->actual_note = note;
  static const uint16_t adpcm_tonetable[0x10] = {
    // 0788
    // ???
    // different from round((16000*2*0x10000/(8000000/144))*(2**((i-7)/12)))
    0x6264,
    0x6840,
    0x6e74,
    0x7506,
    0x7bfc,
    0x835e,
    0x8b2e,
    0x9376,// g
    0x9c3c,
    0xa588,
    0xaf62,
    0xb9d0,
    0, 0, 0, 0
  };
  uint8_t octave = note >> 4;
  uint16_t tonefreq = adpcm_tonetable[note & 0x0f];
  if (octave <= 5) {
    tonefreq >>= (5 - octave);
  } else {
    // 06a8
    octave = 5;
    if (!(tonefreq & 0x8000)) {
      tonefreq <<= 1;
      octave++;
    }
    part->actual_note = (part->actual_note & 0xf) | (octave << 4);
  }
  part->actual_freq = tonefreq;
}

// 0c97
static void pmd_note_freq_ppz8(
  struct pmd_part *part,
  uint8_t note
) {
  if ((note & 0xf) == 0xf) {
    // 0cd8
    part->actual_note = 0xff;
    if (!part->lfof.freq && !part->lfof_b.freq) {
      part->actual_freq = 0;
      part->actual_freq_upper = 0;
    }
    return;
  }
  part->actual_note = note;
  static const uint16_t ppz8_tonetable[0x10] = {
    // 0cf1
    // documented in ppz8.doc
    // different from round(0x8000*(2**(i/12)))
    0x8000,
    0x87a6,
    0x8fb3,
    0x9838,
    0xa146,
    0xaade,
    0xb4ff,
    0xbfcc,
    0xcb34,
    0xd747,
    0xe418,
    0xf1a5,
    0, 0, 0, 0
  };
  int octave = (note >> 4) - 4;
  uint32_t freq = ppz8_tonetable[note & 0x0f];
  if (octave < 0) {
    freq >>= -octave;
  } else {
    freq <<= octave;
  }
  part->actual_freq = freq;
  part->actual_freq_upper = freq >> 16;
}

// 14a0
static void pmd_part_calc_gate(
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (pmd->datalen < (part->ptr + 1)) {
    if (pmd->data[part->ptr] == 0xc1) {
      part->ptr++;
      part->gate = 0;
      return;
    }
  }
  uint8_t gate = part->gate_abs;
  if (part->gate_rel) {
    // 14ae
    gate += ((uint16_t)part->len_cnt * part->gate_rel) >> 8;
  }
  // 14b8
  if (part->gate_rand_range) {
    // 14be
    uint16_t range = part->gate_rand_range & 0x7f;
    range++;
    uint16_t rand = pmd_rand(pmd, range);
    if (part->gate_rand_range & 0x80) {
      gate -= rand;
    } else {
      gate += rand;
    }
  }
  // 14de
  if (part->gate_min) {
    int len = part->len_cnt - part->gate_min;
    if (len < 0) {
      part->gate = 0;
      return;
    }
    if (gate >= len) {
      gate = len;
    }
  }
  // 14f2
  part->gate = gate;
}

// 2b4c
// bl = slotmask
// al = vol
static void pmd_fm_vol_out_sub(
  uint8_t slotmask,
  uint8_t *table,
  uint8_t vol
) {
  for (int s = 0; s < 4; s++) {
    if (!(slotmask & (1<<(7-s)))) continue;
    if (!(vol & 0x80)) {
      int newvol = table[s] - vol;
      if (newvol < 0) newvol = 0;
      table[s] = newvol;
    } else {
      int newvol = table[s] + (uint8_t)(-vol);
      if (newvol > 0xff) newvol = 0xff;
      table[s] = newvol;
    }
  }
}

// 2a38
static void pmd_fm_vol_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint8_t slotmask = part->fm_slotmask;
  if (!slotmask) return;
  uint8_t vol = part->volume_save;
  if (vol) {
    vol--;
  } else {
    vol = part->vol;
  }
  // 2a4e
  if (part != &pmd->parts[PMD_PART_FM_EFF]) {
    // 2a56
    if (pmd->fm_voldown) {
      uint8_t voldown = -pmd->fm_voldown;
      vol = (vol * voldown) >> 8;
    }
    if (pmd->fadeout_vol) {
      uint8_t fadeout = -pmd->fadeout_vol;
      vol = (vol * fadeout) >> 8;
    }
  }
  // 2a72
  // 4, 3, 2, 1
  uint8_t table[4] = {
    0x80, 0x80, 0x80, 0x80
  };
  vol ^= 0xff;
  uint8_t oslotmask = slotmask & part->fm_slotout;
  uint8_t update_slots = oslotmask;
  for (int s = 0; s < 4; s++) {
    if (oslotmask & (1<<(7-s))) table[s] = vol;
  }
  // 2aa8
  if (vol != 0xff) {
    if (part->lfof.vol) {
      uint8_t lslotmask = slotmask & part->vol_lfo_slotmask;
      update_slots |= lslotmask;
      pmd_fm_vol_out_sub(lslotmask, table, part->lfo_diff);
    }
    if (part->lfof_b.vol) {
      uint8_t lslotmask = slotmask & part->vol_lfo_slotmask_b;
      update_slots |= lslotmask;
      pmd_fm_vol_out_sub(lslotmask, table, part->lfo_diff_b);
    }
  }
  // 2ad3
  for (int s = 0; s < 4; s++) {
    if (!(update_slots & (1<<(7-s)))) continue;
    static const uint8_t tladdr[4] = {
      0x4b, 0x43, 0x47, 0x3f
    };
    static const uint8_t toneind[4] = {
      3, 1, 2, 0
    };
    int sout = table[s] + part->fm_tone_tl[toneind[s]];
    if (sout > 0xff) sout = 0xff;
    sout -= 0x80;
    if (sout < 0) sout = 0;
    pmd_reg_write(work, pmd, tladdr[s]+pmd->proc_ch, sout);
  }
}

// 2b70
static void pmd_ssg_vol_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->ssg_env_state_old == SSG_ENV_STATE_OLD_OFF ||
      (part->ssg_env_state_old == SSG_ENV_STATE_OLD_NEW &&
       part->ssg_env_state_new == SSG_ENV_STATE_NEW_OFF)) {
    return;
  }
  uint8_t vol = part->volume_save;
  if (vol) {
    vol--;
  } else {
    vol = part->vol;
  }
  // 2b91
  if (pmd->ssg_voldown) {
    unsigned voldown = (uint8_t)(-pmd->ssg_voldown);
    vol = vol * voldown >> 8;
  }
  if (pmd->fadeout_vol) {
    unsigned fadeout = (uint8_t)(-pmd->fadeout_vol);
    vol = vol * fadeout >> 8;
  }
  // 2bad
  if (vol) {
    if (part->ssg_env_state_old == SSG_ENV_STATE_OLD_NEW) {
      // 2bb7
      unsigned envvol = part->ssg_env_vol;
      if (!envvol) {
        // 2bd9
        vol = 0;
        // -> 2c0f
      } else {
        vol = ((vol * (envvol+1)) & 0xff) >> 3;
        if (vol & 1) {
          vol >>= 1;
          vol++;
        } else {
          vol >>= 1;
        }
      }
    } else {
      // 2bd4
      vol += part->ssg_env_vol;
      if (vol & 0x80) vol = 0; // -> 2c0f
      if (vol > 0xf) vol = 0xf;
    }
    if (vol) {
      // 2be4
      if (part->lfof.vol || part->lfof_b.vol) {
        int32_t lfovol = 0;
        // TODO:
        if (part->lfof.vol) lfovol += part->lfo_diff;
        if (part->lfof_b.vol) lfovol += part->lfo_diff_b;
        lfovol += vol;
        vol = lfovol;
        if (lfovol < 0) {
          vol = 0;
        } else if (lfovol > 0xf) {
          vol = 0xf;
        }
      }
    }
  }
  // 2c0f
  work->opna_writereg(work, 0x07+pmd->proc_ch, vol);
}

// 049a
static void pmd_adpcm_vol_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint8_t vol = part->volume_save;
  if (!vol) vol = part->vol;
  
  // 04a4
  if (pmd->adpcm_voldown) {
    uint8_t voldown = -pmd->adpcm_voldown;
    vol = vol * voldown >> 8;
  }
  // 04b3
  if (pmd->fadeout_vol) {
    uint8_t fadeout = -pmd->fadeout_vol;
    fadeout = ((uint16_t)fadeout * fadeout) >> 8;
    vol = vol * fadeout >> 8;
  }
  if (vol) {
    if (part->ssg_env_state_old == SSG_ENV_STATE_OLD_NEW) {
      // 04d0
      uint8_t envvol = part->ssg_env_vol;
      if (!envvol) {
        // 04fd
        vol = 0;
        // -> 053f
      } else {
        int ivol = (vol * (envvol+1)) >> 3;
        if (ivol & 1) {
          ivol >>= 1;
          ivol++;
        } else {
          ivol >>= 1;
        }
        vol = ivol;
      }
    } else {
      // 04e8
      int newvol = vol + (u8s8(part->ssg_env_vol) << 4);
      if (newvol > 0xff) newvol = 0xff;
      if (newvol < 0) newvol = 0;
      vol = newvol;
    }
    if (vol) {
      if (part->lfof.vol || part->lfof_b.vol) {
        int32_t lfovol = 0;
        if (part->lfof.vol) vol += part->lfo_diff;
        if (part->lfof_b.vol) vol += part->lfo_diff_b;
        lfovol += vol;
        vol = lfovol;
        if (lfovol < 0) {
          vol = 0;
        } else if (lfovol > 0xff) {
          vol = 0xff;
        }
      }
    }
  }
  // 053f
  work->opna_writereg(work, 0x10b, vol);
}

// 0b33
static void pmd_ppz8_vol_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint8_t vol = part->volume_save;
  if (!vol) vol = part->vol;
  
  // 0b3f
  if (pmd->ppz8_voldown) {
    uint8_t voldown = -pmd->ppz8_voldown;
    vol = vol * voldown >> 8;
  }
  // 0b4c
  if (pmd->fadeout_vol) {
    uint8_t fadeout = -pmd->fadeout_vol;
    vol = vol * fadeout >> 8;
  }
  // 0b59
  if (vol) {
    if (part->ssg_env_state_old == SSG_ENV_STATE_OLD_NEW) {
      uint8_t envvol = part->ssg_env_vol;
      if (!envvol) {
        vol = 0;
        // -> 0bd4
      } else {
        int ivol = (vol * (envvol+1)) >> 3;
        if (ivol & 1) {
          ivol >>= 1;
          ivol++;
        } else {
          ivol >>= 1;
        }
        vol = ivol;
      }
    } else {
      // 0b7d
      int newvol = vol + (u8s8(part->ssg_env_vol) << 4);
      if (newvol > 0xff) newvol = 0xff;
      if (newvol < 0) newvol = 0;
      vol = newvol;
    }
    // 0ba4
    if (part->lfof.vol || part->lfof_b.vol) {
      int32_t lfovol = 0;
      if (part->lfof.vol) lfovol += part->lfo_diff;
      if (part->lfof_b.vol) lfovol += part->lfo_diff_b;
      lfovol += vol;
      vol = lfovol;
      if (lfovol < 0) {
        vol = 0;
      } else if (lfovol > 0xff) {
        vol = 0xff;
      }
    }
  }
  // 0bd4
  if (work->ppz8) {
    if (vol) {
      work->ppz8_functbl->channel_volume(work->ppz8, pmd->proc_ch, vol >> 4);
    } else {
      work->ppz8_functbl->channel_stop(work->ppz8, pmd->proc_ch);
    }
  }
}

// 2985
static void pmd_ssg_freq_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint16_t freq = part->actual_freq;
  if (!freq) return;
  freq += (unsigned)part->portamento_diff;
  if (!part->flagext.detune) {
    // 2996
    freq += (unsigned)(-part->detune);
    if (part->lfof.freq) {
      freq += (unsigned)(-part->lfo_diff);
    }
    if (part->lfof_b.freq) {
      freq += (unsigned)(-part->lfo_diff_b);
    }
  } else {
    // 29ad
    if (part->detune) {
      int32_t det = (((int32_t)freq) * ((int32_t)part->detune)) >> 12;
      if (det >= 0) {
        det++;
      } else {
        det--;
      }
      freq += (uint16_t)(-det);
    }
    if (part->lfof.freq || part->lfof_b.freq) {
      // 29db
      int32_t lfo = 0;
      if (part->lfof.freq) lfo += part->lfo_diff;
      if (part->lfof_b.freq) lfo += part->lfo_diff_b;
      if (lfo) {
        lfo = (((int32_t)freq) * lfo) >> 12;
        if (lfo >= 0) {
          lfo++;
        } else {
          lfo--;
        }
        freq += (uint16_t)(-lfo);
      }
    }
    // 2a0d
  }
  // 2a10
  if (freq >= 0x8000) freq = 0;
  if (freq >= 0x1000) freq = 0x0fff;
  // 2a20
  part->output_freq = freq;
  work->opna_writereg(work, (pmd->proc_ch-1)*2, freq);
  work->opna_writereg(work, (pmd->proc_ch-1)*2+1, freq>>8);
}

// 0620
static void pmd_adpcm_freq_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)pmd;
  uint16_t freq = part->actual_freq;
  if (!freq) return;
  freq += (unsigned)part->portamento_diff;
  uint32_t det = 0;
  if (part->lfof.freq || part->lfof_b.freq) {
    // 29db
    if (part->lfof.freq) det += part->lfo_diff;
    if (part->lfof_b.freq) det += part->lfo_diff_b;
    det <<= 2;
  }
  // 0649
  det += part->detune;
  int32_t newfreq = freq + u16s16(det);
  if (newfreq > 0xffff) newfreq = 0xffff;
  if (newfreq < 0) newfreq = 0;
  freq = newfreq;
  part->output_freq = freq;
  work->opna_writereg(work, 0x109, freq);
  work->opna_writereg(work, 0x10a, freq >> 8);
}

// 0c27
static void pmd_ppz8_freq_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint32_t freq = part->actual_freq | (((uint32_t)part->actual_freq_upper) << 16);
  if (!freq) return;
  if (part->portamento_diff) {
    freq += part->portamento_diff << 4;
  }
  int32_t det = 0;
  if (part->lfof.freq || part->lfof_b.freq) {
    if (part->lfof.freq) det += part->lfo_diff;
    if (part->lfof_b.freq) det += part->lfo_diff_b;
  }
  // 0c6a
  det += part->detune;
  det *= freq >> 8;
  int64_t outfreq = freq + det;
  if (outfreq < 0) outfreq = 0;
  if (outfreq > INT32_MAX) outfreq = INT32_MAX;
  part->output_freq = outfreq;
  if (work->ppz8) {
    work->ppz8_functbl->channel_freq(work->ppz8, pmd->proc_ch, outfreq);
  }
}

// 2c9f
static uint16_t pmd_part_ssg_readout(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  uint16_t ret = 0x0900<<(pmd->proc_ch-1);
  ret |= pmd_opna_read_ssgout(work);
  return ret;
}

// 2c67
static void pmd_ssg_note_noise_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->actual_note == 0xff) return;
  uint16_t noteout = pmd_part_ssg_readout(work, pmd);
  uint8_t ssgout = noteout | (noteout >> 8);
  uint8_t pmdout = (noteout >> 8) & part->ssg_mix;
  ssgout &= ~pmdout;
  work->opna_writereg(work, 0x07, ssgout);
  if (pmd->ssg_noise_freq != pmd->ssg_noise_freq_wrote) {
    if (pmd->ssgeff_num & 0x80) {
      work->opna_writereg(work, 0x06, pmd->ssg_noise_freq);
      pmd->ssg_noise_freq_wrote = pmd->ssg_noise_freq;
    }
  }
}

// 148b
static void pmd_part_loop_check(
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  pmd->loop.looped &= part->loop.looped;
  pmd->loop.ended &= part->loop.ended;
  pmd->loop.env &= part->loop.env;
}

// 2c19
static void pmd_part_fm_keyon(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->actual_note == 0xff) return;
  uint8_t *slotkey = &pmd->fm_slotkey[pmd->proc_ch-1+(3*pmd->opna_a1)];
  *slotkey |= part->fm_slotmask;
  if (part->slot_delay_cnt) {
    *slotkey &= part->slot_delay_mask;
  }
  work->opna_writereg(work, 0x28, *slotkey | (pmd->proc_ch-1) | (pmd->opna_a1<<2));
}

// 0547
static void pmd_part_adpcm_keyon(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->actual_note == 0xff) return;
  work->opna_writereg(work, 0x101, 0x02);
  work->opna_writereg(work, 0x100, 0x21);
  work->opna_writereg(work, 0x102, pmd->adpcm_start);
  work->opna_writereg(work, 0x103, pmd->adpcm_start >> 8);
  work->opna_writereg(work, 0x104, pmd->adpcm_stop);
  work->opna_writereg(work, 0x105, pmd->adpcm_stop >> 8);
  if (!pmd->adpcm_start_loop && !pmd->adpcm_stop_loop) {
    work->opna_writereg(work, 0x100, 0xa0);
    work->opna_writereg(work, 0x101, part->pan | 0x02);
  } else {
    work->opna_writereg(work, 0x100, 0xb0);
    work->opna_writereg(work, 0x101, part->pan | 0x02);
    work->opna_writereg(work, 0x102, pmd->adpcm_start_loop);
    work->opna_writereg(work, 0x103, pmd->adpcm_start_loop >> 8);
    work->opna_writereg(work, 0x104, pmd->adpcm_stop_loop);
    work->opna_writereg(work, 0x105, pmd->adpcm_stop_loop >> 8);
  }
}

// 0bf6
static void pmd_part_ppz8_keyon(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->actual_note == 0xff) return;
  if (work->ppz8) {
    work->ppz8_functbl->channel_play(work->ppz8, pmd->proc_ch, part->tonenum);
  }
}

// 2942
static uint32_t pmd_blkfnum_normalize(
  uint16_t blk,
  uint16_t fnum
) {
  for (;;) {
    if ((fnum & 0x8000) || (fnum < 0x26a)) {
      int32_t iblk = (int32_t)blk - 0x0800;
      if (iblk < 0) {
        // 2976
        blk = 0;
        if (fnum & 0x8000) fnum = 0x0008;
        if (fnum < 0x0008) fnum = 0x0008;
        break;
      }
      blk = iblk;
      fnum += 0x26a;
      continue;
    }
    if (fnum < 0x4d4) break;
    // 2950
    blk += 0x0800;
    if (blk != 0x4000) {
      fnum -= 0x26a;
      continue;
    }
    blk = 0x3800;
    if (fnum > 0x07ff) fnum = 0x7ff;
    break;
  }
  return (((uint32_t)blk) << 16) | fnum;
}

// 27d8
static void pmd_fm_freq_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint16_t fnum = part->actual_freq;
  if (!fnum) return;
  if (!part->fm_slotmask) return;
  uint16_t blk = fnum & 0x3800;
  fnum &= 0x7ff;
  fnum += (uint16_t)part->portamento_diff;
  fnum += (uint16_t)part->detune;
  if (pmd->proc_ch != 3 || pmd->opna_a1 || pmd->fm3ex_mode == 0x3f) {
    // 280c
    if (part->lfof.freq) fnum += (uint16_t)part->lfo_diff;
    if (part->lfof_b.freq) fnum += (uint16_t)part->lfo_diff_b;
    uint32_t blkfnum = pmd_blkfnum_normalize(blk, fnum);
    blkfnum |= blkfnum >> 16;
    blkfnum &= 0xffff;
    part->output_freq = blkfnum;
    pmd_reg_write(work, pmd, pmd->proc_ch + 0xa3, blkfnum >> 8);
    pmd_reg_write(work, pmd, pmd->proc_ch + 0x9f, blkfnum);
    
  } else {
    // 2837
    uint8_t fm_slotmask = part->fm_slotmask;
    uint8_t lfo_slotmask = part->vol_lfo_slotmask;
    if (!(lfo_slotmask & 0x0f)) {
      lfo_slotmask = 0xf0;
    }
    uint8_t lfo_slotmask_b = part->vol_lfo_slotmask_b;
    if (!(lfo_slotmask_b & 0x0f)) {
      lfo_slotmask_b = 0xf0;
    }
    static const uint8_t slot_addr[2][4] = {
      {0xad, 0xae, 0xac, 0xa6}, {0xa9, 0xaa, 0xa8, 0xa2}
    };
    for (int s = 3; s >= 0; s--) {
      const uint8_t slot_bit = (1 << (4+s));
      if (fm_slotmask & slot_bit) {
        // 2858
        uint16_t fnums = fnum + (uint16_t)pmd->fm3ex_det[s];
        if ((lfo_slotmask & slot_bit) && part->lfof.freq) fnums += (uint16_t)part->lfo_diff;
        if ((lfo_slotmask_b & slot_bit) && part->lfof_b.freq) fnums += (uint16_t)part->lfo_diff_b;
        uint32_t blkfnum = pmd_blkfnum_normalize(blk, fnums);
        blkfnum |= blkfnum >> 16;
        blkfnum &= 0xffff;
        part->output_freq = blkfnum;
        pmd_reg_write(work, pmd, slot_addr[0][s], blkfnum >> 8);
        pmd_reg_write(work, pmd, slot_addr[1][s], blkfnum);
      }
    }
  }
}

// 13b5
static void pmd_part_fm_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->volume_save && part->actual_note != 0xff) {
    if (!pmd->volume_saved) {
      part->volume_save = 0;
    }
    pmd->volume_saved = false;
  }
  pmd_fm_vol_out(work, pmd, part);
  pmd_fm_freq_out(work, pmd, part);
  pmd_part_fm_keyon(work, pmd, part);
  part->note_proc++;
  pmd->no_keyoff = false;
  pmd->volume_saved = false;
  part->keystatus.off = false;
  part->keystatus.off_mask = false;
  if (pmd->datalen > (part->ptr + 1)) {
    if (pmd->data[part->ptr] == 0xfb) {
      part->keystatus.off_mask = true;
    }
  }
  pmd_part_loop_check(pmd, part);
}

// 15d4
static void pmd_part_ssg_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->volume_save && part->actual_note != 0xff) {
    if (!pmd->volume_saved) {
      part->volume_save = 0;
    }
    pmd->volume_saved = false;
  }
  // 15ef
  pmd_ssg_vol_out(work, pmd, part);
  pmd_ssg_freq_out(work, pmd, part);
  pmd_ssg_note_noise_out(work, pmd, part);
  part->note_proc++;
  pmd->no_keyoff = false;
  pmd->volume_saved = 0;
  part->keystatus.off = false;
  part->keystatus.off_mask = false;
  if (pmd->datalen > (part->ptr + 1)) {
    if (pmd->data[part->ptr] == 0xfb) {
      part->keystatus.off_mask = true;
    }
  }
  pmd_part_loop_check(pmd, part);
}

// 01b1
static void pmd_part_adpcm_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->volume_save && part->actual_note != 0xff) {
    if (!pmd->volume_saved) {
      part->volume_save = 0;
    }
    pmd->volume_saved = false;
  }
  pmd_adpcm_vol_out(work, pmd, part);
  pmd_adpcm_freq_out(work, pmd, part);
  if (part->keystatus.off) {
    pmd_part_adpcm_keyon(work, pmd, part);
  }
  part->note_proc++;
  pmd->no_keyoff = false;
  pmd->volume_saved = false;
  part->keystatus.off = false;
  part->keystatus.off_mask = false;
  if (pmd->datalen > (part->ptr + 1)) {
    if (pmd->data[part->ptr] == 0xfb) {
      part->keystatus.off_mask = true;
    }
  }
  pmd_part_loop_check(pmd, part);
}

// 0803
static void pmd_part_ppz8_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->volume_save && part->actual_note != 0xff) {
    if (!pmd->volume_saved) {
      part->volume_save = 0;
    }
    pmd->volume_saved = false;
  }
  pmd_ppz8_vol_out(work, pmd, part);
  pmd_ppz8_freq_out(work, pmd, part);
  if (part->keystatus.off) {
    pmd_part_ppz8_keyon(work, pmd, part);
  }
  // 082d
  part->note_proc++;
  pmd->no_keyoff = false;
  pmd->volume_saved = false;
  part->keystatus.off = false;
  part->keystatus.off_mask = false;
  if (pmd->datalen > (part->ptr + 1)) {
    if (pmd->data[part->ptr] == 0xfb) {
      part->keystatus.off_mask = true;
    }
  }
  pmd_part_loop_check(pmd, part);
}

// none
static bool pmd_part_masked(
  const struct pmd_part *part
) {
  return part->mask.ext || part->mask.effect || part->mask.disabled
    || part->mask.slot || part->mask.mml || part->mask.ff;
}

// 1f7e
static void pmd_fm_freq_out_mask(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (!pmd_part_masked(part)) {
    pmd_fm_freq_out(work, pmd, part);
  }
}

// 1ee3
static void pmd_fm3ex_mode_update(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  int partbit;
  if (part == &pmd->parts[PMD_PART_FM_3]) partbit = 1;
  else if (part == &pmd->parts[PMD_PART_FM_3B]) partbit = 2;
  else if (part == &pmd->parts[PMD_PART_FM_3C]) partbit = 4;
  else partbit = 8;

  bool fm3ex;
  if (!(part->fm_slotmask & 0xf0)) fm3ex = false;
  else if (part->fm_slotmask != 0xf0) fm3ex = true;
  else if (!(part->vol_lfo_slotmask & 0x0f)) fm3ex = false;
  else if (part->lfof.freq) fm3ex = true;
  else if (!(part->vol_lfo_slotmask_b & 0x0f)) fm3ex = false;
  else if (part->lfof_b.freq) fm3ex = true;
  else fm3ex = false;
  if (!fm3ex) {
    pmd->fm3ex_needed &= ~partbit;
  } else {
    pmd->fm3ex_needed |= partbit;
  }
  uint8_t fm3ex_mode;
  if (pmd->fm3ex_needed || pmd->fm3ex_force) {
    fm3ex_mode = 0x7f;
  } else {
    fm3ex_mode = 0x3f;
  }
  if (fm3ex_mode == pmd->fm3ex_mode) return;
  pmd->fm3ex_mode = fm3ex_mode;
  // output to 0x27 to update fm3exmode
  // but mask with 0xcf to prevent timer resetting
  work->opna_writereg(work, 0x27, fm3ex_mode & 0xcf);
  if (fm3ex_mode == 0x3f) return;
  if (part == &pmd->parts[PMD_PART_FM_3]) return;
  pmd_fm_freq_out_mask(work, pmd, &pmd->parts[PMD_PART_FM_3]);
  if (part == &pmd->parts[PMD_PART_FM_3B]) return;
  pmd_fm_freq_out_mask(work, pmd, &pmd->parts[PMD_PART_FM_3B]);
  if (part == &pmd->parts[PMD_PART_FM_3C]) return;
  pmd_fm_freq_out_mask(work, pmd, &pmd->parts[PMD_PART_FM_3C]);
}

// 1d7b
static bool pmd_fm3ex_mode_update_check(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (pmd->proc_ch == 3 && !pmd->opna_a1) {
    pmd_fm3ex_mode_update(work, pmd, part);
    return true;
  }
  return false;
}

// 2268
static void pmd_cmdff_tonenum(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t tonenum = pmd_part_cmdload(pmd, part);
  part->tonenum = tonenum;
  if (!pmd_part_masked(part)) {
    pmd_part_tone_fm(work, pmd, part);
    return;
  }
  // 2277
  uint8_t const * const toneptr = pmd_get_toneptr(pmd, tonenum);
  if (!toneptr) return;
  part->fb_alg = toneptr[0x18];
  pmd_part_tone_tl_update(part, toneptr);
  if (pmd->proc_ch == 3 && part->fm_tone_slotmask && !pmd->opna_a1) {
    uint8_t fb_alg = part->fb_alg;
    if (!(part->fm_slotmask & 0x10)) {
      fb_alg &= 0x07;
      fb_alg |= pmd->fm3ex_fb_alg & 0x38;
    }
    pmd->fm3ex_fb_alg = fb_alg;
    part->fb_alg = fb_alg;
  }
}

// 046c
static void pmd_cmdff_tonenum_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t tonenum = pmd_part_cmdload(pmd, part);
  part->tonenum = tonenum;
  pmd->adpcm_start = pmd->adpcm_addr[tonenum][0];
  pmd->adpcm_stop = pmd->adpcm_addr[tonenum][1];
  pmd->adpcm_start_loop = 0;
  pmd->adpcm_stop_loop = 0;
  pmd->adpcm_release = 0x8000;
}

// 0afd
static void pmd_cmdff_tonenum_ppz8(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  part->tonenum = pmd_part_cmdload(pmd, part);
  // 0a3c
  if (work->ppz8) {
    work->ppz8_functbl->channel_loop_voice(work->ppz8, pmd->proc_ch, part->tonenum);
  }
  // set origfreq
}

// 22b3
static void pmd_cmdfe_gate_abs(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->gate_abs = pmd_part_cmdload(pmd, part);
  part->gate_rand_range = 0;
}

// 22cb
static void pmd_cmdfd_vol(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->vol = pmd_part_cmdload(pmd, part);
}

// 22d0
static void pmd_cmdfc_tempo(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t val = pmd_part_cmdload(pmd, part);
  if (val < 0xfb) {
    pmd->timerb = val;
    pmd->timerb_bak = val;
    pmd_calc_tempo(pmd);
  } else if (val == 0xff) {
    // 22e1
    uint8_t tempo = pmd_part_cmdload(pmd, part);
    if (tempo < 0x12) tempo = 0x12;
    pmd->tempo = tempo;
    pmd->tempo_bak = tempo;
    pmd_calc_tempo_rev(pmd);
  } else if (val == 0xfe) {
    // 22f0
    int timerbdiff = u8s8(pmd_part_cmdload(pmd, part));
    int timerb = pmd->timerb_bak + timerbdiff;
    if (timerb > 0xfa) timerb = 0xfa;
    if (timerb < 0) timerb = 0;
    pmd->timerb = timerb;
    pmd->timerb_bak = timerb;
    pmd_calc_tempo(pmd);
  } else {
    // 2313
    int tempodiff = u8s8(pmd_part_cmdload(pmd, part));
    int tempo = pmd->tempo_bak + tempodiff;
    if (tempo > 0xff) tempo = 0xff;
    if (tempo < 0x12) tempo = 0x12;
    pmd->tempo = tempo;
    pmd->tempo_bak = tempo;
    pmd_calc_tempo_rev(pmd);
  }
  work->timerb = pmd->timerb;
}

// 236b
static void pmd_cmdfb_tie(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  (void)part;
  pmd->no_keyoff = true;
}

// 2371
static void pmd_cmdfa_det(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint16_t val = pmd_part_cmdload(pmd, part);
  val |= ((uint16_t)(pmd_part_cmdload(pmd, part)) << 8);
  part->detune = u16s16(val);
}

// 2376
static void pmd_cmdd5_detrel(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint16_t val = pmd_part_cmdload(pmd, part);
  val |= ((uint16_t)(pmd_part_cmdload(pmd, part)) << 8);
  part->detune += u16s16(val);
}

// 237b
static void pmd_cmdf9_repeat_reset(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint16_t ptr = pmd_part_cmdload(pmd, part);
  ptr |= ((uint16_t)(pmd_part_cmdload(pmd, part)) << 8);
  ptr++;
  if (pmd->datalen < (ptr+1)) {
    // TODO: better error handling
    part->ptr = 0;
  } else {
    pmd->data[ptr] = 0;
  }
}

// 2391
static void pmd_cmdf8_repeat(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t repeat = pmd_part_cmdload(pmd, part);
  if (repeat) {
    // 0xf8 repeat cnt ptr
    if (pmd->datalen < (part->ptr+1)) {
      part->ptr = 0;
      return;
    }
    pmd->data[part->ptr]++;
    uint8_t repeatcnt = pmd_part_cmdload(pmd, part);
    if (repeat == repeatcnt) {
      part->ptr += 2;
      return;
    }
  } else {
    // 0xf8 0x00 0x00 ptr 
    part->ptr++;
    part->loop.looped = true;
    part->loop.ended = false;
  }
  // 23a7
  uint16_t ptr = pmd_part_cmdload(pmd, part);
  ptr |= ((uint16_t)(pmd_part_cmdload(pmd, part)) << 8);
  ptr += 2;
  part->ptr = ptr;
}

// 23bd
static void pmd_cmdf7_repeat_exit(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint16_t ptr = pmd_part_cmdload(pmd, part);
  ptr |= ((uint16_t)(pmd_part_cmdload(pmd, part)) << 8);
  if (pmd->datalen < (ptr+2)) {
    part->ptr = 0;
    return;
  }
  uint8_t repeat = pmd->data[ptr];
  uint8_t repeatcnt = pmd->data[ptr+1];
  if (repeatcnt == (repeat-1)) {
    part->ptr = ptr+4;
  }
}

// 23de
static void pmd_cmdf6_set_loop(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  (void)pmd;
  part->loop_ptr = part->ptr;
}

// 23e2
static void pmd_cmdf5_transpose(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->transpose = pmd_part_cmdload(pmd, part);
}

// 23f4
static void pmd_cmdf4_volinc_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  (void)pmd;
  part->vol += 4;
  if (part->vol > 0x7f) part->vol = 0x7f;
}

// 2409
static void pmd_cmdf4_volinc_ssg(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  (void)pmd;
  if (part->vol < 0xf) part->vol++;
}

// 0421
static void pmd_cmdf4_volinc_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  (void)pmd;
  int newvol = part->vol + 0x10;
  if (newvol > 0xff) newvol = 0xff;
  part->vol = newvol;
}

// 241c
static void pmd_cmdf3_voldec_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  (void)pmd;
  int vol = part->vol - 4;
  if (vol < 0) vol = 0;
  part->vol = vol;
}

// 2435
static void pmd_cmdf3_voldec_ssg(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  (void)pmd;
  if (part->vol) part->vol--;
}

// 0434
static void pmd_cmdf3_voldec_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  (void)pmd;
  int newvol = part->vol - 0x10;
  if (newvol < 0) newvol = 0;
  part->vol = newvol;
}

// 24ea
static void pmd_cmdf2_lfo(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->lfo_set.delay = pmd_part_cmdload(pmd, part);
  part->lfo_set.speed = pmd_part_cmdload(pmd, part);
  part->lfo_set.step = pmd_part_cmdload(pmd, part);
  part->lfo_set.times = pmd_part_cmdload(pmd, part);
  part->lfo = part->lfo_set;
  pmd_lfo_reset(part);
}

// 2513
static void pmd_cmdf1_lfo_switch(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t val = pmd_part_cmdload(pmd, part);
  if (val & 0xf8) val = 1;
  part->lfof.freq = val & (1<<0);
  part->lfof.vol = val & (1<<1);
  part->lfof.sync = val & (1<<2);
  pmd_lfo_reset(part);
}

// 2526
static void pmd_cmdf1_lfo_switch_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  pmd_cmdf1_lfo_switch(work, pmd, part);
  pmd_fm3ex_mode_update_check(work, pmd, part);
}

// 1d31
static void pmd_cmdf1_ppsdrv(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  /* uint8_t data = */pmd_part_cmdload(pmd, part);
  // TODO: PPSDRV
}

// 252c
static void pmd_cmdf0_env_old(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t val;
  val = pmd_part_cmdload(pmd, part);
  part->ssg_env_param_set[SSG_ENV_PARAM_OLD_AL] = val;
  part->ssg_env_param[SSG_ENV_PARAM_OLD_AL] = val;
  val = pmd_part_cmdload(pmd, part);
  part->ssg_env_param_set[SSG_ENV_PARAM_OLD_AD] = val;
  val = pmd_part_cmdload(pmd, part);
  part->ssg_env_param_set[SSG_ENV_PARAM_OLD_SR] = val;
  part->ssg_env_param[SSG_ENV_PARAM_OLD_SR] = val;
  val = pmd_part_cmdload(pmd, part);
  part->ssg_env_param_set[SSG_ENV_PARAM_OLD_RR] = val;
  part->ssg_env_param[SSG_ENV_PARAM_OLD_RR] = val;
  if (part->ssg_env_state_old == SSG_ENV_STATE_OLD_NEW) {
    part->ssg_env_state_old = SSG_ENV_STATE_OLD_RR;
    part->ssg_env_vol = -15;
  }
}

// 2554
static void pmd_cmdef_poke(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t reg = pmd_part_cmdload(pmd, part);
  uint8_t data = pmd_part_cmdload(pmd, part);
  pmd_reg_write(work, pmd, reg, data);
}

// 255c
static void pmd_cmdee_noise_freq(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  pmd->ssg_noise_freq = pmd_part_cmdload(pmd, part);
}

// 2576
static void pmd_cmded_ssgmix(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->ssg_mix = pmd_part_cmdload(pmd, part);
}

// 25cc
static uint8_t pmd_hlfo_delay_check(
  struct pmd_part *part,
  uint8_t pan_ams_pms
) {
  if (part->hlfo_delay) pan_ams_pms &= 0xc0;
  return pan_ams_pms;
}

// 257c
static void pmd_part_set_pan(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t pan
){
  uint8_t pan_ams_pms = (pan & 3) << 6;
  pan_ams_pms |= part->pan & 0x3f;
  part->pan = pan_ams_pms;
  // 258e
  if (pmd->proc_ch == 3 && !pmd->opna_a1) {
    pmd->parts[PMD_PART_FM_3].pan = pan_ams_pms;
    pmd->parts[PMD_PART_FM_3B].pan = pan_ams_pms;
    pmd->parts[PMD_PART_FM_3C].pan = pan_ams_pms;
    pmd->parts[PMD_PART_FM_3D].pan = pan_ams_pms;
  }
  // 25b6
  if (pmd_part_masked(part)) return;
  pan_ams_pms = pmd_hlfo_delay_check(part, pan_ams_pms);
  pmd_reg_write(work, pmd, 0xb3+pmd->proc_ch, pan_ams_pms);
}

// 257b
static void pmd_cmdec_pan(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  pmd_part_set_pan(work, pmd, part, pmd_part_cmdload(pmd, part));
}

// 044d
static void pmd_cmdec_pan_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->pan = pmd_part_cmdload(pmd, part) << 6;
}

// 0aca
static void pmd_cmdec_pan_ppz8(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint8_t pan = pmd_part_cmdload(pmd, part);
  // 0ced
  static const uint8_t pantable[4] = {0, 9, 1, 5};
  part->pan = 0;
  if (pan < 4) part->pan = pantable[pan];
  if (work->ppz8) {
    work->ppz8_functbl->channel_pan(work->ppz8, pmd->proc_ch, part->pan);
  }
}

// 265d
static void pmd_opnarhythm_inc(uint8_t *incdata, uint8_t val) {
  for (int i = 0; i < 6; i++) {
    if (val & (1<<i)) {
      incdata[i]++;
    }
  }
}

// 25ed
static void pmd_cmdeb_opnarhythm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t val = pmd_part_cmdload(pmd, part);
  val &= pmd->opnarhythm_mask;
  if (!val) return;
  if (pmd->fadeout_vol) {
    pmd_opnarhythm_tl_write(work, pmd, pmd->opnarhythm_tl);
  }
  if (!(val & 0x80)) {
    // 260c
    for (int i = 0; i < 6; i++) {
      if (val & (1<<i)) {
        work->opna_writereg(work, 0x18+i, pmd->opnarhythm_ilpan[i]);
      }
    }
  }
  // 2626
  work->opna_writereg(work, 0x10, val);
  if (val & 0x80) {
    // 262d
    pmd_opnarhythm_inc(pmd->opnarhythm_dump_inc, val);
    pmd->opnarhythm &= val ^ 0xff;
  } else {
    // 263b
    pmd_opnarhythm_inc(pmd->opnarhythm_shot_inc, val);
    pmd->opnarhythm |= val;
  }
}

// 266c
static void pmd_cmdea_opnarhythm_il(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t val = pmd_part_cmdload(pmd, part);
  uint8_t i = (val >> 5) - 1;
  if (i >= 6) return;
  uint8_t ilpan = pmd->opnarhythm_ilpan[i];
  ilpan &= ~0x1fu;
  ilpan |= val & 0x1f;
  pmd->opnarhythm_ilpan[i] = ilpan;
  work->opna_writereg(work, 0x18+i, ilpan);
}

// 26c8
static void pmd_cmde9_opnarhythm_pan(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t val = pmd_part_cmdload(pmd, part);
  uint8_t i = (val >> 5) - 1;
  if (i >= 6) return;
  uint8_t ilpan = pmd->opnarhythm_ilpan[i];
  ilpan &= ~0xc0u;
  ilpan |= (val & 0x3) << 6;
  pmd->opnarhythm_ilpan[i] = ilpan;
  work->opna_writereg(work, 0x18+i, ilpan);
}

// 26d8
static void pmd_cmde8_opnarhythm_tl(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t tl = pmd_part_cmdload(pmd, part);
  if (pmd->opnarhythm_voldown) {
    tl = ((uint16_t)tl * (0x100-pmd->opnarhythm_voldown)) >> 8;
  }
  pmd->opnarhythm_tl = tl;
  if (pmd->fadeout_vol) {
    tl = ((uint16_t)tl * (0x100-pmd->fadeout_vol)) >> 8;
  }
  work->opna_writereg(work, 0x11, tl);
}

// 23e7
static void pmd_cmde7_transpose_rel(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->transpose += pmd_part_cmdload(pmd, part);
}

// 26fe
static void pmd_cmde6_opnarhythm_tl_rel(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  int tl = pmd->opnarhythm_tl + u8s8(pmd_part_cmdload(pmd, part));
  if (tl > 0x3f) tl = 0x3f;
  if (tl < 0) tl = 0;
  pmd->opnarhythm_tl = tl;
  if (pmd->fadeout_vol) {
    tl = ((uint16_t)tl * (0x100-pmd->fadeout_vol)) >> 8;
  }
  work->opna_writereg(work, 0x11, tl);
}

// 2695
static void pmd_cmde5_opnarhythm_il_rel(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t i = pmd_part_cmdload(pmd, part) - 1;
  if (i >= 6) {
    part->ptr++;
    return;
  }
  int il = pmd->opnarhythm_ilpan[i] & 0x1f;
  il += u8s8(pmd_part_cmdload(pmd, part));
  if (il > 0x1f) il = 0x1f;
  if (il < 0) il = 0;
  pmd->opnarhythm_ilpan[i] &= ~0x1fu;
  pmd->opnarhythm_ilpan[i] |= il;
  work->opna_writereg(work, 0x18+i, pmd->opnarhythm_ilpan[i]);
}

// 225e
static void pmd_cmde4_hlfo_delay(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->hlfo_delay_set = pmd_part_cmdload(pmd, part);
}

// 2403
static void pmd_cmde3_vol_add_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t vol = part->vol + pmd_part_cmdload(pmd, part);
  if (vol > 0x7f) vol = 0x7f;
  part->vol = vol;
}

// 2416
static void pmd_cmde3_vol_add_ssg(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t vol = part->vol + pmd_part_cmdload(pmd, part);
  if (vol > 0xf) vol = 0xf;
  part->vol = vol;
}

// 042e
static void pmd_cmde3_vol_add_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  int vol = part->vol + pmd_part_cmdload(pmd, part);
  if (vol > 0xff) vol = 0xff;
  part->vol = vol;
}

// FM:    2427
// SSG:   2440
// ADPCM: 043f
static void pmd_cmde2_vol_sub(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  int vol = part->vol - pmd_part_cmdload(pmd, part);
  if (vol < 0) vol = 0;
  part->vol = vol;
}

// 2209
static void pmd_cmde1_ams_pms(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t pan_ams_pms = pmd_part_cmdload(pmd, part);
  pan_ams_pms |= part->pan & 0xc0;
  part->pan = pan_ams_pms;
  if (pmd->proc_ch == 3 && !pmd->opna_a1) {
    pmd->parts[PMD_PART_FM_3].pan = pan_ams_pms;
    pmd->parts[PMD_PART_FM_3B].pan = pan_ams_pms;
    pmd->parts[PMD_PART_FM_3C].pan = pan_ams_pms;
    pmd->parts[PMD_PART_FM_3D].pan = pan_ams_pms;
  }
  if (pmd_part_masked(part)) return;
  pan_ams_pms = pmd_hlfo_delay_check(part, pan_ams_pms);
  pmd_reg_write(work, pmd, 0xb3+pmd->proc_ch, pan_ams_pms);
}

// 2252
static void pmd_cmde0_hlfo(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t hlfo = pmd_part_cmdload(pmd, part);
  pmd->opna_last22 = hlfo;
  work->opna_writereg(work, 0x22, hlfo);
}

// 2263
static void pmd_cmddf_meas_len(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  pmd->meas_len = pmd_part_cmdload(pmd, part);
}

// 21cc
static void pmd_cmdde_echo_init_add_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t vol = part->vol + pmd_part_cmdload(pmd, part);
  if (vol > 0x7f) vol = 0x7f;
  part->volume_save = vol+1;
  pmd->volume_saved = true;
}

// 21e1
static void pmd_cmdde_echo_init_add_ssg(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t vol = part->vol + pmd_part_cmdload(pmd, part);
  if (vol > 0xf) vol = 0xf;
  part->volume_save = vol+1;
  pmd->volume_saved = true;
}

// 21ed
static void pmd_cmdde_echo_init_add_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  int vol = part->vol + pmd_part_cmdload(pmd, part);
  if (vol > 0xfe) vol = 0xfe;
  part->volume_save = vol+1;
  pmd->volume_saved = true;
}

// 21fb
static void pmd_cmddd_echo_init_sub(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  int vol = part->vol - pmd_part_cmdload(pmd, part);
  if (vol < 0) vol = 0;
  part->volume_save = vol+1;
  pmd->volume_saved = true;
}

// 21be
static void pmd_cmddc_status1(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  pmd->status1 = pmd_part_cmdload(pmd, part);
}

// 21c3
static void pmd_cmddb_status1_add(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  pmd->status1 += pmd_part_cmdload(pmd, part);
}

// 2104
static void pmd_cmdda_portamento_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t note = pmd_part_cmdload(pmd, part);
  if (pmd_part_masked(part)) {
    return;
  }
  note = pmd_part_lfo_init_fm(work, pmd, part, note);
  note = pmd_part_note_transpose(part, note);
  pmd_note_freq_fm(part, note);
  uint16_t f1 = part->actual_freq;
  uint8_t n1 = part->actual_note;
  note = pmd_part_cmdload(pmd, part);
  note = pmd_part_note_transpose(part, note);
  pmd_note_freq_fm(part, note);
  uint16_t f2 = part->actual_freq;
  part->actual_freq = f1;
  part->actual_note = n1;
  uint8_t f2blk = f2 >> 11;
  uint8_t f1blk = f1 >> 11;
  f1 &= 0x7ff;
  f2 &= 0x7ff;
  int32_t blkdiff = (int32_t)(f2blk - f1blk) * 0x26a;
  int32_t freqdiff = f2 - f1 + blkdiff;
  uint8_t clocks = pmd_part_cmdload(pmd, part);
  part->len = part->len_cnt = clocks;
  pmd_part_calc_gate(pmd, part);
  int16_t p_add = 0;
  int16_t p_rem = 0;
  if (clocks) {
    p_add = freqdiff / clocks;
    p_rem = freqdiff % clocks;
  }
  part->portamento_add = p_add;
  part->portamento_rem = p_rem;
  part->lfof.portamento = true;
  pmd_part_fm_out(work, pmd, part);
}

// 2176
static void pmd_cmdda_portamento_ssg(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t note = pmd_part_cmdload(pmd, part);
  if (pmd_part_masked(part)) {
    return;
  }
  note = pmd_part_lfo_init_ssg(work, pmd, part, note);
  note = pmd_part_note_transpose(part, note);
  pmd_note_freq_ssg(part, note);
  uint16_t f1 = part->actual_freq;
  uint8_t n1 = part->actual_note;
  note = pmd_part_cmdload(pmd, part);
  note = pmd_part_note_transpose(part, note);
  pmd_note_freq_ssg(part, note);
  uint16_t f2 = part->actual_freq;
  part->actual_freq = f1;
  part->actual_note = n1;
  uint8_t clocks = pmd_part_cmdload(pmd, part);
  part->len = part->len_cnt = clocks;
  int freqdiff = f2 - f1;
  pmd_part_calc_gate(pmd, part);
  int16_t p_add = 0;
  int16_t p_rem = 0;
  if (clocks) {
    p_add = freqdiff / clocks;
    p_rem = freqdiff % clocks;
  }
  part->portamento_add = p_add;
  part->portamento_rem = p_rem;
  part->lfof.portamento = true;
  pmd_part_ssg_out(work, pmd, part);
}

// 03d6
static void pmd_cmdda_portamento_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t note = pmd_part_cmdload(pmd, part);
  if (pmd_part_masked(part)) return;
  pmd_part_lfo_init_ssg(work, pmd, part, note);
  note = pmd_part_note_transpose(part, note);
  pmd_note_freq_adpcm(part, note);
  uint16_t f1 = part->actual_freq;
  uint8_t n1 = part->actual_note;
  note = pmd_part_cmdload(pmd, part);
  note = pmd_part_note_transpose(part, note);
  pmd_note_freq_adpcm(part, note);
  uint16_t f2 = part->actual_freq;
  part->actual_freq = f1;
  part->actual_note = n1;
  int freqdiff = f2 - f1;
  uint8_t clocks = pmd_part_cmdload(pmd, part);
  part->len = part->len_cnt = clocks;
  pmd_part_calc_gate(pmd, part);
  int16_t p_add = 0;
  int16_t p_rem = 0;
  if (clocks) {
    p_add = freqdiff / clocks;
    p_rem = freqdiff % clocks;
  }
  part->portamento_add = p_add;
  part->portamento_rem = p_rem;
  part->lfof.portamento = true;
  pmd_part_adpcm_out(work, pmd, part);
}

// 0a62
static void pmd_cmdda_portamento_ppz8(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t note = pmd_part_cmdload(pmd, part);
  if (pmd_part_masked(part)) return;
  pmd_part_lfo_init_ssg(work, pmd, part, note);
  note = pmd_part_note_transpose(part, note);
  pmd_note_freq_ppz8(part, note);
  uint32_t f1 = (((uint32_t)part->actual_freq_upper) << 16) | part->actual_freq;
  uint8_t n1 = part->actual_note;
  note = pmd_part_cmdload(pmd, part);
  note = pmd_part_note_transpose(part, note);
  pmd_note_freq_ppz8(part, note);
  uint32_t f2 = (((uint32_t)part->actual_freq_upper) << 16) | part->actual_freq;
  part->actual_freq = f1;
  part->actual_freq_upper = f1 >> 16;
  part->actual_note = n1;
  int16_t freqdiff = u16s16((f2 - f1)>>4);
  uint8_t clocks = pmd_part_cmdload(pmd, part);
  part->len = part->len_cnt = clocks;
  pmd_part_calc_gate(pmd, part);
  int16_t p_add = 0;
  int16_t p_rem = 0;
  if (clocks) {
    p_add = freqdiff / clocks;
    p_rem = freqdiff % clocks;
  }
  part->portamento_add = p_add;
  part->portamento_rem = p_rem;
  part->lfof.portamento = true;
  pmd_part_ppz8_out(work, pmd, part);
}

// 20bf
static void pmd_cmdd6_md(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->md_speed = pmd_part_cmdload(pmd, part);
  part->md_speed_set = part->md_speed;
  part->md_depth = pmd_part_cmdload(pmd, part);
}

// 2054
static void pmd_cmdd4_ssgeff(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t cmd = pmd_part_cmdload(pmd, part);
  if (pmd_part_masked(part)) return;
  if (cmd) {
    pmd_ssg_effect(work, pmd, cmd);
  } else {
    pmd_ssgeff_stop(work, pmd);
  }
}

// 206f
static void pmd_cmdd3_fmeff(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  // TODO: 42ae
  uint8_t val = pmd_part_cmdload(pmd, part);
  if (pmd_part_masked(part)) return;
  bool a1 = pmd->opna_a1;
  if (val) {
    //3be1();
  } else {
    //3c43();
  }
  pmd->opna_a1 = a1;
}

// 20b6
static void pmd_cmdd2_fadeout(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  // TODO
  pmd_part_cmdload(pmd, part);
}

// 2561
static void pmd_cmdd0_ssgnoisefreqadd(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  int nf = pmd->ssg_noise_freq + u8s8(pmd_part_cmdload(pmd, part));
  if (nf < 0) nf = 0;
  if (nf > 0x1f) nf = 0x1f;
  pmd->ssg_noise_freq = nf;
}

// 2044
static void pmd_part_fm_keyon_if_not_masked(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (!pmd_part_masked(part) && !part->keystatus.off_mask)
    pmd_part_fm_keyon(work, pmd, part);
}

// 1f88
static void pmd_cmdcf_slotmask(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t data = pmd_part_cmdload(pmd, part);
  if (data & 0x0f) {
    part->fm_slotout = data << 4;
  } else {
    // 1f9c
    uint8_t alg = 0;
    if (pmd->proc_ch == 3 && !pmd->opna_a1) {
      alg = pmd->fm3ex_fb_alg;
    } else {
      // 2dda
      const uint8_t *toneptr = pmd_get_toneptr(pmd, part->tonenum);
      if (toneptr) {
        alg = toneptr[0x18];
      }
    }
    alg &= 7;
    part->fm_slotout = pmd_alg_output_slot_table[alg];
  }
  // 1fc9
  data &= 0xf0;
  if (data == part->fm_slotmask) return;
  // 1fd1
  part->fm_slotmask = data;
  part->mask.slot = !data;
  // 1fe3
  if (pmd_fm3ex_mode_update_check(work, pmd, part)
      && part != &pmd->parts[PMD_PART_FM_3]) {
    pmd_part_fm_keyon_if_not_masked(work, pmd, &pmd->parts[PMD_PART_FM_3]);
    if (part != &pmd->parts[PMD_PART_FM_3B]) {
      pmd_part_fm_keyon_if_not_masked(work, pmd, &pmd->parts[PMD_PART_FM_3B]);
      if (part != &pmd->parts[PMD_PART_FM_3C]) {
        pmd_part_fm_keyon_if_not_masked(work, pmd, &pmd->parts[PMD_PART_FM_3C]);
      }
    }
  }
  // 2012
  uint8_t tonemask = 0;
  if (part->fm_slotmask & 0x80) tonemask |= 0x11;
  if (part->fm_slotmask & 0x40) tonemask |= 0x44;
  if (part->fm_slotmask & 0x20) tonemask |= 0x22;
  if (part->fm_slotmask & 0x10) tonemask |= 0x88;
  part->fm_tone_slotmask = tonemask;
  part->proc_masked = pmd_part_masked(part);
}

// 039a
static void pmd_cmdce_adpcm_loop(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)work;
  uint16_t diff = pmd_part_cmdload(pmd, part);
  diff |= (uint16_t)pmd_part_cmdload(pmd, part) << 8;
  if (!(diff & 0x8000)) diff += pmd->adpcm_start;
  else diff += pmd->adpcm_stop;
  pmd->adpcm_start_loop = diff;
  diff = pmd_part_cmdload(pmd, part);
  diff |= (uint16_t)pmd_part_cmdload(pmd, part) << 8;
  if (diff && !(diff & 0x80)) diff += pmd->adpcm_start;
  else diff += pmd->adpcm_stop;
  pmd->adpcm_stop_loop = diff;
  diff = pmd_part_cmdload(pmd, part);
  diff |= (uint16_t)pmd_part_cmdload(pmd, part) << 8;
  if (diff != 0x8000) {
    if (!(diff & 0x80)) diff += pmd->adpcm_start;
    else diff += pmd->adpcm_stop;
  }
  pmd->adpcm_release = diff;
}

// 0a04
static void pmd_cmdce_ppz8_loop(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint32_t startoff = pmd_part_cmdload(pmd, part);
  startoff |= (uint16_t)pmd_part_cmdload(pmd, part) << 8;
  uint32_t endoff = pmd_part_cmdload(pmd, part);
  endoff |= (uint16_t)pmd_part_cmdload(pmd, part) << 8;
  if (work->ppz8) {
    uint32_t len = work->ppz8_functbl->voice_length(work->ppz8, part->tonenum);
    if (startoff & 0x8000) startoff = len + u16s16(startoff);
    if (endoff & 0x8000) endoff = len + u16s16(endoff);
    work->ppz8_functbl->channel_loopoffset(work->ppz8, pmd->proc_ch, startoff, endoff);
  }
  pmd_part_cmdload(pmd, part);
  pmd_part_cmdload(pmd, part);
}

// 1e1b
static void pmd_cmdcd_env_new(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->ssg_env_param_set[SSG_ENV_PARAM_NEW_AR] = pmd_part_cmdload(pmd, part) & 0x1f;
  part->ssg_env_param_set[SSG_ENV_PARAM_NEW_DR] = pmd_part_cmdload(pmd, part) & 0x1f;
  part->ssg_env_param_set[SSG_ENV_PARAM_NEW_SR] = pmd_part_cmdload(pmd, part) & 0x1f;
  uint8_t data = pmd_part_cmdload(pmd, part);
  part->ssg_env_param_set[SSG_ENV_PARAM_NEW_RR] = data & 0xf;
  part->ssg_env_param_sl = (data >> 4) ^ 0xf;
  part->ssg_env_param_al = pmd_part_cmdload(pmd, part) & 0xf;
  if (part->ssg_env_state_old != SSG_ENV_STATE_OLD_NEW) {
    part->ssg_env_state_old = SSG_ENV_STATE_OLD_NEW;
    part->ssg_env_state_new = SSG_ENV_STATE_NEW_RR;
    part->ssg_env_vol = 0;
  }
  FMDRIVER_DEBUG("SSG NEW ENV: %d %d %d %d %d %d\n",
         part->ssg_env_param_set[SSG_ENV_PARAM_NEW_AR],
         part->ssg_env_param_set[SSG_ENV_PARAM_NEW_DR],
         part->ssg_env_param_set[SSG_ENV_PARAM_NEW_SR],
         part->ssg_env_param_set[SSG_ENV_PARAM_NEW_RR],
         part->ssg_env_param_sl,
         part->ssg_env_param_al);
}

// 1def
static void pmd_cmdcc_det_ext(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->flagext.detune = pmd_part_cmdload(pmd, part) & 1;
}

// 1e16
static void pmd_cmdcb_lfo_waveform(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->lfo_waveform = pmd_part_cmdload(pmd, part);
}

// 1dfa
static void pmd_cmdca_lfo_ext(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->flagext.lfo = pmd_part_cmdload(pmd, part) & 1;
}

// 1e07
static void pmd_cmdc9_env_ext(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->flagext.env = pmd_part_cmdload(pmd, part) & 1;
}

// 1e99
static void pmd_cmdc8_fm3ex_det(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  if (pmd->proc_ch != 3 || pmd->opna_a1) {
    pmd_cmd_null_3(work, pmd, part);
    return;
  }
  // 1ead
  uint8_t mask = pmd_part_cmdload(pmd, part);
  uint16_t val16 = pmd_part_cmdload(pmd, part);
  val16 |= ((uint16_t)pmd_part_cmdload(pmd, part)) << 8;
  int16_t det = u16s16(val16);
  for (int s = 0; s < 4; s++) {
    if (mask & (1<<s)) {
      pmd->fm3ex_det[s] = det;
    }
  }
  // 1ecd
  pmd->fm3ex_force = false;
  for (int s = 0; s < 4; s++) {
    if (pmd->fm3ex_det[s]) pmd->fm3ex_force = true;
  }
  pmd_fm3ex_mode_update(work, pmd, part);
}

// 1e5f
static void pmd_cmdc7_fm3ex_det_add(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  if (pmd->proc_ch != 3 || pmd->opna_a1) {
    pmd_cmd_null_3(work, pmd, part);
    return;
  }
  // 1e73
  uint8_t mask = pmd_part_cmdload(pmd, part);
  uint16_t det = pmd_part_cmdload(pmd, part);
  det |= ((uint16_t)pmd_part_cmdload(pmd, part)) << 8;
  for (int s = 0; s < 4; s++) {
    if (mask & (1<<s)) {
      pmd->fm3ex_det[s] = u16s16((uint16_t)pmd->fm3ex_det[s] + det);
    }
  }
  pmd->fm3ex_force = false;
  for (int s = 0; s < 4; s++) {
    if (pmd->fm3ex_det[s]) pmd->fm3ex_force = true;
  }
  pmd_fm3ex_mode_update(work, pmd, part);
}

// 1dc0
static void pmd_fm3ex_init(
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint16_t ptr
) {
  part->ptr = ptr;
  part->len = part->len_cnt = 1;
  part->keystatus.off = true;
  part->keystatus.off_mask = true;
  part->md_cnt = 0xff;
  part->md_cnt_set = 0xff;
  part->md_cnt_b = 0xff;
  part->md_cnt_set_b = 0xff;
  part->actual_note = 0xff;
  part->curr_note = 0xff;
  part->vol = 0x6c;
  part->pan = pmd->parts[PMD_PART_FM_3].pan;
  part->mask.slot = true;
  part->loop.ended = false;
}

// 1d90
static void pmd_cmdc6_fm3ex_init(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  for (int i = 0; i < 3; i++) {
    uint16_t ptr = pmd_part_cmdload(pmd, part);
    ptr |= ((uint16_t)pmd_part_cmdload(pmd, part)) << 8;
    if (!ptr) continue;
    pmd_fm3ex_init(pmd, &pmd->parts[PMD_PART_FM_3B+i], ptr);
  }
}

// 1d45
static void pmd_cmdc5_lfo_slotmask(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t mask = pmd_part_cmdload(pmd, part);
  mask &= 0x0f;
  if (mask) {
    // 1d4a
    mask <<= 4;
    mask |= 0x0f;
    part->vol_lfo_slotmask = mask;
  } else {
    // 1d59
    part->vol_lfo_slotmask = part->fm_slotout;
  }
  // 1d7b
  pmd_fm3ex_mode_update_check(work, pmd, part);
}

// 22c6
static void pmd_cmdc4_gate_rel(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->gate_rel = pmd_part_cmdload(pmd, part);
}

// 25d9
static void pmd_cmdc3_pan_ex(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t data = pmd_part_cmdload(pmd, part);
  pmd_part_cmdload(pmd, part);
  uint8_t pan;
  if (data == 0) pan = 3;
  else if (data & 0x80) pan = 1;
  else pan = 2;
  pmd_part_set_pan(work, pmd, part, pan);
}

// 0458
static void pmd_cmdc3_pan_ex_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t data = pmd_part_cmdload(pmd, part);
  pmd_part_cmdload(pmd, part);
  uint8_t pan;
  if (!data) pan = 0xc0;
  else if (data & 0x80) pan = 0x40;
  else pan = 0x80;
  part->pan = pan;
}

// 0ae5
static void pmd_cmdc3_pan_ex_ppz8(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t data = pmd_part_cmdload(pmd, part);
  pmd_part_cmdload(pmd, part);
  if (!(data & 0x80)) {
    if (data > 4) data = 4;
  } else {
    if (data < 0xfc) data = 0xfc;
  }
  data += 5;
  uint8_t pan = data;
  part->pan = pan;
  if (work->ppz8) {
    work->ppz8_functbl->channel_pan(work->ppz8, pmd->proc_ch, part->pan);
  }
}

// 2509
static void pmd_cmdc2_lfo_delay(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t d = pmd_part_cmdload(pmd, part);
  part->lfo.delay = d;
  part->lfo_set.delay = d;
  pmd_lfo_reset(part);
}

// 1a95
// DF
static void pmd_cmdc0_ff_fm_voldown(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  pmd->fm_voldown = pmd_part_cmdload(pmd, part);
}

// 1ab0
// DF +/-
static void pmd_cmdc0_fe_fm_voldown_rel(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t vol = pmd_part_cmdload(pmd, part);
  if (!vol) {
    pmd->fm_voldown = pmd->fm_voldown_orig;
  } else {
    int newvol = pmd->fm_voldown + u8s8(vol);
    if (newvol > 0xff) newvol = 0xff;
    if (newvol < 0) newvol = 0;
    pmd->fm_voldown = newvol;
  }
}

// 1a9c
// DS
static void pmd_cmdc0_fd_ssg_voldown(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  pmd->ssg_voldown = pmd_part_cmdload(pmd, part);
}

// 1ad6
// DS +/-
static void pmd_cmdc0_fc_ssg_voldown_rel(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t vol = pmd_part_cmdload(pmd, part);
  if (!vol) {
    pmd->ssg_voldown = pmd->ssg_voldown_orig;
  } else {
    int newvol = pmd->ssg_voldown + u8s8(vol);
    if (newvol > 0xff) newvol = 0xff;
    if (newvol < 0) newvol = 0;
    pmd->ssg_voldown = newvol;
  }
}

// 1aa1
// DP
static void pmd_cmdc0_fb_adpcm_voldown(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  pmd->adpcm_voldown = pmd_part_cmdload(pmd, part);
}

// 1ade
// DP +/-
static void pmd_cmdc0_fa_adpcm_voldown_rel(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t vol = pmd_part_cmdload(pmd, part);
  if (!vol) {
    pmd->adpcm_voldown = pmd->adpcm_voldown_orig;
  } else {
    int newvol = pmd->adpcm_voldown + u8s8(vol);
    if (newvol > 0xff) newvol = 0xff;
    if (newvol < 0) newvol = 0;
    pmd->adpcm_voldown = newvol;
  }
}

// 1aa6
// DR
static void pmd_cmdc0_f9_opnarhythm_voldown(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  pmd->opnarhythm_voldown = pmd_part_cmdload(pmd, part);
}

// 1ae6
// DR +/-
static void pmd_cmdc0_f8_opnarhythm_voldown_rel(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t vol = pmd_part_cmdload(pmd, part);
  if (!vol) {
    pmd->opnarhythm_voldown = pmd->opnarhythm_voldown_orig;
  } else {
    int newvol = pmd->opnarhythm_voldown + u8s8(vol);
    if (newvol > 0xff) newvol = 0xff;
    if (newvol < 0) newvol = 0;
    pmd->opnarhythm_voldown = newvol;
  }
}

// 1a8e
// probably A
static void pmd_cmdc0_f7_pcm86_vol(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  pmd->pcm86_vol_spb = pmd_part_cmdload(pmd, part) & 1;
}


// 1aab
// unimplemented (disabled in PMDPPZ.COM)
static void pmd_cmdc0_f6_ppz8_voldown(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  pmd->ppz8_voldown = pmd_part_cmdload(pmd, part);
}

// 1aee
// unimplemented (disabled in PMDPPZ.COM)
static void pmd_cmdc0_f5_ppz8_voldown_rel(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t vol = pmd_part_cmdload(pmd, part);
  if (!vol) {
    pmd->ppz8_voldown = pmd->ppz8_voldown_orig;
  } else {
    int newvol = pmd->ppz8_voldown + u8s8(vol);
    if (newvol > 0xff) newvol = 0xff;
    if (newvol < 0) newvol = 0;
    pmd->ppz8_voldown = newvol;
  }
}

typedef void (*pmd_cmd_func) (
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
);

static const pmd_cmd_func pmd_cmd_table_c0[] = {
  pmd_cmdc0_ff_fm_voldown,
  pmd_cmdc0_fe_fm_voldown_rel,
  pmd_cmdc0_fd_ssg_voldown,
  pmd_cmdc0_fc_ssg_voldown_rel,
  pmd_cmdc0_fb_adpcm_voldown,
  pmd_cmdc0_fa_adpcm_voldown_rel,
  pmd_cmdc0_f9_opnarhythm_voldown,
  pmd_cmdc0_f8_opnarhythm_voldown_rel,
  pmd_cmdc0_f7_pcm86_vol,
  // f6, f5 is disabled
  pmd_cmdc0_f6_ppz8_voldown,
  pmd_cmdc0_f5_ppz8_voldown_rel,
};

// 1a62
static void pmd_cmdc0_extended(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t data
) {
  if (data < 0xf7) {
    part->ptr = 0;
    return;
  }
  pmd_cmd_table_c0[data^0xff](work, pmd, part);
}

// 1cc0
static void pmd_part_fm_unmask(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (!part->fm_tone_slotmask) return;
  //uint8_t tonenum = part->tonenum;
  uint8_t fm_tone_tl[4];
  for (int i = 0; i < 4; i++) {
    fm_tone_tl[i] = part->fm_tone_tl[i];
  }
  pmd->tonemask_fb_alg = true;
  pmd_part_tone_fm(work, pmd, part);
  pmd->tonemask_fb_alg = false;
  for (int i = 0; i < 4; i++) {
    part->fm_tone_tl[i] = fm_tone_tl[i];
  }
  uint8_t slotmask = (part->fm_slotout ^ 0xff) & part->fm_slotmask;
  if (slotmask) {
    for (int s = 0; s < 4; s++) {
      if (!(slotmask & (1<<(7-s)))) continue;
      static const uint8_t regoff[4] = {3, 1, 2, 0};
      pmd_reg_write(work, pmd, 0x3f+regoff[s]*4+pmd->proc_ch, fm_tone_tl[regoff[s]]);
    }
  }
  // 1d23
  pmd_reg_write(work, pmd, 0xb3+pmd->proc_ch, pmd_hlfo_delay_check(part, part->pan));
}

// 1c50
static void pmd_cmdc0_mml_mask_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t data = pmd_part_cmdload(pmd, part);
  if (data >= 2) {
    pmd_cmdc0_extended(work, pmd, part, data);
    return;
  } else if (data == 1) {
    // 1c5c
    part->mask.mml = false;
    bool masked = pmd_part_masked(part);
    part->mask.mml = true;
    if (!masked) {
      pmd_part_tone_slotmask_stop(work, pmd, part);
    }
    // 1c9c
    part->proc_masked = true;
  } else {
    // 1ca0
    part->mask.mml = false;
    if (pmd_part_masked(part)) {
      part->proc_masked = true;
    } else {
      pmd_part_fm_unmask(work, pmd, part);
      part->proc_masked = false;
    }
  }
}

// 1c7a
static void pmd_cmdc0_mml_mask_ssg(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t data = pmd_part_cmdload(pmd, part);
  if (data >= 2) {
    pmd_cmdc0_extended(work, pmd, part, data);
    return;
  } else if (data == 1) {
    // 1c86
    part->mask.mml = false;
    bool masked = pmd_part_masked(part);
    part->mask.mml = true;
    if (!masked) {
      // 1c90
      uint16_t ssgout = pmd_part_ssg_readout(work, pmd);
      ssgout |= ssgout>>8;
      work->opna_writereg(work, 0x07, ssgout);
    }
    // 1c9c
    part->proc_masked = true;
  } else {
    // 1ca0
    part->mask.mml = false;
    part->proc_masked = pmd_part_masked(part);
  }
}

// 036a
static void pmd_cmdc0_mml_mask_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint8_t data = pmd_part_cmdload(pmd, part);
  if (data >= 2) {
    pmd_cmdc0_extended(work, pmd, part, data);
    return;
  } else if (data == 1) {
    // 0376
    part->mask.mml = false;
    bool masked = pmd_part_masked(part);
    part->mask.mml = true;
    if (!masked) {
      work->opna_writereg(work, 0x101, 0x02);
      work->opna_writereg(work, 0x100, 0x01);
    }
    part->proc_masked = true;
  } else {
    // 0390
    part->mask.mml = false;
    part->proc_masked = pmd_part_masked(part);
  }
}

// 09d8
static void pmd_cmdc0_mml_mask_ppz8(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  uint8_t data = pmd_part_cmdload(pmd, part);
  if (data >= 2) {
    pmd_cmdc0_extended(work, pmd, part, data);
    return;
  } else if (data == 1) {
    // 09e4
    part->mask.mml = false;
    bool masked = pmd_part_masked(part);
    part->mask.mml = true;
    if (!masked) {
      if (work->ppz8) {
        work->ppz8_functbl->channel_stop(work->ppz8, pmd->proc_ch);
      }
    }
    part->proc_masked = true;
    // -> 08ca
  } else {
    // 09fa
    part->mask.mml = false;
    part->proc_masked = pmd_part_masked(part);
    // -> 07cd
  }
}

// 1caa
static void pmd_cmdc0_mml_mask_rhythm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t data = pmd_part_cmdload(pmd, part);
  if (data >= 2) {
    pmd_cmdc0_extended(work, pmd, part, data);
    return;
  } else if (data == 1) {
    part->mask.mml = true;
  } else {
    part->mask.mml = false;
  }
}

// 244e
static void pmd_cmdbf_lfo2(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  pmd_part_lfo_flip(part);
  pmd_cmdf2_lfo(work, pmd, part);
  pmd_part_lfo_flip(part);
}

// 2473
static void pmd_cmdbe_lfo2_switch(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t val = pmd_part_cmdload(pmd, part);
  part->lfof_b.freq = val & (1<<0);
  part->lfof_b.vol = val & (1<<1);
  part->lfof_b.sync = val & (1<<2);
  pmd_part_lfo_flip(part);
  pmd_lfo_reset(part);
  pmd_part_lfo_flip(part);
}

// 248d
static void pmd_cmdbe_lfo2_switch_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  pmd_cmdbe_lfo2_switch(work, pmd, part);
  pmd_fm3ex_mode_update_check(work, pmd, part);
}

// 245f
static void pmd_cmdbd_lfo2_md(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  pmd_part_lfo_flip(part);
  pmd_cmdd6_md(work, pmd, part);
  pmd_part_lfo_flip(part);
}

// 2464
static void pmd_cmdbc_lfo2_waveform(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  pmd_part_lfo_flip(part);
  pmd_cmdcb_lfo_waveform(work, pmd, part);
  pmd_part_lfo_flip(part);
}

// 2469
static void pmd_cmdbb_lfo2_ext(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  pmd_part_lfo_flip(part);
  pmd_cmdca_lfo_ext(work, pmd, part);
  pmd_part_lfo_flip(part);
}

// 1d61
static void pmd_cmdba_lfo2_slotmask(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t mask = pmd_part_cmdload(pmd, part);
  mask &= 0x0f;
  if (mask) {
    // 1d4a
    mask <<= 4;
    mask |= 0x0f;
    part->vol_lfo_slotmask_b = mask;
  } else {
    // 1d59
    part->vol_lfo_slotmask_b = part->fm_slotout;
  }
  // 1d7b
  pmd_fm3ex_mode_update_check(work, pmd, part);
}

// 246e
static void pmd_cmdb9_lfo2_delay(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  pmd_part_lfo_flip(part);
  pmd_cmdc2_lfo_delay(work, pmd, part);
  pmd_part_lfo_flip(part);
}

// 1b87
// command O
static void pmd_cmdb8_slottl(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t data = pmd_part_cmdload(pmd, part);
  uint8_t slotmask = (data & 0xf) & (part->fm_slotmask>>4);
  uint8_t val = pmd_part_cmdload(pmd, part);
  bool masked = pmd_part_masked(part);
  bool relative = data & 0x80;
  static const uint8_t slottable[4] = {0, 2, 1, 3};
  for (int s = 0; s < 4; s++) {
    if (!(slotmask & (1<<s))) continue;
    if (!relative) {
      part->fm_tone_tl[slottable[s]] = val & 0x7f;
    } else {
      uint8_t vol = part->fm_tone_tl[slottable[s]] + val;
      if (vol & 0x80) {
        if (val & 0x80) vol = 0;
        else vol = 0x7f;
      }
      part->fm_tone_tl[slottable[s]] = vol;
    }
    if (!masked) {
      pmd_reg_write(work, pmd,
          0x3f+pmd->proc_ch+4*slottable[s], part->fm_tone_tl[slottable[s]]);
    }
  }
}

// 20cb
static void pmd_cmdb7_lfo_md_cnt(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t data = pmd_part_cmdload(pmd, part);
  bool b = data & 0x80;
  data &= 0x7f;
  if (!data) data = 0xff;
  if (!b) {
    part->md_cnt = part->md_cnt_set = data;
  } else {
    part->md_cnt_b = part->md_cnt_set_b = data;
  }
}

// 1b19
static void pmd_part_write_fb(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t data
) {
  uint8_t newfb = (data & 7) << 3;
  uint8_t fbalg;
  if ((pmd->proc_ch == 3) && (!pmd->opna_a1)) {
    pmd->fm3ex_fb_alg &= 7;
    pmd->fm3ex_fb_alg |= newfb;
    fbalg = pmd->fm3ex_fb_alg;
  } else {
    part->fb_alg &= 7;
    part->fb_alg |= newfb;
    fbalg = part->fb_alg;
  }
  if ((pmd->proc_ch == 3) && (!pmd->opna_a1) && (!(part->fm_slotmask & 0x10))) return;
  pmd_reg_write(work, pmd, 0xaf+pmd->proc_ch, fbalg);
  part->fb_alg = fbalg;
}

// 1b0e
static void pmd_cmdb6_fb(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  uint8_t data = pmd_part_cmdload(pmd, part);
  uint8_t fbalg;
  if (!(data & 0x80)) {
    pmd_part_write_fb(work, pmd, part, data);
  } else {
    // 1b51
    if (!(data & 0x40)) data &= 7;
    if ((pmd->proc_ch == 3) && (!pmd->opna_a1)) {
      fbalg = pmd->fm3ex_fb_alg;
    } else {
      fbalg = part->fb_alg;
    }
    fbalg >>= 3;
    fbalg &= 7;
    fbalg += data;
    if (fbalg & 0x80) {
      pmd_part_write_fb(work, pmd, part, 0);
    } else if (fbalg > 7) {
      pmd_part_write_fb(work, pmd, part, 7);
    } else {
      pmd_part_write_fb(work, pmd, part, fbalg);
    }
  }
}

// 1af6
static void pmd_cmdb5_slot_delay(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  uint8_t data = pmd_part_cmdload(pmd, part);
  data = (~data) & 0xf;
  data <<= 4;
  part->slot_delay_mask = data;
  part->slot_delay = pmd_part_cmdload(pmd, part);
  part->slot_delay_cnt = part->slot_delay;
}

// 099f
static void pmd_cmdb4_ppz8_init(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  for (int i = 0; i < 8; i++) {
    uint16_t ptr = pmd_part_cmdload(pmd, part);
    ptr |= (uint16_t)pmd_part_cmdload(pmd, part) << 8;
    if (!ptr) continue;
    struct pmd_part *ppzpart = &pmd->parts[PMD_PART_PPZ_1+i];
    ppzpart->ptr = ptr;
    ppzpart->len = ppzpart->len_cnt = 1;
    ppzpart->keystatus.off = true;
    ppzpart->keystatus.off_mask = true;
    ppzpart->md_cnt = 0xff;
    ppzpart->md_cnt_set = 0xff;
    ppzpart->md_cnt_b = 0xff;
    ppzpart->md_cnt_set_b = 0xff;
    ppzpart->actual_note = 0xff;
    ppzpart->vol = 0x80;
    ppzpart->pan = 5;
    ppzpart->loop.ended = false;
  }
}

// 22bc
static void pmd_cmdb3_gate_min(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->gate_min = pmd_part_cmdload(pmd, part);
}

// 23ef
static void pmd_cmdb2_transpose_master(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->transpose_master = pmd_part_cmdload(pmd, part);
}

// 22c1
static void pmd_cmdb1_gate_rand_range(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
){
  (void)work;
  part->gate_rand_range = pmd_part_cmdload(pmd, part);
}

enum {
  PMD_CMD_CNT = 0x100-0xb1
};

static const pmd_cmd_func pmd_cmd_table_fm[PMD_CMD_CNT] = {
  pmd_cmdff_tonenum,
  pmd_cmdfe_gate_abs,
  pmd_cmdfd_vol,
  pmd_cmdfc_tempo,
  pmd_cmdfb_tie,
  pmd_cmdfa_det,
  pmd_cmdf9_repeat_reset,
  pmd_cmdf8_repeat,
  pmd_cmdf7_repeat_exit,
  pmd_cmdf6_set_loop,
  pmd_cmdf5_transpose,
  pmd_cmdf4_volinc_fm,
  pmd_cmdf3_voldec_fm,
  pmd_cmdf2_lfo,
  pmd_cmdf1_lfo_switch_fm,
  pmd_cmd_null_4,
  pmd_cmdef_poke,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdec_pan,
  pmd_cmdeb_opnarhythm,
  pmd_cmdea_opnarhythm_il,
  pmd_cmde9_opnarhythm_pan,
  pmd_cmde8_opnarhythm_tl,
  pmd_cmde7_transpose_rel,
  pmd_cmde6_opnarhythm_tl_rel,
  pmd_cmde5_opnarhythm_il_rel,
  pmd_cmde4_hlfo_delay,
  pmd_cmde3_vol_add_fm,
  pmd_cmde2_vol_sub,
  pmd_cmde1_ams_pms,
  pmd_cmde0_hlfo,
  pmd_cmddf_meas_len,
  pmd_cmdde_echo_init_add_fm,
  pmd_cmddd_echo_init_sub,
  pmd_cmddc_status1,
  pmd_cmddb_status1_add,
  pmd_cmdda_portamento_fm,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdd6_md,
  pmd_cmdd5_detrel,
  pmd_cmdd4_ssgeff,
  pmd_cmdd3_fmeff,
  pmd_cmdd2_fadeout,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdcf_slotmask,
  pmd_cmd_null_6,
  pmd_cmd_null_5,
  pmd_cmd_null_1,
  pmd_cmdcb_lfo_waveform,
  pmd_cmdca_lfo_ext,
  pmd_cmd_null_1,
  pmd_cmdc8_fm3ex_det,
  pmd_cmdc7_fm3ex_det_add,
  pmd_cmdc6_fm3ex_init,
  pmd_cmdc5_lfo_slotmask,
  pmd_cmdc4_gate_rel,
  pmd_cmdc3_pan_ex,
  pmd_cmdc2_lfo_delay,
  pmd_cmd_null_0,
  pmd_cmdc0_mml_mask_fm,
  pmd_cmdbf_lfo2,
  pmd_cmdbe_lfo2_switch_fm,
  pmd_cmdbd_lfo2_md,
  pmd_cmdbc_lfo2_waveform,
  pmd_cmdbb_lfo2_ext,
  pmd_cmdba_lfo2_slotmask,
  pmd_cmdb9_lfo2_delay,
  pmd_cmdb8_slottl,
  pmd_cmdb7_lfo_md_cnt,
  pmd_cmdb6_fb,
  pmd_cmdb5_slot_delay,
  pmd_cmd_null_16,
  pmd_cmdb3_gate_min,
  pmd_cmdb2_transpose_master,
  pmd_cmdb1_gate_rand_range
};

static const pmd_cmd_func pmd_cmd_table_ssg[PMD_CMD_CNT] = {
  pmd_cmd_null_1,
  pmd_cmdfe_gate_abs,
  pmd_cmdfd_vol,
  pmd_cmdfc_tempo,
  pmd_cmdfb_tie,
  pmd_cmdfa_det,
  pmd_cmdf9_repeat_reset,
  pmd_cmdf8_repeat,
  pmd_cmdf7_repeat_exit,
  pmd_cmdf6_set_loop,
  pmd_cmdf5_transpose,
  pmd_cmdf4_volinc_ssg,
  pmd_cmdf3_voldec_ssg,
  pmd_cmdf2_lfo,
  pmd_cmdf1_lfo_switch,
  pmd_cmdf0_env_old,
  pmd_cmdef_poke,
  pmd_cmdee_noise_freq,
  pmd_cmded_ssgmix,
  pmd_cmd_null_1,
  pmd_cmdeb_opnarhythm,
  pmd_cmdea_opnarhythm_il,
  pmd_cmde9_opnarhythm_pan,
  pmd_cmde8_opnarhythm_tl,
  pmd_cmde7_transpose_rel,
  pmd_cmde6_opnarhythm_tl_rel,
  pmd_cmde5_opnarhythm_il_rel,
  pmd_cmd_null_1,
  pmd_cmde3_vol_add_ssg,
  pmd_cmde2_vol_sub,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmddf_meas_len,
  pmd_cmdde_echo_init_add_ssg,
  pmd_cmddd_echo_init_sub,
  pmd_cmddc_status1,
  pmd_cmddb_status1_add,
  pmd_cmdda_portamento_ssg,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdd6_md,
  pmd_cmdd5_detrel,
  pmd_cmdd4_ssgeff,
  pmd_cmdd3_fmeff,
  pmd_cmdd2_fadeout,
  pmd_cmd_null_1,
  pmd_cmdd0_ssgnoisefreqadd,
  pmd_cmd_null_1,
  pmd_cmd_null_6,
  pmd_cmdcd_env_new,
  pmd_cmdcc_det_ext,
  pmd_cmdcb_lfo_waveform,
  pmd_cmdca_lfo_ext,
  pmd_cmdc9_env_ext,
  pmd_cmd_null_3,
  pmd_cmd_null_3,
  pmd_cmd_null_6,
  pmd_cmd_null_1,
  pmd_cmdc4_gate_rel,
  pmd_cmd_null_2,
  pmd_cmdc2_lfo_delay,
  pmd_cmd_null_0,
  pmd_cmdc0_mml_mask_ssg,
  pmd_cmdbf_lfo2,
  pmd_cmdbe_lfo2_switch,
  pmd_cmdbd_lfo2_md,
  pmd_cmdbc_lfo2_waveform,
  pmd_cmdbb_lfo2_ext,
  pmd_cmd_null_1,
  pmd_cmdb9_lfo2_delay,
  pmd_cmd_null_2,
  pmd_cmdb7_lfo_md_cnt,
  pmd_cmd_null_1,
  pmd_cmd_null_2,
  pmd_cmd_null_16,
  pmd_cmdb3_gate_min,
  pmd_cmdb2_transpose_master,
  pmd_cmdb1_gate_rand_range
};

static const pmd_cmd_func pmd_cmd_table_rhythm[PMD_CMD_CNT] = {
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdfd_vol,
  pmd_cmdfc_tempo,
  pmd_cmdfb_tie,
  pmd_cmdfa_det,
  pmd_cmdf9_repeat_reset,
  pmd_cmdf8_repeat,
  pmd_cmdf7_repeat_exit,
  pmd_cmdf6_set_loop,
  pmd_cmd_null_1,
  pmd_cmdf4_volinc_ssg,
  pmd_cmdf3_voldec_ssg,
  pmd_cmd_null_4,
  pmd_cmdf1_ppsdrv,
  pmd_cmd_null_4,
  pmd_cmdef_poke,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdeb_opnarhythm,
  pmd_cmdea_opnarhythm_il,
  pmd_cmde9_opnarhythm_pan,
  pmd_cmde8_opnarhythm_tl,
  pmd_cmd_null_1,
  pmd_cmde6_opnarhythm_tl_rel,
  pmd_cmde5_opnarhythm_il_rel,
  pmd_cmd_null_1,
  pmd_cmde3_vol_add_ssg,
  pmd_cmde2_vol_sub,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmddf_meas_len,
  pmd_cmdde_echo_init_add_ssg,
  pmd_cmddd_echo_init_sub,
  pmd_cmddc_status1,
  pmd_cmddb_status1_add,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_2,
  pmd_cmdd5_detrel,
  pmd_cmdd4_ssgeff,
  pmd_cmdd3_fmeff,
  pmd_cmdd2_fadeout,
  pmd_cmd_null_1,
  pmd_cmd_null_1, // d0
  pmd_cmd_null_1,
  pmd_cmd_null_6,
  pmd_cmd_null_5,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_3, // c8
  pmd_cmd_null_3,
  pmd_cmd_null_6,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_2,
  pmd_cmd_null_1,
  pmd_cmd_null_0,
  pmd_cmdc0_mml_mask_rhythm, // c0
  pmd_cmd_null_4,
  pmd_cmd_null_1,
  pmd_cmd_null_2,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_2, // b8
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_2,
  pmd_cmd_null_16,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1
};

static const pmd_cmd_func pmd_cmd_table_adpcm[PMD_CMD_CNT] = {
  pmd_cmdff_tonenum_adpcm,
  pmd_cmdfe_gate_abs,
  pmd_cmdfd_vol,
  pmd_cmdfc_tempo,
  pmd_cmdfb_tie,
  pmd_cmdfa_det,
  pmd_cmdf9_repeat_reset,
  pmd_cmdf8_repeat,
  pmd_cmdf7_repeat_exit,
  pmd_cmdf6_set_loop,
  pmd_cmdf5_transpose,
  pmd_cmdf4_volinc_adpcm,
  pmd_cmdf3_voldec_adpcm,
  pmd_cmdf2_lfo,
  pmd_cmdf1_lfo_switch,
  pmd_cmdf0_env_old,
  pmd_cmdef_poke,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdec_pan_adpcm,
  pmd_cmdeb_opnarhythm,
  pmd_cmdea_opnarhythm_il,
  pmd_cmde9_opnarhythm_pan,
  pmd_cmde8_opnarhythm_tl,
  pmd_cmde7_transpose_rel,
  pmd_cmde6_opnarhythm_tl_rel,
  pmd_cmde5_opnarhythm_il_rel,
  pmd_cmd_null_1,
  pmd_cmde3_vol_add_adpcm,
  pmd_cmde2_vol_sub,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmddf_meas_len,
  pmd_cmdde_echo_init_add_adpcm,
  pmd_cmddd_echo_init_sub,
  pmd_cmddc_status1,
  pmd_cmddb_status1_add,
  pmd_cmdda_portamento_adpcm,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdd6_md,
  pmd_cmdd5_detrel,
  pmd_cmdd4_ssgeff,
  pmd_cmdd3_fmeff,
  pmd_cmdd2_fadeout,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdce_adpcm_loop,
  pmd_cmdcd_env_new,
  pmd_cmd_null_1,
  pmd_cmdcb_lfo_waveform,
  pmd_cmdca_lfo_ext,
  pmd_cmdc9_env_ext,
  pmd_cmd_null_3,
  pmd_cmd_null_3,
  pmd_cmd_null_6,
  pmd_cmd_null_1,
  pmd_cmdc4_gate_rel,
  pmd_cmdc3_pan_ex_adpcm,
  pmd_cmdc2_lfo_delay,
  pmd_cmd_null_0,
  pmd_cmdc0_mml_mask_adpcm,
  pmd_cmdbf_lfo2,
  pmd_cmdbe_lfo2_switch,
  pmd_cmdbd_lfo2_md,
  pmd_cmdbc_lfo2_waveform,
  pmd_cmdbb_lfo2_ext,
  pmd_cmdba_lfo2_slotmask, // slotmask on ADPCM??
  pmd_cmdb9_lfo2_delay,
  pmd_cmd_null_2,
  pmd_cmdb7_lfo_md_cnt,
  pmd_cmd_null_1,
  pmd_cmd_null_2,
  pmd_cmdb4_ppz8_init,
  pmd_cmdb3_gate_min,
  pmd_cmdb2_transpose_master,
  pmd_cmdb1_gate_rand_range
};

static const pmd_cmd_func pmd_cmd_table_ppz8[PMD_CMD_CNT] = {
  pmd_cmdff_tonenum_ppz8,
  pmd_cmdfe_gate_abs,
  pmd_cmdfd_vol,
  pmd_cmdfc_tempo,
  pmd_cmdfb_tie,
  pmd_cmdfa_det,
  pmd_cmdf9_repeat_reset,
  pmd_cmdf8_repeat,
  pmd_cmdf7_repeat_exit,
  pmd_cmdf6_set_loop,
  pmd_cmdf5_transpose,
  pmd_cmdf4_volinc_adpcm,
  pmd_cmdf3_voldec_adpcm,
  pmd_cmdf2_lfo,
  pmd_cmdf1_lfo_switch,
  pmd_cmdf0_env_old,
  pmd_cmdef_poke,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdec_pan_ppz8,
  pmd_cmdeb_opnarhythm,
  pmd_cmdea_opnarhythm_il,
  pmd_cmde9_opnarhythm_pan,
  pmd_cmde8_opnarhythm_tl,
  pmd_cmde7_transpose_rel,
  pmd_cmde6_opnarhythm_tl_rel,
  pmd_cmde5_opnarhythm_il_rel,
  pmd_cmd_null_1,
  pmd_cmde3_vol_add_adpcm,
  pmd_cmde2_vol_sub,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmddf_meas_len,
  pmd_cmdde_echo_init_add_adpcm,
  pmd_cmddd_echo_init_sub,
  pmd_cmddc_status1,
  pmd_cmddb_status1_add,
  pmd_cmdda_portamento_ppz8,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdd6_md,
  pmd_cmdd5_detrel,
  pmd_cmdd4_ssgeff,
  pmd_cmdd3_fmeff,
  pmd_cmdd2_fadeout,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmd_null_1,
  pmd_cmdce_ppz8_loop,
  pmd_cmdcd_env_new,
  pmd_cmd_null_1,
  pmd_cmdcb_lfo_waveform,
  pmd_cmdca_lfo_ext,
  pmd_cmdc9_env_ext,
  pmd_cmd_null_3,
  pmd_cmd_null_3,
  pmd_cmd_null_6,
  pmd_cmd_null_1,
  pmd_cmdc4_gate_rel,
  pmd_cmdc3_pan_ex_ppz8,
  pmd_cmdc2_lfo_delay,
  pmd_cmd_null_0,
  pmd_cmdc0_mml_mask_ppz8,
  pmd_cmdbf_lfo2,
  pmd_cmdbe_lfo2_switch,
  pmd_cmdbd_lfo2_md,
  pmd_cmdbc_lfo2_waveform,
  pmd_cmdbb_lfo2_ext,
  pmd_cmdba_lfo2_slotmask, // slotmask on PPZ8??
  pmd_cmdb9_lfo2_delay,
  pmd_cmd_null_2,
  pmd_cmdb7_lfo_md_cnt,
  pmd_cmd_null_1,
  pmd_cmd_null_2,
  pmd_cmd_null_16,
  pmd_cmdb3_gate_min,
  pmd_cmdb2_transpose_master,
  pmd_cmdb1_gate_rand_range
};

// 1857
static void pmd_part_cmd_ssg(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t cmd
) {
  if (cmd < 0xb1) {
    part->ptr = 0;
    return;
  }
  pmd_cmd_table_ssg[cmd^0xff](work, pmd, part);
}

// 184d
static void pmd_part_cmd_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t cmd
) {
  if (cmd < 0xb1) {
    part->ptr = 0;
    return;
  }
  pmd_cmd_table_fm[cmd^0xff](work, pmd, part);
}

// 1852
static void pmd_part_cmd_rhythm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t cmd
) {
  if (cmd < 0xb1) {
    part->ptr = 0;
    return;
  }
  pmd_cmd_table_rhythm[cmd^0xff](work, pmd, part);
}

// 02c6
static void pmd_part_cmd_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t cmd
) {
  if (cmd < 0xb1) {
    part->ptr = 0;
    return;
  }
  pmd_cmd_table_adpcm[cmd^0xff](work, pmd, part);
}

// 02c6
static void pmd_part_cmd_ppz8(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t cmd
) {
  if (cmd < 0xb1) {
    part->ptr = 0;
    return;
  }
  pmd_cmd_table_ppz8[cmd^0xff](work, pmd, part);
}

// 20e8
static void pmd_portamento_tick(
  struct pmd_part *part
) {
  part->portamento_diff = u16s16(((uint16_t)part->portamento_diff) + ((uint16_t)part->portamento_add));
  if (part->portamento_rem > 0) {
    part->portamento_rem--;
    part->portamento_diff = u16s16(((uint16_t)part->portamento_diff) + 1);
  } else if (part->portamento_rem < 0) {
    part->portamento_rem++;
    part->portamento_diff = u16s16(((uint16_t)part->portamento_diff) - 1);
  }
}

// 3161
static void pmd_ssg_env_tick_old(
  struct pmd_part *part
) {
  switch (part->ssg_env_state_old) {
  case SSG_ENV_STATE_OLD_AL:
    // 3167
    if (--part->ssg_env_param_set[SSG_ENV_PARAM_OLD_AL]) return;
    part->ssg_env_state_old = SSG_ENV_STATE_OLD_SR;
    part->ssg_env_vol = part->ssg_env_param_set[SSG_ENV_PARAM_OLD_AD];
    return;
  case SSG_ENV_STATE_OLD_SR:
    // 317c
    if (!part->ssg_env_param_set[SSG_ENV_PARAM_OLD_SR]) return;
    if (--part->ssg_env_param_set[SSG_ENV_PARAM_OLD_SR]) return;
    part->ssg_env_param_set[SSG_ENV_PARAM_OLD_SR] = part->ssg_env_param[SSG_ENV_PARAM_OLD_SR];
    part->ssg_env_vol--;
    if ((-15 <= u8s8(part->ssg_env_vol)) && (u8s8(part->ssg_env_vol) < 15)) return;
    part->ssg_env_vol = -15;
    return;
  case SSG_ENV_STATE_OLD_RR:
    // 31a3
    if (!part->ssg_env_param_set[SSG_ENV_PARAM_OLD_RR]) {
      part->ssg_env_vol = -15;
      return;
    }
    if (--part->ssg_env_param_set[SSG_ENV_PARAM_OLD_RR]) return;
    part->ssg_env_param_set[SSG_ENV_PARAM_OLD_RR] = part->ssg_env_param[SSG_ENV_PARAM_OLD_RR];
    part->ssg_env_vol--;
    if ((-15 <= u8s8(part->ssg_env_vol)) && (u8s8(part->ssg_env_vol) < 15)) return;
    part->ssg_env_vol = -15;
    return;
  }
}

// 314e
// returns true if volume update needed
static bool pmd_ssg_env_tick_check(
  struct pmd_part *part
) {
  if (part->ssg_env_state_old == SSG_ENV_STATE_OLD_NEW) {
    return pmd_ssg_env_tick_new_check(part);
  }
  uint8_t old_env_vol = part->ssg_env_vol;
  pmd_ssg_env_tick_old(part);
  return old_env_vol != part->ssg_env_vol;
}

// 312e
// returns true if volume update needed
static bool pmd_ssg_env_proc(
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->flagext.env) {
    // 3134
    uint8_t timera_lapsed = pmd->timera_cnt - pmd->timera_cnt_b;
    bool update_needed = false;
    for (int i = 0; i < timera_lapsed; i++) {
      update_needed |= pmd_ssg_env_tick_check(part);
    }
    return update_needed;
  } else {
    return pmd_ssg_env_tick_check(part);
  }
}

// 1617
static void pmd_part_proc_ssg_lfoenv(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  static const struct pmd_part_lfo_flags lfof_z;
  pmd->lfoprocf = lfof_z;
  pmd->lfoprocf_b = lfof_z;
  pmd->lfoprocf.portamento = part->lfof.portamento;
  if (part->lfof.freq || part->lfof.vol || part->lfof.sync || part->lfof.portamento ||
      part->lfof_b.freq || part->lfof_b.vol || part->lfof_b.sync || part->lfof_b.portamento) {
    // 1625
    if (part->lfof.freq || part->lfof.vol) {
      if (pmd_lfo_tick(pmd, part)) {
        pmd->lfoprocf.freq = part->lfof.freq;
        pmd->lfoprocf.vol = part->lfof.vol;
      }
    }
    // 1637
    if (part->lfof_b.freq || part->lfof_b.vol) {
      pmd_part_lfo_flip(part);
      if (pmd_lfo_tick(pmd, part)) {
        pmd->lfoprocf_b.freq = part->lfof_b.freq;
        pmd->lfoprocf_b.vol = part->lfof_b.vol;
      }
      pmd_part_lfo_flip(part);
    }
    // 1659
    if (pmd->lfoprocf.freq || pmd->lfoprocf.portamento || pmd->lfoprocf_b.freq) {
      if (pmd->lfoprocf.portamento) {
        pmd_portamento_tick(part);
      }
      pmd_ssg_freq_out(work, pmd, part);
    }
  }
  // 166d
  if (pmd_ssg_env_proc(pmd, part) || pmd->lfoprocf.vol || pmd->lfoprocf_b.vol || pmd->fadeout_speed) {
    pmd_ssg_vol_out(work, pmd, part);
  }
  pmd_part_loop_check(pmd, part);
}

// 01fa
static void pmd_part_proc_adpcm_lfoenv(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  static const struct pmd_part_lfo_flags lfof_z;
  pmd->lfoprocf = lfof_z;
  pmd->lfoprocf_b = lfof_z;
  pmd->lfoprocf.portamento = part->lfof.portamento;
  if (part->lfof.freq || part->lfof.vol || part->lfof.sync || part->lfof.portamento ||
      part->lfof_b.freq || part->lfof_b.vol || part->lfof_b.sync || part->lfof_b.portamento) {
    if (part->lfof.freq || part->lfof.vol) {
      if (pmd_lfo_tick(pmd, part)) {
        pmd->lfoprocf.freq = part->lfof.freq;
        pmd->lfoprocf.vol = part->lfof.vol;
      }
    }
    if (part->lfof_b.freq || part->lfof_b.vol) {
      pmd_part_lfo_flip(part);
      if (pmd_lfo_tick(pmd, part)) {
        pmd->lfoprocf_b.freq = part->lfof_b.freq;
        pmd->lfoprocf_b.vol = part->lfof_b.vol;
      }
      pmd_part_lfo_flip(part);
    }
    if (pmd->lfoprocf.freq || pmd->lfoprocf.portamento || pmd->lfoprocf_b.freq) {
      if (pmd->lfoprocf.portamento) {
        pmd_portamento_tick(part);
      }
      pmd_adpcm_freq_out(work, pmd, part);
    }
  }
  // 0250
  if (pmd_ssg_env_proc(pmd, part) || pmd->lfoprocf.vol || pmd->lfoprocf_b.vol || pmd->fadeout_speed) {
    // 049a
    pmd_adpcm_vol_out(work, pmd, part);
  }
  pmd_part_loop_check(pmd, part);
}

// 084c
static void pmd_part_proc_ppz8_lfoenv(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  static const struct pmd_part_lfo_flags lfof_z;
  pmd->lfoprocf = lfof_z;
  pmd->lfoprocf_b = lfof_z;
  pmd->lfoprocf.portamento = part->lfof.portamento;
  
  if (part->lfof.freq || part->lfof.vol || part->lfof.sync || part->lfof.portamento ||
      part->lfof_b.freq || part->lfof_b.vol || part->lfof_b.sync || part->lfof_b.portamento) {
    // 085a
    if (part->lfof.freq || part->lfof.vol) {
      if (pmd_lfo_tick(pmd, part)) {
        pmd->lfoprocf.freq = part->lfof.freq;
        pmd->lfoprocf.vol = part->lfof.vol;
      }
    }
    if (part->lfof_b.freq || part->lfof_b.vol) {
      pmd_part_lfo_flip(part);
      if (pmd_lfo_tick(pmd, part)) {
        pmd->lfoprocf_b.freq = part->lfof_b.freq;
        pmd->lfoprocf_b.vol = part->lfof_b.vol;
      }
      pmd_part_lfo_flip(part);
    }
    // 088e
    if (pmd->lfoprocf.freq || pmd->lfoprocf.portamento || pmd->lfoprocf_b.freq) {
      if (pmd->lfoprocf.portamento) {
        pmd_portamento_tick(part);
      }
      pmd_ppz8_freq_out(work, pmd, part);
    }
  }
  // 08a2
  if (pmd_ssg_env_proc(pmd, part) || pmd->lfoprocf.vol || pmd->lfoprocf_b.vol || pmd->fadeout_speed) {
    pmd_ppz8_vol_out(work, pmd, part);
  }
  pmd_part_loop_check(pmd, part);
}

// 16d9
static bool pmd_part_ssg_next_masked(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part,
  uint8_t note
) {
  if (part->mask.ext) return true;
  if (!part->mask.effect) return true;
  if (pmd->ssgeff_priority >= 2) return true;
  if ((note & 0xf) == 0xf) return true;
  if (pmd->ssgeff_priority == 1) {
    pmd_ssgeff_stop(work, pmd);
  }
  part->mask.effect = false;
  return pmd_part_masked(part);
}

// 1561
static void pmd_part_loop_check_masked(
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  pmd->no_keyoff = false;
  pmd->volume_saved = false;
  pmd_part_loop_check(pmd, part);
}

// 1541
static void pmd_part_proc_note_masked(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  (void)work;
  part->actual_freq = 0;
  part->actual_note = 0xff;
  part->curr_note = 0xff;
  part->len = part->len_cnt = pmd_part_cmdload(pmd, part);
  part->note_proc++;
  
  if (!pmd->volume_saved) {
    part->volume_save = 0;
  }
  pmd->no_keyoff = false;
  pmd->volume_saved = false;
  pmd_part_loop_check(pmd, part);
}

// 2cb5
static void pmd_part_off_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->keystatus.off && part->keystatus.off_mask) return;
  pmd_part_do_keyoff_fm(work, pmd, part);
}

// 13f8
static void pmd_part_proc_fm_lfo(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->hlfo_delay) {
    if (!--part->hlfo_delay) {
      pmd_reg_write(work, pmd, 0xb3+pmd->proc_ch, part->pan);
    }
  }
  // 1410
  if (part->slot_delay_cnt) {
    if (!--part->slot_delay_cnt) {
      if (!part->keystatus.off) {
        pmd_part_fm_keyon(work, pmd, part);
      }
    }
  }
  static const struct pmd_part_lfo_flags lfof_z;
  pmd->lfoprocf = lfof_z;
  pmd->lfoprocf_b = lfof_z;
  pmd->lfoprocf.portamento = part->lfof.portamento;
  // 1424
  if (part->lfof.freq || part->lfof.vol || part->lfof.sync || part->lfof.portamento ||
      part->lfof_b.freq || part->lfof_b.vol || part->lfof_b.sync || part->lfof_b.portamento) {
    if (part->lfof.freq || part->lfof.vol) {
      if (pmd_lfo_tick(pmd, part)) {
        pmd->lfoprocf.freq = part->lfof.freq;
        pmd->lfoprocf.vol = part->lfof.vol;
      }
    }
    if (part->lfof_b.freq || part->lfof_b.vol) {
      pmd_part_lfo_flip(part);
      if (pmd_lfo_tick(pmd, part)) {
        pmd->lfoprocf_b.freq = part->lfof.freq;
        pmd->lfoprocf_b.vol = part->lfof.vol;
      }
      pmd_part_lfo_flip(part);
    }
    // 1466
    if (pmd->lfoprocf.freq || pmd->lfoprocf.portamento || pmd->lfoprocf_b.freq) {
      if (pmd->lfoprocf.portamento) {
        pmd_portamento_tick(part);
      }
      pmd_fm_freq_out(work, pmd, part);
    }
    // 147a
  }
  // 1481
  if (pmd->lfoprocf.vol || pmd->lfoprocf_b.vol || pmd->fadeout_speed) {
    pmd_fm_vol_out(work, pmd, part);
  }
  pmd_part_loop_check(pmd, part);
}

// 1350 / 14fc
static void pmd_part_proc_fm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (!part->ptr) return;
  part->proc_masked = pmd_part_masked(part);
  part->len_cnt--;
  if (part->proc_masked) {
    part->keystatus.off = true;
    part->keystatus.off_mask = true;
  } else if (!part->keystatus.off && !part->keystatus.off_mask) {
    if (part->len_cnt <= part->gate) {
      pmd_part_off_fm(work, pmd, part);
      part->keystatus.off = true;
      part->keystatus.off_mask = true;
    }
  }
  // 1377
  if (part->len_cnt) {
    if (part->proc_masked) {
      pmd_part_loop_check(pmd, part);
    } else {
      pmd_part_proc_fm_lfo(work, pmd, part);
    }
    return;
  }
  if (part->proc_masked) {
    // 1505
    if (part->mask.effect && !pmd->fmeff_playing) {
      part->mask.effect = false;
      part->proc_masked = pmd_part_masked(part);
    }
  }
  // 137b
  if (!part->proc_masked) {
    part->lfof.portamento = false;
  }
  for (;;) {
    // 137f / 151b
    uint8_t cmd = pmd_part_cmdload(pmd, part);
    if (cmd & 0x80) {
      if (cmd != 0x80) {
        // 1386 / 1522
        pmd_part_cmd_fm(work, pmd, part, cmd);
        if (cmd == 0xda && !pmd_part_masked(part)) return;
      } else {
        // 138b / 1527
        part->ptr = 0;
        part->loop.looped = true;
        part->loop.ended = true;
        part->actual_note = 0xff;
        if (!part->loop_ptr) {
          if (part->proc_masked) {
            pmd_part_loop_check_masked(pmd, part);
          } else {
            pmd_part_proc_fm_lfo(work, pmd, part);
          }
          return;
        }
        part->ptr = part->loop_ptr;
        part->loop.ended = false;
      }
    } else {
      // 13a5 / 1541
      if (part->proc_masked) {
        pmd_part_proc_note_masked(work, pmd, part);
        return;
      } else {
        cmd = pmd_part_lfo_init_fm(work, pmd, part, cmd);
        cmd = pmd_part_note_transpose(part, cmd);
        pmd_note_freq_fm(part, cmd);
        part->len = part->len_cnt = pmd_part_cmdload(pmd, part);
        pmd_part_calc_gate(pmd, part);
        // 13b5
        pmd_part_fm_out(work, pmd, part);
        return;
      }
    }
  }
}

// 156f / 1689
static void pmd_part_proc_ssg(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  /*
  if (!part->ptr) {
    // original
    if (part->ssg_env_state_old == SSG_ENV_STATE_OLD_NEW) {
      part->ssg_env_state_new = SSG_ENV_STATE_NEW_RR;
    } else {
      part->ssg_env_state_old = SSG_ENV_STATE_OLD_RR;
    }
    // original end
    return;
  }
  */
  part->proc_masked = pmd_part_masked(part);
  
  part->len_cnt--;
  if (part->proc_masked) {
    part->keystatus.off = true;
    part->keystatus.off_mask = true;
  } else if (!part->keystatus.off && !part->keystatus.off_mask) {
    if (part->len_cnt <= part->gate) {
      pmd_part_off_ssg(part);
      part->keystatus.off = true;
      part->keystatus.off_mask = true;
    }
  }
  // 1596
  if (part->len_cnt) {
    if (part->proc_masked) {
      pmd_part_loop_check(pmd, part);
    } else {
      pmd_part_proc_ssg_lfoenv(work, pmd, part);
    }
    return;
  }
  part->lfof.portamento = false;
  for (;;) {
    // 159e / 1699
    uint8_t cmd = pmd_part_cmdload(pmd, part);
    if (cmd & 0x80) {
      if (cmd != 0x80) {
        if (part->proc_masked && (cmd == 0xda)) {
          // 16a0
          part->proc_masked = pmd_part_ssg_next_masked(work, pmd, part, cmd);
        }
        // 15a5
        pmd_part_cmd_ssg(work, pmd, part, cmd);
        if (cmd == 0xda && !pmd_part_masked(part)) return;
      } else {
        // 15aa
        part->ptr = 0;
        part->loop.looped = true;
        part->loop.ended = true;
        part->actual_note = 0xff;
        if (!part->loop_ptr) {
          if (part->proc_masked) {
            pmd_part_loop_check_masked(pmd, part);
          } else {
            // 1617
            pmd_part_proc_ssg_lfoenv(work, pmd, part);
          }
          return;
        }
        // 15bc
        part->ptr = part->loop_ptr;
        part->loop.ended = false;
      }
    } else {
      if (part->proc_masked && pmd_part_ssg_next_masked(work, pmd, part, cmd)) {
        // 16ce
        pmd_part_proc_note_masked(work, pmd, part);
        return;
      } else {
        // 15c4
        cmd = pmd_part_lfo_init_ssg(work, pmd, part, cmd);
        cmd = pmd_part_note_transpose(part, cmd);
        pmd_note_freq_ssg(part, cmd);
        part->len = part->len_cnt = pmd_part_cmdload(pmd, part);
        pmd_part_calc_gate(pmd, part);
        // 15d4
        if (part->volume_save && (part->actual_note != 0xff)) {
          if (!pmd->volume_saved) {
            part->volume_save = 0;
          }
          pmd->volume_saved = false;
        }
        // 15ef
        pmd_ssg_vol_out(work, pmd, part);
        pmd_ssg_freq_out(work, pmd, part);
        pmd_ssg_note_noise_out(work, pmd, part);
        part->note_proc++;
        pmd->no_keyoff = false;
        pmd->volume_saved = false;
        part->keystatus.off = false;
        part->keystatus.off_mask = false;
        // fb
        if (pmd->datalen > (part->ptr + 1)) {
          if (pmd->data[part->ptr] == 0xfb) {
            part->keystatus.off_mask = true;
          }
        }
        pmd_part_loop_check(pmd, part);
        return;
      }
    }
  }
  //pmd_part_proc_ssg_lfoenv(work, pmd, part);
}

// 42d6
static const struct pmd_ssgrhythm_opna_table_entry {
  uint8_t reg;
  uint8_t data;
  uint8_t kon;
} pmd_ssgrhythm_opna_table[11] = {
  {0x18, 0xdf, 0x01},
  {0x19, 0xdf, 0x02},
  {0x1c, 0x5f, 0x10},
  {0x1c, 0xdf, 0x10},
  {0x1c, 0x9f, 0x10},
  {0x1d, 0xd3, 0x20},
  {0x19, 0xdf, 0x02},
  {0x1b, 0x9c, 0x88},
  {0x1a, 0x9d, 0x04},
  {0x1a, 0xdf, 0x04},
  {0x1a, 0x5e, 0x04},
};

// 1813
static void pmd_ssgrhythm_opna_out(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  const struct pmd_ssgrhythm_opna_table_entry *r
) {
  work->opna_writereg(work, r->reg, r->data);
  uint8_t kon = r->kon & pmd->opnarhythm_mask;
  if (!kon) return;
  if (kon & 0x80) {
    // 1828
    work->opna_writereg(work, 0x10, 0x84);
    kon = 0x08 & pmd->opnarhythm_mask;
    if (!kon) return;
  }
  work->opna_writereg(work, 0x10, kon);
}

// 170d
static void pmd_part_proc_opnarhythm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (!part->ptr) return;
  
  if (--part->len_cnt) {
    pmd_part_loop_check(pmd, part);
    return;
  }
  // 171b
  for (;;) {
    while (pmd->r_ptr && (pmd->datalen >= (pmd->r_ptr+1)) && (pmd->data[pmd->r_ptr] != 0xff)) {
      // 1726
      uint8_t rcmd = pmd->data[pmd->r_ptr++];
      if (rcmd & 0x80) {
        // 1785
        if (rcmd & 0x40) {
          uint16_t tmp = part->ptr;
          part->ptr = pmd->r_ptr;
          pmd->r_ptr = tmp;
          pmd_part_cmd_rhythm(work, pmd, part, rcmd);
          tmp = part->ptr;
          part->ptr = pmd->r_ptr;
          pmd->r_ptr = tmp;
          continue;
        }
        // 1794
        if (pmd_part_masked(part)) {
          pmd->ssgrhythm = 0;
          pmd->r_ptr++;
          part->len = part->len_cnt = pmd->data[pmd->r_ptr++];
          pmd_part_loop_check_masked(pmd, part);
          part->note_proc++;
          return;
        }
        pmd->ssgrhythm = ((rcmd << 8) | pmd->data[pmd->r_ptr++]) & 0x3fff;
        if (pmd->ssgrhythm) {
          // 17b0
          if (pmd->ssgrhythm_opna) {
            for (int i = 0; i < 11; i++) {
              if (pmd->ssgrhythm & (1<<i)) {
                pmd_ssgrhythm_opna_out(work, pmd, &pmd_ssgrhythm_opna_table[i]);
              }
            }
          }
          // 17cc
          if (pmd->fadeout_vol) {
            if (pmd->ssgrhythm_opna) {
              pmd_opnarhythm_tl_write(work, pmd, pmd->opnarhythm_tl);
            }
          }
          // 17ea
          if (!pmd->fadeout_vol || pmd->ppsdrv_enabled) {
            // TODO: PPSDRV
          }
          // 180c
          uint8_t eff;
          for (eff = 0;; eff++) {
            if (pmd->ssgrhythm & (1<<eff)) break;
          }
          pmd_ssg_effect(work, pmd, eff);
        }
      } else {
        pmd->ssgrhythm = 0;
      }
      part->len = part->len_cnt = pmd->data[pmd->r_ptr++];
      pmd_part_loop_check_masked(pmd, part);
      part->note_proc++;
      return;
    }
    // 1740
    for (;;) {
      uint8_t cmd = pmd_part_cmdload(pmd, part);
      if (cmd & 0x80) {
        if (cmd != 0x80) {
          pmd_part_cmd_rhythm(work, pmd, part, cmd);
        } else {
          // 1765
          part->ptr = 0;
          part->loop.looped = true;
          part->loop.ended = true;
          if (!part->loop_ptr) {
            part->ptr = 0;
            pmd_part_loop_check_masked(pmd, part);
            return;
          }
          part->ptr = part->loop_ptr;
          part->loop.ended = false;
        }
      } else {
        // 174c
        uint16_t ptr = pmd->r_offset + cmd*2;
        if (pmd->datalen < (ptr + 2)) {
          part->ptr = 0;
          return;
        }
        pmd->r_ptr = pmd->data[ptr] | (((uint16_t)pmd->data[ptr+1]) << 8);
        break;
      }
    }
  }
}

// 05cf
static void pmd_part_off_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (part->ssg_env_state_old == SSG_ENV_STATE_OLD_NEW) {
    if (part->ssg_env_state_new == SSG_ENV_STATE_NEW_RR) return;
  } else {
    if (part->ssg_env_state_old == SSG_ENV_STATE_OLD_RR) return;
  }
  // 05e2
  if (pmd->adpcm_release != 0x8000) {
    work->opna_writereg(work, 0x100, 0x21);
    work->opna_writereg(work, 0x102, pmd->adpcm_release);
    work->opna_writereg(work, 0x103, pmd->adpcm_release>>8);
    work->opna_writereg(work, 0x104, pmd->adpcm_stop);
    work->opna_writereg(work, 0x105, pmd->adpcm_stop>>8);
    work->opna_writereg(work, 0x100, 0xa0);
  }
  pmd_part_off_ssg(part);
}

// 0c0e
static void pmd_part_off_ppz8(
  struct pmd_part *part
) {
  if (part->ssg_env_state_old != SSG_ENV_STATE_OLD_NEW) {
    if (part->ssg_env_state_old != SSG_ENV_STATE_OLD_RR) pmd_part_off_ssg(part);
  } else {
    if (part->ssg_env_state_new != SSG_ENV_STATE_NEW_RR) pmd_part_off_ssg(part);
  }
}

// 0149 / 026c
static void pmd_part_proc_adpcm(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  if (!part->ptr) return;
  part->proc_masked = pmd_part_masked(part);
  part->len_cnt--;
  if (part->proc_masked) {
    part->keystatus.off = true;
    part->keystatus.off_mask = true;
  } else if (!part->keystatus.off && !part->keystatus.off_mask) {
    //0164
    if (part->len_cnt <= part->gate) {
      part->keystatus.off = true;
      part->keystatus.off_mask = true;
      pmd_part_off_adpcm(work, pmd, part);
    }
  }
  // 0170 / 0273
  if (part->len_cnt) {
    if (part->proc_masked) {
      pmd_part_loop_check(pmd, part);
    } else {
      pmd_part_proc_adpcm_lfoenv(work, pmd, part);
    }
    return;
  }
  // PCM effect not implemented
  // 0177
  part->lfof.portamento = false;
  for (;;) {
    // 017b / 029a
    uint8_t cmd = pmd_part_cmdload(pmd, part);
    if (cmd & 0x80) {
      if (cmd != 0x80) {
        pmd_part_cmd_adpcm(work, pmd, part, cmd);
        if (cmd == 0xda && !pmd_part_masked(part)) return;
      } else {
        // 0187
        part->ptr = 0;
        part->loop.looped = true;
        part->loop.ended = true;
        part->actual_note = 0xff;
        if (!part->loop_ptr) {
          if (part->proc_masked) {
            pmd_part_loop_check_masked(pmd, part);
          } else {
            pmd_part_proc_adpcm_lfoenv(work, pmd, part);
          }
          return;
        }
        part->ptr = part->loop_ptr;
        part->loop.ended = false;
      }
    } else {
      // 01a1 / 02a1
      if (part->proc_masked) {
        pmd_part_proc_note_masked(work, pmd, part);
      } else {
        cmd = pmd_part_lfo_init_ssg(work, pmd, part, cmd);
        cmd = pmd_part_note_transpose(part, cmd);
        pmd_note_freq_adpcm(part, cmd);
        part->len = part->len_cnt = pmd_part_cmdload(pmd, part);
        pmd_part_calc_gate(pmd, part);
        pmd_part_adpcm_out(work, pmd, part);
      }
      return;
    }
  }
}

// 079b / 08be
static void pmd_part_proc_ppz8(
  struct fmdriver_work *work,
  struct driver_pmd *pmd,
  struct pmd_part *part
) {
  //if (!part->ptr) return;
  part->proc_masked = pmd_part_masked(part);
  part->len_cnt--;
  if (part->proc_masked) {
    part->keystatus.off = true;
    part->keystatus.off_mask = true;
  } else if (!part->keystatus.off && !part->keystatus.off_mask) {
    // 07b6
    if (part->len_cnt <= part->gate) {
      part->keystatus.off = true;
      part->keystatus.off_mask = true;
      pmd_part_off_ppz8(part);
    }
  }
  // 07c2
  if (part->len_cnt) {
    if (part->proc_masked) {
      pmd_part_loop_check(pmd, part);
    } else {
      pmd_part_proc_ppz8_lfoenv(work, pmd, part);
    }
    return;
  }
  part->lfof.portamento = false;
  for (;;) {
    // 07cd / 08ca
    uint8_t cmd = pmd_part_cmdload(pmd, part);
    if (cmd & 0x80) {
      if (cmd != 0x80) {
        pmd_part_cmd_ppz8(work, pmd, part, cmd);
        if (cmd == 0xda && !pmd_part_masked(part)) return;
      } else {
        // 07d9 / 08d6
        part->ptr = 0;
        part->loop.looped = true;
        part->loop.ended = true;
        part->actual_note = 0xff;
        if (!part->loop_ptr) {
          if (part->proc_masked) {
            pmd_part_loop_check_masked(pmd, part);
          } else {
            pmd_part_proc_ppz8_lfoenv(work, pmd, part);
          }
          return;
        }
        part->ptr = part->loop_ptr;
        part->loop.ended = false;
      }
    } else {
      // 07f3 / 08f3
      if (part->proc_masked) {
        part->actual_freq_upper = 0;
        pmd_part_proc_note_masked(work, pmd, part);
      } else {
        cmd = pmd_part_lfo_init_ssg(work, pmd, part, cmd);
        cmd = pmd_part_note_transpose(part, cmd);
        pmd_note_freq_ppz8(part, cmd);
        part->len = part->len_cnt = pmd_part_cmdload(pmd, part);
        pmd_part_calc_gate(pmd, part);
        pmd_part_ppz8_out(work, pmd, part);
      }
      return;
    }
  }
}

// 11fd
static void pmd_proc_parts(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  pmd->loop.looped = true;
  pmd->loop.ended = true;
  pmd->loop.env = false;
  if (!pmd->opm_flag) {
    for (int c = 0; c < 3; c++) {
      pmd->proc_ch = c+1;
      struct pmd_part *part = &pmd->parts[PMD_PART_SSG_1+c];
      pmd_part_proc_ssg(work, pmd, part);
    }
  }
  // 122a
  pmd_opna_a1_on(pmd);
  for (int c = 0; c < 3; c++) {
    pmd->proc_ch = c+1;
    struct pmd_part *part = &pmd->parts[PMD_PART_FM_4+c];
    pmd_part_proc_fm(work, pmd, part);
  }
  pmd_opna_a1_off(pmd);
  for (int c = 0; c < 3; c++) {
    pmd->proc_ch = c+1;
    struct pmd_part *part = &pmd->parts[PMD_PART_FM_1+c];
    pmd_part_proc_fm(work, pmd, part);
  }
  // 1272
  for (int c = 0; c < 3; c++) {
    struct pmd_part *part = &pmd->parts[PMD_PART_FM_3B+c];
    pmd_part_proc_fm(work, pmd, part);
  }
  // 1284
  if (!pmd->opm_flag) {
    pmd_part_proc_opnarhythm(work, pmd, &pmd->parts[PMD_PART_RHYTHM]);
    pmd_part_proc_adpcm(work, pmd, &pmd->parts[PMD_PART_ADPCM]);
    for (int c = 0; c < 8; c++) {
      pmd->proc_ch = c;
      struct pmd_part *part = &pmd->parts[PMD_PART_PPZ_1+c];
      pmd_part_proc_ppz8(work, pmd, part);
    }
  }
  // 12ef
  if (!pmd->loop.looped && !pmd->loop.ended && !pmd->loop.env) return;
  // 12f7
  for (int i = 0; i < 12; i++) {
    struct pmd_part *part = &pmd->parts[PMD_PART_FM_1+i];
    if (!part->loop.looped || !part->loop.ended || part->loop.env) {
      part->loop.looped = false;
      part->loop.ended = false;
      part->loop.env = false;
    }
  }
  // 130d
  if (!pmd->loop.looped || !pmd->loop.ended || pmd->loop.env) {
    work->timerb_cnt_loop = 0;
    if (++pmd->status2 == 0xff) pmd->status2 = 1;
  } else {
    pmd->status2 = 0xff;
    // stop
    work->playing = false;
  }
  work->loop_cnt = pmd->status2;
}

// 3e2e
static void pmd_update_note_meas(
  struct driver_pmd *pmd
) {
  uint8_t tick = ++pmd->tick_cnt;
  if (tick == pmd->meas_len) {
    pmd->tick_cnt = 0;
    pmd->meas_cnt++;
  }
}

// 3d68
static void pmd_timerb(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  if (pmd->playing) {
    // 3d8f
    FMDRIVER_DEBUG("timerb\n");
    pmd_proc_parts(work, pmd);
    pmd_timerb_write(work, pmd);
    pmd_update_note_meas(pmd);
    pmd->timera_cnt_b = pmd->timera_cnt;
  }
  // 3d9e
  // user interrupt
}

// 329e
static void pmd_fadeout_tick(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  if (pmd->paused) return;
  if (!pmd->fadeout_speed) return;
  int newvol = pmd->fadeout_vol + u8s8(pmd->fadeout_speed);
  if (newvol < 0) {
    newvol = 0;
    pmd->fadeout_vol = 0;
    pmd->fadeout_speed = 0;
    pmd_opnarhythm_tl_write(work, pmd, pmd->opnarhythm_tl);
  } else if (newvol > 0xff) {
    newvol = 0xff;
    pmd->fadeout_speed = 0;
    //if (pmd_fadeout_mstop) pmd->
  }
}

// 3daf
static void pmd_timera(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  if (!(++pmd->timera_cnt & 7)) {
    // 3dbf
    pmd_fadeout_tick(work, pmd);
    // 3e66: fast-forward
  }
  if (pmd->ssgeff_priority) {
    // TODO: PPSDRV
    if (!pmd->ppsdrv_enabled || (!(pmd->ssgeff_num & 0x80))) {
      pmd_ssgeff_tick(work, pmd);
    }
  }
  // 3ddd
  if (pmd->fmeff_playing) {
    // TODO: FMEFF
    // call 0x0f03
  }
  // esc/grph key check??
}

// 3d1b
static void pmd_timer(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  uint8_t status = work->opna_status(work, 0);
  work->opna_writereg(work, 0x27, pmd->fm3ex_mode);
  if (status & 1) {
    pmd_timera(work, pmd);
  }
  if (status & 2) {
    pmd_timerb(work, pmd);
    if (work->playing) {
      work->timerb_cnt++;
      work->timerb_cnt_loop++;
    }
  }
}

enum pmd_part_type {
  PART_TYPE_FM,
  PART_TYPE_SSG,
  PART_TYPE_ADPCM,
  PART_TYPE_PPZ8
};

static const struct {
  uint8_t ind;
  enum pmd_part_type type;
  bool fm3ex;
} pmd_track_map[FMDRIVER_TRACK_NUM] = {
  {PMD_PART_FM_1,  PART_TYPE_FM,    false},
  {PMD_PART_FM_2,  PART_TYPE_FM,    false},
  {PMD_PART_FM_3,  PART_TYPE_FM,    true},
  {PMD_PART_FM_3B, PART_TYPE_FM,    true},
  {PMD_PART_FM_3C, PART_TYPE_FM,    true},
  {PMD_PART_FM_3D, PART_TYPE_FM,    true},
  {PMD_PART_FM_4,  PART_TYPE_FM,    false},
  {PMD_PART_FM_5,  PART_TYPE_FM,    false},
  {PMD_PART_FM_6,  PART_TYPE_FM,    false},
  {PMD_PART_SSG_1, PART_TYPE_SSG,   false},
  {PMD_PART_SSG_2, PART_TYPE_SSG,   false},
  {PMD_PART_SSG_3, PART_TYPE_SSG,   false},
  {PMD_PART_ADPCM, PART_TYPE_ADPCM, false},
  {PMD_PART_PPZ_1, PART_TYPE_PPZ8,   false},
  {PMD_PART_PPZ_2, PART_TYPE_PPZ8,   false},
  {PMD_PART_PPZ_3, PART_TYPE_PPZ8,   false},
  {PMD_PART_PPZ_4, PART_TYPE_PPZ8,   false},
  {PMD_PART_PPZ_5, PART_TYPE_PPZ8,   false},
  {PMD_PART_PPZ_6, PART_TYPE_PPZ8,   false},
  {PMD_PART_PPZ_7, PART_TYPE_PPZ8,   false},
  {PMD_PART_PPZ_8, PART_TYPE_PPZ8,   false},
};

static void pmd_work_status_update(
  struct fmdriver_work *work,
  const struct driver_pmd *pmd
) {
  for (int t = 0; t < FMDRIVER_TRACK_NUM; t++) {
    struct fmdriver_track_status *track = &work->track_status[t];
    const struct pmd_part *part = &pmd->parts[pmd_track_map[t].ind];
    track->playing = !part->loop.ended;
    track->info = FMDRIVER_TRACK_INFO_NORMAL;
    track->ticks = part->len;
    track->ticks_left = part->len_cnt;
    track->key = part->actual_note;
    track->fmslotmask[0] = !(part->fm_slotmask & (1<<4));
    track->fmslotmask[1] = !(part->fm_slotmask & (1<<5));
    track->fmslotmask[2] = !(part->fm_slotmask & (1<<6));
    track->fmslotmask[3] = !(part->fm_slotmask & (1<<7));
    track->tonenum = part->tonenum;
    track->volume = part->vol;
    track->gate = part->gate;
    int detune = part->detune;
    if (detune > INT8_MAX) detune = INT8_MAX;
    if (detune < INT8_MIN) detune = INT8_MIN;
    track->detune = detune;
    track->status[0] = part->lfof.freq ? 'P' : '-';
    track->status[1] = part->lfof.vol ? 'A' : '-';
    track->status[2] = part->lfof.sync ? 'S' : '-';
    track->status[3] = part->lfof_b.freq ? 'P' : '-';
    track->status[4] = part->lfof_b.vol ? 'A' : '-';
    track->status[5] = part->lfof_b.sync ? 'S' : '-';
    track->status[6] = '-';
    track->status[7] = part->lfof.portamento ? 'P' : '-';
    switch (pmd_track_map[t].type) {
    case PART_TYPE_FM:
      track->actual_key = ((track->key & 0xf) == 0xf) ?
        0xff : fmdriver_fm_freq2key(part->output_freq);
      if (pmd_track_map[t].fm3ex) {
        if (part->fm_slotmask != 0xf0) track->info = FMDRIVER_TRACK_INFO_FM3EX;
      }
      break;
    case PART_TYPE_SSG:
      track->info = FMDRIVER_TRACK_INFO_SSG;
      track->actual_key = ((track->key & 0xf) == 0xf) ?
        0xff : fmdriver_ssg_freq2key(part->output_freq);
      track->ssg_tone = part->ssg_mix & (1<<0);
      track->ssg_noise = part->ssg_mix & (1<<3);
      if (part == &pmd->parts[PMD_PART_SSG_3]) {
        if (pmd->ssgeff_on) {
          track->info = FMDRIVER_TRACK_INFO_SSGEFF;
          track->tonenum = pmd->ssgeff_num;
          track->ssg_tone = pmd->ssgeff_tone_mix;
          track->ssg_noise = pmd->ssgeff_noise_mix;
          track->actual_key = pmd->ssgeff_tone_mix ?
            fmdriver_ssg_freq2key(pmd->ssgeff_tonefreq) : 0xff;
        }
      }
      break;
    case PART_TYPE_ADPCM:
      track->actual_key = ((track->key & 0xf) == 0xf) ?
        0xff : pmd_adpcm_freq2key(part->output_freq);
      break;
    case PART_TYPE_PPZ8:
      track->actual_key = ((track->key & 0xf) == 0xf) ?
        0xff : fmdriver_ppz8_freq2key(part->output_freq);
      break;
    }
  }
  work->ssg_noise_freq = pmd->ssg_noise_freq_wrote;
}

// 3c76
static void pmd_opna_interrupt(struct fmdriver_work *work) {
  struct driver_pmd *pmd = (struct driver_pmd *)work->driver;
  // 3d1b
  for (;;) {
    uint8_t status = work->opna_status(work, 0);
    if (status & 3) {
      pmd_timer(work, pmd);
    } else {
      break;
    }
  }
  // 3cc0
  pmd_work_status_update(work, pmd);
}

/*
// 0f4d
static void pmd_mstart(
  struct fmdriver_work *work,
  struct driver_pmd *pmd
) {
  pmd_mstop(work, pmd);
  pmd_reset_state(pmd);
  pmd_data_init(pmd);
  pmd->fadeout_vol = 0;
  if (pmd->adpcm_disabled) {
    pmd->parts[PMD_PART_ADPCM].mask.disabled = true;
  }
  // reset PPZ8
  
  // 0f99
  pmd_reset_opna(work, pmd);
  pmd_reset_timer(work, pmd);
  pmd->playing = true;
  // TODO
  // 4375??
}
*/

bool pmd_load(struct driver_pmd *pmd,
              uint8_t *data, uint16_t datalen) {
  if (!datalen) return false;
  pmd->data = data + 1;
  pmd->datalen = datalen - 1;
  pmd_reset_state(pmd);
  pmd->ssgeff_num = 0xff;
  if (!pmd_data_init(pmd)) return false;
  pmd->fadeout_vol = 0;
  if (pmd->adpcm_disabled) {
    pmd->parts[PMD_PART_ADPCM].mask.disabled = true;
  }
  pmd->opnarhythm_mask = 0xff;
  pmd->ssgrhythm_opna = true;
  return true;
}

// check if null-terminated string is valid
static const char *pmd_check_str(
  const struct driver_pmd *pmd,
  uint16_t ptr
) {
  uint16_t c = ptr;
  while (pmd->datalen >= (c+1)) {
    if (!pmd->data[c++]) return (const char *)&pmd->data[ptr];
  }
  return 0;
}

// 383e
const char *pmd_get_memo(
  const struct driver_pmd *pmd,
  int index
) {
  if (index < -2) return 0;
  if (pmd->datalen < 2) return 0;
  if (read16le(&pmd->data[0]) == 0x18) return 0;
  if (pmd->datalen < (0x18+2)) return 0;
  uint16_t toneptr = read16le(&pmd->data[0x18]);
  if (toneptr < 4) return 0;
  if (pmd->datalen < toneptr) return 0;
  uint8_t flaglow = pmd->data[toneptr-2];
  uint8_t flaghigh = pmd->data[toneptr-1];
  if (flaglow != 0x40) {
    if (flaghigh != 0xfe) return 0;
    if (flaglow < 0x41) return 0;
  }
  if (flaglow >= 0x42) index++;
  if (flaglow >= 0x48) index++;
  if (index < 0) return 0;
  // 3873
  uint16_t memoptr = read16le(&pmd->data[toneptr-4]);
  while ((pmd->datalen >= (memoptr+2)) && read16le(&pmd->data[memoptr])) {
    if (index == 0) {
      if (pmd->datalen < (memoptr+2)) return 0;
      return pmd_check_str(pmd, read16le(&pmd->data[memoptr]));
    }
    memoptr += 2;
    index--;
  }
  return 0;
}

void pmd_filenamecopy(char *dest, const char *src) {
  int i;
  for (i = 0; i < (PMD_FILENAMELEN+1); i++) {
    if (src[i] == '.' || src[i] == ',') {
      dest[i] = 0;
      return;
    }
    dest[i] = src[i];
    if (!src[i]) return;
  }
  dest[0] = 0;
}

static const char *pmd_get_comment(struct fmdriver_work *work, int line) {
  struct driver_pmd *pmd = work->driver;
  if (line < 0) return 0;
  const char *str = pmd_get_memo(pmd, line + 1);
  if (str && !*str) return 0;
  return str;
}

void pmd_init(struct fmdriver_work *work,
              struct driver_pmd *pmd) {
  // TODO: reset ppz8

  // 0f99
  work->driver = pmd;
  work->comment_mode_pmd = true;
  work->get_comment = pmd_get_comment;
  pmd_reset_opna(work, pmd);
  pmd_reset_timer(work, pmd);
  pmd->playing = true;
  work->driver_opna_interrupt = pmd_opna_interrupt;

  /*
  static const int memotable[3] = {1, 4, 5};
  for (int i = 0; i < 3; i++) {
    const char *title = pmd_get_memo(pmd, memotable[i]);
    int c = 0;
    if (title) {
      while (title[c] && (c < (FMDRIVER_TITLE_BUFLEN-1))) {
        work->comment[i][c] = title[c];
        c++;
      }
    }
    work->comment[i][c] = 0;
  }
  */
  const char *pcmfile = pmd_get_memo(pmd, -2);
  if (pcmfile) {
    pmd_filenamecopy(pmd->ppzfile, pcmfile);
    const char *pcm2 = strchr(pcmfile, ',');
    if (pcm2) {
      pmd_filenamecopy(pmd->ppzfile2, pcm2+1);
    }
  }
  pcmfile = pmd_get_memo(pmd, -1);
  if (pcmfile) {
    pmd_filenamecopy(pmd->ppsfile, pcmfile);
  }
  pcmfile = pmd_get_memo(pmd, 0);
  if (pcmfile) {
    pmd_filenamecopy(pmd->ppcfile, pcmfile);
  }
  fmdriver_fillpcmname(work->pcmname[0], pmd->ppcfile);
  strcpy(work->pcmtype[0], "PPC");
  fmdriver_fillpcmname(work->pcmname[1], pmd->ppzfile);
  strcpy(work->pcmtype[1], "PPZ1");
  fmdriver_fillpcmname(work->pcmname[2], pmd->ppzfile2);
  strcpy(work->pcmtype[2], "PPZ2");
  fmdriver_fillpcmname(work->pcmname[3], pmd->ppsfile);
  strcpy(work->pcmtype[3], "PPS");
  work->playing = true;
  // PPS currently unsupported
  work->pcmerror[3] = true;
}

enum {
  PPC_HEADER_SIZE = 30+2+4*256
};

bool pmd_ppc_load(
  struct fmdriver_work *work,
  uint8_t *data, size_t datalen
) {
  struct driver_pmd *pmd = (struct driver_pmd *)work->driver;
  if (datalen < PPC_HEADER_SIZE) return false;
  const char *header = "ADPCM DATA for  PMD ver.4.4-  ";
  for (int i = 0; i < 30; i++) {
    if (data[i] != (uint8_t)header[i]) return false;
  }
  for (int i = 0; i < 256; i++) {
    pmd->adpcm_addr[i][0] = read16le(&data[32+4*i+0]);
    pmd->adpcm_addr[i][1] = read16le(&data[32+4*i+2]);
  }
  
  work->opna_writereg(work, 0x100, 0x01);
  work->opna_writereg(work, 0x110, 0x13);
  work->opna_writereg(work, 0x110, 0x80);
  work->opna_writereg(work, 0x100, 0x60);
  work->opna_writereg(work, 0x101, 0x02);
  work->opna_writereg(work, 0x102, 0x00);
  work->opna_writereg(work, 0x103, 0x00);
  work->opna_writereg(work, 0x104, 0xff);
  work->opna_writereg(work, 0x105, 0xff);
  work->opna_writereg(work, 0x10c, 0xff);
  work->opna_writereg(work, 0x10d, 0xff);

  for (int i = 0; i < 0x4c0; i++) {
    work->opna_writereg(work, 0x108, 0);
  }
  
  for (size_t i = PPC_HEADER_SIZE; i < datalen; i++) {
    work->opna_writereg(work, 0x108, data[i]);
  }

  work->opna_writereg(work, 0x110, 0x0c);
  work->opna_writereg(work, 0x100, 0x01);
  return true;
}
