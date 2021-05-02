#ifndef _KSSPLAY_H_
#define _KSSPLAY_H_

#include <stdint.h>

#include "filters/filter.h"
#include "filters/rc_filter.h"
#include "filters/dc_filter.h"
#include "kss/kss.h"
#include "vm/vm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KSSPLAY_VOL_BITS 9
#define KSSPLAY_VOL_RANGE (48.0 * 2)
#define KSSPLAY_VOL_STEP (KSSPLAY_VOL_RANGE / (1 << KSSPLAY_VOL_BITS))
#define KSSPLAY_VOL_MAX ((1 << (KSSPLAY_VOL_BITS - 1)) - 1)
#define KSSPLAY_VOL_MIN (-(1 << (KSSPLAY_VOL_BITS - 1)))
#define KSSPLAY_MUTE (1 << KSSPLAY_VOL_BITS)
#define KSSPLAY_VOL_MASK ((1 << KSSPLAY_VOL_BITS) - 1)

typedef struct tagKSSPLAY KSSPLAY;
struct tagKSSPLAY {
  KSS *kss;

  uint8_t *main_data;
  uint8_t *bank_data;

  VM *vm;

  uint32_t rate;
  uint32_t nch;
  uint32_t bps;

  double step_cnt;
  double step_inc;

  int32_t master_volume;
  int32_t device_volume[EDSC_MAX];
  uint32_t device_mute[EDSC_MAX];
  int32_t device_pan[EDSC_MAX];
  uint32_t device_type[EDSC_MAX];
  FIR *device_fir[2][EDSC_MAX];
  RCF *rcf[2];
  DCF *dcf[2];
  uint32_t lastout[2];

  uint32_t fade;
  uint32_t fade_step;
  uint32_t fade_flag;
  uint32_t silent;
  uint32_t silent_limit;
  int32_t decoded_length;

  uint32_t cpu_speed; /* 0: AutoDetect, 1...N: x1 ... xN */
  double vsync_freq;

  int scc_disable;
  int opll_stereo;
};

/**
 * psg[0-2]: SQUARE CH 1-3
 *
 * scc[0-4]: WAVE CH 1-5
 *
 * opll[0-8]: FM CH 1-9
 * opll[9]  : Bass Drum
 * opll[10] : Hi-Hat
 * opll[11] : Snare
 * opll[12] : Tom
 * opll[13] : Cymbal
 * opll[14] : DAC (Not supported yet)
 *
 * opl[0-8]: FM CH 1-9
 * opl[9]  : Bass Drum (Not supported yet)
 * opl[10] : Hi-Hat (Not supported yet)
 * opl[11] : Snare (Not supported yet)
 * opl[12] : Tom (Not supported yet)
 * opl[13] : Cymbal (Not supported yet)
 * opl[14] : ADPCM
 *
 * sng[0-2] :SQUARE CH 1-3
 * sng[3]   :NOISE
 *
 * dac[0]: 1-bit DAC
 * dac[1]: SCC 8-bit DAC
 */
typedef struct tagKSSPLAY_PER_CH_OUT {
  int16_t psg[3];
  int16_t scc[5];
  int16_t opll[15];
  int16_t opl[15];
  int16_t sng[4];
  int16_t dac[2];
} KSSPLAY_PER_CH_OUT;

enum { KSSPLAY_FADE_NONE = 0, KSSPLAY_FADE_OUT = 1, KSSPLAY_FADE_END = 2 };

/* Create KSSPLAY object */
KSSPLAY *KSSPLAY_new(uint32_t rate, uint32_t nch, uint32_t bps);

/* Set KSS data to KSSPLAY object */
int KSSPLAY_set_data(KSSPLAY *, KSS *kss);

/* Reset KSSPLAY object */
/* cpu_speed = 0:auto 1:3.58MHz 2:7.16MHz ... */
/* scc_type = 0:auto 1: standard 2:enhanced */
void KSSPLAY_reset(KSSPLAY *, uint32_t song, uint32_t cpu_speed);

/* Calcurate wave samples as int16 array */
void KSSPLAY_calc(KSSPLAY *, int16_t *buf, uint32_t length);

/* Calcurate samples internally. No output is generated. */
void KSSPLAY_calc_silent(KSSPLAY *, uint32_t length);

/**
 * Calcurate wave samples as a sequence of KSSPLAY_PER_CH_OUT struct.
 * This function generates raw wave samples. All volume and filter settings are ignored.
 */
void KSSPLAY_calc_per_ch(KSSPLAY *kssplay, KSSPLAY_PER_CH_OUT *per_ch_out, uint32_t length);

/* Delete KSSPLAY object */
void KSSPLAY_delete(KSSPLAY *);

/* Start fadeout */
void KSSPLAY_fade_start(KSSPLAY *kssplay, uint32_t fade_time);

/* Stop fadeout */
void KSSPLAY_fade_stop(KSSPLAY *kssplay);

int KSSPLAY_read_memory(KSSPLAY *kssplay, uint32_t address);

int KSSPLAY_get_loop_count(KSSPLAY *kssplay);
int KSSPLAY_get_stop_flag(KSSPLAY *kssplay);
int KSSPLAY_get_fade_flag(KSSPLAY *kssplay);

void KSSPLAY_set_rcf(KSSPLAY *kssplay, uint32_t r, uint32_t c);
void KSSPLAY_set_opll_patch(KSSPLAY *kssplay, uint8_t *illdata);
void KSSPLAY_set_cpu_speed(KSSPLAY *kssplay, uint32_t cpu_speed);
void KSSPLAY_set_device_lpf(KSSPLAY *kssplay, uint32_t device, uint32_t cutoff);
void KSSPLAY_set_device_mute(KSSPLAY *kssplay, uint32_t devnum, uint32_t mute);
void KSSPLAY_set_device_pan(KSSPLAY *kssplay, uint32_t devnum, int32_t pan);
void KSSPLAY_set_channel_pan(KSSPLAY *kssplay, uint32_t device, uint32_t ch, uint32_t pan);
void KSSPLAY_set_device_quality(KSSPLAY *kssplay, uint32_t device, uint32_t quality);
void KSSPLAY_set_device_volume(KSSPLAY *kssplay, uint32_t devnum, int32_t vol);
void KSSPLAY_set_master_volume(KSSPLAY *kssplay, int32_t vol);
void KSSPLAY_set_device_mute(KSSPLAY *kssplay, uint32_t devnum, uint32_t mute);
void KSSPLAY_set_channel_mask(KSSPLAY *kssplay, uint32_t device, uint32_t mask);
void KSSPLAY_set_device_type(KSSPLAY *kssplay, uint32_t devnum, uint32_t type);
void KSSPLAY_set_silent_limit(KSSPLAY *kssplay, uint32_t time_in_ms);

void KSSPLAY_set_iowrite_handler(KSSPLAY *kssplay, void *context, void (*handler)(void *context, uint32_t a, uint32_t d));
void KSSPLAY_set_memwrite_handler(KSSPLAY *kssplay, void *context, void (*handler)(void *context, uint32_t a, uint32_t d));

uint32_t KSSPLAY_get_device_volume(KSSPLAY *kssplay, uint32_t devnum);
void KSSPLAY_get_MGStext(KSSPLAY *kssplay, char *buf, int max);
uint8_t KSSPLAY_get_MGS_jump_count(KSSPLAY *kssplay);

void KSSPLAY_write_memory(KSSPLAY *kssplay, uint32_t a, uint32_t d);
void KSSPLAY_write_io(KSSPLAY *kssplay, uint32_t a, uint32_t d);

#ifdef __cplusplus
}
#endif
#endif
