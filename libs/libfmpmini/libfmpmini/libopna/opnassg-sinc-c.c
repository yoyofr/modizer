#include "opnassg.h"

void opna_ssg_sinc_calc_c(unsigned resampler_index, const int16_t *inbuf, int32_t *outbuf) {
  for (int c = 0; c < 3; c++) {
    int32_t chsample = 0;
    for (int j = 0; j < OPNA_SSG_SINCTABLELEN; j++) {
      unsigned sincindex = j;
      if (!(resampler_index&1)) sincindex += OPNA_SSG_SINCTABLELEN;
      chsample += inbuf[(((resampler_index)>>1)+j)*4+c] * opna_ssg_sinctable[sincindex];
    }
    outbuf[c] = chsample;
  }
}
