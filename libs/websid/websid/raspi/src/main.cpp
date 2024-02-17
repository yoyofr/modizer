/*
* Driver used to run WebSid as a native program on a "Raspberry Pi 4B" to 
* drive a SidBerry mounted SID chip.
*
* The program either plays using an integrated playback thread (lower 
* quality) or using a separate playback device driver (which must previsouly 
* have been installed as a kernel module). Upon startup this program 
* automatically selects the best available playback option.
*
* Note: the WebSid emulator uses:
* 
* NTSC:	59.8260895 Hz screen refresh rate 
* 		17095 clock cycles per frame
* 		1022727 Hz clock speed (14.31818MHz/14)
* 		
* PAL:	50.124542  Hz screen refresh rate 
* 		19656 clock cycles per frame
* 		985249 Hz clock speed (17.734475MHz/18)
* 
* Known Limitation: The Raspberry's timer is limited to 1MHz (with a 
* questionable precision that may be distorted by the potentially changing 
* ARM clockrate) and the "SID write" timing is limited by this. When actual 
* PAL/NTSC clock rates are used for the SID, this does benefit the waveform
* output of the SID but it also introduces rounding issues.
*
* todo: For the purpose of comparing real outout to emulator output it might 
* be a  good idea to save 1MHz "script" files and later re-run a 1Mhz clocked 
* SID emulation directly based on that script file.
* 
*
* WebSid (c) 2021 Jürgen Wothke
* version 1.0
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

// reminder: useful "vcgencmd get_config int" to get actually used 
// config.txt settings (incl defaults)


#include <iostream>     // std::cout
#include <string.h>

#include <unistd.h>		// close

// WebSid stuff
#include "../../src/core.h"
#include "../../src/loaders.h"
extern "C" {
#include "../../src/vic.h"	
	// from sidplayer.cpp
uint32_t loadSidFile(uint32_t is_mus, void* in_buffer, uint32_t in_buf_size, 
					uint32_t sample_rate, char* filename, void* basic_ROM, 
					void* char_ROM, void* kernal_ROM);
uint32_t playTune(uint32_t selected_track, uint32_t trace_sid);
char** getMusicInfo();
}

// Raspberry stuff
#include "rpi4_utils.h"
#include "cp1252.h"
#include "gpio_sid.h"

#include "fallback_handler.h"
#include "device_driver_handler.h"

using namespace std;

#define SID_FILE_MAX 0xffff
#define CHANNELS 2

// measure now long it takes to preduce 20ms of audio
//#define CHECK_PERFORMANCE

// just run a test driver instead of relular player
//#define RUN_TEST

static uint8_t _force_1MHz_clock = 0;

static volatile uint32_t _max_frames;

// timings delivered by the emulator are measured in real PAL/NTSC cycles,
// e.g. for a PAL song the used Raspberry-Counter clocking is faster than 
// it should be and timing triggers need to be slowed *down* accordingly 
// (i.e. made larger) to try to get a result closer to what it should be - 
// however respective rounding may likely introduce problems of its own.. 
// (with the GPIO based clock signal, the SID chip should actually be 
// clocked pretty close to correct)

#define NTSC_ADJUST (((double)1)/1.022727)
#define PAL_ADJUST (((double)1)/0.985249)
static double _adjustment = 1;

static PlaybackHandler *_playbackHandler = 0;


void showHelp(char *argv[]) {
	
	cout << "Usage: " << argv[0] << " <song filename> [Options]" << endl;
	cout << "Options: " << endl;
	cout << " -t, --track   : index of the track to play (if more than 1 is available)" << endl;			
	cout << " -v, --version : Show version and copyright information " << endl;
	cout << " -f, --force   : Forces use of 1MHz clock for SID chip" << endl;
	cout << " -h, --help    : Show this help message " << endl << endl;
	
	cout << "Note: MOS6581/8580 player specifically designed for use with a" << endl;
	cout << "Raspberry Pi 4 (it cannot be used with older models!) and with" << endl;
	cout << "the SID-chip adapter/GPIO wiring used in the SidBerry project." << endl << endl;
	
	cout << "The player expects to have CPU core #3 dedicated for its exclusive" << endl;
	cout << "use: add 'isolcpus=3 rcu_nocbs=3 rcu_nocb_poll=3 nohz_full=3 ' to" << endl;
	cout << "/boot/cmdline.txt for best results. " << endl;
	
	exit(1);
}

void handleArgs(int argc, char *argv[], string *filename, int *track) {
	for(int i = 1; i < argc; i++) {
		if((filename->length() == 0) && argv[i][0] != '-') {
			*filename = argv[i];			
		} else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
			cout << "WebSid (RPi4 edition 1.0)" << endl;
			cout << "(C)opyright 2021 by Jürgen Wothke" << endl;
		} else if(!strcmp(argv[i], "-f") || !strcmp(argv[i], "--force")) {
			i++;
			_force_1MHz_clock = 1;
		} else if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--track")) {
			i++;
			*track = atoi(argv[i]) - 1;
		} else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			showHelp(argv);			
		} else {
			cout << "Warning: Invalid parameter " << endl;
		}
	}

	if(filename->length() == 0) {
		showHelp(argv);			
	}
}

size_t loadBuffer(string *filename, unsigned char *buffer) {
	FILE *file = fopen(filename->c_str(), "rb");	
	if(file == NULL) {
		cout << "file not found: " << (*filename) << endl;
		return 0;
	}
	
	size_t size = fread(buffer, 1, SID_FILE_MAX, file);
	fclose(file);
	
	return size;
}

void printSongInfo() {
	unsigned char** info = (unsigned char**)getMusicInfo();
	
	cout << endl;	
	cout << "name     : " << cp1252_to_utf(info[4]) << endl;
	cout << "author   : " << cp1252_to_utf(info[5]) << endl;
	cout << "copyright: " << cp1252_to_utf(info[6]) << endl << endl;
}

void loadSidFile(int argc, char *argv[]) {
    int track = 0;
	string filename = "";
	handleArgs(argc, argv, &filename, &track);
	
	unsigned char buffer[SID_FILE_MAX];
	size_t size = loadBuffer(&filename, buffer);
	
	if ( loadSidFile(0, buffer, size, 44100, (char*)filename.c_str(), 0, 0, 0)) {
		cout << "error: no valid sid file - " << filename << endl;
		exit(1);
	}
	if ( playTune(track, 0)) {
		cout << "error: cannot select track - " << track << endl;
		exit(1);
	}
	
	printSongInfo();
}

void int_callback(int s) {
	// SIGINT (ctrl-C) handler so that proper cleanup can be performed
	_max_frames = 0;		// make play loop end at next iteration	
		
	cout << "playback was interrupted" << endl;
}

void init() {
	installSigHandler(int_callback);
	
	systemTimerSetup();		// init access to system timer counter
}

// callback triggered in the emulator whenever a SID register is written
void recordPokeSID(uint32_t ts, uint8_t reg, uint8_t value) {
	ts = (uint32_t)(_adjustment * ts  + 0.5);
	
	_playbackHandler->recordPokeSID(ts, reg, value);
}

void initPlaybackHandler() {
	int fd;
	volatile uint32_t *buffer_base;
	DeviceDriverHandler::detectDeviceDriver(&fd, &buffer_base);
	
	if (fd < 0) {
		cout <<  "using    : local thread based playback" << endl;
		_playbackHandler= new FallbackHandler();
	} else {
		cout <<  "using    : device driver based player" << endl;		
		_playbackHandler = new DeviceDriverHandler(fd, buffer_base);
	}
}

#ifdef RUN_TEST
#include "testing_only.c"
#endif

int main(int argc, char *argv[]) {
	migrateThreadToCore(2);	// run main thread on isolated core #2 to avoid unnecessary conflicts

	init();
	
	startHyperdrive();
	
#ifdef RUN_TEST
	runTest();
#else	
	initPlaybackHandler();

	_playbackHandler->recordBegin();	
	loadSidFile(argc, argv);// songs already access SID in INIT.. so recording must be ready
	_playbackHandler->recordEnd();		
		
	uint32_t	sample_rate = 44100;
	uint8_t		speed =	FileLoader::getCurrentSongSpeed();
	
	uint16_t	chunk_size = sample_rate / vicFramesPerSecond();// number of samples per call
	int16_t* 	synth_buffer = (int16_t*)malloc(sizeof(int16_t)* (chunk_size * CHANNELS + 1));

	if (_force_1MHz_clock) {
		_adjustment = 1;
		gpioSetClockSID(CLOCK_1MHZ);
	} else {
		if (FileLoader::getNTSCMode()) {
			_adjustment = NTSC_ADJUST;
			gpioSetClockSID(CLOCK_NTSC);
		} else {
			_adjustment = PAL_ADJUST;
			gpioSetClockSID(CLOCK_PAL);
		}
	}
	
#ifdef CHECK_PERFORMANCE
	uint32_t max_micro = 0;
	uint32_t min_micro = 0xffffffff;
#endif

	double frame_in_sec = 1.0 / vicFramesPerSecond();
	_max_frames =  5*60*vicFramesPerSecond();	// 5 minutes

	uint32_t overflow_limit = ((0xffffffff - micros()) / 1000000)*vicFramesPerSecond();	// secs to next counter overflow

	if (overflow_limit < _max_frames) {
		_max_frames = overflow_limit;
		cout << "note: limiting playback time to avoid counter overflow" << endl;
	}
	
	for (uint32_t frame_count = 0; frame_count < _max_frames; frame_count++) {
		_playbackHandler->recordBegin();
				
		// the output size of the below runOneFrame call is actually defined by the
		// requested chunk_size, i.e. it will typically cover between 17-20ms 
		
#ifndef CHECK_PERFORMANCE
		Core::runOneFrame(1, speed, synth_buffer, 0, chunk_size);	// will trigger recordPokeSID callback
		
		_playbackHandler->recordEnd();
#else
		uint32_t start = micros();
	
		Core::runOneFrame(1, speed, synth_buffer, 0, chunk_size);	// will trigger recordPokeSID callback

		uint32_t diff = micros() - start;

		_playbackHandler->recordEnd();

		if (diff > max_micro) max_micro= diff;
		if (diff < min_micro) min_micro= diff;
#endif		
		uint32_t sec= frame_count*frame_in_sec;
		printf("\rplaying [%02d:%02d]", sec/60, sec%60); fflush(stdout);		
	}
	
#ifdef CHECK_PERFORMANCE
	// debugging: check worst case performance
	cout << "\n\nmin: " <<  min_micro/1000 << " ms max: " << max_micro/1000 << endl;
#endif

	// teardown	
	if (_playbackHandler) delete _playbackHandler;
		
	free(synth_buffer);		
#endif
	
	stopHyperdrive();
	systemTimerTeardown();
	
	return 0;
}

