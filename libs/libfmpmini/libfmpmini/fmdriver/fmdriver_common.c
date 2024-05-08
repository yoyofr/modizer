#include "fmdriver/fmdriver_common.h"

uint8_t fmdriver_fm_freq2key(uint16_t freq) {
  int block = freq >> (8+3);
  int f_num = freq & ((1<<(8+3))-1);
  if (!f_num) return 0x00;
  while (!(f_num & (1<<(8+3-1)))) {
    f_num <<= 1;
    block--;
  }
  static const uint16_t freqtab[0xc] = {
    0x042e, // < 9 (a)
    0x046e,
    0x04b1,
    0x04f9,
    0x0544,
    0x0595,
    0x05ea,
    0x0644,
    0x06a3,
    0x0708,
    0x0773,
    0x07e4,
  };
  int note = 0;
  for (; note < 12; note++) {
    if (f_num < freqtab[note]) break;
  }
  note += 9;
  block += (note/12);
  note %= 12;

  if (block < 0) return 0x00;
  if (block > 8) return 0x8b;
  return (block << 4) | note;
}

uint8_t fmdriver_ssg_freq2key(uint16_t freq) {
  if (!freq) return 0x00;
  int octave = -5;
  while (!(freq & 0x8000)) {
    freq <<= 1;
    octave++;
  }
  // 7987200.0 / (64*440*(2**((i+2+0.5)/12))/(1<<8))
  static const uint16_t freqtab[0xc] = {
    0xf57f, // > 0 (c)
    0xe7b8,
    0xdab7,
    0xce70,
    0xc2da,
    0xb7ea,
    0xad98,
    0xa3da,
    0x9aa7,
    0x91f9,
    0x89c8,
    0x820c,
  };
  int note = 0;
  for (; note < 12; note++) {
    if (freq > freqtab[note]) break;
  }
  note += 11;
  octave += (note/12);
  note %= 12;

  if (octave < 0) return 0x00;
  if (octave > 8) return 0x8b;
  return (octave << 4) | note;
}

uint8_t fmdriver_ppz8_freq2key(uint32_t freq) {
  if (!freq) return 0x00;
  int octave = 16+4;
  while (!(freq & 0x80000000u)) {
    freq <<= 1;
    octave--;
  }
  if (octave < 0) return 0x00;
  if (octave > 8) return 0x8b;
  // round(0x8000*(2**((i+0.5)/12)))
  static const uint16_t freqtab[0xc] = {
    0x83c0,
    0x8b96,
    0x93e3,
    0x9cae,
    0xa5ff,
    0xafde,
    0xba53,
    0xc567,
    0xd124,
    0xdd94,
    0xeac1,
    0xf8b6
  };
  uint16_t freqhigh = freq >> 16;
  int note = 0;
  for (; note < 12; note++) {
    if (freqhigh < freqtab[note]) break;
  }
  return (octave << 4) | note;
}
