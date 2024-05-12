#pragma once

#include <stdint.h>

#define WSR_HEADER_MAGIC 0x46525357

typedef struct
{
	uint8_t * ptr;
	uint32_t len;
	uint32_t fil;
	uint32_t cur;
} WSRPlayerSoundBuf;

static inline uint32_t ReadLE16(const uint8_t *pData)
{
	return (pData[0x01] << 8) | (pData[0x00] << 0);
}

static inline uint32_t ReadLE32(const uint8_t *pData)
{
	return (pData[0x03] << 24) | (pData[0x02] << 16) |	(pData[0x01] << 8) | (pData[0x00] << 0);
}
