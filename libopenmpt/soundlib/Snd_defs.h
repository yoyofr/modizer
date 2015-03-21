/*
 * Snd_Defs.h
 * ----------
 * Purpose: Basic definitions of data types, enums, etc. for the playback engine core.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "../common/typedefs.h"
#include "../common/FlagSet.h"


OPENMPT_NAMESPACE_BEGIN


typedef uint32 ROWINDEX;
	const ROWINDEX ROWINDEX_INVALID = uint32_max;
typedef uint16 CHANNELINDEX;
	const CHANNELINDEX CHANNELINDEX_INVALID = uint16_max;
typedef uint16 ORDERINDEX;
	const ORDERINDEX ORDERINDEX_INVALID = uint16_max;
	const ORDERINDEX ORDERINDEX_MAX = uint16_max - 1;
typedef uint16 PATTERNINDEX;
	const PATTERNINDEX PATTERNINDEX_INVALID = uint16_max;
typedef uint8  PLUGINDEX;
	const PLUGINDEX PLUGINDEX_INVALID = uint8_max;
typedef uint16 TEMPO;
typedef uint16 SAMPLEINDEX;
	const SAMPLEINDEX SAMPLEINDEX_INVALID = uint16_max;
typedef uint16 INSTRUMENTINDEX;
	const INSTRUMENTINDEX INSTRUMENTINDEX_INVALID = uint16_max;
typedef uint8 SEQUENCEINDEX;
	const SEQUENCEINDEX SEQUENCEINDEX_INVALID = uint8_max;

typedef uintptr_t SmpLength;


const SmpLength MAX_SAMPLE_LENGTH	= 0x10000000;	// Sample length in *samples*
													// Note: Sample size in bytes can be more than this (= 256 MB).

const ROWINDEX MAX_PATTERN_ROWS			= 1024;
const ORDERINDEX MAX_ORDERS				= 256;
const PATTERNINDEX MAX_PATTERNS			= 240;
const SAMPLEINDEX MAX_SAMPLES			= 4000;
const INSTRUMENTINDEX MAX_INSTRUMENTS	= 256;
const PLUGINDEX MAX_MIXPLUGINS			= 250;

const SEQUENCEINDEX MAX_SEQUENCES		= 50;

const CHANNELINDEX MAX_BASECHANNELS		= 127;	// Maximum pattern channels.
const CHANNELINDEX MAX_CHANNELS			= 256;	// Maximum number of mixing channels.

#define FREQ_FRACBITS		4		// Number of fractional bits in return value of CSoundFile::GetFreqFromPeriod()

// String lengths (including trailing null char)
#define MAX_SAMPLENAME			32
#define MAX_SAMPLEFILENAME		22
#define MAX_INSTRUMENTNAME		32
#define MAX_INSTRUMENTFILENAME	32
#define MAX_PATTERNNAME			32
#define MAX_CHANNELNAME			20

enum MODTYPE
{
	MOD_TYPE_NONE	= 0x00,
	MOD_TYPE_MOD	= 0x01,
	MOD_TYPE_S3M	= 0x02,
	MOD_TYPE_XM		= 0x04,
	MOD_TYPE_MED	= 0x08,
	MOD_TYPE_MTM	= 0x10,
	MOD_TYPE_IT		= 0x20,
	MOD_TYPE_669	= 0x40,
	MOD_TYPE_ULT	= 0x80,
	MOD_TYPE_STM	= 0x100,
	MOD_TYPE_FAR	= 0x200,
	MOD_TYPE_WAV	= 0x400, // PCM as module
	MOD_TYPE_AMF	= 0x800,
	MOD_TYPE_AMS	= 0x1000,
	MOD_TYPE_DSM	= 0x2000,
	MOD_TYPE_MDL	= 0x4000,
	MOD_TYPE_OKT	= 0x8000,
	MOD_TYPE_MID	= 0x10000,
	MOD_TYPE_DMF	= 0x20000,
	MOD_TYPE_PTM	= 0x40000,
	MOD_TYPE_DBM	= 0x80000,
	MOD_TYPE_MT2	= 0x100000,
	MOD_TYPE_AMF0	= 0x200000,
	MOD_TYPE_PSM	= 0x400000,
	MOD_TYPE_J2B	= 0x800000,
	MOD_TYPE_MPT	= 0x1000000,
	MOD_TYPE_IMF	= 0x2000000,
	MOD_TYPE_AMS2	= 0x4000000,
	MOD_TYPE_DIGI	= 0x8000000,
	MOD_TYPE_UAX	= 0x10000000, // sampleset as module
	MOD_TYPE_PLM	= 0x20000000,
};
DECLARE_FLAGSET(MODTYPE)


enum MODCONTAINERTYPE
{
	MOD_CONTAINERTYPE_NONE = 0x0,
	MOD_CONTAINERTYPE_MO3  = 0x1,
	MOD_CONTAINERTYPE_GDM  = 0x2,
	MOD_CONTAINERTYPE_UMX  = 0x3,
	MOD_CONTAINERTYPE_XPK  = 0x4,
	MOD_CONTAINERTYPE_PP20 = 0x5,
	MOD_CONTAINERTYPE_MMCMP= 0x6,
};


// For compatibility mode
#define TRK_IMPULSETRACKER	(MOD_TYPE_IT | MOD_TYPE_MPT)
#define TRK_FASTTRACKER2	(MOD_TYPE_XM)
#define TRK_SCREAMTRACKER	(MOD_TYPE_S3M)
#define TRK_PROTRACKER		(MOD_TYPE_MOD)
#define TRK_ALLTRACKERS		(~Enum<MODTYPE>::value_type())


// Channel flags:
enum ChannelFlags
{
	// Sample Flags
	CHN_16BIT			= 0x01,			// 16-bit sample
	CHN_LOOP			= 0x02,			// looped sample
	CHN_PINGPONGLOOP	= 0x04,			// bidi-looped sample
	CHN_SUSTAINLOOP		= 0x08,			// sample with sustain loop
	CHN_PINGPONGSUSTAIN	= 0x10,			// sample with bidi sustain loop
	CHN_PANNING			= 0x20,			// sample with forced panning
	CHN_STEREO			= 0x40,			// stereo sample
	CHN_REVERSE			= 0x80,			// start sample playback from sample / loop end (Velvet Studio feature) - this is intentionally the same flag as CHN_PINGPONGFLAG.
	// Channel Flags
	CHN_PINGPONGFLAG	= 0x80,			// when flag is on, sample is processed backwards
	CHN_MUTE			= 0x100,		// muted channel
	CHN_KEYOFF			= 0x200,		// exit sustain
	CHN_NOTEFADE		= 0x400,		// fade note (instrument mode)
	CHN_SURROUND		= 0x800,		// use surround channel
	// UNUSED			= 0x1000,
	// UNUSED			= 0x2000,
	CHN_FILTER			= 0x4000,		// Apply resonant filter on sample
	CHN_VOLUMERAMP		= 0x8000,		// Apply volume ramping
	CHN_VIBRATO			= 0x10000,		// Apply vibrato
	CHN_TREMOLO			= 0x20000,		// Apply tremolo
	//CHN_PANBRELLO		= 0x40000,		// Apply panbrello
	CHN_PORTAMENTO		= 0x80000,		// Apply portamento
	CHN_GLISSANDO		= 0x100000,		// Glissando mode
	CHN_FASTVOLRAMP		= 0x200000,		// Force usage of global ramping settings instead of ramping over the complete render buffer length
	CHN_EXTRALOUD		= 0x400000,		// Force sample to play at 0dB
	CHN_REVERB			= 0x800000,		// Apply reverb on this channel
	CHN_NOREVERB		= 0x1000000,	// Disable reverb on this channel
	CHN_SOLO			= 0x2000000,	// solo channel -> CODE#0012 -> DESC="midi keyboard split" -! NEW_FEATURE#0012
	CHN_NOFX			= 0x4000000,	// dry channel -> CODE#0015 -> DESC="channels management dlg" -! NEW_FEATURE#0015
	CHN_SYNCMUTE		= 0x8000000,	// keep sample sync on mute

	// Sample storage flags (also saved in ModSample::uFlags, but are not relevant to mixing)
	SMP_MODIFIED		= 0x1000,	// Sample data has been edited in the tracker
	SMP_KEEPONDISK		= 0x2000,	// Sample is not saved to file, data is restored from original sample file
};
DECLARE_FLAGSET(ChannelFlags)

#define CHN_SAMPLEFLAGS (CHN_16BIT | CHN_LOOP | CHN_PINGPONGLOOP | CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN | CHN_PANNING | CHN_STEREO | CHN_PINGPONGFLAG | CHN_REVERSE)
#define CHN_CHANNELFLAGS (~CHN_SAMPLEFLAGS)

// Sample flags fit into the first 16 bits, and with the current memory layout, storing them as a 16-bit integer gives struct ModSample a nice cacheable 64 bytes size in 32-bit builds.
typedef FlagSet<ChannelFlags, uint16> SampleFlags;


// Instrument envelope-specific flags
enum EnvelopeFlags
{
	ENV_ENABLED		= 0x01,	// env is enabled
	ENV_LOOP		= 0x02,	// env loop
	ENV_SUSTAIN		= 0x04,	// env sustain
	ENV_CARRY		= 0x08,	// env carry
	ENV_FILTER		= 0x10,	// filter env enabled (this has to be combined with ENV_ENABLED in the pitch envelope's flags)
};
DECLARE_FLAGSET(EnvelopeFlags)


// Envelope value boundaries
#define ENVELOPE_MIN		0		// Vertical min value of a point
#define ENVELOPE_MID		32		// Vertical middle line
#define ENVELOPE_MAX		64		// Vertical max value of a point
#define MAX_ENVPOINTS		240		// Maximum length of each instrument envelope


// Flags of 'dF..' datafield in extended instrument properties.
#define dFdd_VOLUME 		0x0001
#define dFdd_VOLSUSTAIN 	0x0002
#define dFdd_VOLLOOP 		0x0004
#define dFdd_PANNING 		0x0008
#define dFdd_PANSUSTAIN 	0x0010
#define dFdd_PANLOOP 		0x0020
#define dFdd_PITCH 			0x0040
#define dFdd_PITCHSUSTAIN 	0x0080
#define dFdd_PITCHLOOP 		0x0100
#define dFdd_SETPANNING 	0x0200
#define dFdd_FILTER 		0x0400
#define dFdd_VOLCARRY 		0x0800
#define dFdd_PANCARRY 		0x1000
#define dFdd_PITCHCARRY 	0x2000
#define dFdd_MUTE 			0x4000


// Instrument-specific flags
enum InstrumentFlags
{
	INS_SETPANNING	= 0x01,	// Panning enabled
	INS_MUTE		= 0x02,	// Instrument is muted
};
DECLARE_FLAGSET(InstrumentFlags)


// envelope types in instrument editor
enum enmEnvelopeTypes
{
	ENV_VOLUME = 0,
	ENV_PANNING,
	ENV_PITCH,
};

// Filter Modes
#define FLTMODE_UNCHANGED		0xFF
#define FLTMODE_LOWPASS			0
#define FLTMODE_HIGHPASS		1


// NNA types (New Note Action)
#define NNA_NOTECUT		0
#define NNA_CONTINUE	1
#define NNA_NOTEOFF		2
#define NNA_NOTEFADE	3

// DCT types (Duplicate Check Types)
#define DCT_NONE		0
#define DCT_NOTE		1
#define DCT_SAMPLE		2
#define DCT_INSTRUMENT	3
#define DCT_PLUGIN		4

// DNA types (Duplicate Note Action)
#define DNA_NOTECUT		0
#define DNA_NOTEOFF		1
#define DNA_NOTEFADE	2


// Module flags - contains both song configuration and playback state... Use SONG_FILE_FLAGS and SONG_PLAY_FLAGS distinguish between the two.
enum SongFlags
{
	SONG_EMBEDMIDICFG	= 0x0001,		// Embed macros in file
	SONG_FASTVOLSLIDES	= 0x0002,		// Old Scream Tracker 3.0 volume slides
	SONG_ITOLDEFFECTS	= 0x0004,		// Old Impulse Tracker effect implementations
	SONG_ITCOMPATGXX	= 0x0008,		// IT "Compatible Gxx" (IT's flag to behave more like other trackers w/r/t portamento effects)
	SONG_LINEARSLIDES	= 0x0010,		// Linear slides vs. Amiga slides
	SONG_PATTERNLOOP	= 0x0020,		// Loop current pattern (pattern editor)
	SONG_STEP			= 0x0040,		// Song is in "step" mode (pattern editor)
	SONG_PAUSED			= 0x0080,		// Song is paused (no tick processing, just rendering audio)
	SONG_FADINGSONG		= 0x0100,		// Song is fading out
	SONG_ENDREACHED		= 0x0200,		// Song is finished
	//SONG_GLOBALFADE	= 0x0400,		// Song is fading out
	//SONG_CPUVERYHIGH	= 0x0800,		// High CPU usage
	SONG_FIRSTTICK		= 0x1000,		// Is set when the current tick is the first tick of the row
	SONG_MPTFILTERMODE	= 0x2000,		// Local filter mode (reset filter on each note)
	SONG_SURROUNDPAN	= 0x4000,		// Pan in the rear channels
	SONG_EXFILTERRANGE	= 0x8000,		// Cutoff Filter has double frequency range (up to ~10Khz)
	SONG_AMIGALIMITS	= 0x10000,		// Enforce amiga frequency limits
	//SONG_ITPROJECT	= 0x20000,		// Is a project file
	//SONG_ITPEMBEDIH	= 0x40000,		// Embed instrument headers in project file
	SONG_BREAKTOROW		= 0x80000,		// Break to row command encountered (internal flag, do not touch)
	SONG_POSJUMP		= 0x100000,		// Position jump encountered (internal flag, do not touch)
	SONG_PT1XMODE		= 0x200000,		// ProTracker 1/2 playback mode
	SONG_PLAYALLSONGS	= 0x400000,		// Play all subsongs consecutively (libopenmpt)
	SONG_VBLANK_TIMING	= 0x800000,		// Use MOD VBlank timing (F21 and higher set speed instead of tempo)
};
DECLARE_FLAGSET(SongFlags)

#define SONG_FILE_FLAGS	(SONG_EMBEDMIDICFG|SONG_FASTVOLSLIDES|SONG_ITOLDEFFECTS|SONG_ITCOMPATGXX|SONG_LINEARSLIDES|SONG_EXFILTERRANGE|SONG_AMIGALIMITS|SONG_PT1XMODE|SONG_VBLANK_TIMING)
#define SONG_PLAY_FLAGS (~SONG_FILE_FLAGS)

// Global Options (Renderer)
#ifndef NO_AGC
#define SNDDSP_AGC            0x40	// automatic gain control
#endif // ~NO_AGC
#ifndef NO_DSP
#define SNDDSP_MEGABASS       0x02	// bass expansion
#define SNDDSP_SURROUND       0x08	// surround mix
#endif // NO_DSP
#ifndef NO_REVERB
#define SNDDSP_REVERB         0x20	// apply reverb
#endif // NO_REVERB
#ifndef NO_EQ
#define SNDDSP_EQ             0x80	// apply EQ
#endif // NO_EQ

#define SNDMIX_SOFTPANNING    0x10	// soft panning mode (this is forced with mixmode RC3 and later)

// Misc Flags (can safely be turned on or off)
#define SNDMIX_MAXDEFAULTPAN	0x80000		// Used by the MOD loader (currently unused)
#define SNDMIX_MUTECHNMODE		0x100000	// Notes are not played on muted channels


#define MAX_GLOBAL_VOLUME 256u

// Resampling modes
enum ResamplingMode
{
	// ATTENTION: Do not change ANY of these values, as they get written out to files in per instrument interpolation settings
	// and old files have these exact values in them which should not change meaning.
	SRCMODE_NEAREST   = 0,
	SRCMODE_LINEAR    = 1,
	SRCMODE_SPLINE    = 2,
	SRCMODE_POLYPHASE = 3,
	SRCMODE_FIRFILTER = 4,
	SRCMODE_DEFAULT   = 5,
};

static inline bool IsKnownResamplingMode(int mode)
{
	return (mode >= 0) && (mode < SRCMODE_DEFAULT);
}


// Release node defines
#define ENV_RELEASE_NODE_UNSET	0xFF
#define NOT_YET_RELEASED		(-1)
STATIC_ASSERT(ENV_RELEASE_NODE_UNSET > MAX_ENVPOINTS);


enum PluginPriority
{
	ChannelOnly,
	InstrumentOnly,
	PrioritiseInstrument,
	PrioritiseChannel,
};

enum PluginMutePriority
{
	EvenIfMuted,
	RespectMutes,
};

//Plugin velocity handling options
enum PLUGVELOCITYHANDLING
{
	PLUGIN_VELOCITYHANDLING_CHANNEL = 0,
	PLUGIN_VELOCITYHANDLING_VOLUME
};

//Plugin volumecommand handling options
enum PLUGVOLUMEHANDLING
{
	PLUGIN_VOLUMEHANDLING_MIDI = 0,
	PLUGIN_VOLUMEHANDLING_DRYWET,
	PLUGIN_VOLUMEHANDLING_IGNORE,
	PLUGIN_VOLUMEHANDLING_CUSTOM,
	PLUGIN_VOLUMEHANDLING_MAX,
};

enum MidiChannel
{
	MidiNoChannel		= 0,
	MidiFirstChannel	= 1,
	MidiLastChannel		= 16,
	MidiMappedChannel	= 17,
};


// Vibrato Types
enum VibratoType
{
	VIB_SINE = 0,
	VIB_SQUARE,
	VIB_RAMP_UP,
	VIB_RAMP_DOWN,
	VIB_RANDOM
};


OPENMPT_NAMESPACE_END
