/*
* The loaders used to handle different types of music files.
*
* WebSid (c) 2020 Jürgen Wothke
* version 0.93
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
#ifndef WEBSID_LOADERS_H
#define WEBSID_LOADERS_H

extern "C" {
#include "base.h"
}

/**
* Abstract "music file" loader.
*
* note: most of the stuff here is kept static to allow for more compiler optimizations.
* Only one instance can be used at a time, i.e. the shared state information is defined by
* whatever instance has been used last (see "load" API).
*/
class FileLoader {
public:
	/**
	* Main factory method used to get suitable loader.
	*/
	static FileLoader* getInstance(uint32_t is_mus, void* in_buffer, uint32_t in_buf_size);
			
	/**
	* Loads a music file into the emulator.
	*/
	virtual uint32_t load(uint8_t* in_buffer, uint32_t in_buf_size, const char* filename, 
							void* basic_ROM, void* char_ROM, void* kernal_ROM, int enable_MD5) = 0;

	/**
	* Select a specific track in a previsouly loaded song (see "load" API).
	*/
	void initTune(uint32_t sample_rate, uint8_t selected_track);

	/**
	* Hook used to detect when a song has played til the end.
	*/
	virtual uint8_t isTrackEnd() { return 0; };	// by default false
	
	// various accessors for music file specific meta information		
	static uint8_t isExtendedSidFile();
	static uint8_t isRSID();	
	static uint8_t getCompatibility();
	static uint8_t getNTSCMode();	
	static const char**  getInfoStrings();
	static uint8_t getCurrentSongSpeed();
		
protected:
	FileLoader();
	
	static uint8_t getValidatedTrack(uint8_t selected_track);
	
	/**
	* Configure the SID chips used in the current emulation.
	*/
	static void configureSids(uint16_t flags, uint8_t* addr_list);

	void init();
	
	static void storeFileInfo();

	static void setRsidMode(uint8_t is_rsid);

	/**
	* Manually override original "video mode" setting from music file.
	*/
	static void setNTSCMode(uint8_t is_ntsc);
};

#endif
