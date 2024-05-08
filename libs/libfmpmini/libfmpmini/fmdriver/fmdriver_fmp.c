#include "fmdriver_fmp.h"
#include "fmdriver_common.h"
#include <string.h>

static uint8_t fmp_rand71(struct driver_fmp *fmp) {
  // on real PC-98, read from I/O port 0x71 (8253 Timer)
  uint8_t val = fmp->rand71;
  fmp->rand71 = (val>>1) | ((((val>>7)^(val>>5)^(val>>4)^(val>>3)^1)&1)<<7);
  return fmp->rand71;
}

static uint8_t fmp_part_cmdload(struct driver_fmp *fmp, struct fmp_part *part) {
  if (part->current_ptr >= fmp->datalen) {
    //exit(2);
    part->current_ptr = 0xffff;
    return 0x74;
  }
  return fmp->data[part->current_ptr++];
}
static uint8_t fmp_part_cmdload_rhythm(struct driver_fmp *fmp, struct fmp_part *part) {
  if (part->current_ptr >= fmp->datalen) {
    part->current_ptr = 0xffff;
    return 0x93;
  }
  return fmp->data[part->current_ptr++];
}

static uint16_t fmp_part_cmdload16(struct driver_fmp *fmp, struct fmp_part *part) {
  uint16_t val = fmp_part_cmdload(fmp, part);
  val |= fmp_part_cmdload(fmp, part) << 8;
  return val;
}

enum {
  OPNA_DTMUL = 0x30,
  OPNA_TL = 0x40,
  OPNA_KSAR = 0x50,
  OPNA_AMDR = 0x60,
  OPNA_SR = 0x70,
  OPNA_SLRR = 0x80,
  OPNA_SSGEG = 0x90,
  OPNA_FNUM1 = 0xa0,
  OPNA_BLKFNUM2 = 0xa4,
  OPNA_FBALG = 0xb0,
  OPNA_LRAMSPMS = 0xb4,
};

static void fmp_part_fm_reg_write(struct fmdriver_work *work,
                                  struct fmp_part *part,
                                  uint8_t addr, uint8_t data) {
  uint16_t outaddr = part->opna_keyon_out & 0x3;
  if (part->opna_keyon_out & 0x4) outaddr += 0x100;
  outaddr += addr;
  work->opna_writereg(work, outaddr, data);
}

// 30ec
static uint16_t fmp_fm_freq(uint8_t note) {
  // 3106
  static const uint16_t freqtab[0xc] = {
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
  };
  return freqtab[note%0xc] + ((note/0xc)<<(3+8));
}

static uint8_t fmp_ssg_octave(uint8_t note) {
  return note/0xc;
}

static uint16_t fmp_ppz_freq(uint8_t note) {
  static const uint16_t freqtab[0xc] = {
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
  };
  return freqtab[note%0xc];
}

// 311e
static uint16_t fmp_part_ssg_freq(struct fmp_part *part, uint8_t note) {
  // 315a
  static const uint16_t freqtab[0xc] = {
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
  };
  uint16_t freq = part->u.ssg.env_f.ppz ? fmp_ppz_freq(note) : freqtab[note%0xc];
  return part->detune + freq;
}

// 3172
static uint16_t fmp_adpcm_freq(uint8_t note) {
  static const uint16_t freqtab[4][0xc] = {
    {
      0xdb22,
      0xdd53,
      0xdfa6,
      0xe21c,
      0xe4b7,
      0xe777,
      0xea65,
      0xed7f,
      0xf0c8,
      0xf443,
      0xf7f4,
      0xfbdc
    },
    {
      0x0000,
      0x0463,
      0x0909,
      0x0df6,
      0x132d,
      0x1864,
      0x1e8a,
      0x24bd,
      0x2b4e,
      0x3244,
      0x39a3,
      0x4173
    },
    {
      0x49ba,
      0x527e,
      0x5bc8,
      0x65a0,
      0x700d,
      0x7b19,
      0x86cc,
      0x9336,
      0xa057,
      0xae42,
      0xbd01,
      0xcca2
    },
    {
      0xc8b4,
      0xc9cc,
      0xcaf5,
      0xcc30,
      0xcd7e,
      0xcedf,
      0xd056,
      0xd1e3,
      0xd387,
      0xd544,
      0xd71c,
      0xd911
    }
  };
  uint16_t octave = note / 0xc;
  octave = (octave - 3) & 3;
  return freqtab[octave][note%0xc];
}

static uint8_t fmp_pdzf_vol_clamp(uint8_t v, uint8_t ev) {
  int16_t ret = v + u8s8(ev);
  if (ret < 0) ret = 0;
  if (ret > 0xf) ret = 0xf;
  return ret;
}

static void fmp_part_pdzf_vol_update(struct fmdriver_work *work,
                                     struct fmp_part *part) {
  part->pdzf.vol = part->type.fm ? (0x7f-part->current_vol)&0xf : part->current_vol - 1;
  if (part->pdzf.mode) {
    uint8_t envvol = part->lfo_f.q ? part->pdzf.env_state.vol : 0;
    if (work->ppz8_functbl) {
      work->ppz8_functbl->channel_volume(
        work->ppz8,
        part->pdzf.ppz8_channel,
        fmp_pdzf_vol_clamp(part->pdzf.vol, envvol)
      );
    }
  }
}

// 29e7
static void fmp_part_fm_vol(struct fmdriver_work *work,
                            struct driver_fmp *fmp,
                            struct fmp_part *part) {
  fmp_part_pdzf_vol_update(work, part);
  uint8_t pvol = part->actual_vol + fmp->fm_vol;
  if (pvol & 0x80) {
    if (fmp->fm_vol & 0x80) {
      pvol = 0;
    } else {
      pvol = 0x7f;
    }
  }
  for (int s = 0; s < 4; s++) {
    // 2a16
    if (!(part->slot_vol_mask & (1<<s))) continue;
    uint8_t svol = pvol - part->u.fm.slot_rel_vol[s];
    if (svol & 0x80) {
      if (part->u.fm.slot_rel_vol[s] & 0x80) {
        svol = 0x7f;
      } else {
        svol = 0;
      }
    }
    // 2a31

    if (fmp->data_version >= 6) {
      svol += part->u.fm.tone_tl[s];
      if (svol & 0x80) svol = 0x7f;
    }
    
    if (!(part->u.fm.slot_mask & (1<<s))) {
      fmp_part_fm_reg_write(work, part, OPNA_TL+4*s, svol);
    }
  }
}

// 3273
static void fmp_set_timer_ch3(struct fmdriver_work *work,
                              struct driver_fmp *fmp) {
  work->opna_writereg(work, 0x27, fmp->timer_ch3 | 0x2a);
}

// 326a
static void fmp_set_tempo(struct fmdriver_work *work, struct driver_fmp *fmp) {
  work->opna_writereg(work, 0x26, fmp->timerb);
  fmp_set_timer_ch3(work, fmp);
}

// 2979
static void fmp_part_keyoff_fm(struct fmdriver_work *work,
                               struct driver_fmp *fmp,
                               struct fmp_part *part) {
  // 2979
  uint8_t out = part->opna_keyon_out;
  if (part->type.fm_3) {
    fmp->fm3_slot_keyon &= (part->u.fm.slot_mask | 0x0f);
    out |= fmp->fm3_slot_keyon;
  }
  work->opna_writereg(work, 0x28, out);
}

// 2961
static void fmp_part_keyoff(struct fmdriver_work *work, struct driver_fmp *fmp,
                            struct fmp_part *part) {
  part->ext_keyon = 0;
  part->status.tie_cont = false;
  if (part->type.adpcm) return;
  if (!part->type.ssg) {
    fmp_part_keyoff_fm(work, fmp, part);
  } else {
    // 29a9
    part->u.ssg.curr_vol = 0xff;
    if (!part->u.ssg.env.release_rate) part->actual_vol = 0;
    part->u.ssg.env_f.portamento = true;
    part->u.ssg.env_f.attack = false;
  }
  if (part->pdzf.mode) {
    part->pdzf.keyon = false;
    if (part->lfo_f.q && part->pdzf.env_param.rr) {
      part->pdzf.env_state.status = PDZF_ENV_REL;
      part->pdzf.env_state.cnt = part->pdzf.env_param.rr;
    } else {
      part->pdzf.env_state.status = PDZF_ENV_OFF;
      if (work->ppz8_functbl) {
        work->ppz8_functbl->channel_stop(
          work->ppz8,
          part->pdzf.ppz8_channel
        );
      }
    }
  }
}

// 1fc8
static bool fmp_cmd62_tempo(struct fmdriver_work *work,
                            struct driver_fmp *fmp,
                            struct fmp_part *part) {
  uint8_t tempo = fmp_part_cmdload(fmp, part);
  fmp->timerb_bak = tempo;
  fmp->timerb = tempo;
  work->timerb = fmp->timerb;
  fmp_set_tempo(work, fmp);
  return true;
}

// 2067
static bool fmp_cmd63_vol_d_fm(struct fmdriver_work *work,
                               struct driver_fmp *fmp,
                               struct fmp_part *part) {
  uint8_t vol = fmp_part_cmdload(fmp, part);
  part->current_vol = vol;
  part->actual_vol = vol;
  fmp_part_fm_vol(work, fmp, part);
  return true;
}

static void fmp_ssg_ppz8_pdzf_mode_update(struct fmdriver_work *work,
                                          struct driver_fmp *fmp,
                                          struct fmp_part *part) {
  uint8_t prev_mode = part->pdzf.mode;
  uint8_t mask = 9 << part->opna_keyon_out;
  if ((fmp->ssg_mix & mask) == mask) {
    part->u.ssg.env_f.ppz = true;
    part->pdzf.mode = 0;
  } else {
    part->u.ssg.env_f.ppz = false;
    if (fmp->pdzf.mode == 2) {
      part->pdzf.mode =
        (part->u.ssg.envbak.startvol || part->u.ssg.envbak.attack_rate) 
        ? 0 : 2;
    }
  }
  if (prev_mode && !part->pdzf.mode) {
    part->pdzf.keyon = false;
    if (work->ppz8_functbl) {
      work->ppz8_functbl->channel_stop(
        work->ppz8,
        part->pdzf.ppz8_channel
      );
    }
  }
}

// 23a2
static bool fmp_cmd63_mix_ssg(struct fmdriver_work *work,
                              struct driver_fmp *fmp,
                              struct fmp_part *part) {
  (void)work;
  uint8_t val = fmp_part_cmdload(fmp, part);
  if (val & 0x80) {
    // 23a6
    fmp->ssg_mix &= val;
    //fmp->ssg_mix_se &= val;
    val = fmp_part_cmdload(fmp, part);
    fmp->ssg_mix |= val;
    //fmp->ssg_mix_se |= val;
    
    fmp_ssg_ppz8_pdzf_mode_update(work, fmp, part);
    // 23e4
    work->opna_writereg(work, 0x07, fmp->ssg_mix);
    // 23ec
  } else {
    // 23f7
    fmp_part_cmdload(fmp, part);
    fmp_part_cmdload16(fmp, part);
    // TODO: PPZ
  }
  return true;
}

// 2574
static bool fmp_cmd63_vol_d_adpcm(struct fmdriver_work *work,
                                  struct driver_fmp *fmp,
                                  struct fmp_part *part) {
  uint8_t vol = fmp_part_cmdload(fmp, part);
  part->current_vol = vol;
  part->actual_vol = vol;
  work->opna_writereg(work, 0x10b, vol);
  return true;
}

// 1fd9
static bool fmp_cmd64_loop(struct fmdriver_work *work,
                           struct driver_fmp *fmp,
                           struct fmp_part *part) {
  (void)work;
  uint8_t loop = fmp_part_cmdload(fmp, part);
  if (--loop) {
    ((uint8_t *)fmp->data)[part->current_ptr-1] = loop;
    uint16_t ptr_diff = fmp_part_cmdload16(fmp, part);
    part->current_ptr -= ptr_diff;
  } else {
    part->current_ptr += 3;
    if (part->current_ptr < fmp->datalen) {
      ((uint8_t *)fmp->data)[part->current_ptr-4] = fmp->data[part->current_ptr-1];
    }
  }
  return true;
}

// 1fee
static bool fmp_cmd65_loopend(struct fmdriver_work *work,
                              struct driver_fmp *fmp,
                              struct fmp_part *part) {
  (void)work;
  uint16_t ptr_diff = fmp_part_cmdload16(fmp, part);
  if ((part->current_ptr+ptr_diff-4) >= fmp->datalen) {
    part->current_ptr = 0xffff;
  } else if (fmp->data[part->current_ptr+ptr_diff-4] == 1) {
    part->current_ptr += ptr_diff;
    ((uint8_t *)fmp->data)[part->current_ptr-4] = fmp->data[part->current_ptr-1];
  }
  return true;
}

// 1fff
static bool fmp_cmd66_tie(struct fmdriver_work *work,
                          struct driver_fmp *fmp,
                          struct fmp_part *part) {
  (void)work;
  (void)fmp;
  part->status.tie = true;
  part->status.slur = false;
  return true;
}

// 200f
static bool fmp_cmd67_q(struct fmdriver_work *work,
                        struct driver_fmp *fmp,
                        struct fmp_part *part) {
  (void)work;
  //TODO
  if (fmp->datainfo.flags.q) {
    part->gate_cnt = fmp_part_cmdload(fmp, part);
  } else {
    part->gate_cmp = fmp_part_cmdload(fmp, part);
  }
  return true;
}


static uint32_t fmp_ppz8_note_freq(uint8_t note) {
  uint32_t freq = fmp_ppz_freq(note);
  uint8_t octave = note/0xc;
  if (octave > 4) {
    freq <<= (octave - 4);
  } else {
    freq >>= (4 - octave);
  }
  return freq;
}

// 201e
static bool fmp_cmd68_pitchbend(struct fmdriver_work *work,
                                struct driver_fmp *fmp,
                                struct fmp_part *part) {
  (void)work;
  uint8_t note = fmp_part_cmdload(fmp, part);
  note += part->note_diff;
  if (note > 0x60) note = 0;
  part->pit.target_note = note;
  uint16_t freq;
  if (!part->type.ssg) {
    freq = fmp_fm_freq(note);
  } else {
    freq = fmp_part_ssg_freq(part, note) >> fmp_ssg_octave(note);
  }
  part->pit.target_freq = freq;
  part->pit.delay = fmp_part_cmdload(fmp, part);
  part->pit.speed = fmp_part_cmdload(fmp, part);
  part->pit.speed_cnt = part->pit.speed;

  part->pdzf.pit.target_note = note;
  part->pdzf.pit.target_freq = fmp_ppz8_note_freq(note);
  part->pdzf.pit.delay = part->pit.delay;
  part->pdzf.pit.speed = part->pit.speed;
  part->pdzf.pit.speed_cnt = part->pit.speed_cnt;
  uint8_t rate = fmp_part_cmdload(fmp, part);
  part->pdzf.pit.rate = rate;
  if (!part->type.fm) rate = -rate;
  part->pit.rate = rate;
  part->status.pitchbend = true;
  part->pdzf.pit.on = true;
  part->pdzf.pit.pitchdiff = 0;
  return true;
}

// 25e3
static bool fmp_cmd68_adpcm(struct fmdriver_work *work,
                            struct driver_fmp *fmp,
                            struct fmp_part *part) {
  (void)work;
  fmp_part_cmdload(fmp, part);
  return true;
}

// 205d
static bool fmp_cmd69_vol_fm(struct fmdriver_work *work,
                             struct driver_fmp *fmp,
                             struct fmp_part *part) {
  static const uint8_t voltbl[16] = {
    64, 59, 56, 52, 48, 42, 40, 37,
    34, 32, 29, 26, 24, 21, 18, 16,
  };
  uint8_t index = fmp_part_cmdload(fmp, part);
  uint8_t vol = (index < 16) ? voltbl[index] : 0x7f;
  part->current_vol = vol;
  part->actual_vol = vol;
  fmp_part_fm_vol(work, fmp, part);
  return true;
}

// 2362
static void fmp_part_ppz8_vol(struct fmdriver_work *work,
                              struct fmp_part *part) {
  uint8_t vol = part->current_vol - 1;
  if (vol > 0xf) vol = 0xf;
  if (work->ppz8_functbl) {
    work->ppz8_functbl->channel_volume(work->ppz8,
                                    part->opna_keyon_out,
                                    vol);
  }
}

// 234d
static void fmp_part_ssg_vol(struct fmdriver_work *work,
                             struct fmp_part *part) {
  fmp_part_pdzf_vol_update(work, part);
  if (!part->u.ssg.env_f.ppz) {
    // 2353
    uint8_t vol = part->current_vol;
    part->u.ssg.vol = ((unsigned)(vol-1) << 8)/vol;
  } else {
    fmp_part_ppz8_vol(work, part);
  }
}

// 2348
static bool fmp_cmd69_vol_ssg(struct fmdriver_work *work,
                              struct driver_fmp *fmp,
                              struct fmp_part *part) {
  (void)work;
  part->current_vol = fmp_part_cmdload(fmp, part) + 1;
  // added to prevent zero division
  if (!part->current_vol) part->current_vol = 1;
  fmp_part_ssg_vol(work, part);
  return true;
}

static bool fmp_cmd69_adpcm(struct fmdriver_work *work,
                            struct driver_fmp *fmp,
                            struct fmp_part *part) {
  // 205d: invalid command for adpcm part
  (void)work;
  fmp_part_cmdload(fmp, part);
  return true;
}

static void fmp_part_fm_relvol(struct fmdriver_work *work,
                               struct driver_fmp *fmp,
                               struct fmp_part *part,
                               uint8_t vol) {
  uint8_t newvol = part->current_vol - vol;
  if (newvol & 0x80) {
    newvol = (vol & 0x80) ? 0x7f : 0;
  }
  part->current_vol = newvol;
  part->actual_vol = newvol;
  fmp_part_fm_vol(work, fmp, part);
}

// 2080
static bool fmp_cmd6a_voldec_fm(struct fmdriver_work *work,
                                struct driver_fmp *fmp,
                                struct fmp_part *part) {
  fmp_part_fm_relvol(work, fmp, part, -3);
  return true;
}

// 2086
static bool fmp_cmd6b_volinc_fm(struct fmdriver_work *work,
                                struct driver_fmp *fmp,
                                struct fmp_part *part) {
  fmp_part_fm_relvol(work, fmp, part, 3);
  return true;
}

// 238a
static bool fmp_cmd6a_voldec_ssg(struct fmdriver_work *work,
                                 struct driver_fmp *fmp,
                                 struct fmp_part *part) {
  (void)fmp;
  (void)work;
  if (part->current_vol > 1) part->current_vol--;
  if (!part->current_vol) part->current_vol = 1;
  fmp_part_ssg_vol(work, part);
  return true;
}
// 2396
static bool fmp_cmd6b_volinc_ssg(struct fmdriver_work *work,
                                 struct driver_fmp *fmp,
                                 struct fmp_part *part) {
  (void)fmp;
  (void)work;
  if (part->current_vol < 0x10) part->current_vol++;
  fmp_part_ssg_vol(work, part);
  return true;
}

// 25f5
static void fmp_part_adpcm_relvol(struct fmdriver_work *work,
                                  struct fmp_part *part,
                                  uint8_t vol) {
  uint16_t relvol = vol;
  if (vol & 0x80) relvol |= 0xff00;
  uint16_t newvol = part->current_vol + relvol;
  if (newvol & 0x100) {
    newvol = (vol & 0x80) ? 0 : 0xff;
  }
  part->current_vol = newvol;
  part->actual_vol = newvol;
  work->opna_writereg(work, 0x10b, newvol);
}

// 259e
static bool fmp_cmd6a_voldec_adpcm(struct fmdriver_work *work,
                                   struct driver_fmp *fmp,
                                   struct fmp_part *part) {
  (void)fmp;
  fmp_part_adpcm_relvol(work, part, -3);
  return true;
}

// 259e
static bool fmp_cmd6b_volinc_adpcm(struct fmdriver_work *work,
                                   struct driver_fmp *fmp,
                                   struct fmp_part *part) {
  (void)fmp;
  fmp_part_adpcm_relvol(work, part, 3);
  return true;
}

// 208c
static bool fmp_cmd6c_kondelay(struct fmdriver_work *work,
                               struct driver_fmp *fmp,
                               struct fmp_part *part) {
  (void)work;
  part->keyon_delay = fmp_part_cmdload(fmp, part);
  return true;
}

// 2090
static bool fmp_cmd6d_detune(struct fmdriver_work *work,
                             struct driver_fmp *fmp,
                             struct fmp_part *part) {
  (void)work;
  uint16_t detune = fmp_part_cmdload(fmp, part);
  if (detune & 0x80) detune |= 0xff00;
  part->detune = detune;
  return true;
}

// 2095
static bool fmp_cmd6e_poke(struct fmdriver_work *work,
                           struct driver_fmp *fmp,
                           struct fmp_part *part) {
  (void)work;
  uint16_t port = fmp_part_cmdload(fmp, part);
  uint8_t data = fmp_part_cmdload(fmp, part);
  // 3222?
  if (part->opna_keyon_out & 0x04) port |= 0x100;
  work->opna_writereg(work, port, data);
  return true;
}

// 20b7
static void fmp_part_sync(struct fmp_part *part) {
  if (part->sync != 1) part->sync = 0;
}

// 209e
static bool fmp_cmd6f_sync(struct fmdriver_work *work,
                           struct driver_fmp *fmp,
                           struct fmp_part *part) {
  (void)work;
  // sync
  // command S{channels}
  uint8_t val = fmp_part_cmdload(fmp, part);
  if (!(val & 0x80)) {
    for (int i = 0; i < 3; i++) {
      if (val & (1<<i)) fmp_part_sync(&fmp->parts[FMP_PART_FM_1+i]);
    }
    for (int i = 0; i < 3; i++) {
      if (val & (1<<(i+3))) fmp_part_sync(&fmp->parts[FMP_PART_SSG_1+i]);
    }
  } else {
    for (int i = 0; i < 3; i++) {
      if (val & (1<<i)) fmp_part_sync(&fmp->parts[FMP_PART_FM_4+i]);
    }
    if (val & (1<<3)) fmp_part_sync(&fmp->parts[FMP_PART_ADPCM]);
    if (val & (1<<4)) {
      if (fmp->rhythm.sync != 1) fmp->rhythm.sync = 0;
    }
  }
  return true;
}

// 20eb
static bool fmp_cmd70_wait(struct fmdriver_work *work,
                           struct driver_fmp *fmp,
                           struct fmp_part *part) {
  (void)work;
  (void)fmp;
  // command W
  part->sync = 2;
  return false;
}

// 20f6
static bool fmp_cmd71_tone_fm(struct fmdriver_work *work,
                              struct driver_fmp *fmp,
                              struct fmp_part *part) {
  uint8_t tone = fmp_part_cmdload(fmp, part);
  part->tone = tone;
  for (int i = 0; i < 4; i++) {
    if (part->u.fm.slot_mask & (1<<i)) continue;
    fmp_part_fm_reg_write(work, part, OPNA_SLRR+4*i, 0x0f);
  }
  for (int i = 0; i < 4; i++) {
    part->u.fm.slot_rel_vol[i] = 0;
  }
  // 211b
  uint16_t fmtoneptr = fmp->datainfo.fmtoneptr + (tone*25);
  for (int i = 0; i < 4; i++) {
    part->u.fm.tone_tl[i] = fmp->data[fmtoneptr+4+i];
  }
  for (int p = 0; p < 6; p++) {
    for (int s = 0; s < 4; s++) {
      if (part->u.fm.slot_mask & (1<<s)) continue;
      fmp_part_fm_reg_write(work, part,
                            OPNA_DTMUL+(p*0x10)+4*s,
                            fmp->data[fmtoneptr+(p*4)+s]);
    }
  }
  uint8_t fbalg = fmp->data[fmtoneptr+0x18];
  // 2160
  if (part->type.fm_3) {
    if (part->u.fm.slot_mask & 1) {
      fbalg = (fbalg & 0x38) | (fmp->fm3_alg & 0x07);
    }
  }
  fmp_part_fm_reg_write(work, part, OPNA_FBALG, fbalg);
  if (part->type.fm_3) {
    fmp->fm3_alg = fbalg & 0x07;
  }
  uint8_t alg = fbalg & 0x7;
  static const uint8_t alg_vol_tbl[8] = {
    0x8, 0x8, 0x8, 0x8, 0xc, 0xe, 0xe, 0xf,
  };
  part->slot_vol_mask = alg_vol_tbl[alg];
  fmp_part_fm_vol(work, fmp, part);
  return true;
}

// 2458
static void fmp_envreset_ssg(struct fmp_part *part) {
  part->u.ssg.env.startvol = part->u.ssg.envbak.startvol;
  part->u.ssg.env.attack_rate = part->u.ssg.envbak.attack_rate;
  part->u.ssg.env.decay_rate = part->u.ssg.envbak.decay_rate;
  part->u.ssg.env.sustain_lv = part->u.ssg.envbak.sustain_lv;
  part->u.ssg.env.sustain_rate = part->u.ssg.envbak.sustain_rate;
  part->u.ssg.env.release_rate = part->u.ssg.envbak.release_rate;
}

// 2458
static bool fmp_cmd71_envreset_ssg(struct fmdriver_work *work,
                                    struct driver_fmp *fmp,
                                    struct fmp_part *part) {
  (void)work;
  fmp_part_cmdload(fmp, part);
  fmp_envreset_ssg(part);
  return true;
}

// 25a8
static bool fmp_cmd71_tone_adpcm(struct fmdriver_work *work,
                                 struct driver_fmp *fmp,
                                 struct fmp_part *part) {
  (void)work;
  uint8_t tone = fmp_part_cmdload(fmp, part);
  part->tone = tone;
  if (!(tone & 0x80)) {
    // 25b4
    uint16_t adpcmptr = fmp->datainfo.adpcmptr+6*tone;
    if (adpcmptr+6 <= fmp->datalen) {
      part->u.adpcm.startaddr = read16le(&fmp->data[adpcmptr+0]);
      part->u.adpcm.endaddr = read16le(&fmp->data[adpcmptr+2]);
      part->u.adpcm.deltat = read16le(&fmp->data[adpcmptr+4]);
    }
  } else {
    // 25cc
    tone &= 0x7f;
    part->u.adpcm.startaddr = fmp->adpcm_startaddr[tone];
    part->u.adpcm.endaddr = fmp->adpcm_endaddr[tone];
    part->u.adpcm.deltat = fmp->adpcm_deltat;
  }
  return true;
}

// 20f2
static bool fmp_cmd72_deflen(struct fmdriver_work *work,
                             struct driver_fmp *fmp,
                             struct fmp_part *part) {
  (void)work;
  part->default_len = fmp_part_cmdload(fmp, part);
  return true;
}

// 1fac
static bool fmp_cmd73_relvol_fm(struct fmdriver_work *work,
                                struct driver_fmp *fmp,
                                struct fmp_part *part) {
  fmp_part_fm_relvol(work, fmp, part, fmp_part_cmdload(fmp, part));
  return true;
}

// 2424
static bool fmp_cmd73_noise_ssg(struct fmdriver_work *work,
                                struct driver_fmp *fmp,
                                struct fmp_part *part) {
  // TODO: ignoring se flag
  uint8_t val = fmp_part_cmdload(fmp, part);
  part->u.ssg.env_f.noise = true;
  if (val & 0x80) {
    part->u.ssg.env_f.noise = false;
  } else {
    fmp->ssg_noise_freq = val;
    work->opna_writereg(work, 0x06, val);
  }
  return true;
}

// 25f5
static bool fmp_cmd73_relvol_adpcm(struct fmdriver_work *work,
                                   struct driver_fmp *fmp,
                                   struct fmp_part *part) {
  fmp_part_adpcm_relvol(work, part, fmp_part_cmdload(fmp, part));
  return true;
}

/*
// ????
// 1b64
static void fmp() {
  
}
*/

// 2466
static bool fmp_cmd74_loop(struct fmdriver_work *work,
                           struct driver_fmp *fmp,
                           struct fmp_part *part) {
  (void)work;
  // TODO: se flag
  fmp->part_loop_bit &= ~part->part_bit;
  if (!fmp->part_loop_bit) {
    if (!--fmp->loop_dec) {
      fmp->status.looped = true;
      // al=2; 1b64();
    }
    // 248c
    fmp->loop_cnt++;
    work->loop_cnt = fmp->loop_cnt;
    work->timerb_cnt_loop = 0;
    fmp->part_loop_bit = fmp->part_playing_bit;
    // al=2; 1b64();
  }
  // 24a2
  uint16_t ptr = part->loop_ptr;
  if (ptr != 0xffff) {
    part->current_ptr = ptr;
    return true;
  } else {
    fmp->part_loop_bit &= ~part->part_bit;
    fmp->part_playing_bit &= ~part->part_bit;
    if (!fmp->part_playing_bit) {
      // 24bc
      // 3e16();
      fmp->status.stopped = true;
      fmp->status.looped = true;
      work->loop_cnt = -1;
    }
    // 24f0
    if (!part->type.rhythm) {
      part->prev_note = 0x61; // rest
      part->pdzf.prev_note = 0x61;
      part->status.off = true;
      if (part->type.ssg) {
        part->actual_vol = 0;
        // 0x6a?
      }
      fmp_part_keyoff(work, fmp, part);
    } else {
      // 2512
      fmp->rhythm.status = 1;
    }
    return false;
  }
}

// 22e0
static bool fmp_cmd75_lfo(struct fmdriver_work *work,
                          struct driver_fmp *fmp,
                          struct fmp_part *part) {
  (void)work;
  uint8_t val = fmp_part_cmdload(fmp, part);
  if (val & 0x80) {
    // LFO on/off
    // MD/S command
    // 2313
    bool set = val & 0x40;
    if (val & (1<<0)) part->lfo_f.e = set;
    if (val & (1<<1)) part->lfo_f.w = set;
    if (val & (1<<2)) part->lfo_f.a = set;
    if (val & (1<<3)) part->lfo_f.r = set;
    if (val & (1<<4)) part->lfo_f.q = set;
    if (val & (1<<5)) part->lfo_f.p = set;
    if (!set) {
      part->pdzf.lfodiff = 0;
      if (!part->type.fm) {
        part->actual_freq = fmp_part_ssg_freq(part, part->prev_note);
        part->prev_freq = 0;
      } else {
        part->actual_freq = fmp_fm_freq(part->prev_note);
        part->prev_freq = 0;
      }
    } else {
      if (part->lfo_f.e) part->lfo_f.a = false;
    }
  } else {
    // 22e4
    // EON/EOFF (FM3)
    if (val & 0x40) {
      // 22f1
      fmp->timer_ch3 |= 0x40;
      if (val & 0x20) {
        fmp->ch3_se_freqdiff[0] = u8s8(fmp_part_cmdload(fmp, part));
        fmp->ch3_se_freqdiff[1] = u8s8(fmp_part_cmdload(fmp, part));
        fmp->ch3_se_freqdiff[2] = u8s8(fmp_part_cmdload(fmp, part));
        fmp->ch3_se_freqdiff[3] = u8s8(fmp_part_cmdload(fmp, part));
      }
    } else {
      // 22e9
      fmp->timer_ch3 &= 0x3f;
    }
    fmp_set_timer_ch3(work, fmp);
  }
  return true;
}

// 2fa2
static void fmp_init_lfo(struct fmp_lfo *lfo) {
  lfo->delay_cnt = lfo->delay;
  lfo->speed_cnt = 1;
  lfo->depth_cnt = lfo->depth >> 1;
  int16_t rate2 = u16s16(lfo->rate);
  if (lfo->waveform != FMP_LFOWF_TRIANGLE) {
    rate2 *= lfo->depth;
    // 2fc4
    if (lfo->waveform == FMP_LFOWF_STAIRCASE) {
      rate2 >>= 1;
      lfo->depth_cnt = 1;
    }
  }
  // 2fd0
  if (lfo->waveform == FMP_LFOWF_RANDOM) {
    rate2 = 0;
  }
  // 2fd8
  lfo->rate2 = rate2;
}

// 2f84
static void fmp_part_init_lfo_pqr(struct fmp_part *part) {
  if (part->lfo_f.p) fmp_init_lfo(&part->lfo[FMP_LFO_P]);
  if (part->lfo_f.q) fmp_init_lfo(&part->lfo[FMP_LFO_Q]);
  if (part->lfo_f.r) fmp_init_lfo(&part->lfo[FMP_LFO_R]);
}

// 21a6
static void fmp_cmd_lfo(struct driver_fmp *fmp,
                        struct fmp_part *part,
                        struct fmp_lfo *lfo) {
  lfo->delay = fmp_part_cmdload(fmp, part);
  lfo->speed = fmp_part_cmdload(fmp, part);
  uint16_t rate = fmp_part_cmdload(fmp, part);
  if (rate & 0x80) rate |= 0xff00;
  lfo->rate = rate;
  lfo->rate2 = rate;
  lfo->depth = fmp_part_cmdload(fmp, part);
  lfo->waveform = fmp_part_cmdload(fmp, part);
  if (lfo->waveform == 6) {
    lfo->rate = (lfo->rate&0xff)*lfo->depth;
  }
  part->lfo_f.lfo = true;
  fmp_part_init_lfo_pqr(part);
}

// 21a1
static bool fmp_cmd76_lfo_p(struct fmdriver_work *work,
                            struct driver_fmp *fmp,
                            struct fmp_part *part) {
  (void)work;
  part->lfo_f.p = true;
  fmp_cmd_lfo(fmp, part, &part->lfo[FMP_LFO_P]);
  return true;
}
// 21d0
static bool fmp_cmd77_lfo_q(struct fmdriver_work *work,
                            struct driver_fmp *fmp,
                            struct fmp_part *part) {
  (void)work;
  part->lfo_f.q = true;
  struct fmp_lfo *lfo = &part->lfo[FMP_LFO_Q];
  fmp_cmd_lfo(fmp, part, lfo);
  part->pdzf.env_param.rr = lfo->delay - 2;
  part->pdzf.env_param.sr = lfo->speed;
  part->pdzf.env_param.dd = u16s16(lfo->rate);
  part->pdzf.env_param.al = lfo->depth;
  return true;
}
// 21da
static bool fmp_cmd78_lfo_r(struct fmdriver_work *work,
                            struct driver_fmp *fmp,
                            struct fmp_part *part) {
  (void)work;
  part->lfo_f.r = true;
  struct fmp_lfo *lfo = &part->lfo[FMP_LFO_R];
  fmp_cmd_lfo(fmp, part, lfo);
  if (part->pdzf.mode != 2 || lfo->depth == 0) {
    part->pdzf.voice = lfo->speed;
    if (part->pdzf.mode == 2 && (lfo->delay != 2)) {
      uint8_t upan = lfo->delay - 2;
      if (upan < 10) {
        part->pdzf.pan = upan - 5;
      }
    } else {
      int8_t pan = u8s8(lfo->rate);
      if (pan < -4) pan = -4;
      if (pan > 4) pan = 4;
      part->pdzf.pan = pan;
    }
  }
  return true;
}

static void fmp_pdzf_loop_freq(struct fmdriver_work *work,
                               struct driver_fmp *fmp,
                               struct fmp_part *part,
                               uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3) {
  (void)fmp;
  if (!part->pdzf.mode) return;
  if (part->pdzf.mode != 2 || part->lfo[2].depth == 0) {
    uint32_t start = (d0 << 8) | d1;
    uint32_t end = (d2 << 8) | d3;
    if (start == 0xffff) start = (uint32_t)-1;
    if (end == 0xffff) end = (uint32_t)-1;
    if ((start != (uint32_t)-1) && (end != (uint32_t)-1)) {
      if (start >= end) {
        start = (uint32_t)-1;
        end = (uint32_t)-1;
      }
    }
    if (work->ppz8_functbl) {
      work->ppz8_functbl->channel_loopoffset(
        work->ppz8,
        part->pdzf.ppz8_channel,
        start, end
      );
    }
  }
  if (part->pdzf.mode == 2) {
    if (part->lfo[2].depth == 1 || part->lfo[2].depth == 2) {
      uint32_t addr = ((uint32_t)d0) << 24;
      addr |= ((uint32_t)d1) << 16;
      addr |= ((uint32_t)d2) << 8;
      addr |= d3;
      if (part->lfo[2].depth == 1) {
        part->pdzf.loopstart32 = addr;
      } else {
        part->pdzf.loopend32 = addr;
      }
      if (work->ppz8_functbl) {
        work->ppz8_functbl->channel_loopoffset(
          work->ppz8,
          part->pdzf.ppz8_channel,
          part->pdzf.loopstart32, part->pdzf.loopend32
        );
      }
    }
  }
}

static bool fmp_cmd79_lfo_a_fm(struct fmdriver_work *work,
                            struct driver_fmp *fmp,
                            struct fmp_part *part) {
  (void)work;
  part->lfo_f.a = true;
  part->lfo_f.e = false;
  uint8_t v = fmp_part_cmdload(fmp, part);
  part->u.fm.alfo.delay = v;
  part->u.fm.alfo.delay_cnt = v;
  v = fmp_part_cmdload(fmp, part);
  part->u.fm.alfo.speed = v;
  part->u.fm.alfo.speed_cnt = v;
  v = fmp_part_cmdload(fmp, part);
  part->u.fm.alfo.rate = v;
  part->u.fm.alfo.rate_orig = v;
  v = fmp_part_cmdload(fmp, part);
  part->u.fm.alfo.depth = v;
  part->u.fm.alfo.depth_cnt = v;
  fmp_pdzf_loop_freq(work, fmp, part,
                     part->u.fm.alfo.delay, part->u.fm.alfo.speed,
                     part->u.fm.alfo.rate, part->u.fm.alfo.depth
  );
  return true;
}

// 244f
static bool fmp_cmd79_env_ssg(struct fmdriver_work *work,
                          struct driver_fmp *fmp,
                          struct fmp_part *part) {
  (void)work;
  part->u.ssg.envbak.startvol = fmp_part_cmdload(fmp, part);
  part->u.ssg.envbak.attack_rate = fmp_part_cmdload(fmp, part);
  part->u.ssg.envbak.decay_rate = fmp_part_cmdload(fmp, part);
  part->u.ssg.envbak.sustain_lv = fmp_part_cmdload(fmp, part);
  part->u.ssg.envbak.sustain_rate = fmp_part_cmdload(fmp, part);
  part->u.ssg.envbak.release_rate = fmp_part_cmdload(fmp, part);
  fmp_envreset_ssg(part);
  fmp_ssg_ppz8_pdzf_mode_update(work, fmp, part);
  fmp_pdzf_loop_freq(work, fmp, part,
                     part->u.ssg.envbak.decay_rate,
                     part->u.ssg.envbak.sustain_lv,
                     part->u.ssg.envbak.sustain_rate,
                     part->u.ssg.envbak.release_rate
  );
  return true;
}

// 2009
static bool fmp_cmd7a_tie(struct fmdriver_work *work,
                          struct driver_fmp *fmp,
                          struct fmp_part *part) {
  (void)work;
  (void)fmp;
  part->status.tie = true;
  part->status.slur = true;
  return true;
}

// 1f9e
static bool fmp_cmd7b_transpose(struct fmdriver_work *work,
                                struct driver_fmp *fmp,
                                struct fmp_part *part) {
  (void)work;
  uint8_t val = fmp_part_cmdload(fmp, part);
  if (val) part->note_diff += val;
  else part->note_diff = 0;
  return true;
}

// 2229
static void fmp_part_write_pan(struct fmdriver_work *work,
                               struct fmp_part *part) {
  fmp_part_fm_reg_write(work, part, OPNA_LRAMSPMS, part->pan_ams_pms);
}

static void fmp_fm_pdzf_mode_update(struct fmdriver_work *work,
                                    struct driver_fmp *fmp,
                                    struct fmp_part *part) {
  uint8_t prev_mode = part->pdzf.mode;
  part->pdzf.mode = (part->u.fm.slot_mask == 0xff) ? fmp->pdzf.mode : 0;
  if (prev_mode && !part->pdzf.mode) {
    part->pdzf.keyon = false;
    if (work->ppz8_functbl) {
      work->ppz8_functbl->channel_stop(
        work->ppz8,
        part->pdzf.ppz8_channel
      );
    }
  }
}

// 2206
static bool fmp_cmd7c_lfo_pan_fm(struct fmdriver_work *work,
                               struct driver_fmp *fmp,
                               struct fmp_part *part) {
  uint8_t val1 = fmp_part_cmdload(fmp, part);
  if (val1 & 0x01) {
    uint8_t val2 = fmp_part_cmdload(fmp, part);
    uint8_t val3 = fmp_part_cmdload(fmp, part);
    if (val3 & 0x80) {
      if (val3 & 0x20) {
        // 229b
        // HLFO delay
        // 7c 01 val a0: DH
        part->u.fm.hlfo_delay = val2;
      } else {
        // 227c
        // HLFO on/off
        // 7c 01 00 c0: SH1/MDH1: HLFO ON
        // 7c 01 00 80: SH0/MDH0: HLFO OFF
        val3 &= 0x7f;
        val3 >>= 3;
        part->u.fm.hlfo_freq &= 0xf7;
        part->u.fm.hlfo_freq |= val3;
        work->opna_writereg(work, 0x22, part->u.fm.hlfo_freq);
        if (!val3) {
          part->pan_ams_pms &= 0xc0;
          fmp_part_write_pan(work, part);
        }
      }
    } else {
      // 226b
      // HLFO parameters
      // 7c 01 val2 val3 val4: MH
      part->u.fm.hlfo_delay = val2;
      part->u.fm.hlfo_freq = val3 | 0x08;
      part->u.fm.hlfo_apms = fmp_part_cmdload(fmp, part);
    }
  } else {
    // 220a
    if (val1 & 0x02) {
      // 229f
      // LFO W settings
      // 7c 02 xx xx xx xx xx: MW
      part->lfo_f.w = true;
      uint8_t val = fmp_part_cmdload(fmp, part);
      part->u.fm.wlfo.delay = val;
      part->u.fm.wlfo.delay_cnt = val;
      val = fmp_part_cmdload(fmp, part);
      part->u.fm.wlfo.speed = val;
      part->u.fm.wlfo.speed_cnt = val;
      val = fmp_part_cmdload(fmp, part);
      part->u.fm.wlfo.rate = val;
      part->u.fm.wlfo.rate_orig = val;
      part->u.fm.wlfo.rate_curr = 0;
      val = fmp_part_cmdload(fmp, part);
      part->u.fm.wlfo.depth = val;
      part->u.fm.wlfo.depth_cnt = val;
      val = fmp_part_cmdload(fmp, part);
      part->u.fm.wlfo.sync = val;
      
      int pdzf_i = (part - &fmp->parts[FMP_PART_FM_EX1]);
      if ((pdzf_i == 1) || (pdzf_i == 2)) {
        struct pdzf_rhythm *pr = &fmp->pdzf.rhythm[pdzf_i-1];
        pr->voice[0] = part->u.fm.wlfo.delay;
        pr->voice[1] = part->u.fm.wlfo.speed;
        int16_t panpot = u8s8(part->u.fm.wlfo.rate);
        if (panpot < -4) panpot = -4;
        if (panpot > 4) panpot = 4;
        pr->pan = panpot;
        pr->note = part->u.fm.wlfo.depth;
        pr->enabled = true;
      }
    } else {
      // 2211
      if (val1 & 0x04) {
        // 22c7
        // mask slot
        // 7c 04 val: EX
        uint8_t val = fmp_part_cmdload(fmp, part);
        fmp->timer_ch3 |= 0x40;
        if (val == 0xff) {
          fmp_part_keyoff(work, fmp, part);
          fmp->timer_ch3 &= 0x3f;
        }
        part->u.fm.slot_mask = val;
        fmp_fm_pdzf_mode_update(work, fmp, part);
      } else {
        // 2218
        // PAN
        // 7c val1: P
        part->pan_ams_pms &= 0x3f;
        part->pan_ams_pms |= val1;
        if (part->type.fm_3) {
          // 2236
          fmp->parts[FMP_PART_FM_3].pan_ams_pms &= 0x3f;
          fmp->parts[FMP_PART_FM_3].pan_ams_pms |= val1;
          fmp->parts[FMP_PART_FM_EX1].pan_ams_pms &= 0x3f;
          fmp->parts[FMP_PART_FM_EX1].pan_ams_pms |= val1;
          fmp->parts[FMP_PART_FM_EX2].pan_ams_pms &= 0x3f;
          fmp->parts[FMP_PART_FM_EX2].pan_ams_pms |= val1;
          fmp->parts[FMP_PART_FM_EX3].pan_ams_pms &= 0x3f;
          fmp->parts[FMP_PART_FM_EX3].pan_ams_pms |= val1;
        }
        fmp_part_write_pan(work, part);
      }
    }
  }
  return true;
}

// 1f78
static bool fmp_cmd7c_tone_ssg(struct fmdriver_work *work,
                               struct driver_fmp *fmp,
                               struct fmp_part *part) {
  (void)work;
  part->tone = fmp_part_cmdload(fmp, part);
  uint16_t toneptr = fmp->datainfo.ssgtoneptr + (part->tone*6);
  if ((fmp->datainfo.ssgtoneptr != 0xffff) && (fmp->datalen >= (toneptr+6))) {
    part->u.ssg.envbak.startvol = fmp->data[toneptr+0];
    part->u.ssg.envbak.attack_rate = fmp->data[toneptr+1];
    part->u.ssg.envbak.decay_rate = fmp->data[toneptr+2];
    part->u.ssg.envbak.sustain_lv = fmp->data[toneptr+3];
    part->u.ssg.envbak.sustain_rate = fmp->data[toneptr+4];
    part->u.ssg.envbak.release_rate = fmp->data[toneptr+5];
  }
  fmp_ssg_ppz8_pdzf_mode_update(work, fmp, part);
  fmp_pdzf_loop_freq(work, fmp, part,
                    part->u.ssg.envbak.decay_rate,
                    part->u.ssg.envbak.sustain_lv,
                    part->u.ssg.envbak.sustain_rate,
                    part->u.ssg.envbak.release_rate
  );
  fmp_envreset_ssg(part);
  return true;
}

// 25e4
static void fmp_adpcm_pan(struct fmdriver_work *work,
                          struct driver_fmp *fmp,
                          struct fmp_part *part,
                          uint8_t val) {
  val &= 0xc0;
  part->pan_ams_pms = val;
  work->opna_writereg(work, 0x101, val | fmp->adpcm_c1);
}

// 25e4
static bool fmp_cmd7c_pan_adpcm(struct fmdriver_work *work,
                                struct driver_fmp *fmp,
                                struct fmp_part *part) {
  fmp_adpcm_pan(work, fmp, part, fmp_part_cmdload(fmp, part));
  return true;
}

// 1f70
static bool fmp_cmd7d_sync(struct fmdriver_work *work,
                           struct driver_fmp *fmp,
                           struct fmp_part *part) {
  (void)work;
  // unknown MML
  fmp->sync.data = fmp_part_cmdload(fmp, part);
  fmp->sync.cnt++;
  return true;
}

// 1f46
static bool fmp_cmd7e_loop_det(struct fmdriver_work *work,
                               struct driver_fmp *fmp,
                               struct fmp_part *part) {
  (void)work;
  uint8_t val1 = fmp_part_cmdload(fmp, part);
  if (val1) {
    fmp->loop_dec = val1;
    fmp->loop_times = val1;
    // Fadeout not implemented
  } else {
    // detune relative
    // DX
    int16_t det = u8s8(fmp_part_cmdload(fmp, part));
    part->detune += det;
  }
  return true;
}
// 1f2c
static bool fmp_cmd7f_lfo_delay(struct fmdriver_work *work,
                                struct driver_fmp *fmp,
                                struct fmp_part *part) {
  (void)work;
  uint8_t val1 = fmp_part_cmdload(fmp, part);
  uint8_t delay = fmp_part_cmdload(fmp, part);
  if (val1 < 3) {
    part->lfo[val1].delay = delay;
  } else if (val1 == 3) {
    // 1f40
    part->u.fm.alfo.delay = delay;
  }
  return true;
}

// 1eaf
static void fmp_update_slot_rel_vol(struct fmdriver_work *work,
                                    struct driver_fmp *fmp,
                                    struct fmp_part *part) {
  uint8_t pvol = part->actual_vol+fmp->fm_vol;
  if (pvol & 0x80) {
    pvol = (fmp->fm_vol & 0x80) ? 0 : 0x7f;
  }
  // 1ed4
  for (int s = 0; s < 4; s++) {
    uint8_t svol;
    if (!(part->slot_vol_mask & (1<<s))) {
      // 1ee2
      svol = part->u.fm.tone_tl[s] - part->u.fm.slot_rel_vol[s];
      if (svol & 0x80) {
        svol = (part->u.fm.slot_rel_vol[s] & 0x80) ? 0x7f : 0;
      }
    } else {
      // 1f00
      svol = pvol - part->u.fm.slot_rel_vol[s];
      if (svol & 0x80) {
        svol = (part->u.fm.slot_rel_vol[s] & 0x80) ? 0x7f : 0;
      }
      svol += part->u.fm.tone_tl[s];
      if (svol & 0x80) svol = 0x7f;
    }
    if (!(part->u.fm.slot_mask & (1<<s))) {
      fmp_part_fm_reg_write(work, part, OPNA_TL+4*s, svol);
    }
    // 1f20
  }
}

// 1e68
static bool fmp_cmde2_slotrelvol_set_fm(struct fmdriver_work *work,
                                        struct driver_fmp *fmp,
                                        struct fmp_part *part) {
  uint8_t val = fmp_part_cmdload(fmp, part);
  uint8_t mask = fmp_part_cmdload(fmp, part);
  for (int i = 0; i < 4; i++) {
    if (mask & (1<<i)) {
      part->u.fm.slot_rel_vol[i] = val;
    }
  }
  fmp_update_slot_rel_vol(work, fmp, part);
  return true;
}

// 1e7d
static bool fmp_cmde3_slotrelvol_add_fm(struct fmdriver_work *work,
                                        struct driver_fmp *fmp,
                                        struct fmp_part *part) {
  uint8_t val = fmp_part_cmdload(fmp, part);
  uint8_t mask = fmp_part_cmdload(fmp, part);
  for (int i = 0; i < 4; i++) {
    if (!(mask & (1<<i))) continue;
    int svol = part->u.fm.slot_rel_vol[i] + val;
    if (svol > 0x7f) {
      svol = (val & 0x80) ? 0 : 0x7f;
    }
    part->u.fm.slot_rel_vol[i] = svol;
  }
  fmp_update_slot_rel_vol(work, fmp, part);
  return true;
}

// 2c5b
static void fmp_part_lfo_calc(struct driver_fmp *fmp,
                              struct fmp_part *part, int num) {
  struct fmp_lfo *lfo = &part->lfo[num];
  uint8_t waveform = lfo->waveform;
  if (waveform > 6) waveform = 0;
  switch (waveform) {
  case FMP_LFOWF_TRIANGLE:
  case FMP_LFOWF_TRIANGLE2:
    // 2c7f
    if (!lfo->delay) return;
    if (--lfo->delay_cnt) return;
    lfo->delay_cnt = 1;
    if (--lfo->speed_cnt) return;
    lfo->speed_cnt = lfo->speed;
    part->actual_freq += lfo->rate2;
    if (num == 0) part->pdzf.lfodiff += u16s16(lfo->rate2);
    if (--lfo->depth_cnt) return;
    lfo->depth_cnt = lfo->depth;
    lfo->rate2 = -lfo->rate2;
    return;
  case FMP_LFOWF_SAWTOOTH:
    // 2cae
    if (!lfo->delay) return;
    if (--lfo->delay_cnt) return;
    lfo->delay_cnt = 1;
    if (--lfo->speed_cnt) return;
    lfo->speed_cnt = lfo->speed;
    part->actual_freq += lfo->rate;
    if (num == 0) part->pdzf.lfodiff += u16s16(lfo->rate);
    if (--lfo->depth_cnt) return;
    lfo->depth_cnt = lfo->depth;
    part->actual_freq -= lfo->rate2;
    if (num == 0) part->pdzf.lfodiff -= u16s16(lfo->rate2);
    return;
  case FMP_LFOWF_SQUARE:
    // 2ce0
    if (!lfo->delay) return;
    if (--lfo->delay_cnt) return;
    lfo->delay_cnt = 1;
    if (--lfo->speed_cnt) return;
    lfo->speed_cnt = lfo->speed;
    part->actual_freq += (u16s16(lfo->rate2) >> (lfo->depth_cnt != 0));
    if (num == 0) part->pdzf.lfodiff += (u16s16(lfo->rate2) >> (lfo->depth_cnt != 0));
    if (lfo->depth_cnt) lfo->depth_cnt = 0;
    lfo->rate2 = -lfo->rate2;
    return;
  case FMP_LFOWF_LINEAR:
    // 2d10
    if (!lfo->delay) return;
    if (!lfo->depth_cnt) return;
    if (--lfo->delay_cnt) return;
    lfo->delay_cnt = 1;
    if (--lfo->speed_cnt) return;
    lfo->speed_cnt = lfo->speed;
    part->actual_freq += lfo->rate;
    if (num == 0) part->pdzf.lfodiff += u16s16(lfo->rate);
    lfo->depth_cnt--;
    return;
  case FMP_LFOWF_STAIRCASE:
    // 2d3a
    if (!lfo->delay) return;
    if (--lfo->delay_cnt) return;
    lfo->delay_cnt = 1;
    if (--lfo->speed_cnt) return;
    lfo->speed_cnt = lfo->speed;
    part->actual_freq += lfo->rate2;
    if (num == 0) part->pdzf.lfodiff += u16s16(lfo->rate2);
    if (--lfo->depth_cnt) return;
    lfo->depth_cnt = 2;
    lfo->rate2 = -lfo->rate2;
    return;
  case FMP_LFOWF_RANDOM:
    if (!lfo->delay) return;
    if (--lfo->delay_cnt) return;
    lfo->delay_cnt = 1;
    if (--lfo->speed_cnt) return;
    lfo->speed_cnt = lfo->speed;
    {
      uint8_t rand = fmp_rand71(fmp);
      uint16_t a = (lfo->rate>>1) - (rand*lfo->depth_cnt)%lfo->rate;
      lfo->depth_cnt = rand;
      part->actual_freq -= lfo->rate2;
      part->actual_freq += a;
      if (num == 0) part->pdzf.lfodiff -= u16s16(lfo->rate2);
      if (num == 0) part->pdzf.lfodiff += u16s16(a);
      lfo->rate2 = a;
      return;
    }
  }
}

// 27c9
static void fmp_part_keyon_fm(struct fmdriver_work *work,
                              struct driver_fmp *fmp,
                              struct fmp_part *part) {
  if (part->lfo_f.mask) return;
  // 27df
  uint8_t out = part->opna_keyon_out;
  if (part->type.fm_3) {
    fmp->fm3_slot_keyon |= ((~part->u.fm.slot_mask)&0xf0);
    out |= fmp->fm3_slot_keyon;
  } else {
    // 27fb
    out |= 0xf0;
  }
  work->opna_writereg(work, 0x28, out);
}

// 2b1c-2b79
static void fmp_part_wlfo(struct fmdriver_work *work,
                          struct fmp_part *part) {
  struct fmp_wlfo *wlfo = &part->u.fm.wlfo;
  // 2b1c
  if (!wlfo->delay) return;
  if (--wlfo->delay_cnt) return;
  wlfo->delay_cnt = 1;
  if (--wlfo->speed_cnt) return;
  wlfo->speed_cnt = wlfo->speed;
  // 2b36
  wlfo->rate_curr += wlfo->rate;
  for (int i = 0; i < 4; i++) {
    // 2b4e
    if (!(wlfo->sync & (1<<i))) continue;
    uint8_t tl = part->u.fm.tone_tl[i] + wlfo->rate_curr;
    if (tl & 0x80) {
      tl = (wlfo->rate_curr & 0x80) ? 0 : 0x7f;
    }
    fmp_part_fm_reg_write(work, part, OPNA_TL+4*i, tl);
  }
  if (!--wlfo->depth_cnt) {
    wlfo->depth_cnt = wlfo->depth;
    wlfo->rate = -wlfo->rate;
  }
  // 2b79
}

// 2e0c
static void fmp_part_freq_ppz8(struct fmdriver_work *work,
                               struct fmp_part *part) {
  uint32_t freq = part->actual_freq;
  uint8_t octave = part->u.ssg.octave;
  if (!octave) return;
  if (octave != 4) {
    if (octave > 4) {
      // 2e22
      freq <<= (octave-4);
    } else {
      freq >>= (4-octave);
    }
  }
  // 2e3b
  int32_t detune = u16s16(part->detune);
  freq += detune << 6;
  if (work->ppz8_functbl) {
    work->ppz8_functbl->channel_freq(work->ppz8, part->opna_keyon_out, freq);
  }
}

uint32_t fmp_pdzf_extended_freqdiff(struct fmp_part *part, int32_t diff) {
  uint32_t freq = fmp_ppz8_note_freq(part->pdzf.prev_note);
  freq += diff << 6;
  int coeff = (part->pdzf.prev_note/0xc) - 5;
  if (coeff > 0) {
    freq += diff << coeff;
  } else {
    freq += diff >> (-coeff);
  }
  return freq;
}

uint32_t fmp_pdzf_extended_freq(struct fmp_part *part) {
  return fmp_pdzf_extended_freqdiff(
    part,
    u16s16(part->detune) + part->pdzf.pit.pitchdiff + part->pdzf.lfodiff
  );
}

static void fmp_part_keyon_pdzf(struct fmdriver_work *work,
                                struct fmp_part *part) {
  uint8_t voice = part->pdzf.voice;
  uint8_t pan = part->pdzf.pan + 5;
  part->pdzf.pit.pitchdiff = 0;
  part->pdzf.lfodiff = 0;
  uint32_t freq;
  if (part->pdzf.mode == 2) {
    freq = fmp_pdzf_extended_freq(part);
  } else {
    freq = fmp_ppz8_note_freq(part->prev_note);
    freq += (u16s16(part->detune) << 3);
  }

  if (!part->pdzf.keyon) {
    if (work->ppz8_functbl) {
      work->ppz8_functbl->channel_play(
        work->ppz8,
        part->pdzf.ppz8_channel,
        voice
      );
    }
    part->pdzf.env_state.status = PDZF_ENV_ATT;
    part->pdzf.env_state.vol = 0;
    part->pdzf.env_state.cnt = part->pdzf.env_param.al;
    part->pdzf.keyon = true;
  }

  if (work->ppz8_functbl) {
    work->ppz8_functbl->channel_pan(
      work->ppz8,
      part->pdzf.ppz8_channel,
      pan
    );
    work->ppz8_functbl->channel_freq(
      work->ppz8,
      part->pdzf.ppz8_channel,
      freq
    );
    work->ppz8_functbl->channel_volume(
      work->ppz8,
      part->pdzf.ppz8_channel,
      fmp_pdzf_vol_clamp(part->pdzf.vol, part->pdzf.env_state.vol)
    );
  }
  part->pdzf.lastfreq = freq;
}

// 27a3
static void fmp_part_keyon(struct fmdriver_work *work,
              struct driver_fmp *fmp,
              struct fmp_part *part) {
  part->ext_keyon = 0xffff;
  part->status.keyon = false;
  if (part->pdzf.mode) {
    fmp_part_keyon_pdzf(work, part);
  }
  if (!part->type.ssg) {
    if (part->lfo_f.lfo) {
      part->actual_freq = fmp_fm_freq(part->prev_note);
      part->prev_freq = 0;
      part->lfo_f.lfo = false;
    }
    fmp_part_keyon_fm(work, fmp, part);
  } else {
    // 2805
    if (part->lfo_f.lfo) {
      uint8_t note = part->prev_note;
      part->u.ssg.octave = fmp_ssg_octave(note);
      part->actual_freq = fmp_part_ssg_freq(part, note);
      part->prev_freq = 0;
      part->lfo_f.lfo = false;
    }
    // 281e
    if (part->lfo_f.mask) {
      part->u.ssg.env_f.attack = false;
      part->u.ssg.env_f.portamento = true;
      return;
    }
    // 2823
    if (part->u.ssg.env_f.ppz) {
      if (work->ppz8_functbl) {
        work->ppz8_functbl->channel_play(
          work->ppz8, part->opna_keyon_out, part->tone
        );
      }
      fmp_part_ppz8_vol(work, part);
      fmp_part_freq_ppz8(work, part);
      return;
    }
    part->u.ssg.env_f.attack = true;
  }
}

// 30b1
static void fmp_part_init_wlfo(struct fmdriver_work *work,
                               struct fmp_part *part) {
  if (!(part->u.fm.wlfo.sync & 0x80)) return;
  for (int s = 0; s < 4; s++) {
    if (!(part->u.fm.wlfo.sync & (1<<s))) continue;
    fmp_part_fm_reg_write(work, part, OPNA_TL+4*s, part->u.fm.tone_tl[s]);
  }
  // 30d5
  part->u.fm.wlfo.delay_cnt = part->u.fm.wlfo.delay;
  part->u.fm.wlfo.depth_cnt = part->u.fm.wlfo.depth;
  part->u.fm.wlfo.rate_curr = 0;
  part->u.fm.wlfo.rate = part->u.fm.wlfo.rate_orig;
}

// 309b
static void fmp_part_hlfo(struct fmdriver_work *work,
                          struct fmp_part *part) {
  work->opna_writereg(work, 0x22, part->u.fm.hlfo_freq);
  part->pan_ams_pms &= 0xc0;
  part->pan_ams_pms |= part->u.fm.hlfo_apms;
  fmp_part_write_pan(work, part);
}

// 2fdc
static void fmp_part_init_lfo_awe(struct fmdriver_work *work,
                                  struct driver_fmp *fmp,
                                  struct fmp_part *part) {
  if (part->lfo_f.w) {
    fmp_part_init_wlfo(work, part);
  }
  if (part->lfo_f.a || part->lfo_f.e || (part->current_vol != part->actual_vol)) {
    part->actual_vol = part->current_vol;
    uint8_t pvol = part->actual_vol + fmp->fm_vol;
    if (pvol & 0x80) {
      pvol = (fmp->fm_vol & 0x80) ? 0 : 0x7f;
    }
    for (int s = 0; s < 4; s++) {
      if (!(part->slot_vol_mask & (1<<s))) {
        // 3024
        uint8_t vol = part->u.fm.tone_tl[s];
        vol -= part->u.fm.slot_rel_vol[s];
        if (vol & 0x80) {
          vol = (part->u.fm.slot_rel_vol[s] & 0x80) ? 0x7f : 0x00;
        }
        // 3039
        if (!(part->u.fm.slot_mask & (1<<s))) {
          fmp_part_fm_reg_write(work, part, OPNA_TL+4*s, vol);
        }
      } else {
        // 3042
        uint8_t svol = pvol;
        if (fmp->data_version >= 0x06) {
          svol -= part->u.fm.slot_rel_vol[s];
          if (svol & 0x80) {
            svol = (part->u.fm.slot_rel_vol[s] & 0x80) ? 0x7f : 0x00;
          }
          // 305c
          svol += part->u.fm.tone_tl[s];
          if (svol & 0x80) svol = 0x7f;
        }
        // 3062
        if (!(part->u.fm.slot_mask & (1<<s))) {
          fmp_part_fm_reg_write(work, part, OPNA_TL+4*s, svol);
        }
      }
      // 3069
    }
  }
  // 3073
  part->u.fm.alfo.delay_cnt = part->u.fm.alfo.delay;
  part->u.fm.alfo.depth_cnt = part->u.fm.alfo.depth;
  // this line not found in fmp but should be there somewhere
  // without this, when LFO E, delay would be random every keyon
  part->u.fm.alfo.speed_cnt = part->u.fm.alfo.speed;
  
  part->u.fm.alfo.rate = part->u.fm.alfo.rate_orig;
  if (part->u.fm.hlfo_freq & 0x80) {
    work->opna_writereg(work, 0x22, 0x00);
    part->u.fm.hlfo_delay_cnt = part->u.fm.hlfo_delay;
    if (!part->u.fm.hlfo_delay_cnt) {
      // 309b
      work->opna_writereg(work, 0x22, part->u.fm.hlfo_freq);
      part->pan_ams_pms &= 0xc0;
      part->pan_ams_pms |= part->u.fm.hlfo_apms;
      fmp_part_write_pan(work, part);
    }
  }
  // 30b0
}

static void fmp_part_pdzf_env(struct fmdriver_work *work,
                              struct driver_fmp *fmp,
                              struct fmp_part *part) {
  (void)fmp;
  switch (part->pdzf.env_state.status) {
  case PDZF_ENV_ATT:
    if (!part->pdzf.env_state.cnt--) {
      part->pdzf.env_state.vol = part->pdzf.env_param.dd;
      part->pdzf.env_state.status = PDZF_ENV_DEC;
      if (part->pdzf.env_state.vol > PDZF_ENV_VOL_MIN) {
        part->pdzf.env_state.cnt = part->pdzf.env_param.sr;
        if (work->ppz8_functbl) {
          work->ppz8_functbl->channel_volume(
            work->ppz8,
            part->pdzf.ppz8_channel,
            fmp_pdzf_vol_clamp(part->pdzf.vol, part->pdzf.env_state.vol)
          );
        }
      } else {
        part->pdzf.env_state.vol = PDZF_ENV_VOL_MIN;
        part->pdzf.env_state.status = PDZF_ENV_OFF;
        if (work->ppz8_functbl) {
          work->ppz8_functbl->channel_stop(
            work->ppz8,
            part->pdzf.ppz8_channel
          );
        }
      }
    }
    break;
  case PDZF_ENV_DEC:
    if (!part->pdzf.env_param.sr) {
      //part->pdzf.env_state.vol = PDZF_ENV_VOL_MIN;
    } else if (!--part->pdzf.env_state.cnt) {
      part->pdzf.env_state.vol--;
      if (part->pdzf.env_state.vol > PDZF_ENV_VOL_MIN) {
        part->pdzf.env_state.cnt = part->pdzf.env_param.sr;
        if (work->ppz8_functbl) {
          work->ppz8_functbl->channel_volume(
            work->ppz8,
            part->pdzf.ppz8_channel,
            fmp_pdzf_vol_clamp(part->pdzf.vol, part->pdzf.env_state.vol)
          );
        }
      }
    }
    if (part->pdzf.env_state.vol <= PDZF_ENV_VOL_MIN) {
        part->pdzf.env_state.vol = PDZF_ENV_VOL_MIN;
        part->pdzf.env_state.status = PDZF_ENV_OFF;
        if (work->ppz8_functbl) {
          work->ppz8_functbl->channel_stop(
            work->ppz8,
            part->pdzf.ppz8_channel
          );
        }
      }
    break;
  case PDZF_ENV_REL:
    if (!--part->pdzf.env_state.cnt) {
      part->pdzf.env_state.vol--;
      if (part->pdzf.env_state.vol > PDZF_ENV_VOL_MIN) {
        part->pdzf.env_state.cnt = part->pdzf.env_param.rr;
        if (work->ppz8_functbl) {
          work->ppz8_functbl->channel_volume(
            work->ppz8,
            part->pdzf.ppz8_channel,
            fmp_pdzf_vol_clamp(part->pdzf.vol, part->pdzf.env_state.vol)
          );
        }
      } else {
        part->pdzf.env_state.vol = PDZF_ENV_VOL_MIN;
        part->pdzf.env_state.status = PDZF_ENV_OFF;
        if (work->ppz8_functbl) {
          work->ppz8_functbl->channel_stop(
            work->ppz8,
            part->pdzf.ppz8_channel
          );
        }
      }
    }
    break;
  case PDZF_ENV_OFF:
    break;
  }
}

// 2ad9
static void fmp_part_lfo(struct fmdriver_work *work,
                         struct driver_fmp *fmp,
                         struct fmp_part *part) {
  // 2ad9
  if (part->lfo_f.p) fmp_part_lfo_calc(fmp, part, 0);
  if (part->lfo_f.q) fmp_part_lfo_calc(fmp, part, 1);
  if (part->lfo_f.r) fmp_part_lfo_calc(fmp, part, 2);
  if (part->type.fm) {
    // 2b03
    if ((part->u.fm.hlfo_freq & 0x04) && part->u.fm.hlfo_delay_cnt) {
      if (!--part->u.fm.hlfo_delay_cnt) {
        fmp_part_hlfo(work, part);
      }
    }
    // 2b17
    if (part->lfo_f.w) {
      fmp_part_wlfo(work, part);
    }
    // 2b79
    if (part->lfo_f.a || part->lfo_f.e) {
      // 2b7e
      if (!part->u.fm.alfo.delay) return;
      if (--part->u.fm.alfo.delay_cnt) return;
      part->u.fm.alfo.delay_cnt = 1;
      if (--part->u.fm.alfo.speed_cnt) return;
      part->u.fm.alfo.speed_cnt = part->u.fm.alfo.speed;
      uint8_t vol = part->actual_vol + part->u.fm.alfo.rate;
      if (vol & 0x80) {
        vol = (part->u.fm.alfo.rate & 0x80) ? 0x00 : 0x7f;
      }
      part->actual_vol = vol;
      if (!part->lfo_f.e) {
        // 2bb3
        fmp_part_fm_vol(work, fmp, part);
        if (--part->u.fm.alfo.depth_cnt) return;
        part->u.fm.alfo.depth_cnt = part->u.fm.alfo.depth;
        part->u.fm.alfo.rate = -part->u.fm.alfo.rate;
      } else {
        // 2bc5
        fmp_part_keyoff_fm(work, fmp, part);
        fmp_part_fm_vol(work, fmp, part);
        fmp_part_keyon_fm(work, fmp, part);
        part->u.fm.alfo.speed_cnt = part->u.fm.alfo.depth_cnt;
        if (part->u.fm.alfo.speed == part->u.fm.alfo.speed_cnt) {
          part->u.fm.alfo.depth_cnt = part->u.fm.alfo.depth;
        } else {
          part->u.fm.alfo.depth_cnt = part->u.fm.alfo.speed;
        }
      }
    } 
  }
}

// 2a51
static void fmp_part_fm(struct fmdriver_work *work, struct driver_fmp *fmp,
                        struct fmp_part *part) {
  if (part->status.lfo_sync) {
    fmp_part_init_lfo_pqr(part);
    fmp_part_init_lfo_awe(work, fmp, part);
    part->actual_freq = fmp_fm_freq(part->prev_note);
    part->status.lfo_sync = false;
    part->pdzf.lfodiff = 0;
  }
  // 2a6a
  if (part->status.pitchbend || fmp->datainfo.flags.lfo_octave_fix) {
    uint16_t fnum = part->actual_freq & 0x7ff;
    if (fnum > 0x4d3u) {
      part->actual_freq += 0x800;
      part->actual_freq -= 0x269;
    } else if (fnum < 0x26au) {
      part->actual_freq -= 0x800;
      part->actual_freq += 0x269;
      if (part->actual_freq & 0x8000) {
        part->actual_freq = 0x200;
      }
    }
  }
  // 2aa3
  if ((part->actual_freq + part->detune) != part->prev_freq) {
    part->prev_freq = part->actual_freq + part->detune;
    if (part->type.fm_3 && (fmp->timer_ch3 & 0x40)) {
      // 2be7
      static const uint8_t ch3_fnum_addr[] = {
        0xad, 0xac, 0xae, 0xa6
      };
      static const uint8_t ch3_index[] = {
        0, 2, 1, 3
      };
      for (int i = 0; i < 4; i++) {
        if (part->u.fm.slot_mask & (1<<i)) continue;
        uint16_t freq = part->prev_freq + fmp->ch3_se_freqdiff[ch3_index[i]];
        work->opna_writereg(work, ch3_fnum_addr[i], freq>>8);
        work->opna_writereg(work, ch3_fnum_addr[i]-4, freq&0xff);
      }
    } else {
      fmp_part_fm_reg_write(work, part, OPNA_BLKFNUM2, part->prev_freq>>8);
      fmp_part_fm_reg_write(work, part, OPNA_FNUM1, part->prev_freq&0xff);
    }
  }
  // 2ad0
  if (part->status.keyon) {
    fmp_part_keyon(work, fmp, part);
  }
  fmp_part_lfo(work, fmp, part);
}

// 2e80-2ed6
static void fmp_part_ssg_env_adsr(struct fmp_part *part) {
  // 2e80
  if (!part->u.ssg.env_f.decay) {
    unsigned newvol = part->actual_vol + part->u.ssg.env.attack_rate;
    if (!(newvol & 0x100) && (part->u.ssg.vol > newvol)) {
      part->actual_vol = newvol;
      return;
    }
    // 2e96
    part->actual_vol = part->u.ssg.vol;
    part->u.ssg.env_f.decay = true;
  }
  // 2e9d
  if (!part->u.ssg.env_f.sustain) {
    unsigned newvol = part->actual_vol - part->u.ssg.env.decay_rate;
    if (!(newvol & 0x100) && (part->u.ssg.env.sustain_lv < newvol)) {
      part->actual_vol = newvol;
      return;
    }
    // 2eb3
    part->actual_vol = part->u.ssg.env.sustain_lv;
    part->u.ssg.env_f.sustain = true;
  }
  // 2eba
  if (part->u.ssg.env_f.release) return;
  unsigned newvol = part->actual_vol - part->u.ssg.env.sustain_rate;
  if (!(newvol & 0x100)) {
    part->actual_vol = newvol;
    return;
  }
  // 2ec8
  part->actual_vol = 0;
  part->u.ssg.env_f.release = true;
  part->u.ssg.env_f.attack = false;
}

// 2e7a
static void fmp_part_ssg_env(struct fmdriver_work *work,
                             struct fmp_part *part) {
  if (part->u.ssg.env_f.attack) {
    fmp_part_ssg_env_adsr(part);
  } else {
    // 2ed6
    if (!part->u.ssg.env_f.portamento) {
      //part->u.ssg.curr_vol = dh
      return;
    } else {
      // 2edc
      if (!part->u.ssg.env_f.release) {
        // 2ee2
        unsigned newvol = part->actual_vol - part->u.ssg.env.release_rate;
        if (!(newvol & 0x100)) {
          part->actual_vol = newvol;
        } else {
          part->actual_vol = 0;
          part->u.ssg.env_f.release = true;
          part->u.ssg.env_f.portamento = false;
        }
      }
    }
  }
  // 2ef6
  uint8_t outvol = ((part->actual_vol * part->current_vol) >> 8);
  if (outvol == part->u.ssg.curr_vol) {
    //part->u.ssg.curr_vol = dh
    return;
  }
  // 2f03
  work->opna_writereg(work, 0x8+part->opna_keyon_out, outvol);
}

// 2db0
static void fmp_part_ssg(struct fmdriver_work *work, struct driver_fmp *fmp,
                         struct fmp_part *part) {
  if (part->status.lfo_sync) {
    fmp_part_init_lfo_pqr(part);
    part->u.ssg.octave = fmp_ssg_octave(part->prev_note);
    part->actual_freq = fmp_part_ssg_freq(part, part->prev_note);
    part->status.lfo_sync = false;
    part->pdzf.lfodiff = 0;
  }
  // 2dc9
  if (part->status.keyon) {
    fmp_part_keyon(work, fmp, part);
  }
  // 2dd2
  if (part->u.ssg.env_f.ppz) return;
  fmp_part_ssg_env(work, part);
  uint16_t freq = part->actual_freq >> part->u.ssg.octave;
  if (freq == part->prev_freq) {
    // 2ad0
    if (part->status.keyon) {
      fmp_part_keyon(work, fmp, part);
    }
  } else {
    part->prev_freq = freq;
    if (!part->eff_chan) {
      // 2df8
      unsigned reg = part->opna_keyon_out*2;
      work->opna_writereg(work, reg, freq&0xff);
      work->opna_writereg(work, reg|1, freq>>8);
    }
  }
  fmp_part_lfo(work, fmp, part);
}

// 2f61
static void fmp_part_ssg_env_reset(struct fmp_part *part) {
  part->u.ssg.env_f.decay = false;
  part->u.ssg.env_f.sustain = false;
  part->u.ssg.env_f.release = false;
  uint8_t vol = part->u.ssg.env.startvol;
  if (vol >= part->u.ssg.vol) {
    part->u.ssg.env_f.decay = true;
    vol = part->u.ssg.vol;
  }
  part->actual_vol = vol;
  if (part->u.ssg.env.sustain_lv >= part->u.ssg.vol) {
    part->u.ssg.env_f.sustain = true;
  }
}

// 28b3
static void fmp_adpcm_addr_set(struct fmdriver_work *work,
                               struct driver_fmp *fmp,
                               struct fmp_part *part) {
  if (part->lfo_f.mask) return;
  work->opna_writereg(work, 0x100, 0x00);
  work->opna_writereg(work, 0x100, 0x01);
  //work->opna_writereg(work, 0x110, 0x08);
  //work->opna_writereg(work, 0x110, 0x80);
  work->opna_writereg(work, 0x101, part->pan_ams_pms | fmp->adpcm_c1);
  work->opna_writereg(work, 0x102, part->u.adpcm.startaddr & 0xff);
  work->opna_writereg(work, 0x103, part->u.adpcm.startaddr >> 8);
  work->opna_writereg(work, 0x104, part->u.adpcm.endaddr & 0xff);
  work->opna_writereg(work, 0x105, part->u.adpcm.endaddr >> 8);
}

// 2853
static void fmp_part_adpcm(struct fmdriver_work *work,
                           struct driver_fmp *fmp,
                           struct fmp_part *part) {
  if (!part->status.keyon) return;
  part->ext_keyon = 0xffff;
  part->status.keyon = false;
  if (part->status.tie_cont) return;
  fmp_adpcm_addr_set(work, fmp, part);
  part->actual_freq = fmp_adpcm_freq(part->prev_note);
  uint16_t freq = part->actual_freq;
  // why does the detune byte has to be swapped??
  freq += (part->detune >> 8) | ((part->detune << 8) & 0xff00);
  freq += part->u.adpcm.deltat;
  work->opna_writereg(work, 0x109, freq & 0xff);
  work->opna_writereg(work, 0x10a, freq >> 8);
  work->opna_writereg(work, 0x100, 0xa0);
}

// 2750
static void fmp_part_keyon_pre(struct driver_fmp *fmp, struct fmp_part *part) {
  // TODO: seproc
  if (fmp->datainfo.flags.q) {
    // 275e
    uint8_t tonelen = part->tonelen_cnt;
    unsigned shift = 0;
    if (tonelen < 0x10) shift = 3;
    tonelen <<= shift;
    tonelen >>= 3;
    tonelen *= part->gate_cnt;
    tonelen >>= shift;
    part->gate_cmp = part->tonelen_cnt - tonelen;
  }
  // 277f
  part->status.keyon = true;
  if (part->type.ssg && !part->status.tie_cont && !part->u.ssg.env_f.ppz) {
    part->u.ssg.env_f.portamento = false;
    if (!part->lfo_f.mask) {
      fmp_part_ssg_env_reset(part);
    }
  }
}

// 1cde
static void fmp_part_pit_end_ssg(struct fmp_part *part) {
  part->u.ssg.octave = fmp_ssg_octave(part->pit.target_note);
  part->actual_freq = fmp_part_ssg_freq(part, part->pit.target_note);
  part->pit.rate = 0;
  part->status.pitchbend = false;
  part->prev_note = part->pit.target_note;
}

// 1cc4
static void fmp_part_pit_end_fm(struct fmp_part *part) {
  uint8_t p_delay_cnt = part->lfo[0].delay_cnt;
  uint8_t q_delay_cnt = part->lfo[1].delay_cnt;
  uint8_t r_delay_cnt = part->lfo[2].delay_cnt;
  fmp_part_init_lfo_pqr(part);
  part->lfo[0].delay_cnt = p_delay_cnt;
  part->lfo[1].delay_cnt = q_delay_cnt;
  part->lfo[2].delay_cnt = r_delay_cnt;
  // 1cdb
  part->actual_freq = part->pit.target_freq;
  part->pit.rate = 0;
  part->status.pitchbend = false;
  part->prev_note = part->pit.target_note;
}

static void fmp_part_pit_pdzf(
  struct fmdriver_work *work, 
  struct fmp_part *part
) {
  (void)work;
  if (part->pdzf.mode != 2) return;
  if (--part->pdzf.pit.delay) return;
  part->pdzf.pit.delay = 1;
  if (--part->pdzf.pit.speed_cnt) return;
  part->pdzf.pit.speed_cnt = part->pdzf.pit.speed;
  int8_t rate = u8s8(part->pdzf.pit.rate);
  if (!rate) return;
  part->pdzf.pit.pitchdiff += rate;
  uint32_t freq = fmp_pdzf_extended_freqdiff(part, part->pdzf.pit.pitchdiff);
  if (((rate > 0) && (freq > part->pdzf.pit.target_freq)) ||
      ((rate < 0) && (freq < part->pdzf.pit.target_freq))) {
    part->pdzf.pit.on = false;
    part->pdzf.prev_note = part->pdzf.pit.target_note;
    part->pdzf.pit.pitchdiff = 0;
  }
}
// 1c4b
static void fmp_part_pit(struct fmp_part *part) {
  if (part->type.adpcm) return;
  if (--part->pit.delay) return;
  part->pit.delay = 1;
  if (--part->pit.speed_cnt) return;
  part->pit.speed_cnt = part->pit.speed;
  uint16_t rate = part->pit.rate;
  if (rate & 0x80) rate |= 0xff00;
  if (!rate) return;
  // 1c77
  uint16_t freq = part->actual_freq + rate;
  if (!(rate & 0x8000)) {
    // rate positive
    // 1c79
    if (!part->type.fm) {
      // 1c83
      if ((freq >> part->u.ssg.octave) <= part->pit.target_freq) {
        part->actual_freq = freq;
      } else {
        fmp_part_pit_end_ssg(part);
      }
    } else {
      // 1c93
      if (freq < part->pit.target_freq) {
        part->actual_freq = freq;
      } else {
        fmp_part_pit_end_fm(part);
      }
    }
  } else {
    // rate negative
    // 1ca5
    if (!part->type.fm) {
      if ((freq >> part->u.ssg.octave) >= part->pit.target_freq) {
        part->actual_freq = freq;
      } else {
        fmp_part_pit_end_ssg(part);
      }
    } else {
      // 1cbf
      if (freq > part->pit.target_freq) {
        part->actual_freq = freq;
      } else {
        fmp_part_pit_end_fm(part);
      }
    }
  }
  // 1cef
}

// 1d81
static bool fmp_part_cmd_exec(struct fmdriver_work *work,
                              struct driver_fmp *fmp,
                              struct fmp_part *part,
                              uint8_t cmd) {
  enum {
    JMPTBL_LEN = 0x80-0x62
  };
  typedef bool (*cmdfunc_t)(struct fmdriver_work *work,
                          struct driver_fmp *fmp,
                          struct fmp_part *part);
  static const cmdfunc_t fm_jmptbl[JMPTBL_LEN] = {
    fmp_cmd62_tempo,
    fmp_cmd63_vol_d_fm,
    fmp_cmd64_loop,
    fmp_cmd65_loopend,
    fmp_cmd66_tie,
    fmp_cmd67_q,
    fmp_cmd68_pitchbend,
    fmp_cmd69_vol_fm,
    fmp_cmd6a_voldec_fm,
    fmp_cmd6b_volinc_fm,
    fmp_cmd6c_kondelay,
    fmp_cmd6d_detune,
    fmp_cmd6e_poke,
    fmp_cmd6f_sync,
    fmp_cmd70_wait,
    fmp_cmd71_tone_fm,
    fmp_cmd72_deflen,
    fmp_cmd73_relvol_fm,
    fmp_cmd74_loop,
    fmp_cmd75_lfo,
    fmp_cmd76_lfo_p,
    fmp_cmd77_lfo_q,
    fmp_cmd78_lfo_r,
    fmp_cmd79_lfo_a_fm,
    fmp_cmd7a_tie,
    fmp_cmd7b_transpose,
    fmp_cmd7c_lfo_pan_fm,
    fmp_cmd7d_sync,
    fmp_cmd7e_loop_det,
    fmp_cmd7f_lfo_delay
  };
  static const cmdfunc_t ssg_jmptbl[JMPTBL_LEN] = {
    fmp_cmd62_tempo,
    fmp_cmd63_mix_ssg,
    fmp_cmd64_loop,
    fmp_cmd65_loopend,
    fmp_cmd66_tie,
    fmp_cmd67_q,
    fmp_cmd68_pitchbend,
    fmp_cmd69_vol_ssg,
    fmp_cmd6a_voldec_ssg,
    fmp_cmd6b_volinc_ssg,
    fmp_cmd6c_kondelay,
    fmp_cmd6d_detune,
    fmp_cmd6e_poke,
    fmp_cmd6f_sync,
    fmp_cmd70_wait,
    fmp_cmd71_envreset_ssg,
    fmp_cmd72_deflen,
    fmp_cmd73_noise_ssg,
    fmp_cmd74_loop,
    fmp_cmd75_lfo,
    fmp_cmd76_lfo_p,
    fmp_cmd77_lfo_q,
    fmp_cmd78_lfo_r,
    fmp_cmd79_env_ssg,
    fmp_cmd7a_tie,
    fmp_cmd7b_transpose,
    fmp_cmd7c_tone_ssg,
    fmp_cmd7d_sync,
    fmp_cmd7e_loop_det,
    fmp_cmd7f_lfo_delay
  };
  static const cmdfunc_t adpcm_jmptbl[JMPTBL_LEN] = {
    fmp_cmd62_tempo,
    fmp_cmd63_vol_d_adpcm,
    fmp_cmd64_loop,
    fmp_cmd65_loopend,
    fmp_cmd66_tie,
    fmp_cmd67_q,
    fmp_cmd68_adpcm,
    fmp_cmd69_adpcm,
    fmp_cmd6a_voldec_adpcm,
    fmp_cmd6b_volinc_adpcm,
    fmp_cmd6c_kondelay,
    fmp_cmd6d_detune,
    fmp_cmd6e_poke,
    fmp_cmd6f_sync,
    fmp_cmd70_wait,
    fmp_cmd71_tone_adpcm,
    fmp_cmd72_deflen,
    fmp_cmd73_relvol_adpcm,
    fmp_cmd74_loop,
    fmp_cmd75_lfo,
    fmp_cmd76_lfo_p,
    fmp_cmd77_lfo_q,
    fmp_cmd78_lfo_r,
    fmp_cmd79_lfo_a_fm,
    fmp_cmd7a_tie,
    fmp_cmd7b_transpose,
    fmp_cmd7c_pan_adpcm,
    fmp_cmd7d_sync,
    fmp_cmd7e_loop_det,
    fmp_cmd7f_lfo_delay
  };
  const cmdfunc_t *functable;
  if (part->type.adpcm) {
    functable = adpcm_jmptbl;
  } else {
    functable = part->type.ssg ? ssg_jmptbl : fm_jmptbl;
  }
  return functable[cmd-0x62](work, fmp, part);
}

static bool fmp_part_cmd_exec2(struct fmdriver_work *work,
                              struct driver_fmp *fmp,
                              struct fmp_part *part,
                              uint8_t cmd) {
  if (!part->type.fm || (cmd > 0xe3)) {
    part->current_ptr = 0xffff;
    return false;
  }
  
  typedef bool (*cmdfunc_t)(struct fmdriver_work *work,
                          struct driver_fmp *fmp,
                          struct fmp_part *part);
  static const cmdfunc_t fm_jmptbl2[2] = {
    fmp_cmde2_slotrelvol_set_fm,
    fmp_cmde3_slotrelvol_add_fm,
  };
  return fm_jmptbl2[cmd-0xe2](work, fmp, part);
}

// 2936
static void fmp_part_keyoff_q(struct fmdriver_work *work,
                              struct driver_fmp *fmp,
                              struct fmp_part *part) {
  if (part->type.adpcm) {
    part->ext_keyon = 0;
    part->status.tie_cont = false;
    work->opna_writereg(work, 0x100, 0x00);
    work->opna_writereg(work, 0x100, 0x01);
    return;
  }
  if (part->type.fm || !part->u.ssg.env_f.ppz) {
    // 2961
    part->ext_keyon = 0;
    part->status.tie_cont = false;
    if (part->type.adpcm) return;
    if (!part->type.ssg) {
      // 2979
      uint8_t keyon = part->opna_keyon_out;
      if (part->type.fm_3) {
        // 2994
        fmp->fm3_slot_keyon &= (part->u.fm.slot_mask | 0x0f);
        keyon |= fmp->fm3_slot_keyon;
      }
      work->opna_writereg(work, 0x28, keyon);
      // 29a2
    } else {
      // 29a9
      part->u.ssg.curr_vol = 0xff;
      if (part->u.ssg.env.release_rate == 0) {
        part->actual_vol = 0;
      }
      part->u.ssg.env_f.portamento = true;
      part->u.ssg.env_f.attack = false;
    }
  } else {
    // 294d
    if (work->ppz8_functbl) {
      work->ppz8_functbl->channel_volume(
        work->ppz8,
        part->opna_keyon_out,
        0
      );
    }
  }
  if (part->pdzf.mode) {
    part->pdzf.keyon = false;
    if (part->lfo_f.q && part->pdzf.env_param.rr) {
      part->pdzf.env_state.status = PDZF_ENV_REL;
      part->pdzf.env_state.cnt = part->pdzf.env_param.rr;
    } else {
      part->pdzf.env_state.status = PDZF_ENV_OFF;
      if (work->ppz8_functbl) {
        work->ppz8_functbl->channel_stop(
          work->ppz8,
          part->pdzf.ppz8_channel
        );
      }
    }
  }
}

// 1c0d
static void fmp_part_cmd(struct fmdriver_work *work, struct driver_fmp *fmp,
                     struct fmp_part *part) {
  if (part->sync > 1) return;
  if (part->sync == 1) {
    // 1c19
    if (part->keyon_delay_cnt) {
      if (!--part->keyon_delay_cnt) {
        fmp_part_keyon_pre(fmp, part);
      }
    }
    // 1c27
    if ((!part->status.tie) && (!part->status.rest)) {
      if (part->tonelen_cnt && (part->tonelen_cnt == part->gate_cmp)) {
        fmp_part_keyoff_q(work, fmp, part);
      }
    }
    // 1c42
    if (part->status.pitchbend) {
      fmp_part_pit(part);
    }
    if (part->pdzf.pit.on) {
      fmp_part_pit_pdzf(work, part);
    }
    // 1cef
    if (--part->tonelen_cnt) return;
    // 1cf5
    if (!part->status.tie) {
      fmp_part_keyoff(work, fmp, part);
      part->status.pitchbend = false;
      part->pdzf.pit.on = false;
    } else {
      // 1d04
      part->status.slur = false;
      part->status.tie = false;
      part->status.tie_cont = true;
    }
  }
  // 1d0c
  part->sync = 1;
  // 1d10
  for (;;) {
    uint8_t cmd = fmp_part_cmdload(fmp, part);
    uint8_t len;
    if (!(cmd & 0x80)) {
      // 1d1a
      if (cmd >= 0x62) {
        if (!fmp_part_cmd_exec(work, fmp, part, cmd)) return;
        continue;
      } else {
        // 1d23
        len = fmp_part_cmdload(fmp, part);
      }
    } else {
      // 1d26
      cmd &= 0x7f;
      if (cmd >= 0x62) {
        if (!fmp_part_cmd_exec2(work, fmp, part, cmd | 0x80)) return;
        continue;
      } else {
        // 1d33
        len = part->default_len;
      }
    }
    // 1d36
    // note
    part->tonelen_cnt = len;
    part->tonelen = len;
    if (cmd == 0x61) {
      // 1d3e
      part->status.tie = false;
      part->status.pitchbend = false;
      part->pdzf.pit.on = false;
      part->status.tie_cont = false;
      part->status.rest = true;
      fmp_part_keyoff(work, fmp, part);
      break;
    } else {
      // 1d4d
      cmd += part->note_diff;
      part->status.rest = false;
      part->keyon_delay_cnt = part->keyon_delay;
      if (!part->keyon_delay) {
        fmp_part_keyon_pre(fmp, part);
      }
      // 1d61
      if (!part->status.tie_cont || cmd != part->prev_note) {
        // 1d6c
        part->status.lfo_sync = true;
        part->status.tie_cont = false;
        if (part->status.slur) {
          fmp_part_keyoff(work, fmp, part);
        }
      }
      // 1d7d
      part->prev_note = cmd;
      part->pdzf.prev_note = cmd;
      break;
    }
  }
}

// 269b
static void fmp_part_pan_vol_rhythm(struct fmdriver_work *work,
                                    struct fmp_rhythm *rhythm,
                                    int part) {
  work->opna_writereg(work, 0x18+part,
                      rhythm->pans[part] | rhythm->volumes[part]);
}

// 2619
static void fmp_part_cmd_rhythm(struct fmdriver_work *work,
                                struct driver_fmp *fmp) {
  struct fmp_rhythm *rhythm = &fmp->rhythm;
  struct fmp_part *part = &rhythm->part;
  if (rhythm->sync > 1) return;
  if (rhythm->sync == 1) {
    // 2623
    if (--rhythm->len_cnt) return;
  }
  // 262a
  for (;;) {
    rhythm->sync = 1;
    uint8_t cmd = fmp_part_cmdload_rhythm(fmp, part);
    if (cmd & 0x80) {
      if (cmd & 0x40) {
        // 26a5
        rhythm->tl_volume = cmd & 0x3f;
        work->opna_writereg(work, 0x11, rhythm->tl_volume);
      } else if (cmd & 0x20) {
        // 26be
        int pan = (cmd & 0x18) << 3;
        rhythm->pans[cmd&0x07] = pan;
        fmp_part_pan_vol_rhythm(work, rhythm, cmd&0x7);
      } else if (cmd == 0x90) {
        // 26d4
        rhythm->default_len = fmp_part_cmdload(fmp, part);
      } else if (cmd == 0x91) {
        // 26db
        fmp_cmd64_loop(work, fmp, part);
      } else if (cmd == 0x92) {
        // 26e2
        fmp_cmd65_loopend(work, fmp, part);
      } else if (cmd == 0x93) {
        // 26e9
        if (!fmp_cmd74_loop(work, fmp, part)) return;
      } else if (cmd == 0x94) {
        // 26ef
        if (rhythm->tl_volume < 0x3f) {
          rhythm->tl_volume++;
          work->opna_writereg(work, 0x11, rhythm->tl_volume);
        }
      } else if (cmd == 0x95) {
        // 26fb
        if (rhythm->tl_volume) {
          rhythm->tl_volume--;
          work->opna_writereg(work, 0x11, rhythm->tl_volume);
        }
      } else {
        switch (cmd & 0xf8) {
        case 0x80:
        default:
          // 268b
          rhythm->volumes[cmd&0x7] = fmp_part_cmdload(fmp, part);
          fmp_part_pan_vol_rhythm(work, rhythm, cmd&0x7);
          break;
        case 0x88:
          // 2707
          {
            uint8_t vol = rhythm->volumes[cmd&0x7] + 1;
            if (vol & 0xe0) vol = 0x1f;
            rhythm->volumes[cmd&0x7] = vol;
          }
          fmp_part_pan_vol_rhythm(work, rhythm, cmd&0x7);
          break;
        case 0x98:
          // 2719
          {
            uint8_t vol = rhythm->volumes[cmd&0x7] - 1;
            if (vol & 0xe0) vol = 0;
            rhythm->volumes[cmd&0x7] = vol;
          }
          fmp_part_pan_vol_rhythm(work, rhythm, cmd&0x7);
          break;
        }
      }
    } else {
      work->opna_writereg(work, 0x10, cmd & rhythm->mask);
      if (fmp->pdzf.mode == 2) {
        uint8_t pdzf_keyon = (cmd & 0x3f) & rhythm->mask;
        bool do_keyon = false;
        int p;
        for (p = 0; p < 6; p++) {
          if ((pdzf_keyon & (1<<p)) && (!rhythm->pans[p]) && fmp->pdzf.rhythm[p&1].enabled) {
            do_keyon = true;
            break;
          }
        }
        if (do_keyon) {
          if (work->ppz8_functbl) {
            uint8_t volume = rhythm->volumes[p] & 0x0f;
            uint8_t voice = fmp->pdzf.rhythm[p&1].voice[((rhythm->volumes[p] & 0x10) >> 4)];
            uint8_t pan = fmp->pdzf.rhythm[p&1].pan + 5;
            uint32_t freq = fmp_ppz8_note_freq(fmp->pdzf.rhythm[p&1].note);
            fmp->pdzf.rhythm_current_note = fmp->pdzf.rhythm[p&1].note;
            uint8_t channel = 6;
            work->ppz8_functbl->channel_play(
              work->ppz8,
              channel,
              voice
            );
            work->ppz8_functbl->channel_volume(
              work->ppz8,
              channel,
              volume
            );
            work->ppz8_functbl->channel_pan(
              work->ppz8,
              channel,
              pan
            );
            work->ppz8_functbl->channel_freq(
              work->ppz8,
              channel,
              freq
            );
          }
        }
      }
      rhythm->len_cnt = (cmd & 0x40) ?
                        rhythm->default_len : fmp_part_cmdload(fmp, part);
      return;
    }
  }
}

static uint8_t fmp_note2key(uint8_t note) {
  uint8_t octave = note / 0xc;
  uint8_t key = note % 0xc;
  key |= octave << 4;
  return key;
}

static const uint8_t fmp_track_map[FMDRIVER_TRACK_NUM] = {
  FMP_PART_FM_1,
  FMP_PART_FM_2,
  FMP_PART_FM_3,
  FMP_PART_FM_EX1,
  FMP_PART_FM_EX2,
  FMP_PART_FM_EX3,
  FMP_PART_FM_4,
  FMP_PART_FM_5,
  FMP_PART_FM_6,
  FMP_PART_SSG_1,
  FMP_PART_SSG_2,
  FMP_PART_SSG_3,
  FMP_PART_ADPCM,
  FMP_PART_PPZ8_1,
  FMP_PART_PPZ8_2,
  FMP_PART_PPZ8_3,
  FMP_PART_PPZ8_4,
  FMP_PART_PPZ8_5,
  FMP_PART_PPZ8_6,
  FMP_PART_PPZ8_7,
  FMP_PART_PPZ8_8,
};

static void fmp_work_status_init(struct fmdriver_work *work,
                                 const struct driver_fmp *fmp) {
  for (int t = 0; t < FMDRIVER_TRACK_NUM; t++) {
    struct fmdriver_track_status *track = &work->track_status[t];
    const struct fmp_part *part = &fmp->parts[fmp_track_map[t]];
    track->playing = !part->status.off;
    track->info = FMDRIVER_TRACK_INFO_NORMAL;
  }
  for (int i = 0; i < 3; i++) {
    work->track_status[FMDRIVER_TRACK_SSG_1+i].ppz8_ch = 1+i;
    work->track_status[FMDRIVER_TRACK_FM_3_EX_1+i].ppz8_ch = 4+i;
  }
}

static void fmp_work_status_update(struct fmdriver_work *work,
                                   const struct driver_fmp *fmp) {
  work->ssg_noise_freq = fmp->ssg_noise_freq;
  for (int t = 0; t < FMDRIVER_TRACK_PPZ8_1; t++) {
    struct fmdriver_track_status *track = &work->track_status[t];
    const struct fmp_part *part = &fmp->parts[fmp_track_map[t]];
    track->playing = !part->status.off;
    track->info = FMDRIVER_TRACK_INFO_NORMAL;
    track->ticks = part->status.off ? 0 : part->tonelen-1;
    track->ticks_left = part->tonelen_cnt;
    track->key = part->status.rest ? 0xff : fmp_note2key(part->prev_note);
    track->tonenum = part->tone;
    track->detune = part->detune - ((part->detune & 0x8000) ? 0x10000 : 0);
    track->status[0] = part->lfo_f.p ? 'P' : '-';
    track->status[1] = part->lfo_f.q ? 'Q' : '-';
    track->status[2] = part->lfo_f.r ? 'R' : '-';
    track->status[3] = part->lfo_f.a ? 'A' : '-';
    track->status[4] = '-';
    track->status[5] = part->lfo_f.e ? 'e' : '-';
    track->status[6] = (part->type.fm && part->u.fm.hlfo_apms) ? 'H' : '-';
    track->status[7] = part->status.pitchbend ? 'P' : '-';
    if (part->type.adpcm) {
      track->actual_key = 0xff;
      track->volume = part->actual_vol;
    } else if (part->type.ssg) {
      struct fmdriver_track_status *ppztrack =
          &work->track_status[FMDRIVER_TRACK_PPZ8_1-1+track->ppz8_ch];
      ppztrack->actual_key = 0xff;
      if (part->pdzf.mode || part->u.ssg.env_f.ppz) {
        ppztrack->playing = !part->status.off;
        ppztrack->key = track->key;
        ppztrack->tonenum = track->tonenum;
        ppztrack->info = FMDRIVER_TRACK_INFO_NORMAL;
      } else {
        ppztrack->playing = false;
        ppztrack->key = 0xff;
      }
      if (part->pdzf.mode) {
        track->info = FMDRIVER_TRACK_INFO_PDZF;
        track->actual_key = part->status.rest ? 0xff : fmdriver_ppz8_freq2key(part->pdzf.lastfreq);
        ppztrack->actual_key = track->actual_key;
      } else if (part->u.ssg.env_f.ppz) {
        track->info = FMDRIVER_TRACK_INFO_PPZ8;
        track->actual_key = 0xff;
      } else {
        track->info = FMDRIVER_TRACK_INFO_SSG;
        track->actual_key = part->status.rest ? 0xff : fmdriver_ssg_freq2key(part->prev_freq);
        track->ssg_tone = !((fmp->ssg_mix >> part->opna_keyon_out) & 1);
        track->ssg_noise = !((fmp->ssg_mix >> part->opna_keyon_out) & 8);
      }
      track->volume = part->current_vol - 1;
    } else {
      if (part->pdzf.mode) {
        struct fmdriver_track_status *ppztrack =
            &work->track_status[FMDRIVER_TRACK_PPZ8_1-1+track->ppz8_ch];
        track->info = FMDRIVER_TRACK_INFO_PDZF;
        track->actual_key = part->status.rest ? 0xff : fmdriver_ppz8_freq2key(part->pdzf.lastfreq);
        ppztrack->actual_key = track->actual_key;
        ppztrack->playing = !part->status.off;
        ppztrack->key = track->key;
        ppztrack->tonenum = work->ppz8 ? work->ppz8->channel[track->ppz8_ch-1].voice : 0;
        ppztrack->info = FMDRIVER_TRACK_INFO_NORMAL;
      } else {
        if (part->u.fm.slot_mask & 0xf0) {
          track->info = FMDRIVER_TRACK_INFO_FM3EX;
          for (int s = 0; s < 4; s++) {
            track->fmslotmask[s] = part->u.fm.slot_mask & (1<<(4+s));
          }
        }
        track->actual_key = part->status.rest ? 0xff : fmdriver_fm_freq2key(part->prev_freq);
      }
      track->volume = 0x7f - part->actual_vol;
    }
  }
  work->track_status[FMDRIVER_TRACK_PPZ8_7].playing = fmp->pdzf.rhythm[0].enabled || fmp->pdzf.rhythm[1].enabled;
  work->track_status[FMDRIVER_TRACK_PPZ8_7].tonenum = work->ppz8 ? work->ppz8->channel[6].voice : 0;
  uint8_t key = 0xff;
  if (work->ppz8 && work->ppz8->channel[6].playing) key = fmp->pdzf.rhythm_current_note;
  work->track_status[FMDRIVER_TRACK_PPZ8_7].key = key;
  work->track_status[FMDRIVER_TRACK_PPZ8_7].actual_key = key;
}

static void fmp_part_pdzf_freq_update(
  struct fmdriver_work *work,
  struct fmp_part *part
) {
  uint32_t newfreq = fmp_pdzf_extended_freq(part);
  if (newfreq != part->pdzf.lastfreq) {
    part->pdzf.lastfreq = newfreq;
    if (work->ppz8_functbl) {
      work->ppz8_functbl->channel_freq(
        work->ppz8,
        part->pdzf.ppz8_channel,
        newfreq
      );
    }
  }
}

// 17f8-1903
static void fmp_timerb(struct fmdriver_work *work, struct driver_fmp *fmp) {
  // 1805
  fmp_set_tempo(work, fmp);

  // 1813
  if (fmp->status.stopped) {
    // TODO: stopped
    // jmp 18c7
    work->playing = false;
  }
  // 1829
  if (!--fmp->clock_divider) {
    fmp->total_clocks++;
    fmp->clock_divider = 10;
  }
  // 1840
  for (int p = 0; p < 6; p++) {
    struct fmp_part *part = &fmp->parts[FMP_PART_FM_1+p];
    if (part->status.off) continue;
    fmp_part_cmd(work, fmp, part);
    fmp_part_fm(work, fmp, part);
  }
  {
    struct fmp_part *part = &fmp->parts[FMP_PART_ADPCM];
    if (!part->status.off) {
      fmp_part_cmd(work, fmp, part);
      fmp_part_adpcm(work, fmp, part);
    }
  }
  for (int p = 0; p < 3; p++) {
    struct fmp_part *part = &fmp->parts[FMP_PART_SSG_1+p];
    if (part->status.off) continue;
    fmp_part_cmd(work, fmp, part);
    fmp_part_ssg(work, fmp, part);
    if (part->pdzf.mode == 2) {
      fmp_part_pdzf_freq_update(work, part);
    }
    if (part->pdzf.mode && part->lfo_f.q) {
      fmp_part_pdzf_env(work, fmp, part);
    }
  }
  // 187d
  for (int p = 0; p < 3; p++) {
    struct fmp_part *part = &fmp->parts[FMP_PART_FM_EX1+p];
    if (part->status.off) continue;
    fmp_part_cmd(work, fmp, part);
    if (part->status.off) continue;
    fmp_part_fm(work, fmp, part);
    if (part->pdzf.mode == 2) {
      fmp_part_pdzf_freq_update(work, part);
    }
    if (part->pdzf.mode && part->lfo_f.q) {
      fmp_part_pdzf_env(work, fmp, part);
    }
  }
  if (!fmp->rhythm.status) {
    fmp_part_cmd_rhythm(work, fmp);
  }
  fmp_work_status_update(work, fmp);
}

static void fmp_init_parts(struct fmdriver_work *work,
                           struct driver_fmp *fmp) {
  const uint8_t deflen = fmp->datainfo.bar >> 2;
  // 3ae7
  // FM1, 2, 3, SSG1, 2, 3
  for (int i = 0; i < 3; i++) {
    struct fmp_part *fpart = &fmp->parts[FMP_PART_FM_1+i];
    struct fmp_part *spart = &fmp->parts[FMP_PART_SSG_1+i];
    spart->default_len = deflen;
    fpart->default_len = deflen;
    spart->gate_cmp = 1;
    fpart->gate_cmp = 1;
    spart->gate_cnt = 8;
    fpart->gate_cnt = 8;
    spart->u.ssg.env.startvol = 0xff;
    spart->u.ssg.env.attack_rate = 0xff;
    spart->u.ssg.env.decay_rate = 0xff;
    spart->u.ssg.env.sustain_lv = 0xff;
    spart->u.ssg.env.release_rate = 0x0a;
    // not in original
    spart->u.ssg.envbak.startvol = 0xff;
    spart->u.ssg.envbak.attack_rate = 0xff;
    spart->u.ssg.envbak.decay_rate = 0xff;
    spart->u.ssg.envbak.sustain_lv = 0xff;
    spart->u.ssg.envbak.release_rate = 0x0a;
    // end
    fpart->current_vol = 0x1a;
    fpart->actual_vol = 0x1a;
    spart->current_vol = 0x0e;
  }
  // FM4, 5, 6, ADPCM
  for (int i = 0; i < 4; i++) {
    struct fmp_part *part = &fmp->parts[FMP_PART_FM_4+i];
    part->default_len = deflen;
    part->gate_cmp = 1;
    part->gate_cnt = 8;
    part->current_vol = 0x1a;
    part->actual_vol = 0x1a;
  }
  {
    struct fmp_part *part = &fmp->parts[FMP_PART_ADPCM];
    part->default_len = deflen;
    part->gate_cmp = 1;
    part->gate_cnt = 8;
    part->current_vol = 0x1a;
    part->actual_vol = 0x1a;
  }
  // FMEX1, 2, 3
  for (int i = 0; i < 3; i++) {
    struct fmp_part *part = &fmp->parts[FMP_PART_FM_EX1+i];
    part->default_len = deflen;
    part->gate_cmp = 1;
    part->gate_cnt = 8;
    part->current_vol = 0x1a;
    part->actual_vol = 0x1a;
    part->u.fm.slot_mask = 0xff;
    part->pan_ams_pms = 0xc0;
  }
  // 3b5f
  fmp->rhythm.status = false;
  fmp->rhythm.len_cnt = 1;
  fmp->rhythm.default_len = deflen;
  //fmp->rhythm.loop_now
  fmp->rhythm.mask = 0x3f;
  fmp->rhythm.tl_volume = 0x3c;
  for (int i = 0; i < 6; i++) {
    fmp->rhythm.volumes[i] = 0x1c;
    fmp->rhythm.pans[i] = 0xc0;
  }
  // couldn't find where this is written,
  // but this must be necessary
  work->opna_writereg(work, 0x11, fmp->rhythm.tl_volume);
  for (int i = 0; i < 6; i++) {
    fmp_part_pan_vol_rhythm(work, &fmp->rhythm, i);
  }
  
  {
    // 3b86
    struct fmp_part *part = &fmp->parts[FMP_PART_ADPCM];
    part->current_vol = 0x80;
    part->actual_vol = 0x80;
    part->gate_cmp = 0;
    work->opna_writereg(work, 0x10b, 0x80);
    fmp_adpcm_pan(work, fmp, part, 0xc0);
  }
  // opna flag mask, reset
  // work->opna_writereg(work, 0x110, 0x1c);
  // work->opna_writereg(work, 0x110, 0x80);
  
  fmp->timerb = 0xca;
  work->timerb = fmp->timerb;
  fmp->timerb_bak = 0xca;

  // 3c79
  for (int i = 0; i < 6; i++) {
    fmp->parts[FMP_PART_FM_1+i].current_ptr
      = fmp->datainfo.partptr[FMP_DATA_FM_1+i];
    fmp->parts[FMP_PART_FM_1+i].loop_ptr
      = fmp->datainfo.loopptr[FMP_DATA_FM_1+i];
  }
  // 3ca3
  for (int i = 0; i < 3; i++) {
    fmp->parts[FMP_PART_SSG_1+i].current_ptr
      = fmp->datainfo.partptr[FMP_DATA_SSG_1+i];
    fmp->parts[FMP_PART_SSG_1+i].loop_ptr
      = fmp->datainfo.loopptr[FMP_DATA_SSG_1+i];
  }
  fmp->rhythm.part.current_ptr
    = fmp->datainfo.partptr[FMP_DATA_RHYTHM];
  fmp->rhythm.part.loop_ptr
    = fmp->datainfo.loopptr[FMP_DATA_RHYTHM];
    
  fmp->parts[FMP_PART_ADPCM].current_ptr
    = fmp->datainfo.partptr[FMP_DATA_ADPCM];
  fmp->parts[FMP_PART_ADPCM].loop_ptr
    = fmp->datainfo.loopptr[FMP_DATA_ADPCM];

  // 3d06
  for (int i = 0; i < 3; i++) {
    fmp->parts[FMP_PART_FM_EX1+i].current_ptr
      = fmp->datainfo.partptr[FMP_DATA_FM_EX1+i];
    fmp->parts[FMP_PART_FM_EX1+i].loop_ptr
      = fmp->datainfo.loopptr[FMP_DATA_FM_EX1+i];
  }

  // 3d2d
  fmp->part_playing_bit = 0x07ff;
  fmp->part_loop_bit = 0x07ff;
  // TODO: other status bit
  fmp->status.looped = false;
  fmp->status.stopped = false;
  fmp->total_clocks = 0;
  fmp->clock_divider = 10;
  //  1b64
  
  // 3c36
  /*
  if (work->ppz8_functbl) {
    work->ppz8_functbl->total_volume(work->ppz8, 
  }
  */
  fmp_fm_pdzf_mode_update(work, fmp, &fmp->parts[FMP_PART_FM_EX1]);
  fmp_fm_pdzf_mode_update(work, fmp, &fmp->parts[FMP_PART_FM_EX2]);
  fmp_fm_pdzf_mode_update(work, fmp, &fmp->parts[FMP_PART_FM_EX3]);
  fmp_ssg_ppz8_pdzf_mode_update(work, fmp, &fmp->parts[FMP_PART_SSG_1]);
  fmp_ssg_ppz8_pdzf_mode_update(work, fmp, &fmp->parts[FMP_PART_SSG_2]);
  fmp_ssg_ppz8_pdzf_mode_update(work, fmp, &fmp->parts[FMP_PART_SSG_3]);
}

static void fmp_struct_init(struct fmdriver_work *work,
                            struct driver_fmp *fmp) {
  // TODO
  //fmp->pdzf.mode = 2;
  // 4e87
  fmp->ssg_mix = 0x38;
  // 3bb7
  work->opna_writereg(work, 0x07, fmp->ssg_mix);

  // 5373
  // enable OPNA 4-6
  work->opna_writereg(work, 0x29, 0x83);
  // stop rhythm
  work->opna_writereg(work, 0x10, 0xbf);
  // reset ADPCM
  work->opna_writereg(work, 0x100, 0x21);
  work->opna_writereg(work, 0x101, 0x02);
  //
  fmp->adpcm_c1 = 0x02;

  // 5394
  // on OPNA, increase volume by 8
  // because OPNA's FM output is 6db lower than OPN
  fmp->fm_vol = -8;

  // 53ca
  // TODO: part at 0x1426
  // 5408
  fmp->parts[FMP_PART_SSG_1].type.ssg = true;
  fmp->parts[FMP_PART_SSG_2].type.ssg = true;
  fmp->parts[FMP_PART_SSG_3].type.ssg = true;
  fmp->parts[FMP_PART_SSG_1].opna_keyon_out = 0;
  fmp->parts[FMP_PART_SSG_2].opna_keyon_out = 1;
  fmp->parts[FMP_PART_SSG_3].opna_keyon_out = 2;
  fmp->parts[FMP_PART_SSG_1].part_bit = 0x0040;
  fmp->parts[FMP_PART_SSG_2].part_bit = 0x0080;
  fmp->parts[FMP_PART_SSG_3].part_bit = 0x0100;
  // 5479
  fmp->parts[FMP_PART_FM_1].type.fm = true;
  fmp->parts[FMP_PART_FM_2].type.fm = true;
  fmp->parts[FMP_PART_FM_3].type.fm = true;
  fmp->parts[FMP_PART_FM_3].type.fm_3 = true;
  fmp->parts[FMP_PART_FM_4].type.fm = true;
  fmp->parts[FMP_PART_FM_5].type.fm = true;
  fmp->parts[FMP_PART_FM_6].type.fm = true;
  fmp->parts[FMP_PART_FM_1].opna_keyon_out = 0x00;
  fmp->parts[FMP_PART_FM_2].opna_keyon_out = 0x01;
  fmp->parts[FMP_PART_FM_3].opna_keyon_out = 0x02;
  fmp->parts[FMP_PART_FM_4].opna_keyon_out = 0x04;
  fmp->parts[FMP_PART_FM_5].opna_keyon_out = 0x05;
  fmp->parts[FMP_PART_FM_6].opna_keyon_out = 0x06;
  fmp->parts[FMP_PART_FM_1].part_bit = 0x0001;
  fmp->parts[FMP_PART_FM_2].part_bit = 0x0002;
  fmp->parts[FMP_PART_FM_3].part_bit = 0x0004;
  fmp->parts[FMP_PART_FM_4].part_bit = 0x0008;
  fmp->parts[FMP_PART_FM_5].part_bit = 0x0010;
  fmp->parts[FMP_PART_FM_6].part_bit = 0x0020;
  // 5502
  fmp->parts[FMP_PART_ADPCM].type.adpcm = true;
  fmp->parts[FMP_PART_ADPCM].part_bit = 0x400;
  fmp->parts[FMP_PART_FM_EX1].type.fm = true;
  fmp->parts[FMP_PART_FM_EX1].type.fm_3 = true;
  fmp->parts[FMP_PART_FM_EX2].type.fm = true;
  fmp->parts[FMP_PART_FM_EX2].type.fm_3 = true;
  fmp->parts[FMP_PART_FM_EX3].type.fm = true;
  fmp->parts[FMP_PART_FM_EX3].type.fm_3 = true;
  fmp->parts[FMP_PART_FM_EX1].opna_keyon_out = 0x02;
  fmp->parts[FMP_PART_FM_EX2].opna_keyon_out = 0x02;
  fmp->parts[FMP_PART_FM_EX3].opna_keyon_out = 0x02;
  fmp->parts[FMP_PART_FM_EX1].part_bit = 0x0800;
  fmp->parts[FMP_PART_FM_EX2].part_bit = 0x1000;
  fmp->parts[FMP_PART_FM_EX3].part_bit = 0x2000;
  fmp->rhythm.part.type.rhythm = true;
  fmp->rhythm.part.part_bit = 0x0200;
  // 5625
  work->opna_writereg(work, 0x27, 0x30);

  // pdzf
  fmp->parts[FMP_PART_SSG_1].pdzf.ppz8_channel = 0;
  fmp->parts[FMP_PART_SSG_1].pdzf.loopstart32 = -1;
  fmp->parts[FMP_PART_SSG_1].pdzf.loopend32 = -1;
  fmp->parts[FMP_PART_SSG_2].pdzf.ppz8_channel = 1;
  fmp->parts[FMP_PART_SSG_2].pdzf.loopstart32 = -1;
  fmp->parts[FMP_PART_SSG_2].pdzf.loopend32 = -1;
  fmp->parts[FMP_PART_SSG_3].pdzf.ppz8_channel = 2;
  fmp->parts[FMP_PART_SSG_3].pdzf.loopstart32 = -1;
  fmp->parts[FMP_PART_SSG_3].pdzf.loopend32 = -1;
  fmp->parts[FMP_PART_FM_EX1].pdzf.ppz8_channel = 3;
  fmp->parts[FMP_PART_FM_EX1].pdzf.loopstart32 = -1;
  fmp->parts[FMP_PART_FM_EX1].pdzf.loopend32 = -1;
  fmp->parts[FMP_PART_FM_EX2].pdzf.ppz8_channel = 4;
  fmp->parts[FMP_PART_FM_EX2].pdzf.loopstart32 = -1;
  fmp->parts[FMP_PART_FM_EX2].pdzf.loopend32 = -1;
  fmp->parts[FMP_PART_FM_EX3].pdzf.ppz8_channel = 5;
  fmp->parts[FMP_PART_FM_EX3].pdzf.loopstart32 = -1;
  fmp->parts[FMP_PART_FM_EX3].pdzf.loopend32 = -1;
  
  // ppz8 unused parts
  for (int i = 0; i < 8; i++) {
    fmp->parts[FMP_PART_PPZ8_1+i].status.off = true;
  }
}

// 1774
static void fmp_opna_interrupt(struct fmdriver_work *work) {
  struct driver_fmp *fmp = (struct driver_fmp *)work->driver;
  if (work->opna_status(work, 0) & 0x02) {
    fmp_timerb(work, fmp);
    if (work->playing) {
      work->timerb_cnt++;
      work->timerb_cnt_loop++;
    }
  }
}

static void fmp_title(
    struct fmdriver_work *work,
    struct driver_fmp *fmp,
    uint16_t offset) {
  (void)work;
  int l = 0;
  int li = 0;
  for (int si = 0;; si++) {
    if ((offset + si) >= fmp->datalen) {
      if (l < 3) fmp->comment[l][0] = 0;
      return;
    }
    if (li >= FMP_COMMENT_BUFLEN) {
      if (l < 3) fmp->comment[l][0] = 0;
      return;
    }
    uint8_t c = fmp->data[offset+si];
    if (l >= 3) {
      // Z8X
      if (c) fmp->pdzf.mode = 1;
      return;
    }
    if (c == '\r') {
      continue;
    } else if (c == '\n') {
      fmp->comment[l][li] = 0;
      li = 0;
      l++;
    } else {
      fmp->comment[l][li] = c;
      if (!c) return;
      li++;
    }
  }
}

static void fmp_check_pdzf(struct driver_fmp *fmp) {
  for (int l = 0; l < 3; l++) {
    if (strstr((const char *)fmp->comment[l], "using PDZF")) fmp->pdzf.mode = 2;
  }
}

/*
// copy title string (CP932) to fmdriver_work struct,
// and detect which PDZF(/Z8X) mode to use
static void fmp_title(struct fmdriver_work *work,
                      struct driver_fmp *fmp,
                      uint16_t offset) {
  int l = 0;
  int i = 0;
  static const uint8_t pdzf_str[] = "using PDZF";
  const uint8_t *data = fmp->data;
  uint16_t datalen = fmp->datalen;
  const uint8_t *pdzf_ptr = pdzf_str;
  fmp->pdzf.mode = 0;
  enum {
    STATE_NORMAL,
    STATE_ESC,
    STATE_CSI,
    STATE_SYNC,
  } esc_state = STATE_NORMAL;
  for (int si = 0; ; si++) {
    if ((offset + i) >= datalen) {
      work->comment[l][0] = 0;
      return;
    }
    if (i > (FMDRIVER_TITLE_BUFLEN-1)) {
      return;
    }
    uint8_t c = data[offset+si];
    
    if (l >= 3) {
      if (c) {
        if (!fmp->pdzf.mode) {
          fmp->pdzf.mode = 1;
        }
      }
      return;
    }
    if (!c) return;
    switch (esc_state) {
    case STATE_SYNC:
      esc_state = STATE_NORMAL;
      continue;
    case STATE_ESC:
      if (c == '[') {
        esc_state = STATE_CSI;
      } else if (c == '!') {
        esc_state = STATE_SYNC;
      } else {
        esc_state = STATE_NORMAL;
      }
      continue;
    case STATE_CSI:
      if (('0' <= c && c <= '9') || c == ';') {
        continue;
      } else {
        esc_state = STATE_NORMAL;
        continue;
      }
    default:
      break;
    }
    // pdzf detection
    if (c == *pdzf_ptr++) {
      if (!*pdzf_ptr) {
        fmp->pdzf.mode = 2;
      }
    } else {
      pdzf_ptr = pdzf_str;
    }

    work->comment[l][i] = c;
    switch (c) {
    case 0:
      return;
    case '\r':
      break;
    case '\n':
      work->comment[l][i] = 0;
      l++;
      i = 0;
      break;
    case 0x1b:
      esc_state = STATE_ESC;
      break;
    default:
      i++;
      break;
    }
  }
}
*/

bool fmp_load(struct driver_fmp *fmp,
              uint8_t *data, uint16_t datalen)
{
  uint16_t offset = read16le(data);
  if ((offset+4) > datalen) {
    FMDRIVER_DEBUG("invalid offset value\n");
    return false;
  }
  uint8_t dataflags;
  bool pviname_valid;
  if ((data[offset] == 'F') && (data[offset+1] == 'M') &&
      (data[offset+2] == 'C')) {
    FMDRIVER_DEBUG("FMP data\n");
    uint8_t dataver = data[offset+3];
    fmp->data_version = dataver & 0x0f;
    if ((dataver & 0xf) >= 0x8) {
      FMDRIVER_DEBUG("call word 4152\n");
    }
    if (dataver <= 0x29) {
      FMDRIVER_DEBUG("format:1\n");
      if (datalen < 0x1au) {
        FMDRIVER_DEBUG("file length shorter than header\n");
        return false;
      }
      fmp->datainfo.partptr[FMP_DATA_FM_1] = read16le(&data[0x02]);
      fmp->datainfo.partptr[FMP_DATA_FM_2] = read16le(&data[0x04]);
      fmp->datainfo.partptr[FMP_DATA_FM_3] = read16le(&data[0x06]);
      fmp->datainfo.partptr[FMP_DATA_FM_4] = 0xffff;
      fmp->datainfo.partptr[FMP_DATA_FM_5] = 0xffff;
      fmp->datainfo.partptr[FMP_DATA_FM_6] = 0xffff;
      fmp->datainfo.partptr[FMP_DATA_SSG_1] = read16le(&data[0x08]);
      fmp->datainfo.partptr[FMP_DATA_SSG_2] = read16le(&data[0x0a]);
      fmp->datainfo.partptr[FMP_DATA_SSG_3] = read16le(&data[0x0c]);
      fmp->datainfo.partptr[FMP_DATA_RHYTHM] = 0xffff;
      fmp->datainfo.partptr[FMP_DATA_ADPCM] = 0xffff;
      fmp->datainfo.partptr[FMP_DATA_FM_EX1] = 0xffff;
      fmp->datainfo.partptr[FMP_DATA_FM_EX2] = 0xffff;
      fmp->datainfo.partptr[FMP_DATA_FM_EX3] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_FM_1] = read16le(&data[0x0e]);
      fmp->datainfo.loopptr[FMP_DATA_FM_2] = read16le(&data[0x10]);
      fmp->datainfo.loopptr[FMP_DATA_FM_3] = read16le(&data[0x12]);
      fmp->datainfo.loopptr[FMP_DATA_FM_4] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_FM_5] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_FM_6] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_SSG_1] = read16le(&data[0x14]);
      fmp->datainfo.loopptr[FMP_DATA_SSG_2] = read16le(&data[0x16]);
      fmp->datainfo.loopptr[FMP_DATA_SSG_3] = read16le(&data[0x18]);
      fmp->datainfo.loopptr[FMP_DATA_RHYTHM] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_ADPCM] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_FM_EX1] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_FM_EX2] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_FM_EX3] = 0xffff;
      fmp->datainfo.bar = data[0x1a];
      dataflags = data[0x1b];
      fmp->datainfo.adpcmptr = 0xffff;
      fmp->datainfo.fmtoneptr = 0x1c;
      pviname_valid = false;
    } else if (dataver <= 0x49) {
      FMDRIVER_DEBUG("format:2\n");
      if (datalen < 0x2eu) {
        FMDRIVER_DEBUG("file length shorter than header\n");
        return false;
      }
      fmp->datainfo.partptr[FMP_DATA_FM_1] = read16le(&data[0x02]);
      fmp->datainfo.partptr[FMP_DATA_FM_2] = read16le(&data[0x04]);
      fmp->datainfo.partptr[FMP_DATA_FM_3] = read16le(&data[0x06]);
      fmp->datainfo.partptr[FMP_DATA_FM_4] = read16le(&data[0x08]);
      fmp->datainfo.partptr[FMP_DATA_FM_5] = read16le(&data[0x0a]);
      fmp->datainfo.partptr[FMP_DATA_FM_6] = read16le(&data[0x0c]);
      fmp->datainfo.partptr[FMP_DATA_SSG_1] = read16le(&data[0x0e]);
      fmp->datainfo.partptr[FMP_DATA_SSG_2] = read16le(&data[0x10]);
      fmp->datainfo.partptr[FMP_DATA_SSG_3] = read16le(&data[0x12]);
      fmp->datainfo.partptr[FMP_DATA_RHYTHM] = read16le(&data[0x14]);
      fmp->datainfo.partptr[FMP_DATA_ADPCM] = read16le(&data[0x16]);
      fmp->datainfo.partptr[FMP_DATA_FM_EX1] = 0xffff;
      fmp->datainfo.partptr[FMP_DATA_FM_EX2] = 0xffff;
      fmp->datainfo.partptr[FMP_DATA_FM_EX3] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_FM_1] = read16le(&data[0x18]);
      fmp->datainfo.loopptr[FMP_DATA_FM_2] = read16le(&data[0x1a]);
      fmp->datainfo.loopptr[FMP_DATA_FM_3] = read16le(&data[0x1c]);
      fmp->datainfo.loopptr[FMP_DATA_FM_4] = read16le(&data[0x1e]);
      fmp->datainfo.loopptr[FMP_DATA_FM_5] = read16le(&data[0x20]);
      fmp->datainfo.loopptr[FMP_DATA_FM_6] = read16le(&data[0x22]);
      fmp->datainfo.loopptr[FMP_DATA_SSG_1] = read16le(&data[0x24]);
      fmp->datainfo.loopptr[FMP_DATA_SSG_2] = read16le(&data[0x26]);
      fmp->datainfo.loopptr[FMP_DATA_SSG_3] = read16le(&data[0x28]);
      fmp->datainfo.loopptr[FMP_DATA_RHYTHM] = read16le(&data[0x2a]);
      fmp->datainfo.loopptr[FMP_DATA_ADPCM] = read16le(&data[0x2c]);
      fmp->datainfo.loopptr[FMP_DATA_FM_EX1] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_FM_EX2] = 0xffff;
      fmp->datainfo.loopptr[FMP_DATA_FM_EX3] = 0xffff;
      fmp->datainfo.bar = data[0x2e];
      dataflags = data[0x2f];
      fmp->datainfo.adpcmptr = read16le(&data[0x30]);
      fmp->datainfo.fmtoneptr = 0x32;
      pviname_valid = true;
    } else if (dataver <= 0x69) {
      FMDRIVER_DEBUG("format:3\n");
      if (datalen < 0x5eu) {
        FMDRIVER_DEBUG("file length shorter than header\n");
        return false;
      }
      fmp->datainfo.partptr[FMP_DATA_FM_1] = read16le(&data[0x02]);
      fmp->datainfo.partptr[FMP_DATA_FM_2] = read16le(&data[0x04]);
      fmp->datainfo.partptr[FMP_DATA_FM_3] = read16le(&data[0x06]);
      fmp->datainfo.partptr[FMP_DATA_FM_4] = read16le(&data[0x08]);
      fmp->datainfo.partptr[FMP_DATA_FM_5] = read16le(&data[0x0a]);
      fmp->datainfo.partptr[FMP_DATA_FM_6] = read16le(&data[0x0c]);
      fmp->datainfo.partptr[FMP_DATA_SSG_1] = read16le(&data[0x0e]);
      fmp->datainfo.partptr[FMP_DATA_SSG_2] = read16le(&data[0x10]);
      fmp->datainfo.partptr[FMP_DATA_SSG_3] = read16le(&data[0x12]);
      fmp->datainfo.partptr[FMP_DATA_RHYTHM] = read16le(&data[0x14]);
      fmp->datainfo.partptr[FMP_DATA_ADPCM] = read16le(&data[0x16]);
      fmp->datainfo.partptr[FMP_DATA_FM_EX1] = read16le(&data[0x18]);
      fmp->datainfo.partptr[FMP_DATA_FM_EX2] = read16le(&data[0x1a]);
      fmp->datainfo.partptr[FMP_DATA_FM_EX3] = read16le(&data[0x1c]);
      fmp->datainfo.loopptr[FMP_DATA_FM_1] = read16le(&data[0x30]);
      fmp->datainfo.loopptr[FMP_DATA_FM_2] = read16le(&data[0x32]);
      fmp->datainfo.loopptr[FMP_DATA_FM_3] = read16le(&data[0x34]);
      fmp->datainfo.loopptr[FMP_DATA_FM_4] = read16le(&data[0x36]);
      fmp->datainfo.loopptr[FMP_DATA_FM_5] = read16le(&data[0x38]);
      fmp->datainfo.loopptr[FMP_DATA_FM_6] = read16le(&data[0x3a]);
      fmp->datainfo.loopptr[FMP_DATA_SSG_1] = read16le(&data[0x3c]);
      fmp->datainfo.loopptr[FMP_DATA_SSG_2] = read16le(&data[0x3e]);
      fmp->datainfo.loopptr[FMP_DATA_SSG_3] = read16le(&data[0x40]);
      fmp->datainfo.loopptr[FMP_DATA_RHYTHM] = read16le(&data[0x42]);
      fmp->datainfo.loopptr[FMP_DATA_ADPCM] = read16le(&data[0x44]);
      fmp->datainfo.loopptr[FMP_DATA_FM_EX1] = read16le(&data[0x46]);
      fmp->datainfo.loopptr[FMP_DATA_FM_EX2] = read16le(&data[0x48]);
      fmp->datainfo.loopptr[FMP_DATA_FM_EX3] = read16le(&data[0x4a]);
      fmp->datainfo.bar = data[0x5e];
      dataflags = data[0x5f];
      fmp->datainfo.adpcmptr = read16le(&data[0x60]);
      fmp->datainfo.fmtoneptr = 0x66;
      pviname_valid = true;
    } else {
      FMDRIVER_DEBUG("invalid format information\n");
      return false;
    }
  } else if ((data[offset] == 'E') && (data[offset+1] == 'L') &&
             (data[offset+2] == 'F')) {
    FMDRIVER_DEBUG("PLAY6 data\n");
    if (datalen < 0x2au) {
      FMDRIVER_DEBUG("file length shorter than header\n");
      return false;
    }
    fmp->data_version = 0x07;
    fmp->datainfo.partptr[FMP_DATA_FM_1] = read16le(&data[0x02]);
    fmp->datainfo.partptr[FMP_DATA_FM_2] = read16le(&data[0x04]);
    fmp->datainfo.partptr[FMP_DATA_FM_3] = read16le(&data[0x06]);
    fmp->datainfo.partptr[FMP_DATA_FM_4] = read16le(&data[0x08]);
    fmp->datainfo.partptr[FMP_DATA_FM_5] = read16le(&data[0x0a]);
    fmp->datainfo.partptr[FMP_DATA_FM_6] = read16le(&data[0x0c]);
    fmp->datainfo.partptr[FMP_DATA_SSG_1] = read16le(&data[0x0e]);
    fmp->datainfo.partptr[FMP_DATA_SSG_2] = read16le(&data[0x10]);
    fmp->datainfo.partptr[FMP_DATA_SSG_3] = read16le(&data[0x12]);
    fmp->datainfo.partptr[FMP_DATA_RHYTHM] = read16le(&data[0x14]);
    fmp->datainfo.partptr[FMP_DATA_ADPCM] = 0xffff;
    fmp->datainfo.partptr[FMP_DATA_FM_EX1] = 0xffff;
    fmp->datainfo.partptr[FMP_DATA_FM_EX2] = 0xffff;
    fmp->datainfo.partptr[FMP_DATA_FM_EX3] = 0xffff;
    fmp->datainfo.loopptr[FMP_DATA_FM_1] = read16le(&data[0x16]);
    fmp->datainfo.loopptr[FMP_DATA_FM_2] = read16le(&data[0x18]);
    fmp->datainfo.loopptr[FMP_DATA_FM_3] = read16le(&data[0x1a]);
    fmp->datainfo.loopptr[FMP_DATA_FM_4] = read16le(&data[0x1c]);
    fmp->datainfo.loopptr[FMP_DATA_FM_5] = read16le(&data[0x1e]);
    fmp->datainfo.loopptr[FMP_DATA_FM_6] = read16le(&data[0x20]);
    fmp->datainfo.loopptr[FMP_DATA_SSG_1] = read16le(&data[0x22]);
    fmp->datainfo.loopptr[FMP_DATA_SSG_2] = read16le(&data[0x24]);
    fmp->datainfo.loopptr[FMP_DATA_SSG_3] = read16le(&data[0x26]);
    fmp->datainfo.loopptr[FMP_DATA_RHYTHM] = read16le(&data[0x28]);
    fmp->datainfo.loopptr[FMP_DATA_ADPCM] = 0xffff;
    fmp->datainfo.loopptr[FMP_DATA_FM_EX1] = 0xffff;
    fmp->datainfo.loopptr[FMP_DATA_FM_EX2] = 0xffff;
    fmp->datainfo.loopptr[FMP_DATA_FM_EX3] = 0xffff;
    fmp->datainfo.bar = data[0x2a];
    dataflags = data[0x2b];
    fmp->datainfo.adpcmptr = 0xffff;
    fmp->datainfo.fmtoneptr = 0x2e;
    pviname_valid = false;
  } else {
    return false;
  }
  fmp->datainfo.flags.q = dataflags & 0x01;
  fmp->datainfo.flags.ppz = dataflags & 0x02;
  fmp->datainfo.flags.lfo_octave_fix = dataflags & 0x04;
  {
    uint16_t ptr = read16le(&data[0x00]) - 2;
    fmp->datainfo.ssgtoneptr = ((ptr+2) > datalen) ? 0xffff : read16le(&data[ptr]);
  }
  // 3a70
  //zero reset part struct
  FMDRIVER_DEBUG("bar: %d\n", fmp->datainfo.bar);
  for (int i = 0; i < 6; i++) {
    FMDRIVER_DEBUG("  FM#%d:   %04X %04X\n",
            i+1, fmp->datainfo.partptr[i], fmp->datainfo.loopptr[i]);
  }
  for (int i = 0; i < 3; i++) {
    FMDRIVER_DEBUG("  SSG#%d:  %04X %04X\n",
            i+1,
            fmp->datainfo.partptr[FMP_DATA_SSG_1+i],
            fmp->datainfo.loopptr[FMP_DATA_SSG_1+i]);
  }
  FMDRIVER_DEBUG("  RHYTHM: %04X %04X\n",
          fmp->datainfo.partptr[FMP_DATA_RHYTHM],
          fmp->datainfo.loopptr[FMP_DATA_RHYTHM]);
  FMDRIVER_DEBUG("  ADPCM:  %04X %04X\n",
          fmp->datainfo.partptr[FMP_DATA_ADPCM],
          fmp->datainfo.loopptr[FMP_DATA_ADPCM]);
  for (int i = 0; i < 3; i++) {
    FMDRIVER_DEBUG("  FMEX#%d: %04X %04X\n",
            i+1,
            fmp->datainfo.partptr[FMP_DATA_FM_EX1+i],
            fmp->datainfo.loopptr[FMP_DATA_FM_EX1+i]);
  }
  FMDRIVER_DEBUG("  FMTONEPTR:  %04X\n", fmp->datainfo.fmtoneptr);
  FMDRIVER_DEBUG("  SSGTONEPTR: %04X\n", fmp->datainfo.ssgtoneptr);
  FMDRIVER_DEBUG("  data version: 0x%01X\n", fmp->data_version);
  uint16_t pcmptr = read16le(data)-0x12;
  if (pcmptr <= datalen && (pcmptr+16) < datalen) {
    for (int i = 0; i < 8; i++) {
      if (pviname_valid) {
        fmp->pvi_name[i] = data[pcmptr+8+i];
      }
      if (pviname_valid && fmp->datainfo.flags.ppz) {
        fmp->ppz_name[i] = data[pcmptr+0+i];
      }
    }
  }
  fmp->bar_tickcnt = fmp->datainfo.bar;
  fmp->data = data;
  fmp->datalen = datalen;
  return true;
}

static const char *fmp_get_comment(struct fmdriver_work *work, int line) {
  if (line < 0) return 0;
  if (line >= 3) return 0;
  struct driver_fmp *fmp = work->driver;
  if (!fmp->comment[line][0]) return 0;
  return (const char *)fmp->comment[line];
}

void fmp_init(struct fmdriver_work *work, struct driver_fmp *fmp) {
  fmp_title(work, fmp, read16le(fmp->data)+4);
  fmp_check_pdzf(fmp);
  work->comment_mode_pmd = false;
  work->get_comment = fmp_get_comment;
  fmp_struct_init(work, fmp);
  fmp_init_parts(work, fmp);
  uint16_t fmtoneptr = fmp->datainfo.fmtoneptr;
  (void)fmtoneptr;
  FMDRIVER_DEBUG(" 000 %03d %03d\n",
                 fmp->data[fmtoneptr+0x18]&0x7,
                 (fmp->data[fmtoneptr+0x18]>>3)&0x7
  );
  fmp_set_tempo(work, fmp);
  work->driver = fmp;
  work->driver_opna_interrupt = fmp_opna_interrupt;
  fmp_work_status_init(work, fmp);
  fmdriver_fillpcmname(work->pcmname[0], fmp->pvi_name);
  fmdriver_fillpcmname(work->pcmname[1], fmp->ppz_name);
  strcpy(work->pcmtype[0], "PVI");
  strcpy(work->pcmtype[1], "PPZ");
  work->playing = true;
}

// 4235
bool fmp_adpcm_load(struct fmdriver_work *work,
                    uint8_t *data, size_t datalen) {
  if (datalen < 0x210) return false;
  if (datalen > (0x210+(1<<18))) return false;
  struct driver_fmp *fmp = (struct driver_fmp *)work->driver;
  fmp->adpcm_deltat = read16le(&data[0x8]);
  // fmp->adpcm_c1 = data[0x0a];
  for (int i = 0; i < 0x80; i++) {
    fmp->adpcm_startaddr[i] = read16le(&data[0x10+i*4+0]);
    fmp->adpcm_endaddr[i] = read16le(&data[0x10+i*4+2]);
  }
  work->opna_writereg(work, 0x100, 0x01);
  work->opna_writereg(work, 0x110, 0x13);
  work->opna_writereg(work, 0x110, 0x80);
  work->opna_writereg(work, 0x100, 0x60);
  work->opna_writereg(work, 0x101, fmp->adpcm_c1);
  work->opna_writereg(work, 0x102, 0x00);
  work->opna_writereg(work, 0x103, 0x00);
  work->opna_writereg(work, 0x104, 0xff);
  work->opna_writereg(work, 0x105, 0xff);
  work->opna_writereg(work, 0x10c, 0xff);
  work->opna_writereg(work, 0x10d, 0xff);
  // 42ca
  for (uint32_t i = 0x210; i < datalen; i++) {
    work->opna_writereg(work, 0x108, data[i]);
  }
  work->opna_writereg(work, 0x110, 0x0c);
  work->opna_writereg(work, 0x100, 0x01);
  return true;
}
