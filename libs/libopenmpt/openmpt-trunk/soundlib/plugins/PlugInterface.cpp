/*
 * PlugInterface.cpp
 * -----------------
 * Purpose: Default plugin interface implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "../Sndfile.h"
#include "PlugInterface.h"
#include "PluginManager.h"
#include "../../common/FileReader.h"
#ifdef MODPLUG_TRACKER
#include "../../mptrack/Moddoc.h"
#include "../../mptrack/Mainfrm.h"
#include "../../mptrack/InputHandler.h"
#include "../../mptrack/AbstractVstEditor.h"
#include "../../mptrack/DefaultVstEditor.h"
// LoadProgram/SaveProgram
#include "../../mptrack/FileDialog.h"
#include "../../mptrack/VstPresets.h"
#include "../../common/mptFileIO.h"
#include "../mod_specifications.h"
#endif // MODPLUG_TRACKER

#include <cmath>

#ifndef NO_PLUGINS

OPENMPT_NAMESPACE_BEGIN


#ifdef MODPLUG_TRACKER
CModDoc *IMixPlugin::GetModDoc() { return m_SndFile.GetpModDoc(); }
const CModDoc *IMixPlugin::GetModDoc() const { return m_SndFile.GetpModDoc(); }
#endif // MODPLUG_TRACKER


IMixPlugin::IMixPlugin(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: m_Factory(factory)
	, m_SndFile(sndFile)
	, m_pMixStruct(mixStruct)
{
	m_MixState.pMixBuffer = (mixsample_t *)((((intptr_t)m_MixBuffer) + 7) & ~7);
	while(m_pMixStruct != &(m_SndFile.m_MixPlugins[m_nSlot]) && m_nSlot < MAX_MIXPLUGINS - 1)
	{
		m_nSlot++;
	}
}


IMixPlugin::~IMixPlugin()
{
#ifdef MODPLUG_TRACKER
	CloseEditor();
	CriticalSection cs;
#endif // MODPLUG_TRACKER

	// First thing to do, if we don't want to hang in a loop
	if (m_Factory.pPluginsList == this) m_Factory.pPluginsList = m_pNext;
	if (m_pMixStruct)
	{
		m_pMixStruct->pMixPlugin = nullptr;
		m_pMixStruct = nullptr;
	}

	if (m_pNext) m_pNext->m_pPrev = m_pPrev;
	if (m_pPrev) m_pPrev->m_pNext = m_pNext;
	m_pPrev = nullptr;
	m_pNext = nullptr;
}


void IMixPlugin::InsertIntoFactoryList()
{
	m_pMixStruct->pMixPlugin = this;

	m_pNext = m_Factory.pPluginsList;
	if(m_Factory.pPluginsList)
	{
		m_Factory.pPluginsList->m_pPrev = this;
	}
	m_Factory.pPluginsList = this;
}


#ifdef MODPLUG_TRACKER

void IMixPlugin::SetSlot(PLUGINDEX slot)
{
	m_nSlot = slot;
	m_pMixStruct = &m_SndFile.m_MixPlugins[slot];
}


CString IMixPlugin::GetFormattedParamName(PlugParamIndex param)
{
	CString paramName = GetParamName(param);
	CString name;
	if(paramName.IsEmpty())
	{
		name = mpt::cformat(_T("%1: Parameter %2"))(mpt::cfmt::dec0<2>(param), mpt::cfmt::dec0<2>(param));
	} else
	{
		name = mpt::cformat(_T("%1: %2"))(mpt::cfmt::dec0<2>(param), paramName);
	}
	return name;
}


// Get a parameter's current value, represented by the plugin.
CString IMixPlugin::GetFormattedParamValue(PlugParamIndex param)
{

	CString paramDisplay = GetParamDisplay(param);
	CString paramUnits = GetParamLabel(param);
	paramDisplay.Trim();
	paramUnits.Trim();
	paramDisplay += _T(" ") + paramUnits;

	return paramDisplay;
}


CString IMixPlugin::GetFormattedProgramName(int32 index)
{
	CString rawname = GetProgramName(index);
	
	// Let's start counting at 1 for the program name (as most MIDI hardware / software does)
	index++;

	CString formattedName;
	if(rawname[0] >= 0 && rawname[0] < _T(' '))
		formattedName = mpt::cformat(_T("%1 - Program %2"))(mpt::cfmt::dec0<2>(index), index);
	else
		formattedName = mpt::cformat(_T("%1 - %2"))(mpt::cfmt::dec0<2>(index), rawname);

	return formattedName;
}


void IMixPlugin::SetEditorPos(int32 x, int32 y)
{
	m_pMixStruct->editorX = x;
	m_pMixStruct->editorY = y;
}


void IMixPlugin::GetEditorPos(int32 &x, int32 &y) const
{
	x = m_pMixStruct->editorX;
	y = m_pMixStruct->editorY;
}


#endif // MODPLUG_TRACKER


bool IMixPlugin::IsBypassed() const
{
	return m_pMixStruct != nullptr && m_pMixStruct->IsBypassed();
}


void IMixPlugin::RecalculateGain()
{
	float gain = 0.1f * static_cast<float>(m_pMixStruct ? m_pMixStruct->GetGain() : 10);
	if(gain < 0.1f) gain = 1.0f;

	if(IsInstrument())
	{
		gain /= m_SndFile.GetPlayConfig().getVSTiAttenuation();
		gain = static_cast<float>(gain * (m_SndFile.m_nVSTiVolume / m_SndFile.GetPlayConfig().getNormalVSTiVol()));
	}
	m_fGain = gain;
}


void IMixPlugin::SetDryRatio(uint32 param)
{
	param = std::min(param, uint32(127));
	m_pMixStruct->fDryRatio = 1.0f - (param / 127.0f);
}


void IMixPlugin::Bypass(bool bypass)
{
	m_pMixStruct->Info.SetBypass(bypass);

#ifdef MODPLUG_TRACKER
	if(m_SndFile.GetpModDoc())
		m_SndFile.GetpModDoc()->UpdateAllViews(nullptr, PluginHint(m_nSlot + 1).Info(), nullptr);
#endif // MODPLUG_TRACKER
}


double IMixPlugin::GetOutputLatency() const
{
	if(GetSoundFile().IsRenderingToDisc())
		return 0;
	else
		return GetSoundFile().m_TimingInfo.OutputLatency;
}


void IMixPlugin::ProcessMixOps(float * MPT_RESTRICT pOutL, float * MPT_RESTRICT pOutR, float * MPT_RESTRICT leftPlugOutput, float * MPT_RESTRICT rightPlugOutput, uint32 numFrames)
{
/*	float *leftPlugOutput;
	float *rightPlugOutput;

	if(m_Effect.numOutputs == 1)
	{
		// If there was just the one plugin output we copy it into our 2 outputs
		leftPlugOutput = rightPlugOutput = mixBuffer.GetOutputBuffer(0);
	} else if(m_Effect.numOutputs > 1)
	{
		// Otherwise we actually only cater for two outputs max (outputs > 2 have been mixed together already).
		leftPlugOutput = mixBuffer.GetOutputBuffer(0);
		rightPlugOutput = mixBuffer.GetOutputBuffer(1);
	} else
	{
		return;
	}*/

	// -> mixop == 0 : normal processing
	// -> mixop == 1 : MIX += DRY - WET * wetRatio
	// -> mixop == 2 : MIX += WET - DRY * dryRatio
	// -> mixop == 3 : MIX -= WET - DRY * wetRatio
	// -> mixop == 4 : MIX -= middle - WET * wetRatio + middle - DRY
	// -> mixop == 5 : MIX_L += wetRatio * (WET_L - DRY_L) + dryRatio * (DRY_R - WET_R)
	//                 MIX_R += dryRatio * (WET_L - DRY_L) + wetRatio * (DRY_R - WET_R)

	MPT_ASSERT(m_pMixStruct != nullptr);

	int mixop;
	if(IsInstrument())
	{
		// Force normal mix mode for instruments
		mixop = 0;
	} else
	{
		mixop = m_pMixStruct->GetMixMode();
	}

	float wetRatio = 1 - m_pMixStruct->fDryRatio;
	float dryRatio = IsInstrument() ? 1 : m_pMixStruct->fDryRatio; // Always mix full dry if this is an instrument

	// Wet / Dry range expansion [0,1] -> [-1,1]
	if(GetNumInputChannels() > 0 && m_pMixStruct->IsExpandedMix())
	{
		wetRatio = 2.0f * wetRatio - 1.0f;
		dryRatio = -wetRatio;
	}

	wetRatio *= m_fGain;
	dryRatio *= m_fGain;

	float * MPT_RESTRICT plugInputL = m_mixBuffer.GetInputBuffer(0);
	float * MPT_RESTRICT plugInputR = m_mixBuffer.GetInputBuffer(1);

	// Mix operation
	switch(mixop)
	{

	// Default mix
	case 0:
		for(uint32 i = 0; i < numFrames; i++)
		{
			//rewbs.wetratio - added the factors. [20040123]
			pOutL[i] += leftPlugOutput[i] * wetRatio + plugInputL[i] * dryRatio;
			pOutR[i] += rightPlugOutput[i] * wetRatio + plugInputR[i] * dryRatio;
		}
		break;

	// Wet subtract
	case 1:
		for(uint32 i = 0; i < numFrames; i++)
		{
			pOutL[i] += plugInputL[i] - leftPlugOutput[i] * wetRatio;
			pOutR[i] += plugInputR[i] - rightPlugOutput[i] * wetRatio;
		}
		break;

	// Dry subtract
	case 2:
		for(uint32 i = 0; i < numFrames; i++)
		{
			pOutL[i] += leftPlugOutput[i] - plugInputL[i] * dryRatio;
			pOutR[i] += rightPlugOutput[i] - plugInputR[i] * dryRatio;
		}
		break;

	// Mix subtract
	case 3:
		for(uint32 i = 0; i < numFrames; i++)
		{
			pOutL[i] -= leftPlugOutput[i] - plugInputL[i] * wetRatio;
			pOutR[i] -= rightPlugOutput[i] - plugInputR[i] * wetRatio;
		}
		break;

	// Middle subtract
	case 4:
		for(uint32 i = 0; i < numFrames; i++)
		{
			float middle = (pOutL[i] + plugInputL[i] + pOutR[i] + plugInputR[i]) / 2.0f;
			pOutL[i] -= middle - leftPlugOutput[i] * wetRatio + middle - plugInputL[i];
			pOutR[i] -= middle - rightPlugOutput[i] * wetRatio + middle - plugInputR[i];
		}
		break;

	// Left / Right balance
	case 5:
		if(m_pMixStruct->IsExpandedMix())
		{
			wetRatio /= 2.0f;
			dryRatio /= 2.0f;
		}

		for(uint32 i = 0; i < numFrames; i++)
		{
			pOutL[i] += wetRatio * (leftPlugOutput[i] - plugInputL[i]) + dryRatio * (plugInputR[i] - rightPlugOutput[i]);
			pOutR[i] += dryRatio * (leftPlugOutput[i] - plugInputL[i]) + wetRatio * (plugInputR[i] - rightPlugOutput[i]);
		}
		break;
	}

	// If dry mix is ticked, we add the unprocessed buffer,
	// except if this is an instrument since then it has already been done:
	if(m_pMixStruct->IsWetMix() && !IsInstrument())
	{
		for(uint32 i = 0; i < numFrames; i++)
		{
			pOutL[i] += plugInputL[i];
			pOutR[i] += plugInputR[i];
		}
	}
}


// Render some silence and return maximum level returned by the plugin.
float IMixPlugin::RenderSilence(uint32 numFrames)
{
	// The JUCE framework doesn't like processing while being suspended.
	const bool wasSuspended = !IsResumed();
	if(wasSuspended)
	{
		Resume();
	}

	float out[2][MIXBUFFERSIZE]; // scratch buffers
	float maxVal = 0.0f;
	m_mixBuffer.ClearInputBuffers(MIXBUFFERSIZE);

	while(numFrames > 0)
	{
		uint32 renderSamples = numFrames;
		LimitMax(renderSamples, mpt::saturate_cast<uint32>(MPT_ARRAY_COUNT(out[0])));
		MemsetZero(out);

		Process(out[0], out[1], renderSamples);
		for(size_t i = 0; i < renderSamples; i++)
		{
			maxVal = std::max(maxVal, std::fabs(out[0][i]));
			maxVal = std::max(maxVal, std::fabs(out[1][i]));
		}

		numFrames -= renderSamples;
	}

	if(wasSuspended)
	{
		Suspend();
	}

	return maxVal;
}


// Get list of plugins to which output is sent. A nullptr indicates master output.
size_t IMixPlugin::GetOutputPlugList(std::vector<IMixPlugin *> &list)
{
	// At the moment we know there will only be 1 output.
	// Returning nullptr means plugin outputs directly to master.
	list.clear();

	IMixPlugin *outputPlug = nullptr;
	if(!m_pMixStruct->IsOutputToMaster())
	{
		PLUGINDEX nOutput = m_pMixStruct->GetOutputPlugin();
		if(nOutput > m_nSlot && nOutput != PLUGINDEX_INVALID)
		{
			outputPlug = m_SndFile.m_MixPlugins[nOutput].pMixPlugin;
		}
	}
	list.push_back(outputPlug);

	return 1;
}


// Get a list of plugins that send data to this plugin.
size_t IMixPlugin::GetInputPlugList(std::vector<IMixPlugin *> &list)
{
	std::vector<IMixPlugin *> candidatePlugOutputs;
	list.clear();

	for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
	{
		IMixPlugin *candidatePlug = m_SndFile.m_MixPlugins[plug].pMixPlugin;
		if(candidatePlug)
		{
			candidatePlug->GetOutputPlugList(candidatePlugOutputs);

			for(auto &outPlug : candidatePlugOutputs)
			{
				if(outPlug == this)
				{
					list.push_back(candidatePlug);
					break;
				}
			}
		}
	}

	return list.size();
}


// Get a list of instruments that send data to this plugin.
size_t IMixPlugin::GetInputInstrumentList(std::vector<INSTRUMENTINDEX> &list)
{
	list.clear();
	const PLUGINDEX nThisMixPlug = m_nSlot + 1;		//m_nSlot is position in mixplug array.

	for(INSTRUMENTINDEX ins = 0; ins <= m_SndFile.GetNumInstruments(); ins++)
	{
		if(m_SndFile.Instruments[ins] != nullptr && m_SndFile.Instruments[ins]->nMixPlug == nThisMixPlug)
		{
			list.push_back(ins);
		}
	}

	return list.size();
}


size_t IMixPlugin::GetInputChannelList(std::vector<CHANNELINDEX> &list)
{
	list.clear();

	PLUGINDEX nThisMixPlug = m_nSlot + 1;		//m_nSlot is position in mixplug array.
	const CHANNELINDEX chnCount = m_SndFile.GetNumChannels();
	for(CHANNELINDEX nChn=0; nChn<chnCount; nChn++)
	{
		if(m_SndFile.ChnSettings[nChn].nMixPlugin == nThisMixPlug)
		{
			list.push_back(nChn);
		}
	}

	return list.size();

}


void IMixPlugin::SaveAllParameters()
{
	if (m_pMixStruct == nullptr)
	{
		return;
	}
	m_pMixStruct->defaultProgram = -1;
	
	// Default implementation: Save all parameter values
	PlugParamIndex numParams = std::min(GetNumParameters(), static_cast<int32>((std::numeric_limits<uint32>::max() - sizeof(uint32)) / sizeof(IEEE754binary32LE)));
	uint32 nLen = numParams * sizeof(IEEE754binary32LE);
	if (!nLen) return;
	nLen += sizeof(uint32);

	try
	{
		m_pMixStruct->pluginData.resize(nLen);
		auto memFile = std::make_pair(mpt::as_span(m_pMixStruct->pluginData), mpt::IO::Offset(0));
		mpt::IO::WriteIntLE<uint32>(memFile, 0);	// Plugin data type
		BeginGetProgram();
		for(PlugParamIndex i = 0; i < numParams; i++)
		{
			mpt::IO::Write(memFile, IEEE754binary32LE(GetParameter(i)));
		}
		EndGetProgram();
	} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
	{
		m_pMixStruct->pluginData.clear();
		MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
	}
}


void IMixPlugin::RestoreAllParameters(int32 /*program*/)
{
	if(m_pMixStruct != nullptr && m_pMixStruct->pluginData.size() >= sizeof(uint32))
	{
		FileReader memFile(mpt::as_span(m_pMixStruct->pluginData));
		uint32 type = memFile.ReadUint32LE();
		if(type == 0)
		{
			const uint32 numParams = GetNumParameters();
			if((m_pMixStruct->pluginData.size() - sizeof(uint32)) >= (numParams * sizeof(IEEE754binary32LE)))
			{
				BeginSetProgram();
				for(uint32 i = 0; i < numParams; i++)
				{
					SetParameter(i, memFile.ReadFloatLE());
				}
				EndSetProgram();
			}
		}
	}
}


#ifdef MODPLUG_TRACKER
void IMixPlugin::ToggleEditor()
{
	// We only really need this mutex for bridged plugins, as we may be processing window messages (in the same thread) while the editor opens.
	// The user could press the toggle button while the editor is loading and thus close the editor while still being initialized.
	// Note that this does not protect against closing the module while the editor is still loading.
	static bool initializing = false;
	if(initializing)
		return;
	initializing = true;

	if (m_pEditor)
	{
		CloseEditor();
	} else
	{
		m_pEditor = OpenEditor();

		if (m_pEditor)
			m_pEditor->OpenEditor(CMainFrame::GetMainFrame());
	}
	initializing = false;
}


// Provide default plugin editor
CAbstractVstEditor *IMixPlugin::OpenEditor()
{
	try
	{
		return new CDefaultVstEditor(*this);
	} MPT_EXCEPTION_CATCH_OUT_OF_MEMORY(e)
	{
		MPT_EXCEPTION_DELETE_OUT_OF_MEMORY(e);
		return nullptr;
	}
}


void IMixPlugin::CloseEditor()
{
	if(m_pEditor)
	{
		if (m_pEditor->m_hWnd) m_pEditor->DoClose();
		delete m_pEditor;
		m_pEditor = nullptr;
	}
}


// Automate a parameter from the plugin GUI (both custom and default plugin GUI)
void IMixPlugin::AutomateParameter(PlugParamIndex param)
{
	CModDoc *modDoc = GetModDoc();
	if(modDoc == nullptr)
	{
		return;
	}

	// TODO: Check if any params are actually automatable, and if there are but this one isn't, chicken out

	if(m_recordAutomation)
	{
		// Record parameter change
		modDoc->RecordParamChange(GetSlot(), param);
	}

	modDoc->SendNotifyMessageToAllViews(WM_MOD_PLUGPARAMAUTOMATE, m_nSlot, param);

	if(auto *vstEditor = GetEditor(); vstEditor && vstEditor->m_hWnd)
	{
		// Mark track modified if GUI is open and format supports plugins
		SetModified();

		// Do not use InputHandler in case we are coming from a bridged plugin editor
		if((GetAsyncKeyState(VK_SHIFT) & 0x8000) && TrackerSettings::Instance().midiMappingInPluginEditor)
		{
			// Shift pressed -> Open MIDI mapping dialog
			CMainFrame::GetMainFrame()->PostMessage(WM_MOD_MIDIMAPPING, m_nSlot, param);
		}

		// Learn macro
		int macroToLearn = vstEditor->GetLearnMacro();
		if (macroToLearn > -1)
		{
			modDoc->LearnMacro(macroToLearn, param);
			vstEditor->SetLearnMacro(-1);
		}
	}
}


void IMixPlugin::SetModified()
{
	CModDoc *modDoc = GetModDoc();
	if(modDoc != nullptr && m_SndFile.GetModSpecifications().supportsPlugins)
	{
		modDoc->SetModified();
	}
}


bool IMixPlugin::SaveProgram()
{
	mpt::PathString defaultDir = TrackerSettings::Instance().PathPluginPresets.GetWorkingDir();
	const bool useDefaultDir = !defaultDir.empty();
	if(!useDefaultDir && m_Factory.dllPath.IsFile())
	{
		defaultDir = m_Factory.dllPath.GetPath();
	}

	CString progName = m_Factory.libraryName.ToCString() + _T(" - ") + GetCurrentProgramName();
	SanitizeFilename(progName);

	FileDialog dlg = SaveFileDialog()
		.DefaultExtension("fxb")
		.DefaultFilename(progName)
		.ExtensionFilter("VST Plugin Programs (*.fxp)|*.fxp|"
			"VST Plugin Banks (*.fxb)|*.fxb||")
		.WorkingDirectory(defaultDir);
	if(!dlg.Show(m_pEditor)) return false;

	if(useDefaultDir)
	{
		TrackerSettings::Instance().PathPluginPresets.SetWorkingDir(dlg.GetWorkingDirectory());
	}

	const bool isBank = (dlg.GetExtension() == P_("fxb"));

	try
	{
		mpt::SafeOutputFile sf(dlg.GetFirstFile(), std::ios::binary, mpt::FlushModeFromBool(TrackerSettings::Instance().MiscFlushFileBuffersOnSave));
		mpt::ofstream &f = sf;
		f.exceptions(f.exceptions() | std::ios::badbit | std::ios::failbit);
		if(f.good() && VSTPresets::SaveFile(f, *this, isBank))
			return true;
	} catch(const std::exception &)
	{
		
	}
	Reporting::Error("Error saving preset.", m_pEditor);
	return false;
}


bool IMixPlugin::LoadProgram(mpt::PathString fileName)
{
	mpt::PathString defaultDir = TrackerSettings::Instance().PathPluginPresets.GetWorkingDir();
	bool useDefaultDir = !defaultDir.empty();
	if(!useDefaultDir && m_Factory.dllPath.IsFile())
	{
		defaultDir = m_Factory.dllPath.GetPath();
	}

	if(fileName.empty())
	{
		FileDialog dlg = OpenFileDialog()
			.DefaultExtension("fxp")
			.ExtensionFilter("VST Plugin Programs and Banks (*.fxp,*.fxb)|*.fxp;*.fxb|"
			"VST Plugin Programs (*.fxp)|*.fxp|"
			"VST Plugin Banks (*.fxb)|*.fxb|"
			"All Files|*.*||")
			.WorkingDirectory(defaultDir);
		if(!dlg.Show(m_pEditor)) return false;

		if(useDefaultDir)
		{
			TrackerSettings::Instance().PathPluginPresets.SetWorkingDir(dlg.GetWorkingDirectory());
		}
		fileName = dlg.GetFirstFile();
	}

	const char *errorStr = nullptr;
	InputFile f(fileName, SettingCacheCompleteFileBeforeLoading());
	if(f.IsValid())
	{
		FileReader file = GetFileReader(f);
		errorStr = VSTPresets::GetErrorMessage(VSTPresets::LoadFile(file, *this));
	} else
	{
		errorStr = "Can't open file.";
	}

	if(errorStr == nullptr)
	{
		if(GetModDoc() != nullptr && GetSoundFile().GetModSpecifications().supportsPlugins)
		{
			GetModDoc()->SetModified();
		}
		return true;
	} else
	{
		Reporting::Error(errorStr, m_pEditor);
		return false;
	}
}


#endif // MODPLUG_TRACKER


////////////////////////////////////////////////////////////////////
// IMidiPlugin: Default implementation of plugins with MIDI input //
////////////////////////////////////////////////////////////////////

IMidiPlugin::IMidiPlugin(VSTPluginLib &factory, CSoundFile &sndFile, SNDMIXPLUGIN *mixStruct)
	: IMixPlugin(factory, sndFile, mixStruct)
	, m_MidiCh{{}}
{
	for(auto &chn : m_MidiCh)
	{
		chn.midiPitchBendPos = EncodePitchBendParam(MIDIEvents::pitchBendCentre); // centre pitch bend on all channels
		chn.ResetProgram();
	}
}


void IMidiPlugin::ApplyPitchWheelDepth(int32 &value, int8 pwd)
{
	if(pwd != 0)
	{
		value = (value * ((MIDIEvents::pitchBendMax - MIDIEvents::pitchBendCentre + 1) / 64)) / pwd;
	} else
	{
		value = 0;
	}
}


// Get the MIDI channel currently associated with a given tracker channel
uint8 IMidiPlugin::GetMidiChannel(CHANNELINDEX trackChannel) const
{
	if(trackChannel >= std::size(m_SndFile.m_PlayState.Chn))
		return 0;

	if(auto ins = m_SndFile.m_PlayState.Chn[trackChannel].pModInstrument; ins != nullptr)
		return ins->GetMIDIChannel(m_SndFile, trackChannel);
	else
		return 0;
}


void IMidiPlugin::MidiCC(MIDIEvents::MidiCC nController, uint8 nParam, CHANNELINDEX trackChannel)
{
	//Error checking
	LimitMax(nController, MIDIEvents::MIDICC_end);
	LimitMax(nParam, uint8(127));
	auto midiCh = GetMidiChannel(trackChannel);

	if(m_SndFile.m_playBehaviour[kMIDICCBugEmulation])
		MidiSend(MIDIEvents::Event(MIDIEvents::evControllerChange, midiCh, nParam, static_cast<uint8>(nController)));	// param and controller are swapped (old broken implementation)
	else
		MidiSend(MIDIEvents::CC(nController, midiCh, nParam));
}


// Bend MIDI pitch for given MIDI channel using fine tracker param (one unit = 1/64th of a note step)
void IMidiPlugin::MidiPitchBend(int32 increment, int8 pwd, CHANNELINDEX trackerChn)
{
	auto midiCh = GetMidiChannel(trackerChn);
	if(m_SndFile.m_playBehaviour[kOldMIDIPitchBends])
	{
		// OpenMPT Legacy: Old pitch slides never were really accurate, but setting the PWD to 13 in plugins would give the closest results.
		increment = (increment * 0x800 * 13) / (0xFF * pwd);
		increment = EncodePitchBendParam(increment);
	} else
	{
		increment = EncodePitchBendParam(increment);
		ApplyPitchWheelDepth(increment, pwd);
	}

	int32 newPitchBendPos = (increment + m_MidiCh[midiCh].midiPitchBendPos) & vstPitchBendMask;
	Limit(newPitchBendPos, EncodePitchBendParam(MIDIEvents::pitchBendMin), EncodePitchBendParam(MIDIEvents::pitchBendMax));

	MidiPitchBend(midiCh, newPitchBendPos);
}


// Set MIDI pitch for given MIDI channel using fixed point pitch bend value (converted back to 0-16383 MIDI range)
void IMidiPlugin::MidiPitchBend(uint8 midiCh, int32 newPitchBendPos)
{
	MPT_ASSERT(EncodePitchBendParam(MIDIEvents::pitchBendMin) <= newPitchBendPos && newPitchBendPos <= EncodePitchBendParam(MIDIEvents::pitchBendMax));
	m_MidiCh[midiCh].midiPitchBendPos = newPitchBendPos;
	MidiSend(MIDIEvents::PitchBend(midiCh, DecodePitchBendParam(newPitchBendPos)));
}


// Apply vibrato effect through pitch wheel commands on a given MIDI channel.
void IMidiPlugin::MidiVibrato(int32 depth, int8 pwd, CHANNELINDEX trackerChn)
{
	auto midiCh = GetMidiChannel(trackerChn);
	depth = EncodePitchBendParam(depth);
	if(depth != 0 || (m_MidiCh[midiCh].midiPitchBendPos & vstVibratoFlag))
	{
		ApplyPitchWheelDepth(depth, pwd);

		// Temporarily add vibrato offset to current pitch
		int32 newPitchBendPos = (depth + m_MidiCh[midiCh].midiPitchBendPos) & vstPitchBendMask;
		Limit(newPitchBendPos, EncodePitchBendParam(MIDIEvents::pitchBendMin), EncodePitchBendParam(MIDIEvents::pitchBendMax));

		MidiSend(MIDIEvents::PitchBend(midiCh, DecodePitchBendParam(newPitchBendPos)));
	}

	// Update vibrato status
	if(depth != 0)
		m_MidiCh[midiCh].midiPitchBendPos |= vstVibratoFlag;
	else
		m_MidiCh[midiCh].midiPitchBendPos &= ~vstVibratoFlag;
}


void IMidiPlugin::MidiCommand(const ModInstrument &instr, uint16 note, uint16 vol, CHANNELINDEX trackChannel)
{
	auto midiCh = GetMidiChannel(trackChannel);
	PlugInstrChannel &channel = m_MidiCh[midiCh];

	uint16 midiBank = instr.wMidiBank - 1;
	uint8 midiProg = instr.nMidiProgram - 1;
	bool bankChanged = (channel.currentBank != midiBank) && (midiBank < 0x4000);
	bool progChanged = (channel.currentProgram != midiProg) && (midiProg < 0x80);
	//get vol in [0,128[
	uint8 volume = static_cast<uint8>(std::min(vol / 2u, 127u));

	// Bank change
	if(bankChanged)
	{
		uint8 high = static_cast<uint8>(midiBank >> 7);
		uint8 low = static_cast<uint8>(midiBank & 0x7F);

		//m_SndFile.ProcessMIDIMacro(trackChannel, false, m_SndFile.m_MidiCfg.szMidiGlb[MIDIOUT_BANKSEL], 0, m_nSlot + 1);
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_BankSelect_Coarse, midiCh, high));
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_BankSelect_Fine, midiCh, low));

		channel.currentBank = midiBank;
	}

	// Program change
	// According to the MIDI specs, a bank change alone doesn't have to change the active program - it will only change the bank of subsequent program changes.
	// Thus we send program changes also if only the bank has changed.
	if(progChanged || (midiProg < 0x80 && bankChanged))
	{
		channel.currentProgram = midiProg;
		//m_SndFile.ProcessMIDIMacro(trackChannel, false, m_SndFile.m_MidiCfg.szMidiGlb[MIDIOUT_PROGRAM], 0, m_nSlot + 1);
		MidiSend(MIDIEvents::ProgramChange(midiCh, midiProg));
	}


	// Specific Note Off
	if(note > NOTE_MAX_SPECIAL)
	{
		uint8 i = static_cast<uint8>(note - NOTE_MAX_SPECIAL - NOTE_MIN);
		if(channel.noteOnMap[i][trackChannel])
		{
			channel.noteOnMap[i][trackChannel]--;
			MidiSend(MIDIEvents::NoteOff(midiCh, i, 0));
		}
	}

	// "Hard core" All Sounds Off on this midi and tracker channel
	// This one doesn't check the note mask - just one note off per note.
	// Also less likely to cause a VST event buffer overflow.
	else if(note == NOTE_NOTECUT)	// ^^
	{
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllNotesOff, midiCh, 0));
		MidiSend(MIDIEvents::CC(MIDIEvents::MIDICC_AllSoundOff, midiCh, 0));

		// Turn off all notes
		for(uint8 i = 0; i < CountOf(channel.noteOnMap); i++)
		{
			channel.noteOnMap[i][trackChannel] = 0;
			MidiSend(MIDIEvents::NoteOff(midiCh, i, volume));
		}

	}

	// All "active" notes off on this midi and tracker channel
	// using note mask.
	else if(note == NOTE_KEYOFF || note == NOTE_FADE) // ==, ~~
	{
		for(uint8 i = 0; i < CountOf(channel.noteOnMap); i++)
		{
			// Some VSTis need a note off for each instance of a note on, e.g. fabfilter.
			while(channel.noteOnMap[i][trackChannel])
			{
				MidiSend(MIDIEvents::NoteOff(midiCh, i, volume));
				channel.noteOnMap[i][trackChannel]--;
			}
		}
	}

	// Note On
	else if(ModCommand::IsNote(static_cast<ModCommand::NOTE>(note)))
	{
		note -= NOTE_MIN;

		// Reset pitch bend on each new note, tracker style.
		// This is done if the pitch wheel has been moved or there was a vibrato on the previous row (in which case the "vstVibratoFlag" bit of the pitch bend memory is set)
		if(m_MidiCh[midiCh].midiPitchBendPos != EncodePitchBendParam(MIDIEvents::pitchBendCentre))
		{
			MidiPitchBend(midiCh, EncodePitchBendParam(MIDIEvents::pitchBendCentre));
		}

		// count instances of active notes.
		// This is to send a note off for each instance of a note, for plugs like Fabfilter.
		// Problem: if a note dies out naturally and we never send a note off, this counter
		// will block at max until note off. Is this a problem?
		// Safe to assume we won't need more than 16 note offs max on a given note?
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable:6385) // false-positive: Reading invalid data from 'channel.noteOnMap': the readable size is '32768' bytes, but 'note' bytes may be read.
#endif // MPT_COMPILER_MSVC
		if(channel.noteOnMap[note][trackChannel] < uint8_max)
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC
		{
			channel.noteOnMap[note][trackChannel]++;
		}

		MidiSend(MIDIEvents::NoteOn(midiCh, static_cast<uint8>(note), volume));
	}
}


bool IMidiPlugin::IsNotePlaying(uint8 note, CHANNELINDEX trackerChn)
{
	if(!ModCommand::IsNote(note) || trackerChn >= std::size(m_MidiCh[GetMidiChannel(trackerChn)].noteOnMap[note]))
		return false;

	note -= NOTE_MIN;
	return (m_MidiCh[GetMidiChannel(trackerChn)].noteOnMap[note][trackerChn] != 0);
}


void IMidiPlugin::ReceiveMidi(uint32 midiCode)
{
	ResetSilence();

	// I think we should only route events to plugins that are explicitely specified as output plugins of the current plugin.
	// This should probably use GetOutputPlugList here if we ever get to support multiple output plugins.
	PLUGINDEX receiver;
	if(m_pMixStruct != nullptr && (receiver = m_pMixStruct->GetOutputPlugin()) != PLUGINDEX_INVALID)
	{
		IMixPlugin *plugin = m_SndFile.m_MixPlugins[receiver].pMixPlugin;
		// Add all events to the plugin's queue.
		plugin->MidiSend(midiCode);
	}

#ifdef MODPLUG_TRACKER
	if(m_recordMIDIOut)
	{
		// Spam MIDI data to all views
		::PostMessage(CMainFrame::GetMainFrame()->GetMidiRecordWnd(), WM_MOD_MIDIMSG, midiCode, reinterpret_cast<LPARAM>(this));
	}
#endif // MODPLUG_TRACKER
}


void IMidiPlugin::ReceiveSysex(mpt::const_byte_span sysex)
{
	ResetSilence();

	// I think we should only route events to plugins that are explicitely specified as output plugins of the current plugin.
	// This should probably use GetOutputPlugList here if we ever get to support multiple output plugins.
	PLUGINDEX receiver;
	if(m_pMixStruct != nullptr && (receiver = m_pMixStruct->GetOutputPlugin()) != PLUGINDEX_INVALID)
	{
		IMixPlugin *plugin = m_SndFile.m_MixPlugins[receiver].pMixPlugin;
		// Add all events to the plugin's queue.
		plugin->MidiSysexSend(sysex);
	}
}


// SNDMIXPLUGIN functions

void SNDMIXPLUGIN::SetGain(uint8 gain)
{
	Info.gain = gain;
	if(pMixPlugin != nullptr) pMixPlugin->RecalculateGain();
}


void SNDMIXPLUGIN::SetBypass(bool bypass)
{
	if(pMixPlugin != nullptr)
		pMixPlugin->Bypass(bypass);
	else
		Info.SetBypass(bypass);
}


void SNDMIXPLUGIN::Destroy()
{
	if(pMixPlugin)
	{
		pMixPlugin->Release();
		pMixPlugin = nullptr;
	}
	pluginData.clear();
	pluginData.shrink_to_fit();
}

OPENMPT_NAMESPACE_END

#endif // NO_PLUGINS
