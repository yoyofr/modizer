////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////

//#include <windows.h>
#include "wsr_types.h" //YOYOFR
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
//#include <commctrl.h>
#include <stdio.h>
//#include <direct.h>
//#include <mbstring.h>
#include <sys/stat.h>
namespace oswan {
#include "oswan.h"
//#include "resource.h"
#include "ws.h"
#include "rom.h"
#include "io.h"
//#include "log.h"
//#include "draw.h"
//#include "input.h"
//#include "gpu.h"
#include "audio.h"
#include "./nec/nec.h"
#include "./nec/necintrf.h"

#define	FLASH_CMD_ADDR1			0x0AAA
#define	FLASH_CMD_ADDR2			0x0555
#define	FLASH_CMD_DATA1			0xAA
#define	FLASH_CMD_DATA2			0x55
#define	FLASH_CMD_RESET			0xF0
#define	FLASH_CMD_ERASE			0x80
#define	FLASH_CMD_ERASE_CHIP	0x10
#define	FLASH_CMD_ERASE_SECT	0x30
#define	FLASH_CMD_CONTINUE_SET	0x20
#define	FLASH_CMD_CONTINUE_RES1	0x90
#define	FLASH_CMD_CONTINUE_RES2	0xF0
#define	FLASH_CMD_CONTINUE_RES3	0x00
#define	FLASH_CMD_WRITE			0xA0

BYTE	*ws_rom;
DWORD	romSize;
DWORD	romAddressMask;
BYTE	*ws_staticRam;
DWORD	sramSize;
DWORD	sramAddressMask;
//BYTE	*externalEeprom;
//DWORD	externalEepromSize;
//DWORD	externalEepromAddressMask;
BYTE	*internalRam;
BYTE	*internalEeprom;

DWORD	ws_cycles=256/8;
DWORD	ws_cyclesByLine=256/8;
WORD	hblank_timer=0;
WORD	hblank_timer_preset=0;
WORD	vblank_timer=0;
WORD	vblank_timer_preset=0;
//DWORD	*sprite_table;
//BYTE	sprite_count=0;
//int		FrameSkip=0;
int		vsync=0;
int ws_gpu_scanline = 0;
int baseBank = 0;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void cpu_writemem20(DWORD addr,BYTE value)
{
  static int flash_command1    = 0;
  static int flash_command2    = 0;
  static int flash_write_set   = 0;
  static int flash_write_one   = 0;
  static int flash_write_reset = 0;
  static int write_enable      = 0;

  DWORD	offset = addr & 0xffff;
  DWORD	bank   = addr >> 16;

  if (!bank) { // 0 - RAM - 16 KB (WS) / 64 KB (WSC) internal RAM
    //ws_gpu_write_byte(offset, value);
	internalRam[offset] = value;	//WSR_PLAYER ws_gpu_write_byteä÷êîÇÃíÜÇ≈é¿çsÇ≥ÇÍÇƒÇ¢ÇΩÇÃÇ≈Ç±Ç±Ç…éùÇ¡ÇƒÇ´ÇΩ
    ws_audio_write_byte(offset, value);
  }
  else if (bank==1 && sramSize) { // 1 - SRAM (cart)
    // WonderWitch
    // FLASH ROM command sequence
    if (flash_command2) {
      if (offset == FLASH_CMD_ADDR1) {
        switch (value) {
        case FLASH_CMD_CONTINUE_SET:
          flash_write_set   = 1;
          flash_write_reset = 0;
          break;
        case FLASH_CMD_WRITE:
          flash_write_one = 1;
          break;
        case FLASH_CMD_RESET:
          break;
        case FLASH_CMD_ERASE:
          break;
        case FLASH_CMD_ERASE_CHIP:
          break;
        case FLASH_CMD_ERASE_SECT:
          break;
//        default:
//          load_write("unknown command %X\n", value);
        }
      }

      flash_command2 = 0;
    }
    else if (flash_command1) {
      if (offset == FLASH_CMD_ADDR2 && value == FLASH_CMD_DATA2)
        flash_command2 = 1;
      
      flash_command1 = 0;
    }
    else if (offset == FLASH_CMD_ADDR1 && value == FLASH_CMD_DATA1)
      flash_command1 = 1;

    if (sramSize != 0x40000 || ws_ioRam[0xc1] < 8) {
      // normal sram
      ws_staticRam[(offset | (ws_ioRam[0xc1] << 16)) & sramAddressMask] = value;
    }
    else if (ws_ioRam[0xc1] >= 8 && ws_ioRam[0xc1] < 15) {
      // FLASH ROM use SRAM bank(port 0xC1:8-14)(0xC1:15 0xF0000-0xFFFFF are write protected)
      if (write_enable || flash_write_one) {
        ws_rom[(unsigned) (offset + romSize - ((DWORD) (16 - ws_ioRam[0xc1]) << 16))] = value;
        write_enable    = 0;
        flash_write_one = 0;
      }
      else if (flash_write_set) {
        switch (value) {
        case FLASH_CMD_WRITE:
          write_enable      = 1;
          flash_write_reset = 0;
          break;
        case FLASH_CMD_CONTINUE_RES1:
          flash_write_reset = 1;
          break;
        case FLASH_CMD_CONTINUE_RES2:
        case FLASH_CMD_CONTINUE_RES3:
          if (flash_write_reset) {
            flash_write_set   = 0;
            flash_write_reset = 0;
          }
          break;
        default:
          flash_write_reset = 0;
        }
      }
    }
  }

  // other banks are read-only

}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
BYTE cpu_readmem20(DWORD addr)
{
	DWORD	offset = addr & 0xffff;
	DWORD	bank = (addr >> 16) & 0xf;

	int romBank;

	switch (bank)
	{
	case 0:		// 0 - RAM - 16 KB (WS) / 64 KB (WSC) internal RAM
		return internalRam[offset];
	case 1:		// 1 - SRAM (cart)
		return ws_staticRam[(offset | (ws_ioRam[0xc1] << 16)) & sramAddressMask];
	case 2:
	case 3:
		romBank = ws_ioRam[0xC0+bank];
		if (romBank < baseBank)
			return 0xff;
		else
			return ws_rom[(unsigned)(offset + ((romBank-baseBank)<<16))];
	default:
		romBank = ((ws_ioRam[0xC0]&0xf)<<4)|(bank&0xf);
		if (romBank < baseBank)
			return 0xff;
		else
			return ws_rom[(unsigned)(offset + ((romBank-baseBank)<<16))];
}
	return 0xff;

#if 0
	switch (bank) {
	case 0: // 0 - RAM - 16 KB (WS) / 64 KB (WSC) internal RAM
		return internalRam[offset];
	case 1: // 1 - SRAM (cart) 
		if (sramSize != 0x40000 || ws_ioRam[0xc1] < 8)
			return ws_staticRam[(offset | (ws_ioRam[0xc1] << 16)) & sramAddressMask];
		else {
			// WonderWitch
			// could not read FLASH ROM from SRAM bank
			return 0xa0;
		}
	case 2:
	case 3:
		return ws_rom[offset + ((ws_ioRam[0xc0 + bank] & ((romSize >> 16) - 1)) << 16)];
	default:
		int romBank = 0xff - (((ws_ioRam[0xc0] & 0xf) << 4) | bank);

		return ws_rom[offset + romSize - (((romBank & ((romSize >> 16) - 1)) + 1) << 16)];
	}

	return 0xff;
#endif

}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_patchRom(void)
{
	if((ws_rom[romSize-10]==0x01)&&(ws_rom[romSize-8]==0x27)) // Detective Conan 
	{
		//  WS cpu is using cache/pipeline or
		//  there's protected ROM bank where 
		//  pointing CS 
		ws_rom[0xfffe8]=0xea;
		ws_rom[0xfffe9]=0x00;
		ws_rom[0xfffea]=0x00;
		ws_rom[0xfffeb]=0x00;
		ws_rom[0xfffec]=0x20;
	}
	// Some games require initialized RAM 
    internalRam[0x75AC]=0x41;
    internalRam[0x75AD]=0x5F;
    internalRam[0x75AE]=0x43;
    internalRam[0x75AF]=0x31;
    internalRam[0x75B0]=0x6E;
    internalRam[0x75B1]=0x5F;
    internalRam[0x75B2]=0x63;
    internalRam[0x75B3]=0x31;
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
int ws_init(const void* p_data, size_t datasize)
{
  if ((ws_rom = ws_rom_load(p_data, datasize)) == NULL) {
   // log_write("Error: cannot load %s\n\n", rompath);
   // MessageBox(hWnd, "Rom cannot be loaded", "Error", MB_OK);
    return(0);
  }

 // externalEepromSize        = ws_rom_eepromSize();
  sramSize					= 0x10000;//ws_rom_sramSize();
  internalRam               = (BYTE*) malloc(0x10000);
  ws_staticRam              = (BYTE*) malloc(sramSize); 
 // externalEeprom            = (BYTE*) malloc(externalEepromSize); // ws_rom_eepromSize(ws_rom,romSize));
  internalEeprom            = (BYTE*) malloc(INTERNAL_EEPRON_SIZE);
  sramAddressMask           = sramSize - 1;
  //externalEepromAddressMask = externalEepromSize - 1;
  romAddressMask            = romSize - 1;
  baseBank = 0x100 - (romSize >> 16);
  //ws_load_sram(rompath);
  memset(ws_staticRam, 0, sramSize);
 // memset(externalEeprom, 0, externalEepromSize);
  memset(internalEeprom, 0, INTERNAL_EEPRON_SIZE);
  ws_patchRom();
  //ws_gpu_init();
  ws_io_init();

  //sprite_table = (DWORD*) malloc(128 * 4);
 // DWORD *ws_sprRamBase=(DWORD*)(internalRam+(((DWORD)ws_ioRam[0x04]&0x3F)<<9)+(ws_ioRam[0x05]<<2));
	//sprite_count=ws_ioRam[0x06];
	//if(sprite_count>0x80)
		//sprite_count=0x80;
	//memcpy(sprite_table,ws_sprRamBase,sprite_count*4);
	return(1);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_memory_reset(void)
{
	memset(internalRam,0,0x10000);
	memset(ws_staticRam, 0, sramSize);
	//memset(externalEeprom, 0, externalEepromSize);
	memset(internalEeprom, 0, INTERNAL_EEPRON_SIZE);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_memory_done(void)
{
  if (ws_rom) {
    free(ws_rom);
    ws_rom = NULL;
  }

  if (ws_staticRam) {
    free(ws_staticRam);
    ws_staticRam = NULL;
  }

  if (internalRam) {
    free(internalRam);
    internalRam = NULL;
  }

  //if (externalEeprom) {
    //free(externalEeprom);
   // externalEeprom = NULL;
  //}

  if (internalEeprom) {
	free(internalEeprom);
    internalEeprom = NULL;
  }
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_reset(void)
{
	ws_memory_reset();
	ws_audio_reset();
	ws_io_reset();
	//ws_gpu_reset();
	ws_gpu_scanline = 0;
	ws_cycles = 256 / 8;
	ws_cyclesByLine = 256 / 8;
	hblank_timer = 0;
	hblank_timer_preset = 0;
	vblank_timer = 0;
	vblank_timer_preset = 0;
   
	nec_reset(NULL);
	nec_set_reg(NEC_SP,0x2000);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
int WsExecuteLine(DWORD *framebuffer, BOOL renderLine)
{
  int drawWholeScreen = 0;
#if 0
  if (WsInputGetState() && (ws_ioRam[0xb2] & 0x02)) // Key Press Interrupt
    ws_ioRam[0xb6] |= 0x02;

  // sprite table buffering
  if (ws_gpu_scanline == 140) {
    DWORD* ws_sprRamBase = (DWORD*) (internalRam+(((DWORD)ws_ioRam[0x04]&0x3F)<<9)+(ws_ioRam[0x05]<<2));
    sprite_count=ws_ioRam[0x06];
    if (sprite_count > 0x80)
      sprite_count = 0x80;

    memcpy(sprite_table, ws_sprRamBase, sprite_count * 4);
  }

  if (renderLine && ws_gpu_scanline < 144)
    ws_gpu_renderScanline(framebuffer);
#endif
  if (hblank_timer == 1 && (ws_ioRam[0xb2] & 0x80)) // HBlank Timer Interrupt
    ws_ioRam[0xb6] |= 0x80;

  // Update HBlank Timer
  WORD hblank_count = (ws_ioRam[0xa8] | (ws_ioRam[0xa9] << 8)) + 1;
  ws_ioRam[0xa8] = (BYTE)  hblank_count;
  ws_ioRam[0xa9] = (BYTE) (hblank_count >> 8);

  if (hblank_timer && (ws_ioRam[0xa2] & 0x01)) { // HBLANK COUNTUP
    hblank_timer--;

    if (!hblank_timer && (ws_ioRam[0xa2] & 0x02))
      hblank_timer = hblank_timer_preset;

    ws_ioRam[0xa4] = (BYTE)  hblank_timer;
    ws_ioRam[0xa5] = (BYTE) (hblank_timer >> 8);
  }
	
  if (ws_gpu_scanline == 144) {
    drawWholeScreen = 1;

    // Update VBlank Timer
    DWORD vblank_count = ( ws_ioRam[0xaa]        | (ws_ioRam[0xab] <<  8) |
                          (ws_ioRam[0xac] << 16) | (ws_ioRam[0xad] << 24)) + 1;
    ws_ioRam[0xaa] = (BYTE)  vblank_count;
    ws_ioRam[0xab] = (BYTE) (vblank_count >>  8);
    ws_ioRam[0xac] = (BYTE) (vblank_count >> 16);
    ws_ioRam[0xad] = (BYTE) (vblank_count >> 24);

    if (ws_ioRam[0xb2] & 0x40) // VBlank Begin Interrupt
      ws_ioRam[0xb6] |= 0x40;

    if (vblank_timer == 1 && (ws_ioRam[0xb2] & 0x20)) // VBlank Timer Interrupt
      ws_ioRam[0xb6] |= 0x20;

    if (vblank_timer && (ws_ioRam[0xa2] & 0x04)) { // VBLANK COUNTUP 
      vblank_timer--;

      if (!vblank_timer && (ws_ioRam[0xa2] & 0x08))
        vblank_timer = vblank_timer_preset;

      ws_ioRam[0xa6] = (BYTE)  vblank_timer;
      ws_ioRam[0xa7] = (BYTE) (vblank_timer >> 8);
    }
  }

  if (ws_ioRam[0x02] == ws_ioRam[0x03] && (ws_ioRam[0xb2] & 0x10)) // Scanline Interrupt
    ws_ioRam[0xb6] |= 0x10;

  int jmax = 96000 / ws_audio_get_bps();
  int imax = 8 / jmax;

  for (int i = 0; i < imax; i++) {
    ws_audio_process(i);

    for (int j = 0; j < jmax; j++) {
      int exec_cycles = nec_execute(ws_cycles);

      ws_cycles += ws_cyclesByLine - exec_cycles;

      if (ws_ioRam[0xb6]) {
        int irqack = ws_ioRam[0xb6];
      
        for (int irqena = 7; irqena >= 0; irqena--) {
          if (irqack & 0x80) {
//            log_write("ACK %02X line %03d\n", ws_ioRam[0xb6], ws_ioRam[0x02]);
            nec_int((ws_ioRam[0xb0] + irqena) << 2);
            break;
          }

          irqack <<= 1;
        }
      }
    }
  }

  // update scanline register
  ws_gpu_scanline = (ws_gpu_scanline + 1) % 159;
  ws_ioRam[0x02] = ws_gpu_scanline;

  return drawWholeScreen;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
#if 0
void WsEmulate(void)
{
#if 0
  static int TblSkip[][10] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 0, 1, 1, 1, 1},
    {0, 1, 1, 0, 1, 1, 0, 1, 1, 1},
    {0, 1, 0, 1, 1, 0, 1, 0, 1, 1},
    {0, 1, 0, 1, 0, 1, 0, 1, 0, 1},
    {0, 0, 1, 0, 1, 0, 0, 1, 0, 1},
    {0, 0, 0, 1, 0, 0, 1, 0, 0, 1},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  };
#endif
  static int   SkipCnt    = 0;
  static int   SkipFrames = 0;
  static int   DrawFrames = 0;
  static DWORD nNormalLast;
  static DWORD nNormalFrac;
  static DWORD totalFrames;
  static DWORD StartT;
  static DWORD LastT;
  DWORD now;
  int   nTime;
  int   nCount;
  int   i;
  HMENU menu;
  char  buf[256];
#if 0
  now = timeGetTime();

  if (app_new_rom) {
    app_new_rom = 0;

    menu = GetMenu(hWnd);

    if (ws_init(ws_rom_path) == 0) {
      ws_rom_path[0] = '\0';

      EnableMenuItem(menu, ID_OPTIONS_WS_COLOR         , MF_ENABLED);
      EnableMenuItem(menu, ID_OPTIONS_WS_MONO          , MF_ENABLED);
      EnableMenuItem(menu, ID_FILE_SAVE_STATE1         , MF_GRAYED );
      EnableMenuItem(menu, ID_FILE_SAVE_STATE2         , MF_GRAYED );
      EnableMenuItem(menu, ID_FILE_SAVE_STATE3         , MF_GRAYED );
      EnableMenuItem(menu, ID_FILE_SAVE_STATE4         , MF_GRAYED );
      EnableMenuItem(menu, ID_FILE_SAVE_STATE5         , MF_GRAYED );
      EnableMenuItem(menu, ID_FILE_LOAD_STATE1         , MF_GRAYED );
      EnableMenuItem(menu, ID_FILE_LOAD_STATE2         , MF_GRAYED );
      EnableMenuItem(menu, ID_FILE_LOAD_STATE3         , MF_GRAYED );
      EnableMenuItem(menu, ID_FILE_LOAD_STATE4         , MF_GRAYED );
      EnableMenuItem(menu, ID_FILE_LOAD_STATE5         , MF_GRAYED );
      EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_START  , MF_GRAYED );
      EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_STOP   , MF_ENABLED);

      return;
    }

    oswan_set_recent_rom();
    app_gameRunning = 1;
    ws_reset();
    ws_set_system(ws_system);
    ws_set_colour_scheme(ws_colourScheme);

    nNormalLast = now;
    nNormalFrac = 0;
    totalFrames = 0;
    SkipFrames  = 0;
    DrawFrames  = 0;
    StartT      = 0;
    LastT       = now;

    EnableMenuItem(menu, ID_OPTIONS_WS_COLOR         , MF_GRAYED );
    EnableMenuItem(menu, ID_OPTIONS_WS_MONO          , MF_GRAYED );
    EnableMenuItem(menu, ID_FILE_SAVE_STATE1         , MF_ENABLED);
    EnableMenuItem(menu, ID_FILE_SAVE_STATE2         , MF_ENABLED);
    EnableMenuItem(menu, ID_FILE_SAVE_STATE3         , MF_ENABLED);
    EnableMenuItem(menu, ID_FILE_SAVE_STATE4         , MF_ENABLED);
    EnableMenuItem(menu, ID_FILE_SAVE_STATE5         , MF_ENABLED);
    EnableMenuItem(menu, ID_FILE_LOAD_STATE5         , MF_ENABLED);
    EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_START  , MF_ENABLED);
    EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_STOP   , MF_GRAYED );
		
    SetStateInfo();
    if (sramSize == 0x40000)
      sprintf(buf, "WonderWitch - %s", OSWAN_CAPTION);
    else if (ws_romHeader.supportSystem == WS_SYSTEM_MONO)
      sprintf(buf, "%s0%02X - %s", ws_rom_developper(), ws_romHeader.cartId, OSWAN_CAPTION);
    else
      sprintf(buf, "%sC%02X - %s", ws_rom_developper(), ws_romHeader.cartId, OSWAN_CAPTION);
		
    SetWindowText(hWnd, (LPCTSTR) buf);
  }

  if (app_gameRunning) {
    ws_audio_play();

    nTime  = now - nNormalLast;
    nCount = (nTime * 12 - nNormalFrac) / 159;
		
    if (vsync && nCount <= 0)
      nCount = 1;
		
    if (nCount <= 0) 
      Sleep(2); 
    else {
      nNormalFrac += nCount * 159;
      nNormalLast += nNormalFrac / 12;
      nNormalFrac %= 12;

      if (nCount > 10) 
        nCount = 10; 

      SkipCnt = (SkipCnt + 9) % 10;

      if (!TblSkip[FrameSkip][SkipCnt]) {
        SkipFrames++;
        nCount++;
      }

      for (i = 0; i < nCount - 1; i++) {
        while (!WsExecuteLine(backbuffer, FALSE));// frame skip
		DrawFrames++;
      }
#endif
      while (!WsExecuteLine(backbuffer, FALSE));
      DrawFrames++;
#if 0
      if (ws_ioRam[0x15] & 6)
        DrawMode |= ((ws_ioRam[0x15] >> 1) & 1);
      else
        DrawMode |= ws_rotated();

      if (DrawMode != OldMode) {
        OldMode = DrawMode;
        WsResize();
      }

      WsDrawFrame();
      if (ws_ioRam[0x15] != OldPort15) {
        OldPort15 = ws_ioRam[0x15];
        WsDrawLCDSegment();
      }
      WsDrawFlip();
#endif
      totalFrames++;
#if 0
      StartT = now;
      if ((StartT - LastT) >= 1000) {
        int fps     = DrawFrames + SkipFrames;
        int parcent = fps * 100 / 75;
        
        LastT      = StartT;
        DrawFrames = 0;
        SkipFrames = 0;

        if (!DrawFull) {
          if (StatusWait)
            StatusWait--;
          else
            oswan_set_status_text(0, "%d fps(%d%%)  Skip rate %d/10", fps, parcent, FrameSkip);
        }
      }
      
      if (ScrShoot) {
        ScrShoot = 0;
        screenshot();
      }
    }
  }
  else 
    Sleep(1);
#endif
}
#endif
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void ws_done(void)
{
  ws_memory_done();
  ws_io_done();
  //ws_gpu_done();
  ws_gpu_scanline = 0;

  //if (sprite_table) {
    //free(sprite_table);
    //sprite_table = NULL;
 // }

  //WsWaveClose();
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
//void	ws_set_colour_scheme(int scheme)
//{
//	//ws_gpu_set_colour_scheme(scheme);
//}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
void	ws_set_system(int system)
{
	//if (system==WS_SYSTEM_MONO)
		//ws_gpu_forceMonoSystem();
	//else
		//ws_gpu_forceColorSystem();
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
#if 0
#define MacroLoadNecRegisterFromFile(F,R)        \
		read(fp,&value,sizeof(value));		\
	    nec_set_reg(R,value); 

int	ws_loadState(char *statepath)
{
	log_write("loading %s\n",statepath);
	WORD	crc=ws_romHeader.checksum;
	WORD	newCrc;
	unsigned	value;

	int fp=open(statepath,O_BINARY|O_RDONLY);
	if (fp==NULL)
		return(0);
	read(fp,&newCrc,2);
	if (newCrc!=crc)
	{
		return(-1);
	}
	MacroLoadNecRegisterFromFile(fp,NEC_IP);
	MacroLoadNecRegisterFromFile(fp,NEC_AW);
	MacroLoadNecRegisterFromFile(fp,NEC_BW);
	MacroLoadNecRegisterFromFile(fp,NEC_CW);
	MacroLoadNecRegisterFromFile(fp,NEC_DW);
	MacroLoadNecRegisterFromFile(fp,NEC_CS);
	MacroLoadNecRegisterFromFile(fp,NEC_DS);
	MacroLoadNecRegisterFromFile(fp,NEC_ES);
	MacroLoadNecRegisterFromFile(fp,NEC_SS);
	MacroLoadNecRegisterFromFile(fp,NEC_IX);
	MacroLoadNecRegisterFromFile(fp,NEC_IY);
	MacroLoadNecRegisterFromFile(fp,NEC_BP);
	MacroLoadNecRegisterFromFile(fp,NEC_SP);
	MacroLoadNecRegisterFromFile(fp,NEC_FLAGS);
	MacroLoadNecRegisterFromFile(fp,NEC_VECTOR);
	MacroLoadNecRegisterFromFile(fp,NEC_PENDING);
	MacroLoadNecRegisterFromFile(fp,NEC_NMI_STATE);
	MacroLoadNecRegisterFromFile(fp,NEC_IRQ_STATE);
	
	read(fp,internalRam,65536);
	read(fp,ws_staticRam,sramSize);
	read(fp,ws_ioRam,256);
	read(fp,ws_paletteColors,8);
	read(fp,ws_palette,16*4*2);
	read(fp,wsc_palette,16*16*4);
	read(fp,&ws_videoMode,1);
	read(fp,&ws_gpu_scanline,1);
	read(fp,externalEeprom,externalEepromSize);	

	ws_audio_readState(fp);
	close(fp);
	
	// force a video mode change to make all tiles dirty
	ws_gpu_clearCache();
	return(1);
}
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
#define MacroStoreNecRegisterToFile(F,R)        \
	    value=nec_get_reg(R); \
		write(fp,&value,sizeof(value));

int	ws_saveState(char *statepath)
{
	WORD	crc=ws_romHeader.checksum;
	unsigned	value;

	log_write("saving %s\n",statepath);

	int	fp=open(statepath,O_BINARY|O_RDWR|O_CREAT, S_IREAD | S_IWRITE);

	if (fp==-1)
		return(0);
	write(fp,&crc,2);
	MacroStoreNecRegisterToFile(fp,NEC_IP);
	MacroStoreNecRegisterToFile(fp,NEC_AW);
	MacroStoreNecRegisterToFile(fp,NEC_BW);
	MacroStoreNecRegisterToFile(fp,NEC_CW);
	MacroStoreNecRegisterToFile(fp,NEC_DW);
	MacroStoreNecRegisterToFile(fp,NEC_CS);
	MacroStoreNecRegisterToFile(fp,NEC_DS);
	MacroStoreNecRegisterToFile(fp,NEC_ES);
	MacroStoreNecRegisterToFile(fp,NEC_SS);
	MacroStoreNecRegisterToFile(fp,NEC_IX);
	MacroStoreNecRegisterToFile(fp,NEC_IY);
	MacroStoreNecRegisterToFile(fp,NEC_BP);
	MacroStoreNecRegisterToFile(fp,NEC_SP);
	MacroStoreNecRegisterToFile(fp,NEC_FLAGS);
	MacroStoreNecRegisterToFile(fp,NEC_VECTOR);
	MacroStoreNecRegisterToFile(fp,NEC_PENDING);
	MacroStoreNecRegisterToFile(fp,NEC_NMI_STATE);
	MacroStoreNecRegisterToFile(fp,NEC_IRQ_STATE);

	write(fp,internalRam,65536);
	write(fp,ws_staticRam,sramSize);
	write(fp,ws_ioRam,256);
	write(fp,ws_paletteColors,8);
	write(fp,ws_palette,16*4*2);
	write(fp,wsc_palette,16*16*4);
	write(fp,&ws_videoMode,1);
	write(fp,&ws_gpu_scanline,1);
	write(fp,externalEeprom,externalEepromSize);	
	
	ws_audio_writeState(fp);
	close(fp);

	return(1);
}
#endif
////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//
////////////////////////////////////////////////////////////////////////////////
int ws_rotated(void)
{
	return(ws_rom[romSize-4]&1);
}
#if 0
void ws_save_sram(char *path)
{
  char  new_path[1024];
  FILE* file;

  if (sramSize || externalEepromSize) {
    sprintf(new_path, "%s%s", WsSramDir, (const char*) _mbsrchr((BYTE*) path, '\\'));

    unsigned int len   = strlen(new_path);
    unsigned int index = len;
    while (--len >= 0) {
      if (new_path[len] == '.') {
        index = len;
        break;
      }

      if (new_path[len] == '\\')
        break;
    }

    sprintf(&new_path[index], ".sav");

    file = fopen(new_path, "wb");
    if (file) {
      if (sramSize)
        fwrite(ws_staticRam, 1, sramSize, file);
      else if (externalEepromSize)
        fwrite(externalEeprom, 1, externalEepromSize, file);

      fclose(file);
    }
  }

  sprintf(new_path, "%s\\Oswan.eep", WsHomeDir);

  file = fopen(new_path, "wb");
  if (file) {
    fwrite(internalEeprom, 1, INTERNAL_EEPRON_SIZE, file);
    fclose(file);
  }

  // WonderWitch
  if (sramSize == 0x40000) {
    file = fopen(ws_rom_path, "wb");
    if (file) {
      fwrite(ws_rom, 1, romSize, file);
      fclose(file);
    }
  }
}

void ws_load_sram(char *path)
{
  char  new_path[1024];
  FILE* file;

  memset(ws_staticRam  , 0, sramSize          );
  memset(externalEeprom, 0, externalEepromSize);

  sprintf(new_path, "%s%s", WsSramDir, (const char*) _mbsrchr((BYTE*) path, '\\'));

  unsigned int len   = strlen(new_path);
  unsigned int index = len;
  while (--len >= 0) {
    if (new_path[len] == '.') {
      index = len;
      break;
    }

    if (new_path[len] == '\\')
      break;
  }

  sprintf(&new_path[index], ".sav");

  file = fopen(new_path, "rb");
  if (file) {
    if (sramSize)
      fread(ws_staticRam, 1, sramSize, file);
    else if (externalEepromSize)
      fread(externalEeprom, 1, externalEepromSize, file);

    fclose(file);
  }

  sprintf(new_path, "%s\\Oswan.eep", WsHomeDir);

  file = fopen(new_path, "rb");
  if (file) {
    fread(internalEeprom, 1, INTERNAL_EEPRON_SIZE, file);
    fclose(file);
  }
  else
    memset(internalEeprom, 0, INTERNAL_EEPRON_SIZE);
}
#endif
}
