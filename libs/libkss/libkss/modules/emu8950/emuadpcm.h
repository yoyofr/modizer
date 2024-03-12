#ifndef _EMUADPCMKSS_H_
#define _EMUADPCMKSS_H_

#include <stdint.h>

typedef struct __OPLKSS_ADPCM {
  uint32_t clk;

  uint8_t reg[0x20];

  uint8_t *wave;      /* ADPCM DATA */
  uint8_t *memory[2]; /* [0] RAM, [1] ROM */

  uint8_t status;

  uint32_t start_addr;
  uint32_t stop_addr;
  uint32_t play_addr;  /* Current play address * 2 */
  uint32_t delta_addr; /* 16bit address */
  uint32_t delta_n;
  uint32_t play_addr_mask;

  uint8_t play_start;

  int32_t output[2];
  uint32_t diff;

} OPLKSS_ADPCM;

OPLKSS_ADPCM *OPLKSS_ADPCM_new(uint32_t clk);
void OPLKSS_ADPCM_reset(OPLKSS_ADPCM *);
void OPLKSS_ADPCM_delete(OPLKSS_ADPCM *);
void OPLKSS_ADPCM_writeReg(OPLKSS_ADPCM *, uint32_t reg, uint32_t val);
int16_t OPLKSS_ADPCM_calc(OPLKSS_ADPCM *);
uint8_t OPLKSS_ADPCM_status(OPLKSS_ADPCM *);
void OPLKSS_ADPCM_resetStatus(OPLKSS_ADPCM *);
void OPLKSS_ADPCM_writeRAM(OPLKSS_ADPCM *, uint32_t start, uint32_t length, const uint8_t *data);
void OPLKSS_ADPCM_writeROM(OPLKSS_ADPCM *, uint32_t start, uint32_t length, const uint8_t *data);
#endif
