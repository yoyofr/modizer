/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* This loader recognizes the following variants of the Protracker
 * module format:
 *
 * - Protracker M.K. and M!K!
 * - Protracker songs
 * - Noisetracker N.T. and M&K! (not tested)
 * - Fast Tracker 6CHN and 8CHN
 * - Fasttracker II/Take Tracker ?CHN and ??CH
 * - Mod's Grave M.K. w/ 8 channels (WOW)
 * - Atari Octalyser CD61 and CD81
 * - Digital Tracker FA04, FA06 and FA08
 * - TakeTracker TDZ4
 * - (unknown) NSMS
 */

#include <ctype.h>
#include <limits.h>
#include "loader.h"
#include "mod.h"

struct mod_magic {
	const char *magic;
	int flag;
	int id;
	int ch;
};

#define TRACKER_PROTRACKER	0
#define TRACKER_NOISETRACKER	1
#define TRACKER_SOUNDTRACKER	2
#define TRACKER_FASTTRACKER	3
#define TRACKER_FASTTRACKER2	4
#define TRACKER_OCTALYSER	5
#define TRACKER_TAKETRACKER	6
#define TRACKER_DIGITALTRACKER	7
#define TRACKER_FLEXTRAX	8
#define TRACKER_MODSGRAVE	9
#define TRACKER_SCREAMTRACKER3	10
#define TRACKER_OPENMPT		11
#define TRACKER_UNKNOWN_CONV	95
#define TRACKER_CONVERTEDST	96
#define TRACKER_CONVERTED	97
#define TRACKER_CLONE		98
#define TRACKER_UNKNOWN		99

#define TRACKER_PROBABLY_NOISETRACKER 20

const struct mod_magic mod_magic[] = {
	{"M.K.", 0, TRACKER_PROTRACKER, 4},
	{"M!K!", 1, TRACKER_PROTRACKER, 4},
	{"M&K!", 1, TRACKER_NOISETRACKER, 4},
	{"N.T.", 1, TRACKER_NOISETRACKER, 4},
	{"6CHN", 0, TRACKER_FASTTRACKER, 6},
	{"8CHN", 0, TRACKER_FASTTRACKER, 8},
	{"CD61", 1, TRACKER_OCTALYSER, 6},	/* Atari STe/Falcon */
	{"CD81", 1, TRACKER_OCTALYSER, 8},	/* Atari STe/Falcon */
	{"TDZ4", 1, TRACKER_TAKETRACKER, 4},	/* see XModule SaveTracker.c */
	{"FA04", 1, TRACKER_DIGITALTRACKER, 4},	/* Atari Falcon */
	{"FA06", 1, TRACKER_DIGITALTRACKER, 6},	/* Atari Falcon */
	{"FA08", 1, TRACKER_DIGITALTRACKER, 8},	/* Atari Falcon */
	{"NSMS", 1, TRACKER_UNKNOWN, 4},	/* in Kingdom.mod */
	{"", 0}
};

static int mod_test(HIO_HANDLE *, char *, const int);
static int mod_load(struct module_data *, HIO_HANDLE *, const int);

const struct format_loader libxmp_loader_mod = {
	"Amiga Protracker/Compatible",
	mod_test,
	mod_load
};

static int validate_pattern(uint8 *buf)
{
	int i, j;

	for (i = 0; i < 64; i++) {
		for (j = 0; j < 4; j++) {
			uint8 *d = buf + (i * 4 + j) * 4;
			if ((d[0] >> 4) > 1) {
				return -1;
			}
		}
	}

	return 0;
}

static int mod_test(HIO_HANDLE * f, char *t, const int start)
{
	int i;
	char buf[4];
	uint8 pat_buf[1024];
	int smp_size, num_pat;
	long size;

	hio_seek(f, start + 1080, SEEK_SET);
	if (hio_read(buf, 1, 4, f) < 4) {
		return -1;
	}

	if (!strncmp(buf + 2, "CH", 2) && isdigit((int)buf[0])
	    && isdigit((int)buf[1])) {
		i = (buf[0] - '0') * 10 + buf[1] - '0';
		if (i > 0 && i <= 32) {
			goto found;
		}
	}

	if (!strncmp(buf + 1, "CHN", 3) && isdigit((int)*buf)) {
		if (*buf - '0') {
			goto found;
		}
	}

	for (i = 0; mod_magic[i].ch; i++) {
		if (!memcmp(buf, mod_magic[i].magic, 4))
			break;
	}
	if (mod_magic[i].ch == 0) {
		return -1;
	}

	/*
	 * Sanity check to prevent loading NoiseRunner and other module
	 * formats with valid magic at offset 1080
	 */

	hio_seek(f, start + 20, SEEK_SET);
	for (i = 0; i < 31; i++) {
		uint8 x;

		hio_seek(f, 22, SEEK_CUR);	/* Instrument name */

		/* OpenMPT can create mods with large samples */
		hio_read16b(f);	/* sample size */

		/* Chris Spiegel tells me that sandman.mod has 0x20 in finetune */
		x = hio_read8(f);
		if (x & 0xf0 && x != 0x20)	/* test finetune */
			return -1;
		if (hio_read8(f) > 0x40)	/* test volume */
			return -1;
		hio_read16b(f);	/* loop start */
		hio_read16b(f);	/* loop size */
	}

	/* Test for UNIC tracker modules
	 *
	 * From Gryzor's Pro-Wizard PW_FORMATS-Engl.guide:
	 * ``The UNIC format is very similar to Protracker... At least in the
	 * heading... same length : 1084 bytes. Even the "M.K." is present,
	 * sometimes !! Maybe to disturb the rippers.. hehe but Pro-Wizard
	 * doesn't test this only!''
	 */

	/* get file size */
	size = hio_size(f);
	smp_size = 0;
	hio_seek(f, start + 20, SEEK_SET);

	/* get samples size */
	for (i = 0; i < 31; i++) {
		hio_seek(f, 22, SEEK_CUR);
		smp_size += 2 * hio_read16b(f);	/* Length in 16-bit words */
		hio_seek(f, 6, SEEK_CUR);
	}

	/* get number of patterns */
	num_pat = 0;
	hio_seek(f, start + 952, SEEK_SET);
	for (i = 0; i < 128; i++) {
		uint8 x = hio_read8(f);
		if (x > 0x7f)
			break;
		if (x > num_pat)
			num_pat = x;
	}
	num_pat++;

	/* see if module size matches UNIC */
	if (start + 1084 + num_pat * 0x300 + smp_size == size)
		return -1;

	/* validate pattern data in an attempt to catch UNICs with MOD size */
	for (i = 0; i < num_pat; i++) {
		hio_seek(f, start + 1084 + 1024 * i, SEEK_SET);
		hio_read(pat_buf, 1024, 1, f);
		if (validate_pattern(pat_buf) < 0)
			return -1;
	}

found:
	hio_seek(f, start + 0, SEEK_SET);
	libxmp_read_title(f, t, 20);

	return 0;
}


static int is_st_ins(char *s)
{
	if (s[0] != 's' && s[0] != 'S')
		return 0;
	if (s[1] != 't' && s[1] != 'T')
		return 0;
	if (s[2] != '-' || s[5] != ':')
		return 0;
	if (!isdigit((int)s[3]) || !isdigit((int)s[4]))
		return 0;

	return 1;
}

static int get_tracker_id(struct module_data *m, struct mod_header *mh, int id)
{
	struct xmp_module *mod = &m->mod;
	int has_loop_0 = 0;
	int has_vol_in_empty_ins = 0;
	int i;

	/* Check if has instruments with loop size 0 */
	for (i = 0; i < 31; i++) {
		if (mh->ins[i].loop_size == 0) {
			has_loop_0 = 1;
			break;
		}
	}

	/* Check if has instruments with size 0 and volume > 0 */
	for (i = 0; i < 31; i++) {
		if (mh->ins[i].size == 0 && mh->ins[i].volume > 0) {
			has_vol_in_empty_ins = 1;
			break;
		}
	}

	/*
	 * Test Protracker-like files
	 */
	if (mh->restart == mod->pat) {
		if (mod->chn == 4) {
			id = TRACKER_SOUNDTRACKER;
		} else {
			id = TRACKER_UNKNOWN;
		}
	} else if (mh->restart == 0x78) {
		if (mod->chn == 4) {
			/* Can't trust this for Noisetracker, MOD.Data City Remix
			 * has Protracker effects and Noisetracker restart byte
			 */
			id = TRACKER_PROBABLY_NOISETRACKER;
		} else {
			id = TRACKER_UNKNOWN;
		}
		return id;
	} else if (mh->restart < 0x7f) {
		if (mod->chn == 4 && !has_vol_in_empty_ins) {
			id = TRACKER_NOISETRACKER;
		} else {
			id = TRACKER_UNKNOWN; /* ? */
		}
		mod->rst = mh->restart;
	} else if (mh->restart == 0x7f) {
		if (mod->chn == 4) {
			if (has_loop_0) {
				id = TRACKER_CLONE;
			}
		} else {
			id = TRACKER_SCREAMTRACKER3;
		}
		return id;
	} else if (mh->restart > 0x7f) {
		id = TRACKER_UNKNOWN; /* ? */
		return id;
	}

	if (!has_loop_0) { /* All loops are size 2 or greater */

		for (i = 0; i < 31; i++) {
			if (mh->ins[i].size == 1 && mh->ins[i].volume == 0) {
				return TRACKER_CONVERTED;
			}
		}

		for (i = 0; i < 31; i++) {
			if (is_st_ins((char *)mh->ins[i].name))
				break;
		}
		if (i == 31) {	/* No st- instruments */
			for (i = 0; i < 31; i++) {
				if (mh->ins[i].size != 0
				    || mh->ins[i].loop_size != 1) {
					continue;
				}

				switch (mod->chn) {
				case 4:
					if (has_vol_in_empty_ins) {
						id = TRACKER_OPENMPT;
					} else {
						id = TRACKER_NOISETRACKER;
						/* or Octalyser */
					}
					break;
				case 6:
				case 8:
					id = TRACKER_OCTALYSER;
					break;
				default:
					id = TRACKER_UNKNOWN;
				}
				return id;
			}

			if (mod->chn == 4) {
				id = TRACKER_PROTRACKER;
			} else if (mod->chn == 6 || mod->chn == 8) {
				/* FastTracker 1.01? */
				id = TRACKER_FASTTRACKER;
			} else {
				id = TRACKER_UNKNOWN;
			}
		}
	} else { /* Has loops with size 0 */
		for (i = 15; i < 31; i++) {
			if (strlen((char *)mh->ins[i].name)
			    || mh->ins[i].size > 0)
				break;
		}
		if (i == 31 && is_st_ins((char *)mh->ins[14].name)) {
			return TRACKER_CONVERTEDST;
		}

		/* Assume that Fast Tracker modules won't have ST- instruments */
		for (i = 0; i < 31; i++) {
			if (is_st_ins((char *)mh->ins[i].name))
				break;
		}
		if (i < 31) {
			return TRACKER_UNKNOWN_CONV;
		}

		if (mod->chn == 4 || mod->chn == 6 || mod->chn == 8) {
			return TRACKER_FASTTRACKER;
		}

		id = TRACKER_UNKNOWN;	/* ??!? */
	}

	return id;
}

static int mod_load(struct module_data *m, HIO_HANDLE *f, const int start)
{
    struct xmp_module *mod = &m->mod;
    int i, j;
    int smp_size, ptsong = 0;
    struct xmp_event *event;
    struct mod_header mh;
    uint8 mod_event[4];
    char pathname[PATH_MAX] = "";
    const char *x, *tracker = "";
    int detected = 0;
    char magic[8], idbuffer[32];
    int ptkloop = 0;			/* Protracker loop */
    int tracker_id = TRACKER_PROTRACKER;
    int out_of_range = 0;

    LOAD_INIT();

    mod->ins = 31;
    mod->smp = mod->ins;
    mod->chn = 0;
    smp_size = 0;
    /*pat_size = 0;*/

    m->period_type = PERIOD_MODRNG;

    hio_read(&mh.name, 20, 1, f);
    for (i = 0; i < 31; i++) {
	hio_read(&mh.ins[i].name, 22, 1, f);	/* Instrument name */
	mh.ins[i].size = hio_read16b(f);	/* Length in 16-bit words */
	mh.ins[i].finetune = hio_read8(f);	/* Finetune (signed nibble) */
	mh.ins[i].volume = hio_read8(f);	/* Linear playback volume */
	mh.ins[i].loop_start = hio_read16b(f);	/* Loop start in 16-bit words */
	mh.ins[i].loop_size = hio_read16b(f);	/* Loop size in 16-bit words */

	smp_size += 2 * mh.ins[i].size;
    }
    mh.len = hio_read8(f);
    mh.restart = hio_read8(f);
    hio_read(&mh.order, 128, 1, f);
    memset(magic, 0, 8);
    hio_read(magic, 1, 4, f);
    if (hio_error(f)) {
        return -1;
    }

    for (i = 0; mod_magic[i].ch; i++) {
	if (!(strncmp (magic, mod_magic[i].magic, 4))) {
	    mod->chn = mod_magic[i].ch;
	    tracker_id = mod_magic[i].id;
	    detected = mod_magic[i].flag;
	    break;
	}
    }

    if (mod->chn == 0) {
	if (!strncmp(magic + 2, "CH", 2) &&
	    isdigit((int)magic[0]) && isdigit((int)magic[1])) {
	    mod->chn = (*magic - '0') * 10 + magic[1] - '0';
	} else if (!strncmp(magic + 1, "CHN", 3) && isdigit((int)*magic)) {
	    mod->chn = *magic - '0';
	} else {
	    return -1;
	}
	tracker_id = mod->chn & 1 ? TRACKER_TAKETRACKER : TRACKER_FASTTRACKER2;
	detected = 1;
    }

    strncpy(mod->name, (char *) mh.name, 20);

    mod->len = mh.len;
    /* mod->rst = mh.restart; */

    if (mod->rst >= mod->len)
	mod->rst = 0;
    memcpy(mod->xxo, mh.order, 128);

    for (i = 0; i < 128; i++) {
	/* This fixes dragnet.mod (garbage in the order list) */
	if (mod->xxo[i] > 0x7f)
		break;
	if (mod->xxo[i] > mod->pat)
	    mod->pat = mod->xxo[i];
    }
    mod->pat++;

    /*pat_size = 256 * mod->chn * mod->pat;*/

    if (libxmp_init_instrument(m) < 0)
	return -1;

    for (i = 0; i < mod->ins; i++) {
	struct xmp_instrument *xxi;
	struct xmp_subinstrument *sub;
	struct xmp_sample *xxs;

	if (libxmp_alloc_subinstrument(mod, i, 1) < 0)
	    return -1;

	if (mh.ins[i].size >= 0x8000) {
	    tracker_id = TRACKER_OPENMPT;
	    detected = 1;
	}

	xxi = &mod->xxi[i];
	sub = &xxi->sub[0];
	xxs = &mod->xxs[i];

	xxs->len = 2 * mh.ins[i].size;
	xxs->lps = 2 * mh.ins[i].loop_start;
	xxs->lpe = xxs->lps + 2 * mh.ins[i].loop_size;
	if (xxs->lpe > xxs->len) {
		xxs->lpe = xxs->len;
	}
	xxs->flg = (mh.ins[i].loop_size > 1 && xxs->lpe >= 4) ?
		XMP_SAMPLE_LOOP : 0;
	sub->fin = (int8)(mh.ins[i].finetune << 4);
	sub->vol = mh.ins[i].volume;
	sub->pan = 0x80;
	sub->sid = i;
	libxmp_instrument_name(mod, i, mh.ins[i].name, 22);

	if (xxs->len > 0) {
		xxi->nsm = 1;
	}
    }

    /*
     * Experimental tracker-detection routine
     */ 

    if (detected)
	goto skip_test;

    /* Test for Flextrax modules
     *
     * FlexTrax is a soundtracker for Atari Falcon030 compatible computers.
     * FlexTrax supports the standard MOD file format (up to eight channels)
     * for compatibility reasons but also features a new enhanced module
     * format FLX. The FLX format is an extended version of the standard
     * MOD file format with support for real-time sound effects like reverb
     * and delay.
     */

    if (0x43c + mod->pat * 4 * mod->chn * 0x40 + smp_size < m->size) {
	int pos = hio_tell(f);
        if (pos < 0) {
           return -1;
        }
	hio_seek(f, start + 0x43c + mod->pat * 4 * mod->chn * 0x40 + smp_size, SEEK_SET);
	hio_read(idbuffer, 1, 4, f);
	hio_seek(f, start + pos, SEEK_SET);

	if (!memcmp(idbuffer, "FLEX", 4)) {
	    tracker_id = TRACKER_FLEXTRAX;
	    goto skip_test;
	}
    }

    /* Test for Mod's Grave WOW modules
     *
     * Stefan Danes <sdanes@marvels.hacktic.nl> said:
     * This weird format is identical to '8CHN' but still uses the 'M.K.' ID.
     * You can only test for WOW by calculating the size of the module for 8 
     * channels and comparing this to the actual module length. If it's equal, 
     * the module is an 8 channel WOW.
     */

    if ((!strncmp(magic, "M.K.", 4) &&
		(0x43c + mod->pat * 32 * 0x40 + smp_size == m->size))) {
	mod->chn = 8;
	tracker_id = TRACKER_MODSGRAVE;
    }
    /* Test for Protracker song files */
    else if ((ptsong = (!strncmp((char *)magic, "M.K.", 4) &&
		(0x43c + mod->pat * 0x400 == m->size)))) {
	tracker_id = TRACKER_PROTRACKER;
	goto skip_test;
    }
    /* something else */
    else {
        tracker_id = get_tracker_id(m, &mh, tracker_id);
    }

skip_test:

    if (mod->chn >= XMP_MAX_CHANNELS) {
        return -1;
    }

    mod->trk = mod->chn * mod->pat;

    for (i = 0; i < mod->ins; i++) {
	D_(D_INFO "[%2X] %-22.22s %04x %04x %04x %c V%02x %+d %c",
		i, mod->xxi[i].name,
		mod->xxs[i].len, mod->xxs[i].lps, mod->xxs[i].lpe,
		(mh.ins[i].loop_size > 1 && mod->xxs[i].lpe > 8) ?
			'L' : ' ', mod->xxi[i].sub[0].vol,
		mod->xxi[i].sub[0].fin >> 4,
		ptkloop && mod->xxs[i].lps == 0 && mh.ins[i].loop_size > 1 &&
			mod->xxs[i].len > mod->xxs[i].lpe ? '!' : ' ');
    }

    if (libxmp_init_pattern(mod) < 0)
	return -1;

    /* Load and convert patterns */
    D_(D_INFO "Stored patterns: %d", mod->pat);

    for (i = 0; i < mod->pat; i++) {
	long pos;

	if (libxmp_alloc_pattern_tracks(mod, i, 64) < 0)
	    return -1;

	pos = hio_tell(f);
	if (pos < 0) {
		return -1;
	}

	for (j = 0; j < (64 * mod->chn); j++) {
            int period;

	    event = &EVENT(i, j % mod->chn, j / mod->chn);
	    hio_read(mod_event, 1, 4, f);

	    period = ((int)(LSN(mod_event[0])) << 8) | mod_event[1];
	    if (period != 0 && (period < 108 || period > 907)) {
		out_of_range = 1;
	    }

	    /* Filter noisetracker events */
	    if (tracker_id == TRACKER_PROBABLY_NOISETRACKER) {
		unsigned char fxt = LSN(mod_event[2]);
		unsigned char fxp = LSN(mod_event[3]);

        	if ((fxt > 0x06 && fxt < 0x0a) || (fxt == 0x0e && fxp > 1)) {
		    tracker_id = TRACKER_UNKNOWN;
		}
	    }
	}

        if (out_of_range) {
            if (tracker_id == TRACKER_UNKNOWN && mh.restart == 0x7f) {
	        tracker_id = TRACKER_SCREAMTRACKER3;
            }

	    /* Check out-of-range notes in Amiga trackers */
            if (tracker_id == TRACKER_PROTRACKER ||
                    tracker_id == TRACKER_NOISETRACKER ||
                    tracker_id == TRACKER_PROBABLY_NOISETRACKER ||
                    tracker_id == TRACKER_SOUNDTRACKER) {   /* note > B-3 */
                tracker_id = TRACKER_UNKNOWN;
            }
        }

	hio_seek(f, pos, SEEK_SET);

	for (j = 0; j < (64 * mod->chn); j++) {
	    event = &EVENT(i, j % mod->chn, j / mod->chn);
	    hio_read(mod_event, 1, 4, f);

	    switch (tracker_id) {
	    case TRACKER_PROBABLY_NOISETRACKER:
	    case TRACKER_NOISETRACKER:
	    	libxmp_decode_noisetracker_event(event, mod_event);
		break;
	    default:	
	        libxmp_decode_protracker_event(event, mod_event);
	    }
	}
    }

    switch (tracker_id) {
    case TRACKER_PROTRACKER:
	tracker = "Protracker";
	ptkloop = 1;
	break;
    case TRACKER_PROBABLY_NOISETRACKER:
    case TRACKER_NOISETRACKER:
	tracker = "Noisetracker";
	m->quirk |= QUIRK_NOBPM;
	break;
    case TRACKER_SOUNDTRACKER:
	tracker = "Soundtracker";
	m->quirk |= QUIRK_NOBPM;
	break;
    case TRACKER_FASTTRACKER:
    case TRACKER_FASTTRACKER2:
	tracker = "Fast Tracker";
	m->period_type = PERIOD_AMIGA;
	break;
    case TRACKER_TAKETRACKER:
	tracker = "Take Tracker";
	m->period_type = PERIOD_AMIGA;
	break;
    case TRACKER_OCTALYSER:
	tracker = "Octalyser";
	break;
    case TRACKER_DIGITALTRACKER:
	tracker = "Digital Tracker";
	break;
    case TRACKER_FLEXTRAX:
	tracker = "Flextrax";
	break;
    case TRACKER_MODSGRAVE:
	tracker = "Mod's Grave";
	break;
    case TRACKER_SCREAMTRACKER3:
	tracker = "Scream Tracker";
	m->period_type = PERIOD_AMIGA;
	break;
    case TRACKER_CONVERTEDST:
    case TRACKER_CONVERTED:
	tracker = "Converted";
	break;
    case TRACKER_CLONE:
	tracker = "Protracker clone";
	m->period_type = PERIOD_AMIGA;
	break;
    case TRACKER_OPENMPT:
	tracker = "OpenMPT";
	ptkloop = 1;
	break;
    default:
    case TRACKER_UNKNOWN_CONV:
    case TRACKER_UNKNOWN:
	tracker = "Unknown tracker";
	m->period_type = PERIOD_AMIGA;
	break;
    }

    if (out_of_range) {
	m->period_type = PERIOD_AMIGA;
    }

    if (tracker_id == TRACKER_MODSGRAVE) {
	snprintf(mod->type, XMP_NAME_SIZE, "%s", tracker);
    } else {
	snprintf(mod->type, XMP_NAME_SIZE, "%s %s", tracker, magic);
    }

    MODULE_INFO();


    /* Load samples */

    if (m->filename && (x = strrchr(m->filename, '/')))
	strncpy(pathname, m->filename, x - m->filename);

    D_(D_INFO "Stored samples: %d", mod->smp);

    for (i = 0; i < mod->smp; i++) {
	int flags;

	if (!mod->xxs[i].len)
	    continue;

	flags = (ptkloop && mod->xxs[i].lps == 0) ? SAMPLE_FLAG_FULLREP : 0;

	if (ptsong) {
	    HIO_HANDLE *s;
	    char sn[256];
	    snprintf(sn, XMP_NAME_SIZE, "%s%s", pathname, mod->xxi[i].name);
	
	    if ((s = hio_open(sn, "rb"))) {
	        if (libxmp_load_sample(m, s, flags, &mod->xxs[i], NULL) < 0) {
		    hio_close(s);
		    return -1;
		}
		hio_close(s);
	    }
	} else {
	    uint8 buf[5];
            long pos;
            int num;

	    if ((pos = hio_tell(f)) < 0) {
                return -1;
            }
            num = hio_read(buf, 1, 5, f);

	    if (num == 5 && !memcmp(buf, "ADPCM", 5)) {
		flags |= SAMPLE_FLAG_ADPCM;
	    } else {
		hio_seek(f, pos, SEEK_SET);
	    }

	    if (libxmp_load_sample(m, f, flags, &mod->xxs[i], NULL) < 0)
		return -1;
	}
    }

    if (tracker_id == TRACKER_PROTRACKER || tracker_id == TRACKER_OPENMPT) {
	m->quirk |= QUIRK_PROTRACK;
    } else if (tracker_id == TRACKER_SCREAMTRACKER3) {
	m->c4rate = C4_NTSC_RATE;
	m->quirk |= QUIRKS_ST3;
	m->read_event_type = READ_EVENT_ST3;
    } else if (tracker_id == TRACKER_FASTTRACKER || tracker_id == TRACKER_FASTTRACKER2 || tracker_id == TRACKER_TAKETRACKER || tracker_id == TRACKER_MODSGRAVE || mod->chn > 4) {
	m->c4rate = C4_NTSC_RATE;
	m->quirk |= QUIRKS_FT2 | QUIRK_FTMOD;
	m->read_event_type = READ_EVENT_FT2;
	m->period_type = PERIOD_AMIGA;
    }

    return 0;
}
