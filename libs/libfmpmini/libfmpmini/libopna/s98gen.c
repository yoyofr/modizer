#include "s98gen.h"

static uint32_t read32le(const void *dataptr, size_t offset) {
  const uint8_t *data = (const uint8_t *)dataptr;
  return ((uint32_t)data[offset+0]) | (((uint32_t)data[offset+1])<<8) |
         (((uint32_t)data[offset+2])<<16) | (((uint32_t)data[offset+3])<<24);
}

bool s98gen_init(struct s98gen *s98, void *s98dataptr, size_t s98data_size) {
  uint8_t *s98data = (uint8_t *)s98dataptr;
  if (s98data_size < 0x20) {
    // size of s98 header
    return false;
  }
  if (s98data[0] != 'S' || s98data[1] != '9' ||
      s98data[2] != '8') {
    // file magic / version check
    return false;
  }
  if ((s98data[3] != '1') && (s98data[3] != '3')) {
    return false;
  }
  if (s98data[0xc] != 0) {
    // COMPRESSING flag not zero
    return false;
  }
  uint32_t clock = 7987200;
  if ((s98data[3] == '3') && s98data[0x1c]) {
    if (s98data_size < 0x30) {
      // size of s98 header + device info
      return false;
    }
    uint32_t devicetype = read32le(s98data, 0x20);
    if (devicetype != 0x04 && devicetype != 0x02) {
      // OPN / OPNA only
      return false;
    }
    clock = read32le(s98data, 0x24);
    // convert OPN clock to OPNA clock
    if (devicetype == 2) clock *= 2;
  }
  s98->s98data = s98data;
  s98->s98data_size = s98data_size;
  s98->current_offset = read32le(s98data, 0x14);
  s98->opnaclock = clock;
  s98->samples_to_generate = 0;
  s98->samples_to_generate_frac = 0;
  s98->timer_numerator = read32le(s98data, 0x04);
  s98->timer_denominator = read32le(s98data, 0x08);
  if (!s98->timer_numerator) s98->timer_numerator = 10;
  if (!s98->timer_denominator) s98->timer_denominator = 1000;
  opna_reset(&s98->opna);
  return true;
}

// returns 0 when failed
static size_t s98gen_getvv(struct s98gen *s98) {
  unsigned shift = 0;
  size_t value = 0;
  do {
    if (s98->s98data_size < (s98->current_offset+1)) return 0;
    value |= (s98->s98data[s98->current_offset] & 0x7f) << ((shift++)*7);
  } while (s98->s98data[s98->current_offset++] & 0x80);
  return value + 2;
}

static void s98gen_set_samples_to_generate(struct s98gen *s98, size_t ticks) {
  uint64_t s = s98->opnaclock * s98->timer_numerator * ticks;
  s <<= 16;
  s /= 144 * s98->timer_denominator;
  s += s98->samples_to_generate_frac;
  s98->samples_to_generate = s >> 16;
  s98->samples_to_generate_frac = s & ((((size_t)1)<<16)-1);
}

static bool s98gen_parse_s98(struct s98gen *s98) {
  for (;;) {
    switch (s98->s98data[s98->current_offset]) {
    case 0x00:
      if (s98->s98data_size < (s98->current_offset + 3)) return false;
      opna_writereg(&s98->opna,
        s98->s98data[s98->current_offset+1],
        s98->s98data[s98->current_offset+2]);
      s98->current_offset += 3;
      break;
    case 0x01:
      if (s98->s98data_size < (s98->current_offset + 3)) return false;
      opna_writereg(&s98->opna,
        s98->s98data[s98->current_offset+1] | 0x100,
        s98->s98data[s98->current_offset+2]);
      s98->current_offset += 3;
      break;
    case 0xfe:
      if (s98->s98data_size < (s98->current_offset+1)) return false;
      s98->current_offset++;
      {
        size_t vv = s98gen_getvv(s98);
        if (!vv) return false;
        s98gen_set_samples_to_generate(s98, vv);
      }
      return true;
    case 0xff:
      if (s98->s98data_size < (s98->current_offset + 1)) return false;
      s98->current_offset++;
      s98gen_set_samples_to_generate(s98, 1);
      return true;
    default:
      return false;
    }
  }
}

bool s98gen_generate(struct s98gen *s98, int16_t *buf, size_t samples) {
  for (size_t i = 0; i < samples; i++) {
    buf[i*2+0] = 0;
    buf[i*2+1] = 0;
  }
  if (samples <= s98->samples_to_generate) {
    opna_mix(&s98->opna, buf, samples);
    s98->samples_to_generate -= samples;
  } else {
    opna_mix(&s98->opna, buf, s98->samples_to_generate);
    buf += s98->samples_to_generate * 2;
    samples -= s98->samples_to_generate;
    s98->samples_to_generate = 0;
    while (samples) {
      if (!s98gen_parse_s98(s98)) return false;
      if (s98->samples_to_generate >= samples) {
        opna_mix(&s98->opna, buf, samples);
        s98->samples_to_generate -= samples;
        samples = 0;
      } else {
        opna_mix(&s98->opna, buf, s98->samples_to_generate);
        buf += s98->samples_to_generate*2;
        samples -= s98->samples_to_generate;
        s98->samples_to_generate = 0;
      }
    }
  }
  return true;
}
