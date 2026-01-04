#include "neserr.h"
#include "handler.h"
#include "audiosys.h"
#include "songinfo.h"

#include "m_kss.h"

#include "device/s_psg.h"
#include "device/s_scc.h"
#include "device/s_sng.h"
#include "device/opl/s_opl.h"
#include "device/divfix.h"

#include "wkmz80/wkmz80.h"

#include <stdio.h>

#define SHIFT_CPS 15
#define BASECYCLES       (3579545)

#define EXTDEVICE_SNG		(1 << 1)

#define EXTDEVICE_FMUNIT		(1 << 0)
#define EXTDEVICE_GGSTEREO	(1 << 2)
#define EXTDEVICE_GGRAM		(1 << 3)

#define EXTDEVICE_MSXMUSIC	(1 << 0)
#define EXTDEVICE_MSXRAM	(1 << 2)
#define EXTDEVICE_MSXAUDIO	(1 << 3)
#define EXTDEVICE_MSXSTEREO	(1 << 4)

#define EXTDEVICE_PAL		(1 << 6)
#define EXTDEVICE_EXRAM		(1 << 7)

#define SND_PSG 0
#define SND_SCC 1
#define SND_MSXMUSIC 2
#define SND_MSXAUDIO 3
#define SND_MAX 4

#define SND_SNG 0
#define SND_FMUNIT 2

#define SND_VOLUME 0x800


typedef struct KSSSEQ_TAG KSSSEQ;

struct  KSSSEQ_TAG {
	KMZ80_CONTEXT ctx;
	KMIF_SOUND_DEVICE *sndp[SND_MAX];
	Uint8 vola[SND_MAX];
	KMEVENT kme;
	KMEVENT_ITEM_ID vsync_id;

	Uint8 *readmap[8];
	Uint8 *writemap[8];

	Uint32 cps;		/* cycles per sample:fixed point */
	Uint32 cpsrem;	/* cycle remain */
	Uint32 cpsgap;	/* cycle gap */
	Uint32 cpf;		/* cycles per frame:fixed point */
	Uint32 cpfrem;	/* cycle remain */
	Uint32 total_cycles;	/* total played cycles */

	Uint32 startsong;
	Uint32 maxsong;

	Uint32 initaddr;
	Uint32 playaddr;

	Uint8 *data;
	Uint32 dataaddr;
	Uint32 datasize;
	Uint8 *bankbase;
	Uint32 bankofs;
	Uint32 banknum;
	Uint32 banksize;
	enum
	{
		KSS_BANK_OFF,
		KSS_16K_MAPPER,
		KSS_8K_MAPPER
	} bankmode;
	enum
	{
		SYNTHMODE_SMS,
		SYNTHMODE_MSX,
		SYNTHMODE_MSXSTEREO
	} synthmode;
	Uint8 sccenable;
	Uint8 rammode;
	Uint8 extdevice;
	Uint8 majutushimode;

	Uint8 ram[0x10000];
	Uint8 usertone_enable[2];
	Uint8 usertone[2][16 * 19];
};

Int32 MSXPSGType = 1;
Int32 MSXPSGVolume = 64;

static Uint32 GetWordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8);
}

static Uint32 GetDwordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

#define GetDwordLEM(p) (Uint32)((((Uint8 *)p)[0] | (((Uint8 *)p)[1] << 8) | (((Uint8 *)p)[2] << 16) | (((Uint8 *)p)[3] << 24)))

static Int32 execute(KSSSEQ *THIS_)
{
	Uint32 cycles;
	THIS_->cpsrem += THIS_->cps;
	cycles = THIS_->cpsrem >> SHIFT_CPS;
	if (THIS_->cpsgap >= cycles)
		THIS_->cpsgap -= cycles;
	else
	{
		Uint32 excycles = cycles - THIS_->cpsgap;
		THIS_->cpsgap = wkmz80_exec(&THIS_->ctx, excycles) - excycles;
	}
	THIS_->cpsrem &= (1 << SHIFT_CPS) - 1;
	THIS_->total_cycles += cycles;
	return 0;
}

__inline static void synth(KSSSEQ *THIS_, Int32 *d)
{
	switch (THIS_->synthmode)
	{
		default:
			NEVER_REACH
		case SYNTHMODE_SMS:
			THIS_->sndp[SND_SNG]->synth(THIS_->sndp[SND_SNG]->ctx, d);
			if (THIS_->sndp[SND_FMUNIT]) THIS_->sndp[SND_FMUNIT]->synth(THIS_->sndp[SND_FMUNIT]->ctx, d);
			break;
		case SYNTHMODE_MSX:
			{
				Int32 b[2];
				b[0] = b[1] = 0;
				THIS_->sndp[SND_PSG]->synth(THIS_->sndp[SND_PSG]->ctx, b);
				b[0] = b[0] * MSXPSGVolume / 64;
				d[0] += b[0];
				d[1] += b[0];
				THIS_->sndp[SND_SCC]->synth(THIS_->sndp[SND_SCC]->ctx, d);
				if (THIS_->sndp[SND_MSXMUSIC]) THIS_->sndp[SND_MSXMUSIC]->synth(THIS_->sndp[SND_MSXMUSIC]->ctx, d);
				if (THIS_->sndp[SND_MSXAUDIO]) THIS_->sndp[SND_MSXAUDIO]->synth(THIS_->sndp[SND_MSXAUDIO]->ctx, d);
			}
			break;
		case SYNTHMODE_MSXSTEREO:
			{
				Int32 b[3];
				b[0] = b[1] = b[2] = 0;
				THIS_->sndp[SND_PSG]->synth(THIS_->sndp[SND_PSG]->ctx, &b[0]);
				b[0] = b[1] = b[0] * MSXPSGVolume / 64;
				THIS_->sndp[SND_SCC]->synth(THIS_->sndp[SND_SCC]->ctx, &b[0]);
				if (THIS_->sndp[SND_MSXMUSIC]) THIS_->sndp[SND_MSXMUSIC]->synth(THIS_->sndp[SND_MSXMUSIC]->ctx, &b[0]);
				if (THIS_->sndp[SND_MSXAUDIO]) THIS_->sndp[SND_MSXAUDIO]->synth(THIS_->sndp[SND_MSXAUDIO]->ctx, &b[1]);
				d[0] += b[1];
				d[1] += b[2] + b[2];
			}
			break;
	}
}

__inline static void volume(KSSSEQ *THIS_, Uint32 v)
{
	v += 256;
	switch (THIS_->synthmode)
	{
		default:
			NEVER_REACH
		case SYNTHMODE_SMS:
			THIS_->sndp[SND_SNG]->volume(THIS_->sndp[SND_SNG]->ctx, v + (THIS_->vola[SND_SNG] << 4) - SND_VOLUME);
			if (THIS_->sndp[SND_FMUNIT]) THIS_->sndp[SND_FMUNIT]->volume(THIS_->sndp[SND_FMUNIT]->ctx, v + (THIS_->vola[SND_FMUNIT] << 4) - SND_VOLUME);
			break;
		case SYNTHMODE_MSX:
		case SYNTHMODE_MSXSTEREO:
			THIS_->sndp[SND_PSG]->volume(THIS_->sndp[SND_PSG]->ctx, v + (THIS_->vola[SND_PSG] << 4) - SND_VOLUME);
			THIS_->sndp[SND_SCC]->volume(THIS_->sndp[SND_SCC]->ctx, v + (THIS_->vola[SND_SCC] << 4) - SND_VOLUME);
			if (THIS_->sndp[SND_MSXMUSIC]) THIS_->sndp[SND_MSXMUSIC]->volume(THIS_->sndp[SND_MSXMUSIC]->ctx, v + (THIS_->vola[SND_MSXMUSIC] << 4) - SND_VOLUME);
			if (THIS_->sndp[SND_MSXAUDIO]) THIS_->sndp[SND_MSXAUDIO]->volume(THIS_->sndp[SND_MSXAUDIO]->ctx, v + (THIS_->vola[SND_MSXAUDIO] << 4) - SND_VOLUME);
			break;
	}
}

static void vsync_setup(KSSSEQ *THIS_)
{
	Int32 cycles;
	THIS_->cpfrem += THIS_->cpf;
	cycles = THIS_->cpfrem >> SHIFT_CPS;
	THIS_->cpfrem &= (1 << SHIFT_CPS) - 1;
	kmevent_settimer(&THIS_->kme, THIS_->vsync_id, cycles);
}

static void play_setup(KSSSEQ *THIS_, Uint32 pc)
{
	Uint32 sp = 0xF380, rp;
	THIS_->ram[--sp] = 0;
	THIS_->ram[--sp] = 0xfe;
	THIS_->ram[--sp] = 0x18;	/* JR +0 */
	THIS_->ram[--sp] = 0x76;	/* HALT */
	rp = sp;
	THIS_->ram[--sp] = (Uint8)(rp >> 8);
	THIS_->ram[--sp] = (Uint8)(rp & 0xff);
	THIS_->ctx.sp = sp;
	THIS_->ctx.pc = pc;
	THIS_->ctx.regs8[REGID_HALTED] = 0;
}

static Uint32 busread_event(void *ctx, Uint32 a)
{
	return 0x38;
}

static Uint32 read_event(void *ctx, Uint32 a)
{
	KSSSEQ *THIS_ = ctx;
	return THIS_->readmap[a >> 13][a];
}



static void KSSMaper8KWrite(KSSSEQ *THIS_, Uint32 a, Uint32 v)
{
	Uint32 page = a >> 13;
	v -= THIS_->bankofs;
	if (v < THIS_->banknum)
	{
		THIS_->readmap[page] = THIS_->bankbase + THIS_->banksize * v - a;
	}
	else
	{
		THIS_->readmap[page] = THIS_->ram;
	}
}

static void KSSMaper16KWrite(KSSSEQ *THIS_, Uint32 a, Uint32 v)
{
	Uint32 page = a >> 13;
	v -= THIS_->bankofs;
	if (v < THIS_->banknum)
	{
		THIS_->readmap[page] = THIS_->readmap[page + 1] = THIS_->bankbase + THIS_->banksize * v - a;
		if (!(THIS_->extdevice & EXTDEVICE_SNG)) THIS_->sccenable = 1;
	}
	else
	{
		THIS_->readmap[page] = THIS_->readmap[page + 1] = THIS_->ram;
		if (!(THIS_->extdevice & EXTDEVICE_SNG)) THIS_->sccenable = !THIS_->rammode;
	}
}

static void write_event(void *ctx, Uint32 a, Uint32 v)
{
	KSSSEQ *THIS_ = ctx;
	Uint32 page = a >> 13;
	if (THIS_->writemap[page])
	{
		THIS_->writemap[page][a] = (Uint8)(v & 0xff);
	}
	else if (THIS_->bankmode == KSS_8K_MAPPER)
	{
		if (a == 0x9000)
			KSSMaper8KWrite(THIS_, 0x8000, v);
		else if (a == 0xb000)
			KSSMaper8KWrite(THIS_, 0xa000, v);
		else if (THIS_->sccenable)
			THIS_->sndp[SND_SCC]->write(THIS_->sndp[SND_SCC]->ctx, a, v);
	}
	else
	{
		if (THIS_->sccenable)
			THIS_->sndp[SND_SCC]->write(THIS_->sndp[SND_SCC]->ctx, a, v);
		else if (THIS_->rammode)
			THIS_->ram[a] = (Uint8)(v & 0xff);
	}
}

static Uint32 ioread_eventSMS(void *ctx, Uint32 a)
{
	return 0xff;
}
static Uint32 ioread_eventMSX(void *ctx, Uint32 a)
{
	KSSSEQ *THIS_ = ctx;
	a &= 0xff;
	if (a == 0xa2)
		return THIS_->sndp[SND_PSG]->read(THIS_->sndp[SND_PSG]->ctx, 0);
	if (THIS_->sndp[SND_MSXAUDIO] && (a & 0xfe) == 0xc0)
		return THIS_->sndp[SND_MSXAUDIO]->read(THIS_->sndp[SND_MSXAUDIO]->ctx, a & 1);
	return 0xff;
}

static void iowrite_eventSMS(void *ctx, Uint32 a, Uint32 v)
{
	KSSSEQ *THIS_ = ctx;
	a &= 0xff;
	if ((a & 0xfe) == 0x7e)
		THIS_->sndp[SND_SNG]->write(THIS_->sndp[SND_SNG]->ctx, 0, v);
	else if (a == 0x06)
		THIS_->sndp[SND_SNG]->write(THIS_->sndp[SND_SNG]->ctx, 1, v);
	else if (THIS_->sndp[SND_FMUNIT] && (a & 0xfe) == 0xf0)
		THIS_->sndp[SND_FMUNIT]->write(THIS_->sndp[SND_FMUNIT]->ctx, a & 1, v);
	else if (a == 0xfe && THIS_->bankmode == KSS_16K_MAPPER)
		KSSMaper16KWrite(THIS_, 0x8000, v);
}
static void iowrite_eventMSX(void *ctx, Uint32 a, Uint32 v)
{
	KSSSEQ *THIS_ = ctx;
	a &= 0xff;
	if ((a & 0xfe) == 0xa0)
		THIS_->sndp[SND_PSG]->write(THIS_->sndp[SND_PSG]->ctx, a & 1, v);
	else if (a == 0xab)
		THIS_->sndp[SND_PSG]->write(THIS_->sndp[SND_PSG]->ctx, 2, v & 1);
	else if (THIS_->sndp[SND_MSXMUSIC] && (a & 0xfe) == 0x7c)
		THIS_->sndp[SND_MSXMUSIC]->write(THIS_->sndp[SND_MSXMUSIC]->ctx, a & 1, v);
	else if (THIS_->sndp[SND_MSXAUDIO] && (a & 0xfe) == 0xc0)
		THIS_->sndp[SND_MSXAUDIO]->write(THIS_->sndp[SND_MSXAUDIO]->ctx, a & 1, v);
	else if (a == 0xfe && THIS_->bankmode == KSS_16K_MAPPER)
		KSSMaper16KWrite(THIS_, 0x8000, v);
}

static void vsync_event(KMEVENT *event, KMEVENT_ITEM_ID curid, KSSSEQ *THIS_)
{
	vsync_setup(THIS_);
	if (THIS_->ctx.regs8[REGID_HALTED]) play_setup(THIS_, THIS_->playaddr);
}

//Memory viewer settings from here
static Uint32 (*memview_memread)(Uint32 a);
static KSSSEQ* memview_context;
static int MEM_MAX,MEM_IO,MEM_RAM,MEM_ROM;
Uint32 memview_memread_kss(Uint32 a){
	return read_event(memview_context,a);
}
//Memory viewer settings up to here

//Dump settings from here
static NEZ_PLAY *pNezPlayDump;
static Uint32 (*dump_MEM_MSX)(Uint32 a,unsigned char* mem);
static Uint32 dump_MEM_MSX_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Memory
		for(i=0;i<0x10000;i++)
			mem[i] = memview_memread_kss(i);
		return i;
	}
	return -2;
}
//----------
extern Uint32 (*ioview_ioread_DEV_AY8910)(Uint32 a);

static Uint32 (*dump_DEV_AY8910)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_AY8910_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register
		for(i=0;i<0x0e;i++)
			mem[i] = ioview_ioread_DEV_AY8910(i);
		return i;

	}
	return -2;
}
//----------
extern Uint32 (*ioview_ioread_DEV_SN76489)(Uint32 a);

static Uint32 (*dump_DEV_SN76489)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_SN76489_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register
		for(i=0;i<0x10;i++)
			mem[i] = ioview_ioread_DEV_SN76489(i);
		return i;

	}
	return -2;
}
static Uint32 dump_DEV_SN76489_bf2(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register
		for(i=0;i<0x11;i++)
			mem[i] = ioview_ioread_DEV_SN76489(i);
		return i;

	}
	return -2;
}
//----------
extern Uint32 (*ioview_ioread_DEV_SCC)(Uint32 a);

Uint32 (*dump_DEV_SCC)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_SCC_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register
		for(i=0;i<0x10;i++)
			mem[i] = ioview_ioread_DEV_SCC(i+0xb0);
		return i;
	case 2://Wave Data - CH1
	case 3://Wave Data - CH2
	case 4://Wave Data - CH3
	case 5://Wave Data - CH4
	case 6://Wave Data - CH5
		for(i=0;i<0x20;i++)
			mem[i] = ioview_ioread_DEV_SCC(i+(menu-2)*0x20);
		return i;

	}
	return -2;
}
//----------
extern Uint32 (*ioview_ioread_DEV_OPL)(Uint32 a);

Uint32 (*dump_DEV_OPL)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_OPL_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register
		for(i=0;i<0x100;i++)
			mem[i] = ioview_ioread_DEV_OPL(i);
		return i;

	}
	return -2;
}
//----------
extern Uint32 (*ioview_ioread_DEV_OPLL)(Uint32 a);

static Uint32 (*dump_DEV_OPLL)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_OPLL_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register
		for(i=0;i<0x40;i++)
			mem[i] = ioview_ioread_DEV_OPLL(i);
		return i;

	}
	return -2;
}
//----------
extern Uint32 (*ioview_ioread_DEV_ADPCM)(Uint32 a);
extern Uint32 (*ioview_ioread_DEV_ADPCM2)(Uint32 a);

static Uint32 (*dump_DEV_ADPCM)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_ADPCM_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register
		for(i=0;i<0xc;i++)
			mem[i] = ioview_ioread_DEV_ADPCM(i);
		return i;
	case 3://Memory
		for(i=0;i<0x10000;i++){
			if(ioview_ioread_DEV_ADPCM2(i)==0x100)break;
			mem[i] = ioview_ioread_DEV_ADPCM2(i);
		}
		return i;

	}
	return -2;
}
//----------
#ifdef EMSCRIPTEN
Uint8 **nez_get_RAM_buffer(NEZ_PLAY *pNezPlay)
{
	return &((KSSSEQ*)(pNezPlay->kssseq))->ram[0];
}
#endif

static void reset(NEZ_PLAY *pNezPlay)
{
	KSSSEQ *THIS_ = pNezPlay->kssseq;
	Uint32 i, freq, song;

	freq = NESAudioFrequencyGet(pNezPlay);
	song = SONGINFO_GetSongNo(pNezPlay->song) - 1;
	if (song >= THIS_->maxsong) song = THIS_->startsong - 1;

	/* sound reset */
	if (THIS_->extdevice & EXTDEVICE_SNG)
	{
		if (THIS_->sndp[SND_FMUNIT] && THIS_->usertone_enable[1]) THIS_->sndp[SND_FMUNIT]->setinst(THIS_->sndp[SND_FMUNIT]->ctx, 0, THIS_->usertone[1], 16 * 19);
	}
	else
	{
		if (THIS_->sndp[SND_MSXMUSIC] && THIS_->usertone_enable[0]) THIS_->sndp[SND_MSXMUSIC]->setinst(THIS_->sndp[SND_MSXMUSIC]->ctx, 0, THIS_->usertone[0], 16 * 19);
	}
	for (i = 0; i < SND_MAX; i++)
	{
		if (THIS_->sndp[i]) THIS_->sndp[i]->reset(THIS_->sndp[i]->ctx, BASECYCLES, freq);
	}

	/* memory reset */
	XMEMSET(THIS_->ram, 0xC9, 0x4000);
	XMEMCPY(THIS_->ram + 0x93, "\xC3\x01\x00\xC3\x09\x00", 6);
	XMEMCPY(THIS_->ram + 0x01, "\xD3\xA0\xF5\x7B\xD3\xA1\xF1\xC9", 8);
	XMEMCPY(THIS_->ram + 0x09, "\xD3\xA0\xDB\xA2\xC9", 5);
	XMEMSET(THIS_->ram + 0x4000, 0, 0xC000);
	if (THIS_->datasize)
	{
		if (THIS_->dataaddr + THIS_->datasize > 0x10000)
			XMEMCPY(THIS_->ram + THIS_->dataaddr, THIS_->data, 0x10000 - THIS_->dataaddr);
		else
			XMEMCPY(THIS_->ram + THIS_->dataaddr, THIS_->data, THIS_->datasize);
	}
	for (i = 0; i < 8; i++)
	{
		THIS_->readmap[i] = THIS_->ram;
		THIS_->writemap[i] = THIS_->ram;
	}
	THIS_->writemap[4] = THIS_->writemap[5] = 0;
	if (THIS_->majutushimode)
	{
		THIS_->writemap[2] = 0;
		THIS_->writemap[3] = 0;
	}
	if (!(THIS_->extdevice & EXTDEVICE_SNG)) THIS_->sccenable = !THIS_->rammode;

	/* cpu reset */
	wkmz80_reset(&THIS_->ctx);
	THIS_->ctx.user = THIS_;
	THIS_->ctx.kmevent = &THIS_->kme;
	THIS_->ctx.memread = read_event;
	THIS_->ctx.memwrite = write_event;
	if (THIS_->extdevice & EXTDEVICE_SNG)
	{
		THIS_->ctx.ioread = ioread_eventSMS;
		THIS_->ctx.iowrite = iowrite_eventSMS;
		THIS_->ctx.regs8[REGID_M1CYCLE] = 1;
	}
	else
	{
		THIS_->ctx.ioread = ioread_eventMSX;
		THIS_->ctx.iowrite = iowrite_eventMSX;
		THIS_->ctx.regs8[REGID_M1CYCLE] = 2;
	}
	THIS_->ctx.busread = busread_event;

	THIS_->ctx.regs8[REGID_A] = (Uint8)((song >> 0) & 0xff);
	THIS_->ctx.regs8[REGID_H] = (Uint8)((song >> 8) & 0xff);
	play_setup(THIS_, THIS_->initaddr);

	THIS_->ctx.exflag = 3;	/* ICE/ACI */
	THIS_->ctx.regs8[REGID_IFF1] = THIS_->ctx.regs8[REGID_IFF2] = 0;
	THIS_->ctx.regs8[REGID_INTREQ] = 0;
	THIS_->ctx.regs8[REGID_IMODE] = 1;

	/* vsync reset */
	kmevent_init(&THIS_->kme);
	THIS_->vsync_id = kmevent_alloc(&THIS_->kme);
	kmevent_setevent(&THIS_->kme, THIS_->vsync_id, vsync_event, THIS_);
	THIS_->cpsgap = THIS_->cpsrem  = 0;
	THIS_->cpfrem  = 0;
	THIS_->cps = DivFix(BASECYCLES, freq, SHIFT_CPS);
	if (THIS_->extdevice & EXTDEVICE_PAL)
		THIS_->cpf = (4 * 342 * 313 / 6) << SHIFT_CPS;
	else
		THIS_->cpf = (4 * 342 * 262 / 6) << SHIFT_CPS;

	{
		/* request execute */
		Uint32 initbreak = 5 << 8; /* 5sec */
		while (!THIS_->ctx.regs8[REGID_HALTED] && --initbreak) wkmz80_exec(&THIS_->ctx, BASECYCLES >> 8);
	}
	vsync_setup(THIS_);
	if (THIS_->ctx.regs8[REGID_HALTED])
	{
		play_setup(THIS_, THIS_->playaddr);
	}
	THIS_->total_cycles = 0;

	//Memory viewer settings from here
	memview_context = THIS_;
	MEM_MAX=0xffff;
	MEM_IO =0x0000;
	MEM_RAM=0xC000;
	MEM_ROM=0x8000;
	memview_memread = memview_memread_kss;
	//Memory viewer settings up to here

}

static void terminate(KSSSEQ *THIS_)
{
	Uint32 i;

	//Dump settings from here
	dump_MEM_MSX     = NULL;
	dump_DEV_AY8910  = NULL;
	dump_DEV_SN76489 = NULL;
	dump_DEV_SCC     = NULL;
	dump_DEV_OPL     = NULL;
	dump_DEV_OPLL    = NULL;
	dump_DEV_ADPCM   = NULL;
	//Dump settings up to here
	for (i = 0; i < SND_MAX; i++)
	{
		if (THIS_->sndp[i]) THIS_->sndp[i]->release(THIS_->sndp[i]->ctx);
	}
	if (THIS_->data) XFREE(THIS_->data);
	XFREE(THIS_);
}

static struct {
	char* title;
	char* artist;
	char* copyright;
	char detail[1024];
}songinfodata;

static Uint32 load(NEZ_PLAY *pNezPlay, KSSSEQ *THIS_, Uint8 *pData, Uint32 uSize)
{
	Uint32 i, headersize;
	XMEMSET(THIS_, 0, sizeof(KSSSEQ));
	for (i = 0; i < SND_MAX; i++) THIS_->sndp[i] = 0;
	for (i = 0; i < SND_MAX; i++) THIS_->vola[i] = 0x80;
	THIS_->data = 0;

	if (GetDwordLE(pData) == GetDwordLEM("KSCC"))
	{
		headersize = 0x10;
		THIS_->startsong = 1;
		THIS_->maxsong = 256;
	}
	else if (GetDwordLE(pData) == GetDwordLEM("KSSX"))
	{
		headersize = 0x10 + pData[0x0e];
		if (pData[0x0e] >= 0x0c)
		{
			THIS_->startsong = GetWordLE(pData + 0x18) + 1;
			THIS_->maxsong = GetWordLE(pData + 0x1a) + 1;
		}
		else
		{
			THIS_->startsong = 1;
			THIS_->maxsong = 256;
		}
		if (pData[0x0e] >= 0x10)
		{
			THIS_->vola[SND_PSG] = 0x100 - (pData[0x1c] ^ 0x80);
			THIS_->vola[SND_SCC] = 0x100 - (pData[0x1d] ^ 0x80);
			THIS_->vola[SND_MSXMUSIC] = 0x100 - (pData[0x1e] ^ 0x80);
			THIS_->vola[SND_MSXAUDIO] = 0x100 - (pData[0x1f] ^ 0x80);
		}
	}
	else
	{
		return NESERR_FORMAT;
	}
	THIS_->dataaddr = GetWordLE(pData + 0x04);
	THIS_->datasize = GetWordLE(pData + 0x06);
	THIS_->initaddr = GetWordLE(pData + 0x08);
	THIS_->playaddr = GetWordLE(pData + 0x0A);
	THIS_->bankofs = pData[0x0C];
	THIS_->banknum = pData[0x0D];
	THIS_->extdevice = pData[0x0F];
	SONGINFO_SetStartSongNo(pNezPlay->song, THIS_->startsong);
	SONGINFO_SetMaxSongNo(pNezPlay->song, THIS_->maxsong);
	SONGINFO_SetInitAddress(pNezPlay->song, THIS_->initaddr);
	SONGINFO_SetPlayAddress(pNezPlay->song, THIS_->playaddr);
	SONGINFO_SetExtendDevice(pNezPlay->song, THIS_->extdevice << 8);

	sprintf(songinfodata.detail,
#ifdef EMSCRIPTEN
"Type         : KS%c%c<br>\r\n\
Load Address : %04XH<br>\r\n\
Load Size    : %04XH<br>\r\n\
Init Address : %04XH<br>\r\n\
Play Address : %04XH<br>\r\n\
Extra Device : %s%s%s%s%s"
#else
"Type         : KS%c%c\r\n\
Load Address : %04XH\r\n\
Load Size    : %04XH\r\n\
Init Address : %04XH\r\n\
Play Address : %04XH\r\n\
Extra Device : %s%s%s%s%s"
#endif
		,pData[0x02],pData[0x03]
		,THIS_->dataaddr,THIS_->datasize,THIS_->initaddr,THIS_->playaddr
		,pData[0x0F]&0x01 ? (pData[0x0F]&0x02 ? "FMUNIT " : "FMPAC ") : ""
		,pData[0x0F]&0x02 ? "SN76489 " : ""
		,pData[0x0F]&0x04 ? (pData[0x0F]&0x02 ? "GGstereo " : "RAM ") : ""
		,pData[0x0F]&0x08 ? (pData[0x0F]&0x02 ? "RAM " : "MSX-AUDIO ") : ""
		,pData[0x0F] ? "" : "None"
	);
	SONGINFO_SetDetail(pNezPlay->song, songinfodata.detail);	// EMSCRIPTEN
	
	if (THIS_->banknum & 0x80)
	{
		THIS_->banknum &= 0x7f;
		THIS_->bankmode = KSS_8K_MAPPER;
		THIS_->banksize = 0x2000;
	}
	else if (THIS_->banknum)
	{
		THIS_->bankmode = KSS_16K_MAPPER;
		THIS_->banksize = 0x4000;
	}
	else
	{
		THIS_->bankmode = KSS_BANK_OFF;
	}
	THIS_->data = XMALLOC(THIS_->datasize + THIS_->banksize * THIS_->banknum + 8);
	if (!THIS_->data) return NESERR_SHORTOFMEMORY;
	THIS_->bankbase = THIS_->data + THIS_->datasize;
	if (uSize > 0x10 && THIS_->datasize)
	{
		Uint32 size;
		XMEMSET(THIS_->data, 0, THIS_->datasize);
		if (THIS_->datasize < uSize - headersize)
			size = THIS_->datasize;
		else
			size = uSize - headersize;
		XMEMCPY(THIS_->data, pData + headersize, size);
	}
	else
	{
		THIS_->datasize = 0;
	}
	if (uSize > (headersize + THIS_->datasize) && THIS_->bankmode != KSS_BANK_OFF)
	{
		Uint32 size;
		XMEMSET(THIS_->bankbase, 0, THIS_->banksize * THIS_->banknum);
		if (THIS_->banksize * THIS_->banknum < uSize - (headersize + THIS_->datasize))
			size = THIS_->banksize * THIS_->banknum;
		else
			size = uSize - (headersize + THIS_->datasize);
		XMEMCPY(THIS_->bankbase, pData + (headersize + THIS_->datasize), size);
	}
	else
	{
		THIS_->banknum = 0;
		THIS_->bankmode = KSS_BANK_OFF;
	}

	//Dump settings from here
	dump_MEM_MSX     = dump_MEM_MSX_bf;
	//Dump settings up to here


	THIS_->majutushimode = 0;
	if (THIS_->extdevice & EXTDEVICE_SNG)
	{
		THIS_->synthmode = SYNTHMODE_SMS;
		if (THIS_->extdevice & EXTDEVICE_GGSTEREO)
		{
			THIS_->sndp[SND_SNG] = SNGSoundAlloc(SNG_TYPE_GAMEGEAR);
			SONGINFO_SetChannel(pNezPlay->song, 2);
			//Dump settings from here
			dump_DEV_SN76489 = dump_DEV_SN76489_bf2;
			//Dump settings up to here
		}
		else
		{
			THIS_->sndp[SND_SNG] = SNGSoundAlloc(SNG_TYPE_SEGAMKIII);
			SONGINFO_SetChannel(pNezPlay->song, 1);
			//Dump settings from here
			dump_DEV_SN76489 = dump_DEV_SN76489_bf;
			//Dump settings up to here
		}
		if (!THIS_->sndp[SND_SNG]) return NESERR_SHORTOFMEMORY;
		if (THIS_->extdevice & EXTDEVICE_FMUNIT)
		{
			THIS_->sndp[SND_FMUNIT] = OPLSoundAlloc(OPL_TYPE_SMSFMUNIT);
			if (!THIS_->sndp[SND_FMUNIT]) return NESERR_SHORTOFMEMORY;
			//Dump settings from here
			dump_DEV_OPLL = dump_DEV_OPLL_bf;
			//Dump settings up to here
		}
		if (THIS_->extdevice & (EXTDEVICE_EXRAM | EXTDEVICE_GGRAM))
		{
			THIS_->rammode = 1;
		}
		else
		{
			THIS_->rammode = 0;
		}
		THIS_->sccenable = 0;
	}
	else
	{
		if (THIS_->extdevice & EXTDEVICE_MSXSTEREO)
		{
			if (THIS_->extdevice & EXTDEVICE_MSXAUDIO)
			{
				SONGINFO_SetChannel(pNezPlay->song, 2);
				THIS_->synthmode = SYNTHMODE_MSXSTEREO;
			}
			else
			{
				SONGINFO_SetChannel(pNezPlay->song, 1);
				THIS_->synthmode = SYNTHMODE_MSX;
				THIS_->majutushimode = 1;
			}
		}
		else
		{
			SONGINFO_SetChannel(pNezPlay->song, 1);
			THIS_->synthmode = SYNTHMODE_MSX;
		}
		if(MSXPSGType){
			THIS_->sndp[SND_PSG] = PSGSoundAlloc(PSG_TYPE_YM2149);
		}else{
			THIS_->sndp[SND_PSG] = PSGSoundAlloc(PSG_TYPE_AY_3_8910);
		}
		if (!THIS_->sndp[SND_PSG]) return NESERR_SHORTOFMEMORY;
		//Dump settings from here
		dump_DEV_AY8910 = dump_DEV_AY8910_bf;
		//Dump settings up to here
		if (THIS_->extdevice & EXTDEVICE_MSXMUSIC)
		{
			THIS_->sndp[SND_MSXMUSIC] = OPLSoundAlloc(OPL_TYPE_MSXMUSIC);
			if (!THIS_->sndp[SND_MSXMUSIC]) return NESERR_SHORTOFMEMORY;
			//Dump settings from here
			dump_DEV_OPLL = dump_DEV_OPLL_bf;
			//Dump settings up to here
		}
		if (THIS_->extdevice & EXTDEVICE_MSXAUDIO)
		{
			THIS_->sndp[SND_MSXAUDIO] = OPLSoundAlloc(OPL_TYPE_MSXAUDIO);
			//THIS_->sndp[SND_MSXAUDIO] = OPLSoundAlloc(OPL_TYPE_OPL2);
			if (!THIS_->sndp[SND_MSXAUDIO]) return NESERR_SHORTOFMEMORY;
			//Dump settings from here
			dump_DEV_OPL = dump_DEV_OPL_bf;
			dump_DEV_ADPCM = dump_DEV_ADPCM_bf;
			//Dump settings up to here
		}
		if (THIS_->extdevice & EXTDEVICE_EXRAM)
		{
			THIS_->rammode = 1;
			THIS_->sccenable = 0;
		}
		else
		{
			THIS_->sndp[SND_SCC] = SCCSoundAlloc();
			if (!THIS_->sndp[SND_SCC]) return NESERR_SHORTOFMEMORY;
			//Dump settings from here
			dump_DEV_SCC = dump_DEV_SCC_bf;
			//Dump settings up to here
			THIS_->rammode = (THIS_->extdevice & EXTDEVICE_MSXRAM) != 0;
			THIS_->sccenable = !THIS_->rammode;
		}
	}
	return NESERR_NOERROR;
}

static Int32 __fastcall KSSSEQExecuteZ80CPU(void *pNezPlay)
{
	execute(((NEZ_PLAY*)pNezPlay)->kssseq);
	return 0;
}

static void __fastcall KSSSEQSoundRenderStereo(void *pNezPlay, Int32 *d)
{
	synth(((NEZ_PLAY*)pNezPlay)->kssseq, d);
}

static Int32 __fastcall KSSSEQSoundRenderMono(void *pNezPlay)
{
	Int32 d[2] = { 0, 0 };
	synth(((NEZ_PLAY*)pNezPlay)->kssseq, d);
#if (((-1) >> 1) == -1)
	return (d[0] + d[1]) >> 1;
#else
	return (d[0] + d[1]) / 2;
#endif
}

const static NES_AUDIO_HANDLER kssseq_audio_handler[] = {
	{ 0, KSSSEQExecuteZ80CPU, 0, },
	{ 3, KSSSEQSoundRenderMono, KSSSEQSoundRenderStereo },
	{ 0, 0, 0, },
};

static void __fastcall KSSSEQVolume(void *pNezPlay, Uint32 v)
{
	if (((NEZ_PLAY*)pNezPlay)->kssseq)
	{
		volume(((NEZ_PLAY*)pNezPlay)->kssseq, v);
	}
}

const static NES_VOLUME_HANDLER kssseq_volume_handler[] = {
	{ KSSSEQVolume, }, 
	{ 0, }, 
};

static void __fastcall KSSSEQReset(void *pNezPlay)
{
	if (((NEZ_PLAY*)pNezPlay)->kssseq) reset((NEZ_PLAY*)pNezPlay);
}

const static NES_RESET_HANDLER kssseq_reset_handler[] = {
	{ NES_RESET_SYS_LAST, KSSSEQReset, },
	{ 0,                  0, },
};

static void __fastcall KSSSEQTerminate(void *pNezPlay)
{
	if (((NEZ_PLAY*)pNezPlay)->kssseq)
	{
		terminate(((NEZ_PLAY*)pNezPlay)->kssseq);
		((NEZ_PLAY*)pNezPlay)->kssseq = 0;
	}
}

const static NES_TERMINATE_HANDLER kssseq_terminate_handler[] = {
	{ KSSSEQTerminate, },
	{ 0, },
};

Uint32 KSSLoad(NEZ_PLAY *pNezPlay, Uint8 *pData, Uint32 uSize)
{
	Uint32 ret;
	KSSSEQ *THIS_;

	THIS_ = XMALLOC(sizeof(KSSSEQ));
	if (!THIS_) return NESERR_SHORTOFMEMORY;
	ret = load(pNezPlay, THIS_, pData, uSize);
	if (ret)
	{
		terminate(THIS_);
		return ret;
	}
	pNezPlay->kssseq = THIS_;
	NESAudioHandlerInstall(pNezPlay, kssseq_audio_handler);
	NESVolumeHandlerInstall(pNezPlay, kssseq_volume_handler);
	NESResetHandlerInstall(pNezPlay->nrh, kssseq_reset_handler);
	NESTerminateHandlerInstall(&pNezPlay->nth, kssseq_terminate_handler);
	return ret;
}
