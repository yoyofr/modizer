// OPL FM sound chip emulator

#ifndef OPL_APU_H
#define OPL_APU_H

#include "blargg_common.h"
#include "Blip_Buffer.h"

struct KMIF_SOUND_DEVICE;

class Opl_Apu {
public:
// Basics

	// Which FM sound chip to emulate
	enum type_t { type_opll = 0x10, type_msxmusic = 0x11, type_smsfmunit = 0x12,
			type_vrc7 = 0x13, type_opl = 0x20, type_msxaudio = 0x21, type_opl2 = 0x22 };
	
	// Initializes emulator. Samples from chip are added to output buffer every period clocks.
	blargg_err_t init( int clock_rate, int sample_rate, blip_time_t period, type_t );
	
	// Sets buffer(s) to generate sound into, or 0 to mute. If only center is not 0,
	// output is mono.
	void set_output( Blip_Buffer* b, Blip_Buffer* = NULL, Blip_Buffer* = NULL ) { output_ = b; }
	
	// Writes data to address register
	void write_addr( int data )                 { addr = data; }
	
	// Emulates to time t, then writes data to current register
	void write_data( blip_time_t, int data );
	
	// Reads from current register
	int read( int data );
	
	// Emulates to time t, then subtracts t from the current time.
	// OK if previous write call had time slightly after t.
	void end_frame( blip_time_t t );
	
// More features
	
	// False if this just a dummy implementation
	static bool supported();
	
	// Resets sound chip
	void reset();
	
	// Sets overall volume, where 1.0 is normal
	void volume( double v )                     { synth.volume( 1.6 / 4096 * v ); }
	
	// Sets treble equalization
	void treble_eq( blip_eq_t const& eq )       { synth.treble_eq( eq ); }

private:
	// noncopyable
	Opl_Apu( const Opl_Apu& );
	Opl_Apu& operator = ( const Opl_Apu& );


// Implementation
public:
	Opl_Apu();
	~Opl_Apu();
	BLARGG_DISABLE_NOTHROW
	enum { osc_count = 1 };
	void set_output( int i, Blip_Buffer* b, Blip_Buffer* = NULL, Blip_Buffer* = NULL ) { output_ = b; }

private:
	Blip_Buffer* output_;
	KMIF_SOUND_DEVICE* opl;
	blip_time_t next_time;
	int last_amp;
	int addr;
	
	int clock_;
	int rate_;
	blip_time_t period_;
	
	Blip_Synth_Fast synth;
	
	void run_until( blip_time_t );
};

#endif
