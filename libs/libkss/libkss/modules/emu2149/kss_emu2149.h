/* emu2149.h */
#ifndef _EMU2149KSS_H_
#define _EMU2149KSS_H_

#include <stdint.h>

#define PSGKSS_MASK_CH(x) (1<<(x))

#ifdef __cplusplus
extern "C"
{
#endif

  typedef struct __PSGKSS
  {

    /* Volume Table */
    uint32_t *voltbl;

    uint8_t reg[0x20];
    int32_t out;

    uint32_t clk, rate, base_incr;
    uint8_t quality;
    uint8_t clk_div;

    uint16_t count[3];
    uint8_t volume[3];
    uint16_t freq[3];
    uint8_t edge[3];
    uint8_t tmask[3];
    uint8_t nmask[3];
    uint32_t mask;

    uint32_t base_count;

    uint8_t env_ptr;
    uint8_t env_face;

    uint8_t env_continue;
    uint8_t env_attack;
    uint8_t env_alternate;
    uint8_t env_hold;
    uint8_t env_pause;

    uint16_t env_freq;
    uint32_t env_count;

    uint32_t noise_seed;
    uint8_t noise_scaler;
    uint8_t noise_count;
    uint8_t noise_freq;

    /* rate converter */
    uint32_t realstep;
    uint32_t psgtime;
    uint32_t psgstep;

    uint32_t freq_limit;

    /* I/O Ctrl */
    uint8_t adr;

    /* output of channels */
    int16_t ch_out[3];

  } PSGKSS;

  void PSGKSS_setQuality (PSGKSS * psg, uint8_t q);
  void PSGKSS_setClock(PSGKSS *psg, uint32_t clk);
  void PSGKSS_setClockDivider(PSGKSS *psg, uint8_t enable);
  void PSGKSS_setRate (PSGKSS * psg, uint32_t rate);
  PSGKSS *PSGKSS_new (uint32_t clk, uint32_t rate);
  void PSGKSS_reset (PSGKSS *);
  void PSGKSS_delete (PSGKSS *);
  void PSGKSS_writeReg (PSGKSS *, uint32_t reg, uint32_t val);
  void PSGKSS_writeIO (PSGKSS * psg, uint32_t adr, uint32_t val);
  uint8_t PSGKSS_readReg (PSGKSS * psg, uint32_t reg);
  uint8_t PSGKSS_readIO (PSGKSS * psg);
  int16_t PSGKSS_calc (PSGKSS *);
  void PSGKSS_setVolumeMode (PSGKSS * psg, int type);
  uint32_t PSGKSS_setMask (PSGKSS *, uint32_t mask);
  uint32_t PSGKSS_toggleMask (PSGKSS *, uint32_t mask);
    
#ifdef __cplusplus
}
#endif

/* deprecated interfaces */
#define PSGKSS_set_quality PSGKSS_setQuality
#define PSGKSS_set_rate PSGKSS_setRate
#define PSGKSS_set_clock PSGKSS_setClock

#endif
