/*
 MDXplayer : PCM8 emulater :-)
 
 Made by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
 Jan.16.1999
 */

/* ------------------------------------------------------------------ */

#include "ModizerConstants.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

# include <string.h>
# include <fcntl.h>
# include <sys/time.h>
# include <time.h>
# include <unistd.h>

# include <sys/ioctl.h>


#include "mdx.h"
#include "pcm8.h"
#include "mdx_ym2151.h"

extern void mdx_update(unsigned char *data,int len,int end_reached);

extern void* ym2151_instance(void);

extern void* create_freeverb(void);
extern void delete_freeverb(void* self);
extern void setparams_freeverb(void* self, int srate, float predelay, float room, float damp, float width, float dry, float wet);
extern void process_freeverb(void* self, unsigned char* data, int len);

/* ------------------------------------------------------------------ */
/* local instances */

typedef struct _pcm8_instances pcm8_instances;
struct _pcm8_instances {
	MDX_DATA *emdx;
	PCM8_WORK work[PCM8_MAX_NOTE];
	
	int pcm8_opened;
	int pcm8_interrupt_active;
	int dev;
	
	int master_volume;
	int master_pan;
	
	int is_encoding_16bit;
	int is_encoding_stereo;
	int dsp_speed;
	
	unsigned char *sample_buffer;
	int *sample_buffer2;
	int sample_buffer_size;
	int dest_buffer_size;
	
	SAMP *ym2151_voice[2];
	
	unsigned char *pcm_buffer;
	int pcm_buffer_ptr;
	int pcm_buffer_size;
	
	int is_pcm_buffer_flushed;
	
	void* freeverb;
};


/* ------------------------------------------------------------------ */
/* local defines */

#define DSP_ALSA_DEVICE_NAME "plughw:0,0"
#define PCM8_MAX_FREQ 5

static const int adpcm_freq_list[] = {
	3900, 5200, 7800, 10400, 15600
};

static const unsigned char riff[]={
	'R','I','F','F',
	0xff,0xff,0xff,0xff,
	'W','A','V','E','f','m','t',' ',
	16,0,0,0, 1,0, 2,0,
	PCM8_MASTER_PCM_RATE%256,PCM8_MASTER_PCM_RATE/256,0,0,
	(PCM8_MASTER_PCM_RATE*4)%256,((PCM8_MASTER_PCM_RATE*4)/256)%256,
	((PCM8_MASTER_PCM_RATE*4)/65536)%256,0,
	4,0, 16,0,
	'd','a','t','a',
	0,0,0,0
};

/* ------------------------------------------------------------------ */
/* class interface */

extern void* _get_pcm8(void);

static pcm8_instances*
__get_instances(void)
{
	return (pcm8_instances*)_get_pcm8();
}

#define __GETSELF  pcm8_instances* self = __get_instances();

void*
_pcm8_initialize(void)
{
	pcm8_instances* self = NULL;
	
	self = (pcm8_instances *)malloc(sizeof(pcm8_instances));
	if (!self) {
		return NULL;
	}
	memset(self, 0, sizeof(pcm8_instances));
	
	self->emdx                  = NULL;
	self->pcm8_opened           = FLAG_FALSE;
	self->pcm8_interrupt_active = 0;
	self->dev                   = -1;
	
	self->master_volume         = 0;
	self->master_pan            = MDX_PAN_N;
	
	self->is_encoding_16bit     = FLAG_FALSE;
	self->is_encoding_stereo    = FLAG_FALSE; 
	self->dsp_speed             = 0;
	
	self->sample_buffer         = NULL;
	self->sample_buffer2        = NULL;
	self->sample_buffer_size    = 0;
	
	self->ym2151_voice[0]       = NULL;
	self->ym2151_voice[1]       = NULL;
	
	self->pcm_buffer            = NULL;
	self->pcm_buffer_ptr        = 0;
	self->pcm_buffer_size       = 0;
	
	
	self->is_pcm_buffer_flushed = FLAG_FALSE;
	
	self->freeverb              = NULL;
	
	return self;
}

void
_pcm8_finalize(void* in_self)
{
	pcm8_instances* self = (pcm8_instances *)in_self;
	if (self) {
		free(self);
	}
}

/* ------------------------------------------------------------------ */
/* implementations */

static int
stdout_open(MDX_DATA* mdx) 
{
	int i=0;
	__GETSELF;
	
	self->is_encoding_16bit = FLAG_TRUE;
	self->is_encoding_stereo = FLAG_TRUE;
	self->dsp_speed = PCM8_MASTER_PCM_RATE;
	
	if (self->emdx->is_output_to_stdout_in_wav) {
		fwrite( riff, 1, 44, stdout );
		for ( i=0 ; i<32*4096 ; i++ )
			fputc( 0,stdout );
	}
	
	return 0;
}

# define oss_open(a) (1)
# define esd_open(a) (1)
# define alsa_open(a) (1)

static void pcm8_close_devs(void) {
	//__GETSELF;
}

static void
pcm8_write_dev(unsigned char* data, int len,int end_reached)
{
	__GETSELF;
	
	if (self->freeverb) {
		process_freeverb(self->freeverb, data, len);
	}
	
	mdx_update(data,len,end_reached);
}

int pcm8_open( MDX_DATA *mdx )
{
	int i;
	//unsigned char* dummy = NULL;
	__GETSELF;
	
	self->emdx = mdx;
	pcm8_close_devs();
	
	if ( self->pcm_buffer != NULL ) free(self->pcm_buffer);
	self->pcm_buffer = NULL;
	
	self->is_encoding_16bit  = FLAG_TRUE;
	self->is_encoding_stereo = FLAG_TRUE;
	self->dsp_speed  = PCM8_MASTER_PCM_RATE;
	self->pcm_buffer_ptr = 0;
	self->pcm_buffer_size = SOUND_BUFFER_SIZE_SAMPLE*4;
	self->pcm_buffer = (unsigned char *)malloc(sizeof(unsigned char)*self->pcm_buffer_size);
	
	self->sample_buffer_size = self->dsp_speed * PCM8_SYSTEM_RATE / 1000;
	
	if ( self->is_encoding_stereo == FLAG_TRUE ) {
		self->dest_buffer_size = self->sample_buffer_size*4;
	}
	else {
		self->dest_buffer_size = self->sample_buffer_size*2;
	}
	
	if ( self->sample_buffer == NULL ) {
		self->sample_buffer =
		(unsigned char *)malloc(sizeof(char)*self->sample_buffer_size*2*2);
	}
	if ( self->sample_buffer2 == NULL ) {
		self->sample_buffer2 =
		(int *)malloc(sizeof(int)*self->sample_buffer_size);
	}
	
	if ( self->sample_buffer == NULL || self->sample_buffer2 == NULL ) {
		if ( self->sample_buffer != NULL ) free( self->sample_buffer );
		if ( self->sample_buffer2 != NULL ) free( self->sample_buffer2 );
		return 1;
	}
	
	for ( i=0 ; i<2 ; i++ ) {
		if ( self->ym2151_voice[i] == NULL ) {
			self->ym2151_voice[i] = (SAMP *)malloc(self->sample_buffer_size*2);
		}
	}
	
	if ( self->ym2151_voice[0] == NULL || self->ym2151_voice[1] == NULL ) {
		if ( self->ym2151_voice[0] != NULL ) free( self->ym2151_voice[0] );
		if ( self->ym2151_voice[1] != NULL ) free( self->ym2151_voice[1] );
		return 1;
	}
	
	if (mdx->is_use_reverb) {
		self->freeverb = create_freeverb();
		
		if (self->freeverb) {
			setparams_freeverb(self->freeverb, self->dsp_speed, mdx->reverb_predelay, mdx->reverb_roomsize, mdx->reverb_damp, mdx->reverb_width, mdx->reverb_dry, mdx->reverb_wet);
		}
	}
	
	/* workaround for making noise in some environment... */
	/*  dummy = malloc(4*1024);
	 if (dummy) {
	 memset(dummy, 0, 4*1024);
	 for (i=0; i<16; i++) {
	 pcm8_write_dev(dummy, 4*1024);
	 }
	 free(dummy);
	 }*/
	
	self->pcm8_opened = FLAG_TRUE;
	self->emdx->dsp_speed = self->dsp_speed;
	
	pcm8_init();
	
	return 0;
}

int pcm8_close( void )
{
	int i;
	int free_all = FLAG_TRUE;
	__GETSELF;
	
	self->pcm8_opened = FLAG_FALSE;
	
	if (self->freeverb) {
		delete_freeverb(self->freeverb);
		self->freeverb = NULL;
	}
	
	pcm8_stop();
	pcm8_close_devs();
	
	if ( free_all == FLAG_TRUE ) {
		for ( i=0 ; i<2 ; i++ ) {
			if ( self->ym2151_voice[i]!=NULL ) {
				free( self->ym2151_voice[i] );
				self->ym2151_voice[i]=NULL;
			}
		}
		
		if ( self->sample_buffer2 != (int *)NULL ) {
			free( self->sample_buffer2 );
			self->sample_buffer2 = NULL;
		}
		
		if ( self->sample_buffer!=NULL ) {
			free( self->sample_buffer );
			self->sample_buffer = NULL;
		}
		
		if ( self->pcm_buffer!=NULL ) {
			free( self->pcm_buffer );
			self->pcm_buffer = NULL;
		}
	}
	
	return 0;
}

void pcm8_init( void )
{
	int i;
	__GETSELF;
	
	for ( i=0; i<PCM8_MAX_NOTE ; i++ ) {
		self->work[i].ptr     = NULL;
		self->work[i].top_ptr = NULL;
		self->work[i].end_ptr = NULL;
		self->work[i].volume  = PCM8_MAX_VOLUME;
		self->work[i].freq    = adpcm_freq_list[PCM8_MAX_FREQ-1];
		self->work[i].adpcm   = FLAG_TRUE;
		
		self->work[i].isloop  = FLAG_FALSE;
		self->work[i].fnum    = 0;
		self->work[i].snum    = 0;
	}
	
	self->master_volume = PCM8_MAX_VOLUME;
	self->master_pan = MDX_PAN_C;
	
	return;
}

/* ------------------------------------------------------------------ */

int pcm8_set_pcm_freq( int ch, int hz ) {
	
	__GETSELF;
	
	if ( self->pcm8_opened == FLAG_FALSE ) return 1;
	if ( ch >= PCM8_MAX_NOTE || ch < 0 ) return 1;
	if ( hz < 0 ) return 1;
	if ( hz >= PCM8_MAX_FREQ ) {
		self->work[ch].adpcm = FLAG_FALSE;
		self->work[ch].freq = 15600;
	} else {
		self->work[ch].freq = adpcm_freq_list[hz];
		self->work[ch].adpcm = FLAG_TRUE;
	}
	
	return 0;
}

int pcm8_note_on( int ch, int *ptr, int size, int* orig_ptr, int orig_size ) {
	
	__GETSELF;
	
	if ( self->emdx->is_use_ym2151==FLAG_TRUE &&
		self->emdx->is_use_pcm8==FLAG_FALSE) {
		return 1;
	}
	
	if ( self->pcm8_opened == FLAG_FALSE ) return 1;
	if ( ch >= PCM8_MAX_NOTE || ch < 0 ) return 1;
	
	if ( self->work[ch].top_ptr!=NULL ) return 0; /* tie */
	
	if (self->work[ch].adpcm) {
		self->work[ch].ptr = ptr;
		self->work[ch].top_ptr = ptr;
		self->work[ch].end_ptr = ptr+size;
	} else {
		self->work[ch].ptr = orig_ptr;
		self->work[ch].top_ptr = orig_ptr;
		self->work[ch].end_ptr = orig_ptr+orig_size;
	}
	
	self->work[ch].isloop = FLAG_FALSE;
	
	return 0;
}

int pcm8_note_off( int ch ) {
	
	__GETSELF;
	
	if ( self->pcm8_opened == FLAG_FALSE ) return 1;
	if ( ch >= PCM8_MAX_NOTE || ch < 0 ) return 1;
	
	self->work[ch].ptr = NULL;
	self->work[ch].top_ptr = NULL;
	self->work[ch].end_ptr = NULL;
	
	self->work[ch].isloop = FLAG_FALSE;
	
	return 0;
}

int pcm8_set_volume( int ch, int val ) {
	
	__GETSELF;
	
	if ( self->pcm8_opened == FLAG_FALSE ) return 1;
	if ( ch >= PCM8_MAX_NOTE ) return 1;
	if ( val > PCM8_MAX_VOLUME ) return 1;
	if ( val < 0 ) return 1;
	
	self->work[ch].volume = val;
	
	return 0;
}

int pcm8_set_master_volume( int val ) {
	
	__GETSELF;
	
	if ( val > PCM8_MAX_VOLUME ) { return 1; }
	if ( val < 0 ) { return 1; }
	
	self->master_volume = val;
	
	return 0;
}

int pcm8_set_pan( int val ) {
	
	__GETSELF;
	
	if ( self->pcm8_opened == FLAG_FALSE ) { return 1; }
	self->master_pan = val;
	
	return 0;
}

/* internal helpers */

static void
__make_noise(pcm8_instances* self)
{
	int i=0;
	int j=0;
	int k=0;
	
	while ( i<self->sample_buffer_size ) {
		j = (int)(8192.0 * rand()/RAND_MAX - 4096) *1.5* self->emdx->fm_noise_vol / 127;
		for ( k=0 ; (k<(32-self->emdx->fm_noise_freq)/4+1&&i<self->sample_buffer_size) ; k++,i++ ) {
			self->sample_buffer2[i] = j;
		}
	}
}

/* ------------------------------------------------------------------ */

/* PCM8 main function: mixes all of PCM sound and OPM emulator */

static inline void pcm8( int flush_for_end )
{
	int ch, i;
	int s=0;
	int *src, *dst;
	
	int is_dst_ran_out;
	int is_note_end;
	int f;
	int buffer_size;
	
	int v, sv;
	int l,r, sl,sr;
	unsigned char l1, l2, r1, r2, v1,v2;
	SAMP *ptr = NULL;
	
	__GETSELF;
	
	buffer_size = self->sample_buffer_size;
	
	/* must I pronounce? */
	
	if ( self->pcm8_opened   == FLAG_FALSE  || 
		self->sample_buffer == NULL ) return;
	
	/* Execute YM2151 emulator */
	
	if ( self->emdx->is_use_ym2151 == FLAG_TRUE &&
		self->emdx->is_use_fm_voice == FLAG_TRUE ) {
		YM2151UpdateOne( ym2151_instance(), self->ym2151_voice, self->sample_buffer_size );
	}
	
	/* reset buffer */
	
	if ( self->emdx->is_use_ym2151 == FLAG_FALSE &&
		self->emdx->fm_noise_vol>0 && self->emdx->fm_noise_freq>=0 ) {
		__make_noise(self);
	} else {
		memset(self->sample_buffer2, 0, sizeof(int)*self->sample_buffer_size);
	}
	
	if (self->emdx->haspdx==FLAG_TRUE) {
		for ( ch=0 ; ch<PCM8_MAX_NOTE ; ch++ ) {
			if ( !self->work[ch].ptr ) { continue; }
			
			/* frequency conversion */
			src = self->work[ch].ptr;
			dst = self->sample_buffer2;
			
			is_dst_ran_out=0;
			is_note_end=0;
			f=self->work[ch].fnum;
			s=self->work[ch].snum;
			
			while(is_dst_ran_out==0) {
				while( f>=0 ) {
					s = *(src++) * self->work[ch].volume / PCM8_MAX_VOLUME;
					if ( src >= self->work[ch].end_ptr ) {
						src--;
						is_note_end=1;
					}
					f -= self->dsp_speed;
				}
				while( f<0 ) {
					*(dst++) += s;
					f += self->work[ch].freq;
					if ( dst >= self->sample_buffer2+self->sample_buffer_size ) {
						is_dst_ran_out=1;
						break;
					}
				}
			}
			if ( is_note_end==1 ) {
				self->work[ch].ptr = NULL;
				self->work[ch].fnum = 0;
				self->work[ch].snum = 0;
			} else {
				self->work[ch].ptr = src;
				self->work[ch].fnum = f;
				self->work[ch].snum = s;
			}
		}
	}
	
	/* now pronouncing ! */
	
	if (self->is_encoding_stereo) {
		/* 16bit stereo */
		for ( i=0 ; i<self->sample_buffer_size ; i++ ) {
			v = self->sample_buffer2[i]/2 * self->master_volume/PCM8_MAX_VOLUME;
			
			switch(self->master_pan) {
				case MDX_PAN_L:
					l=v;
					r=0;
					break;
				case MDX_PAN_R:
					l=0;
					r=v;
					break;
				default:
					l=v;
					r=v;
					break;
			}
			
			if ( self->emdx->is_use_ym2151 == FLAG_TRUE &&
				self->emdx->is_use_fm_voice == FLAG_TRUE ) {
				ptr = (SAMP *)self->ym2151_voice[0];
				l += (int)(ptr[i]) * YM2151EMU_VOLUME;
				
				ptr = (SAMP *)self->ym2151_voice[1];
				r += (int)(ptr[i]) * YM2151EMU_VOLUME;
			}
			
			if ( l<-32768 ) l=-32768;
			else if ( l>32767 ) l=32767;
			if ( r<-32768 ) r=-32768;
			else if ( r>32767 ) r=32767;
			sl = l+32768; /* unsigned short L */
			sr = r+32768; /* unsigned short R */
			
			if ( l<0 ) l=0x10000+l;
			l1 = (unsigned char)(l>>8);
			l2 = (unsigned char)(l&0xff);
			
			if ( r<0 ) r=0x10000+r;
			r1 = (unsigned char)(r>>8);
			r2 = (unsigned char)(r&0xff);
			
			//low endian
			self->sample_buffer[i*4+0] = l2;
			self->sample_buffer[i*4+1] = l1;
			self->sample_buffer[i*4+2] = r2;
			self->sample_buffer[i*4+3] = r1;
		}
	} else {
		/* 16bit mono */
		for ( i=0 ; i<self->sample_buffer_size ; i++ ) {
			v = self->sample_buffer2[i]/2 * self->master_volume/PCM8_MAX_VOLUME;
			
			switch(self->master_pan) {
				case MDX_PAN_L:
					l=v;
					r=0;
					break;
				case MDX_PAN_R:
					l=0;
					r=v;
					break;
				default:
					l=v;
					r=v;
					break;
			}
			
			if ( self->emdx->is_use_ym2151 == FLAG_TRUE &&
				self->emdx->is_use_fm_voice == FLAG_TRUE ) {
				ptr = (SAMP *)self->ym2151_voice[0];
				l += (int)(ptr[i]) * YM2151EMU_VOLUME;
				
				ptr = (SAMP *)self->ym2151_voice[1];
				r += (int)(ptr[i]) * YM2151EMU_VOLUME;
			}
			
			if ( l<-32768 ) l=-32768;
			else if ( l>32767 ) l=32767;
			if ( r<-32768 ) r=-32768;
			else if ( r>32767 ) r=32767;
			v = (l+r)/2;  /*   signed short MONO */
			sv = v+32768; /* unsigned short MONO */
			
			if ( v<0 ) v=0x10000+v;
			v1 = (unsigned char)(v>>8);
			v2 = (unsigned char)(v&0xff);
			
			//low endian
			self->sample_buffer[i*2+0] = v2;
			self->sample_buffer[i*2+1] = v1;
		}
	}
	
    for ( i=0 ; i<self->dest_buffer_size ; i++ ) {
		if ( self->pcm_buffer_ptr >= self->pcm_buffer_size ) {
			pcm8_write_dev(self->pcm_buffer, self->pcm_buffer_size,0);
			self->is_pcm_buffer_flushed = FLAG_TRUE;
			self->pcm_buffer_ptr = 0;
		}
		self->pcm_buffer[self->pcm_buffer_ptr++] = self->sample_buffer[i];
    }
	if (flush_for_end /*&& self->pcm_buffer_ptr*/) {
		memset(self->pcm_buffer+self->pcm_buffer_ptr,0,self->pcm_buffer_size-self->pcm_buffer_ptr);
		pcm8_write_dev(self->pcm_buffer, self->pcm_buffer_size,1);
	}
	
	return;
}


void do_pcm8( int flush_for_end ) {
	
	pcm8(flush_for_end);
	return;
	
}

void pcm8_wait_for_pcm_write(void) {
	//__GETSELF;
}

void
pcm8_clear_buffer_flush_flag(void)
{
	__GETSELF;
	
	self->is_pcm_buffer_flushed = FLAG_FALSE;
}

int
pcm8_buffer_flush_flag(void)
{
	__GETSELF;
	
	return self->is_pcm_buffer_flushed;
}

/* ------------------------------------------------------------------ */

/* Timer hook */
void
pcm8_start(void)
{
}

void
pcm8_stop(void)
{
}
