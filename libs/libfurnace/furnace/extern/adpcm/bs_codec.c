/*
	Encode and decode algorithms for
	Brian Schmidt's ADPCM used in QSound DSP
	
	2018-2019 by superctr.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

// step ADPCM algorithm
static inline int16_t bs_step(int8_t step, int16_t* history, int16_t* step_size)
{
	static const int16_t adpcm_table[16] = {
		154, 154, 128, 102, 77, 58, 58, 58, // 2.4, 2.4, 2.0, 1.6, 1.2, 0.9, 0.9, 0.9
		58, 58, 58, 58, 77, 102, 128, 154   // 0.9, 0.9, 0.9, 0.9, 1.2, 1.6, 2.0, 2.4
	};
	
	int32_t scale = *step_size;
	int32_t delta = ((1+abs(step<<1)) * scale)>>1; // (0.5 + abs(step)) * scale
	int32_t out = *history;
	if(step <= 0)
		delta = -delta;
	out += delta;
	out = CLAMP(out, -32768,32767);
	
	scale = (scale * adpcm_table[8+step])>>6;
	*step_size = CLAMP(scale, 1, 2000);
	*history = out;

	return out;
}

// step high pass filter
static inline int16_t bs_hpf_step(int16_t in, int16_t* history, int32_t *state)
{
	int32_t out;
	*state = (*state>>2) + in - *history;
	*history = in;
	out = (*state>>1) + in;
	return CLAMP(out, -32768,32767);
}

// encode ADPCM with high pass filter. gets better results, i think
void bs_encode(int16_t *buffer,uint8_t *outbuffer,long len)
{
	long i;
	
	int16_t step_size = 10;
	int16_t history = 0;
	uint8_t buf_sample = 0, nibble = 0;
	int16_t filter_history = 0;
	int32_t filter_state = 0;
	
	for(i=0;i<len;i++)
	{
		int step = bs_hpf_step(*buffer++,&filter_history,&filter_state) - history;
		step = (step / step_size)>>1;
		step = CLAMP(step, -8, 7);
		if(nibble)
			*outbuffer++ = buf_sample | (step&15);
		else
			buf_sample = (step&15)<<4;
		nibble^=1;
		bs_step(step, &history, &step_size);
	}
}

void bs_decode(uint8_t *buffer,int16_t *outbuffer,long len)
{
	long i;
	
	int16_t step_size = 10;
	int16_t history = 0;
	uint8_t nibble = 0;
	
	for(i=0;i<len;i++)
	{
		int8_t step = (*(int8_t*)buffer)<<nibble;
		step >>= 4;
		if(nibble)
			buffer++;
		nibble^=4;
		*outbuffer++ = bs_step(step, &history, &step_size);
	}
}
