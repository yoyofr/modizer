//
// /home/ms/source/sidplay/libsidplay/emu/RCS/6510_.cpp,v
//
// --------------------------------------------------------------------------
// Copyright (c) 1994-1997 Michael Schwendt. All rights reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// THIS SOFTWARE  IS PROVIDED  BY THE  AUTHOR ``AS  IS'' AND  ANY EXPRESS OR
// IMPLIED  WARRANTIES,  INCLUDING,   BUT  NOT  LIMITED   TO,  THE   IMPLIED
// WARRANTIES OF MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR PURPOSE  ARE
// DISCLAIMED.  IN NO EVENT SHALL  THE AUTHOR OR CONTRIBUTORS BE LIABLE  FOR
// ANY DIRECT,  INDIRECT, INCIDENTAL,  SPECIAL, EXEMPLARY,  OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT  LIMITED TO, PROCUREMENT OF  SUBSTITUTE GOODS
// OR SERVICES;  LOSS OF  USE, DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION)
// HOWEVER  CAUSED  AND  ON  ANY  THEORY  OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  IN
// ANY  WAY  OUT  OF  THE  USE  OF  THIS  SOFTWARE,  EVEN  IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// --------------------------------------------------------------------------
//
// MOS-6510 Interpreter. Known bugs, missing features, incompatibilities:
//
//  - No support for code execution in Basic-ROM/Kernal-ROM.
//    Only execution of code in RAM is allowed.
//  - Probably inconsistent emulation of illegal instructions part 3.
//  - No detection of deadlocks.
//  - Faked RTI (= RTS).
//  - Anybody knows, whether it is ``Kernel'' instead of ``Kernal'' ?
//    Perhaps it is a proper name invented by CBM ?
//    It is spelled ``Kernal'' in nearly every C64 documentation !
//

#include "compconf.h"
#ifdef SID_HAVE_EXCEPTIONS
#include <new>
#endif
#include "6510_.h"
#include "myendian.h"
#include "emucfg.h"


ubyte* c64mem1 = 0;               // 64KB C64-RAM
ubyte* c64mem2 = 0;               // Basic-ROM, VIC, SID, I/O, Kernal-ROM
bool sidKeysOff[32];              // key_off detection
bool sidKeysOn[32];               // key_on detection
ubyte sidLastValue = 0;           // last value written to the SID
ubyte optr3readWave = 0;          // D41B
ubyte optr3readEnve = 0;          // D41C

// --------------------------------------------------------------------------

static ubyte AC, XR, YR;          // 6510 processor registers
static uword PC, SP;              // program-counter, stack-pointer
// PC is only used temporarily ! 
// The current program-counter is pPC-pPCbase

static ubyte* pPCbase;            // pointer to RAM/ROM buffer base
static ubyte* pPCend;             // pointer to RAM/ROM buffer end
static ubyte* pPC;                // pointer to PC location

// ---------------------------------------------------- Memory de-/allocation

static ubyte* c64ramBuf = 0;
static ubyte* c64romBuf = 0;

bool c64memFree()
{
	if ( c64romBuf != 0 )
	{
		delete[] c64romBuf;
		c64romBuf = (c64mem2 = 0);
	}
	if ( c64ramBuf != 0 )
	{
		delete[] c64ramBuf;
		c64ramBuf = (c64mem1 = 0);
	}
	return true;
}

bool c64memAlloc()
{
	c64memFree();
	bool wasSuccess = true;
#ifdef SID_HAVE_EXCEPTIONS
	if (( c64ramBuf = new(std::nothrow) ubyte[65536+256] ) == 0 )
#else
	if (( c64ramBuf = new ubyte[65536+256] ) == 0 )
#endif
	{
		wasSuccess = false;
	}
#ifdef SID_HAVE_EXCEPTIONS
	if (( c64romBuf = new(std::nothrow) ubyte[65536+256] ) == 0 )
#else
	if (( c64romBuf = new ubyte[65536+256] ) == 0 )
#endif
	{
		wasSuccess = false;
	}
	if (!wasSuccess)
	{
		c64memFree();
	}
	else
	{
		// Make the memory buffers accessible to the whole emulator engine.
		c64mem1 = c64ramBuf;
		c64mem2 = c64romBuf;
	}
	return wasSuccess;
}

// ------------------------------------------------------ (S)tatus (R)egister
// MOS-6510 SR: NV-BDIZC
//              76543210
//

struct statusRegister
{
	unsigned Carry     : 1;
	unsigned Zero      : 1;
	unsigned Interrupt : 1;
	unsigned Decimal   : 1;
	unsigned Break     : 1;
	unsigned NotUsed   : 1;
	unsigned oVerflow  : 1;
	unsigned Negative  : 1;
};

static statusRegister SR;

// Some handy defines to ease SR access.
#define CF SR.Carry
#define ZF SR.Zero
#define IF SR.Interrupt
#define DF SR.Decimal
#define BF SR.Break
#define NU SR.NotUsed
#define VF SR.oVerflow
#define NF SR.Negative

inline void affectNZ(ubyte reg)
{
	ZF = (reg == 0);
	NF = ((reg & 0x80) != 0);
}

inline void resetSR()
{
	// Explicit paranthesis looks great.
	CF = (ZF = (IF = (DF = (BF = (VF = (NF = 0))))));
	NU = 1;
}

inline ubyte codeSR()
{
	register ubyte tempSR = CF;
	tempSR |= (ZF<<1);
	tempSR |= (IF<<2);
	tempSR |= (DF<<3);
	tempSR |= (BF<<4);
	tempSR |= (NU<<5);
	tempSR |= (VF<<6);
	tempSR |= (NF<<7);
	return tempSR;
}

inline void decodeSR(ubyte stackByte)
{
	CF = (stackByte & 1);               
	ZF = ((stackByte & 2) !=0 );
	IF = ((stackByte & 4) !=0 );
	DF = ((stackByte & 8) !=0 );
	BF = ((stackByte & 16) !=0 );
	NU = 1;  // if used or writable, ((stackByte & 32) !=0 );
	VF = ((stackByte & 64) !=0 );
	NF = ((stackByte & 128) !=0 );
}

// --------------------------------------------------------------------------
// Handling conditional branches.

inline void branchIfClear(ubyte flag)
{
	if (flag == 0)
	{
		PC = pPC-pPCbase;   // calculate 16-bit PC
		PC += (sbyte)*pPC;  // add offset, keep it 16-bit (uword)
		pPC = pPCbase+PC;   // calc new pointer-PC
	}
	pPC++;
}

inline void branchIfSet(ubyte flag)
{
	if (flag != 0)
	{
		PC = pPC-pPCbase;   // calculate 16-bit PC
		PC += (sbyte)*pPC;  // add offset, keep it 16-bit (uword)
		pPC = pPCbase+PC;   // calc new pointer-PC
	}
	pPC++;
}

// --------------------------------------------------------------------------
// Addressing modes:
// Calculating 8/16-bit effective addresses out of data operands.

inline uword abso()
{
	return readLEword(pPC);
}

inline uword absx()
{
	return readLEword(pPC)+XR;
}

inline uword absy()
{
	return readLEword(pPC)+YR;
}

inline ubyte imm()
{
	return *pPC;
}

inline uword indx()
{
	return readEndian(c64mem1[(*pPC+1+XR)&0xFF],c64mem1[(*pPC+XR)&0xFF]);
}

inline uword indy()
{
	return YR+readEndian(c64mem1[(*pPC+1)&0xFF],c64mem1[*pPC]);
}

inline ubyte zp()
{
	return *pPC;
}

inline ubyte zpx()
{
	return *pPC+XR;
}

inline ubyte zpy()
{
	return *pPC+YR;
}

// --------------------------------------------------------------------------
// LIFO-Stack:
//
// |xxxxxx|
// |xxxxxx|
// |______|<- SP <= (hi)-return-address
// |______|      <= (lo)
// |______|
//

static bool stackIsOkay = true;

inline void resetSP()
{
	SP = 0x1ff;          // SP to top of stack
	stackIsOkay = true;
}

inline void checkSP()
{
	stackIsOkay = ((SP>0xff)&&(SP<=0x1ff));  // check boundaries
}

inline void RTS_()
{ 
    SP++;
    PC =readEndian( c64mem1[SP +1], c64mem1[SP] ) +1;
    pPC = pPCbase+PC;
    SP++;
    checkSP();
}

// --------------------------------------------------------------------------
// Relevant configurable memory banks:
//
//  $A000 to $BFFF = RAM, Basic-ROM
//  $C000 to $CFFF = RAM
//  $D000 to $DFFF = RAM, I/O, Char-ROM 
//  $E000 to $FFFF = RAM, Kernal-ROM
//
// Bank-Select Register $01:
// 
//   Bits
//   210    $A000-$BFFF   $D000-$DFFF   $E000-$FFFF
//  ------------------------------------------------
//   000       RAM           RAM            RAM
//   001       RAM        Char-ROM          RAM
//   010       RAM        Char-ROM      Kernal-ROM
//   011    Basic-ROM     Char-ROM      Kernal-ROM
//   100       RAM           RAM            RAM
//   101       RAM           I/O            RAM
//   110       RAM           I/O        Kernal-ROM
//   111    Basic-ROM        I/O        Kernal-ROM
//
// "Transparent ROM" mode:
//
// Basic-ROM and Kernal-ROM are considered transparent to read/write access.
// Basic-ROM is also considered transparent to branches (JMP, BCC, ...).
// I/O and Kernal-ROM are togglable via bank-select register $01.

static bool isBasic;   // these flags are used to not have to repeatedly
static bool isIO;      // evaluate the bank-select register for each
static bool isKernal;  // address operand

static ubyte* bankSelReg;  // pointer to RAM[1], bank-select register

static udword fakeReadTimer;


inline void evalBankSelect()
{
	// Determine new memory configuration.
	isBasic = ((*bankSelReg & 3) == 3);
	isIO = ((*bankSelReg & 7) > 4);
	isKernal = ((*bankSelReg & 2) != 0);
}


// Upon JMP/JSR prevent code execution in Basic-ROM/Kernal-ROM.
inline void evalBankJump()
{
	if (PC < 0xA000)
	{
		;
	}
	else
	{
		// Get high-nibble of address.
		switch (PC >> 12)
		{
		 case 0xa:
		 case 0xb:
			{
				if (isBasic)
				{
					RTS_();
				}
				break;
			}
		 case 0xc:
			{
				break;
			}
		 case 0xd:
			{
				if (isIO)
				{
					RTS_();
				}
				break;
			}
		 case 0xe:
		 case 0xf:
		 default:  // <-- just to please the compiler
			{
				if (isKernal)
				{
					RTS_();
				}
				break;
			}
		}
	}
}


// Functions to retrieve data.

static ubyte readData_bs(uword addr)
{
	if (addr < 0xA000)
	{
		return c64mem1[addr];
	}
	else
	{
		// Get high-nibble of address.
		switch (addr >> 12)
		{
		 case 0xa:
		 case 0xb:
			{
				if (isBasic)
					return c64mem2[addr];
				else
					return c64mem1[addr];
			}
		 case 0xc:
			{
				return c64mem1[addr];
			}
		 case 0xd:
			{
				if (isIO)
				{
					uword tempAddr = (addr & 0xfc1f);
					// Not SID ?
					if (( tempAddr & 0xff00 ) != 0xd400 )
					{
						switch (addr)
						{
						 case 0xd011:
						 case 0xd012:
						 case 0xdc04:
						 case 0xdc05:
							{
                                fakeReadTimer = fakeReadTimer*13+1;
								return (ubyte)(fakeReadTimer>>3);
							}
						 default:
							{
								return c64mem2[addr];
							}
						}
					}
					else
					{
						// $D41D/1E/1F, $D43D/, ... SID not mirrored
						if (( tempAddr & 0x00ff ) >= 0x001d )
							return(c64mem2[addr]); 
						// (Mirrored) SID.
						else
						{
							switch (tempAddr)
							{
							 case 0xd41b:
								{
									return optr3readWave;
								}
							 case 0xd41c:
								{
									return optr3readEnve;
								}
							 default:
								{
									return sidLastValue;
								}
							}
						}
					}
				}
				else
					return c64mem1[addr];
			}
		 case 0xe:
		 case 0xf:
		 default:  // <-- just to please the compiler
			{
				if (isKernal)
					return c64mem2[addr];
				else
					return c64mem1[addr];
			}
		}
	}
}

static ubyte readData_transp(uword addr)
{
	if (addr < 0xD000)
	{
		return c64mem1[addr];
	}
	else
	{
		// Get high-nibble of address.
		switch (addr >> 12)
		{
		 case 0xd:
			{
				if (isIO)
				{
					uword tempAddr = (addr & 0xfc1f);
					// Not SID ?
					if (( tempAddr & 0xff00 ) != 0xd400 )
					{
						switch (addr)
						{
						 case 0xd011:
						 case 0xd012:
						 case 0xdc04:
						 case 0xdc05:
							{
                                fakeReadTimer = fakeReadTimer*13+1;
								return (ubyte)(fakeReadTimer>>3);
							}
						 default:
							{
								return c64mem2[addr];
							}
						}
					}
					else
					{
						// $D41D/1E/1F, $D43D/, ... SID not mirrored
						if (( tempAddr & 0x00ff ) >= 0x001d )
							return(c64mem2[addr]); 
						// (Mirrored) SID.
						else
						{
							switch (tempAddr)
							{
							 case 0xd41b:
								{
									return optr3readWave;
								}
							 case 0xd41c:
								{
									return optr3readEnve;
								}
							 default:
								{
									return sidLastValue;
								}
							}
						}
					}
				}
				else
					return c64mem1[addr];
			}
		 case 0xe:
		 case 0xf:
		 default:  // <-- just to please the compiler
			{
				return c64mem1[addr];
			}
		}
	}
}

static ubyte readData_plain(uword addr)
{
	return c64mem1[addr];
}

inline ubyte readData_zp(uword addr)
{
	return c64mem1[addr];
}


// Functions to store data.

static void writeData_bs(uword addr, ubyte data)
{
	if ((addr < 0xd000) || (addr >= 0xe000))
	{
		c64mem1[addr] = data;
		if (addr == 0x01)  // write to Bank-Select Register ?
		{
			evalBankSelect();
		}
	}
	else
	{
		if (isIO)
		{
			// Check whether real SID or mirrored SID.
			uword tempAddr = (addr & 0xfc1f);
			// Not SID ?
			if (( tempAddr & 0xff00 ) != 0xd400 )
			{
				c64mem2[addr] = data;
			}
			// $D41D/1E/1F, $D43D/3E/3F, ...
			// Map to real address to support PlaySID
			// Extended SID Chip Registers.
			else if (( tempAddr & 0x00ff ) >= 0x001d )
			{
				// Mirrored SID.
				c64mem2[addr] = (sidLastValue = data);
			}
			else
			{
				// SID.
				c64mem2[tempAddr] = (sidLastValue = data);
				// Handle key_ons.
				sidKeysOn[tempAddr&0x001f] = sidKeysOn[tempAddr&0x001f] || ((data&1)!=0); 
				// Handle key_offs.
				sidKeysOff[tempAddr&0x001f] = sidKeysOff[tempAddr&0x001f] || ((data&1)==0); 
			}
		}
		else
		{
			c64mem1[addr] = data;
		}
	}
}

static void writeData_plain(uword addr, ubyte data)
{
	// Check whether real SID or mirrored SID.
	uword tempAddr = (addr & 0xfc1f);
	// Not SID ?
	if (( tempAddr & 0xff00 ) != 0xd400 )
	{
		c64mem1[addr] = data; 
	}
	// $D41D/1E/1F, $D43D/3E/3F, ...
	// Map to real address to support PlaySID
	// Extended SID Chip Registers.
	else if (( tempAddr & 0x00ff ) >= 0x001d )
	{
		// Mirrored SID.
		c64mem1[addr] = (sidLastValue = data);
	}
	else
	{
		// SID.
		c64mem2[tempAddr] = (sidLastValue = data);
		// Handle key_ons.
		sidKeysOn[tempAddr&0x001f] = sidKeysOn[tempAddr&0x001f] || ((data&1)!=0); 
		// Handle key_offs.
		sidKeysOff[tempAddr&0x001f] = sidKeysOff[tempAddr&0x001f] || ((data&1)==0); 
	}
}

inline void writeData_zp(uword addr, ubyte data)
{
	c64mem1[addr] = data;
	if (addr == 0x01)  // write to Bank-Select Register ?
	{
		evalBankSelect();
	}
}


// Use pointers to allow plain-memory modifications.
static ubyte (*readData)(uword) = &readData_bs;
static void (*writeData)(uword,ubyte) = &writeData_bs;

// --------------------------------------------------------------------------
// Legal instructions in alphabetical order.
//

inline void ADC_m(ubyte x)
{
	if ( DF == 1 )
	{
		uword AC2 = AC +x +CF;                
		ZF = ( AC2 == 0 );
		if ((( AC & 15 ) + ( x & 15 ) + CF ) > 9 )
		{
			AC2 += 6;
		}
		VF = ((( AC ^ x ^ AC2 ) & 0x80 ) != 0 ) ^ CF;
		NF = (( AC2 & 128 ) != 0 );
		if ( AC2 > 0x99 )
		{
			AC2 += 96;
		}
		CF = ( AC2 > 0x99 );
		AC = ( AC2 & 255 );
	}
	else
	{
		uword AC2 = AC +x +CF;                
		CF = ( AC2 > 255 );
		VF = ((( AC ^ x ^ AC2 ) & 0x80 ) != 0 ) ^ CF;
		affectNZ( AC = ( AC2 & 255 ));
	}
}
static void ADC_imm()  { ADC_m(imm()); pPC++; }
static void ADC_abso()  { ADC_m( readData(abso()) ); pPC += 2; }
static void ADC_absx()  { ADC_m( readData(absx()) ); pPC += 2; }
static void ADC_absy()  { ADC_m( readData(absy()) ); pPC += 2; }
static void ADC_indx()  { ADC_m( readData(indx()) ); pPC++; }
static void ADC_indy()  { ADC_m( readData(indy()) ); pPC++; }
static void ADC_zp()  { ADC_m( readData_zp(zp()) ); pPC++; }
static void ADC_zpx()  { ADC_m( readData_zp(zpx()) ); pPC++; }


inline void AND_m(ubyte x)
{
	affectNZ( AC &= x );
}
static void AND_imm()  { AND_m(imm()); pPC++; }
static void AND_abso()  { AND_m( readData(abso()) ); pPC += 2; }
static void AND_absx()  { AND_m( readData(absx()) ); pPC += 2; }
static void AND_absy()  { AND_m( readData(absy()) ); pPC += 2; }
static void AND_indx()  { AND_m( readData(indx()) ); pPC++; }
static void AND_indy()  { AND_m( readData(indy()) ); pPC++; }
static void AND_zp()  { AND_m( readData_zp(zp()) ); pPC++; }
static void AND_zpx()  { AND_m( readData_zp(zpx()) ); pPC++; }


inline ubyte ASL_m(ubyte x)
{ 
	CF = (( x & 128 ) != 0 );
	affectNZ( x <<= 1);
	return x;
}
static void ASL_AC()
{ 
	AC = ASL_m(AC);                         
}
static void ASL_abso()
{ 
	uword tempAddr = abso();
	pPC += 2;
	writeData( tempAddr, ASL_m( readData(tempAddr)) );
}
static void ASL_absx()
{ 
	uword tempAddr = absx();
	pPC += 2;
	writeData( tempAddr, ASL_m( readData(tempAddr)) );
}
static void ASL_zp()
{ 
	uword tempAddr = zp();
	pPC++;
	writeData_zp( tempAddr, ASL_m( readData_zp(tempAddr)) );
}
static void ASL_zpx()
{ 
	uword tempAddr = zpx();
	pPC++;
	writeData_zp( tempAddr, ASL_m( readData_zp(tempAddr)) );
}


static void BCC_()  { branchIfClear(CF); }

static void BCS_()  { branchIfSet(CF); }

static void BEQ_()  { branchIfSet(ZF); }


inline void BIT_m(ubyte x)
{ 
	ZF = (( AC & x ) == 0 );                  
	VF = (( x & 64 ) != 0 );
	NF = (( x & 128 ) != 0 );
}
static void BIT_abso()  { BIT_m( readData(abso()) ); pPC += 2; }
static void BIT_zp()  {	BIT_m( readData_zp(zp()) );	pPC++; }


static void BMI_()  { branchIfSet(NF); }

static void BNE_()  { branchIfClear(ZF); }

static void BPL_()  { branchIfClear(NF); }


static void BRK_()
{ 
	BF = (IF = 1);
#if !defined(NO_RTS_UPON_BRK)
	RTS_();
#endif
}


static void BVC_()  { branchIfClear(VF); }

static void BVS_()  { branchIfSet(VF); }


static void CLC_()  { CF = 0; }

static void CLD_()  { DF = 0; }

static void CLI_()  { IF = 0; }

static void CLV_()  { VF = 0; }


inline void CMP_m(ubyte x)
{ 
	ZF = ( AC == x );                     
	CF = ( AC >= x );                         
	NF = ( (sbyte)( AC - x ) < 0 );
}
static void CMP_abso()  { CMP_m( readData(abso()) ); pPC += 2; }
static void CMP_absx()  { CMP_m( readData(absx()) ); pPC += 2; }
static void CMP_absy()  { CMP_m( readData(absy()) ); pPC += 2; }
static void CMP_imm()  { CMP_m(imm()); pPC++; }
static void CMP_indx()  { CMP_m( readData(indx()) ); pPC++; }
static void CMP_indy()  { CMP_m( readData(indy()) ); pPC++; }
static void CMP_zp()  { CMP_m( readData_zp(zp()) ); pPC++; }
static void CMP_zpx()  { CMP_m( readData_zp(zpx()) ); pPC++; }


inline void CPX_m(ubyte x)
{ 
	ZF = ( XR == x );                         
	CF = ( XR >= x );                         
	NF = ( (sbyte)( XR - x ) < 0 );      
}
static void CPX_abso()  { CPX_m( readData(abso()) ); pPC += 2; }
static void CPX_imm()  { CPX_m(imm()); pPC++; }
static void CPX_zp()  { CPX_m( readData_zp(zp()) ); pPC++; }


inline void CPY_m(ubyte x)
{ 
	ZF = ( YR == x );                         
	CF = ( YR >= x );                         
	NF = ( (sbyte)( YR - x ) < 0 );      
}
static void CPY_abso()  { CPY_m( readData(abso()) ); pPC += 2; }
static void CPY_imm()  { CPY_m(imm()); pPC++; }
static void CPY_zp()  { CPY_m( readData_zp(zp()) ); pPC++; }


inline void DEC_m(uword addr)
{ 
	ubyte x = readData(addr);                   
	affectNZ(--x);
	writeData(addr, x);
}
inline void DEC_m_zp(uword addr)
{ 
	ubyte x = readData_zp(addr);
	affectNZ(--x);
	writeData_zp(addr, x);
}
static void DEC_abso()  { DEC_m( abso() ); pPC += 2; }
static void DEC_absx()  { DEC_m( absx() ); pPC += 2; }
static void DEC_zp()  { DEC_m_zp( zp() ); pPC++; }
static void DEC_zpx()  { DEC_m_zp( zpx() ); pPC++; }


static void DEX_()  { affectNZ(--XR); }

static void DEY_()  { affectNZ(--YR); }


inline void EOR_m(ubyte x)
{ 
	AC ^= x;                          
	affectNZ(AC);                         
}
static void EOR_abso()  { EOR_m( readData(abso()) ); pPC += 2; }
static void EOR_absx()  { EOR_m( readData(absx()) ); pPC += 2; }
static void EOR_absy()  { EOR_m( readData(absy()) ); pPC += 2; }
static void EOR_imm()  { EOR_m(imm()); pPC++; }
static void EOR_indx()  { EOR_m( readData(indx()) ); pPC++; }
static void EOR_indy()  { EOR_m( readData(indy()) ); pPC++; }
static void EOR_zp()  { EOR_m( readData_zp(zp()) ); pPC++; }
static void EOR_zpx()  { EOR_m( readData_zp(zpx()) ); pPC++; }


inline void INC_m(uword addr)
{
	ubyte x = readData(addr);                   
	affectNZ(++x);
	writeData(addr, x);
}
inline void INC_m_zp(uword addr)
{
	ubyte x = readData_zp(addr);                   
	affectNZ(++x);
	writeData_zp(addr, x);
}
static void INC_abso()  { INC_m( abso() ); pPC += 2; }
static void INC_absx()  { INC_m( absx() ); pPC += 2; }
static void INC_zp()  { INC_m_zp( zp() ); pPC++; }
static void INC_zpx()  { INC_m_zp( zpx() ); pPC++; }


static void INX_()  { affectNZ(++XR); }

static void INY_()  { affectNZ(++YR); }



static void JMP_()
{ 
	PC = abso();
	pPC = pPCbase+PC;
	evalBankJump();
}

static void JMP_transp()
{ 
	PC = abso();
	if ( (PC>=0xd000) && isKernal )
	{
		RTS_();  // will set pPC
	}
	else
	{
		pPC = pPCbase+PC;
	}
}

static void JMP_plain()
{ 
	PC = abso();
	pPC = pPCbase+PC;
}


static void JMP_vec()
{ 
	uword tempAddrLo = abso();
	uword tempAddrHi = (tempAddrLo&0xFF00) | ((tempAddrLo+1)&0x00FF);
	PC = readEndian(readData(tempAddrHi),readData(tempAddrLo));
	pPC = pPCbase+PC;
	evalBankJump();
}

static void JMP_vec_transp()
{ 
	uword tempAddrLo = abso();
	uword tempAddrHi = (tempAddrLo&0xFF00) | ((tempAddrLo+1)&0x00FF);
	PC = readEndian(readData(tempAddrHi),readData(tempAddrLo));
	if ( (PC>=0xd000) && isKernal )
	{
		RTS_();  // will set pPC
	}
	else
	{
		pPC = pPCbase+PC;
	}
}

static void JMP_vec_plain()
{ 
	uword tempAddrLo = abso();
	uword tempAddrHi = (tempAddrLo&0xFF00) | ((tempAddrLo+1)&0x00FF);
	PC = readEndian(readData(tempAddrHi),readData(tempAddrLo));
	pPC = pPCbase+PC;
}


inline void JSR_main()
{
	uword tempPC = abso();
	pPC += 2;
	PC = pPC-pPCbase;
	PC--;
	SP--;
	writeLEword(c64mem1+SP,PC);
	SP--;
	checkSP();
	PC = tempPC;
}

static void JSR_()
{ 
	JSR_main();
	pPC = pPCbase+PC;
	evalBankJump();
}

static void JSR_transp()
{ 
	JSR_main();
	if ( (PC>=0xd000) && isKernal )
	{
		RTS_();  // will set pPC
	}
	else
	{
		pPC = pPCbase+PC;
	}
}

static void JSR_plain()
{ 
	JSR_main();
	pPC = pPCbase+PC;
}


static void LDA_abso()  { affectNZ( AC = readData(abso()) ); pPC += 2; }
static void LDA_absx()  { affectNZ( AC = readData( absx() )); pPC += 2; }
static void LDA_absy()  { affectNZ( AC = readData( absy() ) ); pPC += 2; }
static void LDA_imm()  { affectNZ( AC = imm() ); pPC++; }
static void LDA_indx()  { affectNZ( AC = readData( indx() ) ); pPC++; }
static void LDA_indy()  { affectNZ( AC = readData( indy() ) ); pPC++; }
static void LDA_zp()  { affectNZ( AC = readData_zp( zp() ) ); pPC++; }
static void LDA_zpx()  { affectNZ( AC = readData_zp( zpx() ) ); pPC++; }


static void LDX_abso()  { affectNZ(XR=readData(abso())); pPC += 2; }
static void LDX_absy()  { affectNZ(XR=readData(absy())); pPC += 2; }
static void LDX_imm()  { affectNZ(XR=imm()); pPC++; }
static void LDX_zp()  { affectNZ(XR=readData_zp(zp())); pPC++; }
static void LDX_zpy()  { affectNZ(XR=readData_zp(zpy())); pPC++; }


static void LDY_abso()  { affectNZ(YR=readData(abso())); pPC += 2; }
static void LDY_absx()  { affectNZ(YR=readData(absx())); pPC += 2; }
static void LDY_imm()  { affectNZ(YR=imm()); pPC++; }
static void LDY_zp()  { affectNZ(YR=readData_zp(zp())); pPC++; }
static void LDY_zpx()  { affectNZ(YR=readData_zp(zpx())); pPC++; }


inline ubyte LSR_m(ubyte x)
{ 
	CF = x & 1;
	x >>= 1;
	NF = 0;
	ZF = (x == 0);
	return x;
}
static void LSR_AC()
{ 
	AC = LSR_m(AC);
}
static void LSR_abso()
{ 
	uword tempAddr = abso();
	pPC += 2; 
	writeData( tempAddr, (LSR_m( readData(tempAddr))) );
}
static void LSR_absx()
{ 
	uword tempAddr = absx();
	pPC += 2; 
	writeData( tempAddr, (LSR_m( readData(tempAddr))) );
}
static void LSR_zp()
{ 
	uword tempAddr = zp();
	pPC++; 
	writeData_zp( tempAddr, (LSR_m( readData_zp(tempAddr))) );
}
static void LSR_zpx()
{ 
	uword tempAddr = zpx();
	pPC++; 
	writeData_zp( tempAddr, (LSR_m( readData_zp(tempAddr))) );
}


inline void ORA_m(ubyte x)
{
	affectNZ( AC |= x );
}
static void ORA_abso()  { ORA_m( readData(abso()) ); pPC += 2; }
static void ORA_absx()  { ORA_m( readData(absx()) ); pPC += 2; }
static void ORA_absy()  { ORA_m( readData(absy()) ); pPC += 2; }
static void ORA_imm()  { ORA_m(imm()); pPC++; }
static void ORA_indx()  { ORA_m( readData(indx()) ); pPC++; }
static void ORA_indy()  { ORA_m( readData(indy()) ); pPC++; }
static void ORA_zp()  { ORA_m( readData_zp(zp()) ); pPC++; }
static void ORA_zpx()  { ORA_m( readData_zp(zpx()) ); pPC++; }


static void NOP_()  { }

static void PHA_()  { c64mem1[SP--] = AC; }


static void PHP_()
{ 
	c64mem1[SP--] = codeSR();
}


static void PLA_()
{ 
	affectNZ(AC=c64mem1[++SP]);
}


static void PLP_()
{ 
	decodeSR(c64mem1[++SP]);
}

inline ubyte ROL_m(ubyte x)
{ 
	ubyte y = ( x << 1 ) + CF;
	CF = (( x & 0x80 ) != 0 );
	affectNZ(y);
	return y;
}
static void ROL_AC()  { AC=ROL_m(AC); }
static void ROL_abso()
{ 
	uword tempAddr = abso();
	pPC += 2; 
	writeData( tempAddr, ROL_m( readData(tempAddr)) );
}
static void ROL_absx()
{ 
	uword tempAddr = absx();
	pPC += 2; 
	writeData( tempAddr, ROL_m( readData(tempAddr)) );
}
static void ROL_zp()
{ 
	uword tempAddr = zp();
	pPC++; 
	writeData_zp( tempAddr, ROL_m( readData_zp(tempAddr)) );
}
static void ROL_zpx()
{ 
	uword tempAddr = zpx();
	pPC++; 
	writeData_zp( tempAddr, ROL_m( readData_zp(tempAddr)) );
}

inline ubyte ROR_m(ubyte x)
{ 
	ubyte y = ( x >> 1 ) | ( CF << 7 );
	CF = ( x & 1 );
	affectNZ(y);
	return y;
}
static void ROR_AC()
{ 
	AC = ROR_m(AC);
}
static void ROR_abso()
{ 
	uword tempAddr = abso();
	pPC += 2; 
	writeData( tempAddr, ROR_m( readData(tempAddr)) );
}
static void ROR_absx()
{ 
	uword tempAddr = absx();
	pPC += 2; 
	writeData( tempAddr, ROR_m( readData(tempAddr)) );
}
static void ROR_zp()
{ 
	uword tempAddr = zp();
	pPC++; 
	writeData_zp( tempAddr, ROR_m( readData_zp(tempAddr)) );
}
static void ROR_zpx()
{ 
	uword tempAddr = zpx();
	pPC++; 
	writeData_zp( tempAddr, ROR_m( readData_zp(tempAddr)) );
}


static void RTI_()	
{
	// equal to RTS_();
	SP++;
	PC =readEndian( c64mem1[SP +1], c64mem1[SP] ) +1;
	pPC = pPCbase+PC;
	SP++;
	checkSP();
}


// RTS_() is inline.


inline void SBC_m(ubyte s)
{ 
	s = (~s) & 255;
	if ( DF == 1 )
	{
		uword AC2 = AC +s +CF;                
		ZF = ( AC2 == 0 );
		if ((( AC & 15 ) + ( s & 15 ) + CF ) > 9 )
		{
			AC2 += 6;
		}
		VF = ((( AC ^ s ^ AC2 ) & 0x80 ) != 0 ) ^ CF;
		NF = (( AC2 & 128 ) != 0 );
		if ( AC2 > 0x99 )
		{
			AC2 += 96;
		}
		CF = ( AC2 > 0x99 );
		AC = ( AC2 & 255 );
	}
	else
	{
		uword AC2 = AC + s + CF;
		CF = ( AC2 > 255 );
		VF = ((( AC ^ s ^ AC2 ) & 0x80 ) != 0 ) ^ CF;
		affectNZ( AC = ( AC2 & 255 ));
	}
}
static void SBC_abso()  { SBC_m( readData(abso()) ); pPC += 2; }
static void SBC_absx()  { SBC_m( readData(absx()) ); pPC += 2; }
static void SBC_absy()  { SBC_m( readData(absy()) ); pPC += 2; }
static void SBC_imm()  { SBC_m(imm()); pPC++; }
static void SBC_indx()  { SBC_m( readData( indx()) ); pPC++; }
static void SBC_indy()  { SBC_m( readData(indy()) ); pPC++; }
static void SBC_zp()  { SBC_m( readData_zp(zp()) ); pPC++; }
static void SBC_zpx()  { SBC_m( readData_zp(zpx()) ); pPC++; }


static void SEC_()  { CF=1; }

static void SED_()  { DF=1; }

static void SEI_()  { IF=1; }


static void STA_abso()  { writeData( abso(), AC ); pPC += 2; }
static void STA_absx()  { writeData( absx(), AC ); pPC += 2; }
static void STA_absy()  { writeData( absy(), AC ); pPC += 2; }
static void STA_indx()  { writeData( indx(), AC ); pPC++; }
static void STA_indy()  { writeData( indy(), AC ); pPC++; }
static void STA_zp()  { writeData_zp( zp(), AC ); pPC++; }
static void STA_zpx() { writeData_zp( zpx(), AC ); pPC++; }


static void STX_abso()  { writeData( abso(), XR ); pPC += 2; }
static void STX_zp()  { writeData_zp( zp(), XR ); pPC++; }
static void STX_zpy()  { writeData_zp( zpy(), XR ); pPC++; }


static void STY_abso()  { writeData( abso(), YR ); pPC += 2; }
static void STY_zp()  { writeData_zp( zp(), YR ); pPC++; }
static void STY_zpx()  { writeData_zp( zpx(), YR ); pPC++; }


static void TAX_()  { affectNZ(XR=AC); }

static void TAY_()  { affectNZ(YR=AC); }


static void TSX_()
{ 
	XR = SP & 255;													
	affectNZ(XR);
}

static void TXA_()  { affectNZ(AC=XR); }

static void TXS_()  { SP = XR | 0x100; checkSP(); }

static void TYA_()  { affectNZ(AC=YR); }


// --------------------------------------------------------------------------
// Illegal codes/instructions part (1).

static void ILL_TILT()  { }

static void ILL_1NOP()  { }

static void ILL_2NOP()  { pPC++; }

static void ILL_3NOP()  { pPC += 2; }


// --------------------------------------------------------------------------
// Illegal codes/instructions part (2).

inline void ASLORA_m(uword addr)
{
	ubyte x = ASL_m(readData(addr));
	writeData(addr,x);
	ORA_m(x);
}
inline void ASLORA_m_zp(uword addr)
{
	ubyte x = ASL_m(readData_zp(addr));
	writeData_zp(addr,x);
	ORA_m(x);
}
static void ASLORA_abso()
{
	ASLORA_m(abso());
	pPC += 2; 
}
static void ASLORA_absx()
{
	ASLORA_m(absx());
	pPC += 2; 
}
static void ASLORA_absy()
{
	ASLORA_m(absy());
	pPC += 2; 
}
static void ASLORA_indx()
{
	ASLORA_m(indx());
	pPC++; 
}
static void ASLORA_indy()
{
	ASLORA_m(indy());
	pPC++; 
}
static void ASLORA_zp()
{
	ASLORA_m_zp(zp());
	pPC++; 
} 
static void ASLORA_zpx()
{
	ASLORA_m_zp(zpx());
	pPC++; 
}


inline void ROLAND_m(uword addr)
{
	uword x = ROL_m(readData(addr));
	writeData(addr,x);
	AND_m(x);
}
inline void ROLAND_m_zp(uword addr)
{
	uword x = ROL_m(readData_zp(addr));
	writeData_zp(addr,x);
	AND_m(x);
}
static void ROLAND_abso()
{
	ROLAND_m(abso());
	pPC += 2; 
}
static void ROLAND_absx()
{
	ROLAND_m(absx());
	pPC += 2; 
}
static void ROLAND_absy()
{
	ROLAND_m(absy());
	pPC += 2; 
}  
static void ROLAND_indx()
{
	ROLAND_m(indx());
	pPC++; 
}  
static void ROLAND_indy()
{
	ROLAND_m(indy());
	pPC++; 
} 
static void ROLAND_zp()
{
	ROLAND_m_zp(zp());
	pPC++; 
} 
static void ROLAND_zpx()
{
	ROLAND_m_zp(zpx());
	pPC++; 
} 


inline void LSREOR_m(uword addr)
{
	uword x = LSR_m(readData(addr));
	writeData(addr,x);
	EOR_m(x);
}
inline void LSREOR_m_zp(uword addr)
{
	uword x = LSR_m(readData_zp(addr));
	writeData_zp(addr,x);
	EOR_m(x);
}
static void LSREOR_abso()
{
	LSREOR_m(abso());
	pPC += 2; 
}
static void LSREOR_absx()
{
	LSREOR_m(absx());
	pPC += 2; 
}  
static void LSREOR_absy()
{
	LSREOR_m(absy());
	pPC += 2; 
}  
static void LSREOR_indx()
{
	LSREOR_m(indx());
	pPC++; 
} 
static void LSREOR_indy()
{
	LSREOR_m(indy());
	pPC++; 
}  
static void LSREOR_zp()
{
	LSREOR_m_zp(zp());
	pPC++; 
}  
static void LSREOR_zpx()
{
	LSREOR_m_zp(zpx());
	pPC++; 
}  


inline void RORADC_m(uword addr)
{
	ubyte x = ROR_m(readData(addr));
	writeData(addr,x);
	ADC_m(x);
}
inline void RORADC_m_zp(uword addr)
{
	ubyte x = ROR_m(readData_zp(addr));
	writeData_zp(addr,x);
	ADC_m(x);
}
static void RORADC_abso()
{
	RORADC_m(abso());
	pPC += 2; 
} 
static void RORADC_absx()
{
	RORADC_m(absx());
	pPC += 2; 
} 
static void RORADC_absy()
{
	RORADC_m(absy());
	pPC += 2; 
}  
static void RORADC_indx()
{
	RORADC_m(indx());
	pPC++; 
}  
static void RORADC_indy()
{
	RORADC_m(indy());
	pPC++; 
} 
static void RORADC_zp()
{
	RORADC_m_zp(zp());
	pPC++; 
}  
static void RORADC_zpx()
{
	RORADC_m_zp(zpx());
	pPC++; 
}


inline void DECCMP_m(uword addr)
{
	ubyte x = readData(addr);
	writeData(addr,(--x));
	CMP_m(x);
}
inline void DECCMP_m_zp(uword addr)
{
	ubyte x = readData_zp(addr);
	writeData_zp(addr,(--x));
	CMP_m(x);
}
static void DECCMP_abso()
{
	DECCMP_m(abso());
	pPC += 2; 
}  
static void DECCMP_absx()
{
	DECCMP_m(absx());
	pPC += 2; 
}  
static void DECCMP_absy()
{
	DECCMP_m(absy());
	pPC += 2; 
}  
static void DECCMP_indx()
{
	DECCMP_m(indx());
	pPC++; 
}  
static void DECCMP_indy()
{
	DECCMP_m(indy());
	pPC++; 
} 
static void DECCMP_zp()
{
	DECCMP_m_zp(zp());
	pPC++; 
} 
static void DECCMP_zpx()
{
	DECCMP_m_zp(zpx());
	pPC++; 
} 


inline void INCSBC_m(uword addr)
{
	ubyte x = readData(addr);
	writeData(addr,(++x));
	SBC_m(x);
}
inline void INCSBC_m_zp(uword addr)
{
	ubyte x = readData_zp(addr);
	writeData_zp(addr,(++x));
	SBC_m(x);
}
static void INCSBC_abso()
{
	INCSBC_m(abso());
	pPC += 2; 
} 
static void INCSBC_absx()
{
	INCSBC_m(absx());
	pPC += 2; 
}  
static void INCSBC_absy()
{
	INCSBC_m(absy());
	pPC += 2; 
}  
static void INCSBC_indx()
{
	INCSBC_m(indx());
	pPC++; 
}  
static void INCSBC_indy()
{
	INCSBC_m(indy());
	pPC++; 
} 
static void INCSBC_zp()
{
	INCSBC_m_zp(zp());
	pPC++; 
} 
static void INCSBC_zpx()
{
	INCSBC_m_zp(zpx());
	pPC++; 
}  


// --------------------------------------------------------------------------
// Illegal codes/instructions part (3). This implementation is considered to
// be only partially working due to inconsistencies in the available
// documentation.
// Note: In some of the functions emulated, defined instructions are used and
// already increment the PC ! Take care, and do not increment further !
// Double-setting of processor flags can occur, too.

static void ILL_0B() // equal to 2B
{ 
	AND_imm();
	CF = NF;
}
	
static void ILL_4B()
{ 
	AND_imm();
	LSR_AC();
}
	
static void ILL_6B()
{ 
	if (DF == 0)
	{
		AND_imm();
		ROR_AC();
		CF = (AC & 1);
		VF = (AC >> 5) ^ (AC >> 6);
	}
}
	
static void ILL_83()
{
	writeData(indx(),AC & XR);
	pPC++; 
}

static void ILL_87()
{
	writeData_zp(zp(),AC & XR);
	pPC++; 
}

static void ILL_8B()
{ 
	TXA_();
	AND_imm();
}
	
static void ILL_8F()
{
	writeData(abso(),AC & XR);
	pPC += 2; 
}

static void ILL_93()
{
	writeData(indy(), AC & XR & (1+(readData((*pPC)+1) & 0xFF)));
	pPC++; 
}

static void ILL_97()
{ 
	writeData_zp(zpx(),AC & XR);
	pPC++; 
}

static void ILL_9B()
{
	SP = 0x100 | (AC & XR);
	writeData(absy(),(SP & ((*pPC+1)+1)));
	pPC += 2;
	checkSP();
}

static void ILL_9C()
{
	writeData(absx(),(YR & ((*pPC+1)+1)));
	pPC += 2;
}

static void ILL_9E()
{ 
	writeData(absy(),(XR & ((*pPC+1)+1)));
	pPC += 2;
}

static void ILL_9F()
{
	writeData(absy(),(AC & XR & ((*pPC+1)+1)));
	pPC += 2;
}

static void ILL_A3()
{
	LDA_indx();
	TAX_();
}

static void ILL_A7()
{
	LDA_zp();
	TAX_();
}

static void ILL_AB()
// Taken from VICE because a single music player has been found
// (not in HVSC) which seems to need ILL_AB implemented like this
// (or similarly) in order to work.
{
    AC = XR = ((AC|0xee)&(*pPC++));
    affectNZ( AC );
}

static void ILL_AF()
{
	LDA_abso();
	TAX_();
}

static void ILL_B3()
{
	LDA_indy();
	TAX_();
}

static void ILL_B7()
{
	affectNZ(AC = readData_zp(zpy()));  // would be LDA_zpy()
	TAX_();
	pPC++; 
}

static void ILL_BB()
{
	XR = (SP & absy());
	pPC += 2;
	TXS_();
	TXA_();
}

static void ILL_BF()
{
	LDA_absy();
	TAX_();
}

static void ILL_CB()
{ 
	uword tmp = XR & AC;
	tmp -= imm();
	CF = (tmp > 255);
	affectNZ(XR=(tmp&255));
}

static void ILL_EB()
{ 
	SBC_imm();
}

// --------------------------------------------------------------------------

static ptr2func instrList[] =
{
	&BRK_, &ORA_indx, &ILL_TILT, &ASLORA_indx, &ILL_2NOP, &ORA_zp, &ASL_zp, &ASLORA_zp,
	&PHP_, &ORA_imm, &ASL_AC, &ILL_0B, &ILL_3NOP, &ORA_abso, &ASL_abso, &ASLORA_abso,
	&BPL_, &ORA_indy, &ILL_TILT, &ASLORA_indy, &ILL_2NOP, &ORA_zpx, &ASL_zpx, &ASLORA_zpx,
	&CLC_, &ORA_absy, &ILL_1NOP, &ASLORA_absy, &ILL_3NOP, &ORA_absx, &ASL_absx, &ASLORA_absx,
	&JSR_, &AND_indx, &ILL_TILT, &ROLAND_indx, &BIT_zp, &AND_zp, &ROL_zp, &ROLAND_zp,
	&PLP_, &AND_imm, &ROL_AC, &ILL_0B, &BIT_abso, &AND_abso, &ROL_abso, &ROLAND_abso,
	&BMI_, &AND_indy, &ILL_TILT, &ROLAND_indy, &ILL_2NOP, &AND_zpx, &ROL_zpx, &ROLAND_zpx,
	&SEC_, &AND_absy, &ILL_1NOP, &ROLAND_absy, &ILL_3NOP, &AND_absx, &ROL_absx, &ROLAND_absx,
	// 0x40
	&RTI_, &EOR_indx, &ILL_TILT, &LSREOR_indx, &ILL_2NOP, &EOR_zp, &LSR_zp, &LSREOR_zp,
	&PHA_, &EOR_imm, &LSR_AC, &ILL_4B, &JMP_, &EOR_abso, &LSR_abso, &LSREOR_abso,
	&BVC_, &EOR_indy, &ILL_TILT, &LSREOR_indy, &ILL_2NOP, &EOR_zpx, &LSR_zpx, &LSREOR_zpx,
	&CLI_, &EOR_absy, &ILL_1NOP, &LSREOR_absy, &ILL_3NOP, &EOR_absx, &LSR_absx, &LSREOR_absx,
	&RTS_, &ADC_indx, &ILL_TILT, &RORADC_indx, &ILL_2NOP, &ADC_zp, &ROR_zp, &RORADC_zp,
	&PLA_, &ADC_imm, &ROR_AC, &ILL_6B, &JMP_vec, &ADC_abso, &ROR_abso, &RORADC_abso,
	&BVS_, &ADC_indy, &ILL_TILT, &RORADC_indy, &ILL_2NOP, &ADC_zpx, &ROR_zpx, &RORADC_zpx,
	&SEI_, &ADC_absy, &ILL_1NOP, &RORADC_absy, &ILL_3NOP, &ADC_absx, &ROR_absx, &RORADC_absx,
	// 0x80
	&ILL_2NOP, &STA_indx, &ILL_2NOP, &ILL_83, &STY_zp, &STA_zp, &STX_zp, &ILL_87,
	&DEY_, &ILL_2NOP, &TXA_, &ILL_8B, &STY_abso, &STA_abso, &STX_abso, &ILL_8F,
	&BCC_, &STA_indy, &ILL_TILT, &ILL_93, &STY_zpx, &STA_zpx, &STX_zpy, &ILL_97,
	&TYA_, &STA_absy, &TXS_, &ILL_9B, &ILL_9C, &STA_absx, &ILL_9E, &ILL_9F,
	&LDY_imm, &LDA_indx, &LDX_imm, &ILL_A3, &LDY_zp, &LDA_zp, &LDX_zp, &ILL_A7,
	&TAY_, &LDA_imm, &TAX_, &ILL_AB, &LDY_abso, &LDA_abso, &LDX_abso, &ILL_AF,
	&BCS_, &LDA_indy, &ILL_TILT, &ILL_B3, &LDY_zpx, &LDA_zpx, &LDX_zpy, &ILL_B7,
	&CLV_, &LDA_absy, &TSX_, &ILL_BB, &LDY_absx, &LDA_absx, &LDX_absy, &ILL_BF,
	// 0xC0
	&CPY_imm, &CMP_indx, &ILL_2NOP, &DECCMP_indx, &CPY_zp, &CMP_zp, &DEC_zp, &DECCMP_zp,
	&INY_, &CMP_imm, &DEX_, &ILL_CB, &CPY_abso, &CMP_abso, &DEC_abso, &DECCMP_abso,
	&BNE_, &CMP_indy, &ILL_TILT, &DECCMP_indy, &ILL_2NOP, &CMP_zpx, &DEC_zpx, &DECCMP_zpx,
	&CLD_, &CMP_absy, &ILL_1NOP, &DECCMP_absy, &ILL_3NOP, &CMP_absx, &DEC_absx, &DECCMP_absx,
	&CPX_imm, &SBC_indx, &ILL_2NOP, &INCSBC_indx, &CPX_zp, &SBC_zp, &INC_zp, &INCSBC_zp,
	&INX_, &SBC_imm,  &NOP_, &ILL_EB, &CPX_abso, &SBC_abso, &INC_abso, &INCSBC_abso,
	&BEQ_, &SBC_indy, &ILL_TILT, &INCSBC_indy, &ILL_2NOP, &SBC_zpx, &INC_zpx, &INCSBC_zpx,
	&SED_, &SBC_absy, &ILL_1NOP, &INCSBC_absy, &ILL_3NOP, &SBC_absx, &INC_absx, &INCSBC_absx
};


static int memoryMode = MPU_TRANSPARENT_ROM;  // the default


void initInterpreter(int inMemoryMode)
{
	memoryMode = inMemoryMode;
	if (memoryMode == MPU_TRANSPARENT_ROM)
	{
		readData = &readData_transp;
		writeData = &writeData_bs;
		instrList[0x20] = &JSR_transp;
		instrList[0x4C] = &JMP_transp;
		instrList[0x6C] = &JMP_vec_transp;
		// Make the memory buffers accessible to the whole emulator engine.
		// Use two distinct 64KB memory areas.
		c64mem1 = c64ramBuf;
		c64mem2 = c64romBuf;
	}
	else if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
	{
		readData = &readData_plain;
		writeData = &writeData_plain;
		instrList[0x20] = &JSR_plain;
		instrList[0x4C] = &JMP_plain;
		instrList[0x6C] = &JMP_vec_plain;
		// Make the memory buffers accessible to the whole emulator engine.
		// Use a single 64KB memory area.
		c64mem2 = (c64mem1 = c64ramBuf);
	}
	else  // if (memoryMode == MPU_BANK_SWITCHING)
	{
		readData = &readData_bs;
		writeData = &writeData_bs;
		instrList[0x20] = &JSR_;
		instrList[0x4C] = &JMP_;
		instrList[0x6C] = &JMP_vec;
		// Make the memory buffers accessible to the whole emulator engine.
		// Use two distinct 64KB memory areas.
		c64mem1 = c64ramBuf;
		c64mem2 = c64romBuf;
	}
	bankSelReg = c64ramBuf+1;  // extra pointer
	// Set code execution segment to RAM.
	pPCbase = c64ramBuf;
	pPCend = c64ramBuf+65536;
}


bool interpreter(uword p, ubyte ramrom, ubyte a, ubyte x, ubyte y)
{  
	if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
	{
		AC = a;
		XR = 0;
		YR = 0;
	}
	else
	{
		*bankSelReg = ramrom;
		evalBankSelect();
		AC = a;
		XR = x;
		YR = y;
	}

	// Set program-counter (pointer instead of raw PC).
	pPC = pPCbase+p;
	
	resetSP();
	resetSR();            
	sidKeysOff[4] = (sidKeysOff[4+7] = (sidKeysOff[4+14] = false));
	sidKeysOn[4] = (sidKeysOn[4+7] = (sidKeysOn[4+14] = false));
	
	do
	{ 
		(*instrList[*(pPC++)])();
	}
	while (stackIsOkay&&(pPC<pPCend));
	
	return true;
}


void c64memReset(int clockSpeed, ubyte randomSeed)
{
	fakeReadTimer += randomSeed;
	
	if ((c64mem1 != 0) && (c64mem2 != 0))
	{
		c64mem1[0] = 0x2F;
		// defaults: Basic-ROM on, Kernal-ROM on, I/O on
		c64mem1[1] = 0x07;
		evalBankSelect();
		
		// CIA-Timer A $DC04/5 = $4025 PAL, $4295 NTSC
		if (clockSpeed == SIDTUNE_CLOCK_NTSC)
		{
			c64mem1[0x02a6] = 0;     // NTSC
			c64mem2[0xdc04] = 0x95;
			c64mem2[0xdc05] = 0x42;
		}
		else  // if (clockSpeed == SIDTUNE_CLOCK_PAL)
		{
			c64mem1[0x02a6] = 1;     // PAL
			c64mem2[0xdc04] = 0x25;
			c64mem2[0xdc05] = 0x40;
		}
		
		// fake VBI-interrupts that do $D019, BMI ...
		c64mem2[0xd019] = 0xff;
		
		// software vectors
		// IRQ to $EA31
		c64mem1[0x0314] = 0x31;
		c64mem1[0x0315] = 0xea;
		// BRK to $FE66
		c64mem1[0x0316] = 0x66;
		c64mem1[0x0317] = 0xfe;
		// NMI to $FE47
		c64mem1[0x0318] = 0x47;
		c64mem1[0x0319] = 0xfe;
		
		// hardware vectors 
		if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
		{
			c64mem1[0xff48] = 0x6c;
			c64mem1[0xff49] = 0x14;
			c64mem1[0xff4a] = 0x03;
			c64mem1[0xfffa] = 0xf8;
			c64mem1[0xfffb] = 0xff;
			c64mem1[0xfffe] = 0x48;
			c64mem1[0xffff] = 0xff;
		}
		else
		{
			// NMI to $FE43
			c64mem1[0xfffa] = 0x43;
			c64mem1[0xfffb] = 0xfe;
			// RESET to $FCE2
			c64mem1[0xfffc] = 0xe2;
			c64mem1[0xfffd] = 0xfc;
			// IRQ to $FF48
			c64mem1[0xfffe] = 0x48;
			c64mem1[0xffff] = 0xff;
		}
		
		// clear SID
		for ( int i = 0; i < 0x1d; i++ )
		{
			c64mem2[0xd400 +i] = 0;
		}
		// default Mastervolume, no filter
		c64mem2[0xd418] = (sidLastValue = 0x0f);
	}
}


void c64memClear()
{ 
	// Clear entire RAM and ROM.
	for ( udword i = 0; i < 0x10000; i++ )
	{
		c64mem1[i] = 0; 
		if (memoryMode != MPU_PLAYSID_ENVIRONMENT)
		{
			c64mem2[i] = 0; 
		}
		sidLastValue = 0;
	}
	if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
	{
		// Fill Kernal-ROM address space with RTI instructions.
		for ( udword j = 0xE000; j < 0x10000; j++ )
		{
			c64mem1[j] = 0x40;
		}
	}
	else
	{
		// Fill Basic-ROM address space with RTS instructions.
		for ( udword j1 = 0xA000; j1 < 0xC000; j1++ )
		{
			c64mem2[j1] = 0x60;
		}
		// Fill Kernal-ROM address space with RTI instructions.
		for ( udword j2 = 0xE000; j2 < 0x10000; j2++ )
		{
			c64mem2[j2] = 0x40;
		}
	}
}


//  Input: A 16-bit effective address
// Output: A default bank-select value for $01.
ubyte c64memRamRom( uword address )
{ 
	if (memoryMode == MPU_PLAYSID_ENVIRONMENT)
	{
		return 4;  // RAM only, but special I/O mode
	}
	else
	{
		if ( address < 0xa000 )
		{
			return 7;  // Basic-ROM, Kernal-ROM, I/O
		}
		else if ( address < 0xd000 )
		{
			return 6;  // Kernal-ROM, I/O
		}
		else if ( address >= 0xe000 )
		{
			return 5;  // I/O only
		}
		else
		{
			return 4;  // RAM only
		}
	}
}
