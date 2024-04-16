/*
 * gbsplay is a Gameboy sound player
 *
 * 2003-2021 (C) by Tobias Diedrich <ranma+gbsplay@tdiedrich.de>
 *                  Christian Garbs <mitch@cgarbs.de>
 *
 * Licensed under GNU GPL v1 or, at your option, any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "mapper.h"
#include "gbhw.h"
#include "gbcpu.h"
#include "libgbs.h"
#include "gbs_internal.h"
#include "crc32.h"

#ifdef USE_ZLIB
#include <zlib.h>
#endif

/* Max GB rom size is 4MiB (mapper with 256 banks) */
#define GB_MAX_ROM_SIZE (256 * 0x4000)

#define HDR_LEN_GBS	0x70
#define HDR_LEN_GBR	0x20
#define HDR_LEN_GB	0x150
#define HDR_LEN_GZIP	10
#define HDR_LEN_VGM	0x100

#define GBS_MAGIC		"GBS"
#define GBR_MAGIC		"GBRF"
#define GD3_MAGIC		"Gd3 "
#define VGM_MAGIC		"Vgm "
#define GZIP_MAGIC		"\037\213\010"

const char *boot_rom_file = ".dmg_rom.bin";

enum filetype {
	FILETYPE_GBS = 0,
	FILETYPE_GBR = 1,
	FILETYPE_GB  = 2,
	FILETYPE_VGM = 3,
};

struct gbs_subsong_info {
	uint32_t len;  /* GBS_LEN_DIV (1024) == 1 second */
	char *title;
};

struct gbs {
	char *buf;
	int buf_owned;
	uint8_t version;
	uint8_t songs;
	uint8_t defaultsong;
	uint8_t defaultbank;
	uint16_t load;
	uint16_t init;
	uint16_t play;
	uint16_t stack;
	uint8_t tma;
	uint8_t tac;
	char *title;
	char *author;
	char *copyright;
	unsigned long codelen;
	char *code;
	size_t filesize;
	uint32_t crc;
	uint32_t crcnow;
	struct gbs_subsong_info *subsong_info;
	char *strings;
	char v1strings[33*3];
	uint8_t *rom;
	unsigned long romsize;

	long long ticks;
	int16_t lmin, lmax, lvol, rmin, rmax, rvol;
	long subsong_timeout, silence_timeout, fadeout, gap;
	long long silence_start;
	int subsong;

	struct gbs_output_buffer *buffer;

	gbs_io_cb io_cb;
	void *io_cb_priv;

	gbs_step_cb step_cb;
	void *step_cb_priv;

	gbs_sound_cb sound_cb;
	void *sound_cb_priv;

	gbs_nextsubsong_cb nextsubsong_cb;
	void *nextsubsong_cb_priv;

	struct gbs_metadata metadata;
	struct gbs_channel_status step_cb_channels[4];
	struct gbs_status status; // note: this contains a separate gbs_channel_status[] to not interfere with the step callback
	struct gbhw_buffer gbhw_buf;
	struct gbhw gbhw;
	struct mapper *mapper;

	enum filetype filetype;
};

const struct gbs_metadata *gbs_get_metadata(struct gbs* const gbs)
{
	gbs->metadata.title = gbs->title;
	gbs->metadata.author = gbs->author;
	gbs->metadata.copyright = gbs->copyright;
	return &gbs->metadata;
}

static void update_status_on_subsong_change(struct gbs* const gbs);

void gbs_configure(struct gbs* const gbs, long subsong, long subsong_timeout, long silence_timeout, long subsong_gap, long fadeout)
{
	gbs->subsong = subsong;
	update_status_on_subsong_change(gbs);
	if (silence_timeout) {
		if (gbs->filetype == FILETYPE_GB && silence_timeout < 10) {
			/* GB ROMs usually have a fairly long silence in the beginning,
			   enforce a minimum timeout for convenience. */
			silence_timeout = 10;
		}
	}
	gbs->subsong_timeout = subsong_timeout;
	gbs->silence_timeout = silence_timeout;
	gbs->gap = subsong_gap;
	gbs->fadeout = fadeout;
}

void gbs_set_loop_mode(struct gbs* const gbs, enum gbs_loop_mode mode)
{
	gbs->status.loop_mode = mode;
	/* Remaining time clamping may change based on loop_mode */
	update_status_on_subsong_change(gbs);
}

void gbs_cycle_loop_mode(struct gbs* const gbs)
{
	switch (gbs->status.loop_mode) {
	default:
	case LOOP_OFF:    gbs_set_loop_mode(gbs, LOOP_RANGE);  break;
	case LOOP_RANGE:  gbs_set_loop_mode(gbs, LOOP_SINGLE); break;
	case LOOP_SINGLE: gbs_set_loop_mode(gbs, LOOP_OFF);    break;
	}
}

static int wave_autocorrelate(uint8_t wave[32], int ofs)
{
	int i, sum = 0;

	for (i=0; i<32; i++) {
		sum += (int)wave[i] * (int)wave[(i+ofs) % 32];
	}

	return sum;
}

static int wave_scalefactor(const struct gbs* const gbs)
{
	static const uint8_t offsets[] = {
		31, /* offset 1 instead of direct self-correlation, this simplifies the algorithm */
		16, /* best correlation offset if pattern repeats two times */
		8,  /* test for pattern repeating four times */
		4,  /* eight times */
		2,  /* sixteen times */
		/* thirty-two would be a DC voltage */
	};
	uint8_t wave[32];
	int i, imax = 0, summax = 0;
	/* Read waveform ram */
	for (i=0; i<16; i++) {
		uint8_t b = gbs_io_peek(gbs, 0xff30+i);
		wave[2*i] = b >> 4;
		wave[2*i+1] = b & 0xf;
	}
	/* Find best autocorrelation, most likely one of the first two */
	for (i=0; i<ARRAY_SIZE(offsets); i++) {
		int sum = wave_autocorrelate(wave, offsets[i]);
		if (sum > summax) {
			imax = i;
			summax = sum;
		}
	}
	return imax;
}


/* FIXME: Clean up the api and make public? */
static int gbs_midi_note(const struct gbs* const gbs, long div_tc, int ch)
{
    //YOYOFR: add subnote
//#define FREQ(x,ch) (131072. / ((x)<<((ch)>1)))
//#define FREQTONOTE(f) (lround(log2((f)/C0HZ)*12)+C0MIDI)
//#define NOTE(x,ch) FREQTONOTE(FREQ(x,ch))
    double freq=(131072. / ((div_tc)<<((ch)>1)));
    double dnote=log2((freq)/C0HZ)*12+C0MIDI;
    int note=lround(dnote);
    int subnote=lround((dnote-note)*8.0f);
    if (subnote>7) subnote=7;
    if (subnote<-7) subnote=-7;
    note=note|((subnote<8?subnote:subnote+7+8)<<8);
//YOYOFR
    
	//int note = NOTE(div_tc, ch);
	if (ch == 2) {
		// Scale by scalefactor octaves:
		// 0: no repetitions, keep as is
		// 1: repeated twice, one octave higher
		// 2: repeated four times, two octaves higher
		// ...
		note += wave_scalefactor(gbs) * 12;
	}
	return note;
}

void gbs_configure_channels(struct gbs* const gbs, long mute_0, long mute_1, long mute_2, long mute_3) {
	gbs->gbhw.ch[0].mute = mute_0;
	gbs->gbhw.ch[1].mute = mute_1;
	gbs->gbhw.ch[2].mute = mute_2;
	gbs->gbhw.ch[3].mute = mute_3;
}

void gbs_configure_output(struct gbs* const gbs, struct gbs_output_buffer *gbs_buf, long rate) {
	struct gbhw_buffer *gbhw_buf = &gbs->gbhw_buf;

	gbhw_set_rate(&gbs->gbhw, rate);

	gbs->buffer = gbs_buf;

	gbhw_buf->data = gbs_buf->data;
	gbhw_buf->bytes = gbs_buf->bytes;
	gbhw_buf->pos = gbs_buf->pos;

	gbhw_set_buffer(&gbs->gbhw, gbhw_buf);
}

long gbs_init(struct gbs* const gbs, long subsong)
{
	struct gbhw *gbhw = &gbs->gbhw;
	struct gbcpu *gbcpu = &gbhw->gbcpu;

	gbhw_init(gbhw);

	if (subsong == -1) subsong = gbs->defaultsong - 1;
	if (subsong >= gbs->songs) {
		fprintf(stderr, _("Subsong number out of range (min=0, max=%d).\n"), (int)gbs->songs - 1);
		return 0;
	}

	if (gbs->defaultbank != 1) {
		gbcpu_mem_put(gbcpu, 0x2000, gbs->defaultbank);
	}
	gbhw_io_put(gbhw, 0xff06, gbs->tma);
	gbhw_io_put(gbhw, 0xff07, gbs->tac);
	gbhw_io_put(gbhw, 0xffff, 0x05);

	REGS16_W(gbcpu->regs, SP, gbs->stack);

	/* put halt breakpoint PC on stack */
	gbcpu->halt_at_pc = 0xffff;
	REGS16_W(gbcpu->regs, GBS_PC, 0xff80);
	REGS16_W(gbcpu->regs, HL, gbcpu->halt_at_pc);
	gbcpu_mem_put(gbcpu, 0xff80, 0xe5); /* push hl */
	gbcpu_step(gbcpu);
	/* clear regs/memory touched by stack setup */
	REGS16_W(gbcpu->regs, HL, 0x0000);
	gbcpu_mem_put(gbcpu, 0xff80, 0x00);

	REGS16_W(gbcpu->regs, GBS_PC, gbs->init);
	gbcpu->regs.rn.a = subsong;

	gbs->ticks = 0;
	gbs->subsong = subsong;

	update_status_on_subsong_change(gbs);

	return 1;
}

// FIXME: or just include a *ptr to the whole RAM in struct gbs_status?
uint8_t gbs_io_peek(const struct gbs* const gbs, uint16_t addr) {
	return gbhw_io_peek(&gbs->gbhw, addr);
}

static long chvol(const struct gbhw_channel channels[], long ch)
{
	long v;

	if (channels[ch].mute ||
	    channels[ch].master == 0 ||
	    (channels[ch].leftgate == 0 &&
	     channels[ch].rightgate == 0)) return 0;

	if (ch == 2)
		v = (3-((channels[2].env_volume+3)&3)) << 2;
	else v = channels[ch].env_volume;

	return v;
}


static void map_channel_status(const struct gbhw_channel from[], struct gbs_channel_status to[]) {
	for (long i = 0; i < 4; i++) {
		to[i].mute = from[i].mute;
		to[i].vol = chvol(from, i);
		to[i].div_tc = from[i].div_tc;
		to[i].playing = from[i].running && from[i].master && from[i].env_volume;
	}
}

void gbs_set_nextsubsong_cb(struct gbs* const gbs, gbs_nextsubsong_cb cb, void *priv)
{
	gbs->nextsubsong_cb = cb;
	gbs->nextsubsong_cb_priv = priv;
}

static void wrap_io_callback(cycles_t cycles, uint32_t addr, uint8_t value, void *priv)
{
	struct gbs *gbs = priv;
	gbs->io_cb(gbs, cycles, addr, value, gbs->io_cb_priv);
}

void gbs_set_io_callback(struct gbs* const gbs, gbs_io_cb fn, void *priv)
{
	gbs->io_cb = fn;
	gbs->io_cb_priv = priv;
	gbhw_set_io_callback(&gbs->gbhw, wrap_io_callback, gbs);
}

static void wrap_step_callback(const cycles_t cycles, const struct gbhw_channel ch[], void *priv)
{
	struct gbs* gbs = priv;
	map_channel_status(ch, gbs->step_cb_channels);
	gbs->step_cb(gbs, cycles, gbs->step_cb_channels, gbs->step_cb_priv);
}

void gbs_set_step_callback(struct gbs* const gbs, gbs_step_cb fn, void *priv)
{
	gbs->step_cb = fn;
	gbs->step_cb_priv = priv;
	gbhw_set_step_callback(&gbs->gbhw, wrap_step_callback, gbs);
}

static void wrap_sound_callback(void *priv)
{
	struct gbs* gbs = priv;
	gbs->buffer->pos = gbs->gbhw_buf.pos;
	gbs->sound_cb(gbs, gbs->buffer, gbs->sound_cb_priv);
}

void gbs_set_sound_callback(struct gbs* const gbs, gbs_sound_cb fn, void *priv)
{
	gbs->sound_cb = fn;
	gbs->sound_cb_priv = priv;
	gbhw_set_callback(&gbs->gbhw, wrap_sound_callback, gbs);
}

long gbs_set_filter(struct gbs* const gbs, enum gbs_filter_type type) {
	return gbhw_set_filter(&gbs->gbhw, type);
}
static long gbs_nextsubsong(struct gbs* const gbs)
{
	if (gbs->nextsubsong_cb != NULL) {
		return gbs->nextsubsong_cb(gbs, gbs->nextsubsong_cb_priv);
	} else {
		gbs->subsong++;
		if (gbs->subsong >= gbs->songs)
			return false;
		gbs_init(gbs, gbs->subsong);
	}
	return true;
}

long gbs_step(struct gbs* const gbs, long time_to_work)
{
	struct gbhw *gbhw = &gbs->gbhw;

	cycles_t cycles = gbhw_step(gbhw, time_to_work);
	long time;

	if (cycles < 0) {
		return false;
	}

	gbs->ticks += cycles;

	gbhw_calc_minmax(gbhw, &gbs->lmin, &gbs->lmax, &gbs->rmin, &gbs->rmax);
	gbs->lvol = -gbs->lmin > gbs->lmax ? -gbs->lmin : gbs->lmax;
	gbs->rvol = -gbs->rmin > gbs->rmax ? -gbs->rmin : gbs->rmax;

	time = gbs->ticks / GBHW_CLOCK;
	if (gbs->silence_timeout) {
		if (gbs->lmin == gbs->lmax && gbs->rmin == gbs->rmax) {
			if (gbs->silence_start == 0)
				gbs->silence_start = gbs->ticks;
		} else gbs->silence_start = 0;
	}

	if (gbs->silence_start &&
	    (gbs->ticks - gbs->silence_start) / GBHW_CLOCK >= gbs->silence_timeout) {
		if (gbs->subsong_info[gbs->subsong].len == 0) {
			gbs->subsong_info[gbs->subsong].len = gbs->ticks * GBS_LEN_DIV / GBHW_CLOCK;
		}
		return gbs_nextsubsong(gbs);
	}

	if (gbs->subsong_timeout && gbs->status.loop_mode != LOOP_SINGLE) {
		if (gbs->fadeout &&
		    time >= gbs->subsong_timeout - gbs->fadeout - gbs->gap)
			gbhw_master_fade(gbhw, 128/gbs->fadeout, 0);
		if (time >= gbs->subsong_timeout - gbs->gap)
			gbhw_master_fade(gbhw, 128*16, 0);
		if (time >= gbs->subsong_timeout)
			return gbs_nextsubsong(gbs);
	}

	return true;
}

void gbs_print_info(const struct gbs* const gbs, long verbose)
{
	printf(_("GBSVersion:       %u\n"
	         "Title:            \"%s\"\n"
	         "Author:           \"%s\"\n"
	         "Copyright:        \"%s\"\n"
	         "Load address:     0x%04x\n"
	         "Init address:     0x%04x\n"
	         "Play address:     0x%04x\n"
	         "Stack pointer:    0x%04x\n"
	         "File size:        0x%08x\n"
	         "ROM size:         0x%08lx (%ld banks)\n"
	         "Subsongs:         %u\n"
	         "Default subsong:  %u\n"),
	       gbs->version,
	       gbs->title,
	       gbs->author,
	       gbs->copyright,
	       gbs->load,
	       gbs->init,
	       gbs->play,
	       gbs->stack,
	       (unsigned int)gbs->filesize,
	       gbs->romsize,
	       gbs->romsize/0x4000,
	       gbs->songs,
	       gbs->defaultsong);
	if (gbs->tac & 0x04) {
		printf(_("Timing:           %2.2fHz timer%s\n"),
		       gbhw_calc_timer_hz(gbs->tac, gbs->tma),
		       (gbs->tac & 0x78) == 0x40 ? _(" + VBlank (ugetab)") : "");
	} else {
		printf(_("Timing:           %s\n"),
		       _("59.7Hz VBlank\n"));
	}
	if (gbs->defaultbank != 1) {
		printf(_("Bank @0x4000:     %d\n"), gbs->defaultbank);
	}
	printf(_("CRC32:            0x%08lx\n"), (unsigned long)gbs->crcnow);
}

static void update_status_on_subsong_change(struct gbs* const gbs) {
	struct gbs_status *status = &gbs->status;

	status->songtitle = gbs->subsong_info[gbs->subsong].title;
	if (!status->songtitle) {
		status->songtitle = _("Untitled");
	}
	status->subsong = gbs->subsong;
	status->subsong_len = gbs->subsong_info[gbs->subsong].len;
	if (status->subsong_len == 0 && gbs->status.loop_mode != LOOP_SINGLE) {
		status->subsong_len = gbs->subsong_timeout * 1024;
	}
}

const struct gbs_status* gbs_get_status(struct gbs* const gbs) {

	struct gbs_status *status = &gbs->status;
	const struct gbhw *gbhw = &gbs->gbhw;

	status->rvol = gbs->rvol;
	status->lvol = gbs->lvol;
	status->ticks = gbs->ticks;

	map_channel_status(gbhw->ch, status->ch);

	return status;
}

long gbs_toggle_mute(struct gbs* const gbs, long channel) {
	return gbs->gbhw.ch[channel].mute ^= 1;
}

//YOYOFR
long gbs_toggle_setmute(struct gbs* const gbs, long channel,long muteval) {
    return (gbs->gbhw.ch[channel].mute = muteval);
}
void gbs_set_default_length(struct gbs* const gbs, long length) {
    gbs->subsong_timeout=length;
}
//YOYOFR

static void gbs_free(struct gbs* const gbs)
{
	gbhw_cleanup(&gbs->gbhw);
	if (gbs->mapper)
		mapper_free(gbs->mapper);
	if (gbs->buf && gbs->buf_owned)
		free(gbs->buf);
	if (gbs->rom)
		free(gbs->rom);
	if (gbs->subsong_info)
		free(gbs->subsong_info);
	free(gbs);
}

void gbs_close(struct gbs* const gbs)
{
	gbs_free(gbs);
}

static uint32_t readint(const char* const buf, long bytes)
{
	long i;
	long shift = 0;
	uint32_t res = 0;

	for (i=0; i<bytes; i++) {
		res |= (unsigned char)buf[i] << shift;
		shift += 8;
	}

	return res;
}

long gbs_write(const struct gbs* const gbs, const char* const name)
{
	long fd;
	char pad[16];
	long newlen = gbs->filesize;
	long namelen = strlen(name);
	char *tmpname = malloc(namelen + sizeof(".tmp\0"));

	memcpy(tmpname, name, namelen);
	sprintf(&tmpname[namelen], ".tmp");
	memset(pad, 0xff, sizeof(pad));

	if ((fd = open(tmpname, O_WRONLY|O_CREAT|O_TRUNC, 0644)) == -1) {
		fprintf(stderr, _("Could not open %s: %s\n"), name, strerror(errno));
		return 0;
	}

	if (write(fd, gbs->buf, newlen) == newlen) {
		int ret = 1;
		close(fd);
		if (rename(tmpname, name) == -1) {
			fprintf(stderr, _("Could not rename %s to %s: %s\n"), tmpname, name, strerror(errno));
			ret = 0;
		}
		return ret;
	}
	close(fd);

	return 1;
}

void gbs_write_rom(const struct gbs* const gbs, FILE *out, const uint8_t* const logo_data)
{
	if (gbs->rom[0x104] != 0xce) {
		unsigned long tmp = gbs->romsize;
		int i;
		uint8_t rom_size = 0;
		uint8_t chksum = 0x19;

		while (tmp > 32768) {
			rom_size++;
			tmp >>= 1;
		}
		if (rom_size > 8) {
			fputs(_("ROM size above limit (8 MiB)!"), stderr);
		}
		memcpy(&gbs->rom[0x104], logo_data, 0x30);
		snprintf((char*)&gbs->rom[0x134], 16, "%s", gbs->title);
		gbs->rom[0x147] = 0x02;  /* MBC1+RAM */
		gbs->rom[0x148] = rom_size;
		gbs->rom[0x149] = 0x02;  /* 8KiB of RAM */

		for (i = 0x134; i < 0x14c; i++) {
			chksum += gbs->rom[i];
		}
		gbs->rom[0x14d] = -chksum;
	}
	fwrite(gbs->rom, 1, gbs->romsize, out);
}

static struct gbs* gbs_new(char *buf)
{
	struct gbs* gbs = calloc(sizeof(struct gbs), 1);
	gbhw_init_struct(&gbs->gbhw);
	gbs->silence_timeout = 2*60;
	gbs->subsong_timeout = 2*60;
	gbs->gap = 2;
	gbs->fadeout = 3;
	gbs->songs = 1;
	gbs->defaultsong = 1;
	gbs->defaultbank = 1;
	gbs->init = 0x100;
	gbs->play = 0x100;
	gbs->stack = 0xfffe;
	gbs->buf = buf;
	return gbs;
}

const uint8_t *gbs_get_bootrom()
{
	static uint8_t bootrom[256];
	char *bootname = NULL;
	size_t name_len;
	FILE *romf;

	name_len = strlen(getenv("HOME")) + strlen(boot_rom_file) + 2;
	bootname = malloc(name_len);
	snprintf(bootname, name_len, "%s/%s", getenv("HOME"), boot_rom_file);
	romf = fopen(bootname, "rb");
	free(bootname);
	if (!romf) {
		return NULL;
	}
	if (fread(bootrom, 1, sizeof(bootrom), romf) != sizeof(bootrom)) {
		fclose(romf);
		return NULL;
	}
	fclose(romf);

	return bootrom;
}

static struct gbs *gb_open(const char* const name, char *buf, size_t size)
{
	long i;
	struct gbs* gbs = gbs_new(buf);
	char *na_str = _("gb / not available");
	const uint8_t *bootrom = gbs_get_bootrom();

	UNUSED(name);

	/* Test if this looks like a valid rom header title */
	for (i=0x0134; i<0x0143; i++) {
		if (!(isalnum(buf[i]) || isspace(buf[i])))
			break;
	}
	if (buf[i] == 0) {
		/* Title looks valid and is zero-terminated. */
		gbs->title = &buf[0x0134];
	} else {
		gbs->title = na_str;
	}
	gbs->filetype = FILETYPE_GB;
	gbs->author = na_str;
	gbs->copyright = na_str;
	gbs->code = buf;
	gbs->filesize = size;

	gbs->subsong_info = calloc(sizeof(struct gbs_subsong_info), gbs->songs);
	gbs->codelen = size - 0x20;
	gbs->crcnow = gbs_crc32(0, buf, gbs->filesize);
	gbs->romsize = (gbs->codelen + 0x3fff) & ~0x3fff;

	gbs->rom = calloc(1, gbs->romsize);
	memcpy(gbs->rom, buf, gbs->codelen);

	gbs->mapper = mapper_gb(&gbs->gbhw.gbcpu, gbs->rom, gbs->romsize, buf[0x147], buf[0x148], buf[0x149]);
	if (!gbs->mapper) {
		fprintf(stderr, _("Unsupported cartridge type: 0x%02x\n"), buf[0x147]);
		gbs_free(gbs);
		return NULL;
	}

	/* For accuracy testing purposes, support boot rom. */
	if (bootrom != NULL) {
		gbhw_enable_bootrom(&gbs->gbhw, bootrom);
		gbs->init = 0;
	}
	gbs->buf_owned = 1;
	return gbs;
}

static struct gbs *gbr_open(const char* const name, char *buf, size_t size)
{
	long i;
	struct gbs* gbs = gbs_new(buf);
	char *na_str = _("gbr / not available");
	uint16_t vsync_addr;
	uint16_t timer_addr;

	if (strncmp(buf, GBR_MAGIC, 4) != 0) {
		fprintf(stderr, _("Not a GBR-File: %s\n"), name);
		gbs_free(gbs);
		return NULL;
	}
	if (buf[0x07] < 1 || buf[0x07] > 3) {
		fprintf(stderr, _("Unsupported timerflag value: %d\n"), buf[0x07]);
		gbs_free(gbs);
		return NULL;
	}
	gbs->filetype = FILETYPE_GBR;
	gbs->songs = 255;
	gbs->defaultbank = buf[0x06];
	gbs->init  = readint(&buf[0x08], 2);
	vsync_addr = readint(&buf[0x0a], 2);
	timer_addr = readint(&buf[0x0c], 2);

	if (buf[0x07] == 1) {
		gbs->play = vsync_addr;
	} else {
		gbs->play = timer_addr;
	}
	gbs->tma = buf[0x0e];
	gbs->tac = buf[0x0f];

	/* Test if this looks like a valid rom header title */
	for (i=0x0154; i<0x0163; i++) {
		if (!(isalnum(buf[i]) || isspace(buf[i])))
			break;
	}
	if (buf[i] == 0) {
		/* Title looks valid and is zero-terminated. */
		gbs->title = &buf[0x0154];
	} else {
		gbs->title = na_str;
	}
	gbs->author = na_str;
	gbs->copyright = na_str;
	gbs->code = &buf[0x20];
	gbs->filesize = size;

	gbs->subsong_info = calloc(sizeof(struct gbs_subsong_info), gbs->songs);
	gbs->codelen = size - 0x20;
	gbs->crcnow = gbs_crc32(0, buf, gbs->filesize);
	gbs->romsize = (gbs->codelen + 0x3fff) & ~0x3fff;

	gbs->rom = calloc(1, gbs->romsize);
	memcpy(gbs->rom, &buf[0x20], gbs->codelen);

	gbs->mapper = mapper_gbr(&gbs->gbhw.gbcpu, gbs->rom, gbs->romsize, buf[5], buf[6]);

	gbs->rom[0x40] = 0xd9; /* reti */
	gbs->rom[0x50] = 0xd9; /* reti */
	if (buf[0x07] & 1) {
		/* V-Blank */
		gbs->rom[0x40] = 0xcd; /* call imm16 */
		gbs->rom[0x41] = vsync_addr & 0xff;
		gbs->rom[0x42] = vsync_addr >> 8;
		gbs->rom[0x43] = 0xd9; /* reti */
	}
	if (buf[0x07] & 2) {
		/* Timer */
		gbs->rom[0x50] = 0xcd; /* call imm16 */
		gbs->rom[0x51] = timer_addr & 0xff;
		gbs->rom[0x52] = timer_addr >> 8;
		gbs->rom[0x53] = 0xd9; /* reti */
	}

	gbs->buf_owned = 1;
	return gbs;
}

static uint32_t le32(const char* const buf)
{
	const uint8_t *b = (const uint8_t*)buf;
	return b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24);
}

static uint16_t le16(const char* const buf)
{
	const uint8_t *b = (const uint8_t*)buf;
	return b[0] | (b[1] << 8);
}

static void emit(struct gbs* const gbs, long* const code_used, uint8_t data, long reserve)
{
	long remain = gbs->codelen - *code_used;
	uint8_t *code = (uint8_t*) &gbs->code[*code_used];
	if (reserve + 1 > remain) {
		while (remain-- > 0) {
			*(code++) = 0xc9;  /* RET */
			(*code_used)++;
		}
		gbs->code = realloc(gbs->code, gbs->codelen + 0x4000);
		gbs->codelen += 0x4000;
		code = (uint8_t*) &gbs->code[*code_used];
	}
	*(code++) = data;
	(*code_used)++;
}

static void gd3_parse(struct gbs **gbs, const char* const gd3, long gd3_len)
{
	char *buf;
	char *s;
	long ofs = 12;
	long idx = 0;
	if (gd3_len < 12 || gd3_len > 4096) {
		return;
	}
	if (strncmp(gd3, GD3_MAGIC, 4) != 0) {
		return;
	}
	if (le32(&gd3[4]) != 0x00000100) {
		return;
	}
	if (le32(&gd3[8]) != gd3_len - ofs) {
		return;
	}
	*gbs = realloc(*gbs, sizeof(struct gbs) + gd3_len);
	s = buf = (char *)&(*gbs)[1];
	while (ofs < gd3_len) {
		uint16_t val = le16(&gd3[ofs]);
		if (val == 0) {
			*(buf++) = 0;
			switch (idx) {
			case 0: (*gbs)->subsong_info[0].title = s; break;
			case 2: (*gbs)->title = s; break;
			case 6: (*gbs)->author = s; break;
			default: break;
			}
			s = buf;
			idx++;
		} else if (val < 256) {
			*(buf++) = val;
		} else {
			*(buf++) = '?';
		}
		ofs += 2;
	}
}

static struct gbs *vgm_open(const char* const name, char* const buf, size_t size)
{
	struct gbs* gbs = gbs_new(buf);
	char *na_str = _("vgm / not available");
	char *gd3 = NULL;
	char *data;
	long dmg_clock;
	uint32_t eof_ofs;
	long gd3_ofs;
	long gd3_len;
	long data_ofs;
	long data_len;
	long vgm_parsed = false;
	long total_wait;
	long total_clocks;
	long code_used;
	long addr;
	long jpaddr;

	if (strncmp(buf, VGM_MAGIC, 4) != 0) {
		fprintf(stderr, _("Not a VGM-File: %s\n"), name);
		gbs_free(gbs);
		return NULL;
	}
	if (buf[0x09] != 1 || buf[0x08] < 0x61) {
		fprintf(stderr, _("Unsupported VGM version: %d.%02x\n"), buf[0x09], buf[0x08]);
		gbs_free(gbs);
		return NULL;
	}
	dmg_clock = le32(&buf[0x80]);
	if (dmg_clock != 4194304) {
		fprintf(stderr, _("Unsupported DMG clock: %ldHz\n"), dmg_clock);
		gbs_free(gbs);
		return NULL;
	}
	eof_ofs = le32(&buf[0x4]) + 0x4;
	if (eof_ofs > size) {
		fprintf(stderr, _("Bad file size in header: %ld\n"), eof_ofs);
		gbs_free(gbs);
		return NULL;
	}
	gd3_ofs = le32(&buf[0x14]) + 0x14;
	if (gd3_ofs == 0x14) {
		gd3_ofs = eof_ofs;
		gd3_len = 0;
	} else {
		gd3_len = eof_ofs - gd3_ofs;
		gd3 = &buf[gd3_ofs];
		if (gd3_len < 4 || strncmp(gd3, GD3_MAGIC, 4) != 0) {
			fprintf(stderr, _("Bad GD3 offset: %08lx\n"), gd3_ofs);
			gbs_free(gbs);
			return NULL;
		}
	}
	data_ofs = le32(&buf[0x34]) + 0x34;
	data_len = gd3_ofs - data_ofs;
	data = &buf[data_ofs];
	if (data_len < 0) {
		fprintf(stderr, _("Bad data length: %ld\n"), data_len);
		gbs_free(gbs);
		return NULL;
	}

	gbs->filetype = FILETYPE_VGM;
	gbs->codelen = 0x4000;
	gbs->code = calloc(1, gbs->codelen);
	code_used = 0;

	total_wait = total_clocks = 0;
	while (!vgm_parsed) {
		switch ((uint8_t)*data) {
		default:
			fprintf(stderr, _("Unsupported VGM opcode: 0x%02x\n"), *data);
			gbs_free(gbs);
			return NULL;
		case 0x61:  /* Wait n samples */
			total_wait += le16(&data[1]);
			data += 2;
			break;
		case 0x62:  /* Wait 735 (1/60s) */
			total_wait += 735;
			break;
		case 0x63:  /* Wait 882 (1/50s) */
			total_wait += 882;
			break;
		case 0x66:  /* End of sound data */
			vgm_parsed = true;
			break;
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
		case 0x78:
		case 0x79:
		case 0x7a:
		case 0x7b:
		case 0x7c:
		case 0x7d:
		case 0x7e:
		case 0x7f:
			/* Wait n+1 samples */
			total_wait += (*data & 0xf) + 1;
			break;
		case 0xb3:  /* DMG write */
			{
				long delay = (total_wait * 41943L / 441L) - total_clocks;
				long units = (delay + 31) / 64;
				uint8_t reg = 0x10 + ((uint8_t)data[1] & 0x7f);
				uint8_t val = (uint8_t)data[2];

				while (units > 0) {
					long d = units > 0x10000 ?  0x10000 : units;
					emit(gbs, &code_used, 0xcf, 3); // RST 0x08
					emit(gbs, &code_used, (d - 1) & 0xff, 0);
					emit(gbs, &code_used, (d - 1) >> 8, 0);
					units -= d;
					total_clocks += d * 64;
				}
				/* LD a, imm8 */
				emit(gbs, &code_used, 0x3e, 4);
				emit(gbs, &code_used, val, 0);
				/* LDH (a8), A */
				emit(gbs, &code_used, 0xe0, 0);
				emit(gbs, &code_used, reg, 0);
			}
			data += 2;
			break;
		}
		data++;
	}
	/* RST 0x38 */
	emit(gbs, &code_used, 0xff, 1);

	gbs->load = 0x0400;
	gbs->init = 0x0440;
	gbs->play = 0x0404;
	gbs->tma = 0;
	gbs->tac = 0;
	gbs->title = na_str;
	gbs->author = na_str;
	gbs->copyright = na_str;
	gbs->filesize = size;
	gbs->crcnow = gbs_crc32(0, buf, gbs->filesize);

	gbs->subsong_info = calloc(sizeof(struct gbs_subsong_info), gbs->songs);
	gbs->subsong_info[0].len = total_clocks / (GBHW_CLOCK / GBS_LEN_DIV);

	if (gd3_len > 0) {
		gd3_parse(&gbs, gd3, gd3_len);
	}

	gbs->romsize = gbs->codelen + 0x4000;
	gbs->rom = calloc(1, gbs->romsize);
	memcpy(&gbs->rom[0x4000], gbs->code, gbs->codelen);

	gbs->mapper = mapper_gbs(&gbs->gbhw.gbcpu, gbs->rom, gbs->romsize);

	/* 16 + 52 for RST + setup */
	addr = 0x8;
	gbs->rom[addr++] = 0xe1; /* 12: pop hl */
	gbs->rom[addr++] = 0x2a; /*  8: ld a, (hl+) */
	gbs->rom[addr++] = 0x5f; /*  4: ld e, a */
	gbs->rom[addr++] = 0x2a; /*  8: ld a, (hl+) */
	gbs->rom[addr++] = 0x57; /*  4: ld d, a */
	gbs->rom[addr++] = 0xe5; /* 16: push hl */

	/* 64 cycles for loop */
	jpaddr = addr;
	gbs->rom[addr++] = 0x7a; /*  4: ld a, d */
	gbs->rom[addr++] = 0xb3; /*  4: or e */
	gbs->rom[addr++] = 0xc8; /*  8: ret z */
	gbs->rom[addr++] = 0x1b; /*  8: dec de */
	gbs->rom[addr++] = 0x23; /*  8: inc hl */
	gbs->rom[addr++] = 0x23; /*  8: inc hl */
	gbs->rom[addr++] = 0x23; /*  8: inc hl */
	gbs->rom[addr++] = 0xc3; /* 16: jp @loop */
	gbs->rom[addr++] = jpaddr & 0xff;
	gbs->rom[addr++] = jpaddr >> 8;

	/* Trap opcode 0xff (rst 0x38) execution */
	jpaddr = addr = 0x38;
	gbs->rom[addr++] = 0xf3; /* di */
	gbs->rom[addr++] = 0x76; /* halt */
	gbs->rom[addr++] = 0xc3; /* jp $ */
	gbs->rom[addr++] = jpaddr & 0xff;
	gbs->rom[addr++] = jpaddr >> 8;

	gbs->rom[0x0040] = 0xd9; /* reti */
	gbs->rom[0x0050] = 0xd9; /* reti */

	gbs->rom[0x0100] = 0x00; /* nop */
	gbs->rom[0x0101] = 0xc3; /* jp */
	gbs->rom[0x0102] = gbs->init & 0xff;
	gbs->rom[0x0103] = gbs->init >> 8;

	addr = gbs->init;
	gbs->rom[addr++] = 0xf3; /* di */
	gbs->rom[addr++] = 0x3e; /* ld a, 1 */
	gbs->rom[addr++] = 0x01;
	jpaddr = addr;
	gbs->rom[addr++] = 0xe0; /* ldh (0x80), a */
	gbs->rom[addr++] = 0x80;
	gbs->rom[addr++] = 0x21; /* ld hl, 0x2000 */
	gbs->rom[addr++] = 0x00;
	gbs->rom[addr++] = 0x20;
	gbs->rom[addr++] = 0x77; /* ld (hl), a */
	gbs->rom[addr++] = 0xcd; /* call 0x4000 */
	gbs->rom[addr++] = 0x00;
	gbs->rom[addr++] = 0x40;
	gbs->rom[addr++] = 0xf0; /* ldh a, (0x80) */
	gbs->rom[addr++] = 0x80;
	gbs->rom[addr++] = 0x3c; /* inc a */
	gbs->rom[addr++] = 0xc3; /* jp @loop */
	gbs->rom[addr++] = jpaddr & 0xff;
	gbs->rom[addr++] = jpaddr >> 8;

	gbs->buf_owned = 1;
	return gbs;
}

static struct gbs* gbs_open_internal(const char* const name, char* const buf, size_t size)
{
	struct gbs* const gbs = gbs_new(buf);
	long i, addr, jpaddr;

	UNUSED(name);

	gbs->version = buf[0x03];
	if (gbs->version != 1) {
		fprintf(stderr, _("GBS Version %d unsupported.\n"), gbs->version);
		gbs_free(gbs);
		return NULL;
	}

	gbs->songs = buf[0x04];
	if (gbs->songs < 1) {
		fprintf(stderr, _("Number of subsongs = %d is unreasonable.\n"), gbs->songs);
		gbs_free(gbs);
		return NULL;
	}

	gbs->defaultsong = buf[0x05];
	if (gbs->defaultsong < 1 || gbs->defaultsong > gbs->songs) {
		fprintf(stderr, _("Default subsong %d is out of range [1..%d].\n"), gbs->defaultsong, gbs->songs);
		gbs_free(gbs);
		return NULL;
	}

	gbs->load  = readint(&buf[0x06], 2);
	gbs->init  = readint(&buf[0x08], 2);
	gbs->play  = readint(&buf[0x0a], 2);
	gbs->stack = readint(&buf[0x0c], 2);
	gbs->tma = buf[0x0e];
	gbs->tac = buf[0x0f];

	memcpy(gbs->v1strings, &buf[0x10], 32);
	memcpy(gbs->v1strings+33, &buf[0x30], 32);
	memcpy(gbs->v1strings+66, &buf[0x50], 32);
	gbs->title = gbs->v1strings;
	gbs->author = gbs->v1strings+33;
	gbs->copyright = gbs->v1strings+66;
	gbs->code = &buf[0x70];
	gbs->filesize = size;

	gbs->subsong_info = calloc(sizeof(struct gbs_subsong_info), gbs->songs);
	gbs->codelen = size - HDR_LEN_GBS;
	gbs->crcnow = gbs_crc32(0, buf, gbs->filesize);

	gbs->romsize = (gbs->codelen + gbs->load + 0x3fff) & ~0x3fff;

	gbs->rom = calloc(1, gbs->romsize);
	memcpy(&gbs->rom[gbs->load], gbs->code, gbs->codelen);

	gbs->mapper = mapper_gbs(&gbs->gbhw.gbcpu, gbs->rom, gbs->romsize);

	for (i=0; i<8; i++) {
		long addr = gbs->load + 8*i; /* jump address */
		gbs->rom[8*i]   = 0xc3; /* jp imm16 */
		gbs->rom[8*i+1] = addr & 0xff;
		gbs->rom[8*i+2] = addr >> 8;
	}
	if ((gbs->tac & 0x78) == 0x40) { /* ugetab int vector extension */
		/* V-Blank */
		gbs->rom[0x40] = 0xcd; /* call imm16 */
		gbs->rom[0x41] = (gbs->load + 0x40) & 0xff;
		gbs->rom[0x42] = (gbs->load + 0x40) >> 8;
		/*
		 * ugetab is not well-spec'd, it is unclear where you
		 * have to call play from. Calling it from vblank
		 * seems to work.
		 */
		gbs->rom[0x43] = 0xcd; /* call imm16 */
		gbs->rom[0x44] = gbs->play & 0xff;
		gbs->rom[0x45] = gbs->play >> 8;
		gbs->rom[0x46] = 0xd9; /* reti */
		/* Timer */
		gbs->rom[0x50] = 0xcd; /* call imm16 */
		gbs->rom[0x51] = (gbs->load + 0x48) & 0xff;
		gbs->rom[0x52] = (gbs->load + 0x48) >> 8;
		gbs->rom[0x53] = 0xd9; /* reti */
	} else if (gbs->tac & 0x04) { /* timer enabled */
		/* V-Blank */
		gbs->rom[0x40] = 0xd9; /* reti */
		/* Timer */
		gbs->rom[0x50] = 0xcd; /* call imm16 */
		gbs->rom[0x51] = gbs->play & 0xff;
		gbs->rom[0x52] = gbs->play >> 8;
		gbs->rom[0x53] = 0xd9; /* reti */
	} else {
		/* V-Blank */
		gbs->rom[0x40] = 0xcd; /* call imm16 */
		gbs->rom[0x41] = gbs->play & 0xff;
		gbs->rom[0x42] = gbs->play >> 8;
		gbs->rom[0x43] = 0xd9; /* reti */
		/* Timer */
		gbs->rom[0x50] = 0xd9; /* reti */
	}
	gbs->rom[0x48] = 0xd9; /* reti (LCD Stat) */
	gbs->rom[0x58] = 0xd9; /* reti (Serial) */
	gbs->rom[0x60] = 0xd9; /* reti (Joypad) */

	/* In case this is dumped as a ROM */
	gbs->rom[0x0100] = 0x00; /* nop */
	gbs->rom[0x0101] = 0xc3; /* jp */
	gbs->rom[0x0102] = 0x150 & 0xff;
	gbs->rom[0x0103] = 0x150 >> 8;

	addr = 0x150;
	gbs->rom[addr++] = 0x21;  /* LD hl */
	gbs->rom[addr++] = gbs->stack & 0xff;
	gbs->rom[addr++] = gbs->stack >> 8;
	gbs->rom[addr++] = 0xf9;  /* LD sp, hl */

	gbs->rom[addr++] = 0x3e;  /* LD a, imm8 */
	gbs->rom[addr++] = gbs->tma;
	gbs->rom[addr++] = 0xe0;  /* LDH (a8), A */
	gbs->rom[addr++] = 0x06;  /* TMA reg */

	gbs->rom[addr++] = 0x3e;  /* LD a, imm8 */
	gbs->rom[addr++] = gbs->tac;
	gbs->rom[addr++] = 0xe0;  /* LDH (a8), A */
	gbs->rom[addr++] = 0x07;  /* TAC reg */

	gbs->rom[addr++] = 0x26;  /* LD h, imm8 */
	gbs->rom[addr++] = 0x20;
	gbs->rom[addr++] = 0x36;  /* LD (HL), imm8 */
	gbs->rom[addr++] = gbs->defaultbank;

	/*
	 * Call init function while interrupts are still disabled,
	 * otherwise nightmode.gbs breaks. This is per spec:
	 * "PLAY - Begins after INIT process is complete"
	 */
	gbs->rom[addr++] = 0x3e; /* LD a, imm8 */
	gbs->rom[addr++] = 0x00; /* first song */
	gbs->rom[addr++] = 0xcd; /* call imm16 */
	gbs->rom[addr++] = gbs->init & 0xff;
	gbs->rom[addr++] = gbs->init >> 8;
	/* Enable interrupts now */
	gbs->rom[addr++] = 0x3e;  /* LD a, imm8 */
	gbs->rom[addr++] = 0x05;  /* enable vblank + timer */
	gbs->rom[addr++] = 0xe0;  /* LDH (a8), A */
	gbs->rom[addr++] = 0xff;  /* IE reg */

	jpaddr = addr;
	gbs->rom[addr++] = 0x76; /* halt */
	gbs->rom[addr++] = 0xc3; /* jp @loop */
	gbs->rom[addr++] = jpaddr & 0xff;
	gbs->rom[addr++] = jpaddr >> 8;

	if (gbs->load < addr) {
		fprintf(stderr, _("Load address %04x overlaps with replayer end %04x.\n"),
			gbs->load, addr);
		gbs_free(gbs);
		return NULL;
	}

	gbs->buf_owned = 1;
	return gbs;
}

static struct gbs* gbs_open_mem(const char* const name, char* const buf, size_t size);

#ifdef USE_ZLIB
static struct gbs *gzip_open(const char* const name, char* const buf, size_t size)
{
	struct gbs* gbs = NULL;
	int ret;
	char *out = malloc(GB_MAX_ROM_SIZE);
	z_stream strm;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = (Bytef*)buf;
	strm.avail_in = size;
	strm.next_out = (Bytef*)out;
	strm.avail_out = GB_MAX_ROM_SIZE;

	/* inflate with gzip auto-detect */
	ret = inflateInit2(&strm, 15|32);
	if (ret != Z_OK) {
		fprintf(stderr, _("Could not open %s: inflateInit2: %d\n"), name, ret);
		goto exit_free;
	}

	ret = inflate(&strm, Z_FINISH);
	if (ret != Z_STREAM_END) {
		fprintf(stderr, _("Could not open %s: inflate: %d\n"), name, ret);
		goto exit_free;
	}
	inflateEnd(&strm);
	gbs = gbs_open_mem(name, out, GB_MAX_ROM_SIZE - strm.avail_out);

exit_free:
	if (gbs == NULL || gbs->buf != out) {
		free(out);
	}
	return gbs;
}
#else
static struct gbs *gzip_open(const char* const name, char* const buf, size_t size)
{
	fprintf(stderr, _("Could not open %s: %s\n"), name, _("Not compiled with zlib support"));
	return NULL;
}
#endif

static struct gbs* gbs_open_mem(const char* const name, char* const buf, size_t size)
{
	if (size > HDR_LEN_GZIP && strncmp(buf, GZIP_MAGIC, 3) == 0) {
		return gzip_open(name, buf, size);
	}
	if (size > HDR_LEN_GBR && strncmp(buf, GBR_MAGIC, 4) == 0) {
		return gbr_open(name, buf, size);
	}
	if (size > HDR_LEN_VGM && strncmp(buf, VGM_MAGIC, 4) == 0) {
		return vgm_open(name, buf, size);
	}
	if (size > HDR_LEN_GBS && strncmp(buf, GBS_MAGIC, 3) == 0) {
		return gbs_open_internal(name, buf, size);
	}
	if (size > HDR_LEN_GB && gbs_crc32(0, &buf[0x104], 48) == 0x46195417) {
		return gb_open(name, buf, size);
	}
	fprintf(stderr, _("Not a GBS-File: %s\n"), name);
	return NULL;
}

struct gbs* gbs_open(const char* const name)
{
	struct gbs* gbs = NULL;
	FILE *f;
	struct stat st;
	char *buf;

	if ((f = fopen(name, "rb")) == NULL) {
		fprintf(stderr, _("Could not open %s: %s\n"), name, strerror(errno));
		return NULL;
	}
	if (fstat(fileno(f), &st) == -1) {
		fprintf(stderr, _("Could not stat %s: %s\n"), name, strerror(errno));
		goto exit_close;
	}
	if (st.st_size > GB_MAX_ROM_SIZE) {
		fprintf(stderr, _("Could not read %s: %s\n"), name, _("Bigger than allowed maximum (4MiB)"));
		goto exit_close;
	}
	buf = malloc(st.st_size);
	if (fread(buf, 1, st.st_size, f) != st.st_size) {
		fprintf(stderr, _("Could not read %s: %s\n"), name, strerror(errno));
		goto exit_free;
	}

	gbs = gbs_open_mem(name, buf, st.st_size);
	if (gbs != NULL) {
		gbs->status.songs = gbs->songs;
		gbs->status.defaultsong = gbs->defaultsong;
		gbs->status.subsong = gbs->defaultsong - 1;
	}

exit_free:
	if (gbs == NULL || gbs->buf != buf)
		free(buf);
exit_close:
	fclose(f);
	return gbs;
}

struct gbs_internal_api gbs_internal_api = {
	.version = GBS_VERSION,
	.get_bootrom = gbs_get_bootrom,
	.write_rom = gbs_write_rom,
	.print_info = gbs_print_info,
	.midi_note = gbs_midi_note,
};
