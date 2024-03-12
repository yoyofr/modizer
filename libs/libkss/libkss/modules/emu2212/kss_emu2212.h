#ifndef _EMU2212KSS_H_
#define _EMU2212KSS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCCKSS_STANDARD 0
#define SCCKSS_ENHANCED 1

#define SCCKSS_MASK_CH(x) (1<<(x))

typedef struct __SCCKSS {

  uint32_t clk, rate ,base_incr, quality ;

  int16_t out;
  uint32_t type ;
  uint32_t mode ;
  uint32_t active;
  uint32_t base_adr;
  uint32_t mask ;
  
  uint32_t realstep ;
  uint32_t scctime ;
  uint32_t sccstep ;

  uint32_t incr[5] ;

  int8_t  wave[5][32] ;

  uint32_t count[5] ;
  uint32_t freq[5] ;
  uint32_t phase[5] ;
  uint32_t volume[5] ;
  uint32_t offset[5] ;
  uint8_t reg[0x100-0xC0];

  int ch_enable ;
  int ch_enable_next ;

  int cycle_4bit ;
  int cycle_8bit ;
  int refresh ;
  int rotate[5] ;

  int16_t ch_out[5] ;

} SCCKSS ;


SCCKSS *SCCKSS_new(uint32_t c, uint32_t r) ;
void SCCKSS_reset(SCCKSS *scc) ;
void SCCKSS_set_rate(SCCKSS *scc, uint32_t r);
void SCCKSS_set_quality(SCCKSS *scc, uint32_t q) ;
void SCCKSS_set_type(SCCKSS *scc, uint32_t type) ;
void SCCKSS_delete(SCCKSS *scc) ;
int16_t SCCKSS_calc(SCCKSS *scc) ;
void SCCKSS_write(SCCKSS *scc, uint32_t adr, uint32_t val) ;
void SCCKSS_writeReg(SCCKSS *scc, uint32_t adr, uint32_t val) ;
uint32_t SCCKSS_read(SCCKSS *scc, uint32_t adr) ;
uint32_t SCCKSS_setMask(SCCKSS *scc, uint32_t adr) ;
uint32_t SCCKSS_toggleMask(SCCKSS *scc, uint32_t adr) ;

#ifdef __cplusplus
}
#endif

#endif
