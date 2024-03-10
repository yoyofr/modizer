//
//  ModizerVoicesData.h
//  modizer
//
//  Created by Yohann Magnien on 05/04/2021.
//

#ifndef ModizerVoicesData_h
#define ModizerVoicesData_h

#include "ModizerConstants.h"
#include <sys/types.h>

extern signed char *m_voice_buff[SOUND_MAXVOICES_BUFFER_FX];
extern signed int *m_voice_buff_accumul_temp[SOUND_MAXVOICES_BUFFER_FX];
extern unsigned char *m_voice_buff_accumul_temp_cnt[SOUND_MAXVOICES_BUFFER_FX];

extern int m_genNumVoicesChannels;

extern int64_t m_voice_current_ptr[SOUND_MAXVOICES_BUFFER_FX];
extern int64_t m_voice_prev_current_ptr[SOUND_MAXVOICES_BUFFER_FX];
extern int m_voice_ChipID[SOUND_MAXVOICES_BUFFER_FX];
extern int m_voice_systemColor[SOUND_VOICES_MAX_ACTIVE_CHIPS];
extern int m_voice_voiceColor[SOUND_MAXVOICES_BUFFER_FX];
extern char vgmVRC7,vgm2610b;
extern int HC_voicesMuteMask1,HC_voicesMuteMask2;
extern int v2m_voices_mask;
extern signed char m_voice_current_system,m_voice_current_systemSub;
extern int m_voice_current_samplerate;
extern char m_voice_current_systemPairedOfs;
extern char m_voicesStatus[SOUND_MAXMOD_CHANNELS];
extern int m_voice_current_sample;

extern int m_voicesForceOfs;

#define LIMIT8(a) (a>127?127:(a<-128?-128:a))

#endif /* ModizerVoicesData_h */
