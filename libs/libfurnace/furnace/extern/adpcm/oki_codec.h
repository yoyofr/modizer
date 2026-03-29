#ifndef OKI_CODEC_H
#define OKI_CODEC_H

/*
	Encode and decode algorithms for
	OKI ADPCM

	Only difference between MSM6295 and MSM6258 is that the nibbles are swapped.
	MSM6295 reads from MSB to LSB. MSM6258 reads from LSB to MSB.

	Dialogic 'VOX' PCM reads from MSB to LSB, therefore should use the MSM6295 functions.

	2019-2022 by superctr.
*/

#include <stdint.h>

/**
 * Given (len) amount of PCM samples in buffer,
 * return encoded ADPCM samples in outbuffer.
 * Output buffer should be at least (len/2) elements large.
 */
void oki_encode(int16_t *buffer,uint8_t *outbuffer,long len);
void oki6258_encode(int16_t *buffer,uint8_t *outbuffer,long len);

/**
 * Given ADPCM samples in (buffer), return (len) amount of
 * decoded PCM samples in (outbuffer).
 * Output buffer should be at least (len*2) elements large.
 */
void oki_decode(uint8_t *buffer,int16_t *outbuffer,long len);
void oki6258_decode(uint8_t *buffer,int16_t *outbuffer,long len);

#endif
