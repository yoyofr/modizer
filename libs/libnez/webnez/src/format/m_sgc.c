#include "neserr.h"
#include "handler.h"
#include "audiosys.h"
#include "songinfo.h"

#include "m_sgc.h"

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

#define DEVICE_PAL		(1 << 0)

#define SND_SNG 0
#define SND_FMUNIT 1
#define SND_MAX 2


#define SND_VOLUME 0x800

enum{
	SYNTHMODE_SMS,
	SYNTHMODE_GG,
	SYNTHMODE_CV,
	SYNTHMODE_MAX
};

const char *system_name[]={
	"Sega Master System","Game Gear","Coleco Vision","?????"
};

typedef struct SGCSEQ_TAG SGCSEQ;

struct  SGCSEQ_TAG {
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


	Uint32 clockmode;	// 0x05		0:NTSC 1:PAL
	Uint32 loadaddr;	// 0x08,0x09
	Uint32 initaddr;	// 0x0a,0x0b
	Uint32 playaddr;	// 0x0c,0x0d
	Uint32 stackaddr;	// 0x0e,0x0f
	Uint32 rstaddr[7];	// 0x12-0x1f
	Uint8 mappernum[4];	// 0x20-0x23
	Uint32 startsong;	// 0x24
	Uint32 maxsong;		// 0x25
	Uint32 firstse;	// 0x26
	Uint32 lastse;	// 0x27
	Uint32 systype;		// 0x28		0:SMS 1:GG 2:ColecoVision

	Uint32 tmaxsong;

	Uint8 *data;
	Uint32 datasize;
	Uint8 *bankbase;
	Uint32 bankofs;
	Uint32 banknum;
	Uint32 banksize;

	Uint8 ram[0x6000];
	Uint8 bios[0x2000];
};

char ColecoBIOSFilePath[0x200];

static Uint32 GetWordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8);
}

static Uint32 GetDwordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

#define GetDwordLEM(p) (Uint32)((((Uint8 *)p)[0] | (((Uint8 *)p)[1] << 8) | (((Uint8 *)p)[2] << 16) | (((Uint8 *)p)[3] << 24)))

/*
void OPLLSetTone(Uint8 *p, Uint32 type)
{
	if ((GetDwordLE(p) & 0xf0ffffff) == GetDwordLEM("ILL0"))
		XMEMCPY(usertone[type], p, 16 * 19);
	else
		XMEMCPY(usertone[type], p, 8 * 15);
	usertone_enable[type] = 1;
}
*/

static Int32 execute(SGCSEQ *THIS_)
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

__inline static void synth(SGCSEQ *THIS_, Int32 *d)
{
	switch (THIS_->systype)
	{
		default:
			NEVER_REACH
		case SYNTHMODE_SMS:
			THIS_->sndp[SND_SNG]->synth(THIS_->sndp[SND_SNG]->ctx, d);
			THIS_->sndp[SND_FMUNIT]->synth(THIS_->sndp[SND_FMUNIT]->ctx, d);
			break;
		case SYNTHMODE_GG:
		case SYNTHMODE_CV:
			THIS_->sndp[SND_SNG]->synth(THIS_->sndp[SND_SNG]->ctx, d);
			break;
	}
}

__inline static void volume(SGCSEQ *THIS_, Uint32 v)
{
	v += 256;
	switch (THIS_->systype)
	{
		default:
			NEVER_REACH
		case SYNTHMODE_SMS:
			THIS_->sndp[SND_SNG]->volume(THIS_->sndp[SND_SNG]->ctx, v + (THIS_->vola[SND_SNG] << 4) - SND_VOLUME);
			THIS_->sndp[SND_FMUNIT]->volume(THIS_->sndp[SND_FMUNIT]->ctx, v + (THIS_->vola[SND_FMUNIT] << 4) - SND_VOLUME);
			break;
		case SYNTHMODE_GG:
		case SYNTHMODE_CV:
			THIS_->sndp[SND_SNG]->volume(THIS_->sndp[SND_SNG]->ctx, v + (THIS_->vola[SND_SNG] << 4) - SND_VOLUME);
			break;
	}
}

static void vsync_setup(SGCSEQ *THIS_)
{
	Int32 cycles;
	THIS_->cpfrem += THIS_->cpf;
	cycles = THIS_->cpfrem >> SHIFT_CPS;
	THIS_->cpfrem &= (1 << SHIFT_CPS) - 1;
	kmevent_settimer(&THIS_->kme, THIS_->vsync_id, cycles);
}

static void play_setup(SGCSEQ *THIS_, Uint32 pc)
{
	THIS_->bios[0x0]=0xcd;
	THIS_->bios[0x1]=pc&0xff;
	THIS_->bios[0x2]=pc>>8;
	THIS_->bios[0x3]=0x76;
	THIS_->bios[0x4]=0x18;
	THIS_->bios[0x5]=0xfe;
	THIS_->ctx.pc = 0x0000;
	THIS_->ctx.regs8[REGID_HALTED] = 0;
}

static Uint32 busread_event(void *ctx, Uint32 a)
{
	return 0x38;
}

static Uint32 read_event(void *ctx, Uint32 a)
{
	SGCSEQ *THIS_ = ctx;
	if(a<0x400)
	{
		return THIS_->bios[a];
	}
	else if(a>=0xfffc && THIS_->systype <= SYNTHMODE_GG)
	{
		return THIS_->mappernum[a&0x3];
	}
	else if (THIS_->readmap[a >> 13])
	{
		return THIS_->readmap[a >> 13][a&0x1fff];
	}
	return 0x00;
}


static void write_event(void *ctx, Uint32 a, Uint32 v)
{
	SGCSEQ *THIS_ = ctx;
	Uint32 page = a >> 13;
	if (a >= 0xfffd && THIS_->systype <= SYNTHMODE_GG)
	{
		Uint8 b = (a & 0x3);
		THIS_->mappernum[b] = v;
		if(THIS_->readmap[4]==THIS_->ram && a==0xffff)return;
		THIS_->readmap[b*2-2] = THIS_->data + THIS_->mappernum[b] * 0x4000;
		THIS_->readmap[b*2-1] = THIS_->data + THIS_->mappernum[b] * 0x4000 + 0x2000;
		THIS_->writemap[b*2-2] = 0;
		THIS_->writemap[b*2-1] = 0;

	}
	else if (a == 0xfffc && THIS_->systype <= SYNTHMODE_GG)
	{
		if(v&0x8){
			//RAM
			THIS_->readmap [4] = THIS_->ram;
			THIS_->writemap[4] = THIS_->ram;
			THIS_->readmap [5] = THIS_->ram;
			THIS_->writemap[5] = THIS_->ram;
		}else{
			//ROM
			THIS_->readmap [4] = THIS_->data + THIS_->mappernum[3] * 0x4000;
			THIS_->writemap[4] = 0;
			THIS_->readmap [5] = THIS_->data + THIS_->mappernum[3] * 0x4000 + 0x2000;
			THIS_->writemap[5] = 0;
		}

	}
	else if (THIS_->writemap[page])
	{
		THIS_->writemap[page][a&0x1fff] = (Uint8)(v & 0xff);
	}
}

static Uint32 ioread_eventSMS(void *ctx, Uint32 a)
{
	return 0xff;
}

static void iowrite_eventSMS(void *ctx, Uint32 a, Uint32 v)
{
	SGCSEQ *THIS_ = ctx;
	a &= 0xff;
	if ((a & 0xfe) == 0x7e)
		THIS_->sndp[SND_SNG]->write(THIS_->sndp[SND_SNG]->ctx, 0, v);
	else if (a == 0x06)
		THIS_->sndp[SND_SNG]->write(THIS_->sndp[SND_SNG]->ctx, 1, v);
	else if (THIS_->sndp[SND_FMUNIT] && (a & 0xfe) == 0xf0)
		THIS_->sndp[SND_FMUNIT]->write(THIS_->sndp[SND_FMUNIT]->ctx, a & 1, v);

}

static Uint32 ioread_eventCV(void *ctx, Uint32 a)
{
	return 0xff;
}

static void iowrite_eventCV(void *ctx, Uint32 a, Uint32 v)
{
	SGCSEQ *THIS_ = ctx;
	a &= 0xff;
	if ((a & 0xe0) == 0xe0)
		THIS_->sndp[SND_SNG]->write(THIS_->sndp[SND_SNG]->ctx, a & 1, v);
}

static void vsync_event(KMEVENT *event, KMEVENT_ITEM_ID curid, SGCSEQ *THIS_)
{
	vsync_setup(THIS_);
	if (THIS_->ctx.regs8[REGID_HALTED]) play_setup(THIS_, THIS_->playaddr);
}

//ここからメモリービュアー設定
static Uint32 (*memview_memread)(Uint32 a);
static SGCSEQ* memview_context;
static int MEM_MAX,MEM_IO,MEM_RAM,MEM_ROM;
Uint32 memview_memread_sgc(Uint32 a){
	return read_event(memview_context,a);
}
//ここまでメモリービュアー設定

//ここからダンプ設定
static NEZ_PLAY *pNezPlayDump;
static Uint32 (*dump_MEM_MSX)(Uint32 a,unsigned char* mem);
static Uint32 dump_MEM_MSX_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Memory
		for(i=0;i<0x10000;i++)
			mem[i] = memview_memread_sgc(i);
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



static void reset(NEZ_PLAY *pNezPlay)
{
	SGCSEQ *THIS_ = pNezPlay->sgcseq;
	Uint32 i, freq, song;

	freq = NESAudioFrequencyGet(pNezPlay);
	song = SONGINFO_GetSongNo(pNezPlay->song) - 1;
	if (song >= THIS_->tmaxsong) song = THIS_->startsong;
	if(song >= THIS_->maxsong) song += THIS_->firstse - THIS_->maxsong;

	/* sound reset */
	for (i = 0; i < SND_MAX; i++)
	{
		if (THIS_->sndp[i]) THIS_->sndp[i]->reset(THIS_->sndp[i]->ctx, BASECYCLES, freq);
	}

	/* memory reset */
	XMEMSET(THIS_->ram, 0, sizeof(THIS_->ram));
	XMEMSET(THIS_->bios, 0, sizeof(THIS_->bios));
	switch(THIS_->systype){
	case SYNTHMODE_SMS:
	case SYNTHMODE_GG:
		//0400-3FFF : ROM
		THIS_->readmap [0] = THIS_->data + THIS_->mappernum[1] * 0x4000;
		THIS_->writemap[0] = 0;
		THIS_->readmap [1] = THIS_->data + THIS_->mappernum[1] * 0x4000 + 0x2000;
		THIS_->writemap[1] = 0;
		//4000-7FFF : ROM
		THIS_->readmap [2] = THIS_->data + THIS_->mappernum[2] * 0x4000;
		THIS_->writemap[2] = 0;
		THIS_->readmap [3] = THIS_->data + THIS_->mappernum[2] * 0x4000 + 0x2000;
		THIS_->writemap[3] = 0;
		//8000-BFFF : ROM
		THIS_->readmap [4] = THIS_->data + THIS_->mappernum[3] * 0x4000;
		THIS_->writemap[4] = 0;
		THIS_->readmap [5] = THIS_->data + THIS_->mappernum[3] * 0x4000 + 0x2000;
		THIS_->writemap[5] = 0;
		//C000-FFFF : RAM
		THIS_->readmap [6] = THIS_->ram;
		THIS_->writemap[6] = THIS_->ram;
		THIS_->readmap [7] = THIS_->ram;
		THIS_->writemap[7] = THIS_->ram;
		//0000-03FF : FREE
		for (i = 1; i < 8; i++)
		{
			THIS_->bios[i*8]=0xc3;
			THIS_->bios[i*8+1]=THIS_->rstaddr[i-1]&0xff;
			THIS_->bios[i*8+2]=THIS_->rstaddr[i-1]>>8;
		}
		MEM_IO =0x8000;
		MEM_RAM=0xC000;
		MEM_ROM=0x4000;
		break;
	case SYNTHMODE_CV:
		//0000-1FFF : BIOS
		THIS_->readmap [0] = THIS_->bios;
		THIS_->writemap[0] = 0;
		//2000-5FFF : FREE
		THIS_->readmap [1] = 0;
		THIS_->writemap[1] = 0;
		THIS_->readmap [2] = 0;
		THIS_->writemap[2] = 0;
		//6000-7FFF : RAM
		THIS_->readmap [3] = THIS_->ram;
		THIS_->writemap[3] = THIS_->ram;
		//8000-FFFF : ROM
		THIS_->readmap [4] = THIS_->data + 0x0000;
		THIS_->writemap[4] = 0;
		THIS_->readmap [5] = THIS_->data + 0x2000;
		THIS_->writemap[5] = 0;
		THIS_->readmap [6] = THIS_->data + 0x4000;
		THIS_->writemap[6] = 0;
		THIS_->readmap [7] = THIS_->data + 0x6000;
		THIS_->writemap[7] = 0;

		XMEMSET(THIS_->bios, 0xc9, sizeof(THIS_->bios));

		{	//BIOSのロード 
			static FILE *fp = NULL;
			fp = fopen(ColecoBIOSFilePath, "rb");
			if (fp){
				fread(THIS_->bios,0x01,0x2000,fp);
				fclose(fp);
			}
		}
/*		for (i = 1; i < 8; i++)
		{
			THIS_->bios[i*8]=0xc3;
			THIS_->bios[i*8+1]=THIS_->rstaddr[i-1]&0xff;
			THIS_->bios[i*8+2]=THIS_->rstaddr[i-1]>>8;
		}
*/		MEM_IO =0x0000;
		MEM_RAM=0x6000;
		MEM_ROM=0x8000;
		break;
	default:
		break;
	}

	/* cpu reset */
	wkmz80_reset(&THIS_->ctx);
	THIS_->ctx.user = THIS_;
	THIS_->ctx.kmevent = &THIS_->kme;
	THIS_->ctx.memread = read_event;
	THIS_->ctx.memwrite = write_event;
	switch(THIS_->systype){
	case SYNTHMODE_SMS:
	case SYNTHMODE_GG:
		THIS_->ctx.ioread = ioread_eventSMS;
		THIS_->ctx.iowrite = iowrite_eventSMS;
		THIS_->ctx.regs8[REGID_M1CYCLE] = 1;
		break;
	case SYNTHMODE_CV:
		THIS_->ctx.ioread = ioread_eventCV;
		THIS_->ctx.iowrite = iowrite_eventCV;
		THIS_->ctx.regs8[REGID_M1CYCLE] = 1;
		break;
	default:
		break;
	}
	THIS_->ctx.busread = busread_event;

	THIS_->ctx.regs8[REGID_A] = (Uint8)((song >> 0) & 0xff);
	THIS_->ctx.regs8[REGID_H] = (Uint8)((song >> 8) & 0xff);
	THIS_->ctx.sp = THIS_->stackaddr;
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
	if (THIS_->clockmode&0x1)
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

	//ここからメモリービュアー設定
	memview_context = THIS_;
	MEM_MAX=0xffff;
	memview_memread = memview_memread_sgc;
	//ここまでメモリービュアー設定

}

static void terminate(SGCSEQ *THIS_)
{
	Uint32 i;

	//ここからダンプ設定
	dump_MEM_MSX     = NULL;
	dump_DEV_SN76489 = NULL;
	dump_DEV_OPLL    = NULL;
	//ここまでダンプ設定
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
static Uint8 titlebuffer[0x21];
static Uint8 artistbuffer[0x21];
static Uint8 copyrightbuffer[0x21];

static Uint32 load(NEZ_PLAY *pNezPlay, SGCSEQ *THIS_, Uint8 *pData, Uint32 uSize)
{
	Uint32 i, headersize;
	XMEMSET(THIS_, 0, sizeof(SGCSEQ));
	for (i = 0; i < SND_MAX; i++) THIS_->sndp[i] = 0;
	for (i = 0; i < SND_MAX; i++) THIS_->vola[i] = 0x80;
	THIS_->data = 0;
	if ((GetDwordLE(pData + 0) & 0x00ffffff) == GetDwordLEM("SGC"))
	{
		headersize = 0xa0;
		THIS_->clockmode = pData[0x05];
		THIS_->loadaddr = GetWordLE(pData + 0x08);
		THIS_->initaddr = GetWordLE(pData + 0x0a);
		THIS_->playaddr = GetWordLE(pData + 0x0c);
		THIS_->stackaddr = GetWordLE(pData + 0x0e);
		for(i=0;i<7;i++)THIS_->rstaddr[i] = GetWordLE(pData + 0x12 + i*2);
		for(i=0;i<4;i++)THIS_->mappernum[i] = pData[0x20+i];
		THIS_->startsong = pData[0x24];
		THIS_->maxsong = pData[0x25];
		THIS_->firstse = pData[0x26];
		THIS_->lastse = pData[0x27];
		THIS_->systype = pData[0x28];
		if(THIS_->systype>=SYNTHMODE_MAX)return NESERR_PARAMETER;
		if(THIS_->firstse < THIS_->maxsong
		|| THIS_->firstse > THIS_->lastse
		|| THIS_->firstse == 0){
			THIS_->firstse = THIS_->lastse = 0;
		}
		THIS_->tmaxsong = THIS_->maxsong + THIS_->lastse - THIS_->firstse + (THIS_->firstse?1:0);
	}
	else
	{
		return NESERR_FORMAT;
	}
	SONGINFO_SetStartSongNo(pNezPlay->song, THIS_->startsong+1);
	SONGINFO_SetMaxSongNo(pNezPlay->song, THIS_->tmaxsong);
	SONGINFO_SetInitAddress(pNezPlay->song, THIS_->initaddr);
	SONGINFO_SetPlayAddress(pNezPlay->song, THIS_->playaddr);
//	SONGINFO_SetExtendDevice(pNezPlay->song, THIS_->extdevice << 8);

	sprintf(songinfodata.detail,
#ifdef EMSCRIPTEN
"Type          : SGC<br>\r\n\
System        : %s<br>\r\n\
Song Max      : %d<br>\r\n\
Start Song    : %d<br>\r\n\
First SE      : %d<br>\r\n\
Last SE       : %d<br>\r\n\
Load Address  : %04XH<br>\r\n\
Init Address  : %04XH<br>\r\n\
Play Address  : %04XH<br>\r\n\
Stack Address : %04XH<br>\r\n\
Clock Mode    : %s<br>\r\n\
<br>\r\n\
RST 08H ADR   : %04XH<br>\r\n\
RST 10H ADR   : %04XH<br>\r\n\
RST 18H ADR   : %04XH<br>\r\n\
RST 20H ADR   : %04XH<br>\r\n\
RST 28H ADR   : %04XH<br>\r\n\
RST 30H ADR   : %04XH<br>\r\n\
RST 38H ADR   : %04XH<br>\r\n\
<br>\r\n\
First ROM Bank(0400-3FFF): %02XH<br>\r\n\
First ROM Bank(4000-7FFF): %02XH<br>\r\n\
First ROM Bank(8000-BFFF): %02XH<br>\r\n\
RAM Bank      (8000-BFFF): %d"
#else
"Type          : SGC\r\n\
System        : %s\r\n\
Song Max      : %d\r\n\
Start Song    : %d\r\n\
First SE      : %d\r\n\
Last SE       : %d\r\n\
Load Address  : %04XH\r\n\
Init Address  : %04XH\r\n\
Play Address  : %04XH\r\n\
Stack Address : %04XH\r\n\
Clock Mode    : %s\r\n\
\r\n\
RST 08H ADR   : %04XH\r\n\
RST 10H ADR   : %04XH\r\n\
RST 18H ADR   : %04XH\r\n\
RST 20H ADR   : %04XH\r\n\
RST 28H ADR   : %04XH\r\n\
RST 30H ADR   : %04XH\r\n\
RST 38H ADR   : %04XH\r\n\
\r\n\
First ROM Bank(0400-3FFF): %02XH\r\n\
First ROM Bank(4000-7FFF): %02XH\r\n\
First ROM Bank(8000-BFFF): %02XH\r\n\
RAM Bank      (8000-BFFF): %d\r\n\
"
#endif
		,system_name[THIS_->systype]
		,THIS_->maxsong
		,THIS_->startsong
		,THIS_->firstse
		,THIS_->lastse
		,THIS_->loadaddr,THIS_->initaddr,THIS_->playaddr,THIS_->stackaddr
		,THIS_->clockmode&0x01 ? "PAL" : "NTSC"
		,THIS_->rstaddr[0]
		,THIS_->rstaddr[1]
		,THIS_->rstaddr[2]
		,THIS_->rstaddr[3]
		,THIS_->rstaddr[4]
		,THIS_->rstaddr[5]
		,THIS_->rstaddr[6]
		,THIS_->mappernum[1]
		,THIS_->mappernum[2]
		,THIS_->mappernum[3]
		,THIS_->mappernum[0]&0x8 ? 1:0
	);
	SONGINFO_SetDetail(pNezPlay->song, songinfodata.detail);	// EMSCRIPTEN	

	XMEMSET(titlebuffer, 0, 0x21);
	XMEMCPY(titlebuffer, pData + 0x0040, 0x20);
	songinfodata.title=titlebuffer;
	SONGINFO_SetTitle(pNezPlay->song, titlebuffer);

	XMEMSET(artistbuffer, 0, 0x21);
	XMEMCPY(artistbuffer, pData + 0x0060, 0x20);
	songinfodata.artist=artistbuffer;
	SONGINFO_SetArtist(pNezPlay->song, artistbuffer);

	XMEMSET(copyrightbuffer, 0, 0x21);
	XMEMCPY(copyrightbuffer, pData + 0x0080, 0x20);
	songinfodata.copyright=copyrightbuffer;
	SONGINFO_SetCopyright(pNezPlay->song, copyrightbuffer);

	//ここからダンプ設定
	dump_MEM_MSX     = dump_MEM_MSX_bf;
	//ここまでダンプ設定

	switch(THIS_->systype){
	case SYNTHMODE_SMS:
		THIS_->sndp[SND_SNG] = SNGSoundAlloc(SNG_TYPE_SEGAMKIII);
		THIS_->sndp[SND_FMUNIT] = OPLSoundAlloc(OPL_TYPE_SMSFMUNIT);
		if (!THIS_->sndp[SND_FMUNIT]) return NESERR_SHORTOFMEMORY;
		SONGINFO_SetChannel(pNezPlay->song, 1);
		//ここからダンプ設定
		dump_DEV_SN76489 = dump_DEV_SN76489_bf;
		dump_DEV_OPLL = dump_DEV_OPLL_bf;
		//ここまでダンプ設定
		THIS_->datasize = uSize - headersize;
		THIS_->banknum = ((THIS_->datasize+THIS_->loadaddr)>>14)+1;
		if(THIS_->banknum < 3)THIS_->banknum = 3;
		THIS_->banksize = 0x4000;
		if(THIS_->mappernum[1] >= THIS_->banknum)return NESERR_PARAMETER;
		if(THIS_->mappernum[2] >= THIS_->banknum)return NESERR_PARAMETER;
		if(THIS_->mappernum[3] >= THIS_->banknum)return NESERR_PARAMETER;

		THIS_->data = XMALLOC(THIS_->banksize * THIS_->banknum + 8);
		if (!THIS_->data) return NESERR_SHORTOFMEMORY;

		XMEMSET(THIS_->data, 0, THIS_->banknum * THIS_->banksize);

		XMEMCPY(THIS_->data + THIS_->loadaddr, pData + headersize, THIS_->datasize);

		break;
	case SYNTHMODE_GG:
		THIS_->sndp[SND_SNG] = SNGSoundAlloc(SNG_TYPE_GAMEGEAR);
		SONGINFO_SetChannel(pNezPlay->song, 2);
		//ここからダンプ設定
		dump_DEV_SN76489 = dump_DEV_SN76489_bf2;
		//ここまでダンプ設定
		THIS_->datasize = uSize - headersize;
		THIS_->banknum = ((THIS_->datasize+THIS_->loadaddr)>>14)+1;
		if(THIS_->banknum < 3)THIS_->banknum = 3;
		THIS_->banksize = 0x4000;
		if(THIS_->mappernum[1] >= THIS_->banknum)return NESERR_PARAMETER;
		if(THIS_->mappernum[2] >= THIS_->banknum)return NESERR_PARAMETER;
		if(THIS_->mappernum[3] >= THIS_->banknum)return NESERR_PARAMETER;

		THIS_->data = XMALLOC(THIS_->banksize * THIS_->banknum + 8);
		if (!THIS_->data) return NESERR_SHORTOFMEMORY;

		XMEMSET(THIS_->data, 0, THIS_->banknum * THIS_->banksize);

		XMEMCPY(THIS_->data + THIS_->loadaddr, pData + headersize, THIS_->datasize);

		break;
	case SYNTHMODE_CV:
		THIS_->sndp[SND_SNG] = SNGSoundAlloc(SNG_TYPE_SN76489AN);
		SONGINFO_SetChannel(pNezPlay->song, 1);
		//ここからダンプ設定
		dump_DEV_SN76489 = dump_DEV_SN76489_bf;
		//ここまでダンプ設定
		THIS_->banknum = 4;
		THIS_->banksize = 0x2000;
		THIS_->datasize = uSize - headersize;
		if (THIS_->datasize > 0x8000)
			THIS_->datasize = 0x10000 - THIS_->loadaddr;
		THIS_->data = XMALLOC(THIS_->banksize * THIS_->banknum + 8);
		if (!THIS_->data) return NESERR_SHORTOFMEMORY;

		XMEMSET(THIS_->data, 0, THIS_->banknum * THIS_->banksize);

		XMEMCPY(THIS_->data + THIS_->loadaddr - 0x8000, pData + headersize, THIS_->datasize);

		break;
	default:
		break;
	}

	return NESERR_NOERROR;
}

static Int32 __fastcall SGCSEQExecuteZ80CPU(void *pNezPlay)
{
	execute(((NEZ_PLAY*)pNezPlay)->sgcseq);
	return 0;
}

static void __fastcall SGCSEQSoundRenderStereo(void *pNezPlay, Int32 *d)
{
	synth(((NEZ_PLAY*)pNezPlay)->sgcseq, d);
}

static Int32 __fastcall SGCSEQSoundRenderMono(void *pNezPlay)
{
	Int32 d[2] = { 0, 0 };
	synth(((NEZ_PLAY*)pNezPlay)->sgcseq, d);
#if (((-1) >> 1) == -1)
	return (d[0] + d[1]) >> 1;
#else
	return (d[0] + d[1]) / 2;
#endif
}

const static NES_AUDIO_HANDLER sgcseq_audio_handler[] = {
	{ 0, SGCSEQExecuteZ80CPU, 0, },
	{ 3, SGCSEQSoundRenderMono, SGCSEQSoundRenderStereo },
	{ 0, 0, 0, },
};

static void __fastcall SGCSEQVolume(void *pNezPlay, Uint32 v)
{
	if (((NEZ_PLAY*)pNezPlay)->sgcseq)
	{
		volume(((NEZ_PLAY*)pNezPlay)->sgcseq, v);
	}
}

const static NES_VOLUME_HANDLER sgcseq_volume_handler[] = {
	{ SGCSEQVolume, }, 
	{ 0, }, 
};

static void __fastcall SGCSEQReset(void *pNezPlay)
{
	if (((NEZ_PLAY*)pNezPlay)->sgcseq) reset((NEZ_PLAY*)pNezPlay);
}

const static NES_RESET_HANDLER sgcseq_reset_handler[] = {
	{ NES_RESET_SYS_LAST, SGCSEQReset, },
	{ 0,                  0, },
};

static void __fastcall SGCSEQTerminate(void *pNezPlay)
{
	if (((NEZ_PLAY*)pNezPlay)->sgcseq)
	{
		terminate(((NEZ_PLAY*)pNezPlay)->sgcseq);
		((NEZ_PLAY*)pNezPlay)->sgcseq = 0;
	}
}

const static NES_TERMINATE_HANDLER sgcseq_terminate_handler[] = {
	{ SGCSEQTerminate, },
	{ 0, },
};

Uint32 SGCLoad(NEZ_PLAY *pNezPlay, Uint8 *pData, Uint32 uSize)
{
	Uint32 ret;
	SGCSEQ *THIS_;

	THIS_ = XMALLOC(sizeof(SGCSEQ));
	if (!THIS_) return NESERR_SHORTOFMEMORY;
	ret = load(pNezPlay, THIS_, pData, uSize);
	if (ret)
	{
		terminate(THIS_);
		return ret;
	}
	pNezPlay->sgcseq = THIS_;
	NESAudioHandlerInstall(pNezPlay, sgcseq_audio_handler);
	NESVolumeHandlerInstall(pNezPlay, sgcseq_volume_handler);
	NESResetHandlerInstall(pNezPlay->nrh, sgcseq_reset_handler);
	NESTerminateHandlerInstall(&pNezPlay->nth, sgcseq_terminate_handler);
	return ret;
}
