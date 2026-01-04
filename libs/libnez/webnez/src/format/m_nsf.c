/* NSF mapper(4KBx8) */

#include "neserr.h"
#include "handler.h"
#include "audiosys.h"
#include "songinfo.h"
#include "nsf6502.h"
#include "device/nes/s_apu.h"
#include "device/nes/s_mmc5.h"
#include "device/nes/s_vrc6.h"
#include "device/nes/s_vrc7.h"
#include "device/nes/s_fds.h"
#include "device/nes/s_n106.h"
#include "device/nes/s_fme7.h"
#include <stdio.h>

#define NSF_MAPPER_STATIC 0
#include "m_nsf.h"

#define EXTSOUND_VRC6	(1 << 0)
#define EXTSOUND_VRC7	(1 << 1)
#define EXTSOUND_FDS	(1 << 2)
#define EXTSOUND_MMC5	(1 << 3)
#define EXTSOUND_N106	(1 << 4)
#define EXTSOUND_FME7	(1 << 5)
#define EXTSOUND_J86	(1 << 6)	/* JALECO-86 */

static struct {
	char* title;
	char* artist;
	char* copyright;
	char detail[1024];
}songinfodata;
static Uint8 titlebuffer[0x21];
static Uint8 artistbuffer[0x21];
static Uint8 copyrightbuffer[0x21];

/* RAM area */
static Uint32 __fastcall ReadRam(void *pNezPlay, Uint32 A)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->ram[A & 0x07FF];
}
static void __fastcall WriteRam(void *pNezPlay, Uint32 A, Uint32 V)
{
	((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->ram[A & 0x07FF] = (Uint8)V;
}

/* SRAM area */
static Uint32 __fastcall ReadStaticArea(void *pNezPlay, Uint32 A)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->static_area[A - 0x6000];
}
static void __fastcall WriteStaticArea(void *pNezPlay, Uint32 A, Uint32 V)
{
	((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->static_area[A - 0x6000] = (Uint8)V;
}

#if !NSF_MAPPER_STATIC
static Uint32 __fastcall ReadRom8000(void *pNezPlay, Uint32 A)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[0][A];
}
static Uint32 __fastcall ReadRom9000(void *pNezPlay, Uint32 A)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[1][A];
}
static Uint32 __fastcall ReadRomA000(void *pNezPlay, Uint32 A)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[2][A];
}
static Uint32 __fastcall ReadRomB000(void *pNezPlay, Uint32 A)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[3][A];
}
static Uint32 __fastcall ReadRomC000(void *pNezPlay, Uint32 A)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[4][A];
}
static Uint32 __fastcall ReadRomD000(void *pNezPlay, Uint32 A)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[5][A];
}
static Uint32 __fastcall ReadRomE000(void *pNezPlay, Uint32 A)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[6][A];
}
static Uint32 __fastcall ReadRomF000(void *pNezPlay, Uint32 A)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[7][A];
}

// ROMに値を書き込むのはできないはずで不正なのだが、
// これをしないとFDS環境のNSFが動作しないのでしょうがない…

static void __fastcall WriteRom8000(void *pNezPlay, Uint32 A, Uint32 V)
{
	((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[0][A] = (Uint8)V;
}
static void __fastcall WriteRom9000(void *pNezPlay, Uint32 A, Uint32 V)
{
	((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[1][A] = (Uint8)V;
}
static void __fastcall WriteRomA000(void *pNezPlay, Uint32 A, Uint32 V)
{
	((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[2][A] = (Uint8)V;
}
static void __fastcall WriteRomB000(void *pNezPlay, Uint32 A, Uint32 V)
{
	((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[3][A] = (Uint8)V;
}
static void __fastcall WriteRomC000(void *pNezPlay, Uint32 A, Uint32 V)
{
	((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[4][A] = (Uint8)V;
}
static void __fastcall WriteRomD000(void *pNezPlay, Uint32 A, Uint32 V)
{
	((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[5][A] = (Uint8)V;
}
static void __fastcall WriteRomE000(void *pNezPlay, Uint32 A, Uint32 V)
{
	((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[6][A] = (Uint8)V;
}
static void __fastcall WriteRomF000(void *pNezPlay, Uint32 A, Uint32 V)
{
	((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->bank[7][A] = (Uint8)V;
}
#endif

/* Mapper I/O */
static void __fastcall WriteMapper(void *pNezPlay, Uint32 A, Uint32 V)
{
	Uint32 bank;
	NSFNSF *nsf = ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf);
	bank = A - 0x5FF0;
	if (bank < 6 || bank > 15) return;
#if !NSF_MAPPER_STATIC
	if (bank >= 8)
	{
		if (V < nsf->banknum)
		{
			nsf->bank[bank - 8] = &nsf->bankbase[V << 12] - (bank << 12);
		}
		else
			nsf->bank[bank - 8] = nsf->zero_area - (bank << 12);
		return;
	}
#endif
	if (V < nsf->banknum)
	{
		XMEMCPY(&nsf->static_area[(bank - 6) << 12], &nsf->bankbase[V << 12], 0x1000);
	}
	else
		XMEMSET(&nsf->static_area[(bank - 6) << 12], 0x00, 0x1000);
}

static Uint32 __fastcall Read2000(void *pNezPlay, Uint32 A)
{
	NSFNSF *nsf = ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf);
	switch(A & 0x7){
	case 0:
		return 0xff;
	case 1:
		return 0xff;
	case 2:
		return nsf->counter2002++ ? 0x7f : 0xff;
	case 3:
		return 0xff;
	case 4:
		return 0xff;
	case 5:
		return 0xff;
	case 6:
		return 0xff;
	case 7:
		return 0xff;
	}
	return 0xff;
}


static NES_READ_HANDLER nsf_mapper_read_handler[] = {
	{ 0x0000,0x0FFF,ReadRam, },
	{ 0x1000,0x1FFF,ReadRam, },
	{ 0x2000,0x2007,Read2000, },
	{ 0x6000,0x6FFF,ReadStaticArea, },
	{ 0x7000,0x7FFF,ReadStaticArea, },
#if !NSF_MAPPER_STATIC
	{ 0x8000,0x8FFF,ReadRom8000, },
	{ 0x9000,0x9FFF,ReadRom9000, },
	{ 0xA000,0xAFFF,ReadRomA000, },
	{ 0xB000,0xBFFF,ReadRomB000, },
	{ 0xC000,0xCFFF,ReadRomC000, },
	{ 0xD000,0xDFFF,ReadRomD000, },
	{ 0xE000,0xEFFF,ReadRomE000, },
	{ 0xF000,0xFFFF,ReadRomF000, },
#else
	{ 0x8000,0x8FFF,ReadStaticArea, },
	{ 0x9000,0x9FFF,ReadStaticArea, },
	{ 0xA000,0xAFFF,ReadStaticArea, },
	{ 0xB000,0xBFFF,ReadStaticArea, },
	{ 0xC000,0xCFFF,ReadStaticArea, },
	{ 0xD000,0xDFFF,ReadStaticArea, },
	{ 0xE000,0xEFFF,ReadStaticArea, },
	{ 0xF000,0xFFFF,ReadStaticArea, },
#endif
	{ 0     ,0     ,0, },
};
static NES_WRITE_HANDLER nsf_mapper_write_handler[] = {
	{ 0x0000,0x0FFF,WriteRam, },
	{ 0x1000,0x1FFF,WriteRam, },
	{ 0x6000,0x6FFF,WriteStaticArea, },
	{ 0x7000,0x7FFF,WriteStaticArea, },
	{ 0     ,0     ,0, },
};
static NES_WRITE_HANDLER nsf_mapper_write_handler_fds[] = {
	{ 0x0000,0x0FFF,WriteRam, },
	{ 0x1000,0x1FFF,WriteRam, },
	{ 0x6000,0x6FFF,WriteStaticArea, },
	{ 0x7000,0x7FFF,WriteStaticArea, },
	{ 0x8000,0x8FFF,WriteRom8000, },
	{ 0x9000,0x9FFF,WriteRom9000, },
	{ 0xA000,0xAFFF,WriteRomA000, },
	{ 0xB000,0xBFFF,WriteRomB000, },
	{ 0xC000,0xCFFF,WriteRomC000, },
	{ 0xD000,0xDFFF,WriteRomD000, },
	{ 0xE000,0xEFFF,WriteRomE000, },
	{ 0xF000,0xFFFF,WriteRomF000, },
	{ 0     ,0     ,0, },
};
static NES_WRITE_HANDLER nsf_mapper_write_handler2[] = {
	{ 0x5FF6,0x5FFF,WriteMapper, },
	{ 0     ,0     ,0, },
};

Uint8 *NSFGetHeader(NEZ_PLAY *pNezPlay)
{
	return ((NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf)->head;
}

static Uint GetWordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8);
}

static void __fastcall ResetBank(void *pNezPlay)
{
	Uint i, startbank;
	NSFNSF *nsf = (NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf;
	XMEMSET(nsf->ram, 0, 0x0800);
	startbank = GetWordLE(nsf->head + 8) >> 12;
	for (i = 0; i < 16; i++)
		WriteMapper(pNezPlay, 0x5FF0 + i, 0x10000/* off */);
	if (nsf->banksw)
	{
		for (i = 0; (startbank + i) < 8; i++)
			WriteMapper(pNezPlay, 0x5FF0 + (startbank + i) , i);
		if (nsf->banksw & 2)
		{
			WriteMapper(pNezPlay, 0x5FF6, nsf->head[0x70 + 6]);
			WriteMapper(pNezPlay, 0x5FF7, nsf->head[0x70 + 7]);
		}
		for (i = 8; i < 16; i++)
			WriteMapper(pNezPlay, 0x5FF0 + i, nsf->head[0x70 + i - 8]);
	}
	else
	{
		for (i = startbank; i < 16; i++)
			WriteMapper(pNezPlay, 0x5FF0 + i , i - startbank);
	}
}

const static NES_RESET_HANDLER nsf_mapper_reset_handler[] = {
	{ NES_RESET_SYS_FIRST, ResetBank, },
	{ 0,                   0, },
};

//ここからダンプ設定
static NEZ_PLAY *pNezPlayDump;
Uint32 (*dump_MEM_FC)(Uint32 a,unsigned char* mem);
static Uint32 dump_MEM_FC_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Memory
		for(i=0;i<0x10000;i++)
			mem[i] = i<0x2000||i>0x2fff ? NES6502Read(pNezPlayDump, i):0;
		return i;
	}
	return -2;
}
//----------
extern Uint8 *regdata_2a03;
Uint32 (*dump_DEV_2A03)(Uint32 a,unsigned char* mem);
const Uint8 *BASE64="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static Uint32 dump_DEV_2A03_bf(Uint32 menu,unsigned char* mem){
	int i,j,k,l,adr;Uint32 ac;
	unsigned char membf[0x1000];
	switch(menu){
	case 1://Register
		for(i=0;i<=0x17;i++)
			mem[i] = regdata_2a03[i];
		return i;
	case 2://DPCM Data
		for(i=0,adr=regdata_2a03[0x12]*0x40L+0xc000;i<regdata_2a03[0x13]*0x10L+1;i++){
			mem[i] = NES6502Read(pNezPlayDump, adr);
			adr++;if(adr>0xffff)adr=0x8000;
		}
		return i;
	case 10://*DPCM Data to FlMML
		for(i=0,adr=regdata_2a03[0x12]*0x40L+0xc000;i<regdata_2a03[0x13]*0x10L+1;i++){
			membf[i] = NES6502Read(pNezPlayDump, adr);
			adr++;if(adr>0xffff)adr=0x8000;
		}
		k=sprintf(mem,"#WAV9 [n],%d,%d,",regdata_2a03[0x11]&0x7f,(regdata_2a03[0x10]&0x40)?1:0);
		for(j=0;j<i;j+=3){
			ac=0;l=0;
			if(j+0<i){ac|=membf[j+0]<<16;l+=2;}
			if(j+1<i){ac|=membf[j+1]<< 8;l+=1;}
			if(j+2<i){ac|=membf[j+2]<< 0;l+=1;}
			l--; mem[k] = l>=0 ? BASE64[(ac>>18)&0x3f] : '=';k++;
			l--; mem[k] = l>=0 ? BASE64[(ac>>12)&0x3f] : '=';k++;
			l--; mem[k] = l>=0 ? BASE64[(ac>> 6)&0x3f] : '=';k++;
			l--; mem[k] = l>=0 ? BASE64[(ac>> 0)&0x3f] : '=';k++;
		}
		mem[k]=0;
		return -1;
	}
	return -2;
}
//----------
extern Uint8 *fds_regdata;
extern Uint8 *fds_regdata2;
extern Uint8 *fds_regdata3;
extern int FDS_RealMode;

#define FDSOUT(x) ((x*4+(((x&(1<<0))>>0)+((x&(1<<1))>>1)+((x&(1<<2))>>2)+((x&(1<<3))>>3)+((x&(1<<4))>>4)+((x&(1<<5))>>5))*(0+(x*3)/0x5)))
#define REGREAD(y) ((int)FDSOUT(fds_regdata2[(y)&0x3f]))
#define REGREAD2(y) ((int)fds_regdata2[(y)&0x3f]/4)

Uint32 (*dump_DEV_FDS)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_FDS_bf(Uint32 menu,unsigned char* mem){
	int i,j,k;
	static const char *hexstr="0123456789ABCDEF";

	switch(menu){
	case 1://Register
		for(i=0;i<0x10;i++)
			mem[i] = fds_regdata[i];
		return i;

	case 2://Wave Data
		for(i=0;i<0x40;i++)
			mem[i] = fds_regdata2[i];
		return i;

	case 3://Sweep Envelope Data
		for(i=0;i<0x20;i++)
			mem[i] = fds_regdata3[i*2];
		return i;

	case 8://*Wave Data to MCK
		for(i=0,j=0;i<0x40;i++){
			mem[j++] = 'x';
			mem[j++] = hexstr[fds_regdata2[i]>>4];
			mem[j++] = hexstr[fds_regdata2[i]&0xf];
			mem[j++] = ',';
			if((i&0xf)==0xf){
				mem[j++] = '\r';
				mem[j++] = '\n';
			}
		}
		mem[j++] = 0;
		return -1;

	case 9://*Wave Data to FlMML
		j=sprintf(mem,"#WAV13 [n],");
		for(i=0;i<0x40;i++){
			if(FDS_RealMode&0x2)k=(int)(FDSOUT(fds_regdata2[i])/1.8671876);
			else				k=fds_regdata2[i]<<2;
			mem[j++] = hexstr[k>>4];
			mem[j++] = hexstr[k&0xf];
		}
		mem[j++] = 0;
		return -1;

	case 10://*Wave Data to GBwav
		if(FDS_RealMode&0x2){
			for(i=0;i<0x10;i++)
				mem[i] = ((REGREAD(i*4+0)+REGREAD(i*4+1))/2/30)<<4
					   | ((REGREAD(i*4+2)+REGREAD(i*4+3))/2/30);
		}else{
			for(i=0;i<0x10;i++)
				mem[i] = ((REGREAD2(i*4+0)+REGREAD2(i*4+1))/2)<<4
					   | ((REGREAD2(i*4+2)+REGREAD2(i*4+3))/2);
		}
		return 0x10;
	}
	return -2;
}
//----------
extern Uint8 *mmc5_regdata;

Uint32 (*dump_DEV_MMC5)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_MMC5_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register
		for(i=0;i<=0x15;i++)
			mem[i] = mmc5_regdata[i];
		return i;

	}
	return -2;
}
//----------
extern Uint8 *vrc6_regdata;
extern Uint8 *vrc6_regdata2;
extern Uint8 *vrc6_regdata3;

Uint32 (*dump_DEV_VRC6)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_VRC6_bf(Uint32 menu,unsigned char* mem){
	int i;
	switch(menu){
	case 1://Register
		for(i=0;i<0x3;i++){
			mem[i+0] = vrc6_regdata[i];
			mem[i+3] = vrc6_regdata2[i];
			mem[i+6] = vrc6_regdata3[i];
		}
		return i*3;

	}
	return -2;
}
//----------
extern Uint8 *n106_regdata;

Uint32 (*dump_DEV_N106)(Uint32 a,unsigned char* mem);
static Uint32 dump_DEV_N106_bf(Uint32 menu,unsigned char* mem){
	int i,j,k,l;
	switch(menu){
	case 1://Register
		for(i=0;i<0x80;i++)
			mem[i] = n106_regdata[i];
		return i;
	case 2://Wave Data - CH1
	case 3://Wave Data - CH2
	case 4://Wave Data - CH3
	case 5://Wave Data - CH4
	case 6://Wave Data - CH5
	case 7://Wave Data - CH6
	case 8://Wave Data - CH7
	case 9://Wave Data - CH8
		j=(menu-2)*8;
		k=n106_regdata[(j+0x46)];
		for(i=0;i<0x80-((n106_regdata[(j+0x44)]&0xfc)>>1);i++){
			l = k&1 ? n106_regdata[k>>1]&0xf : n106_regdata[k>>1]&0xf0;k++;
			k&=0xff;
			l|= k&1 ? n106_regdata[k>>1]&0xf : n106_regdata[k>>1]&0xf0;k++;
			k&=0xff;
			mem[i] = l;
		}
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
//ここまでダンプ設定

static void __fastcall Terminate(void *pNezPlay)
{
	NSFNSF *nsf = (NSFNSF*)((NEZ_PLAY*)pNezPlay)->nsf;
	if (nsf)
	{
		//ここからダンプ設定
		dump_MEM_FC    = NULL;
		dump_DEV_2A03  = NULL;
		dump_DEV_FDS   = NULL;
		dump_DEV_MMC5  = NULL;
		dump_DEV_VRC6  = NULL;
		dump_DEV_N106  = NULL;
		dump_DEV_AY8910= NULL;
		dump_DEV_OPLL  = NULL;
		//ここまでダンプ設定
		if (nsf->bankbase)
		{
			XFREE(nsf->bankbase);
			nsf->bankbase = 0;
		}
		XFREE(nsf);
		((NEZ_PLAY*)pNezPlay)->nsf = 0;
	}
}

const static NES_TERMINATE_HANDLER nsf_mapper_terminate_handler[] = {
	{ Terminate, },
	{ 0, },
};

static Uint NSFMapperInitialize(NEZ_PLAY *pNezPlay, Uint8 *pData, Uint uSize)
{
	Uint32 size = uSize;
	Uint8 *data = pData;
	NSFNSF *nsf = (NSFNSF*)pNezPlay->nsf;

	size += GetWordLE(nsf->head + 8) & 0x0FFF;
	size  = (size + 0x0FFF) & ~0x0FFF;

	nsf->bankbase = XMALLOC(size + 8);
	if (!nsf->bankbase) return NESERR_SHORTOFMEMORY;

	nsf->banknum = size >> 12;
	XMEMSET(nsf->bankbase, 0, size);
	XMEMCPY(nsf->bankbase + (GetWordLE(nsf->head + 8) & 0x0FFF), data, uSize);

	nsf->banksw = nsf->head[0x70] || nsf->head[0x71] || nsf->head[0x72] || nsf->head[0x73] || nsf->head[0x74] || nsf->head[0x75] || nsf->head[0x76] || nsf->head[0x77];
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_FDS) nsf->banksw <<= 1;
	return NESERR_NOERROR;
}

Uint NSFDeviceInitialize(NEZ_PLAY *pNezPlay)
{
	NSFNSF *nsf = (NSFNSF*)pNezPlay->nsf;
	NESResetHandlerInstall(pNezPlay->nrh, nsf_mapper_reset_handler);
	NESTerminateHandlerInstall(&pNezPlay->nth, nsf_mapper_terminate_handler);
	NESReadHandlerInstall(pNezPlay, nsf_mapper_read_handler);
	//FDSのみをサポートしている場合のみ8000-DFFFも書き込み可能-
	if(pNezPlay->song->extdevice == 4){
		NESWriteHandlerInstall(pNezPlay, nsf_mapper_write_handler_fds);
	}else{
		NESWriteHandlerInstall(pNezPlay, nsf_mapper_write_handler);
	}

	if (nsf->banksw) NESWriteHandlerInstall(pNezPlay, nsf_mapper_write_handler2);

	XMEMSET(nsf->ram, 0, 0x0800);
	XMEMSET(nsf->static_area, 0, sizeof(nsf->static_area));
#if !NSF_MAPPER_STATIC
	XMEMSET(nsf->zero_area, 0, sizeof(nsf->zero_area));
#endif

	APUSoundInstall(pNezPlay);
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_VRC6) VRC6SoundInstall(pNezPlay);
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_VRC7) VRC7SoundInstall(pNezPlay);
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_FDS)  FDSSoundInstall(pNezPlay);
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_MMC5)
	{
		MMC5SoundInstall(pNezPlay);
		MMC5MutiplierInstall(pNezPlay);
		MMC5ExtendRamInstall(pNezPlay);
	}
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_N106) N106SoundInstall(pNezPlay);
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_FME7) FME7SoundInstall(pNezPlay);

	nsf->counter2002 = 0;	//暫定

	//ここからダンプ設定
	pNezPlayDump = pNezPlay;
	dump_MEM_FC = dump_MEM_FC_bf;
	dump_DEV_2A03 = dump_DEV_2A03_bf;
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_FDS)  dump_DEV_FDS   = dump_DEV_FDS_bf;
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_MMC5) dump_DEV_MMC5  = dump_DEV_MMC5_bf;
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_VRC6) dump_DEV_VRC6  = dump_DEV_VRC6_bf;
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_N106) dump_DEV_N106  = dump_DEV_N106_bf;
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_FME7) dump_DEV_AY8910= dump_DEV_AY8910_bf;
	if (SONGINFO_GetExtendDevice(pNezPlay->song) & EXTSOUND_VRC7) dump_DEV_OPLL  = dump_DEV_OPLL_bf;
	//ここまでダンプ設定
	return NESERR_NOERROR;
}


static void NSFMapperSetInfo(NEZ_PLAY *pNezPlay, Uint8 *pData)
{
	XMEMCPY(((NSFNSF*)pNezPlay->nsf)->head, pData, 0x80);
	SONGINFO_SetStartSongNo(pNezPlay->song, pData[0x07]);
	SONGINFO_SetMaxSongNo(pNezPlay->song, pData[0x06]);
	SONGINFO_SetExtendDevice(pNezPlay->song, pData[0x7B]);
	SONGINFO_SetInitAddress(pNezPlay->song, GetWordLE(pData + 0x0A));
	SONGINFO_SetPlayAddress(pNezPlay->song, GetWordLE(pData + 0x0C));
	SONGINFO_SetChannel(pNezPlay->song, 1);

	XMEMSET(titlebuffer, 0, 0x21);
	XMEMCPY(titlebuffer, pData + 0x000e, 0x20);
	songinfodata.title=titlebuffer;
	SONGINFO_SetTitle(pNezPlay->song, titlebuffer);

	XMEMSET(artistbuffer, 0, 0x21);
	XMEMCPY(artistbuffer, pData + 0x002e, 0x20);
	songinfodata.artist=artistbuffer;
	SONGINFO_SetArtist(pNezPlay->song, artistbuffer);

	XMEMSET(copyrightbuffer, 0, 0x21);
	XMEMCPY(copyrightbuffer, pData + 0x004e, 0x20);
	songinfodata.copyright=copyrightbuffer;
	SONGINFO_SetCopyright(pNezPlay->song, copyrightbuffer);
	sprintf(songinfodata.detail,
#ifdef EMSCRIPTEN
"Type          : NSF<br>\r\n\
Song Max      : %d<br>\r\n\
Start Song    : %d<br>\r\n\
Load Address  : %04XH<br>\r\n\
Init Address  : %04XH<br>\r\n\
Play Address  : %04XH<br>\r\n\
NTSC/PAL Mode : %s<br>\r\n\
NTSC Speed    : %04XH (%4.0fHz)<br>\r\n\
PAL Speed     : %04XH (%4.0fHz)<br>\r\n\
Extend Device : %s%s%s%s%s%s%s<br>\r\n\
<br>\r\n\
Set First ROM Bank                    : %d<br>\r\n\
First ROM Bank(8000-8FFF)             : %02XH<br>\r\n\
First ROM Bank(9000-9FFF)             : %02XH<br>\r\n\
First ROM Bank(A000-AFFF)             : %02XH<br>\r\n\
First ROM Bank(B000-BFFF)             : %02XH<br>\r\n\
First ROM Bank(C000-CFFF)             : %02XH<br>\r\n\
First ROM Bank(D000-DFFF)             : %02XH<br>\r\n\
First ROM Bank(E000-EFFF or 6000-6FFF): %02XH<br>\r\n\
First ROM Bank(F000-FFFF or 7000-7FFF): %02XH"
#else
"Type          : NSF\r\n\
Song Max      : %d\r\n\
Start Song    : %d\r\n\
Load Address  : %04XH\r\n\
Init Address  : %04XH\r\n\
Play Address  : %04XH\r\n\
NTSC/PAL Mode : %s\r\n\
NTSC Speed    : %04XH (%4.0fHz)\r\n\
PAL Speed     : %04XH (%4.0fHz)\r\n\
Extend Device : %s%s%s%s%s%s%s\r\n\
\r\n\
Set First ROM Bank                    : %d\r\n\
First ROM Bank(8000-8FFF)             : %02XH\r\n\
First ROM Bank(9000-9FFF)             : %02XH\r\n\
First ROM Bank(A000-AFFF)             : %02XH\r\n\
First ROM Bank(B000-BFFF)             : %02XH\r\n\
First ROM Bank(C000-CFFF)             : %02XH\r\n\
First ROM Bank(D000-DFFF)             : %02XH\r\n\
First ROM Bank(E000-EFFF or 6000-6FFF): %02XH\r\n\
First ROM Bank(F000-FFFF or 7000-7FFF): %02XH"
#endif
		,pData[0x06],pData[0x07],GetWordLE(pData + 0x08),GetWordLE(pData + 0x0A),GetWordLE(pData + 0x0C)
		,pData[0x7A]&0x02 ? "NTSC + PAL" : pData[0x7A]&0x01 ? "PAL" : "NTSC"
		,GetWordLE(pData + 0x6E),GetWordLE(pData + 0x6E) ? 1000000.0/GetWordLE(pData + 0x6E) : 0
		,GetWordLE(pData + 0x78),GetWordLE(pData + 0x78) ? 1000000.0/GetWordLE(pData + 0x78) : 0
		,pData[0x7B]&0x01 ? "VRC6 " : ""
		,pData[0x7B]&0x02 ? "VRC7 " : ""
		,pData[0x7B]&0x04 ? "2C33 " : ""
		,pData[0x7B]&0x08 ? "MMC5 " : ""
		,pData[0x7B]&0x10 ? "Namco1xx " : ""
		,pData[0x7B]&0x20 ? "Sunsoft5B " : ""
		,pData[0x7B] ? "" : "None"
		,pData[0x70]|pData[0x71]|pData[0x72]|pData[0x73]|pData[0x74]|pData[0x75]|pData[0x76]|pData[0x77] ? 1 : 0
		,pData[0x70],pData[0x71],pData[0x72],pData[0x73],pData[0x74],pData[0x75],pData[0x76],pData[0x77]
	);
	SONGINFO_SetDetail(pNezPlay->song, songinfodata.detail);	// EMSCRIPTEN	
}

Uint NSFLoad(NEZ_PLAY *pNezPlay, Uint8 *pData, Uint uSize)
{
	Uint ret;
	NSFNSF *THIS_ = (NSFNSF *)XMALLOC(sizeof(NSFNSF));
	if (!THIS_) return NESERR_SHORTOFMEMORY;
	XMEMSET(THIS_, 0, sizeof(NSFNSF));
	THIS_->fds_type = 2;
	pNezPlay->nsf = THIS_;
	NESMemoryHandlerInitialize(pNezPlay);
	NSFMapperSetInfo(pNezPlay, pData);
	ret = NSF6502Install(pNezPlay);
	if (ret) return ret;
	ret = NSFMapperInitialize(pNezPlay, pData + 0x80, uSize - 0x80);
	if (ret) return ret;
	ret = NSFDeviceInitialize(pNezPlay);
	if (ret) return ret;
	SONGINFO_SetSongNo(pNezPlay->song, SONGINFO_GetStartSongNo(pNezPlay->song));
	return NESERR_NOERROR;
}

