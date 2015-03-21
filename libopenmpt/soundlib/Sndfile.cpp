/*
 * Sndfile.cpp
 * -----------
 * Purpose: Core class of the playback engine. Every song is represented by a CSoundFile object.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/Mptrack.h"	// For CTrackApp::OpenURL
#include "../mptrack/TrackerSettings.h"
#include "../mptrack/Moddoc.h"
#include "../mptrack/Reporting.h"
#endif // MODPLUG_TRACKER
#ifdef MPT_EXTERNAL_SAMPLES
#include "../common/mptFileIO.h"
#endif // MPT_EXTERNAL_SAMPLES
#include "../common/version.h"
#include "../common/AudioCriticalSection.h"
#include "../common/mptIO.h"
#include "../common/serialization_utils.h"
#include "Sndfile.h"
#include "tuningcollection.h"
#include "../common/StringFixer.h"
#include "../common/FileReader.h"
#include <sstream>
#include <time.h>

#ifndef NO_ARCHIVE_SUPPORT
#include "../unarchiver/unarchiver.h"
#endif // NO_ARCHIVE_SUPPORT


OPENMPT_NAMESPACE_BEGIN


// -> CODE#0027
// -> DESC="per-instrument volume ramping setup (refered as attack)"

/*---------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------
MODULAR (in/out) ModInstrument :
-----------------------------------------------------------------------------------------------

* to update:
------------

- both following functions need to be updated when adding a new member in ModInstrument :

void WriteInstrumentHeaderStructOrField(ModInstrument * input, FILE * file, uint32 only_this_code, int16 fixedsize);
bool ReadInstrumentHeaderField(ModInstrument * input, uint32 fcode, int16 fsize, FileReader &file);

- see below for body declaration.


* members:
----------

- 32bit identification CODE tag (must be unique)
- 16bit content SIZE in byte(s)
- member field


* CODE tag naming convention:
-----------------------------

- have a look below in current tag dictionnary
- take the initial ones of the field name
- 4 caracters code (not more, not less)
- must be filled with '.' caracters if code has less than 4 caracters
- for arrays, must include a '[' caracter following significant caracters ('.' not significant!!!)
- use only caracters used in full member name, ordered as they appear in it
- match caracter attribute (small,capital)

Example with "PanEnv.nLoopEnd" , "PitchEnv.nLoopEnd" & "VolEnv.Values[MAX_ENVPOINTS]" members :
- use 'PLE.' for PanEnv.nLoopEnd
- use 'PiLE' for PitchEnv.nLoopEnd
- use 'VE[.' for VolEnv.Values[MAX_ENVPOINTS]


* In use CODE tag dictionary (alphabetical order): [ see in Sndfile.cpp ]
-------------------------------------------------------------------------

						!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						!!! SECTION TO BE UPDATED !!!
						!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		[EXT]	means external (not related) to ModInstrument content

C...	[EXT]	nChannels
ChnS	[EXT]	IT/MPTM: Channel settings for channels 65-127 if needed (doesn't fit to IT header).
CS..			nCutSwing
CWV.	[EXT]	dwCreatedWithVersion
DCT.			nDCT;
dF..			dwFlags;
DGV.	[EXT]	nDefaultGlobalVolume
DT..	[EXT]	nDefaultTempo;
DNA.			nDNA;
EBIH	[EXT]	embeded instrument header tag (ITP file format)
FM..			nFilterMode;
fn[.			filename[12];
FO..			nFadeOut;
GV..			nGlobalVol;
IFC.			nIFC;
IFR.			nIFR;
K[.				Keyboard[128];
LSWV	[EXT]	Last Saved With Version
MB..			wMidiBank;
MC..			nMidiChannel;
MDK.			nMidiDrumKey;
MIMA	[EXT]									MIdi MApping directives
MiP.			nMixPlug;
MP..			nMidiProgram;
MPTS	[EXT]									Extra song info tag
MPTX	[EXT]									EXTRA INFO tag
MSF.	[EXT]									Mod(Specific)Flags
n[..			name[32];
NNA.			nNNA;
NM[.			NoteMap[128];
P...			nPan;
PE..			PanEnv.nNodes;
PE[.			PanEnv.Values[MAX_ENVPOINTS];
PiE.			PitchEnv.nNodes;
PiE[			PitchEnv.Values[MAX_ENVPOINTS];
PiLE			PitchEnv.nLoopEnd;
PiLS			PitchEnv.nLoopStart;
PiP[			PitchEnv.Ticks[MAX_ENVPOINTS];
PiSB			PitchEnv.nSustainStart;
PiSE			PitchEnv.nSustainEnd;
PLE.			PanEnv.nLoopEnd;
PLS.			PanEnv.nLoopStart;
PMM.	[EXT]	nPlugMixMode;
PP[.			PanEnv.Ticks[MAX_ENVPOINTS];
PPC.			nPPC;
PPS.			nPPS;
PS..			nPanSwing;
PSB.			PanEnv.nSustainStart;
PSE.			PanEnv.nSustainEnd;
PTTL			wPitchToTempoLock;
PVEH			nPluginVelocityHandling;
PVOH			nPluginVolumeHandling;
R...			nResampling;
RP..	[EXT]	nRestartPos;
RPB.	[EXT]	nRowsPerBeat;
RPM.	[EXT]	nRowsPerMeasure;
RS..			nResSwing;
SEP@	[EXT]	chunk SEPARATOR tag
SPA.	[EXT]	m_nSamplePreAmp;
TM..	[EXT]	nTempoMode;
VE..			VolEnv.nNodes;
VE[.			VolEnv.Values[MAX_ENVPOINTS];
VLE.			VolEnv.nLoopEnd;
VLS.			VolEnv.nLoopStart;
VP[.			VolEnv.Ticks[MAX_ENVPOINTS];
VR..			nVolRampUp;
VS..			nVolSwing;
VSB.			VolEnv.nSustainStart;
VSE.			VolEnv.nSustainEnd;
VSTV	[EXT]	nVSTiVolume;
PERN			PitchEnv.nReleaseNode
AERN			PanEnv.nReleaseNode
VERN			VolEnv.nReleaseNode
PFLG			PitchEnv.dwFlag
AFLG			PanEnv.dwFlags
VFLG			VolEnv.dwFlags
MPWD			MIDI Pitch Wheel Depth
-----------------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------------*/

#define MULTICHAR_STRING_TO_INT(str) MULTICHAR4_LE_MSVC((str)[0],(str)[1],(str)[2],(str)[3])

#ifndef MODPLUG_NO_FILESAVE

template<typename T, bool is_signed> struct IsNegativeFunctor { bool operator()(T val) const { return val < 0; } };
template<typename T> struct IsNegativeFunctor<T, true> { bool operator()(T val) const { return val < 0; } };
template<typename T> struct IsNegativeFunctor<T, false> { bool operator()(T /*val*/) const { return false; } };

template<typename T>
bool IsNegative(const T &val)
{
	return IsNegativeFunctor<T, std::numeric_limits<T>::is_signed>()(val);
}

// ------------------------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for single type members ONLY (non-array)
// ------------------------------------------------------------------------------------------
#define WRITE_MPTHEADER_sized_member(name,type,code) \
	static_assert(sizeof(input->name) == sizeof(type), "Instrument property does match specified type!");\
	fcode = MULTICHAR_STRING_TO_INT(#code);\
	fsize = sizeof( type );\
	if(only_this_code == Util::MaxValueOfType(only_this_code)) \
	{ \
		mpt::IO::WriteIntLE<uint32>(file, fcode); \
		mpt::IO::WriteIntLE<int16>(file, fsize); \
	} else if(only_this_code == fcode)\
	{ \
		MPT_ASSERT(fixedsize == fsize); \
	} \
	if(only_this_code == fcode || only_this_code == Util::MaxValueOfType(only_this_code)) \
	{ \
		type tmp = input-> name; \
		tmp = SwapBytesReturnLE(tmp); \
		fwrite(&tmp , 1 , fsize , file); \
	} \
/**/

// -----------------------------------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for single type members which are written truncated
// -----------------------------------------------------------------------------------------------------
#define WRITE_MPTHEADER_trunc_member(name,type,code) \
	static_assert(sizeof(input->name) > sizeof(type), "Instrument property would not be truncated, use WRITE_MPTHEADER_sized_member instead!");\
	fcode = MULTICHAR_STRING_TO_INT(#code);\
	fsize = sizeof( type );\
	if(only_this_code == Util::MaxValueOfType(only_this_code)) \
	{ \
		mpt::IO::WriteIntLE<uint32>(file, fcode); \
		mpt::IO::WriteIntLE<int16>(file, fsize); \
		type tmp = (type)(input-> name ); \
		tmp = SwapBytesReturnLE(tmp); \
		fwrite(&tmp , 1 , fsize , file); \
	} else if(only_this_code == fcode)\
	{ \
		/* hackish workaround to resolve mismatched size values: */ \
		/* nResampling was a long time declared as uint32 but these macro tables used uint16 and UINT. */ \
		/* This worked fine on little-endian, on big-endian not so much. Thus support writing size-mismatched fields. */ \
		MPT_ASSERT(fixedsize >= fsize); \
		type tmp = (type)(input-> name ); \
		tmp = SwapBytesReturnLE(tmp); \
		fwrite(&tmp , 1 , fsize , file); \
		if(fixedsize > fsize) \
		{ \
			for(int16 i = 0; i < fixedsize - fsize; ++i) \
			{ \
				uint8 fillbyte = !IsNegative(tmp) ? 0 : 0xff; /* sign extend */ \
				fwrite(&fillbyte, 1, 1, file); \
			} \
		} \
	} \
/**/

// ------------------------------------------------------------------------
// Convenient macro to help WRITE_HEADER declaration for array members ONLY
// ------------------------------------------------------------------------
#define WRITE_MPTHEADER_array_member(name,type,code,arraysize) \
	STATIC_ASSERT(sizeof(type) == sizeof(input-> name [0])); \
	MPT_ASSERT(sizeof(input->name) >= sizeof(type) * arraysize);\
	fcode = MULTICHAR_STRING_TO_INT(#code);\
	fsize = sizeof( type ) * arraysize;\
	if(only_this_code == Util::MaxValueOfType(only_this_code)) \
	{ \
		mpt::IO::WriteIntLE<uint32>(file, fcode); \
		mpt::IO::WriteIntLE<int16>(file, fsize); \
	} else if(only_this_code == fcode)\
	{ \
		/* MPT_ASSERT(fixedsize <= fsize); */ \
		fsize = fixedsize; /* just trust the size we got passed */ \
	} \
	if(only_this_code == fcode || only_this_code == Util::MaxValueOfType(only_this_code)) \
	{ \
		for(std::size_t i = 0; i < fsize/sizeof(type); ++i) \
		{ \
			type tmp; \
			tmp = input-> name [i]; \
			tmp = SwapBytesReturnLE(tmp); \
			fwrite(&tmp, 1, sizeof(type), file); \
		} \
	} \
/**/

// Create 'dF..' entry.
static DWORD CreateExtensionFlags(const ModInstrument& ins)
//---------------------------------------------------------
{
	DWORD dwFlags = 0;
	if(ins.VolEnv.dwFlags[ENV_ENABLED])		dwFlags |= dFdd_VOLUME;
	if(ins.VolEnv.dwFlags[ENV_SUSTAIN])		dwFlags |= dFdd_VOLSUSTAIN;
	if(ins.VolEnv.dwFlags[ENV_LOOP])		dwFlags |= dFdd_VOLLOOP;
	if(ins.PanEnv.dwFlags[ENV_ENABLED])		dwFlags |= dFdd_PANNING;
	if(ins.PanEnv.dwFlags[ENV_SUSTAIN])		dwFlags |= dFdd_PANSUSTAIN;
	if(ins.PanEnv.dwFlags[ENV_LOOP])		dwFlags |= dFdd_PANLOOP;
	if(ins.PitchEnv.dwFlags[ENV_ENABLED])	dwFlags |= dFdd_PITCH;
	if(ins.PitchEnv.dwFlags[ENV_SUSTAIN])	dwFlags |= dFdd_PITCHSUSTAIN;
	if(ins.PitchEnv.dwFlags[ENV_LOOP])		dwFlags |= dFdd_PITCHLOOP;
	if(ins.dwFlags[INS_SETPANNING])			dwFlags |= dFdd_SETPANNING;
	if(ins.PitchEnv.dwFlags[ENV_FILTER])	dwFlags |= dFdd_FILTER;
	if(ins.VolEnv.dwFlags[ENV_CARRY])		dwFlags |= dFdd_VOLCARRY;
	if(ins.PanEnv.dwFlags[ENV_CARRY])		dwFlags |= dFdd_PANCARRY;
	if(ins.PitchEnv.dwFlags[ENV_CARRY])		dwFlags |= dFdd_PITCHCARRY;
	if(ins.dwFlags[INS_MUTE])				dwFlags |= dFdd_MUTE;
	return dwFlags;
}

// Write (in 'file') 'input' ModInstrument with 'code' & 'size' extra field infos for each member
void WriteInstrumentHeaderStructOrField(ModInstrument * input, FILE * file, uint32 only_this_code, int16 fixedsize)
{
uint32 fcode;
int16 fsize;

if(only_this_code != Util::MaxValueOfType(only_this_code))
{
	MPT_ASSERT(fixedsize > 0);
}

WRITE_MPTHEADER_sized_member(	nFadeOut				, UINT			, FO..							)

if(only_this_code == Util::MaxValueOfType(only_this_code) || only_this_code == MULTICHAR_STRING_TO_INT("dF..")){ // dwFlags needs to be constructed so write it manually.
	//WRITE_MPTHEADER_sized_member(	dwFlags					, DWORD			, dF..							)
	uint32 dwFlags = CreateExtensionFlags(*input);
	fcode = MULTICHAR_STRING_TO_INT("dF..");
	fsize = sizeof(dwFlags);
	if(!only_this_code)
	{
		mpt::IO::WriteIntLE<int32>(file, fcode);
		mpt::IO::WriteIntLE<int16>(file, fsize);
	}
	dwFlags = SwapBytesReturnLE(dwFlags);
	fwrite(&dwFlags, 1, fsize, file);
}

WRITE_MPTHEADER_sized_member(	nGlobalVol				, uint32		, GV..							)
WRITE_MPTHEADER_sized_member(	nPan					, uint32		, P...							)
WRITE_MPTHEADER_sized_member(	VolEnv.nNodes			, uint32		, VE..							)
WRITE_MPTHEADER_sized_member(	PanEnv.nNodes			, uint32		, PE..							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nNodes			, uint32		, PiE.							)
WRITE_MPTHEADER_sized_member(	VolEnv.nLoopStart		, uint8			, VLS.							)
WRITE_MPTHEADER_sized_member(	VolEnv.nLoopEnd			, uint8			, VLE.							)
WRITE_MPTHEADER_sized_member(	VolEnv.nSustainStart	, uint8			, VSB.							)
WRITE_MPTHEADER_sized_member(	VolEnv.nSustainEnd		, uint8			, VSE.							)
WRITE_MPTHEADER_sized_member(	PanEnv.nLoopStart		, uint8			, PLS.							)
WRITE_MPTHEADER_sized_member(	PanEnv.nLoopEnd			, uint8			, PLE.							)
WRITE_MPTHEADER_sized_member(	PanEnv.nSustainStart	, uint8			, PSB.							)
WRITE_MPTHEADER_sized_member(	PanEnv.nSustainEnd		, uint8			, PSE.							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nLoopStart		, uint8			, PiLS							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nLoopEnd		, uint8			, PiLE							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nSustainStart	, uint8			, PiSB							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nSustainEnd	, uint8			, PiSE							)
WRITE_MPTHEADER_sized_member(	nNNA					, uint8			, NNA.							)
WRITE_MPTHEADER_sized_member(	nDCT					, uint8			, DCT.							)
WRITE_MPTHEADER_sized_member(	nDNA					, uint8			, DNA.							)
WRITE_MPTHEADER_sized_member(	nPanSwing				, uint8			, PS..							)
WRITE_MPTHEADER_sized_member(	nVolSwing				, uint8			, VS..							)
WRITE_MPTHEADER_sized_member(	nIFC					, uint8			, IFC.							)
WRITE_MPTHEADER_sized_member(	nIFR					, uint8			, IFR.							)
WRITE_MPTHEADER_sized_member(	wMidiBank				, uint16		, MB..							)
WRITE_MPTHEADER_sized_member(	nMidiProgram			, uint8			, MP..							)
WRITE_MPTHEADER_sized_member(	nMidiChannel			, uint8			, MC..							)
WRITE_MPTHEADER_sized_member(	nMidiDrumKey			, uint8			, MDK.							)
WRITE_MPTHEADER_sized_member(	nPPS					, int8			, PPS.							)
WRITE_MPTHEADER_sized_member(	nPPC					, uint8			, PPC.							)
WRITE_MPTHEADER_array_member(	VolEnv.Ticks			, uint16		, VP[.		, ((input->VolEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	PanEnv.Ticks			, uint16		, PP[.		, ((input->PanEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	PitchEnv.Ticks			, uint16		, PiP[		, ((input->PitchEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	VolEnv.Values			, uint8			, VE[.		, ((input->VolEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	PanEnv.Values			, uint8			, PE[.		, ((input->PanEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	PitchEnv.Values			, uint8			, PiE[		, ((input->PitchEnv.nNodes > 32) ? MAX_ENVPOINTS : 32))
WRITE_MPTHEADER_array_member(	NoteMap					, uint8			, NM[.		, 128				)
WRITE_MPTHEADER_array_member(	Keyboard				, uint16		, K[..		, 128				)
WRITE_MPTHEADER_array_member(	name					, char			, n[..		, 32				)
WRITE_MPTHEADER_array_member(	filename				, char			, fn[.		, 12				)
WRITE_MPTHEADER_sized_member(	nMixPlug				, uint8			, MiP.							)
WRITE_MPTHEADER_sized_member(	nVolRampUp				, uint16		, VR..							)
WRITE_MPTHEADER_trunc_member(	nResampling				, uint16		, R...							)
WRITE_MPTHEADER_sized_member(	nCutSwing				, uint8			, CS..							)
WRITE_MPTHEADER_sized_member(	nResSwing				, uint8			, RS..							)
WRITE_MPTHEADER_sized_member(	nFilterMode				, uint8			, FM..							)
WRITE_MPTHEADER_sized_member(	nPluginVelocityHandling	, uint8			, PVEH							)
WRITE_MPTHEADER_sized_member(	nPluginVolumeHandling	, uint8			, PVOH							)
WRITE_MPTHEADER_sized_member(	wPitchToTempoLock		, uint16		, PTTL							)
WRITE_MPTHEADER_sized_member(	PitchEnv.nReleaseNode	, uint8			, PERN							)
WRITE_MPTHEADER_sized_member(	PanEnv.nReleaseNode		, uint8			, AERN							)
WRITE_MPTHEADER_sized_member(	VolEnv.nReleaseNode		, uint8			, VERN							)
WRITE_MPTHEADER_sized_member(	PitchEnv.dwFlags		, uint32		, PFLG							)
WRITE_MPTHEADER_sized_member(	PanEnv.dwFlags			, uint32		, AFLG							)
WRITE_MPTHEADER_sized_member(	VolEnv.dwFlags			, uint32		, VFLG							)
WRITE_MPTHEADER_sized_member(	midiPWD					, int8			, MPWD							)
}

#endif // !MODPLUG_NO_FILESAVE


// --------------------------------------------------------------------------------------------
// Convenient macro to help GET_HEADER declaration for single type members ONLY (non-array)
// --------------------------------------------------------------------------------------------
#define GET_MPTHEADER_sized_member(name,type,code) \
	if(fcode == MULTICHAR_STRING_TO_INT(#code)) {\
		if( fsize <= sizeof( type ) ) \
		{ \
			/* hackish workaround to resolve mismatched size values: */ \
			/* nResampling was a long time declared as uint32 but these macro tables used uint16 and UINT. */ \
			/* This worked fine on little-endian, on big-endian not so much. Thus support reading size-mismatched fields. */ \
			type tmp; \
			if(!file.CanRead(fsize)) return false; \
			tmp = file.ReadTruncatedIntLE<type>(fsize); \
			STATIC_ASSERT(sizeof(tmp) == sizeof(input-> name )); \
			memcpy(&(input-> name ), &tmp, sizeof(type)); \
			return true; \
		} \
	} else \
/**/

// --------------------------------------------------------------------------------------------
// Convenient macro to help GET_HEADER declaration for array members ONLY
// --------------------------------------------------------------------------------------------
#define GET_MPTHEADER_array_member(name,type,code,arraysize) \
	if(fcode == MULTICHAR_STRING_TO_INT(#code)) {\
		if( fsize <= sizeof( type ) * arraysize ) \
		{ \
			if(!file.CanRead(sizeof(type) * arraysize)) return false; \
			for(std::size_t i = 0; i < arraysize; ++i) \
			{ \
				input-> name [i] = file.ReadIntLE<type>(); \
			} \
			return true; \
		} \
	} else \
/**/


// Return a pointer on the wanted field in 'input' ModInstrument given field code & size
bool ReadInstrumentHeaderField(ModInstrument *input, uint32 fcode, uint16 fsize, FileReader &file)
{
if(input == nullptr) return false;

GET_MPTHEADER_sized_member(	nFadeOut				, UINT			, FO..							)
GET_MPTHEADER_sized_member(	dwFlags					, uint32		, dF..							)
GET_MPTHEADER_sized_member(	nGlobalVol				, UINT			, GV..							)
GET_MPTHEADER_sized_member(	nPan					, UINT			, P...							)
GET_MPTHEADER_sized_member(	VolEnv.nNodes			, UINT			, VE..							)
GET_MPTHEADER_sized_member(	PanEnv.nNodes			, UINT			, PE..							)
GET_MPTHEADER_sized_member(	PitchEnv.nNodes			, UINT			, PiE.							)
GET_MPTHEADER_sized_member(	VolEnv.nLoopStart		, uint8			, VLS.							)
GET_MPTHEADER_sized_member(	VolEnv.nLoopEnd			, uint8			, VLE.							)
GET_MPTHEADER_sized_member(	VolEnv.nSustainStart	, uint8			, VSB.							)
GET_MPTHEADER_sized_member(	VolEnv.nSustainEnd		, uint8			, VSE.							)
GET_MPTHEADER_sized_member(	PanEnv.nLoopStart		, uint8			, PLS.							)
GET_MPTHEADER_sized_member(	PanEnv.nLoopEnd			, uint8			, PLE.							)
GET_MPTHEADER_sized_member(	PanEnv.nSustainStart	, uint8			, PSB.							)
GET_MPTHEADER_sized_member(	PanEnv.nSustainEnd		, uint8			, PSE.							)
GET_MPTHEADER_sized_member(	PitchEnv.nLoopStart		, uint8			, PiLS							)
GET_MPTHEADER_sized_member(	PitchEnv.nLoopEnd		, uint8			, PiLE							)
GET_MPTHEADER_sized_member(	PitchEnv.nSustainStart	, uint8			, PiSB							)
GET_MPTHEADER_sized_member(	PitchEnv.nSustainEnd	, uint8			, PiSE							)
GET_MPTHEADER_sized_member(	nNNA					, uint8			, NNA.							)
GET_MPTHEADER_sized_member(	nDCT					, uint8			, DCT.							)
GET_MPTHEADER_sized_member(	nDNA					, uint8			, DNA.							)
GET_MPTHEADER_sized_member(	nPanSwing				, uint8			, PS..							)
GET_MPTHEADER_sized_member(	nVolSwing				, uint8			, VS..							)
GET_MPTHEADER_sized_member(	nIFC					, uint8			, IFC.							)
GET_MPTHEADER_sized_member(	nIFR					, uint8			, IFR.							)
GET_MPTHEADER_sized_member(	wMidiBank				, uint16		, MB..							)
GET_MPTHEADER_sized_member(	nMidiProgram			, uint8			, MP..							)
GET_MPTHEADER_sized_member(	nMidiChannel			, uint8			, MC..							)
GET_MPTHEADER_sized_member(	nMidiDrumKey			, uint8			, MDK.							)
GET_MPTHEADER_sized_member(	nPPS					, int8			, PPS.							)
GET_MPTHEADER_sized_member(	nPPC					, uint8			, PPC.							)
GET_MPTHEADER_array_member(	VolEnv.Ticks			, uint16		, VP[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PanEnv.Ticks			, uint16		, PP[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PitchEnv.Ticks			, uint16		, PiP[		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	VolEnv.Values			, uint8			, VE[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PanEnv.Values			, uint8			, PE[.		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	PitchEnv.Values			, uint8			, PiE[		, MAX_ENVPOINTS		)
GET_MPTHEADER_array_member(	NoteMap					, uint8			, NM[.		, 128				)
GET_MPTHEADER_array_member(	Keyboard				, uint16		, K[..		, 128				)
GET_MPTHEADER_array_member(	name					, char			, n[..		, 32				)
GET_MPTHEADER_array_member(	filename				, char			, fn[.		, 12				)
GET_MPTHEADER_sized_member(	nMixPlug				, uint8			, MiP.							)
GET_MPTHEADER_sized_member(	nVolRampUp				, uint16		, VR..							)
GET_MPTHEADER_sized_member(	nResampling				, UINT			, R...							)
GET_MPTHEADER_sized_member(	nCutSwing				, uint8			, CS..							)
GET_MPTHEADER_sized_member(	nResSwing				, uint8			, RS..							)
GET_MPTHEADER_sized_member(	nFilterMode				, uint8			, FM..							)
GET_MPTHEADER_sized_member(	wPitchToTempoLock		, uint16		, PTTL							)
GET_MPTHEADER_sized_member(	nPluginVelocityHandling	, uint8			, PVEH							)
GET_MPTHEADER_sized_member(	nPluginVolumeHandling	, uint8			, PVOH							)
GET_MPTHEADER_sized_member(	PitchEnv.nReleaseNode	, uint8			, PERN							)
GET_MPTHEADER_sized_member(	PanEnv.nReleaseNode		, uint8			, AERN							)
GET_MPTHEADER_sized_member(	VolEnv.nReleaseNode		, uint8			, VERN							)
GET_MPTHEADER_sized_member(	PitchEnv.dwFlags		, uint32		, PFLG							)
GET_MPTHEADER_sized_member(	PanEnv.dwFlags			, uint32		, AFLG							)
GET_MPTHEADER_sized_member(	VolEnv.dwFlags			, uint32		, VFLG							)
GET_MPTHEADER_sized_member(	midiPWD					, int8			, MPWD							)
{}

return false;

}

// -! NEW_FEATURE#0027


mpt::ustring FileHistory::AsISO8601() const
//-----------------------------------------
{
	tm date = loadDate;
	if(openTime > 0)
	{
		// Calculate the date when editing finished.
		double openSeconds = (double)openTime / (double)HISTORY_TIMER_PRECISION;
		tm tmpLoadDate = loadDate;
		time_t loadDateSinceEpoch = mpt::Date::Unix::FromUTC(&tmpLoadDate);
		double timeScaleFactor = difftime(2, 1);
		time_t saveDateSinceEpoch = loadDateSinceEpoch + Util::Round<time_t>(openSeconds / timeScaleFactor);
		const tm * tmpSaveDate = gmtime(&saveDateSinceEpoch);
		if(tmpSaveDate)
		{
			date = *tmpSaveDate;
		}
	}
	// We assume date in UTC here.
	// This is not 100% correct because FileHistory does not contain complete timezone information.
	return mpt::Date::ToShortenedISO8601(date);
}


//////////////////////////////////////////////////////////
// CSoundFile

#ifdef MODPLUG_TRACKER
CTuningCollection* CSoundFile::s_pTuningsSharedBuiltIn(0);
CTuningCollection* CSoundFile::s_pTuningsSharedLocal(0);
#endif

#if MPT_COMPILER_MSVC
#pragma warning(disable : 4355) // "'this' : used in base member initializer list"
#endif
CSoundFile::CSoundFile() :
	m_pTuningsTuneSpecific(nullptr),
	m_pModSpecs(&ModSpecs::itEx),
	m_nType(MOD_TYPE_NONE),
	Patterns(*this),
	Order(*this),
#ifdef MODPLUG_TRACKER
	m_MIDIMapper(*this),
#endif
	visitedSongRows(*this),
	m_pCustomLog(nullptr)
#if MPT_COMPILER_MSVC
#pragma warning(default : 4355) // "'this' : used in base member initializer list"
#endif
//----------------------
{
	MemsetZero(MixSoundBuffer);
	MemsetZero(MixRearBuffer);
	MemsetZero(MixFloatBuffer);
	gnDryLOfsVol = 0;
	gnDryROfsVol = 0;
	m_nType = MOD_TYPE_NONE;
	m_ContainerType = MOD_CONTAINERTYPE_NONE;
	m_nChannels = 0;
	m_nMixChannels = 0;
	m_nSamples = 0;
	m_nInstruments = 0;
#ifndef MODPLUG_TRACKER
	m_nFreqFactor = m_nTempoFactor = 65536;
#endif
	m_nMinPeriod = 32;
	m_nMaxPeriod = 0x7FFF;
	m_nRepeatCount = 0;
	m_PlayState.m_nSeqOverride = ORDERINDEX_INVALID;
	m_PlayState.m_bPatternTransitionOccurred = false;
	m_nTempoMode = tempo_mode_classic;
	m_bIsRendering = false;

#ifdef MODPLUG_TRACKER
	m_lockOrderStart = m_lockOrderEnd = ORDERINDEX_INVALID;
	m_pModDoc = nullptr;
	m_bChannelMuteTogglePending.reset();

	m_nDefaultRowsPerBeat = m_PlayState.m_nCurrentRowsPerBeat = (TrackerSettings::Instance().m_nRowHighlightBeats) ? TrackerSettings::Instance().m_nRowHighlightBeats : 4;
	m_nDefaultRowsPerMeasure = m_PlayState.m_nCurrentRowsPerMeasure = (TrackerSettings::Instance().m_nRowHighlightMeasures >= m_nDefaultRowsPerBeat) ? TrackerSettings::Instance().m_nRowHighlightMeasures : m_nDefaultRowsPerBeat * 4;
#else
	m_nDefaultRowsPerBeat = m_PlayState.m_nCurrentRowsPerBeat = 4;
	m_nDefaultRowsPerMeasure = m_PlayState.m_nCurrentRowsPerMeasure = 16;
#endif // MODPLUG_TRACKER

	m_dwLastSavedWithVersion=0;
	m_dwCreatedWithVersion=0;

	MemsetZero(m_PlayState.ChnMix);
	MemsetZero(Instruments);
	MemsetZero(m_szNames);
	MemsetZero(m_MixPlugins);
	m_PlayState.m_lTotalSampleCount = 0;
	m_PlayState.m_bPositionChanged = true;

#ifndef MODPLUG_TRACKER
	m_pTuningsBuiltIn = new CTuningCollection();
	LoadBuiltInTunings();
#endif
	m_pTuningsTuneSpecific = new CTuningCollection("Tune specific tunings");
}


CSoundFile::~CSoundFile()
//-----------------------
{
	Destroy();
	delete m_pTuningsTuneSpecific;
	m_pTuningsTuneSpecific = nullptr;
#ifndef MODPLUG_TRACKER
	delete m_pTuningsBuiltIn;
	m_pTuningsBuiltIn = nullptr;
#endif
}


void CSoundFile::AddToLog(LogLevel level, const mpt::ustring &text) const
//-----------------------------------------------------------------------
{
/*	if(m_pCustomLog)
	{
		m_pCustomLog->AddToLog(level, text);
	} else
	{
		#ifdef MODPLUG_TRACKER
			if(GetpModDoc()) GetpModDoc()->AddToLog(level, text);
		#else
			Log(level, text);
		#endif
	}*/
}


// Global variable initializer for loader functions
void CSoundFile::InitializeGlobals()
//----------------------------------
{
	// Do not add or change any of these values! And if you do, review each and every loader to check if they require these defaults!
	m_nType = MOD_TYPE_NONE;
	m_ContainerType = MOD_CONTAINERTYPE_NONE;
	m_nChannels = 0;
	m_nInstruments = 0;
	m_nSamples = 0;
	m_nSamplePreAmp = 48;
	m_nVSTiVolume = 48;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 125;
	m_nDefaultGlobalVolume = MAX_GLOBAL_VOLUME;
	m_nRestartPos = 0;
	m_SongFlags.reset();
	m_nMinPeriod = 16;
	m_nMaxPeriod = 32767;
	m_dwLastSavedWithVersion = m_dwCreatedWithVersion = 0;

	SetMixLevels(mixLevels_compatible);
	SetModFlags(FlagSet<ModSpecificFlag>());

	Patterns.ClearPatterns();

	songName.clear();
	songArtist.clear();
	songMessage.clear();
	madeWithTracker.clear();
	m_FileHistory.clear();
}


void CSoundFile::InitializeChannels()
//-----------------------------------
{
	for(CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS; nChn++)
	{
		InitChannel(nChn);
	}
}


#ifdef MODPLUG_TRACKER
bool CSoundFile::Create(FileReader file, ModLoadingFlags loadFlags, CModDoc *pModDoc)
//-----------------------------------------------------------------------------------
{
	m_pModDoc = pModDoc;
#else
bool CSoundFile::Create(FileReader file, ModLoadingFlags loadFlags)
//-----------------------------------------------------------------
{
#endif // MODPLUG_TRACKER

	m_nMixChannels = 0;
#ifndef MODPLUG_TRACKER
	m_nFreqFactor = m_nTempoFactor = 65536;
#endif
	m_PlayState.m_nGlobalVolume = MAX_GLOBAL_VOLUME;

	InitializeGlobals();
	Order.resize(1);

	// Playback
	m_PlayState.m_nPatternDelay = 0;
	m_PlayState.m_nFrameDelay = 0;
	m_PlayState.m_nNextRow = 0;
	m_PlayState.m_nRow = 0;
	m_PlayState.m_nPattern = 0;
	m_PlayState.m_nCurrentOrder = 0;
	m_PlayState.m_nNextOrder = 0;
	m_PlayState.m_nNextPatStartRow = 0;
	m_PlayState.m_nSeqOverride = ORDERINDEX_INVALID;

	m_nMaxOrderPosition = 0;
	MemsetZero(m_PlayState.ChnMix);
	MemsetZero(Instruments);
	MemsetZero(m_szNames);
	MemsetZero(m_MixPlugins);

	if(file.IsValid())
	{
#ifndef NO_ARCHIVE_SUPPORT
		CUnarchiver unarchiver(file);
		if(unarchiver.ExtractBestFile(GetSupportedExtensions(true)))
		{
			file = unarchiver.GetOutputFile();
		}
#endif

		MODCONTAINERTYPE packedContainerType = MOD_CONTAINERTYPE_NONE;
		std::vector<char> unpackedData;
		if(packedContainerType == MOD_CONTAINERTYPE_NONE && UnpackXPK(unpackedData, file)) packedContainerType = MOD_CONTAINERTYPE_XPK;
		if(packedContainerType == MOD_CONTAINERTYPE_NONE && UnpackPP20(unpackedData, file)) packedContainerType = MOD_CONTAINERTYPE_PP20;
		if(packedContainerType == MOD_CONTAINERTYPE_NONE && UnpackMMCMP(unpackedData, file)) packedContainerType = MOD_CONTAINERTYPE_MMCMP;
		if(packedContainerType != MOD_CONTAINERTYPE_NONE)
		{
			file = FileReader(&(unpackedData[0]), unpackedData.size());
		}

		file.Rewind();
		const uint8 *lpStream = reinterpret_cast<const unsigned char*>(file.GetRawData());
		DWORD dwMemLength = file.GetLength();

		if(!ReadXM(file, loadFlags)
		 && !ReadIT(file, loadFlags)
		 && !ReadS3M(file, loadFlags)
		 && !ReadITProject(file, loadFlags)
#ifdef MODPLUG_TRACKER
		// this makes little sense for a module player library
		 && !ReadWav(file, loadFlags)
#endif // MODPLUG_TRACKER
		 && !ReadSTM(file, loadFlags)
		 && !ReadMed(lpStream, dwMemLength, loadFlags)
		 && !ReadMTM(file, loadFlags)
		 && !ReadMDL(lpStream, dwMemLength, loadFlags)
		 && !ReadDBM(file, loadFlags)
		 && !Read669(file, loadFlags)
		 && !ReadFAR(file, loadFlags)
		 && !ReadAMS(file, loadFlags)
		 && !ReadAMS2(file, loadFlags)
		 && !ReadOKT(file, loadFlags)
		 && !ReadPTM(file, loadFlags)
		 && !ReadUlt(file, loadFlags)
		 && !ReadDMF(file, loadFlags)
		 && !ReadDSM(file, loadFlags)
		 && !ReadUMX(file, loadFlags)
		 && !ReadAMF_Asylum(file, loadFlags)
		 && !ReadAMF_DSMI(file, loadFlags)
		 && !ReadPSM(file, loadFlags)
		 && !ReadPSM16(file, loadFlags)
		 && !ReadMT2(file, loadFlags)
#ifdef MODPLUG_TRACKER
		 && !ReadMID(lpStream, dwMemLength, loadFlags)
#endif // MODPLUG_TRACKER
		 && !ReadGDM(file, loadFlags)
		 && !ReadIMF(file, loadFlags)
		 && !ReadDIGI(file, loadFlags)
		 && !ReadPLM(file, loadFlags)
		 && !ReadAM(file, loadFlags)
		 && !ReadJ2B(file, loadFlags)
		 && !ReadMO3(file, loadFlags)
		 && !ReadMod(file, loadFlags)
		 && !ReadICE(file, loadFlags)
		 && !ReadM15(file, loadFlags))
		{
			m_nType = MOD_TYPE_NONE;
			m_ContainerType = MOD_CONTAINERTYPE_NONE;
		}

		if(packedContainerType != MOD_CONTAINERTYPE_NONE && m_ContainerType == MOD_CONTAINERTYPE_NONE)
		{
			m_ContainerType = packedContainerType;
		}

		if(madeWithTracker.empty())
		{
			madeWithTracker = ModTypeToTracker(GetType());
		}

#ifndef NO_ARCHIVE_SUPPORT
		// Read archive comment if there is no song comment
		if(songMessage.empty())
		{
			songMessage.assign(mpt::ToLocale(unarchiver.GetComment()));
		}
#endif

	} else
	{
		// New song
		m_dwCreatedWithVersion = MptVersion::num;
	}

	// Adjust channels
	for(CHANNELINDEX ich = 0; ich < MAX_BASECHANNELS; ich++)
	{
		LimitMax(ChnSettings[ich].nVolume, uint16(64));
		if (ChnSettings[ich].nPan > 256) ChnSettings[ich].nPan = 128;
		m_PlayState.Chn[ich].Reset(ModChannel::resetTotal, *this, ich);
	}

	// Checking samples, load external samples
	for(SAMPLEINDEX nSmp = 1; nSmp <= m_nSamples; nSmp++)
	{
		// Adjust song / sample names
		mpt::String::SetNullTerminator(m_szNames[nSmp]);
		ModSample &sample = Samples[nSmp];

#ifdef MPT_EXTERNAL_SAMPLES
		if(SampleHasPath(nSmp))
		{
			mpt::PathString filename = GetSamplePath(nSmp);
			if(!file.GetFileName().empty())
			{
				filename = filename.RelativePathToAbsolute(file.GetFileName().GetPath());
			} else if(GetpModDoc() != nullptr)
			{
				filename = filename.RelativePathToAbsolute(GetpModDoc()->GetPathNameMpt().GetPath());
			}
			if(!LoadExternalSample(nSmp, filename))
			{
#ifndef MODPLUG_TRACKER
				// OpenMPT has its own way of reporting this error in CModDoc.
				AddToLog(LogError, mpt::String::Print(MPT_USTRING("Unable to load sample %1: %2"), i, filename.ToUnicode()));
#endif // MODPLUG_TRACKER
			}
		} else
		{
			sample.uFlags.reset(SMP_KEEPONDISK);
		}
#endif // MPT_EXTERNAL_SAMPLES

		if(sample.pSample)
		{
			sample.PrecomputeLoops(*this, false);
		} else if(!sample.uFlags[SMP_KEEPONDISK])
		{
			sample.nLength = 0;
			sample.nLoopStart = 0;
			sample.nLoopEnd = 0;
			sample.nSustainStart = 0;
			sample.nSustainEnd = 0;
			sample.uFlags.reset(CHN_LOOP | CHN_PINGPONGLOOP | CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN);
		}
		if(sample.nGlobalVol > 64) sample.nGlobalVol = 64;
	}
	// Check invalid instruments
	while ((m_nInstruments > 0) && (!Instruments[m_nInstruments])) m_nInstruments--;
	// Set default values
	if (!m_nDefaultTempo) m_nDefaultTempo = 125;
	if (!m_nDefaultSpeed) m_nDefaultSpeed = 6;
	m_PlayState.m_nMusicSpeed = m_nDefaultSpeed;
	m_PlayState.m_nMusicTempo = m_nDefaultTempo;
	m_PlayState.m_nGlobalVolume = static_cast<int32>(m_nDefaultGlobalVolume);
	m_PlayState.m_lHighResRampingGlobalVolume = m_PlayState.m_nGlobalVolume<<VOLUMERAMPPRECISION;
	m_PlayState.m_nGlobalVolumeDestination = m_PlayState.m_nGlobalVolume;
	m_PlayState.m_nSamplesToGlobalVolRampDest = 0;
	m_PlayState.m_nGlobalVolumeRampAmount = 0;
	m_PlayState.m_nNextOrder = 0;
	m_PlayState.m_nCurrentOrder = 0;
	m_PlayState.m_nPattern = 0;
	m_PlayState.m_nBufferCount = 0;
	m_PlayState.m_dBufferDiff = 0;
	m_PlayState.m_nTickCount = m_PlayState.m_nMusicSpeed;
	m_PlayState.m_nNextRow = 0;
	m_PlayState.m_nRow = 0;

	RecalculateSamplesPerTick();
	visitedSongRows.Initialize(true);

	if ((m_nRestartPos >= Order.size()) || (Order[m_nRestartPos] >= Patterns.Size())) m_nRestartPos = 0;

#ifndef NO_VST
	// plugin loader
	std::string notFoundText;
	std::vector<SNDMIXPLUGININFO *> notFoundIDs;

	if (gpMixPluginCreateProc && (loadFlags & loadPluginData))
	{
		for(PLUGINDEX plug = 0; plug < MAX_MIXPLUGINS; plug++)
		{
			if(m_MixPlugins[plug].IsValidPlugin())
			{
				gpMixPluginCreateProc(m_MixPlugins[plug], *this);
				if (m_MixPlugins[plug].pMixPlugin)
				{
					// plugin has been found
					m_MixPlugins[plug].pMixPlugin->RestoreAllParameters(m_MixPlugins[plug].defaultProgram);
				} else
				{
					// plugin not found - add to list
					bool found = false;
					for(std::vector<SNDMIXPLUGININFO *>::const_iterator i = notFoundIDs.begin(); i != notFoundIDs.end(); ++i)
					{
						if((**i).dwPluginId2 == (**i).dwPluginId2
							&& (**i).dwPluginId1 == (**i).dwPluginId1)
						{
							found = true;
							break;
						}
					}

					if(!found)
					{
						notFoundText.append(m_MixPlugins[plug].GetLibraryName());
						notFoundText.append("\n");
						notFoundIDs.push_back(&m_MixPlugins[plug].Info); // add this to the list of missing IDs so we will find the needed plugins later when calling KVRAudio
					}
				}
			}
		}
	}

	// Display a nice message so the user sees which plugins are missing
	// TODO: Use IDD_MODLOADING_WARNINGS dialog (NON-MODAL!) to display all warnings that are encountered when loading a module.
	if(!notFoundIDs.empty())
	{
		if(notFoundIDs.size() == 1)
		{
			notFoundText = "The following plugin has not been found:\n\n" + notFoundText + "\nDo you want to search for it online?";
		} else
		{
			notFoundText = "The following plugins have not been found:\n\n" + notFoundText + "\nDo you want to search for them online?";
		}
		if (Reporting::Confirm(mpt::ToWide(mpt::CharsetUTF8, notFoundText.c_str()), L"OpenMPT - Plugins missing", false, true) == cnfYes)
		{
			std::string url = "http://resources.openmpt.org/plugins/search.php?p=";
			for(std::vector<SNDMIXPLUGININFO *>::const_iterator i = notFoundIDs.begin(); i != notFoundIDs.end(); ++i)
			{
				url += mpt::fmt::HEX0<8>(LittleEndian((**i).dwPluginId2));
				url += (**i).szLibraryName;
				url += "%0a";
			}
			CTrackApp::OpenURL(mpt::PathString::FromUTF8(url));
		}
	}
#endif // NO_VST

	// Set up mix levels
	SetMixLevels(m_nMixLevels);

	if(GetType() == MOD_TYPE_NONE)
	{
		return false;
	}

	SetModSpecsPointer(m_pModSpecs, GetBestSaveFormat());
	const ORDERINDEX CacheSize = ModSequenceSet::s_nCacheSize; // workaround reference to static const member problem
	const ORDERINDEX nMinLength = std::min(CacheSize, GetModSpecifications().ordersMax);
	if (Order.GetLength() < nMinLength)
		Order.resize(nMinLength);

	// When reading a file made with an older version of MPT, it might be necessary to upgrade some settings automatically.
	if(m_dwLastSavedWithVersion)
	{
		UpgradeModule();
	}
	return true;
}


bool CSoundFile::Destroy()
//------------------------
{
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		m_PlayState.Chn[i].pModInstrument = nullptr;
		m_PlayState.Chn[i].pModSample = nullptr;
		m_PlayState.Chn[i].pCurrentSample = nullptr;
		m_PlayState.Chn[i].nLength = 0;
	}

	Patterns.DestroyPatterns();

	songName.clear();
	songArtist.clear();
	songMessage.clear();
	madeWithTracker.clear();
	m_FileHistory.clear();

	for(SAMPLEINDEX i = 1; i < MAX_SAMPLES; i++)
	{
		Samples[i].FreeSample();
	}
	for(INSTRUMENTINDEX i = 0; i < MAX_INSTRUMENTS; i++)
	{
		delete Instruments[i];
		Instruments[i] = nullptr;
	}
	for(PLUGINDEX i = 0; i < MAX_MIXPLUGINS; i++)
	{
		m_MixPlugins[i].Destroy();
	}

	m_nType = MOD_TYPE_NONE;
	m_ContainerType = MOD_CONTAINERTYPE_NONE;
	m_nChannels = m_nSamples = m_nInstruments = 0;
	return true;
}


//////////////////////////////////////////////////////////////////////////
// Misc functions


void CSoundFile::SetDspEffects(DWORD DSPMask)
//-------------------------------------------
{
#ifdef ENABLE_ASM
#ifndef NO_REVERB
	if(!(GetProcSupport() & PROCSUPPORT_MMX)) DSPMask &= ~SNDDSP_REVERB;
#endif
#endif
	m_MixerSettings.DSPMask = DSPMask;
	InitPlayer(false);
}


void CSoundFile::SetPreAmp(UINT nVol)
//-----------------------------------
{
	if (nVol < 1) nVol = 1;
	if (nVol > 0x200) nVol = 0x200;	// x4 maximum
#ifndef NO_AGC
	if ((nVol < m_MixerSettings.m_nPreAmp) && (nVol) && (m_MixerSettings.DSPMask & SNDDSP_AGC))
	{
		m_AGC.Adjust(m_MixerSettings.m_nPreAmp, nVol);
	}
#endif
	m_MixerSettings.m_nPreAmp = nVol;
}


double CSoundFile::GetCurrentBPM() const
//--------------------------------------
{
	double bpm;

	if (m_nTempoMode == tempo_mode_modern)
	{
		// With modern mode, we trust that true bpm is close enough to what user chose.
		bpm = static_cast<double>(m_PlayState.m_nMusicTempo);	// This avoids oscillation due to tick-to-tick corrections.
	} else
	{
		//with other modes, we calculate it:
		double ticksPerBeat = m_PlayState.m_nMusicSpeed * m_PlayState.m_nCurrentRowsPerBeat;	//ticks/beat = ticks/row * rows/beat
		double samplesPerBeat = m_PlayState.m_nSamplesPerTick * ticksPerBeat;		//samps/beat = samps/tick * ticks/beat
		bpm =  m_MixerSettings.gdwMixingFreq/samplesPerBeat * 60;		//beats/sec  = samps/sec  / samps/beat
	}																	//beats/min  =  beats/sec * 60

	return bpm;
}


void CSoundFile::ResetPlayPos()
//-----------------------------
{
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
		m_PlayState.Chn[i].Reset(ModChannel::resetSetPosFull, *this, i);

	InitializeVisitedRows();
	m_SongFlags.reset(SONG_FADINGSONG | SONG_ENDREACHED);

	m_PlayState.m_nGlobalVolume = m_nDefaultGlobalVolume;
	m_PlayState.m_nMusicSpeed = m_nDefaultSpeed;
	m_PlayState.m_nMusicTempo = m_nDefaultTempo;

	// do not ramp global volume when starting playback
	m_PlayState.m_lHighResRampingGlobalVolume = m_PlayState.m_nGlobalVolume<<VOLUMERAMPPRECISION;
	m_PlayState.m_nGlobalVolumeDestination = m_PlayState.m_nGlobalVolume;
	m_PlayState.m_nSamplesToGlobalVolRampDest = 0;
	m_PlayState.m_nGlobalVolumeRampAmount = 0;

	m_PlayState.m_nNextOrder = 0;
	m_PlayState.m_nNextRow = 0;
	m_PlayState.m_nTickCount = m_PlayState.m_nMusicSpeed;
	m_PlayState.m_nBufferCount = 0;
	m_PlayState.m_nPatternDelay = 0;
	m_PlayState.m_nFrameDelay = 0;
	m_PlayState.m_nNextPatStartRow = 0;
	//m_nSeqOverride = 0;
}



void CSoundFile::SetCurrentOrder(ORDERINDEX nOrder)
//-------------------------------------------------
{
	while ((nOrder < Order.size()) && (Order[nOrder] == Order.GetIgnoreIndex())) nOrder++;
	if ((nOrder >= Order.size()) || (Order[nOrder] >= Patterns.Size())) return;
	for (CHANNELINDEX j = 0; j < MAX_CHANNELS; j++)
	{
		m_PlayState.Chn[j].nPeriod = 0;
		m_PlayState.Chn[j].nNote = NOTE_NONE;
		m_PlayState.Chn[j].nPortamentoDest = 0;
		m_PlayState.Chn[j].nCommand = 0;
		m_PlayState.Chn[j].nPatternLoopCount = 0;
		m_PlayState.Chn[j].nPatternLoop = 0;
		m_PlayState.Chn[j].nVibratoPos = m_PlayState.Chn[j].nTremoloPos = m_PlayState.Chn[j].nPanbrelloPos = 0;
		//IT compatibility 15. Retrigger
		if(IsCompatibleMode(TRK_IMPULSETRACKER))
		{
			m_PlayState.Chn[j].nRetrigCount = 0;
			m_PlayState.Chn[j].nRetrigParam = 1;
		}
		m_PlayState.Chn[j].nTremorCount = 0;
	}

#ifndef NO_VST
	// Stop hanging notes from VST instruments as well
	StopAllVsti();
#endif // NO_VST

	if (!nOrder)
	{
		ResetPlayPos();
	} else
	{
		m_PlayState.m_nNextOrder = nOrder;
		m_PlayState.m_nRow = m_PlayState.m_nNextRow = 0;
		m_PlayState.m_nPattern = 0;
		m_PlayState.m_nTickCount = m_PlayState.m_nMusicSpeed;
		m_PlayState.m_nBufferCount = 0;
		m_PlayState.m_nPatternDelay = 0;
		m_PlayState.m_nFrameDelay = 0;
		m_PlayState.m_nNextPatStartRow = 0;
	}

	m_SongFlags.reset(SONG_FADINGSONG | SONG_ENDREACHED);
}

//rewbs.VSTCompliance
void CSoundFile::SuspendPlugins()
//-------------------------------
{
	for (PLUGINDEX iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
	{
		if (!m_MixPlugins[iPlug].pMixPlugin)
			continue;  //most common branch

		IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
		if (m_MixPlugins[iPlug].pMixState && pPlugin->IsResumed())
		{
			pPlugin->NotifySongPlaying(false);
			pPlugin->HardAllNotesOff();
			pPlugin->Suspend();
		}
	}
}

void CSoundFile::ResumePlugins()
//------------------------------
{
	for (PLUGINDEX iPlug=0; iPlug<MAX_MIXPLUGINS; iPlug++)
	{
		if (!m_MixPlugins[iPlug].pMixPlugin)
			continue;  //most common branch

		IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
		if (m_MixPlugins[iPlug].pMixState && !pPlugin->IsResumed())
		{
			pPlugin->NotifySongPlaying(true);
			pPlugin->Resume();
		}
	}
}


void CSoundFile::StopAllVsti()
//----------------------------
{
	for (PLUGINDEX iPlug = 0; iPlug < MAX_MIXPLUGINS; iPlug++)
	{
		if (!m_MixPlugins[iPlug].pMixPlugin)
			continue;  //most common branch

		IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
		if (m_MixPlugins[iPlug].pMixState && pPlugin->IsResumed())
		{
			pPlugin->HardAllNotesOff();
		}
	}
}


void CSoundFile::SetMixLevels(mixLevels levels)
//---------------------------------------------
{
	m_nMixLevels = levels;
	m_PlayConfig.SetMixLevels(m_nMixLevels);
	RecalculateGainForAllPlugs();
}


void CSoundFile::RecalculateGainForAllPlugs()
//-------------------------------------------
{
	for (PLUGINDEX iPlug = 0; iPlug < MAX_MIXPLUGINS; iPlug++)
	{
		if (!m_MixPlugins[iPlug].pMixPlugin)
			continue;  //most common branch

		IMixPlugin *pPlugin = m_MixPlugins[iPlug].pMixPlugin;
		if (m_MixPlugins[iPlug].pMixState)
		{
			pPlugin->RecalculateGain();
		}
	}
}


//end rewbs.VSTCompliance

void CSoundFile::ResetChannels()
//------------------------------
{
	m_SongFlags.reset(SONG_FADINGSONG | SONG_ENDREACHED);
	m_PlayState.m_nBufferCount = 0;
	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		m_PlayState.Chn[i].nROfs = m_PlayState.Chn[i].nLOfs = 0;
		m_PlayState.Chn[i].nLength = 0;
	}
}


#ifdef MODPLUG_TRACKER

void CSoundFile::PatternTranstionChnSolo(const CHANNELINDEX chnIndex)
//-------------------------------------------------------------------
{
	if(chnIndex >= m_nChannels)
		return;

	for(CHANNELINDEX i = 0; i<m_nChannels; i++)
	{
		m_bChannelMuteTogglePending[i] = !ChnSettings[i].dwFlags[CHN_MUTE];
	}
	m_bChannelMuteTogglePending[chnIndex] = ChnSettings[chnIndex].dwFlags[CHN_MUTE];
}


void CSoundFile::PatternTransitionChnUnmuteAll()
//----------------------------------------------
{
	for(CHANNELINDEX i = 0; i<m_nChannels; i++)
	{
		m_bChannelMuteTogglePending[i] = ChnSettings[i].dwFlags[CHN_MUTE];
	}
}

#endif // MODPLUG_TRACKER


void CSoundFile::LoopPattern(PATTERNINDEX nPat, ROWINDEX nRow)
//------------------------------------------------------------
{
	if(!Patterns.IsValidPat(nPat))
	{
		m_SongFlags.reset(SONG_PATTERNLOOP);
	} else
	{
		if(nRow >= Patterns[nPat].GetNumRows()) nRow = 0;
		m_PlayState.m_nPattern = nPat;
		m_PlayState.m_nRow = m_PlayState.m_nNextRow = nRow;
		m_PlayState.m_nTickCount = m_PlayState.m_nMusicSpeed;
		m_PlayState.m_nPatternDelay = 0;
		m_PlayState.m_nFrameDelay = 0;
		m_PlayState.m_nBufferCount = 0;
		m_PlayState.m_nNextPatStartRow = 0;
		m_SongFlags.set(SONG_PATTERNLOOP);
	}
}


//rewbs.playSongFromCursor
void CSoundFile::DontLoopPattern(PATTERNINDEX nPat, ROWINDEX nRow)
//----------------------------------------------------------------
{
	if(!Patterns.IsValidPat(nPat)) nPat = 0;
	if(nRow >= Patterns[nPat].GetNumRows()) nRow = 0;
	m_PlayState.m_nPattern = nPat;
	m_PlayState.m_nRow = m_PlayState.m_nNextRow = nRow;
	m_PlayState.m_nTickCount = m_PlayState.m_nMusicSpeed;
	m_PlayState.m_nPatternDelay = 0;
	m_PlayState.m_nFrameDelay = 0;
	m_PlayState.m_nBufferCount = 0;
	m_PlayState.m_nNextPatStartRow = 0;
	m_SongFlags.reset(SONG_PATTERNLOOP);
}
//end rewbs.playSongFromCursor


MODTYPE CSoundFile::GetBestSaveFormat() const
//-------------------------------------------
{
	switch(GetType())
	{
	case MOD_TYPE_MOD:
	case MOD_TYPE_S3M:
	case MOD_TYPE_XM:
	case MOD_TYPE_IT:
	case MOD_TYPE_MPT:
		return GetType();
	case MOD_TYPE_AMF0:
	case MOD_TYPE_DIGI:
		return MOD_TYPE_MOD;
	case MOD_TYPE_MED:
		if(m_nDefaultTempo == 125 && m_nDefaultSpeed == 6 && !m_nInstruments)
		{
			for(PATTERNINDEX i = 0; i < Patterns.Size(); i++)
			{
				if(Patterns.IsValidPat(i) && Patterns[i].GetNumRows() != 64)
					return MOD_TYPE_XM;
			}
			return MOD_TYPE_MOD;
		}
		return MOD_TYPE_XM;
	case MOD_TYPE_669:
	case MOD_TYPE_FAR:
	case MOD_TYPE_STM:
	case MOD_TYPE_DSM:
	case MOD_TYPE_AMF:
	case MOD_TYPE_MTM:
		return MOD_TYPE_S3M;
	case MOD_TYPE_AMS:
	case MOD_TYPE_AMS2:
	case MOD_TYPE_DMF:
	case MOD_TYPE_DBM:
	case MOD_TYPE_IMF:
	case MOD_TYPE_PSM:
	case MOD_TYPE_J2B:
	case MOD_TYPE_ULT:
	case MOD_TYPE_OKT:
	case MOD_TYPE_MT2:
	case MOD_TYPE_MDL:
	case MOD_TYPE_PTM:
	default:
		return MOD_TYPE_IT;
	}
}


const char *CSoundFile::GetSampleName(SAMPLEINDEX nSample) const
//--------------------------------------------------------------
{
	MPT_ASSERT(nSample <= GetNumSamples());
	if (nSample < MAX_SAMPLES)
	{
		return m_szNames[nSample];
	} else
	{
		return "";
	}
}


const char *CSoundFile::GetInstrumentName(INSTRUMENTINDEX nInstr) const
//---------------------------------------------------------------------
{
	if((nInstr >= MAX_INSTRUMENTS) || (!Instruments[nInstr]))
		return "";

	MPT_ASSERT(nInstr <= GetNumInstruments());
	return Instruments[nInstr]->name;
}


bool CSoundFile::InitChannel(CHANNELINDEX nChn)
//---------------------------------------------
{
	if(nChn >= MAX_BASECHANNELS) return true;

	ChnSettings[nChn].Reset();
	m_PlayState.Chn[nChn].Reset(ModChannel::resetTotal, *this, nChn);

#ifdef MODPLUG_TRACKER
	if(GetpModDoc() != nullptr)
	{
		GetpModDoc()->Record1Channel(nChn, false);
		GetpModDoc()->Record2Channel(nChn, false);
	}
#endif // MODPLUG_TRACKER

#ifdef MODPLUG_TRACKER
	m_bChannelMuteTogglePending[nChn] = false;
#endif // MODPLUG_TRACKER

	return false;
}


// Check whether a sample is used.
// In sample mode, the sample numbers in all patterns are checked.
// In instrument mode, it is only checked if a sample is referenced by an instrument (but not if the sample is actually played anywhere)
bool CSoundFile::IsSampleUsed(SAMPLEINDEX nSample) const
//------------------------------------------------------
{
	if ((!nSample) || (nSample > GetNumSamples())) return false;
	if (GetNumInstruments())
	{
		for (INSTRUMENTINDEX i = 1; i <= GetNumInstruments(); i++)
		{
			if(IsSampleReferencedByInstrument(nSample, i))
			{
				return true;
			}
		}
	} else
	{
		for (PATTERNINDEX i = 0; i < Patterns.Size(); i++) if (Patterns.IsValidPat(i))
		{
			CPattern::const_iterator mEnd = Patterns[i].End();
			for(CPattern::const_iterator m = Patterns[i].Begin(); m != mEnd; m++)
			{
				if (m->instr == nSample && !m->IsPcNote()) return true;
			}
		}
	}
	return false;
}


bool CSoundFile::IsInstrumentUsed(INSTRUMENTINDEX nInstr) const
//-------------------------------------------------------------
{
	if ((!nInstr) || (nInstr > GetNumInstruments()) || (!Instruments[nInstr])) return false;
	for (PATTERNINDEX i = 0; i < Patterns.Size(); i++) if (Patterns.IsValidPat(i))
	{
		CPattern::const_iterator mEnd = Patterns[i].End();
		for(CPattern::const_iterator m = Patterns[i].Begin(); m != mEnd; m++)
		{
			if (m->instr == nInstr && !m->IsPcNote()) return true;
		}
	}
	return false;
}


// Detect samples that are referenced by an instrument, but actually not used in a song.
// Only works in instrument mode. Unused samples are marked as false in the vector.
SAMPLEINDEX CSoundFile::DetectUnusedSamples(std::vector<bool> &sampleUsed) const
//------------------------------------------------------------------------------
{
	sampleUsed.assign(GetNumSamples() + 1, false);

	if(GetNumInstruments() == 0)
	{
		return 0;
	}
	SAMPLEINDEX nExt = 0;

	for (PATTERNINDEX nPat = 0; nPat < Patterns.Size(); nPat++) if(Patterns.IsValidPat(nPat))
	{
		CPattern::const_iterator pEnd = Patterns[nPat].End();
		for(CPattern::const_iterator p = Patterns[nPat].Begin(); p != pEnd; p++)
		{
			if(p->IsNote())
			{
				if ((p->instr) && (IsInRange(p->instr, (INSTRUMENTINDEX)0, MAX_INSTRUMENTS)))
				{
					ModInstrument *pIns = Instruments[p->instr];
					if (pIns)
					{
						SAMPLEINDEX n = pIns->Keyboard[p->note-1];
						if (n <= GetNumSamples()) sampleUsed[n] = true;
					}
				} else
				{
					for (INSTRUMENTINDEX k = GetNumInstruments(); k >= 1; k--)
					{
						ModInstrument *pIns = Instruments[k];
						if (pIns)
						{
							SAMPLEINDEX n = pIns->Keyboard[p->note-1];
							if (n <= GetNumSamples()) sampleUsed[n] = true;
						}
					}
				}
			}
		}
	}
	for (SAMPLEINDEX ichk = GetNumSamples(); ichk >= 1; ichk--)
	{
		if ((!sampleUsed[ichk]) && (Samples[ichk].pSample)) nExt++;
	}

	return nExt;
}


// Destroy samples where keepSamples index is false. First sample is keepSamples[1]!
SAMPLEINDEX CSoundFile::RemoveSelectedSamples(const std::vector<bool> &keepSamples)
//---------------------------------------------------------------------------------
{
	if(keepSamples.empty())
	{
		return 0;
	}

	SAMPLEINDEX nRemoved = 0;
	for(SAMPLEINDEX nSmp = std::min(GetNumSamples(), static_cast<SAMPLEINDEX>(keepSamples.size() - 1)); nSmp >= 1; nSmp--)
	{
		if(!keepSamples[nSmp])
		{
			CriticalSection cs;

#ifdef MODPLUG_TRACKER
			if(GetpModDoc())
			{
				GetpModDoc()->GetSampleUndo().PrepareUndo(nSmp, sundo_replace, "Remove Sample");
			}
#endif // MODPLUG_TRACKER

			if(DestroySample(nSmp))
			{
				strcpy(m_szNames[nSmp], "");
				nRemoved++;
			}
			if((nSmp == GetNumSamples()) && (nSmp > 1)) m_nSamples--;
		}
	}
	return nRemoved;
}


bool CSoundFile::DestroySample(SAMPLEINDEX nSample)
//-------------------------------------------------
{
	if(!nSample || nSample >= MAX_SAMPLES)
	{
		return false;
	}
	if(Samples[nSample].pSample == nullptr)
	{
		return true;
	}

	ModSample &sample = Samples[nSample];

	for(CHANNELINDEX i = 0; i < MAX_CHANNELS; i++)
	{
		if(m_PlayState.Chn[i].pModSample == &sample)
		{
			m_PlayState.Chn[i].nPos = 0;
			m_PlayState.Chn[i].nLength = 0;
			m_PlayState.Chn[i].pCurrentSample = nullptr;
		}
	}

	sample.FreeSample();
	sample.nLength = 0;
	sample.uFlags.reset(CHN_16BIT | CHN_STEREO);

#ifdef MODPLUG_TRACKER
	ResetSamplePath(nSample);
#endif
	return true;
}


bool CSoundFile::DestroySampleThreadsafe(SAMPLEINDEX nSample)
//-----------------------------------------------------------
{
	CriticalSection cs;
	return DestroySample(nSample);
}


#ifdef MODPLUG_TRACKER
void CSoundFile::DeleteStaticdata()
//---------------------------------
{
	delete s_pTuningsSharedLocal; s_pTuningsSharedLocal = nullptr;
	delete s_pTuningsSharedBuiltIn; s_pTuningsSharedBuiltIn = nullptr;
}
#endif


#ifdef MODPLUG_TRACKER
bool CSoundFile::SaveStaticTunings()
//----------------------------------
{
	if(s_pTuningsSharedLocal->Serialize() != CTuningCollection::SERIALIZATION_SUCCESS)
	{
		AddToLog(LogError, MPT_USTRING("Static tuning serialisation failed"));
		return false;
	}
	return true;
}
#endif


#ifdef MODPLUG_TRACKER
bool CSoundFile::LoadStaticTunings()
//----------------------------------
{
	if(s_pTuningsSharedLocal || s_pTuningsSharedBuiltIn) return true;
	//For now not allowing to reload tunings(one should be careful when reloading them
	//since various parts may use addresses of the tuningobjects).

	s_pTuningsSharedBuiltIn = new CTuningCollection;
	s_pTuningsSharedLocal = new CTuningCollection("Local tunings");

	// Load built-in tunings.
	const char* pData = nullptr;
	HGLOBAL hglob = nullptr;
	size_t nSize = 0;
	if (LoadResource(MAKEINTRESOURCE(IDR_BUILTIN_TUNINGS), TEXT("TUNING"), pData, nSize, hglob) != nullptr)
	{
		std::istringstream iStrm(std::string(pData, nSize));
		s_pTuningsSharedBuiltIn->Deserialize(iStrm);
	}
	if(s_pTuningsSharedBuiltIn->GetNumTunings() == 0)
	{
		MPT_ASSERT(false);
		CTuningRTI* pT = new CTuningRTI;
		//Note: Tuning collection class handles deleting.
		pT->CreateGeometric(1,1);
		if(s_pTuningsSharedBuiltIn->AddTuning(pT))
			delete pT;
	}

	// Load local tunings.
	s_pTuningsSharedLocal->SetSavefilePath(
		TrackerDirectories::Instance().GetDefaultDirectory(DIR_TUNING)
		+ MPT_PATHSTRING("local_tunings")
		+ mpt::PathString::FromUTF8(CTuningCollection::s_FileExtension)
		);
	s_pTuningsSharedLocal->Deserialize();

	// Enabling adding/removing of tunings for standard collection
	// only for debug builds.
	#ifdef DEBUG
		s_pTuningsSharedBuiltIn->SetConstStatus(CTuningCollection::EM_ALLOWALL);
	#else
		s_pTuningsSharedBuiltIn->SetConstStatus(CTuningCollection::EM_CONST);
	#endif

	return false;
}
#else
#include "Tunings/built-inTunings.h"
void CSoundFile::LoadBuiltInTunings()
//-----------------------------------
{
	std::string data(built_inTunings_tc_data, built_inTunings_tc_data + built_inTunings_tc_size);
	std::istringstream iStrm(data);
	m_pTuningsBuiltIn->Deserialize(iStrm);
}
#endif


std::string CSoundFile::GetNoteName(const ModCommand::NOTE note, const INSTRUMENTINDEX inst) const
//------------------------------------------------------------------------------------------------
{
	// For MPTM instruments with custom tuning, find the appropriate note name. Else, use default note names.
	if(ModCommand::IsNote(note) && GetType() == MOD_TYPE_MPT && inst >= 1 && inst <= GetNumInstruments() && Instruments[inst] && Instruments[inst]->pTuning)
	{
		return Instruments[inst]->pTuning->GetNoteName(note - NOTE_MIDDLEC);
	} else
	{
		return GetNoteName(note);
	}
}


std::string CSoundFile::GetNoteName(const ModCommand::NOTE note)
//--------------------------------------------------------------
{
	if(ModCommand::IsSpecialNote(note))
	{
		const char specialNoteNames[][4] = { "PCs",  "PC ", "~~~", "^^^", "===" };
		STATIC_ASSERT(CountOf(specialNoteNames) == NOTE_MAX_SPECIAL - NOTE_MIN_SPECIAL + 1);

		return specialNoteNames[note - NOTE_MIN_SPECIAL];
	} else if(ModCommand::IsNote(note))
	{
		char name[4];
		MemCopy<char[4]>(name, szNoteNames[(note - NOTE_MIN) % 12]);	// e.g. "C#"
		name[2] = '0' + (note - NOTE_MIN) / 12;	// e.g. 5
		return name; //szNoteNames[(note - NOTE_MIN) % 12] + std::string(1, '0' + (note - NOTE_MIN) / 12);
	} else if(note == NOTE_NONE)
	{
		return "...";
	}
	return "???";
}


void CSoundFile::SetModSpecsPointer(const CModSpecifications*& pModSpecs, const MODTYPE type)
//-------------------------------------------------------------------------------------------
{
	switch(type)
	{
		case MOD_TYPE_MPT:
			pModSpecs = &ModSpecs::mptm;
		break;

		case MOD_TYPE_IT:
			pModSpecs = &ModSpecs::itEx;
		break;

		case MOD_TYPE_XM:
			pModSpecs = &ModSpecs::xmEx;
		break;

		case MOD_TYPE_S3M:
			pModSpecs = &ModSpecs::s3mEx;
		break;

		case MOD_TYPE_MOD:
		default:
			pModSpecs = &ModSpecs::mod;
			break;
	}
}


FlagSet<ModSpecificFlag> CSoundFile::GetModFlagMask(MODTYPE oldtype, MODTYPE newtype) const
//-----------------------------------------------------------------------------------------
{
	const Enum<MODTYPE>::value_type combined = oldtype | newtype;

	// XM <-> IT/MPT conversion.
	if(combined == (MOD_TYPE_IT|MOD_TYPE_XM) || combined == (MOD_TYPE_MPT|MOD_TYPE_XM))
		return MSF_COMPATIBLE_PLAY | MSF_MIDICC_BUGEMULATION | MSF_OLD_MIDI_PITCHBENDS;

	// IT <-> MPT conversion.
	if(combined == (MOD_TYPE_IT|MOD_TYPE_MPT))
		return FlagSet<ModSpecificFlag>().flip();

	return MSF_COMPATIBLE_PLAY;
}


void CSoundFile::ChangeModTypeTo(const MODTYPE& newType)
//------------------------------------------------------
{
	const MODTYPE oldtype = GetType();
	m_nType = newType;
	SetModSpecsPointer(m_pModSpecs, m_nType);

	if(oldtype == newType)
		return;

	SetupMODPanning(); // Setup LRRL panning scheme if needed

	m_ModFlags = m_ModFlags & GetModFlagMask(oldtype, newType);

	Order.OnModTypeChanged(oldtype);
	Patterns.OnModTypeChanged(oldtype);
}


bool CSoundFile::SetTitle(const std::string &newTitle)
//----------------------------------------------------
{
	if(songName != newTitle)
	{
		songName = newTitle;
		return true;
	}
	return false;
}


double CSoundFile::GetPlaybackTimeAt(ORDERINDEX ord, ROWINDEX row, bool updateVars, bool updateSamplePos)
//-------------------------------------------------------------------------------------------------------
{
	const GetLengthType t = GetLength(updateVars ? (updateSamplePos ? eAdjustSamplePositions : eAdjust) : eNoAdjust, GetLengthTarget(ord, row)).back();
	if(t.targetReached) return t.duration;
	else return -1; //Given position not found from play sequence.
}


// Calculate the length of a tick, depending on the tempo mode.
// This differs from GetTickDuration() by not accumulating errors
// because this is not called once per tick but in unrelated
// circumstances. So this should not update error accumulation.
void CSoundFile::RecalculateSamplesPerTick()
//------------------------------------------
{
	switch(m_nTempoMode)
	{
	case tempo_mode_classic:
	default:
		m_PlayState.m_nSamplesPerTick = (m_MixerSettings.gdwMixingFreq * 5) / (m_PlayState.m_nMusicTempo << 1);
		break;

	case tempo_mode_modern:
		m_PlayState.m_nSamplesPerTick = m_MixerSettings.gdwMixingFreq * (60 / m_PlayState.m_nMusicTempo / (m_PlayState.m_nMusicSpeed * m_PlayState.m_nCurrentRowsPerBeat));
		break;

	case tempo_mode_alternative:
		m_PlayState.m_nSamplesPerTick = m_MixerSettings.gdwMixingFreq / m_PlayState.m_nMusicTempo;
		break;
	}
#ifndef MODPLUG_TRACKER
	m_PlayState.m_nSamplesPerTick = Util::muldivr(m_PlayState.m_nSamplesPerTick, m_nTempoFactor, 65536);
#endif // !MODPLUG_TRACKER
}


// Get length of a tick in sample, with tick-to-tick tempo correction in modern tempo mode.
// This has to be called exactly once per tick because otherwise the error accumulation
// goes wrong.
uint32_t CSoundFile::GetTickDuration(uint32_t tempo, uint32_t speed, ROWINDEX rowsPerBeat)
//----------------------------------------------------------------------------------------
{
	UINT retval = 0;
	switch(m_nTempoMode)
	{
	case tempo_mode_classic:
	default:
		retval = (m_MixerSettings.gdwMixingFreq * 5) / (tempo << 1);
		break;

	case tempo_mode_alternative:
		retval = m_MixerSettings.gdwMixingFreq / tempo;
		break;

	case tempo_mode_modern:
		{
			double accurateBufferCount = static_cast<double>(m_MixerSettings.gdwMixingFreq) * (60.0 / static_cast<double>(tempo) / (static_cast<double>(speed * rowsPerBeat)));
			UINT bufferCount = static_cast<int>(accurateBufferCount);
			m_PlayState.m_dBufferDiff += accurateBufferCount - bufferCount;

			//tick-to-tick tempo correction:
			if(m_PlayState.m_dBufferDiff >= 1)
			{
				bufferCount++;
				m_PlayState.m_dBufferDiff--;
			} else if(m_PlayState.m_dBufferDiff <= -1)
			{
				bufferCount--;
				m_PlayState.m_dBufferDiff++;
			}
			MPT_ASSERT(fabs(m_PlayState.m_dBufferDiff) < 1);
			retval = bufferCount;
		}
		break;
	}
#ifndef MODPLUG_TRACKER
	// when the user modifies the tempo, we do not really care about accurate tempo error accumulation
	retval = Util::muldivr(retval, m_nTempoFactor, 65536);
#endif // !MODPLUG_TRACKER
	return retval;
}


// Get the duration of a row in milliseconds, based on the current rows per beat and given speed and tempo settings.
double CSoundFile::GetRowDuration(UINT tempo, UINT speed) const
//-------------------------------------------------------------
{
	switch(m_nTempoMode)
	{
	case tempo_mode_classic:
	default:
		return static_cast<double>(2500 * speed) / static_cast<double>(tempo);

	case tempo_mode_modern:
		{
			// If there are any row delay effects, the row length factor compensates for those.
			return 60000.0 / static_cast<double>(tempo) / static_cast<double>(m_PlayState.m_nCurrentRowsPerBeat);
		}

	case tempo_mode_alternative:
		return static_cast<double>(1000 * speed) / static_cast<double>(tempo);
	}
}


const CModSpecifications& CSoundFile::GetModSpecifications(const MODTYPE type)
//----------------------------------------------------------------------------
{
	const CModSpecifications* p = nullptr;
	SetModSpecsPointer(p, type);
	return *p;
}


// Find an unused sample slot. If it is going to be assigned to an instrument, targetInstrument should be specified.
// SAMPLEINDEX_INVLAID is returned if no free sample slot could be found.
SAMPLEINDEX CSoundFile::GetNextFreeSample(INSTRUMENTINDEX targetInstrument, SAMPLEINDEX start) const
//--------------------------------------------------------------------------------------------------
{
	// Find empty slot in two passes - in the first pass, we only search for samples with empty sample names,
	// in the second pass we check all samples with non-empty sample names.
	for(int passes = 0; passes < 2; passes++)
	{
		for(SAMPLEINDEX i = start; i <= GetModSpecifications().samplesMax; i++)
		{
			// When loading into an instrument, ignore non-empty sample names. Else, only use this slot if the sample name is empty or we're in second pass.
			if((i > GetNumSamples() && passes == 1)
				|| (Samples[i].pSample == nullptr && (!m_szNames[i][0] || passes == 1 || targetInstrument != INSTRUMENTINDEX_INVALID))
				|| (targetInstrument != INSTRUMENTINDEX_INVALID && IsSampleReferencedByInstrument(i, targetInstrument)))	// Not empty, but already used by this instrument. XXX this should only be done when replacing an instrument with a single sample! Otherwise it will use an inconsistent sample map!
			{
				// Empty slot, so it's a good candidate already.

				// In instrument mode, check whether any instrument references this sample slot. If that is the case, we won't use it as it could lead to unwanted conflicts.
				// If we are loading the sample *into* an instrument, we should also not consider that instrument's sample map, since it might be inconsistent at this time.
				bool isReferenced = false;
				for(INSTRUMENTINDEX ins = 1; ins <= GetNumInstruments(); ins++)
				{
					if(ins == targetInstrument)
					{
						continue;
					}
					if(IsSampleReferencedByInstrument(i, ins))
					{
						isReferenced = true;
						break;
					}
				}
				if(!isReferenced)
				{
					return i;
				}
			}
		}
	}

	return SAMPLEINDEX_INVALID;
}


// Find an unused instrument slot.
// INSTRUMENTINDEX_INVALID is returned if no free instrument slot could be found.
INSTRUMENTINDEX CSoundFile::GetNextFreeInstrument(INSTRUMENTINDEX start) const
//----------------------------------------------------------------------------
{
	for(INSTRUMENTINDEX i = start; i <= GetModSpecifications().instrumentsMax; i++)
	{
		if(Instruments[i] == nullptr)
		{
			return i;
		}
	}

	return INSTRUMENTINDEX_INVALID;
}


// Check whether a given sample is used by a given instrument.
bool CSoundFile::IsSampleReferencedByInstrument(SAMPLEINDEX sample, INSTRUMENTINDEX instr) const
//----------------------------------------------------------------------------------------------
{
	ModInstrument *targetIns = nullptr;
	if(instr > 0 && instr <= GetNumInstruments())
	{
		targetIns = Instruments[instr];
	}
	if(targetIns != nullptr)
	{
		for(size_t note = 0; note < NOTE_MAX /*CountOf(targetIns->Keyboard)*/; note++)
		{
			if(targetIns->Keyboard[note] == sample)
			{
				return true;
			}
		}
	}
	return false;
}


ModInstrument *CSoundFile::AllocateInstrument(INSTRUMENTINDEX instr, SAMPLEINDEX assignedSample)
//----------------------------------------------------------------------------------------------
{
	if(instr == 0 || instr >= MAX_INSTRUMENTS)
	{
		return nullptr;
	}

	ModInstrument *ins = Instruments[instr];
	if(ins != nullptr)
	{
		// Re-initialize instrument
		*ins = ModInstrument(assignedSample);
	} else
	{
		// Create new instrument
		Instruments[instr] = ins = new (std::nothrow) ModInstrument(assignedSample);
	}
	return ins;
}


void CSoundFile::PrecomputeSampleLoops(bool updateChannels)
//---------------------------------------------------------
{
	for(SAMPLEINDEX i = 1; i <= GetNumSamples(); i++)
	{
		Samples[i].PrecomputeLoops(*this, updateChannels);
	}
}


#ifdef MPT_EXTERNAL_SAMPLES
// Load external waveform, but keep sample properties like frequency, panning, etc...
// Returns true if the file could be loaded.
bool CSoundFile::LoadExternalSample(SAMPLEINDEX smp, const mpt::PathString &filename)
//-----------------------------------------------------------------------------------
{
	bool ok = false;
	InputFile f(filename);

	if(f.IsValid())
	{
		const ModSample origSample = Samples[smp];
		char origName[MAX_SAMPLENAME];
		mpt::String::Copy(origName, m_szNames[smp]);

		FileReader file = GetFileReader(f);
		ok = ReadSampleFromFile(smp, file, false);
		if(ok)
		{
			// Copy over old attributes, but keep new sample data
			ModSample &sample = GetSample(smp);
			SmpLength newLength = sample.nLength;
			void *newData = sample.pSample;
			SampleFlags newFlags = sample.uFlags;

			sample = origSample;
			sample.nLength = newLength;
			sample.pSample = newData;
			sample.uFlags.set(CHN_16BIT, newFlags[CHN_16BIT]);
			sample.uFlags.set(CHN_STEREO, newFlags[CHN_STEREO]);
			sample.uFlags.reset(SMP_MODIFIED);
			sample.SanitizeLoops();
		}
		mpt::String::Copy(m_szNames[smp], origName);
	}
	SetSamplePath(smp, filename);
	return ok;
}
#endif // MPT_EXTERNAL_SAMPLES


// Set up channel panning and volume suitable for MOD + similar files. If the current mod type is not MOD, bForceSetup has to be set to true.
void CSoundFile::SetupMODPanning(bool bForceSetup)
//------------------------------------------------
{
	// Setup LRRL panning, max channel volume
	if(!(GetType() & MOD_TYPE_MOD) && bForceSetup == false) return;

	for(CHANNELINDEX nChn = 0; nChn < MAX_BASECHANNELS; nChn++)
	{
		ChnSettings[nChn].nVolume = 64;
		ChnSettings[nChn].dwFlags.reset(CHN_SURROUND);
		if(m_MixerSettings.MixerFlags & SNDMIX_MAXDEFAULTPAN)
			ChnSettings[nChn].nPan = (((nChn & 3) == 1) || ((nChn & 3) == 2)) ? 256 : 0;
		else
			ChnSettings[nChn].nPan = (((nChn & 3) == 1) || ((nChn & 3) == 2)) ? 0xC0 : 0x40;
	}
}


OPENMPT_NAMESPACE_END
