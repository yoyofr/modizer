/*
 * ModChannel.cpp
 * --------------
 * Purpose: Module Channel header class and helpers
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "Sndfile.h"
#include "ModChannel.h"

OPENMPT_NAMESPACE_BEGIN

void ModChannel::Reset(ResetFlags resetMask, const CSoundFile &sndFile, CHANNELINDEX sourceChannel)
//-------------------------------------------------------------------------------------------------
{
	if(resetMask & resetSetPosBasic)
	{
		nNote = nNewNote = NOTE_NONE;
		nNewIns = nOldIns = 0;
		pModSample = nullptr;
		pModInstrument = nullptr;
		nPortamentoDest = 0;
		nCommand = CMD_NONE;
		nPatternLoopCount = 0;
		nPatternLoop = 0;
		nFadeOutVol = 0;
		dwFlags.set(CHN_KEYOFF | CHN_NOTEFADE);
		//IT compatibility 15. Retrigger
		if(sndFile.IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			nRetrigParam = 1;
			nRetrigCount = 0;
		}
		nTremorCount = 0;
		nEFxSpeed = 0;
		proTrackerOffset = 0;
		lastZxxParam = 0xFF;
		isFirstTick = false;
	}

	if(resetMask & resetSetPosAdvanced)
	{
		nPeriod = 0;
		nPos = 0;
		nPosLo = 0;
		nLength = 0;
		nLoopStart = 0;
		nLoopEnd = 0;
		nROfs = nLOfs = 0;
		pModSample = nullptr;
		pModInstrument = nullptr;
		nCutOff = 0x7F;
		nResonance = 0;
		nFilterMode = 0;
		rightVol = leftVol = 0;
		newRightVol = newLeftVol = 0;
		rightRamp = leftRamp = 0;
		nVolume = 256;
		nVibratoPos = nTremoloPos = nPanbrelloPos = 0;
		nOldHiOffset = 0;

		//-->Custom tuning related
		m_ReCalculateFreqOnFirstTick = false;
		m_CalculateFreq = false;
		m_PortamentoFineSteps = 0;
		m_PortamentoTickSlide = 0;
		m_Freq = 0;
		m_VibratoDepth = 0;
		//<--Custom tuning related.
	}

	if(resetMask & resetChannelSettings)
	{
		if(sourceChannel < MAX_BASECHANNELS)
		{
			dwFlags = sndFile.ChnSettings[sourceChannel].dwFlags;
			nPan = sndFile.ChnSettings[sourceChannel].nPan;
			nGlobalVol = sndFile.ChnSettings[sourceChannel].nVolume;
		} else
		{
			dwFlags.reset();
			nPan = 128;
			nGlobalVol = 64;
		}
		nRestorePanOnNewNote = 0;
		nRestoreCutoffOnNewNote = 0;
		nRestoreResonanceOnNewNote = 0;

	}
}


OPENMPT_NAMESPACE_END
