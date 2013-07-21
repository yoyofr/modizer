// Game_Music_Emu 0.6-pre. http://www.slack.net/~ant/

#include "Dual_Resampler.h"

/* Copyright (C) 2003-2008 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#include "blargg_source.h"

// TODO: fix this. hack since resampler holds back some output.
int const resampler_extra = 34;

int const stereo = 2;

Dual_Resampler::Dual_Resampler() { }

Dual_Resampler::~Dual_Resampler() { }

blargg_err_t Dual_Resampler::reset( int pairs )
{
	// expand allocations a bit
	RETURN_ERR( sample_buf.resize( (pairs + (pairs >> 2)) * 2 ) );
	resize( pairs );
	resampler_size = oversamples_per_frame + (oversamples_per_frame >> 2);
	RETURN_ERR( resampler.resize_buffer( resampler_size ) );
	resampler.clear();
	return blargg_ok;
}

void Dual_Resampler::resize( int pairs )
{
	int new_sample_buf_size = pairs * 2;
	//new_sample_buf_size = new_sample_buf_size / 4 * 4; // TODO: needed only for 3:2 downsampler
	if ( sample_buf_size != new_sample_buf_size )
	{
		if ( (unsigned) new_sample_buf_size > sample_buf.size() )
		{
			check( false );
			return;
		}
		sample_buf_size = new_sample_buf_size;
		oversamples_per_frame = int (pairs * resampler.rate()) * 2 + 2;
		clear();
	}
}

void Dual_Resampler::clear()
{
	buf_pos = sample_buf_size;
	resampler.clear();
}


void Dual_Resampler::play_frame_( Stereo_Buffer& stereo_buf, dsample_t out [] )
{
	int pair_count = sample_buf_size >> 1;
	blip_time_t blip_time = stereo_buf.center()->count_clocks( pair_count );
	int sample_count = oversamples_per_frame - resampler.written() + resampler_extra;
	
	int new_count = set_callback.f( set_callback.data, blip_time, sample_count, resampler.buffer() );
	assert( new_count < resampler_size );
	
	stereo_buf.end_frame( blip_time );
	assert( stereo_buf.samples_avail() == pair_count * 2 );
	
	resampler.write( new_count );
	
	int count = resampler.read( sample_buf.begin(), sample_buf_size );
	assert( count == sample_buf_size );
	
	mix_samples( stereo_buf, out );
	stereo_buf.left()->remove_samples( pair_count );
	stereo_buf.right()->remove_samples( pair_count );
	stereo_buf.center()->remove_samples( pair_count );
}

void Dual_Resampler::dual_play( int count, dsample_t out [], Stereo_Buffer& stereo_buf )
{
	// empty extra buffer
	int remain = sample_buf_size - buf_pos;
	if ( remain )
	{
		if ( remain > count )
			remain = count;
		count -= remain;
		memcpy( out, &sample_buf [buf_pos], remain * sizeof *out );
		out += remain;
		buf_pos += remain;
	}
	
	// entire frames
	while ( count >= sample_buf_size )
	{
		play_frame_( stereo_buf, out );
		out += sample_buf_size;
		count -= sample_buf_size;
	}
	
	// extra
	if ( count )
	{
		play_frame_( stereo_buf, sample_buf.begin() );
		buf_pos = count;
		memcpy( out, sample_buf.begin(), count * sizeof *out );
		out += count;
	}
}

void Dual_Resampler::mix_samples( Stereo_Buffer& stereo_buf, dsample_t out_ [] )
{
	// lol hax
	if ( ((Tracked_Blip_Buffer*)stereo_buf.left())->non_silent() | ((Tracked_Blip_Buffer*)stereo_buf.right())->non_silent() )
		mix_stereo( stereo_buf, out_ );
	else
		mix_mono( stereo_buf, out_ );
}

void Dual_Resampler::mix_mono( Stereo_Buffer& stereo_buf, dsample_t out_ [] )
{
	int const bass = BLIP_READER_BASS( *stereo_buf.center() );
	BLIP_READER_BEGIN( sn, *stereo_buf.center() );
	
	int count = sample_buf_size >> 1;
	BLIP_READER_ADJ_( sn, count );
	
	typedef dsample_t stereo_dsample_t [2];
	stereo_dsample_t* BLARGG_RESTRICT out = (stereo_dsample_t*) out_ + count;
	stereo_dsample_t const* BLARGG_RESTRICT in =
			(stereo_dsample_t const*) sample_buf.begin() + count;
	int offset = -count;
	int const gain = gain_;
	do
	{
		int s = BLIP_READER_READ_RAW( sn ) >> (blip_sample_bits - 16);
		BLIP_READER_NEXT_IDX_( sn, bass, offset );
		
		int l = (in [offset] [0] * gain >> gain_bits) + s;
		int r = (in [offset] [1] * gain >> gain_bits) + s;
		
		BLIP_CLAMP( l, l );
		out [offset] [0] = (blip_sample_t) l;
		
		BLIP_CLAMP( r, r );
		out [offset] [1] = (blip_sample_t) r;
	}
	while ( ++offset );
	
	BLIP_READER_END( sn, *stereo_buf.center() );
}

void Dual_Resampler::mix_stereo( Stereo_Buffer& stereo_buf, dsample_t out_ [] )
{
	int const bass = BLIP_READER_BASS( *stereo_buf.center() );
	BLIP_READER_BEGIN( snc, *stereo_buf.center() );
	BLIP_READER_BEGIN( snl, *stereo_buf.left() );
	BLIP_READER_BEGIN( snr, *stereo_buf.right() );
	
	int count = sample_buf_size >> 1;
	BLIP_READER_ADJ_( snc, count );
	BLIP_READER_ADJ_( snl, count );
	BLIP_READER_ADJ_( snr, count );
	
	typedef dsample_t stereo_dsample_t [2];
	stereo_dsample_t* BLARGG_RESTRICT out = (stereo_dsample_t*) out_ + count;
	stereo_dsample_t const* BLARGG_RESTRICT in =
			(stereo_dsample_t const*) sample_buf.begin() + count;
	int offset = -count;
	int const gain = gain_;
	do
	{
		int sc = BLIP_READER_READ_RAW( snc ) >> (blip_sample_bits - 16);
		int sl = BLIP_READER_READ_RAW( snl ) >> (blip_sample_bits - 16);
		int sr = BLIP_READER_READ_RAW( snr ) >> (blip_sample_bits - 16);
		BLIP_READER_NEXT_IDX_( snc, bass, offset );
		BLIP_READER_NEXT_IDX_( snl, bass, offset );
		BLIP_READER_NEXT_IDX_( snr, bass, offset );
		
		int l = (in [offset] [0] * gain >> gain_bits) + sl + sc;
		int r = (in [offset] [1] * gain >> gain_bits) + sr + sc;
		
		BLIP_CLAMP( l, l );
		out [offset] [0] = (blip_sample_t) l;
		
		BLIP_CLAMP( r, r );
		out [offset] [1] = (blip_sample_t) r;
	}
	while ( ++offset );
	
	BLIP_READER_END( snc, *stereo_buf.center() );
	BLIP_READER_END( snl, *stereo_buf.left() );
	BLIP_READER_END( snr, *stereo_buf.right() );
}
