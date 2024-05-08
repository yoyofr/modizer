#ifndef LIBOPNA_OPNAADPCM_H_INCLUDED
#define LIBOPNA_OPNAADPCM_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#ifdef LIBOPNA_ENABLE_LEVELDATA
#include "leveldata/leveldata.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct opna_adpcm {
  uint8_t control1;
  uint8_t control2;
  uint8_t vol;
  uint16_t delta;
  uint16_t start;
  uint16_t end;
  uint16_t limit;
  uint32_t ramptr;
  uint16_t step;
  uint8_t *ram;
  int16_t acc;
  int16_t prev_acc;
  uint16_t adpcmd;
  int16_t out;
  bool masked;
#ifdef LIBOPNA_ENABLE_LEVELDATA
  struct leveldata leveldata;
#endif
};

void opna_adpcm_reset(struct opna_adpcm *adpcm);
void opna_adpcm_mix(struct opna_adpcm *adpcm, int16_t *buf, unsigned samples);
void opna_adpcm_writereg(struct opna_adpcm *adpcm, unsigned reg, unsigned val);

enum {
  OPNA_ADPCM_RAM_SIZE = (1<<18)
};

void opna_adpcm_set_ram_256k(struct opna_adpcm *adpcm, void *ram);
void *opna_adpcm_get_ram(struct opna_adpcm *adpcm);

#ifdef __cplusplus
}
#endif

#endif // LIBOPNA_OPNAADPCM_H_INCLUDED
