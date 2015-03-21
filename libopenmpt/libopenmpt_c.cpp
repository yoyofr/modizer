/*
 * libopenmpt_c.cpp
 * ----------------
 * Purpose: libopenmpt C interface implementation
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#include "BuildSettings.h"

#include "libopenmpt_internal.h"
#include "libopenmpt.h"

#include "libopenmpt_impl.hpp"

#include <limits>
#include <stdexcept>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef NO_LIBOPENMPT_C

namespace openmpt {

static const char * strdup( const char * src ) {
	char * dst = (char*)std::malloc( std::strlen( src ) + 1 );
	if ( !dst ) {
		return NULL;
	}
	std::strcpy( dst, src );
	return dst;
}

class callbacks_streambuf : public std::streambuf {
public:
	callbacks_streambuf( openmpt_stream_callbacks callbacks_, void * stream_ ) : callbacks(callbacks_), stream(stream_), put_back(4096), buf_size(65536), buffer(buf_size) {
		return;
	}
private:
	int_type underflow() {
		if ( gptr() < egptr() ) {
			return traits_type::to_int_type( *gptr() );
		}
		char * base = &buffer.front();
		char * start = base;
		if ( eback() == base ) {
			std::size_t put_back_count = std::min<std::size_t>( put_back, egptr() - base );
			std::memmove( base, egptr() - put_back_count, put_back_count );
			start += put_back_count;
		}
		if ( !callbacks.read ) {
			return traits_type::eof();
		}
		std::size_t n = callbacks.read( stream, start, buffer.size() - ( start - base ) );
		if ( n == 0 ) {
			return traits_type::eof();
		}
		setg( base, start, start + n );
		return traits_type::to_int_type( *gptr() );
	}
	callbacks_streambuf( const callbacks_streambuf & );
	callbacks_streambuf & operator = ( const callbacks_streambuf & );
private:
	openmpt_stream_callbacks callbacks;
	void * stream;
	const std::size_t put_back;
	const std::size_t buf_size;
	std::vector<char> buffer;
}; // class callbacks_streambuf

class callbacks_istream : public std::istream {
private:
	callbacks_streambuf buf;
private:
	callbacks_istream( const callbacks_istream & );
	callbacks_istream & operator = ( const callbacks_istream & );
public:
	callbacks_istream( openmpt_stream_callbacks callbacks, void * stream ) : std::istream(&buf), buf(callbacks, stream) {
		return;
	}
	~callbacks_istream() {
		return;
	}
}; // class callbacks_istream

class logfunc_logger : public log_interface {
private:
	openmpt_log_func m_logfunc;
	void * m_user;
public:
	logfunc_logger( openmpt_log_func func, void * user ) : m_logfunc(func), m_user(user) {
		return;
	}
	virtual ~logfunc_logger() {
		return;
	}
	virtual void log( const std::string & message ) const {
		if ( m_logfunc ) {
			m_logfunc( message.c_str(), m_user );
		} else {
			openmpt_log_func_default( message.c_str(), m_user );
		}
	}
}; // class logfunc_logger

static std::string format_exception( const char * const function ) {
	std::string err;
	try {
		throw;
	} catch ( openmpt::exception & e ) {
		err += function;
		err += ": ";
		err += "ERROR: ";
		err += e.what();
	} catch ( std::exception & e ) {
		err += function;
		err += ": ";
		err += "INTERNAL ERROR: ";
		err += e.what();
	} catch ( ... ) {
		err += function;
		err += ": ";
		err += "UNKOWN INTERNAL ERROR";
	}
	return err;
}

} // namespace openmpt

extern "C" {

struct openmpt_module {
	openmpt_log_func logfunc;
	void * user;
	openmpt::module_impl * impl;
};

} // extern "C"

namespace openmpt {

static void do_report_exception( const char * const function, openmpt_log_func const logfunc = 0, void * const user = 0, openmpt::module_impl * const impl = 0 ) {
	const std::string message = format_exception( function );
	if ( impl ) {
		impl->PushToCSoundFileLog( message );
	} else if ( logfunc ) {
		logfunc( message.c_str(), user );
	} else {
		openmpt_log_func_default( message.c_str(), NULL );
	}
}

static void report_exception( const char * const function, openmpt_module * mod = 0 ) {
	do_report_exception( function, mod ? mod->logfunc : 0, mod ? mod->user : 0, mod ? mod->impl : 0 );
}

static void report_exception( const char * const function, openmpt_log_func const logfunc, void * const user ) {
	do_report_exception( function, logfunc, user );
}

namespace interface {

template < typename T >
void check_soundfile( T * mod ) {
	if ( !mod ) {
		throw openmpt::exception("module * not valid");
	}
}

template < typename T >
void check_pointer( T * p ) {
	if ( !p ) {
		throw openmpt::exception("null pointer");
	}
}

} // namespace interface

} // namespace openmpt

extern "C" {

uint32_t openmpt_get_library_version(void) {
	try {
		return openmpt::get_library_version();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__ );
	}
	return 0;
}

uint32_t openmpt_get_core_version(void) {
	try {
		return openmpt::get_core_version();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__ );
	}
	return 0;
}

void openmpt_free_string( const char * str ) {
	try {
		std::free( (void*)str );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__ );
	}
	return;
}

const char * openmpt_get_string( const char * key ) {
	try {
		if ( !key ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( openmpt::string::get( key ).c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__ );
	}
	return NULL;
}

const char * openmpt_get_supported_extensions(void) {
	try {
		std::string retval;
		bool first = true;
		std::vector<std::string> supported_extensions = openmpt::module_impl::get_supported_extensions();
		for ( std::vector<std::string>::iterator i = supported_extensions.begin(); i != supported_extensions.end(); ++i ) {
			if ( first ) {
				first = false;
			} else {
				retval += ";";
			}
			retval += *i;
		}
		return openmpt::strdup( retval.c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__ );
	}
	return NULL;
}

int openmpt_is_extension_supported( const char * extension ) {
	try {
		if ( !extension ) {
			return 0;
		}
		return openmpt::module_impl::is_extension_supported( extension ) ? 1 : 0;
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__ );
	}
	return 0;
}

void openmpt_log_func_default( const char * message, void * /*user*/ ) {
	fprintf( stderr, "openmpt: %s\n", message );
	fflush( stderr );
}

void openmpt_log_func_silent( const char * /*message*/, void * /*user*/ ) {
	return;
}

double openmpt_could_open_propability( openmpt_stream_callbacks stream_callbacks, void * stream, double effort, openmpt_log_func logfunc, void * user ) {
	try {
		openmpt::callbacks_istream istream( stream_callbacks, stream );
#ifdef LIBOPENMPT_ANCIENT_COMPILER
		return openmpt::module_impl::could_open_propability( istream, effort, std::tr1::shared_ptr<openmpt::logfunc_logger>( new openmpt::logfunc_logger( logfunc ? logfunc : openmpt_log_func_default, user ) ) );
#else
		return openmpt::module_impl::could_open_propability( istream, effort, std::make_shared<openmpt::logfunc_logger>( logfunc ? logfunc : openmpt_log_func_default, user ) );
#endif
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, logfunc, user );
	}
	return 0.0;
}

openmpt_module * openmpt_module_create( openmpt_stream_callbacks stream_callbacks, void * stream, openmpt_log_func logfunc, void * user, const openmpt_module_initial_ctl * ctls ) {
	try {
		openmpt_module * mod = (openmpt_module*)std::malloc( sizeof( openmpt_module ) );
		if ( !mod ) {
			throw std::bad_alloc();
		}
		mod->logfunc = logfunc ? logfunc : openmpt_log_func_default;
		mod->user = user;
		mod->impl = 0;
		try {
			std::map< std::string, std::string > ctls_map;
			if ( ctls ) {
				for ( const openmpt_module_initial_ctl * it = ctls; it->ctl; ++it ) {
					if ( it->value ) {
						ctls_map[ it->ctl ] = it->value;
					} else {
						ctls_map.erase( it->ctl );
					}
				}
			}
			openmpt::callbacks_istream istream( stream_callbacks, stream );
#ifdef LIBOPENMPT_ANCIENT_COMPILER
			mod->impl = new openmpt::module_impl( istream, std::tr1::shared_ptr<openmpt::logfunc_logger>( new openmpt::logfunc_logger( mod->logfunc, mod->user ) ), ctls_map );
#else
			mod->impl = new openmpt::module_impl( istream, std::make_shared<openmpt::logfunc_logger>( mod->logfunc, mod->user ), ctls_map );
#endif
			return mod;
		} catch ( ... ) {
			openmpt::report_exception( __FUNCTION__, mod );
		}
		delete mod->impl;
		mod->impl = 0;
		std::free( (void*)mod );
		mod = NULL;
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__ );
	}
	return NULL;
}

openmpt_module * openmpt_module_create_from_memory( const void * filedata, size_t filesize, openmpt_log_func logfunc, void * user, const openmpt_module_initial_ctl * ctls ) {
	try {
		openmpt_module * mod = (openmpt_module*)std::malloc( sizeof( openmpt_module ) );
		if ( !mod ) {
			throw std::bad_alloc();
		}
		mod->logfunc = logfunc ? logfunc : openmpt_log_func_default;
		mod->user = user;
		mod->impl = 0;
		try {
			std::map< std::string, std::string > ctls_map;
			if ( ctls ) {
				for ( const openmpt_module_initial_ctl * it = ctls; it->ctl; ++it ) {
					if ( it->value ) {
						ctls_map[ it->ctl ] = it->value;
					} else {
						ctls_map.erase( it->ctl );
					}
				}
			}
#ifdef LIBOPENMPT_ANCIENT_COMPILER
			mod->impl = new openmpt::module_impl( filedata, filesize, std::tr1::shared_ptr<openmpt::logfunc_logger>( new openmpt::logfunc_logger( mod->logfunc, mod->user ) ), ctls_map );
#else
			mod->impl = new openmpt::module_impl( filedata, filesize, std::make_shared<openmpt::logfunc_logger>( mod->logfunc, mod->user ), ctls_map );
#endif
			return mod;
		} catch ( ... ) {
			openmpt::report_exception( __FUNCTION__, mod );
		}
		delete mod->impl;
		mod->impl = 0;
		std::free( (void*)mod );
		mod = NULL;
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__ );
	}
	return NULL;
}

void openmpt_module_destroy( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		delete mod->impl;
		mod->impl = 0;
		std::free( (void*)mod );
		mod = NULL;
		return;
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return;
}

int openmpt_module_select_subsong( openmpt_module * mod, int32_t subsong ) {
	try {
		openmpt::interface::check_soundfile( mod );
		mod->impl->select_subsong( subsong );
		return 1;
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

int openmpt_module_set_repeat_count( openmpt_module * mod, int32_t repeat_count ) {
	try {
		openmpt::interface::check_soundfile( mod );
		mod->impl->set_repeat_count( repeat_count );
		return 1;
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
int32_t openmpt_module_get_repeat_count( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_repeat_count();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

double openmpt_module_get_duration_seconds( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_duration_seconds();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0.0;
}

double openmpt_module_set_position_seconds( openmpt_module * mod, double seconds ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->set_position_seconds( seconds );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0.0;
}
double openmpt_module_get_position_seconds( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_position_seconds();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0.0;
}

double openmpt_module_set_position_order_row( openmpt_module * mod, int32_t order, int32_t row ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->set_position_order_row( order, row );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0.0;
}

int openmpt_module_get_render_param( openmpt_module * mod, int param, int32_t * value ) {
	try {
		openmpt::interface::check_soundfile( mod );
		openmpt::interface::check_pointer( value );
		*value = mod->impl->get_render_param( (openmpt::module::render_param)param );
		return 1;
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
int openmpt_module_set_render_param( openmpt_module * mod, int param, int32_t value ) {
	try {
		openmpt::interface::check_soundfile( mod );
		mod->impl->set_render_param( (openmpt::module::render_param)param, value );
		return 1;
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

size_t openmpt_module_read_mono( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * mono ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->read( samplerate, count, mono );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
size_t openmpt_module_read_stereo( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * left, int16_t * right ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->read( samplerate, count, left, right );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
size_t openmpt_module_read_quad( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * left, int16_t * right, int16_t * rear_left, int16_t * rear_right ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->read( samplerate, count, left, right, rear_left, rear_right );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
size_t openmpt_module_read_float_mono( openmpt_module * mod, int32_t samplerate, size_t count, float * mono ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->read( samplerate, count, mono );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
size_t openmpt_module_read_float_stereo( openmpt_module * mod, int32_t samplerate, size_t count, float * left, float * right ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->read( samplerate, count, left, right );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
size_t openmpt_module_read_float_quad( openmpt_module * mod, int32_t samplerate, size_t count, float * left, float * right, float * rear_left, float * rear_right ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->read( samplerate, count, left, right, rear_left, rear_right );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
size_t openmpt_module_read_interleaved_stereo( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * interleaved_stereo ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->read_interleaved_stereo( samplerate, count, interleaved_stereo );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
size_t openmpt_module_read_interleaved_quad( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * interleaved_quad ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->read_interleaved_quad( samplerate, count, interleaved_quad );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
size_t openmpt_module_read_interleaved_float_stereo( openmpt_module * mod, int32_t samplerate, size_t count, float * interleaved_stereo ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->read_interleaved_stereo( samplerate, count, interleaved_stereo );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
size_t openmpt_module_read_interleaved_float_quad( openmpt_module * mod, int32_t samplerate, size_t count, float * interleaved_quad ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->read_interleaved_quad( samplerate, count, interleaved_quad );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

const char * openmpt_module_get_metadata_keys( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		std::string retval;
		bool first = true;
		std::vector<std::string> metadata_keys = mod->impl->get_metadata_keys();
		for ( std::vector<std::string>::iterator i = metadata_keys.begin(); i != metadata_keys.end(); ++i ) {
			if ( first ) {
				first = false;
			} else {
				retval += ";";
			}
			retval += *i;
		}
		return openmpt::strdup( retval.c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return NULL;
}
const char * openmpt_module_get_metadata( openmpt_module * mod, const char * key ) {
	try {
		openmpt::interface::check_soundfile( mod );
		openmpt::interface::check_pointer( key );
		return openmpt::strdup( mod->impl->get_metadata( key ).c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return NULL;
}

LIBOPENMPT_API int32_t openmpt_module_get_current_speed( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_speed();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_current_tempo( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_tempo();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_current_order( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_order();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_current_pattern( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_pattern();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_current_row( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_row();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_current_playing_channels( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_playing_channels();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

LIBOPENMPT_API float openmpt_module_get_current_channel_vu_mono( openmpt_module * mod, int32_t channel ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_channel_vu_mono( channel );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0.0;
}
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_left( openmpt_module * mod, int32_t channel ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_channel_vu_left( channel );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0.0;
}
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_right( openmpt_module * mod, int32_t channel ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_channel_vu_right( channel );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0.0;
}
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_rear_left( openmpt_module * mod, int32_t channel ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_channel_vu_rear_left( channel );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0.0;
}
LIBOPENMPT_API float openmpt_module_get_current_channel_vu_rear_right( openmpt_module * mod, int32_t channel ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_current_channel_vu_rear_right( channel );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0.0;
}

LIBOPENMPT_API int32_t openmpt_module_get_num_subsongs( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_num_subsongs();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_num_channels( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_num_channels();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_num_orders( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_num_orders();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_num_patterns( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_num_patterns();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_num_instruments( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_num_instruments();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}
LIBOPENMPT_API int32_t openmpt_module_get_num_samples( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_num_samples();
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

LIBOPENMPT_API const char * openmpt_module_get_subsong_name( openmpt_module * mod, int32_t index ) {
	try {
		openmpt::interface::check_soundfile( mod );
		std::vector<std::string> names = mod->impl->get_subsong_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return NULL;
}
LIBOPENMPT_API const char * openmpt_module_get_channel_name( openmpt_module * mod, int32_t index ) {
	try {
		openmpt::interface::check_soundfile( mod );
		std::vector<std::string> names = mod->impl->get_channel_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return NULL;
}
LIBOPENMPT_API const char * openmpt_module_get_order_name( openmpt_module * mod, int32_t index ) {
	try {
		openmpt::interface::check_soundfile( mod );
		std::vector<std::string> names = mod->impl->get_order_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return NULL;
}
LIBOPENMPT_API const char * openmpt_module_get_pattern_name( openmpt_module * mod, int32_t index ) {
	try {
		openmpt::interface::check_soundfile( mod );
		std::vector<std::string> names = mod->impl->get_pattern_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return NULL;
}
LIBOPENMPT_API const char * openmpt_module_get_instrument_name( openmpt_module * mod, int32_t index ) {
	try {
		openmpt::interface::check_soundfile( mod );
		std::vector<std::string> names = mod->impl->get_instrument_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return NULL;
}
LIBOPENMPT_API const char * openmpt_module_get_sample_name( openmpt_module * mod, int32_t index ) {
	try {
		openmpt::interface::check_soundfile( mod );
		std::vector<std::string> names = mod->impl->get_sample_names();
		if ( names.size() >= (std::size_t)std::numeric_limits<int32_t>::max() ) {
			throw std::runtime_error("too many names");
		}
		if ( index < 0 || index >= (int32_t)names.size() ) {
			return openmpt::strdup( "" );
		}
		return openmpt::strdup( names[index].c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return NULL;
}

LIBOPENMPT_API int32_t openmpt_module_get_order_pattern( openmpt_module * mod, int32_t order ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_order_pattern( order );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

LIBOPENMPT_API int32_t openmpt_module_get_pattern_num_rows( openmpt_module * mod, int32_t pattern ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_pattern_num_rows( pattern );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

LIBOPENMPT_API uint8_t openmpt_module_get_pattern_row_channel_command( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, int command ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return mod->impl->get_pattern_row_channel_command( pattern, row, channel, command );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

LIBOPENMPT_API const char * openmpt_module_format_pattern_row_channel_command( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, int command ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return openmpt::strdup( mod->impl->format_pattern_row_channel_command( pattern, row, channel, command ).c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

LIBOPENMPT_API const char * openmpt_module_highlight_pattern_row_channel_command( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, int command ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return openmpt::strdup( mod->impl->highlight_pattern_row_channel_command( pattern, row, channel, command ).c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

LIBOPENMPT_API const char * openmpt_module_format_pattern_row_channel( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, size_t width, int pad ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return openmpt::strdup( mod->impl->format_pattern_row_channel( pattern, row, channel, width, pad ? true : false ).c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

LIBOPENMPT_API const char * openmpt_module_highlight_pattern_row_channel( openmpt_module * mod, int32_t pattern, int32_t row, int32_t channel, size_t width, int pad ) {
	try {
		openmpt::interface::check_soundfile( mod );
		return openmpt::strdup( mod->impl->highlight_pattern_row_channel( pattern, row, channel, width, pad ? true : false ).c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

const char * openmpt_module_get_ctls( openmpt_module * mod ) {
	try {
		openmpt::interface::check_soundfile( mod );
		std::string retval;
		bool first = true;
		std::vector<std::string> ctls = mod->impl->get_ctls();
		for ( std::vector<std::string>::iterator i = ctls.begin(); i != ctls.end(); ++i ) {
			if ( first ) {
				first = false;
			} else {
				retval += ";";
			}
			retval += *i;
		}
		return openmpt::strdup( retval.c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return NULL;
}

const char * openmpt_module_ctl_get( openmpt_module * mod, const char * ctl ) {
	try {
		openmpt::interface::check_soundfile( mod );
		openmpt::interface::check_pointer( ctl );
		return openmpt::strdup( mod->impl->ctl_get( ctl ).c_str() );
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return NULL;
}

int openmpt_module_ctl_set( openmpt_module * mod, const char * ctl, const char * value ) {
	try {
		openmpt::interface::check_soundfile( mod );
		openmpt::interface::check_pointer( ctl );
		openmpt::interface::check_pointer( value );
		mod->impl->ctl_set( ctl, value );
		return 1;
	} catch ( ... ) {
		openmpt::report_exception( __FUNCTION__, mod );
	}
	return 0;
}

} // extern "C"

#endif // NO_LIBOPENMPT_C
