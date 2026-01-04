#include "neserr.h"
#include "handler.h"
#include "audiosys.h"
#include "songinfo.h"

#include "device/s_hes.h"
#include "device/s_hesad.h"
#include "device/divfix.h"

#include "m_hes.h"
#include <stdio.h>

/* -------------------- */
/*  km6502 HuC6280 I/F  */
/* -------------------- */
#define USE_DIRECT_ZEROPAGE 0
#define USE_CALLBACK 1
#define USE_INLINEMMC 0
#define USE_USERPOINTER 1
#define External __inline static
#include "wkmz80/wkmevent.h"
#include "km6502/km6280m.h"

#define SHIFT_CPS 15
#define HES_BASECYCLES (21477270)
#define HES_TIMERCYCLES (1024 * 3)

#if 0

HES
system clock 21477270Hz
CPUH clock 21477270Hz system clock 
CPUL clock 3579545 system clock / 6

FF		I/O
F9-FB	SGX-RAM
F8		RAM
F7		BATTERY RAM
80-87	CD-ROM^2 RAM
00-		ROM

#endif

typedef struct HESHES_TAG HESHES;
typedef Uint32 (*READPROC)(HESHES *THIS_, Uint32 a);
typedef void (*WRITEPROC)(HESHES *THIS_, Uint32 a, Uint32 v);

struct  HESHES_TAG {
	struct K6280_Context ctx;
	KMIF_SOUND_DEVICE *hessnd;
	KMIF_SOUND_DEVICE *hespcm;
	KMEVENT kme;
	KMEVENT_ITEM_ID vsync;
	KMEVENT_ITEM_ID timer;

	Uint32 bp;			/* break point */
	Uint32 breaked;		/* break point flag */

	Uint32 cps;				/* cycles per sample:fixed point */
	Uint32 cpsrem;			/* cycle remain */
	Uint32 cpsgap;			/* cycle gap */
	Uint32 total_cycles;	/* total played cycles */

	Uint8 mpr[0x8];
	Uint8 firstmpr[0x8];
	Uint8 *memmap[0x100];
	Uint32 initaddr;

	Uint32 playerromaddr;
	Uint8 playerrom[0x10];

	Uint8 hestim_RELOAD;	/* IO $C01 ($C00)*/
	Uint8 hestim_COUNTER;	/* IO $C00 */
	Uint8 hestim_START;		/* IO $C01 */
	Uint8 hesvdc_STATUS;
	Uint8 hesvdc_CR;
	Uint8 hesvdc_ADR;
};


static struct {
	char* title;
	char* artist;
	char* copyright;
	char detail[1024];
}songinfodata;


static Uint32 km6280_exec(struct K6280_Context *ctx, Uint32 cycles)
{
	HESHES *THIS_ = (HESHES *)ctx->user;
	Uint32 kmecycle;
	kmecycle = ctx->clock = 0;
	while (ctx->clock < cycles)
	{
#if 1
		if (!THIS_->breaked)
#else
		if (1)
#endif
		{
			K6280_Exec(ctx);	/* Execute 1op */
			if (ctx->PC == THIS_->bp)
			{
				if(((THIS_->ctx.iRequest)&(THIS_->ctx.iMask^0x3)&(K6280_INT1|K6280_TIMER))==0)
					THIS_->breaked = 1;
			}
		}
		else
		{
			Uint32 nextcount;
			/* break時は次のイベントまで一度に進める */
			nextcount = THIS_->kme.item[THIS_->kme.item[0].next].count;
			if (kmevent_gettimer(&THIS_->kme, 0, &nextcount))
			{
				/* イベント有り */
				if (ctx->clock + nextcount < cycles)
					ctx->clock += nextcount;	/* 期間中にイベント有り */
				else
					ctx->clock = cycles;		/* 期間中にイベント無し */
			}
			else
			{
				/* イベント無し */
				ctx->clock = cycles;
			}
		}
		/* イベント進行 */
		kmevent_process(&THIS_->kme, ctx->clock - kmecycle);
		kmecycle = ctx->clock;
	}
	ctx->clock = 0;
	return kmecycle;
}

static Int32 execute(HESHES *THIS_)
{
	Uint32 cycles;
	THIS_->cpsrem += THIS_->cps;
	cycles = THIS_->cpsrem >> SHIFT_CPS;
	if (THIS_->cpsgap >= cycles)
		THIS_->cpsgap -= cycles;
	else
	{
		Uint32 excycles = cycles - THIS_->cpsgap;
		THIS_->cpsgap = km6280_exec(&THIS_->ctx, excycles) - excycles;
	}
	THIS_->cpsrem &= (1 << SHIFT_CPS) - 1;
	THIS_->total_cycles += cycles;
	return 0;
}

__inline static void synth(HESHES *THIS_, Int32 *d)
{
	THIS_->hessnd->synth(THIS_->hessnd->ctx, d);
	THIS_->hespcm->synth(THIS_->hespcm->ctx, d);
}

__inline static void volume(HESHES *THIS_, Uint32 v)
{
	THIS_->hessnd->volume(THIS_->hessnd->ctx, v);
	THIS_->hespcm->volume(THIS_->hespcm->ctx, v);
}


static void vsync_setup(HESHES *THIS_)
{
	kmevent_settimer(&THIS_->kme, THIS_->vsync, 4 * 342 * 262);
}

static void timer_setup(HESHES *THIS_)
{
	kmevent_settimer(&THIS_->kme, THIS_->timer, HES_TIMERCYCLES);
}

static void write_6270(HESHES *THIS_, Uint32 a, Uint32 v)
{
	switch (a)
	{
		case 0:
			THIS_->hesvdc_ADR = (Uint8)v;
			break;
		case 2:
			switch (THIS_->hesvdc_ADR)
			{
				case 5:	/* CR */
					THIS_->hesvdc_CR = (Uint8)v;
					break;
			}
			break;
		case 3:
			break;
	}
}

static Uint32 read_6270(HESHES *THIS_, Uint32 a)
{
	Uint32 v = 0;
	if (a == 0)
	{
		if (THIS_->hesvdc_STATUS)
		{
			THIS_->hesvdc_STATUS = 0;
			v = 0x20;
		}
		THIS_->ctx.iRequest &= ~K6280_INT1;
#if 0
		v = 0x20;	/* 常にVSYNC期間 */
#endif
	}
	return v;
}

static Uint32 read_io(HESHES *THIS_, Uint32 a)
{
	switch (a >> 10)
	{
		case 0:	/* VDC */	
			return read_6270(THIS_, a & 3);
		case 2:	/* PSG */
			return THIS_->hessnd->read(THIS_->hessnd->ctx, a & 0xf);
		case 3:	/* TIMER */
			if (a & 1)
				return THIS_->hestim_START;
			else
				return THIS_->hestim_COUNTER;
		case 5:	/* IRQ */
			switch (a & 15)
			{
				case 2:
					{
						Uint32 v = 0xf8;
						if (!(THIS_->ctx.iMask & K6280_TIMER)) v |= 4;
						if (!(THIS_->ctx.iMask & K6280_INT1)) v |= 2;
						if (!(THIS_->ctx.iMask & K6280_INT2)) v |= 1;
						return v;
					}
				case 3:
					{
						Uint8 v = 0;
						if (THIS_->ctx.iRequest & K6280_TIMER) v |= 4;
						if (THIS_->ctx.iRequest & K6280_INT1) v |= 2;
						if (THIS_->ctx.iRequest & K6280_INT2) v |= 1;
#if 0
						THIS_->ctx.iRequest &= ~(K6280_TIMER | K6280_INT1 | K6280_INT2);
#endif
						return v;
					}
			}
			return 0x00;
		case 7:
			a -= THIS_->playerromaddr;
			if (a < 0x10) return THIS_->playerrom[a];
			return 0xff;
		case 6:	/* CDROM */
			switch (a & 15)
			{
				case 0x0a:
				case 0x0b:
				case 0x0c:
				case 0x0d:
				case 0x0e://デバッグ用
				case 0x0f://デバッグ用
					return THIS_->hespcm->read(THIS_->hespcm->ctx, a & 0xf);
			}
		default:
		case 1:	/* VCE */
		case 4:	/* PAD */
			return 0xff;
	}
}

static void write_io(HESHES *THIS_, Uint32 a, Uint32 v)
{
	switch (a >> 10)
	{
		case 0:	/* VDC */
			write_6270(THIS_, a & 3, v);
			break;
		case 2:	/* PSG */
			THIS_->hessnd->write(THIS_->hessnd->ctx, a & 0xf, v);
			break;
		case 3:	/* TIMER */
			switch (a & 1)
			{
				case 0:
					THIS_->hestim_RELOAD = (Uint8)(v & 127);
					break;
				case 1:
					v &= 1;
					if (v && !THIS_->hestim_START)
						THIS_->hestim_COUNTER = THIS_->hestim_RELOAD;
					THIS_->hestim_START = (Uint8)v;
					break;
			}
			break;
		case 5:	/* IRQ */
			switch (a & 15)
			{
				case 2:
					THIS_->ctx.iMask &= ~(K6280_TIMER | K6280_INT1 | K6280_INT2);
					if (!(v & 4)) THIS_->ctx.iMask |= K6280_TIMER;
					if (!(v & 2)) THIS_->ctx.iMask |= K6280_INT1;
					if (!(v & 1)) THIS_->ctx.iMask |= K6280_INT2;
					break;
				case 3:
					THIS_->ctx.iRequest &= ~K6280_TIMER;
					break;
			}
			break;
		case 6:	/* CDROM */
			switch (a & 15)
			{
				case 0x08:
				case 0x09:
				case 0x0a:
				case 0x0b:
				case 0x0d:
				case 0x0e:
				case 0x0f:
					THIS_->hespcm->write(THIS_->hespcm->ctx, a & 0xf, v);
					break;
			}
			break;
		default:
		case 1:	/* VCE */
		case 4:	/* PAD */
		case 7:
			break;
			
	}
}


static Uint32 Callback read_event(void *ctx, Uword a)
{
	HESHES *THIS_ = ctx;
	Uint8 page = THIS_->mpr[a >> 13];
	if (THIS_->memmap[page])
		return THIS_->memmap[page][a & 0x1fff];
	else if (page == 0xff)
		return read_io(THIS_, a & 0x1fff);
	else
		return 0xff;
}

static void Callback write_event(void *ctx, Uword a, Uword v)
{
	HESHES *THIS_ = ctx;
	Uint8 page = THIS_->mpr[a >> 13];
	if (THIS_->memmap[page])
		THIS_->memmap[page][a & 0x1fff] = (Uint8)v;
	else if (page == 0xff)
		write_io(THIS_, a & 0x1fff, v);
}

static Uint32 Callback readmpr_event(void *ctx, Uword a)
{
	HESHES *THIS_ = ctx;
	Uint32 i;
	for (i = 0; i < 8; i++) if (a & (1 << i)) return THIS_->mpr[i];
	return 0xff;
}

static void Callback writempr_event(void *ctx, Uword a, Uword v)
{
	HESHES *THIS_ = ctx;
	Uint32 i;
	if (v < 0x80 && !THIS_->memmap[v]) return;
	for (i = 0; i < 8; i++) if (a & (1 << i)) THIS_->mpr[i] = (Uint8)v;
}

static void Callback write6270_event(void *ctx, Uword a, Uword v)
{
	HESHES *THIS_ = ctx;
	write_6270(THIS_, a & 0x1fff, v);
}


static void vsync_event(KMEVENT *event, KMEVENT_ITEM_ID curid, HESHES *THIS_)
{
	vsync_setup(THIS_);
	if (THIS_->hesvdc_CR & 8)
	{
		THIS_->ctx.iRequest |= K6280_INT1;
		THIS_->breaked = 0;
	}
	THIS_->hesvdc_STATUS = 1;
}

static void timer_event(KMEVENT *event, KMEVENT_ITEM_ID curid, HESHES *THIS_)
{
	if (THIS_->hestim_START && THIS_->hestim_COUNTER-- == 0)
	{
		THIS_->hestim_COUNTER = THIS_->hestim_RELOAD;
		THIS_->ctx.iRequest |= K6280_TIMER;
		THIS_->breaked = 0;
	}
	timer_setup(THIS_);
}

//ここからメモリービュアー設定
static Uint32 (*memview_memread)(Uint32 a);
static HESHES* memview_context;
static int MEM_MAX,MEM_IO,MEM_RAM,MEM_ROM;
Uint32 memview_memread_hes(Uint32 a){
	if(a>=0x1800&&a<0x1c00&&(a&0xf)==0xa)return 0xff;
	return read_event(memview_context,a);
}
//ここまでメモリービュアー設定

//ここからダンプ設定
static NEZ_PLAY *pNezPlayDump;
Uint32 (*dump_MEM_PCE)(Uint32 a,unsigned char* mem);
static Uint32 dump_MEM_PCE_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Memory
		for(i=0;i<0x10000;i++)
			mem[i] = memview_memread_hes(i);
		return i;
	}
	return -2;
}
//----------
extern Uint32 pce_ioview_ioread_bf(Uint32);

Uint32 (*dump_DEV_HUC6230)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_HUC6230_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register 1
		for(i=0;i<0x0a;i++)
			mem[i] = pce_ioview_ioread_bf(i);
		return i;

	case 2://Register 2
		for(i=0;i<0x60;i++)
			mem[i] = pce_ioview_ioread_bf(i+0x22);
		return i;

	case 3://Wave Data - CH1
	case 4://Wave Data - CH2
	case 5://Wave Data - CH3
	case 6://Wave Data - CH4
	case 7://Wave Data - CH5
	case 8://Wave Data - CH6
		for(i=0;i<0x20;i++)
			mem[i] = pce_ioview_ioread_bf(i+0x100+(menu-3)*0x20);
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
	case 1://Register 1
		for(i=0;i<0x8;i++)
			mem[i] = ioview_ioread_DEV_ADPCM(i+8);
		return i;
	case 2://Register 2[ADR/LEN]
		for(i=0;i<0x6;i++)
			mem[i] = ioview_ioread_DEV_ADPCM(i+0x10);
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

static void reset(NEZ_PLAY *pNezPlay)
{
	HESHES *THIS_ = pNezPlay->heshes;
	Uint32 i, initbreak;
	Uint32 freq = NESAudioFrequencyGet(pNezPlay);

	THIS_->hessnd->reset(THIS_->hessnd->ctx, HES_BASECYCLES, freq);
	THIS_->hespcm->reset(THIS_->hespcm->ctx, HES_BASECYCLES, freq);
	kmevent_init(&THIS_->kme);

	/* RAM CLEAR */
	for (i = 0xf8; i <= 0xfb; i++)
		if (THIS_->memmap[i]) XMEMSET(THIS_->memmap[i], 0, 0x2000);

	THIS_->cps = DivFix(HES_BASECYCLES, freq, SHIFT_CPS);
	THIS_->ctx.user = THIS_;
	THIS_->ctx.ReadByte = read_event;
	THIS_->ctx.WriteByte = write_event;
	THIS_->ctx.ReadMPR = readmpr_event;
	THIS_->ctx.WriteMPR = writempr_event;
	THIS_->ctx.Write6270 = write6270_event;

	THIS_->vsync = kmevent_alloc(&THIS_->kme);
	THIS_->timer = kmevent_alloc(&THIS_->kme);
	kmevent_setevent(&THIS_->kme, THIS_->vsync, vsync_event, THIS_);
	kmevent_setevent(&THIS_->kme, THIS_->timer, timer_event, THIS_);

	THIS_->bp = THIS_->playerromaddr + 3;
	for (i = 0; i < 8; i++) THIS_->mpr[i] = THIS_->firstmpr[i];

	THIS_->breaked = 0;
	THIS_->cpsrem = THIS_->cpsgap = THIS_->total_cycles = 0;

	THIS_->ctx.A = (SONGINFO_GetSongNo(pNezPlay->song) - 1) & 0xff;
	THIS_->ctx.P = K6280_Z_FLAG + K6280_I_FLAG;
	THIS_->ctx.X = THIS_->ctx.Y = 0;
	THIS_->ctx.S = 0xFF;
	THIS_->ctx.PC = THIS_->playerromaddr;
	THIS_->ctx.iRequest = 0;
	THIS_->ctx.iMask = ~0;
	THIS_->ctx.lowClockMode = 0;

	THIS_->playerrom[0x00] = 0x20;	/* JSR */
	THIS_->playerrom[0x01] = (Uint8)((THIS_->initaddr >> 0) & 0xff);
	THIS_->playerrom[0x02] = (Uint8)((THIS_->initaddr >> 8) & 0xff);
	THIS_->playerrom[0x03] = 0x4c;	/* JMP */
	THIS_->playerrom[0x04] = (Uint8)(((THIS_->playerromaddr + 3) >> 0) & 0xff);
	THIS_->playerrom[0x05] = (Uint8)(((THIS_->playerromaddr + 3) >> 8) & 0xff);

	THIS_->hesvdc_STATUS = 0;
	THIS_->hesvdc_CR = 0;
	THIS_->hesvdc_ADR = 0;
	vsync_setup(THIS_);
	THIS_->hestim_RELOAD = THIS_->hestim_COUNTER = THIS_->hestim_START = 0;
	timer_setup(THIS_);

	/* request execute(5sec) */
	initbreak = 5 << 8;
	while (!THIS_->breaked && --initbreak)
		km6280_exec(&THIS_->ctx, HES_BASECYCLES >> 8);

	if (THIS_->breaked)
	{
		THIS_->breaked = 0;
		THIS_->ctx.P &= ~K6280_I_FLAG;
	}

	THIS_->cpsrem = THIS_->cpsgap = THIS_->total_cycles = 0;

	//ここからメモリービュアー設定
	memview_context = THIS_;
	MEM_MAX=0xffff;
	MEM_IO =0x0000;
	MEM_RAM=0x2000;
	MEM_ROM=0x4000;
	memview_memread = memview_memread_hes;
	//ここまでメモリービュアー設定

	//ここからダンプ設定
	pNezPlayDump = pNezPlay;
	dump_MEM_PCE     = dump_MEM_PCE_bf;
	dump_DEV_HUC6230 = dump_DEV_HUC6230_bf;
	dump_DEV_ADPCM   = dump_DEV_ADPCM_bf;
	//ここまでダンプ設定

}

static void terminate(HESHES *THIS_)
{
	Uint32 i;

	//ここからダンプ設定
	dump_MEM_PCE     = NULL;
	dump_DEV_HUC6230 = NULL;
	dump_DEV_ADPCM   = NULL;
	//ここまでダンプ設定

	if (THIS_->hessnd) THIS_->hessnd->release(THIS_->hessnd->ctx);
	if (THIS_->hespcm) THIS_->hespcm->release(THIS_->hespcm->ctx);
	for (i = 0; i < 0x100; i++) if (THIS_->memmap[i]) XFREE(THIS_->memmap[i]);
	XFREE(THIS_);
}

static Uint32 GetWordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8);
}

static Uint32 GetDwordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static Uint32 alloc_physical_address(HESHES *THIS_, Uint32 a, Uint32 l)
{
	Uint8 page = (Uint8)(a >> 13);
	Uint8 lastpage = (Uint8)((a + l - 1) >> 13);
	for (; page <= lastpage; page++)
	{
		if (!THIS_->memmap[page])
		{
			THIS_->memmap[page] = (Uint8*)XMALLOC(0x2000);
			if (!THIS_->memmap[page]) return 0;
			XMEMSET(THIS_->memmap[page], 0, 0x2000);
		}
	}
	return 1;
}

static void copy_physical_address(HESHES *THIS_, Uint32 a, Uint32 l, Uint8 *p)
{
	Uint8 page = (Uint8)(a >> 13);
	Uint32 w;
	if (a & 0x1fff)
	{
		w = 0x2000 - (a & 0x1fff);
		if (w > l) w = l;
		XMEMCPY(THIS_->memmap[page++] + (a & 0x1fff), p, w);
		p += w;
		l -= w;
	}
	while (l)
	{
		w = (l > 0x2000) ? 0x2000 : l;
		XMEMCPY(THIS_->memmap[page++], p, w);
		p += w;
		l -= w;
	}
}


static Uint32 load(NEZ_PLAY *pNezPlay, HESHES *THIS_, Uint8 *pData, Uint32 uSize)
{
	Uint32 i, p;
	XMEMSET(THIS_, 0, sizeof(HESHES));
	THIS_->hessnd = 0;
	THIS_->hespcm = 0;
	for (i = 0; i < 0x100; i++) THIS_->memmap[i] = 0;

	if (uSize < 0x20) return NESERR_FORMAT;
	SONGINFO_SetStartSongNo(pNezPlay->song, pData[5] + 1);
	SONGINFO_SetMaxSongNo(pNezPlay->song, 256);
	SONGINFO_SetChannel(pNezPlay->song, 2);
	SONGINFO_SetExtendDevice(pNezPlay->song, 0);
	for (i = 0; i < 8; i++) THIS_->firstmpr[i] = pData[8 + i];
	THIS_->playerromaddr = 0x1ff0;
	THIS_->initaddr = GetWordLE(pData + 0x06);
	SONGINFO_SetInitAddress(pNezPlay->song, THIS_->initaddr);
	SONGINFO_SetPlayAddress(pNezPlay->song, 0);

	sprintf(songinfodata.detail,
#ifdef EMSCRIPTEN
"Type           : HES<br>\r\n\
Start Song     : %02XH<br>\r\n\
Init Address   : %04XH<br>\r\n\
First Mapper 0 : %02XH<br>\r\n\
First Mapper 1 : %02XH<br>\r\n\
First Mapper 2 : %02XH<br>\r\n\
First Mapper 3 : %02XH<br>\r\n\
First Mapper 4 : %02XH<br>\r\n\
First Mapper 5 : %02XH<br>\r\n\
First Mapper 6 : %02XH<br>\r\n\
First Mapper 7 : %02XH"
#else
"Type           : HES\r\n\
Start Song     : %02XH\r\n\
Init Address   : %04XH\r\n\
First Mapper 0 : %02XH\r\n\
First Mapper 1 : %02XH\r\n\
First Mapper 2 : %02XH\r\n\
First Mapper 3 : %02XH\r\n\
First Mapper 4 : %02XH\r\n\
First Mapper 5 : %02XH\r\n\
First Mapper 6 : %02XH\r\n\
First Mapper 7 : %02XH"
#endif
		,pData[5],THIS_->initaddr
		,pData[0x8]
		,pData[0x9]
		,pData[0xa]
		,pData[0xb]
		,pData[0xc]
		,pData[0xd]
		,pData[0xe]
		,pData[0xf]
		);

	SONGINFO_SetDetail(pNezPlay->song, songinfodata.detail);	// EMSCRIPTEN

	if (!alloc_physical_address(THIS_, 0xf8 << 13, 0x2000))	/* RAM */
		return NESERR_SHORTOFMEMORY;
	if (!alloc_physical_address(THIS_, 0xf9 << 13, 0x2000))	/* SGX-RAM */
		return NESERR_SHORTOFMEMORY;
	if (!alloc_physical_address(THIS_, 0xfa << 13, 0x2000))	/* SGX-RAM */
		return NESERR_SHORTOFMEMORY;
	if (!alloc_physical_address(THIS_, 0xfb << 13, 0x2000))	/* SGX-RAM */
		return NESERR_SHORTOFMEMORY;
	if (!alloc_physical_address(THIS_, 0x00 << 13, 0x2000))	/* IPL-ROM */
		return NESERR_SHORTOFMEMORY;
	for (p = 0x10; p + 0x10 < uSize; p += 0x10 + GetDwordLE(pData + p + 4))
	{
		if (GetDwordLE(pData + p) == 0x41544144)	/* 'DATA' */
		{
			Uint32 a, l;
			l = GetDwordLE(pData + p + 4);
			a = GetDwordLE(pData + p + 8);
			if (!alloc_physical_address(THIS_, a, l)) return NESERR_SHORTOFMEMORY;
			if (l > uSize - p - 0x10) l = uSize - p - 0x10;
			copy_physical_address(THIS_, a, l, pData + p + 0x10);
		}
	}
	THIS_->hessnd = HESSoundAlloc();
	if (!THIS_->hessnd) return NESERR_SHORTOFMEMORY;
	THIS_->hespcm = HESAdPcmAlloc();
	if (!THIS_->hespcm) return NESERR_SHORTOFMEMORY;

	return NESERR_NOERROR;

}


static Int32 __fastcall ExecuteHES(void *pNezPlay)
{
	return ((NEZ_PLAY*)pNezPlay)->heshes ? execute((HESHES*)((NEZ_PLAY*)pNezPlay)->heshes) : 0;
}

static void __fastcall HESSoundRenderStereo(void *pNezPlay, Int32 *d)
{
	synth((HESHES*)((NEZ_PLAY*)pNezPlay)->heshes, d);
}

static Int32 __fastcall HESSoundRenderMono(void *pNezPlay)
{
	Int32 d[2] = { 0,0 } ;
	synth((HESHES*)((NEZ_PLAY*)pNezPlay)->heshes, d);
#if (((-1) >> 1) == -1)
	return (d[0] + d[1]) >> 1;
#else
	return (d[0] + d[1]) / 2;
#endif
}

const static NES_AUDIO_HANDLER heshes_audio_handler[] = {
	{ 0, ExecuteHES, 0, },
	{ 3, HESSoundRenderMono, HESSoundRenderStereo },
	{ 0, 0, 0, },
};

static void __fastcall HESHESVolume(void *pNezPlay, Uint32 v)
{
	if (((NEZ_PLAY*)pNezPlay)->heshes)
	{
		volume((HESHES*)((NEZ_PLAY*)pNezPlay)->heshes, v);
	}
}

const static NES_VOLUME_HANDLER heshes_volume_handler[] = {
	{ HESHESVolume, }, 
	{ 0, }, 
};

static void __fastcall HESHESReset(void *pNezPlay)
{
	if (((NEZ_PLAY*)pNezPlay)->heshes) reset((NEZ_PLAY*)pNezPlay);
}

const static NES_RESET_HANDLER heshes_reset_handler[] = {
	{ NES_RESET_SYS_LAST, HESHESReset, },
	{ 0,                  0, },
};

static void __fastcall HESHESTerminate(void *pNezPlay)
{
	if (((NEZ_PLAY*)pNezPlay)->heshes)
	{
		terminate((HESHES*)((NEZ_PLAY*)pNezPlay)->heshes);
		((NEZ_PLAY*)pNezPlay)->heshes = 0;
	}
}

const static NES_TERMINATE_HANDLER heshes_terminate_handler[] = {
	{ HESHESTerminate, },
	{ 0, },
};

Uint32 HESLoad(NEZ_PLAY *pNezPlay, Uint8 *pData, Uint32 uSize)
{
	Uint32 ret;
	HESHES *THIS_;

	THIS_ = (HESHES *)XMALLOC(sizeof(HESHES));
	if (!THIS_) return NESERR_SHORTOFMEMORY;
	ret = load(pNezPlay, THIS_, pData, uSize);
	if (ret)
	{
		terminate(THIS_);
		return ret;
	}
	pNezPlay->heshes = THIS_;
	NESAudioHandlerInstall(pNezPlay, heshes_audio_handler);
	NESVolumeHandlerInstall(pNezPlay, heshes_volume_handler);
	NESResetHandlerInstall(pNezPlay->nrh, heshes_reset_handler);
	NESTerminateHandlerInstall(&pNezPlay->nth, heshes_terminate_handler);
	return ret;
}
