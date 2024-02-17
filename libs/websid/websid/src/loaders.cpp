/*
* The loaders used to handle different types of music files.
*
* WebSid (c) 2020 Jürgen Wothke
* version 0.93
*
* Below code has been roughly refactored from the old C impl. Many of the implementation
* details have not been migrated to C++ (i.e. instance vars/methods) to avoid additional
* indirections that might slow down the code. From a code structuring/readability point
* of view that should already be an improvement.
*
* Only one loader is used for a specific song, i.e. at any given moment!
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

extern "C" {
#include "md5.h"

#include "core.h"
#include "memory.h"
}
#include "sid.h"

#include "loaders.h"

#include "compute!.h"

#define MULTI_SID_TYPE 0x4E


static MD5_CTX		_md5;
static char			_md5_hex_string[32 + 1];


// ---- these are implementation classes that should rather be hidden from the interface ----

#ifdef TEST
/**
* This is used for testcases only.
*/
class TestFileLoader : public FileLoader {
public:
	TestFileLoader();

	virtual uint32_t load(uint8_t* in_buffer, uint32_t in_buf_size, char* filename,
							void* basic_ROM, void* char_ROM, void* kernal_ROM, int enable_MD5);
};
#endif

/**
* This loads Compute! MUS player files.
*/
class MusFileLoader : public FileLoader {
public:
	MusFileLoader();

	virtual uint32_t load(uint8_t* in_buffer, uint32_t in_buf_size, const char* filename,
							void* basic_ROM, void* char_ROM, void* kernal_ROM, int enable_MD5);

	virtual uint8_t isTrackEnd();
private:
	static uint16_t loadComputeSidplayerData(uint8_t* mus_song_file, uint32_t mus_song_file_len);
};

/**
* This loads SID format files.
*/
class SidFileLoader : public FileLoader {
public:
	SidFileLoader();

	virtual uint32_t load(uint8_t* in_buffer, uint32_t in_buf_size, const char* filename,
							void* basic_ROM, void* char_ROM, void* kernal_ROM, int enable_MD5);
};


// ----  meta information originating from music file  ----------------------------------

static uint8_t 	_sid_file_version;

static uint8_t	_is_rsid;

static uint8_t 	_ntsc_mode= 0;

static uint8_t 	_compatibility;	// i.e. song should play on a real C64
static uint8_t 	_basic_prog;
static uint16_t _free_space;

static uint16_t	_load_addr, _init_addr, _play_addr, _load_end_addr;
static uint8_t 	_selected_track, _max_track;
static uint32_t	_play_speed;

// song specific infos
	// 0: load_addr;
	// 1: play_speed;
	// 2: max_sub_song;
	// 3: actual_sub_song;
	// 4: song_same;
	// 5: song_author;
	// 6: song_copyright;
	// 7: md5 of sid file
static void* _load_result[8];

#define MAX_INFO_LEN 32
#define MAX_INFO_LINES 5

static 	char 	_song_name[MAX_INFO_LEN + 1],
				_song_author[MAX_INFO_LEN + 1],
				_song_copyright[MAX_INFO_LEN + 1],
				_song_info_trash[MAX_INFO_LEN + 1];

static char* _info_texts[MAX_INFO_LINES];

static void resetInfoText() {
	_info_texts[0] = _song_name;
	_info_texts[1] = _song_author;
	_info_texts[2] = _song_copyright;

	// .mus files may have more lines with unstructured text... ignore
	_info_texts[3] = _song_info_trash;
	_info_texts[4] = _song_info_trash;

	memset(_song_name, 0, MAX_INFO_LEN);
	memset(_song_author, 0, MAX_INFO_LEN);
	memset(_song_copyright, 0, MAX_INFO_LEN);
}


// ----  abstract base class of all loaders  --------------------------------------------

FileLoader::FileLoader() {
}

// respective loader singletons:
#ifdef TEST
static TestFileLoader	_test_loader;
#endif
static SidFileLoader	_sid_loader;
static MusFileLoader	_mus_loader;


FileLoader* FileLoader::getInstance(uint32_t is_mus, void* in_buffer, uint32_t in_buf_size) {
#ifdef TEST
	return &_test_loader;
#else
	if (is_mus) {
		return &_mus_loader;
	} else {
		// poor man's input validation
		uint8_t *input_file_buffer= (uint8_t *)in_buffer;
		if ( (in_buf_size < 0x7c) || !((input_file_buffer[0x00] == 0x50) || (input_file_buffer[0x00] == 0x52))) {
			return 0;	// we need at least a header that starts with "P" or "R"
		}
		return &_sid_loader;
	}
#endif
}

void FileLoader::init() {
	_init_addr = 0;
	_play_addr = 0;
	_load_end_addr = 0;
	_selected_track = 0;
	_max_track = 0;
	_play_speed = 0;

	resetInfoText();
}
uint8_t FileLoader::getNTSCMode() {
	return _ntsc_mode;
}
uint8_t FileLoader::getValidatedTrack(uint8_t selected_track) {
	return  (selected_track >= _max_track) ? _selected_track : selected_track;
}
void FileLoader::setRsidMode(uint8_t is_rsid) {
	_is_rsid = is_rsid;
}

uint8_t FileLoader::isExtendedSidFile() {
	return _sid_file_version == MULTI_SID_TYPE;
}

uint8_t FileLoader::isRSID() {
	return _is_rsid;
}

uint8_t FileLoader::getCompatibility(){
	return _compatibility;
}

void FileLoader::setNTSCMode(uint8_t is_ntsc) {
	_ntsc_mode = is_ntsc;
}

void FileLoader::initTune(uint32_t sample_rate, uint8_t selected_track) {
	_selected_track = getValidatedTrack(selected_track);

	uint8_t timerDrivenPSID = (!_is_rsid && (FileLoader::getCurrentSongSpeed() == 1));

	Core::startupTune(sample_rate, _selected_track,
					_is_rsid, timerDrivenPSID, _ntsc_mode, _compatibility, _basic_prog,
					_free_space, &_init_addr, _load_end_addr, _play_addr);
}

void FileLoader::storeFileInfo() {
	_load_result[0] = &_load_addr;
	_load_result[1] = &_play_speed;
	_load_result[2] = &_max_track;
	_load_result[3] = &_selected_track;
	_load_result[4] = _song_name;
	_load_result[5] = _song_author;
	_load_result[6] = _song_copyright;
	_load_result[7] = _md5_hex_string;
}



const char** FileLoader::getInfoStrings() {
	return (const char**)_load_result;
}

static uint8_t get_bit(uint32_t val, uint8_t b) {
    return (uint8_t) ((val >> b) & 1);
}

uint8_t FileLoader::getCurrentSongSpeed() {
	/*
	* PSID V2: songSpeed 0: means screen refresh based, i.e. ignore
	*                       timer settings an just play 1x per refresh
	*					 1: means CIA 1 timer A or 60hz (this is the ROM's default)
	*/
	return get_bit(_play_speed, _selected_track > 31 ? 31 : _selected_track);
}

void FileLoader::configureSids(uint16_t flags, uint8_t* addr_list) {
	SIDConfigurator* cf = SID::getHWConfigurator();
	cf->configure(FileLoader::isExtendedSidFile(), _sid_file_version, flags, addr_list);
}

// ------------------  handling of unit tests  -------------------------------------------------------------------------

#ifdef TEST

TestFileLoader::TestFileLoader() {
}

static uint16_t loadTestFromMemory(void *buf, uint32_t buflen) {
	uint8_t *pdata = (uint8_t*)buf;;
    uint8_t data_file_offset = 0;

	uint16_t load_addr;

	// original C64 binary file format
	load_addr = pdata[0];
	load_addr |= pdata[1] << 8;

	data_file_offset += 2;

	int32_t size = buflen - 2;
	if (size < 0 || size > 0xffff) {
		return 0;		// illegal sid file
	}
	Core::loadSongBinary(&pdata[2], load_addr, size, 0);
    return load_addr;
}

uint8_t match(const char* test, char* filename) {
	static char path[128];
	snprintf(path, 128, "/websid_test/tests/cpu/%s", test);
	return !strcmp(filename, path);
}

void skipCode(const char* test, char* filename, uint8_t* buf, uint16_t from, uint16_t to) {

	if (match(test, filename)) {
		uint16_t load_addr= 0x0801;	// same for all tests

		fprintf(stderr, "  disabling test of DEC mode\n");

		// a "JMP" to the next test is patched into the start of the DEC mode test)
		buf[from - load_addr + 2] = 0x4c;		// note: buffer contains 2 byte header
		buf[from - load_addr + 3] = to & 0xff;
		buf[from - load_addr + 4] = to >> 8;
	}
}

void disableDecimalModeTests(uint8_t* in_buffer, uint32_t in_buf_size, char* filename) {
	if (in_buf_size > (0x09ff - 0x0801)) {	// just in case
		// decimal mode is not implemented and respective parts of
		// ADC/SBC/ARR tests are disabled below

		skipCode("adcb", filename, in_buffer, 0x08b3, 0x098c);
		skipCode("adca", filename, in_buffer, 0x08b3, 0x098c);

		skipCode("adczx", filename, in_buffer, 0x08b3, 0x0993);

		skipCode("adcax", filename, in_buffer, 0x08b4, 0x099e);
		skipCode("adcay", filename, in_buffer, 0x08b4, 0x099e);

		skipCode("adcix", filename, in_buffer, 0x08bc, 0x0998);
		skipCode("adciy", filename, in_buffer, 0x08bc, 0x0998);

		skipCode("adcz", filename, in_buffer, 0x08af, 0x0989);

		skipCode("sbcb", filename, in_buffer, 0x08b3, 0x0984);
		skipCode("sbca", filename, in_buffer, 0x08b3, 0x0984);

		skipCode("sbcz", filename, in_buffer, 0x08af, 0x0981);

		skipCode("sbczx", filename, in_buffer, 0x08b3, 0x098b);

		skipCode("sbcax", filename, in_buffer, 0x08b4, 0x0996);
		skipCode("sbcay", filename, in_buffer, 0x08b4, 0x0996);

		skipCode("sbcix", filename, in_buffer, 0x08bc, 0x0990);
		skipCode("sbciy", filename, in_buffer, 0x08bc, 0x0990);

		skipCode("sbcb(eb)", filename, in_buffer, 0x08b7, 0x0988);

		skipCode("arrb", filename, in_buffer, 0x0899, 0x0959);
	}
}

void disableVICTests(uint8_t* in_buffer, uint32_t in_buf_size, char* filename) {
	if (match("mmufetch", filename)) {
		fprintf(stderr, "  disabling VIC dependent test part\n");

		// note: this test depends on the bank-resetting logik of the original
		// kernal rom EA31 IRQ handler.. (see EA75)

		// disable the VIC test by removing the STAs here
		//;097B    8D 02 D0    STA $D002
		//;097E    8D 03 D0    STA $D003

		if (in_buf_size > (0x097E - 0x0801 + 2)) {
			in_buffer[0x097B - 0x0801 + 2] = 0xad;
			in_buffer[0x097E - 0x0801 + 2] = 0xad;
		}
	}
}

void disableBRKTest(uint8_t* in_buffer, uint32_t in_buf_size, char* filename) {
	if (match("nmi", filename)) {
		fprintf(stderr, "  disabling BRK timing test\n");

		if (in_buf_size > (0x0c29 - 0x0801 + 2)) {
			in_buffer[0x0c29 - 0x0801 + 2] = 0x01;	// skip opcode 0x00
		}
	}
}

uint32_t TestFileLoader::load(uint8_t* in_buffer, uint32_t in_buf_size, char* filename,
								void* basic_ROM, void* char_ROM, void* kernal_ROM, int enable_MD5) {

	fprintf(stderr, "starting test %s\n", filename);

	// exclude non existing functionalities from tests
	disableDecimalModeTests(in_buffer, in_buf_size, filename);
	disableVICTests(in_buffer, in_buf_size, filename);
	disableBRKTest(in_buffer, in_buf_size, filename);

	init();

	_sid_file_version = 2;
	_basic_prog = 0;
	_compatibility = 1;

	setRsidMode(1);

	FileLoader::setNTSCMode(0);

	FileLoader::configureSids(0, 0); 	// just use one *old* SID at d400

	memInitTest();

	_init_addr = loadTestFromMemory(in_buffer, in_buf_size);

	if (_init_addr) {
		if (_init_addr != 0x0801) {
			fprintf(stderr, "ERROR: unexpected start for  test: %s len: %lu addr: %d\n",
					filename, in_buf_size, _init_addr);
		}
		Core::rsidRunTest();
	} else {
		fprintf(stderr, "ERROR: cannot load test: %s\n", (const char*)filename);
	}
	return 0;
}

#endif

// ------------------  handling of Compute!'s SidPlayer (.mus files) ---------------------------------------------------

#define MUS_HEAD 0x8
#define MUS_BASE_ADDR 0x17b0
#define MUS_PLAYER_START MUS_BASE_ADDR
	// where the .mus music file is loaded
#define MUS_DATA_START 0x281e
	// where the player looks for the pointers into the 3 voice command streams (the same is repeated at pos +9)
//#define MUS_VOICE_PTRS 0x2743
#define MUS_PLAY_INDICATOR 0x269B

const static uint16_t MUS_REL_DATA_START = MUS_DATA_START - MUS_BASE_ADDR;
const static uint16_t MUS_MAX_SIZE = MUS_REL_DATA_START;
const static uint16_t MUS_MAX_SONG_SIZE = 0xA000 - MUS_DATA_START;	// stop at BASIC ROM.. or how big are these songs?

	// buffer used to combine .mus and player
static uint8_t*			_mus_mem_buffer = 0;							// represents memory at MUS_BASE_ADDR
const static uint16_t	_mus_mem_buffer_size = 0xA000 - MUS_BASE_ADDR;


static uint16_t musGetOffset(uint8_t* buf) {
	return (((uint16_t)buf[1]) << 8) + buf[0];
}

static void musGetSizes(uint8_t* mus_song_file, uint16_t* v1len, uint16_t* v2len,
						uint16_t* v3len, uint16_t* track_data_len) {

	(*v1len) = musGetOffset(mus_song_file + 2);
	(*v2len) = musGetOffset(mus_song_file + 4);
	(*v3len) = musGetOffset(mus_song_file + 6);

	(*track_data_len) = MUS_HEAD + (*v1len) + (*v2len) + (*v3len);
}

static uint8_t musIsTrackEnd(uint8_t voice) {
	return memReadRAM(MUS_PLAY_INDICATOR) == 0x0;
	/*
	// check for end of .mus voice 0 (other voices might be shorter & loop)
	uint16_t addr= (memReadRAM(voice + MUS_VOICE_PTRS+3) << 8) + memReadRAM(voice + MUS_VOICE_PTRS) - 2;	// pointer stops past the 0x14f HALT command!
	return (memReadRAM(addr) == 0x1) && (memReadRAM(addr+1) == 0x4f); 	// HALT command 0x14f
	*/
}

static void musMapInfoTexts(uint8_t* mus_song_file, uint32_t mus_song_file_len, uint16_t track_data_len) {
	if (mus_song_file_len <= track_data_len) return;

	uint8_t* buffer = mus_song_file + track_data_len;
	uint16_t max_info_len = mus_song_file_len - track_data_len;

	resetInfoText();

	uint8_t line = 0;
	uint8_t current_len = 0;
	for (uint8_t j= 0; j<max_info_len; j++) {	// iterate over all the remaining chars in the file
		uint8_t ch = buffer[j];

		// remove C64 special chars.. don't have that font anyway
		if (!(ch == 0xd) && ((ch < 0x20) || (ch > 0x60))) continue;

		if (current_len < MAX_INFO_LEN) {
			char* dest = _info_texts[line];
			dest[current_len++] = (ch == 0xd) ? 0 : ch;

			// last one wins.. hopefully the 0 terminator..
			if (MAX_INFO_LEN == current_len) current_len--;
		} else {
			// ignore: should not be possible..
		}
		if ((ch == 0xd) && (current_len > 0)) {	// remove empty lines
			// start new line..
			current_len = 0;
			line++;
			if (line >= MAX_INFO_LINES) break;
		}
	}
}

MusFileLoader::MusFileLoader() {
}

// Compute!'s .mus files require an addtional player that must installed with the song file.
uint16_t MusFileLoader::loadComputeSidplayerData(uint8_t *mus_song_file, uint32_t mus_song_file_len) {
	// note: the player can also be used in RSID mode (but for some reason the timing is then much slower..)
	_sid_file_version = 2;
	_basic_prog = 0;
	_compatibility = 1;
	FileLoader::setNTSCMode(1);// .mus stuff is mostly from the US..

	FileLoader::configureSids(0, 0); 	// just use one *old* SID at d400

	_load_addr = MUS_BASE_ADDR;
	_load_end_addr = 0x9fff;

	_init_addr = MUS_BASE_ADDR;
	_play_addr = 0x1a07;		// unused in RSID emulation

	uint16_t pSize = COMPUTESIDPLAYER_LENGTH;
	if((pSize > MUS_MAX_SIZE) || (mus_song_file_len > MUS_MAX_SONG_SIZE)) return 0; // ERROR

	// prepare temp input buffer
	if (_mus_mem_buffer == 0) {
		_mus_mem_buffer = (uint8_t*)malloc(_mus_mem_buffer_size);	// represents mem from MUS_BASE_ADDR to $9fff
	}
	memcpy(_mus_mem_buffer, computeSidplayer, pSize);

	// patch: put INIT in endless loop rather than RTS (more convenient for RSID emulation)
	_mus_mem_buffer[0x002e] = 0x4c;
	_mus_mem_buffer[0x002f] = 0xde;
	_mus_mem_buffer[0x030] = 0x17;


	// patch/configure the MUS player
	_mus_mem_buffer[0x17ca - MUS_BASE_ADDR]= (!_ntsc_mode) & 0x1;	// NTSC by default.. so this is not really needed


	// patch "INIT" routine to load the MUS file from other address
//	uint16_t addr = MUS_DATA_START + 2;					// skip 2-bytes header, e.g. start at $2820
//	_mus_mem_buffer[0x17ef - MUS_BASE_ADDR] = addr & 0xff;
//	_mus_mem_buffer[0x17f1 - MUS_BASE_ADDR] = addr >>8;

	memcpy(_mus_mem_buffer + MUS_REL_DATA_START, mus_song_file, mus_song_file_len);


	uint16_t v1len, v2len, v3len, track_data_len;
	musGetSizes(mus_song_file, &v1len, &v2len, &v3len, &track_data_len);
	if (track_data_len >= mus_song_file_len) {
#ifdef EMSCRIPTEN
		EM_ASM_({ console.log('info cannot be retrieved  from corrupt .mus file');});	// less mem than inclusion of fprintf
#else
		fprintf(stderr, "info cannot be retrieved  from corrupt .mus file\n");
#endif
		return 0;
	}

 	musMapInfoTexts(mus_song_file, mus_song_file_len, track_data_len);

	return 1;
}

uint32_t MusFileLoader::load(uint8_t* input_file_buffer, uint32_t in_buf_size, const char* filename,
							void* basic_ROM, void* char_ROM, void* kernal_ROM, int enable_MD5) {

	init();

	_md5_hex_string[0] = 0;	// not available for MUS

	memResetKernelROM((uint8_t*)kernal_ROM);

	setRsidMode(1);
	memResetRAM(FileLoader::getNTSCMode(), !FileLoader::isRSID());

	// todo: the same kind of impl could be used for .sid files that contain .mus data.. (see respective flag)
	if (!loadComputeSidplayerData(input_file_buffer, in_buf_size)) {
		return 1;
	}

	Core::loadSongBinary(_mus_mem_buffer, _load_addr, _mus_mem_buffer_size, 0);

	storeFileInfo();
	return 0;
}

uint8_t MusFileLoader::isTrackEnd() {
	return musIsTrackEnd(0);
}

// ------------------  handling of regular SID format (.sid files) ------------------------------------------------------

SidFileLoader::SidFileLoader() {
}

static uint16_t loadSIDFromMemory(void* sid_data, uint16_t* load_addr, uint16_t* load_end_addr,
									uint16_t* init_addr, uint16_t* play_addr, uint8_t* subsongs,
									uint8_t* startsong, uint32_t* speed, uint32_t file_size,
									int32_t* load_size, uint8_t basic_mode) {

    uint8_t *pdata = (uint8_t*)sid_data;;
    uint8_t data_file_offset = pdata[7];

    *load_addr = pdata[8] << 8;
    *load_addr |= pdata[9];

    *init_addr = pdata[10] << 8;
    *init_addr |= pdata[11];

    *play_addr = pdata[12] << 8;
    *play_addr |= pdata[13];

	// "sid file format" stupidity: use of 16 bits just so that
	// counter can run from 1 to 256 instead of 0 to 255:
	uint16_t tracks = pdata[0xf] | (((uint16_t)pdata[0xe]) << 8);
	if (tracks == 0) tracks = 1;
	if (tracks > 0xff) tracks = 0xff;
    *subsongs = tracks & 0xff;

	uint16_t start = pdata[0x11] | (((uint16_t)pdata[0x10]) << 8);
	if (!start) start = 1;
	if (start > tracks) start = tracks;
    *startsong = (start & 0xff) - 1;	// start at index 0

	if (*load_addr == 0) {
		// original C64 binary file format

		*load_addr = pdata[data_file_offset];
		*load_addr |= pdata[data_file_offset + 1] << 8;

		data_file_offset += 2;
	}
	if (*init_addr == 0) {
		*init_addr = *load_addr;	// 0 implies that init routine is at load_addr
	}

	// more "sid file format" sillyness: only 32 "speed" bits for possible 256 tracks
    *speed = pdata[0x12] << 24;
    *speed |= pdata[0x13] << 16;
    *speed |= pdata[0x14] << 8;
    *speed |= pdata[0x15];

	*load_end_addr = *load_addr + file_size - data_file_offset;

	int32_t size= file_size - data_file_offset;
	if (size < 0 || size > 0xffff) {
		return 0;		// illegal sid file
	}

	// find a space to put the starter code: the number of hoops you have
	// to jump through just to find 6 free bytes is a bad joke..

	uint8_t start_page = pdata[0x78];
	uint8_t driver_size = 33;	// see memory.c: _driverPSID

	_free_space = 0;
	if (start_page == 0xff) {
		// no space available
	} else if (start_page == 0x0) {
		if (((*load_addr) + size) < (0xcfff - driver_size)) {
			_free_space = 0xcfff - driver_size;
		} else if ((*load_addr) >= (0x0400 + driver_size)) {
			_free_space = (0x0400 + driver_size);
		}
	} else {
		_free_space = ((uint16_t)start_page) << 8;
	}

	Core::loadSongBinary(&pdata[data_file_offset], *load_addr, size, basic_mode);
	*load_size = size;

    return *load_addr;
}

static uint16_t parseSYS(uint16_t start, uint16_t end) {
	// parse a simple "123 SYS3000" BASIC command
	uint16_t result = 0;
	uint8_t c = 0;
	for (uint16_t i= start; i<=end; i++) {
		c = memReadRAM(i);
		if (!c) break;

		if ((c >= 0x30) && (c <= 0x39) ) {
			result = result * 10 + (c - 0x30);
		} else if (result > 0) {
			break;
		}
	}
	return result;
}

void startFromBasic(uint16_t* init_addr, uint8_t roms_available, int32_t load_size) {
	if (_basic_prog) {
		if (roms_available) {
			// todo: when actual ROMS are available, then the regular RESET might always be used
			// to make sure that RAM is is consistently initialized
			(*init_addr) = 0xa7ae;	// BASIC "next statement"
		} else {
#ifdef EMSCRIPTEN
			EM_ASM_({ console.log("FATAL ERROR: This BASIC song requires emulator to be configured with optional KERNAL ROM and BASIC ROM");});
#else
			fprintf(stderr, "FATAL ERROR: This BASIC song requires emulator to be configured with optional KERNAL ROM and BASIC ROM\n");
#endif
		}
	} else {

		if ((*init_addr) == 0x0801) {	// typical garbage: points to BASIC tokens..
			uint16_t nextLine = memReadRAM(*init_addr) | ((memReadRAM((*init_addr) + 1)) << 8);
			if (!memReadRAM(nextLine)) {
				// one line program	(bad luck if additional REM etc lines are used..)
				if (memReadRAM(((*init_addr) + 4)) == 0x9e) {	// command is SYS
/*					if (roms_available) {
						// use of regular reset should reduce the risk of any flawed RAM initialization
						(*init_addr) = 0xa7ae;	// BASIC "next statement"
					} else {	*/
						// don't have C64 ROMs. if BASIC program is just used to
						// jump to some address (SYS $....) then try to do that for
						// simple one line programs..
						(*init_addr) = parseSYS((*init_addr) + 5, nextLine);
//					}
				}
			}
		}
	}
}

uint32_t SidFileLoader::load(uint8_t* input_file_buffer, uint32_t in_buf_size, const char* filename,
							void* basic_ROM, void* char_ROM, void* kernal_ROM, int enable_MD5) {

	init();

	_md5_hex_string[0] = 0;

	memResetKernelROM((uint8_t*)kernal_ROM);

	uint8_t is_rsid = (input_file_buffer[0x00] != 0x50) ? 1 : 0;
	setRsidMode(is_rsid);

	_sid_file_version = input_file_buffer[0x05];

	uint16_t flags = (_sid_file_version > 1) ?
					(((uint16_t)input_file_buffer[0x77]) | (((uint16_t)input_file_buffer[0x77]) << 8)) : 0x0;

	uint8_t ntsc_mode = (_sid_file_version >= 2) && (flags & 0x8); // NTSC bit
	FileLoader::setNTSCMode(ntsc_mode);

	_compatibility = ( (_sid_file_version & 0x2) &&  ((flags & 0x2) == 0));

	// dummy env that still may be overridden by hard RESET used for BASIC progs
	memResetRAM(ntsc_mode, !is_rsid);

	// note: _basic_prog is one of those idiotic .sid-file-format flags that
	// are pretty useless.. it is kind of a tag for those songs that are completely
	// written in BASIC. But other songs may also use BASIC statements (e.g. for
	// some SYS xxxx startup call) or they may just use utility BASIC ROM sub-routines
	// (testcase: MiniSEQ_Demo_Songs.sid)
	_basic_prog= (is_rsid && (flags & 0x2));	// a C64 BASIC program needs to be started..

	// it seems that any .sid is entitled to use BASIC ROM routines
	// without setting the _basic_prog flag
	memResetBasicROM((uint8_t *)basic_ROM);

//	if (_basic_prog) {
		// only  needed for BASIC progs
		if (basic_ROM && kernal_ROM) {
			Core::callKernalROMReset();
		}
//	}

	if (is_rsid) memResetCharROM((uint8_t *)char_ROM);

	FileLoader::configureSids(flags, &(input_file_buffer[0x7a]));

	strncpy(_song_name,      (const char*)input_file_buffer + 0x16, 32);
	strncpy(_song_author,    (const char*)input_file_buffer + 0x36, 32);
	strncpy(_song_copyright, (const char*)input_file_buffer + 0x56, 32);

	int32_t load_size = 0;

	// loading of song binary is the last step in the general memory initialization
	if (!loadSIDFromMemory(input_file_buffer, &_load_addr, &_load_end_addr, &_init_addr,
			&_play_addr, &_max_track, &_selected_track, &_play_speed,
			in_buf_size, &load_size, _basic_prog && basic_ROM && kernal_ROM)) {

		return 1;	// could not load file
	}

	//if (_basic_prog) // seems that a song may use BASIC without having this flag set...
	if (is_rsid)
		startFromBasic(&_init_addr, basic_ROM && kernal_ROM, load_size);

//	EM_ASM_({ console.log('START:  $' + ($0).toString(16));}, _init_addr);

	/*if(enable_MD5)
	{
		// md5 key used to make lookups in HVSC Songlength.md5 DB..
		MD5Init(&_md5);
		MD5Update(&_md5, input_file_buffer, in_buf_size);
		MD5Final (&_md5);

		uint8_t *b = _md5.digest;
		snprintf(_md5_hex_string, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]);
	}*/

	storeFileInfo();
	return 0;
}
