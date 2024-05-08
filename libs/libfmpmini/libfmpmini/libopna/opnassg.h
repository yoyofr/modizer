#ifndef LIBOPNA_OPNASSG_H_INCLUDED
#define LIBOPNA_OPNASSG_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#ifdef LIBOPNA_ENABLE_LEVELDATA
#include "leveldata/leveldata.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define OPNA_SSG_SINCTABLEBIT 7
#define OPNA_SSG_SINCTABLELEN (1<<OPNA_SSG_SINCTABLEBIT)

struct opna_ssg_ch {
  uint16_t tone_counter;
  bool out;
};

struct opna_ssg {
  uint8_t regs[0x10];
  struct opna_ssg_ch ch[3];
  uint8_t noise_counter;
  uint32_t lfsr;
  uint16_t env_counter;
  uint8_t env_level;
  bool env_att;
  bool env_alt;
  bool env_hld;
  bool env_holding;
  unsigned mask;
  int32_t previn[3];
  int32_t prevout[3];
  uint32_t mix;
  bool ymf288;
};

struct opna_ssg_resampler {
  int16_t buf[OPNA_SSG_SINCTABLELEN*4 * 2];
  unsigned index;
#ifdef LIBOPNA_ENABLE_LEVELDATA
  struct leveldata leveldata[3];
#endif
};

void opna_ssg_reset(struct opna_ssg *ssg);
void opna_ssg_resampler_reset(struct opna_ssg_resampler *resampler);
// generate raw data
// Monoral
// Output level: [-32766, 32766]
// Samplerate: clock / 8
// (on opna: masterclock / 32
// 7987200 / 32 = 249600)
void opna_ssg_generate_raw(struct opna_ssg *ssg, int16_t *buf, int samples);

// mix samplerate converted data for mixing with OPNA output
// call to buffer written with OPNA output
// samplerate: 7987200/144 Hz
//            (55466.66..) Hz
struct oscillodata;
void opna_ssg_mix_55466(
  struct opna_ssg *ssg, struct opna_ssg_resampler *resampler,
  int16_t *buf, int samples, struct oscillodata *oscillo, unsigned offset);
void opna_ssg_writereg(struct opna_ssg *ssg, unsigned reg, unsigned val);
unsigned opna_ssg_readreg(const struct opna_ssg *ssg, unsigned reg);
// channel level (0 - 31)
int opna_ssg_channel_level(const struct opna_ssg *ssg, int ch);
unsigned opna_ssg_tone_period(const struct opna_ssg *ssg, int ch);

static inline void opna_ssg_set_ymf288(
    struct opna_ssg *ssg, struct opna_ssg_resampler *resampler, bool ymf288) {
  // enable bit-perfect with YMF288/OPN3-L mode
  if (ssg->ymf288 != ymf288) {
    ssg->ymf288 = ymf288;
    memset(resampler->buf, 0, sizeof(resampler->buf));
  }
}

static inline void opna_ssg_set_mix(struct opna_ssg *ssg, uint32_t mix) {
  // 0x10000: default (PC-9801-86 equivalent)
  // only valid when !ymf288
  ssg->mix = mix;
}

typedef void (*opna_ssg_sinc_calc_func_type)(unsigned resampler_index,
                                             const int16_t *inbuf, int32_t *outbuf);
extern opna_ssg_sinc_calc_func_type opna_ssg_sinc_calc_func;
void opna_ssg_sinc_calc_c(unsigned resampler_index,
                          const int16_t *inbuf, int32_t *outbuf) __attribute__((hot, optimize(3)));
void opna_ssg_sinc_calc_neon(unsigned, const int16_t *, int32_t *);
void opna_ssg_sinc_calc_sse2(unsigned, const int16_t *, int32_t *) __attribute__((hot, optimize(3)));

extern const int16_t opna_ssg_sinctable[OPNA_SSG_SINCTABLELEN*2];

#ifdef __cplusplus
}
#endif

#endif // LIBOPNA_OPNASSG_H_INCLUDED
