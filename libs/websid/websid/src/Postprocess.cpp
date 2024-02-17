/*
* Add-on audio postprocessing.
*
* WebSid (c) 2023 Jürgen Wothke
* version 0.95
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include <stdio.h>

extern "C" {
#include "stereo/LVCS.h"
}

#include "sid.h"
extern "C" void sidSetPanning(uint8_t sid_idx, uint8_t voice_idx, float panning);


#include "Postprocess.h"


// keep the private stuff private:

static float _panning[] = {	// panning per SID/voice (max 10 SIDs..)
	0.5, 0.4, 0.6,
	0.5, 0.6, 0.4,
	0.5, 0.4, 0.6,
	0.5, 0.6, 0.4,
	0.5, 0.4, 0.6,
	0.5, 0.6, 0.4,
	0.5, 0.4, 0.6,
	0.5, 0.6, 0.4,
	0.5, 0.4, 0.6,
	0.5, 0.6, 0.4,
	};

static float _no_panning[] = {
	0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	};

static int32_t					_effect_level = -1; // stereo disabled by default;	16384=low 32767=LVCS_EFFECT_HIGH
static LVM_UINT16				_reverb_level = 100;
static LVCS_SpeakerType_en		_speaker_type = LVCS_HEADPHONES; // LVCS_EX_HEADPHONES

static LVCS_Handle_t			_lvcs_handle = LVM_NULL;	// just a typecast PTR to be later set to the above instance
static LVCS_MemTab_t			_lvcs_mem_tab;
static LVCS_Capabilities_t		_lvc_caps;
static LVCS_Params_t			_lvcs_params;

static uint32_t					_sample_rate;

static LVM_Fs_en getSampleRateEn(uint32_t sample_rate)
{
	switch (sample_rate) {
		case 8000:
			return LVM_FS_8000;
		case 11025:
			return LVM_FS_11025;
		case 12000:
			return LVM_FS_12000;
		case 16000:
			return LVM_FS_16000;
		case 22050:
			return LVM_FS_22050;
		case 24000:
			return LVM_FS_24000;
		case 32000:
			return LVM_FS_32000;
		case 44100:
			return LVM_FS_44100;
		case 48000:
			return LVM_FS_48000;
		default:
			fprintf(stderr, "warning: samplerate (%u) is not supported by pseudo stereo impl.", sample_rate);
			return LVM_FS_DUMMY;	// higher sample rates should not be used here
	}
}

static void configurePseudoStereo(uint16_t size)
{
	if (!size) return;

	if (_lvcs_handle == LVM_NULL)
	{
		// capabilities used for LVCS_Memory and LVCS_Init must be the same!
		_lvc_caps.MaxBlockSize= size;
		_lvc_caps.CallBack= LVM_NULL;

		if (LVCS_Memory(LVM_NULL, &_lvcs_mem_tab, &_lvc_caps))
		{	// orig code patched to alloc used buffers!
			fprintf(stderr, "error: LVCS_Memory\n");
		}

		if (LVCS_Init(&_lvcs_handle, &_lvcs_mem_tab, &_lvc_caps))
		{
			fprintf(stderr, "error: LVCS_Init\n");
		}

	//	fprintf(stderr, "alloc stereo for size %d\n", size);
	}

	// caution: LVCS_GetParameters is a garbage API: changing the returned "ref to original" will cause any
	// changes to be ignored by LVCS_Control() since there is no difference to the "original". use a
	// separage copy for the input params:

	//#define LVCS_STEREOENHANCESWITCH    0x0001      /* Stereo enhancement enable control */
	//#define LVCS_REVERBSWITCH           0x0002      /* Reverberation enable control */
	//#define LVCS_EQUALISERSWITCH        0x0004      /* Equaliser enable control */
	//#define LVCS_BYPASSMIXSWITCH        0x0008      /* Bypass mixer enable control */
	_lvcs_params.OperatingMode = LVCS_ON;	// LVCS_ON means that all the above 4 bits are set

	_lvcs_params.EffectLevel    = _effect_level < 0 ? 0 : _effect_level;
	_lvcs_params.ReverbLevel    = _reverb_level; // supposedly in %!

	_lvcs_params.SpeakerType = _speaker_type;

	_lvcs_params.SourceFormat = LVCS_STEREO;		// with "per voice panning" input signal is "always" stereo
	_lvcs_params.CompressorMode = LVM_MODE_OFF;
	_lvcs_params.SampleRate = getSampleRateEn(_sample_rate);

	if (LVCS_Control(_lvcs_handle, &_lvcs_params))
	{
		fprintf(stderr, "error: LVCS_Control\n");
	}
}

using namespace websid;

void Postprocess::applyStereoEnhance(int16_t* buffer, uint16_t chunk_size)
{
	uint32_t s;
	if((_effect_level > 0) && (s = LVCS_Process(_lvcs_handle, (const LVM_INT16*)buffer, buffer, chunk_size)))
	{
// fixme: this seems to trigger: 		fprintf(stderr, "error: LVCS_Process %lu %hu\n", s, _chunk_size);
	}
}

void Postprocess::setReverbLevel(int32_t chunk_size, uint16_t reverb_level)
{
	_reverb_level = reverb_level;
	configurePseudoStereo(chunk_size);
}

uint16_t Postprocess::getReverbLevel()
{
	return _reverb_level;
}

static void enablePanning(uint8_t on)
{
	for (int sid_idx = 0; sid_idx < MAX_SIDS; sid_idx++) {
		for (int voice_idx = 0; voice_idx < 3; voice_idx++) {
			sidSetPanning(sid_idx, voice_idx, on ? _panning[sid_idx*3 + voice_idx] : 0.5);
		}
	}
}

void Postprocess::setStereoLevel(int32_t chunk_size, int32_t effect_level)
{
	_effect_level = effect_level;
	configurePseudoStereo(chunk_size);

	enablePanning(_effect_level >= 0);
}

int32_t Postprocess::getStereoLevel()
{
	return _effect_level;
}

uint8_t Postprocess::getHeadphoneMode()
{
	return _speaker_type == LVCS_HEADPHONES ? 0 : 1;
}

void Postprocess::setHeadphoneMode(int32_t chunk_size, uint8_t mode)
{
	_speaker_type = mode ? LVCS_EX_HEADPHONES : LVCS_HEADPHONES;
	configurePseudoStereo(chunk_size);
}

void Postprocess::setPanning(uint8_t sid_idx, uint8_t voice_idx, float panning)
{
	if ((sid_idx < MAX_SIDS) && (voice_idx < 3))
	{
		_panning[sid_idx*3 + voice_idx] = panning;

		enablePanning(_effect_level >= 0);
	}
}

float Postprocess::getPanning(uint8_t sid_idx, uint8_t voice_idx)
{
	if ((sid_idx < MAX_SIDS) && (voice_idx < 3))
		return _panning[sid_idx*3 + voice_idx];
	return -1;
}

void Postprocess::init(int32_t chunk_size, uint32_t sample_rate)
{
	_sample_rate = sample_rate;
	SID::initPanning(_effect_level >= 0 ? _panning : _no_panning);

	configurePseudoStereo(chunk_size);
}

void Postprocess::initPanningCfg(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9,
								float p10, float p11, float p12, float p13, float p14, float p15, float p16, float p17, float p18, float p19,
								float p20, float p21, float p22, float p23, float p24, float p25, float p26, float p27, float p28, float p29) {

	_panning[0] = p0;
	_panning[1] = p1;
	_panning[2] = p2;
	_panning[3] = p3;
	_panning[4] = p4;
	_panning[5] = p5;
	_panning[6] = p6;
	_panning[7] = p7;
	_panning[8] = p8;
	_panning[9] = p9;
	_panning[10] = p10;
	_panning[11] = p11;
	_panning[12] = p12;
	_panning[13] = p13;
	_panning[14] = p14;
	_panning[15] = p15;
	_panning[16] = p16;
	_panning[17] = p17;
	_panning[18] = p18;
	_panning[19] = p19;
	_panning[20] = p20;
	_panning[11] = p21;
	_panning[22] = p22;
	_panning[23] = p23;
	_panning[24] = p24;
	_panning[25] = p25;
	_panning[26] = p26;
	_panning[27] = p27;
	_panning[28] = p28;
	_panning[29] = p29;
}
