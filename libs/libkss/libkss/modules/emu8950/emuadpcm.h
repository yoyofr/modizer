#ifndef _EMUADPCM_H_
#define _EMUADPCM_H_

#include <stdint.h>

typedef struct __KSSOPL_ADPCM {
  uint32_t clk, rate;

  uint8_t reg[0x20];

  uint8_t *wave;      /* ADPCM DATA */
  uint8_t *memory[2]; /* [0] RAM, [1] ROM */

  uint8_t status; /* STATUS Registar */

  uint32_t start_addr;
  uint32_t stop_addr;
  uint32_t play_addr;  /* Current play address * 2 */
  uint32_t delta_addr; /* 16bit address */
  uint32_t delta_n;
  uint32_t play_addr_mask;

  uint32_t play_start;

  int32_t output[2];
  uint32_t diff;

  void *timer1_user_data;
  void *timer2_user_data;
  void *(*timer1_func)(void *user);
  void *(*timer2_func)(void *user);

} KSSOPL_ADPCM;

KSSOPL_ADPCM *KSSOPL_ADPCM_new(uint32_t clk, uint32_t rate);
void KSSOPL_ADPCM_setRate(KSSOPL_ADPCM *, uint32_t rate);
void KSSOPL_ADPCM_reset(KSSOPL_ADPCM *);
void KSSOPL_ADPCM_delete(KSSOPL_ADPCM *);
void KSSOPL_ADPCM_writeReg(KSSOPL_ADPCM *, uint32_t reg, uint32_t val);
int16_t KSSOPL_ADPCM_calc(KSSOPL_ADPCM *);
uint8_t KSSOPL_ADPCM_status(KSSOPL_ADPCM *);
void KSSOPL_ADPCM_writeRAM(KSSOPL_ADPCM *_this, uint32_t start, uint32_t length, const uint8_t *data);
void KSSOPL_ADPCM_writeROM(KSSOPL_ADPCM *_this, uint32_t start, uint32_t length, const uint8_t *data);
#endif
