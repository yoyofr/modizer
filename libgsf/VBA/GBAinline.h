// -*- C++ -*-
// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

extern int loadedsize;

#ifndef VBA_GBAinline_H
#define VBA_GBAinline_H

#include "System.h"
#include "Port.h"
//#include "RTC.h"

//extern bool cpuSramEnabled;
//extern bool cpuFlashEnabled;
//extern bool cpuEEPROMEnabled;
//extern bool cpuEEPROMSensorEnabled;

#define CPUReadByteQuick(addr) \
  map[(addr)>>24].address[(addr) & map[(addr)>>24].mask]

#define CPUReadHalfWordQuick(addr) \
  READ16LE(((u16*)&map[(addr)>>24].address[(addr) & map[(addr)>>24].mask]))

#define CPUReadMemoryQuick(addr) \
  READ32LE(((u32*)&map[(addr)>>24].address[(addr) & map[(addr)>>24].mask]))

inline u32 CPUReadMemory(u32 address)
{

#ifdef DEV_VERSION
  if(address & 3) {  
    if(systemVerbose & VERBOSE_UNALIGNED_MEMORY) {
      log("Unaligned word read: %08x at %08x\n", address, armMode ?
          armNextPC - 4 : armNextPC - 2);
    }
  }
#endif
  
  u32 value;
  switch(address >> 24) {
  case 0:
    if(reg[15].I >> 24) {
      if(address < 0x4000) {
#ifdef DEV_VERSION
        if(systemVerbose & VERBOSE_ILLEGAL_READ) {
          log("Illegal word read: %08x at %08x\n", address, armMode ?
              armNextPC - 4 : armNextPC - 2);
        }
#endif
        
        value = READ32LE(((u32 *)&biosProtected));
      }
      else goto unreadable;
    } else
      value = READ32LE(((u32 *)&bios[address & 0x3FFC]));
    break;
  case 2:
    value = READ32LE(((u32 *)&workRAM[address & 0x3FFFC]));
    break;
  case 3:
    value = READ32LE(((u32 *)&internalRAM[address & 0x7ffC]));
    break;
  case 4:
    if((address < 0x4000400) && ioReadable[address & 0x3fc]) {
      if(ioReadable[(address & 0x3fc) + 2])
        value = READ32LE(((u32 *)&ioMem[address & 0x3fC]));
      else
        value = READ16LE(((u16 *)&ioMem[address & 0x3fc]));
    } else goto unreadable;
    break;
  case 5:
    value = READ32LE(((u32 *)&paletteRAM[address & 0x3fC]));
    break;
  case 6:
    value = READ32LE(((u32 *)&vram[address & 0x1fffc]));
    break;
  case 7:
    value = READ32LE(((u32 *)&oam[address & 0x3FC]));
    break;
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
    if((address & 0x1FFFFFC) > loadedsize)
		  value = 0xFFFFFFFF;
	else
          value = READ32LE(((u32 *)&rom[address&0x1FFFFFC]));
    break;    
  case 13:
//    if(cpuEEPROMEnabled)
      // no need to swap this
//      return eepromRead(address);
    goto unreadable;
 // case 14:
  //  if(cpuFlashEnabled | cpuSramEnabled)
      // no need to swap this
      //return flashRead(address);
    // default
  default:
  unreadable:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_ILLEGAL_READ) {
      log("Illegal word read: %08x at %08x\n", address, armMode ?
          armNextPC - 4 : armNextPC - 2);
    }
#endif
    
    //    if(ioMem[0x205] & 0x40) {
      if(armState) {
        value = CPUReadMemoryQuick(reg[15].I);
      } else {
        value = CPUReadHalfWordQuick(reg[15].I) |
          CPUReadHalfWordQuick(reg[15].I) << 16;
      }
      //  } else {
      //      value = *((u32 *)&bios[address & 0x3ffc]);
      //    }
      //        return 0xFFFFFFFF;
  }

  if(address & 3) {
#ifdef C_CORE
    int shift = (address & 3) << 3;
    value = (value >> shift) | (value << (32 - shift));
#else    
#ifdef __GNUC__
    asm("and $3, %%ecx;"
        "shl $3 ,%%ecx;"
        "ror %%cl, %0"
        : "=r" (value)
        : "r" (value), "c" (address));
#else
    __asm {
      mov ecx, address;
      and ecx, 3;
      shl ecx, 3;
      ror [dword ptr value], cl;
    }
#endif
#endif
  }
  return value;
}

extern u32 myROM[];

inline u32 CPUReadHalfWord(u32 address)
{
#ifdef DEV_VERSION      
  if(address & 1) {
    if(systemVerbose & VERBOSE_UNALIGNED_MEMORY) {
      log("Unaligned halfword read: %08x at %08x\n", address, armMode ?
          armNextPC - 4 : armNextPC - 2);
    }
  }
#endif
  
  u32 value;
  
  switch(address >> 24) {
  case 0:
    if (reg[15].I >> 24) {
      if(address < 0x4000) {
#ifdef DEV_VERSION
        if(systemVerbose & VERBOSE_ILLEGAL_READ) {
          log("Illegal halfword read: %08x at %08x\n", address, armMode ?
              armNextPC - 4 : armNextPC - 2);
        }
#endif
        value = READ16LE(((u16 *)&biosProtected[address&2]));
      } else goto unreadable;
    } else
      value = READ16LE(((u16 *)&bios[address & 0x3FFE]));
    break;
  case 2:
    value = READ16LE(((u16 *)&workRAM[address & 0x3FFFE]));
    break;
  case 3:
    value = READ16LE(((u16 *)&internalRAM[address & 0x7ffe]));
    break;
  case 4:
    if((address < 0x4000400) && ioReadable[address & 0x3fe])
      value =  READ16LE(((u16 *)&ioMem[address & 0x3fe]));
    else goto unreadable;
    break;
  case 5:
    value = READ16LE(((u16 *)&paletteRAM[address & 0x3fe]));
    break;
  case 6:
    value = READ16LE(((u16 *)&vram[address & 0x1fffe]));
    break;
  case 7:
    value = READ16LE(((u16 *)&oam[address & 0x3fe]));
    break;
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
    //if(address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8)
    //  value = rtcRead(address);
    //else
	  if((address & 0x1FFFFFE) > loadedsize)
		  value = 0xFFFF;
	  else
		  value = READ16LE(((u16 *)&rom[address & 0x1FFFFFE]));
    break;    
  case 13:
//    if(cpuEEPROMEnabled)
      // no need to swap this
//      return  eepromRead(address);
    goto unreadable;
 // case 14:
 //   if(cpuFlashEnabled | cpuSramEnabled)
      // no need to swap this
      //return flashRead(address);
    // default
  default:
  unreadable:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_ILLEGAL_READ) {
      log("Illegal halfword read: %08x at %08x\n", address, armMode ?
          armNextPC - 4 : armNextPC - 2);
    }
#endif
    extern bool cpuDmaHack;
    extern u32 cpuDmaLast;
    extern int cpuDmaCount;
    if(cpuDmaHack && cpuDmaCount) {
      value = (u16)cpuDmaLast;
    } else {
      if(armState) {
        value = CPUReadHalfWordQuick(reg[15].I + (address & 2));
      } else {
        value = CPUReadHalfWordQuick(reg[15].I);
      }
    }
    //    return value;
    //    if(address & 1)
    //      value = (value >> 8) | ((value & 0xFF) << 24);
    //    return 0xFFFF;
    break;
  }

  if(address & 1) {
    value = (value >> 8) | (value << 24);
  }
  
  return value;
}

inline u16 CPUReadHalfWordSigned(u32 address)
{
  u16 value = CPUReadHalfWord(address);
  if((address & 1))
    value = (s8)value;
  return value;
}

inline u8 CPUReadByte(u32 address)
{
  switch(address >> 24) {
  case 0:
    if (reg[15].I >> 24) {
      if(address < 0x4000) {
#ifdef DEV_VERSION
        if(systemVerbose & VERBOSE_ILLEGAL_READ) {
          log("Illegal byte read: %08x at %08x\n", address, armMode ?
              armNextPC - 4 : armNextPC - 2);
        }
#endif
        return biosProtected[address & 3];
      } else goto unreadable;
    }
    return bios[address & 0x3FFF];
  case 2:
    return workRAM[address & 0x3FFFF];
  case 3:
    return internalRAM[address & 0x7fff];
  case 4:
    if((address < 0x4000400) && ioReadable[address & 0x3ff])
      return ioMem[address & 0x3ff];
    else goto unreadable;
  case 5:
    return paletteRAM[address & 0x3ff];
  case 6:
    return vram[address & 0x1ffff];
  case 7:
    return oam[address & 0x3ff];
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
      if((address & 0x1FFFFFF) > loadedsize)
		  return 0xFF;
	  else
          return rom[address & 0x1FFFFFF];        
  case 13:
//    if(cpuEEPROMEnabled)
//      return eepromRead(address);
    goto unreadable;
  case 14:
  //  if(cpuSramEnabled | cpuFlashEnabled)
  //    return flashRead(address);
 /*   if(cpuEEPROMSensorEnabled) {
      switch(address & 0x00008f00) {
      case 0x8200:
        return systemGetSensorX() & 255;
      case 0x8300:
        return (systemGetSensorX() >> 8)|0x80;
      case 0x8400:
        return systemGetSensorY() & 255;
      case 0x8500:
        return systemGetSensorY() >> 8;
      }
    }*/
    // default
  default:
  unreadable:
#ifdef DEV_VERSION
        if(systemVerbose & VERBOSE_ILLEGAL_READ) {
          log("Illegal byte read: %08x at %08x\n", address, armMode ?
              armNextPC - 4 : armNextPC - 2);
        }
#endif
    
    if(armState) {
      return CPUReadByteQuick(reg[15].I+(address & 3));
    } else {
      return CPUReadByteQuick(reg[15].I+(address & 1));
    }
    //    return 0xFF;
    break;
  }
}

inline void CPUWriteMemory(u32 address, u32 value)
{
#ifdef DEV_VERSION
  if(address & 3) {
    if(systemVerbose & VERBOSE_UNALIGNED_MEMORY) {
      log("Unaliagned word write: %08x to %08x from %08x\n",
          value,
          address,
          armMode ? armNextPC - 4 : armNextPC - 2);
    }
  }
#endif
  
  switch(address >> 24) {
  case 0x02:
#ifdef SDL
    if(*((u32 *)&freezeWorkRAM[address & 0x3FFFC]))
      cheatsWriteMemory((u32 *)&workRAM[address & 0x3FFFC],
                        value,
                        *((u32 *)&freezeWorkRAM[address & 0x3FFFC]));
    else
#endif
      WRITE32LE(((u32 *)&workRAM[address & 0x3FFFC]), value);
    break;
  case 0x03:
#ifdef SDL
    if(*((u32 *)&freezeInternalRAM[address & 0x7ffc]))
      cheatsWriteMemory((u32 *)&internalRAM[address & 0x7FFC],
                        value,
                        *((u32 *)&freezeInternalRAM[address & 0x7ffc]));
    else
#endif
      WRITE32LE(((u32 *)&internalRAM[address & 0x7ffC]), value);
    break;
  case 0x04:
    CPUUpdateRegister((address & 0x3FC), value & 0xFFFF);
    CPUUpdateRegister((address & 0x3FC) + 2, (value >> 16));
    break;
  case 0x05:
    WRITE32LE(((u32 *)&paletteRAM[address & 0x3FC]), value);
    break;
  case 0x06:
    if(address & 0x10000)
      WRITE32LE(((u32 *)&vram[address & 0x17ffc]), value);
    else
      WRITE32LE(((u32 *)&vram[address & 0x1fffc]), value);
    break;
  case 0x07:
    WRITE32LE(((u32 *)&oam[address & 0x3fc]), value);
    break;
  case 0x0D:
//    if(cpuEEPROMEnabled) {
//      eepromWrite(address, value);
//      break;
//    }
    goto unwritable;
  //case 0x0E:
    //if(!eepromInUse | cpuSramEnabled | cpuFlashEnabled) {
  //    (*cpuSaveGameFunc)(address, (u8)value);
  //    break;
  //  }
    // default
  default:
  unwritable:
#ifdef DEV_VERSION
    if(systemVerbose & VERBOSE_ILLEGAL_WRITE) {
      log("Illegal word write: %08x to %08x from %08x\n",
          value,
          address,
          armMode ? armNextPC - 4 : armNextPC - 2);
    }
#endif
    break;
  }
}

#endif //VBA_GBAinline_H
