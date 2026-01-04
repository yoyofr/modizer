#include "neserr.h"
#include "handler.h"
#include "audiosys.h"
#include "songinfo.h"

#include "m_gbr.h"
#include "device/s_dmg.h"
#include "device/divfix.h"

#include "wkmz80/wkmz80.h"
#include <stdio.h>
#define SHIFT_CPS 15

#define DMG_BASECYCLES (4096 * 1024)

#define TEKKA_PATCH_ENABLE 0
#define MARIO2_PATCH_ENABLE 0
#define GBS2GB_EMULATION 0
#if 0

DMG
system clock 4194304Hz
hsync clock 9198Hz (456 cycles)
vsync clock 59.73Hz (456*154cycles)
timer0 clock 4096Hz   (system clock / 1024)
timer1 clock 262144Hz (system clock / 16)
timer2 clock 65536Hz  (system clock / 64)
timer3 clock 16384Hz  (system clock / 256)

sound length counter 256Hz
sound length(ch3) counter 2Hz
sweep 128Hz
envelope 64Hz

h/v = 154 = 144(vdisp)+10(vbrank) lines

FF80-FFFF high RAM
FF00-FF4B I/O
E000-FE00 (Mirror of 8kB Internal RAM)
D000-DFFF 4kB Internal RAM / 4kB switchable CGB Internal RAM bank
C000-CFFF 4kB Internal RAM
A000-BFFF 8kB External switchable RAM bank
8000-9FFF 8kB VRAM
6000-7FFF 16kB External switchable ROM bank / MBC1 ROM/RAM Select
4000-5FFF ...                               / RAM Bank Select
2000-3FFF 16kB External ROM bank #0         / ROM Bank Select
0000-1FFF ...                               / RAM Bank enable

#endif

const static Uint32 timer_clock_table[4] = {
	10, 4, 6, 8,
};

typedef struct GBRDMG_TAG GBRDMG;
typedef Uint32 (*READPROC)(GBRDMG *THIS_, Uint32 a);
typedef void (*WRITEPROC)(GBRDMG *THIS_, Uint32 a, Uint32 v);

struct  GBRDMG_TAG {
	KMZ80_CONTEXT ctx;
	KMIF_SOUND_DEVICE *dmgsnd;
	KMEVENT kme;
	KMEVENT_ITEM_ID vsync;
	KMEVENT_ITEM_ID timer;

	Uint32 cps;		/* cycles per sample:fixed point */
	Uint32 cpsrem;	/* cycle remain */
	Uint32 cpsgap;	/* cycle gap */
	Uint32 total_cycles;	/* total played cycles */

	Uint32 startsong;
	Uint32 maxsong;

	Uint32 loadaddr;
	Uint32 initaddr;
	Uint32 vsyncaddr;
	Uint32 timeraddr;
	//---+ [changes_rough.txt]
	Uint32 playaddr;
	//---+
	Uint32 stackaddr;

	Uint8 *bankrom;
	Uint8 *playerrom;
	Uint32 playerromioaddr;

	Uint8 *readmap[16];
	READPROC readproc[16];
	Uint8 *writemap[16];
	WRITEPROC writeproc[16];

	Uint8 highram[0x80];
	Uint8 io[0x180];
	Uint8 ram[8 * 0x1000];
	Uint8 vram[0x2000];
	Uint8 bankram[0x10 * 0x2000];
#if USE_DUMMYRAM
	Uint8 dummyram[0x2000];
#endif

	Uint8 isgbr;
	Uint8 bankromnum;
	Uint8 bankromfirst[2];
	Uint8 gb_DIV;
	Uint8 gb_TIMA;
	Uint8 gb_TMA;
	Uint8 gb_TMC;
	Uint8 gb_IE;
	Uint8 gb_IF;
	Uint8 timerflag;
	Uint8 firstTMA;
	Uint8 firstTMC;
	Uint8 isCGB;
	//---+ [changes_rough.txt]
	Uint8 useINT;
	//---+
	Uint8 rambankenable;
	Uint8 rombankselect;
	Uint8 rombankselecthi;
	Uint8 rambankselect;
	Uint8 romramselect;
	enum {
		GB_MAPPER_ROM,
		GB_MAPPER_MBC1,
		GB_MAPPER_MBC2,
		GB_MAPPER_MBC3,
		GB_MAPPER_MBC5,
		GB_MAPPER_SSC,
		GB_MAPPER_GBR
	} mapper_type;

	Uint8 titlebuffer[0x21];
	Uint8 artistbuffer[0x21];
	Uint8 copyrightbuffer[0x21];

};

static struct {
	char* title;
	char* artist;
	char* copyright;
	char detail[1024];
}songinfodata;


static Int32 execute(GBRDMG *THIS_)
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

__inline static void synth(GBRDMG *THIS_, Int32 *d)
{
	THIS_->dmgsnd->synth(THIS_->dmgsnd->ctx, d);
}

__inline static void volume(GBRDMG *THIS_, Uint32 v)
{
	THIS_->dmgsnd->volume(THIS_->dmgsnd->ctx, v);
}

static void vsync_setup(GBRDMG *THIS_)
{
	kmevent_settimer(&THIS_->kme, THIS_->vsync, THIS_->isCGB ? (154 * 456 * 2) : (154 * 456));
}

static void vsync_setup2(GBRDMG *THIS_)
{
	kmevent_settimer(&THIS_->kme, THIS_->vsync, 1);
}

static void timer_setup(GBRDMG *THIS_)
{
	if (THIS_->gb_TMC & 0x4)
	{
		kmevent_settimer(&THIS_->kme, THIS_->timer, (1 << timer_clock_table[THIS_->gb_TMC & 3]) * (256 - THIS_->gb_TIMA));
	}
	else
	{
		kmevent_settimer(&THIS_->kme, THIS_->timer, 0);
	}
}

static void timer_setup2(GBRDMG *THIS_)
{
	if (THIS_->gb_TMC & 0x4)
	{
		kmevent_settimer(&THIS_->kme, THIS_->timer, 1);
	}
	else
	{
		kmevent_settimer(&THIS_->kme, THIS_->timer, 0);
	}
}

static void timer_update_TIMA(GBRDMG *THIS_)
{
	Uint32 cnt;
	/* タイマ動作中なら現カウンタを取得 */
	if (kmevent_gettimer(&THIS_->kme, THIS_->timer, &cnt))
	{
		cnt >>= timer_clock_table[THIS_->gb_TMC & 3];
		THIS_->gb_TIMA = (Uint8)((256 - cnt) & 0xff);
	}
}



static void map_read_proc8k(GBRDMG *THIS_, Uint32 a, READPROC proc)
{
	Uint32 page = a >> 12;
	THIS_->readproc[page] = THIS_->readproc[page + 1] = proc;
}
static void map_read_mem4k(GBRDMG *THIS_, Uint32 a, Uint8 *p)
{
	Uint32 page = a >> 12;
	THIS_->readproc[page] = 0;
	THIS_->readmap[page] = p - (page << 12);
}
static void map_read_mem8k(GBRDMG *THIS_, Uint32 a, Uint8 *p)
{
	Uint32 page = a >> 12;
	THIS_->readproc[page] = THIS_->readproc[page + 1] = 0;
	THIS_->readmap[page] = THIS_->readmap[page + 1] = p - (page << 12);
}
static void map_write_proc8k(GBRDMG *THIS_, Uint32 a, WRITEPROC proc)
{
	Uint32 page = a >> 12;
	THIS_->writeproc[page] = THIS_->writeproc[page + 1] = proc;
}
static void map_write_mem4k(GBRDMG *THIS_, Uint32 a, Uint8 *p)
{
	Uint32 page = a >> 12;
	THIS_->writeproc[page] = 0;
	THIS_->writemap[page] = p - (page << 12);
}
static void map_write_mem8k(GBRDMG *THIS_, Uint32 a, Uint8 *p)
{
	Uint32 page = a >> 12;
	THIS_->writeproc[page] = THIS_->writeproc[page + 1] = 0;
	THIS_->writemap[page] = THIS_->writemap[page + 1] = p - (page << 12);
}

static Uint32 read_null(GBRDMG *THIS_, Uint32 a)
{
	return 0xFF;
}
static void write_null(GBRDMG *THIS_, Uint32 a, Uint32 v)
{
}

static Uint32 read_E000(GBRDMG *THIS_, Uint32 a)
{
	if (a >= 0xff80 && a != 0xffff)
		return THIS_->highram[a & 0x7f];
	else if (a < 0xfe00)
		return THIS_->ram[a & 0x1fff];
	else if (0xff10 <= a &&  a <= 0xff3f)
		return THIS_->dmgsnd->read(THIS_->dmgsnd->ctx, a);
	else {
		switch (a)
		{
			case 0xff04:	/* DIV */
				return (THIS_->gb_DIV + ((THIS_->total_cycles + THIS_->ctx.cycle) >> 8)) & 0xff;
			case 0xff05:	/* TIMA */
				timer_update_TIMA(THIS_);
				return THIS_->gb_TIMA;
			case 0xff06:	/* TMA */
				return THIS_->gb_TMA;
			case 0xff07:	/* TMC */
				return THIS_->gb_TMC;
			case 0xff0f:	/* IF */
				return THIS_->gb_IF;
//---+ [ff4d_fix.txt]
			case 0xff4d:	/* Speed Switch */
				if (THIS_->isCGB == 0) {
				return (Uint8)0x7e;
				} else {
				return THIS_->io[a & 0x1ff] | 0x80;
				}
//---+
			case 0xffff:	/* IE */
				return THIS_->gb_IE;
		}
		return THIS_->io[a & 0x1ff];
	}
}
static void write_E000(GBRDMG *THIS_, Uint32 a, Uint32 v)
{
	if (a >= 0xff80 && a != 0xffff)
		THIS_->highram[a & 0x7f] = (Uint8)v;
	else if (a < 0xfe00)
		THIS_->ram[a & 0x1fff] = (Uint8)v;
	else if (0xff10 <= a &&  a <= 0xff3f)
		THIS_->dmgsnd->write(THIS_->dmgsnd->ctx, a, v);
	else {
		THIS_->io[a & 0x1ff] = (Uint8)v;
		switch (a)
		{
			case 0xff04:	/* DIV */
				THIS_->gb_DIV = (v - ((THIS_->total_cycles + THIS_->ctx.cycle) >> 8)) & 0xff;
				break;
			case 0xff05:	/* TIMA */
				THIS_->gb_TIMA = (Uint8)v;
				timer_setup(THIS_);
				break;
			case 0xff06:	/* TMA */
				THIS_->gb_TMA = (Uint8)v;
				break;
			case 0xff07:	/* TMC */
				{
					Uint32 nextcount;
					double cnt;
					kmevent_gettimer(&THIS_->kme, THIS_->timer, &nextcount);
					cnt = ((double)nextcount) / (1 << timer_clock_table[THIS_->gb_TMC & 3]);
					THIS_->gb_TMC = (Uint8)v;
					cnt = cnt * (1 << timer_clock_table[THIS_->gb_TMC & 3]);
					if (THIS_->gb_TMC & 0x4)
					{
						kmevent_settimer(&THIS_->kme, THIS_->timer, (Uint32)cnt);
					}
					else
					{
						kmevent_settimer(&THIS_->kme, THIS_->timer, 0);
					}
				}
				//THIS_->gb_TIMA = THIS_->gb_TMA;
				//timer_setup(THIS_);
				break;
			case 0xff0f:	/* IF */
				THIS_->ctx.regs8[REGID_INTREQ] = THIS_->gb_IF = (Uint8)v;
				break;
//---+ [ff4d_fix.txt]
			case 0xff4d:	/* Speed Switch */
				/* The XOR(^) is normally done during an OP_STOP command */
				if (THIS_->isCGB == 0) {
				THIS_->io[a & 0x1ff] = (Uint8)0x7e;
				} else {
				if (((Uint8)v & 1) == 1) {
				THIS_->io[a & 0x1ff] = (Uint8)0xfe;
				}
				}
				break;
//---+
			case 0xff70:	/* SVBK */
				{
					Uint32 rampage = v & 7;
					if (!rampage) rampage = 1;
					map_read_mem4k(THIS_, 0xd000, &THIS_->ram[rampage << 12]);
					map_write_mem4k(THIS_, 0xd000, &THIS_->ram[rampage << 12]);
				}
				break;
			case 0xffff:	/* IE */
				THIS_->gb_IE = (Uint8)v;
				THIS_->ctx.regs8[REGID_INTMASK] = THIS_->gb_IE & 0x5;
				break;
		}
	}
}

static void force_rombank(GBRDMG *THIS_, Uint32 bank, Uint32 data)
{
	Uint32 base = bank << 14;
	if (data >= THIS_->bankromnum)
	{
#if 1
		/* 無視してみる */
#else
		/* bank #0 を割り当ててみる */
		map_read_mem8k(THIS_, base + 0x0000, THIS_->bankrom);
		map_read_mem8k(THIS_, base + 0x2000, THIS_->bankrom + 0x2000);
		/* 無効領域マップしてみる */
		map_read_proc8k(THIS_, base + 0x0000, read_null);
		map_read_proc8k(THIS_, base + 0x2000, read_null);
#endif
	}
	else
	{
		map_read_mem8k(THIS_, base + 0x0000, THIS_->bankrom + (data << 14));
		map_read_mem8k(THIS_, base + 0x2000, THIS_->bankrom + (data << 14) + 0x2000);
	}
}

static void update_banks(GBRDMG *THIS_)
{
	Uint32 rambankenable = 0;
	Uint32 rambank = 0;
	Uint32 rombank;
	rombank = THIS_->rombankselect;
	switch (THIS_->mapper_type)
	{
		case GB_MAPPER_ROM:
			rombank = 1;
			rambankenable = 0;
			rambank = 0;
			break;
		case GB_MAPPER_MBC1:
		case GB_MAPPER_SSC:
			if (THIS_->romramselect)
			{
				rambank = THIS_->rambankselect;
			}
			else
			{
				rambank = 0;
				rombank += (THIS_->rambankselect << 5);
			}
			rambankenable = (THIS_->mapper_type == GB_MAPPER_SSC) || (THIS_->rambankenable == 0xa);
			if (rombank == 0) rombank = 1;
			break;
		case GB_MAPPER_MBC2:
			rambankenable = (THIS_->rambankenable == 0xa);
			rambank = 0;
			if (rombank == 0) rombank = 1;
			break;
		case GB_MAPPER_MBC3:
			rambank = THIS_->rambankselect;
			rambankenable = (rambank < 4) || (THIS_->rambankenable == 0xa);
			if (rombank == 0) rombank = 1;
			break;
		case GB_MAPPER_MBC5:
			rambank = THIS_->rambankselect;
			rombank += THIS_->rombankselecthi << 8;
			rambankenable = (THIS_->rambankenable == 0xa);
			break;
		case GB_MAPPER_GBR:
			rambank = THIS_->rambankselect;
			rombank += THIS_->rombankselecthi << 8;
			rambankenable = (THIS_->rambankenable == 0xa);
			if (rombank == 0) rombank = 1;
			break;
	}
	if (rambankenable)
	{
		map_write_mem8k(THIS_, 0xA000, THIS_->bankram + (rambank << 13));
		map_read_mem8k(THIS_, 0xA000, THIS_->bankram + (rambank << 13));
	}
	else
	{
#if USE_DUMMYRAM
		map_write_mem8k(THIS_, 0xA000, THIS_->dummyram);
		map_read_mem8k(THIS_, 0xA000, THIS_->dummyram);
#else
		map_write_proc8k(THIS_, 0xA000, write_null);
		map_read_proc8k(THIS_, 0xA000, read_null);
#endif
	}
	force_rombank(THIS_, 1, rombank);
}

static void write_rambankenable(GBRDMG *THIS_, Uint32 a, Uint32 v)
{
	switch (THIS_->mapper_type)
	{
		case GB_MAPPER_MBC1:
		case GB_MAPPER_SSC:
		case GB_MAPPER_MBC3:
		case GB_MAPPER_MBC5:
			THIS_->rambankenable = (Uint8)(v & 0x0f);
			update_banks(THIS_);
			break;
		case GB_MAPPER_MBC2:
			if (!(a & 0x0100))
			{
				THIS_->rambankenable = (Uint8)(v & 0x0f);
				update_banks(THIS_);
			}
			break;
		default:
			break;
	}
}


static void write_rombankselect(GBRDMG *THIS_, Uint32 a, Uint32 v)
{
	switch (THIS_->mapper_type)
	{
		case GB_MAPPER_MBC1:
		case GB_MAPPER_SSC:
			THIS_->rombankselect = (Uint8)(v & 0x1f);
			update_banks(THIS_);
			break;
		case GB_MAPPER_MBC2:
			if (a & 0x0100)
			{
				THIS_->rombankselect = (Uint8)(v & 0x0f);
				update_banks(THIS_);
			}
			break;
		case GB_MAPPER_MBC3:
			THIS_->rombankselect = (Uint8)(v & 0x7f);
			update_banks(THIS_);
			break;
		case GB_MAPPER_MBC5:
		case GB_MAPPER_GBR:
			if (a & 0x01000)
				THIS_->rombankselecthi = (Uint8)(v & 1);
			else
				THIS_->rombankselect = (Uint8)(v & 0xff);
			update_banks(THIS_);
			break;
		default:
			break;
	}
}
static void write_rambankselect(GBRDMG *THIS_, Uint32 a, Uint32 v)
{
	switch (THIS_->mapper_type)
	{
		case GB_MAPPER_MBC1:
		case GB_MAPPER_SSC:
			THIS_->rambankselect = (Uint8)(v & 0x03);
			update_banks(THIS_);
			break;
		case GB_MAPPER_MBC3:
		case GB_MAPPER_MBC5:
		case GB_MAPPER_GBR:
			THIS_->rambankselect = (Uint8)(v & 0x0f);
			update_banks(THIS_);
			break;
		default:
			break;
	}
}

static void write_romramselect(GBRDMG *THIS_, Uint32 a, Uint32 v)
{
	switch (THIS_->mapper_type)
	{
		case GB_MAPPER_MBC1:
		case GB_MAPPER_SSC:
			THIS_->romramselect = (Uint8)(v & 0x1);
			update_banks(THIS_);
			break;
		default:
			break;
	}
}


static Uint32 read_event(void *ctx, Uint32 a)
{
	GBRDMG *THIS_ = ctx;
	Uint32 page = a >> 12;
	if (THIS_->readproc[page])
		return THIS_->readproc[page](THIS_, a);
	else
		return THIS_->readmap[page][a];
}

static void write_event(void *ctx, Uint32 a, Uint32 v)
{
	GBRDMG *THIS_ = ctx;
	Uint32 page = a >> 12;
	if (THIS_->writeproc[page])
		THIS_->writeproc[page](THIS_, a, v);
	else
		THIS_->writemap[page][a] = (Uint8)v;
}

static void vsync_event(KMEVENT *event, KMEVENT_ITEM_ID curid, GBRDMG *THIS_)
{

	vsync_setup(THIS_);
	THIS_->gb_IF |= 1;
	THIS_->ctx.regs8[REGID_INTREQ] |= 1;
	//---+ [changes_rough.txt]
	if (THIS_->useINT) THIS_->ctx.playflag |= 0x41;
	//---+
	if (THIS_->gb_IE & 1)
	{
	}//else
	//	vsync_setup2(THIS_);
}

static void timer_event(KMEVENT *event, KMEVENT_ITEM_ID curid, GBRDMG *THIS_)
{
	THIS_->gb_TIMA = THIS_->gb_TMA;
	timer_setup(THIS_);
	THIS_->gb_IF |= 4;
	THIS_->ctx.regs8[REGID_INTREQ] |= 4;
	//---+ [changes_rough.txt]
	if (THIS_->useINT) THIS_->ctx.playflag |= 0x44;
	//---+
	if (THIS_->gb_IE & 4)
	{
	}//else
	//	timer_setup2(THIS_);

}

//ここからダンプ設定
static NEZ_PLAY *pNezPlayDump;
Uint32 (*dump_MEM_GB)(Uint32 a,unsigned char* mem);
static Uint32 dump_MEM_GB_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Memory
		for(i=0;i<0x10000;i++)
			mem[i] = read_event(pNezPlayDump->gbrdmg,i);
		return i;
	}
	return -2;
}
//----------
extern Uint8 *gb_regdata;

Uint32 (*dump_DEV_DMG)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_DMG_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register
		for(i=0;i<0x30;i++)
			mem[i] = gb_regdata[i];
		return i;

	case 2://Wave Data
		for(i=0;i<0x10;i++)
			mem[i] = gb_regdata[i+0x20];
		return i;
	}
	return -2;
}
//----------


static void reset(NEZ_PLAY *pNezPlay)
{
	GBRDMG *THIS_ = pNezPlay->gbrdmg;
	Uint32 song, initbreak;
	Uint32 freq = NESAudioFrequencyGet(pNezPlay);
	song = SONGINFO_GetSongNo(pNezPlay->song) - 1;
	if (song >= THIS_->maxsong) song = THIS_->startsong - 1;

	THIS_->playerromioaddr = 0x9fc0;
	THIS_->playerrom = &THIS_->vram[THIS_->playerromioaddr & 0x1fff];
	wkmdmg_reset(&THIS_->ctx);
	XMEMSET(THIS_->highram, 0, 0x80);
	XMEMSET(THIS_->io, 0, 0x180);
	XMEMSET(THIS_->ram, 0, 8 * 0x1000);
	XMEMSET(THIS_->vram, 0, 0x2000);
	XMEMSET(THIS_->bankram, 0, 0x10 * 0x2000);
#if USE_DUMMYRAM
	XMEMSET(THIS_->dummyram, 0, 0x2000);
#endif
	THIS_->dmgsnd->reset(THIS_->dmgsnd->ctx, DMG_BASECYCLES, freq);
	kmevent_init(&THIS_->kme);
	THIS_->cpsrem = THIS_->cpsgap = 0;
	THIS_->cps = DivFix(DMG_BASECYCLES, freq, SHIFT_CPS + THIS_->isCGB);
	THIS_->ctx.user = THIS_;
	THIS_->ctx.kmevent = &THIS_->kme;
	THIS_->ctx.memread = read_event;
	THIS_->ctx.memwrite = write_event;
	THIS_->vsync = kmevent_alloc(&THIS_->kme);
	THIS_->timer = kmevent_alloc(&THIS_->kme);
	kmevent_setevent(&THIS_->kme, THIS_->vsync, vsync_event, THIS_);
	kmevent_setevent(&THIS_->kme, THIS_->timer, timer_event, THIS_);

	THIS_->rambankenable = 0xa/*THIS_->isgbr ? 0xa : 0*/;
	THIS_->rambankselect = 0;
	THIS_->romramselect =  0;
	THIS_->rombankselecthi = 0;
	force_rombank(THIS_, 0, 0);
	force_rombank(THIS_, 1, 0);
	force_rombank(THIS_, 0, THIS_->bankromfirst[0]);
	THIS_->rombankselect = THIS_->bankromfirst[1];
	map_write_proc8k(THIS_, 0x0000, write_rambankenable);
	map_write_proc8k(THIS_, 0x2000, write_rombankselect);
	map_write_proc8k(THIS_, 0x4000, write_rambankselect);
	map_write_proc8k(THIS_, 0x6000, write_romramselect);
	map_read_mem8k(THIS_, 0x8000, THIS_->vram);
	map_write_mem8k(THIS_, 0x8000, THIS_->vram);
	map_read_mem8k(THIS_, 0xC000, THIS_->ram);
	map_write_mem8k(THIS_, 0xC000, THIS_->ram);
	map_read_proc8k(THIS_, 0xE000, read_E000);
	map_write_proc8k(THIS_, 0xE000, write_E000);
	update_banks(THIS_);

	THIS_->ctx.regs8[REGID_A] = (Uint8)(song & 0xff);
#if 1
	THIS_->ctx.regs8[REGID_IMODE] = 4;	/* VECTOR IRQ MODE */
#else
	THIS_->ctx.regs8[REGID_IMODE] = 5;	/* VECTOR CALL MODE */
#endif
	THIS_->ctx.exflag = 2;	/* AUTOIRQCLAR */
	THIS_->ctx.vector[0] = THIS_->vsyncaddr;
	THIS_->ctx.vector[2] = THIS_->timeraddr;
	//---+ [changes_rough.txt]
	THIS_->ctx.playbase = THIS_->playaddr;
	THIS_->ctx.playflag = 0;
	//---+

	/* 9fc0 DMA */
	THIS_->playerrom[0x00] = 0xCD;	/* CALL */
	THIS_->playerrom[0x01] = (THIS_->initaddr >> 0) & 0xFF;
	THIS_->playerrom[0x02] = (THIS_->initaddr >> 8) & 0xFF;
	THIS_->playerrom[0x03] = 0x18;	/* JR */
	THIS_->playerrom[0x04] = 0xfe;	/* -2 */
#if TEKKA_PATCH_ENABLE
#if 0
	/*
		TEKKA.GBR(TEKKAMAN BLADE)対策 (PLAYからのRET前に余分なPOP) Ver.1(廃止)
		  こちらの方がわかりやすい？Wiz外伝で異常。
	*/
	THIS_->playerrom[0x05] = 0xcd;	/* CALL */
	THIS_->playerrom[0x06] = ((THIS_->playerromioaddr + 0x0a) >> 0) & 0xFF;
	THIS_->playerrom[0x07] = ((THIS_->playerromioaddr + 0x0a) >> 8) & 0xFF;
	THIS_->playerrom[0x08] = 0x18;	/* JR */
	THIS_->playerrom[0x09] = 0xfb;	/* -5 */
	THIS_->playerrom[0x0a] = 0xcd;	/* CALL */
	THIS_->playerrom[0x0b] = ((THIS_->playerromioaddr + 0x0f) >> 0) & 0xFF;
	THIS_->playerrom[0x0c] = ((THIS_->playerromioaddr + 0x0f) >> 8) & 0xFF;
	THIS_->playerrom[0x0d] = 0x18;	/* JR */
	THIS_->playerrom[0x0e] = 0xfb;	/* -5 */
	THIS_->playerrom[0x0f] = 0xcd;	/* CALL */
	THIS_->playerrom[0x10] = ((THIS_->playerromioaddr + 0x14) >> 0) & 0xFF;
	THIS_->playerrom[0x11] = ((THIS_->playerromioaddr + 0x14) >> 8) & 0xFF;
	THIS_->playerrom[0x12] = 0x18;	/* JR */
	THIS_->playerrom[0x13] = 0xfb;	/* -5 */
	THIS_->playerrom[0x14] = 0xcd;	/* CALL */
	THIS_->playerrom[0x15] = ((THIS_->playerromioaddr + 0x18) >> 0) & 0xFF;
	THIS_->playerrom[0x16] = ((THIS_->playerromioaddr + 0x18) >> 8) & 0xFF;
	THIS_->playerrom[0x17] = 0x18;	/* JR */
	THIS_->playerrom[0x18] = 0xfb;	/* -5 & EI */
	THIS_->playerrom[0x19] = 0x00;	/* NOP */
	THIS_->playerrom[0x1a] = 0x76;	/* HALT */
	THIS_->playerrom[0x1b] = 0x18;	/* JR */
	THIS_->playerrom[0x1c] = 0xfb;	/* -5 */
#else
	/*
		TEKKA.GBR対策 (PLAYからのRET前に余分なPOP) Ver.2
		  割り込み禁止(Wiz外伝対策)
	*/
	THIS_->playerrom[0x05] = 0xf3;	/* DI */
	THIS_->playerrom[0x06] = 0xcd;	/* CALL */
	THIS_->playerrom[0x07] = ((THIS_->playerromioaddr + 0x0e) >> 0) & 0xFF;
	THIS_->playerrom[0x08] = ((THIS_->playerromioaddr + 0x0e) >> 8) & 0xFF;
	THIS_->playerrom[0x09] = 0x31;	/* LD SP,nn */
	THIS_->playerrom[0x0a] = (THIS_->stackaddr >> 0) & 0xFF;
	THIS_->playerrom[0x0b] = (THIS_->stackaddr >> 8) & 0xFF;
	THIS_->playerrom[0x0c] = 0x18;	/* JR */
	THIS_->playerrom[0x0d] = 0xf7;	/* -9 */
	THIS_->playerrom[0x0e] = 0xf5;	/* PUSH AF */
	THIS_->playerrom[0x0f] = 0x7d;	/* LD A,L */
	THIS_->playerrom[0x10] = 0xea;	/* LD (nn),A */
	THIS_->playerrom[0x11] = ((THIS_->playerromioaddr + 0x1e) >> 0) & 0xFF;
	THIS_->playerrom[0x12] = ((THIS_->playerromioaddr + 0x1e) >> 8) & 0xFF;
	THIS_->playerrom[0x13] = 0x7c;	/* LD A,H */
	THIS_->playerrom[0x14] = 0xea;	/* LD (nn),A */
	THIS_->playerrom[0x15] = ((THIS_->playerromioaddr + 0x20) >> 0) & 0xFF;
	THIS_->playerrom[0x16] = ((THIS_->playerromioaddr + 0x20) >> 8) & 0xFF;
	THIS_->playerrom[0x17] = 0xf1;	/* POP AF */
	THIS_->playerrom[0x18] = 0xe1;	/* POP HL */
	THIS_->playerrom[0x19] = 0xe5;	/* PUSH HL */
	THIS_->playerrom[0x1a] = 0xe5;	/* PUSH HL */
	THIS_->playerrom[0x1b] = 0xe5;	/* PUSH HL */
	THIS_->playerrom[0x1c] = 0xe5;	/* PUSH HL */
	THIS_->playerrom[0x1d] = 0x2e;	/* LD L,n */
	THIS_->playerrom[0x1e] = 0x00;
	THIS_->playerrom[0x1f] = 0x26;	/* LD H,n */
	THIS_->playerrom[0x20] = 0x00;
	THIS_->playerrom[0x21] = 0xfb;	/* EI */
	THIS_->playerrom[0x22] = 0x00;	/* NOP */
	THIS_->playerrom[0x23] = 0x76;	/* HALT */
	THIS_->playerrom[0x24] = 0x18;	/* JR */
	THIS_->playerrom[0x25] = 0xfb;	/* -5 */
#endif
#else
#if GBS2GB_EMULATION
	/*
		再生アドレスに飛んだときのレジスタの値を、GBS2GBに合わせるテスト
	*/
	THIS_->playerrom[0x05] = 0x21;	/* LD HL,0c03 */
	THIS_->playerrom[0x06] = 0x03;	
	THIS_->playerrom[0x07] = 0x0c;	
	THIS_->playerrom[0x08] = 0x11;	/* LD DE,0001 */
	THIS_->playerrom[0x09] = 0x01;	
	THIS_->playerrom[0x0A] = 0x00;	
	THIS_->playerrom[0x0B] = 0x01;	/* LD BC,0080 */
	THIS_->playerrom[0x0C] = 0x80;	
	THIS_->playerrom[0x0D] = 0x00;	
	THIS_->playerrom[0x0E] = 0x3e;	/* LD A,3 */
	THIS_->playerrom[0x0F] = 0x03;	
	THIS_->playerrom[0x10] = 0xFB;	/* EI */
	THIS_->playerrom[0x11] = 0x00;	/* NOP */
	THIS_->playerrom[0x12] = 0x76;	/* HALT */
	THIS_->playerrom[0x13] = 0x18;	/* JR */
	THIS_->playerrom[0x14] = 0xf0;	/* -5 */
#else
	THIS_->playerrom[0x05] = 0xFB;	/* EI */
//	THIS_->playerrom[0x06] = 0x00;	/* NOP */
	THIS_->playerrom[0x06] = 0x76;	/* HALT */
	THIS_->playerrom[0x07] = 0x18;	/* JR */
	THIS_->playerrom[0x08] = 0xfc;	/* -5 */
	THIS_->playerrom[0x09] = 0xc9;	/* -5 */
#endif
#endif
	THIS_->ctx.pc = THIS_->playerromioaddr;
	THIS_->ctx.sp = THIS_->stackaddr;
	THIS_->ctx.regs8[REGID_IFF1] = THIS_->ctx.regs8[REGID_IFF2] = 0;

	THIS_->ctx.regs8[REGID_INTREQ] = THIS_->gb_IF = 0;
	THIS_->ctx.rstbase = THIS_->loadaddr;

	THIS_->ctx.regs8[REGID_INTMASK] = THIS_->gb_IE = THIS_->timerflag;
	THIS_->gb_TMA = THIS_->firstTMA;
	THIS_->gb_TMC = THIS_->firstTMC;
	THIS_->gb_TIMA = THIS_->gb_TMA;


	/* request execute */
	initbreak = 5 << 8;
	while (THIS_->ctx.pc != THIS_->playerromioaddr + 3 && --initbreak) wkmz80_exec(&THIS_->ctx, DMG_BASECYCLES >> 8);
	THIS_->playerrom[4] = 0x00;	/* -2 */

	/* timer enable */
	/* nemesis1/2, last bible1/2 */
	if (THIS_->timerflag & 4) THIS_->gb_TMC |= 4;

	vsync_setup(THIS_);
	timer_setup(THIS_);
	/* THIS_->gb_TIMA = THIS_->gb_TMA; */
	/* THIS_->gb_IE = THIS_->timerflag; */

	THIS_->total_cycles = 0;

	//ここからダンプ設定
	pNezPlayDump = pNezPlay;
	dump_MEM_GB  = dump_MEM_GB_bf;
	dump_DEV_DMG = dump_DEV_DMG_bf;
	//ここまでダンプ設定

}

static void terminate(GBRDMG *THIS_)
{
	//ここからダンプ設定
	dump_MEM_GB  = NULL;
	dump_DEV_DMG = NULL;
	//ここまでダンプ設定
	if (THIS_->dmgsnd) THIS_->dmgsnd->release(THIS_->dmgsnd->ctx);
	if (THIS_->bankrom) XFREE(THIS_->bankrom);
	XFREE(THIS_);
}

static Uint32 GetWordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8);
}
static void validateText(char *txt, int len) {
	for (int i= 0; i<len; i++) {
		if (!txt[i]) break;
		if (txt[i] & 0x80) {
			txt[0]= 0;	// non ASCII garbage
			break;
		}
	}		
}
static Uint32 load(NEZ_PLAY *pNezPlay, GBRDMG *THIS_, Uint8 *pData, Uint32 uSize)
{

	XMEMSET(THIS_, 0, sizeof(GBRDMG));
	THIS_->dmgsnd = 0;
	THIS_->bankrom = 0;
	if (pData[0] != 0x47 || pData[1] != 0x42) return NESERR_FORMAT;
	if (pData[2] == 0x52)
	{
		if (pData[3] != 0x46) return NESERR_FORMAT;
		THIS_->isgbr = 1;	/* 'GBRF' GameBoy Ripped sound Format */
	}
	else if (pData[2] == 0x53)
		THIS_->isgbr = 0;	/* 'GBS' GameBoy Sound format */
	else
		return NESERR_FORMAT;

	if (THIS_->isgbr)
	{
		if (uSize < 0x20) return NESERR_FORMAT;
		THIS_->maxsong = 256;
		THIS_->startsong = 1;
		SONGINFO_SetStartSongNo(pNezPlay->song, THIS_->startsong);
		SONGINFO_SetMaxSongNo(pNezPlay->song, THIS_->maxsong);
		THIS_->bankromnum = pData[4];
		if (!THIS_->bankromnum) return NESERR_FORMAT;
		THIS_->bankromfirst[0] = pData[5];
		THIS_->bankromfirst[1] = pData[6];
		THIS_->timerflag = 0;
		if (pData[7] & 1) THIS_->timerflag |= 1;
		if (pData[7] & 2) THIS_->timerflag |= 4;
		THIS_->loadaddr = 0;
		THIS_->initaddr = GetWordLE(pData + 0x08);
		THIS_->vsyncaddr = GetWordLE(pData + 0x0a);
		THIS_->timeraddr = GetWordLE(pData + 0x0c);
#if 1
		THIS_->stackaddr = 0x9fc0;	/* vram */
#else
		/*  Dr.Mario 再生不可 */
		/*  JUDGE RED 再生不可 */
		THIS_->stackaddr = 0xe000;	/* intrnal ram */
#endif
		THIS_->firstTMA = pData[0x0e];
		THIS_->firstTMC = pData[0x0f] & 0x7f;
		//ヘッダ0FHの最上位ビットが立っていて、その上位４ビットが全部立っていないならGBC
		//上位４ビットが全部立っている = タイマー未使用ビットを全部立たせてる = GBCとは限らない
		THIS_->isCGB = (pData[0x0f] & 0x80) && (pData[0x0f] & 0xf0) != 0xf0 ? 1 : 0;
		THIS_->mapper_type = GB_MAPPER_GBR;
		uSize -= 0x20;
		pData += 0x20;
		if (uSize > ((Uint32)THIS_->bankromnum) << 14)
			uSize = ((Uint32)THIS_->bankromnum) << 14;

		XMEMSET(THIS_->titlebuffer, 0, 0x21);
		XMEMCPY(THIS_->titlebuffer, pData + 0x0134, 0x10);		
		validateText(THIS_->titlebuffer, 0x10);
		songinfodata.title=THIS_->titlebuffer;
		SONGINFO_SetTitle(pNezPlay->song, songinfodata.title);

		XMEMSET(THIS_->artistbuffer, 0, 0x21);
		songinfodata.artist=THIS_->artistbuffer;
		SONGINFO_SetArtist(pNezPlay->song, songinfodata.artist);

		XMEMSET(THIS_->copyrightbuffer, 0, 0x21);
		songinfodata.copyright=THIS_->copyrightbuffer;
		SONGINFO_SetCopyright(pNezPlay->song, songinfodata.copyright);
		sprintf(songinfodata.detail,
#ifdef EMSCRIPTEN
"Type               : GBRF<br>\r\n\
Bank ROM Num       : %XH<br>\r\n\
Bank ROM(0000-3FFF): %XH<br>\r\n\
Bank ROM(4000-7FFF): %XH<br>\r\n\
Init Address       : %04XH<br>\r\n\
VSync Address      : %04XH<br>\r\n\
Timer Address      : %04XH<br>\r\n\
Interrupt Flag     : %s%s<br>\r\n\
GBC Flag(2x Speed) : %d<br>\r\n\
First TMA          : %02XH<br>\r\n\
First TMC          : %02XH"
#else
"Type               : GBRF\r\n\
Bank ROM Num       : %XH\r\n\
Bank ROM(0000-3FFF): %XH\r\n\
Bank ROM(4000-7FFF): %XH\r\n\
Init Address       : %04XH\r\n\
VSync Address      : %04XH\r\n\
Timer Address      : %04XH\r\n\
Interrupt Flag     : %s%s\r\n\
GBC Flag(2x Speed) : %d\r\n\
First TMA          : %02XH\r\n\
First TMC          : %02XH"
#endif
			,THIS_->bankromnum,THIS_->bankromfirst[0],THIS_->bankromfirst[1],THIS_->initaddr
			,THIS_->vsyncaddr,THIS_->timeraddr
			,(THIS_->timerflag&1) ? "VSync " : "" , (THIS_->timerflag>>2) ? "Timer " : ""
			,THIS_->isCGB,THIS_->firstTMA,THIS_->firstTMC);
		
		SONGINFO_SetDetail(pNezPlay->song, songinfodata.detail);	// EMSCRIPTEN
		
	}
	else
	{
		Uint32 size;
		if (uSize < 0x70) return NESERR_FORMAT;
		THIS_->maxsong = pData[0x04];
		THIS_->startsong = pData[0x05];
		SONGINFO_SetStartSongNo(pNezPlay->song, THIS_->startsong);
		SONGINFO_SetMaxSongNo(pNezPlay->song, THIS_->maxsong);
		THIS_->bankromfirst[0] = 0;
		THIS_->bankromfirst[1] = 1;
		THIS_->loadaddr = GetWordLE(pData + 0x06);
		THIS_->initaddr = GetWordLE(pData + 0x08);
		//--- - [changes_rough.txt]
		//THIS_->vsyncaddr = GetWordLE(pData + 0x0a);
		//THIS_->timeraddr = GetWordLE(pData + 0x0a);
		//THIS_->stackaddr = GetWordLE(pData + 0x0c);
		//THIS_->timerflag = (pData[0x0f] & 4) ? 4 : 1;
		//--- -

		//---+ [changes_rough.txt]
		
		//なんか心配な時用。上位４ビットが全部立っていないなら有効としておく。（ときメモ辺りとか）
		//THIS_->useINT = ((pData[0x0f] & 0x44) == 0x44 && (pData[0x0f] & 0xf0) != 0xf0)? 1 : 0;

		//言われるがままにソースを修正したが、一部のGBSが再生されなくなったのはこの部分のせい
		THIS_->useINT = ((pData[0x0f] & 0x44) == 0x44 )? 1 : 0;
		
		if (THIS_->useINT & 1) {
			THIS_->timerflag = 0x05; /* Timer + VBlank */
			THIS_->vsyncaddr = THIS_->loadaddr + 0x40; //GetWordLE(pData + 0x0a);
			THIS_->timeraddr = THIS_->loadaddr + 0x48; //GetWordLE(pData + 0x0a);
			THIS_->playaddr = GetWordLE(pData + 0x0a);
		}
		else{
			THIS_->timerflag = (pData[0x0f] & 4) ? 4 : 1;
			THIS_->vsyncaddr = (pData[0x0f] & 4) ? 0x9fc9 : GetWordLE(pData + 0x0a);
			THIS_->timeraddr = (pData[0x0f] & 4) ? GetWordLE(pData + 0x0a) : 0x9fc9;
		}
		THIS_->stackaddr = GetWordLE(pData + 0x0c);
		//---+
		THIS_->firstTMA = pData[0x0e];
		THIS_->firstTMC = pData[0x0f] & 7;
		THIS_->isCGB = (pData[0x0f] & 0x80) ? 1 : 0;
		THIS_->mapper_type = GB_MAPPER_MBC1;

		XMEMSET(THIS_->titlebuffer, 0, 0x21);
		XMEMCPY(THIS_->titlebuffer, pData + 0x0010, 0x20);
		songinfodata.title=THIS_->titlebuffer;
		SONGINFO_SetTitle(pNezPlay->song, songinfodata.title);
 
		XMEMSET(THIS_->artistbuffer, 0, 0x21);
		XMEMCPY(THIS_->artistbuffer, pData + 0x0030, 0x20);
		songinfodata.artist=THIS_->artistbuffer;
		SONGINFO_SetArtist(pNezPlay->song, songinfodata.artist);

		XMEMSET(THIS_->copyrightbuffer, 0, 0x21);
		XMEMCPY(THIS_->copyrightbuffer, pData + 0x0050, 0x20);
		songinfodata.copyright=THIS_->copyrightbuffer;
		SONGINFO_SetCopyright(pNezPlay->song, songinfodata.copyright);
		sprintf(songinfodata.detail,
"Type              : GBS\r\n\
Song Max          : %d\r\n\
Start Song        : %d\r\n\
Load Address      : %04XH\r\n\
Init Address      : %04XH\r\n\
Play Address      : %04XH\r\n\
Stack Address     : %04XH\r\n\
Timer Mode        : %d\r\n\
First TMA         : %02XH\r\n\
First TMC         : %02XH\r\n\
GBC Mode(2x Speed): %d\r\n\
Use INT           : %d"
			,THIS_->maxsong,THIS_->startsong,THIS_->loadaddr,THIS_->initaddr
			,THIS_->vsyncaddr,THIS_->stackaddr,THIS_->timerflag>>2,THIS_->firstTMA
			,THIS_->firstTMC,THIS_->isCGB,THIS_->useINT);
		
		uSize -= 0x70;
		pData += 0x70;
		size = uSize + THIS_->loadaddr;
		if (size < 0x8000) size = 0x8000;
		size = (size + 0x3fff) & ~ 0x3fff;
		THIS_->bankromnum = (Uint8)(size >> 14);
		
	}
	SONGINFO_SetChannel(pNezPlay->song, 2);
	SONGINFO_SetExtendDevice(pNezPlay->song, 0);
	SONGINFO_SetInitAddress(pNezPlay->song, THIS_->initaddr);
	//---+
	if (THIS_->useINT == 1)
	SONGINFO_SetPlayAddress(pNezPlay->song, THIS_->playaddr);
	else
	//---+
	SONGINFO_SetPlayAddress(pNezPlay->song, (THIS_->timerflag & 1) ? THIS_->vsyncaddr : THIS_->timeraddr);
	THIS_->bankrom = (Uint8 *)XMALLOC(THIS_->bankromnum << 14);
	if (!THIS_->bankrom) return NESERR_SHORTOFMEMORY;
	XMEMSET(THIS_->bankrom, 0, THIS_->bankromnum << 14);
#if MARIO2_PATCH_ENABLE
	/*
		MARIO2.GBR(SUPER MARIO LAND)対策 (ファイルサイズ縮小のため#0と#1を交換)
	*/
	if (THIS_->bankromnum == 2 && THIS_->bankromfirst[0] == 1 && THIS_->bankromfirst[1] == 0)
	{
		/* 4000-7FFFには#0は選択できないので本来は不正 */
		if (uSize > 0x4000) XMEMCPY(THIS_->bankrom + 0x0000, pData + 0x4000, uSize - 0x4000);
		XMEMCPY(THIS_->bankrom + 0x4000, pData + 0x0000, (uSize > 0x4000) ? 0x4000 : uSize);
		THIS_->bankromfirst[0] = 0;
		THIS_->bankromfirst[1] = 1;
	}
	else
	{
		XMEMCPY(THIS_->bankrom + THIS_->loadaddr, pData, uSize);
	}
#else
	XMEMCPY(THIS_->bankrom + THIS_->loadaddr, pData, uSize);
#endif
	THIS_->dmgsnd = DMGSoundAlloc();
	if (!THIS_->dmgsnd) return NESERR_SHORTOFMEMORY;
	return NESERR_NOERROR;
}





static Int32 __fastcall ExecuteDMGCPU(void *pNezPlay)
{
	return ((NEZ_PLAY*)pNezPlay)->gbrdmg ? execute((GBRDMG*)((NEZ_PLAY*)pNezPlay)->gbrdmg) : 0;
}

static void __fastcall DMGSoundRenderStereo(void *pNezPlay, Int32 *d)
{
	synth((GBRDMG*)((NEZ_PLAY*)pNezPlay)->gbrdmg, d);
}

static Int32 __fastcall DMGSoundRenderMono(void *pNezPlay)
{
	Int32 d[2] = { 0,0 } ;
	synth((GBRDMG*)((NEZ_PLAY*)pNezPlay)->gbrdmg, d);
#if (((-1) >> 1) == -1)
	return (d[0] + d[1]) >> 1;
#else
	return (d[0] + d[1]) / 2;
#endif
}

const static NES_AUDIO_HANDLER gbrdmg_audio_handler[] = {
	{ 0, ExecuteDMGCPU, 0, },
	{ 3, DMGSoundRenderMono, DMGSoundRenderStereo },
	{ 0, 0, 0, },
};

static void __fastcall GBRDMGVolume(void *pNezPlay, Uint32 v)
{
	if (((NEZ_PLAY*)pNezPlay)->gbrdmg)
	{
		volume((GBRDMG*)((NEZ_PLAY*)pNezPlay)->gbrdmg, v);
	}
}

const static NES_VOLUME_HANDLER gbrdmg_volume_handler[] = {
	{ GBRDMGVolume, }, 
	{ 0, }, 
};

static void __fastcall GBRDMGCPUReset(void *pNezPlay)
{
	if (((NEZ_PLAY*)pNezPlay)->gbrdmg) reset((NEZ_PLAY*)pNezPlay);
}

const static NES_RESET_HANDLER gbrdmg_reset_handler[] = {
	{ NES_RESET_SYS_LAST, GBRDMGCPUReset, },
	{ 0,                  0, },
};

static void __fastcall GBRDMGCPUTerminate(void *pNezPlay)
{
	if (((NEZ_PLAY*)pNezPlay)->gbrdmg)
	{
		terminate((GBRDMG*)((NEZ_PLAY*)pNezPlay)->gbrdmg);
		((NEZ_PLAY*)pNezPlay)->gbrdmg = 0;
	}
}

const static NES_TERMINATE_HANDLER gbrdmg_terminate_handler[] = {
	{ GBRDMGCPUTerminate, },
	{ 0, },
};

Uint32 GBRLoad(NEZ_PLAY *pNezPlay, Uint8 *pData, Uint32 uSize)
{
	Uint32 ret;
	GBRDMG *THIS_;

	THIS_ = (GBRDMG *)XMALLOC(sizeof(GBRDMG));
	if (!THIS_) return NESERR_SHORTOFMEMORY;
	ret = load(pNezPlay, THIS_, pData, uSize);
	if (ret)
	{
		terminate(THIS_);
		return ret;
	}
	pNezPlay->gbrdmg = THIS_;
	NESAudioHandlerInstall(pNezPlay, gbrdmg_audio_handler);
	NESVolumeHandlerInstall(pNezPlay, gbrdmg_volume_handler);
	NESResetHandlerInstall(pNezPlay->nrh, gbrdmg_reset_handler);
	NESTerminateHandlerInstall(&pNezPlay->nth, gbrdmg_terminate_handler);
	return ret;
}
