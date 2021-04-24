/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

struct SDMA
{
	bool ReverseTransfer;
	bool HDMAIndirectAddressing;
	bool UnusedBit43x0;
	bool AAddressFixed;
	bool AAddressDecrement;
	uint8_t TransferMode;
	uint8_t BAddress;
	uint16_t AAddress;
	uint8_t ABank;
	uint16_t DMACount_Or_HDMAIndirectAddress;
	uint8_t IndirectBank;
	uint16_t Address;
	bool Repeat;
	uint8_t LineCount;
	uint8_t UnknownByte;
	bool DoTransfer;
};

extern SDMA DMA[8];

bool S9xDoDMA(uint8_t);
void S9xStartHDMA();
uint8_t S9xDoHDMA(uint8_t);
void S9xResetDMA();
