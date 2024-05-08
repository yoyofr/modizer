#include "opnaadpcm.h"

enum {
  C1_START = 0x80,
  C1_REC = 0x40,
  C1_MEMEXT = 0x20,
  C1_REPEAT = 0x10,
  C1_RESET = 0x01,
  C1_MASK = 0xf9,
};

enum {
  C2_L = 0x80,
  C2_R = 0x40,
  C2_8BIT = 0x02,
  C2_MASK = 0xcf,
};

static const uint8_t adpcm_table[8] = {
  57, 57, 57, 57, 77, 102, 128, 153,
};

void opna_adpcm_reset(struct opna_adpcm *adpcm) {
  adpcm->control1 = 0;
  adpcm->control2 = 0;
  adpcm->vol = 0;
  adpcm->delta = 0;
  adpcm->start = 0;
  adpcm->end = 0;
  adpcm->limit = 0;
  adpcm->ramptr = 0;
  adpcm->step = 0;
  adpcm->ram = 0;
  adpcm->acc = 0;
  adpcm->prev_acc = 0;
  adpcm->adpcmd = 127;
  adpcm->out = 0;
#ifdef LIBOPNA_ENABLE_LEVELDATA
  leveldata_init(&adpcm->leveldata);
#endif
}

static uint32_t addr_conv(const struct opna_adpcm *adpcm, uint16_t a) {
  uint32_t a32 = a;
  return (adpcm->control2 & C2_8BIT) ? (a32<<6) : (a32<<3);
}
static uint32_t addr_conv_e(const struct opna_adpcm *adpcm, uint16_t a) {
  uint32_t a32 = a+1;
  uint32_t ret = (adpcm->control2 & C2_8BIT) ? (a32<<6) : (a32<<3);
  return ret-1;
}

static void adpcm_calc(struct opna_adpcm *adpcm) {
  uint32_t step = (uint32_t)adpcm->step + (uint32_t)adpcm->delta;
  adpcm->step = step & 0xffff;
  if (step >> 16) {
    if (adpcm->ramptr == addr_conv(adpcm, adpcm->limit)) {
      adpcm->ramptr = 0;
    }
    if (adpcm->ramptr == addr_conv_e(adpcm, adpcm->end)) {
      if (adpcm->control1 & C1_REPEAT) {
        adpcm->ramptr = addr_conv(adpcm, adpcm->start);
        adpcm->acc = 0;
        adpcm->adpcmd = 127;
        adpcm->prev_acc = 0;
      } else {
        // TODO: set EOS
        adpcm->control1 = 0;
        adpcm->out = 0;
        adpcm->prev_acc = 0;
      }
    }
    uint8_t data = 0;
    if (adpcm->ram) {
      data = adpcm->ram[(adpcm->ramptr>>1)&(OPNA_ADPCM_RAM_SIZE-1)];
    }
    if (adpcm->ramptr&1) {
      data &= 0x0f;
    } else {
      data >>= 4;
    }
    adpcm->ramptr++;
    adpcm->ramptr &= (1<<(24+1))-1;

    adpcm->prev_acc = adpcm->acc;
    int32_t acc_d = (((data&7)<<1)|1);
    if (data&8) acc_d = -acc_d;
    int32_t acc = adpcm->acc + (acc_d * adpcm->adpcmd / 8);
    if (acc < -32768) acc = -32768;
    if (acc > 32767) acc = 32767;
    adpcm->acc = acc;

    uint32_t adpcmd = (adpcm->adpcmd * adpcm_table[data&7] / 64);
    if (adpcmd < 127) adpcmd = 127;
    if (adpcmd > 24576) adpcmd = 24576;
    adpcm->adpcmd = adpcmd;
  }
  int32_t out = (int32_t)adpcm->prev_acc * (0x10000-adpcm->step);
  out += (int32_t)adpcm->acc * adpcm->step;
  out >>= 16;
  out *= adpcm->vol;
  out >>= 8;
  if (out < -32768) out = -32768;
  if (out > 32767) out = 32767;
  adpcm->out = out;
}

void opna_adpcm_writereg(struct opna_adpcm *adpcm, unsigned reg, unsigned val) {
  val &= 0xff;
  if (reg < 0x100) return;
  if (reg >= 0x111) return;
  reg &= 0xff;
  switch (reg) {
  case 0x00:
    adpcm->control1 = val & C1_MASK;
    if (adpcm->control1 & C1_START) {
      adpcm->step = 0;
      adpcm->acc = 0;
      adpcm->prev_acc = 0;
      adpcm->out = 0;
      adpcm->adpcmd = 127;
    }
    if (adpcm->control1 & C1_MEMEXT) {
      adpcm->ramptr = addr_conv(adpcm, adpcm->start);
    }
    if (adpcm->control1 & C1_RESET) {
      adpcm->control1 = 0;
      // TODO: set BRDY
    }
    break;
  case 0x01:
    adpcm->control2 = val & C2_MASK;
    break;
  case 0x02:
    adpcm->start &= 0xff00;
    adpcm->start |= val;
    break;
  case 0x03:
    adpcm->start &= 0x00ff;
    adpcm->start |= (val<<8);
    break;
  case 0x04:
    adpcm->end &= 0xff00;
    adpcm->end |= val;
    break;
  case 0x05:
    adpcm->end &= 0x00ff;
    adpcm->end |= (val<<8);
    break;
  case 0x08:
    // data write
    if ((adpcm->control1 & (C1_START|C1_REC|C1_MEMEXT)) == (C1_REC|C1_MEMEXT)) {
      // external memory write
      if (adpcm->ramptr != addr_conv_e(adpcm, adpcm->end)) {
        if (adpcm->ram) {
          adpcm->ram[(adpcm->ramptr>>1)&(OPNA_ADPCM_RAM_SIZE-1)] = val;
        }
        adpcm->ramptr += 2;
      } else {
        // TODO: set EOS
      }
    }
    break;
  case 0x09:
    adpcm->delta &= 0xff00;
    adpcm->delta |= val;
    break;
  case 0x0a:
    adpcm->delta &= 0x00ff;
    adpcm->delta |= (val<<8);
    break;
  case 0x0b:
    adpcm->vol = val;
    break;
  case 0x0c:
    adpcm->limit &= 0xff00;
    adpcm->limit |= val;
    break;
  case 0x0d:
    adpcm->limit &= 0x00ff;
    adpcm->limit |= (val<<8);
    break;
  }
}

void opna_adpcm_mix(struct opna_adpcm *adpcm, int16_t *buf, unsigned samples) {
  unsigned level = 0;
  if (!adpcm->ram || !(adpcm->control1 & C1_START)) {
#ifdef LIBOPNA_ENABLE_LEVELDATA
    leveldata_update(&adpcm->leveldata, level);
#endif
    return;
  }
  for (unsigned i = 0; i < samples; i++) {
    adpcm_calc(adpcm);
    {
      int clevel = adpcm->out>>1;
      if (clevel < 0) clevel = -clevel;
      if (((unsigned)clevel) > level) level = clevel;
    }
    if (!adpcm->masked) {
      int32_t lo = buf[i*2+0];
      int32_t ro = buf[i*2+1];
      if (adpcm->control2 & C2_L) lo += (adpcm->out>>1);
      if (adpcm->control2 & C2_R) ro += (adpcm->out>>1);
      if (lo < INT16_MIN) lo = INT16_MIN;
      if (lo > INT16_MAX) lo = INT16_MAX;
      if (ro < INT16_MIN) ro = INT16_MIN;
      if (ro > INT16_MAX) ro = INT16_MAX;
      buf[i*2+0] = lo;
      buf[i*2+1] = ro;
    }
    if (!(adpcm->control1 & C1_START)) return;
  }
#ifdef LIBOPNA_ENABLE_LEVELDATA
  leveldata_update(&adpcm->leveldata, level);
#endif
}

void opna_adpcm_set_ram_256k(struct opna_adpcm *adpcm, void *ram) {
  adpcm->ram = ram;
}

void *opna_adpcm_get_ram(struct opna_adpcm *adpcm) {
  return adpcm->ram;
}
