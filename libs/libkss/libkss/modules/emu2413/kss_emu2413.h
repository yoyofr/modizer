#ifndef _EMU2413KSS_H_
#define _EMU2413KSS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OPLLKSS_DEBUG 0

enum OPLLKSS_TONE_ENUM { OPLLKSS_2413_TONE = 0, OPLLKSS_VRC7_TONE = 1, OPLLKSS_281B_TONE = 2 };

/* voice data */
typedef struct __OPLLKSS_PATCH {
  uint32_t TL, FB, EG, ML, AR, DR, SL, RR, KR, KL, AM, PM, WS;
} OPLLKSS_PATCH;

/* slot */
typedef struct __OPLLKSS_SLOT {
  uint8_t number;

  /* type flags:
   * 000000SM
   *       |+-- M: 0:modulator 1:carrier
   *       +--- S: 0:normal 1:single slot mode (sd, tom, hh or cym)
   */
  uint8_t type;

  OPLLKSS_PATCH *patch; /* voice parameter */

  /* slot output */
  int32_t output[2]; /* output value, latest and previous. */

  /* phase generator (pg) */
  uint16_t *wave_table; /* wave table */
  uint32_t pg_phase;    /* pg phase */
  uint32_t pg_out;      /* pg output, as index of wave table */
  uint8_t pg_keep;      /* if 1, pg_phase is preserved when key-on */
  uint16_t blk_fnum;    /* (block << 9) | f-number */
  uint16_t fnum;        /* f-number (9 bits) */
  uint8_t blk;          /* block (3 bits) */

  /* envelope generator (eg) */
  uint8_t eg_state;  /* current state */
  int32_t volume;    /* current volume */
  uint8_t key_flag;  /* key-on flag 1:on 0:off */
  uint8_t sus_flag;  /* key-sus option 1:on 0:off */
  uint16_t tll;      /* total level + key scale level*/
  uint8_t rks;       /* key scale offset (rks) for eg speed */
  uint8_t eg_rate_h; /* eg speed rate high 4bits */
  uint8_t eg_rate_l; /* eg speed rate low 2bits */
  uint32_t eg_shift; /* shift for eg global counter, controls envelope speed */
  uint32_t eg_out;   /* eg output */

  uint32_t update_requests; /* flags to debounce update */

#if OPLLKSS_DEBUG
  uint8_t last_eg_state;
#endif
} OPLLKSS_SLOT;

/* mask */
#define OPLLKSS_MASK_CH(x) (1 << (x))
#define OPLLKSS_MASK_HH (1 << (9))
#define OPLLKSS_MASK_CYM (1 << (10))
#define OPLLKSS_MASK_TOM (1 << (11))
#define OPLLKSS_MASK_SD (1 << (12))
#define OPLLKSS_MASK_BD (1 << (13))
#define OPLLKSS_MASK_RHYTHM (OPLLKSS_MASK_HH | OPLLKSS_MASK_CYM | OPLLKSS_MASK_TOM | OPLLKSS_MASK_SD | OPLLKSS_MASK_BD)

/* rate conveter */
typedef struct __OPLLKSS_RateConv {
  int ch;
  double timer;
  double f_ratio;
  int16_t *sinc_table;
  int16_t **buf;
} OPLLKSS_RateConv;

OPLLKSS_RateConv *OPLLKSS_RateConv_new(double f_inp, double f_out, int ch);
void OPLLKSS_RateConv_reset(OPLLKSS_RateConv *conv);
void OPLLKSS_RateConv_putData(OPLLKSS_RateConv *conv, int ch, int16_t data);
int16_t OPLLKSS_RateConv_getData(OPLLKSS_RateConv *conv, int ch);
void OPLLKSS_RateConv_delete(OPLLKSS_RateConv *conv);

typedef struct __OPLLKSS {
  uint32_t clk;
  uint32_t rate;

  uint8_t chip_type;

  uint32_t adr;

  double inp_step;
  double out_step;
  double out_time;

  uint8_t reg[0x40];
  uint8_t test_flag;
  uint32_t slot_key_status;
  uint8_t rhythm_mode;

  uint32_t eg_counter;

  uint32_t pm_phase;
  int32_t am_phase;

  uint8_t lfo_am;

  uint32_t noise;
  uint8_t short_noise;

  int32_t patch_number[9];
  OPLLKSS_SLOT slot[18];
  OPLLKSS_PATCH patch[19 * 2];

  uint8_t pan[16];
  float pan_fine[16][2];

  uint32_t mask;

  /* channel output */
  /* 0..8:tone 9:bd 10:hh 11:sd 12:tom 13:cym */
  int16_t ch_out[14];

  int16_t mix_out[2];

  OPLLKSS_RateConv *conv;
} OPLLKSS;

OPLLKSS *OPLLKSS_new(uint32_t clk, uint32_t rate);
void OPLLKSS_delete(OPLLKSS *);

void OPLLKSS_reset(OPLLKSS *);
void OPLLKSS_resetPatch(OPLLKSS *, uint8_t);

/**
 * Set output wave sampling rate.
 * @param rate sampling rate. If clock / 72 (typically 49716 or 49715 at 3.58MHz) is set, the internal rate converter is
 * disabled.
 */
void OPLLKSS_setRate(OPLLKSS *opll, uint32_t rate);

/**
 * Set internal calcuration quality. Currently no effects, just for compatibility.
 * >= v1.0.0 always synthesizes internal output at clock/72 Hz.
 */
void OPLLKSS_setQuality(OPLLKSS *opll, uint8_t q);

/**
 * Set pan pot (extra function - not YM2413 chip feature)
 * @param ch 0..8:tone 9:bd 10:hh 11:sd 12:tom 13:cym 14,15:reserved
 * @param pan 0:mute 1:right 2:left 3:center
 * ```
 * pan: 76543210
 *            |+- bit 1: enable Left output
 *            +-- bit 0: enable Right output
 * ```
 */
void OPLLKSS_setPan(OPLLKSS *opll, uint32_t ch, uint8_t pan);

/**
 * Set fine-grained panning
 * @param ch 0..8:tone 9:bd 10:hh 11:sd 12:tom 13:cym 14,15:reserved
 * @param pan output strength of left/right channel.
 *            pan[0]: left, pan[1]: right. pan[0]=pan[1]=1.0f for center.
 */
void OPLLKSS_setPanFine(OPLLKSS *opll, uint32_t ch, float pan[2]);

/**
 * Set chip type. If vrc7 is selected, r#14 is ignored.
 * This method not change the current ROM patch set.
 * To change ROM patch set, use OPLLKSS_resetPatch.
 * @param type 0:YM2413 1:VRC7
 */
void OPLLKSS_setChipType(OPLLKSS *opll, uint8_t type);

void OPLLKSS_writeIO(OPLLKSS *opll, uint32_t reg, uint8_t val);
void OPLLKSS_writeReg(OPLLKSS *opll, uint32_t reg, uint8_t val);

/**
 * Calculate one sample
 */
int16_t OPLLKSS_calc(OPLLKSS *opll);

/**
 * Calulate stereo sample
 */
void OPLLKSS_calcStereo(OPLLKSS *opll, int32_t out[2]);

void OPLLKSS_setPatch(OPLLKSS *, const uint8_t *dump);
void OPLLKSS_copyPatch(OPLLKSS *, int32_t, OPLLKSS_PATCH *);

/**
 * Force to refresh.
 * External program should call this function after updating patch parameters.
 */
void OPLLKSS_forceRefresh(OPLLKSS *);

void OPLLKSS_dumpToPatch(const uint8_t *dump, OPLLKSS_PATCH *patch);
void OPLLKSS_patchToDump(const OPLLKSS_PATCH *patch, uint8_t *dump);
void OPLLKSS_getDefaultPatch(int32_t type, int32_t num, OPLLKSS_PATCH *);

/**
 *  Set channel mask
 *  @param mask mask flag: OPLLKSS_MASK_* can be used.
 *  - bit 0..8: mask for ch 1 to 9 (OPLLKSS_MASK_CH(i))
 *  - bit 9: mask for Hi-Hat (OPLLKSS_MASK_HH)
 *  - bit 10: mask for Top-Cym (OPLLKSS_MASK_CYM)
 *  - bit 11: mask for Tom (OPLLKSS_MASK_TOM)
 *  - bit 12: mask for Snare Drum (OPLLKSS_MASK_SD)
 *  - bit 13: mask for Bass Drum (OPLLKSS_MASK_BD)
 */
uint32_t OPLLKSS_setMask(OPLLKSS *, uint32_t mask);

/**
 * Toggler channel mask flag
 */
uint32_t OPLLKSS_toggleMask(OPLLKSS *, uint32_t mask);

/* for compatibility */
#define OPLLKSS_set_rate OPLLKSS_setRate
#define OPLLKSS_set_quality OPLLKSS_setQuality
#define OPLLKSS_set_pan OPLLKSS_setPan
#define OPLLKSS_set_pan_fine OPLLKSS_setPanFine
#define OPLLKSS_calc_stereo OPLLKSS_calcStereo
#define OPLLKSS_reset_patch OPLLKSS_resetPatch
#define OPLLKSS_dump2patch OPLLKSS_dumpToPatch
#define OPLLKSS_patch2dump OPLLKSS_patchToDump
#define OPLLKSS_setChipMode OPLLKSS_setChipType

#ifdef __cplusplus
}
#endif

#endif
