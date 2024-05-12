//////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __IO_H__
#define __IO_H__
#define INTERNAL_EEPRON_SIZE 128

extern	BYTE	*ws_ioRam;
extern	BYTE	ws_key_start;
extern	BYTE	ws_key_left;
extern	BYTE	ws_key_right;
extern	BYTE	ws_key_up;
extern	BYTE	ws_key_down;
extern	BYTE	ws_key_left_y;
extern	BYTE	ws_key_right_y;
extern	BYTE	ws_key_up_y;
extern	BYTE	ws_key_down_y;
extern	BYTE	ws_key_button_1;
extern	BYTE	ws_key_button_2;

extern	BYTE	SerialInputBuffer[256];
extern	BYTE	SerialOutputBuffer[256];
extern	int		SerialInputIndex;
extern	int		SerialOutputIndex;

void ws_io_init(void);
void ws_io_reset(void);
void ws_io_flipControls(void);
void ws_io_done(void);

#endif