#ifndef MYON_FMPLAYER_OSCILLO_H_INCLUDED
#define MYON_FMPLAYER_OSCILLO_H_INCLUDED

#include <stdint.h>

enum {
  OSCILLO_SAMPLE_COUNT = 8192,
  OSCILLO_OFFSET_SHIFT = 10,
};

struct oscillodata {
  int16_t buf[OSCILLO_SAMPLE_COUNT];
  unsigned offset;
  
};

#endif // MYON_FMPLAYER_OSCILLO_H_INCLUDED
