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

#define MAXVAL(a,b) (a>b?a:b)

extern signed char *m_voice_buff[SOUND_MAXVOICES_BUFFER_FX];
extern signed int *m_voice_buff_accumul_temp[SOUND_MAXVOICES_BUFFER_FX];
extern unsigned char *m_voice_buff_accumul_temp_cnt[SOUND_MAXVOICES_BUFFER_FX];
extern int m_voice_buff_adjustement;
extern int m_voice_fadeout_factor;

extern int64_t mdz_ratio_fp_cnt,mdz_ratio_fp_inc,mdz_ratio_fp_inv_inc;
extern double mdz_pbratio;

//timdity
extern unsigned char m_voice_channel_mapping[256];
extern unsigned char m_channel_voice_mapping[256];

extern int m_genNumVoicesChannels,m_genNumMidiVoicesChannels;

extern unsigned int vgm_last_vol[SOUND_MAXVOICES_BUFFER_FX];
extern unsigned int vgm_last_note[SOUND_MAXVOICES_BUFFER_FX];
extern unsigned char vgm_last_instr[SOUND_MAXVOICES_BUFFER_FX];
extern unsigned int vgm_last_sample_address[SOUND_MAXVOICES_BUFFER_FX];
extern unsigned int vgm_last_sample_address_inst[256];
extern unsigned char vgm_last_sample_address_lastupdate[SOUND_MAXVOICES_BUFFER_FX];

extern int64_t m_voice_current_ptr[SOUND_MAXVOICES_BUFFER_FX];
extern int64_t m_voice_prev_current_ptr[SOUND_MAXVOICES_BUFFER_FX];
extern int m_voice_ChipID[SOUND_MAXVOICES_BUFFER_FX];
extern int m_voice_systemColor[SOUND_VOICES_MAX_ACTIVE_CHIPS];
extern int m_voice_voiceColor[SOUND_MAXVOICES_BUFFER_FX];
extern char vgmVRC7,vgm2610b;
extern int HC_voicesMuteMask1,HC_voicesMuteMask2;
extern int64_t generic_mute_mask;

extern signed char m_voice_current_system,m_voice_current_systemSub;
extern int m_voice_current_samplerate;
extern double m_voice_current_rateratio;
extern char m_voice_current_systemPairedOfs;
extern char m_voice_current_total;
extern char m_voicesStatus[SOUND_MAXMOD_CHANNELS];

extern int m_voicesForceOfs;

extern int64_t mdz_ratio_fp_cnt,mdz_ratio_fp_inc,mdz_ratio_fp_inv_inc;

#define LIMIT8(a) (a>127?127:(a<-128?-128:a))

#endif /* ModizerVoicesData_h */
