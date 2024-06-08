/////////////////////////////////////////////////////////////////////////////////////////////////
// Audio
/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sound information thanks to toshi (wscamp wonderswan emulator)
// Note that sound is far from perfect for now.
//
// fixes by zalas 2002-08-21
//
/////////////////////////////////////////////////////////////////////////////////////////////////
//#include <windows.h>
#include "wsr_types.h" //YOYOFR
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

//TODO:  MODIZER changes start / YOYOFR
#include "../../../../src/ModizerVoicesData.h"
//TODO:  MODIZER changes end / YOYOFR


namespace oswan {
//#include "resource.h"
//#include "oswan.h"
//#include "log.h"
#include "ws.h"
#include "./nec/nec.h"
#include "io.h"
#include "audio.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
// defines
/////////////////////////////////////////////////////////////////////////////////////////////////
#define BIT(n) (1 << n)

/////////////////////////////////////////////////////////////////////////////////////////////////
// static variables
/////////////////////////////////////////////////////////////////////////////////////////////////
static const int BUFSIZEP =    32;
static const int BUFSIZEN = 65536;
static const int BUFCNT   =     4;
static const int DECODEBUFSIZE = 3072*2;	//WSR_PLAYER 本当はbpsによって変えたほうがよい


static int Verbose = 0;

static int is_playing   = 0;
static int is_recording = 0;

static int channel_enabled[] = {1, 1, 1, 1, 1, 1, 1, 1};
static int WsDirectSound     =     0;
static int bps               = 48000;
static int buffer_size       =  1920;
static int byte_buffer_size  =  3840;
static int MainVol           =     3;
static int WsWaveVol         =    30;

static unsigned char PDataN[8][BUFSIZEN];
static int           RandData[BUFSIZEN];
static unsigned char PData[4][BUFSIZEP];

static int channel_used[]       = {0, 0, 0, 0, 0, 0};
static int channel_is_playing[] = {0, 0, 0, 0, 0, 0};
static int ChCurLVol[]          = {0, 0, 0, 0, 0, 0};
static int ChCurRVol[]          = {0, 0, 0, 0, 0, 0};
static int ChCurPer[]           = {0, 0, 0, 0, 0, 0};
static int SwpOn                = 0;
static int WaveMap              = 0;
static int SwpTime              = 0;
static int SwpStep              = 0;
static int SwpCurPeriod         = 0;
static int CntSwp               = 0;
static int NoisePattern         = 0;

//static LPDIRECTSOUND       lpDS  = NULL;
//static LPDIRECTSOUNDBUFFER lpPSB = NULL;
//static LPDIRECTSOUNDBUFFER lpSB  = NULL;

//static HWAVEOUT hWaveOut = NULL;
//static WAVEHDR  whdr[BUFCNT];

//static FILE *wav = NULL;

static short waveData[DECODEBUFSIZE];


static int rBuf = 0;
static int wBuf = 0;
static int wPos = 0;


//WSR_PLAYER
typedef struct
{
	UINT32 offset;
	UINT32 delta;
	UINT32 pos;
} WS_AUDIO;

static WS_AUDIO ws_audio[6];
/////////////////////////////////////////////////////////////////////////////////////////////////
// static functions
/////////////////////////////////////////////////////////////////////////////////////////////////
static void         SoundErr(LPCSTR Msg);
static unsigned int ws_audio_mrand(unsigned int Degree);
//void         ws_audio_device_init(void);
static void         ws_audio_device_done(void);
int          WsDirectSoundCreate(void);
void         WsDirectSoundRelease(void);
int          WsWaveCreate(void);
void         WsWaveRelease(void);

static int	 ws_audio_play_channel(int channel);
static int  ws_audio_stop_channel(int channel);
static void ws_audio_set_channel_frequency(int channel,int period);
static void ws_audio_set_channel_pan(int channel,int left,int right);
static void ws_audio_set_channels_pbuf(int address, int data);

static void ws_audio_process_noise(int count);
static void ws_audio_process_sweep(int count);
static BYTE ws_audio_process_voice(int count);
static void ws_audio_process_hyper_voice(int count, BYTE* hvoice_l, BYTE* hvoice_r);
static void WsWaveSet(BYTE voice, BYTE hvoice_l, BYTE hvoice_r);
//void WsDirectSoundOutProc(void);
//void WsWaveOutProc(void);


/////////////////////////////////////////////////////////////////////////////////////////////////
// is_playing property getter (read only property)
/////////////////////////////////////////////////////////////////////////////////////////////////
int ws_audio_is_playing(void)
{
  return is_playing;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// is_recording property getter (read only property)
/////////////////////////////////////////////////////////////////////////////////////////////////
int ws_audio_is_recording(void)
{
  return is_recording;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//  channel_enabled property setter
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_set_channel_enabled(int channel, int value)
{
  if (channel_enabled[channel] == value)
    return;

  channel_enabled[channel] = (value ? 1 : 0);

  if (value)
    ws_audio_play_channel(channel);
  else
    ws_audio_stop_channel(channel);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// channel_enabled property getter
/////////////////////////////////////////////////////////////////////////////////////////////////
int ws_audio_get_channel_enabled(int channel)
{
  return channel_enabled[channel];
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// device property setter (1 = DirectSound 0 = WaveOut)
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
void ws_audio_set_device(int value)
{
  if (WsDirectSound == value)
    return;

  if (lpDS || hWaveOut) {
    ws_audio_device_done();
    WsDirectSound = value;
    ws_audio_device_init();
  }
  else {
    WsDirectSound = value;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// device property getter (1 = DirectSound 0 = WaveOut)
/////////////////////////////////////////////////////////////////////////////////////////////////
int ws_audio_get_device(void)
{
  return WsDirectSound;
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////
// bps property setter
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_set_bps(int value)
{
  value = (value / 12000) * 12000;

  if (value != 12000 && value != 24000 && value != 48000 && value != 96000)
	  value = 48000;

  if (bps == value)
    return;

  int old_buffer_size = ws_audio_get_buffer_size();

 // if (lpDS || hWaveOut) {
    ws_audio_device_done();
   bps = value;

   // ws_audio_device_init();
  //}
 // else {
    //bps = value;
 // }

  //ws_audio_set_buffer_size(old_buffer_size);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// bps property getter
/////////////////////////////////////////////////////////////////////////////////////////////////
int ws_audio_get_bps(void)
{
  return bps;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// buffer_size property setter (unit is msec)
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_set_buffer_size(int value)
{
  value = value * bps * 2 / 1000;

  if (buffer_size == value)
    return;

 // if (lpDS || hWaveOut) {
    ws_audio_device_done();
    buffer_size      = value;
    byte_buffer_size = sizeof(short) * value;
   // ws_audio_device_init();
 // }
 // else {
    buffer_size      = value;
   // byte_buffer_size = sizeof(short) * value;
  //}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// buffer_size property getter (unit is msec)
/////////////////////////////////////////////////////////////////////////////////////////////////
int ws_audio_get_buffer_size(void)
{
  return buffer_size * 1000 / bps / 2;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// main_volume property setter
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_set_main_volume(int value)
{
  if (MainVol == value)
    return;

  if (value < 0)
    MainVol = 0;
  else if (value > 15)
    MainVol = 15;
  else
    MainVol = value;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// main_volume property getter
/////////////////////////////////////////////////////////////////////////////////////////////////
int ws_audio_get_main_volume()
{
  return MainVol;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// wave_volume property setter
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_set_wave_volume(int value)
{
  WsWaveVol = value;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// wave_volume property getter
/////////////////////////////////////////////////////////////////////////////////////////////////
int ws_audio_get_wave_volume()
{
  return WsWaveVol;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// initialize audio device
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_init(void)
{
  int i, j;

  for (i = 0; i < 8; i++)
    for (j = 0; j < BUFSIZEN; j++)
      PDataN[i][j] = ((ws_audio_mrand(15 - i) & 1) ? 15 : 0);

  for (i = 0; i < BUFSIZEN; i++)
    RandData[i] = ws_audio_mrand(15);

  //ws_audio_device_init();
  ws_audio_reset();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// play sound
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_play(void)
{
  if (is_playing)
    return;

  is_playing = 1;
  rBuf       = 0;
  wBuf       = 2;
  wPos       = 0;

  int i;

  //for (i = 0; i < BUFCNT; i++)
    // (waveData[i])
     // ZeroMemory(waveData[i], byte_buffer_size);

  for (i = 0; i < 7; i++)
    ws_audio_play_channel(i);
#if 0
  if (lpSB) {
    LPVOID ptr1;
    DWORD  len1;

    if (lpSB->Lock(0, byte_buffer_size * BUFCNT, &ptr1, &len1, NULL, NULL, 0) == DS_OK) {
      ZeroMemory(ptr1, len1);
      lpSB->Unlock(ptr1, len1, NULL, 0);
    }

    if (lpSB->SetCurrentPosition(0) != DS_OK)
      SoundErr("Sound play position set error");

    if (lpSB->Play(0, 0, DSBPLAY_LOOPING) != DS_OK)
      SoundErr("Sound play error");
  }

  if (hWaveOut) {
    WsWaveOutProc();
    WsWaveOutProc();
  }
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// reset sound
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_reset(void)
{
  int i;

  ws_audio_stop();
  //WsWaveClose();

  memset(&ws_audio, 0, sizeof(ws_audio));

  for (i = 0; i < 4; i++)
    memset(PData[i],0, BUFSIZEP);

  for (i = 0; i < 6; i++) {
    channel_used[i]       = 0;
    channel_is_playing[i] = 0;
    ChCurLVol[i]          = 0;
    ChCurRVol[i]          = 0;
    ChCurPer[i]           = 0;
  }

  SwpOn        = 0;
  WaveMap      = 0;
  SwpTime      = 0;
  SwpStep      = 0;
  SwpCurPeriod = 0;
  CntSwp       = 0;
  NoisePattern = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// stop sound
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_stop(void)
{
  if (!is_playing)
    return;

  is_playing = 0;

  for (int i = 0; i < 7; i++)
    ws_audio_stop_channel(i);

  //if (lpSB)
   // if (lpSB->Stop() != DS_OK)
      //SoundErr("Sound stop error");

  //if (hWaveOut)
   // waveOutReset(hWaveOut);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// release audio device
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_done(void)
{
  ws_audio_device_done();
  //WsWaveClose();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_port_write(DWORD port,BYTE value)
{
  int i;

  ws_ioRam[port] = value;

  switch (port) {
  case 0x80:
  case 0x81:
    i = ((ws_ioRam[0x81] & 0x07) << 8) | ws_ioRam[0x80];
    ws_audio_set_channel_frequency(0, i);
    break;
  case 0x82:
  case 0x83:
    i = ((ws_ioRam[0x83] & 0x07) << 8) | ws_ioRam[0x82];
    ws_audio_set_channel_frequency(1, i);
    break;
  case 0x84:
  case 0x85:
    i = ((ws_ioRam[0x85] & 0x07) << 8) | ws_ioRam[0x84];
    ws_audio_set_channel_frequency(2, i);
    break;
  case 0x86:
  case 0x87:
    i = ((ws_ioRam[0x87] & 0x07) << 8) | ws_ioRam[0x86];
    ws_audio_set_channel_frequency(3, i);
    ws_audio_set_channel_frequency(5, i); // noise
    break;
  case 0x88:
    ws_audio_set_channel_pan(0, (value >> 4) & 0x0f, value & 0x0f);
    break;
  case 0x89:
    if (channel_used[1])
      ws_audio_set_channel_pan(1, (value >> 4) & 0x0f, value & 0x0f);

    break;
  case 0x8A:
    ws_audio_set_channel_pan(2, (value >> 4) & 0x0f, value & 0x0f);
    break;
  case 0x8B:
    if (channel_used[5])
      ws_audio_set_channel_pan(5, (value >> 4) & 0x0f, value & 0x0f);
    else
      ws_audio_set_channel_pan(3, (value >> 4) & 0x0f, value & 0x0f);

    break;
  case 0x8C:
    SwpStep = (signed char) value;
    break;
  case 0x8D:
    SwpTime = (((int) value) + 1) << 5;
    break;
  case 0x8E:
    NoisePattern = value & 0x07;
    break;
  case 0x8F:
    WaveMap = ((int) value) << 6;
    for (i = 0; i < 64; i++)
      ws_audio_set_channels_pbuf(i, internalRam[WaveMap + i]);

    break;
  case 0x90:
    if (value & 0x01) {
      channel_used[0] = 1;
      ws_audio_play_channel(0);
    }
    else {
      channel_used[0] = 0;
      ws_audio_stop_channel(0);
    }

    if ((value & 0x22) == 0x02) {
      channel_used[1] = 1;
      ws_audio_play_channel(1);
    }
    else {
      channel_used[1] = 0;
      ws_audio_stop_channel(1);
    }

    if (value & 0x20) {
      if (!channel_used[4]) {
        ws_ioRam[0x89] = 0x80;
        channel_used[4] = 1;
      }

      ws_audio_play_channel(4);
    }
    else {
      channel_used[4] = 0;
      ws_audio_stop_channel(4);
    }

    if ((value & 0x44) == 0x04) {
      channel_used[2] = 1;
      SwpOn           = 0;
      ws_audio_play_channel(2);
    }
    else if ((value & 0x44) == 0x44) {
      channel_used[2] = 1;
      SwpOn           = 1;
      ws_audio_play_channel(6);
    }
    else {
      channel_used[2] = 0;
      SwpOn           = 0;
      ws_audio_stop_channel(2);
    }

    if ((value & 0x88) == 0x08) {
      channel_used[3] = 1;
      ws_audio_play_channel(3);
    }
    else {
      channel_used[3] = 0;
      ws_audio_stop_channel(3);
    }

    if ((value & 0x88) == 0x88) {
      channel_used[5] = 1;
      ws_audio_play_channel(5);
    }
    else {
      channel_used[5] = 0;
      ws_audio_stop_channel(5);
    }

    break;
  case 0x91:
    // bits 1-2 volume of internal speaker
    value |= 0x80;
    ws_ioRam[port] = value;	// Always have external speaker
    break;
  case 0x92:	// Noise Counter Shift Register (15 bits)
  case 0x93:
    break;
  case 0x94:	// Volume (4 bit)?
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
BYTE ws_audio_port_read(BYTE port)
{
  return ws_ioRam[port];
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_write_byte(DWORD offset, BYTE value)
{
  if (!((offset - WaveMap) & 0xffc0)) {
    ws_audio_set_channels_pbuf(offset & 0x003f, value);
    internalRam[offset] = value;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_process(int count)
{
  ws_audio_process_noise(count);
  ws_audio_process_sweep(count);

  BYTE voice = ws_audio_process_voice(count);

  BYTE hvoice_l;
  BYTE hvoice_r;
  ws_audio_process_hyper_voice(count, &hvoice_l, &hvoice_r);

  WsWaveSet(voice, hvoice_l, hvoice_r);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
void ws_audio_readState(int fp)
{
  read(fp, &SwpOn       , sizeof(int));
  read(fp, &WaveMap     , sizeof(int));
  read(fp, &SwpTime     , sizeof(int));
  read(fp, &SwpStep     , sizeof(int));
  read(fp, &SwpCurPeriod, sizeof(int));
  read(fp, &CntSwp      , sizeof(int));
  read(fp, &NoisePattern, sizeof(int));

  read(fp, channel_used      , sizeof(int) * 6);
  read(fp, channel_is_playing, sizeof(int) * 6);
  read(fp, ChCurLVol         , sizeof(int) * 6);
  read(fp, ChCurRVol         , sizeof(int) * 6);
  read(fp, ChCurPer          , sizeof(int) * 6);

  read(fp, PData, sizeof(unsigned char) * 4 * BUFSIZEP);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
void ws_audio_writeState(int fp)
{
  write(fp, &SwpOn       , sizeof(int));
  write(fp, &WaveMap     , sizeof(int));
  write(fp, &SwpTime     , sizeof(int));
  write(fp, &SwpStep     , sizeof(int));
  write(fp, &SwpCurPeriod, sizeof(int));
  write(fp, &CntSwp      , sizeof(int));
  write(fp, &NoisePattern, sizeof(int));

  write(fp, channel_used      , sizeof(int) * 6);
  write(fp, channel_is_playing, sizeof(int) * 6);
  write(fp, ChCurLVol         , sizeof(int) * 6);
  write(fp, ChCurRVol         , sizeof(int) * 6);
  write(fp, ChCurPer          , sizeof(int) * 6);

  write(fp, PData, sizeof(unsigned char) * 4 * BUFSIZEP);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////

void WsWaveOpen(void)
{
  if (is_recording)
    return;

  BYTE WavHeader[] = {
    0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00,
    0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
    0x10, 0x00, 0x00, 0x00, 0x01, 0x00
  };
  WORD	WavCh           = 2; // mono = 1: stereo = 2
  DWORD	WavSamplingRate = bps;
  WORD	WavBit          = 16; // Wav bits (8 or 16)
  BYTE	WavHeader2[] = {
    0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00
  };
  WORD	WavBlock = (WavBit / 8) * WavCh;
  DWORD	WavBytes = WavSamplingRate * WavBlock;

  static int filecount = 0;
  char fname[1024];

  while (1) {
    sprintf(fname, "%s%s[%.3i].wav", WsWavDir, BaseName, filecount++);
    wav = fopen(fname,"r");
    if (wav)
      fclose(wav);
    else if (filecount > 100)
      return;
    else
      break;
  }

  if (wav = fopen(fname, "wb")) {
    fwrite(WavHeader       , sizeof(BYTE) , 22, wav);
    fwrite(&WavCh          , sizeof(WORD) ,  1, wav);
    fwrite(&WavSamplingRate, sizeof(DWORD),  1, wav);
    fwrite(&WavBytes       , sizeof(DWORD),  1, wav);
    fwrite(&WavBlock       , sizeof(WORD) ,  1, wav);
    fwrite(&WavBit         , sizeof(WORD) ,  1, wav);
    fwrite(WavHeader2      , sizeof(BYTE) ,  8, wav);
  }

  is_recording = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////

void WsWaveClose(void)
{
  if (!is_recording)
    return;

  if (wav) {
    long fsize = ftell(wav);
    if (fsize >= 44) {
      fsize -= 8;
      fseek(wav, 4, SEEK_SET);
      fwrite(&fsize, sizeof(long), 1, wav);

      fsize -= 36;
      fseek(wav, 40, SEEK_SET);
      fwrite(&fsize, sizeof(long), 1, wav);
	}

    fclose(wav);
    wav = NULL;

    HMENU menu = GetMenu(hWnd);
    EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_START, MF_ENABLED);
    EnableMenuItem(menu, ID_OPTIONS_SOUND_REC_STOP , MF_GRAYED );
  }

  is_recording = 0;
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void SoundErr(LPCSTR Msg)
{
  //MessageBox(NULL, Msg, "WsSound", MB_ICONEXCLAMATION | MB_OK);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static unsigned int ws_audio_mrand(unsigned int Degree)
{
  typedef struct {
    unsigned int N;
    int          InputBit;
    int          Mask;
  } POLYNOMIAL;

  static POLYNOMIAL TblMask[] = {
    { 2, BIT( 2), BIT(0) | BIT(1)},
    { 3, BIT( 3), BIT(0) | BIT(1)},
    { 4, BIT( 4), BIT(0) | BIT(1)},
    { 5, BIT( 5), BIT(0) | BIT(2)},
    { 6, BIT( 6), BIT(0) | BIT(1)},
    { 7, BIT( 7), BIT(0) | BIT(1)},
    { 8, BIT( 8), BIT(0) | BIT(2) | BIT(3) | BIT(4)},
    { 9, BIT( 9), BIT(0) | BIT(4)},
    {10, BIT(10), BIT(0) | BIT(3)},
    {11, BIT(11), BIT(0) | BIT(2)},
    {12, BIT(12), BIT(0) | BIT(1) | BIT(4) | BIT(6)},
    {13, BIT(13), BIT(0) | BIT(1) | BIT(3) | BIT(4)},
    {14, BIT(14), BIT(0) | BIT(1) | BIT(4) | BIT(5)},
    {15, BIT(15), BIT(0) | BIT(1)},
    { 0, 0      , 0}
  };

  static POLYNOMIAL* pTbl = TblMask;
  static int ShiftReg=pTbl->InputBit-1;
  int XorReg=0;
  int Masked;

  if (Degree >= 16) {
    pTbl     = TblMask;
    ShiftReg = pTbl->InputBit - 1;
    return 0;
  }

	if(pTbl->N!=Degree)
	{
		pTbl=TblMask;
		while(pTbl->N)
		{
			if(pTbl->N==Degree)
			{
				break;
			}
			pTbl++;
		}
		if(!pTbl->N)
		{
			pTbl--;
		}

		ShiftReg&=pTbl->InputBit-1;
		if(!ShiftReg)
		{
			ShiftReg=pTbl->InputBit-1;
		}
	}

	Masked=ShiftReg&pTbl->Mask;
	while(Masked)
	{
		XorReg^=Masked&0x01;
		Masked>>=1;
	}

	if(XorReg)
	{
		ShiftReg|=pTbl->InputBit;
	}
	else
	{
		ShiftReg&=~pTbl->InputBit;
	}
	ShiftReg>>=1;

	return ShiftReg;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
static void ws_audio_device_init()
{
  for (int i = 0; i < BUFCNT; i++)
    waveData[i] = (short*) malloc(byte_buffer_size);

  if (WsDirectSound)
    WsDirectSoundCreate();
  else
    WsWaveCreate();
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void ws_audio_device_done()
{
  ws_audio_stop();
#if 0
  if (WsDirectSound)
    WsDirectSoundRelease();
  else
    WsWaveRelease();

  for (int i = 0; i < BUFCNT; i++)
    if (waveData[i]) {
      free(waveData[i]);
      waveData[i] = NULL;
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
static int WsDirectSoundCreate(void)
{
  HRESULT result;

  result = DirectSoundCreate(NULL, &lpDS, NULL);
  if (result != DS_OK) {
    SoundErr("Could not create direct sounnd object");
    WsDirectSoundRelease();
	return -1;
  }

  result = lpDS->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
  if (result != DS_OK) {
    SoundErr("Could not set cooperative level");
    WsDirectSoundRelease();
    return -1;
  }

  DSBUFFERDESC dsbd;
  ZeroMemory(&dsbd, sizeof(dsbd));
  dsbd.dwSize  = sizeof(dsbd);
  dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
  if (FAILED(lpDS->CreateSoundBuffer(&dsbd, &lpPSB, NULL))) {
    SoundErr("Could not create primary sound buffer");
    WsDirectSoundRelease();
    return -1;
  }

  WAVEFORMATEX wfx;
  ZeroMemory(&wfx, sizeof(WAVEFORMATEX));
  wfx.wFormatTag      = WAVE_FORMAT_PCM;
  wfx.nSamplesPerSec  = bps;
  wfx.nChannels       = 2;
  wfx.wBitsPerSample  = 16;
  wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
  if (FAILED(lpPSB->SetFormat(&wfx))) {
    SoundErr("Cound not change format of primary sound buffer");
    WsDirectSoundRelease();
    return -1;
  }

  PCMWAVEFORMAT	pcmwf;
  ZeroMemory(&pcmwf, sizeof(PCMWAVEFORMAT));
  pcmwf.wf.wFormatTag      = WAVE_FORMAT_PCM;
  pcmwf.wf.nSamplesPerSec  = bps;
  pcmwf.wf.nChannels       = 2;
  pcmwf.wBitsPerSample     = 16;
  pcmwf.wf.nBlockAlign     = pcmwf.wf.nChannels * pcmwf.wBitsPerSample / 8;
  pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
  dsbd.dwFlags       = DSBCAPS_LOCSOFTWARE | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
  dsbd.dwBufferBytes = byte_buffer_size * BUFCNT;
  dsbd.lpwfxFormat   = (LPWAVEFORMATEX) &pcmwf;
  result = lpDS->CreateSoundBuffer(&dsbd, &lpSB, NULL);
  if (result != DS_OK) {
    SoundErr("Could not create sound buffers");
    WsDirectSoundRelease();
	return -1;
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void WsDirectSoundRelease(void)
{
  if (lpSB) {
    lpSB->Release();
    lpSB = NULL;
  }

  if (lpPSB) {
    lpPSB->Release();
    lpPSB = NULL;
  }

  if (lpDS) {
    lpDS->Release();
    lpDS = NULL;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static int WsWaveCreate(void)
{
  MMRESULT     result;
  WAVEFORMATEX wfe;
  char         msg[256];

  ZeroMemory(&wfe, sizeof(wfe));
  wfe.wFormatTag      = WAVE_FORMAT_PCM;
  wfe.nSamplesPerSec  = bps;
  wfe.wBitsPerSample  = 16;
  wfe.nChannels       = 2;
  wfe.nBlockAlign     = wfe.nChannels * wfe.wBitsPerSample / 8;
  wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nBlockAlign;

  result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfe, (DWORD) hWnd, 0, CALLBACK_WINDOW);
  if (result != MMSYSERR_NOERROR) {
    waveOutGetErrorText(result, msg, sizeof(msg));
    SoundErr(msg);
    WsWaveRelease();
    return -1;
  }

  for (int i = 0; i < BUFCNT; i++) {
    whdr[i].lpData         = (char*) waveData[i];
    whdr[i].dwBufferLength = byte_buffer_size;
    whdr[i].dwFlags        = 0;
    whdr[i].dwLoops        = 1;
    result = waveOutPrepareHeader(hWaveOut, &whdr[i], sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
      waveOutGetErrorText(result, msg, sizeof(msg));
      SoundErr(msg);
      WsWaveRelease();
      return -1;
    }
  }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void WsWaveRelease(void)
{
  for(int i = 0; i < BUFCNT; i++)
    waveOutUnprepareHeader(hWaveOut, &whdr[i], sizeof(WAVEHDR));

  waveOutClose(hWaveOut);

  hWaveOut = NULL;
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////
// start playing a channel
/////////////////////////////////////////////////////////////////////////////////////////////////
static int ws_audio_play_channel(int channel)
{
  if (channel == 7)
    return 0;

  if (channel == 6) {
    if (!channel_enabled[6] || !channel_used[2] || !SwpOn)
      return 0;
    else
      channel = 2;
  }
  else if (!channel_enabled[channel] || !channel_used[channel])
    return 0;

  channel_is_playing[channel] = 1;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// stop playing a channel
/////////////////////////////////////////////////////////////////////////////////////////////////
static int ws_audio_stop_channel(int channel)
{
  if (channel == 7)
    return 0;

  if (channel == 6) {
    if (!SwpOn)
      return 0;
    else
      channel = 2;
  }

  channel_is_playing[channel] = 0;

  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void ws_audio_set_channel_frequency(int channel, int period)
{
  //if (ChCurPer[channel] == period)
   // return;

  ChCurPer[channel] = period;

  if (channel == 2)
    SwpCurPeriod = period;

  double freq = 3072000 / (2048 - period);
  ws_audio[channel].delta = (UINT32)(freq * 65536 / bps);

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void ws_audio_set_channel_pan(int channel, int left, int right)
{
  ChCurLVol[channel] = left;
  ChCurRVol[channel] = right;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void ws_audio_set_channels_pbuf(int address, int data)
{
  int i = (address & 0x30) >> 4;
  int j = (address & 0x0f) << 1;

  PData[i][j    ] = ( data       & 0x0f);
  PData[i][j + 1] = ((data >> 4) & 0x0f);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void ws_audio_process_noise(int count)
{
  static int n = 0;

  if (count != 0)
    return;

  n = (n + 1) % BUFSIZEN;

  int i = RandData[n];
  ws_ioRam[0x92] = (BYTE)  i; // noise counter shift register
  ws_ioRam[0x93] = (BYTE) (i >> 8);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void ws_audio_process_sweep(int count)
{
  // (8/3) * (ws_ioRam[0x8D] + 1)ms
  // (8/3) / (1000/12000) = 32
  // SwpTime = (ws_ioRam[0x8D] + 1) * 32

  if (count != 0)
    return;

  if ((SwpStep) && SwpOn) { // sweep on
    if (CntSwp < 0) {
      CntSwp        = SwpTime;
      SwpCurPeriod += SwpStep;
      SwpCurPeriod &= 0x7ff;

	  double freq = 3072000 / (2048 - SwpCurPeriod);
	  ws_audio[2].delta = (long)(freq * 65536 / bps);
    }

    CntSwp--;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static BYTE ws_audio_process_voice(int count)
{
  if ((ws_ioRam[0x52] & 0x88) == 0x80) { // DMA start
    int i =                          (ws_ioRam[0x4f] << 8) | ws_ioRam[0x4e]; // size
    int j = (ws_ioRam[0x4c] << 16) | (ws_ioRam[0x4b] << 8) | ws_ioRam[0x4a]; // start bank:address
    int k = ((ws_ioRam[0x52] & 0x03) == 0x03 ? 2 : 1);

    ws_ioRam[0x89] = cpu_readmem20(j);

    if (bps == 12000) {
      i -= k;
      j += k;
    }
    else if ((count % (bps / 12000 / k)) == 0) {
      i--;
      j++;
    }

    if (i <= 0) {
      i = 0;
      ws_ioRam[0x52] &= 0x7f; // DMA end
    }

    ws_ioRam[0x4a] = (BYTE)  j;
    ws_ioRam[0x4b] = (BYTE) (j >>  8);
    ws_ioRam[0x4c] = (BYTE) (j >> 16);
    ws_ioRam[0x4e] = (BYTE)  i;
    ws_ioRam[0x4f] = (BYTE) (i >>  8);
  }

  return (channel_is_playing[4] ? ws_ioRam[0x89] : 0x80);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
static void ws_audio_process_hyper_voice(int count, BYTE* hvoice_l, BYTE* hvoice_r)
{
  static int index = 0;

  if (channel_enabled[7] && (ws_ioRam[0x52] & 0x98) == 0x98) { // Hyper Voice On?
    int  address = (ws_ioRam[0x4c] << 16) | (ws_ioRam[0x4b] << 8) | ws_ioRam[0x4a];
    int  size    =                          (ws_ioRam[0x4f] << 8) | ws_ioRam[0x4e];

    int value1 = cpu_readmem20(address + index);

    if (value1 < 0x80)
      *hvoice_l = (BYTE) (value1 + 0x80);
    else
      *hvoice_l = (BYTE) (value1 - 0x80);

    *hvoice_r = *hvoice_l;

    if (count == 0)
      if (size <= (++index))
       index = 0;
  }
  else {
    *hvoice_l = 0x80;
    *hvoice_r = 0x80;
    index     = 0;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
extern void FlushBufferWSR(const short* finalWave, unsigned long length);

static void WsWaveSet(BYTE voice, BYTE hvoice_l, BYTE hvoice_r)
{
//WSR_PLAYER 音程がおかしくなるときがあったので、一部のコードを変更
  //static int point[]    = {0, 0, 0, 0, 0};
 // static int preindex[] = {0, 0, 0, 0, 0};
  //int    index;
  short  value;
  short  VV, HVL, HVR;
  long	 LL, RR;
  short lVol[5] = { 0, 0, 0, 0, 0, };
  short rVol[5] = { 0, 0, 0, 0, 0, };

  for (int channel = 0; channel < 4; channel++) {
    if (channel_is_playing[channel]) {
      //if (channel == 2 && SwpOn) {
         //sweep
        //index = (3072000 / bps) * point[channel] / (2048 - SwpCurPeriod);
     // }
	 // else

			{

				ws_audio[channel].offset += ws_audio[channel].delta;
				unsigned cnt = ws_audio[channel].offset >> 16;
				ws_audio[channel].offset &= (1<<16) - 1;
				ws_audio[channel].pos += cnt;
				ws_audio[channel].pos &= (BUFSIZEP - 1);
				//index = (3072000 / bps) * point[channel] / (2048 - ChCurPer[channel]);
			}

			//if ((index %= BUFSIZEP) == 0 && preindex[channel])
			// point[channel] = 0;

			// preindex[channel] = index;
			//point[channel]++;
			value = ((short)PData[channel][ws_audio[channel].pos]) - 8;
			lVol[channel] = value * ChCurLVol[channel];
			rVol[channel] = value * ChCurRVol[channel];
    }
    else {
      lVol[channel] = 0;
      rVol[channel] = 0;
    }
  }

  // noise
  if (channel_is_playing[5]) {
	  ws_audio[5].offset += ws_audio[5].delta;
	  int cnt = ws_audio[5].offset >> 16;
	  ws_audio[5].offset &= (1<<16) - 1;
	  ws_audio[5].pos += cnt;
	  ws_audio[5].pos &= (BUFSIZEN - 1);

    //index = (3072000 / bps) * point[4] / (2048 - ChCurPer[5]);
    //if ((index %= BUFSIZEN) == 0 && preindex[4])
     // point[4] = 0;

    //preindex[4] = index;
   // point[4]++;
    value = ((short) PDataN[NoisePattern][ws_audio[5].pos]) - 8;
    lVol[4] = value * ChCurLVol[5];
    rVol[4] = value * ChCurRVol[5];
  }
  else {
    lVol[4] = 0;
    rVol[4] = 0;
  }

  VV  = (((short) voice   ) - 0x80) * 2;
  HVL = (((short) hvoice_l) - 0x80) * 2;
  HVR = (((short) hvoice_r) - 0x80) * 2;

  // mix
  LL = (lVol[0] + lVol[1] + lVol[2] + lVol[3] + lVol[4] + VV + HVL) * WsWaveVol;
  RR = (rVol[0] + rVol[1] + rVol[2] + rVol[3] + rVol[4] + VV + HVR) * WsWaveVol;
#if 0
  if (wav) {
    fwrite(&LL, sizeof(short), 1, wav);
    fwrite(&RR, sizeof(short), 1, wav);
  }
#endif
  //サチレーションチェック
  if (LL > 32767)
	  LL = 32767;
  if (LL < -32768)
	  LL = -32768;
  if (RR > 32767)
	  RR = 32767;
  if (RR < -32768)
	  RR = -32768;

  waveData[wPos++] = (short)LL;
  waveData[wPos++] = (short)RR;
    
    //YOYOFR
    int64_t smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/48000;
    for (int jj=0;jj<6;jj++) {
        int64_t ofs_start=m_voice_current_ptr[jj];
        int64_t ofs_end=(m_voice_current_ptr[jj]+smplIncr);
        
        int val;
        if (jj<4) val=(MAXVAL(lVol[jj],rVol[jj])*WsWaveVol)>>5;
        else if (jj==4) val=((VV+MAXVAL(HVL,HVR))*WsWaveVol)>>5;
        else if (jj==5) val=(MAXVAL(lVol[4],rVol[4])*WsWaveVol)>>5;
        if (ofs_end>ofs_start)
            for (;;) {
                m_voice_buff[jj][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)]=LIMIT8( val );
                ofs_start+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                if (ofs_start>=ofs_end) break;
            }
        while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4*2) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*4*2<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
        m_voice_current_ptr[jj]=ofs_end;
    }
    //YOYOFR
    
    //YOYOFR
    static int last_chanout[6];
    for (int ii=0;ii<6;ii++) {
        int val;
        if (ii<4) val=MAXVAL(lVol[ii],rVol[ii])*WsWaveVol;
        else if( ii==4) val=MAXVAL(MAXVAL(VV,HVL),HVR)*WsWaveVol;
        else val=MAXVAL(lVol[4],rVol[4])*WsWaveVol;
        if ( channel_is_playing[ii] && (val!=last_chanout[ii])) {
            int vol=0;
            vol=MAXVAL(ChCurLVol[ii],ChCurRVol[ii]);
            
            if (vol) {
                if ( (ii==0)||(ii==1)||(ii==2)||(ii==3)||(ii==5)) vgm_last_note[ii]=3*440.0*(double)ws_audio[ii].delta/65536.0;
                else vgm_last_note[ii]=220;
                vgm_last_instr[ii]=ii;
                vgm_last_vol[ii]=1;
            }
        }
        last_chanout[ii]=val;
    }
    //YOYOFR

  if (wPos >= DECODEBUFSIZE) {
#if 0
    if (lpSB)
      WsDirectSoundOutProc();

    if (hWaveOut)
      WsWaveOutProc();
#endif
    wPos = 0;
    //wBuf = (wBuf + 1) % BUFCNT;
    FlushBufferWSR(waveData, (DECODEBUFSIZE << 1));
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
static void WsDirectSoundOutProc(void)
{
  DWORD   dwPlayPos;
  DWORD   dwWritePos;
  LPVOID  ptr1;
  DWORD   len1;

  if (lpSB->GetCurrentPosition(&dwPlayPos, &dwWritePos) == DS_OK) {
    dwWritePos = (((dwPlayPos / byte_buffer_size) + 2) % BUFCNT);

    if (dwWritePos == (DWORD) ((wBuf + BUFCNT - 1) % BUFCNT))
      dwWritePos = wBuf;

    if (lpSB->Lock(dwWritePos * byte_buffer_size,
                   byte_buffer_size, &ptr1, &len1, NULL, NULL, 0) == DS_OK) {
      memcpy(ptr1, waveData[wBuf], len1);
      lpSB->Unlock(ptr1, len1, NULL, 0);
    }

    wBuf = dwWritePos;
  }
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#if 0
static void WsWaveOutProc(void)
{
  MMRESULT result;

  result = waveOutWrite(hWaveOut, &whdr[rBuf], sizeof(WAVEHDR));
  if (result != MMSYSERR_NOERROR) {
    char msg[255];
    waveOutGetErrorText(result, msg, sizeof(msg));
  }

  rBuf = (rBuf + 1) % BUFCNT;
}
#endif
}
