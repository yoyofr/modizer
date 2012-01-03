/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef STDC_HEADERS
#  include <string.h>
#elif HAVE_STRINGS_H
#  include <strings.h>
#endif /* stdc */

#include "timidity.h"

#if USE_ALTIVEC
#define sizeof_vector 16

/* vector version of memset(). */
void v_memset(void* destp, int c, size_t len)
{
    register int32 dest = (int)destp;

    /* If it's worth using altivec code */
    if( len >= 2*sizeof_vector ) { /* 32 byte */
        int i,xlen;
	int cccc[4] = { c, c, c, c, };
        vint32* destv;
	vint32  pat = *(vint32*) cccc; 
	
	/* first, write things to word boundary. */
	if(( xlen = (int)dest % sizeof_vector ) != 0) {
	    libc_memset(destp,c,xlen);
	    len  -= xlen;
	    dest += xlen;
	}

	/* this is the maion loop. */
	destv = (vint32*) dest;
	for( i = 0; i < len / sizeof_vector; i++ ) {
  	    destv[i] = pat;
	}
	dest += i * sizeof_vector;
	len %= sizeof_vector;
    }

    /* write remaining few bytes. */
    libc_memset((void*)dest,0,len);
}

/* a bit faster version. */
static vint32 vzero = (vint32)(0);
void v_memzero(void* destp, size_t len)
{
    register int32 dest = (int)destp;

    /* If it's worth using altivec code */
    if( len >= 2*sizeof_vector ) { /* 32 byte */
        int i,xlen;
        vint32* destv;
	
	/* first, write things to word boundary. */
	if(( xlen = (int)dest % sizeof_vector ) != 0) {
	    libc_memset(destp,0,xlen);
	    len  -= xlen;
	    dest += xlen;
	}

	/* this is the maion loop. */
	destv = (vint32*) dest;
	for( i = 0; i < len / sizeof_vector; i++ ) {
  	    destv[i] = vzero;
	}
	dest += i * sizeof_vector;
	len %= sizeof_vector;
    }

    /* write remaining few bytes. */
    libc_memset((void*)dest,0,len);
}


/* this is an alternate version of set_dry_signal() in reverb.c */
void v_set_dry_signal(void* destp, const int32* buf, int32 n)
{
    int i = 0;
    if( n>=8 ) {
        vint32* dest = (vint32*) destp;
	for( ; i < n / 4; i++) {
            dest[i] = vec_add(dest[i],((vint32*) buf)[i]);
	}
    }
    /* write remaining few bytes. */
    for( i *= 4; i < n; i++) {
        ((int32*) destp)[i] += buf[i];
    }
}
#endif /* USE_ALTIVEC */

#if ( defined(__BORLANDC__) && (__BORLANDC__ >= 1380) )
int32 imuldiv8(int32 a, int32 b) {
	_asm {
		mov eax, a
		mov edx, b
		imul edx
		shr eax, 8
		shl edx, 24
		or  eax, edx
	}
}

int32 imuldiv16(int32 a, int32 b) {
	_asm {
		mov eax, a
		mov edx, b
		imul edx
		shr eax, 16
		shl edx, 16
		or  eax, edx
	}
}

int32 imuldiv24(int32 a, int32 b) {
	_asm {
		mov eax, a
		mov edx, b
		imul edx
		shr eax, 24
		shl edx, 8
		or  eax, edx
	}
}

int32 imuldiv28(int32 a, int32 b) {
	_asm {
		mov eax, a
		mov edx, b
		imul edx
		shr eax, 28
		shl edx, 4
		or  eax, edx
	}
}
#endif
