/***************************************************************************
                            dma.c  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

//*************************************************************************//
// History of changes:
//
// 2002/05/15 - Pete
// - generic cleanup for the Peops release
//
//*************************************************************************//

#include "stdafx.h"

#define _IN_DMA

//#include "externals.h"
////////////////////////////////////////////////////////////////////////
// READ DMA (many values)
////////////////////////////////////////////////////////////////////////

void  sexySPUreadDMAMem(u32 * pusPSXMem,int iSize)
{
	if (spuAddr+(iSize<<1)>=0x80000)
 	{
 		memcpy(pusPSXMem,&spuMem[spuAddr>>1],0x7ffff-spuAddr+1);
		memcpy(pusPSXMem+(0x7ffff-spuAddr+1),spuMem,(iSize<<1)-(0x7ffff-spuAddr+1));
		spuAddr=(iSize<<1)-(0x7ffff-spuAddr+1);
	} else {
		memcpy(pusPSXMem,&spuMem[spuAddr>>1],iSize<<1);
		spuAddr+=(iSize<<1);
	}
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

// to investigate: do sound data updates by writedma affect spu
// irqs? Will an irq be triggered, if new data is written to
// the memory irq address?

////////////////////////////////////////////////////////////////////////
// WRITE DMA (many values)
////////////////////////////////////////////////////////////////////////

void  sexySPUwriteDMAMem(unsigned short * pusPSXMem,int iSize)
{
	if (spuAddr+(iSize<<1)>0x7ffff)
	{
 		memcpy(&spuMem[spuAddr>>1],pusPSXMem,0x7ffff-spuAddr+1);
		memcpy(spuMem,pusPSXMem+(0x7ffff-spuAddr+1),(iSize<<1)-(0x7ffff-spuAddr+1));
		spuAddr=(iSize<<1)-(0x7ffff-spuAddr+1);
  	} else {
  		memcpy(&spuMem[spuAddr>>1],pusPSXMem,iSize<<1);
  		spuAddr+=(iSize<<1);
  	}
}

////////////////////////////////////////////////////////////////////////

