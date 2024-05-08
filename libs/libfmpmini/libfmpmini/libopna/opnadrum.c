#include "opnadrum.h"

static const uint16_t steps[49] = {
  16,  17,   19,   21,   23,   25,   28,
  31,  34,   37,   41,   45,   50,   55,
  60,  66,   73,   80,   88,   97,  107,
  118, 130,  143,  157,  173,  190,  209,
  230, 253,  279,  307,  337,  371,  408,
  449, 494,  544,  598,  658,  724,  796,
  876, 963, 1060, 1166, 1282, 1411, 1552
};

static const int8_t step_inc[8] = {
  -1, -1, -1, -1, 2, 5, 7, 9
};

void opna_drum_reset(struct opna_drum *drum) {
  for (int d = 0; d < 6; d++) {
    drum->drums[d].data = 0;
    drum->drums[d].playing = false;
    drum->drums[d].index = 0;
    drum->drums[d].len = 0;
    drum->drums[d].level = 0;
    drum->drums[d].left = false;
    drum->drums[d].right = false;
#ifdef LIBOPNA_ENABLE_LEVELDATA
    leveldata_init(&drum->drums[d].leveldata);
#endif
  }
  drum->total_level = 0;
  drum->mask = 0;
}

void opna_drum_set_rom(struct opna_drum *drum, void *romptr) {
  uint8_t *rom = (uint8_t *)romptr;
  static const struct {
    unsigned start;
    unsigned end;
    int div;
  } part[6] = {
    {OPNA_ROM_BD_START,  OPNA_ROM_SD_START-1,  3},
    {OPNA_ROM_SD_START,  OPNA_ROM_TOP_START-1, 3},
    {OPNA_ROM_TOP_START, OPNA_ROM_HH_START-1,  3},
    {OPNA_ROM_HH_START,  OPNA_ROM_TOM_START-1, 3},
    {OPNA_ROM_TOM_START, OPNA_ROM_RIM_START-1, 6},
    {OPNA_ROM_RIM_START, OPNA_ROM_SIZE-1,      6},
  };
  drum->drums[0].data = drum->rom_bd;
  drum->drums[1].data = drum->rom_sd;
  drum->drums[2].data = drum->rom_top;
  drum->drums[3].data = drum->rom_hh;
  drum->drums[4].data = drum->rom_tom;
  drum->drums[5].data = drum->rom_rim;
  for (int p = 0; p < 6; p++) {
    drum->drums[p].playing = false;
    drum->drums[p].index = 0;
    unsigned addr = part[p].start << 1;
    int step = 0;
    unsigned acc = 0;
    int outindex = 0;
    for (;;) {
      if ((addr>>1) == part[p].end) break;
      unsigned data = rom[addr>>1];
      if (!(addr&1)) data >>= 4;
      data &= ((1<<4)-1);
      int acc_diff = ((((data&7)<<1)|1) * steps[step]) >> 3;
      if (data&8) acc_diff = -acc_diff;
      acc += acc_diff;
      step += step_inc[data&7];
      if (step < 0) step = 0;
      if (step > 48) step = 48;
      addr++;

      int out = acc & ((1u<<12)-1);
      if (out >= (1<<11)) out -= (1<<12);
      int16_t out16 = out << 4;
      for (int i = 0; i < part[p].div; i++) {
        drum->drums[p].data[outindex] = out16;
        outindex++;
      }
    }
    drum->drums[p].len = outindex;
  }
}

void opna_drum_mix(struct opna_drum *drum, int16_t *buf, int samples) {
  unsigned levels[6] = {0};
  for (int i = 0; i < samples; i++) {
    int32_t lo = buf[i*2+0];
    int32_t ro = buf[i*2+1];
    for (int d = 0; d < 6; d++) {
      if (drum->drums[d].playing && drum->drums[d].data) {
        int co = drum->drums[d].data[drum->drums[d].index];
        co >>= 4;
        unsigned level = (drum->drums[d].level^0x1f) + (drum->total_level^0x3f);
        co *= 15 - (level&7);
        co >>= 1+(level>>3);
        unsigned outlevel = co > 0 ? co : -co;
        if (!drum->drums[d].left && !drum->drums[d].right) outlevel = 0;
        if (outlevel > levels[d]) levels[d] = outlevel;
        if (!(drum->mask & (1u << d))) {
          if (drum->drums[d].left) lo += co;
          if (drum->drums[d].right) ro += co;
        }
        drum->drums[d].index++;
        if (drum->drums[d].index == drum->drums[d].len) {
          drum->drums[d].index = 0;
          drum->drums[d].playing = false;
        }
      }
    }
    if (lo < INT16_MIN) lo = INT16_MIN;
    if (lo > INT16_MAX) lo = INT16_MAX;
    if (ro < INT16_MIN) ro = INT16_MIN;
    if (ro > INT16_MAX) ro = INT16_MAX;
    buf[i*2+0] = lo;
    buf[i*2+1] = ro;
  }
#ifdef LIBOPNA_ENABLE_LEVELDATA
  for (int d = 0; d < 6; d++) {
    leveldata_update(&drum->drums[d].leveldata, levels[d]);
  }
#endif
}

void opna_drum_writereg(struct opna_drum *drum, unsigned reg, unsigned val) {
  val &= 0xff;
  switch (reg) {
  case 0x10:
    for (int d = 0; d < 6; d++) {
      if (val & (1<<d)) {
        drum->drums[d].playing = !(val & 0x80);
        drum->drums[d].index = 0;
      }
    }
    break;
  case 0x11:
    drum->total_level = val & 0x3f;
    break;
  case 0x18:
  case 0x19:
  case 0x1a:
  case 0x1b:
  case 0x1c:
  case 0x1d:
    {
      int d = reg - 0x18;
      drum->drums[d].left = val & 0x80;
      drum->drums[d].right = val & 0x40;
      drum->drums[d].level = val & 0x1f;
    }
    break;
  default:
    break;
  }
}
