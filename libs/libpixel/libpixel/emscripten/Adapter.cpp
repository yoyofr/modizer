/*
* This file adapts "PxTone Collage Collage" & "Organya" to the interface expected by my generic JavaScript player..
*
* Organya seems to be some kind of precursor of PxTone created by Daisuke "Pixel" Amaya.
* I had wrongly assumed that there might be some synergy decoding japaneese song titles to unicode
* but it seems Organya does not have any title info at all.
*
* Copyright (C) 2019-2023 Juergen Wothke
*
* LICENSE the same license used by PxTone is extended to this adapter.
*/

#include <stdio.h>
#include <vector>
#include <string>
#include <stdint.h>

#include <emscripten.h>

#include <pxtnService.h>
#include <pxtnError.h>

#include "MetaInfoHelper.h"
using emsutil::MetaInfoHelper;

// organya
extern "C" void unload_org(void);
extern "C" int org_play(const char *fn, char *buf);
extern "C" int org_getoutputtime(void);
extern "C" int org_currentpos();
extern "C" int org_gensamples();

#define BUF_SIZE	1024	// note: organya only uses 576
#define TEXT_MAX	1024


extern "C" char	*sample_buffer=0;


namespace pixel {
	class Adapter {
	public:
		Adapter() : _pxtn(NULL), _desc(NULL), _organya_mode(false), _fileBuffer(NULL), _fileBufferLen(0), _isReady(false),
					_playTime(0), _totalTime(0), _sampleRate(44100), _channels(2)
		{
		}

		int loadFile(char *filename, void *inBuffer, uint32_t inBufSize, uint32_t sampleRate, uint32_t audioBufSize, uint32_t scopesEnabled)
		{
			// fixme: sampleRate, audioBufSize, scopesEnabled support not implemented

			allocBuffers(inBuffer, inBufSize);

			_sampleRate = _organya_mode ? 44100 : sampleRate;	// keep 44100 for the benefit of organya

			char * point;
			_organya_mode = ((point = strrchr(filename,'.')) != NULL ) && (strcmp(point,".org") == 0);

			bool success = false;
			_pxtn = new pxtnService();
			_desc = new pxtnDescriptor();

			if (_organya_mode)
			{
				success= !org_play((const char *)filename, _fileBuffer);
			}
			else
			{
				// pxtone
				if (_pxtn->init() == pxtnOK)
				{
					if (_pxtn->set_destination_quality(2, _sampleRate))
					{
						if (_desc->set_memory_r(_fileBuffer, inBufSize) && (_pxtn->read(_desc) == pxtnOK) && (_pxtn->tones_ready() == pxtnOK))
						{
							success = true;

							pxtnVOMITPREPARATION prep = {0};
							//prep.flags |= pxtnVOMITPREPFLAG_loop; // don't loop
							prep.start_pos_float = 0;
							prep.master_volume = 1; //(volume / 100.0f);

							if (!_pxtn->moo_preparation(&prep))
							{
								success = false;
							}
						}
						else
						{
							_pxtn->evels->Release();
						}
					}
				}
			}
			return success ? 0 : 1;
		}

		int setSubsong(int subsong)
		{
			// fixme: are there subsongs in any of the formats?

			_playTime = 0;

			if (_pxtn)
			{
				_totalTime = _pxtn->moo_get_total_sample();
			}

			updateTrackInfo();

			_isReady = true;

			return 0;
		}

		void teardown (void) {
			_isReady= false;

			MetaInfoHelper::getInstance()->clear();

			if (_organya_mode)
			{
				if (_pxtn)
				{
					_pxtn->clear();
					delete(_pxtn);
					_pxtn = NULL;

					delete(_desc);
					_desc = NULL;
				}
			}
			else
			{
				unload_org();
			}

			if (sample_buffer) 	{ free(sample_buffer); sample_buffer = NULL; }
			if (_fileBuffer) 	{ free(_fileBuffer); _fileBuffer = NULL; _fileBufferLen = 0; }
		}

		int getSampleRate()
		{
			return _sampleRate;
		}

		char* getSampleBuffer() {
			return (char*)sample_buffer;
		}

		long getSampleBufferLength() {
			return 	_organya_mode ? 576 : BUF_SIZE;		// organya uses hard coded..
		}


		int getCurrentPosition()
		{
			if (_organya_mode)
			{
				return _isReady ? org_currentpos() : -1;
			}
			else
			{
				return _isReady ? _playTime / _sampleRate * 1000 : -1;
			}
		}

		void seekPosition(int ms)
		{
			// fixme: not implemented
		}

		int getMaxPosition()
		{
			if (_organya_mode)
			{
				return _isReady ? org_getoutputtime() : -1;
			}
			else
			{
				return _isReady ? _totalTime / _sampleRate * 1000 : -1;
			}
		}

		int genSamples() {
			if (!_isReady) return 0;		// don't trigger a new "song end" while still initializing

			if (_organya_mode)
			{
				if (!org_gensamples()) return 1;
			}
			else
			{
				if (!_pxtn->Moo(sample_buffer, pxtnBITPERSAMPLE / 8 * _channels * BUF_SIZE))
				{
					return 1; // end
				}
				else
				{
					_playTime += BUF_SIZE;
				}
			}
			return 0;
		}
	private:
		void allocBuffers(void *inBuffer, uint32_t inBufSize)
		{
			if (sample_buffer || _fileBuffer)
			{
				fprintf(stderr, "error: mem leak - buffers have not been freed!\n");
			}

			sample_buffer = (char *)calloc(pxtnBITPERSAMPLE / 8 * _channels * BUF_SIZE, sizeof(char));

			_fileBuffer = (char *)calloc(inBufSize, sizeof(char));
			memcpy(_fileBuffer, inBuffer, inBufSize);
			_fileBufferLen = inBufSize;
		}

		void updateTrackInfo() {
			MetaInfoHelper *info = MetaInfoHelper::getInstance();

			if (!_organya_mode)
			{
				int32_t t_len = 0;
				const char *title= _pxtn->text->get_name_buf(&t_len);
				int32_t c_len = 0;
				const char *comment= _pxtn->text->get_comment_buf(&c_len);

				// iconv based mapping does not seem to wprk in emscripten
				// therefore handling it on the JavaScript side

				info->setText(0, title, "");
				info->setText(1, comment, "");
			}
			else
			{
				info->setText(0, "", "");
				info->setText(1, "", "");
			}
		}
	private:
		pxtnService*	_pxtn;
		pxtnDescriptor*	_desc;

		bool 			_organya_mode;	// pxtone vs organya

		char*			_fileBuffer;
		uint32_t		_fileBufferLen;

		bool			_isReady;

		uint32_t		_playTime;
		uint32_t		_totalTime;

		uint32_t		_sampleRate;
		uint16_t		_channels;
	};
};

pixel::Adapter _adapter;



// old style EMSCRIPTEN C function export to JavaScript.
// todo: code might be cleaned up using EMSCRIPTEN's "new" Embind feature:
// https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html
#define EMBIND(retType, func)  \
	extern "C" retType func __attribute__((noinline)); \
	extern "C" retType EMSCRIPTEN_KEEPALIVE func

// --- standard functions
EMBIND(int, emu_load_file(char *filename, void *inBuffer, uint32_t inBufSize, uint32_t sampleRate, uint32_t audioBufSize, uint32_t scopesEnabled)) {
	return _adapter.loadFile(filename, inBuffer, inBufSize, sampleRate, audioBufSize, scopesEnabled); }
EMBIND(void, emu_teardown())						{ _adapter.teardown(); }
EMBIND(int, emu_get_sample_rate())					{ return _adapter.getSampleRate(); }
EMBIND(int, emu_set_subsong(int subsong))			{ return _adapter.setSubsong(subsong); }
EMBIND(const char**, emu_get_track_info())			{ return MetaInfoHelper::getInstance()->getMeta(); }
EMBIND(int, emu_compute_audio_samples())			{ return _adapter.genSamples(); }
EMBIND(char*, emu_get_audio_buffer())				{ return _adapter.getSampleBuffer(); }
EMBIND(int, emu_get_audio_buffer_length())			{ return _adapter.getSampleBufferLength(); }
EMBIND(int, emu_get_current_position())				{ return _adapter.getCurrentPosition(); }
EMBIND(void, emu_seek_position(int ms))				{ _adapter.seekPosition(ms);  }
EMBIND(int, emu_get_max_position())					{ return _adapter.getMaxPosition(); }

