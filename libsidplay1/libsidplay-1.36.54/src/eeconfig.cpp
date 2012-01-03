//
// /home/ms/source/sidplay/libsidplay/RCS/eeconfig.cpp,v
//
// SID Emulator Engine configuration interface class.
// Quick documentation in header file ``include/emucfg.h''.

#include "eeconfig.h"

// ------------------------------------------------------------- constructors

emuEngine::emuEngine()
{
	// Set the defaults.
	config.frequency = 44100*1;
	config.bitsPerSample = SIDEMU_16BIT;
	config.sampleFormat = SIDEMU_SIGNED_PCM;
	config.channels = SIDEMU_MONO;
	config.sidChips = 1;
	config.volumeControl = SIDEMU_NONE;
	config.mos8580 = false;
	config.measuredVolume = true;
	config.digiPlayerScans = 10*50;
	config.emulateFilter = true;
	config.autoPanning = SIDEMU_NONE;
	config.memoryMode = MPU_BANK_SWITCHING;
	config.clockSpeed = SIDTUNE_CLOCK_PAL;
	config.forceSongSpeed = false;
	
#if defined(SIDEMU_TIME_COUNT)
	// Reset data counter.
	bytesCountTotal = (bytesCountSong = 0);
	secondsTotal = (secondsThisSong = 0);
#endif
	
	isThreeVoiceTune = false;
	
	extern void sidEmuResetAutoPanning(int);
	sidEmuResetAutoPanning(config.autoPanning);

	// Allocate memory for the interpreter.
	c64memFree();
	MPUstatus = c64memAlloc();
	
	// Allocate memory for the SID emulator engine.
	freeMem();
	if ( MPUstatus && allocMem() )
	{
		setRandomSeed();
		MPUreset();
		configureSID();
		initMixerEngine();
		setDefaultVoiceVolumes();
		setDefaultFilterStrength();
		reset();
		isReady = true;
	}
	else
	{
		isReady = false;
	}
}

// -------------------------------------------------------------- destructors

emuEngine::~emuEngine()
{
	c64memFree();
	freeMem();
}

// -------------------------------------------------- public member functions

const char* versionString = emu_version;

const char* emuEngine::getVersionString()
{
	return versionString;
}

bool emuEngine::setConfig( struct emuConfig& inCfg )
{
	bool gotInvalidConfig = false;
	
	// Validate input value.
	if ((inCfg.memoryMode == MPU_BANK_SWITCHING)
		|| (inCfg.memoryMode == MPU_TRANSPARENT_ROM)
		|| (inCfg.memoryMode == MPU_PLAYSID_ENVIRONMENT))			
	{
		config.memoryMode = inCfg.memoryMode;
	}
	else
	{
		gotInvalidConfig = true;  // invalid settings
	}

	// Validate input value.
	// Check various settings before doing a single SID-config call.
	bool newSIDconfig = false;
	bool newFilterInit = false;

	if ((inCfg.clockSpeed == SIDTUNE_CLOCK_PAL)
		|| (inCfg.clockSpeed == SIDTUNE_CLOCK_NTSC))
	{
		if (inCfg.clockSpeed != config.clockSpeed)
		{
			config.clockSpeed = inCfg.clockSpeed;
			newSIDconfig = true;
		}
	}
	else
	{
		gotInvalidConfig = true;  // invalid settings
	}

	if (inCfg.forceSongSpeed != config.forceSongSpeed)
	{
		config.forceSongSpeed = (inCfg.forceSongSpeed == true);
	}

	// Range-check the sample frequency.
	if (( inCfg.frequency >= 4000 ) && ( inCfg.frequency <= 48000 ))
	{	
		// Has it changed ? Then do the necessary initialization.
		if ( inCfg.frequency != config.frequency )
		{
			config.frequency = inCfg.frequency;
			newSIDconfig = true;
			newFilterInit = true;
		}
	}
	else
	{
		gotInvalidConfig = true;  // invalid settings
	}

	if (inCfg.measuredVolume != config.measuredVolume)
	{
		config.measuredVolume = (inCfg.measuredVolume == true);
		newSIDconfig = true;
	}

	// The mixer mode, the sample format, the number of channels and bits per
	// sample all affect the mixing tables and settings.
	// Hence we define a handy flag here.
	bool newMixerSettings = false;

	// Is the requested sample format valid?
	if (( inCfg.sampleFormat == SIDEMU_UNSIGNED_PCM ) 
		|| ( inCfg.sampleFormat == SIDEMU_SIGNED_PCM ))
	{
		// Has it changed ? Then do the necessary initialization.
		if ( inCfg.sampleFormat != config.sampleFormat )
		{
			config.sampleFormat = inCfg.sampleFormat;
			newMixerSettings = true;
		}
	}
	else
	{
		gotInvalidConfig = true;  // invalid settings
	}
	
	// Is the requested number of channels valid?
	if (( inCfg.channels == SIDEMU_MONO ) 
		|| ( inCfg.channels == SIDEMU_STEREO ))
	{
		// Has it changed ? Then do the necessary initialization.
		if ( inCfg.channels != config.channels )
		{
			config.channels = inCfg.channels;
			setDefaultVoiceVolumes();
			newMixerSettings = true;
		}
	}
	else
	{
		gotInvalidConfig = true;  // invalid settings
	}
	
	// Is the requested sample precision valid?
	if (( inCfg.bitsPerSample == SIDEMU_8BIT ) 
		|| ( inCfg.bitsPerSample == SIDEMU_16BIT ))
	{
		// Has it changed ? Then do the necessary initialization.
		if ( inCfg.bitsPerSample != config.bitsPerSample )
		{
			config.bitsPerSample = inCfg.bitsPerSample;
			newMixerSettings = true;
		}
	}
	else
	{
		gotInvalidConfig = true;  // invalid settings
	}
	
	// Is the requested mixing mode valid?
	if (( inCfg.volumeControl == SIDEMU_NONE ) 
		|| ( inCfg.volumeControl == SIDEMU_VOLCONTROL )
		|| ( inCfg.volumeControl == SIDEMU_FULLPANNING )
		|| ( inCfg.volumeControl == SIDEMU_HWMIXING )
		|| ( inCfg.volumeControl == SIDEMU_STEREOSURROUND ))
	{
		// Has it changed ? Then do the necessary initialization.
		if ( inCfg.volumeControl != config.volumeControl )
		{
			config.volumeControl = inCfg.volumeControl;
			setDefaultVoiceVolumes();
			newMixerSettings = true;
		}
	}
	else
	{
		gotInvalidConfig = true;  // invalid settings
	}

	if ((inCfg.autoPanning == SIDEMU_NONE)
		|| (inCfg.autoPanning == SIDEMU_CENTEREDAUTOPANNING))
	{
		if (inCfg.autoPanning != config.autoPanning)
		{
			config.autoPanning = inCfg.autoPanning;
			if (config.autoPanning != SIDEMU_NONE)
			{
				if (( config.volumeControl != SIDEMU_FULLPANNING )
					&& ( config.volumeControl != SIDEMU_STEREOSURROUND ))
				{
					config.autoPanning = false;
					gotInvalidConfig = true;  // wrong mixing mode
				}
			}
			extern void sidEmuResetAutoPanning(int);
			sidEmuResetAutoPanning(config.autoPanning);
		}
	}
	else
	{
		gotInvalidConfig = true;  // invalid panning mode
	}
	
	if (inCfg.emulateFilter != config.emulateFilter)
	{
		config.emulateFilter = (inCfg.emulateFilter == true);
		newSIDconfig = true;      // filter
		newMixerSettings = true;  // amplification
	}
	// Range-check the filter settings.
	if (( inCfg.filterFs >= 1.0 ) && ( inCfg.filterFm != 0 ))
	{
		// Have they changed? Then do the necessary initialization.
		if ((inCfg.filterFs != config.filterFs) 
			|| (inCfg.filterFm != config.filterFm)
			|| (inCfg.filterFt != config.filterFt))
		{
			config.filterFs = inCfg.filterFs;
			config.filterFm = inCfg.filterFm;
			config.filterFt = inCfg.filterFt;
			newFilterInit = true;
		}
	}
	else
	{
		gotInvalidConfig = true;  // invalid settings
	}
	
	if (inCfg.digiPlayerScans != config.digiPlayerScans)
	{
		config.digiPlayerScans = inCfg.digiPlayerScans;
		newMixerSettings = true;  // extra amplification
	}

	if ((config.channels==SIDEMU_MONO) &&
		((config.volumeControl==SIDEMU_STEREOSURROUND)
		 ||(config.autoPanning!=SIDEMU_NONE)))
	{
		gotInvalidConfig = true;  // invalid settings
	}

	if (inCfg.mos8580 != config.mos8580)
	{
		config.mos8580 = (inCfg.mos8580 == true);
		newSIDconfig = true;
	}

	// Here re-initialize the SID, if required.
	if (newSIDconfig)
	{
		configureSID();
	}
	
	// Here re-initialize the mixer engine, if required.
	if (newMixerSettings)
	{
		initMixerEngine();
	}

		// Here re-initialize the filter settings, if required.
	if (newFilterInit)
	{
		filterTableInit();
	}

	// Return flag, whether input config was valid.
	return !gotInvalidConfig;
}

void emuEngine::getConfig( struct emuConfig& outCfg )
{
	outCfg = config;
}

void emuEngine::setDefaultFilterStrength()
{
	config.filterFs = SIDEMU_DEFAULTFILTERFS;
	config.filterFm = SIDEMU_DEFAULTFILTERFM;
	config.filterFt = SIDEMU_DEFAULTFILTERFT;
	filterTableInit();
}

bool emuEngine::verifyEndianess()
{
	// Test endianess. Should swap bytes.
	ubyte endTest[2];
	writeLEword(endTest,0x55aa);
	if (0xaa55!=readBEword(endTest))
		return false;
	else
		return true;
}

#if defined(SIDEMU_TIME_COUNT)
int emuEngine::getSecondsThisSong()
{
	return secondsThisSong;
}

int emuEngine::getSecondsTotal()
{
	return secondsTotal;
}
#endif

// ------------------------------------------------- private member functions

bool emuEngine::freeMem()
{
	if ( ampMod1x8 != 0 )
		delete[] ampMod1x8;
	ampMod1x8 = 0;
	if ( signedPanMix8 != 0 )
		delete[] signedPanMix8;
	signedPanMix8 = 0;
	if ( signedPanMix16 != 0 )
		delete[] signedPanMix16;
	signedPanMix16 = 0;
	return true;
}

bool emuEngine::allocMem()
{
	// Keep track of memory allocation failures.
	bool wasSuccess = true;
	// Seems as if both tables are needed for panning-mixing with 16-bit samples.
	// 8-bit
#ifdef SID_HAVE_EXCEPTIONS
	if (( ampMod1x8 = new(std::nothrow) sbyte[256*256] ) == 0 )
#else
	if (( ampMod1x8 = new sbyte[256*256] ) == 0 )
#endif
		wasSuccess = false;
#ifdef SID_HAVE_EXCEPTIONS
	if (( signedPanMix8 = new(std::nothrow) sbyte[256*256] ) == 0 )
#else
	if (( signedPanMix8 = new sbyte[256*256] ) == 0 )
#endif
		wasSuccess = false;
	// 16-bit
#ifdef SID_HAVE_EXCEPTIONS
	if (( signedPanMix16 = new(std::nothrow) sword[256*256] ) == 0 )
#else
	if (( signedPanMix16 = new sword[256*256] ) == 0 )
#endif
		wasSuccess = false;

	if (!wasSuccess)
	{
		freeMem();
	}
	return wasSuccess;
}

void emuEngine::MPUreset()
{
	if (MPUstatus)
	{
		initInterpreter(config.memoryMode);
		c64memClear();
		c64memReset(config.clockSpeed,randomSeed);
	}
}

ubyte * emuEngine::MPUreturnRAMbase()
{
	if (MPUstatus)
	{
		return c64mem1;
	}
	else
	{
		return 0;
	}
}

void emuEngine::setRandomSeed()
{
	time_t now = time(NULL);
	randomSeed = (ubyte)now;
}

bool emuEngine::reset()
{
	if (isReady)
	{
		// Check internal mixer state whether tables have to be redone.
		if (config.digiPlayerScans != 0)
		{
			if (isThreeVoiceAmplified != isThreeVoiceTune)  // needs diff.ampl.
			{
				initMixerEngine();
			}
		}
		else if (isThreeVoiceAmplified)  // and do not want amplification
		{
			initMixerEngine();
		}
		extern bool sidEmuReset();
		sidEmuReset();
		
		resetSampleEmu();
	}
	return isReady;
}

bool emuEngine::resetSampleEmu()
{
	sampleEmuReset();
	return true;
}

void emuEngine::amplifyThreeVoiceTunes(bool inIsThreeVoiceTune)
{
	// ::initMixerEngine depends on this and config.digiPlayerScans.
	// Specify whether a sidtune uses only three voices (here: no digis).
	isThreeVoiceTune = inIsThreeVoiceTune;
}

// Initialize the SID chip and everything that depends on the frequency.
void emuEngine::configureSID()
{
	extern void sidEmuConfigure(udword PCMfrequency, bool measuredEnveValues, 
								bool isNewSID, bool emulateFilter, int clockSpeed);
	sidEmuConfigure(config.frequency,config.measuredVolume,config.mos8580,
					config.emulateFilter,config.clockSpeed);
}

void emuEngine::initMixerEngine()
{
	uword uk;

	// If just three (instead of four) voices, do different amplification,
	// if that is desired (digiPlayerScans!=0).
	if ((config.digiPlayerScans!=0) && isThreeVoiceTune)
	{
		isThreeVoiceAmplified = true;
	}
	else
	{
		isThreeVoiceAmplified = false;
	}

#if defined(SID_FPUMIXERINIT)
	sdword si, sj;

	// 8-bit volume modulation tables.
	float filterAmpl = 1.0;
	if (config.emulateFilter)
	{
		filterAmpl = 0.7;
	}
	uk = 0;
	for ( si = 0; si < 256; si++ )
	{
		for ( sj = -128; sj < 128; sj++, uk++ )
		{
			ampMod1x8[uk] = (sbyte)(((si*sj)/255)*filterAmpl);
		}
	}

	// Determine single-voice de-amplification.
	float ampDiv;  // logical voices per physical channel
	if ( config.volumeControl == SIDEMU_HWMIXING )
	{
		ampDiv = 1.0;
	}
	else if ((config.channels==SIDEMU_STEREO) &&
			 ((config.volumeControl==SIDEMU_NONE)
			  ||(config.volumeControl==SIDEMU_VOLCONTROL)))
	{
		ampDiv = 2.0;
	}
	else  // SIDEMU_MONO or SIDEMU_FULLPANNING or SIDEMU_STEREOSURROUND
	{
		if (isThreeVoiceAmplified)
		{
			ampDiv = 3.0;
		}
		else
		{
			ampDiv = 4.0;
		}
	}

	uk = 0;
	for ( si = 0; si < 256; si++ )
	{
		for ( sj = -128; sj < 128; sj++, uk++ )
		{
			// 8-bit mixing modulation tables.
			signedPanMix8[uk] = (sbyte)(((si*sj)/255)/ampDiv);
			// 16-bit mixing modulation tables.
			signedPanMix16[uk] = (uword)((si*sj)/ampDiv);
		}
	}
#else
	sdword si, sj;

	// 8-bit volume modulation tables.
	sword filterAmpl = 10;
	if (config.emulateFilter)
	{
		filterAmpl = 7;
	}
	uk = 0;
	for ( si = 0; si < 256; si++ )
	{
		for ( sj = -128; sj < 128; sj++, uk++ )
		{
			ampMod1x8[uk] = (sbyte)((si*sj*filterAmpl)/(255*10));
		}
	}

	// Determine single-voice de-amplification.
	short ampDiv;  // logical voices per physical channel
	if ( config.volumeControl == SIDEMU_HWMIXING )
	{
		ampDiv = 1;
	}
	else if ((config.channels==SIDEMU_STEREO) &&
			 ((config.volumeControl==SIDEMU_NONE)
			  ||(config.volumeControl==SIDEMU_VOLCONTROL)))
	{
		ampDiv = 2;
	}
	else  // SIDEMU_MONO or SIDEMU_FULLPANNING or SIDEMU_STEREOSURROUND
	{
		if (isThreeVoiceAmplified)
		{
			ampDiv = 3;
		}
		else
		{
			ampDiv = 4;
		}
	}

	uk = 0;
	for ( si = 0; si < 256; si++ )
	{
		for ( sj = -128; sj < 128; sj++, uk++ )
		{
			// 8-bit mixing modulation tables.
			signedPanMix8[uk] = (sbyte)((si*sj)/(255*ampDiv));
			// 16-bit mixing modulation tables.
			signedPanMix16[uk] = (uword)((si*sj)/ampDiv);
		}
	}
#endif  

	// ------------------------------------------------- Init mixer function.
	
	extern void* fill8bitMono(void*, udword);
	extern void* fill8bitMonoControl(void*, udword);
	extern void* fill8bitSplit(void*, udword);
	extern void* fill8bitStereo(void*, udword);
	extern void* fill8bitStereoControl(void*, udword);
	extern void* fill8bitStereoSurround(void*, udword);
	extern void* fill16bitMono(void*, udword);
	extern void* fill16bitMonoControl(void*, udword);
	extern void* fill16bitStereo(void*, udword);
	extern void* fill16bitStereoControl(void*, udword);
	extern void* fill16bitStereoSurround(void*, udword);
	extern void* fill16bitSplit(void*, udword);
	
	typedef void* (*ptr2fillfunc)(void*,udword);
	// Fill function lookup-table.
	// bits, mono/stereo, control
	const ptr2fillfunc fillfunctions[2][2][4] =
	{
		{
			{
				&fill8bitMono, &fill8bitSplit, &fill8bitMonoControl, &fill8bitMonoControl
			},
			{
				&fill8bitStereo, &fill8bitSplit, &fill8bitStereoControl, &fill8bitStereoSurround
			},
		},
		{
			{
				&fill16bitMono, &fill16bitSplit, &fill16bitMonoControl, &fill16bitMonoControl
			},
			{
				&fill16bitStereo, &fill16bitSplit, &fill16bitStereoControl, &fill16bitStereoSurround
			},
		}
	};

	int bitsIndex, monoIndex, controlIndex;

	// Define the ``zero'' sample for signed or unsigned samples.
	ubyte zero8bit = 0x80;
	uword zero16bit = 0;
	
	if ( config.bitsPerSample == SIDEMU_16BIT )
	{
		bitsIndex = 1;
		switch( config.sampleFormat )
		{
			// waveform and amplification tables are signed samples, 
			// so adjusting the sign should do the conversion 
		 case SIDEMU_SIGNED_PCM:
			{
				zero16bit = 0;
				break;
			}
		 case SIDEMU_UNSIGNED_PCM:
		 default:
			{
				zero16bit = 0x8000;
				break;
			}
		}
	}
	else  // if ( config.bitsPerSample == SIDEMU_8BIT )
	{
		bitsIndex = 0;
		switch( config.sampleFormat )
		{
			// waveform and amplification tables are signed samples, 
			// so adjusting the sign should do the conversion 
		 case SIDEMU_SIGNED_PCM:
			{
				zero8bit = 0;
				break;
			}
		 case SIDEMU_UNSIGNED_PCM:
		 default:
			{
				zero8bit = 0x80;
				break;
			}
		}
	} // end else config.bitsPerSample
	
	switch( config.channels )
	{
	 case SIDEMU_MONO:
		{
			monoIndex = 0;
			break;
		}
	 case SIDEMU_STEREO:
	 default:
		{
			monoIndex = 1;
			break;
		}
	}
	
	if ( config.volumeControl == SIDEMU_NONE )
		controlIndex = 0;
	else if ( config.volumeControl == SIDEMU_HWMIXING )
		controlIndex = 1;
	else if ( config.volumeControl == SIDEMU_STEREOSURROUND )
		controlIndex = 3;
	else
		controlIndex = 2;

	extern void* (*sidEmuFillFunc)(void*, udword);
	sidEmuFillFunc = fillfunctions[bitsIndex][monoIndex][controlIndex];

	// Call a function which inits more local tables.
	extern void MixerInit(bool threeVoiceAmplify, ubyte zero8, uword zero16);
	MixerInit(isThreeVoiceAmplified,zero8bit,zero16bit);
	
	// ----------------------------------------------------------------------
	
	// ensure that samplebuffer will be divided into
	// correct number of samples
	//  8-bit mono: buflen x = x samples
	//      stereo:          = x/2 samples
	// 16-bit mono: buflen x = x/2 samples
	//      stereo:          = x/4 samples
	extern ubyte bufferScale;
	bufferScale = 0;
	// HWMIXING mode does not know about stereo.
	if ((config.channels == SIDEMU_STEREO)
		&& !(config.volumeControl == SIDEMU_HWMIXING))
		bufferScale++;
	if ( config.bitsPerSample == SIDEMU_16BIT )  
		bufferScale++;
}


void emuEngine::setDefaultVoiceVolumes()
{ 
	// Adjust default mixing gain. Does not matter, whether this will be used.
	// Signed 8-bit samples will be added to base array index. 
	// So middle must be 0x80.
	// [-80,-81,...,-FE,-FF,0,1,...,7E,7F]
	// Mono: left channel only used.
	if ( config.channels == SIDEMU_MONO )
	{
		setVoiceVolume(1,255,0,256);
		setVoiceVolume(2,255,0,256);
		setVoiceVolume(3,255,0,256);
		setVoiceVolume(4,255,0,256);
	}
	// Stereo:
	else
	{ // if ( config.channels == SIDEMU_STEREO )
		if ( config.volumeControl == SIDEMU_STEREOSURROUND )
		{
			setVoiceVolume(1,255,255,256);
			setVoiceVolume(2,255,255,256);
			setVoiceVolume(3,255,255,256);
			setVoiceVolume(4,255,255,256);
		}
		else
		{
			setVoiceVolume(1,255,0,256);
			setVoiceVolume(2,0,255,256);
			setVoiceVolume(3,255,0,256);
			setVoiceVolume(4,0,255,256);
		}
	}
}


void emuEngine::filterTableInit()
{
	uword uk;
	// Parameter calculation has not been moved to a separate function
	// by purpose.
	const float filterRefFreq = 44100.0;
	
	extern filterfloat filterTable[0x800];
	float yMax = 1.0;
	float yMin = 0.01;
	uk = 0;
	for ( float rk = 0; rk < 0x800; rk++ )
	{
		filterTable[uk] = (((exp(rk/0x800*log(config.filterFs))/config.filterFm)+config.filterFt)
			*filterRefFreq) / config.frequency;
		if ( filterTable[uk] < yMin )
			filterTable[uk] = yMin;
		if ( filterTable[uk] > yMax )
			filterTable[uk] = yMax;
		uk++;
	}

	extern filterfloat bandPassParam[0x800];
	yMax = 0.22;
	yMin = 0.05;  // less for some R1/R4 chips
	float yAdd = (yMax-yMin)/2048.0;
	float yTmp = yMin;
	uk = 0;
	// Some C++ compilers still have non-local scope!
	for ( float rk2 = 0; rk2 < 0x800; rk2++ )
	{
		bandPassParam[uk] = (yTmp*filterRefFreq) / config.frequency;
		yTmp += yAdd;
		uk++;
	}
	
	extern filterfloat filterResTable[16];
	float resDyMax = 1.0;
	float resDyMin = 2.0;
	float resDy = resDyMin;
	for ( uk = 0; uk < 16; uk++ )
	{
		filterResTable[uk] = resDy;
		resDy -= (( resDyMin - resDyMax ) / 15 );
	}
	filterResTable[0] = resDyMin;
	filterResTable[15] = resDyMax;
}

bool emuEngine::setVoiceVolume(int voice, ubyte leftLevel, ubyte rightLevel, uword total)
{
	if ( config.volumeControl == SIDEMU_NONE )
		return false;
	if ((voice < 1) || (voice > 4) || (total > 256))
		return false;
	if ( config.channels == SIDEMU_MONO )
		rightLevel = 0;
	extern void sidEmuSetVoiceVolume(int v,uword L,uword R,uword T);
	sidEmuSetVoiceVolume(voice,leftLevel,rightLevel,total);
	return true;
}

uword emuEngine::getVoiceVolume(int voice)
{
	extern uword sidEmuReturnVoiceVolume(int);
	if (( voice < 1 ) || ( voice > 4 ))
		return 0;
	else
		return sidEmuReturnVoiceVolume(voice);
}
