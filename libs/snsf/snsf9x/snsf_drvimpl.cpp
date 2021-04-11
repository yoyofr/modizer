#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
//YOYOFR
#include <string.h>
#include "snsf_drvimpl.h"



#undef min
#undef max

#include "snes9x/snes9x.h"
#include "snes9x/apu/apu.h"
#include "snes9x/apu/linear_resampler.h"
#include "snes9x/apu/hermite_resampler.h"
#include "snes9x/apu/bspline_resampler.h"
#include "snes9x/apu/osculating_resampler.h"
#include "snes9x/apu/sinc_resampler.h"
#include "snes9x/memmap.h"

static struct
{
	unsigned char *rom;
	unsigned romsize;
	unsigned char *sram;
	unsigned sramsize;
	unsigned first;
	unsigned base;
	unsigned save;
} loaderwork = { 0, 0, 0, 0, 0, 0, 0 };

static DWORD getdwordle(const BYTE *pData)
{
	return pData[0] | (((DWORD)pData[1]) << 8) | (((DWORD)pData[2]) << 16) | (((DWORD)pData[3]) << 24);
}


#define MIN(x,y) ((x)<(y)?(x):(y))




static INT32 InterpolationLevel = 1;
static INT32 Resampler = 1;
static UINT32 SampleRate = 32000;
static bool DisableSurround = true;
static bool ReverseStereo = false;

void snsf_set_extend_param(unsigned dwId, const wchar_t *lpPtr)
{
	if (!lpPtr)
		return;
}

void snsf_set_extend_param_void(unsigned dwId, const void* lpPtr)
{
	if (!lpPtr)
		return;

	switch (dwId)
	{
	case 1:
		InterpolationLevel = *(INT32*)lpPtr;
		break;
	case 2:
		Resampler = *(INT32*)lpPtr;
		break;
	case 3:
		SampleRate = *(UINT32*)lpPtr;
		if (SampleRate < 8000)
			SampleRate = 8000;
		else if (SampleRate > 192000)
			SampleRate = 192000;
		break;
	case 4:
		DisableSurround = *(INT32*)lpPtr > 0 ? true : false;
		break;
	case 5:
		ReverseStereo = *(INT32*)lpPtr > 0 ? true : false;
	default:
		break;
	}
}

void WinSetDefaultValues ()
{
	// TODO: delete the parts that are already covered by the default values in WinRegisterConfigItems

	// ROM Options
	memset (&Settings, 0, sizeof (Settings));
	
	// Tracing options
	Settings.TraceDMA =	false;
	Settings.TraceHDMA = false;
	Settings.TraceVRAM = false;
	Settings.TraceUnknownRegisters = false;
	Settings.TraceDSP =	false;

	// ROM timing options (see also	H_Max above)
	Settings.PAL = false;
	Settings.FrameTimePAL =	20000;
	Settings.FrameTimeNTSC = 16667;
	Settings.FrameTime = 16667;

	// CPU options
	Settings.Paused	= false;

	Settings.TakeScreenshot=false;
	

}

static void InitSnes9X()
{
	WinSetDefaultValues ();

	// Sound options
	Settings.SoundSync = true;
	Settings.Mute =	false;
	Settings.SoundPlaybackRate = SampleRate;
	Settings.SixteenBitSound = true;
	Settings.Stereo	= true;
	Settings.ReverseStereo = ReverseStereo;
	Settings.DisableSurround = DisableSurround;
	Settings.InterpolationLevel = InterpolationLevel;

    Memory.Init();

    S9xInitAPU();
	switch (Resampler)
	{
	case 4:
		S9xInitSound<SincResampler>(10, 0);
		break;
	case 3:
		S9xInitSound<OsculatingResampler>(10, 0);
		break;
	case 2:
		S9xInitSound<BsplineResampler>(10, 0);
		break;
	case 1:
		S9xInitSound<HermiteResampler>(10, 0);
		break;
	default:
		S9xInitSound<LinearResampler>(10, 0);
		break;
	}
}

static unsigned long dwChannelMuteOld = 0;
unsigned long dwChannelMute;

class BUFFER
{
public:
	unsigned char *buf;
	unsigned fil;
	unsigned cur;
	unsigned len;
	BUFFER()
	{
		buf = 0;
		fil = 0;
		cur = 0;
	}
	~BUFFER()
	{
		Term();
	}
	bool Init()
	{
		len = 2*2*SampleRate/5;
		buf = (unsigned char *)calloc(1,len);
		return buf != NULL;
	}
	void Fill(void)
	{
		if (dwChannelMuteOld != dwChannelMute)
		{
			S9xSetSoundControl(uint8(dwChannelMute));
			dwChannelMuteOld = dwChannelMute;
		}
		S9xSyncSound();
		S9xMainLoop();
		Mix();
	}
	void Term(void)
	{
		if (buf)
		{
			free(buf);
			buf = 0;
		}
	}
	void Mix()
	{
		unsigned bytes = (S9xGetSampleCount() << 1) & ~3;
		unsigned bleft = (len - fil) & ~3;
		if (bytes == 0) return;
		if (bytes > bleft) bytes = bleft;
		ZeroMemory(buf + fil, bytes);
	    S9xMixSamples(buf + fil, bytes >> 1);
		fil += bytes;
	}
};
static BUFFER buffer;

int snsf_start(const unsigned char *lrombuf, INT32 lromsize, const unsigned char *srambuf, INT32 sramsize)
{
	InitSnes9X();

	if (!buffer.Init())
		return 0;

	if (!Memory.LoadROMSNSF(lrombuf, lromsize, srambuf, sramsize))
		return 0;

	//S9xSetPlaybackRate(Settings.SoundPlaybackRate);
	S9xSetSoundMute(false);

	// bad hack for gradius3snsf.rar
	//Settings.TurboMode = true;

	return 1;
}

int snsf_gen(void *pbuffer, unsigned samples)
{
	int ret = 0;
	unsigned bytes = samples << 2;
	unsigned char *des = (unsigned char *)pbuffer;
	while (bytes > 0)
	{
		unsigned remain = buffer.fil - buffer.cur;
		while (remain == 0)
		{
			buffer.cur = 0;
			buffer.fil = 0;
			buffer.Fill();

			remain = buffer.fil - buffer.cur;
		}
		unsigned len = remain;
		if (len > bytes)
		{
			len = bytes;
		}
		//CopyMemory(des, buffer.buf + buffer.cur, len);
        memcpy(des, buffer.buf + buffer.cur, len);
		bytes -= len;
		des += len;
		buffer.cur += len;
		ret += len;
	}
	return ret;
}

void snsf_term(void)
{
	S9xReset();
    Memory.Deinit ();
    S9xDeinitAPU ();
	buffer.Term();
}

void S9xMessage (int type, int message_no, const char *str)
{
}

bool8 S9xOpenSoundDevice (void)
{
    return (TRUE);
}
