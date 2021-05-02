/*
 * 2004-05-23 : Patched for GG Stereo by RuRuRu
 */
#include "kssplay.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAL_FREQ 50.0
#define NTSC_FREQ (3579545.0 / 59718.0) /* 59.94 */

static int32_t volume_table[1 << KSSPLAY_VOL_BITS];

static int mgs_pos = 0;
static char mgs_text[256];
static int mgs_text_update(VM *vm, uint32_t a, uint32_t d) {
  char buf[128] = "";
  int adr, i, j;

  adr = vm->IO[0x43] + (vm->IO[0x44] << 8);

  if (adr) {
    for (i = 0; i < 127; i++) {
      buf[i] = MMAP_read_memory(vm->mmap, adr + i);
      if (buf[i] == '\0') {
        break;
      }
    }
    buf[i] = '\0';

    for (i = 0; i < 127 && buf[i] != '\0'; i++) {
      switch (buf[i]) {
      case 0x2:
        mgs_pos = ((unsigned char)buf[++i]) % 80;
        break;

      case 0x3:
        mgs_pos = ((unsigned char)buf[++i]) % 80;
        for (j = 0; j < mgs_pos; j++)
          mgs_text[j] = ' ';
        break;

      case 0x1:
        mgs_pos = 0;
        break;

      default:
        mgs_text[mgs_pos++] = buf[i];
        break;
      }
    }

    mgs_text[mgs_pos] = '\0';
    mgs_text[80] = '\0';
  }

  return 0;
}

/*
 TBL:  0 --- 255,  256 --- 511
 VOL:  0 --- 255, -256 ---  -1
*/
static void make_voltable(void) {
  int i;
  for (i = KSSPLAY_VOL_MIN; i <= KSSPLAY_VOL_MAX; i++) {
    if (i >= 0) {
      volume_table[i] = (int32_t)((1 << 12) * pow(2, i / (6.0 / KSSPLAY_VOL_STEP)));
    } else {
      volume_table[KSSPLAY_MUTE + i] = (int32_t)((1 << 12) * pow(2, i / (6.0 / KSSPLAY_VOL_STEP)));
    }
  }
}

static inline int32_t apply_volume(int32_t data, int32_t vol) {
  return (data * volume_table[(uint32_t)(vol & (KSSPLAY_MUTE - 1))]) >> 12;
}

int KSSPLAY_set_data(KSSPLAY *kssplay, KSS *kss) {
  uint32_t logical_size, header_size;
  int i;

  if (kss->kssx && 0 < kss->extra_size) {
    header_size = 16 + kss->extra_size;
    for (i = 0; i < EDSC_MAX; i++) {
      kssplay->device_volume[i] = (kss->vol[i] << (KSSPLAY_VOL_BITS - 8));
    }
  } else {
    header_size = 16;
    for (i = 0; i < EDSC_MAX; i++) {
      kssplay->device_volume[i] = 0;
    }
  }

  /* Prevent buffer over run when KSS HEADER is inconsistent for its data size. */
  if (kss->bank_mode == BANK_16K) {
    logical_size = header_size + kss->load_len + kss->bank_num * 0x4000;
  } else {
    logical_size = header_size + kss->load_len + kss->bank_num * 0x2000;
  }
  kss->data = realloc(kss->data, logical_size);

  kssplay->kss = kss;

  kssplay->main_data = kss->data + header_size;
  kssplay->bank_data = kssplay->main_data + kss->load_len;

  switch (kss->type) {
  case MBMDATA:
    kssplay->vm->scc_disable = 1;
    break;
  case KSSDATA:
    kssplay->vm->scc_disable = (kss->bank_mode == BANK_16K) ? kss->ram_mode : 0;
    break;
  default:
    kssplay->vm->scc_disable = 0;
    break;
  }

  kssplay->vm->DA8_enable = kssplay->kss->DA8_enable;

  return 0;
}

static uint32_t getclk(KSSPLAY *kssplay) {
  if (kssplay->cpu_speed == 0) {
    if (kssplay->kss->fmpac || kssplay->kss->msx_audio) {
      return MSX_CLK * 2; /* 7.16MHz */
    } else {
      return MSX_CLK;
    }
  }
  if (kssplay->cpu_speed <= 8) {
    return MSX_CLK * kssplay->cpu_speed;
  }
  return MSX_CLK;
}

void KSSPLAY_get_MGStext(KSSPLAY *kssplay, char *buf, int max) {
  strncpy(buf, mgs_text, max);
  buf[max - 1] = '\0';
}

void KSSPLAY_set_speed(KSSPLAY *kssplay, uint32_t cpu_speed) {
  kssplay->cpu_speed = cpu_speed;
  VM_set_clock(kssplay->vm, getclk(kssplay), kssplay->vsync_freq);
}

void KSSPLAY_set_device_type(KSSPLAY *kssplay, uint32_t devnum, uint32_t type) {
  if (!kssplay->vm) {
    return;
  }

  switch (devnum) {
  case EDSC_PSG:
    VM_set_PSG_type(kssplay->vm, type);
    break;
  case EDSC_SCC:
    VM_set_SCC_type(kssplay->vm, type);
    break;
  case EDSC_OPLL:
    VM_set_OPLL_type(kssplay->vm, type);
    break;
  default:
    break;
  }
  kssplay->device_type[devnum] = type;
}

void KSSPLAY_reset(KSSPLAY *kssplay, uint32_t song, uint32_t cpu_speed) {
  int i;

  if (!kssplay->kss) {
    return;
  }

  kssplay->cpu_speed = cpu_speed;

  if (kssplay->vsync_freq == 0) {
    kssplay->vsync_freq = kssplay->kss->pal_mode ? PAL_FREQ : NTSC_FREQ;
  }

  VM_init_memory(kssplay->vm, kssplay->kss->ram_mode, kssplay->kss->load_adr, kssplay->kss->load_len,
                 kssplay->main_data);
  VM_init_bank(kssplay->vm, kssplay->kss->bank_mode, kssplay->kss->bank_num, kssplay->kss->bank_offset,
               kssplay->bank_data);

  for (i = 0; i < EDSC_MAX; i++) {
    KSSPLAY_set_device_type(kssplay, i, kssplay->device_type[i]);
  }

  VM_reset_device(kssplay->vm);
  VM_reset(kssplay->vm, getclk(kssplay), kssplay->kss->init_adr, kssplay->kss->play_adr, kssplay->vsync_freq, song,
           kssplay->kss->DA8_enable);

  kssplay->fade = 0;
  kssplay->fade_flag = 0;

  kssplay->step_inc = kssplay->vm->clock / kssplay->rate;
  kssplay->step_cnt = 0;
  kssplay->decoded_length = 0;
  kssplay->silent = 0;

  if (kssplay->kss->type == MGSDATA) {
    VM_set_wioproc(kssplay->vm, 0x44, mgs_text_update);
  }
  memset(mgs_text, 0, 128);
}

KSSPLAY *KSSPLAY_new(uint32_t rate, uint32_t nch, uint32_t bps) {
  KSSPLAY *kssplay;
  int i, j;

  if (volume_table[0] == 0) {
    make_voltable();
  }

  if ((bps != 16) || (nch > 2))
    return NULL;

  if (!(kssplay = malloc(sizeof(KSSPLAY)))) {
    return NULL;
  }

  memset(kssplay, 0, sizeof(KSSPLAY));

  if (!(kssplay->vm = VM_new(rate))) {
    free(kssplay);
    return NULL;
  }

  kssplay->rate = rate;
  kssplay->nch = nch;
  kssplay->bps = bps;
  kssplay->silent_limit = 5000;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < EDSC_MAX; j++) {
      kssplay->device_fir[i][j] = FIR_new();
      FIR_disable(kssplay->device_fir[i][j]);
    }

    kssplay->rcf[i] = RCF_new();
    // RCF_reset(kssplay->rcf[i], rate, 4.7e3, 0.010e-6);
    RCF_disable(kssplay->rcf[i]);
    kssplay->dcf[i] = DCF_new();
    DCF_reset(kssplay->dcf[i], rate);
  }

  return kssplay;
}

void KSSPLAY_set_device_quality(KSSPLAY *kssplay, uint32_t devnum, uint32_t quality) {
  switch (devnum) {
  case EDSC_PSG:
    PSG_set_quality(kssplay->vm->psg, quality);
    SNG_set_quality(kssplay->vm->sng, quality);
    break;
  case EDSC_SCC:
    SCC_set_quality(kssplay->vm->scc, quality);
    break;
  case EDSC_OPLL:
    OPLL_set_quality(kssplay->vm->opll, quality);
    break;
  default:
    break;
  }
}

void KSSPLAY_set_device_mute(KSSPLAY *kssplay, uint32_t devnum, uint32_t mute) {
  if (devnum < EDSC_MAX) {
    kssplay->device_mute[devnum] = mute;
  }
}

uint32_t KSSPLAY_get_device_volume(KSSPLAY *kssplay, uint32_t devnum) {
  if (devnum < EDSC_MAX) {
    return kssplay->device_volume[devnum];
  } else {
    return 0;
  }
}

void KSSPLAY_set_device_volume(KSSPLAY *kssplay, uint32_t devnum, int32_t vol) {
  if (devnum < EDSC_MAX) {
    kssplay->device_volume[devnum] = vol;
  }
}
void KSSPLAY_set_master_volume(KSSPLAY *kssplay, int32_t vol) { kssplay->master_volume = vol; }

void KSSPLAY_set_channel_pan(KSSPLAY *kssplay, uint32_t device, uint32_t ch, uint32_t pan) {
  switch (device) {
  case EDSC_OPLL:
    if (kssplay->vm->opll) {
      OPLL_set_pan(kssplay->vm->opll, ch, pan);
    }
    break;
  default:
    break;
  }
}

void KSSPLAY_set_device_pan(KSSPLAY *kssplay, uint32_t device, int32_t pan) { kssplay->device_pan[device] = pan; }

void KSSPLAY_set_rcf(KSSPLAY *kssplay, uint32_t r, uint32_t c) {
  if (r != 0 && c != 0) {
    RCF_reset(kssplay->rcf[0], kssplay->rate, (double)r, (double)c / 1.0e9);
    RCF_reset(kssplay->rcf[1], kssplay->rate, (double)r, (double)c / 1.0e9);
  } else {
    RCF_disable(kssplay->rcf[0]);
    RCF_disable(kssplay->rcf[1]);
  }
}

void KSSPLAY_set_device_lpf(KSSPLAY *kssplay, uint32_t device, uint32_t cutoff) {
  FIR_reset(kssplay->device_fir[0][device], kssplay->rate, cutoff, 31);
  FIR_reset(kssplay->device_fir[1][device], kssplay->rate, cutoff, 31);
}

void KSSPLAY_delete(KSSPLAY *kssplay) {
  int i, j;

  if (kssplay) {
    VM_delete(kssplay->vm);

    for (i = 0; i < 2; i++) {
      for (j = 0; j < EDSC_MAX; j++) {
        FIR_delete(kssplay->device_fir[i][j]);
      }
      RCF_delete(kssplay->rcf[i]);
      DCF_delete(kssplay->dcf[i]);
    }
    free(kssplay);
  }
}

int KSSPLAY_get_fade_flag(KSSPLAY *kssplay) { return kssplay->fade_flag; }

#define FADE_BIT 8
#define FADE_BASE_BIT 23

void KSSPLAY_fade_start(KSSPLAY *kssplay, uint32_t fade_time) {
  if (fade_time) {
    kssplay->fade = (uint32_t)1 << (FADE_BIT + FADE_BASE_BIT);
    kssplay->fade_step = ((uint32_t)((double)kssplay->fade / (kssplay->rate * fade_time / 1000))) >> (kssplay->nch - 1);
    if (kssplay->fade_step == 0) {
      kssplay->fade_step = 1;
    }
    kssplay->fade_flag = KSSPLAY_FADE_OUT;
  } else {
    kssplay->fade = 0;
    kssplay->fade_flag = KSSPLAY_FADE_END;
  }
}

void KSSPLAY_fade_stop(KSSPLAY *kssplay) {
  kssplay->fade_flag = KSSPLAY_FADE_END;
  kssplay->fade = 0;
}

static inline int32_t fader(KSSPLAY *kssplay, int32_t sample) {
  if (kssplay->fade_flag == KSSPLAY_FADE_OUT) {
    if (kssplay->fade > kssplay->fade_step) {
      kssplay->fade -= kssplay->fade_step;
      return ((int32_t)(sample * (kssplay->fade >> FADE_BASE_BIT)) >> FADE_BIT);
    } else {
      kssplay->fade = 0;
      kssplay->fade_flag = KSSPLAY_FADE_END;
      return 0;
    }
  } else if (kssplay->fade_flag == KSSPLAY_FADE_END) {
    return 0;
  } else {
    return sample;
  }
}

static inline int16_t compress(int32_t wav) {
  const int32_t vt = 32768 * 8 / 10;

  if (wav < -vt) {
    wav = ((wav + vt) >> 2) - vt;
  } else if (vt < wav) {
    wav = ((wav - vt) >> 2) + vt;
  }

  if (wav < -32767) {
    wav = -32767;
  } else if (32767 < wav) {
    wav = 32767;
  }

  return (int16_t)wav;
}

static inline int clip(int min, int val, int max) {
  if (max < val) {
    return max;
  } else if (val < min) {
    return min;
  } else {
    return val;
  }
}

static inline void apply_fade(int16_t *buf, int32_t len, int32_t fade) {
  int i;
  for (i = 0; i < len; i++) {
    buf[i] = (int16_t)((int32_t)(buf[i] * (fade >> FADE_BASE_BIT)) >> FADE_BIT);
  }
}

static inline void fade_per_ch(KSSPLAY *kssplay, KSSPLAY_PER_CH_OUT *out) {
  if (kssplay->fade_flag == KSSPLAY_FADE_OUT) {
    if (kssplay->fade > kssplay->fade_step) {
      kssplay->fade -= kssplay->fade_step;
      apply_fade(out->psg, 3, kssplay->fade);
      apply_fade(out->scc, 5, kssplay->fade);
      apply_fade(out->opll, 15, kssplay->fade);
      apply_fade(out->opl, 15, kssplay->fade);
      apply_fade(out->sng, 4, kssplay->fade);
      apply_fade(out->dac, 2, kssplay->fade);
    } else {
      kssplay->fade = 0;
      kssplay->fade_flag = KSSPLAY_FADE_END;
      memset(out, 0, sizeof(*out));
    }
  } else if (kssplay->fade_flag == KSSPLAY_FADE_END) {
    memset(out, 0, sizeof(*out));
  }
}

static inline void process_vm(KSSPLAY *kssplay) {
  int step_int = (int)kssplay->step_cnt;
  kssplay->step_cnt += kssplay->step_inc;
  if (step_int > 1) {
    kssplay->step_cnt -= step_int;
    VM_exec(kssplay->vm, step_int);
  }
}

static inline void calc_per_ch(KSSPLAY *kssplay, KSSPLAY_PER_CH_OUT *out) {

  process_vm(kssplay);

  memset(out, 0, sizeof(*out));

  if (kssplay->kss->msx_audio && !kssplay->device_mute[EDSC_OPL]) {
    OPL_calc(kssplay->vm->opl);
    memcpy(out->opl, kssplay->vm->opl->ch_out, sizeof(out->opl));
  }

  if (kssplay->kss->fmpac && !kssplay->device_mute[EDSC_OPLL]) {
    OPLL_calc(kssplay->vm->opll);
    memcpy(out->opll, kssplay->vm->opll->ch_out, sizeof(out->opll));
  }

  if (!kssplay->device_mute[EDSC_PSG]) {
    if (kssplay->kss->sn76489) {
      SNG_calc(kssplay->vm->sng);
      memcpy(out->sng, kssplay->vm->sng->ch_out, sizeof(out->sng));
    } else {
      PSG_calc(kssplay->vm->psg);
      memcpy(out->psg, kssplay->vm->psg->ch_out, sizeof(out->psg));
    }
  }

  if (!kssplay->device_mute[EDSC_SCC]) {
    SCC_calc(kssplay->vm->scc);
    memcpy(out->scc, kssplay->vm->scc->ch_out, sizeof(out->scc));
  }

  out->dac[0] = kssplay->vm->DA1;
  out->dac[1] = kssplay->vm->DA8;

  fade_per_ch(kssplay, out);
}

void KSSPLAY_calc_per_ch(KSSPLAY *kssplay, KSSPLAY_PER_CH_OUT *out, uint32_t length) {
  uint32_t i;
  for (i = 0; i < length; i++) {
    calc_per_ch(kssplay, out + i);
  }
}

static inline void calc_mono(KSSPLAY *kssplay, int16_t *buf, uint32_t length) {
  uint32_t i;
  int32_t d;
  int32_t volume[EDSC_MAX];

  for (i = 0; i < EDSC_MAX; i++) {
    volume[i] = clip(-256, kssplay->device_volume[i] + kssplay->master_volume, 255);
  }

  for (i = 0; i < length; i++) {
    process_vm(kssplay);

    if (kssplay->kss->msx_audio && !kssplay->device_mute[EDSC_OPL]) {
      d = apply_volume(FIR_calc(kssplay->device_fir[0][EDSC_OPL], OPL_calc(kssplay->vm->opl)), volume[EDSC_OPL]);
    } else {
      d = 0;
    }

    if (kssplay->kss->fmpac && !kssplay->device_mute[EDSC_OPLL]) {
      d += apply_volume(FIR_calc(kssplay->device_fir[0][EDSC_OPLL], OPLL_calc(kssplay->vm->opll)), volume[EDSC_OPLL]);
    }

    if (!kssplay->device_mute[EDSC_PSG]) {
      if (kssplay->kss->sn76489) {
        d += apply_volume(FIR_calc(kssplay->device_fir[0][EDSC_PSG], SNG_calc(kssplay->vm->sng)), volume[EDSC_PSG]);
      } else {
        d += apply_volume(FIR_calc(kssplay->device_fir[0][EDSC_PSG], PSG_calc(kssplay->vm->psg) + kssplay->vm->DA1),
                          volume[EDSC_PSG]);
      }
    }

    if (!kssplay->device_mute[EDSC_SCC]) {
      d += apply_volume(FIR_calc(kssplay->device_fir[0][EDSC_SCC], SCC_calc(kssplay->vm->scc) + kssplay->vm->DA8),
                        volume[EDSC_SCC]);
    }

    /* Check silent span */
    if (kssplay->lastout[0] == d) {
      kssplay->silent++;
    } else {
      kssplay->lastout[0] = d;
      kssplay->silent = 0;
    }

    d = fader(kssplay, DCF_calc(kssplay->dcf[0], d));
    buf[i] = compress(RCF_calc(kssplay->rcf[0], d));
  }
}

static inline void calc_stereo(KSSPLAY *kssplay, int16_t *buf, uint32_t length) {
  uint32_t i;
  int32_t ch[2], b[2], c;
  int32_t volume[EDSC_MAX][2];

#define neg(x) ((x < 0) ? x : 0)

  for (i = 0; i < EDSC_MAX; i++) {
    volume[i][0] = clip(-256, kssplay->device_volume[i] + kssplay->master_volume + neg(kssplay->device_pan[i]), 255);
    volume[i][1] = clip(-256, kssplay->device_volume[i] + kssplay->master_volume + neg(-kssplay->device_pan[i]), 255);
  }

  for (i = 0; i < length; i++) {
    process_vm(kssplay);

    if (kssplay->kss->msx_audio && !kssplay->device_mute[EDSC_OPL]) {
      c = FIR_calc(kssplay->device_fir[0][EDSC_OPL], OPL_calc(kssplay->vm->opl));
      ch[0] = apply_volume(c, volume[EDSC_OPL][0]);
      ch[1] = apply_volume(c, volume[EDSC_OPL][1]);
    } else
      ch[0] = ch[1] = 0;

    if (kssplay->kss->fmpac && !kssplay->device_mute[EDSC_OPLL]) {
      if (kssplay->opll_stereo) {
        OPLL_calc_stereo(kssplay->vm->opll, b);
        ch[0] += apply_volume(FIR_calc(kssplay->device_fir[0][EDSC_OPLL], b[0]), volume[EDSC_OPLL][0]);
        ch[1] += apply_volume(FIR_calc(kssplay->device_fir[1][EDSC_OPLL], b[1]), volume[EDSC_OPLL][1]);
      } else {
        c = FIR_calc(kssplay->device_fir[0][EDSC_OPLL], OPLL_calc(kssplay->vm->opll));
        ch[0] += apply_volume(c, volume[EDSC_OPLL][0]);
        ch[1] += apply_volume(c, volume[EDSC_OPLL][1]);
      }
    }

    if (!kssplay->device_mute[EDSC_PSG]) {
      if (kssplay->kss->sn76489) {
        if (kssplay->kss->stereo) {
          SNG_calc_stereo(kssplay->vm->sng, b);
          ch[0] += apply_volume(FIR_calc(kssplay->device_fir[0][EDSC_PSG], b[0]), volume[EDSC_PSG][0]);
          ch[1] += apply_volume(FIR_calc(kssplay->device_fir[1][EDSC_PSG], b[1]), volume[EDSC_PSG][1]);
        } else {
          c = FIR_calc(kssplay->device_fir[0][EDSC_PSG], SNG_calc(kssplay->vm->sng));
          ch[0] += apply_volume(c, volume[EDSC_PSG][0]);
          ch[1] += apply_volume(c, volume[EDSC_PSG][1]);
        }
      } else {
        c = FIR_calc(kssplay->device_fir[0][EDSC_PSG], PSG_calc(kssplay->vm->psg) + kssplay->vm->DA1);
        ch[0] += apply_volume(c, volume[EDSC_PSG][0]);
        ch[1] += apply_volume(c, volume[EDSC_PSG][1]);
      }
    }

    if (!kssplay->device_mute[EDSC_SCC]) {
      c = FIR_calc(kssplay->device_fir[0][EDSC_SCC], SCC_calc(kssplay->vm->scc) + kssplay->vm->DA8);
      ch[0] += apply_volume(c, volume[EDSC_SCC][0]);
      ch[1] += apply_volume(c, volume[EDSC_SCC][1]);
    }

    /* Check silent span */
    if (kssplay->lastout[0] == ch[0] && kssplay->lastout[1] == ch[1]) {
      kssplay->silent++;
    } else {
      kssplay->lastout[0] = ch[0];
      kssplay->lastout[1] = ch[1];
      kssplay->silent = 0;
    }

    ch[0] = fader(kssplay, DCF_calc(kssplay->dcf[0], ch[0]));
    ch[1] = fader(kssplay, DCF_calc(kssplay->dcf[1], ch[1]));
    buf[(i << 1)] = compress(RCF_calc(kssplay->rcf[0], ch[0]));
    buf[(i << 1) + 1] = compress(RCF_calc(kssplay->rcf[1], ch[1]));
  }
}

void KSSPLAY_calc(KSSPLAY *kssplay, int16_t *buf, uint32_t length) {
  if (kssplay->nch == 1) {
    calc_mono(kssplay, buf, length);
  } else {
    calc_stereo(kssplay, buf, length);
  }

  kssplay->decoded_length += length;
  LPDETECT_update(kssplay->vm->lpde, kssplay->decoded_length * 1000 / kssplay->rate, 30 * 1000, 1000);
}

void KSSPLAY_calc_silent(KSSPLAY *kssplay, uint32_t length) {
  uint32_t i;
  for (i = 0; i < length; i++) {
    process_vm(kssplay);
  }
  kssplay->decoded_length += length;
  LPDETECT_update(kssplay->vm->lpde, kssplay->decoded_length * 1000 / kssplay->rate, 30 * 1000, 1000);

  if (KSSPLAY_get_fade_flag(kssplay) == KSSPLAY_FADE_OUT) {
    if (kssplay->fade > kssplay->fade_step * length) {
      kssplay->fade -= kssplay->fade_step * length;
    } else {
      kssplay->fade = 0;
      kssplay->fade_flag = KSSPLAY_FADE_END;
    }
  }
}

int KSSPLAY_get_loop_count(KSSPLAY *kssplay) {
  if (kssplay->kss->type == KSSDATA) {
    return LPDETECT_count(kssplay->vm->lpde, kssplay->decoded_length * 1000 / kssplay->rate);
  } else {
    return kssplay->vm->IO[LOOPIO];
  }
}

int KSSPLAY_get_stop_flag(KSSPLAY *kssplay) {
  if (kssplay->silent_limit != 0 && (kssplay->silent * 1000 / kssplay->rate > kssplay->silent_limit)) {
    return 1;
  }
  return kssplay->vm->IO[STOPIO];
}

int KSSPLAY_read_memory(KSSPLAY *kssplay, uint32_t address) { return MMAP_read_memory(kssplay->vm->mmap, address); }

void KSSPLAY_set_opll_patch(KSSPLAY *kssplay, uint8_t *data) {
  if (kssplay->vm->opll)
    OPLL_setPatch(kssplay->vm->opll, data);
}

void KSSPLAY_set_channel_mask(KSSPLAY *kssplay, uint32_t device, uint32_t mask) {
  switch (device) {
  case EDSC_PSG:
    if (kssplay->vm->psg) {
      PSG_setMask(kssplay->vm->psg, mask);
    }
    break;
  case EDSC_SCC:
    if (kssplay->vm->scc) {
      SCC_setMask(kssplay->vm->scc, mask);
    }
    break;
  case EDSC_OPLL:
    if (kssplay->vm->opll) {
      OPLL_setMask(kssplay->vm->opll, mask);
    }
    break;
  case EDSC_OPL:
    if (kssplay->vm->opl) {
      OPL_setMask(kssplay->vm->opl, mask);
    }
    break;
  }
}

void KSSPLAY_set_silent_limit(KSSPLAY *kssplay, uint32_t time_in_ms) { kssplay->silent_limit = time_in_ms; }

void KSSPLAY_set_iowrite_handler(KSSPLAY *kssplay, void *context,
                                 void (*handler)(void *context, uint32_t a, uint32_t d)) {
  kssplay->vm->iowrite_handler_context = context ? context : kssplay;
  kssplay->vm->iowrite_handler = (void (*)(void *, uint32_t, uint32_t))handler;
}
void KSSPLAY_set_memwrite_handler(KSSPLAY *kssplay, void *context,
                                  void (*handler)(void *context, uint32_t a, uint32_t d)) {
  kssplay->vm->memwrite_handler_context = context ? context : kssplay;
  kssplay->vm->memwrite_handler = (void (*)(void *, uint32_t, uint32_t))handler;
}

void KSSPLAY_write_io(KSSPLAY *kssplay, uint32_t a, uint32_t d) { VM_write_io(kssplay->vm, a, d); }

void KSSPLAY_write_memory(KSSPLAY *kssplay, uint32_t a, uint32_t d) { VM_write_memory(kssplay->vm, a, d); }

uint8_t KSSPLAY_get_MGS_jump_count(KSSPLAY *kssplay) {
  if (kssplay->kss && kssplay->kss->type == MGSDATA) {
    uint8_t al = MMAP_read_memory(kssplay->vm->mmap, 0x7ff0);
    uint8_t ah = MMAP_read_memory(kssplay->vm->mmap, 0x7ff1);
    uint16_t adr = (ah << 8) | al;
    return MMAP_read_memory(kssplay->vm->mmap, adr + 6);
  }
  return 0;
}