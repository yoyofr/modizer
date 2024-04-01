/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2021 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */


#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>

//TODO:  MODIZER changes start / YOYOFR
#include "../../../src/ModizerVoicesData.h"
extern int seek_needed;
//TODO:  MODIZER changes end / YOYOFR


#include "gbcpu.h"
#include "gbhw.h"
#include "impulse.h"

#define FILTER_CONST_OFF 1.0
/* From blargg's "Game Boy Sound Operation" doc */
#define FILTER_CONST_DMG 0.999958
#define FILTER_CONST_CGB 0.998943

#define REG_TIMA 0x05
#define REG_TMA  0x06
#define REG_TAC  0x07
#define REG_IF   0x0f
#define REG_IE   0x7f /* Nominally 0xff, but we remap it to 0x7f internally. */

static const uint8_t ioregs_ormask[GBHW_IOREGS_SIZE] = {
	/* 0x00 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* 0x10 */ 0x80, 0x3f, 0x00, 0xff, 0xbf,
	/* 0x15 */ 0xff, 0x3f, 0x00, 0xff, 0xbf,
	/* 0x1a */ 0x7f, 0xff, 0x9f, 0xff, 0xbf,
	/* 0x1f */ 0xff, 0xff, 0x00, 0x00, 0xbf,
	/* 0x24 */ 0x00, 0x00, 0x70, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};
static const uint8_t ioregs_initdata[GBHW_IOREGS_SIZE] = {
	/* 0x00 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* sound registers */
	/* 0x10 */ 0x80, 0xbf, 0x00, 0x00, 0xbf,
	/* 0x15 */ 0x00, 0x3f, 0x00, 0x00, 0xbf,
	/* 0x1a */ 0x7f, 0xff, 0x9f, 0x00, 0xbf,
	/* 0x1f */ 0x00, 0xff, 0x00, 0x00, 0xbf,
	/* 0x24 */ 0x77, 0xf3, 0xf1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/* wave pattern memory, taken from gbsound.txt v0.99.19 (12/31/2002) */
	/* 0x30 */ 0xac, 0xdd, 0xda, 0x48, 0x36, 0x02, 0xcf, 0x16, 0x2c, 0x04, 0xe5, 0x2c, 0xac, 0xdd, 0xda, 0x48,
};

/* Duty base counters:
 * a: 01010101
 * b: 00110011
 * c: 00001111
 *
 * Combinations:
 * c&b&!a: 00000010 (12.5%)
 * c&b:    00000011 (25%)
 * c:      00001111 (50%)
 * !(c&b): 11111100 (75%)
 */

static const char dutylookup[4] = {
	0x02, 0x03, 0x0f, 0xfc
};
static const long len_mask[4] = {
	0x3f, 0x3f, 0xff, 0x3f
};

#define MASTER_VOL_MIN	0
#define MASTER_VOL_MAX	(256*256)

static const long vblanktc = 70224; /* ~59.73 Hz (vblankctr)*/
static const long vblankclocks = 4560;

static const long msec_cycles = GBHW_CLOCK/1000;

#define SOUND_DIV_MULT 0x10000LL

#define IMPULSE_WIDTH (1 << IMPULSE_W_SHIFT)
#define IMPULSE_N (1 << IMPULSE_N_SHIFT)
#define IMPULSE_N_MASK (IMPULSE_N - 1)

static const long main_div_tc = 4;
static const long sweep_div_tc = 2048;

static inline long timertc_from_tac(uint8_t tac)
{
	static const long tac_to_cycles[4] = {
		/* As formula: 16 << (((tac + 3) & 3) << 1) */
		GBHW_CLOCK / 4096,    /* 1024 CPU cycles per TIMA tick */
		GBHW_CLOCK / 262144,  /*   16 CPU cycles per TIMA tick */
		GBHW_CLOCK / 65536,   /*   64 CPU cycles per TIMA tick */
		GBHW_CLOCK / 16384,   /*  256 CPU cycles per TIMA tick */
	};
	long timertc = tac_to_cycles[tac&3];
	if ((tac & 0xf0) == 0x80) timertc /= 2; /* emulate GBC mode */
	return timertc;
}

void gbhw_init_struct(struct gbhw *gbhw) {
	gbhw->apu_on = 1;
	gbhw->io_written = 0;

	gbhw->filter_constant = FILTER_CONST_DMG;
	gbhw->filter_enabled = 1;
	gbhw->cap_factor = 0x10000;

	gbhw->update_level = 0;
	gbhw->sequence_ctr = 0;
	gbhw->halted_noirq_cycles = 0;

	gbhw->timertc = 16;

	gbhw->rom_lockout = 1;

	gbhw->soundbuf = NULL; /* externally visible output buffer */
	gbhw->impbuf = NULL;   /* internal impulse output buffer */

	gblfsr_reset(&gbhw->lfsr);

	gbhw->sound_div_tc = 0;

	gbhw->last_l_value = 0;
	gbhw->last_r_value = 0;
	gbhw->ch3_next_nibble = 0;
    
    //YOYOFR
    for (int ii=0;ii<4;ii++) gbhw->ch[ii].last_lvl=0;
    //YOYOFR

	gbcpu_init_struct(&gbhw->gbcpu);
}

static uint32_t bootrom_get(void *priv, uint32_t addr)
{
	struct gbhw *gbhw = priv;
	if ((addr >> 8) == 0 && gbhw->rom_lockout == 0) {
		return gbhw->boot_rom[addr & 0xff];
	}
	return gbhw->boot_shadow_get.get(
		gbhw->boot_shadow_get.priv, addr);
}

static void bootrom_put(void *priv, uint32_t addr, uint8_t val)
{
	struct gbhw *gbhw = priv;
	gbhw->boot_shadow_put.put(
		gbhw->boot_shadow_put.priv, addr, val);
}

static uint32_t io_get(void *priv, uint32_t addr)
{
	struct gbhw *gbhw = priv;
	if (addr >= 0xff80 && addr <= 0xfffe) {
		return gbhw->hiram[addr & GBHW_HIRAM_MASK];
	}
	if (addr >= 0xff10 &&
	           addr <= 0xff3f) {
		uint8_t val = gbhw->ioregs[addr & GBHW_IOREGS_MASK];
		if (addr == 0xff26) {
			long i;
			val &= 0xf0;
			for (i=0; i<4; i++) {
				if (gbhw->ch[i].running) {
					val |= (1 << i);
				}
			}
		}
		val |= ioregs_ormask[addr & GBHW_IOREGS_MASK];
		DPRINTF("io_get(%04x): %02x\n", addr, val);
		return val;
	}
	switch (addr) {
	case 0xff00:  // P1
		return 0;
	case 0xff04:  // DIV
		// DIV increments at 16384Hz
		return ((gbhw->sum_cycles >> 8) + gbhw->divoffset) & 0xff;
	case 0xff05:  // TIMA
	case 0xff06:  // TMA
	case 0xff07:  // TAC
	case 0xff0f:  // IF
		return gbhw->ioregs[addr & GBHW_IOREGS_MASK];
	case 0xff41: /* LCDC Status */
		if (gbhw->vblankctr > vblanktc - vblankclocks) {
			return 0x01;  /* vblank */
		} else {
			/* ~108.7uS per line */
			long t = (2 * vblanktc - gbhw->vblankctr) % 456;
			if (t < 204) {
				/* 48.6uS in hblank (201-207 clks) */
				return 0x00;
			} else if (t < 284) {
				/* 19uS in OAM scan (77-83 clks) */
				return 0x02;
			}
		}
		return 0x03;  /* both OAM and display RAM busy */
	case 0xff44: /* LCD Y-coordinate */
		return ((2 * vblanktc - vblankclocks - gbhw->vblankctr) / 456) % 154;
	case 0xff70:  // CGB ram bank switch
		WARN_ONCE("ioread from SVBK (CGB mode) ignored.\n");
		return 0xff;
	case 0xffff:
		return gbhw->ioregs[0x7f];
	default:
		WARN_ONCE("ioread from 0x%04x unimplemented.\n", (unsigned int)addr);
		DPRINTF("io_get(%04x)\n", addr);
		return 0xff;
	}
}

static uint32_t intram_get(void *priv, uint32_t addr)
{
	struct gbhw *gbhw = priv;
//	DPRINTF("intram_get(%04x)\n", addr);
	return gbhw->intram[addr & GBHW_INTRAM_MASK];
}

static void apu_reset(struct gbhw *gbhw)
{
	long i;
	int mute_tmp[4];

	for (i = 0; i < 4; i++) {
		mute_tmp[i] = gbhw->ch[i].mute;
	}
	assert(sizeof(gbhw->ch) == sizeof(struct gbhw_channel) * 4);
	memset(gbhw->ch, 0, sizeof(gbhw->ch));
	for (i = 0xff10; i < 0xff26; i++) {
		gbhw->ioregs[i & GBHW_IOREGS_MASK] = 0;
	}
	for (i = 0; i < 4; i++) {
		gbhw->ch[i].len = 0;
		gbhw->ch[i].len_gate = 0;
		gbhw->ch[i].volume = 0;
		gbhw->ch[i].env_volume = 0;
		gbhw->ch[i].duty_ctr = 0;
		gbhw->ch[i].div_tc = 1;
		gbhw->ch[i].master = 1;
		gbhw->ch[i].running = 0;
		gbhw->ch[i].mute = mute_tmp[i];
	}
	gbhw->sequence_ctr = 0;
}

static void linkport_atexit(void);

static void linkport_write(long c)
{
	static char buf[256];
	static unsigned long idx = 0;
	static long exit_handler_set = 0;
	static long enabled = 1;

	if (!enabled) {
		return;
	}
	if (!(c == -1 || c == '\r' || c == '\n' || (c >= 0x20 && c <= 0x7f))) {
		enabled = 0;
		fprintf(stderr, "Link port output %02lx ignored.\n", c);
		return;
	}
	if (c != -1 && idx < (sizeof(buf) - 1)) {
		buf[idx++] = c;
		buf[idx] = 0;
	}
	if (c == '\n' || (c == -1 && idx > 0)) {
		fprintf(stderr, "Link port text: %s", buf);
		idx = 0;
		if (!exit_handler_set) {
			atexit(linkport_atexit);
		}
	}
}

static void linkport_atexit(void)
{
	linkport_write(-1);
}

static void sequencer_update_len(struct gbhw *gbhw, long chn)
{
	if (gbhw->ch[chn].len_enable && gbhw->ch[chn].len_gate) {
		gbhw->ch[chn].len++;
		gbhw->ch[chn].len &= len_mask[chn];
		if (gbhw->ch[chn].len == 0) {
			gbhw->ch[chn].env_volume = 0;
			gbhw->ch[chn].env_tc = 0;
			gbhw->ch[chn].running = 0;
			gbhw->ch[chn].len_gate = 0;
		}
	}
}

static long sweep_check_overflow(struct gbhw *gbhw)
{
	long val = (2048 - gbhw->ch[0].div_tc_shadow) >> gbhw->ch[0].sweep_shift;

	if (gbhw->ch[0].sweep_shift == 0) {
		return 1;
	}

	if (!gbhw->ch[0].sweep_dir) {
		if (gbhw->ch[0].div_tc_shadow <= val) {
			gbhw->ch[0].running = 0;
			return 0;
		}
	}
	return 1;
}

static void io_put(void *priv, uint32_t addr, uint8_t val)
{
	struct gbhw *gbhw = priv;
	long chn = (addr - 0xff10)/5;

	if (addr >= 0xff80 && addr <= 0xfffe) {
		gbhw->hiram[addr & GBHW_HIRAM_MASK] = val;
		return;
	}

	gbhw->io_written = 1;

	if (gbhw->iocallback)
		gbhw->iocallback(gbhw->sum_cycles, addr, val, gbhw->iocallback_priv);

	if (gbhw->apu_on == 0 && addr >= 0xff10 && addr < 0xff26) {
		return;
	}
	gbhw->ioregs[addr & GBHW_IOREGS_MASK] = val;
	DPRINTF(" ([0x%04x]=%02x) ", addr, val);
	switch (addr) {
		case 0xff02:
			if (val & 0x80) {
				linkport_write(gbhw->ioregs[1]);
			}
			break;
		case 0xff04:  // DIV
			// Writing DIV sets it to 0
			gbhw->divoffset = -(gbhw->sum_cycles >> 8);
			break;
		case 0xff05:  // TIMA
		case 0xff06:  // TMA
			break;
		case 0xff07:  // TAC
			gbhw->timertc = timertc_from_tac(val);
			if (gbhw->timerctr > gbhw->timertc) {
				gbhw->timerctr = 0;
			}
			break;
		case 0xff0f:  // IF
			break;
		case 0xff10:
			gbhw->ch[0].sweep_ctr = gbhw->ch[0].sweep_tc = ((val >> 4) & 7);
			gbhw->ch[0].sweep_dir = (val >> 3) & 1;
			gbhw->ch[0].sweep_shift = val & 7;

			break;
		case 0xff11:
		case 0xff16:
		case 0xff20:
			{
				long duty_ctr = val >> 6;
				long len = val & 0x3f;

				gbhw->ch[chn].duty_val = dutylookup[duty_ctr];
				gbhw->ch[chn].len = len;
				gbhw->ch[chn].len_gate = 1;

				break;
			}
		case 0xff12:
		case 0xff17:
		case 0xff21:
			{
				long vol = val >> 4;
				long envdir = (val >> 3) & 1;
				long envspd = val & 7;

				gbhw->ch[chn].volume = vol;
				gbhw->ch[chn].env_dir = envdir;
				gbhw->ch[chn].env_ctr = gbhw->ch[chn].env_tc = envspd;

				gbhw->ch[chn].master = (val & 0xf8) != 0;
				if (!gbhw->ch[chn].master) {
					gbhw->ch[chn].running = 0;
				}
			}
			break;
		case 0xff13:
		case 0xff14:
		case 0xff18:
		case 0xff19:
		case 0xff1d:
		case 0xff1e:
			{
				long div = gbhw->ioregs[0x13 + 5*chn];
				long old_len_enable = gbhw->ch[chn].len_enable;

				div |= ((long)gbhw->ioregs[0x14 + 5*chn] & 7) << 8;
				gbhw->ch[chn].div_tc = 2048 - div;

				if (addr == 0xff13 ||
				    addr == 0xff18 ||
				    addr == 0xff1d) break;

				gbhw->ch[chn].len_enable = (gbhw->ioregs[0x14 + 5*chn] & 0x40) > 0;
				if ((val & 0x80) == 0x80) {
					gbhw->ch[chn].env_volume = gbhw->ch[chn].volume;
					if (!gbhw->ch[chn].len_gate) {
						gbhw->ch[chn].len_gate = 1;
						if (old_len_enable == 1 &&
						    gbhw->ch[chn].len_enable == 1 &&
						    (gbhw->sequence_ctr & 1) == 1) {
							// Trigger that un-freezes enabled length should clock it
							sequencer_update_len(gbhw, chn);
						}
					}
					if (gbhw->ch[chn].master) {
						gbhw->ch[chn].running = 1;
					}
					if (addr == 0xff1e) {
						gbhw->ch3pos = 0;
					}
					if (addr == 0xff14) {
						gbhw->ch[0].div_tc_shadow = gbhw->ch[0].div_tc;
						sweep_check_overflow(gbhw);
					}
				}
				if (old_len_enable == 0 &&
				    gbhw->ch[chn].len_enable == 1 &&
				    (gbhw->sequence_ctr & 1) == 1) {
					// Enabling in first half of length period should clock length
					sequencer_update_len(gbhw, chn);
				}
			}

//			printf(" ch%ld: vol=%02d envd=%ld envspd=%ld duty_ctr=%ld len=%03d len_en=%ld key=%04d gate=%ld%ld\n", chn, gbhw->ch[chn].volume, gbhw->ch[chn].env_dir, gbhw->ch[chn].env_tc, gbhw->ch[chn].duty_ctr, gbhw->ch[chn].len, gbhw->ch[chn].len_enable, gbhw->ch[chn].div_tc, gbhw->ch[chn].leftgate, gbhw->ch[chn].rightgate);
			break;
		case 0xff15:
			break;
		case 0xff1a:
			gbhw->ch[2].master = (gbhw->ioregs[0x1a] & 0x80) > 0;
			if (!gbhw->ch[2].master) {
				gbhw->ch[2].running = 0;
			}
			break;
		case 0xff1b:
			gbhw->ch[2].len = val;
			gbhw->ch[2].len_gate = 1;
			break;
		case 0xff1c:
			{
				long vol = (gbhw->ioregs[0x1c] >> 5) & 3;
				gbhw->ch[2].env_volume = gbhw->ch[2].volume = vol;
				break;
			}
		case 0xff1f:
			break;
		case 0xff22:
		case 0xff23:
			{
				long reg = gbhw->ioregs[0x22];
				long shift = reg >> 4;
				long rate = reg & 7;
				long old_len_enable = gbhw->ch[chn].len_enable;
				gbhw->ch[3].div_ctr = 0;
				gbhw->ch[3].div_tc = 16 << shift;
				gblfsr_set_narrow(&gbhw->lfsr, (reg & 8) > 0);
				if (rate) gbhw->ch[3].div_tc *= rate;
				else gbhw->ch[3].div_tc /= 2;
				gbhw->ch[chn].len_enable = (gbhw->ioregs[0x23] & 0x40) > 0;
				if (addr == 0xff22) break;

				if (val & 0x80) {  /* trigger */
					gblfsr_trigger(&gbhw->lfsr);
					gbhw->ch[chn].env_volume = gbhw->ch[chn].volume;
					if (!gbhw->ch[chn].len_gate) {
						gbhw->ch[chn].len_gate = 1;
						if (old_len_enable == 1 &&
						    gbhw->ch[chn].len_enable == 1 &&
						    (gbhw->sequence_ctr & 1) == 1) {
							// Trigger that un-freezes enabled length should clock it
							sequencer_update_len(gbhw, chn);
						}
					}
					if (gbhw->ch[3].master) {
						gbhw->ch[3].running = 1;
					}
				}
				if (old_len_enable == 0 &&
				    gbhw->ch[chn].len_enable == 1 &&
				    (gbhw->sequence_ctr & 1) == 1) {
					// Enabling in first half of length period should clock length
					sequencer_update_len(gbhw, chn);
				}
//				printf(" ch4: vol=%02d envd=%ld envspd=%ld duty_ctr=%ld len=%03d len_en=%ld key=%04d gate=%ld%ld\n", gbhw->ch[3].volume, gbhw->ch[3].env_dir, gbhw->ch[3].env_ctr, gbhw->ch[3].duty_ctr, gbhw->ch[3].len, gbhw->ch[3].len_enable, gbhw->ch[3].div_tc, gbhw->ch[3].leftgate, gbhw->ch[3].rightgate);
			}
			break;
		case 0xff25:
			gbhw->ch[0].leftgate = (val & 0x10) > 0;
			gbhw->ch[0].rightgate = (val & 0x01) > 0;
			gbhw->ch[1].leftgate = (val & 0x20) > 0;
			gbhw->ch[1].rightgate = (val & 0x02) > 0;
			gbhw->ch[2].leftgate = (val & 0x40) > 0;
			gbhw->ch[2].rightgate = (val & 0x04) > 0;
			gbhw->ch[3].leftgate = (val & 0x80) > 0;
			gbhw->ch[3].rightgate = (val & 0x08) > 0;
			gbhw->update_level = 1;
			break;
		case 0xff26:
			if (val & 0x80) {
				gbhw->ioregs[0x26] = 0x80;
				gbhw->apu_on = 1;
			} else {
				apu_reset(gbhw);
				gbhw->apu_on = 0;
			}
			break;
		case 0xff70:
			WARN_ONCE("iowrite to SVBK (CGB mode) ignored.\n");
			break;
		case 0xff00:
		case 0xff24:
		case 0xff27:
		case 0xff28:
		case 0xff29:
		case 0xff2a:
		case 0xff2b:
		case 0xff2c:
		case 0xff2d:
		case 0xff2e:
		case 0xff2f:
		case 0xff30:
		case 0xff31:
		case 0xff32:
		case 0xff33:
		case 0xff34:
		case 0xff35:
		case 0xff36:
		case 0xff37:
		case 0xff38:
		case 0xff39:
		case 0xff3a:
		case 0xff3b:
		case 0xff3c:
		case 0xff3d:
		case 0xff3e:
		case 0xff3f:
		case 0xff50: /* bootrom lockout reg */
			if (val == 0x01) {
				gbhw->rom_lockout = 1;
			}
			break;
		case 0xffff:
			break;
		default:
			WARN_ONCE("iowrite to 0x%04x unimplemented (val=%02x).\n", addr, val);
			break;
	}
}

static void intram_put(void *priv, uint32_t addr, uint8_t val)
{
	struct gbhw *gbhw = priv;
	gbhw->intram[addr & GBHW_INTRAM_MASK] = val;
}

static void sequencer_step(struct gbhw *gbhw)
{
	long i;
	long clock_len = 0;
	long clock_env = 0;
	long clock_sweep = 0;

	switch (gbhw->sequence_ctr & 7) {
	case 0: clock_len = 1; break;
	case 1: break;
	case 2: clock_len = 1; clock_sweep = 1; break;
	case 3: break;
	case 4: clock_len = 1; break;
	case 5: break;
	case 6: clock_len = 1; clock_sweep = 1; break;
	case 7: clock_env = 1; break;
	}

	gbhw->sequence_ctr++;

	if (clock_sweep && gbhw->ch[0].sweep_tc) {
		gbhw->ch[0].sweep_ctr--;
		if (gbhw->ch[0].sweep_ctr < 0) {
			long val = (2048 - gbhw->ch[0].div_tc_shadow) >> gbhw->ch[0].sweep_shift;

			gbhw->ch[0].sweep_ctr = gbhw->ch[0].sweep_tc;
			if (sweep_check_overflow(gbhw)) {
				if (gbhw->ch[0].sweep_dir) {
					gbhw->ch[0].div_tc_shadow += val;
				} else {
					gbhw->ch[0].div_tc_shadow -= val;
				}
				gbhw->ch[0].div_tc = gbhw->ch[0].div_tc_shadow;
			}
		}
	}
	for (i=0; clock_len && i<4; i++) {
		sequencer_update_len(gbhw, i);
	}
	for (i=0; clock_env && i<4; i++) {
		if (gbhw->ch[i].env_tc) {
			gbhw->ch[i].env_ctr--;
			if (gbhw->ch[i].env_ctr <=0 ) {
				gbhw->ch[i].env_ctr = gbhw->ch[i].env_tc;
				if (gbhw->ch[i].running) {
					if (!gbhw->ch[i].env_dir) {
						if (gbhw->ch[i].env_volume > 0)
							gbhw->ch[i].env_volume--;
					} else {
						if (gbhw->ch[i].env_volume < 15)
						gbhw->ch[i].env_volume++;
					}
				}
			}
		}
	}
	if (gbhw->master_fade) {
		gbhw->master_volume += gbhw->master_fade;
		if ((gbhw->master_fade > 0 &&
		     gbhw->master_volume >= gbhw->master_dstvol) ||
		    (gbhw->master_fade < 0 &&
		     gbhw->master_volume <= gbhw->master_dstvol)) {
			gbhw->master_fade = 0;
			gbhw->master_volume = gbhw->master_dstvol;
		}
	}
}

void gbhw_master_fade(struct gbhw* const gbhw, long speed, long dstvol)
{
	if (dstvol < MASTER_VOL_MIN) dstvol = MASTER_VOL_MIN;
	if (dstvol > MASTER_VOL_MAX) dstvol = MASTER_VOL_MAX;
	gbhw->master_dstvol = dstvol;
	if (dstvol > gbhw->master_volume)
		gbhw->master_fade = speed;
	else gbhw->master_fade = -speed;
}

#define GET_NIBBLE(p, n) ({ \
	long index = ((n) >> 1) & 0xf; \
	long shift = (~(n) & 1) << 2; \
	(((p)[index] >> shift) & 0xf); })

static void gb_flush_buffer(struct gbhw *gbhw)
{
	long i;
	long overlap;
	long l_smpl, r_smpl;
	long l_cap, r_cap;

	assert(gbhw->soundbuf != NULL);
	assert(gbhw->impbuf != NULL);

	/* integrate buffer */
	l_smpl = gbhw->soundbuf->l_lvl;
	r_smpl = gbhw->soundbuf->r_lvl;
	l_cap = gbhw->soundbuf->l_cap;
	r_cap = gbhw->soundbuf->r_cap;
	for (i=0; i<gbhw->soundbuf->samples; i++) {
		long l_out, r_out;
		l_smpl = l_smpl + gbhw->impbuf->data32[i*2  ];
		r_smpl = r_smpl + gbhw->impbuf->data32[i*2+1];
		if (gbhw->filter_enabled && gbhw->cap_factor <= 0x10000) {
			/*
			 * RC High-pass & DC decoupling filter. Gameboy
			 * Classic uses 1uF and 510 Ohms in series,
			 * followed by 10K Ohms pot to ground between
			 * CPU output and amplifier input, which gives a
			 * cutoff frequency of 15.14Hz.
			 */
			l_out = (l_smpl - l_cap) >> 16;
			r_out = (r_smpl - r_cap) >> 16;
			/* cap factor is 0x10000 for a factor of 1.0 */
			l_cap = l_smpl - l_out * gbhw->cap_factor;
			r_cap = r_smpl - r_out * gbhw->cap_factor;
		} else {
			l_out = l_smpl >> 16;
			r_out = r_smpl >> 16;
		}
		gbhw->soundbuf->data[i*2  ] = l_out * gbhw->master_volume / MASTER_VOL_MAX;
		gbhw->soundbuf->data[i*2+1] = r_out * gbhw->master_volume / MASTER_VOL_MAX;
		if (l_out > gbhw->lmaxval) gbhw->lmaxval = l_out;
		if (l_out < gbhw->lminval) gbhw->lminval = l_out;
		if (r_out > gbhw->rmaxval) gbhw->rmaxval = r_out;
		if (r_out < gbhw->rminval) gbhw->rminval = r_out;
	}
	gbhw->soundbuf->pos = gbhw->soundbuf->samples;
	gbhw->soundbuf->l_lvl = l_smpl;
	gbhw->soundbuf->r_lvl = r_smpl;
	gbhw->soundbuf->l_cap = l_cap;
	gbhw->soundbuf->r_cap = r_cap;
    
    //YOYOFR
    /* integrate buffer */
    for (int ii=0;ii<4;ii++) {
        l_smpl = gbhw->soundbuf->lvl_ch[ii];
        l_cap = gbhw->soundbuf->cap_ch[ii];
        for (i=0; i<gbhw->soundbuf->samples; i++) {
            long l_out;
            l_smpl = l_smpl + m_voice_buff_accumul_temp[ii][i];
            if (gbhw->filter_enabled && gbhw->cap_factor <= 0x10000) {
                /*
                 * RC High-pass & DC decoupling filter. Gameboy
                 * Classic uses 1uF and 510 Ohms in series,
                 * followed by 10K Ohms pot to ground between
                 * CPU output and amplifier input, which gives a
                 * cutoff frequency of 15.14Hz.
                 */
                l_out = (l_smpl - l_cap) >> 16;
                /* cap factor is 0x10000 for a factor of 1.0 */
                l_cap = l_smpl - l_out * gbhw->cap_factor;
            } else {
                l_out = l_smpl >> 16;
            }
            //final rendering in voices idx 0 to 3
            m_voice_buff[ii][i]=LIMIT8(((l_out * gbhw->master_volume / MASTER_VOL_MAX)>>7));
        }
        gbhw->soundbuf->lvl_ch[ii] = l_smpl;
        gbhw->soundbuf->cap_ch[ii] = l_cap;
    }
    //YOYOFR

	if (gbhw->callback != NULL) gbhw->callback(gbhw->callbackpriv);

	overlap = gbhw->impbuf->samples - gbhw->soundbuf->samples;
    
	memmove(gbhw->impbuf->data32, gbhw->impbuf->data32+(2*gbhw->soundbuf->samples), 8*overlap);
    
    //YOYOFR
    for (int ii=0;ii<4;ii++) {
        memmove(m_voice_buff_accumul_temp[ii], m_voice_buff_accumul_temp[ii]+(gbhw->soundbuf->samples), 4*overlap);
        memset(m_voice_buff_accumul_temp[ii] + overlap, 0, gbhw->impbuf->bytes/2 - 4*overlap);
    }
    //YOYOFR
    
    
	memset(gbhw->impbuf->data32 + 2*overlap, 0, gbhw->impbuf->bytes - 8*overlap);
	assert(gbhw->impbuf->bytes == gbhw->impbuf->samples*8);
	assert(gbhw->soundbuf->bytes == gbhw->soundbuf->samples*4);
	memset(gbhw->soundbuf->data, 0, gbhw->soundbuf->bytes);
	gbhw->soundbuf->pos = 0;

	gbhw->impbuf->cycles -= (gbhw->sound_div_tc * gbhw->soundbuf->samples) / SOUND_DIV_MULT;
}

static void gb_change_level(struct gbhw *gbhw, long l_ofs, long r_ofs)
{
	long pos;
	long imp_idx;
	long imp_l = -IMPULSE_WIDTH/2;
	long imp_r = IMPULSE_WIDTH/2;
	long i;
	const int32_t *ptr = base_impulse;

	assert(gbhw->impbuf != NULL);
	pos = (long)(gbhw->impbuf->cycles * SOUND_DIV_MULT / gbhw->sound_div_tc);
	imp_idx = (long)((gbhw->impbuf->cycles << IMPULSE_N_SHIFT)*SOUND_DIV_MULT / gbhw->sound_div_tc) & IMPULSE_N_MASK;
	assert(pos + imp_r < gbhw->impbuf->samples);
	assert(pos + imp_l >= 0);

	ptr += imp_idx * IMPULSE_WIDTH;

	for (i=imp_l; i<imp_r; i++) {
		long bufi = pos + i;
		long impi = i + IMPULSE_WIDTH/2;
		gbhw->impbuf->data32[bufi*2  ] += ptr[impi] * l_ofs;
		gbhw->impbuf->data32[bufi*2+1] += ptr[impi] * r_ofs;
	}

	gbhw->impbuf->l_lvl += l_ofs*256;
	gbhw->impbuf->r_lvl += r_ofs*256;
}

static void gb_change_level_mdz(struct gbhw *gbhw, long l_ofs,signed int *voice_buffer)
{
    long pos;
    long imp_idx;
    long imp_l = -IMPULSE_WIDTH/2;
    long imp_r = IMPULSE_WIDTH/2;
    long i;
    const int32_t *ptr = base_impulse;

    assert(gbhw->impbuf != NULL);
    pos = (long)(gbhw->impbuf->cycles * SOUND_DIV_MULT / gbhw->sound_div_tc);
    imp_idx = (long)((gbhw->impbuf->cycles << IMPULSE_N_SHIFT)*SOUND_DIV_MULT / gbhw->sound_div_tc) & IMPULSE_N_MASK;
    assert(pos + imp_r < gbhw->impbuf->samples);
    assert(pos + imp_l >= 0);

    ptr += imp_idx * IMPULSE_WIDTH;

    for (i=imp_l; i<imp_r; i++) {
        long bufi = pos + i;
        long impi = i + IMPULSE_WIDTH/2;
        //gbhw->impbuf->data32[bufi*2  ] += ptr[impi] * l_ofs;
        //gbhw->impbuf->data32[bufi*2+1] += ptr[impi] * r_ofs;
        voice_buffer[bufi&(SOUND_BUFFER_SIZE_SAMPLE*2-1)]+=ptr[impi] * l_ofs;
    }

    //gbhw->impbuf->l_lvl += l_ofs*256;
    //gbhw->impbuf->r_lvl += r_ofs*256;
}


static void gb_sound(struct gbhw *gbhw, cycles_t cycles)
{
	long i, j;
	long l_lvl = 0, r_lvl = 0;
    
    //TODO:  MODIZER changes start / YOYOFR
    int64_t smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/GBHW_CLOCK;
    //TODO:  MODIZER changes end / YOYOFR
    
	assert(gbhw->impbuf != NULL);

	for (j=0; j<cycles; j++) {
		gbhw->main_div++;
		gbhw->impbuf->cycles++;
		if (gbhw->impbuf->cycles*SOUND_DIV_MULT >= gbhw->sound_div_tc*(gbhw->impbuf->samples - IMPULSE_WIDTH/2))
			gb_flush_buffer(gbhw);

		if (gbhw->ch[2].running) {
			gbhw->ch[2].div_ctr--;
			if (gbhw->ch[2].div_ctr <= 0) {
				long val = gbhw->ch3_next_nibble;
				long pos = gbhw->ch3pos++;
				gbhw->ch3_next_nibble = GET_NIBBLE(&gbhw->ioregs[0x30], pos) * 2;
				gbhw->ch[2].div_ctr = gbhw->ch[2].div_tc*2;
				if (gbhw->ch[2].env_volume) {
					val = val >> (gbhw->ch[2].env_volume-1);
				} else val = 0;
				gbhw->ch[2].lvl = val - 15;
				gbhw->update_level = 1;
			}
		}

		if (gbhw->ch[3].running) {
			gbhw->ch[3].div_ctr--;
			if (gbhw->ch[3].div_ctr <= 0) {
				long val;
				gbhw->ch[3].div_ctr = gbhw->ch[3].div_tc;
				val = gbhw->ch[3].env_volume * 2 * gblfsr_next_value(&gbhw->lfsr);
				gbhw->ch[3].lvl = val - 15;
				gbhw->update_level = 1;
			}
		}

		if (gbhw->main_div > main_div_tc) {
			gbhw->main_div -= main_div_tc;

			for (i=0; i<2; i++) if (gbhw->ch[i].running) {
				long bit = (gbhw->ch[i].duty_val >> gbhw->ch[i].duty_ctr) & 1;
				long val = bit * 2 * gbhw->ch[i].env_volume;
				gbhw->ch[i].lvl = val - 15;
				gbhw->ch[i].div_ctr--;
				if (gbhw->ch[i].div_ctr <= 0) {
					gbhw->ch[i].div_ctr = gbhw->ch[i].div_tc;
					gbhw->ch[i].duty_ctr++;
					gbhw->ch[i].duty_ctr &= 7;
				}
			}

			gbhw->sweep_div += 1;
			if (gbhw->sweep_div >= sweep_div_tc) {
				gbhw->sweep_div = 0;
				sequencer_step(gbhw);
			}
			gbhw->update_level = 1;
		}

		if (gbhw->update_level) {
			gbhw->update_level = 0;
			l_lvl = 0;
			r_lvl = 0;
            
			for (i=0; i<4; i++) {
				if (gbhw->ch[i].mute)
					continue;
                if (gbhw->ch[i].leftgate) {
                    l_lvl += gbhw->ch[i].lvl;
                }
                if (gbhw->ch[i].rightgate) {
                    r_lvl += gbhw->ch[i].lvl;
                }
                //TODO:  MODIZER changes start / YOYOFR
                if (seek_needed==-1) {
                    long val=0;
                    if (gbhw->ch[i].leftgate || gbhw->ch[i].rightgate ) val=(gbhw->ch[i].lvl);
                    if (gbhw->ch[i].last_lvl!=val) {
                        gb_change_level_mdz(gbhw,val-gbhw->ch[i].last_lvl,m_voice_buff_accumul_temp[i]);
                        gbhw->ch[i].last_lvl=val;
                    }
                }
                //TODO:  MODIZER changes end / YOYOFR
			}

			if (l_lvl != gbhw->last_l_value || r_lvl != gbhw->last_r_value) {
				gb_change_level(gbhw, l_lvl - gbhw->last_l_value, r_lvl - gbhw->last_r_value);
				gbhw->last_l_value = l_lvl;
				gbhw->last_r_value = r_lvl;
			}
            

		}
        
        //TODO:  MODIZER changes start / YOYOFR
//        if (seek_needed==-1)
//        for (int i=0;i<4;i++) {
//            int64_t ofs_start=m_voice_current_ptr[i];
//            int64_t ofs_end=(m_voice_current_ptr[i]+smplIncr);
//            
//            long val=0;
//            if (! (gbhw->ch[i].mute) ) {
//                if (gbhw->ch[i].leftgate || gbhw->ch[i].rightgate ) val=(gbhw->ch[i].lvl)<<2;
//            }
//            for (;;) {
//                m_voice_buff[i][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)]=LIMIT8(val);
//                ofs_start+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
//                if (ofs_start>=ofs_end) break;
//            }
//            while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*2*4) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*2*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
//            m_voice_current_ptr[i]=ofs_end;
//        }
        //TODO:  MODIZER changes end / YOYOFR
	}
}

void gbhw_set_callback(struct gbhw *gbhw, gbhw_callback_fn fn, void *priv)
{
	gbhw->callback = fn;
	gbhw->callbackpriv = priv;
}

void gbhw_set_io_callback(struct gbhw *gbhw, gbhw_iocallback_fn fn, void *priv)
{
	gbhw->iocallback = fn;
	gbhw->iocallback_priv = priv;
}

void gbhw_set_step_callback(struct gbhw *gbhw, gbhw_stepcallback_fn fn, void *priv)
{
	gbhw->stepcallback = fn;
	gbhw->stepcallback_priv = priv;
}

static void gbhw_impbuf_reset(struct gbhw *gbhw)
{
	assert(gbhw->sound_div_tc != 0);
	gbhw->impbuf->cycles = (long)(gbhw->sound_div_tc * IMPULSE_WIDTH/2 / SOUND_DIV_MULT);
	gbhw->impbuf->l_lvl = 0;
	gbhw->impbuf->r_lvl = 0;
	memset(gbhw->impbuf->data32, 0, gbhw->impbuf->bytes);
}

void gbhw_set_buffer(struct gbhw* const gbhw, struct gbhw_buffer *buffer)
{
	long impbuf_bytes;

	gbhw->soundbuf = buffer;
	gbhw->soundbuf->samples = gbhw->soundbuf->bytes / 4;

	if (gbhw->impbuf) free(gbhw->impbuf);
	impbuf_bytes = (gbhw->soundbuf->samples + IMPULSE_WIDTH + 1) * 8;
	gbhw->impbuf = malloc(sizeof(*gbhw->impbuf) + impbuf_bytes);
	if (gbhw->impbuf == NULL) {
		fprintf(stderr, "%s", _("Memory allocation failed!\n"));
		return;
	}
	memset(gbhw->impbuf, 0, sizeof(*gbhw->impbuf));
	gbhw->impbuf->data32 = (void*)(gbhw->impbuf+1);
	gbhw->impbuf->bytes = impbuf_bytes;
	gbhw->impbuf->samples = impbuf_bytes / 8;
	gbhw_impbuf_reset(gbhw);
}

static void gbhw_update_filter(struct gbhw *gbhw)
{
	double cap_constant = pow(gbhw->filter_constant, (double)GBHW_CLOCK / gbhw->sample_rate);
	gbhw->cap_factor = round(65536.0 * cap_constant);
}

long gbhw_set_filter(struct gbhw* const gbhw, enum gbs_filter_type type)
{
	switch (type) {
	case FILTER_OFF:
		gbhw->filter_enabled = 0;
		gbhw->filter_constant = FILTER_CONST_OFF;
		break;

	case FILTER_DMG:
		gbhw->filter_enabled = 1;
		gbhw->filter_constant = FILTER_CONST_DMG;
		break;

	case FILTER_CGB:
		gbhw->filter_enabled = 1;
		gbhw->filter_constant = FILTER_CONST_CGB;
		break;

	default:
		return 0; // invalid
	}

	gbhw_update_filter(gbhw);

	return 1;
}

void gbhw_set_rate(struct gbhw* const gbhw, long rate)
{
	gbhw->sample_rate = rate;
	gbhw->sound_div_tc = GBHW_CLOCK*SOUND_DIV_MULT/rate;
	gbhw_update_filter(gbhw);
}

void gbhw_calc_minmax(struct gbhw* const gbhw, int16_t *lmin, int16_t *lmax, int16_t *rmin, int16_t *rmax)
{
	if (gbhw->lminval == INT_MAX) return;
	*lmin = gbhw->lminval;
	*lmax = gbhw->lmaxval;
	*rmin = gbhw->rminval;
	*rmax = gbhw->rmaxval;
	gbhw->lminval = gbhw->rminval = INT_MAX;
	gbhw->lmaxval = gbhw->rmaxval = INT_MIN;
}

float gbhw_calc_timer_hz(uint8_t tac, uint8_t tma)
{
	long timertc = timertc_from_tac(tac);
	return GBHW_CLOCK / timertc / (float)(256 - tma);
}

/*
 * Initialize Gameboy hardware emulation.
 * The size should be a multiple of 0x4000,
 * so we don't need range checking in rom_get and
 * rombank_get.
 */
void gbhw_init(struct gbhw* const gbhw)
{
	long i;
	gbhw_iocallback_fn saved_callback = gbhw->iocallback;
	/* Disable IO callback to hide memory pokes done in gbhw_init. */
	gbhw->iocallback = NULL;

	gbhw->vblankctr = vblanktc;
	gbhw->timerctr = 0;
	gbhw->divoffset = 0;
	gbhw->main_div = 0;
	gbhw->sweep_div = 0;

	if (gbhw->impbuf)
		gbhw_impbuf_reset(gbhw);
	gbhw->master_volume = MASTER_VOL_MAX;
	gbhw->master_fade = 0;
	gbhw->apu_on = 1;
	if (gbhw->soundbuf) {
		gbhw->soundbuf->pos = 0;
		gbhw->soundbuf->l_lvl = 0;
		gbhw->soundbuf->r_lvl = 0;
		gbhw->soundbuf->l_cap = 0;
		gbhw->soundbuf->r_cap = 0;
	}
	gbhw->lminval = gbhw->rminval = INT_MAX;
	gbhw->lmaxval = gbhw->rmaxval = INT_MIN;
	apu_reset(gbhw);
	assert(sizeof(gbhw->intram) == GBHW_INTRAM_SIZE);
	memset(gbhw->intram, 0, sizeof(gbhw->intram));
	memset(gbhw->hiram, 0, sizeof(gbhw->hiram));
	memset(gbhw->ioregs, 0, sizeof(gbhw->ioregs));
	for (i=0x10; i<0x40; i++) {
		io_put(gbhw, 0xff00 + i, ioregs_initdata[i]);
	}

	gbhw->sum_cycles = 0;
	gbhw->halted_noirq_cycles = 0;
	gbhw->ch[0].duty_ctr = 0;
	gbhw->ch[1].duty_ctr = 0;
	gbhw->ch3pos = 0;
	gbhw->ch3_next_nibble = 0;
	gbhw->last_l_value = 0;
	gbhw->last_r_value = 0;
    
    //YOYOFR
    for (int ii=0;ii<4;ii++) gbhw->ch[ii].last_lvl=0;
    //YOYOFR

	gbcpu_init(&gbhw->gbcpu);
	gbcpu_add_mem(&gbhw->gbcpu, 0xc0, 0xfe, intram_put, intram_get, gbhw);
	gbcpu_add_mem(&gbhw->gbcpu, 0xff, 0xff, io_put, io_get, gbhw);

	gbhw->iocallback = saved_callback;  /* restore IO callback */
}

void gbhw_cleanup(struct gbhw* const gbhw)
{
	if (gbhw->impbuf) free(gbhw->impbuf);
}

void gbhw_enable_bootrom(struct gbhw* const gbhw, const uint8_t *rombuf)
{
	memcpy(gbhw->boot_rom, rombuf, sizeof(gbhw->boot_rom));
	gbhw->rom_lockout = 0;
	gbhw->boot_shadow_get = gbhw->gbcpu.getlookup[0];
	gbhw->boot_shadow_put = gbhw->gbcpu.putlookup[0];
	gbcpu_add_mem(&gbhw->gbcpu, 0x00, 0x00, bootrom_put, bootrom_get, gbhw);
}

/* internal for gbs.c, not exported from libgbs */
void gbhw_io_put(struct gbhw* const gbhw, uint16_t addr, uint8_t val) {
	if (addr != 0xffff && (addr < 0xff00 || addr > 0xff7f))
		return;
	io_put(gbhw, addr, val);
}

/* unmasked peek used by gbsplay.c to print regs */
uint8_t gbhw_io_peek(const struct gbhw* const gbhw, uint16_t addr)
{
	if (addr >= 0xff10 && addr <= 0xff3f) {
		return gbhw->ioregs[addr & GBHW_IOREGS_MASK];
	}
	return 0xff;
}


void gbhw_check_if(struct gbhw *gbhw)
{
	struct gbcpu *gbcpu = &gbhw->gbcpu;

	uint8_t mask = 0x01; /* lowest bit is highest priority irq */
	uint8_t vec = 0x40;
	if (!gbcpu->ime) {
		/* interrupts disabled */
		if (gbhw->ioregs[REG_IF] & gbhw->ioregs[REG_IE]) {
			/* but will still exit halt */
			gbcpu->halted = 0;
		}
		return;
	}
	while (mask <= 0x10) {
		if (gbhw->ioregs[REG_IF] & gbhw->ioregs[REG_IE] & mask) {
			gbhw->ioregs[REG_IF] &= ~mask;
			gbcpu->halted = 0;
			gbcpu_intr(gbcpu, vec);
			break;
		}
		vec += 0x08;
		mask <<= 1;
	}
}

static void blargg_debug(struct gbcpu *gbcpu)
{
	long i;

	/* Blargg GB debug output signature. */
	if (gbcpu_mem_get(gbcpu, 0xa001) != 0xde ||
	    gbcpu_mem_get(gbcpu, 0xa002) != 0xb0 ||
	    gbcpu_mem_get(gbcpu, 0xa003) != 0x61) {
		return;
	}

	fprintf(stderr, "\nBlargg debug output:\n");

	for (i = 0xa004; i < 0xb000; i++) {
		uint8_t c = gbcpu_mem_get(gbcpu, i);
		if (c == 0 || c >= 128) {
			return;
		}
		if (c < 32 && c != 10 && c != 13) {
			return;
		}
		fputc(c, stderr);
	}
}

/**
 * @param time_to_work  emulated time in milliseconds
 * @return  elapsed cpu cycles
 */
cycles_t gbhw_step(struct gbhw *gbhw, long time_to_work)
{
	struct gbcpu *gbcpu = &gbhw->gbcpu;
	cycles_t cycles_total = 0;

	time_to_work *= msec_cycles;
	
	while (cycles_total < time_to_work) {
		long maxcycles = time_to_work - cycles_total;
		cycles_t cycles = 0;

		if (gbhw->vblankctr > 0 && gbhw->vblankctr < maxcycles) maxcycles = gbhw->vblankctr;
		if (gbhw->timerctr > 0 && gbhw->timerctr < maxcycles) maxcycles = gbhw->timerctr;

		gbhw->io_written = 0;
		while (cycles < maxcycles && !gbhw->io_written) {
			long step;
			gbhw_check_if(gbhw);
			step = gbcpu_step(gbcpu);
			if (gbcpu->halted) {
				gbhw->halted_noirq_cycles += step;
				if (gbcpu->ime == 0 &&
				    (gbhw->ioregs[REG_IE] == 0 ||
				     gbhw->halted_noirq_cycles > GBHW_CLOCK/10)) {
					fprintf(stderr, "CPU locked up (halt with interrupts disabled).\n");
					blargg_debug(gbcpu);
					return -1;
				}
			} else {
				gbhw->halted_noirq_cycles = 0;
			}
			if (step < 0) return step;
			cycles += step;
			gbhw->sum_cycles += step;
			gbhw->vblankctr -= step;
			if (gbhw->vblankctr <= 0) {
				gbhw->vblankctr += vblanktc;
				gbhw->ioregs[REG_IF] |= 0x01;
				DPRINTF("vblank_interrupt\n");
			}
			gb_sound(gbhw, step);
			if (gbhw->stepcallback)
			   gbhw->stepcallback(gbhw->sum_cycles, gbhw->ch, gbhw->stepcallback_priv);
		}

		if (gbhw->ioregs[REG_TAC] & 4) {
			if (gbhw->timerctr > 0) gbhw->timerctr -= cycles;
			while (gbhw->timerctr <= 0) {
				gbhw->timerctr += gbhw->timertc;
				gbhw->ioregs[REG_TIMA]++;
				//DPRINTF("TIMA=%02x\n", ioregs[REG_TIMA]);
				if (gbhw->ioregs[REG_TIMA] == 0) {
					gbhw->ioregs[REG_TIMA] = gbhw->ioregs[REG_TMA];
					gbhw->ioregs[REG_IF] |= 0x04;
					DPRINTF("timer_interrupt\n");
				}
			}
		}
		cycles_total += cycles;
	}

	return cycles_total;
}
