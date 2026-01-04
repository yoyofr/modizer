/*
* This file adapts nezplay/NETplug++ to the interface expected by my generic JavaScript player..
*
* Copyright (C) 2018-2023 Juergen Wothke
*
* With added MoonBlaster support.
*
* Credits:   NEZplug++ & mbm2kss by Mamiya (and nezplay by BouKiCHi)
*
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


/*
	where to find test files:

	.sgc (SGC) https://www.zophar.net/music/sega-game-gear-sgc/ (some files use some vgm format that doesn't work here)
	.kss (KSSX, KSCC, ) https://www.zophar.net/music/kss
	.hes (HESM) https://ftp.modland.com/pub/modules/HES/
	.nsf (NESM) https://ftp.modland.com/pub/modules/Nintendo%20Sound%20Format/
	.ay, cpc.emul (ZXAY) https://zxart.ee/eng/music/ , https://ftp.modland.com/pub/modules/AY%20Emul/
	.gbr(GBRF, GBS) https://ftp.modland.com/pub/modules/Gameboy%20Sound%20System%20GBR/
	(NESL)
*/

#include <emscripten.h>
#include <stdio.h>
#include <stdint.h>

#include <iostream>
#include <fstream>

#include <nezplug.h>
#include <audiosys.h>
#include <songinfo.h>

#include "MetaInfoHelper.h"

using emsutil::MetaInfoHelper;

// MBB2KSS converter
extern "C" void mbm2kss_init();
extern "C" int mbm2kss(char *mbmpath, int devtype, int isntsc);
extern "C" char *mbm2kss_title(uint8_t *data, uint32_t size);
extern "C" void mbm2kss_result(uint8_t **buffer, uint32_t *length);
extern "C" int mbm2kss_length();

// NEZ stuff
extern int NSF_noise_random_reset;
extern int NSF_2A03Type;
extern int Namco106_Realmode;
extern int Namco106_Volume;
extern int GBAMode;
extern int MSXPSGType;
extern int MSXPSGVolume;
extern int FDS_RealMode;
extern int LowPassFilterLevel;
extern int NESAPUVolume;
extern int NESRealDAC;
extern int Always_stereo;

extern "C" uint8_t* nez_get_RAM_buffer(NEZ_PLAY *pNezPlay);


#define CHANNELS 2
#define BYTES_PER_SAMPLE 2
#define SAMPLE_BUF_SIZE	1024


#define SNAPSHOT 0x10000

namespace nez {
	using std::string;

	class MBMHandler {
	public:
		MBMHandler() : _mbmTitle(0)
		{
		}
		~MBMHandler()
		{
		}

		int load(char *filename, uint8_t ** inBuffer, uint32_t *inBufSize)
		{

			int devtype = 3; 	// 0:msxaudio 1:msxmusic 2:both 3:stereo
			int isntsc = strstr(_mbmTitle, "50H") == 0 ? 1 : 0;	// testcase: Jer Der songs

			if (strstr(filename, "50Hz")) isntsc = 0;

			int res = mbm2kss(filename, devtype, isntsc);
			if( res != 0)
			{
				return res;
			}

			mbm2kss_result(inBuffer, inBufSize);

			_songLength = mbm2kss_length();
			_mbmTitle = mbm2kss_title(*inBuffer, *inBufSize);

			return 0;
		}

		const char* getTitle()
		{
			return _mbmTitle;
		}

		int songEnd(NEZ_PLAY *nezPlay)
		{
			if (!nezPlay) return 0;

			// by default the song would loop endlessly

			uint8_t *ram = (uint8_t*)nez_get_RAM_buffer(nezPlay);
			return ram[0xda05] == _songLength;
		}

	private:
		const char* _mbmTitle;
		uint8_t	_songLength;
	};


	class Adapter {
	public:
		Adapter() : _sampleRate(44100), _bufferLen(0), _sampleBuffer(NULL), _samplesAvailable(0), _loopSong(false), _nezPlay(NULL), _bufferCopy(0), _isMbmMode(false)
		{
			emuSetup();	// proper NEZ init does seem to need the teardown
		}
		~Adapter()
		{
		}

		int loadFile(char *filename, uint8_t* inBuffer, uint32_t inBufSize, uint32_t sampleRate, uint32_t audioBufSize, uint32_t scopesEnabled)
		{
			_sampleRate = sampleRate;
			allocBuffers(audioBufSize);

			if ((_isMbmMode = endsWith(filename, ".MBM")))
			{
				int res =_mbm.load(filename, &inBuffer, &inBufSize);
				if (res != 0)	return res;
			}

			// it seems that nezPlug corrupts the used input data and Emscripten
			// passes its original cached data -- so we better use a copy..

			inBuffer = getBufferCopy(inBuffer, inBufSize);

			if (emuSetup())
			{
				SONGINFO_Reset(_nezPlay->song);	// e.g. HES does not set anything..

				if (!NEZLoad(_nezPlay, inBuffer, inBufSize))
				{
					MetaInfoHelper *info = MetaInfoHelper::getInstance();

					std::string title = SONGINFO_GetTitle(_nezPlay->song);
					if (title.length() == 0)
					{
						title = filename;
						title.erase( title.find_last_of( '.' ) );	// remove ext
					}
					setText(0, _isMbmMode ? _mbm.getTitle() : title.c_str());
					setText(1, trim(SONGINFO_GetArtist(_nezPlay->song)).c_str());
					setText(2, trim(SONGINFO_GetCopyright(_nezPlay->song)).c_str());

					setNumText(3, NEZGetSongNo(_nezPlay));
					setNumText(4, NEZGetSongMax(_nezPlay));

					// this originally allowed for 4x longer text.. is that a problem?
					setText(5, SONGINFO_GetDetail(_nezPlay->song));
					return 0;
				}
			}
			else
			{
				// error (currently dead code)
			}
			return 1;
		}

		int setSubsong(int track)
		{
			// .hes files do contain multiple tracks but used IDs are not obvious

			if (track >= 0)
			{
				NEZSetSongNo(_nezPlay, track );
			}
			NEZReset(_nezPlay);		// without this there is no sound..

			std::string title = SONGINFO_GetTitle(_nezPlay->song);
			setText(0, _isMbmMode ? _mbm.getTitle() : title.c_str());	// AY EMUL tracks have names
			setNumText(3, NEZGetSongNo(_nezPlay));

			return 0;
		}

		void setLoop(int loop)
		{
			_loopSong = loop;
		}

		void teardown()
		{
			if (_nezPlay != NULL)
			{
				NEZDelete(_nezPlay);
				_nezPlay = NULL;
			}
		}

		int genSamples()
		{
			NEZRender(_nezPlay, _sampleBuffer, _bufferLen);
			_samplesAvailable = _bufferLen;

			if (_isMbmMode && !_loopSong)
			{
				return _mbm.songEnd(_nezPlay);

			}
			return 0;
		}

		char* getSampleBuffer()
		{
			return (char*)_sampleBuffer;
		}

		int getSampleBufferLength()
		{
			return _samplesAvailable;
		}

		int getSampleRate()
		{
			return _sampleRate;
		}

	private:
		void allocBuffers(int len)
		{
			if (len > _bufferLen)
			{
				if (_sampleBuffer) free(_sampleBuffer);

				_sampleBuffer = (int16_t*)malloc(sizeof(int16_t) * len * CHANNELS);
				_bufferLen = len;
			}
		}

		void setText(int idx, const char *t)
		{
			MetaInfoHelper *info = MetaInfoHelper::getInstance();
			info->setText(idx, t, "");
		}

		void setNumText(int idx, int num)
		{
			char tmpBuf[8];
			snprintf(tmpBuf, 8, "%d", num);

			MetaInfoHelper *info = MetaInfoHelper::getInstance();
			info->setText(idx, tmpBuf, "");
		}

		std::string trim(const std::string& str)
		{
			size_t first = str.find_first_not_of(' ');
			if (std::string::npos == first)
			{
				return str;
			}
			size_t last = str.find_last_not_of(' ');
			return str.substr(first, (last - first + 1));
		}

		uint8_t * getBufferCopy(const uint8_t * inBuffer, size_t inBufSize)
		{
			if (_bufferCopy)
			{
				free(_bufferCopy);
			}

			_bufferCopy = (uint8_t*)malloc(inBufSize);
			memcpy( _bufferCopy, inBuffer, inBufSize );
			return _bufferCopy;
		}

		int emuSetup()
		{
			// defaults: frequency = 48000; channel = 1;
			//           naf_type = NES_AUDIO_FILTER_NONE;

			_nezPlay = NEZNew();

			NEZSetFrequency(_nezPlay, _sampleRate);
			NEZSetChannel(_nezPlay, CHANNELS);

			NSF_noise_random_reset = 0;
			NSF_2A03Type = 1;
			Namco106_Realmode = 1;
			Namco106_Volume = 16;
			GBAMode = 0;
			MSXPSGType = 1;
			MSXPSGVolume = 64;
			FDS_RealMode = 3;
			LowPassFilterLevel = 16;
			NESAPUVolume = 64;
			NESRealDAC = 1;
			Always_stereo = 1;

			mbm2kss_init();

			return 1;
		}

		int endsWith(const char *str, const char *suffix)
		{
			if (!str || !suffix)
				return 0;
			size_t lenstr = strlen(str);
			size_t lensuffix = strlen(suffix);
			if (lensuffix >  lenstr)
				return 0;
			return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
		}

	private:
		int _sampleRate;
		int _bufferLen;
		int16_t* _sampleBuffer;
		int _samplesAvailable;
		bool _loopSong;

		NEZ_PLAY *_nezPlay;

		uint8_t* _bufferCopy;

		bool _isMbmMode;
		MBMHandler _mbm;
	};
};

nez::Adapter _adapter;


// old style EMSCRIPTEN C function export to JavaScript.
// todo: code might be cleaned up using EMSCRIPTEN's "new" Embind feature:
// https://emscripten.org/docs/porting/connecting_cpp_and_javascript/embind.html
#define EMBIND(retType, func)  \
	extern "C" retType func __attribute__((noinline)); \
	extern "C" retType EMSCRIPTEN_KEEPALIVE func

EMBIND(int, emu_load_file(char *filename, void * inBuffer, uint32_t inBufSize, uint32_t sampleRate, uint32_t audioBufSize, uint32_t scopesEnabled)) {
	return _adapter.loadFile(filename, (uint8_t*)inBuffer, inBufSize, sampleRate, audioBufSize, scopesEnabled); }
EMBIND(void, emu_teardown())						{ _adapter.teardown(); }
EMBIND(int, emu_get_sample_rate())					{ return _adapter.getSampleRate();}
EMBIND(int, emu_set_subsong(int track))				{ return _adapter.setSubsong(track); }
EMBIND(const char**, emu_get_track_info())			{ return MetaInfoHelper::getInstance()->getMeta(); }
EMBIND(char*, emu_get_audio_buffer())				{ return _adapter.getSampleBuffer(); }
EMBIND(int, emu_get_audio_buffer_length())			{ return _adapter.getSampleBufferLength(); }
EMBIND(int, emu_compute_audio_samples())			{ return _adapter.genSamples(); }
EMBIND(int, emu_get_current_position())				{ return -1; /* not implemented */ }
EMBIND(void, emu_seek_position(int ms))				{ }
EMBIND(int, emu_get_max_position())					{ return -1; /* not implemented */ }

// --- add-on
EMBIND(void, emu_set_loop(int loop))				{ _adapter.setLoop(loop); }

