/***************************************************************************
 * Gens: PWM audio emulator.                                               *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville                       *
 * Copyright (c) 2003-2004 by Stéphane Akhoun                              *
 * Copyright (c) 2008-2009 by David Korth                                  *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "../../stdtype.h"
#include "../snddef.h"
#include "../EmuHelper.h"
#include "../EmuCores.h"
#include "pwm.h"

//TODO:  MODIZER changes start / YOYOFR
#include "../../../../../src/ModizerVoicesData.h"
//TODO:  MODIZER changes end / YOYOFR


typedef struct _pwm_chip pwm_chip;


static void PWM_Init(void* info);
static void PWM_Recalc_Scale(pwm_chip* chip);

static void PWM_Set_Cycle(pwm_chip* chip, UINT16 cycle);
static void PWM_Set_Int(pwm_chip* chip, UINT8 int_time);

static void PWM_Update(void* info, UINT32 length, DEV_SMPL **buf);

static UINT8 device_start_pwm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
static void device_stop_pwm(void* info);

static void pwm_chn_w(void* info, UINT8 Channel, UINT16 data);

static void pwm_set_mute_mask(void *info, UINT32 MuteMask);


static DEVDEF_RWFUNC devFunc[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D16, 0, pwm_chn_w},
	{RWF_CHN_MUTE | RWF_WRITE, DEVRW_ALL, 0, pwm_set_mute_mask},
	{0x00, 0x00, 0, NULL}
};
static DEV_DEF devDef =
{
	"PWM", "Gens/GS", FCC_GENS,
	
	device_start_pwm,
	device_stop_pwm,
	PWM_Init,
	PWM_Update,
	
	NULL,	// SetOptionBits
	pwm_set_mute_mask,
	NULL,	// SetPanning
	NULL,	// SetSampleRateChangeCallback
	NULL,	// SetLoggingCallback
	NULL,	// LinkDevice
	
	devFunc,	// rwFuncs
};

const DEV_DEF* devDefList_32X_PWM[] =
{
	&devDef,
	NULL
};


struct _pwm_chip
{
	DEV_DATA _devData;
	
	UINT16 PWM_Cycle;
	UINT8 PWM_Int;
	UINT8 PWM_Int_Cnt;
	UINT8 PWM_Mode;
	UINT8 PWM_Mute;
	UINT16 PWM_Out_R;
	UINT16 PWM_Out_L;
	
	// PWM scaling variables.
	INT32 PWM_Offset;
	INT32 PWM_Scale;
#define PWM_Loudness	0
	//UINT8 PWM_Loudness;
	
	UINT32 clock;
};


/**
 * PWM_Init(): Initialize the PWM audio emulator.
 */
static void PWM_Init(void* info)
{
	pwm_chip* chip = (pwm_chip*)info;
	
	chip->PWM_Mode = 0;
	chip->PWM_Out_R = 0;
	chip->PWM_Out_L = 0;
	
	//PWM_Loudness = 0;
	PWM_Set_Cycle(chip, 0);
	PWM_Set_Int(chip, 0);
}


static void PWM_Recalc_Scale(pwm_chip* chip)
{
	chip->PWM_Offset = (chip->PWM_Cycle / 2) + 1;
	chip->PWM_Scale = 0x7FFF00 / chip->PWM_Offset;
}


static void PWM_Set_Cycle(pwm_chip* chip, UINT16 cycle)
{
	cycle--;
	chip->PWM_Cycle = (cycle & 0xFFF);
	
	PWM_Recalc_Scale(chip);
}


static void PWM_Set_Int(pwm_chip* chip, UINT8 int_time)
{
	int_time &= 0x0F;
	if (int_time)
		chip->PWM_Int = chip->PWM_Int_Cnt = int_time;
	else
		chip->PWM_Int = chip->PWM_Int_Cnt = 16;
}


INLINE DEV_SMPL PWM_Update_Scale(pwm_chip* chip, INT16 PWM_In)
{
	if (PWM_In == 0)
		return 0;
	
	// TODO: Chilly Willy's new scaling algorithm breaks drx's Sonic 1 32X (with PWM drums).
	//return (((PWM_In & 0xFFF) - chip->PWM_Offset) * chip->PWM_Scale) >> (8 - PWM_Loudness);
	
	// Knuckles' Chaotix: Tachy Touch uses the values 0xF?? for negative values
	// This small modification fixes the terrible pops.
	PWM_In &= 0xFFF;
	if (PWM_In & 0x800)
		PWM_In |= ~0xFFF;
	return ((PWM_In - chip->PWM_Offset) * chip->PWM_Scale) >> (8 - PWM_Loudness);
}


static void PWM_Update(void* info, UINT32 length, DEV_SMPL **buf)
{
	pwm_chip* chip = (pwm_chip*)info;
	DEV_SMPL tmpOutL;
	DEV_SMPL tmpOutR;
	UINT32 i;
	
    //YOYOFR
	if (chip->PWM_Mute==3)
	{
		memset(buf[0], 0, length * sizeof(DEV_SMPL));
		memset(buf[1], 0, length * sizeof(DEV_SMPL));
		return;
	}
    
    //TODO:  MODIZER changes start / YOYOFR
    //search first voice linked to current chip
    static int last_out[2];
    static int last_out_same_cnt[2]={0,0};
    int m_voice_ofs=-1;
    int m_total_channels=2;
    for (int ii=0;ii<=SOUND_MAXVOICES_BUFFER_FX-m_total_channels;ii++) {
        if (m_voice_ChipID[ii]==m_voice_current_system) {
            m_voice_ofs=ii+(m_voice_current_systemSub?m_voice_current_systemPairedOfs:0);
            m_voice_current_total=m_total_channels;
            break;
        }
    }
    if (m_voice_ofs>=0) m_voicesForceOfs=m_voice_ofs;
    //printf("opn:%d / %lf delta:%lf\n",OPN->ST.rate,OPN->ST.freqbase,DELTAT->freqbase);
    if (!m_voice_current_samplerate) {
        m_voice_current_samplerate=44100;
        //printf("voice sample rate null\n");
    }
    int64_t smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/m_voice_current_samplerate;
    //TODO:  MODIZER changes end / YOYOFR
	
	// New PWM scaling algorithm provided by Chilly Willy on the Sonic Retro forums.
	tmpOutL = PWM_Update_Scale(chip, (INT16)chip->PWM_Out_L);
	tmpOutR = PWM_Update_Scale(chip, (INT16)chip->PWM_Out_R);
    
    //YOYOFR
    if (m_voice_ofs>=0) {
        int limit_silence=(int64_t)m_voice_current_samplerate*50/1000;  //50ms
        if (last_out[0]==tmpOutL) last_out_same_cnt[0]+=length;
        else last_out_same_cnt[0]=0;
        if (last_out[1]==tmpOutR) last_out_same_cnt[1]+=length;
        else last_out_same_cnt[1]=0;
        
        if (chip->PWM_Cycle)
            if (!(chip->PWM_Mute&1) && (last_out_same_cnt[0]<limit_silence) ) {
            vgm_last_note[0+m_voice_ofs]=440.0f*((double)chip->clock/chip->PWM_Cycle)/22050.0f;
            vgm_last_sample_addr[0+m_voice_ofs]=m_voice_ofs+0;
            int newvol=1;
            vgm_last_vol[0+m_voice_ofs]=newvol;
        }
        if (!(chip->PWM_Mute&2) && (last_out_same_cnt[1]<limit_silence) ) {
            vgm_last_note[1+m_voice_ofs]=440.0f*((double)chip->clock/chip->PWM_Cycle)/22050.0f;
            vgm_last_sample_addr[1+m_voice_ofs]=m_voice_ofs+0;
            int newvol=1;
            vgm_last_vol[1+m_voice_ofs]=newvol;
        }
        last_out[0]=tmpOutL;
        last_out[1]=tmpOutR;
    }
    //YOYOFR
    
	for (i = 0; i < length; i ++)
	{
        if (chip->PWM_Mute&1) buf[0][i]=0;
        else buf[0][i] = tmpOutL;
        if (chip->PWM_Mute&2) buf[1][i] = 0;
        else buf[1][i] = tmpOutR;
        
        //TODO:  MODIZER changes start / YOYOFR
        if (m_voice_ofs>=0) {
            int64_t ofs_start=m_voice_current_ptr[m_voice_ofs+0];
            int64_t ofs_end=(ofs_start+smplIncr);
            
            if (ofs_end>ofs_start)
            for (;;) {
                INT32 val=tmpOutL;
                if (!(chip->PWM_Mute&1)) m_voice_buff[m_voice_ofs+0][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)]=LIMIT8((val>>7));
                val=tmpOutR;
                if (!(chip->PWM_Mute&2)) m_voice_buff[m_voice_ofs+1][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)]=LIMIT8((val>>7));
                ofs_start+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                if (ofs_start>=ofs_end) break;
            }
            while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4*2) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*4*2<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
            m_voice_current_ptr[m_voice_ofs+0]=ofs_end;
            m_voice_current_ptr[m_voice_ofs+1]=ofs_end;
        }
        //TODO:  MODIZER changes end / YOYOFR
	}
	
	return;
}


static UINT8 device_start_pwm(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	pwm_chip *chip;
	UINT32 rate;
	
	chip = (pwm_chip *)calloc(1, sizeof(pwm_chip));
	if (chip == NULL)
		return 0xFF;
	
	chip->clock = cfg->clock;
	
	rate = 22020;	// that's the rate the PWM uses in most games
	SRATE_CUSTOM_HIGHEST(cfg->srMode, rate, cfg->smplRate);
	
	//PWM_Init(chip);
	pwm_set_mute_mask(chip, 0x00);
	
	chip->_devData.chipInf = chip;
	INIT_DEVINF(retDevInf, &chip->_devData, rate, &devDef);
	return 0x00;
}

static void device_stop_pwm(void* info)
{
	pwm_chip* chip = (pwm_chip*)info;
	free(chip);
	
	return;
}

static void pwm_chn_w(void* info, UINT8 Channel, UINT16 data)
{
	pwm_chip* chip = (pwm_chip*)info;
	
	switch(Channel)
	{
	case 0x00/2:	// control register
		PWM_Set_Int(chip, data >> 8);
		break;
	case 0x02/2:	// cycle register
		PWM_Set_Cycle(chip, data);
		break;
	case 0x04/2:	// l ch
		chip->PWM_Out_L = data;
		break;
	case 0x06/2:	// r ch
		chip->PWM_Out_R = data;
		if (! chip->PWM_Mode)
		{
			if (chip->PWM_Out_L == chip->PWM_Out_R)
			{
				// fixes these terrible pops when
				// starting/stopping/pausing the song
				chip->PWM_Offset = data;
				chip->PWM_Mode = 0x01;
			}
		}
		break;
	case 0x08/2:	// mono ch
		chip->PWM_Out_L = data;
		chip->PWM_Out_R = data;
		if (! chip->PWM_Mode)
		{
			chip->PWM_Offset = data;
			chip->PWM_Mode = 0x01;
		}
		break;
	}
	
	return;
}

static void pwm_set_mute_mask(void *info, UINT32 MuteMask)
{
	pwm_chip* chip = (pwm_chip*)info;
	
	chip->PWM_Mute = (MuteMask >> 0) & 0x03;//YOYOFR
	
	return;
}
