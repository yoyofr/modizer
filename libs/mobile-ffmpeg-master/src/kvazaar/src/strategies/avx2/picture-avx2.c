/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Kvazaar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

/*
 * \file
 */

#include "global.h"

#if COMPILE_INTEL_AVX2
#include "strategies/avx2/picture-avx2.h"
#include "strategies/avx2/reg_sad_pow2_widths-avx2.h"

#include <immintrin.h>
#include <emmintrin.h>
#include <mmintrin.h>
#include <xmmintrin.h>
#include <string.h>
#include "kvazaar.h"
#include "strategies/strategies-picture.h"
#include "strategyselector.h"
#include "strategies/generic/picture-generic.h"

/**
 * \brief Calculate Sum of Absolute Differences (SAD)
 *
 * Calculate Sum of Absolute Differences (SAD) between two rectangular regions
 * located in arbitrary points in the picture.
 *
 * \param data1   Starting point of the first picture.
 * \param data2   Starting point of the second picture.
 * \param width   Width of the region for which SAD is calculated.
 * \param height  Height of the region for which SAD is calculated.
 * \param stride  Width of the pixel array.
 *
 * \returns Sum of Absolute Differences
 */
uint32_t kvz_reg_sad_avx2(const kvz_pixel * const data1, const kvz_pixel * const data2,
                          const int width, const int height, const unsigned stride1, const unsigned stride2)
{
  if (width == 0)
    return 0;
  if (width == 4)
    return reg_sad_w4(data1, data2, height, stride1, stride2);
  if (width == 8)
    return reg_sad_w8(data1, data2, height, stride1, stride2);
  if (width == 12)
    return reg_sad_w12(data1, data2, height, stride1, stride2);
  if (width == 16)
    return reg_sad_w16(data1, data2, height, stride1, stride2);
  if (width == 24)
    return reg_sad_w24(data1, data2, height, stride1, stride2);
  if (width == 32)
    return reg_sad_w32(data1, data2, height, stride1, stride2);
  if (width == 64)
    return reg_sad_w64(data1, data2, height, stride1, stride2);
  else
    return reg_sad_arbitrary(data1, data2, width, height, stride1, stride2);
}

/**
* \brief Calculate SAD for 8x8 bytes in continuous memory.
*/
static INLINE __m256i inline_8bit_sad_8x8_avx2(const __m256i *const a, const __m256i *const b)
{
  __m256i sum0, sum1;
  sum0 = _mm256_sad_epu8(_mm256_load_si256(a + 0), _mm256_load_si256(b + 0));
  sum1 = _mm256_sad_epu8(_mm256_load_si256(a + 1), _mm256_load_si256(b + 1));

  return _mm256_add_epi32(sum0, sum1);
}


/**
* \brief Calculate SAD for 16x16 bytes in continuous memory.
*/
static INLINE __m256i inline_8bit_sad_16x16_avx2(const __m256i *const a, const __m256i *const b)
{
  const unsigned size_of_8x8 = 8 * 8 / sizeof(__m256i);

  // Calculate in 4 chunks of 16x4.
  __m256i sum0, sum1, sum2, sum3;
  sum0 = inline_8bit_sad_8x8_avx2(a + 0 * size_of_8x8, b + 0 * size_of_8x8);
  sum1 = inline_8bit_sad_8x8_avx2(a + 1 * size_of_8x8, b + 1 * size_of_8x8);
  sum2 = inline_8bit_sad_8x8_avx2(a + 2 * size_of_8x8, b + 2 * size_of_8x8);
  sum3 = inline_8bit_sad_8x8_avx2(a + 3 * size_of_8x8, b + 3 * size_of_8x8);

  sum0 = _mm256_add_epi32(sum0, sum1);
  sum2 = _mm256_add_epi32(sum2, sum3);

  return _mm256_add_epi32(sum0, sum2);
}


/**
* \brief Get sum of the low 32 bits of four 64 bit numbers from __m256i as uint32_t.
*/
static INLINE uint32_t m256i_horizontal_sum(const __m256i sum)
{
  // Add the high 128 bits to low 128 bits.
  __m128i mm128_result = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extractf128_si256(sum, 1));
  // Add the high 64 bits  to low 64 bits.
  uint32_t result[4];
  _mm_storeu_si128((__m128i*)result, mm128_result);
  return result[0] + result[2];
}


static unsigned sad_8bit_8x8_avx2(const kvz_pixel *buf1, const kvz_pixel *buf2)
{
  const __m256i *const a = (const __m256i *)buf1;
  const __m256i *const b = (const __m256i *)buf2;
  __m256i sum = inline_8bit_sad_8x8_avx2(a, b);

  return m256i_horizontal_sum(sum);
}


static unsigned sad_8bit_16x16_avx2(const kvz_pixel *buf1, const kvz_pixel *buf2)
{
  const __m256i *const a = (const __m256i *)buf1;
  const __m256i *const b = (const __m256i *)buf2;
  __m256i sum = inline_8bit_sad_16x16_avx2(a, b);

  return m256i_horizontal_sum(sum);
}


static unsigned sad_8bit_32x32_avx2(const kvz_pixel *buf1, const kvz_pixel *buf2)
{
  const __m256i *const a = (const __m256i *)buf1;
  const __m256i *const b = (const __m256i *)buf2;

  const unsigned size_of_8x8 = 8 * 8 / sizeof(__m256i);
  const unsigned size_of_32x32 = 32 * 32 / sizeof(__m256i);

  // Looping 512 bytes at a time seems faster than letting VC figure it out
  // through inlining, like inline_8bit_sad_16x16_avx2 does.
  __m256i sum0 = inline_8bit_sad_8x8_avx2(a, b);
  for (unsigned i = size_of_8x8; i < size_of_32x32; i += size_of_8x8) {
    __m256i sum1 = inline_8bit_sad_8x8_avx2(a + i, b + i);
    sum0 = _mm256_add_epi32(sum0, sum1);
  }

  return m256i_horizontal_sum(sum0);
}


static unsigned sad_8bit_64x64_avx2(const kvz_pixel * buf1, const kvz_pixel * buf2)
{
  const __m256i *const a = (const __m256i *)buf1;
  const __m256i *const b = (const __m256i *)buf2;

  const unsigned size_of_8x8 = 8 * 8 / sizeof(__m256i);
  const unsigned size_of_64x64 = 64 * 64 / sizeof(__m256i);

  // Looping 512 bytes at a time seems faster than letting VC figure it out
  // through inlining, like inline_8bit_sad_16x16_avx2 does.
  __m256i sum0 = inline_8bit_sad_8x8_avx2(a, b);
  for (unsigned i = size_of_8x8; i < size_of_64x64; i += size_of_8x8) {
    __m256i sum1 = inline_8bit_sad_8x8_avx2(a + i, b + i);
    sum0 = _mm256_add_epi32(sum0, sum1);
  }

  return m256i_horizontal_sum(sum0);
}

static unsigned satd_4x4_8bit_avx2(const kvz_pixel *org, const kvz_pixel *cur)
{

  __m128i original = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)org));
  __m128i current = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)cur));

  __m128i diff_lo = _mm_sub_epi16(current, original);

  original = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(org + 8)));
  current = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(cur + 8)));

  __m128i diff_hi = _mm_sub_epi16(current, original);


  //Hor
  __m128i row0 = _mm_hadd_epi16(diff_lo, diff_hi);
  __m128i row1 = _mm_hsub_epi16(diff_lo, diff_hi);

  __m128i row2 = _mm_hadd_epi16(row0, row1);
  __m128i row3 = _mm_hsub_epi16(row0, row1);

  //Ver
  row0 = _mm_hadd_epi16(row2, row3);
  row1 = _mm_hsub_epi16(row2, row3);

  row2 = _mm_hadd_epi16(row0, row1);
  row3 = _mm_hsub_epi16(row0, row1);

  //Abs and sum
  row2 = _mm_abs_epi16(row2);
  row3 = _mm_abs_epi16(row3);

  row3 = _mm_add_epi16(row2, row3);

  row3 = _mm_add_epi16(row3, _mm_shuffle_epi32(row3, _MM_SHUFFLE(1, 0, 3, 2) ));
  row3 = _mm_add_epi16(row3, _mm_shuffle_epi32(row3, _MM_SHUFFLE(0, 1, 0, 1) ));
  row3 = _mm_add_epi16(row3, _mm_shufflelo_epi16(row3, _MM_SHUFFLE(0, 1, 0, 1) ));

  unsigned sum = _mm_extract_epi16(row3, 0);
  unsigned satd = (sum + 1) >> 1;

  return satd;
}


static void satd_8bit_4x4_dual_avx2(
  const pred_buffer preds, const kvz_pixel * const orig, unsigned num_modes, unsigned *satds_out) 
{

  __m256i original = _mm256_broadcastsi128_si256(_mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)orig)));
  __m256i pred = _mm256_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)preds[0]));
  pred = _mm256_inserti128_si256(pred, _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)preds[1])), 1);

  __m256i diff_lo = _mm256_sub_epi16(pred, original);

  original = _mm256_broadcastsi128_si256(_mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(orig + 8))));
  pred = _mm256_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(preds[0] + 8)));
  pred = _mm256_inserti128_si256(pred, _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)(preds[1] + 8))), 1);

  __m256i diff_hi = _mm256_sub_epi16(pred, original);

  //Hor
  __m256i row0 = _mm256_hadd_epi16(diff_lo, diff_hi);
  __m256i row1 = _mm256_hsub_epi16(diff_lo, diff_hi);

  __m256i row2 = _mm256_hadd_epi16(row0, row1);
  __m256i row3 = _mm256_hsub_epi16(row0, row1);

  //Ver
  row0 = _mm256_hadd_epi16(row2, row3);
  row1 = _mm256_hsub_epi16(row2, row3);

  row2 = _mm256_hadd_epi16(row0, row1);
  row3 = _mm256_hsub_epi16(row0, row1);

  //Abs and sum
  row2 = _mm256_abs_epi16(row2);
  row3 = _mm256_abs_epi16(row3);

  row3 = _mm256_add_epi16(row2, row3);

  row3 = _mm256_add_epi16(row3, _mm256_shuffle_epi32(row3, _MM_SHUFFLE(1, 0, 3, 2) ));
  row3 = _mm256_add_epi16(row3, _mm256_shuffle_epi32(row3, _MM_SHUFFLE(0, 1, 0, 1) ));
  row3 = _mm256_add_epi16(row3, _mm256_shufflelo_epi16(row3, _MM_SHUFFLE(0, 1, 0, 1) ));

  unsigned sum1 = _mm_extract_epi16(_mm256_castsi256_si128(row3), 0);
  sum1 = (sum1 + 1) >> 1;

  unsigned sum2 = _mm_extract_epi16(_mm256_extracti128_si256(row3, 1), 0);
  sum2 = (sum2 + 1) >> 1;

  satds_out[0] = sum1;
  satds_out[1] = sum2;
}

static INLINE void hor_transform_row_avx2(__m128i* row){
  
  __m128i mask_pos = _mm_set1_epi16(1);
  __m128i mask_neg = _mm_set1_epi16(-1);
  __m128i sign_mask = _mm_unpacklo_epi64(mask_pos, mask_neg);
  __m128i temp = _mm_shuffle_epi32(*row, _MM_SHUFFLE(1, 0, 3, 2));
  *row = _mm_sign_epi16(*row, sign_mask);
  *row = _mm_add_epi16(*row, temp);

  sign_mask = _mm_unpacklo_epi32(mask_pos, mask_neg);
  temp = _mm_shuffle_epi32(*row, _MM_SHUFFLE(2, 3, 0, 1));
  *row = _mm_sign_epi16(*row, sign_mask);
  *row = _mm_add_epi16(*row, temp);

  sign_mask = _mm_unpacklo_epi16(mask_pos, mask_neg);
  temp = _mm_shufflelo_epi16(*row, _MM_SHUFFLE(2,3,0,1));
  temp = _mm_shufflehi_epi16(temp, _MM_SHUFFLE(2,3,0,1));
  *row = _mm_sign_epi16(*row, sign_mask);
  *row = _mm_add_epi16(*row, temp);
}

static INLINE void hor_transform_row_dual_avx2(__m256i* row){
  
  __m256i mask_pos = _mm256_set1_epi16(1);
  __m256i mask_neg = _mm256_set1_epi16(-1);
  __m256i sign_mask = _mm256_unpacklo_epi64(mask_pos, mask_neg);
  __m256i temp = _mm256_shuffle_epi32(*row, _MM_SHUFFLE(1, 0, 3, 2));
  *row = _mm256_sign_epi16(*row, sign_mask);
  *row = _mm256_add_epi16(*row, temp);

  sign_mask = _mm256_unpacklo_epi32(mask_pos, mask_neg);
  temp = _mm256_shuffle_epi32(*row, _MM_SHUFFLE(2, 3, 0, 1));
  *row = _mm256_sign_epi16(*row, sign_mask);
  *row = _mm256_add_epi16(*row, temp);

  sign_mask = _mm256_unpacklo_epi16(mask_pos, mask_neg);
  temp = _mm256_shufflelo_epi16(*row, _MM_SHUFFLE(2,3,0,1));
  temp = _mm256_shufflehi_epi16(temp, _MM_SHUFFLE(2,3,0,1));
  *row = _mm256_sign_epi16(*row, sign_mask);
  *row = _mm256_add_epi16(*row, temp);
}

static INLINE void add_sub_avx2(__m128i *out, __m128i *in, unsigned out_idx0, unsigned out_idx1, unsigned in_idx0, unsigned in_idx1)
{
  out[out_idx0] = _mm_add_epi16(in[in_idx0], in[in_idx1]);
  out[out_idx1] = _mm_sub_epi16(in[in_idx0], in[in_idx1]);
}

static INLINE void ver_transform_block_avx2(__m128i (*rows)[8]){

  __m128i temp0[8];
  add_sub_avx2(temp0, (*rows), 0, 1, 0, 1);
  add_sub_avx2(temp0, (*rows), 2, 3, 2, 3);
  add_sub_avx2(temp0, (*rows), 4, 5, 4, 5);
  add_sub_avx2(temp0, (*rows), 6, 7, 6, 7);

  __m128i temp1[8];
  add_sub_avx2(temp1, temp0, 0, 1, 0, 2);
  add_sub_avx2(temp1, temp0, 2, 3, 1, 3);
  add_sub_avx2(temp1, temp0, 4, 5, 4, 6);
  add_sub_avx2(temp1, temp0, 6, 7, 5, 7);

  add_sub_avx2((*rows), temp1, 0, 1, 0, 4);
  add_sub_avx2((*rows), temp1, 2, 3, 1, 5);
  add_sub_avx2((*rows), temp1, 4, 5, 2, 6);
  add_sub_avx2((*rows), temp1, 6, 7, 3, 7);
  
}

static INLINE void add_sub_dual_avx2(__m256i *out, __m256i *in, unsigned out_idx0, unsigned out_idx1, unsigned in_idx0, unsigned in_idx1)
{
  out[out_idx0] = _mm256_add_epi16(in[in_idx0], in[in_idx1]);
  out[out_idx1] = _mm256_sub_epi16(in[in_idx0], in[in_idx1]);
}


static INLINE void ver_transform_block_dual_avx2(__m256i (*rows)[8]){

  __m256i temp0[8];
  add_sub_dual_avx2(temp0, (*rows), 0, 1, 0, 1);
  add_sub_dual_avx2(temp0, (*rows), 2, 3, 2, 3);
  add_sub_dual_avx2(temp0, (*rows), 4, 5, 4, 5);
  add_sub_dual_avx2(temp0, (*rows), 6, 7, 6, 7);

  __m256i temp1[8];
  add_sub_dual_avx2(temp1, temp0, 0, 1, 0, 2);
  add_sub_dual_avx2(temp1, temp0, 2, 3, 1, 3);
  add_sub_dual_avx2(temp1, temp0, 4, 5, 4, 6);
  add_sub_dual_avx2(temp1, temp0, 6, 7, 5, 7);

  add_sub_dual_avx2((*rows), temp1, 0, 1, 0, 4);
  add_sub_dual_avx2((*rows), temp1, 2, 3, 1, 5);
  add_sub_dual_avx2((*rows), temp1, 4, 5, 2, 6);
  add_sub_dual_avx2((*rows), temp1, 6, 7, 3, 7);
  
}

INLINE static void haddwd_accumulate_avx2(__m128i *accumulate, __m128i *ver_row)
{
  __m128i abs_value = _mm_abs_epi16(*ver_row);
  *accumulate = _mm_add_epi32(*accumulate, _mm_madd_epi16(abs_value, _mm_set1_epi16(1)));
}

INLINE static void haddwd_accumulate_dual_avx2(__m256i *accumulate, __m256i *ver_row)
{
  __m256i abs_value = _mm256_abs_epi16(*ver_row);
  *accumulate = _mm256_add_epi32(*accumulate, _mm256_madd_epi16(abs_value, _mm256_set1_epi16(1)));
}

INLINE static unsigned sum_block_avx2(__m128i *ver_row)
{
  __m128i sad = _mm_setzero_si128();
  haddwd_accumulate_avx2(&sad, ver_row + 0);
  haddwd_accumulate_avx2(&sad, ver_row + 1);
  haddwd_accumulate_avx2(&sad, ver_row + 2);
  haddwd_accumulate_avx2(&sad, ver_row + 3); 
  haddwd_accumulate_avx2(&sad, ver_row + 4);
  haddwd_accumulate_avx2(&sad, ver_row + 5);
  haddwd_accumulate_avx2(&sad, ver_row + 6);
  haddwd_accumulate_avx2(&sad, ver_row + 7);

  sad = _mm_add_epi32(sad, _mm_shuffle_epi32(sad, _MM_SHUFFLE(1, 0, 3, 2)));
  sad = _mm_add_epi32(sad, _mm_shuffle_epi32(sad, _MM_SHUFFLE(0, 1, 0, 1)));

  return _mm_cvtsi128_si32(sad);
}

INLINE static void sum_block_dual_avx2(__m256i *ver_row, unsigned *sum0, unsigned *sum1)
{
  __m256i sad = _mm256_setzero_si256();
  haddwd_accumulate_dual_avx2(&sad, ver_row + 0);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 1);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 2);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 3); 
  haddwd_accumulate_dual_avx2(&sad, ver_row + 4);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 5);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 6);
  haddwd_accumulate_dual_avx2(&sad, ver_row + 7);

  sad = _mm256_add_epi32(sad, _mm256_shuffle_epi32(sad, _MM_SHUFFLE(1, 0, 3, 2)));
  sad = _mm256_add_epi32(sad, _mm256_shuffle_epi32(sad, _MM_SHUFFLE(0, 1, 0, 1)));

  *sum0 = _mm_cvtsi128_si32(_mm256_extracti128_si256(sad, 0));
  *sum1 = _mm_cvtsi128_si32(_mm256_extracti128_si256(sad, 1));
}

INLINE static __m128i diff_row_avx2(const kvz_pixel *buf1, const kvz_pixel *buf2)
{
  __m128i buf1_row = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)buf1));
  __m128i buf2_row = _mm_cvtepu8_epi16(_mm_loadl_epi64((__m128i*)buf2));
  return _mm_sub_epi16(buf1_row, buf2_row);
}

INLINE static __m256i diff_row_dual_avx2(const kvz_pixel *buf1, const kvz_pixel *buf2, const kvz_pixel *orig)
{
  __m128i temp1 = _mm_loadl_epi64((__m128i*)buf1);
  __m128i temp2 = _mm_loadl_epi64((__m128i*)buf2);
  __m128i temp3 = _mm_loadl_epi64((__m128i*)orig);
  __m256i buf1_row = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64(temp1, temp2));
  __m256i buf2_row = _mm256_cvtepu8_epi16(_mm_broadcastq_epi64(temp3));

  return _mm256_sub_epi16(buf1_row, buf2_row);
}

INLINE static void diff_blocks_avx2(__m128i (*row_diff)[8],
                                                           const kvz_pixel * buf1, unsigned stride1,
                                                           const kvz_pixel * orig, unsigned stride_orig)
{
  (*row_diff)[0] = diff_row_avx2(buf1 + 0 * stride1, orig + 0 * stride_orig);
  (*row_diff)[1] = diff_row_avx2(buf1 + 1 * stride1, orig + 1 * stride_orig);
  (*row_diff)[2] = diff_row_avx2(buf1 + 2 * stride1, orig + 2 * stride_orig);
  (*row_diff)[3] = diff_row_avx2(buf1 + 3 * stride1, orig + 3 * stride_orig);
  (*row_diff)[4] = diff_row_avx2(buf1 + 4 * stride1, orig + 4 * stride_orig);
  (*row_diff)[5] = diff_row_avx2(buf1 + 5 * stride1, orig + 5 * stride_orig);
  (*row_diff)[6] = diff_row_avx2(buf1 + 6 * stride1, orig + 6 * stride_orig);
  (*row_diff)[7] = diff_row_avx2(buf1 + 7 * stride1, orig + 7 * stride_orig);

}

INLINE static void diff_blocks_dual_avx2(__m256i (*row_diff)[8],
                                                           const kvz_pixel * buf1, unsigned stride1,
                                                           const kvz_pixel * buf2, unsigned stride2,
                                                           const kvz_pixel * orig, unsigned stride_orig)
{
  (*row_diff)[0] = diff_row_dual_avx2(buf1 + 0 * stride1, buf2 + 0 * stride2, orig + 0 * stride_orig);
  (*row_diff)[1] = diff_row_dual_avx2(buf1 + 1 * stride1, buf2 + 1 * stride2, orig + 1 * stride_orig);
  (*row_diff)[2] = diff_row_dual_avx2(buf1 + 2 * stride1, buf2 + 2 * stride2, orig + 2 * stride_orig);
  (*row_diff)[3] = diff_row_dual_avx2(buf1 + 3 * stride1, buf2 + 3 * stride2, orig + 3 * stride_orig);
  (*row_diff)[4] = diff_row_dual_avx2(buf1 + 4 * stride1, buf2 + 4 * stride2, orig + 4 * stride_orig);
  (*row_diff)[5] = diff_row_dual_avx2(buf1 + 5 * stride1, buf2 + 5 * stride2, orig + 5 * stride_orig);
  (*row_diff)[6] = diff_row_dual_avx2(buf1 + 6 * stride1, buf2 + 6 * stride2, orig + 6 * stride_orig);
  (*row_diff)[7] = diff_row_dual_avx2(buf1 + 7 * stride1, buf2 + 7 * stride2, orig + 7 * stride_orig);

}

INLINE static void hor_transform_block_avx2(__m128i (*row_diff)[8])
{
  hor_transform_row_avx2((*row_diff) + 0);
  hor_transform_row_avx2((*row_diff) + 1);
  hor_transform_row_avx2((*row_diff) + 2);
  hor_transform_row_avx2((*row_diff) + 3);
  hor_transform_row_avx2((*row_diff) + 4);
  hor_transform_row_avx2((*row_diff) + 5);
  hor_transform_row_avx2((*row_diff) + 6);
  hor_transform_row_avx2((*row_diff) + 7);
}

INLINE static void hor_transform_block_dual_avx2(__m256i (*row_diff)[8])
{
  hor_transform_row_dual_avx2((*row_diff) + 0);
  hor_transform_row_dual_avx2((*row_diff) + 1);
  hor_transform_row_dual_avx2((*row_diff) + 2);
  hor_transform_row_dual_avx2((*row_diff) + 3);
  hor_transform_row_dual_avx2((*row_diff) + 4);
  hor_transform_row_dual_avx2((*row_diff) + 5);
  hor_transform_row_dual_avx2((*row_diff) + 6);
  hor_transform_row_dual_avx2((*row_diff) + 7);
}

static void kvz_satd_8bit_8x8_general_dual_avx2(const kvz_pixel * buf1, unsigned stride1,
                                                const kvz_pixel * buf2, unsigned stride2,
                                                const kvz_pixel * orig, unsigned stride_orig,
                                                unsigned *sum0, unsigned *sum1)
{
  __m256i temp[8];

  diff_blocks_dual_avx2(&temp, buf1, stride1, buf2, stride2, orig, stride_orig);
  hor_transform_block_dual_avx2(&temp);
  ver_transform_block_dual_avx2(&temp);
  
  sum_block_dual_avx2(temp, sum0, sum1);

  *sum0 = (*sum0 + 2) >> 2;
  *sum1 = (*sum1 + 2) >> 2;
}

/**
* \brief  Calculate SATD between two 4x4 blocks inside bigger arrays.
*/
static unsigned kvz_satd_4x4_subblock_8bit_avx2(const kvz_pixel * buf1,
                                                const int32_t     stride1,
                                                const kvz_pixel * buf2,
                                                const int32_t     stride2)
{
  // TODO: AVX2 implementation
  return kvz_satd_4x4_subblock_generic(buf1, stride1, buf2, stride2);
}

static void kvz_satd_4x4_subblock_quad_avx2(const kvz_pixel *preds[4],
                                       const int stride,
                                       const kvz_pixel *orig,
                                       const int orig_stride,
                                       unsigned costs[4])
{
  // TODO: AVX2 implementation
  kvz_satd_4x4_subblock_quad_generic(preds, stride, orig, orig_stride, costs);
}

static unsigned satd_8x8_subblock_8bit_avx2(const kvz_pixel * buf1, unsigned stride1, const kvz_pixel * buf2, unsigned stride2)
{
  __m128i temp[8];

  diff_blocks_avx2(&temp, buf1, stride1, buf2, stride2);
  hor_transform_block_avx2(&temp);
  ver_transform_block_avx2(&temp);
  
  unsigned sad = sum_block_avx2(temp);

  unsigned result = (sad + 2) >> 2;
  return result;
}

static void satd_8x8_subblock_quad_avx2(const kvz_pixel **preds,
  const int stride,
  const kvz_pixel *orig,
  const int orig_stride,
  unsigned *costs)
{
  kvz_satd_8bit_8x8_general_dual_avx2(preds[0], stride, preds[1], stride, orig, orig_stride, &costs[0], &costs[1]);
  kvz_satd_8bit_8x8_general_dual_avx2(preds[2], stride, preds[3], stride, orig, orig_stride, &costs[2], &costs[3]);
}

SATD_NxN(8bit_avx2,  8)
SATD_NxN(8bit_avx2, 16)
SATD_NxN(8bit_avx2, 32)
SATD_NxN(8bit_avx2, 64)
SATD_ANY_SIZE(8bit_avx2)

// Function macro for defining hadamard calculating functions
// for fixed size blocks. They calculate hadamard for integer
// multiples of 8x8 with the 8x8 hadamard function.
#define SATD_NXN_DUAL_AVX2(n) \
static void satd_8bit_ ## n ## x ## n ## _dual_avx2( \
  const pred_buffer preds, const kvz_pixel * const orig, unsigned num_modes, unsigned *satds_out)  \
{ \
  unsigned x, y; \
  satds_out[0] = 0; \
  satds_out[1] = 0; \
  unsigned sum1 = 0; \
  unsigned sum2 = 0; \
  for (y = 0; y < (n); y += 8) { \
  unsigned row = y * (n); \
  for (x = 0; x < (n); x += 8) { \
  kvz_satd_8bit_8x8_general_dual_avx2(&preds[0][row + x], (n), &preds[1][row + x], (n), &orig[row + x], (n), &sum1, &sum2); \
  satds_out[0] += sum1; \
  satds_out[1] += sum2; \
    } \
    } \
  satds_out[0] >>= (KVZ_BIT_DEPTH-8); \
  satds_out[1] >>= (KVZ_BIT_DEPTH-8); \
}

static void satd_8bit_8x8_dual_avx2(
  const pred_buffer preds, const kvz_pixel * const orig, unsigned num_modes, unsigned *satds_out) 
{ 
  unsigned x, y; 
  satds_out[0] = 0;
  satds_out[1] = 0;
  unsigned sum1 = 0;
  unsigned sum2 = 0;
  for (y = 0; y < (8); y += 8) { 
  unsigned row = y * (8); 
  for (x = 0; x < (8); x += 8) { 
  kvz_satd_8bit_8x8_general_dual_avx2(&preds[0][row + x], (8), &preds[1][row + x], (8), &orig[row + x], (8), &sum1, &sum2); 
  satds_out[0] += sum1;
  satds_out[1] += sum2;
      } 
      } 
  satds_out[0] >>= (KVZ_BIT_DEPTH-8);
  satds_out[1] >>= (KVZ_BIT_DEPTH-8);
}

//SATD_NXN_DUAL_AVX2(8) //Use the non-macro version
SATD_NXN_DUAL_AVX2(16)
SATD_NXN_DUAL_AVX2(32)
SATD_NXN_DUAL_AVX2(64)

#define SATD_ANY_SIZE_MULTI_AVX2(suffix, num_parallel_blocks) \
  static cost_pixel_any_size_multi_func satd_any_size_## suffix; \
  static void satd_any_size_ ## suffix ( \
      int width, int height, \
      const kvz_pixel **preds, \
      const int stride, \
      const kvz_pixel *orig, \
      const int orig_stride, \
      unsigned num_modes, \
      unsigned *costs_out, \
      int8_t *valid) \
  { \
    unsigned sums[num_parallel_blocks] = { 0 }; \
    const kvz_pixel *pred_ptrs[4] = { preds[0], preds[1], preds[2], preds[3] };\
    const kvz_pixel *orig_ptr = orig; \
    costs_out[0] = 0; costs_out[1] = 0; costs_out[2] = 0; costs_out[3] = 0; \
    if (width % 8 != 0) { \
      /* Process the first column using 4x4 blocks. */ \
      for (int y = 0; y < height; y += 4) { \
        kvz_satd_4x4_subblock_ ## suffix(preds, stride, orig, orig_stride, sums); \
            } \
      orig_ptr += 4; \
      for(int blk = 0; blk < num_parallel_blocks; ++blk){\
        pred_ptrs[blk] += 4; \
            }\
      width -= 4; \
            } \
    if (height % 8 != 0) { \
      /* Process the first row using 4x4 blocks. */ \
      for (int x = 0; x < width; x += 4 ) { \
        kvz_satd_4x4_subblock_ ## suffix(pred_ptrs, stride, orig_ptr, orig_stride, sums); \
            } \
      orig_ptr += 4 * orig_stride; \
      for(int blk = 0; blk < num_parallel_blocks; ++blk){\
        pred_ptrs[blk] += 4 * stride; \
            }\
      height -= 4; \
        } \
    /* The rest can now be processed with 8x8 blocks. */ \
    for (int y = 0; y < height; y += 8) { \
      orig_ptr = &orig[y * orig_stride]; \
      pred_ptrs[0] = &preds[0][y * stride]; \
      pred_ptrs[1] = &preds[1][y * stride]; \
      pred_ptrs[2] = &preds[2][y * stride]; \
      pred_ptrs[3] = &preds[3][y * stride]; \
      for (int x = 0; x < width; x += 8) { \
        satd_8x8_subblock_ ## suffix(pred_ptrs, stride, orig_ptr, orig_stride, sums); \
        orig_ptr += 8; \
        pred_ptrs[0] += 8; \
        pred_ptrs[1] += 8; \
        pred_ptrs[2] += 8; \
        pred_ptrs[3] += 8; \
        costs_out[0] += sums[0]; \
        costs_out[1] += sums[1]; \
        costs_out[2] += sums[2]; \
        costs_out[3] += sums[3]; \
      } \
    } \
    for(int i = 0; i < num_parallel_blocks; ++i){\
      costs_out[i] = costs_out[i] >> (KVZ_BIT_DEPTH - 8);\
    } \
    return; \
  }

SATD_ANY_SIZE_MULTI_AVX2(quad_avx2, 4)


static unsigned pixels_calc_ssd_avx2(const kvz_pixel *const ref, const kvz_pixel *const rec,
                 const int ref_stride, const int rec_stride,
                 const int width)
{
  __m256i ssd_part;
  __m256i diff = _mm256_setzero_si256();
  __m128i sum;

  __m256i ref_epi16;
  __m256i rec_epi16;

  __m128i ref_row0, ref_row1, ref_row2, ref_row3;
  __m128i rec_row0, rec_row1, rec_row2, rec_row3;

  int ssd;

  switch (width) {

  case 4:

    ref_row0 = _mm_cvtsi32_si128(*(int32_t*)&(ref[0 * ref_stride]));
    ref_row1 = _mm_cvtsi32_si128(*(int32_t*)&(ref[1 * ref_stride]));
    ref_row2 = _mm_cvtsi32_si128(*(int32_t*)&(ref[2 * ref_stride]));
    ref_row3 = _mm_cvtsi32_si128(*(int32_t*)&(ref[3 * ref_stride]));

    ref_row0 = _mm_unpacklo_epi32(ref_row0, ref_row1);
    ref_row1 = _mm_unpacklo_epi32(ref_row2, ref_row3);
    ref_epi16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64(ref_row0, ref_row1) );

    rec_row0 = _mm_cvtsi32_si128(*(int32_t*)&(rec[0 * rec_stride]));
    rec_row1 = _mm_cvtsi32_si128(*(int32_t*)&(rec[1 * rec_stride]));
    rec_row2 = _mm_cvtsi32_si128(*(int32_t*)&(rec[2 * rec_stride]));
    rec_row3 = _mm_cvtsi32_si128(*(int32_t*)&(rec[3 * rec_stride]));

    rec_row0 = _mm_unpacklo_epi32(rec_row0, rec_row1);
    rec_row1 = _mm_unpacklo_epi32(rec_row2, rec_row3);
    rec_epi16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64(rec_row0, rec_row1) );

    diff = _mm256_sub_epi16(ref_epi16, rec_epi16);
    ssd_part =  _mm256_madd_epi16(diff, diff);

    sum = _mm_add_epi32(_mm256_castsi256_si128(ssd_part), _mm256_extracti128_si256(ssd_part, 1));
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(1, 0, 3, 2)));
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(0, 1, 0, 1)));

    ssd = _mm_cvtsi128_si32(sum);

    return ssd >> (2*(KVZ_BIT_DEPTH-8));
    break;

  default:

    ssd_part = _mm256_setzero_si256();
    for (int y = 0; y < width; y += 8) {
      for (int x = 0; x < width; x += 8) {
        for (int i = 0; i < 8; i += 2) {
          ref_epi16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&(ref[x + (y + i) * ref_stride])), _mm_loadl_epi64((__m128i*)&(ref[x + (y + i + 1) * ref_stride]))));
          rec_epi16 = _mm256_cvtepu8_epi16(_mm_unpacklo_epi64(_mm_loadl_epi64((__m128i*)&(rec[x + (y + i) * rec_stride])), _mm_loadl_epi64((__m128i*)&(rec[x + (y + i + 1) * rec_stride]))));
          diff = _mm256_sub_epi16(ref_epi16, rec_epi16);
          ssd_part = _mm256_add_epi32(ssd_part, _mm256_madd_epi16(diff, diff));
        }
      }
    }

    sum = _mm_add_epi32(_mm256_castsi256_si128(ssd_part), _mm256_extracti128_si256(ssd_part, 1));
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(1, 0, 3, 2)));
    sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(0, 1, 0, 1)));

    ssd = _mm_cvtsi128_si32(sum);

    return ssd >> (2*(KVZ_BIT_DEPTH-8));
    break;
  }
}

static void inter_recon_bipred_avx2(const int hi_prec_luma_rec0,
 const int hi_prec_luma_rec1,
 const int hi_prec_chroma_rec0,
 const int hi_prec_chroma_rec1,
 const int height,
 const int width,
 const int ypos,
 const int xpos,
 const hi_prec_buf_t*high_precision_rec0,
 const hi_prec_buf_t*high_precision_rec1,
 lcu_t* lcu,
 kvz_pixel* temp_lcu_y,
 kvz_pixel* temp_lcu_u,
 kvz_pixel* temp_lcu_v,
bool predict_luma,
bool predict_chroma)
{
  int y_in_lcu, x_in_lcu;
  int shift = 15 - KVZ_BIT_DEPTH;
  int offset = 1 << (shift - 1);
  __m256i temp_epi8, temp_y_epi32, sample0_epi32, sample1_epi32, temp_epi16;
  int32_t * pointer = 0;
  __m256i offset_epi32 = _mm256_set1_epi32(offset);

  for (int temp_y = 0; temp_y < height; ++temp_y) {

   y_in_lcu = ((ypos + temp_y) & ((LCU_WIDTH)-1));

   for (int temp_x = 0; temp_x < width; temp_x += 8) {
    x_in_lcu = ((xpos + temp_x) & ((LCU_WIDTH)-1));

    if (predict_luma) {
      bool use_8_elements = ((temp_x + 8) <= width);

      if (!use_8_elements) {
        if (width < 4) {
          // If width is smaller than 4 there's no need to use SIMD
          for (int temp_i = 0; temp_i < width; ++temp_i) {
            x_in_lcu = ((xpos + temp_i) & ((LCU_WIDTH)-1));

            int sample0_y = (hi_prec_luma_rec0 ? high_precision_rec0->y[y_in_lcu * LCU_WIDTH + x_in_lcu] : (temp_lcu_y[y_in_lcu * LCU_WIDTH + x_in_lcu] << (14 - KVZ_BIT_DEPTH)));
            int sample1_y = (hi_prec_luma_rec1 ? high_precision_rec1->y[y_in_lcu * LCU_WIDTH + x_in_lcu] : (lcu->rec.y[y_in_lcu * LCU_WIDTH + x_in_lcu] << (14 - KVZ_BIT_DEPTH)));

            lcu->rec.y[y_in_lcu * LCU_WIDTH + x_in_lcu] = (kvz_pixel)kvz_fast_clip_32bit_to_pixel((sample0_y + sample1_y + offset) >> shift);
          }
        }

        else {
          // Load total of 4 elements from memory to vector
          sample0_epi32 = hi_prec_luma_rec0 ? _mm256_cvtepi16_epi32(_mm_loadl_epi64((__m128i*) &(high_precision_rec0->y[y_in_lcu * LCU_WIDTH + x_in_lcu]))) :
            _mm256_slli_epi32(_mm256_cvtepu8_epi32(_mm_cvtsi32_si128(*(int32_t*)&(temp_lcu_y[y_in_lcu * LCU_WIDTH + x_in_lcu]))), 14 - KVZ_BIT_DEPTH);


          sample1_epi32 = hi_prec_luma_rec1 ? _mm256_cvtepi16_epi32(_mm_loadl_epi64((__m128i*) &(high_precision_rec1->y[y_in_lcu * LCU_WIDTH + x_in_lcu]))) :
            _mm256_slli_epi32(_mm256_cvtepu8_epi32(_mm_cvtsi32_si128(*(int32_t*) &(lcu->rec.y[y_in_lcu * LCU_WIDTH + x_in_lcu]))), 14 - KVZ_BIT_DEPTH);


          // (sample1 + sample2 + offset)>>shift 
          temp_y_epi32 = _mm256_add_epi32(sample0_epi32, sample1_epi32);
          temp_y_epi32 = _mm256_add_epi32(temp_y_epi32, offset_epi32);
          temp_y_epi32 = _mm256_srai_epi32(temp_y_epi32, shift);

          // Pack the bits from 32-bit to 8-bit
          temp_epi16 = _mm256_packs_epi32(temp_y_epi32, temp_y_epi32);
          temp_epi16 = _mm256_permute4x64_epi64(temp_epi16, _MM_SHUFFLE(3, 1, 2, 0));
          temp_epi8 = _mm256_packus_epi16(temp_epi16, temp_epi16);

          pointer = (int32_t*)&(lcu->rec.y[(y_in_lcu)* LCU_WIDTH + x_in_lcu]);
          *pointer = _mm_cvtsi128_si32(_mm256_castsi256_si128(temp_epi8));



          for (int temp_i = temp_x + 4; temp_i < width; ++temp_i) {
            x_in_lcu = ((xpos + temp_i) & ((LCU_WIDTH)-1));

            int16_t sample0_y = (hi_prec_luma_rec0 ? high_precision_rec0->y[y_in_lcu * LCU_WIDTH + x_in_lcu] : (temp_lcu_y[y_in_lcu * LCU_WIDTH + x_in_lcu] << (14 - KVZ_BIT_DEPTH)));
            int16_t sample1_y = (hi_prec_luma_rec1 ? high_precision_rec1->y[y_in_lcu * LCU_WIDTH + x_in_lcu] : (lcu->rec.y[y_in_lcu * LCU_WIDTH + x_in_lcu] << (14 - KVZ_BIT_DEPTH)));

            lcu->rec.y[y_in_lcu * LCU_WIDTH + x_in_lcu] = (kvz_pixel)kvz_fast_clip_32bit_to_pixel((sample0_y + sample1_y + offset) >> shift);
          }

        }
      } else {
        // Load total of 8 elements from memory to vector
        sample0_epi32 = hi_prec_luma_rec0 ? _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*) &(high_precision_rec0->y[y_in_lcu * LCU_WIDTH + x_in_lcu]))) :
          _mm256_slli_epi32(_mm256_cvtepu8_epi32((_mm_loadl_epi64((__m128i*) &(temp_lcu_y[y_in_lcu * LCU_WIDTH + x_in_lcu])))), 14 - KVZ_BIT_DEPTH);

        sample1_epi32 = hi_prec_luma_rec1 ? _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*) &(high_precision_rec1->y[y_in_lcu * LCU_WIDTH + x_in_lcu]))) :
          _mm256_slli_epi32(_mm256_cvtepu8_epi32((_mm_loadl_epi64((__m128i*) &(lcu->rec.y[y_in_lcu * LCU_WIDTH + x_in_lcu])))), 14 - KVZ_BIT_DEPTH);

        // (sample1 + sample2 + offset)>>shift 
        temp_y_epi32 = _mm256_add_epi32(sample0_epi32, sample1_epi32);
        temp_y_epi32 = _mm256_add_epi32(temp_y_epi32, offset_epi32);
        temp_y_epi32 = _mm256_srai_epi32(temp_y_epi32, shift);

        // Pack the bits from 32-bit to 8-bit
        temp_epi16 = _mm256_packs_epi32(temp_y_epi32, temp_y_epi32);
        temp_epi16 = _mm256_permute4x64_epi64(temp_epi16, _MM_SHUFFLE(3, 1, 2, 0));
        temp_epi8 = _mm256_packus_epi16(temp_epi16, temp_epi16);

        // Store 64-bits from vector to memory
        _mm_storel_epi64((__m128i*)&(lcu->rec.y[(y_in_lcu)* LCU_WIDTH + x_in_lcu]), _mm256_castsi256_si128(temp_epi8));
      }
    }
   }
  }
  for (int temp_y = 0; temp_y < height >> 1; ++temp_y) {
   int y_in_lcu = (((ypos >> 1) + temp_y) & (LCU_WIDTH_C - 1));
   
   for (int temp_x = 0; temp_x < width >> 1; temp_x += 8) {

    int x_in_lcu = (((xpos >> 1) + temp_x) & (LCU_WIDTH_C - 1));

    if (predict_chroma) {
      if ((width >> 1) < 4) {
        // If width>>1 is smaller than 4 there's no need to use SIMD

        for (int temp_i = 0; temp_i < width >> 1; ++temp_i) {
          int temp_x_in_lcu = (((xpos >> 1) + temp_i) & (LCU_WIDTH_C - 1));
          int16_t sample0_u = (hi_prec_chroma_rec0 ? high_precision_rec0->u[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] : (temp_lcu_u[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] << (14 - KVZ_BIT_DEPTH)));
          int16_t sample1_u = (hi_prec_chroma_rec1 ? high_precision_rec1->u[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] : (lcu->rec.u[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] << (14 - KVZ_BIT_DEPTH)));
          lcu->rec.u[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] = (kvz_pixel)kvz_fast_clip_32bit_to_pixel((sample0_u + sample1_u + offset) >> shift);

          int16_t sample0_v = (hi_prec_chroma_rec0 ? high_precision_rec0->v[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] : (temp_lcu_v[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] << (14 - KVZ_BIT_DEPTH)));
          int16_t sample1_v = (hi_prec_chroma_rec1 ? high_precision_rec1->v[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] : (lcu->rec.v[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] << (14 - KVZ_BIT_DEPTH)));
          lcu->rec.v[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] = (kvz_pixel)kvz_fast_clip_32bit_to_pixel((sample0_v + sample1_v + offset) >> shift);
        }
      }

      else {

        bool use_8_elements = ((temp_x + 8) <= (width >> 1));

        __m256i temp_u_epi32, temp_v_epi32;

        if (!use_8_elements) {
          // Load 4 pixels to vector
          sample0_epi32 = hi_prec_chroma_rec0 ? _mm256_cvtepi16_epi32(_mm_loadl_epi64((__m128i*) &(high_precision_rec0->u[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))) :
            _mm256_slli_epi32(_mm256_cvtepu8_epi32(_mm_cvtsi32_si128(*(int32_t*) &(temp_lcu_u[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))), 14 - KVZ_BIT_DEPTH);

          sample1_epi32 = hi_prec_chroma_rec1 ? _mm256_cvtepi16_epi32(_mm_loadl_epi64((__m128i*) &(high_precision_rec1->u[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))) :
            _mm256_slli_epi32(_mm256_cvtepu8_epi32(_mm_cvtsi32_si128(*(int32_t*) &(lcu->rec.u[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))), 14 - KVZ_BIT_DEPTH);

          // (sample1 + sample2 + offset)>>shift 
          temp_u_epi32 = _mm256_add_epi32(sample0_epi32, sample1_epi32);
          temp_u_epi32 = _mm256_add_epi32(temp_u_epi32, offset_epi32);
          temp_u_epi32 = _mm256_srai_epi32(temp_u_epi32, shift);



          sample0_epi32 = hi_prec_chroma_rec0 ? _mm256_cvtepi16_epi32(_mm_loadl_epi64((__m128i*) &(high_precision_rec0->v[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))) :
            _mm256_slli_epi32(_mm256_cvtepu8_epi32(_mm_cvtsi32_si128(*(int32_t*) &(temp_lcu_v[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))), 14 - KVZ_BIT_DEPTH);

          sample1_epi32 = hi_prec_chroma_rec1 ? _mm256_cvtepi16_epi32(_mm_loadl_epi64((__m128i*) &(high_precision_rec1->v[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))) :
            _mm256_slli_epi32(_mm256_cvtepu8_epi32(_mm_cvtsi32_si128(*(int32_t*) &(lcu->rec.v[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))), 14 - KVZ_BIT_DEPTH);


          // (sample1 + sample2 + offset)>>shift 
          temp_v_epi32 = _mm256_add_epi32(sample0_epi32, sample1_epi32);
          temp_v_epi32 = _mm256_add_epi32(temp_v_epi32, offset_epi32);
          temp_v_epi32 = _mm256_srai_epi32(temp_v_epi32, shift);


          temp_epi16 = _mm256_packs_epi32(temp_u_epi32, temp_u_epi32);
          temp_epi16 = _mm256_permute4x64_epi64(temp_epi16, _MM_SHUFFLE(3, 1, 2, 0));
          temp_epi8 = _mm256_packus_epi16(temp_epi16, temp_epi16);

          pointer = (int32_t*)&(lcu->rec.u[(y_in_lcu)* LCU_WIDTH_C + x_in_lcu]);
          *pointer = _mm_cvtsi128_si32(_mm256_castsi256_si128(temp_epi8));


          temp_epi16 = _mm256_packs_epi32(temp_v_epi32, temp_v_epi32);
          temp_epi16 = _mm256_permute4x64_epi64(temp_epi16, _MM_SHUFFLE(3, 1, 2, 0));
          temp_epi8 = _mm256_packus_epi16(temp_epi16, temp_epi16);

          pointer = (int32_t*)&(lcu->rec.v[(y_in_lcu)* LCU_WIDTH_C + x_in_lcu]);
          *pointer = _mm_cvtsi128_si32(_mm256_castsi256_si128(temp_epi8));

          for (int temp_i = 4; temp_i < width >> 1; ++temp_i) {

            // Use only if width>>1 is not divideble by 4
            int temp_x_in_lcu = (((xpos >> 1) + temp_i) & (LCU_WIDTH_C - 1));
            int16_t sample0_u = (hi_prec_chroma_rec0 ? high_precision_rec0->u[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] : (temp_lcu_u[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] << (14 - KVZ_BIT_DEPTH)));
            int16_t sample1_u = (hi_prec_chroma_rec1 ? high_precision_rec1->u[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] : (lcu->rec.u[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] << (14 - KVZ_BIT_DEPTH)));
            lcu->rec.u[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] = (kvz_pixel)kvz_fast_clip_32bit_to_pixel((sample0_u + sample1_u + offset) >> shift);

            int16_t sample0_v = (hi_prec_chroma_rec0 ? high_precision_rec0->v[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] : (temp_lcu_v[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] << (14 - KVZ_BIT_DEPTH)));
            int16_t sample1_v = (hi_prec_chroma_rec1 ? high_precision_rec1->v[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] : (lcu->rec.v[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] << (14 - KVZ_BIT_DEPTH)));
            lcu->rec.v[y_in_lcu * LCU_WIDTH_C + temp_x_in_lcu] = (kvz_pixel)kvz_fast_clip_32bit_to_pixel((sample0_v + sample1_v + offset) >> shift);
          }
        } else {
          // Load 8 pixels to vector
          sample0_epi32 = hi_prec_chroma_rec0 ? _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*) &(high_precision_rec0->u[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))) :
            _mm256_slli_epi32(_mm256_cvtepu8_epi32(_mm_loadl_epi64((__m128i*) &(temp_lcu_u[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))), 14 - KVZ_BIT_DEPTH);

          sample1_epi32 = hi_prec_chroma_rec1 ? _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*) &(high_precision_rec1->u[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))) :
            _mm256_slli_epi32(_mm256_cvtepu8_epi32(_mm_loadl_epi64((__m128i*) &(lcu->rec.u[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))), 14 - KVZ_BIT_DEPTH);

          // (sample1 + sample2 + offset)>>shift 
          temp_u_epi32 = _mm256_add_epi32(sample0_epi32, sample1_epi32);
          temp_u_epi32 = _mm256_add_epi32(temp_u_epi32, offset_epi32);
          temp_u_epi32 = _mm256_srai_epi32(temp_u_epi32, shift);

          sample0_epi32 = hi_prec_chroma_rec0 ? _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*) &(high_precision_rec0->v[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))) :
            _mm256_slli_epi32(_mm256_cvtepu8_epi32(_mm_loadl_epi64((__m128i*) &(temp_lcu_v[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))), 14 - KVZ_BIT_DEPTH);

          sample1_epi32 = hi_prec_chroma_rec1 ? _mm256_cvtepi16_epi32(_mm_loadu_si128((__m128i*) &(high_precision_rec1->v[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))) :
            _mm256_slli_epi32(_mm256_cvtepu8_epi32(_mm_loadl_epi64((__m128i*) &(lcu->rec.v[y_in_lcu * LCU_WIDTH_C + x_in_lcu]))), 14 - KVZ_BIT_DEPTH);


          // (sample1 + sample2 + offset)>>shift 
          temp_v_epi32 = _mm256_add_epi32(sample0_epi32, sample1_epi32);
          temp_v_epi32 = _mm256_add_epi32(temp_v_epi32, offset_epi32);
          temp_v_epi32 = _mm256_srai_epi32(temp_v_epi32, shift);

          temp_epi16 = _mm256_packs_epi32(temp_u_epi32, temp_u_epi32);
          temp_epi16 = _mm256_permute4x64_epi64(temp_epi16, _MM_SHUFFLE(3, 1, 2, 0));
          temp_epi8 = _mm256_packus_epi16(temp_epi16, temp_epi16);

          // Store 64-bit integer into memory
          _mm_storel_epi64((__m128i*)&(lcu->rec.u[(y_in_lcu)* LCU_WIDTH_C + x_in_lcu]), _mm256_castsi256_si128(temp_epi8));

          temp_epi16 = _mm256_packs_epi32(temp_v_epi32, temp_v_epi32);
          temp_epi16 = _mm256_permute4x64_epi64(temp_epi16, _MM_SHUFFLE(3, 1, 2, 0));
          temp_epi8 = _mm256_packus_epi16(temp_epi16, temp_epi16);

          // Store 64-bit integer into memory
          _mm_storel_epi64((__m128i*)&(lcu->rec.v[(y_in_lcu)* LCU_WIDTH_C + x_in_lcu]), _mm256_castsi256_si128(temp_epi8));
        }
      }
    }
   }
  }
}

static optimized_sad_func_ptr_t get_optimized_sad_avx2(int32_t width)
{
  if (width == 0)
    return reg_sad_w0;
  if (width == 4)
    return reg_sad_w4;
  if (width == 8)
    return reg_sad_w8;
  if (width == 12)
    return reg_sad_w12;
  if (width == 16)
    return reg_sad_w16;
  if (width == 24)
    return reg_sad_w24;
  if (width == 32)
    return reg_sad_w32;
  if (width == 64)
    return reg_sad_w64;
  else
    return NULL;
}

static uint32_t ver_sad_avx2(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                             int32_t width, int32_t height, uint32_t stride)
{
  if (width == 0)
    return 0;
  if (width == 4)
    return ver_sad_w4(pic_data, ref_data, height, stride);
  if (width == 8)
    return ver_sad_w8(pic_data, ref_data, height, stride);
  if (width == 12)
    return ver_sad_w12(pic_data, ref_data, height, stride);
  if (width == 16)
    return ver_sad_w16(pic_data, ref_data, height, stride);
  else
    return ver_sad_arbitrary(pic_data, ref_data, width, height, stride);
}

static uint32_t hor_sad_avx2(const kvz_pixel *pic_data, const kvz_pixel *ref_data,
                             int32_t width, int32_t height, uint32_t pic_stride,
                             uint32_t ref_stride, uint32_t left, uint32_t right)
{
  if (width == 4)
    return hor_sad_sse41_w4(pic_data, ref_data, height,
                            pic_stride, ref_stride, left, right);
  if (width == 8)
    return hor_sad_sse41_w8(pic_data, ref_data, height,
                            pic_stride, ref_stride, left, right);
  if (width == 16)
    return hor_sad_sse41_w16(pic_data, ref_data, height,
                             pic_stride, ref_stride, left, right);
  if (width == 32)
    return hor_sad_avx2_w32 (pic_data, ref_data, height,
                             pic_stride, ref_stride, left, right);
  else
    return hor_sad_sse41_arbitrary(pic_data, ref_data, width, height,
                                   pic_stride, ref_stride, left, right);
}

static double pixel_var_avx2_largebuf(const kvz_pixel *buf, const uint32_t len)
{
  const float len_f  = (float)len;
  const __m256i zero = _mm256_setzero_si256();

  int64_t sum;
  size_t i;
  __m256i sums = zero;
  for (i = 0; i + 31 < len; i += 32) {
    __m256i curr = _mm256_loadu_si256((const __m256i *)(buf + i));
    __m256i curr_sum = _mm256_sad_epu8(curr, zero);
            sums = _mm256_add_epi64(sums, curr_sum);
  }
  __m128i sum_lo = _mm256_castsi256_si128  (sums);
  __m128i sum_hi = _mm256_extracti128_si256(sums,   1);
  __m128i sum_3  = _mm_add_epi64           (sum_lo, sum_hi);
  __m128i sum_4  = _mm_shuffle_epi32       (sum_3,  _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sum_5  = _mm_add_epi64           (sum_3,  sum_4);

  _mm_storel_epi64((__m128i *)&sum, sum_5);

  // Remaining len mod 32 pixels
  for (; i < len; ++i) {
    sum += buf[i];
  }

  float   mean_f = (float)sum / len_f;
  __m256  mean   = _mm256_set1_ps(mean_f);
  __m256  accum  = _mm256_setzero_ps();

  for (i = 0; i + 31 < len; i += 32) {
    __m128i curr0    = _mm_loadl_epi64((const __m128i *)(buf + i +  0));
    __m128i curr1    = _mm_loadl_epi64((const __m128i *)(buf + i +  8));
    __m128i curr2    = _mm_loadl_epi64((const __m128i *)(buf + i + 16));
    __m128i curr3    = _mm_loadl_epi64((const __m128i *)(buf + i + 24));

    __m256i curr0_32 = _mm256_cvtepu8_epi32(curr0);
    __m256i curr1_32 = _mm256_cvtepu8_epi32(curr1);
    __m256i curr2_32 = _mm256_cvtepu8_epi32(curr2);
    __m256i curr3_32 = _mm256_cvtepu8_epi32(curr3);

    __m256  curr0_f  = _mm256_cvtepi32_ps  (curr0_32);
    __m256  curr1_f  = _mm256_cvtepi32_ps  (curr1_32);
    __m256  curr2_f  = _mm256_cvtepi32_ps  (curr2_32);
    __m256  curr3_f  = _mm256_cvtepi32_ps  (curr3_32);

    __m256  curr0_sd = _mm256_sub_ps       (curr0_f,  mean);
    __m256  curr1_sd = _mm256_sub_ps       (curr1_f,  mean);
    __m256  curr2_sd = _mm256_sub_ps       (curr2_f,  mean);
    __m256  curr3_sd = _mm256_sub_ps       (curr3_f,  mean);

    __m256  curr0_v  = _mm256_mul_ps       (curr0_sd, curr0_sd);
    __m256  curr1_v  = _mm256_mul_ps       (curr1_sd, curr1_sd);
    __m256  curr2_v  = _mm256_mul_ps       (curr2_sd, curr2_sd);
    __m256  curr3_v  = _mm256_mul_ps       (curr3_sd, curr3_sd);

    __m256  curr01   = _mm256_add_ps       (curr0_v,  curr1_v);
    __m256  curr23   = _mm256_add_ps       (curr2_v,  curr3_v);
    __m256  curr     = _mm256_add_ps       (curr01,   curr23);
            accum    = _mm256_add_ps       (accum,    curr);
  }
  __m256d accum_d  = _mm256_castps_pd     (accum);
  __m256d accum2_d = _mm256_permute4x64_pd(accum_d, _MM_SHUFFLE(1, 0, 3, 2));
  __m256  accum2   = _mm256_castpd_ps     (accum2_d);

  __m256  accum3   = _mm256_add_ps        (accum,  accum2);
  __m256  accum4   = _mm256_permute_ps    (accum3, _MM_SHUFFLE(1, 0, 3, 2));
  __m256  accum5   = _mm256_add_ps        (accum3, accum4);
  __m256  accum6   = _mm256_permute_ps    (accum5, _MM_SHUFFLE(2, 3, 0, 1));
  __m256  accum7   = _mm256_add_ps        (accum5, accum6);

  __m128  accum8   = _mm256_castps256_ps128(accum7);
  float   var_sum  = _mm_cvtss_f32         (accum8);

  // Remaining len mod 32 pixels
  for (; i < len; ++i) {
    float diff = buf[i] - mean_f;
    var_sum += diff * diff;
  }

  return  var_sum / len_f;
}

#ifdef INACCURATE_VARIANCE_CALCULATION

// Assumes that u is a power of two
static INLINE uint32_t ilog2(uint32_t u)
{
  return _tzcnt_u32(u);
}

// A B C D | E F G H (8x32b)
//        ==>
// A+B C+D | E+F G+H (4x64b)
static __m256i hsum_epi32_to_epi64(const __m256i v)
{
  const __m256i zero    = _mm256_setzero_si256();
        __m256i v_shufd = _mm256_shuffle_epi32(v, _MM_SHUFFLE(3, 3, 1, 1));
        __m256i sums_32 = _mm256_add_epi32    (v, v_shufd);
        __m256i sums_64 = _mm256_blend_epi32  (sums_32, zero, 0xaa);
  return        sums_64;
}

static double pixel_var_avx2(const kvz_pixel *buf, const uint32_t len)
{
  assert(sizeof(*buf) == 1);
  assert((len & 31) == 0);

  // Uses Q8.7 numbers to measure mean and deviation, so variances are Q16.14
  const uint64_t sum_maxwid     = ilog2(len) + (8 * sizeof(*buf));
  const __m128i normalize_sum   = _mm_cvtsi32_si128(sum_maxwid - 15); // Normalize mean to [0, 32767], so signed 16-bit subtraction never overflows
  const __m128i debias_sum      = _mm_cvtsi32_si128(1 << (sum_maxwid - 16));
  const float varsum_to_f       = 1.0f / (float)(1 << (14 + ilog2(len)));

  const bool power_of_two = (len & (len - 1)) == 0;
  if (sum_maxwid > 32 || sum_maxwid < 15 || !power_of_two) {
    return pixel_var_avx2_largebuf(buf, len);
  }

  const __m256i zero      = _mm256_setzero_si256();
  const __m256i himask_15 = _mm256_set1_epi16(0x7f00);

  uint64_t vars;
  size_t i;
  __m256i sums = zero;
  for (i = 0; i < len; i += 32) {
    __m256i curr = _mm256_loadu_si256((const __m256i *)(buf + i));
    __m256i curr_sum = _mm256_sad_epu8(curr, zero);
            sums = _mm256_add_epi64(sums, curr_sum);
  }
  __m128i sum_lo = _mm256_castsi256_si128  (sums);
  __m128i sum_hi = _mm256_extracti128_si256(sums,   1);
  __m128i sum_3  = _mm_add_epi64           (sum_lo, sum_hi);
  __m128i sum_4  = _mm_shuffle_epi32       (sum_3,  _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sum_5  = _mm_add_epi64           (sum_3,  sum_4);
  __m128i sum_5n = _mm_srl_epi32           (sum_5,  normalize_sum);
          sum_5n = _mm_add_epi32           (sum_5n, debias_sum);

  __m256i sum_n  = _mm256_broadcastw_epi16 (sum_5n);

  __m256i accum = zero;
  for (i = 0; i < len; i += 32) {
    __m256i curr = _mm256_loadu_si256((const __m256i *)(buf + i));

    __m256i curr0    = _mm256_slli_epi16  (curr,  7);
    __m256i curr1    = _mm256_srli_epi16  (curr,  1);
            curr0    = _mm256_and_si256   (curr0, himask_15);
            curr1    = _mm256_and_si256   (curr1, himask_15);

    __m256i dev0     = _mm256_sub_epi16   (curr0, sum_n);
    __m256i dev1     = _mm256_sub_epi16   (curr1, sum_n);

    __m256i vars0    = _mm256_madd_epi16  (dev0,  dev0);
    __m256i vars1    = _mm256_madd_epi16  (dev1,  dev1);

    __m256i varsum   = _mm256_add_epi32   (vars0, vars1);
            varsum   = hsum_epi32_to_epi64(varsum);
            accum    = _mm256_add_epi64   (accum, varsum);
  }
  __m256i accum2 = _mm256_permute4x64_epi64(accum,  _MM_SHUFFLE(1, 0, 3, 2));
  __m256i accum3 = _mm256_add_epi64        (accum,  accum2);
  __m256i accum4 = _mm256_permute4x64_epi64(accum3, _MM_SHUFFLE(2, 3, 1, 0));
  __m256i v_tot  = _mm256_add_epi64        (accum3, accum4);
  __m128i vt128  = _mm256_castsi256_si128  (v_tot);

  _mm_storel_epi64((__m128i *)&vars, vt128);

  return (float)vars * varsum_to_f;
}

#else // INACCURATE_VARIANCE_CALCULATION

static double pixel_var_avx2(const kvz_pixel *buf, const uint32_t len)
{
  return pixel_var_avx2_largebuf(buf, len);
}

#endif // !INACCURATE_VARIANCE_CALCULATION

#endif //COMPILE_INTEL_AVX2

int kvz_strategy_register_picture_avx2(void* opaque, uint8_t bitdepth)
{
  bool success = true;
#if COMPILE_INTEL_AVX2
  // We don't actually use SAD for intra right now, other than 4x4 for
  // transform skip, but we might again one day and this is some of the
  // simplest code to look at for anyone interested in doing more
  // optimizations, so it's worth it to keep this maintained.
  if (bitdepth == 8){

    success &= kvz_strategyselector_register(opaque, "reg_sad", "avx2", 40, &kvz_reg_sad_avx2);
    success &= kvz_strategyselector_register(opaque, "sad_8x8", "avx2", 40, &sad_8bit_8x8_avx2);
    success &= kvz_strategyselector_register(opaque, "sad_16x16", "avx2", 40, &sad_8bit_16x16_avx2);
    success &= kvz_strategyselector_register(opaque, "sad_32x32", "avx2", 40, &sad_8bit_32x32_avx2);
    success &= kvz_strategyselector_register(opaque, "sad_64x64", "avx2", 40, &sad_8bit_64x64_avx2);

    success &= kvz_strategyselector_register(opaque, "satd_4x4", "avx2", 40, &satd_4x4_8bit_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_8x8", "avx2", 40, &satd_8x8_8bit_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_16x16", "avx2", 40, &satd_16x16_8bit_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_32x32", "avx2", 40, &satd_32x32_8bit_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_64x64", "avx2", 40, &satd_64x64_8bit_avx2);

    success &= kvz_strategyselector_register(opaque, "satd_4x4_dual", "avx2", 40, &satd_8bit_4x4_dual_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_8x8_dual", "avx2", 40, &satd_8bit_8x8_dual_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_16x16_dual", "avx2", 40, &satd_8bit_16x16_dual_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_32x32_dual", "avx2", 40, &satd_8bit_32x32_dual_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_64x64_dual", "avx2", 40, &satd_8bit_64x64_dual_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_any_size", "avx2", 40, &satd_any_size_8bit_avx2);
    success &= kvz_strategyselector_register(opaque, "satd_any_size_quad", "avx2", 40, &satd_any_size_quad_avx2);

    success &= kvz_strategyselector_register(opaque, "pixels_calc_ssd", "avx2", 40, &pixels_calc_ssd_avx2);
    success &= kvz_strategyselector_register(opaque, "inter_recon_bipred", "avx2", 40, &inter_recon_bipred_avx2);
    success &= kvz_strategyselector_register(opaque, "get_optimized_sad", "avx2", 40, &get_optimized_sad_avx2);
    success &= kvz_strategyselector_register(opaque, "ver_sad", "avx2", 40, &ver_sad_avx2);
    success &= kvz_strategyselector_register(opaque, "hor_sad", "avx2", 40, &hor_sad_avx2);

    success &= kvz_strategyselector_register(opaque, "pixel_var", "avx2", 40, &pixel_var_avx2);

  }
#endif
  return success;
}
