
#include "psflib.h"
#include "usf.h"

static void * psf_file_fopen( const char * uri );
static void * psf_file_fopen( const char * uri );
static int psf_file_fseek( void * handle, int64_t offset, int whence );
static int psf_file_fclose( void * handle );
static size_t psf_file_fread( void * buffer, size_t size, size_t count, void * handle );
static long psf_file_ftell( void * handle );

inline unsigned get_be16( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [0] << 8 |
            (unsigned) ((unsigned char const*) p) [1];
}

inline unsigned get_le32( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [3] << 24 |
            (unsigned) ((unsigned char const*) p) [2] << 16 |
            (unsigned) ((unsigned char const*) p) [1] <<  8 |
            (unsigned) ((unsigned char const*) p) [0];
}

inline unsigned get_be32( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [0] << 24 |
            (unsigned) ((unsigned char const*) p) [1] << 16 |
            (unsigned) ((unsigned char const*) p) [2] <<  8 |
            (unsigned) ((unsigned char const*) p) [3];
}

inline void set_le32( void* p, unsigned n )
{
    ((unsigned char*) p) [0] = (unsigned char) n;
    ((unsigned char*) p) [1] = (unsigned char) (n >> 8);
    ((unsigned char*) p) [2] = (unsigned char) (n >> 16);
    ((unsigned char*) p) [3] = (unsigned char) (n >> 24);
} 

const psf_file_callbacks psf_file_system =
{
	"\\/|:",
	psf_file_fopen,
	psf_file_fread,
	psf_file_fseek,
	psf_file_fclose,
	psf_file_ftell
};

static void * psf_file_fopen( const char * uri )
{
	FILE *f;
	f = fopen(uri, "r");
	return f;
}

static size_t psf_file_fread( void * buffer, size_t size, size_t count, void * handle )
{
	size_t bytes_read = fread(buffer,size,count,(FILE*)handle);
	return bytes_read / size;
}

static int psf_file_fseek( void * handle, int64_t offset, int whence )
{
	int result = fseek((FILE*)handle, offset, whence);
	return result;
}

static int psf_file_fclose( void * handle )
{
	fclose((FILE*)handle);
	return 0;
}

static long psf_file_ftell( void * handle )
{
	long pos = ftell((FILE*) handle);
	return pos;
}

struct usf_loader_state
{
	uint32_t enable_compare;
	uint32_t enable_fifo_full;
	void * emu_state;

	usf_loader_state()
		: enable_compare(0), enable_fifo_full(0),
		  emu_state(0)
	{ }

	~usf_loader_state()
	{
		if ( emu_state )
			free( emu_state );
	}
};
int usf_loader(void * context, const uint8_t * exe, size_t exe_size, const uint8_t * reserved, size_t reserved_size)
{
	struct usf_loader_state * state = ( struct usf_loader_state * ) context;
    if ( exe_size > 0 ) 
		return -1;

	return usf_upload_section( state->emu_state, reserved, reserved_size );
}

int usf_info(void * context, const char * name, const char * value)
{
	struct usf_loader_state * state = ( struct usf_loader_state * ) context;
    
    printf("%s %s\n",name,value);

	if ( strcasecmp( name, "_enablecompare" ) == 0 && strlen( value ) )
		state->enable_compare = 1;
	else if ( strcasecmp( name, "_enablefifofull" ) == 0 && strlen( value ) )
		state->enable_fifo_full = 1;

	return 0;
}
