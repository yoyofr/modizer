#include "nezplug.h"
#include "audiosys.h"

/* ---------------------- */
/*  Audio Render Handler  */
/* ---------------------- */

#define SHIFT_BITS 8

Int32 output2[2],filter;
int LowPassFilterLevel=8,lowlevel;

void NESAudioFilterSet(NEZ_PLAY *pNezPlay, Uint filter)
{
	pNezPlay->naf_type = filter;
	pNezPlay->naf_prev[0] = 0x8000;
	pNezPlay->naf_prev[1] = 0x8000;
	output2[0] = 0x7fffffff;
	output2[1] = 0x7fffffff;
}

void NESAudioRender(NEZ_PLAY *pNezPlay, Int16 *bufp, Uint buflen)
{
	Uint maxch = NESAudioChannelGet(pNezPlay);
	while (buflen--)
	{
		NES_AUDIO_HANDLER *ph;
		Int32 accum[2] = { 0, 0 };
		Uint ch;

		for (ph = pNezPlay->nah; ph; ph = ph->next)
		{
			if (!(ph->fMode & 1) || bufp)
			{
				if (pNezPlay->channel == 2 && ph->fMode & 2)
				{
					ph->Proc2(pNezPlay, accum);
				}
				else
				{
					Int32 devout = ph->Proc(pNezPlay);
					accum[0] += devout;
					accum[1] += devout;
				}
			}
		}
		if (bufp)
		{
			for (ch = 0; ch < maxch; ch++)
			{
				Uint32 output[2];
				accum[ch] += (0x10000 << SHIFT_BITS);
				if (accum[ch] < 0)
					output[ch] = 0;
				else if (accum[ch] > (0x20000 << SHIFT_BITS) - 1)
					output[ch] = (0x20000 << SHIFT_BITS) - 1;
				else
					output[ch] = accum[ch];
				output[ch] >>= SHIFT_BITS;

				//DCオフセットフィルタ
				if(!(pNezPlay->naf_type&4)){
					Int32 buffer;
					if (output2[ch] == 0x7fffffff){
						output2[ch] = ((Int32)output[ch]<<14) - 0x40000000;
						//output2[ch] *= -1;
					}
					output2[ch] -= (output2[ch] - (((Int32)output[ch]<<14) - 0x40000000))/(64*filter);
					buffer =  output[ch]/2 - output2[ch]/0x8000;

					if (buffer < 0)
						output[ch] = 0;
					else if (buffer > 0xffff)
						output[ch] = 0xffff;
					else
						output[ch] = buffer;
				}else{
					output[ch] >>= 1;
				}
				switch (pNezPlay->naf_type&3)
				{
					case NES_AUDIO_FILTER_LOWPASS:
						{
							Uint32 prev = pNezPlay->naf_prev[ch];
							//output[ch] = (output[ch] + prev) >> 1;
							output[ch] = (output[ch] * lowlevel + prev * filter) / (lowlevel+filter);
							pNezPlay->naf_prev[ch] = output[ch];
						}
						break;
					case NES_AUDIO_FILTER_WEIGHTED:
						{
							Uint32 prev = pNezPlay->naf_prev[ch];
							pNezPlay->naf_prev[ch] = output[ch];
							output[ch] = (output[ch] + output[ch] + output[ch] + prev) >> 2;
						}
						break;
					case NES_AUDIO_FILTER_LOWPASS_WEIGHTED:
						{
							Uint32 prev = pNezPlay->naf_prev[ch];
							//output[ch] = (output[ch] + prev) >> 1;
							output[ch] = (output[ch] * lowlevel + prev * filter) / (lowlevel+filter);
							pNezPlay->naf_prev[ch] = output[ch];
						}
						{
							Uint32 prev = pNezPlay->naf_prev[ch];
							pNezPlay->naf_prev[ch] = output[ch];
							output[ch] = (output[ch] + output[ch] + output[ch] + prev) >> 2;
						}
						break;
				}
				*bufp++ = (Int16)(((Int32)output[ch]) - 0x8000);
			}
		}
	}
}

void NESVolume(NEZ_PLAY *pNezPlay, Uint volume)
{
	NES_VOLUME_HANDLER *ph;
	for (ph = pNezPlay->nvh; ph; ph = ph->next) ph->Proc(pNezPlay, volume);
}

static void NESAudioHandlerInstallOne(NEZ_PLAY *pNezPlay, const NES_AUDIO_HANDLER *ph)
{
	NES_AUDIO_HANDLER *nh;

	/* Add to tail of list*/
	nh = XMALLOC(sizeof(NES_AUDIO_HANDLER));
	nh->fMode = ph->fMode;
	nh->Proc = ph->Proc;
	nh->Proc2 = ph->Proc2;
	nh->next = 0;

	if (pNezPlay->nah)
	{
		NES_AUDIO_HANDLER *p = pNezPlay->nah;
		while (p->next) p = p->next;
		p->next = nh;
	}
	else
	{
		pNezPlay->nah = nh;
	}
}
void NESAudioHandlerInstall(NEZ_PLAY *pNezPlay, const NES_AUDIO_HANDLER * ph)
{
	for (;(ph->fMode&2)?(!!ph->Proc2):(!!ph->Proc);ph++) {
		NESAudioHandlerInstallOne(pNezPlay, ph);
	}
}

void NESAudioHandlerTerminate(NEZ_PLAY *pNezPlay)
{
	NES_AUDIO_HANDLER *p = pNezPlay->nah, *next;
	while (p) {
		next = p->next;
		XFREE(p);
		p = next;
	}
}

void NESVolumeHandlerInstall(NEZ_PLAY *pNezPlay, const NES_VOLUME_HANDLER * ph)
{
	NES_VOLUME_HANDLER *nh;

	/* Add to tail of list*/
	for (;ph->Proc;ph++)
	{
		nh = XMALLOC(sizeof(NES_VOLUME_HANDLER));
		nh->Proc = ph->Proc;
		nh->next = pNezPlay->nvh;

		pNezPlay->nvh = nh;
	}
}

void NESVolumeHandlerTerminate(NEZ_PLAY *pNezPlay)
{
	NES_VOLUME_HANDLER *p = pNezPlay->nvh, *next;
	while (p) {
		next = p->next;
		XFREE(p);
		p = next;
	}
}

void NESAudioHandlerInitialize(NEZ_PLAY *pNezPlay)
{
	pNezPlay->nah = 0;
	pNezPlay->nvh = 0;
}

void NESAudioFrequencySet(NEZ_PLAY *pNezPlay, Uint freq)
{
	pNezPlay->frequency = freq;
	filter = freq/3000;
	if(!filter)filter=1;

	lowlevel = 33-LowPassFilterLevel;
}
Uint NESAudioFrequencyGet(NEZ_PLAY *pNezPlay)
{
	return pNezPlay->frequency;
}

void NESAudioChannelSet(NEZ_PLAY *pNezPlay, Uint ch)
{
	pNezPlay->channel = ch;
}
Uint NESAudioChannelGet(NEZ_PLAY *pNezPlay)
{
	return pNezPlay->channel;
}
