#include "libopna/opnassg.h"
#include <emmintrin.h>

void opna_ssg_sinc_calc_sse2(unsigned resampler_index, const int16_t *inbuf, int32_t *outbuf) {
  inbuf += (resampler_index >> 1) * 4;
  const int16_t *sinctable = opna_ssg_sinctable;
  if (!(resampler_index & 1u)) sinctable += OPNA_SSG_SINCTABLELEN;
  __m128i outacc[3];
  for (int c = 0; c < 3; c++) {
    outacc[c] = _mm_setzero_si128();
  }
  for (int j = 0; j < 16; j++) {
    // 8 samples per loop
    __m128i in4[4];
    for (int i = 0; i < 4; i++) {
      in4[i] = _mm_loadu_si128((__m128i *)&inbuf[(j*4+i)*8]);
    }
    // in4: x 2 1 0 x 2 1 0
    for (int i = 0; i < 4; i++) {
      in4[i] = _mm_shuffle_epi32(in4[i], 0xd8);
    }
    // in4: x 2 x 2 1 0 1 0
    for (int i = 0; i < 4; i++) {
      in4[i] = _mm_shufflehi_epi16(in4[i], 0xd8);
    }
    for (int i = 0; i < 4; i++) {
      in4[i] = _mm_shufflelo_epi16(in4[i], 0xd8);
    }
    // in4: x x 2 2 1 1 0 0
    __m128i inl[2], inh[2];
    for (int i = 0; i < 2; i++) {
      inl[i] = _mm_unpacklo_epi32(in4[i*2+0], in4[i*2+1]);
      inh[i] = _mm_unpackhi_epi32(in4[i*2+0], in4[i*2+1]);
    }
    // inl: 1 1 1 1 0 0 0 0
    // inh: x x x x 2 2 2 2
    __m128i in[3];
    in[0] = _mm_unpacklo_epi64(inl[0], inl[1]);
    in[1] = _mm_unpackhi_epi64(inl[0], inl[1]);
    in[2] = _mm_unpacklo_epi64(inh[0], inh[1]);
    __m128i sinc = _mm_loadu_si128((__m128i *)&sinctable[j*8]);
    __m128i out[3];
    for (int c = 0; c < 3; c++) {
      out[c] = _mm_madd_epi16(in[c], sinc);
    }
    for (int c = 0; c < 3; c++) {
      outacc[c] = _mm_add_epi32(outacc[c], out[c]);
    }
  }
  for (int c = 0; c < 3; c++) {
    outacc[c] = _mm_add_epi32(outacc[c], _mm_shuffle_epi32(outacc[c], 0x4e));
    outacc[c] = _mm_add_epi32(outacc[c], _mm_shuffle_epi32(outacc[c], 0xb1));
    outbuf[c] = ((uint32_t)_mm_extract_epi16(outacc[c], 0)) | (((uint32_t)_mm_extract_epi16(outacc[c], 1))<<16);
  }
}
