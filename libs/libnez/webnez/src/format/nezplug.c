#include "nezplug.h"

#include "neserr.h"
#include "handler.h"
#include "audiosys.h"
#include "songinfo.h"

#include "device/kmsnddev.h"
#include "m_hes.h"
#include "m_gbr.h"
#include "m_zxay.h"
#include "m_nsf.h"
#include "m_kss.h"
#include "m_nsd.h"
#include "m_sgc.h"


Uint8 chmask[0x80]={
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};
extern int (*ioview_ioread_DEV_2A03   )(int a);
extern int (*ioview_ioread_DEV_FDS    )(int a);
extern int (*ioview_ioread_DEV_MMC5   )(int a);
extern int (*ioview_ioread_DEV_VRC6   )(int a);
extern int (*ioview_ioread_DEV_N106   )(int a);
extern int (*ioview_ioread_DEV_DMG    )(int a);
extern int (*ioview_ioread_DEV_HUC6230)(int a);
extern int (*ioview_ioread_DEV_AY8910 )(int a);
extern int (*ioview_ioread_DEV_SN76489)(int a);
extern int (*ioview_ioread_DEV_SCC    )(int a);
extern int (*ioview_ioread_DEV_OPL    )(int a);
extern int (*ioview_ioread_DEV_OPLL   )(int a);
extern int (*ioview_ioread_DEV_ADPCM  )(int a);
extern int (*ioview_ioread_DEV_ADPCM2 )(int a);
extern int (*ioview_ioread_DEV_MSX    )(int a);

static struct {
	char* title;
	char* artist;
	char* copyright;
	char detail[1024];
}songinfodata;

static int (*memview_memread)(int a);


static Uint GetWordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8);
}

static Uint32 GetDwordLE(Uint8 *p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}
#define GetDwordLEM(p) (Uint32)((((Uint8 *)p)[0] | (((Uint8 *)p)[1] << 8) | (((Uint8 *)p)[2] << 16) | (((Uint8 *)p)[3] << 24)))


NEZ_PLAY* NEZNew()
{
	NEZ_PLAY *pNezPlay = (NEZ_PLAY*)XMALLOC(sizeof(NEZ_PLAY));

	if (pNezPlay != NULL) {
		XMEMSET(pNezPlay, 0, sizeof(NEZ_PLAY));
		pNezPlay->song = SONGINFO_New();
		if (!pNezPlay->song) {
			XFREE(pNezPlay);
			return 0;
		}
		pNezPlay->frequency = 48000;
		pNezPlay->channel = 1;
		pNezPlay->naf_type = NES_AUDIO_FILTER_NONE;
		pNezPlay->naf_prev[0] = pNezPlay->naf_prev[1] = 0x8000;
	}


	return pNezPlay;
}

void NEZDelete(NEZ_PLAY *pNezPlay)
{
	if (pNezPlay != NULL) {
		ioview_ioread_DEV_2A03   =NULL;
		ioview_ioread_DEV_FDS    =NULL;
		ioview_ioread_DEV_MMC5   =NULL;
		ioview_ioread_DEV_VRC6   =NULL;
		ioview_ioread_DEV_N106   =NULL;
		ioview_ioread_DEV_DMG    =NULL;
		ioview_ioread_DEV_HUC6230=NULL;
		ioview_ioread_DEV_AY8910 =NULL;
		ioview_ioread_DEV_SN76489=NULL;
		ioview_ioread_DEV_SCC    =NULL;
		ioview_ioread_DEV_OPL    =NULL;
		ioview_ioread_DEV_OPLL   =NULL;
		ioview_ioread_DEV_ADPCM  =NULL;
		ioview_ioread_DEV_ADPCM2 =NULL;
		ioview_ioread_DEV_MSX    =NULL;
		memview_memread=NULL;

		NESTerminate(pNezPlay);
		NESAudioHandlerTerminate(pNezPlay);
		NESVolumeHandlerTerminate(pNezPlay);
		SONGINFO_Delete(pNezPlay->song);
		XFREE(pNezPlay);
	}
}


void NEZSetSongNo(NEZ_PLAY *pNezPlay, Uint uSongNo)
{
	if (pNezPlay == 0) return;
	SONGINFO_SetSongNo(pNezPlay->song, uSongNo);
}


void NEZSetFrequency(NEZ_PLAY *pNezPlay, Uint freq)
{
	if (pNezPlay == 0) return;
	NESAudioFrequencySet(pNezPlay, freq);
}

void NEZSetChannel(NEZ_PLAY *pNezPlay, Uint ch)
{
	if (pNezPlay == 0) return;
	NESAudioChannelSet(pNezPlay, ch);
}

void NEZReset(NEZ_PLAY *pNezPlay)
{
	if (pNezPlay == 0) return;
	NESReset(pNezPlay);
	NESVolume(pNezPlay, pNezPlay->volume);
}

void NEZSetFilter(NEZ_PLAY *pNezPlay, Uint filter)
{
	if (pNezPlay == 0) return;
	NESAudioFilterSet(pNezPlay, filter);
}

void NEZVolume(NEZ_PLAY *pNezPlay, Uint uVolume)
{
	if (pNezPlay == 0) return;
	pNezPlay->volume = uVolume;
	NESVolume(pNezPlay, pNezPlay->volume);
}

void NEZAPUVolume(NEZ_PLAY *pNezPlay, Int32 uVolume)
{
	if (pNezPlay == 0) return;
	if (pNezPlay->nsf == 0) return;
	((NSFNSF*)pNezPlay->nsf)->apu_volume = uVolume;
}

void NEZDPCMVolume(NEZ_PLAY *pNezPlay, Int32 uVolume)
{
	if (pNezPlay == 0) return;
	if (pNezPlay->nsf == 0) return;
	((NSFNSF*)pNezPlay->nsf)->dpcm_volume = uVolume;
}

void NEZRender(NEZ_PLAY *pNezPlay, void *bufp, Uint buflen)
{
	if (pNezPlay == 0) return;
	NESAudioRender(pNezPlay, (Int16*)bufp, buflen);
}

Uint NEZGetSongNo(NEZ_PLAY *pNezPlay)
{
	if (pNezPlay == 0) return 0;
	return SONGINFO_GetSongNo(pNezPlay->song);
}

Uint NEZGetSongStart(NEZ_PLAY *pNezPlay)
{
	if (pNezPlay == 0) return 0;
	return SONGINFO_GetStartSongNo(pNezPlay->song);
}

Uint NEZGetSongMax(NEZ_PLAY *pNezPlay)
{
	if (pNezPlay == 0) return 0;
	return SONGINFO_GetMaxSongNo(pNezPlay->song);
}

Uint NEZGetChannel(NEZ_PLAY *pNezPlay)
{
	if (pNezPlay == 0) return 1;
	return SONGINFO_GetChannel(pNezPlay->song);
}

Uint NEZGetFrequency(NEZ_PLAY *pNezPlay)
{
	if (pNezPlay == 0) return 1;
	return NESAudioFrequencyGet(pNezPlay);
}

Uint NEZLoad(NEZ_PLAY *pNezPlay, Uint8 *pData, Uint uSize)
{
	Uint ret = NESERR_NOERROR;
	ioview_ioread_DEV_2A03   =NULL;
	ioview_ioread_DEV_FDS    =NULL;
	ioview_ioread_DEV_MMC5   =NULL;
	ioview_ioread_DEV_VRC6   =NULL;
	ioview_ioread_DEV_N106   =NULL;
	ioview_ioread_DEV_DMG    =NULL;
	ioview_ioread_DEV_HUC6230=NULL;
	ioview_ioread_DEV_AY8910 =NULL;
	ioview_ioread_DEV_SN76489=NULL;
	ioview_ioread_DEV_SCC    =NULL;
	ioview_ioread_DEV_OPL    =NULL;
	ioview_ioread_DEV_OPLL   =NULL;
	ioview_ioread_DEV_ADPCM  =NULL;
	ioview_ioread_DEV_ADPCM2 =NULL;
	ioview_ioread_DEV_MSX    =NULL;
	memview_memread=NULL;

	songinfodata.title=NULL;
	songinfodata.artist=NULL;
	songinfodata.copyright=NULL;
	songinfodata.detail[0]=0;

	while (1)
	{
		if (!pNezPlay || !pData) {
			ret = NESERR_PARAMETER;
			break;
		}
		NESTerminate(pNezPlay);
		NESHandlerInitialize(pNezPlay->nrh, pNezPlay->nth);
		NESAudioHandlerInitialize(pNezPlay);
		if (uSize < 8)
		{
			ret = NESERR_FORMAT;
			break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("KSCC"))
		{
			/* KSS */
			ret =  KSSLoad(pNezPlay, pData, uSize);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("KSSX"))
		{
			/* KSS */
			ret =  KSSLoad(pNezPlay, pData, uSize);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("HESM"))
		{
			/* HES */
			ret =  HESLoad(pNezPlay, pData, uSize);
			if (ret) break;
		}
		else if (uSize > 0x220 && GetDwordLE(pData + 0x200) == GetDwordLEM("HESM"))
		{
			/* HES(+512byte header) */
			ret =  HESLoad(pNezPlay, pData + 0x200, uSize - 0x200);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("NESM") && pData[4] == 0x1A)
		{
			/* NSF */
			ret = NSFLoad(pNezPlay, pData, uSize);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("ZXAY") && GetDwordLE(pData + 4) == GetDwordLEM("EMUL"))
		{
			/* ZXAY */
			ret =  ZXAYLoad(pNezPlay, pData, uSize);
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("GBRF"))
		{
			/* GBR */
			ret =  GBRLoad(pNezPlay, pData, uSize);
			if (ret) break;
		}
		else if ((GetDwordLE(pData + 0) & 0x00ffffff) == GetDwordLEM("GBS\x0"))
		{
			/* GBS */
			ret =  GBRLoad(pNezPlay, pData, uSize);
			if (ret) break;
		}
		else if (pData[0] == 0xc3 && uSize > GetWordLE(pData + 1) + 4 && GetWordLE(pData + 1) > 0x70 && (GetDwordLE(pData + GetWordLE(pData + 1) - 0x70) & 0x00ffffff) == GetDwordLEM("GBS\x0"))
		{
			/* GB(GBS player) */
			ret =  GBRLoad(pNezPlay, pData + GetWordLE(pData + 1) - 0x70, uSize - (GetWordLE(pData + 1) - 0x70));
			if (ret) break;
		}
		else if (GetDwordLE(pData + 0) == GetDwordLEM("NESL") && pData[4] == 0x1A)
		{
			/* NSD */
			ret = NSDLoad(pNezPlay, pData, uSize);
			if (ret) break;
		}
		else if ((GetDwordLE(pData + 0) & 0x00ffffff) == GetDwordLEM("SGC"))
		{
			/* SGC */
			ret = SGCLoad(pNezPlay, pData, uSize);
			if (ret) break;
		}
		else
		{
			ret = NESERR_FORMAT;
			break;
		}
		return NESERR_NOERROR;
	}
	NESTerminate(pNezPlay);
	return ret;
}


void NEZGetFileInfo(char **p1, char **p2, char **p3, char **p4)
{
	*p1 = songinfodata.title;
	*p2 = songinfodata.artist;
	*p3 = songinfodata.copyright;
	*p4 = songinfodata.detail;
}

