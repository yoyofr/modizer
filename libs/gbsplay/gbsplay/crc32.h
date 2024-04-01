#ifndef _GBS_CRC32_H_
#define _GBS_CRC32_H_

#include <stddef.h>

unsigned long gbs_crc32(unsigned long crc, const char *buf, size_t len);

#endif
