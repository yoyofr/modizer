/////////////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __AUDIO_H__
#define __AUDIO_H__

//#include <dsound.h>


/////////////////////////////////////////////////////////////////////////////////////////////////
// functions
/////////////////////////////////////////////////////////////////////////////////////////////////
int ws_audio_is_playing(void);
int ws_audio_is_recording(void);

void ws_audio_set_device(int value);
int  ws_audio_get_device(void);
void ws_audio_set_bps(int value);
int  ws_audio_get_bps();
void ws_audio_set_buffer_size(int value);
int  ws_audio_get_buffer_size();
void ws_audio_set_channel_enabled(int channel, int value);
int  ws_audio_get_channel_enabled(int channel);
void ws_audio_set_main_volume(int value);
int  ws_audio_get_main_volume();
void ws_audio_set_wave_volumne(int value);
int  ws_audio_get_wave_volumne();

void ws_audio_init(void);
void ws_audio_play(void);
void ws_audio_reset(void);
void ws_audio_stop(void);
void ws_audio_done(void);

void ws_audio_port_write(DWORD port,BYTE value);
BYTE ws_audio_port_read(BYTE port);
void ws_audio_write_byte(DWORD offset, BYTE value);
void ws_audio_process(int count);

void ws_audio_readState(int fp);
void ws_audio_writeState(int fp);

void WsWaveOpen(void);
void WsWaveClose(void);

#endif