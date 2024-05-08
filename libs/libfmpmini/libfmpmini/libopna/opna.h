#ifndef LIBOPNA_OPNA_H_INCLUDED
#define LIBOPNA_OPNA_H_INCLUDED

#include "opnafm.h"
#include "opnassg.h"
#include "opnadrum.h"
#include "opnaadpcm.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  LIBOPNA_CHAN_FM_1 = 0x0001,
  LIBOPNA_CHAN_FM_2 = 0x0002,
  LIBOPNA_CHAN_FM_3 = 0x0004,
  LIBOPNA_CHAN_FM_4 = 0x0008,
  LIBOPNA_CHAN_FM_5 = 0x0010,
  LIBOPNA_CHAN_FM_6 = 0x0020,
  LIBOPNA_CHAN_SSG_1 = 0x0040,
  LIBOPNA_CHAN_SSG_2 = 0x0080,
  LIBOPNA_CHAN_SSG_3 = 0x0100,
  LIBOPNA_CHAN_DRUM_BD = 0x0200,
  LIBOPNA_CHAN_DRUM_SD = 0x0400,
  LIBOPNA_CHAN_DRUM_TOP = 0x0800,
  LIBOPNA_CHAN_DRUM_HH = 0x1000,
  LIBOPNA_CHAN_DRUM_TOM = 0x2000,
  LIBOPNA_CHAN_DRUM_RIM = 0x4000,
  LIBOPNA_CHAN_DRUM_ALL = 0x7e00,
  LIBOPNA_CHAN_ADPCM = 0x8000,
};

enum {
  LIBOPNA_OSCILLO_TRACK_COUNT = 11
};

struct opna {
  struct opna_fm fm;
  struct opna_ssg ssg;
  struct opna_drum drum;
  struct opna_adpcm adpcm;
  struct opna_ssg_resampler resampler;
  unsigned mask;
  uint64_t generated_frames;
};

void opna_reset(struct opna *opna);
void opna_writereg(struct opna *opna, unsigned reg, unsigned val);
unsigned opna_readreg(const struct opna *opna, unsigned reg);
void opna_mix(struct opna *opna, int16_t *buf, unsigned samples);
struct oscillodata;
void opna_mix_oscillo(struct opna *opna, int16_t *buf, unsigned samples, struct oscillodata *oscillo);
unsigned opna_get_mask(const struct opna *opna);
void opna_set_mask(struct opna *opna, unsigned mask);

#ifdef __cplusplus
}
#endif

#endif // LIBOPNA_OPNA_H_INCLUDED
