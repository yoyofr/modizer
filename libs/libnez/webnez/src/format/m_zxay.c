#include "neserr.h"
#include "handler.h"
#include "audiosys.h"
#include "songinfo.h"

#include "m_zxay.h"

#include "device/s_psg.h"
#include "device/divfix.h"

#include "wkmz80/wkmz80.h"

#define IRQ_PATCH 2

#define SHIFT_CPS 15
#define ZX_BASECYCLES       (3579545)
#define AMSTRAD_BASECYCLES  (2000000)

typedef struct {
	KMZ80_CONTEXT ctx;
	KMIF_SOUND_DEVICE *sndp;
	KMIF_SOUND_DEVICE *amstrad_sndp;
	KMEVENT kme;
	KMEVENT_ITEM_ID vsync_id;

	Uint32 cps;		/* cycles per sample:fixed point */
	Uint32 cpsrem;	/* cycle remain */
	Uint32 cpsgap;	/* cycle gap */
	Uint32 total_cycles;	/* total played cycles */

	Uint32 startsong;
	Uint32 maxsong;

	Uint32 initaddr;
	Uint32 playaddr;
	Uint32 spinit;

	Uint8 ram[0x10004];
	Uint8 *data;
	Uint8 *datalimit;

	Uint8 amstrad_ppi_a;
	Uint8 amstrad_ppi_c;
#ifdef AMSTRAD_PPI_CONNECT_TYPE_B
	Uint8 amstrad_ppi_va;
	Uint8 amstrad_ppi_vd;
#endif
} ZXAY;

static Int32 execute(ZXAY *THIS_)
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

__inline static void synth(ZXAY *THIS_, Int32 *d)
{
	THIS_->sndp->synth(THIS_->sndp->ctx, d);
	THIS_->amstrad_sndp->synth(THIS_->amstrad_sndp->ctx, d);
}

__inline static void volume(ZXAY *THIS_, Uint32 v)
{
	THIS_->sndp->volume(THIS_->sndp->ctx, v);
	THIS_->amstrad_sndp->volume(THIS_->amstrad_sndp->ctx, v);
}


static void vsync_setup(ZXAY *THIS_)
{
	kmevent_settimer(&THIS_->kme, THIS_->vsync_id, 313 * 4 * 342 / 6);
}

static Uint32 read_event(void *ctx, Uint32 a)
{
	ZXAY *THIS_ = ctx;
	return THIS_->ram[a];
}

static Uint32 busread_event(void *ctx, Uint32 a)
{
	return 0x38;
}

static void write_event(void *ctx, Uint32 a, Uint32 v)
{
	ZXAY *THIS_ = ctx;
	THIS_->ram[a] = (Uint8)v;
}

static Uint32 ioread_event(void *ctx, Uint32 a)
{
	return 0xff;
}

static void iowrite_amstrad_ppi_update(ZXAY *THIS_)
{
	switch ((THIS_->amstrad_ppi_c >> 6) & 3)
	{
#ifdef AMSTRAD_PPI_CONNECT_TYPE_B
		case 0:
			THIS_->amstrad_sndp->write(THIS_->amstrad_sndp->ctx, 0, THIS_->amstrad_ppi_va);
			THIS_->amstrad_sndp->write(THIS_->amstrad_sndp->ctx, 1, THIS_->amstrad_ppi_vd);
			break;
		case 2:
			THIS_->amstrad_ppi_vd = THIS_->amstrad_ppi_a;
			break;
		case 3:
			THIS_->amstrad_ppi_va = THIS_->amstrad_ppi_a;
			break;
#else
		case 2:
			THIS_->amstrad_sndp->write(THIS_->amstrad_sndp->ctx, 1, THIS_->amstrad_ppi_a);
			break;
		case 3:
			THIS_->amstrad_sndp->write(THIS_->amstrad_sndp->ctx, 0, THIS_->amstrad_ppi_a);
			break;
#endif
	}
}
static void iowrite_event(void *ctx, Uint32 a, Uint32 v)
{
	ZXAY *THIS_ = ctx;
	if (a == 0xFFFD)					/* ZXspectrum */
		THIS_->sndp->write(THIS_->sndp->ctx, 0, v);
	else if (a == 0xBFFD)				/* ZXspectrum */
		THIS_->sndp->write(THIS_->sndp->ctx, 1, v);
	else if ((a & 0xFF) == 0x00FE)				/* 1 bit D/A */
	{
		THIS_->sndp->write(THIS_->sndp->ctx, 2, (v & 0x10)?1:0);
	}
	else if ((a & 0x0B00) == 0x0000)	/* Amstrad CPC 8255 port A */
	{
		THIS_->amstrad_ppi_a = (Uint8)v;
		iowrite_amstrad_ppi_update(THIS_);
	}
	else if ((a & 0x0B00) == 0x0200)	/* Amstrad CPC 8255 port C */
	{
		THIS_->amstrad_ppi_c = (Uint8)v;
		iowrite_amstrad_ppi_update(THIS_);
	}
}

static void vsync_event(KMEVENT *event, KMEVENT_ITEM_ID curid, ZXAY *THIS_)
{
	vsync_setup(THIS_);
#if IRQ_PATCH == 1
	if (THIS_->ctx.pc == 0x0019)
	{
		THIS_->ctx.regs8[REGID_HALTED] = 0;
		THIS_->ctx.pc = 0x0016;
	}
#elif IRQ_PATCH == 2
	if (THIS_->ctx.pc == 0x0018 || THIS_->ctx.pc == 0x0019)
	{
		THIS_->ctx.regs8[REGID_HALTED] = 0;
		THIS_->ctx.pc = 0x0015;
	}
#endif
	THIS_->ctx.regs8[REGID_INTREQ] |= 1;
}

static Uint32 GetWordBE(Uint8 *p)
{
	return p[1] | (p[0] << 8);
}

static void SetWordLE(Uint8 *p, Uint32 v)
{
	p[0] = (Uint8)(v >> (8 * 0)) & 0xFF;
	p[1] = (Uint8)(v >> (8 * 1)) & 0xFF;
}

static Uint8 *ZXAYOffset(Uint8 *p)
{
	Uint32 ofs = GetWordBE(p) ^ 0x8000;
	return p + ofs - 0x8000;
}
static void reset(NEZ_PLAY *pNezPlay)
{
	ZXAY *THIS_ = pNezPlay->zxay;
	Uint32 i, freq, song, reginit;
	Uint8 *p, *p2;

	freq = NESAudioFrequencyGet(pNezPlay);
	song = SONGINFO_GetSongNo(pNezPlay->song) - 1;
	if (song >= THIS_->maxsong) song = THIS_->startsong - 1;

#ifdef EMSCRIPTEN
	// get song title
	Uint16 u= ((Uint16)THIS_->data[0x12] << 8) +THIS_->data[0x13];			//+18 tab PTR
	Int16 songtab_ptr= *((Int16*)(&u));

	Uint8 *song_struct= &THIS_->data[0x12] + songtab_ptr + (song << 2);

	u= ((Uint16)song_struct[0x0] << 8) +song_struct[0x1];
	Int16 songname_ptr= *((Int16*)(&u));
	SONGINFO_SetTitle(pNezPlay->song, song_struct + songname_ptr);
#endif
	/* sound reset */
	THIS_->sndp->reset(THIS_->sndp->ctx, ZX_BASECYCLES, freq);
	THIS_->amstrad_sndp->reset(THIS_->amstrad_sndp->ctx, AMSTRAD_BASECYCLES, freq);
	for (i = 0x7; i <= 0xa; i++)
	{
		THIS_->sndp->write(THIS_->sndp->ctx, 0, i);
		THIS_->sndp->write(THIS_->sndp->ctx, 1, (i == 0x07) ? 0x38 : 0x08);
		THIS_->amstrad_sndp->write(THIS_->amstrad_sndp->ctx, 0, i);
		THIS_->amstrad_sndp->write(THIS_->amstrad_sndp->ctx, 1, (i == 0x07) ? 0x38 : 0x08);
	}
	THIS_->amstrad_ppi_a = 0;
	THIS_->amstrad_ppi_c = 0;
#ifdef AMSTRAD_PPI_CONNECT_TYPE_B
	THIS_->amstrad_ppi_va = 0;
	THIS_->amstrad_ppi_vd = 0;
#endif

	/* memory reset */
	XMEMSET(THIS_->ram, 0, sizeof(THIS_->ram));
	p = ZXAYOffset(THIS_->data + 18);
	p = ZXAYOffset(p + song * 4 + 2);
	reginit = GetWordBE(p + 8);
	p2 = ZXAYOffset(p + 10);
	THIS_->spinit = GetWordBE(p2);
	THIS_->initaddr = GetWordBE(p2 + 2);
	THIS_->playaddr = GetWordBE(p2 + 4);
	p = ZXAYOffset(p + 12);
	XMEMSET(&THIS_->ram[0x0000], 0xFF, 0x4000);
	XMEMSET(&THIS_->ram[0x4000], 0x00, 0xC000);
#if IRQ_PATCH == 1
	/* NEZplug */
	XMEMCPY(&THIS_->ram[0x0000], 
		"\xf3\x00\x00\x21\x00\x00\x7c\xb5"
		"\x20\x08\xed\x57\x67\x2d\x5e\x23"
		"\x56\xeb\x22\x17\x00\x00\xcd\x00"
		"\x00\x18\xfe\xc9", 0x1C);
#elif IRQ_PATCH == 2
	/* NEZplug with HALT */
	XMEMCPY(&THIS_->ram[0x0000], 
		"\xf3\x00\x00\x21\x00\x00\x7c\xb5"
		"\x20\x08\xed\x57\x67\x2d\x5e\x23"
		"\x56\xeb\x22\x16\x00\xcd\x00\x00"
		"\x76\x18\xfd\xc9", 0x1C);
#elif IRQ_PATCH == 3
	/* DeliAY with EI */
	XMEMCPY(&THIS_->ram[0x0000], 
		"\xf3\x00\xfb\x21\x00\x00\x7c\xb5"
		"\x20\x08\xed\x57\x67\x2d\x5e\x23"
		"\x56\xeb\x22\x17\x00\x00\xcd\x00"
		"\x00\x18\xfe\xc9", 0x1C);
#else
	/* DeliAY */
	XMEMCPY(&THIS_->ram[0x0000], 
		"\xf3\x00\x00\x21\x00\x00\x7c\xb5"
		"\x20\x08\xed\x57\x67\x2d\x5e\x23"
		"\x56\xeb\x22\x17\x00\x76\xcd\x00"
		"\x00\x18\xfa\xc9", 0x1C);
#endif
	THIS_->ram[0x0038] = 0xC9;
	if (!THIS_->initaddr) THIS_->initaddr = GetWordBE(p);	/* DEFAULT */
	SetWordLE(&THIS_->ram[0x0001], THIS_->initaddr);
	SONGINFO_SetInitAddress(pNezPlay->song, THIS_->initaddr);
	SONGINFO_SetPlayAddress(pNezPlay->song, THIS_->playaddr);
	SetWordLE(&THIS_->ram[0x0004], THIS_->playaddr);

	do
	{
		Uint32 load,size;
		load = GetWordBE(p);
		size = GetWordBE(p + 2);
		p2 = ZXAYOffset(p + 4);
		if (load + size > 0x10000) size = 0x10000 - load;
		if (p2 + size > THIS_->datalimit) size = THIS_->datalimit - p2;
		XMEMCPY(&THIS_->ram[load], p2, size);
		p += 6;
	} while (GetWordBE(p));

#if	IRQ_PATCH == 3
	/* return address from init */
	THIS_->ram[(THIS_->spinit - 2) & 0xffff] = 0x02;
	THIS_->ram[(THIS_->spinit - 1) & 0xffff] = 0x00;
#else
	THIS_->ram[(THIS_->spinit - 2) & 0xffff] = 0x03;
	THIS_->ram[(THIS_->spinit - 1) & 0xffff] = 0x00;
#endif

	/* cpu reset */
	wkmz80_reset(&THIS_->ctx);
	THIS_->ctx.user = THIS_;
	THIS_->ctx.kmevent = &THIS_->kme;
	THIS_->ctx.memread = read_event;
	THIS_->ctx.memwrite = write_event;
	THIS_->ctx.ioread = ioread_event;
	THIS_->ctx.iowrite = iowrite_event;
	THIS_->ctx.busread = busread_event;

	THIS_->ctx.regs8[REGID_A] = THIS_->ctx.regs8[REGID_B] = THIS_->ctx.regs8[REGID_D] = THIS_->ctx.regs8[REGID_H] = THIS_->ctx.regs8[REGID_IXH] = THIS_->ctx.regs8[REGID_IYH] = (Uint8)((reginit >> 8) & 0xff);
	THIS_->ctx.regs8[REGID_F] = THIS_->ctx.regs8[REGID_C] = THIS_->ctx.regs8[REGID_E] = THIS_->ctx.regs8[REGID_L] = THIS_->ctx.regs8[REGID_IXL] = THIS_->ctx.regs8[REGID_IYL] = (Uint8)((reginit >> 0) & 0xff);
	THIS_->ctx.saf = THIS_->ctx.sbc = THIS_->ctx.sde = THIS_->ctx.shl = reginit;
	THIS_->ctx.sp = (THIS_->spinit - 2) & 0xffff;
	THIS_->ctx.pc = THIS_->initaddr;

	THIS_->ctx.exflag = 3;	/* ICE/ACI */
	THIS_->ctx.regs8[REGID_M1CYCLE] = 1;
	THIS_->ctx.regs8[REGID_IFF1] = THIS_->ctx.regs8[REGID_IFF2] = 1;
	THIS_->ctx.regs8[REGID_INTREQ] = 0;
#if	IRQ_PATCH == 3
	THIS_->ctx.regs8[REGID_IMODE] = 4;	/* VECTOR INT MODE */
#else
	THIS_->ctx.regs8[REGID_IMODE] = 5;	/* VECTOR CALL MODE */
#endif
	THIS_->ctx.vector[0] = 0x001B;

	/* vsync reset */
	kmevent_init(&THIS_->kme);
	THIS_->vsync_id = kmevent_alloc(&THIS_->kme);
	kmevent_setevent(&THIS_->kme, THIS_->vsync_id, vsync_event, THIS_);
	THIS_->cpsgap = THIS_->cpsrem  = 0;
	THIS_->cps = DivFix(ZX_BASECYCLES, freq, SHIFT_CPS);

#if 0
	{
		/* request execute */
		Uint32 initbreak = 5 << 8; /* 5sec */
		while (THIS_->ctx.pc != THIS_->initsp && --initbreak) wkmz80_exec(&THIS_->ctx, ZX_BASECYCLES >> 8);
	}
	vsync_setup(THIS_);
	THIS_->total_cycles = 0;
#else
	vsync_setup(THIS_);
	THIS_->total_cycles = 0;
#endif
}

static void terminate(ZXAY *THIS_)
{
	if (THIS_->sndp) THIS_->sndp->release(THIS_->sndp->ctx);
	if (THIS_->amstrad_sndp) THIS_->amstrad_sndp->release(THIS_->amstrad_sndp->ctx);
	if (THIS_->data) XFREE(THIS_->data);
	XFREE(THIS_);
}

static Uint32 load(NEZ_PLAY *pNezPlay, ZXAY *THIS_, Uint8 *pData, Uint32 uSize)
{
	XMEMSET(THIS_, 0, sizeof(ZXAY));
	THIS_->sndp = THIS_->amstrad_sndp = 0;
	THIS_->data = 0;

	THIS_->data = (Uint8*)XMALLOC(uSize);
	if (!THIS_->data) return NESERR_SHORTOFMEMORY;
	XMEMCPY(THIS_->data, pData, uSize);
	THIS_->datalimit = THIS_->data + uSize;
	THIS_->maxsong = pData[0x10] + 1;
	THIS_->startsong = pData[0x11] + 1;

#ifdef EMSCRIPTEN
	// get composer info - same for all tracks
	Uint16 u= ((Uint16)THIS_->data[0x0c] << 8) +THIS_->data[0x0d];	//+12 creator PTR
	Int16 ptr= *((Int16*)(&u));										// is signed
	char *creator= &THIS_->data[0x0c] + ptr;
	SONGINFO_SetArtist(pNezPlay->song, creator);
#endif
	SONGINFO_SetStartSongNo(pNezPlay->song, THIS_->startsong);
	SONGINFO_SetMaxSongNo(pNezPlay->song, THIS_->maxsong);
	SONGINFO_SetExtendDevice(pNezPlay->song, 0);
	SONGINFO_SetChannel(pNezPlay->song, 1);

	THIS_->sndp = PSGSoundAlloc(PSG_TYPE_AY_3_8910);
	THIS_->amstrad_sndp = PSGSoundAlloc(PSG_TYPE_YM2149);
	if (!THIS_->sndp || !THIS_->amstrad_sndp) return NESERR_SHORTOFMEMORY;
	return NESERR_NOERROR;
}



static Int32 __fastcall ZXAYExecuteZ80CPU(void *pNezPlay)
{
	return ((NEZ_PLAY*)pNezPlay)->zxay ? execute((ZXAY*)((NEZ_PLAY*)pNezPlay)->zxay) : 0;
}

static void __fastcall ZXAYSoundRenderStereo(void *pNezPlay, Int32 *d)
{
	synth((ZXAY*)((NEZ_PLAY*)pNezPlay)->zxay, d);
}

static Int32 __fastcall ZXAYSoundRenderMono(void *pNezPlay)
{
	Int32 d[2] = { 0, 0 };
	synth((ZXAY*)((NEZ_PLAY*)pNezPlay)->zxay, d);
#if (((-1) >> 1) == -1)
	return (d[0] + d[1]) >> 1;
#else
	return (d[0] + d[1]) / 2;
#endif
}

const static NES_AUDIO_HANDLER zxay_audio_handler[] = {
	{ 0, ZXAYExecuteZ80CPU, 0, },
	{ 3, ZXAYSoundRenderMono, ZXAYSoundRenderStereo },
	{ 0, 0, 0, },
};

static void __fastcall ZXAYVolume(void *pNezPlay, Uint32 v)
{
	if (((NEZ_PLAY*)pNezPlay)->zxay)
	{
		volume((ZXAY*)((NEZ_PLAY*)pNezPlay)->zxay, v);
	}
}

const static NES_VOLUME_HANDLER zxay_volume_handler[] = {
	{ ZXAYVolume, }, 
	{ 0, }, 
};

static void __fastcall ZXAYReset(void *pNezPlay)
{
	if (((NEZ_PLAY*)pNezPlay)->zxay) reset((NEZ_PLAY*)pNezPlay);
}

const static NES_RESET_HANDLER zxay_reset_handler[] = {
	{ NES_RESET_SYS_LAST, ZXAYReset, },
	{ 0,                  0, },
};

static void __fastcall ZXAYTerminate(void *pNezPlay)
{
	if (((NEZ_PLAY*)pNezPlay)->zxay)
	{
		terminate((ZXAY*)((NEZ_PLAY*)pNezPlay)->zxay);
		((NEZ_PLAY*)pNezPlay)->zxay = 0;
	}
}

const static NES_TERMINATE_HANDLER zxay_terminate_handler[] = {
	{ ZXAYTerminate, },
	{ 0, },
};

Uint32 ZXAYLoad(NEZ_PLAY *pNezPlay, Uint8 *pData, Uint32 uSize)
{
	Uint32 ret;
	ZXAY *THIS_;
	
	THIS_ = (ZXAY *)XMALLOC(sizeof(ZXAY));
	if (!THIS_) return NESERR_SHORTOFMEMORY;
	ret = load(pNezPlay, THIS_, pData, uSize);
	if (ret)
	{
		terminate(THIS_);
		return ret;
	}
	pNezPlay->zxay = THIS_;
	NESAudioHandlerInstall(pNezPlay, zxay_audio_handler);
	NESVolumeHandlerInstall(pNezPlay, zxay_volume_handler);
	NESResetHandlerInstall(pNezPlay->nrh, zxay_reset_handler);
	NESTerminateHandlerInstall(&pNezPlay->nth, zxay_terminate_handler);
	return ret;
}
