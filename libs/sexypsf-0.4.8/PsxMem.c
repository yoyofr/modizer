/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2002  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <stdlib.h>

#include "PsxCommon.h"

void LoadPSXMem(u32 address, s32 length, char *data)
{
 while(length>0)
 {
  if(address&65535)
  {
   u32 tmplen;

   //puts("Squishy");
   tmplen=((65536-(address&65535))>length)?length:65536-(address&65535);
   if(psxMemLUT[address>>16])
    memcpy((psxMemLUT[address>>16]+(address&65535)),data,tmplen);
   address+=tmplen;
   data+=tmplen;
   length-=tmplen;
   //printf("%08x %08x\n",address,tmplen);
   continue;
  }
  if(psxMemLUT[address>>16])
  {
   memcpy((psxMemLUT[address>>16]),data,(length<65536)?length:65536);
  }
  data+=65536;
  address+=65536;
  length-=65536;
 }
}

static int writeok;
int psxMemInit() {
	int i;

	writeok=1;

	psxMemLUT = malloc( 0x10000 * sizeof *psxMemLUT);
	memset(psxMemLUT, 0, 0x10000 * sizeof *psxMemLUT);

	psxM = (s8 *)malloc( 0x00200000);
	psxP = (s8 *)malloc( 0x00010000);
	psxH = (s8 *)malloc( 0x00010000);

	psxR = (s8 *)malloc( 0x00080000);
	if (psxMemLUT == NULL || psxM == NULL || psxP == NULL || psxH == NULL || psxR == NULL) {
		return -1;
	}

	for (i=0; i<0x80; i++) psxMemLUT[i + 0x0000] = (char*)(&psxM[(i & 0x1f) << 16]);

	memcpy(psxMemLUT + 0x8000, psxMemLUT, 0x80 * sizeof *psxMemLUT);
	memcpy(psxMemLUT + 0xa000, psxMemLUT, 0x80 * sizeof *psxMemLUT);

	for (i=0; i<0x01; i++) psxMemLUT[i + 0x1f00] = (char*)(&psxP[i << 16]);

	for (i=0; i<0x01; i++) psxMemLUT[i + 0x1f80] = (char*)(&psxH[i << 16]);

	for (i=0; i<0x08; i++) psxMemLUT[i + 0xbfc0] = (char*)(&psxR[i << 16]);

	return 0;
}

void psxMemReset() {
	memset(psxM, 0, 0x00200000);
	memset(psxP, 0, 0x00010000);
}

void psxMemShutdown() {
	free(psxM);
	free(psxP);
	free(psxH);
	free(psxR);
	free(psxMemLUT);
}

u8 psxMemRead8(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80) {
		if (mem < 0x1f801000)
			return psxHu8(mem);
		else
			return psxHwRead8(mem);
	} else {
		p = (char *)(psxMemLUT[t]);
		if (p != NULL) {
			return *(u8 *)(p + (mem & 0xffff));
		} else {
			return 0;
		}
	}
}

s8 psxMemReadS8(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80) {
		if (mem < 0x1f801000)
			return (s8)psxHu8(mem);
		else
			return (s8)psxHwRead8(mem);
	} else {
		p = (char *)(psxMemLUT[t]);
		if (p != NULL) {
			return *(s8 *)(p + (mem & 0xffff));
		} else {
			return 0;
		}
	}
}

u16 psxMemRead16(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80) {
		if (mem < 0x1f801000)
			return BFLIP16(psxHu16(mem));
		else
			return psxHwRead16(mem);
	} else {
		p = (char *)(psxMemLUT[t]);
		if (p != NULL) {
			return BFLIP16(*(u16 *)(p + (mem & 0xffff)));
		} else {
			return 0;
		}
	}
}

s16 psxMemReadS16(u32 mem) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80) {
		if (mem < 0x1f801000)
			return (s16)BFLIP16(psxHu16(mem));
		else
			return (s16)psxHwRead16(mem);
	} else {
		p = (char *)(psxMemLUT[t]);
		if (p != NULL) {
			return BFLIP16(*(s16 *)(p + (mem & 0xffff)));
		} else {
			return 0;
		}
	}
}

#define LE32(p) ((p)[0] + ((p)[1] << 8) + ((p)[2] << 16) + ((p)[3] << 24))
u32 psxMemRead32(u32 mem) {
	u32 t=mem>>16;
	u32 m=mem&0xFFFF;
	if (t!=0x1f80)
		// UNALIGNED READ in ff6 and ct soundtracks :(
		//return BFLIP32(*((u32*)(((u8*)psxMemLUT[t])+m)));
		return LE32((u8 *)psxMemLUT[t]+m);
	else if (m<0x1000)
		return BFLIP32(*((u32*)(((u8*)psxH)+m)));
	else
		return (u32)psxHwRead32(mem);
}

void psxMemWrite8(u32 mem, u8 value) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80) {
		if (mem < 0x1f801000)
			psxHu8(mem) = value;
		else
			psxHwWrite8(mem, value);
	} else {
		p = (char *)(psxMemLUT[t]);
		if (p != NULL) {
			*(u8  *)(p + (mem & 0xffff)) = value;
		}
	}
}

#define SETLE16(p, v) ((p)[0] = (v), (p)[1] = (v) >> 8)
void psxMemWrite16(u32 mem, u16 value) {
	char *p;
	u32 t;

	t = mem >> 16;
	if (t == 0x1f80) {
		if (mem < 0x1f801000)
			psxHu16(mem) = BFLIP16(value);
		else
			psxHwWrite16(mem, value);
	} else {
		p = (char *)(psxMemLUT[t]);
		if (p != NULL) {
			// UNALIGNED WRITES in Suikoden
			//*(u16 *)(p + (mem & 0xffff)) = BFLIP16(value);
			SETLE16((p + (mem & 0xffff)), value);
		}
	}
}

#define SETLE32(p, v) ((p)[0] = (v), (p)[1] = (v) >> 8, (p)[2] = (v) >> 16, (p)[3] = (v) >> 24)
void psxMemWrite32(u32 mem, u32 value) {
	char *p;
	u32 t;

//	if ((mem&0x1fffff) == 0x71E18 || value == 0x48088800) SysPrintf("t2fix!!\n");
	t = mem >> 16;
	if (t == 0x1f80) {
		if (mem < 0x1f801000)
			psxHu32(mem) = BFLIP32(value);
		else
			psxHwWrite32(mem, value);
	} else {
		p = (char *)(psxMemLUT[t]);
		if (p != NULL) {
			// UNALIGNED WRITES in Brave Fencer Musashi
			//*(u32 *)(p + (mem & 0xffff)) = BFLIP32(value);
			SETLE32(p + (mem & 0xffff), value);
		} else {
			if (mem != 0xfffe0130) {

			} else {
				int i;

				switch (value) {
					case 0x800: case 0x804:
						if (writeok == 0) break;
						writeok = 0;
						memset(psxMemLUT + 0x0000, 0, 0x80 * sizeof *psxMemLUT);
						memset(psxMemLUT + 0x8000, 0, 0x80 * sizeof *psxMemLUT);
						memset(psxMemLUT + 0xa000, 0, 0x80 * sizeof *psxMemLUT);
						break;
					case 0x1e988:
						if (writeok == 1) break;
						writeok = 1;
						for (i=0; i<0x80; i++) psxMemLUT[i + 0x0000] = (char*)(&psxM[(i & 0x1f) << 16]);
						memcpy(psxMemLUT + 0x8000, psxMemLUT, 0x80 * sizeof *psxMemLUT);
						memcpy(psxMemLUT + 0xa000, psxMemLUT, 0x80 * sizeof *psxMemLUT);
						break;
					default:
						break;
				}
			}
		}
	}
}

