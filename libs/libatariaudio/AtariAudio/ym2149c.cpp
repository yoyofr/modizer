/*--------------------------------------------------------------------
	Atari Audio Library
	Small & accurate ATARI-ST audio emulation
	Arnaud Carré aka Leonard/Oxygene
	@leonard_coder
--------------------------------------------------------------------*/
// Tiny & cycle accurate ym2149 emulation.
// operate at original YM freq divided by 8 (so 250Khz, as nothing runs faster in the chip)
#include <assert.h>
#include "ym2149c.h"
#include "ym2149_tables.h"

static uint32_t sRndSeed = 1;
uint16_t stdLibRand()
{
	sRndSeed = sRndSeed*214013+2531011;
	return uint16_t((sRndSeed >> 16) & 0x7fff);
}

void	Ym2149c::Reset(uint32_t hostReplayRate, uint32_t ymClock)
{
	for (int v = 0; v < 3; v++)
	{
		m_toneCounter[v] = 0;
		m_tonePeriod[v] = 0;
	}
	m_toneEdges = (stdLibRand()&0x111)*0xf;		// YM internal edge state are un-predictable
	m_insideTimerIrq = false;
	m_hostReplayRate = hostReplayRate;
	m_ymClockOneEighth = ymClock/8;
	m_resamplingDividor = (hostReplayRate << 12) / m_ymClockOneEighth;
	m_noiseRndRack = 1;
	m_noiseEnvHalf = 0;
	for (int r=0;r<14;r++)
		WriteReg(r, (7==r)?0x3f:0);
	m_selectedReg = 0;
	m_currentLevel = 0;
	m_innerCycle = 0;
	m_envPos = 0;
	m_currentDebugThreeVoices = 0;
}

void	Ym2149c::WritePort(uint8_t port, uint8_t value)
{
	if (port & 2)
		WriteReg(m_selectedReg, value);
	else
		m_selectedReg = value;
}

void	Ym2149c::WriteReg(int reg, uint8_t value)
{
	if ((unsigned int)reg < 14)
	{
		static const uint8_t regMask[14] = { 0xff,0x0f,0xff,0x0f,0xff,0x0f,0x1f,0x3f,0x1f,0x1f,0x1f,0xff,0xff,0x0f };
		m_regs[reg] = value & regMask[reg];

		switch (reg)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		{
			const int voice = reg >> 1;
			m_tonePeriod[voice] = (m_regs[voice * 2 + 1] << 8) | m_regs[voice * 2];
/*
			// when period change and counter is above, counter will restart at next YM cycle
			// but there is some delay if you do that on 3 voices using CPU. Simulate this delay
			// with empirical offset
			if (m_toneCounter[voice] >= m_tonePeriod[voice])
				m_toneCounter[voice] = voice * 8;
*/
			if (m_tonePeriod[voice] <= 1)
			{
				if (m_insideTimerIrq)
					m_edgeNeedReset[voice] = true;
			}
			break;
		}
		case 6:
			m_noisePeriod = m_regs[6];
			break;
		case 7:
		{
			static const uint32_t masks[8] = { 0x000,0x00f,0x0f0, 0x0ff, 0xf00, 0xf0f, 0xff0, 0xfff };
			m_toneMask = masks[value & 0x7];
			m_noiseMask = masks[(value >> 3) & 0x7];
		}
			break;
		case 11:
		case 12:
			m_envPeriod = (m_regs[12] << 8) | m_regs[11];
			break;
		case 13:
		{
			static const uint8_t shapeToEnv[16] = { 0,0,0,0,1,1,1,1,2,3,4,5,6,7,8,9 };
			m_pCurrentEnv = s_envData + shapeToEnv[m_regs[13]] * 64;
			m_envPos = -32;
			m_envCounter = 0;
			break;
		}
		default:
			break;
		}
	}
}

uint8_t Ym2149c::ReadPort(uint8_t port) const
{
	if (0 == (port & 2))
		return m_regs[m_selectedReg];
	return ~0;
}

int16_t	Ym2149c::dcAdjust(uint16_t v)
{
	m_dcAdjustSum -= m_dcAdjustBuffer[m_dcAdjustPos];
	m_dcAdjustSum += v;
	m_dcAdjustBuffer[m_dcAdjustPos] = v;
	m_dcAdjustPos++;
	m_dcAdjustPos &= (1 << kDcAdjustHistoryBit) - 1;
	int32_t ov = int32_t(v) - int32_t(m_dcAdjustSum >> kDcAdjustHistoryBit);
	// max amplitude is 15bits (not 16) so dc adjuster should never overshoot
	return int16_t(ov);
}

// Tick internal YM2149 state machine at 250Khz ( 2Mhz/8 )
uint16_t Ym2149c::Tick()
{

	// three voices at same time
	const uint32_t vmask = (m_toneEdges | m_toneMask) & (m_currentNoiseMask | m_noiseMask);

	// update internal state
	for (int v = 0; v < 3; v++)
	{
		m_toneCounter[v]++;
		if (m_toneCounter[v] >= m_tonePeriod[v])
		{
			m_toneEdges ^= 0xf<<(v*4);
			m_toneCounter[v] = 0;
		}
	}

	// noise & env state machine is running half speed
	m_noiseEnvHalf ^= 1;
	if (m_noiseEnvHalf)
	{
		// noise
		m_noiseCounter++;
		if (m_noiseCounter >= m_noisePeriod)
		{
			m_currentNoiseMask = ((m_noiseRndRack ^ (m_noiseRndRack >> 2))&1) ? ~0 : 0;
			m_noiseRndRack = (m_noiseRndRack >> 1) | ((m_currentNoiseMask&1) << 16);
			m_noiseCounter = 0;
		}

		m_envCounter++;
		if ( m_envCounter >= m_envPeriod )
		{
			m_envPos++;
			if (m_envPos > 0)
				m_envPos &= 31;
			m_envCounter = 0;
		}
	}
	return vmask;
}

// called at host replay rate ( like 48Khz )
// internally update YM chip state machine at 250Khz and average output for each host sample
int16_t Ym2149c::ComputeNextSample(uint32_t* pSampleDebugInfo)
{
	uint16_t highMask = 0;
	do
	{
		highMask |= Tick();
		m_innerCycle += m_hostReplayRate;
	}
	while (m_innerCycle < m_ymClockOneEighth);
	m_innerCycle -= m_ymClockOneEighth;

	const uint32_t envLevel = m_pCurrentEnv[m_envPos + 32];
	uint32_t levels;
	levels  = ((m_regs[8] & 0x10) ? envLevel : m_regs[8]) << 0;
	levels |= ((m_regs[9] & 0x10) ? envLevel : m_regs[9]) << 4;
	levels |= ((m_regs[10] & 0x10) ? envLevel : m_regs[10]) << 8;
	levels &= highMask;
	assert(levels < 0x1000);
    
    m_currentLevel=levels;//YOYOFR

	const uint32_t indexA = ((levels >> 0) & 15) + ((m_tonePeriod[0] > 1)?0:16);
	const uint32_t indexB = ((levels >> 4) & 15) + ((m_tonePeriod[1] > 1)?0:16);
	const uint32_t indexC = ((levels >> 8) & 15) + ((m_tonePeriod[2] > 1)?0:16);
	uint32_t levelA = s_ym2149LogLevels[indexA];
	uint32_t levelB = s_ym2149LogLevels[indexB];
	uint32_t levelC = s_ym2149LogLevels[indexC];

	int16_t out = dcAdjust(levelA + levelB + levelC);
	if (pSampleDebugInfo)
		*pSampleDebugInfo = (s_ViewVolTab[indexA] << 0) | (s_ViewVolTab[indexB] << 8) | (s_ViewVolTab[indexC] << 16);

	return out;
}

void	Ym2149c::InsideTimerIrq(bool inside)
{
	if (!inside)
	{
		// when exiting timer IRQ code, do any pending edge reset ( "square-sync" modern fx )
		for (int v = 0; v < 3; v++)
		{
			if (m_edgeNeedReset[v])
			{
				m_toneEdges ^= 0xf<<(v*4);
				m_toneCounter[v] = 0;
				m_edgeNeedReset[v] = false;
			}
		}
	}
	m_insideTimerIrq = inside;
}

//YOYOFR
int Ym2149c::getYM2149_Freq(int channel) {
    double period=0;
    
    //int vol = (m_currentLevel>>(channel*4))&15;
    
    if( !(m_toneMask&(1<<(channel*4)))  ) period=m_tonePeriod[channel%3];
    else if( !(m_noiseMask&(1<<(channel*4))) ) period=m_noisePeriod;
    //m_noisePeriod
    double freq=0;
    if (period) freq=m_ymClockOneEighth/period/2;
    return freq;
}
