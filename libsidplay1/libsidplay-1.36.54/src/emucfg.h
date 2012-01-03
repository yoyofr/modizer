//
// /home/ms/source/sidplay/libsidplay/include/RCS/emucfg.h,v
// 
// This is an interface to the Emulator Engine and does not access any
// audio hardware or driver.
//
// THIS CLASS INTERFACE CAN ONLY BE USED TO INSTANTIATE A -SINGLE OBJECT-.
// DUE TO EFFICIENCY CONCERNS, THE EMULATOR ENGINE CONSISTS OF -GLOBAL-
// OBJECTS. Currently, the only aim of this class is to allow a safe
// initialization of those objects.
// 

#ifndef SIDPLAY1_EMUCFG_H
#define SIDPLAY1_EMUCFG_H


#include "compconf.h"
#include "mytypes.h"
#include "sidtune.h"


// An instance of this structure is used to transport emulator settings
// to and from the interface class.
struct emuConfig
{
	uword frequency;       // [frequency] = Hz, min=4000, max=48000
	int bitsPerSample;     // see below, ``Sample precision''
	int sampleFormat;      // see below, ``Sample encoding''
	int channels;          // see below, ``Number of physical audio channels''
	int sidChips;          // --- unsupported ---
	int volumeControl;     // see below, ``Volume control modes''

	bool mos8580;          // true, false (just the waveforms)
	bool measuredVolume;   // true, false

	bool emulateFilter;    // true, false
	float filterFs;        // 1.0 <= Fs
	float filterFm;        // Fm != 0
	float filterFt;        //
	
	int memoryMode;        // MPU_BANK_SWITCHING, MPU_TRANSPARENT_ROM,
                           // MPU_PLAYSID_ENVIRONMENT
	
	int clockSpeed;        // SIDTUNE_CLOCK_PAL, SIDTUNE_CLOCK_NTSC

	bool forceSongSpeed;   // true, false
	
	//
	// Working, but experimental.
	//
	int digiPlayerScans;   // 0=off, number of C64 music player calls used to
	                       // scan it for PlaySID Extended SID Register usage.

	int autoPanning;       // see below, ``Auto-panning''
};


// Memory mode settings:
//
// MPU_BANK_SWITCHING: Does emulate every bank-switching that one can
// consider as useful for music players.
//
// MPU_TRANSPARENT_ROM: An emulator environment with partial bank-switching.
// Required to run sidtunes which:
//
//  - use the RAM under the I/O address space
//  - use RAM at $E000-$FFFA + jump to Kernal functions
//
// MPU_PLAYSID_ENVIRONMENT: A PlaySID-like emulator environment.
// Required to run sidtunes which:
//
//  - are specific to PlaySID
//  - do not contain bank-switching code
//
// Sidtunes that would not run on a real C64 because their players were
// prepared to assume the certain emulator environment provided by PlaySID.

enum
{
	// Memory mode settings.
	//
	MPU_BANK_SWITCHING = 0x20,
	MPU_TRANSPARENT_ROM,
	MPU_PLAYSID_ENVIRONMENT,
		
	// Volume control modes. Use ``SIDEMU_NONE'' for no control.
	//
	SIDEMU_VOLCONTROL = 0x40,
	SIDEMU_FULLPANNING,
	SIDEMU_HWMIXING,
	SIDEMU_STEREOSURROUND,

	// Auto-panning modes. Use ``SIDEMU_NONE'' for none.
	//
	SIDEMU_CENTEREDAUTOPANNING = 0x50,

	// This can either be used as a dummy, or where one does not
	// want to make an alternative setting.
	SIDEMU_NONE = 0x1000
};


// Sample format and configuration constants. The values are intended to 
// be distinct from each other. Some of the constants have a most obvious
// value, so they can be used in calculations.

// Sample encoding (format).
const int SIDEMU_UNSIGNED_PCM = 0x80;
const int SIDEMU_SIGNED_PCM = 0x7F;

// Number of physical audio channels.
// ``Stereo'' means interleaved channel data.
const int SIDEMU_MONO = 1;
const int SIDEMU_STEREO = 2;

// Sample precision (bits per sample). The endianess of the stored samples
// is machine dependent.
const int SIDEMU_8BIT = 8;
const int SIDEMU_16BIT = 16;


// Auto-panning modes. Only valid for mixing modes ``SIDEMU_FULLPANNING''
// or ``SIDEMU_STEREOSURROUND''.
//
// The volume levels left/right build the panning boundaries. The panning
// range is the difference between left and right level. After enabling
// this you can override the default levels with your own ones using the
// setVoiceVolume() function. A default is provided to ensure sane
// initial settings.
// NOTE: You can mute a voice by setting left=right=0 or total=0.
//
// Auto-panning starts each new note on the opposite pan-position and
// then moves between the left and right volume level.
//
// Centered auto-panning starts in the middle, moves outwards and then
// toggles between the two pan-positions like normal auto-panning.


// Default filter parameters.
const float SIDEMU_DEFAULTFILTERFS = 400.0;
const float SIDEMU_DEFAULTFILTERFM = 60.0;
const float SIDEMU_DEFAULTFILTERFT = 0.05;


// Volume control modes
//
//	 int volumeControl;
//	 bool setVoiceVolume(int voice, ubyte leftLevel, ubyte rightLevel, uword total);
//	 uword getVoiceVolume(int voice);
//
// Relative voice volume is ``total'' from 0 (mute) to 256 (max). If you use it,
// you don't have to modulate each L/R level yourself.
//
// A noticable difference between FULLPANNING and VOLCONTROL is FULLPANNING's
// capability to mix four (all) logical voices to a single physical audio
// channel, whereas VOLCONTROL is only able to mix two (half of all) logical
// voices to a physical channel.
// Therefore VOLCONTROL results in slightly better sample quality, because
// it mixes at higher amplitude. Especially when using a sample precision of
// 8-bit and stereo. In mono mode both modes operate equally.
//
// NOTE: Changing the volume control mode resets the current volume
// level settings for all voices to a default:
//
//     MONO  | left | right     STEREO | left | right
//   -----------------------   -----------------------
//   voice 1 |  255 |   0      voice 1 |  255 |   0
//   voice 2 |  255 |   0      voice 2 |    0 | 255
//   voice 3 |  255 |   0      voice 3 |  255 |   0
//   voice 4 |  255 |   0      voice 4 |    0 | 255
//
//   SURROUND | left | right 
//   ------------------------
//    voice 1 |  255 |  255
//    voice 2 |  255 |  255
//    voice 3 |  255 |  255
//    voice 4 |  255 |  255
//
//
// Because of the asymmetric ``three-voice'' nature of most sidtunes, it is
// strongly advised to *not* use plain stereo without pan-positioning the
// voices.
//
//	 int digiPlayerScans;
//
// If the integer above is set to ``x'', the sidtune will be scanned x player
// calls for PlaySID digis on the fourth channel. If no digis are used, the
// sidtune is hopefully ``three-voice-only'' and can be amplified a little bit.
//
//
// SIDEMU_NONE
//
//   No volume control at all. Volume level of each voice is not adjustable.
//   Voices cannot be turned off. No panning possible. Most likely maximum
//   software mixing speed.
//
//
// SIDEMU_VOLCONTROL
//
//   In SIDEMU_STEREO mode two voices should build a pair, satisfying the
//   equation (leftlevel_A + leftlevel_B) <= 255. Generally, the equations:
//     sum leftLevel(i) <= 512   and   sum rightLevel(i) <= 512
//   must be satisfied, i = [1,2,3,4].
//
//   In SIDEMU_MONO mode only the left level is used to specify a voice's
//   volume. If you specify a right level, it will be set to zero.
//
//
// SIDEMU_FULLPANNING
//
//   Volume level of each voice is adjustable between 255 (max) and 0 (off).
//   Each voice can be freely positioned between left and right, or both.
//
//
// SIDEMU_STEREOSURROUND
//
//   Volume level of each voice is adjustable between 255 (max) and 0 (off).
//   Each voice can be freely positioned between left and right.
//   Effect is best for left=255 plus right=255.
//
//
// SIDEMU_HWMIXING
//
//   Used for external mixing only. The sample buffer is split into four (4)
//   equivalent chunks, each representing a single voice. The client has to
//   take care of the sample buffer length to be dividable by four.


class sidTune;


class emuEngine
{
 public:  // --------------------------------------------------------- public

	// The constructor creates and initializes the object with defaults.
	// Upon successful creation, use emuEngine::getConfig(...) to
	// retrieve the default settings.
	emuEngine();
	virtual ~emuEngine();  // destructor

	const char* getVersionString();
	
	// Set and retrieve the SID emulator settings. Invalid values will not
	// be accepted.
	// Returns: false, if invalid values.
	//          true, else.
	bool setConfig( struct emuConfig& );
	void getConfig( struct emuConfig& );

	// Use this function together with a valid sidTune-object to fill
	// a buffer with calculated sample data.
	friend void sidEmuFillBuffer(emuEngine&, sidTune&,
								 void* buffer, udword bufLen );

	// See ``sidtune.h'' for info on these.
	friend bool sidEmuInitializeSong(emuEngine &, sidTune &, uword songNum);
	friend bool sidEmuInitializeSongOld(emuEngine &, sidTune &,	uword songNum);
	
	// Reset the filter parameters to default settings.
	void setDefaultFilterStrength();
		
	// This will even work during playback, but only in volume control modes
	// SIDEMU_VOLCONTROL, SIDEMU_FULLPANNING or SIDEMU_STEREOSURROUND. For the
	// modes SIDEMU_NONE or SIDEMU_HWMIXING this function has no effect.
    //
	// voice=[1,2,3,4], leftLevel=[0,..,255], rightLevel=[0,...,255]
	// total: 0=mute, ~128=middle, 256=max
	bool setVoiceVolume(int voice, ubyte leftLevel, ubyte rightLevel, uword total);
	
	// Returns: high-byte = left level, low-byte = right level.
	uword getVoiceVolume(int voice);
	
    // Only useful to determine the state of a newly created object and
	// the current state after returning from a member function.
    operator bool()  { return isReady; }
    bool getStatus()  { return isReady; }

	// Public to the user, but need not be used explicitly.
	bool reset();
	bool resetSampleEmu();
	void amplifyThreeVoiceTunes(bool isThreeVoiceTune);
	
	// Used for a run-time endianess check on Unix. Verifies the compiled
	// code and returns ``false'' if incorrectly set up.
	bool verifyEndianess();

#if defined(SIDEMU_TIME_COUNT)
	int getSecondsThisSong();
	int getSecondsTotal();
	
	void resetSecondsThisSong()  { secondsThisSong = 0; }
#endif
	
	
 protected:  // --------------------------------------------------- protected

	// Set a random (!) random seed value.
	virtual void setRandomSeed();  // default uses ``time.h''
	
	
 private:  // ------------------------------------------------------- private

	bool isReady;
	emuConfig config;

#if defined(SIDEMU_TIME_COUNT)
    // Used for time and listening mileage.
	udword bytesCountTotal, bytesCountSong;
	int secondsTotal, secondsThisSong;
#endif
	
	// 6510-interpreter.
	//
	bool MPUstatus;
	int memoryMode, clockSpeed;
	ubyte randomSeed;
	
	void MPUreset();
	ubyte * MPUreturnRAMbase();

	//
	bool isThreeVoiceAmplified;  // keep track of current mixer state
	bool isThreeVoiceTune;       // this toggled from outside

	bool freeMem();
	bool allocMem();
	void configureSID();
	void initMixerEngine();
	void initMixerFunction();
	void setDefaultVoiceVolumes();
	void filterTableInit();
   
    emuEngine(const emuEngine&);
    emuEngine& operator=(emuEngine&);
};


#endif  /* SIDPLAY1_EMUCFG_H */
