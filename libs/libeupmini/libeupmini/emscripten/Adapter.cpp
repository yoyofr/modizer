/*
* This file adapts "eupmini" and exports the relevant API to JavaScript, e.g. for
* use in my generic JavaScript player.
*
* Copyright (C) 2023 Juergen Wothke
*
*
* Reminder: The original player uses a variable size audio output buffer (size depending on
* song's "tempo"). And sample rate is hardcoded (see audioout.hpp).
*
*
* Credits:
*
* Based on: https://github.com/gzaffin/eupmini
* Copyright 1995-1997, 2000 Tomoaki Hayasaka. Win32 porting 2002, 2003 IIJIMA Hiromitsu aka Delmonta, and anonymous K. 2018 Giangiacomo Zaffini
*
*
* License: GPL2.0 (inherited from base project)
*/

#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>

#include "eupplayer.hpp"
#include "eupplayer_townsEmulator.hpp"

#include "MetaInfoHelper.h"

using emsutil::MetaInfoHelper;


#define RAW_INFO_MAX	0x40

extern "C" int ems_request_file(const char *filename); // must be implemented on JavaScript side (also see callback.js)

// that hardcoded PCM dependency is such a dumbfuckery..
struct pcm_struct pcm;

namespace eup {

	using std::string;

	class Adapter {
	public:
		Adapter() : _samplesAvailable(0), _maxPlayLength(-1), _currentPlayLength(0), _initialized(false), _eupbuf(NULL), _player(NULL)
		{
		}

		~Adapter()
		{
			trashEmu();
		}

		void trashEmu()
		{
			if (_player) delete _player;
			if (_device) delete _device;

			_player= NULL;
			_device = NULL;
		}

		void initEmu()
		{
			_device = new EUP_TownsEmulator;
			_player = new EUPPlayer();
			_device->rate(streamAudioRate); /* Hz */

			_player->outputDevice(_device);


			// fucking global var garbage!
		    memset(&pcm, 0, sizeof(pcm));
			pcm.on = true;
			pcm.read_pos = streamAudioBufferSamples;	// mark all as read
		}

		void teardown()
		{
			MetaInfoHelper::getInstance()->clear();

			if (_initialized) {
				if (_player) {
					_player->stopPlaying();
					trashEmu();
				}
				_initialized= false;
			}
		}

		void sanitizeName(u_char *filename)
		{
			for (int i = 7; i>0; i--) {
				if (filename[i] == 0x20) {	// crop trailing spaces
					filename[i] = 0;
				} else {
					break;
				}
			}
		}

		int loadFile(char *filename, void *inBuffer, uint32_t inBufSize, uint32_t sampleRate, uint32_t audioBufSize, uint32_t scopesEnabled)
		{
			// fixme: sampleRate, audioBufSize, scopesEnabled support not implemented

			if ((inBuffer == NULL) || (inBufSize <= 0)) return 1;	// error

			initEmu();	// seems emu instance cannot be reused..

			if (_eupbuf) free(_eupbuf);

			_eupbuf= (u_char *)malloc(inBufSize);
			memcpy(_eupbuf, inBuffer, inBufSize);

			int nlen = strlen(filename);
			if (nlen < 4 || strcasecmp(".eup", &(filename[nlen-4]))) return 1;	// only load "eup" files

			bool isFilenameUpper= (filename[nlen-3] == 'E');

			string fn(filename);
			string eupDir(fn.substr(0, fn.rfind("/") + 1));

			// arrrrggh: tempo controls the used "timestep" and
			// thereby the size of the used audiobuffer
			_player->tempo(_eupbuf[2048 + 5] + 30);

			for (int n = 0; n < 32; n++) {
				_player->mapTrack_toChannel(n, _eupbuf[0x394 + n]);
			}

			// FM/PCM may be arbitrarily mapped to the available 14 slots
			for (int n = 0; n < 6; n++) {
				_device->assignFmDeviceToChannel(_eupbuf[0x6d4 + n], n);
			}
			for (int n = 0; n < 8; n++) {
				_device->assignPcmDeviceToChannel(_eupbuf[0x6da + n]);
			}


			{
				u_char instrument[] = {
					' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // name
					// one setting per operator:
					17, 33, 10, 17,                         // detune / multiple
					25, 10, 57, 0,                          // output level
					154, 152, 218, 216,                     // key scale / attack rate
					15, 12, 7, 12,                          // amon / decay rate
					0, 5, 3, 5,                             // sustain rate
					38, 40, 70, 40,                         // sustain level / release rate

					20,                                     // feedback / algorithm
					0xc0,                                   // pan, LFO
					0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				};
				for (int n = 0; n < 128; n++) {
					_device->setFmInstrumentParameter(n, instrument);	// data is copied into global buffer
				}
			}

			{
				sanitizeName(_eupbuf + 0x6e2);

				char fn0[16];
				memcpy(fn0, _eupbuf + 0x6e2, 8);
				fn0[8] = '\0';

				if (strlen(fn0)) {
					string fn1(eupDir + string(fn0) + (isFilenameUpper ? ".FMB" : ".fmb"));

					if (ems_request_file(fn1.c_str()) < 0)	return -1;	// retry later (handle async loading)

					FILE *f = fopen(fn1.c_str(), "rb");
					if (f != NULL) {

						struct stat statbuf1;
						fstat(fileno(f), &statbuf1);
						u_char buf1[statbuf1.st_size];

						fread(buf1, statbuf1.st_size, 1, f);
						fclose(f);

						if ((statbuf1.st_size - 8) / 48 > 128) {
							fprintf(stderr, "error: illegal instrument idx: %d %d\n", (statbuf1.st_size - 8) / 48, statbuf1.st_size);
						}

						for (int n = 0; n < (statbuf1.st_size - 8) / 48; n++) {
							_device->setFmInstrumentParameter(n, buf1 + 8 + 48 * n);
						}
					}
					else {
						fprintf(stderr, "error: FMB file not found\n");
					}
				}
			}

			{
				sanitizeName(_eupbuf + 0x6ea);

				char fn0[16];
				memcpy(fn0, _eupbuf + 0x6ea, 8);
				fn0[8] = '\0';

				if (strlen(fn0)) {
					string fn1(eupDir + string(fn0) + (isFilenameUpper ? ".PMB" : ".pmb"));

					if (ems_request_file(fn1.c_str()) < 0)	return -1;	// retry later  (handle async loading)

					FILE *f = fopen(fn1.c_str(), "rb");
					if (f != NULL) {
						struct stat statbuf1;
						fstat(fileno(f), &statbuf1);
						u_char buf1[statbuf1.st_size];
						fread(buf1, statbuf1.st_size, 1, f);
						fclose(f);
						_device->setPcmInstrumentParameters(buf1, statbuf1.st_size);
					}
					else {
						fprintf(stderr, "error: PMB file not found\n");
					}
				}
			}

			_player->startPlaying(_eupbuf + 2048 + 6);

			setSongInfo(filename);

			_initialized = true;
			return 0;
		}

		void setInfo(int idx, const char *in, int len)
		{
			MetaInfoHelper *info = MetaInfoHelper::getInstance();

			if (len > RAW_INFO_MAX-1) len = RAW_INFO_MAX-1;

			strncpy(_tmpTxtBuffer, (const char*)in, len);
			_tmpTxtBuffer[len] = 0;

			info->setText(idx, _tmpTxtBuffer, "");
		}

		void setSongInfo(const char *filename)
		{
			// EUP file  seems to start with 2 strings: at offset $0 and $20
			// in some songs $0 is empty while $20 contains the file name

			const char *txt= (const char*)_eupbuf;

			int len = strlen(txt);
			if (!len) {
				string fn(filename);
				string name(fn.substr(fn.rfind("/") + 1));

				txt = name.c_str();
				len = strlen(txt);

				setInfo(0, txt, len);
			}
			else {
				setInfo(0, txt, len);
			}

			txt= (const char*)&_eupbuf[0x20];
			len = strlen(txt);
			setInfo(1, txt, len);
		}

		int genSamples()
		{
			// note: the garbage euplayer implementation directly accesses global "pcm"
			// var (pcm.read_pos, pcm.write_pos)
			if (!_initialized) return 0;

			if (_player->isPlaying()) {
				_player->nextTick();

				_samplesAvailable = pcm.write_pos;
				_currentPlayLength += ((double)_samplesAvailable) / streamAudioRate;
			}
			else {
				_initialized = false;
				return 1; 	// end song
			}


			return 0;
		}

		char* getSampleBuffer()
		{
			return (char*)pcm.buffer;
		}

		int getSampleBufferLength()
		{
			return _samplesAvailable;
		}

		int getCurrentPosition()
		{
			return _currentPlayLength * 1000;
		}

		int getMaxPosition()
		{
			return _maxPlayLength * 1000;
		}

	private:
		int _samplesAvailable;
		int _maxPlayLength;
		double _currentPlayLength;

		bool _initialized;
		u_char *_eupbuf;

		EUP_TownsEmulator *_device;
		EUPPlayer *_player;

		char _tmpTxtBuffer[RAW_INFO_MAX];
	};

};

eup::Adapter _adapter;


// old style EMSCRIPTEN C function export to JavaScript.
// todo: code might be cleaned up using EMSCRIPTEN's "new" Embind feature:
// https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html
#define EMBIND(retType, func)  \
	extern "C" retType func __attribute__((noinline)); \
	extern "C" retType EMSCRIPTEN_KEEPALIVE func


EMBIND(void, emu_teardown())						{ _adapter.teardown(); }
EMBIND(int, emu_load_file(char *filename, void *inBuffer, uint32_t inBufSize, uint32_t sampleRate, uint32_t audioBufSize, uint32_t scopesEnabled)) {
	return _adapter.loadFile(filename, inBuffer, inBufSize, sampleRate, audioBufSize, scopesEnabled); }
EMBIND(int, emu_get_sample_rate())					{ return streamAudioRate;}
EMBIND(int, emu_set_subsong(int track))				{ return 0;	/*there are no subsongs*/ }
EMBIND(const char**, emu_get_track_info())			{ return MetaInfoHelper::getInstance()->getMeta(); }
EMBIND(char*, emu_get_audio_buffer())				{ return _adapter.getSampleBuffer(); }
EMBIND(int, emu_get_audio_buffer_length())			{ return _adapter.getSampleBufferLength(); }
EMBIND(int, emu_compute_audio_samples())			{ return _adapter.genSamples(); }
EMBIND(int, emu_get_current_position())				{ return _adapter.getCurrentPosition(); }
EMBIND(void, emu_seek_position(int frames))			{ }
EMBIND(int, emu_get_max_position())					{ return _adapter.getMaxPosition(); }
