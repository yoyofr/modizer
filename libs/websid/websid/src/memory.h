/*
* This contains everything to do with the emulation of C64 memory access.
* 
* <p>WebSid (c) 2019 Jürgen Wothke
* <p>version 0.93
* 
* Terms of Use: This software is licensed under a CC BY-NC-SA 
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/
 
#ifndef WEBSID_MEM_H
#define WEBSID_MEM_H

#include "base.h"

#define MEMORY_SIZE 65536

// setup/initialization
void	memResetBasicROM(uint8_t* rom);
void	memResetCharROM(uint8_t* rom);
void	memResetKernelROM(uint8_t* rom);
void	memResetRAM(uint8_t is_ntsc, uint8_t is_psid);
void	memResetIO();

void	memSetupBASIC(uint16_t len);
void	memRsidInit(uint16_t free_space, uint16_t* init_addr, uint8_t actual_subsong, uint8_t basic_mode);
void	memRsidMain(uint16_t free_space, uint16_t* init_addr);

// regular memory access (uses current bank settings)
uint8_t	memGet(uint16_t addr);
void	memSet(uint16_t addr, uint8_t value);

// RAM access 
uint8_t memReadRAM(uint16_t addr);
void	memWriteRAM(uint16_t addr, uint8_t value);
	// utils
uint8_t	memMatch(uint16_t addr, uint8_t* pattern, uint8_t len);
void	memCopyToRAM(uint8_t* src, uint16_t dest_addr, uint32_t len);
void	memCopyFromRAM(uint8_t* dest, uint16_t src_addr, uint32_t len);
void	memSaveSnapshot();
void	memRestoreSnapshot();


// I/O area access 
uint8_t	memReadIO(uint16_t addr);
void	memWriteIO(uint16_t addr, uint8_t value);

// PSID crap
void	memSetDefaultBanksPSID(uint8_t is_rsid, uint16_t init_addr, uint16_t load_end_addr);
void	memResetBanksPSID(uint16_t play_addr);
uint16_t memPsidMain(uint16_t free_space, uint16_t play_addr);

#ifdef TEST
void	memInitTest();
#endif

#endif