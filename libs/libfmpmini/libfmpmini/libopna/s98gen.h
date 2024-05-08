#ifndef MYON_S98GEN_H_INCLUDED
#define MYON_S98GEN_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "opna.h"

#ifdef __cplusplus
extern "C" {
#endif

struct s98gen {
  struct opna opna;
  uint8_t *s98data;
  size_t s98data_size;
  size_t current_offset;
  uint32_t opnaclock;
  uint32_t samples_to_generate;
  uint16_t samples_to_generate_frac;
  uint32_t timer_numerator;
  uint32_t timer_denominator;
};

// returns true if initialization succeeded
// returns false without touching *s98 when initialization failed (invalid data)
bool s98gen_init(struct s98gen *s98, void *s98data, size_t s98data_size);
bool s98gen_generate(struct s98gen *s98, int16_t *buf, size_t samples);

#ifdef __cplusplus
}
#endif

#endif // MYON_S98GEN_H_INCLUDED
