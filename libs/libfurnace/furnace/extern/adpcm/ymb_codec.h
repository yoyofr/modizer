#ifndef YM_CODEC_H
#define YM_CODEC_H

/*
	Encode and decode algorithms for
	Y8950/YM2608/YM2610 ADPCM-B
	
	2018 by superctr.
*/

#include <stdint.h>

/**
 * Given (len) amount of PCM samples in buffer,
 * return encoded ADPCM samples in outbuffer.
 * Output buffer should be at least (len/2) elements large.
 */
void ymb_encode(int16_t *buffer,uint8_t *outbuffer,long len);

/**
 * Given ADPCM samples in (buffer), return (len) amount of
 * decoded PCM samples in (outbuffer).
 * Output buffer should be at least (len*2) elements large.
 */
void ymb_decode(uint8_t *buffer,int16_t *outbuffer,long len);

#endif
