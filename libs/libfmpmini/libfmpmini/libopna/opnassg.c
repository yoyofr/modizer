#include "opnassg.h"
#ifdef LIBOPNA_ENABLE_OSCILLO
#include "oscillo/oscillo.h"
#endif

// if (i < 2) voltable[i] = 0;
// else       voltable[i] = round((0x7fff / 3.0) * pow(2.0, (i - 31)/4.0));
/*
static const int16_t voltable[32] = {
      0,     0,    72,    85,
    101,   121,   144,   171,
    203,   241,   287,   341,
    406,   483,   574,   683,
    812,   965,  1148,  1365,
   1624,  1931,  2296,  2731,
   3247,  3862,  4592,  5461,
   6494,  7723,  9185, 10922
};
*/

// captured from YMF288
static const uint16_t voltable[32] = {
      0,     0,     0,     0,
      4,     8,    12,    16,
     20,    24,    28,    32,
     36,    44,    52,    64,
     76,    92,   108,   128,
    152,   180,   216,   256,
    304,   360,   428,   512,
    608,   720,   856,  1020,
};

/*
 * on YMF288:
 * when TP >= 8:
 *
 *          |
 *  +voltab +     -----     -----     --
 *          |
 *        0 +---------------------------
 *          |
 *  -voltab +          -----     -----
 *          |
 *
 * when TP < 8:
 *          |
 *  +voltab +     ----------------------
 *          |
 *        0 +---------------------------
 *          |
 *  -voltab +
 *          |
 *
 * when /TONE=1 && /NOISE=1 (both diabled)
 *          |
 * +2voltab +     ----------------------
 *          |
 *          +
 *          |
 *        0 +---------------------------
 *          |
 */

// GNU Octave
// Fc = 7987200
// Ff = Fc/144
// Fs = Fc/32
// Fe = 20000
// O = (((Ff/2)-Fe)*2)/(Fs/2)
// B = 128 * O / 2
// FILTER=sinc(linspace(-127.5,127.5,256)*2/9/2).*rotdim(kaiser(256,B))
// FILTERI=round(FILTER(1:128).*32768)
#if 0
const int16_t opna_ssg_sinctable[OPNA_SSG_SINCTABLELEN*2] = {
      1,     0,    -1,    -2,    -3,    -5,    -6,    -6,
     -6,    -5,    -2,     2,     7,    11,    16,    19,
     20,    18,    13,     5,    -5,   -17,   -29,   -38,
    -44,   -45,   -40,   -29,   -11,    12,    36,    60,
     79,    90,    91,    80,    56,    21,   -22,   -68,
   -112,  -146,  -166,  -166,  -144,  -100,   -37,    39,
    119,   193,   251,   282,   280,   241,   166,    61,
    -64,  -195,  -315,  -406,  -455,  -450,  -385,  -264,
    -96,   101,   306,   491,   632,   705,   694,   593,
    405,   147,  -154,  -464,  -744,  -954, -1062, -1043,
   -889,  -607,  -220,   230,   692,  1108,  1421,  1580,
   1552,  1322,   902,   328,  -343, -1032, -1655, -2125,
  -2369, -2333, -1994, -1365,  -498,   523,  1585,  2557,
   3306,  3714,  3690,  3185,  2206,   815,  -868, -2673,
  -4391, -5798, -6670, -6809, -6067, -4359, -1681,  1886,
   6178, 10957, 15928, 20765, 25133, 28724, 31275, 32600,
  32600, 31275, 28724, 25133, 20765, 15928, 10957,  6178,
   1886, -1681, -4359, -6067, -6809, -6670, -5798, -4391,
  -2673,  -868,   815,  2206,  3185,  3690,  3714,  3306,
   2557,  1585,   523,  -498, -1365, -1994, -2333, -2369,
  -2125, -1655, -1032,  -343,   328,   902,  1322,  1552,
   1580,  1421,  1108,   692,   230,  -220,  -607,  -889,
  -1043, -1062,  -954,  -744,  -464,  -154,   147,   405,
    593,   694,   705,   632,   491,   306,   101,   -96,
   -264,  -385,  -450,  -455,  -406,  -315,  -195,   -64,
     61,   166,   241,   280,   282,   251,   193,   119,
     39,   -37,  -100,  -144,  -166,  -166,  -146,  -112,
    -68,   -22,    21,    56,    80,    91,    90,    79,
     60,    36,    12,   -11,   -29,   -40,   -45,   -44,
    -38,   -29,   -17,    -5,     5,    13,    18,    20,
     19,    16,    11,     7,     2,    -2,    -5,    -6,
     -6,    -6,    -5,    -3,    -2,    -1,     0,     1,
};
#endif
const int16_t opna_ssg_sinctable[OPNA_SSG_SINCTABLELEN*2] = {
      1,    -1,    -3,    -6,    -6,    -2,     7,    16,
     20,    13,    -5,   -29,   -44,   -40,   -11,    36,
     79,    91,    56,   -22,  -112,  -166,  -144,   -37,
    119,   251,   280,   166,   -64,  -315,  -455,  -385,
    -96,   306,   632,   694,   405,  -154,  -744, -1062,
   -889,  -220,   692,  1421,  1552,   902,  -343, -1655,
  -2369, -1994,  -498,  1585,  3306,  3690,  2206,  -868,
  -4391, -6670, -6067, -1681,  6178, 15928, 25133, 31275,
  32600, 28724, 20765, 10957,  1886, -4359, -6809, -5798,
  -2673,   815,  3185,  3714,  2557,   523, -1365, -2333,
  -2125, -1032,   328,  1322,  1580,  1108,   230,  -607,
  -1043,  -954,  -464,   147,   593,   705,   491,   101,
   -264,  -450,  -406,  -195,    61,   241,   282,   193,
     39,  -100,  -166,  -146,   -68,    21,    80,    90,
     60,    12,   -29,   -45,   -38,   -17,     5,    18,
     19,    11,     2,    -5,    -6,    -5,    -2,     0,
      0,    -2,    -5,    -6,    -5,     2,    11,    19,
     18,     5,   -17,   -38,   -45,   -29,    12,    60,
     90,    80,    21,   -68,  -146,  -166,  -100,    39,
    193,   282,   241,    61,  -195,  -406,  -450,  -264,
    101,   491,   705,   593,   147,  -464,  -954, -1043,
   -607,   230,  1108,  1580,  1322,   328, -1032, -2125,
  -2333, -1365,   523,  2557,  3714,  3185,   815, -2673,
  -5798, -6809, -4359,  1886, 10957, 20765, 28724, 32600,
  31275, 25133, 15928,  6178, -1681, -6067, -6670, -4391,
   -868,  2206,  3690,  3306,  1585,  -498, -1994, -2369,
  -1655,  -343,   902,  1552,  1421,   692,  -220,  -889,
  -1062,  -744,  -154,   405,   694,   632,   306,   -96,
   -385,  -455,  -315,   -64,   166,   280,   251,   119,
    -37,  -144,  -166,  -112,   -22,    56,    91,    79,
     36,   -11,   -40,   -44,   -29,    -5,    13,    20,
     16,     7,    -2,    -6,    -6,    -3,    -1,     1,
};

opna_ssg_sinc_calc_func_type opna_ssg_sinc_calc_func = opna_ssg_sinc_calc_c;

void opna_ssg_reset(struct opna_ssg *ssg) {
  *ssg = (struct opna_ssg) {
    .mix = 0x10000,
    .ymf288 = true,
  };
}

void opna_ssg_resampler_reset(struct opna_ssg_resampler *resampler) {
  for (int i = 0; i < OPNA_SSG_SINCTABLELEN; i++) {
    resampler->buf[i] = 0;
  }
  resampler->index = 0;
#ifdef LIBOPNA_ENABLE_LEVELDATA
  for (int c = 0; c < 3; c++) {
    leveldata_init(&resampler->leveldata[c]);
  }
#endif
}

void opna_ssg_writereg(struct opna_ssg *ssg, unsigned reg, unsigned val) {
  if (reg > 0xfu) return;
  val &= 0xff;
  ssg->regs[reg] = val;

  if (reg == 0xd) {
    ssg->env_att = ssg->regs[0xd] & 0x4;
    if (ssg->regs[0xd] & 0x8) {
      ssg->env_alt = ssg->regs[0xd] & 0x2;
      ssg->env_hld = ssg->regs[0xd] & 0x1;
    } else {
      ssg->env_alt = ssg->env_att;
      ssg->env_hld = true;
    }
    ssg->env_holding = false;
    ssg->env_level = 0;
    ssg->env_counter = 0;
  }
}

unsigned opna_ssg_readreg(const struct opna_ssg *ssg, unsigned reg) {
  if (reg > 0xfu) return 0xff;
  return ssg->regs[reg];
}

unsigned opna_ssg_tone_period(const struct opna_ssg *ssg, int ch) {
  if (ch < 0) return 0;
  if (ch >= 3) return 0;
  return ssg->regs[0+ch*2] | ((ssg->regs[1+ch*2] & 0xf) << 8);
}

static bool opna_ssg_chan_env(const struct opna_ssg *ssg, int chan) {
  return ssg->regs[0x8+chan] & 0x10;
}
static int opna_ssg_tone_volume(const struct opna_ssg *ssg, int chan) {
  return ssg->regs[0x8+chan] & 0xf;
}

static bool opna_ssg_tone_out(const struct opna_ssg *ssg, int chan) {
  unsigned reg = ssg->regs[0x7] >> chan;
  return (ssg->ch[chan].out || (reg & 0x1)) && ((ssg->lfsr & 1) || (reg & 0x8));
}

static bool opna_ssg_tone_out_ymf288(const struct opna_ssg *ssg, int chan) {
  unsigned reg = ssg->regs[0x7] >> chan;
  bool toneout = 
      opna_ssg_tone_period(ssg, chan) < 8 ? true : ssg->ch[chan].out;
  return (toneout || (reg & 0x1)) && ((ssg->lfsr & 1) || (reg & 0x8));
}

static bool opna_ssg_tone_silent(const struct opna_ssg *ssg, int chan) {
  unsigned reg = ssg->regs[0x7] >> chan;
  return (reg & 0x1) && (reg & 0x8);
}

static int opna_ssg_noise_period(const struct opna_ssg *ssg) {
  return ssg->regs[0x6] & 0x1f;
}

static int opna_ssg_env_period(const struct opna_ssg *ssg) {
  return (ssg->regs[0xc] << 8) | ssg->regs[0xb];
}

static int opna_ssg_env_level(const struct opna_ssg *ssg) {
  return ssg->env_att ? ssg->env_level : 31-ssg->env_level;
}

int opna_ssg_channel_level(const struct opna_ssg *ssg, int ch) {
  return opna_ssg_chan_env(ssg, ch)
       ? opna_ssg_env_level(ssg)
       : (opna_ssg_tone_volume(ssg, ch) << 1) + 1;
}

#define COEFF 0x3fff
#define COEFFSH 14

// 3 samples per frame
// output buf: 0 1 2 x 0 1 2 x ...
void opna_ssg_generate_raw(struct opna_ssg *ssg, int16_t *buf, int samples) {
  for (int i = 0; i < samples; i++) {
    if (((++ssg->noise_counter) >> 1) >= opna_ssg_noise_period(ssg)) {
      ssg->noise_counter = 0;
      ssg->lfsr |= (!((ssg->lfsr & 1) ^ ((ssg->lfsr >> 3) & 1))) << 17;
      ssg->lfsr >>= 1;
    }
    if (!ssg->env_holding) {
      if (++ssg->env_counter >= opna_ssg_env_period(ssg)) {
        ssg->env_counter = 0;
        ssg->env_level++;
        if (ssg->env_level == 0x20) {
          ssg->env_level = 0;
          if (ssg->env_alt) {
            ssg->env_att = !ssg->env_att;
          }
          if (ssg->env_hld) {
            ssg->env_level = 0x1f;
            ssg->env_holding = true;
          }
        }
      }
    }

    //int16_t out = 0;
    for (int ch = 0; ch < 3; ch++) {
      buf[i*4+ch] = 0;
      if (++ssg->ch[ch].tone_counter >= opna_ssg_tone_period(ssg, ch)) {
        ssg->ch[ch].tone_counter = 0;
        ssg->ch[ch].out = !ssg->ch[ch].out;
      }
      if (!ssg->ymf288) {
        // OPNA output level + HPF
        int32_t previntmp = 0;
        if (opna_ssg_tone_out(ssg, ch)) {
          int level = opna_ssg_channel_level(ssg, ch);
          previntmp = voltable[level]*5;
        }
        previntmp *= COEFF;
        ssg->prevout[ch] = previntmp - ssg->previn[ch] + ((((int64_t)COEFF)*ssg->prevout[ch]) >> COEFFSH);
        ssg->previn[ch] = previntmp;
        buf[i*4+ch] = ssg->prevout[ch] >> COEFFSH;
      } else {
        // YMF288
        int level = opna_ssg_channel_level(ssg, ch);
        if (!opna_ssg_tone_silent(ssg, ch)) {
          buf[i*4+ch] = (opna_ssg_tone_out_ymf288(ssg, ch) ? voltable[level] : -voltable[level]);
        } else {
          buf[i*4+ch] = voltable[level]*2;
        }
      }
      
    }
    //buf[i] = out / 2;
  }
}

#define BUFINDEX(n) ((((resampler->index)>>1)+n)&(OPNA_SSG_SINCTABLELEN-1))

void opna_ssg_mix_55466(
  struct opna_ssg *ssg, struct opna_ssg_resampler *resampler,
  int16_t *buf, int samples,
  struct oscillodata *oscillo, unsigned offset
) {
#ifdef LIBOPNA_ENABLE_OSCILLO
  if (oscillo) {
    for (unsigned c = 0; c < 3; c++) {
      unsigned period = (opna_ssg_tone_period(ssg, c) << OSCILLO_OFFSET_SHIFT) * 2 * 32 / 144;
      if (period) {
        oscillo[c].offset += (samples << OSCILLO_OFFSET_SHIFT);
        oscillo[c].offset %= period;
      } else {
        oscillo[c].offset = 0;
      }
    }
  }
#else
  (void)oscillo;
  (void)offset;
#endif
  unsigned level[3] = {0};
  for (int i = 0; i < samples; i++) {
    {
      int ssg_samples = ((resampler->index + 9)>>1) - ((resampler->index)>>1);
      int16_t ssgbuf[20];
      opna_ssg_generate_raw(ssg, ssgbuf, ssg_samples);
      for (int j = 0; j < ssg_samples; j++) {
        resampler->buf[BUFINDEX(j)*4+0] = ssgbuf[j*4+0];
        resampler->buf[BUFINDEX(j)*4+1] = ssgbuf[j*4+1];
        resampler->buf[BUFINDEX(j)*4+2] = ssgbuf[j*4+2];
      }
      resampler->index += 9;
    }
    int32_t sample = 0;
    resampler->index &= (1u<<(OPNA_SSG_SINCTABLEBIT+1))-1;
    memcpy(resampler->buf + OPNA_SSG_SINCTABLELEN*4, resampler->buf, OPNA_SSG_SINCTABLELEN*4*sizeof(*resampler->buf));
    int32_t outbuf[3];
    if (!ssg->ymf288) {
      // OPNA analog: bandlimited sinc resample
      opna_ssg_sinc_calc_func(resampler->index, resampler->buf, outbuf);
      for (int ch = 0; ch < 3; ch++) {
        outbuf[ch] >>= 16;
        outbuf[ch] *= 13000;
        outbuf[ch] >>= 16;
        outbuf[ch] *= ssg->mix;
        outbuf[ch] >>= 16;
      }
    } else {
      // YMF288: average of the samples (equivalent to FIR with rectangular function
      for (int ch = 0; ch < 3; ch++) {
        int ind = (resampler->index & 1) ? BUFINDEX(5) : BUFINDEX(0);
        outbuf[ch] = resampler->buf[ind*4+ch];
        for (int s = 0; s < 4; s++) {
          outbuf[ch] += resampler->buf[BUFINDEX(s+1)*4+ch] * 2;
        }
        outbuf[ch] /= 9;
      }
    }
    for (int ch = 0; ch < 3; ch++) {
#ifdef LIBOPNA_ENABLE_OSCILLO
      if (oscillo) oscillo[ch].buf[offset+i] = outbuf[ch] << 1;
#endif
      int32_t nlevel = outbuf[ch];
      if (nlevel < 0) nlevel = -nlevel;
      if (((unsigned)nlevel) > level[ch]) level[ch] = nlevel;
      if (!(ssg->mask & (1<<ch))) sample += outbuf[ch];
    }

    int32_t lo = buf[i*2+0];
    int32_t ro = buf[i*2+1];
    lo += sample;
    ro += sample;
    if (lo < INT16_MIN) lo = INT16_MIN;
    if (lo > INT16_MAX) lo = INT16_MAX;
    if (ro < INT16_MIN) ro = INT16_MIN;
    if (ro > INT16_MAX) ro = INT16_MAX;
    buf[i*2+0] = lo;
    buf[i*2+1] = ro;
  }
#ifdef LIBOPNA_ENABLE_LEVELDATA
  for (int c = 0; c < 3; c++) {
    leveldata_update(&resampler->leveldata[c], level[c]);
  }
#endif
}
#undef BUFINDEX
