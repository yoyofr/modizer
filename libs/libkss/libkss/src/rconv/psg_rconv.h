#ifndef _PSGKSS_RCONV_H_
#define _PSGKSS_RCONV_H_

#include "emu2149/kss_emu2149.h"

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct __PSGKSS_RateConv {
    double timer;
    double f_ratio;
    int16_t *sinc_table;
    int16_t *buf;

    double f_out;
    double f_inp;
    double out_time;

    uint8_t quality;

    PSGKSS *psg;
  } PSGKSS_RateConv;

  PSGKSS_RateConv *PSGKSS_RateConv_new(void);
  void PSGKSS_RateConv_reset(PSGKSS_RateConv *conv, PSGKSS * psg, uint32_t f_out);
  void PSGKSS_RateConv_setQuality(PSGKSS_RateConv *conv, uint8_t quality);
  int16_t PSGKSS_RateConv_calc(PSGKSS_RateConv *conv);
  void PSGKSS_RateConv_delete(PSGKSS_RateConv *conv);

#ifdef __cplusplus
}
#endif

#endif
