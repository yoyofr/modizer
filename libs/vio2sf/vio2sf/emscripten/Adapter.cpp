/*
* This file adapts "2SF decoder" to the interface expected by my generic JavaScript player..
*
* Copyright (C) 2019-2023 Juergen Wothke
*
* LICENSE: GPL
*/

#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>     /* malloc, free, rand */

#include <exception>
#include <iostream>
#include <fstream>

#include "MetaInfoHelper.h"
using emsutil::MetaInfoHelper;


#define BUF_SIZE	1024
#define TEXT_MAX	255
#define NUM_MAX	15

// see Sound::Sample::CHANNELS
#define CHANNELS 2				
#define BYTES_PER_SAMPLE 2
#define SAMPLE_BUF_SIZE	1024


// interface to twosfplug.cpp
extern	void ds_setup (void);
extern	void ds_boost_volume(unsigned char b);
extern	int32_t ds_end_song_position ();
extern	int32_t ds_current_play_position ();
extern	int32_t ds_get_sample_rate ();
extern	int ds_load_file(const char *uri);
extern	int ds_read(int16_t *output_buffer, uint16_t outSize);
extern	int ds_seek_position (int ms);

void ds_meta_set(const char * tag, const char * value) {
	MetaInfoHelper *info = MetaInfoHelper::getInstance();
	
	// propagate selected meta info for use in GUI
	if (!strcasecmp(tag, "title")) 
	{
		info->setText(0, value, "");
	} 
	else if (!strcasecmp(tag, "artist")) 
	{
		info->setText(1, value, "");
	} 
	else if (!strcasecmp(tag, "album")) 
	{
		info->setText(2, value, "");
	} 
	else if (!strcasecmp(tag, "date")) 
	{
		info->setText(3, value, "");
	} 
	else if (!strcasecmp(tag, "genre")) 
	{
		info->setText(4, value, "");
	} 
	else if (!strcasecmp(tag, "copyright")) 
	{
		info->setText(5, value, "");
	} 
	else if (!strcasecmp(tag, "usfby")) 
	{
		info->setText(6, value, "");
	} 
}


namespace ds {
	class Adapter {
	public:
		Adapter() : _samplesAvailable(0)
		{
		}
		
		int loadFile(char *filename, void * inBuffer, uint32_t inBufSize, uint32_t sampleRate, uint32_t audioBufSize, uint32_t scopesEnabled)
		{	
			// fixme: sampleRate, audioBufSize, scopesEnabled support not implemented (much is hardcoded in the DS emulator)
			
			ds_setup();
			return (ds_load_file(filename) == 0) ? 0 : -1;
		}

		void teardown() 
		{
			MetaInfoHelper::getInstance()->clear();
					
			cleanOutputBuffer();
		}

		int getSampleRate()
		{
			return ds_get_sample_rate();
		}

		int setBoost(unsigned char boost) 
		{
			ds_boost_volume(boost);
			return 0;
		}

		char* getSampleBuffer()
		{
			return (char*)_sampleBuffer;
		}
		
		long getSampleBufferLength() 
		{
			return _samplesAvailable;
		}

		int getMaxPosition() 
		{
			return ds_end_song_position();
		}
		
		int getCurrentPosition() 
		{
			return ds_current_play_position();
		}
		
		void seekPosition(int ms) 
		{
			ds_seek_position(ms);
		}
		
		int genSamples() 
		{
			_samplesAvailable =  ds_read((short*)_sampleBuffer, SAMPLE_BUF_SIZE);			
			return _samplesAvailable ? 0 : 1;
		}
	private:
		void cleanOutputBuffer() 
		{
			for (int i = 0; i < (SAMPLE_BUF_SIZE * CHANNELS); i++) 
			{
				_sampleBuffer[i]= 0;
			}	
		}
	private:
		signed short _sampleBuffer[SAMPLE_BUF_SIZE * CHANNELS];
		int _samplesAvailable;
	};
};

ds::Adapter _adapter;



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
EMBIND(int, emu_set_subsong(int track))				{ return 0;	/*there are no subsongs*/ }
EMBIND(const char**, emu_get_track_info())			{ return MetaInfoHelper::getInstance()->getMeta(); }
EMBIND(int, emu_compute_audio_samples())			{ return _adapter.genSamples(); }
EMBIND(char*, emu_get_audio_buffer())				{ return _adapter.getSampleBuffer(); }
EMBIND(int, emu_get_audio_buffer_length())			{ return _adapter.getSampleBufferLength(); }
EMBIND(int, emu_get_current_position())				{ return _adapter.getCurrentPosition(); }
EMBIND(void, emu_seek_position(int ms))			{ _adapter.seekPosition(ms);  }
EMBIND(int, emu_get_max_position())					{ return _adapter.getMaxPosition(); }


// --- add-on
EMBIND(int, emu_set_boost(int boost))				{ return _adapter.setBoost(boost); }
