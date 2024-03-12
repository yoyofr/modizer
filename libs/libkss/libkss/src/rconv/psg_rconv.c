#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

//TODO:  MODIZER changes start / YOYOFR
#include "../../../../src/ModizerVoicesData.h"
//TODO:  MODIZER changes end / YOYOFR


#include "psg_rconv.h"

#ifndef INLINE
#if defined(_MSC_VER)
#define INLINE __inline
#elif defined(__GNUC__)
#define INLINE __inline__
#else
#define INLINE inline
#endif
#endif

/* don't forget min/max is defined as a macro in stdlib.h of Visual C. */
#ifndef min
static INLINE int min(int i, int j) { return (i < j) ? i : j; }
#endif

#define _PI_ 3.14159265358979323846264338327950288

/**
 * Sampling rate converter using sinc interporation.
 *
 * This code was made with reference to http://blog-yama.a-quest.com/?eid=970185
 */

/* LW is truncate length of sinc(x) calculation.
 * Lower LW is faster, higher LW results better quality.
 * LW must be a non-zero positive even number, no upper limit.
 * LW=16 or greater is recommended.
 */
#define LW 16
/* resolution of sinc(x) table. sinc(x) where 0.0≦x≦1.0 corresponds to sinc_table[0..SINC_RESO] */
#define SINC_RESO 256
#define SINC_AMP_BITS 12

static double blackman(double x) { return 0.42 - 0.5 * cos(2 * _PI_ * x) + 0.08 * cos(4 * _PI_ * x); }
static double sinc(double x) { return (x == 0.0 ? 1.0 : sin(_PI_ * x) / (_PI_ * x)); }
static double windowed_sinc(double x) { return blackman(0.5 + 0.5 * x / (LW / 2)) * sinc(x); }

static void reset(PSGKSS_RateConv *conv) {
  conv->timer = 0;
  memset(conv->buf, 0, sizeof(conv->buf[0]) * LW);
}

PSGKSS_RateConv *PSGKSS_RateConv_new(void) {
  PSGKSS_RateConv *conv = malloc(sizeof(PSGKSS_RateConv));

  conv->buf = malloc(sizeof(conv->buf[0]) * LW);
  conv->sinc_table = malloc(sizeof(conv->sinc_table[0]) * SINC_RESO * LW / 2);
  conv->psg = NULL;
  conv->f_ratio = 0;
  conv->quality = 2;

  return conv;
}

void update_psg_rate_quality(PSGKSS_RateConv *conv) {
  if (conv->psg != NULL) {
    switch (conv->quality) {
    case 0:
      // low (no interporation)
      PSGKSS_setRate(conv->psg, (uint32_t)conv->f_out);
      PSGKSS_setQuality(conv->psg, 0);
      break;
    case 1:
      // mid (light-weight interporation)
      PSGKSS_setRate(conv->psg, (uint32_t)conv->f_out);
      PSGKSS_setQuality(conv->psg, 1);
      break;
    case 2:
    default:
      // high (sinc interporation)
      PSGKSS_setQuality(conv->psg, 0);
      PSGKSS_setRate(conv->psg, (uint32_t)conv->f_inp);
      break;
    }
  }
  reset(conv);
}

/**
 * @brief Specify the rate conversion quality
 *
 * @param quality 
 * - 0: low quality (no interporation, fastest)
 * - 1: mid quality (mean interporation)
 * - 2: best quality (sinc interporation, default)
 */
void PSGKSS_RateConv_setQuality(PSGKSS_RateConv *conv, uint8_t quality) {
  if (conv->quality != quality) {
    conv->quality = quality;
    update_psg_rate_quality(conv);
  }
}

/**
 * @param psg PSGKSS object. The object must be initialized before calling this method.
 * @param f_out output sample rate
 */
void PSGKSS_RateConv_reset(PSGKSS_RateConv *conv, PSGKSS *psg, uint32_t f_out) {

  double f_inp = psg->clk / (psg->clk_div ? 16 : 8);
  double f_ratio = f_inp / f_out;

  conv->psg = psg;

  conv->f_inp = f_inp;
  conv->f_out = f_out;
  conv->out_time = 0;

  update_psg_rate_quality(conv);

  if (conv->f_ratio != f_ratio) {
    /* create sinc_table for positive 0 <= x < LW/2. */
    for (int i = 0; i < SINC_RESO * LW / 2; i++) {
      const double x = (double)i / SINC_RESO;
      if (f_out < f_inp) {
        /* for downsampling */
        conv->sinc_table[i] = (1 << SINC_AMP_BITS) * windowed_sinc(x / f_ratio) / f_ratio;
      } else {
        /* for upsampling */
        conv->sinc_table[i] = (1 << SINC_AMP_BITS) * windowed_sinc(x);
      }
    }
    conv->f_ratio = f_ratio;
  }
}

static inline int16_t lookup_sinc_table(int16_t *table, double x) {
  int index = (int)(x * SINC_RESO);
  if (index < 0)
    index = -index;
  return table[min(SINC_RESO * LW / 2 - 1, index)];
}

/* put original data to this converter at f_inp. */
static void putData(PSGKSS_RateConv *conv, int16_t data) {
  int16_t *buf = conv->buf;
  for (int i = 0; i < LW - 1; i++) {
    buf[i] = buf[i + 1];
  }
  buf[LW - 1] = data;
}

/* get resampled data from this converter at f_out. */
/* this function must be called f_out times per f_inp times of putData calls. */
static int16_t getData(PSGKSS_RateConv *conv) {
  int16_t *buf = conv->buf;
  conv->timer += conv->f_ratio;
  const double dn = conv->timer - floor(conv->timer);

  int32_t sum = 0;
  for (int k = 0; k < LW; k++) {
    double x = (double)k - (LW / 2 - 1) - dn;
    sum += buf[k] * lookup_sinc_table(conv->sinc_table, x);
  }
  return sum >> SINC_AMP_BITS;
}

void PSGKSS_RateConv_delete(PSGKSS_RateConv *conv) {
  if (conv != NULL) {
    free(conv->buf);
    free(conv->sinc_table);
    free(conv);
  }
}

int16_t PSGKSS_RateConv_calc(PSGKSS_RateConv *conv) {
    //YOYOFR
    m_voice_current_rateratio=conv->f_ratio;
    //YOYOFR
  if (conv->quality < 2) {
    return PSGKSS_calc(conv->psg);
  }
  while (conv->f_inp > conv->out_time) {
    conv->out_time += conv->f_out;
    putData(conv, PSGKSS_calc(conv->psg));
  }
  conv->out_time -= conv->f_inp;
  return getData(conv);
}
