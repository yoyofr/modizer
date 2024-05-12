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

#ifndef __WS_H__
#define __WS_H__

extern BYTE*  ws_rom;
extern DWORD  romSize;
extern DWORD  romAddressMask;
extern BYTE*  externalEeprom;
extern DWORD  externalEepromAddressMask;
extern BYTE*  internalRam;
extern BYTE*  internalEeprom;
extern DWORD* sprite_table;
extern BYTE   sprite_count;
extern DWORD  ws_cyclesByLine;
extern int    FrameSkip;
extern DWORD  vblank_count;
extern WORD   hblank_timer;
extern WORD   hblank_timer_preset;
extern WORD   vblank_timer;
extern WORD   vblank_timer_preset;
extern int    vsync;

int	 ws_init(const void* p_data, size_t datasize);
void ws_done(void);
int	 WsExecuteLine(DWORD*, BOOL);
void WsEmulate(void);
void ws_reset();
//void ws_set_colour_scheme(int);
void ws_set_system(int);
int  ws_rotated(void);
int  ws_loadState(char*);
int  ws_saveState(char*);
void ws_load_sram(char*);
void ws_save_sram(char*);

#endif
