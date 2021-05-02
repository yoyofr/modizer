/****************************************************************************

  emu2212.c -- S.C.C. emulator by Mitsutaka Okazaki 2001-2016

  2001 09-30 : Version 1.00
  2001 10-03 : Version 1.01 -- Added SCC_set_quality().
  2002 02-14 : Version 1.10 -- Added SCC_writeReg(), SCC_set_type().
                               Fixed SCC_write().
  2002 02-17 : Version 1.11 -- Fixed SCC_write().
  2002 03-02 : Version 1.12 -- Removed SCC_init & SCC_close.
  2003 09-19 : Version 1.13 -- Added SCC_setMask() and SCC_toggleMask()
  2004 10-21 : Version 1.14 -- Fixed the problem where SCC+ is disabled.
  2015 12-13 : Version 1.15 -- Changed own integer types to C99 stdint.h types.
  2016 09-06 : Version 1.16 -- Support per-channel output.

  Registar map for SCC_writeReg()

  $00-1F : WaveTable CH.A
  $20-3F : WaveTable CH.B
  $40-5F : WaveTable CH.C
  $60-7F : WaveTable CH.D&E(SCC), CH.D(SCC+)
  $80-9F : WaveTable CH.E
 
  $C0    : CH.A Freq(L)
  $C1    : CH.A Freq(H)
  $C2    : CH.B Freq(L)
  $C3    : CH.B Freq(H)
  $C4    : CH.C Freq(L)
  $C5    : CH.C Freq(H)
  $C6    : CH.D Freq(L)
  $C7    : CH.D Freq(H)
  $C8    : CH.E Freq(L)
  $C9    : CH.E Freq(H)

  $D0    : CH.A Volume
  $D1    : CH.B Volume
  $D2    : CH.C Volume
  $D3    : CH.D Volume
  $D4    : CH.E Volume
 
  $E0    : Bit0 = 0:SCC, 1:SCC+
  $E1    : CH mask
  $E2    : Extra Flags

*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emu2212.h"

#define GETA_BITS 22

static void
internal_refresh (SCC * scc)
{
  if (scc->quality)
  {
    scc->base_incr = 2 << GETA_BITS;
    scc->realstep = (uint32_t) ((1 << 31) / scc->rate);
    scc->sccstep = (uint32_t) ((1 << 31) / (scc->clk / 2));
    scc->scctime = 0;
  }
  else
  {
    scc->base_incr = (uint32_t) ((double) scc->clk * (1 << GETA_BITS) / scc->rate);
  }
}

uint32_t
SCC_setMask (SCC *scc, uint32_t mask)
{
  uint32_t ret = 0;
  if(scc)
  {
    ret = scc->mask;
    scc->mask = mask;
  }  
  return ret;
}

uint32_t
SCC_toggleMask (SCC *scc, uint32_t mask)
{
  uint32_t ret = 0;
  if(scc)
  {
    ret = scc->mask;
    scc->mask ^= mask;
  }
  return ret;
}

void
SCC_set_quality (SCC * scc, uint32_t q)
{
  scc->quality = q;
  internal_refresh (scc);
}

void
SCC_set_rate (SCC * scc, uint32_t r)
{
  scc->rate = r ? r : 44100;
  internal_refresh (scc);
}

SCC *
SCC_new (uint32_t c, uint32_t r)
{
  SCC *scc;

  scc = (SCC *) malloc (sizeof (SCC));
  if (scc == NULL)
    return NULL;
  memset(scc, 0, sizeof (SCC));

  scc->clk = c;
  scc->rate = r ? r : 44100;
  SCC_set_quality (scc, 0);
  scc->type = SCC_ENHANCED;
  return scc;
}

void
SCC_reset (SCC * scc)
{
  int i, j;

  if (scc == NULL)
    return;

  scc->mode = 0;
  scc->active = 0;
  scc->base_adr = 0x9000;

  for (i = 0; i < 5; i++)
  {
    for (j = 0; j < 5; j++)
      scc->wave[i][j] = 0;
    scc->count[i] = 0;
    scc->freq[i] = 0;
    scc->phase[i] = 0;
    scc->volume[i] = 0;
    scc->offset[i] = 0;
    scc->rotate[i] = 0;
    scc->ch_out[i] = 0;
  }

  memset(scc->reg,0,0x100-0xC0);

  scc->mask = 0;

  scc->ch_enable = 0xff;
  scc->ch_enable_next = 0xff;

  scc->cycle_4bit = 0;
  scc->cycle_8bit = 0;
  scc->refresh = 0;

  scc->out = 0;

  return;
}

void
SCC_delete (SCC * scc)
{
  if (scc != NULL)
    free (scc);
}

static inline void
update_output (SCC * scc)
{
  int i;

  for (i = 0; i < 5; i++)
  {
    scc->count[i] = (scc->count[i] + scc->incr[i]);

    if (scc->count[i] & (1 << (GETA_BITS + 5)))
    {
      scc->count[i] &= ((1 << (GETA_BITS + 5)) - 1);
      scc->offset[i] = (scc->offset[i] + 31) & scc->rotate[i];
      scc->ch_enable &= ~(1 << i);
      scc->ch_enable |= scc->ch_enable_next & (1 << i);
    }

    if (scc->ch_enable & (1 << i))
    {
      scc->phase[i] = ((scc->count[i] >> (GETA_BITS)) + scc->offset[i]) & 0x1F;
      if(!(scc->mask & SCC_MASK_CH(i)))
        scc->ch_out[i] += (scc->volume[i] * scc->wave[i][scc->phase[i]]) & 0xfff0;
    }

    scc->ch_out[i] >>= 1;
  }

}

static inline int16_t 
mix_output(SCC * scc) {
  scc->out = scc->ch_out[0] + scc->ch_out[1] + scc->ch_out[2] + scc->ch_out[3] + scc->ch_out[4];
  return (int16_t)scc->out;
}

int16_t
SCC_calc (SCC * scc)
{
  if (!scc->quality) {
    update_output(scc);
    return mix_output(scc);
  }

  while (scc->realstep > scc->scctime)
  {
    scc->scctime += scc->sccstep;
    update_output(scc);
  }
  scc->scctime -= scc->realstep;

  return mix_output(scc);
}

uint32_t
SCC_readReg (SCC * scc, uint32_t adr)
{
  if (adr < 0xA0)
    return scc->wave[adr >> 5][adr & 0x1f];
  else if( 0xC0 < adr && adr < 0xF0 )
    return scc->reg[adr-0xC0];
  else
    return 0;
}

void
SCC_writeReg (SCC * scc, uint32_t adr, uint32_t val)
{
  int ch;
  uint32_t freq;

  adr &= 0xFF;

  if (adr < 0xA0)
  {
    ch = (adr & 0xF0) >> 5;
    if (!scc->rotate[ch])
    {
      scc->wave[ch][adr & 0x1F] = (int8_t) val;
      if (scc->mode == 0 && ch == 3)
        scc->wave[4][adr & 0x1F] = (int8_t) val;
    }
  }
  else if (0xC0 <= adr && adr <= 0xC9)
  {
    scc->reg[adr-0xC0] = val;
    ch = (adr & 0x0F) >> 1;
    if (adr & 1)
      scc->freq[ch] = ((val & 0xF) << 8) | (scc->freq[ch] & 0xFF);
    else
      scc->freq[ch] = (scc->freq[ch] & 0xF00) | (val & 0xFF);

    if (scc->refresh)
      scc->count[ch] = 0;
    freq = scc->freq[ch];
    if (scc->cycle_8bit)
      freq &= 0xFF;
    if (scc->cycle_4bit)
      freq >>= 8;
    if (freq <= 8)
      scc->incr[ch] = 0;
    else
      scc->incr[ch] = scc->base_incr / (freq + 1);
  }
  else if (0xD0 <= adr && adr <= 0xD4)
  {
    scc->reg[adr-0xC0] = val;
    scc->volume[adr & 0x0F] = (uint8_t) (val & 0xF);
  }
  else if (adr == 0xE0)
  {
    scc->reg[adr-0xC0] = val;
    scc->mode = (uint8_t) val & 1;
  }
  else if (adr == 0xE1)
  {
    scc->reg[adr-0xC0] = val;
    scc->ch_enable_next = (uint8_t) val & 0x1F;
  }
  else if (adr == 0xE2)
  {
    scc->reg[adr-0xC0] = val;
    scc->cycle_4bit = val & 1;
    scc->cycle_8bit = val & 2;
    scc->refresh = val & 32;
    if (val & 64)
      for (ch = 0; ch < 5; ch++)
        scc->rotate[ch] = 0x1F;
    else
      for (ch = 0; ch < 5; ch++)
        scc->rotate[ch] = 0;
    if (val & 128)
      scc->rotate[3] = scc->rotate[4] = 0x1F;
  }

  return;
}

static inline void
write_standard (SCC * scc, uint32_t adr, uint32_t val)
{
  adr &= 0xFF;

  if (adr < 0x80)               /* wave */
  {
    SCC_writeReg (scc, adr, val);
  }
  else if (adr < 0x8A)          /* freq */
  {
    SCC_writeReg (scc, adr + 0xC0 - 0x80, val);
  }
  else if (adr < 0x8F)          /* volume */
  {
    SCC_writeReg (scc, adr + 0xD0 - 0x8A, val);
  }
  else if (adr == 0x8F)         /* ch enable */
  {
    SCC_writeReg (scc, 0xE1, val);
  }
  else if (0xE0 <= adr)         /* flags */
  {
    SCC_writeReg (scc, 0xE2, val);
  }
}

static inline void
write_enhanced (SCC * scc, uint32_t adr, uint32_t val)
{
  adr &= 0xFF;

  if (adr < 0xA0)               /* wave */
  {
    SCC_writeReg (scc, adr, val);
  }
  else if (adr < 0xAA)          /* freq */
  {
    SCC_writeReg (scc, adr + 0xC0 - 0xA0, val);
  }
  else if (adr < 0xAF)          /* volume */
  {
    SCC_writeReg (scc, adr + 0xD0 - 0xAA, val);
  }
  else if (adr == 0xAF)         /* ch enable */
  {
    SCC_writeReg (scc, 0xE1, val);
  }
  else if (0xC0 <= adr && adr <= 0xDF)  /* flags */
  {
    SCC_writeReg (scc, 0xE2, val);
  }
}

static inline uint32_t 
read_enhanced (SCC * scc, uint32_t adr)
{
  adr &= 0xFF;
  if (adr < 0xA0)
    return SCC_readReg (scc, adr);
  else if (adr < 0xAA)
    return SCC_readReg (scc, adr + 0xC0 - 0xA0);
  else if (adr < 0xAF)
    return SCC_readReg (scc, adr + 0xD0 - 0xAA);
  else if (adr == 0xAF)
    return SCC_readReg (scc, 0xE1);
  else if (0xC0 <= adr && adr <= 0xDF)
    return SCC_readReg (scc, 0xE2);
  else
    return 0;
}

static inline uint32_t
read_standard (SCC * scc, uint32_t adr)
{
  adr &= 0xFF;
  if(adr<0x80)
    return SCC_readReg (scc, adr);
  else if (0xA0<=adr&&adr<=0xBF)
    return SCC_readReg (scc, 0x80+(adr&0x1F));
  else if (adr < 0x8A)          
    return SCC_readReg (scc, adr + 0xC0 - 0x80);
  else if (adr < 0x8F)          
    return SCC_readReg (scc, adr + 0xD0 - 0x8A);
  else if (adr == 0x8F)         
    return SCC_readReg (scc, 0xE1);
  else if (0xE0 <= adr)         
    return SCC_readReg (scc, 0xE2);
  else return 0;
}

uint32_t
SCC_read (SCC * scc, uint32_t adr)
{
  if( scc->type == SCC_ENHANCED && (adr&0xFFFE) == 0xBFFE ) 
    return (scc->base_adr>>8)&0x20;
  
  if( adr < scc->base_adr ) return 0;
  adr -= scc->base_adr;
  
  if( adr == 0 ) 
  {
    if(scc->mode) return 0x80; else return 0x3F;
  }

  if(!scc->active||adr<0x800||0x8FF<adr) return 0;

  switch (scc->type) 
  {
  case SCC_STANDARD:
      return read_standard (scc, adr);
    break;
  case SCC_ENHANCED:
    if(!scc->mode)
      return read_standard (scc, adr);
    else 
      return read_enhanced (scc, adr);
    break;
  default:
    break;
  }

  return 0;
}

void
SCC_write (SCC * scc, uint32_t adr, uint32_t val)
{
  val = val & 0xFF;

  if( scc->type == SCC_ENHANCED && (adr&0xFFFE) == 0xBFFE ) 
  {
    scc->base_adr = 0x9000 | ((val&0x20)<<8);
    return;
  }
  
  if( adr < scc->base_adr ) return;
  adr -= scc->base_adr;

  if(adr == 0) 
  {
    if( val == 0x3F ) 
    {
      scc->mode = 0;
      scc->active = 1;
    }
    else if( val&0x80 && scc->type == SCC_ENHANCED)
    {
      scc->mode = 1;
      scc->active = 1;
    }
    else
    {
      scc->mode = 0;
      scc->active = 0;
    }
    return;
  }
  
  if(!scc->active||adr<0x800||0x8FF<adr) return;

  switch (scc->type) 
  {
  case SCC_STANDARD:
      write_standard (scc, adr, val);
    break;
  case SCC_ENHANCED:
    if(scc->mode)
      write_enhanced (scc, adr, val);
    else 
      write_standard (scc, adr, val);
  default:
    break;
  }

  return;
}

void
SCC_set_type (SCC * scc, uint32_t type)
{
  scc->type = type;
}
