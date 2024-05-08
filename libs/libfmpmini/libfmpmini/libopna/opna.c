#include "opna.h"
#ifdef LIBOPNA_ENABLE_OSCILLO
#include "oscillo/oscillo.h"
#endif
#include <string.h>

void opna_reset(struct opna *opna) {
  opna_fm_reset(&opna->fm);
  opna_ssg_reset(&opna->ssg);
  opna_ssg_resampler_reset(&opna->resampler);
  opna_drum_reset(&opna->drum);
  opna_adpcm_reset(&opna->adpcm);
  opna->mask = 0;
  opna->generated_frames = 0;
}

void opna_writereg(struct opna *opna, unsigned reg, unsigned val) {
  val &= 0xff;
  opna_fm_writereg(&opna->fm, reg, val);
  opna_ssg_writereg(&opna->ssg, reg, val);
  opna_drum_writereg(&opna->drum, reg, val);
  opna_adpcm_writereg(&opna->adpcm, reg, val);
}

unsigned opna_readreg(const struct opna *opna, unsigned reg) {
  if (reg > 0xfu) return 0xff;
  return opna_ssg_readreg(&opna->ssg, reg);
}

void opna_mix(struct opna *opna, int16_t *buf, unsigned samples) {
  opna_mix_oscillo(opna, buf, samples, 0);
}

void opna_mix_oscillo(struct opna *opna, int16_t *buf, unsigned samples, struct oscillodata *oscillo) {
#ifdef LIBOPNA_ENABLE_OSCILLO
  if (oscillo) {
    for (int i = 0; i < LIBOPNA_OSCILLO_TRACK_COUNT; i++) {
      memmove(&oscillo[i].buf[0],
              &oscillo[i].buf[samples],
              (OSCILLO_SAMPLE_COUNT - samples)*sizeof(oscillo[i].buf[0]));
    }
  }
  unsigned offset = OSCILLO_SAMPLE_COUNT - samples;
  struct oscillodata *oscillofm = oscillo ? &oscillo[0] : 0;
  struct oscillodata *oscillossg = oscillo ? &oscillo[6] : 0;
#else
  (void)oscillo;
  struct oscillodata *oscillofm = 0, *oscillossg = 0;
  unsigned offset = 0;
#endif
  opna_fm_mix(&opna->fm, buf, samples, oscillofm, offset);
  opna_ssg_mix_55466(&opna->ssg, &opna->resampler, buf, samples,
                     oscillossg, offset);
  opna_drum_mix(&opna->drum, buf, samples);
  opna_adpcm_mix(&opna->adpcm, buf, samples);
  opna->generated_frames += samples;
}

unsigned opna_get_mask(const struct opna *opna) {
  return opna->mask;
}

void opna_set_mask(struct opna *opna, unsigned mask) {
  opna->mask = mask & 0xffffu;
  opna->fm.mask = mask & ((1<<(6+1))-1);
  opna->ssg.mask = (mask >> 6) & ((1<<(3+1))-1);
  opna->adpcm.masked = mask & LIBOPNA_CHAN_ADPCM;
  opna->drum.mask = (mask >> 9) & ((1<<(6+1))-1);
}
