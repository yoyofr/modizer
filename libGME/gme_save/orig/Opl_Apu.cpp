// Just wraps Nezplug's OPL sound core with Blip_Buffer-based resampling

#include "Opl_Apu.h"

#include "s_opl.h"
#include <string.h>

#include "blargg_source.h"

bool Opl_Apu::supported() { return true; }

Opl_Apu::Opl_Apu() { opl = 0; }

//static int min_amp = INT_MAX, max_amp = INT_MIN;

blargg_err_t Opl_Apu::init( int clock, int rate, blip_time_t period, type_t type )
{
	clock_  = clock;
	rate_   = rate;
	period_ = period;
	CHECK_ALLOC( opl = OPLSoundAlloc( type ) );
	
	set_output( 0 );
	volume( 1.0 );
	reset();
	return 0;
}

Opl_Apu::~Opl_Apu()
{
	if ( opl )
		opl->release( opl );
	
//dprintf( "%d, %d\n", min_amp, max_amp );
}

void Opl_Apu::reset()
{
	addr      = 0;
	next_time = 0;
	last_amp  = 0;
	
	opl->reset( opl, clock_, rate_ );
	opl->volume( opl, 0x80 );
}

void Opl_Apu::write_data( blip_time_t time, int data )
{
	if ( time > next_time )
		run_until( time );
	
	opl->write( opl, 0, addr );
	opl->write( opl, 1, data );
}

int Opl_Apu::read( int data )
{
	return opl->read( opl, data );
}

void Opl_Apu::run_until( blip_time_t end_time )
{
	require( end_time > next_time );
	
	Blip_Buffer* const output = this->output_;
	if ( !output )
	{
		next_time = end_time;
		return;
	}
	
	blip_time_t time = next_time;
	do
	{
		int amp = opl->synth( opl ) >> 11;
//if ( min_amp > amp ) min_amp = amp;
//if ( max_amp < amp ) max_amp = amp;
		
		int delta = amp - last_amp;
		if ( delta )
		{
			last_amp = amp;
			synth.offset_inline( time, delta, output );
		}
		time += period_;
	}
	while ( time < end_time );
	
	next_time = time;
}

void Opl_Apu::end_frame( blip_time_t time )
{
	if ( time > next_time )
		run_until( time );
	
	next_time -= time;
	assert( next_time >= 0 );
	
	if ( output_ )
		output_->set_modified();
}
