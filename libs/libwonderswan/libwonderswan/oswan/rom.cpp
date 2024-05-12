/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//#include <windows.h>
#include "wsr_types.h" //YOYOFR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <malloc.h>
#include <io.h>
#include <fcntl.h>
namespace oswan {
#include "oswan.h"
#include "ws.h"
//#include "log.h"
#include "rom.h"
//#include "zlib/unzip.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
// variables
/////////////////////////////////////////////////////////////////////////////////////////////////
ws_romHeaderStruct ws_romHeader;

/////////////////////////////////////////////////////////////////////////////////////////////////
// static variables
/////////////////////////////////////////////////////////////////////////////////////////////////
static const BYTE WS_ROM_SIZE_2MBIT   = 1;
static const BYTE WS_ROM_SIZE_4MBIT   = 2;
static const BYTE WS_ROM_SIZE_8MBIT   = 3;
static const BYTE WS_ROM_SIZE_16MBIT  = 4;
static const BYTE WS_ROM_SIZE_24MBIT  = 5;
static const BYTE WS_ROM_SIZE_32MBIT  = 6;
static const BYTE WS_ROM_SIZE_48MBIT  = 7;
static const BYTE WS_ROM_SIZE_64MBIT  = 8;
static const BYTE WS_ROM_SIZE_128MBIT = 9;

static const BYTE WS_SRAM_SIZE_NONE  = 0;
static const BYTE WS_SRAM_SIZE_64k   = 1;
static const BYTE WS_SRAM_SIZE_256k  = 2;
static const BYTE WS_SRAM_SIZE_1024k = 3;
static const BYTE WS_SRAM_SIZE_2048k = 4;
static const BYTE WS_SRAM_SIZE_4096k = 5;

static const BYTE WS_EEPROM_SIZE_NONE = 0x00;
static const BYTE WS_EEPROM_SIZE_1k   = 0x10;
static const BYTE WS_EEPROM_SIZE_16k  = 0x20;
static const BYTE WS_EEPROM_SIZE_8k   = 0x50;

static const char *devCode[] = {
  "***", "BAN", "TAT", "TMY", "KEX", "DTE", "AAE","MDE", // 00-07
  "NHB", "***", "CCJ", "SUM", "SUN", "PAW", "BPR","***", // 08-0f
  "JLC", "MGA", "KNM", "***", "***", "***", "KBS","BTM", // 10-17
  "KGT", "SRV", "CFT", "MGH", "***", "BEC", "NAP","BVL", // 18-1f
  "ATN", "KDX", "HAL", "YKE", "OMM", "LAY", "KDK","SHL", // 20-27
  "SQR", "***", "SCC", "TMC", "***", "NMC", "SES","HTR", // 28-2f
  "***", "VGD", "MGT", "WIZ", "***", "***", "CPC","DDJ", // 30-37
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// static functions
/////////////////////////////////////////////////////////////////////////////////////////////////
BYTE* ws_rom_load_zip(char* path);
static int   ws_rom_set_header(BYTE* rom, DWORD rom_size);
static void  ws_rom_patch_rom(BYTE* rom, int rom_size, int checksum);
//void  ws_rom_dump_info(BYTE* rom, int rom_size, int checksum);

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BYTE* ws_rom_load(const void* p_data, size_t datasize)
{
  //ZeroMemory(&ws_romHeader, sizeof(ws_romHeaderStruct));
    memset(&ws_romHeader,0, sizeof(ws_romHeaderStruct));
  
  romSize = datasize;
  DWORD romBank = (datasize + 0xffff) >> 16;
  BYTE* rom = (BYTE*)malloc(0x10000 * romBank);

  if (rom == NULL) {
	  return NULL;
  }
   
  memcpy(rom, p_data, datasize);
  if (ws_rom_set_header(rom, romSize))
    return rom;

  return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
DWORD ws_rom_romSize(void)
{
  switch (ws_romHeader.romSize) {
    case WS_ROM_SIZE_2MBIT:   return   2;
    case WS_ROM_SIZE_4MBIT:   return   4;
    case WS_ROM_SIZE_8MBIT:   return   8;
    case WS_ROM_SIZE_16MBIT:  return  16;
    case WS_ROM_SIZE_24MBIT:  return  24;
    case WS_ROM_SIZE_32MBIT:  return  32;
    case WS_ROM_SIZE_48MBIT:  return  48;
    case WS_ROM_SIZE_64MBIT:  return  64;
    case WS_ROM_SIZE_128MBIT: return 128;
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
DWORD ws_rom_eepromSize(void)
{
  switch (ws_romHeader.eepromSize & 0xf0) {
  case WS_EEPROM_SIZE_NONE:	return 0x000;
  case WS_EEPROM_SIZE_1k:   return 0x080;
  case WS_EEPROM_SIZE_16k:  return 0x800;
  case WS_EEPROM_SIZE_8k:   return 0x400;
  }
  
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
DWORD ws_rom_sramSize(void)
{
  switch (ws_romHeader.eepromSize & 0xf) {
  case WS_SRAM_SIZE_NONE:  return 0x00000;
  case WS_SRAM_SIZE_64k:   return 0x02000;
  case WS_SRAM_SIZE_256k:  return 0x08000;
  case WS_SRAM_SIZE_1024k: return 0x20000;
  case WS_SRAM_SIZE_2048k: return 0x40000;
  case WS_SRAM_SIZE_4096k: return 0x80000;
  }
  
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
const char* ws_rom_developper(void)
{
  return (ws_romHeader.developperId < 0x38 ? devCode[ws_romHeader.developperId] : "???");
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
static BYTE* ws_rom_load_zip(char* path) {
  unzFile fp;

  if (!(fp = unzOpen(path)))
    return NULL;

  if (unzGoToFirstFile(fp) != UNZ_OK) {
    unzClose(fp);
    return NULL;
  }

  BYTE*         rom;
  unz_file_info file_info;
  char          szFileName[256];
  unsigned long gROMLength;

  do {
    if (unzGetCurrentFileInfo(fp, &file_info, szFileName, 256, NULL, 0, NULL, 0) != UNZ_OK)
      continue;

    if (stricmp(&szFileName[strlen(szFileName) - 3], ".ws" ) != 0 &&
        stricmp(&szFileName[strlen(szFileName) - 4], ".wsc") != 0)
      continue;

    if (unzOpenCurrentFile(fp) != UNZ_OK)
      continue;

    gROMLength = file_info.uncompressed_size;
    rom        = (BYTE*) malloc(gROMLength);

    if (unzReadCurrentFile(fp, rom, gROMLength) == (int) gROMLength) {
      unzCloseCurrentFile(fp);
      unzClose(fp);
      romSize = gROMLength;
      return rom;
    }
    else {
      unzCloseCurrentFile(fp);
      unzClose(fp);
      free(rom);
      return NULL;
    }

    unzCloseCurrentFile(fp);
  }
  while(unzGoToNextFile(fp) == UNZ_OK);

  unzClose(fp);

  return NULL;
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static int ws_rom_set_header(BYTE* rom, DWORD rom_size)
{
  if (rom == NULL || rom_size < sizeof(ws_romHeaderStruct))
    return 0;

  memcpy(&ws_romHeader, rom + rom_size - sizeof(ws_romHeaderStruct), sizeof(ws_romHeaderStruct));

  int checksum = 0;
  for (DWORD i = 0; i < rom_size - 2; i++)
    checksum = (checksum + rom[i]) & 0xffff;

 // ws_rom_patch_rom(rom, rom_size, checksum);

//  if (ws_rom_romSize() * 1024 * 1024 / 8 != rom_size)
//    return 0;

  //ws_rom_dump_info(rom, rom_size, checksum);

  return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////

static void ws_rom_patch_rom(BYTE* rom, int rom_size, int checksum)
{
  // turn tablist rom patch
  if (ws_romHeader.developperId == 0x00 && ws_romHeader.cartId == 0x17 && checksum == 0x7c1d) {
    ws_romHeader.supportSystem = WS_SYSTEM_MONO;
    return;
  }

  // degital partner rom patch
  if (ws_romHeader.developperId == 0x01 && ws_romHeader.cartId == 0x2c && checksum == 0x28ff) {
    ws_romHeader.eepromSize = WS_SRAM_SIZE_256k;
    return;
  }

  // trump collection2 rom patch (bad dump rom?)
  if (ws_romHeader.developperId == 0x17 && ws_romHeader.cartId == 0x02 && checksum == 0x14e2) {
    if (rom_size == 0x100000)
      ws_romHeader.romSize = WS_ROM_SIZE_8MBIT;

    return;
  }

  // yamato rom patch
  if (ws_romHeader.developperId == 0x01 && ws_romHeader.cartId == 0x09 && checksum == 0x50ca) {
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    return;
  }

  // konan3 rom patch
  if (ws_romHeader.developperId == 0x01 && ws_romHeader.cartId == 0x0e && checksum == 0x9238) {
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    return;
  }

  // rakujyan rom patch
  if (ws_romHeader.developperId == 0x01 && ws_romHeader.cartId == 0x10 && checksum == 0x751f) {
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    return;
  }

  // degimon medley rom patch
  if (ws_romHeader.developperId == 0x01 && ws_romHeader.cartId == 0x14 && checksum == 0x698f) {
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    return;
  }

  // start hearts rom patch
  if (ws_romHeader.developperId == 0x01 && ws_romHeader.cartId == 0x16 && checksum == 0x8eed) {
    ws_romHeader.eepromSize = WS_SRAM_SIZE_256k;
    return;
  }

  // start hearts (demo) rom patch
  if (ws_romHeader.developperId == 0x00 && ws_romHeader.cartId == 0x00 && checksum == 0xb5a8) {
    ws_romHeader.eepromSize = WS_SRAM_SIZE_256k;
    return;
  }

  // degimon battle spirit rom patch
  if (ws_romHeader.developperId == 0x01 && ws_romHeader.cartId == 0x1a && checksum == 0x04f1) {
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    return;
  }

  // degimon battle spirit Ver 1.5 rom patch
  if (ws_romHeader.developperId == 0x01 && ws_romHeader.cartId == 0x30 && checksum == 0x443e) {
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    return;
  }

  // dekotora densetsu rom patch
  if (ws_romHeader.developperId == 0x18 && ws_romHeader.cartId == 0x03 && checksum == 0xd6ec) {
    ws_romHeader.eepromSize = WS_SRAM_SIZE_256k;
    return;
  }

  //  final fantasy rom patch
  if (ws_romHeader.developperId == 0x28 && ws_romHeader.cartId == 0x01 && checksum == 0x26db) {
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    return;
  }

  //  hataraku chokobo rom patch
  if (ws_romHeader.developperId == 0x28 && ws_romHeader.cartId == 0x04 && checksum == 0xe0af) {
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    return;
  }

  // sorobang rom patch
  if (ws_romHeader.developperId == 0x18 && ws_romHeader.cartId == 0x09 && checksum == 0xb87c) {
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    return;
  }

  //  guilty gear rom patch
  if (ws_romHeader.developperId == 0x0b && ws_romHeader.cartId == 0x07 && checksum == 0xbfdf) {
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    return;
  }

  // RUN=DIM rom patch
  if (ws_romHeader.developperId == 0x00 && ws_romHeader.cartId == 0x80 && checksum == 0xbea4) {
    ws_romHeader.developperId  = 0x37;
    ws_romHeader.cartId        = 0x01;
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    ws_romHeader.checksum      = 0xbea4;
    return;
  }

  //  crush gear rom patch
  if (ws_romHeader.developperId == 0x33 && ws_romHeader.cartId == 0x01 && checksum == 0xee90) {
    ws_romHeader.eepromSize = WS_SRAM_SIZE_256k;
    return;
  }

  // dicing knight rom patch
  if (ws_romHeader.developperId == 0x00 && ws_romHeader.cartId == 0x00 && checksum == 0x8b1c) {
    ws_romHeader.romSize       = WS_ROM_SIZE_16MBIT;
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    ws_romHeader.checksum      = 0x8b1c;
    return;
  }

  // judgement silversword rom patch
  if (ws_romHeader.developperId == 0x00 && ws_romHeader.cartId == 0x00 && checksum == 0x9deb) {
    ws_romHeader.romSize       = WS_ROM_SIZE_16MBIT;
    ws_romHeader.supportSystem = WS_SYSTEM_COLOR;
    ws_romHeader.checksum      = 0x9deb;
    return;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
static void ws_rom_dump_info(BYTE* rom, int rom_size, int checksum)
{
  log_write("rom: developper Id  0x%.2x(%s)\n", ws_romHeader.developperId, ws_rom_developper());
  log_write("rom: cart Id        0x%.2x\n", ws_romHeader.cartId);
  log_write("rom: support system %s\n", ws_romHeader.supportSystem == WS_SYSTEM_MONO ?
                                        "Wonderswan mono" : "Wonderswan color");
  log_write("rom: size           %i Mbits\n", ws_rom_romSize());

  log_write("rom: sram           ");
  switch (ws_romHeader.eepromSize & 0xf) {
  case WS_SRAM_SIZE_NONE:  log_write("None\n"     ); break;
  case WS_SRAM_SIZE_64k:   log_write("64 Kbits\n" ); break;
  case WS_SRAM_SIZE_256k:  log_write("256 Kbits\n"); break;
  case WS_SRAM_SIZE_1024k: log_write("1 Mbits\n"  ); break;
  case WS_SRAM_SIZE_2048k: log_write("2 Mbits\n"  ); break;
  case WS_SRAM_SIZE_4096k: log_write("4 Mbits\n"  ); break;
  default:                 log_write("Unknown\n"  );
  }

  log_write("rom: eeprom         ");
  switch (ws_romHeader.eepromSize & 0xf0) {
  case WS_EEPROM_SIZE_NONE: log_write("None\n"    ); break;
  case WS_EEPROM_SIZE_1k:   log_write("1 Kbits\n" ); break;
  case WS_EEPROM_SIZE_16k:  log_write("16 Kbits\n"); break;
  case WS_EEPROM_SIZE_8k:   log_write("8 Kbits\n" ); break;
  default:                  log_write("Unknown\n" );
  }
	
  log_write("rom: rtc            %s\n", ws_romHeader.realtimeClock ? "Yes" : "None");
  log_write("rom: checksum       0x%.4x(0x%.4x)\n\n", ws_romHeader.checksum, checksum);
}
#endif
}
