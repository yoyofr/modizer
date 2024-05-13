/*
* This file adapts "ZXTune" to the interface expected by my generic JavaScript player..
*
* Copyright (C) 2015-2023 Juergen Wothke
*
* LICENSE
*
* This library is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2.1 of the License, or (at
* your option) any later version. This library is distributed in the hope
* that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/

#include <stdio.h>
#include <contract.h>
#include <pointers.h>
#include <types.h>
#include <stdlib.h>     /* malloc, free, rand */
#include <exception>
#include <iostream>
#include <fstream>

#include <emscripten.h>

#include "MetaInfoHelper.h"
using emsutil::MetaInfoHelper;

#include "Spectre.h"

// for some reason my old Emscripten version DOES NOT provide std:exception
// with its stdlib... if you are building with a better version then disable this
// BLOODY_HACK
#ifdef BLOODY_HACK
namespace std
{
	exception::~exception() _NOEXCEPT
	{
	}

	const char* exception::what() const _NOEXCEPT
	{
	  return "std::exception";
	}
}
#endif


// see Sound::Sample::CHANNELS
#define CHANNELS 2
#define BYTES_PER_SAMPLE 2


namespace zxtune {
	class Adapter {
	public:
		Adapter() : _zxtune(0), _bufferSize(0), _sampleBuffer(NULL), _samplesAvailable(0)
		{
		}

		int loadFile(char *filename, void *inBuffer, uint32_t inBufSize, uint32_t sampleRate, uint32_t audioBufSize, unsigned char scopesEnabled)
		{
			// fixme: scopesEnabled support not implemented

			allocBuffers(audioBufSize);

			try	{
				Dump dump;
				readFile(std::string(filename), dump);

				createZxTune(filename, dump, sampleRate);
			}
			catch (const std::exception&) {
				std::cerr << "fatal error: "<< std::endl;
				return -1;
			}
			return 0;
		}

		int setSubsong(int track)
		{
			track += 1;	// JavaScript uses 0..n but zxTune 1..n+1

			_songinfo.reset();
			_zxtune->get_song_info(track, _songinfo);
			_zxtune->decodeInitialize(track, _songinfo);

			updateTrackInfo();
			return 0;
		}

		void teardown()
		{
			MetaInfoHelper::getInstance()->clear();

			if (_zxtune != 0)
			{
				delete _zxtune;
				_zxtune = 0;
			}
		}

		int getSampleRate()
		{
			return _zxtune->getSampleRate();
		}

		int genSamples()
		{
			_samplesAvailable = _zxtune->render_sound((char*)_sampleBuffer, _bufferSize);

			return _samplesAvailable ? 0 : 1;	// currently no -1
		}

		char* getSampleBuffer()
		{
			return (char*)_sampleBuffer;
		}

		long getSampleBufferLength()
		{
			return _samplesAvailable;
		}

		int getCurrentPosition()
		{
			return _zxtune->get_current_position();
		}

		void seekPosition(int ms)
		{
			_zxtune->seek_position(ms);
		}

		int getMaxPosition()
		{
			return _zxtune->get_max_position();
		}

	private:
		void allocBuffers(int size)
		{
			if (size > _bufferSize)
			{
				if (_sampleBuffer) free(_sampleBuffer);

				_sampleBuffer = (uint32_t*)malloc((size * BYTES_PER_SAMPLE * CHANNELS)+1);
				_bufferSize = size;
			}
		}

		void createZxTune(char *songmodule, Dump &dump, int sampleRate)
		{
			if (_zxtune != 0) delete _zxtune;

			_zxtune = new ZxTuneWrapper(std::string(songmodule),  &dump.front(), dump.size(), sampleRate);
			_zxtune->parseModules();
		}

		void readFile(const std::string& name, Dump& result)
		{
			std::ifstream stream(name.c_str(), std::ios::binary);
			if (!stream)
			{
				throw std::runtime_error("Failed to open " + name);
			}
			stream.seekg(0, std::ios_base::end);
			const std::size_t size = stream.tellg();
			stream.seekg(0);

			Dump tmp(size);
			stream.read(safe_ptr_cast<char*>(&tmp[0]), tmp.size());
			result.swap(tmp);
		}

		void updateTrackInfo()
		{
			MetaInfoHelper *info = MetaInfoHelper::getInstance();

			info->setText(0, _songinfo.get_title(), "");
			info->setText(1, _songinfo.get_author(), "");
			info->setText(2, _songinfo.get_comment(), "");
			info->setText(3, _songinfo.get_program(), "");
			info->setText(4, _songinfo.get_subpath(), "");
			info->setText(5, _songinfo.get_total_tracks(), "");
			info->setText(6, _songinfo.get_track_number(), "");
		}

	private:
		ZxTuneWrapper* _zxtune;
		SongInfo _songinfo;

		int _bufferSize;

		uint32_t *_sampleBuffer;	// zxtune expects uint32_t-array - proper alignment is crucial!
		int _samplesAvailable;
	};
};

zxtune::Adapter _adapter;


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
EMBIND(int, emu_set_subsong(int track))				{ return _adapter.setSubsong(track); }
EMBIND(const char**, emu_get_track_info())			{ return MetaInfoHelper::getInstance()->getMeta(); }
EMBIND(int, emu_compute_audio_samples())			{ return _adapter.genSamples(); }
EMBIND(char*, emu_get_audio_buffer())				{ return _adapter.getSampleBuffer(); }
EMBIND(int, emu_get_audio_buffer_length())			{ return _adapter.getSampleBufferLength(); }
EMBIND(int, emu_get_current_position())				{ return _adapter.getCurrentPosition(); }
EMBIND(void, emu_seek_position(int ms))			{ _adapter.seekPosition(ms);  }
EMBIND(int, emu_get_max_position())					{ return _adapter.getMaxPosition(); }
