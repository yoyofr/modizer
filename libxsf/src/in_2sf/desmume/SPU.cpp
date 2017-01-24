/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2006 Theo Berkau
	Copyright (C) 2008-2012 DeSmuME team

	Ideas borrowed from Stephane Dallongeville's SCSP core

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "XSFCommon.h"

#include <queue>
#include <vector>
#include <cstdlib>
#include <cstring>
#ifndef M_PI
static const double M_PI = 3.14159265358979323846;
#endif
#include "MMU.h"
#include "SPU.h"
#include "mem.h"
#include "readwrite.h"
#include "armcpu.h"
#include "NDSSystem.h"
#include "matrix.h"

static inline int16_t read16(uint32_t addr) { return _MMU_read16<ARMCPU_ARM7,MMU_AT_DEBUG>(addr); }
static inline uint8_t read08(uint32_t addr) { return _MMU_read08<ARMCPU_ARM7,MMU_AT_DEBUG>(addr); }
static inline int8_t read_s8(uint32_t addr) { return _MMU_read08<ARMCPU_ARM7,MMU_AT_DEBUG>(addr); }

static const int K_ADPCM_LOOPING_RECOVERY_INDEX = 99999;
static const int COSINE_INTERPOLATION_RESOLUTION = 8192;

static auto synchronizer = std::unique_ptr<ISynchronizingAudioBuffer>(metaspu_construct(ESynchMethod_N));

std::unique_ptr<SPU_struct> SPU_core, SPU_user;
int SPU_currentCoreNum = SNDCORE_DUMMY;
static int volume = 100;

static size_t buffersize = 0;
static ESynchMode synchmode = ESynchMode_DualSynchAsynch;
static ESynchMethod synchmethod = ESynchMethod_N;

static int SNDCoreId = -1;
static SoundInterface_struct *SNDCore = nullptr;
extern SoundInterface_struct *SNDCoreList[];

static const int format_shift[] = { 2, 1, 3, 0 };

static const int8_t indextbl[8] =
{
	-1, -1, -1, -1, 2, 4, 6, 8
};

static const uint16_t adpcmtbl[89] =
{
	0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x0010,
	0x0011, 0x0013, 0x0015, 0x0017, 0x0019, 0x001C, 0x001F, 0x0022, 0x0025,
	0x0029, 0x002D, 0x0032, 0x0037, 0x003C, 0x0042, 0x0049, 0x0050, 0x0058,
	0x0061, 0x006B, 0x0076, 0x0082, 0x008F, 0x009D, 0x00AD, 0x00BE, 0x00D1,
	0x00E6, 0x00FD, 0x0117, 0x0133, 0x0151, 0x0173, 0x0198, 0x01C1, 0x01EE,
	0x0220, 0x0256, 0x0292, 0x02D4, 0x031C, 0x036C, 0x03C3, 0x0424, 0x048E,
	0x0502, 0x0583, 0x0610, 0x06AB, 0x0756, 0x0812, 0x08E0, 0x09C3, 0x0ABD,
	0x0BD0, 0x0CFF, 0x0E4C, 0x0FBA, 0x114C, 0x1307, 0x14EE, 0x1706, 0x1954,
	0x1BDC, 0x1EA5, 0x21B6, 0x2515, 0x28CA, 0x2CDF, 0x315B, 0x364B, 0x3BB9,
	0x41B2, 0x4844, 0x4F7E, 0x5771, 0x602F, 0x69CE, 0x7462, 0x7FFF
};

static const int16_t wavedutytbl[8][8] = {
	{ -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
	{ -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF }
};

static int32_t precalcdifftbl[89][16];
static uint8_t precalcindextbl[89][8];
static double cos_lut[COSINE_INTERPOLATION_RESOLUTION];

static const double ARM7_CLOCK = 33513982;

static const double samples_per_hline = (DESMUME_SAMPLE_RATE / 59.8261f) / 263.0f;

static double samples = 0;

template<typename T> static inline T MinMax(T val, T min, T max)
{
	if (val < min)
		return min;
	else if (val > max)
		return max;
	else
		return val;
}

//--------------external spu interface---------------

int SPU_ChangeSoundCore(int coreid, int Buffersize)
{
	buffersize = Buffersize;

	SPU_user.reset();

	// Make sure the old core is freed
	if (SNDCore)
		SNDCore->DeInit();

	// So which core do we want?
	if (coreid == SNDCORE_DEFAULT)
		coreid = 0; // Assume we want the first one

	SPU_currentCoreNum = coreid;

	// Go through core list and find the id
	for (int i = 0; SNDCoreList[i]; ++i)
		if (SNDCoreList[i]->id == coreid)
		{
			// Set to current core
			SNDCore = SNDCoreList[i];
			break;
		}

	SNDCoreId = coreid;

	// If the user picked the dummy core, disable the user spu
	if (SNDCore == &SNDDummy)
		return 0;

	// If the core wasnt found in the list for some reason, disable the user spu
	if (!SNDCore)
		return -1;

	// Since it failed, instead of it being fatal, disable the user spu
	if (SNDCore->Init(buffersize * 2) == -1)
	{
		SNDCore = nullptr;
		return -1;
	}

	SNDCore->SetVolume(volume);

	SPU_SetSynchMode(synchmode, synchmethod);

	return 0;
}

void SPU_ReInit()
{
	SPU_Init(SNDCoreId, buffersize);
}

int SPU_Init(int coreid, int Buffersize)
{
	// Build the cosine interpolation LUT
	int i;
	for (i = 0; i < COSINE_INTERPOLATION_RESOLUTION; ++i)
		cos_lut[i] = (1.0 - std::cos((static_cast<double>(i) / COSINE_INTERPOLATION_RESOLUTION) * M_PI)) * 0.5;

	SPU_core.reset(new SPU_struct(std::ceil(samples_per_hline)));
	SPU_Reset();

	int j;
	// create adpcm decode accelerator lookups
	for (i = 0; i < 16; ++i)
		for (j = 0; j < 89; ++j)
		{
			precalcdifftbl[j][i] = ((i & 0x7) * 2 + 1) * adpcmtbl[j] / 8;
			if (i & 0x8)
				precalcdifftbl[j][i] = -precalcdifftbl[j][i];
		}
	for (i = 0; i < 8; ++i)
		for (j = 0; j < 89; ++j)
			precalcindextbl[j][i] = MinMax(j + indextbl[i], 0, 88);

	return SPU_ChangeSoundCore(coreid, Buffersize);
}

void SPU_CloneUser()
{
	if (SPU_user)
	{
		memcpy(SPU_user->channels, SPU_core->channels, sizeof(SPU_core->channels));
		SPU_user->regs = SPU_core->regs;
	}
}

void SPU_SetSynchMode(ESynchMode mode, ESynchMethod method)
{
	synchmode = mode;
	if (synchmethod != method)
	{
		synchmethod = method;
		// grr does this need to be locked? spu might need a lock method
		// or maybe not, maybe the platform-specific code that calls this function can deal with it.
		synchronizer.reset(metaspu_construct(synchmethod));
	}

	SPU_user.reset();

	if (synchmode == ESynchMode_DualSynchAsynch)
	{
		SPU_user.reset(new SPU_struct(buffersize));
		SPU_CloneUser();
	}
}

void SPU_Reset()
{
	SPU_core->reset();

	if (SPU_user)
	{
		if (SNDCore)
		{
			SNDCore->DeInit();
			SNDCore->Init(SPU_user->bufsize * 2);
			SNDCore->SetVolume(volume);
		}
		SPU_user->reset();
	}

	// zero - 09-apr-2010: this concerns me, regarding savestate synch.
	// After 0.9.6, lets experiment with removing it and just properly zapping the spu instead
	// Reset Registers
	for (int i = 0x400; i < 0x51D; ++i)
		T1WriteByte(MMU.ARM7_REG, i, 0);

	samples = 0;
}

//------------------------------------------

void SPU_struct::reset()
{
	memset(&this->sndbuf[0], 0, bufsize * 2 * 4);
	memset(&this->outbuf[0], 0, bufsize * 2 * 2);

	memset(this->channels, 0, sizeof(channel_struct) * 16);

	reconstruct(&this->regs);

	for (int i = 0; i < 16; ++i)
		this->channels[i].num = i;
}

SPU_struct::SPU_struct(int Buffersize) : bufpos(0), buflength(0), sndbuf(new int32_t[Buffersize * 2]), outbuf(new int16_t[Buffersize * 2]), bufsize(Buffersize)
{
	this->reset();
}

void SPU_DeInit()
{
	if (SNDCore)
		SNDCore->DeInit();
	SNDCore = nullptr;

	SPU_core.reset();
	SPU_user.reset();
}

//////////////////////////////////////////////////////////////////////////////

void SPU_struct::ShutUp()
{
	for (int i = 0; i < 16; ++i)
		this->channels[i].status = CHANSTAT_STOPPED;
}

static inline void adjust_channel_timer(channel_struct *chan)
{
	chan->sampinc = (ARM7_CLOCK / (DESMUME_SAMPLE_RATE * 2)) / (0x10000 - chan->timer);
}

void SPU_struct::KeyProbe(int chan_num)
{
	channel_struct &thischan = this->channels[chan_num];
	if (thischan.status == CHANSTAT_STOPPED)
	{
		if (thischan.keyon && this->regs.masteren)
			this->KeyOn(chan_num);
	}
	else if (thischan.status == CHANSTAT_PLAY)
	{
		if (!thischan.keyon || !this->regs.masteren)
			this->KeyOff(chan_num);
	}
}

void SPU_struct::KeyOff(int channel)
{
	//printf("keyoff%d\n",channel);
	channel_struct &thischan = this->channels[channel];
	thischan.status = CHANSTAT_STOPPED;
}

void SPU_struct::KeyOn(int channel)
{
	channel_struct &thischan = this->channels[channel];
	thischan.status = CHANSTAT_PLAY;

	thischan.totlength = thischan.length + thischan.loopstart;
	adjust_channel_timer(&thischan);

	//printf("keyon %d totlength:%d\n",channel,thischan.totlength);

	//LOG("Channel %d key on: vol = %d, datashift = %d, hold = %d, pan = %d, waveduty = %d, repeat = %d, format = %d, source address = %07X,"
	//		"timer = %04X, loop start = %04X, length = %06X, MMU.ARM7_REG[0x501] = %02X\n", channel, chan->vol, chan->datashift, chan->hold,
	//		chan->pan, chan->waveduty, chan->repeat, chan->format, chan->addr, chan->timer, chan->loopstart, chan->length, T1ReadByte(MMU.ARM7_REG, 0x501));

	switch (thischan.format)
	{
		case 0: // 8-bit
			//hischan.loopstart = thischan.loopstart << 2;
			//hischan.length = (thischan.length << 2) + thischan.loopstart;
			thischan.sampcnt = -3;
			break;
		case 1: // 16-bit
			//thischan.loopstart = thischan.loopstart << 1;
			//thischan.length = (thischan.length << 1) + thischan.loopstart;
			thischan.sampcnt = -3;
			break;
		case 2: // ADPCM
			thischan.pcm16b = read16(thischan.addr);
			thischan.pcm16b_last = thischan.pcm16b;
			thischan.index = read08(thischan.addr + 2) & 0x7F;
			thischan.lastsampcnt = 7;
			thischan.sampcnt = -3;
			thischan.loop_index = K_ADPCM_LOOPING_RECOVERY_INDEX;
			//thischan.loopstart = thischan.loopstart << 3;
			//hischan.length = (thischan.length << 3) + thischan.loopstart;
			break;
		case 3: // PSG
			thischan.sampcnt = -1;
			thischan.x = 0x7FFF;
	}

	thischan.double_totlength_shifted = thischan.totlength << format_shift[thischan.format];

	if (thischan.format != 3 && fEqual(thischan.double_totlength_shifted, 0.0))
	{
		printf("INFO: Stopping channel %d due to zero length\n", channel);
		thischan.status = CHANSTAT_STOPPED;
	}
}

//////////////////////////////////////////////////////////////////////////////

template<typename T> static inline void SETBYTE(uint32_t which, T &oldval, uint8_t newval) { oldval = (oldval & (~(0xFF << (which * 8)))) | (newval << (which * 8)); }
static inline uint8_t GETBYTE(uint32_t which, uint32_t val) { return (val >> (which * 8)) & 0xFF; }

uint8_t SPU_ReadByte(uint32_t addr)
{
	addr &= 0xFFF;
	return SPU_core->ReadByte(addr);
}
uint16_t SPU_ReadWord(uint32_t addr)
{
	addr &= 0xFFF;
	return SPU_core->ReadWord(addr);
}
uint32_t SPU_ReadLong(uint32_t addr)
{
	addr &= 0xFFF;
	return SPU_core->ReadLong(addr);
}

uint16_t SPU_struct::ReadWord(uint32_t addr)
{
	return this->ReadByte(addr) | (this->ReadByte(addr + 1) << 8);
}

uint32_t SPU_struct::ReadLong(uint32_t addr)
{
	return this->ReadByte(addr) | (this->ReadByte(addr + 1) << 8) | (this->ReadByte(addr + 2) << 16) | (ReadByte(addr + 3) << 24);
}

uint8_t SPU_struct::ReadByte(uint32_t addr)
{
	switch (addr)
	{
		// SOUNDCNT
		case 0x500:
			return this->regs.mastervol;
		case 0x501:
			return this->regs.ctl_left | (this->regs.ctl_right << 2) | (this->regs.ctl_ch1bypass << 4) | (this->regs.ctl_ch3bypass << 5) | (this->regs.masteren << 7);
		case 0x502:
		case 0x503:
			return 0;

		// SOUNDBIAS
		case 0x504:
			return this->regs.soundbias & 0xFF;
		case 0x505:
			return (this->regs.soundbias >> 8) & 0xFF;
		case 0x506:
		case 0x507:
			return 0;

		// SNDCAP0CNT/SNDCAP1CNT
		case 0x508:
		case 0x509:
		{
			uint32_t which = addr - 0x508;
			return this->regs.cap[which].add | (this->regs.cap[which].source << 1) | (this->regs.cap[which].oneshot << 2) | (this->regs.cap[which].bits8 << 3)
				//| (regs.cap[which].active<<7); //? which is right? need test
				| (this->regs.cap[which].runtime.running << 7);
		}

		// SNDCAP0DAD
		case 0x510:
			return GETBYTE(0, this->regs.cap[0].dad);
		case 0x511:
			return GETBYTE(1, this->regs.cap[0].dad);
		case 0x512:
			return GETBYTE(2, this->regs.cap[0].dad);
		case 0x513:
			return GETBYTE(3, this->regs.cap[0].dad);

		// SNDCAP0LEN
		case 0x514:
			return GETBYTE(0, this->regs.cap[0].len);
		case 0x515:
			return GETBYTE(1, this->regs.cap[0].len);
		case 0x516:
		case 0x517:
			return 0; //not used

		// SNDCAP1DAD
		case 0x518:
			return GETBYTE(0, this->regs.cap[1].dad);
		case 0x519:
			return GETBYTE(1, this->regs.cap[1].dad);
		case 0x51A:
			return GETBYTE(2, this->regs.cap[1].dad);
		case 0x51B:
			return GETBYTE(3, this->regs.cap[1].dad);

		// SNDCAP1LEN
		case 0x51C:
			return GETBYTE(0, this->regs.cap[1].len);
		case 0x51D:
			return GETBYTE(1, this->regs.cap[1].len);
		case 0x51E:
		case 0x51F:
			return 0; //not used

		default:
		{
			// individual channel regs

			uint32_t chan_num = (addr >> 4) & 0xF;
			if (chan_num > 0xF)
				return 0;
			channel_struct &thischan = this->channels[chan_num];

			switch (addr & 0xF)
			{
				case 0x0:
					return thischan.vol;
				case 0x1:
				{
					uint8_t ret = thischan.datashift;
					if (ret == 4)
						ret = 3;
					ret |= thischan.hold << 7;
					return ret;
				}
				case 0x2:
					return thischan.pan;
				case 0x3:
					return thischan.waveduty | (thischan.repeat << 3) | (thischan.format << 5) | (thischan.status == CHANSTAT_PLAY ? 0x80 : 0);
				case 0x4:
					return 0; //return GETBYTE(0, thischan.addr); //not readable
				case 0x5:
					return 0; //return GETBYTE(1, thischan.addr); //not readable
				case 0x6:
					return 0; //return GETBYTE(2, thischan.addr); //not readable
				case 0x7:
					return 0; //return GETBYTE(3, thischan.addr); //not readable
				case 0x8:
					return GETBYTE(0, thischan.timer);
				case 0x9:
					return GETBYTE(1, thischan.timer);
				case 0xA:
					return GETBYTE(0, thischan.loopstart);
				case 0xB:
					return GETBYTE(1, thischan.loopstart);
				case 0xC:
					return 0; //return GETBYTE(0, thischan.length); //not readable
				case 0xD:
					return 0; //return GETBYTE(1, thischan.length); //not readable
				case 0xE:
					return 0; //return GETBYTE(2, thischan.length); //not readable
				case 0xF:
					return 0; //return GETBYTE(3, thischan.length); //not readable
				default:
					return 0; //impossible
			} // switch on individual channel regs
		} // default case
	} // switch on address
}

SPUFifo::SPUFifo()
{
	this->reset();
}

void SPUFifo::reset()
{
	this->head = this->tail = this->size = 0;
}

void SPUFifo::enqueue(int16_t val)
{
	if (this->size == 16)
		return;
	this->buffer[this->tail] = val;
	++this->tail;
	this->tail &= 15;
	++this->size;
}

int16_t SPUFifo::dequeue()
{
	if (!this->size)
		return 0;
	++this->head;
	this->head &= 15;
	int16_t ret = this->buffer[this->head];
	--this->size;
	return ret;
}

void SPU_struct::ProbeCapture(int which)
{
	// VERY UNTESTED -- HOW MUCH OF THIS RESETS, AND WHEN?

	if (!this->regs.cap[which].active)
	{
		this->regs.cap[which].runtime.running = 0;
		return;
	}

	REGS::CAP &cap = this->regs.cap[which];
	cap.runtime.running = 1;
	cap.runtime.curdad = cap.dad;
	uint32_t len = cap.len;
	if (!len)
		len = 1;
	cap.runtime.maxdad = cap.dad + len * 4;
	cap.runtime.sampcnt = 0;
	cap.runtime.fifo.reset();
}

void SPU_struct::WriteByte(uint32_t addr, uint8_t val)
{
	switch (addr)
	{
		// SOUNDCNT
		case 0x500:
			this->regs.mastervol = val & 0x7F;
			break;
		case 0x501:
			this->regs.ctl_left = val & 3;
			this->regs.ctl_right = (val >> 2) & 3;
			this->regs.ctl_ch1bypass = (val >> 4) & 1;
			this->regs.ctl_ch3bypass = (val >> 5) & 1;
			this->regs.masteren = (val >> 7) & 1;
			for (int i = 0; i < 16; ++i)
				this->KeyProbe(i);
			break;
		case 0x502:
		case 0x503:
			break; // not used

		// SOUNDBIAS
		case 0x504:
			SETBYTE(0, this->regs.soundbias, val);
			break;
		case 0x505:
			SETBYTE(1, this->regs.soundbias, val & 3);
			break;
		case 0x506:
		case 0x507:
			break; // these dont answer anyway

		// SNDCAP0CNT/SNDCAP1CNT
		case 0x508:
		case 0x509:
		{
			uint32_t which = addr - 0x508;
			this->regs.cap[which].add = static_cast<uint8_t>(BIT0(val));
			this->regs.cap[which].source = static_cast<uint8_t>(BIT1(val));
			this->regs.cap[which].oneshot = static_cast<uint8_t>(BIT2(val));
			this->regs.cap[which].bits8 = static_cast<uint8_t>(BIT3(val));
			this->regs.cap[which].active = static_cast<uint8_t>(BIT7(val));
			this->ProbeCapture(which);
			break;
		}

		// SNDCAP0DAD
		case 0x510:
			SETBYTE(0, this->regs.cap[0].dad, val);
			break;
		case 0x511:
			SETBYTE(1, this->regs.cap[0].dad, val);
			break;
		case 0x512:
			SETBYTE(2, this->regs.cap[0].dad, val);
			break;
		case 0x513:
			SETBYTE(3, this->regs.cap[0].dad, val & 7);
			break;

		// SNDCAP0LEN
		case 0x514:
			SETBYTE(0, this->regs.cap[0].len, val);
			break;
		case 0x515:
			SETBYTE(1, this->regs.cap[0].len, val);
			break;
		case 0x516:
		case 0x517:
			break; // not used

		// SNDCAP1DAD
		case 0x518:
			SETBYTE(0, this->regs.cap[1].dad, val);
			break;
		case 0x519:
			SETBYTE(1, this->regs.cap[1].dad, val);
			break;
		case 0x51A:
			SETBYTE(2, this->regs.cap[1].dad, val);
			break;
		case 0x51B:
			SETBYTE(3, this->regs.cap[1].dad, val & 7);
			break;

		// SNDCAP1LEN
		case 0x51C:
			SETBYTE(0, this->regs.cap[1].len, val);
			break;
		case 0x51D:
			SETBYTE(1, this->regs.cap[1].len, val);
			break;
		case 0x51E:
		case 0x51F:
			break; // not used

		default:
		{
			// individual channel regs

			uint32_t chan_num = (addr >> 4) & 0xF;
			if (chan_num>0xF)
				break;
			channel_struct &thischan = this->channels[chan_num];

			switch (addr & 0xF)
			{
				case 0x0:
					thischan.vol = val & 0x7F;
					break;
				case 0x1:
					thischan.datashift = val & 0x3;
					if (thischan.datashift == 3)
						thischan.datashift = 4;
					thischan.hold = (val >> 7) & 0x1;
					break;
				case 0x2:
					thischan.pan = val & 0x7F;
					break;
				case 0x3:
					thischan.waveduty = val & 0x7;
					thischan.repeat = (val >> 3) & 0x3;
					thischan.format = (val >> 5) & 0x3;
					thischan.keyon = static_cast<uint8_t>(BIT7(val));
					this->KeyProbe(chan_num);
					break;
				case 0x4:
					SETBYTE(0, thischan.addr, val);
					break;
				case 0x5:
					SETBYTE(1, thischan.addr, val);
					break;
				case 0x6:
					SETBYTE(2, thischan.addr, val);
					break;
				case 0x7:
					SETBYTE(3, thischan.addr, val & 0x7);
					break; // only 27 bits of this register are used
				case 0x8:
					SETBYTE(0, thischan.timer, val);
					adjust_channel_timer(&thischan);
					break;
				case 0x9:
					SETBYTE(1, thischan.timer, val);
					adjust_channel_timer(&thischan);
					break;
				case 0xA:
					SETBYTE(0, thischan.loopstart, val);
					break;
				case 0xB:
					SETBYTE(1, thischan.loopstart, val);
					break;
				case 0xC:
					SETBYTE(0, thischan.length, val);
					break;
				case 0xD:
					SETBYTE(1, thischan.length, val);
					break;
				case 0xE:
					SETBYTE(2, thischan.length, val & 0x3F);
					break; // only 22 bits of this register are used
				case 0xF:
					SETBYTE(3, thischan.length, 0);
					break;
			} // switch on individual channel regs
		} // default case
	} // switch on address
}

void SPU_WriteByte(uint32_t addr, uint8_t val)
{
	//printf("%08X: chan:%02X reg:%02X val:%02X\n",addr,(addr>>4)&0xF,addr&0xF,val);
	addr &= 0xFFF;

	SPU_core->WriteByte(addr, val);
	if (SPU_user)
		SPU_user->WriteByte(addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void SPU_struct::WriteWord(uint32_t addr, uint16_t val)
{
	this->WriteByte(addr, val & 0xFF);
	this->WriteByte(addr + 1, (val >> 8) & 0xFF);
}

void SPU_WriteWord(uint32_t addr, uint16_t val)
{
	//printf("%08X: chan:%02X reg:%02X val:%04X\n",addr,(addr>>4)&0xF,addr&0xF,val);
	addr &= 0xFFF;

	SPU_core->WriteWord(addr, val);
	if (SPU_user)
		SPU_user->WriteWord(addr, val);
}

//////////////////////////////////////////////////////////////////////////////

void SPU_struct::WriteLong(uint32_t addr, uint32_t val)
{
	this->WriteByte(addr,val & 0xFF);
	this->WriteByte(addr + 1,(val >> 8) & 0xFF);
	this->WriteByte(addr + 2,(val >> 16) & 0xFF);
	this->WriteByte(addr + 3,(val >> 24) & 0xFF);
}

void SPU_WriteLong(uint32_t addr, uint32_t val)
{
	//printf("%08X: chan:%02X reg:%02X val:%08X\n",addr,(addr>>4)&0xF,addr&0xF,val);
	addr &= 0xFFF;

	SPU_core->WriteLong(addr, val);
	if (SPU_user)
		SPU_user->WriteLong(addr, val);
}

//////////////////////////////////////////////////////////////////////////////

static inline int32_t Interpolate(int32_t a, int32_t b, double ratio, SPUInterpolationMode INTERPOLATE_MODE)
{
	double sampleA = static_cast<double>(a);
	double sampleB = static_cast<double>(b);
	ratio = ratio - u32floor(ratio);

	switch (INTERPOLATE_MODE)
	{
		case SPUInterpolation_Cosine:
			// Cosine Interpolation Formula:
			// ratio2 = (1 - cos(ratio * M_PI)) / 2
			// sampleI = sampleA * (1 - ratio2) + sampleB * ratio2
			return s32floor((cos_lut[static_cast<unsigned>(ratio * COSINE_INTERPOLATION_RESOLUTION)] * (sampleB - sampleA)) + sampleA);

		case SPUInterpolation_Linear:
			// Linear Interpolation Formula:
			// sampleI = sampleA * (1 - ratio) + sampleB * ratio
			return s32floor((ratio * (sampleB - sampleA)) + sampleA);

		default:
			break;
	}

	return a;
}

//////////////////////////////////////////////////////////////////////////////

static inline void Fetch8BitData(const channel_struct *const chan, int32_t *const data, SPUInterpolationMode INTERPOLATE_MODE)
{
	if (chan->sampcnt < 0)
	{
		*data = 0;
		return;
	}

	uint32_t loc = u32floor(chan->sampcnt);
	if (INTERPOLATE_MODE != SPUInterpolation_None)
	{
		int32_t a = static_cast<int32_t>(read_s8(chan->addr + loc) << 8);
		if (loc < (chan->totlength << 2) - 1)
		{
			int32_t b = static_cast<int32_t>(read_s8(chan->addr + loc + 1) << 8);
			a = Interpolate(a, b, chan->sampcnt, INTERPOLATE_MODE);
		}
		*data = a;
	}
	else
		*data = static_cast<int32_t>(read_s8(chan->addr + loc) << 8);
}

static inline void Fetch16BitData(const channel_struct *const chan, int32_t *const data, SPUInterpolationMode INTERPOLATE_MODE)
{
	if (chan->sampcnt < 0)
	{
		*data = 0;
		return;
	}

	if (INTERPOLATE_MODE != SPUInterpolation_None)
	{
		uint32_t loc = u32floor(chan->sampcnt);

		int32_t a = static_cast<int32_t>(read16(loc * 2 + chan->addr));
		if (loc < (chan->totlength << 1) - 1)
		{
			int32_t b = static_cast<int32_t>(read16(loc * 2 + chan->addr + 2));
			a = Interpolate(a, b, chan->sampcnt, INTERPOLATE_MODE);
		}
		*data = a;
	}
	else
		*data = read16(chan->addr + u32floor(chan->sampcnt) * 2);
}

static inline void FetchADPCMData(channel_struct *const chan, int32_t *const data, SPUInterpolationMode INTERPOLATE_MODE)
{
	if (chan->sampcnt < 8)
	{
		*data = 0;
		return;
	}

	// No sense decoding, just return the last sample
	if (chan->lastsampcnt != u32floor(chan->sampcnt))
	{
		uint32_t endExclusive = u32floor(chan->sampcnt + 1);
		for (uint32_t i = chan->lastsampcnt + 1; i < endExclusive; ++i)
		{
			uint32_t shift = (i & 1) << 2;
			uint32_t data4bit = static_cast<uint32_t>(read08(chan->addr + (i >> 1))) >> shift;

			int32_t diff = precalcdifftbl[chan->index][data4bit & 0xF];
			chan->index = precalcindextbl[chan->index][data4bit & 0x7];

			chan->pcm16b_last = chan->pcm16b;
			chan->pcm16b = MinMax(chan->pcm16b+diff, -0x8000, 0x7FFF);

			if (i == static_cast<uint32_t>(chan->loopstart << 3))
			{
				if (chan->loop_index != K_ADPCM_LOOPING_RECOVERY_INDEX)
					printf("over-snagging\n");
				chan->loop_pcm16b = chan->pcm16b;
				chan->loop_index = chan->index;
			}
		}

		chan->lastsampcnt = u32floor(chan->sampcnt);
	}

	if (INTERPOLATE_MODE != SPUInterpolation_None)
		*data = Interpolate(static_cast<int32_t>(chan->pcm16b_last), static_cast<int32_t>(chan->pcm16b), chan->sampcnt, INTERPOLATE_MODE);
	else
		*data = static_cast<int32_t>(chan->pcm16b);
}

static inline void FetchPSGData(channel_struct *chan, int32_t *data)
{
	if (chan->sampcnt < 0)
	{
		*data = 0;
		return;
	}

	if (chan->num < 8)
		*data = 0;
	else if (chan->num < 14)
		*data = static_cast<int32_t>(wavedutytbl[chan->waveduty][u32floor(chan->sampcnt) & 0x7]);
	else
	{
		if (chan->lastsampcnt == u32floor(chan->sampcnt))
		{
			*data = static_cast<int32_t>(chan->psgnoise_last);
			return;
		}

		uint32_t max = u32floor(chan->sampcnt);
		for (uint32_t i = chan->lastsampcnt; i < max; ++i)
		{
			if (chan->x & 0x1)
			{
				chan->x = (chan->x >> 1) ^ 0x6000;
				chan->psgnoise_last = -0x7FFF;
			}
			else
			{
				chan->x >>= 1;
				chan->psgnoise_last = 0x7FFF;
			}
		}

		chan->lastsampcnt = u32floor(chan->sampcnt);

		*data = static_cast<int32_t>(chan->psgnoise_last);
	}
}

//////////////////////////////////////////////////////////////////////////////

static inline void MixL(SPU_struct *SPU, channel_struct *chan, int32_t data)
{
	data = spumuldiv7(data, chan->vol) >> chan->datashift;
	SPU->sndbuf[SPU->bufpos << 1] += data;
}

static inline void MixR(SPU_struct *SPU, channel_struct *chan, int32_t data)
{
	data = spumuldiv7(data, chan->vol) >> chan->datashift;
	SPU->sndbuf[(SPU->bufpos << 1) + 1] += data;
}

static inline void MixLR(SPU_struct *SPU, channel_struct *chan, int32_t data)
{
	data = spumuldiv7(data, chan->vol) >> chan->datashift;
	SPU->sndbuf[SPU->bufpos << 1] += spumuldiv7(data, 127 - chan->pan);
	SPU->sndbuf[(SPU->bufpos << 1) + 1] += spumuldiv7(data, chan->pan);
}

//////////////////////////////////////////////////////////////////////////////

static inline void TestForLoop(SPU_struct *SPU, channel_struct *chan, int FORMAT)
{
	int shift = !FORMAT ? 2 : 1;

	chan->sampcnt += chan->sampinc;

	if (chan->sampcnt > chan->double_totlength_shifted)
	{
		// Do we loop? Or are we done?
		if (chan->repeat == 1)
		{
			while (chan->sampcnt > chan->double_totlength_shifted)
				chan->sampcnt -= chan->double_totlength_shifted - static_cast<double>(chan->loopstart << shift);
			//chan->sampcnt = (double)(chan->loopstart << shift);
		}
		else
		{
			SPU->KeyOff(chan->num);
			SPU->bufpos = SPU->buflength;
		}
	}
}

static inline void TestForLoop2(SPU_struct *SPU, channel_struct *chan)
{
	chan->sampcnt += chan->sampinc;

	if (chan->sampcnt > chan->double_totlength_shifted)
	{
		// Do we loop? Or are we done?
		if (chan->repeat == 1)
		{
			while (chan->sampcnt > chan->double_totlength_shifted)
				chan->sampcnt -= chan->double_totlength_shifted - static_cast<double>(chan->loopstart << 3);

			if (chan->loop_index == K_ADPCM_LOOPING_RECOVERY_INDEX)
			{
				chan->pcm16b = read16(chan->addr);
				chan->index = read08(chan->addr + 2) & 0x7F;
				chan->lastsampcnt = 7;
			}
			else
			{
				chan->pcm16b = chan->loop_pcm16b;
				chan->index = chan->loop_index;
				chan->lastsampcnt = chan->loopstart << 3;
			}
		}
		else
		{
			chan->status = CHANSTAT_STOPPED;
			SPU->KeyOff(chan->num);
			SPU->bufpos = SPU->buflength;
		}
	}
}

static inline void SPU_Mix(SPU_struct *SPU, channel_struct *chan, int32_t data, int CHANNELS)
{
	switch (CHANNELS)
	{
		case 0:
			MixL(SPU, chan, data);
			break;
		case 1:
			MixLR(SPU, chan, data);
			break;
		case 2:
			MixR(SPU, chan, data);
	}
	SPU->lastdata = data;
}

// WORK
static inline void ____SPU_ChanUpdate(SPU_struct *const SPU, channel_struct *const chan, int FORMAT, SPUInterpolationMode INTERPOLATE_MODE, int CHANNELS)
{
	for (; SPU->bufpos < SPU->buflength; ++SPU->bufpos)
	{
		if (CHANNELS != -1)
		{
			int32_t data = 0;
			switch (FORMAT)
			{
				case 0:
					Fetch8BitData(chan, &data, INTERPOLATE_MODE);
					break;
				case 1:
					Fetch16BitData(chan, &data, INTERPOLATE_MODE);
					break;
				case 2:
					FetchADPCMData(chan, &data, INTERPOLATE_MODE);
					break;
				case 3:
					FetchPSGData(chan, &data);
			}
			SPU_Mix(SPU, chan, data, CHANNELS);
		}

		switch (FORMAT)
		{
			case 0:
			case 1:
				TestForLoop(SPU, chan, FORMAT);
				break;
			case 2:
				TestForLoop2(SPU, chan);
				break;
			case 3:
				chan->sampcnt += chan->sampinc;
		}
	}
}

static inline void ___SPU_ChanUpdate(bool actuallyMix, SPU_struct *const SPU, channel_struct *const chan, int FORMAT, SPUInterpolationMode INTERPOLATE_MODE)
{
	if (!actuallyMix)
		____SPU_ChanUpdate(SPU, chan, FORMAT, INTERPOLATE_MODE, -1);
	else if (!chan->pan)
		____SPU_ChanUpdate(SPU, chan, FORMAT, INTERPOLATE_MODE, 0);
	else if (chan->pan == 127)
		____SPU_ChanUpdate(SPU, chan, FORMAT, INTERPOLATE_MODE, 2);
	else
		____SPU_ChanUpdate(SPU, chan, FORMAT, INTERPOLATE_MODE, 1);
}

static inline void __SPU_ChanUpdate(bool actuallyMix, SPU_struct *const SPU, channel_struct *const chan, SPUInterpolationMode INTERPOLATE_MODE)
{
	switch (chan->format)
	{
		case 0:
			___SPU_ChanUpdate(actuallyMix, SPU, chan, 0, INTERPOLATE_MODE);
			break;
		case 1:
			___SPU_ChanUpdate(actuallyMix, SPU, chan, 1, INTERPOLATE_MODE);
			break;
		case 2:
			___SPU_ChanUpdate(actuallyMix, SPU, chan, 2, INTERPOLATE_MODE);
			break;
		case 3:
			___SPU_ChanUpdate(actuallyMix, SPU, chan, 3, INTERPOLATE_MODE);
			break;
		default:
			assert(false);
	}
}

static inline void _SPU_ChanUpdate(bool actuallyMix, SPU_struct *const SPU, channel_struct *const chan)
{
	switch (CommonSettings.spuInterpolationMode)
	{
		case SPUInterpolation_None:
			__SPU_ChanUpdate(actuallyMix, SPU, chan, SPUInterpolation_None);
			break;
		case SPUInterpolation_Linear:
			__SPU_ChanUpdate(actuallyMix, SPU, chan, SPUInterpolation_Linear);
			break;
		case SPUInterpolation_Cosine:
			__SPU_ChanUpdate(actuallyMix, SPU, chan, SPUInterpolation_Cosine);
			break;
	}
}

// ENTERNEW
static void SPU_MixAudio_Advanced(bool, SPU_struct *SPU, int length)
{
	// the advanced spu function correctly handles all sound control mixing options, as well as capture
	// this code is not entirely optimal, as it relies on sort of manhandling the core mixing functions
	// in order to get the results it needs.

	// THIS IS MAX HACKS!!!!
	// AND NEEDS TO BE REWRITTEN ALONG WITH THE DEEPEST PARTS OF THE SPU
	// ONCE WE KNOW THAT IT WORKS

	// BIAS gets ignored since our spu is still not bit perfect,
	// and it doesnt matter for purposes of capture

	// -----------DEBUG CODE
	bool skipcap = false;
	// -----------------

	int32_t samp0[] = { 0, 0 };

	// believe it or not, we are going to do this one sample at a time.
	// like i said, it is slower.
	for (int samp = 0; samp < length; ++samp)
	{
		SPU->sndbuf[0] = SPU->sndbuf[1] = 0;
		SPU->buflength = 1;

		int32_t capmix[] = { 0, 0 }, mix[] = { 0, 0 };
		int32_t chanout[16];
		int32_t submix[32];

		// generate each channel, and helpfully mix it at the same time
		for (int i = 0; i < 16; ++i)
		{
			channel_struct *chan = &SPU->channels[i];

			if (chan->status == CHANSTAT_PLAY)
			{
				SPU->bufpos = 0;

				bool bypass = false;
				if (i == 1 && SPU->regs.ctl_ch1bypass)
					bypass = true;
				if (i == 3 && SPU->regs.ctl_ch3bypass)
					bypass = true;

				// output to mixer unless we are bypassed.
				// dont output to mixer if the user muted us
				bool outputToMix = true;
				if (CommonSettings.spu_muteChannels[i])
					outputToMix = false;
				if (bypass)
					outputToMix = false;
				bool outputToCap = outputToMix;
				if (CommonSettings.spu_captureMuted && !bypass)
					outputToCap = true;

				// channels 1 and 3 should probably always generate their audio
				// internally at least, just in case they get used by the spu output
				bool domix = outputToCap || outputToMix || i == 1 || i == 3;

				// clear the output buffer since this is where _SPU_ChanUpdate wants to accumulate things
				SPU->sndbuf[0] = SPU->sndbuf[1] = 0;

				// get channel's next output sample.
				_SPU_ChanUpdate(domix, SPU, chan);
				chanout[i] = SPU->lastdata >> chan->datashift;

				// save the panned results
				submix[i * 2] = SPU->sndbuf[0];
				submix[i * 2 + 1] = SPU->sndbuf[1];

				// send sample to our capture mix
				if (outputToCap)
				{
					capmix[0] += submix[i * 2];
					capmix[1] += submix[i * 2 + 1];
				}

				// send sample to our main mixer
				if (outputToMix)
				{
					mix[0] += submix[i * 2];
					mix[1] += submix[i * 2 + 1];
				}
			}
			else
				chanout[i] = submix[i * 2] = submix[i * 2 + 1] = 0;
		} // foreach channel

		int32_t mixout[] = { mix[0], mix[1] };
		int32_t capmixout[] = { capmix[0], capmix[1] };
		int32_t sndout[] = { 0, 0 };
		int32_t capout[2];

		// create SPU output
		switch (SPU->regs.ctl_left)
		{
			case SPU_struct::REGS::LOM_LEFT_MIXER:
				sndout[0] = mixout[0];
				break;
			case SPU_struct::REGS::LOM_CH1:
				sndout[0] = submix[2];
				break;
			case SPU_struct::REGS::LOM_CH3:
				sndout[0] = submix[6];
				break;
			case SPU_struct::REGS::LOM_CH1_PLUS_CH3:
				sndout[0] = submix[2] + submix[6];
		}
		switch (SPU->regs.ctl_right)
		{
			case SPU_struct::REGS::ROM_RIGHT_MIXER:
				sndout[1] = mixout[1];
				break;
			case SPU_struct::REGS::ROM_CH1:
				sndout[1] = submix[3];
				break;
			case SPU_struct::REGS::ROM_CH3:
				sndout[1] = submix[7];
				break;
			case SPU_struct::REGS::ROM_CH1_PLUS_CH3:
				sndout[1] = submix[3] + submix[7];
		}

		// generate capture output ("capture bugs" from gbatek are not emulated)
		if (!SPU->regs.cap[0].source)
			capout[0] = capmixout[0]; // cap0 = L-mix
		else if (SPU->regs.cap[0].add)
			capout[0] = chanout[0] + chanout[1]; // cap0 = ch0+ch1
		else
			capout[0] = chanout[0]; // cap0 = ch0

		if (!SPU->regs.cap[1].source)
			capout[1] = capmixout[1]; // cap1 = R-mix
		else if (SPU->regs.cap[1].add)
			capout[1] = chanout[2] + chanout[3]; // cap1 = ch2+ch3
		else
			capout[1] = chanout[2]; // cap1 = ch2

		capout[0] = MinMax(capout[0], -0x8000, 0x7FFF);
		capout[1] = MinMax(capout[1], -0x8000, 0x7FFF);

		// write the output sample where it is supposed to go
		if (!samp)
		{
			samp0[0] = sndout[0];
			samp0[1] = sndout[1];
		}
		else
		{
			SPU->sndbuf[samp * 2] = sndout[0];
			SPU->sndbuf[samp * 2 + 1] = sndout[1];
		}

		for (int capchan = 0; capchan < 2; ++capchan)
		{
			if (SPU->regs.cap[capchan].runtime.running)
			{
				SPU_struct::REGS::CAP &cap = SPU->regs.cap[capchan];
				uint32_t last = u32floor(cap.runtime.sampcnt);
				cap.runtime.sampcnt += SPU->channels[2 * capchan + 1].sampinc;
				uint32_t curr = u32floor(cap.runtime.sampcnt);
				for (uint32_t j = last; j < curr; ++j)
				{
					// so, this is a little strange. why go through a fifo?
					// it seems that some games will set up a reverb effect by capturing
					// to the nearly same address as playback, but ahead by a couple.
					// So, playback will always end up being what was captured a couple of samples ago.
					// This system counts on playback always having read ahead 16 samples.
					// In that case, playback will end up being what was processed at one entire buffer length ago,
					// since the 16 samples would have read ahead before they got captured over

					// It's actually the source channels which should have a fifo, but we are
					// not going to take the hit in speed and complexity. Save it for a future rewrite.
					// Instead, what we do here is delay the capture by 16 samples to create a similar effect.
					// Subjectively, it seems to be working.

					// Don't do anything until the fifo is filled, so as to delay it
					if (cap.runtime.fifo.size < 16)
					{
						cap.runtime.fifo.enqueue(static_cast<int16_t>(capout[capchan]));
						continue;
					}

					// (actually capture sample from fifo instead of most recently generated)
					int32_t sample = cap.runtime.fifo.dequeue();
					cap.runtime.fifo.enqueue(static_cast<int16_t>(capout[capchan]));

					uint32_t multiplier;
					if (cap.bits8)
					{
						int8_t sample8 = static_cast<int8_t>(sample >> 8);
						if (skipcap)
							_MMU_write08<1, MMU_AT_DMA>(cap.runtime.curdad, 0);
						else
							_MMU_write08<1, MMU_AT_DMA>(cap.runtime.curdad, sample8);
						++cap.runtime.curdad;
						multiplier = 4;
					}
					else
					{
						int16_t sample16 = static_cast<int16_t>(sample);
						if (skipcap)
							_MMU_write16<1, MMU_AT_DMA>(cap.runtime.curdad, 0);
						else
							_MMU_write16<1, MMU_AT_DMA>(cap.runtime.curdad, sample16);
						cap.runtime.curdad += 2;
						multiplier = 2;
					}

					if (cap.runtime.curdad >= cap.runtime.maxdad)
					{
						cap.runtime.curdad = cap.dad;
						cap.runtime.sampcnt -= cap.len * multiplier;
					}
				} // sampinc loop
			} // if capchan running
		} // capchan loop
	} // main sample loop

	SPU->sndbuf[0] = samp0[0];
	SPU->sndbuf[1] = samp0[1];
}

// ENTER
static void SPU_MixAudio(bool actuallyMix, SPU_struct *SPU, int length)
{
	if (actuallyMix)
	{
		memset(&SPU->sndbuf[0], 0, length * 4 * 2);
		memset(&SPU->outbuf[0], 0, length * 2 * 2);
	}

	// we used to use master enable here, and do nothing if audio is disabled.
	// now, master enable is emulated better..
	// but for a speed optimization we will still do it
	if (!SPU->regs.masteren)
		return;

	bool advanced = CommonSettings.spu_advanced;

	// branch here so that slow computers don't have to take the advanced (slower) codepath.
	// it remainds to be seen exactly how much slower it is
	// if it isnt much slower then we should refactor everything to be simpler, once it is working
	if (advanced && SPU == SPU_core.get())
		SPU_MixAudio_Advanced(actuallyMix, SPU, length);
	else
	{
		// non-advanced mode
		for (int i = 0; i < 16; ++i)
		{
			channel_struct *chan = &SPU->channels[i];

			if (chan->status != CHANSTAT_PLAY)
				continue;

			SPU->bufpos = 0;
			SPU->buflength = length;

			// Mix audio
			_SPU_ChanUpdate(!CommonSettings.spu_muteChannels[i] && actuallyMix, SPU, chan);
		}
	}

	// we used to bail out if speakers were disabled.
	// this is technically wrong. sound may still be captured, or something.
	// in all likelihood, any game doing this probably master disabled the SPU also
	// so, optimization of this case is probably not necessary.
	// later, we'll just silence the output
	bool speakers = T1ReadWord(MMU.ARM7_REG, 0x304) & 0x01;

	uint8_t vol = SPU->regs.mastervol;

	// convert from 32-bit->16-bit
	if (actuallyMix && speakers)
		for (int i = 0; i < length * 2; ++i)
		{
			// Apply Master Volume
			SPU->sndbuf[i] = spumuldiv7(SPU->sndbuf[i], vol);
			int16_t outsample = static_cast<int16_t>(MinMax(SPU->sndbuf[i], -0x8000, 0x7FFF));
			SPU->outbuf[i] = outsample;
		}
}

//////////////////////////////////////////////////////////////////////////////

// emulates one hline of the cpu core.
// this will produce a variable number of samples, calculated to keep a 44100hz output
// in sync with the emulator framerate
int spu_core_samples = 0;
void SPU_Emulate_core()
{
	bool needToMix = true;

	samples += samples_per_hline;
	spu_core_samples = static_cast<int>(samples);
	samples -= spu_core_samples;

	// We don't need to mix audio for Dual Synch/Asynch mode since we do this
	// later in SPU_Emulate_user(). Disable mixing here to speed up processing.
	// However, recording still needs to mix the audio, so make sure we're also
	// not recording before we disable mixing.
	if (synchmode == ESynchMode_DualSynchAsynch)
		needToMix = false;

	SPU_MixAudio(needToMix, SPU_core.get(), spu_core_samples);

	if (!SNDCore)
		return;

	if (SNDCore->FetchSamples)
		SNDCore->FetchSamples(&SPU_core->outbuf[0], spu_core_samples, synchmode, synchronizer.get());
	else
		SPU_DefaultFetchSamples(&SPU_core->outbuf[0], spu_core_samples, synchmode, synchronizer.get());
}

void SPU_Emulate_user(bool /*mix*/)
{
	static std::vector<int16_t> postProcessBuffer;
	static size_t postProcessBufferSize = 0;
	size_t processedSampleCount = 0;

	if (!SNDCore)
		return;

	// Check to see how many free samples are available.
	// If there are some, fill up the output buffer.
	size_t freeSampleCount = SNDCore->GetAudioSpace();
	if (!freeSampleCount)
		return;

	//printf("mix %i samples\n", audiosize);
	if (freeSampleCount > buffersize)
		freeSampleCount = buffersize;

	// If needed, resize the post-process buffer to guarantee that
	// we can store all the sound data.
	if (postProcessBufferSize < freeSampleCount * 2 * sizeof(int16_t))
	{
		postProcessBufferSize = freeSampleCount * 2 * sizeof(int16_t);
		postProcessBuffer.resize(postProcessBufferSize);
	}

	if (SNDCore->PostProcessSamples)
		processedSampleCount = SNDCore->PostProcessSamples(&postProcessBuffer[0], freeSampleCount, synchmode, synchronizer.get());
	else
		processedSampleCount = SPU_DefaultPostProcessSamples(&postProcessBuffer[0], freeSampleCount, synchmode, synchronizer.get());

	SNDCore->UpdateAudio(&postProcessBuffer[0], processedSampleCount);
}

void SPU_DefaultFetchSamples(int16_t *sampleBuffer, size_t sampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer)
{
	if (synchMode == ESynchMode_Synchronous)
		theSynchronizer->enqueue_samples(sampleBuffer, sampleCount);
}

size_t SPU_DefaultPostProcessSamples(int16_t *postProcessBuffer, size_t requestedSampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer)
{
	size_t processedSampleCount = 0;

	switch (synchMode)
	{
		case ESynchMode_DualSynchAsynch:
			if (SPU_user)
			{
				SPU_MixAudio(true, SPU_user.get(), requestedSampleCount);
				memcpy(postProcessBuffer, &SPU_user->outbuf[0], requestedSampleCount * 2 * sizeof(int16_t));
				processedSampleCount = requestedSampleCount;
			}
			break;

		case ESynchMode_Synchronous:
			processedSampleCount = theSynchronizer->output_samples(postProcessBuffer, requestedSampleCount);
	}

	return processedSampleCount;
}

//////////////////////////////////////////////////////////////////////////////
// Dummy Sound Interface
//////////////////////////////////////////////////////////////////////////////

int SNDDummyInit(int) { return 0; }
void SNDDummyDeInit() {}
void SNDDummyUpdateAudio(int16_t *, uint32_t) { }
uint32_t SNDDummyGetAudioSpace() { return DESMUME_SAMPLE_RATE / 60 + 5; }
void SNDDummyMuteAudio() {}
void SNDDummyUnMuteAudio() {}
void SNDDummySetVolume(int) {}
void SNDDummyClearBuffer() {}
void SNDDummyFetchSamples(int16_t *, size_t, ESynchMode, ISynchronizingAudioBuffer *) { }
size_t SNDDummyPostProcessSamples(int16_t *, size_t, ESynchMode, ISynchronizingAudioBuffer *) { return 0; }

SoundInterface_struct SNDDummy =
{
	SNDCORE_DUMMY,
	"Dummy Sound Interface",
	SNDDummyInit,
	SNDDummyDeInit,
	SNDDummyUpdateAudio,
	SNDDummyGetAudioSpace,
	SNDDummyMuteAudio,
	SNDDummyUnMuteAudio,
	SNDDummySetVolume,
	SNDDummyClearBuffer,
	SNDDummyFetchSamples,
	SNDDummyPostProcessSamples
};
