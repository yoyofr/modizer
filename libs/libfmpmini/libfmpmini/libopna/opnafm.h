#ifndef LIBOPNA_OPNAFM_H_INCLUDED
#define LIBOPNA_OPNAFM_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#ifdef LIBOPNA_ENABLE_LEVELDATA
#include "leveldata/leveldata.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LIBOPNA_FM_ENV_MAX 1023
enum {
  ENV_ATTACK,
  ENV_DECAY,
  ENV_SUSTAIN,
  ENV_RELEASE,
  ENV_OFF,
};

struct opna_fm_slot {
  // 20bits, upper 10 bits will be the index to sine table
  uint32_t phase;
  // 10 bits
  uint16_t env;
  // 12 bits
  uint16_t env_hires;
  uint16_t env_count;
  uint8_t env_state;
  uint8_t rate_shifter;
  uint8_t rate_selector;
  uint8_t rate_mul;

  uint8_t rate_shifter_hires;
  uint8_t rate_selector_hires;
  uint8_t rate_mul_hires;

  uint8_t tl;
  uint8_t sl;

  uint8_t ar;
  uint8_t dr;
  uint8_t sr;
  uint8_t rr;

  uint8_t mul;
  uint8_t det;
  uint8_t ks;

  uint8_t keycode;

  // set with opna_write
  bool keyon_ext;
  // synchronized with env update (once per 3 samples)
  bool keyon;
  // set when opna_fm_slotout called
  int16_t prevout;
};

struct opna_fm_channel {
  struct opna_fm_slot slot[4];

  // save 2 samples for slot 1 feedback
  uint16_t fbmem;
  // save sample for long (>2) chain of slots
  uint16_t alg_mem;

  uint8_t alg;
  uint8_t fb;
  uint16_t fnum;
  uint8_t blk;
#ifdef LIBOPNA_ENABLE_LEVELDATA
  struct leveldata leveldata;
#endif
};

struct opna_fm {
  struct opna_fm_channel channel[6];

  // remember here what was written on higher byte,
  // actually write when lower byte written
  uint8_t blkfnum_h;
  // channel 3 blk, fnum
  struct {
    uint16_t fnum[3];
    uint8_t blk[3];
    uint8_t mode;
  } ch3;

  // do envelope once per 3 samples
  uint8_t env_div3;

  // pan
  bool lselect[6];
  bool rselect[6];

  // mask
  // when (1<<channel), the channel is masked
  unsigned mask;
  
  bool hires_sin;
  bool hires_env;
};

void opna_fm_reset(struct opna_fm *fm);
struct oscillodata;
void opna_fm_mix(struct opna_fm *fm, int16_t *buf, unsigned samples, struct oscillodata *oscillo, unsigned offset);
void opna_fm_writereg(struct opna_fm *fm, unsigned reg, unsigned val);

//
void opna_fm_chan_reset(struct opna_fm_channel *chan);
void opna_fm_chan_phase(struct opna_fm_channel *chan);
void opna_fm_slot_env(struct opna_fm_slot *slot, bool hires_env);
void opna_fm_chan_set_blkfnum(struct opna_fm_channel *chan, unsigned blk, unsigned fnum);

struct opna_fm_frame {
  int16_t data[2];
};

struct opna_fm_frame opna_fm_chanout(struct opna_fm_channel *chan, bool hires_sin, bool hires_env);
void opna_fm_slot_key(struct opna_fm_channel *chan, int slotnum, bool keyon);

void opna_fm_chan_set_alg(struct opna_fm_channel *chan, unsigned alg);
void opna_fm_chan_set_fb(struct opna_fm_channel *chan, unsigned fb);
void opna_fm_slot_set_ar(struct opna_fm_slot *slot, unsigned ar);
void opna_fm_slot_set_dr(struct opna_fm_slot *slot, unsigned dr);
void opna_fm_slot_set_sr(struct opna_fm_slot *slot, unsigned sr);
void opna_fm_slot_set_rr(struct opna_fm_slot *slot, unsigned rr);
void opna_fm_slot_set_sl(struct opna_fm_slot *slot, unsigned sl);
void opna_fm_slot_set_tl(struct opna_fm_slot *slot, unsigned tl);
void opna_fm_slot_set_ks(struct opna_fm_slot *slot, unsigned ks);
void opna_fm_slot_set_mul(struct opna_fm_slot *slot, unsigned mul);
void opna_fm_slot_set_det(struct opna_fm_slot *slot, unsigned det);

static inline void opna_fm_set_hires_sin(struct opna_fm *fm, bool hires) {
  fm->hires_sin = hires;
}

static inline void opna_fm_set_hires_env(struct opna_fm *fm, bool hires) {
  fm->hires_env = hires;
}

#ifdef __cplusplus
}
#endif

#endif /* LIBOPNA_OPNAFM_H_INCLUDED */
