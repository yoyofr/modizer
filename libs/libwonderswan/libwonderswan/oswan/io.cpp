////////////////////////////////////////////////////////////////////////////////
// I/O ports
////////////////////////////////////////////////////////////////////////////////
//
//
//
//
//
//
//////////////////////////////////////////////////////////////////////////////

//#include <windows.h>
#include "wsr_types.h" //YOYOFR
#include <stdlib.h>

#include <stdio.h>

#include <time.h>
namespace oswan {
//#include "oswan.h"
#include "ws.h"
#include "io.h"
//#include "log.h"
#include "rom.h"
#include "./nec/nec.h"
#include "./nec/necintrf.h"
#include "initialio.h"
//#include "gpu.h"
#include "audio.h"
//#include "witch.h"

BYTE	*ws_ioRam = NULL;

BYTE	ws_key_start;
BYTE	ws_key_left;
BYTE	ws_key_right;
BYTE	ws_key_up;
BYTE	ws_key_down;
BYTE	ws_key_left_y;
BYTE	ws_key_right_y;
BYTE	ws_key_up_y;
BYTE	ws_key_down_y;
BYTE	ws_key_button_1;
BYTE	ws_key_button_2;

BYTE	SerialInputBuffer[256];
BYTE	SerialOutputBuffer[256];
int		SerialInputIndex = 0;
int		SerialOutputIndex = 0;

int		rtcDataRegisterReadCount=0;

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
void ws_io_reset(void)
{
	int i = 0;
    ws_key_start=0;
    ws_key_left=0;
    ws_key_right=0;
    ws_key_up=0;
    ws_key_down=0;
    ws_key_left_y=0;
    ws_key_right_y=0;
    ws_key_up_y=0;
    ws_key_down_y=0;
    ws_key_button_1=0;
    ws_key_button_2=0;

    for (i=0;i<0x80;i++)
        cpu_writeport(i,initialIoValue[i]);
    for (i=0x80;i<0x95;i++)
        ws_ioRam[i]=initialIoValue[i];
    for (i=0x95;i<0xc9;i++)
		cpu_writeport(i,initialIoValue[i]);
	ws_ioRam[0xb6]=0;
	SerialInputIndex = 0;
	SerialOutputIndex = 0;
    rtcDataRegisterReadCount = 0;
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
void ws_io_init(void)
{
    if (ws_ioRam == NULL)
        ws_ioRam = (BYTE*)malloc(0x100);

    ws_io_reset();
    //DrawMode = 0;
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
/*void ws_io_flipControls(void)
{
    ws_key_flipped=!ws_key_flipped;
}*/
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
void ws_io_done(void)
{
  if (ws_ioRam) {
    free(ws_ioRam);
    ws_ioRam = NULL;
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
BYTE cpu_readport(BYTE port)
{
    int		w1,w2,i;
	BYTE	value;

//    log_write("read port %02X = %02X\n", port, ws_ioRam[port]);

    switch (port)
    {
    case 0x4e:
    case 0x4f:
    case 0x50:
    case 0x51:  
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83:
    case 0x84:
    case 0x85:
    case 0x86:
    case 0x87:
    case 0x88:
    case 0x89:
    case 0x8a:
    case 0x8b:
    case 0x8c:
    case 0x8d:
    case 0x8e:
    case 0x8f:
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
                return(ws_audio_port_read(port));
	case 0xa0: return ws_ioRam[0xa0] | 2;
    case 0xb1:
				if (SerialInputIndex == 0) return 0;
				value = SerialInputBuffer[0];
				SerialInputIndex--;
				for (i = 0; i < SerialInputIndex; i++) {
					SerialInputBuffer[i] = SerialInputBuffer[i+1];
				}
				return(value);
    case 0xb2:
//				fprintf(log_get(),"read port %02X\n",port);
                break;
    case 0xb3:  // Serial IO
				value = ws_ioRam[0xb3];
				if (SerialInputIndex) value |= 0x01; // data is received in buffer
				value |= 0x04; // data is sent to buffer
                return(value);
    case 0xb5:  w1=ws_ioRam[0xb5];
                w2=0x00;
#if 0
                if(w1&0x40) //start A B
                {
                    w2=(ws_key_start<<1)|(ws_key_button_1<<2)|(ws_key_button_2<<3);
                }
                else if(w1&0x20) // X cursor
                {
                   if (DrawMode & 1)
                       w2=(ws_key_up<<1)|(ws_key_right<<2)|(ws_key_down<<3)|(ws_key_left<<0);
                   else
                        w2=(ws_key_up<<0)|(ws_key_right<<1)|(ws_key_down<<2)|(ws_key_left<<3);
                }
                else if(w1&0x10) // Y cursor
                {
                    if (DrawMode & 1)
                        w2=(ws_key_up_y<<1)|(ws_key_right_y<<2)|(ws_key_down_y<<3)|(ws_key_left_y);
                    else
                        w2=(ws_key_up_y<<0)|(ws_key_right_y<<1)|(ws_key_down_y<<2)|(ws_key_left_y<<3);
                }
#endif
                return (BYTE)((w1&0xf0)|w2);
    case 0xba:  // eeprom even byte read
				return ws_ioRam[0xba];
    case 0xbb:  // eeprom odd byte read
				return ws_ioRam[0xbb];
    case 0xbe:  // internal eeprom status/command register
                return ws_ioRam[0xbe]&3;

    case 0xc0 : // ???
                return ((ws_ioRam[0xc0]&0xf)|0x20);
    case 0x00 :
    case 0x01 :
    case 0xc1 :
    case 0xc2 :
    case 0xc3 :
				return (ws_ioRam[port]);

    case 0xc4:  // external eeprom even byte read
#if 0
                w1=(((((WORD)ws_ioRam[0xc7])<<8)|((WORD)ws_ioRam[0xc6]))<<1)&(externalEepromAddressMask);
                return externalEeprom[w1];
#endif
				return 0;

    case 0xc5:  // external eeprom odd byte read
#if 0
                w1=(((((WORD)ws_ioRam[0xc7])<<8)|((WORD)ws_ioRam[0xc6]))<<1)&(externalEepromAddressMask);
                return externalEeprom[w1+1];
#endif
				return 0;

    case 0xc8:  // external eeprom status/command register
                return ws_ioRam[0xc8]&3;

    case 0xca : // RTC Command and status register
                // set ack to always 1
                return (ws_ioRam[0xca]|0x80);
    case 0xcb : // RTC data register
		return 0;
#if 0
                if(ws_ioRam[0xca]==0x15)    // get time command 
                {
					BYTE year, mon, mday, wday, hour, min, sec, j;
                    struct tm *newtime;
                    time_t long_time;
                    time( &long_time );                
                    newtime = localtime( &long_time ); 

                    #define  BCD(value) ((value/10)<<4)|(value%10)
                    switch(rtcDataRegisterReadCount)
                    {
                      case 0:   rtcDataRegisterReadCount++;
								year = newtime->tm_year;
								year %= 100;
                                return BCD(year);
                      case 1:   rtcDataRegisterReadCount++;
								mon = newtime->tm_mon;
								mon++;
                                return BCD(mon);
                      case 2:   rtcDataRegisterReadCount++;
								mday = newtime->tm_mday;
                                return BCD(mday);
                      case 3:   rtcDataRegisterReadCount++;
								wday = newtime->tm_wday;
                                return BCD(wday);
                      case 4:   rtcDataRegisterReadCount++;
								hour = newtime->tm_hour;
                                j = BCD(hour);
//high bit denotes PM, e.g. 0x00 to 0x11 for 00:00 to 11:00 and 0x92 to 0xA3 for 12:00 to 23:00
								if(hour>11)
									j |= 0x80;
								return j;
                      case 5:   rtcDataRegisterReadCount++;
								min = newtime->tm_min;
                                return BCD(min);
                      case 6:   rtcDataRegisterReadCount=0;
								sec = newtime->tm_sec;
                                return BCD(sec);
                    }
                    return 0;
                }
                else
                {
                    // set ack
                    return (ws_ioRam[0xcb]|0x80);
                }
#endif
    }
	return ws_ioRam[port];
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
void cpu_writeport(DWORD port,BYTE value)
{
  int w1,n,i,j;
  int unknown_io_port=0;

#define DMASL   ws_ioRam[0x40]      
#define DMASH   ws_ioRam[0x41]      
#define DMASB   ws_ioRam[0x42]      
#define DMADB   ws_ioRam[0x43]      
#define DMADL   ws_ioRam[0x44]      
#define DMADH   ws_ioRam[0x45]      
#define DMACL   ws_ioRam[0x46]      
#define DMACH   ws_ioRam[0x47]      
#define DMACTL  ws_ioRam[0x48]

  switch (port) {
  case 0x00:
  case 0x01:
  case 0x02:
  case 0x03:
  case 0x04:
  case 0x05:
  case 0x06:
  case 0x07:
//    log_write("%02x -> port %02X\n", value, port);
    break;
  case 0x08:
  case 0x09:
  case 0x0a:
  case 0x0b:
//    log_write("%03d -> port %02X\n", value, port);
    break;
  case 0x0c:
  case 0x0d:
  case 0x0e:
  case 0x0f:
//    log_write("%03d -> port %02X\n", value, port);
    break;
  case 0x10:
  case 0x11:
  case 0x12:
  case 0x13:
  case 0x14:
//    log_write("%03d -> port %02X\n", value, port);
    break;
  case 0x15:
   // if (value & 0x04)
     // DrawMode &= 2;
   // else if (value & 0x02)
     // DrawMode |= 1;

    break;
  case 0x40:
  case 0x41:
  case 0x42:
  case 0x43:
  case 0x44:
  case 0x45:
  case 0x46:
  case 0x47:
    break;
  case 0x48:
    if (value & 0x80) {
      n =                 (DMACH << 8) | DMACL;
      i = (DMASB << 16) | (DMASH << 8) | DMASL;
      j =                 (DMADH << 8) | DMADL;

      while (n > 0) {
        cpu_writemem20(j, cpu_readmem20(i));
        i++;
        j++;
        n--;
      }

      DMASB = (BYTE) ((i >> 16) & 0xff);
      DMASH = (BYTE) ((i >>  8) & 0xff);
      DMASL = (BYTE) ( i        & 0xff);
      DMADB = (BYTE) ((j >> 16) & 0xff);
      DMADH = (BYTE) ((j >>  8) & 0xff);
      DMADL = (BYTE) ( j        & 0xff);
      DMACH = (BYTE) ((n >>  8) & 0xff);
      DMACL = (BYTE) ( n        & 0xff);

      value &= 0x7f;
    }

    break;
  case 0x4a:
  case 0x4b:
  case 0x4c:
  case 0x4d:
  case 0x4e:
  case 0x4f:
  case 0x50:
  case 0x51:
  case 0x52:
//    oswan_set_status_text(1, "hyper voice port %02X = %02X\n", port, value);
//    log_write("hyper voice port %02X = %02X\n", port, value);
  case 0x80:
  case 0x81:
  case 0x82:
  case 0x83:
  case 0x84:
  case 0x85:
  case 0x86:
  case 0x87:
  case 0x88:
  case 0x89:
  case 0x8a:
  case 0x8b:
  case 0x8c:
  case 0x8d:
  case 0x8e:
  case 0x8f:
  case 0x90:
  case 0x91:
  case 0x92:
  case 0x93:
  case 0x94:
    ws_audio_port_write(port, value);
    return;
  case 0xa2:
//    log_write("%02X -> port %02X\n", value, port);
    if (value & 0x01)
      hblank_timer = hblank_timer_preset;
    else
      hblank_timer = 0;

    if (value & 0x04)
      vblank_timer = vblank_timer_preset;
    else
      vblank_timer = 0;

    break;
  case 0xa4:
    hblank_timer_preset = (ws_ioRam[0xa5] << 8) | value;
    hblank_timer        = hblank_timer_preset;
    break;
  case 0xa5:
    hblank_timer_preset = (value << 8) | ws_ioRam[0xa4];
    hblank_timer        = hblank_timer_preset;
    break;
  case 0xa6:
    vblank_timer_preset = (ws_ioRam[0xa7] << 8) | value;
    vblank_timer        = vblank_timer_preset;
    break;
  case 0xa7:
    vblank_timer_preset = (value << 8) | ws_ioRam[0xa6];
    vblank_timer        = vblank_timer_preset;
    break;
  case 0xb1:
//    log_write("%02X -> port %02X\n", value, port);
    ws_ioRam[0xb1] = value;
    if (SerialOutputIndex > sizeof(SerialOutputBuffer))
      return;

    SerialOutputBuffer[SerialOutputIndex] = value;
    SerialOutputIndex++;
   // SendMessage(hWitchWnd, WM_USER_SEND, 0, 0);
			
    return;
  case 0xb2:
  case 0xb3:
  case 0xb5:
    break;
  case 0xb6:
    ws_ioRam[0xb6] &= ~value;
    return;
  case 0xba: // internal EEPROM data
  case 0xbb: 
    break;
  case 0xbe: 
    if (value & 0x20) { // ack eeprom write
      w1 = (ws_ioRam[0xbc] << 1) & 0x7f;
      internalEeprom[w1    ] = ws_ioRam[0xba];
      internalEeprom[w1 + 1] = ws_ioRam[0xbb];
      ws_ioRam[0xbe] = 2;
    }
    else if (value & 0x10) { // ack eeprom read
      w1 = (ws_ioRam[0xbc] << 1) & 0x7f;
      ws_ioRam[0xba] = internalEeprom[w1    ];
      ws_ioRam[0xbb] = internalEeprom[w1 + 1];
      ws_ioRam[0xbe] = 1;
    }

    return;
  case 0xc0: // Bank selector
  case 0xc1:
  case 0xc2:
  case 0xc3:
//    log_write("%02X -> port %02X\n", value, port);
    break;
  case 0xc4:
  case 0xc5:
  case 0xc6:
  case 0xc7:
    break;
  case 0xc8:
    if (value & 0x40) // ack both
      ws_ioRam[0xc8] = 3;
    else if (value & 0x20) { // ack eeprom write
      //w1 = (((((WORD) ws_ioRam[0xc7]) << 8) | ((WORD) ws_ioRam[0xc6])) << 1) & externalEepromAddressMask;
     // externalEeprom[w1    ] = ws_ioRam[0xc4];
     // externalEeprom[w1 + 1] = ws_ioRam[0xc5];
      ws_ioRam[0xc8] = 2;
    }
    else if (value & 0x10) { // ack eeprom read
     // w1 = (((((WORD) ws_ioRam[0xc7]) << 8) | ((WORD) ws_ioRam[0xc6])) << 1) & externalEepromAddressMask;
      //ws_ioRam[0xc4] = externalEeprom[w1    ];
     // ws_ioRam[0xc5] = externalEeprom[w1 + 1];
      ws_ioRam[0xc8] = 1;
    }
    else
      ws_ioRam[0xc8] = 0;
    
    return;
  case 0xca:
    //if (value == 0x15)
      //rtcDataRegisterReadCount = 0; 

    break;
  default:
    unknown_io_port = 1;
  }

  ws_ioRam[port] = value;
  //ws_gpu_port_write(port, value);

//  if ((ws_gpu_unknownPort) && (unknown_io_port))
//    oswan_set_status_text(1, "unknown io port%2X=%2X", port, value);
//    log_write("io: writing 0x%.2x to unknown port 0x%.2x\n", value, port); 
}
}
