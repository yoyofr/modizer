/*
	2SF Decoder: for playing Nintendo DS Sound Format files (.2SF/.MINI2SF).
	
	Based on kode54's: https://www.foobar2000.org/components/view/foo_input_vio2sf (version 0.24.16) 
	and on my earlier port of the similar GSF player.

	As compared to kode54's original 2sf.cpp, the code has been patched to NOT
	rely on any fubar2000 base impls.  (The migration must have messed up
	the original "silence suppression" impl - which seems to be rather crappy
	anyway and not worth the trouble - and it has just been disabled here.) 
	
	Note: kode54's various psflib based decoder impls (see gsf, PSX, etc) use a copy/paste
	approach that amounts to 90% of the respective impls. It just seems bloody
	stupid to not pull out the respective code into some common shared base impl - 
	but I really don't feel like cleaning up kode54's mess.	
		
	NOTE: This emulator seems to make a total mess of the big/little endian handling 
	(see mem.h where the respective impls do exactly the inverse of what their names suggest),
	i.e. it seems that in *some* cases the memory layout is inverse - but not 
	always (see LE_TO_LOCAL_32).
		
	DeSmuME v0.8.0 Copyright (C) 2006 yopyop Copyright (C) 2006-2007 DeSmuME team	
	foo_input_2sf Copyright Christopher Snowhill
	web port Copyright © 2019 Juergen Wothke. It is distributed under the same GPL license (NOTE:
	The GPL license DOES NOT apply to the example code in the "htdocs" folder which is
	merely provided as an example).
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdexcept>
#include <set>

#include <codecvt>
#include <locale>
#include <string>

#include <string.h>

#include "vio_psflib.h"
#include <zlib.h>

#include "../vio2sf/desmume/state.h"

#include "circular_buffer.h"

#define stricmp strcasecmp

// emulation logic seems to be hardcoded to this..
#define AUDIO_BUF_SIZE 512

void print_message(void * context, const char * message) {
	// for now just a dummy noop
}


// implemented on JavaScript side (also see callback.js) for "on-demand" file load:
extern "C" int ems_request_file(const char *filename);

// just some fillers to change kode54's below code as little as possible
class exception_io_data: public std::runtime_error {
public:
	exception_io_data(const char *what= "exception_io_data") : std::runtime_error(what) {}
};
int stricmp_utf8(std::string const& s1, const char* s2) {	
    return strcasecmp(s1.c_str(), s2);
}
int stricmp_utf8(const char* s1, const char* s2) {	
    return strcasecmp(s1, s2);
}
int stricmp_utf8_partial(std::string const& s1,  const char* s2) {
	std::string s1pref= s1.substr(0, strlen(s2));	
    return strcasecmp(s1pref.c_str(), s2);
}

# define strdup_(s)							      \
  (__extension__							      \
    ({									      \
      const char *__old = (s);						      \
      size_t __len = strlen (__old) + 1;				      \
      char *__new = (char *) malloc (__len);			      \
      (char *) memcpy (__new, __old, __len);				      \
    }))


#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

// callback defined elsewhere 
extern void ds_meta_set(const char * name, const char * value);


#define DB_FILE FILE
	
struct FileAccess_t {	
	void* (*fopen)( const char * uri );
	size_t (*fread)( void * buffer, size_t size, size_t count, void * handle );
	int (*fseek)( void * handle, int64_t offset, int whence );

	long int (*ftell)( void * handle );
	int (*fclose)( void * handle  );

	size_t (*fgetlength)( FILE * f);
};
static struct FileAccess_t *g_file= 0;

static void * vio_psf_file_fopen( const char * uri ) {
    void *file= g_file->fopen( uri );
	return file;
}
static size_t vio_psf_file_fread( void * buffer, size_t size, size_t count, void * handle ) {
    size_t ret= g_file->fread( buffer, size, count, handle );
	return ret;
}
static int vio_psf_file_fseek( void * handle, int64_t offset, int whence ) {
    int ret= g_file->fseek( handle, offset, whence );
	return ret;
}
static int vio_psf_file_fclose( void * handle ) {
    g_file->fclose( handle );
    return 0;	
}
static long vio_psf_file_ftell( void * handle ) {
    long ret= g_file->ftell( handle );
	return ret;
}
const vio_psf_file_callbacks psf_file_system = {
    "\\/|:",
    vio_psf_file_fopen,
    vio_psf_file_fread,
    vio_psf_file_fseek,
    vio_psf_file_fclose,
    vio_psf_file_ftell
};


//#define DBG(a) OutputDebugString(a)
#define DBG(a)

static unsigned int cfg_infinite= 0;
static unsigned int cfg_deflength= 170000;
static unsigned int cfg_deffade= 10000;
static unsigned int cfg_resampling_quality= 4;

// note: the "silence detection" logic is broken but with kode54's copy/paste approach to 
// code reuse I don't care to investigate/fix his crap in all the various (slightly different)
// implementations - so it is just disabled here
static unsigned int cfg_suppressopeningsilence = 0;
static unsigned int cfg_suppressendsilence = 0;
static unsigned int cfg_endsilenceseconds= 5;

static const char field_length[]="2sf_length";
static const char field_fade[]="2sf_fade";

#define BORK_TIME 0xC0CAC01A


// note: old impl already used for GSF (2sf.cpp uses some kind of updated version here)
static unsigned long parse_time_crap(const char *input)
{
    if (!input) return BORK_TIME;
    int len = strlen(input);
    if (!len) return BORK_TIME;
    int value = 0;
    {
        int i;
        for (i = len - 1; i >= 0; i--)
        {
            if ((input[i] < '0' || input[i] > '9') && input[i] != ':' && input[i] != ',' && input[i] != '.')
            {
                return BORK_TIME;
            }
        }
    }

    char * foo = strdup_( input );

    if ( !foo )
        return BORK_TIME;

    char * bar = foo;
    char * strs = bar + strlen( foo ) - 1;
    char * end;
    while (strs > bar && (*strs >= '0' && *strs <= '9'))
    {
        strs--;
    }
    if (*strs == '.' || *strs == ',')
    {
        // fraction of a second
        strs++;
        if (strlen(strs) > 3) strs[3] = 0;
        value = strtoul(strs, &end, 10);
        switch (strlen(strs))
        {
        case 1:
            value *= 100;
            break;
        case 2:
            value *= 10;
            break;
        }
        strs--;
        *strs = 0;
        strs--;
    }
    while (strs > bar && (*strs >= '0' && *strs <= '9'))
    {
        strs--;
    }
    // seconds
    if (*strs < '0' || *strs > '9') strs++;
    value += strtoul(strs, &end, 10) * 1000;
    if (strs > bar)
    {
        strs--;
        *strs = 0;
        strs--;
        while (strs > bar && (*strs >= '0' && *strs <= '9'))
        {
            strs--;
        }
        if (*strs < '0' || *strs > '9') strs++;
        value += strtoul(strs, &end, 10) * 60000;
        if (strs > bar)
        {
            strs--;
            *strs = 0;
            strs--;
            while (strs > bar && (*strs >= '0' && *strs <= '9'))
            {
                strs--;
            }
            value += strtoul(strs, &end, 10) * 3600000;
        }
    }
    free( foo );
    return value;
}

// hack: dummy impl to replace foobar2000 stuff
const int MAX_INFO_LEN= 10;

class file_info {
	double len;
	
	// no other keys implemented
	const char* sampleRate;
	const char* channels;
	
	std::vector<std::string> requiredLibs;
	
public:
	file_info() {
		sampleRate = (const char*)malloc(MAX_INFO_LEN);
		channels = (const char*)malloc(MAX_INFO_LEN);
	}
	~file_info() {
		free((void*)channels);
		free((void*)sampleRate);
	}
	void reset() {
		requiredLibs.resize(0);
	}
	std::vector<std::string> get_required_libs() {
		return requiredLibs;
	}
	void info_set_int(const char *tag, int value) {
		if (!stricmp_utf8(tag, "samplerate")) {
			snprintf((char*)sampleRate, MAX_INFO_LEN, "%d", value);
		} else if (!stricmp_utf8(tag, "channels")) {
			snprintf((char*)channels, MAX_INFO_LEN, "%d", value);
		}
		// who cares.. just ignore
	}
	const char *info_get(std::string &t) {
		const char *tag= t.c_str();
		
		if (!stricmp_utf8(tag, "samplerate")) {
			return sampleRate;
		} else if (!stricmp_utf8(tag, "channels")) {
			return channels;
		}
		return "unavailable";
	}

	void set_length(double l) {
		len= l;
	}
	
	void info_set_lib(std::string& tag, const char * value) {
		// EMSCRIPTEN depends on all libs being loaded before the song can be played!
		requiredLibs.push_back(std::string(value));	
	}

	// 
	unsigned int meta_get_count() { return 0;}
	unsigned int meta_enum_value_count(unsigned int i) { return 0; }
	const char* meta_enum_value(unsigned int i, unsigned int k) { return "dummy";}
	void meta_modify_value(unsigned int i, unsigned int k, const char *value) {}
	unsigned int info_get_count() {return 0;}
	const char* info_enum_name(unsigned int i) { return "dummy";}

	void info_set(const char * name, const char * value) {}
	void info_set(std::string& name, const char * value) {}
	const char* info_enum_value(unsigned int i) {return "dummy";}
	void info_set_replaygain(const char *tag, const char *value) {}
	void info_set_replaygain(std::string &tag, const char *value) {}
	void meta_add(const char *tag, const char *value) {}
	void meta_add(std::string &tag, const char *value) {}
};


static void info_meta_ansi( file_info & info )
{
/* FIXME eventually migrate original impl
 
	for ( unsigned i = 0, j = info.meta_get_count(); i < j; i++ )
	{
		for ( unsigned k = 0, l = info.meta_enum_value_count( i ); k < l; k++ )
		{
			const char * value = info.meta_enum_value( i, k );
			info.meta_modify_value( i, k, pfc::stringcvt::string_utf8_from_ansi( value ) );
		}
	}
	for ( unsigned i = 0, j = info.info_get_count(); i < j; i++ )
	{
		const char * name = info.info_enum_name( i );
		
		if ( name[ 0 ] == '_' )
			info.info_set( std::string( name ), pfc::stringcvt::string_utf8_from_ansi( info.info_enum_value( i ) ) );
	}
*/
}

struct vio2sf_psf_info_meta_state
{
	file_info * info;

	std::string name;

	bool utf8;

	int tag_song_ms;
	int tag_fade_ms;

    vio2sf_psf_info_meta_state()
		: info( 0 ), utf8( false ), tag_song_ms( 0 ), tag_fade_ms( 0 ) {}
};

static int psf_info_meta(void * context, const char * name, const char * value) {
	// typical tags: _lib, _enablecompare(on), _enableFIFOfull(on), fade, volume
	// game, genre, year, copyright, track, title, length(x:x.xxx), artist
	
	// FIXME: various "_"-settings are currently not used to configure the emulator
	
    vio2sf_psf_info_meta_state * state = ( vio2sf_psf_info_meta_state * ) context;

	std::string & tag = state->name;

	tag.assign(name);

	if (!stricmp_utf8(tag, "game"))
	{
		DBG("reading game as album");
		tag.assign("album");
	}
	else if (!stricmp_utf8(tag, "year"))
	{
		DBG("reading year as date");
		tag.assign("date");
	}

	if (!stricmp_utf8_partial(tag, "replaygain_"))
	{
		DBG("reading RG info");
		state->info->info_set_replaygain(tag, value);
	}
	else if (!stricmp_utf8(tag, "length"))
	{
		DBG("reading length");
		int temp = parse_time_crap(value);
		if (temp != BORK_TIME)
		{
			state->tag_song_ms = temp;
			state->info->info_set_int(field_length, state->tag_song_ms);
		}
	}
	else if (!stricmp_utf8(tag, "fade"))
	{
		DBG("reading fade");
		int temp = parse_time_crap(value);
		if (temp != BORK_TIME)
		{
			state->tag_fade_ms = temp;
			state->info->info_set_int(field_fade, state->tag_fade_ms);
		}
	}
	else if (!stricmp_utf8(tag, "utf8"))
	{
		state->utf8 = true;
	}
	else if (!stricmp_utf8_partial(tag, "_lib"))
	{
		DBG("found _lib");
		state->info->info_set_lib(tag, value);
	}

	// 2sf additions start
	else if (!stricmp_utf8(tag, "_frames"))
	{
		DBG("found _frames");
		state->info->info_set(tag, value);
	}
	else if (!stricmp_utf8(tag, "_clockdown"))
	{
		DBG("found legacy _clockdown");
		state->info->info_set(tag, value);
	}
	else if (!stricmp_utf8(tag, "_vio2sf_sync_type"))
	{
		DBG("found vio2sf sync type");
		state->info->info_set(tag, value);
	}
	else if (!stricmp_utf8(tag, "_vio2sf_arm9_clockdown_level"))
	{
		DBG("found vio2sf arm9 clockdown");
		state->info->info_set(tag, value);
	}
	else if (!stricmp_utf8(tag, "_vio2sf_arm7_clockdown_level"))
	{
		DBG("found vio2sf arm7 clockdown");
		state->info->info_set(tag, value);
	}	
	// 2sf additions end
	
	else if (tag[0] == '_')
	{
		DBG("found unknown required tag, failing");
		return -1;
	}
	else
	{
		state->info->meta_add( tag, value );
	}

	// handle description stuff elsewhere
	ds_meta_set(tag.c_str(), value);
	
	return 0;
}

inline unsigned get_le32( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [3] << 24 |
            (unsigned) ((unsigned char const*) p) [2] << 16 |
            (unsigned) ((unsigned char const*) p) [1] <<  8 |
            (unsigned) ((unsigned char const*) p) [0];
}

// 2sf addition start
struct twosf_loader_state
{	
	uint8_t * state;
	uint8_t * rom;
	size_t rom_size;
	size_t state_size;

	int initial_frames;
	int sync_type;
	int clockdown;
	int arm9_clockdown_level;
	int arm7_clockdown_level;

	twosf_loader_state()
		: rom(0), state(0), rom_size(0), state_size(0),
		initial_frames(-1), sync_type(0), clockdown(0),
		arm9_clockdown_level(0), arm7_clockdown_level(0)
	{
	}

	~twosf_loader_state()
	{
		if (rom) { free(rom); rom= 0; }
		if (state) { free(state); state= 0;}	
	}
};

// note: used both in "issave=0" and "issave=1" mode
static int load_twosf_map(struct twosf_loader_state *state, int issave, const unsigned char *udata, unsigned usize)
{
	if (usize < 8) return -1;

	unsigned char *iptr;
	size_t isize;
	unsigned char *xptr;
	unsigned xsize = get_le32(udata + 4);
	unsigned xofs = get_le32(udata + 0);
	if (issave)
	{
		iptr = state->state;
		isize = state->state_size;
		state->state = 0;
		state->state_size = 0;
	}
	else
	{
		iptr = state->rom;
		isize = state->rom_size;
		state->rom = 0;
		state->rom_size = 0;
	}
	if (!iptr)
	{
		size_t rsize = xofs + xsize;
		if (!issave)
		{
			rsize -= 1;
			rsize |= rsize >> 1;
			rsize |= rsize >> 2;
			rsize |= rsize >> 4;
			rsize |= rsize >> 8;
			rsize |= rsize >> 16;
			rsize += 1;
		}
		iptr = (unsigned char *) malloc(rsize + 10);	// note: cleaned up upon size change and "state deletion"
		if (!iptr)
			return -1;
		memset(iptr, 0, rsize + 10);
		isize = rsize;
	}
	else if (isize < xofs + xsize)
	{
		size_t rsize = xofs + xsize;
		if (!issave)
		{
			rsize -= 1;
			rsize |= rsize >> 1;
			rsize |= rsize >> 2;
			rsize |= rsize >> 4;
			rsize |= rsize >> 8;
			rsize |= rsize >> 16;
			rsize += 1;
		}
		xptr = (unsigned char *) realloc(iptr, xofs + rsize + 10);
		if (!xptr)
		{
			free(iptr);		// note: previous buffer is cleanup up
			return -1;
		}
		iptr = xptr;
		isize = rsize;
	}
	memcpy(iptr + xofs, udata + 8, xsize);
	if (issave)
	{
		state->state = iptr;
		state->state_size = isize;
	}
	else
	{
		state->rom = iptr;
		state->rom_size = isize;
	}
	return 0;
}


// note: only used in "issave=1" mode
static int load_twosf_mapz(struct twosf_loader_state *state, int issave, const unsigned char *zdata, unsigned zsize, unsigned zcrc)
{
	int ret;
	int zerr;
	uLongf usize = 8;
	uLongf rsize = usize;
	unsigned char *udata;
	unsigned char *rdata;

	udata = (unsigned char *) malloc(usize);
	if (!udata)
		return -1;

	while (Z_OK != (zerr = uncompress(udata, &usize, zdata, zsize)))
	{
		if (Z_MEM_ERROR != zerr && Z_BUF_ERROR != zerr)
		{
			free(udata);
			return -1;
		}
		if (usize >= 8)
		{
			usize = get_le32(udata + 4) + 8;
			if (usize < rsize)
			{
				rsize += rsize;
				usize = rsize;
			}
			else
				rsize = usize;
		}
		else
		{
			rsize += rsize;
			usize = rsize;
		}
		rdata = (unsigned char *) realloc(udata, usize);
		if (!rdata)
		{
			free(udata);
			return -1;
		}
		udata = rdata;
	}

	rdata = (unsigned char *) realloc(udata, usize);
	if (!rdata)
	{
		free(udata);
		return -1;
	}

	if (0)
	{
		uLong ccrc = crc32(crc32(0L, Z_NULL, 0), rdata, (uInt) usize);
		if (ccrc != zcrc)
			return -1;
	}

	ret = load_twosf_map(state, issave, rdata, (unsigned) usize);
	free(rdata);
	return ret;
}
static int twosf_loader(void * context, const uint8_t * exe, size_t exe_size,
						const uint8_t * reserved, size_t reserved_size)
{
	struct twosf_loader_state * state = ( struct twosf_loader_state * ) context;

	if ( exe_size >= 8 )
	{
		if ( load_twosf_map(state, 0, exe, (unsigned) exe_size) )
			return -1;
	}

	if ( reserved_size )
	{
		size_t resv_pos = 0;
		if ( reserved_size < 16 )
			return -1;
		while ( resv_pos + 12 < reserved_size )
		{
			unsigned save_size = get_le32(reserved + resv_pos + 4);
			unsigned save_crc = get_le32(reserved + resv_pos + 8);
			if (get_le32(reserved + resv_pos + 0) == 0x45564153)
			{
				if (resv_pos + 12 + save_size > reserved_size)
					return -1;
				if (load_twosf_mapz(state, 1, reserved + resv_pos + 12, save_size, save_crc))
					return -1;
			}
			resv_pos += 12 + save_size;
		}
	}

	return 0;
}

static int twosf_info(void * context, const char * name, const char * value)
{
	struct twosf_loader_state * state = ( struct twosf_loader_state * ) context;
	char *end;

	if ( !stricmp_utf8( name, "_frames" ) )
	{
		state->initial_frames = strtol( value, &end, 10 );
	}
	else if ( !stricmp_utf8( name, "_clockdown" ) )
	{
		state->clockdown = strtol( value, &end, 10 );
	}
	else if ( !stricmp_utf8( name, "_vio2sf_sync_type") )
	{
		state->sync_type = strtol( value, &end, 10 );
	}
	else if ( !stricmp_utf8( name, "_vio2sf_arm9_clockdown_level" ) )
	{
		state->arm9_clockdown_level = strtol( value, &end, 10 );
	}
	else if ( !stricmp_utf8( name, "_vio2sf_arm7_clockdown_level" ) )
	{
		state->arm7_clockdown_level = strtol( value, &end, 10 );
	}

	return 0;
}

// 2sf addition end

struct ds_running_state
{
	int16_t samples[AUDIO_BUF_SIZE * 2];
};

/*
	removed: inheritance, remove_tags(), retag(), etc
	
	note: global instance reused between songs
*/
class input_twosf
{
	twosf_loader_state *m_state;
	struct ds_running_state m_output;
	
	// 2sf specific state
	NDS_state *m_emu;
	
	std::string m_path;
	file_info m_info;
		
	// copy/paste standard play loop handling
	bool no_loop, eof;

	circular_buffer<signed short> silence_test_buffer;
//	signed short sample_buffer[AUDIO_BUF_SIZE*2];			// unused without silence detection..

	int err;
	int data_written,remainder,pos_delta,startsilence,silence;

	double ds_emu_pos;	// in seconds

	int song_len,fade_len;
	int tag_song_ms,tag_fade_ms;

//	bool do_filter, do_suppressendsilence;

public:
	input_twosf() : silence_test_buffer( 0 ), m_emu( 0 ), m_state(0) 
	{
		memset(&m_output, 0, sizeof(m_output));		
	}

    void set_no_loop(int value) {no_loop=value;}
    
	void resetPlayback() {
		ds_emu_pos = 0.;

		startsilence = silence = 0;

		eof = 0;
		err = 0;
		data_written = 0;
		remainder = 0;
		pos_delta = 0;
		no_loop = 1;

		calcfade();		
	}	

	void reset() {
		silence_test_buffer.reset();
		
		// redundant: see decode_initialize
		resetPlayback();

//		do_filter= do_suppressendsilence= false;
		song_len= fade_len= tag_song_ms= tag_fade_ms= 0;
		memset(&m_output, 0, sizeof(m_output));
		
		m_info.reset();
	}
	
	~input_twosf() {
		shutdown();
	}

	int32_t getCurrentPlayPosition() { return data_written*1000/getSamplesRate() + pos_delta; }
	int32_t getEndPlayPosition() { return tag_song_ms; }
	int32_t getSamplesRate() { return 44100; }	// seems 2SF is hardcoded to this..
	
	std::vector<std::string> splitpath(const std::string& str, 
								const std::set<char> &delimiters) {

		std::vector<std::string> result;

		char const* pch = str.c_str();
		char const* start = pch;
		for(; *pch; ++pch) {
			if (delimiters.find(*pch) != delimiters.end()) {
				if (start != pch) {
					std::string str(start, pch);
					result.push_back(str);
				} else {
					result.push_back("");
				}
				start = pch + 1;
			}
		}
		result.push_back(start);

		return result;
	}
	
	
	void shutdown()
	{
		if ( m_emu )
		{
			state_deinit( m_emu );
			free( m_emu );
			m_emu = NULL;
		}
	}

	int open(const char * p_path ) {
		reset();
		
		m_path = p_path;

        vio2sf_psf_info_meta_state info_state;
		info_state.info = &m_info;
		
		// INFO: info_state is what is later passed as "context" in the callbacks
		//       psf_info_meta then is the "target"
	

		//std::string_fast msgbuf;

		//int ret = psf_load(p_path, &psf_file_system, 0x24, 0, 0, psf_info_meta, &info_state, 0, print_message, &msgbuf);
		int ret = vio_psf_load(p_path, &psf_file_system, 0x24, 0, 0, psf_info_meta, &info_state, 0, print_message, 0);

		//console::print(msgbuf);
		//msgbuf.reset();
		
		if ( ret <= 0 )
			throw exception_io_data( "Not a 2SF file" );

		if ( !info_state.utf8 )
			info_meta_ansi( m_info );

		tag_song_ms = info_state.tag_song_ms;
		tag_fade_ms = info_state.tag_fade_ms;

		if (!tag_song_ms)
		{
			tag_song_ms = cfg_deflength;
			tag_fade_ms = cfg_deffade;
		}

		m_info.set_length( (double)( tag_song_ms + tag_fade_ms ) * .001 );
		m_info.info_set_int( "samplerate", 44100 );
		m_info.info_set_int( "channels", 2 );

// emscripten lib loading additions below: 		
		
		// song may depend on some lib-file(s) that first must be loaded! 
		// (enter "retry-mode" if something is missing)
		std::set<char> delims; delims.insert('\\'); delims.insert('/');
		
		std::vector<std::string> p = splitpath(m_path, delims);		
		std::string path= m_path.substr(0, m_path.length()-p.back().length());
		
		
		// make sure the file will be available in the FS when the song asks for it later..		
		std::vector<std::string>libs= m_info.get_required_libs();
		for (std::vector<std::string>::const_iterator iter = libs.begin(); iter != libs.end(); ++iter) {
			const std::string& libName= *iter;
			const std::string libFile= path + libName;

			/*int r= ems_request_file(libFile.c_str());	// trigger load & check if ready
			if (r <0) {
				return -1; // file not ready
			}*/
		}
		return 0;
	}

	void decode_initialize() {

		shutdown();

		if (m_state) {
			delete m_state;
		}
		m_state= new twosf_loader_state();

		// 2sf impl start
		m_emu = ( NDS_state * ) calloc( 1, sizeof(NDS_state) );
		if ( !m_emu )
			throw std::bad_alloc();

		if ( state_init( m_emu ) )
			throw std::bad_alloc();

		if ( !m_state->rom && !m_state->state )
		{
		//	std::string_fast msgbuf;

			int ret = vio_psf_load(m_path.c_str(), &psf_file_system, 0x24, twosf_loader, m_state, twosf_info, m_state, 1, print_message, 0);

		//	console::print(msgbuf);
		//	msgbuf.reset();

			if ( ret < 0 )
				throw exception_io_data( "Invalid 2SF" );

			if (!m_state->arm7_clockdown_level)
				m_state->arm7_clockdown_level = m_state->clockdown;
			if (!m_state->arm9_clockdown_level)
				m_state->arm9_clockdown_level = m_state->clockdown;
		}

		m_emu->dwInterpolation = cfg_resampling_quality;
		m_emu->dwChannelMute = 0;

		m_emu->initial_frames = m_state->initial_frames;
		m_emu->sync_type = m_state->sync_type;
		m_emu->arm7_clockdown_level = m_state->arm7_clockdown_level;
		m_emu->arm9_clockdown_level = m_state->arm9_clockdown_level;

		if ( m_state->rom ) {
			state_setrom( m_emu, m_state->rom, m_state->rom_size, 0 );
		}
		state_loadstate( m_emu, m_state->state, m_state->state_size );

		// 2sf impl end
		
		// -------------- copy/paste standard handling:
				
		resetPlayback();
		

		/* is broken anyway..
		
		do_suppressendsilence = !! cfg_suppressendsilence;

		unsigned skip_max = cfg_endsilenceseconds * 44100;

		if ( cfg_suppressopeningsilence ) // ohcrap
		{
			for (;;)
			{
				unsigned skip_howmany = skip_max - silence;
				unsigned unskippable = 0;

				// 2sf start
				if (skip_howmany > AUDIO_BUF_SIZE) skip_howmany = AUDIO_BUF_SIZE;
				state_render( m_emu, m_output.samples, skip_howmany );
				// 2sf end
				
				
				short * foo = m_output.samples;
				unsigned i;
				for ( i = 0; i < skip_howmany; ++i )
				{
					if ( foo[ 0 ] || foo[ 1 ] ) break;
					foo += 2;
				}
				silence += i;
				if ( i < skip_howmany )
				{
					remainder = skip_howmany - i + unskippable;
					memmove( m_output.samples, foo, remainder * sizeof( short ) * 2 );
					break;
				}
				if ( silence >= skip_max )
				{
					eof = true;
					break;
				}
			}

			startsilence += silence;
			silence = 0;
		}
		if ( do_suppressendsilence ) silence_test_buffer.resize( skip_max * 2 );
*/
	}
	
	// same (old) impl already used for GSF
	int decode_run( int16_t** output_buffer, uint16_t *output_samples) {

		if ( ( eof || err < 0 ) /*&& !silence_test_buffer.data_available() */) return false;

//		if ( no_loop && tag_song_ms && ( pos_delta + MulDiv( data_written, 1000, 44100 ) ) >= tag_song_ms + tag_fade_ms ) return false;
		if ( no_loop && tag_song_ms && ( data_written  >= (song_len + fade_len)) ) return false;

		unsigned int written = 0;

		int samples;

		/* actually responsible for noise/click at "song end" - but not needed anyway
		if ( no_loop )
		{
			samples = ( song_len + fade_len ) - data_written;
			if ( samples > AUDIO_BUF_SIZE ) samples = AUDIO_BUF_SIZE;
		}
		else
		*/
		{
			samples = AUDIO_BUF_SIZE;
		}

		short * ptr;
/* is broken anyway
		if ( do_suppressendsilence )
		{
			if ( !eof )
			{
				unsigned free_space = silence_test_buffer.free_space() / 2;
				while ( free_space )
				{

					unsigned samples_to_render;
					if ( remainder )
					{
						samples_to_render = remainder;
						remainder = 0;
					}
					else
					{
						// 2sf start
						samples_to_render = free_space;
						if (samples_to_render > AUDIO_BUF_SIZE) samples_to_render = AUDIO_BUF_SIZE;
						
						state_render( m_emu, m_output.samples, samples_to_render );
						// 2sf end						
					}
					silence_test_buffer.write( m_output.samples, samples_to_render * 2 );
					free_space -= samples_to_render;
					if ( remainder )
					{
						memmove( m_output.samples, m_output.samples + samples_to_render * 2, remainder * 4 );
					}
				}
			}

			if ( silence_test_buffer.test_silence() )
			{
				eof = true;
				return false;
			}

			written = silence_test_buffer.data_available() / 2;
		//	if ( written > AUDIO_BUF_SIZE ) written = AUDIO_BUF_SIZE;
		//	sample_buffer.grow_size( written * 2 );
		
//			silence_test_buffer.read( sample_buffer, written * 2 );
//			ptr = sample_buffer;
		}
		else
			*/
		{
			/* always 0 here - with all the commented code above
			if ( remainder )
			{
				written = remainder;		
				remainder = 0;
			}
			else
			*/
			{
				// 2sf start
				written = samples;
				state_render( m_emu, m_output.samples, written );
				// 2sf end
			}

			ptr = m_output.samples;
		}

// note: copy/paste standard impl
		ds_emu_pos += double( written ) / 44100.;

		int d_start, d_end;
		d_start = data_written;
		data_written += written;
		d_end = data_written;

		if ( tag_song_ms && (d_end > song_len) && no_loop )
		{
			short * foo = ptr;
			int n;
			for( n = d_start; n < d_end; ++n )
			{
				if ( n > song_len )
				{
					if ( n > (song_len + fade_len) )
					{						
						foo[ 0 ] = 0;
						foo[ 1 ] = 0;
					}
					else
					{
						int bleh = song_len + fade_len - n;
						foo[ 0 ] = MulDiv( foo[ 0 ], bleh, fade_len );
						foo[ 1 ] = MulDiv( foo[ 1 ], bleh, fade_len );
					}
				}
				foo += 2;
			}
		}
	
		*output_buffer = ptr;
		*output_samples= written;
		return true;
	}

	// reused old impl from GSF
	void decode_seek( double p_seconds ) {
		eof = false;

		double buffered_time = (double)(silence_test_buffer.data_available() / 2) / 44100.0;

		ds_emu_pos += buffered_time;

		silence_test_buffer.reset();

		if ( p_seconds < ds_emu_pos )
		{
			decode_initialize( );
		}
		unsigned int howmany = ( int )( ( p_seconds - ds_emu_pos )*44100 );

		// more abortable, and emu doesn't like doing huge numbers of samples per call anyway
		while ( howmany )
		{
			// 2sf start
			unsigned int samples = (howmany > AUDIO_BUF_SIZE) ? AUDIO_BUF_SIZE : howmany;
			state_render( m_emu, m_output.samples, samples );
			// 2sf end
						
			howmany -= samples;
		}

		data_written = 0;
		pos_delta = ( int )( p_seconds * 1000. );
		ds_emu_pos = p_seconds;

		calcfade();

	}
    
    void set_mute_mask(int mask) {
        m_emu->dwChannelMute=mask;
    }
private:
	double MulDiv(int ms, int sampleRate, int d) {
		return ((double)ms)*sampleRate/d;
	}

	void calcfade() {
		song_len=MulDiv(tag_song_ms-pos_delta,44100,1000);
		fade_len=MulDiv(tag_fade_ms,44100,1000);
	}
};
static input_twosf g_input_2sf;

// ------------------------------------------------------------------------------------------------------- 


void ds_boost_volume(unsigned char b) { /*noop*/}

int32_t ds_get_sample_rate() {
	return g_input_2sf.getSamplesRate();
}

int32_t ds_end_song_position() {
	// base for seeking
	return g_input_2sf.getEndPlayPosition();	// in ms
}

int32_t ds_current_play_position() {
	return g_input_2sf.getCurrentPlayPosition();
}

int ds_load_file(const char *uri) {
	try {
		
		int retVal= g_input_2sf.open(uri);
		if (retVal < 0) return retVal;	// trigger retry later
		
		g_input_2sf.decode_initialize();

		return 0;
	} catch(...) {
		return -1;
	}
}

void ds_set_mute_mask (int mask) {
    g_input_2sf.set_mute_mask(mask);
}

int16_t *available_buffer= 0;
uint16_t available_buffer_size= 0;

int ds_read(int16_t *output_buffer, uint16_t out_size) {
	uint16_t requested_size= out_size;
	
	while (out_size) {
		if (available_buffer_size) {
			if (available_buffer_size >= out_size) {
				memcpy(output_buffer, available_buffer, out_size<<2);
				available_buffer+= out_size<<2;
				available_buffer_size-= out_size;
				return requested_size;
			} else {
				memcpy(output_buffer, available_buffer, available_buffer_size<<2);
				available_buffer= 0;
				output_buffer+= available_buffer_size<<2;
				out_size-= available_buffer_size;
			}
		} else {
			if(!g_input_2sf.decode_run( &available_buffer, &available_buffer_size)) {								
				return 0; 	// end song (just ignore whatever output might actually be there)
			}
		}
	}
	return requested_size;
}

int ds_seek_position (int ms) {
	g_input_2sf.decode_seek( ((double)ms)/1000);
    return 0;
}

// use "regular" file ops - which are provided by Emscripten (just make sure all files are previously loaded)

void* em_fopen( const char * uri ) { 
	return (void*)fopen(uri, "r");
}
size_t em_fread( void * buffer, size_t size, size_t count, void * handle ) {
	return fread(buffer, size, count, (FILE*)handle );
}
int em_fseek( void * handle, int64_t offset, int whence ) {
	return fseek( (FILE*) handle, offset, whence );
}
long int em_ftell( void * handle ) {
	return  ftell( (FILE*) handle );
}
int em_fclose( void * handle  ) {
	return fclose( (FILE *) handle  );
}

size_t em_fgetlength( FILE * f) {
	int fd= fileno(f);
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;	
}	

void ds_setup (void) {
	if (!g_file) {
		g_file = (struct FileAccess_t*) malloc(sizeof( struct FileAccess_t ));
		
		g_file->fopen= em_fopen;
		g_file->fread= em_fread;
		g_file->fseek= em_fseek;
		g_file->ftell= em_ftell;
		g_file->fclose= em_fclose;		
		g_file->fgetlength= em_fgetlength;
	}
}

//yoyofr
void ds_set_loop(bool loop) {
    if (loop) g_input_2sf.set_no_loop(0);
    else g_input_2sf.set_no_loop(1);
}


