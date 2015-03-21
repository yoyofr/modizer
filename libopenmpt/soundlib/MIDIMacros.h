/*
 * MIDIMacros.h
 * ------------
 * Purpose: Helper functions / classes for MIDI Macro functionality.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#ifdef MODPLUG_TRACKER
OPENMPT_NAMESPACE_BEGIN
class CSoundFile;
OPENMPT_NAMESPACE_END
#include "plugins/PlugInterface.h"
#include "Snd_defs.h"
#endif // MODPLUG_TRACKER

OPENMPT_NAMESPACE_BEGIN

// Parametered macro presets
enum parameteredMacroType
{
	sfx_unused = 0,
	sfx_cutoff,			// Type 1 - Z00 - Z7F controls resonant filter cutoff
	sfx_reso,			// Type 2 - Z00 - Z7F controls resonant filter resonance
	sfx_mode,			// Type 3 - Z00 - Z7F controls resonant filter mode (lowpass / highpass)
	sfx_drywet,			// Type 4 - Z00 - Z7F controls plugin Dry / Wet ratio
	sfx_plug,			// Type 5 - Z00 - Z7F controls a plugin parameter
	sfx_cc,				// Type 6 - Z00 - Z7F controls MIDI CC
	sfx_channelAT,		// Type 7 - Z00 - Z7F controls Channel Aftertouch
	sfx_polyAT,			// Type 8 - Z00 - Z7F controls Poly Aftertouch
	sfx_custom,

	sfx_max
};


// Fixed macro presets
enum fixedMacroType
{
	zxx_unused = 0,
	zxx_reso4Bit,		// Type 1 - Z80 - Z8F controls resonant filter resonance
	zxx_reso7Bit,		// Type 2 - Z80 - ZFF controls resonant filter resonance
	zxx_cutoff,			// Type 3 - Z80 - ZFF controls resonant filter cutoff
	zxx_mode,			// Type 4 - Z80 - ZFF controls resonant filter mode (lowpass / highpass)
	zxx_resomode,		// Type 5 - Z80 - Z9F controls resonance + filter mode
	zxx_channelAT,		// Type 6 - Z80 - ZFF controls Channel Aftertouch
	zxx_polyAT,			// Type 7 - Z80 - ZFF controls Poly Aftertouch
	zxx_custom,

	zxx_max
};


// Global macro types
enum
{
	MIDIOUT_START = 0,
	MIDIOUT_STOP,
	MIDIOUT_TICK,
	MIDIOUT_NOTEON,
	MIDIOUT_NOTEOFF,
	MIDIOUT_VOLUME,
	MIDIOUT_PAN,
	MIDIOUT_BANKSEL,
	MIDIOUT_PROGRAM,
};


#define NUM_MACROS 16	// number of parametered macros
#define MACRO_LENGTH 32	// max number of chars per macro


#ifdef NEEDS_PRAGMA_PACK
#pragma pack(push, 1)
#endif

struct PACKED MIDIMacroConfigData
{
	char szMidiGlb[9][MACRO_LENGTH];		// Global MIDI macros
	char szMidiSFXExt[16][MACRO_LENGTH];	// Parametric MIDI macros
	char szMidiZXXExt[128][MACRO_LENGTH];	// Fixed MIDI macros
};

STATIC_ASSERT(sizeof(MIDIMacroConfigData) == 4896); // this is directly written to files, so the size must be correct!

//=======================================================
class PACKED MIDIMacroConfig : public MIDIMacroConfigData
//=======================================================
{

public:

	MIDIMacroConfig() { Reset(); };

	// Get macro type from a macro string
	parameteredMacroType GetParameteredMacroType(size_t macroIndex) const;
	fixedMacroType GetFixedMacroType() const;

	// Create a new macro
protected:
	void CreateParameteredMacro(char (&parameteredMacro)[MACRO_LENGTH], parameteredMacroType macroType, int subType) const;
public:
	void CreateParameteredMacro(size_t macroIndex, parameteredMacroType macroType, int subType = 0)
	{
		CreateParameteredMacro(szMidiSFXExt[macroIndex], macroType, subType);
	};
	std::string CreateParameteredMacro(parameteredMacroType macroType, int subType = 0) const
	{
		char parameteredMacro[MACRO_LENGTH];
		CreateParameteredMacro(parameteredMacro, macroType, subType);
		return std::string(parameteredMacro);
	};

protected:
	void CreateFixedMacro(char (&fixedMacros)[128][MACRO_LENGTH], fixedMacroType macroType) const;
public:
	void CreateFixedMacro(fixedMacroType macroType)
	{
		CreateFixedMacro(szMidiZXXExt, macroType);
	};

#ifdef MODPLUG_TRACKER

	// Translate macro type or macro string to macro name
	CString GetParameteredMacroName(size_t macroIndex, PLUGINDEX plugin, const CSoundFile &sndFile) const;
	CString GetParameteredMacroName(parameteredMacroType macroType) const;
	CString GetFixedMacroName(fixedMacroType macroType) const;

	// Extract information from a parametered macro string.
	int MacroToPlugParam(size_t macroIndex) const;
	int MacroToMidiCC(size_t macroIndex) const;

	// Check if any macro can automate a given plugin parameter
	int FindMacroForParam(PlugParamIndex param) const;

#endif // MODPLUG_TRACKER

	// Check if a given set of macros is the default IT macro set.
	bool IsMacroDefaultSetupUsed() const;

	// Reset MIDI macro config to default values.
	void Reset();

	// Sanitize all macro config strings.
	void Sanitize();

	// Fix old-format (not conforming to IT's MIDI macro definitions) MIDI config strings.
	void UpgradeMacros();

protected:

	// Helper function for FixMacroFormat()
	void UpgradeMacroString(char *macro) const;

	// Remove blanks and other unwanted characters from macro strings for internal usage.
	std::string GetSafeMacro(const char *macro) const;

};

STATIC_ASSERT(sizeof(MIDIMacroConfig) == sizeof(MIDIMacroConfigData)); // this is directly written to files, so the size must be correct!

#ifdef NEEDS_PRAGMA_PACK
#pragma pack(pop)
#endif

OPENMPT_NAMESPACE_END
