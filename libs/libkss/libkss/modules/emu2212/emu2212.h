#ifndef _EMU2212_H_
#define _EMU2212_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCC_STANDARD 0
#define SCC_ENHANCED 1

#define SCC_MASK_CH(x) (1<<(x))

typedef struct __SCC {

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

} SCC ;


SCC *SCC_new(uint32_t c, uint32_t r) ;
void SCC_reset(SCC *scc) ;
void SCC_set_rate(SCC *scc, uint32_t r);
void SCC_set_quality(SCC *scc, uint32_t q) ;
void SCC_set_type(SCC *scc, uint32_t type) ;
void SCC_delete(SCC *scc) ;
int16_t SCC_calc(SCC *scc) ;
void SCC_write(SCC *scc, uint32_t adr, uint32_t val) ;
void SCC_writeReg(SCC *scc, uint32_t adr, uint32_t val) ;
uint32_t SCC_read(SCC *scc, uint32_t adr) ;
uint32_t SCC_setMask(SCC *scc, uint32_t adr) ;
uint32_t SCC_toggleMask(SCC *scc, uint32_t adr) ;

#ifdef __cplusplus
}
#endif

#endif
