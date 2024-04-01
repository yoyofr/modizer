/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2021 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#ifndef _GBHW_H_
#define _GBHW_H_

#include <inttypes.h>

#include "common.h"
#include "libgbs.h"
#include "gbcpu.h"
#include "gblfsr.h"

#define GBHW_CLOCK 4194304

#define GBHW_INTRAM_SIZE 0x2000
#define GBHW_INTRAM_MASK (GBHW_INTRAM_SIZE - 1)
#define GBHW_IOREGS_SIZE 0x80
#define GBHW_IOREGS_MASK (GBHW_IOREGS_SIZE - 1)
#define GBHW_HIRAM_SIZE  0x80
#define GBHW_HIRAM_MASK  (GBHW_HIRAM_SIZE - 1)

#define GBHW_BOOT_ROM_SIZE 256

struct gbhw_buffer {
	int16_t *data;   /* only for soundbuf */
	int32_t *data32; /* only for impbuf */
	long pos;
	long l_lvl;
	long r_lvl;
	long l_cap;
	long r_cap;
    //YOYOFR
    long lvl_ch[4];
    long cap_ch[4];    
    //YOYOFR
	long bytes;
	long samples;
	cycles_t cycles;
};

struct gbhw_channel {
	long mute;
	long running;
	long master;
	long leftgate;
	long rightgate;
	long lvl;
    long last_lvl;//yoyofr
	long volume;
	long env_volume;
	long env_dir;
	long env_tc;
	long env_ctr;
	long sweep_dir;
	long sweep_tc;
	long sweep_ctr;
	long sweep_shift;
	long len;
	long len_enable;
	long len_gate;
	long div_tc;
	long div_tc_shadow;
	long div_ctr;
	long duty_val;
	long duty_ctr;
};

typedef void (*gbhw_callback_fn)(void *priv);
typedef void (*gbhw_iocallback_fn)(cycles_t cycles, uint32_t addr, uint8_t value, void *priv);
typedef void (*gbhw_stepcallback_fn)(const cycles_t cycles, const struct gbhw_channel[], void *priv);

struct gbhw {
	long apu_on;
	long io_written;

	long lminval, lmaxval, rminval, rmaxval;
	double filter_constant;
	int filter_enabled;
	long cap_factor;

	long master_volume;
	long master_fade;
	long master_dstvol;
	long sample_rate;
	long update_level;
	long sequence_ctr;
	cycles_t halted_noirq_cycles;

	long vblankctr;
	long timertc;
	long timerctr;
	long divoffset;

	cycles_t sum_cycles;

	long rom_lockout;

	gbhw_callback_fn callback;
	void *callbackpriv;
	struct gbhw_buffer *soundbuf; /* externally visible output buffer */
	struct gbhw_buffer *impbuf;   /* internal impulse output buffer */

	gbhw_iocallback_fn iocallback;
	void *iocallback_priv;

	gbhw_stepcallback_fn stepcallback;
	void *stepcallback_priv;

	struct gblfsr lfsr;

	long long sound_div_tc;
	long main_div;
	long sweep_div;

	long ch3pos;
	long last_l_value, last_r_value;
	long ch3_next_nibble;

	struct gbcpu gbcpu;

	struct gbhw_channel ch[4];

	uint8_t hiram[GBHW_HIRAM_SIZE];
	uint8_t intram[GBHW_INTRAM_SIZE];
	uint8_t ioregs[GBHW_IOREGS_SIZE];

	uint8_t boot_rom[GBHW_BOOT_ROM_SIZE];
	struct get_entry boot_shadow_get;
	struct put_entry boot_shadow_put;
};

void gbhw_set_callback(struct gbhw* const gbhw, gbhw_callback_fn fn, void *priv);
void gbhw_set_io_callback(struct gbhw* const gbhw, gbhw_iocallback_fn fn, void *priv);
void gbhw_set_step_callback(struct gbhw* const gbhw, gbhw_stepcallback_fn fn, void *priv);
long gbhw_set_filter(struct gbhw* const gbhw, enum gbs_filter_type type);
void gbhw_set_rate(struct gbhw* const gbhw, long rate);
void gbhw_set_buffer(struct gbhw* const gbhw, struct gbhw_buffer *buffer);
void gbhw_init(struct gbhw* const gbhw);
void gbhw_init_struct(struct gbhw* const gbhw);
void gbhw_cleanup(struct gbhw* const gbhw);
void gbhw_enable_bootrom(struct gbhw* const gbhw, const uint8_t *rombuf);
void gbhw_master_fade(struct gbhw* const gbhw, long speed, long dstvol);
void gbhw_calc_minmax(struct gbhw* const gbhw, int16_t *lmin, int16_t *lmax, int16_t *rmin, int16_t *rmax);
float gbhw_calc_timer_hz(uint8_t tac, uint8_t tma);
cycles_t gbhw_step(struct gbhw* const gbhw, long time_to_work);
uint8_t gbhw_io_peek(const struct gbhw* const gbhw, uint16_t addr);  /* unmasked peek */
void gbhw_io_put(struct gbhw* const gbhw, uint16_t addr, uint8_t val);

#endif
