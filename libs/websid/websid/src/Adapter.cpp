/*
* This file provides the interface to the JavaScript world (i.e. it
* provides all the APIs expected by the "backend adapter").
*
* It also handles the output audio buffer logic.
*
*
* The general naming conventions used in this project are:
*
*  - type names use camelcase & start uppercase
*  - method/function names use camelcase & start lowercase
*  - vars use lowercase and _ delimiters
*  - private vars (instance vars, etc) start with _
*  - global constants and defines are all uppercase
*  - cross C-file APIs (for those older parts that still use C)
*    start with a prefix that reflects the file that provides them,
*    e.g. "vic...()" is provided by "vic.c".
*
* WebSid (c) 2019-2023 Jürgen Wothke
* version 0.96
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
// see RaspiWebSid
#define EMSCRIPTEN_KEEPALIVE
#endif

extern "C" {
#include "system.h"
#include "vic.h"
#include "core.h"
#include "memory.h"
}
#include "filter6581.h"
#include "sid.h"
extern "C" void 	sidWriteMem(uint16_t addr, uint8_t value);

#include "Postprocess.h"
using websid::Postprocess;

#include "SidSnapshot.h"
using websid::SidSnapshot;

#include "loaders.h"

static FileLoader*	_loader;


#define CHANNELS			2
#define MAX_VOICES 			(MAX_SIDS*4)
#define MAX_SCOPE_BUFFERS 	(MAX_SIDS*4)


namespace websid {

	class Adapter {
	public:
		Adapter() : _proc_buf_size(0), _sound_buffer(NULL), _scope_buffers(NULL), _available_samples(0),
					_scopes_enabled(false), _ready_to_play(false)
		{
		}

		int loadFile(char *filename, uint8_t* in_buffer, uint32_t in_buf_size, uint32_t sample_rate, uint32_t audio_buf_size, uint32_t scopes_enabled,
					void* basic_ROM, void* char_ROM, void* kernal_ROM, int enable_MD5)
		{
			_ready_to_play = false;

			_scopes_enabled = scopes_enabled;

			// what webaudio is using (fixme: WebSid uses a different output granularity - see _chunk_size)
			_proc_buf_size = audio_buf_size;

			// see "pseudo stereo" limitation
			_sample_rate = sample_rate > 48000 ? 48000 : sample_rate;

			// Compute! Sidplayer file (stereo files not supported)
			uint32_t is_mus = endsWith(filename, ".mus") || endsWith(filename, ".str");

			_loader = FileLoader::getInstance(is_mus, (void*)in_buffer, in_buf_size);

			if (!_loader) return 1;	// error

			uint32_t result = _loader->load(in_buffer, in_buf_size, filename, basic_ROM, char_ROM, kernal_ROM, enable_MD5);

			if (!result)
			{
				setNTSC(FileLoader::getNTSCMode());
			}
			return result;
		}

		int setSubsong(uint32_t track)
		{
			_ready_to_play = false;
			_sound_started = 0;

			// testcase: Baroque_Music_64_BASIC (takes 100sec before playing - without this speedup)
			_skip_silence_loop = 10;

			// fixme: the separate handling of the INIT call is an annoying legacy
			// of the old PSID impl.. respective PSID handling should better be moved into
			// the C64 side driver so that users of the emulator do not need
			// to handle this potentially long running emu scenario (see SID callbacks
			// triggered on Raspberry SID device).
			_loader->initTune(_sample_rate, track);

			Postprocess::init(_chunk_size, _sample_rate);

			reset();

			_ready_to_play = true;

			return 0;
		}

		uint32_t getSampleBufferLength()
		{
			return _available_samples;
		}

		char* getSampleBuffer()
		{
			return (char*) _sound_buffer;
		}

		uint32_t getSampleRate()
		{
			return _sample_rate;
		}

		int getNumberTraceStreams()
		{
			return SID::getNumberUsedChips() * 4;
		}

		const char** getTraceStreams()
		{
			return (const char**)_scope_buffers;
		}

		int32_t genSamples()
		{
			if(!_ready_to_play) return 0;

		#ifdef TEST
			return 0;
		#endif

			_available_samples = genSampleChunk();

			SidSnapshot::record();

			if (_loader->isTrackEnd())  // "play" must have been called before 1st use of this check
			{
				return -1;
			}

			return _available_samples ? 0 : 1;
		}

		void setNTSC(uint8_t is_ntsc)
		{
			Core::resetTimings(_sample_rate, is_ntsc);
			reset();
		}

		void setStereoLevel(int32_t effect_level)
		{
			Postprocess::setStereoLevel(_chunk_size, effect_level);
		}

		void setReverbLevel(uint16_t reverb_level)
		{
			Postprocess::setReverbLevel(_chunk_size, reverb_level);
		}

		void setHeadphoneMode(uint8_t mode)
		{
			Postprocess::setHeadphoneMode(_chunk_size, mode);
		}
	private:
		bool endsWith(const char* s1, const char* s2)
		{
			std::string fullString(s1);
			std::string ending(s2);

			if (fullString.length() >= ending.length())
			{
				return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
			}
			else
			{
				return false;
			}
		}

		uint32_t genSampleChunk()
		{
			// fixme: the "runOneFrame" granularity is much larger than what would be needed
			// in a AudioWorklet context (i.e. 128). replace with an API that supports arbitrary
			// chunk sizes

			uint8_t is_simple_sid_mode = !FileLoader::isExtendedSidFile();
			uint8_t speed =	FileLoader::getCurrentSongSpeed();

			// limit "skipping" so as not to make the browser unresponsive
			for (uint16_t i = 0; i < _skip_silence_loop; i++)
			{
				Core::runOneFrame(is_simple_sid_mode, speed, _sound_buffer,
									_scope_buffers, _chunk_size);

				if (_sound_started || SID::isAudible())
				{
					_sound_started = 1;
					break;
				}
			}
			Postprocess::applyStereoEnhance(_sound_buffer, _chunk_size);
			return _chunk_size;
		}

		void resetSoundBuffer(uint16_t size)
		{
			if (_sound_buffer) free(_sound_buffer);

			_sound_buffer = (int16_t*)malloc(sizeof(int16_t)*
								(size * CHANNELS + 1));
		}

		void freeScopeBuffers()
		{
			// trace output (corresponding to _sound_buffer)
			if (_scope_buffers)
			{
				for (int i= 0; i<MAX_VOICES; i++) {
					if (_scope_buffers[i])
					{
						free(_scope_buffers[i]);
						_scope_buffers[i] = 0;
					}
				}
				free(_scope_buffers);
				_scope_buffers = 0;
			}
		}

		void allocScopeBuffers(uint16_t size)
		{
			// availability of _scope_buffers controls
			// if SID will generate the respective output

			if (_scopes_enabled)
			{
				if (!_scope_buffers)
				{
					_scope_buffers = (int16_t**)calloc(sizeof(int16_t*), MAX_VOICES);
				}
				for (int i= 0; i<MAX_VOICES; i++) {
					_scope_buffers[i] = (int16_t*)calloc(sizeof(int16_t), size + 1);
				}
			}
			else
			{
				if (_scope_buffers)
				{
					for (int i= 0; i<MAX_VOICES; i++) {
						if (_scope_buffers[i]) {
							free(_scope_buffers[i]);
							_scope_buffers[i] = 0;
						}
					}
					free(_scope_buffers);
					_scope_buffers = 0; // disables respective SID rendering
				}
			}
		}

		void resetScopeBuffers(uint16_t size)
		{
			freeScopeBuffers();
			allocScopeBuffers(size);
		}

		void reset()
		{
			// number of samples corresponding to one simulated frame/
			// screen (granularity of emulation is 1 screen), e.g.
			// NTSC: 735*60=44100		800*60=48000
			// PAL: 882*50=44100		960*50=48000

			_chunk_size = _sample_rate / vicFramesPerSecond();

			resetSoundBuffer(_chunk_size);
			resetScopeBuffers(_chunk_size);

			_available_samples = 0;

			SidSnapshot::init(_chunk_size, _proc_buf_size);
		}
	private:
		uint32_t	_proc_buf_size;	// WebAudio side processor buffer size

		// these buffers are "per frame" i.e. 1 screen refresh, e.g. 822 samples
		uint16_t 	_chunk_size;

		int16_t* 	_sound_buffer;
		int16_t** 	_scope_buffers;

		uint32_t 	_available_samples;

		uint8_t	 	_sound_started;
		uint8_t	 	_skip_silence_loop;

		uint32_t	_sample_rate;

		bool		_scopes_enabled;
		bool		_ready_to_play;
	};
};

websid::Adapter _adapter;


// old style EMSCRIPTEN C function export to JavaScript.
// todo: code might be cleaned up using EMSCRIPTEN's "new" Embind feature:
// https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html
#define EMBIND(retType, func)  \
	extern "C" retType func __attribute__((noinline)); \
	extern "C" retType EMSCRIPTEN_KEEPALIVE func

// --- standard

EMBIND(int, emu_load_file(char *filename, uint8_t* in_buffer, uint32_t in_buf_size, uint32_t sample_rate, uint32_t audio_buf_size, uint32_t scopes_enabled, void* basic_ROM, void* char_ROM, void* kernal_ROM, int enable_MD5))
	{ return _adapter.loadFile(filename, in_buffer, in_buf_size, sample_rate, audio_buf_size, scopes_enabled, basic_ROM, char_ROM, kernal_ROM, enable_MD5); }
EMBIND(void, emu_teardown())						{ }
EMBIND(int, emu_get_sample_rate())					{ return _adapter.getSampleRate(); }
EMBIND(int, emu_set_subsong(int track))				{ return _adapter.setSubsong(track); }
EMBIND(const char**, emu_get_track_info())			{ return FileLoader::getInfoStrings(); }
EMBIND(int, emu_compute_audio_samples())			{ return _adapter.genSamples(); }
EMBIND(char*, emu_get_audio_buffer())				{ return _adapter.getSampleBuffer(); }
EMBIND(int, emu_get_audio_buffer_length())			{ return _adapter.getSampleBufferLength(); }
EMBIND(int, emu_get_current_position())				{ return 0; }
EMBIND(void, emu_seek_position(int ms))				{ /* not supported */}
EMBIND(int, emu_get_max_position())					{ return 0; }
EMBIND(int, emu_number_trace_streams())				{ return _adapter.getNumberTraceStreams(); }
EMBIND(const char**, emu_get_trace_streams())		{ return _adapter.getTraceStreams(); }


// --- add-ons

	// non-SID audio postprocessing effects
EMBIND(void, emu_set_panning_cfg(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12, float p13, float p14, float p15, float p16, float p17, float p18, float p19, float p20, float p21, float p22, float p23, float p24, float p25, float p26, float p27, float p28, float p29))
	{ Postprocess::initPanningCfg(p0, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, p21, p22, p23, p24, p25, p26, p27, p28, p29); }
EMBIND(float, emu_get_panning(uint8_t sid_idx, uint8_t voice_idx))					{ return Postprocess::getPanning(sid_idx, voice_idx); }
EMBIND(void, emu_set_panning(uint8_t sid_idx, uint8_t voice_idx, float panning))	{ Postprocess::setPanning(sid_idx, voice_idx, panning); }
EMBIND(int32_t, emu_get_stereo_level())												{ return Postprocess::getStereoLevel(); }
EMBIND(void, emu_set_stereo_level(int32_t effect_level))							{ _adapter.setStereoLevel(effect_level); }
EMBIND(uint16_t, emu_get_reverb())													{ return Postprocess::getReverbLevel(); }
EMBIND(void, emu_set_reverb(uint16_t reverb_level))									{ _adapter.setReverbLevel(reverb_level);}
EMBIND(uint8_t, emu_get_headphone_mode())											{ return Postprocess::getHeadphoneMode(); }
EMBIND(void, emu_set_headphone_mode(uint8_t mode))									{ _adapter.setHeadphoneMode(mode); }

	// access to "historic" SID register data
EMBIND(uint16_t, emu_get_sid_register(uint8_t sid_idx, uint16_t reg, uint8_t buf_idx, uint32_t tick))
	{ return SidSnapshot::getRegister(sid_idx, reg, buf_idx, tick); }
EMBIND(uint16_t, emu_read_voice_level(uint8_t sid_idx, uint8_t voice_idx, uint8_t buf_idx, uint32_t tick))
	{ return SidSnapshot::readVoiceLevel(sid_idx, voice_idx, buf_idx, tick); }

	// manipulation of/access to emulator state
EMBIND(void, emu_set_sid_register(uint8_t sid_idx, uint16_t reg, uint8_t value))
															{ sidWriteMem(SID::getSIDBaseAddr(sid_idx) + reg, value); }
EMBIND(uint8_t, emu_is_6581())								{ return (uint8_t)SID::isSID6581(); }
EMBIND(uint8_t, emu_set_6581(uint8_t is6581))				{ return SID::setSID6581((bool)is6581); }
EMBIND(uint8_t, emu_is_ntsc())								{ return FileLoader::getNTSCMode(); }
EMBIND(void, emu_set_ntsc(uint8_t is_ntsc))					{ return _adapter.setNTSC(is_ntsc); }
EMBIND(void, emu_enable_voice(uint8_t sid_idx, uint8_t voice, uint8_t on))		{ SID::setMute(sid_idx, voice, !on); }
EMBIND(int, emu_sid_count())								{ return SID::getNumberUsedChips(); }
EMBIND(int, emu_get_sid_base(uint8_t sid_idx))				{ return SID::getSIDBaseAddr(sid_idx); }
EMBIND(uint16_t, emu_read_ram(uint16_t addr))				{ return memReadRAM(addr); }
EMBIND(void, emu_write_ram(uint16_t addr, uint8_t value))	{ memWriteRAM(addr, value); }

	// digi playback status information
EMBIND(uint8_t, emu_get_digi_type())						{ return SID::getGlobalDigiType(); }
EMBIND(const char*, emu_get_digi_desc())					{ return SID::getGlobalDigiTypeDesc(); }
EMBIND(uint16_t, emu_get_digi_rate())						{ return SID::getGlobalDigiRate() * vicFramesPerSecond(); }

	// 6581 SID filter configuration
EMBIND(int, emu_set_filter_config_6581(double base, double max, double steepness, double x_offset, double distort, double distort_offset, double distort_scale, double distort_threshold, double kink))
	{ return Filter6581::setFilterConfig6581(base, max, steepness, x_offset, distort, distort_offset, distort_scale, distort_threshold, kink); }
EMBIND(double*, emu_get_filter_cfg_6581())					{ return Filter6581::getFilterConfig6581(); }
EMBIND(double*, emu_get_filter_cutoff_6581(int slice))		{ return Filter6581::getCutoff6581(slice); }

