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

#include "../peops/stdafx.h"

#define _IN_DMA

extern uint32 psx_ram[(2*1024*1024)/4];

//#include "externals.h"
////////////////////////////////////////////////////////////////////////
// READ DMA (many values)
////////////////////////////////////////////////////////////////////////

void SPUreadDMAMem(u32 usPSXMem,int iSize)
{
 int i;
 u16 *ram16 = (u16 *)&psx_ram[0];

 for(i=0;i<iSize;i++)
  {
   ram16[usPSXMem>>1]=spuMem[spuAddr>>1];		// spu addr got by writeregister
   usPSXMem+=2;
   spuAddr+=2;                                         // inc spu addr
   if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
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

void SPUwriteDMAMem(u32 usPSXMem,int iSize)
{
 int i;
 u16 *ram16 = (u16 *)&psx_ram[0];

 for(i=0;i<iSize;i++)
  {
//  printf("main RAM %x => SPU %x\n", usPSXMem, spuAddr);
   spuMem[spuAddr>>1] = ram16[usPSXMem>>1];
   usPSXMem+=2;                  			// spu addr got by writeregister
   spuAddr+=2;                                         // inc spu addr
   if(spuAddr>0x7ffff) spuAddr=0;                      // wrap
  }
}

////////////////////////////////////////////////////////////////////////

