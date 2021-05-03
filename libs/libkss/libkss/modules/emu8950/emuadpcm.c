/*
 * ADPCM for Y8950 version test. No timer, no A/D converter.
 *
 */
#include "emuadpcm.h"
#include "assert.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#define DMAX 0x5FFF
#define DMIN 0x7F
#define DDEF 0x7F

#define DECODE_MAX 32767
#define DECODE_MIN (-32768)

#define CLAP(min, x, max) ((x < min) ? min : (max < x) ? max : x)

/* Bitmask for register $04 */
#define R04_ST1 1          /* Timer1 Start */
#define R04_ST2 2          /* Timer2 Start */
#define R04_MASK_BUF_RDY 8 /* Mask 'Buffer Ready' signal */
#define R04_MASK_EOS 16    /* Mask 'End of sequence' */
#define R04_MASK_T2 32     /* Mask Timer2 flag */
#define R04_MASK_T1 64     /* Mask Timer1 flag */
#define R04_IRQ_RESET 128  /* IRQ RESET */

/* Bitmask for register $07 */
#define R07_RESET 1
#define R07_SP_OFF 8
#define R07_REPEAT 16
#define R07_MEMORY_DATA 32
#define R07_REC 64
#define R07_START 128

/* Bitmask for register $08 */
#define R08_ROM 1
#define R08_64K 2
#define R08_DA_AD 4
#define R08_SAMPL 8
#define R08_NOTE_SET 64
#define R08_CSM 128

/* Bitmask for status register */
#define STATUS_EOS (R04_MASK_EOS | 0x80)
#define STATUS_BUF_RDY (R04_MASK_BUF_RDY | 0x80)
#define STATUS_T2 (R04_MASK_T2 | 0x80)
#define STATUS_T1 (R04_MASK_T1 | 0x80)

#define RAM_SIZE (256 * 1024)
#define ROM_SIZE (256 * 1024)

#define rate_adjust(x, clk, rate) (uint32_t)((double)(x)*clk / 72 / rate + 0.5) /* +0.5 to round */

FILE *fp;

KSSOPL_ADPCM *KSSOPL_ADPCM_new(uint32_t clk, uint32_t rate) {
  KSSOPL_ADPCM *_this;

  _this = (KSSOPL_ADPCM *)malloc(sizeof(KSSOPL_ADPCM));
  if (!_this)
    return NULL;

  _this->clk = clk;
  _this->rate = rate ? rate : 44100;

  /* 256Kbytes RAM */
  _this->memory[0] = (uint8_t *)malloc(RAM_SIZE);
  if (!_this->memory[0])
    goto Error_Exit;
  memset(_this->memory[0], 0, RAM_SIZE);

  /* 256Kbytes ROM */
  _this->memory[1] = (uint8_t *)malloc(ROM_SIZE);
  if (!_this->memory[1])
    goto Error_Exit;
  memset(_this->memory[1], 0, ROM_SIZE);

  KSSOPL_ADPCM_reset(_this);

  return _this;

Error_Exit:
  KSSOPL_ADPCM_delete(_this);
  return NULL;
}

void KSSOPL_ADPCM_setRate(KSSOPL_ADPCM *_this, uint32_t r) { _this->rate = r ? r : 44100; }

void KSSOPL_ADPCM_delete(KSSOPL_ADPCM *_this) {
  if (_this) {
    free(_this->memory[0]);
    free(_this->memory[1]);
    free(_this);
  }
}

void KSSOPL_ADPCM_reset(KSSOPL_ADPCM *_this) {
  int i;

  for (i = 0; i < 0x20; i++)
    _this->reg[i] = 0;

  _this->play_start = 0;
  _this->status = 0;
  _this->play_addr = 0;
  _this->start_addr = 0;
  _this->stop_addr = 0;
  _this->delta_addr = 0;
  _this->delta_n = 0;
  _this->wave = _this->memory[0];
  _this->play_addr_mask = _this->reg[0x08] & R08_64K ? (1 << 17) - 1 : (1 << 19) - 1;
  _this->output[0] = _this->output[1] = 0;
  _this->reg[0x04] = 0x18;
}

#define GETA_BITS 14
#define DELTA_ADDR_MAX (1 << (16 + GETA_BITS))
#define DELTA_ADDR_MASK (DELTA_ADDR_MAX - 1)

/* Update KSSOPL_ADPCM data stage (Register $0F) */
static inline int update_stage(KSSOPL_ADPCM *_this) {
  _this->delta_addr += _this->delta_n;

  if (_this->delta_addr & DELTA_ADDR_MAX) {
    _this->delta_addr &= DELTA_ADDR_MASK;
    _this->play_addr = (_this->play_addr + 1) & (_this->play_addr_mask);

    if (_this->play_addr == (_this->stop_addr & _this->play_addr_mask)) {
      if (_this->reg[0x07] & R07_REPEAT) {
        _this->play_addr = _this->start_addr & (_this->play_addr_mask);
      } else {
        _this->play_start = 0;
        _this->status |= STATUS_EOS;
      }
    } else {
      _this->reg[0x0F] = _this->wave[_this->play_addr >> 1];
    }

    return 1;
  }

  return 0;
}

static inline void update_output(KSSOPL_ADPCM *_this, uint32_t val) {
  static uint32_t F[] = {
      57, 57, 57, 57, 77, 102, 128, 153 // This table values are from ymdelta.c by Tatsuyuki Satoh.
  };

  _this->output[1] = _this->output[0];

  if (val & 8)
    _this->output[0] -= (_this->diff * ((val & 7) * 2 + 1)) >> 3;
  else
    _this->output[0] += (_this->diff * ((val & 7) * 2 + 1)) >> 3;

  _this->output[0] = CLAP(DECODE_MIN, _this->output[0], DECODE_MAX);
  _this->diff = CLAP(DMIN, (_this->diff * F[val & 7]) >> 6, DMAX);
}

static inline uint32_t calc(KSSOPL_ADPCM *_this) {
  uint32_t val;

  if (_this->play_start && update_stage(_this)) {
    if (_this->play_addr & 1)
      val = _this->reg[0x0F] & 0x0F;
    else
      val = _this->reg[0x0F] >> 4;

    update_output(_this, val);
  }

  return ((_this->output[0] + _this->output[1]) * (_this->reg[0x12] & 0xff)) >> 13;
}

int16_t KSSOPL_ADPCM_calc(KSSOPL_ADPCM *_this) {
  if (_this->reg[0x07] & R07_SP_OFF)
    return 0;

  return calc(_this);
}

/* mode= 0:RAM256k 1:ROM 2:RAM64k */
uint32_t decode_start_address(uint8_t mode, uint8_t l, uint8_t h) {
  switch (mode) {
  case 0:
    return ((h << 8) | l) << 2;
  default:
    return ((h << 8) | l) << 5;  
  }
}

uint32_t decode_stop_address(uint8_t mode, uint8_t l, uint8_t h) {
  switch (mode) {
  case 0:
    return (((h << 8) | l) << 2) | 3;
  default:
    return (((h << 8) | l) << 5) | 31;
  }
}

void KSSOPL_ADPCM_writeReg(KSSOPL_ADPCM *_this, uint32_t adr, uint32_t data) {
  adr &= 0x1f;
  data &= 0xff;

  switch (adr) {
  case 0x00: /* NOT USED */
    _this->reg[0x00] = data;
    break;

  case 0x01: /* TEST */
    _this->reg[0x01] = data;
    break;

  case 0x02: /* TIMER1 (reso. 80us) */
    _this->reg[0x02] = data;
    break;

  case 0x03: /* TIMER2 (reso. 320us) */
    _this->reg[0x03] = data;
    break;

  case 0x04: /* FLAG CONTROL */
    if (data & R04_IRQ_RESET) {
      _this->status &= (data & 0x78);
    } else {
      _this->reg[0x04] = data;
    }
    assert((_this->reg[0x04] & 0x80) == 0);
    break;

  case 0x05: /* (KEYBOARD IN) */
    _this->reg[0x05] = data;
    break;

  case 0x06: /* (KEYBOARD OUT) */
    _this->reg[0x06] = data;
    break;

  case 0x07: /* START/REC/MEM DATA/REPEAT/SP-OFF/RESET */
    if (data & R07_RESET) {
      _this->play_start = 0;
      break;
    } else if (data & R07_START) {
      _this->play_start = 1;
      _this->play_addr = _this->start_addr & _this->play_addr_mask;
      _this->delta_addr = 0;
      _this->output[0] = 0;
      _this->output[1] = 0;
      _this->diff = DDEF;
    }
    _this->reg[0x07] = data;
    break;

  case 0x08: /* CSM/KEY BOARD SPLIT/SAMPLE/DA AD/64K/ROM */
    _this->reg[0x08] = data;
    _this->wave = _this->reg[0x08] & R08_ROM ? _this->memory[1] : _this->memory[0];
    _this->play_addr_mask = _this->reg[0x08] & R08_64K ? (1 << 17) - 1 : (1 << 19) - 1;
    break;

  case 0x09: /* START ADDRESS (L) */
  case 0x0A: /* START ADDRESS (H) */
    _this->reg[adr] = data;
    _this->start_addr = decode_start_address(_this->reg[0x08] & 3, _this->reg[0x09], _this->reg[0x0A]) << 1;
    break;

  case 0x0B: /* STOP ADDRESS (L) */
  case 0x0C: /* STOP ADDRESS (H) */
    _this->reg[adr] = data;
    _this->stop_addr = decode_stop_address(_this->reg[0x08] & 3, _this->reg[0x0B], _this->reg[0x0C]) << 1;
    break;

  case 0x0D: /* PRESCALE (L) */
    _this->reg[0x0D] = data;
    break;

  case 0x0E: /* PRESCALE (H) */
    _this->reg[0x0E] = data;
    break;

  case 0x0F: /* KSSOPL_ADPCM-DATA */
    _this->reg[0x0F] = data;

    if ((_this->reg[0x07] & R07_REC) && (_this->reg[0x07] & R07_MEMORY_DATA)) {
      _this->wave[_this->play_addr >> 1] = data;
      _this->play_addr = (_this->play_addr + 2) & (_this->play_addr_mask);
      if (_this->play_addr >= (_this->stop_addr & _this->play_addr_mask)) {
        //_this->status |= STATUS_EOS; /* Bug? */
      }
    }
    _this->status |= STATUS_BUF_RDY;
    break;

  case 0x10: /* DELTA-N (L) */
  case 0x11: /* DELTA-N (H) */
    _this->reg[adr] = data;
    _this->delta_n = rate_adjust(((_this->reg[0x11] << 8) | _this->reg[0x10]) << GETA_BITS, _this->clk, _this->rate);
    break;

  case 0x12: /* ENVELOP CONTROL */
    _this->reg[0x12] = data;
    break;

  default:
    break;
  }
}

uint8_t KSSOPL_ADPCM_status(KSSOPL_ADPCM *_this) { return _this->status; }

void KSSOPL_ADPCM_writeRAM(KSSOPL_ADPCM *_this, uint32_t start, uint32_t length, const uint8_t *data) {
  if (start >= RAM_SIZE) return;
  if (start + length > RAM_SIZE) {
    length = RAM_SIZE - start;
  }
  memcpy(_this->memory[0] + start, data, length);
}

void KSSOPL_ADPCM_writeROM(KSSOPL_ADPCM *_this, uint32_t start, uint32_t length, const uint8_t *data) {
  if (start >= ROM_SIZE) return;
  if (start + length > ROM_SIZE) {
    length = ROM_SIZE - start;
  }
  memcpy(_this->memory[1] + start, data, length);
}
