/* Extended Module Player
 * Copyright (C) 1996-2016 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * This file is part of the Extended Module Player and is distributed
 * under the terms of the GNU Lesser General Public License. See COPYING.LIB
 * for more information.
 */

#include <stdio.h>
#include "common.h"
#include "inflate.h"
#include "depacker.h"
#include "crc32.h"

/* See RFC1952 for further information */

/* The flag byte is divided into individual bits as follows:

   bit 0   FTEXT
   bit 1   FHCRC
   bit 2   FEXTRA
   bit 3   FNAME
   bit 4   FCOMMENT
   bit 5   reserved
   bit 6   reserved
   bit 7   reserved
*/

#define FLAG_FTEXT	(1 << 0)
#define FLAG_FHCRC	(1 << 1)
#define FLAG_FEXTRA	(1 << 2)
#define FLAG_FNAME	(1 << 3)
#define FLAG_FCOMMENT	(1 << 4)

struct member {
	uint8 id1;
	uint8 id2;
	uint8 cm;
	uint8 flg;
	uint32 mtime;
	uint8 xfl;
	uint8 os;
};

static int test_gzip(unsigned char *b)
{
	return b[0] == 31 && b[1] == 139;
}

static int decrunch_gzip(FILE *in, FILE *out)
{
	struct member member;
	int val, c;
	uint32 crc;

	libxmp_crc32_init_A();

	member.id1 = read8(in, NULL);
	member.id2 = read8(in, NULL);
	member.cm  = read8(in, NULL);
	member.flg = read8(in, NULL);
	member.mtime = read32l(in, NULL);
	member.xfl = read8(in, NULL);
	member.os  = read8(in, NULL);

	if (member.cm != 0x08) {
		return -1;
	}

	if (member.flg & FLAG_FEXTRA) {
		int xlen = read16l(in, NULL);
		if (fseek(in, xlen, SEEK_CUR) < 0) {
			return -1;
		}
	}

	if (member.flg & FLAG_FNAME) {
		do {
			c = read8(in, NULL);
		} while (c != 0);
	}

	if (member.flg & FLAG_FCOMMENT) {
		do {
			c = read8(in, NULL);
		} while (c != 0);
	}

	if (member.flg & FLAG_FHCRC) {
		read16l(in, NULL);
	}
	
	val = libxmp_inflate(in, out, &crc, 1);
	if (val != 0) {
		return -1;
	}

	/* Check CRC32 */
	val = read32l(in, NULL);
	if (val != crc) {
		return -1;
	}

	/* Check file size */
	val = read32l(in, NULL);
	if (val != ftell(out)) {
		return -1;
	}

	return 0;
}

struct depacker libxmp_depacker_gzip = {
	test_gzip,
	decrunch_gzip
};
