// based on orginal "in_org" WinAmp plugin by JTE
// License:  DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

//TODO:  MODIZER changes start / YOYOFR
#include "../../../../src/ModizerVoicesData.h"
extern uint64_t organya_mute_mask;
//TODO:  MODIZER changes end / YOYOFR


#include "Wave100.inc"

// Org-02 drums
#include "Bass01.inc"
#include "Bass02.inc"
#include "Snare01.inc"
#include "Snare02.inc"
#include "Tom01.inc"
#include "HiClose.inc"
#include "HiOpen.inc"
#include "Crash.inc"
#include "Per01.inc"
#include "Per02.inc"
#include "Bass03.inc"
#include "Tom02.inc"

// Org-01 (beta) drums
#include "Bass01Beta.inc"
#include "CrashBeta.inc"
#include "HiCloseBeta.inc"
#include "HiOpenBeta.inc"
#include "Snare01Beta.inc"

// OrgMaker 2 drums
#include "Bass04.inc"
#include "Bass05.inc"
#include "Snare03.inc"
#include "Snare04.inc"
#include "HiClos2.inc"
#include "HiOpen2.inc"
#include "HiClos3.inc"
#include "HiOpen3.inc"
#include "Crash02.inc"
#include "RevSym.inc"
#include "Ride01.inc"
#include "Tom03.inc"
#include "Tom04.inc"
#include "Orcdrm.inc"
#include "Bell.inc"
#include "Cat.inc"

#define VERSION "v1.08 config"

#define FALSE 0
#define TRUE 1

#define MAX_PATH 1024
char lastfn[MAX_PATH];
static int decode_pos_ms; // current decoding position, in milliseconds

// post this to the main window at end of file (after playback as stopped)
#define WM_WA_MPEG_EOF WM_USER+2

// Currently only 1 or 2 channel is supported.
// Can do any BPS and SAMPLERATE.
#define NCH (2)
#define BPS (16)			// use same as for PXtone
#define SAMPLERATE (44100)
//char sample_buffer[576*NCH*(BPS/8)*2]; // sample buffer

//extern char *pixel_sample_buffer;	// use same as for PXtone


#define CHANNELS 16
typedef struct
{
	unsigned int position;
	unsigned char key,len,vol,pan;
} orgnote_t;

typedef struct
{
	unsigned short pitch;
	unsigned char inst,pi;
	unsigned short num_notes;
	double pos;
	const unsigned char *block;
	unsigned int length;
	unsigned short freq;
	short wave;
	orgnote_t *notes;
	struct
	{
		unsigned int position;
		unsigned char key,len,pan,dimmed;
		unsigned short vol;
	} playing;
} orgchan_t;

typedef struct
{
	unsigned char loaded;
	char version_string[7];
	unsigned char version;
	unsigned short tempo;
	unsigned char steps,beats;
	unsigned int loop_start,loop_end,
		step,tick,num_channels,num_instruments,nonlooping;
	orgchan_t chan[CHANNELS];
} org_t;
org_t org;

static void clear_sound(void)
{
	int i;
	for(i = 0; i < CHANNELS; i++)
	{
		org.chan[i].pos = -2.0;
		org.chan[i].playing.position = 0;
		org.chan[i].playing.len = 0;
	}
}


int maxstep=3; //julian edit
int sstep=0; //julian edit
static int set_step(unsigned int new_step)
{
	int i, j;
	org.step = new_step;
	if(org.step >= org.loop_start && org.nonlooping)
		return 1;
	else if(org.step == org.loop_end)
	{
		sstep++; //julian edit
		if(sstep<maxstep||maxstep==0){ //julian edit
    		clear_sound();  
    		decode_pos_ms = org.loop_start*org.tempo; //julian edit
	       	return set_step(org.loop_start); //julian edit
		}else{ //julian edit
            sstep=0;
            org.nonlooping=1; 
        } //julian edit
	}
	for(i = 0; i < CHANNELS; i++)
	{
		if(org.chan[i].playing.len > 0)
			org.chan[i].playing.len--;

		j = 0;
		if(org.chan[i].playing.position)
			j = org.chan[i].playing.position;
		for(; j < org.chan[i].num_notes; j++)
			if(org.chan[i].notes[j].position == org.step)
			{
				org.chan[i].playing.position = j+1;
				if(org.chan[i].notes[j].key != 0xFF)
				{
					org.chan[i].playing.key = org.chan[i].notes[j].key;
					if(org.chan[i].pi)
						org.chan[i].playing.len = 1;
					else
						org.chan[i].playing.len = org.chan[i].notes[j].len;
					org.chan[i].pos = -1.0;
					org.chan[i].playing.dimmed = FALSE;
				}
				if(org.chan[i].notes[j].vol != 0xFF)
				{
					if (i < CHANNELS/2) {
						org.chan[i].playing.vol = org.chan[i].notes[j].vol+50;
						if (org.chan[i].playing.dimmed)
							org.chan[i].playing.vol = 3 * org.chan[i].playing.vol / 4;
					} else
						org.chan[i].playing.vol = org.chan[i].notes[j].vol+50;
				}
				if(org.chan[i].notes[j].pan != 0xFF)
					org.chan[i].playing.pan = org.chan[i].notes[j].pan;
			}
	}
	return 0;
}

//yoyofr
int org_setPosition(int pos_ms) {
    if(!org.loaded || (org.tempo==0) )
        return -1000;
    unsigned int new_step=pos_ms/org.tempo;
    if(org.nonlooping) {
        if (new_step<org.step) {
            clear_sound();
        }
        set_step(new_step);
        return 0;
    }
    sstep=0;
    while (new_step>org.loop_end) {
        new_step-=(org.loop_end-org.loop_start);
        sstep++;
    }
    
    if (new_step<org.step) {
        clear_sound();
    }
    
    set_step(new_step);
    return 0;
}
//yoyofr


void unload_org(void)
{
	int i;
	if(!org.loaded)
		return;
	for(i = 0; i < CHANNELS; i++)
		free(org.chan[i].notes);
	org.loaded = 0;
	org.step = 0;
	org.tick = 0;
}
static void memread(void *ptr, size_t size, size_t count, char **buf) {
	memcpy(ptr, (*buf), size*count); (*buf)+= size*count;
}

static int load_org(const char *fn, char **buf)
{
//	FILE *fp;	get rid of FS access
	int i, j;
	if(!strcmp(fn,lastfn) && org.loaded)
	{
		clear_sound();
		set_step(0);
		return 0;
	}
	unload_org();
/*
	fp = fopen(fn,"rb");
	if(!fp)
		return 1;
*/
//	fread(org.version_string, 1, 6, fp);
	memread(org.version_string, 1, 6, buf);
	org.version_string[6] = '\0';
	if(strncmp(org.version_string,"Org-0",5) || org.version_string[5] < '1' || org.version_string[5] > '3')
	{
//		fclose(fp);
		fprintf(stderr,"Invalid Org header '%s' (Only 'Org-01' through 'Org-03' supported)",org.version_string);
		return 1;
	}
	org.version = atoi(&org.version_string[4]);
//	fread(&org.tempo, 2, 1, fp);
	memread(&org.tempo, 2, 1, buf);
//	org.steps = getc(fp); // steps per beat
	memread(&org.steps, 1, 1, buf);// steps per beat
//	org.beats = getc(fp); // beats per bar
	memread(&org.beats, 1, 1, buf);// beats per bar

	//	fread(&org.loop_start, 4, 1, fp);
	memread(&org.loop_start, 4, 1, buf);
//	fread(&org.loop_end, 4, 1, fp);
	memread(&org.loop_end, 4, 1, buf);

	org.num_channels = CHANNELS;
	org.num_instruments = CHANNELS/2;
	for(i = 0; i < CHANNELS; i++)
	{
		memset(&org.chan[i],0,sizeof(org.chan[i]));
		memset(&org.chan[i].playing,0,sizeof(org.chan[i].playing));
//		fread(&org.chan[i].pitch, 2, 1, fp);
		memread(&org.chan[i].pitch, 2, 1, buf);
//		fread(&org.chan[i].inst, 1, 1, fp);
		memread(&org.chan[i].inst, 1, 1, buf);
		if(i < CHANNELS/2)
		{
			org.chan[i].block = &Wave100[(org.chan[i].inst)*WAVELENGTH];
			org.chan[i].length = WAVELENGTH+1;
			org.chan[i].freq = 44100+org.chan[i].pitch-1000;
		}
		else
		{
			if (org.version == 1)
			{
			org.chan[i].freq = 22050;
			switch(org.chan[i].inst)
			{
				case 0:
					org.chan[i].block = Bass01Beta;
					org.chan[i].length = BASS01BETALENGTH;
					break;
				case 1:
					org.chan[i].block = Bass02;
					org.chan[i].length = BASS02LENGTH;
					break;
				case 2:
					org.chan[i].block = Snare01Beta;
					org.chan[i].length = SNARE01BETALENGTH;
					break;
				case 3:
					org.chan[i].block = Snare02;
					org.chan[i].length = SNARE02LENGTH;
					break;
				case 4:
					org.chan[i].block = Tom01;
					org.chan[i].length = TOM01LENGTH;
					break;
				case 5:
					org.chan[i].block = HiCloseBeta;
					org.chan[i].length = HICLOSEBETALENGTH;
					break;
				case 6:
					org.chan[i].block = HiOpenBeta;
					org.chan[i].length = HIOPENBETALENGTH;
					break;
				case 7:
					org.chan[i].block = CrashBeta;
					org.chan[i].length = CRASHBETALENGTH;
					break;
				case 8:
					org.chan[i].block = Per01;
					org.chan[i].length = PER01LENGTH;
					break;
				case 9:
					org.chan[i].block = Per02;
					org.chan[i].length = PER02LENGTH;
					break;
				case 10:
					org.chan[i].block = Bass03;
					org.chan[i].length = BASS03LENGTH;
					break;
				case 11:
					org.chan[i].block = Tom02;
					org.chan[i].length = TOM02LENGTH;
					break;
				default:
					org.chan[i].block = NULL;
					org.chan[i].length = 0;
					{
						fprintf(stderr,"Invalid Org instrument %d",org.chan[i].inst);
					}
					break;
			}
			}
			else if (org.version == 2)
			{
			org.chan[i].freq = 22050;
			switch(org.chan[i].inst)
			{
				case 0:
					org.chan[i].block = Bass01;
					org.chan[i].length = BASS01LENGTH;
					break;
				case 1:
					org.chan[i].block = Bass02;
					org.chan[i].length = BASS02LENGTH;
					break;
				case 2:
					org.chan[i].block = Snare01;
					org.chan[i].length = SNARE01LENGTH;
					break;
				case 3:
					org.chan[i].block = Snare02;
					org.chan[i].length = SNARE02LENGTH;
					break;
				case 4:
					org.chan[i].block = Tom01;
					org.chan[i].length = TOM01LENGTH;
					break;
				case 5:
					org.chan[i].block = HiClose;
					org.chan[i].length = HICLOSELENGTH;
					break;
				case 6:
					org.chan[i].block = HiOpen;
					org.chan[i].length = HIOPENLENGTH;
					break;
				case 7:
					org.chan[i].block = Crash;
					org.chan[i].length = CRASHLENGTH;
					break;
				case 8:
					org.chan[i].block = Per01;
					org.chan[i].length = PER01LENGTH;
					break;
				case 9:
					org.chan[i].block = Per02;
					org.chan[i].length = PER02LENGTH;
					break;
				case 10:
					org.chan[i].block = Bass03;
					org.chan[i].length = BASS03LENGTH;
					break;
				case 11:
					org.chan[i].block = Tom02;
					org.chan[i].length = TOM02LENGTH;
					break;
				// OrgMaker 2 EN
				case 12:
					org.chan[i].block = Bass04;
					org.chan[i].length = BASS04LENGTH;
					break;
				case 13:
					org.chan[i].block = Bass05;
					org.chan[i].length = BASS05LENGTH;
					break;
				case 14: // Snare03
					org.chan[i].block = Snare03;
					org.chan[i].length = SNARE03LENGTH;
					break;
				case 15: // Snare04
					org.chan[i].block = Snare04;
					org.chan[i].length = SNARE04LENGTH;
					break;
				case 16: // HiClos2
					org.chan[i].block = HiClos2;
					org.chan[i].length = HICLOS2LENGTH;
					break;
				case 17: // HiOpen2
					org.chan[i].block = HiOpen2;
					org.chan[i].length = HIOPEN2LENGTH;
					break;
				case 18: // HiClos3
					org.chan[i].block = HiClos3;
					org.chan[i].length = HICLOS3LENGTH;
					break;
				case 19: // HiOpen3
					org.chan[i].block = HiOpen3;
					org.chan[i].length = HIOPEN3LENGTH;
					break;
				case 20:
					org.chan[i].block = Crash02;
					org.chan[i].length = CRASH02LENGTH;
					break;
				case 21:
					org.chan[i].block = RevSym;
					org.chan[i].length = REVSYMLENGTH;
					break;
				case 22: // Ride01
					org.chan[i].block = Ride01;
					org.chan[i].length = RIDE01LENGTH;
					break;
				case 23: // Tom03
					org.chan[i].block = Tom03;
					org.chan[i].length = TOM03LENGTH;
					break;
				case 24: // Tom04
					org.chan[i].block = Tom04;
					org.chan[i].length = TOM04LENGTH;
					break;
				case 25:
					org.chan[i].block = Orcdrm;
					org.chan[i].length = ORCDRMLENGTH;
					break;
				case 26:
					org.chan[i].block = Bell;
					org.chan[i].length = BELLLENGTH;
					break;
				case 27:
					org.chan[i].block = Cat;
					org.chan[i].length = CATLENGTH;
					break;
				default:
					org.chan[i].block = NULL;
					org.chan[i].length = 0;
					{
						fprintf(stderr,"Invalid Org instrument %d",org.chan[i].inst);
					}
					break;
			}
			}
			org.chan[i].freq += org.chan[i].pitch-1000;
		}
//		fread(&org.chan[i].pi, 1, 1, fp);
		memread(&org.chan[i].pi, 1, 1, buf);
//		fread(&org.chan[i].num_notes, 2, 1, fp);
		memread(&org.chan[i].num_notes, 2, 1, buf);
		if(org.chan[i].num_notes > 0)
		{
			org.chan[i].notes = malloc(sizeof(orgnote_t)*org.chan[i].num_notes);
			memset(org.chan[i].notes,0,sizeof(*org.chan[i].notes));
		}
		else
		{
			org.num_channels--;
			if(i < CHANNELS/2)
				org.num_instruments--;
		}
	}

	org.nonlooping = 1;
	for(i = 0; i < CHANNELS; i++)
	{
		if(org.chan[i].num_notes <= 0)
			continue;
		for(j = 0; j < org.chan[i].num_notes; j++)
		{
//			fread(&org.chan[i].notes[j].position, 4, 1, fp);
			memread(&org.chan[i].notes[j].position, 4, 1, buf);
			if(org.chan[i].notes[j].position > org.loop_start
			&& org.chan[i].notes[j].position < org.loop_end)
				org.nonlooping = 0;
		}
		for(j = 0; j < org.chan[i].num_notes; j++)
//			fread(&org.chan[i].notes[j].key, 1, 1, fp);
			memread(&org.chan[i].notes[j].key, 1, 1, buf);
		for(j = 0; j < org.chan[i].num_notes; j++)
//			fread(&org.chan[i].notes[j].len, 1, 1, fp);
			memread(&org.chan[i].notes[j].len, 1, 1, buf);
		for(j = 0; j < org.chan[i].num_notes; j++)
//			fread(&org.chan[i].notes[j].vol, 1, 1, fp);
			memread(&org.chan[i].notes[j].vol, 1, 1, buf);
		for(j = 0; j < org.chan[i].num_notes; j++)
//			fread(&org.chan[i].notes[j].pan, 1, 1, fp);
			memread(&org.chan[i].notes[j].pan, 1, 1, buf);
	}
//	fclose(fp);

	org.loaded = 1;
	clear_sound();
	set_step(0);
	return 0;
}

//YOYOFR
void org_setLoopNb(int loopNb) {
    maxstep=loopNb+1;
}
    
void org_init(void)
{           
	maxstep=3; 
	org.loaded = 0;
}


int org_getlength(void)
{
	if(!org.loaded)
		return -1000;
	if(org.nonlooping)
		return org.loop_start*org.tempo;
	return org.loop_end*org.tempo+(org.loop_end-org.loop_start)*org.tempo*(maxstep-1);
}

int org_play(const char *fn, char *buf) {
	char *tmp= buf;
	if(load_org(fn, &tmp))
		return 1;
	strcpy(lastfn,fn);
	decode_pos_ms=0;
	//memset(pixel_sample_buffer, 0, 576*NCH*(BPS/8)*2);

	return 0;
}

int org_getoutputtime(void)
{
	return org.loop_end*org.tempo;
}

static int get_samples(char *buf,int samplesNb)
{
	int l, wave;
	double pan, move;
	if(org.nonlooping && org.step >= org.loop_start)
		return 0;
    
    
    //TODO:  MODIZER changes start / YOYOFR
    //search first voice linked to current chip
    if (!m_voice_current_samplerate) {
        m_voice_current_samplerate=44100;
        //printf("voice sample rate null\n");
    }
    if (samplesNb!=1024) {
        //printf("okok\n");
    }
    int64_t smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/m_voice_current_samplerate;
    //TODO:  MODIZER changes end / YOYOFR
    
	for(l = 0; l < samplesNb*NCH*(BPS/8)/*576*NCH*(BPS/8)*/; l++)
	{
		int i;
		// Calculate the current waves
		if(l%(NCH*(BPS/8)) == 0)
		{
			for(i = 0; i < CHANNELS; i++)
			{
				double fraction;
				if(org.chan[i].pos < -1.0)
					continue;
				if(((i < CHANNELS/2 && org.chan[i].playing.len <= 0) || i >= CHANNELS/2)
					&& org.chan[i].pos > 0.0 && (unsigned int)org.chan[i].pos >= org.chan[i].length+1)
				{
					org.chan[i].pos = -2.0;
					org.chan[i].wave = 0;
					continue;
				}
				fraction = org.chan[i].pos - floor(org.chan[i].pos);
				if (i >= CHANNELS/2) {
					if ((int)org.chan[i].pos > 0)
						org.chan[i].wave = (signed)(org.chan[i].block[(int)(org.chan[i].pos-1)]-0x80)*(1.0-fraction);
					else
						org.chan[i].wave = 0;
					if (((unsigned int)org.chan[i].pos)+1 < org.chan[i].length)
						org.chan[i].wave += (signed)(org.chan[i].block[(int)org.chan[i].pos]-0x80)*fraction;
				}
				else {
					if ((int)org.chan[i].pos > 0)
						org.chan[i].wave = (signed char)org.chan[i].block[(int)(org.chan[i].pos-1)]*(1.0-fraction);
					else
						org.chan[i].wave = 0;
					if (((unsigned int)org.chan[i].pos)+1 < org.chan[i].length)
						org.chan[i].wave += (signed char)org.chan[i].block[(int)org.chan[i].pos]*fraction;
				}
				org.chan[i].wave *= org.chan[i].playing.vol / 256.0;
			}
		}

		// Copy in the current waves
		wave = 0x00;
		for(i = 0; i < CHANNELS; i++)
		{
            int smpl;
            if(org.chan[i].pos < -1.0) {
                if(l%(NCH*(BPS/8)) == 0) {
                    //TODO:  MODIZER changes start / YOYOFR
                    smpl=0;//org.chan[i].wave*pan;
                    int64_t ofs_start=m_voice_current_ptr[i];
                    int64_t ofs_end=(m_voice_current_ptr[i]+smplIncr);
                    for (;;) {
                        m_voice_buff[i][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)]=LIMIT8(((smpl)>>0));
                        ofs_start+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                        if (ofs_start>=ofs_end) break;
                    }
                    while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*2*4) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*2*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                    m_voice_current_ptr[i]=ofs_end;
                    //TODO:  MODIZER changes end / YOYOFR
                }
            } else {
                
                pan = 1.0;
                if(NCH == 2)
                {
                    pan = org.chan[i].playing.pan/8.0;
                    if((l/(BPS/8))%NCH == 0)
                        pan = 1.5-pan;
                    if(pan > 1.0)
                        pan = 1.0;
                }
                //if (i>=CHANNELS/2)
                if (organya_mute_mask&(1<<i)) smpl=0;
                else {
                    wave += org.chan[i].wave*pan;
                    smpl=org.chan[i].wave*pan;
                }
                
                if(l%(NCH*(BPS/8)) == 0) {
                    //TODO:  MODIZER changes start / YOYOFR
                    
                    int64_t ofs_start=m_voice_current_ptr[i];
                    int64_t ofs_end=(m_voice_current_ptr[i]+smplIncr);
                    for (;;) {
                        m_voice_buff[i][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)]=LIMIT8(((smpl)>>0));
                        ofs_start+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                        if (ofs_start>=ofs_end) break;
                    }
                    while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*2*4) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*2*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                    m_voice_current_ptr[i]=ofs_end;
                    //TODO:  MODIZER changes end / YOYOFR
                }
            }
		}

		// Clip the wave, don't overflow!
		if (wave > 0x7F)
			wave = 0x7F;
		if (wave < -0x80)
			wave = -0x80;
		if (BPS == 8)
			buf[l] = wave+0x80;
		else
			buf[l] = wave;

		// Move the waves along
		if(l%(NCH*(BPS/8)) == 0)
		{
			for(i = 0; i < CHANNELS; i++)
			{
				if(org.chan[i].pos < -1.0)
					continue;
				if(i < CHANNELS/2)
					move = pow(2.0,(org.chan[i].playing.key/12.0)-2.4);
				else
					move = org.chan[i].playing.key/28.0;
				if (move < 0.0)
					move = -move;
				move *= (double)org.chan[i].freq/SAMPLERATE;
				org.chan[i].pos += move;
				if (!org.chan[i].playing.dimmed && org.chan[i].pos >= org.chan[i].length)
				{
					org.chan[i].playing.vol = 3 * org.chan[i].playing.vol / 4;
					org.chan[i].playing.dimmed = TRUE;
				}
				while(i < CHANNELS/2 && org.chan[i].pos >= org.chan[i].length && org.chan[i].playing.len > 0)
					org.chan[i].pos -= org.chan[i].length;
			}
			org.tick++;
			if(org.tick >= (unsigned)org.tempo*SAMPLERATE/1000)
			{
				org.tick = 0;
				if(set_step(org.step+1))
					break;
			}
		}

	}
    
    //YOYOFR
    for (int i=0;i<CHANNELS;i++) {
        //YOYOFR
        if (!(organya_mute_mask&(1<<i))) {
            int32_t vol=org.chan[i].playing.vol;
            
            if (vol && (org.chan[i].pos >= -1.0)) {
                if(i < CHANNELS/2) {
                    float note=org.chan[i].playing.key;
                    vgm_last_note[i]=440.0*pow(2,(note)/12-2.4)*(double)org.chan[i].freq/SAMPLERATE;
                } else {
                    float note=org.chan[i].playing.key;
                    vgm_last_note[i]=440.0*note/28.0*(double)org.chan[i].freq/SAMPLERATE;
                }
                vgm_last_sample_addr[i]=i;
                if (!vgm_last_vol[i]) vgm_last_vol[i]=1;
            }
        }
    }
    //YOYOFR
    
	return l;
}

int org_currentpos() {
	return decode_pos_ms;
}

int org_gensamples(char *pixel_sample_buffer,int samplesNb) {    // use same as for PXtone) {
	int l= get_samples(pixel_sample_buffer,samplesNb);
	if(!l) {
		return 0;
	} else {
		decode_pos_ms+=(l/NCH/(BPS/8))*1000/SAMPLERATE;
		return 1;
	}
}
