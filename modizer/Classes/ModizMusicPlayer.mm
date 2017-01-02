//
//  ModizMusicPlayer.mm
//  modizer
//
//  Created by Yohann Magnien on 12/06/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#include <pthread.h>
#include <sqlite3.h>
#include <sys/xattr.h>

#include "fex.h"

extern pthread_mutex_t db_mutex;
extern pthread_mutex_t play_mutex;

int iModuleLength;
double iCurrentTime;
int mod_message_updated;



#import "ModizMusicPlayer.h"

//NVDSP
#import "Novocaine.h"
#import "NVDSP.h"
#import "NVPeakingEQFilter.h"
#import "NVSoundLevelMeter.h"
#import "NVClippingDetection.h"
NVClippingDetection *nvdsp_CDT;
float nvdsp_sr = 44100;
// define center frequencies of the bands
float nvdsp_centerFrequencies[10];
// define Q factor of the bands
float nvdsp_QFactor = 2.0f;
// define initial gain
float nvdsp_initialGain = 0.0f;
NVPeakingEQFilter *nvdsp_PEQ[EQUALIZER_NB_BANDS];
BOOL nvdsp_EQ;
float nvdsp_outData[SOUND_BUFFER_SIZE_SAMPLE*2];

#import "EQViewController.h"

//

//SIDPLAY
//SID1
#import "emucfg.h"
//SID2
#import "SidTune.h"
#import "sidplay2.h"
#import "resid.h"
//#import "residfp.h"



/* file types */
static char gmetype[64];

static uint32 ao_type;
volatile int mSlowDevice;
static volatile int moveToPrevSubSong,moveToNextSubSong,mod_wantedcurrentsub,mChangeOfSong,mNewModuleLength,moveToSubSong,moveToSubSongIndex;
static int sampleVolume,mInterruptShoudlRestart;
//static volatile int genCurOffset,genCurOffsetCnt;
static char str_name[1024];
static char stil_info[MAX_STIL_DATA_LENGTH];
char mod_message[8192+MAX_STIL_DATA_LENGTH];
static char mod_name[256];
static char mod_filename[512];
static char archive_filename[512];

static int mSingleFileType;

static char song_md5[33];

char mplayer_error_msg[1024];

static int mSingleSubMode;


#define DEFAULT_MODPLUG 0
#define DEFAULT_DUMB 1
#define DEFAULT_UADE 2

#define DEFAULT_ASAP 0
#define DEFAULT_SAPGME 1

#define DEFAULT_VGMVGM 0
#define DEFAULT_VGMGME 1

static int mdz_IsArchive,mdz_ArchiveFilesCnt,mdz_currentArchiveIndex;
static int mdz_defaultMODPLAYER,mdz_defaultSAPPLAYER,mdz_defaultVGMPLAYER;
static char **mdz_ArchiveFilesList;
static char **mdz_ArchiveFilesListAlias;

static NSFileManager *mFileMngr;

//GENERAL
static int mPanning,mPanningValue;

//DUMB
static float dumb_MastVol;

//MDX
static MDX_DATA *mdx;
static PDX_DATA *pdx;

//HVL
static int hvl_sample_to_write,mHVLinit;
//STSOUND
static YMMUSIC *ymMusic;
//SC68
static api68_t *sc68;

//SID
static int mSIDFilterON;
static sidplay2 *mSidEmuEngine;
static ReSIDBuilder *mBuilder;
//static ReSIDfpBuilder *mBuilder;
static SidTune *mSidTune;
static emuEngine *mSid1EmuEngine;
static sidTune *mSid1Tune;

static int sid_forceClock;
static int sid_forceModel;

//GME
static gme_info_t* gme_info;
static gme_equalizer_t gme_eq;
static gme_effects_t gme_fx;

//AOSDK
static struct {
	uint32 sig;
	const char *name;
	int32 (*start)(uint8 *, uint32, int32, int32);
	int32 (*gen)(int16 *, uint32);
	int32 (*stop)(void);
	int32 (*command)(int32, int32);
	uint32 rate;
	int32 (*fillinfo)(ao_display_info *);
} ao_types[] = {
	{ 0x50534601, "Sony PlayStation (.psf)", psf_start, psf_gen, psf_stop, psf_command, 60, psf_fill_info },
	{ 0x53505500, "Sony PlayStation (.spu)", spu_start, spu_gen, spu_stop, spu_command, 60, spu_fill_info },
	{ 0x50534602, "Sony PlayStation 2 (.psf2)", psf2_start, psf2_gen, psf2_stop, psf2_command, 60, psf2_fill_info },
    { 0x50534641, "Capcom QSound (.qsf)", qsf_start, qsf_gen, qsf_stop, qsf_command, 60, qsf_fill_info },
	{ 0x50534611, "Sega Saturn (.ssf)", ssf_start, ssf_gen, ssf_stop, ssf_command, 60, ssf_fill_info },
	{ 0x50534612, "Sega Dreamcast (.dsf)", dsf_start, dsf_gen, dsf_stop, dsf_command, 60, dsf_fill_info },
	{ 0xffffffff, "", NULL, NULL, NULL, NULL, 0, NULL }
};

int uade_song_end_trigger;


/****************/

void gsf_update(unsigned char *pSound,int lBytes);
extern "C" char gsf_libfile[1024];

extern "C" GD3_TAG VGMTag;
extern "C" VGM_HEADER VGMHead;
extern "C" UINT32 VGMMaxLoop;

int PreferJapTag=0;
static const wchar_t* GetTagStrEJ(const wchar_t* EngTag, const wchar_t* JapTag)
{
    const wchar_t* RetTag;
    
    if (EngTag == NULL || ! wcslen(EngTag))
    {
        RetTag = JapTag;
    }
    else if (JapTag == NULL || ! wcslen(JapTag))
    {
        RetTag = EngTag;
    }
    else
    {
        if (! PreferJapTag)
            RetTag = EngTag;
        else
            RetTag = JapTag;
    }
    
    if (RetTag == NULL)
        return L"";
    else
        return RetTag;
}


extern "C" int lha_main(int argc, char *argv[]);
extern "C" {
    //GSF
#include "gsf.h"
#include "psftag.h"
    //ASAP
#include "asap.h"
    
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
    //#include "readmidi.h"
#include "resample.h"
#include "output.h"
#include "controls.h"
#include "miditrace.h"
#include "timer.h"
#include "arc.h"
#include "aq.h"
#include "wrd.h"
    extern volatile int intr;
    
    extern int aosdk_dsf_samplecycle_ratio;
    extern unsigned char aosdk_dsf_22khz;
    extern int aosdk_dsfdsp;
    extern int aosdk_ssf_samplecycle_ratio;
    extern int aosdk_ssfdsp;
    
    static int optGENDefaultLength=SONG_DEFAULT_LENGTH;
    
    static char tim_filepath[1024];
    static int tim_max_voices=DEFAULT_VOICES;
    static int tim_reverb=0;
    static int tim_resampler=RESAMPLE_LINEAR;
    extern int tim_init(char *path);
    extern int tim_main(int argc,char **argv);
    extern int tim_close();
    
    static int tim_open_output(void); /* 0=success, 1=warning, -1=fatal error */
    static void tim_close_output(void);
    static int tim_output_data(char *buf, int32 nbytes);
    static int tim_acntl(int request, void *arg);
    
    extern int tim_midilength,tim_pending_seek,tim_current_voices,tim_lyrics_started;
    int tim_notes[SOUND_BUFFER_NB][DEFAULT_VOICES];
    int tim_notes_cpy[SOUND_BUFFER_NB][DEFAULT_VOICES];
    unsigned char tim_voicenb[SOUND_BUFFER_NB];
    unsigned char tim_voicenb_cpy[SOUND_BUFFER_NB];
    
    PlayMode ios_play_mode = {
        //  DEFAULT_RATE, PE_16BIT|PE_SIGNED, PF_PCM_STREAM|PF_CAN_TRACE,
        44100, PE_16BIT|PE_SIGNED, PF_PCM_STREAM,
        -1,
        {0}, /* default: get all the buffer fragments you can */
        "iOS pcm device", 'd',
        "",
        tim_open_output,
        tim_close_output,
        tim_output_data,
        tim_acntl
    };
    extern PlayMode *play_mode;
    
    
    //DUMB
#include "dumb.h"
#include "it.h"
    int it_max_channels;
    
    int dumb_it_callback_loop(void *data) {
        (void)data;
        return 0;
    }
    
    
    typedef struct DUH_PLAYER {
        int n_channels;
        DUH_SIGRENDERER *dr;
        float volume;
    } DUH_PLAYER;
    DUH *duh;
    DUH_PLAYER *duh_player;
}

//ASAP
static unsigned char *ASAP_module;
static int ASAP_module_len;
static struct ASAP asap;


extern "C" {
    int gSpcSlowAPU=1;
	//GSF
	int defvolume=1000;
	int relvolume=1000;
	int TrackLength=0;
	int FadeLength=0;
	int IgnoreTrackLength, DefaultLength=150000;
	int playforever=0;
	int fileoutput=0;
	int TrailingSilence=1000;
	int DetectSilence=0, silencedetected=0, silencelength=5;
	
}
int cpupercent=0, sndNumChannels;
int sndBitsPerSample=16;

int deflen=120,deffade=4;

extern unsigned short soundFinalWave[1470];
extern int soundBufferLen;

extern char soundEcho;
extern char soundLowPass;
extern char soundReverse;
extern char soundQuality;

float decode_pos_ms; // current decoding position, in milliseconds
int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.

static int g_playing = 0;


extern "C" int LengthFromString(const char * timestring);
extern "C" int VolumeFromString(const char * volumestring);

extern "C" void end_of_track() {
	g_playing = 0;
}

extern "C" int GSFshoudlReset;
extern "C" int GSFsndSamplesPerSec;

extern "C" void writeSound(void) {
	int lBytes = (int)soundBufferLen;
	unsigned char *pSound=(unsigned char*)soundFinalWave;
	
	if (g_playing==0) return;
	
	gsf_update(pSound,lBytes);
	
	
	decode_pos_ms += (lBytes/(2*sndNumChannels) * 1000)/(float)GSFsndSamplesPerSec;
    if (seek_needed!=-1) {
        if (seek_needed<decode_pos_ms) {
            GSFshoudlReset=1;
            decode_pos_ms=0;
        }
    }
    
    //	printf("write : %d\n",lBytes);
}


void gsf_loop() {
    //	printf("enter loop %d,%d\n",TrackLength,decode_pos_ms);
	while (g_playing) {
		int remaining = TrackLength - (int)decode_pos_ms;
		if (remaining<0) {
			// this happens during silence period
			remaining = 0;
		}
		EmulationLoop();
		
        //		printf("remaining : %d\n",remaining);
        
	}
    //	printf("exit\n");
}


/*****************/

//LAZY USF
#ifdef ARCH_MIN_ARM_NEON
#include <arm_neon.h>
#endif

#include "../../libLazyusf/misc.h"
int32_t lzu_sample_rate;
#define LZU_SAMPLE_SIZE 1024
int16_t *lzu_sample_data;
float *lzu_sample_data_float;
float *lzu_sample_converted_data_float;
usf_loader_state * lzu_state;

//#include "corlett.h"
extern corlett_t *usf_info_data;

uint32 psfTimeToMS(char *str)
{
    int x, c=0;
    uint32 acc=0;
    char s[100];
    
    strncpy(s,str,100);
    s[99]=0;
    
    for (x=strlen(s); x>=0; x--)
    {
        if (s[x]=='.' || s[x]==',')
        {
            acc=atoi(s+x+1);
            s[x]=0;
        }
        else if (s[x]==':')
        {
            if(c==0)
            {
                acc+=atoi(s+x+1)*10;
            }
            else if(c==1)
            {
                acc+=atoi(s+x+(x?1:0))*10*60;
            }
            
            c++;
            s[x]=0;
        }
        else if (x==0)
        {
            if(c==0)
            { 
                acc+=atoi(s+x)*10;
            }
            else if(c==1) 
            {
                acc+=atoi(s+x)*10*60;
            }
            else if(c==2) 
            {
                acc+=atoi(s+x)*10*60*60;
            }
        }
    }
    
    acc*=100;
    return(acc);
}


////////////////////
extern "C" {
#include "../libresample/libsamplerate-0.1.8/src/samplerate.h"
}
#define SRC_DEFAULT_CONVERTER SRC_SINC_MEDIUM_QUALITY
double src_ratio;
SRC_STATE *src_state;
SRC_DATA src_data;

long src_callback(void *cb_data, float **data);

////////////////////

extern "C" {
	extern int sexypsf_missing_psflib;
	extern char sexypsf_psflib_str[256];
	extern int aopsf2_missing_psflib;
	extern char aopsf2_psflib_str[256];
    
	void mdx_update(unsigned char *data,int len,int end_reached);
	
	// redirect stubs to interface the Z80 core to the QSF engine
	/*	uint8 memory_read(uint16 addr)	{
	 return qsf_memory_read(addr);
	 }
	 uint8 memory_readop(uint16 addr) {
	 return memory_read(addr);
	 }
	 uint8 memory_readport(uint16 addr) {
	 return qsf_memory_readport(addr);
	 }
	 void memory_write(uint16 addr, uint8 byte) {
	 qsf_memory_write(addr, byte);
	 }
	 void memory_writeport(uint16 addr, uint8 byte) {
	 qsf_memory_writeport(addr, byte);
	 }
	 // stub for MAME stuff
	 int change_pc(int foo){
	 return 0;
	 }
	 */
	
	
	//UADE
#include "uadecontrol.h"
#include "ossupport.h"
#include "uadeconfig.h"
#include "uadeconf.h"
#include "sysincludes.h"
#include "songdb.h"
#include "amigafilter.h"
#include "uadeipc.h"
#include "uadeconstants.h"
#include "md5.h"
	
	
	void uade_dummy_wait() {
		[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_UADE_MS];
	}
	
	int uade_main (int argc, char **argv);
	
	struct uade_state UADEstate,UADEstatebase;
	char UADEconfigname[PATH_MAX];
	char UADEplayername[PATH_MAX];
	char UADEscorename[PATH_MAX];
	
}

static char my_data [] = "Our cleanup function was called";

/* Example cleanup function automatically called when emulator is deleted. */
static void my_cleanup( void* my_data ) {
	//NSLog(@"\n%s\n", (char*) my_data );
}

static 	int buffer_ana_subofs;
static short int **buffer_ana;
static volatile int uadeThread_running,timThread_running;
static volatile int buffer_ana_gen_ofs,buffer_ana_play_ofs;
static volatile int *buffer_ana_flag;
static volatile int bGlobalIsPlaying,bGlobalShouldEnd,bGlobalSeekProgress,bGlobalEndReached,bGlobalSoundGenInProgress,bGlobalSoundHasStarted;
static volatile int mNeedSeek,mNeedSeekTime;

// String holding the relative path to the source directory
static const char *pathdir;

static int tim_open_output(void) {
    return 0;
} /* 0=success, 1=warning, -1=fatal error */

static void tim_close_output(void) {
}

static int tim_output_data(char *buf, int32 nbytes) {
    if (mSlowDevice) nbytes*=2; //22Khz
    if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
		g_playing=0;
		return 0;
	}
	while (buffer_ana_flag[buffer_ana_gen_ofs]) {
		[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
		if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
			g_playing=0;
			return 0;
		}
	}
    
    if ((tim_midilength!=-1) && (tim_midilength!=iModuleLength)) {
        iModuleLength=tim_midilength;
        mod_message_updated=2;
    }
    
	int to_fill=SOUND_BUFFER_SIZE_SAMPLE*2*2-buffer_ana_subofs;
	if (nbytes<to_fill) {
		if (!mSlowDevice) {
			memcpy( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)buf,nbytes);
		} else {
			signed int *dst=(signed int *)((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs);
			signed int *src=(signed int *)buf;
			for (int i=0;i<nbytes/2/4;i++) {
				dst[i*2]=src[i];
				dst[i*2+1]=src[i];
			}
		}
		
		buffer_ana_subofs+=nbytes;
		
	} else {
        if (!mSlowDevice) {
			memcpy((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)buf,to_fill);
		} else {
			signed int *dst=(signed int *)((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs);
			signed int *src=(signed int *)buf;
			for (int i=0;i<to_fill/4/2;i++) {
				dst[i*2]=src[i];
				dst[i*2+1]=src[i];
			}
		}
        
		nbytes-=to_fill;
		buffer_ana_subofs=0;
        
        int i,voices,vol;
        memset(tim_notes[buffer_ana_gen_ofs],0,DEFAULT_VOICES*4);
        for(i = voices = 0; i < upper_voices; i++) {
            if(voice[i].status != VOICE_FREE) {
                vol=(voice[i].left_mix+voice[i].right_mix);//((int)(voice[i].envelope_volume>>24)<<16)|
                if (vol<0) vol=-vol;
                vol=vol>>3;
                if (vol>0xFF) vol=0xFF;
                
                //vol=(int)(voice[i].envelope_volume>>24);
                
                tim_notes[buffer_ana_gen_ofs][voices++]=
                voice[i].note|
                ((int)(voice[i].channel)<<8)|
                ((int)vol<<16)|
                ((int)(voice[i].status)<<24);
            }
        }
        tim_voicenb[buffer_ana_gen_ofs]=voices;
        
		
		buffer_ana_flag[buffer_ana_gen_ofs]=1;
        
        if ((mNeedSeek==2)&&(tim_pending_seek==-1)) {
			mNeedSeek=3;
			buffer_ana_flag[buffer_ana_gen_ofs]|=2;
 		}
        
		
		buffer_ana_gen_ofs++;
		if (buffer_ana_gen_ofs==SOUND_BUFFER_NB) buffer_ana_gen_ofs=0;
		
		if (nbytes>=SOUND_BUFFER_SIZE_SAMPLE*2*2) {
			NSLog(@"*****************\n*****************\n***************");
		} else if (nbytes) {
			while (buffer_ana_flag[buffer_ana_gen_ofs]) {
				[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
				if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
					g_playing=0;
					return 0;
				}
			}
			if (!mSlowDevice) {
				memcpy((char*)(buffer_ana[buffer_ana_gen_ofs]),((char*)buf)+to_fill,nbytes);
			} else {
				signed int *dst=(signed int *)(char*)(buffer_ana[buffer_ana_gen_ofs]);
				signed int *src=(signed int *)(((char*)buf)+to_fill/2);
				for (int i=0;i<nbytes/2/4;i++) {
					dst[i*2]=src[i];
					dst[i*2+1]=src[i];
				}
			}
            
			buffer_ana_subofs=nbytes;
		}
	}
    
    if (mNeedSeek==1) {
        tim_pending_seek=mNeedSeekTime/1000;
        bGlobalSeekProgress=-1;
        mNeedSeek=2;
    }
    
    return 0;
}

static int tim_acntl(int request, void *arg) {
    //    NSLog(@"acntl: req %d",request);
    
    switch (request){
        case PM_REQ_GETFRAGSIZ:
            *((int *)arg) = 4096>>mSlowDevice;
            return 0;
            
        case PM_REQ_GETQSIZ:
            //            NSLog(@"acntl: PM_REQ_GETQSIZ");
            /*            if (total_bytes == -1)
             return -1;
             *((int *)arg) = sizeof(globals.buffer);*/
            return -1;
            
        case PM_REQ_GETFILLABLE:
            //            NSLog(@"acntl: PM_REQ_GETFILLABLE");
            /*             if (total_bytes == -1)
             return -1;
             if(!(dpm.encoding & PE_MONO)) i >>= 1;
             if(dpm.encoding & PE_16BIT) i >>= 1;
             *((int *)arg) = i;*/
            return -1;
            
        case PM_REQ_GETFILLED:
            //             NSLog(@"acntl: PM_REQ_GETFILLED");
            /*             i = pstatus.queue;
             if(!(dpm.encoding & PE_MONO)) i >>= 1;
             if(dpm.encoding & PE_16BIT) i >>= 1;
             *((int *)arg) = i;*/
            return -1;
            
        case PM_REQ_GETSAMPLES:
            //            NSLog(@"acntl: PM_REQ_GETSAMPLES");
            //            *((int *)arg) = globals.samples*2;
            /*{
             static int c=0;
             if( c%0x100000==0 ){
             ctl->cmsg(CMSG_INFO, VERB_VERBOSE,
             "getsamples = %d", *((int *)arg));
             
             }
             }*/
            
            return -1;
            
        case PM_REQ_DISCARD:
            //            NSLog(@"acntl: PM_REQ_DISCARD");
            return 0;
            
        case PM_REQ_PLAY_START:
            //            NSLog(@"acntl: PM_REQ_PLAY_START");
            return 0;
            
        case PM_REQ_FLUSH:
            //            NSLog(@"acntl: PM_REQ_FLUSH");
            return 0;
        case PM_REQ_OUTPUT_FINISH:
            //            NSLog(@"acntl: PM_REQ_OUTPUT_FINISH");
            return 0;
            
        default:
            break;
    }
    return -1;
}


static void md5_from_buffer(char *dest, size_t destlen,char * buf, size_t bufsize)
{
	uint8_t md5[16];
	int ret;
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (const unsigned char*)buf, bufsize);
	MD5Final(md5, &ctx);
	ret =
	snprintf(dest, destlen,
		     "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
		     md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6],
		     md5[7], md5[8], md5[9], md5[10], md5[11], md5[12], md5[13],
		     md5[14], md5[15]);
	if (ret >= destlen || ret != 32) {
		fprintf(stderr, "md5 buffer error (%d/%zd)\n", ret, destlen);
		exit(1);
	}
}

static void writeLEword(unsigned char ptr[2], int someWord)
{
	ptr[0] = (someWord & 0xFF);
	ptr[1] = (someWord >> 8);
}


int ao_get_lib(char *filename, uint8 **buffer, uint64 *length) {
	uint8 *filebuf;
	uint32 size;
	FILE *auxfile;
	struct dirent **filelist = {0};
	const char *directory = ".";
	int fcount = -1;
	int i = 0;
	int found=0;
	
	//some aux files come from case insensitive OS, so parsing of dir is needed...
	fcount = scandir(pathdir, &filelist, 0, alphasort);
	
	if(fcount < 0) {
		perror(directory);
		return AO_FAIL;
	}
	
	for(i = 0; i < fcount; i++)  {
		if (!strcasecmp(filelist[i]->d_name,filename)) {
			found=1;
			strcpy(filename,filelist[i]->d_name);
		}
        //	free(filelist[i]);
	}
	//free(filelist);
    
    if (found==0) {
        char tmpname1[64];
        char tmpname2[64];
        int k=0;
        for (int j=0;j<strlen(filename);j++) {
            if ( ((filename[j]>='a')&&(filename[j]<='z'))||
                ((filename[j]>='A')&&(filename[j]<='Z'))||
                ((filename[j]>='0')&&(filename[j]<='9')) ) tmpname1[k++]=filename[j];
            if (k==63) break;
        }
        tmpname1[k]=0;
        for(i = 0; i < fcount; i++)  {
            k=0;
            for (int j=0;j<strlen(filelist[i]->d_name);j++) {
                if ( ((filelist[i]->d_name[j]>='a')&&(filelist[i]->d_name[j]<='z'))||
                    ((filelist[i]->d_name[j]>='A')&&(filelist[i]->d_name[j]<='Z'))||
                    ((filelist[i]->d_name[j]>='0')&&(filelist[i]->d_name[j]<='9')) ) tmpname2[k++]=filelist[i]->d_name[j];
                if (k==63) break;
            }
            tmpname2[k]=0;
            if (!strcasecmp(tmpname2,tmpname1)) {
                found=1;
                strcpy(filename,filelist[i]->d_name);
                break;
            }
        }
    }
    
    for (i=0;i<fcount;i++) {
        free(filelist[i]);
    }
    free(filelist);
	
	auxfile = fopen([[NSString stringWithFormat:@"%s/%s",pathdir,filename] UTF8String] , "rb");
	if (!auxfile) {
		printf("Unable to find auxiliary file %s\n", filename);
		return AO_FAIL;
	}
	fseek(auxfile, 0, SEEK_END);
	size = ftell(auxfile);
	fseek(auxfile, 0, SEEK_SET);
	filebuf = (uint8*)malloc(size);
	if (!filebuf) {
		fclose(auxfile);
		printf("ERROR: could not allocate %d bytes of memory\n", size);
		return AO_FAIL;
	}
	fread(filebuf, size, 1, auxfile);
	fclose(auxfile);
	*buffer = filebuf;
	*length = (uint64)size;
	return AO_SUCCESS;
}

void iPhoneDrv_AudioCallback(void *data, AudioQueueRef mQueue, AudioQueueBufferRef mBuffer) {
	ModizMusicPlayer *mplayer=(ModizMusicPlayer*)data;
    if ( !  mplayer.mQueueIsBeingStopped ) {
		[mplayer iPhoneDrv_Update:mBuffer];
    }
}

/************************************************/
/* Handle phone calls interruptions             */
/************************************************/
void interruptionListenerCallback (void *inUserData,UInt32 interruptionState ) {
	ModizMusicPlayer *mplayer=(ModizMusicPlayer*)inUserData;
	if (interruptionState == kAudioSessionBeginInterruption) {
		mInterruptShoudlRestart=0;
		if ([mplayer isPlaying] && (mplayer.bGlobalAudioPause==0)) {
			[mplayer Pause:YES];
			mInterruptShoudlRestart=1;
		}
	}
    else if (interruptionState == kAudioSessionEndInterruption) {
		// if the interruption was removed, and the app had been playing, resume playback
		if (mInterruptShoudlRestart) {
            //check if headphone is used?
            CFStringRef newRoute;
            UInt32 size = sizeof(CFStringRef);
            AudioSessionGetProperty(kAudioSessionProperty_AudioRoute,&size,&newRoute);
            if (newRoute) {
                if (CFStringCompare(newRoute,CFSTR("Headphone"),NULL)==kCFCompareEqualTo) {  //
                    mInterruptShoudlRestart=0;
                }
            }
            
			if (mInterruptShoudlRestart) [mplayer Pause:NO];
			mInterruptShoudlRestart=0;
		}
		
		UInt32 sessionCategory = kAudioSessionCategory_MediaPlayback;
		AudioSessionSetProperty (
								 kAudioSessionProperty_AudioCategory,
								 sizeof (sessionCategory),
								 &sessionCategory
								 );
		AudioSessionSetActive (true);
	}
}
/*************************************************/
/* Audio property listener                       */
/*************************************************/
void propertyListenerCallback (void                   *inUserData,                                 // 1
							   AudioSessionPropertyID inPropertyID,                                // 2
							   UInt32                 inPropertyValueSize,                         // 3
							   const void             *inPropertyValue ) {
	if (inPropertyID==kAudioSessionProperty_AudioRouteChange ) {
		ModizMusicPlayer *mplayer = (ModizMusicPlayer *) inUserData; // 6
		if ([mplayer isPlaying]) {
            UInt32 routeSize = sizeof (CFStringRef);
            CFStringRef route;
            
			CFDictionaryRef routeChangeDictionary = (CFDictionaryRef)inPropertyValue;        // 8
			NSString *oldroute = (NSString*)CFDictionaryGetValue (
																  routeChangeDictionary,
																  CFSTR (kAudioSession_AudioRouteChangeKey_OldRoute)
																  );
			/* Known values of route:
             * "Headset"
             * "Headphone"
             * "Speaker"
             * "SpeakerAndMicrophone"
             * "HeadphonesAndMicrophone"
             * "HeadsetInOut"
             * "ReceiverAndMicrophone"
             * "Lineout"
             */
            
            
            OSStatus error = AudioSessionGetProperty (kAudioSessionProperty_AudioRoute,
                                                      &routeSize,
                                                      &route);
            if (!error && (route != NULL)) {
                NSString *new_route = (NSString*)route;
                NSRange r;
                int pause_audio=1;
                r=[new_route rangeOfString:@"Head"];
                if (r.location!=NSNotFound) pause_audio=0;
                r.location=NSNotFound;
                r=[new_route rangeOfString:@"Line"];
                if (r.location!=NSNotFound) pause_audio=0;
                
                if (pause_audio) {  //New route is not headphone or lineout
                    if (([oldroute rangeOfString:@"Head"].location==NSNotFound) && ([oldroute rangeOfString:@"Line"].location==NSNotFound)) {
                        //old route was neither headphone or lineout
                        //do not pause audio
                        pause_audio=0;
                    }
                    if (pause_audio) [mplayer Pause:YES];
                }
                
            }
        }
    }
}

@implementation ModizMusicPlayer
@synthesize mod_subsongs,mod_currentsub,mod_minsub,mod_maxsub,mLoopMode;
@synthesize mPlayType; //1:GME, 2:libmodplug, 3:Adplug, 4:AO, 5:SexyPSF, 6:UADE
@synthesize mp_datasize;
@synthesize optForceMono;
//Adplug stuff
@synthesize adPlugPlayer,adplugDB;
@synthesize opl;
@synthesize opl_towrite;
@synthesize mADPLUGopltype;
//GME stuff
@synthesize gme_emu;
//SID
@synthesize optSIDoptim,mSidEngineType,mAskedSidEngineType;
//AO stuff
@synthesize ao_buffer;
@synthesize ao_info;

//Modplug stuff
@synthesize mp_settings;
@synthesize mp_file;
@synthesize mp_data;
@synthesize mVolume;
@synthesize numChannels,numPatterns,numSamples,numInstr,mPatternDataAvail;
@synthesize genRow,genPattern,/*genOffset,*/playRow,playPattern;//,playOffset;
@synthesize genVolData,playVolData;
//Player status
@synthesize bGlobalAudioPause;
//for spectrum analyzer
@synthesize buffer_ana_cpy;
@synthesize mAudioQueue;
@synthesize mBuffers;
@synthesize mQueueIsBeingStopped;

- (BOOL)addSkipBackupAttributeToItemAtPath:(NSString*)path
{
    //    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    //  NSString *documentsDirectory = [paths objectAtIndex:0];
    const char* filePath = [path fileSystemRepresentation];
    
    const char* attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    
    int result = setxattr(filePath, attrName, &attrValue, sizeof(attrValue), 0, 0);
    return result == 0;
}


//*****************************************
//Internal playback functions
-(api68_t*)setupSc68 {
	api68_init_t init68;
	
	NSBundle *bundle = [NSBundle mainBundle];
	NSString *path = [bundle bundlePath];
	NSMutableString *argData = [NSMutableString stringWithString:@"--sc68_data="];
	[argData appendString:path];
	[argData appendString:@"/Contents/Resources"];
	
	char *t = (char *)[argData UTF8String];
	char *args[] = { (char*)"sc68", t, NULL };
	
	memset(&init68, 0, sizeof(init68));
	init68.alloc = (void *(*)(unsigned int))malloc;
	init68.free = free;
	init68.argc = 2;
	init68.argv = args;
	
	api68_t* api = api68_init(&init68);
	
	return api;
}
-(id) initMusicPlayer {
	mFileMngr=[[NSFileManager alloc] init];
	
	if ((self = [super init])) {
		AudioSessionInitialize (
								NULL,
								NULL,
								interruptionListenerCallback,
								self
								);
		UInt32 sessionCategory = kAudioSessionCategory_MediaPlayback;
		AudioSessionSetProperty (
								 kAudioSessionProperty_AudioCategory,
								 sizeof (sessionCategory),
								 &sessionCategory
								 );
		
		//Check if still required or not
		Float32 preferredBufferDuration = SOUND_BUFFER_SIZE_SAMPLE*1.0f/PLAYBACK_FREQ;                      // 1
		AudioSessionSetProperty (                                     // 2
								 kAudioSessionProperty_PreferredHardwareIOBufferDuration,
								 sizeof (preferredBufferDuration),
								 &preferredBufferDuration
								 );
		AudioSessionPropertyID routeChangeID = kAudioSessionProperty_AudioRouteChange;    // 1
		AudioSessionAddPropertyListener (                                 // 2
										 routeChangeID,                                                 // 3
										 propertyListenerCallback,                                      // 4
										 self                                                       // 5
										 );
		AudioSessionSetActive (true);
		
		
		buffer_ana_flag=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
		buffer_ana=(short int**)malloc(SOUND_BUFFER_NB*sizeof(unsigned short int *));
		buffer_ana_cpy=(short int**)malloc(SOUND_BUFFER_NB*sizeof(unsigned short int *));
		buffer_ana_subofs=0;
		for (int i=0;i<SOUND_BUFFER_NB;i++) {
			buffer_ana[i]=(short int *)malloc(SOUND_BUFFER_SIZE_SAMPLE*2*2);
			buffer_ana_cpy[i]=(short int *)malloc(SOUND_BUFFER_SIZE_SAMPLE*2*2);
			buffer_ana_flag[i]=0;
		}
		
		//Global
		bGlobalShouldEnd=0;
		bGlobalSoundGenInProgress=0;
		optForceMono=0;
		mod_subsongs=0;
		mod_message_updated=0;
		bGlobalAudioPause=0;
		bGlobalIsPlaying=0;
		mVolume=1.0f;
		mLoopMode=0;
        
        mPanning=0;
        mPanningValue=64; //75%
		
		mdz_ArchiveFilesList=NULL;
        mdz_ArchiveFilesListAlias=NULL;
		mdz_ArchiveFilesCnt=0;
		mdz_IsArchive=0;
		mdz_currentArchiveIndex=0;
        //Timidity
        
		//
        //DUMB
        dumb_MastVol=1.0f;
        dumb_register_memfiles();
        duh=NULL; duh_player=NULL;
        //
        
		//MODPLUG specific
		mp_file=NULL;
		genPattern=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
		genRow=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
		//genOffset=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
		playPattern=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
		playRow=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        
        genVolData=(unsigned char*)malloc(SOUND_BUFFER_NB*64);
        playVolData=(unsigned char*)malloc(SOUND_BUFFER_NB*64);
		//playOffset=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
		//
		//GME specific
		optGMEFadeOut=1000;
        if (mSlowDevice) {
            gme_eq.treble = -14; // -50.0 = muffled, 0 = flat, +5.0 = extra-crisp
            gme_eq.bass   = 80;  // 1 = full bass, 90 = average, 16000 = almost no bass
            gme_fx.enabled=0;
            gme_fx.echo = 0.0f;
            gme_fx.surround = 0.0f;
            gme_fx.stereo = 0.0f;
            
        } else {
            gme_eq.treble = -14; // -50.0 = muffled, 0 = flat, +5.0 = extra-crisp
            gme_eq.bass   = 80;  // 1 = full bass, 90 = average, 16000 = almost no bass
            gme_fx.enabled=0;
            gme_fx.echo = 0.0f;
            gme_fx.surround = 0.0f;
            gme_fx.stereo = 0.8f;
        }
        
        //
        // ADPLUG specific
        mADPLUGopltype=0;
		//
		//UADE specific
		mUADE_OptLED=0;
		mUADE_OptNORM=0;
		mUADE_OptPOSTFX=1;
		mUADE_OptPAN=0;
		mUADE_OptPANValue=0.7f;
		mUADE_OptHEAD=0;
		mUADE_OptGAIN=0;
		mUADE_OptGAINValue=0.5f;
		mUADE_OptChange=0;
		uadeThread_running=0;
        timThread_running=0;
		
		// init  UADE stuff
		char uadeconfname[PATH_MAX];
		memset(&UADEstate, 0, sizeof UADEstate);
		memset(&UADEstatebase, 0, sizeof UADEstatebase);
		//load conf
		NSString *uade_path = [[NSBundle mainBundle] resourcePath];
		sprintf(UADEstatebase.config.basedir.name,"%s",[uade_path UTF8String]);
		UADEstatebase.config.basedir_set=1;
		if (uade_load_initial_config(uadeconfname,sizeof(uadeconfname),&UADEstate.config, &UADEstatebase.config)==0) {
			NSLog(@"Not able to load uade.conf from ~/.uade2/ or %s/.\n",UADEstate.config.basedir.name);
			//TODO : decide if continue or stop
		} else {
			//NSLog(@"Loaded configuration: %s\n", uadeconfname);
		}
		sprintf(UADEstate.config.basedir.name,"%s",[uade_path UTF8String]);
		sprintf(UADEconfigname, "%s/uaerc",UADEstate.config.basedir.name);
		sprintf(UADEscorename, "%s/score",UADEstate.config.basedir.name);
		
		//HVL specific
		mHVLinit=0;
		//
		// SIDPLAY
		// Init SID emu engine
        mSIDFilterON=1;
		mSid1EmuEngine=NULL;
		
		mSidEngineType=2;
		mSidEmuEngine=NULL;
		mBuilder=NULL;
		mSidTune=NULL;
		
		mSid1Tune=NULL;
		optSIDoptim=1;
        
        sid_forceModel=0;
        sid_forceClock=0;
		//
		// SC68
		sc68 = [self setupSc68];
		//
        
        
        // NVDSP
        //
        nvdsp_EQ=0;
        nvdsp_sr = PLAYBACK_FREQ;
        //clipping util
        nvdsp_CDT = [[NVClippingDetection alloc] init];
        
        // define center frequencies of the bands
        nvdsp_centerFrequencies[0] = 60.0f;
        nvdsp_centerFrequencies[1] = 170.0f;
        nvdsp_centerFrequencies[2] = 310.0f;
        nvdsp_centerFrequencies[3] = 600.0f;
        nvdsp_centerFrequencies[4] = 1000.0f;
        nvdsp_centerFrequencies[5] = 3000.0f;
        nvdsp_centerFrequencies[6] = 6000.0f;
        nvdsp_centerFrequencies[7] = 12000.0f;
        nvdsp_centerFrequencies[8] = 14000.0f;
        nvdsp_centerFrequencies[9] = 16000.0f;
        
        // define Q factor of the bands
        nvdsp_QFactor = 2.0f;
        
        // define initial gain
        nvdsp_initialGain = 0.0f;
        
        // init PeakingFilters
        // You'll later need to be able to set the gain for these (as the sliders change)
        // So define them somewhere global using NVPeakingEQFilter *PEQ[10];
        for (int i = 0; i < 10; i++) {
            nvdsp_PEQ[i] = [[NVPeakingEQFilter alloc] initWithSamplingRate:nvdsp_sr];
            nvdsp_PEQ[i].Q = nvdsp_QFactor;
            nvdsp_PEQ[i].centerFrequency = nvdsp_centerFrequencies[i];
            nvdsp_PEQ[i].G = nvdsp_initialGain;
        }
        
        //restore EQ settings
        [EQViewController restoreEQSettings];
        
		[self iPhoneDrv_Init];
		
		[NSThread detachNewThreadSelector:@selector(generateSoundThread) toTarget:self withObject:NULL];
	}
	return self;
}
-(void) dealloc {
	bGlobalShouldEnd=1;
	if (bGlobalIsPlaying) [self Stop];
	[self iPhoneDrv_Exit];
	for (int i=0;i<SOUND_BUFFER_NB;i++) {
		free(buffer_ana_cpy[i]);
		free(buffer_ana[i]);
	}
	free(buffer_ana_cpy);
	free(buffer_ana);
	free((void*)buffer_ana_flag);
	free(playRow);
	free(playPattern);
    free(playVolData);
	//free(playOffset);
	free(genRow);
	free(genPattern);
    free(genVolData);
	//free(genOffset);
    
    if (duh_player) {
		free(duh_player);duh_player=NULL;
    }
	if (duh) {
		unload_duh(duh); duh=NULL;
    }
    [mFileMngr release];
	
	[super dealloc];
}
-(BOOL) iPhoneDrv_Init {
    AudioStreamBasicDescription mDataFormat;
    UInt32 err;
    
    /* We force this format for iPhone */
    mDataFormat.mFormatID = kAudioFormatLinearPCM;
    mDataFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger |
	kAudioFormatFlagIsPacked;
	
	mDataFormat.mSampleRate = PLAYBACK_FREQ;
    
	mDataFormat.mBitsPerChannel = 16;
    
	mDataFormat.mChannelsPerFrame = 2;
    
    mDataFormat.mBytesPerFrame = (mDataFormat.mBitsPerChannel>>3) * mDataFormat.mChannelsPerFrame;
	
    mDataFormat.mFramesPerPacket = 1;
    mDataFormat.mBytesPerPacket = mDataFormat.mBytesPerFrame;
    
    /* Create an Audio Queue... */
    err = AudioQueueNewOutput( &mDataFormat,
							  iPhoneDrv_AudioCallback,
							  self,
							  NULL, //CFRunLoopGetCurrent(),
							  kCFRunLoopCommonModes,
							  0,
							  &mAudioQueue );
    
    /* ... and its associated buffers */
    mBuffers = (AudioQueueBufferRef*)malloc( sizeof(AudioQueueBufferRef) * SOUND_BUFFER_NB );
    for (int i=0; i<SOUND_BUFFER_NB; i++) {
		AudioQueueBufferRef mBuffer;
		err = AudioQueueAllocateBuffer( mAudioQueue, SOUND_BUFFER_SIZE_SAMPLE*2*2, &mBuffer );
		
		mBuffers[i]=mBuffer;
    }
    /* Set initial playback volume */
    err = AudioQueueSetParameter( mAudioQueue, kAudioQueueParam_Volume, mVolume );
    
	
	
    return 1;//VC_Init();
}
-(void) iPhoneDrv_Exit {
    AudioQueueDispose( mAudioQueue, true );
    free( mBuffers );
}
-(void) setIphoneVolume:(float) vol {
	UInt32 err;
	mVolume=vol;
    err = AudioQueueSetParameter( mAudioQueue, kAudioQueueParam_Volume, mVolume );
}
-(float) getIphoneVolume {
	UInt32 err;
    err = AudioQueueGetParameter( mAudioQueue, kAudioQueueParam_Volume, &mVolume );
	return mVolume;
}
-(BOOL) iPhoneDrv_PlayStart {
    UInt32 err;
    UInt32 i;
	
    AudioQueueStop( mAudioQueue, TRUE );
    /*
     * Enqueue all the allocated buffers before starting the playback.
     * The audio callback will be called as soon as one buffer becomes
     * available for filling.
     */
	
	mQueueIsBeingStopped=FALSE;
	
	buffer_ana_gen_ofs=buffer_ana_play_ofs=0;
	buffer_ana_subofs=0;
    //	genCurOffset=0;genCurOffsetCnt=0;
	for (int i=0;i<SOUND_BUFFER_NB;i++) {
		buffer_ana_flag[i]=0;
		playRow[i]=playPattern[i]=genRow[i]=genPattern[i]=0;//=genOffset[i]=playOffset[i]=0;
        memset(playVolData+i*64,0,64);
        memset(genVolData+i*64,0,64);
		memset(buffer_ana[i],0,SOUND_BUFFER_SIZE_SAMPLE*2*2);
		memset(buffer_ana_cpy[i],0,SOUND_BUFFER_SIZE_SAMPLE*2*2);
        memset(tim_notes[i],0,DEFAULT_VOICES*4);
        memset(tim_notes_cpy[i],0,DEFAULT_VOICES*4);
        tim_voicenb[i]=tim_voicenb_cpy[i]=0;
	}
    
	sampleVolume=0;
    for (i=0; i<SOUND_BUFFER_NB; i++) {
		memset(mBuffers[i]->mAudioData,0,SOUND_BUFFER_SIZE_SAMPLE*2*2);
		mBuffers[i]->mAudioDataByteSize = SOUND_BUFFER_SIZE_SAMPLE*2*2;
		AudioQueueEnqueueBuffer( mAudioQueue, mBuffers[i], 0, NULL);
		//		[self iPhoneDrv_FillAudioBuffer:mBuffers[i]];
		
    }
	bGlobalAudioPause=0;
    /* Start the Audio Queue! */
	//AudioQueuePrime( mAudioQueue, 0,NULL );
    err = AudioQueueStart( mAudioQueue, NULL );
	
    return 1;
}
-(BOOL) iPhoneDrv_LittlePlayStart {
    UInt32 err;
    
    bGlobalAudioPause=2;
    err = AudioQueueStart( mAudioQueue, NULL );
	
    return 1;
}

-(void) iPhoneDrv_PlayWaitStop {
	int counter=0;
    mQueueIsBeingStopped = TRUE;
	//	NSLog(@"stopping queue");
	bGlobalAudioPause=2;
	AudioQueueStop( mAudioQueue, FALSE );
	while ([self isEndReached]==NO) {
		[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
		counter++;
		if (counter*DEFAULT_WAIT_TIME_MS>2) break;
	}
	AudioQueueReset( mAudioQueue );
    mQueueIsBeingStopped = FALSE;
}
-(void) iPhoneDrv_PlayStop {
    mQueueIsBeingStopped = TRUE;
	//	NSLog(@"stopping queue");
	bGlobalAudioPause=2;
    AudioQueueStop( mAudioQueue, TRUE );
	AudioQueueReset( mAudioQueue );
    mQueueIsBeingStopped = FALSE;
}
-(void) iPhoneDrv_Update:(AudioQueueBufferRef) mBuffer {
    /* the real processing takes place in FillAudioBuffer */
	[self iPhoneDrv_FillAudioBuffer:mBuffer];
}
-(BOOL) iPhoneDrv_FillAudioBuffer:(AudioQueueBufferRef) mBuffer {
	int skip_queue=0;
	mBuffer->mAudioDataByteSize = SOUND_BUFFER_SIZE_SAMPLE*2*2;
	if (bGlobalAudioPause==2) {
		memset(mBuffer->mAudioData, 0, SOUND_BUFFER_SIZE_SAMPLE*2*2);
		if (bGlobalAudioPause==2) skip_queue=1;//return 0;  //End of song
    } else {
		//consume another buffer
		if (buffer_ana_flag[buffer_ana_play_ofs]) {
			bGlobalSoundHasStarted++;
			if (buffer_ana_flag[buffer_ana_play_ofs]&2) { //changed currentTime
				iCurrentTime=mNeedSeekTime;
				mNeedSeek=0;
				bGlobalSeekProgress=0;
			}
			
			if (mPatternDataAvail) {//Modplug
				playPattern[buffer_ana_play_ofs]=genPattern[buffer_ana_play_ofs];
				playRow[buffer_ana_play_ofs]=genRow[buffer_ana_play_ofs];
                memcpy(playVolData+buffer_ana_play_ofs*64,genVolData+buffer_ana_play_ofs*64,64);
                //				playOffset[buffer_ana_play_ofs]=genOffset[buffer_ana_play_ofs];
			}
            if (mPlayType==15) {//Timidity
                memcpy(tim_notes_cpy[buffer_ana_play_ofs],tim_notes[buffer_ana_play_ofs],DEFAULT_VOICES*4);
                tim_voicenb_cpy[buffer_ana_play_ofs]=tim_voicenb[buffer_ana_play_ofs];
            }
			
			iCurrentTime+=1000.0f*SOUND_BUFFER_SIZE_SAMPLE/PLAYBACK_FREQ;
			
			if (mPlayType==10) {//SC68
				iCurrentTime=api68_seek(sc68, -1,NULL);
			}
			
			/*	if ((iModuleLength>0)&&(iCurrentTime>iModuleLength)) {
			 if (mPlayType==8) {//SID does not track end, so use length
			 //iCurrentTime=0;
			 bGlobalAudioPause=2;
			 }
			 }*/
            
            /* Panning effect. Turns stereo into mono in a specific degree */
            if (mPanning) {
                int i, l, r, m;
                short int *sm=buffer_ana[buffer_ana_play_ofs];
                for (i = 0; i < SOUND_BUFFER_SIZE_SAMPLE; i += 1) {
                    l = 32768+sm[0];
                    r = 32768+sm[1];
                    m = (r - l) * mPanningValue;
                    sm[0] = (((l << 8) + m) >> 8)-32768;
                    sm[1] = (((r << 8) - m) >> 8)-32768;
                    sm += 2;
                }
            }
            
			
			if (sampleVolume<256) {
				if (optForceMono) {
					for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
						int val=((short int *)mBuffer->mAudioData)[i*2+1]=((buffer_ana[buffer_ana_play_ofs][i*2]+buffer_ana[buffer_ana_play_ofs][i*2+1])/2)*sampleVolume/256;
						((short int *)mBuffer->mAudioData)[i*2]=val;
						((short int *)mBuffer->mAudioData)[i*2+1]=val;
                        if (sampleVolume<256) sampleVolume++;
					}
				} else for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
					((short int *)mBuffer->mAudioData)[i*2]=buffer_ana[buffer_ana_play_ofs][i*2]*sampleVolume/256;
					((short int *)mBuffer->mAudioData)[i*2+1]=buffer_ana[buffer_ana_play_ofs][i*2+1]*sampleVolume/256;
                    if (sampleVolume<256) sampleVolume++;
				}
			}
			else {
				if (optForceMono) {
					for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
						int val=((short int *)mBuffer->mAudioData)[i*2+1]=((buffer_ana[buffer_ana_play_ofs][i*2]+buffer_ana[buffer_ana_play_ofs][i*2+1])/2)*sampleVolume/256;
						((short int *)mBuffer->mAudioData)[i*2]=val;
						((short int *)mBuffer->mAudioData)[i*2+1]=val;
					}
				} else memcpy((char*)mBuffer->mAudioData,buffer_ana[buffer_ana_play_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*2);
			}
			memcpy(buffer_ana_cpy[buffer_ana_play_ofs],buffer_ana[buffer_ana_play_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*2);
			
			if (buffer_ana_flag[buffer_ana_play_ofs]&4) { //end reached
				//iCurrentTime=0;
				bGlobalAudioPause=2;
			}
			
			if (buffer_ana_flag[buffer_ana_play_ofs]&8) { //end reached but continue to play
				iCurrentTime=0;
				iModuleLength=mNewModuleLength;
				mChangeOfSong=0;
				mod_message_updated=1;
			}
			
			
			buffer_ana_flag[buffer_ana_play_ofs]=0;
			buffer_ana_play_ofs++;
			if (buffer_ana_play_ofs==SOUND_BUFFER_NB) buffer_ana_play_ofs=0;
		} else {
			memset((char*)mBuffer->mAudioData,0,SOUND_BUFFER_SIZE_SAMPLE*2*2);  //WARNING : not fast enough!!
		}
	}
	if (!skip_queue) {
        if (nvdsp_EQ) {
        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
            nvdsp_outData[i*2]=(float)(((short int *)mBuffer->mAudioData)[i*2])/32768.0;
            nvdsp_outData[i*2+1]=(float)(((short int *)mBuffer->mAudioData)[i*2+1])/32768.0;
        }
        
        // apply the filter
        for (int i = 0; i < 10; i++) {
            [nvdsp_PEQ[i] filterData:nvdsp_outData numFrames:SOUND_BUFFER_SIZE_SAMPLE numChannels:2];
        }
        [nvdsp_CDT counterClipping:nvdsp_outData numFrames:SOUND_BUFFER_SIZE_SAMPLE numChannels:2];
        
        // measure output level
        //        outputLevelBuffer = [outPutWatcher getdBLevel:outData numFrames:numFrames numChannels:numChannels];
        
        
        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
            ((short int *)mBuffer->mAudioData)[i*2]=nvdsp_outData[i*2]*32767.0;
            ((short int *)mBuffer->mAudioData)[i*2+1]=nvdsp_outData[i*2+1]*32767.0;
        }
        
        }
        
        
		AudioQueueEnqueueBuffer( mAudioQueue, mBuffer, 0, NULL);
		if (bGlobalAudioPause==2) {
			AudioQueueStop( mAudioQueue, FALSE );
		}
	} else {
		buffer_ana_play_ofs++;
		if (buffer_ana_play_ofs==SOUND_BUFFER_NB) buffer_ana_play_ofs=0;
	}
    return 0;
}
-(int) getCurrentPlayedBufferIdx {
	return buffer_ana_play_ofs;
}
void mdx_update(unsigned char *data,int len,int end_reached) {
    if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
		mdx_stop();
		return;
	}
	
	while (buffer_ana_flag[buffer_ana_gen_ofs]) {
		[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
		if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
			mdx_stop();
			return;
		}
	}
	
	int to_fill=SOUND_BUFFER_SIZE_SAMPLE*2*2-buffer_ana_subofs;
	
	if (mSlowDevice) len=len*2;
	
	if (len<to_fill) {
		
		if (mSlowDevice) {
			signed int *dst=(signed int *)((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs);
			signed int *src=(signed int *)data;
			for (int i=len/8-1;i>=0;i--) {
				dst[i*2]=src[i];
				dst[i*2+1]=src[i];
			}
		} else memcpy( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)data,len);
		buffer_ana_subofs+=len;
	} else {
		
		if (mSlowDevice) {
			signed int *dst=(signed int *)((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs);
			signed int *src=(signed int *)data;
			for (int i=to_fill/8-1;i>=0;i--) {
				dst[i*2]=src[i];
				dst[i*2+1]=src[i];
			}
		} else memcpy((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)data,to_fill);
		
		len-=to_fill;
		buffer_ana_subofs=0;
		
		
		if (mNeedSeek==2) {
			mNeedSeek=3;
			buffer_ana_flag[buffer_ana_gen_ofs]=3;
 		} else buffer_ana_flag[buffer_ana_gen_ofs]=1;
		
		if (end_reached) {
			buffer_ana_flag[buffer_ana_gen_ofs]|=4;
		}
		
		if (mNeedSeek==1) { //ask for seeking
			mNeedSeek=2;  //taken into account
			//NSLog(@"sexy seek : %d",mNeedSeekTime);
			//sexy_seek(mNeedSeekTime);
			len=0;
		}
		
		buffer_ana_gen_ofs++;
		if (buffer_ana_gen_ofs==SOUND_BUFFER_NB) buffer_ana_gen_ofs=0;
		
		if (len>=SOUND_BUFFER_SIZE_SAMPLE*2*2) {
			NSLog(@"*****************\n*****************\n***************");
		} else if (len) {
			
			while (buffer_ana_flag[buffer_ana_gen_ofs]) {
				[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
				if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
					mdx_stop();
					return;
				}
			}
			if (mSlowDevice) {
				signed int *dst=(signed int *)buffer_ana[buffer_ana_gen_ofs];
				signed int *src=(signed int *)((char*)data+to_fill);
				for (int i=len/8-1;i>=0;i--) {
					dst[i*2]=src[i];
					dst[i*2+1]=src[i];
				}
			} else memcpy((char*)(buffer_ana[buffer_ana_gen_ofs]),((char*)data)+to_fill,len);
			buffer_ana_subofs=len;
		}
	}
}
void gsf_update(unsigned char* pSound,int lBytes) {
	if (soundQuality==2) lBytes*=2; //22Khz
	if (soundQuality==4) lBytes*=4; //11Khz
	
    if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
		g_playing=0;
		return;
	}
	while (buffer_ana_flag[buffer_ana_gen_ofs]) {
		[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
		if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
			g_playing=0;
			return;
		}
	}
	int to_fill=SOUND_BUFFER_SIZE_SAMPLE*2*2-buffer_ana_subofs;
	if (lBytes<to_fill) {
		if (soundQuality==1) {
			memcpy( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)pSound,lBytes);
		} else if (soundQuality==2) {
			signed int *dst=(signed int *)((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs);
			signed int *src=(signed int *)pSound;
			for (int i=0;i<lBytes/2/4;i++) {
				dst[i*2]=src[i];
				dst[i*2+1]=src[i];
			}
		} else {
			signed int *dst=(signed int *)((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs);
			signed int *src=(signed int *)pSound;
			for (int i=0;i<lBytes/4/4;i++) {
				dst[i*4]=src[i];
				dst[i*4+1]=src[i];
				dst[i*4+2]=src[i];
				dst[i*4+3]=src[i];
			}
		}
		buffer_ana_subofs+=lBytes;
		
	} else {
		if (soundQuality==1) {
			memcpy((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)pSound,to_fill);
		} else if (soundQuality==2) {
			signed int *dst=(signed int *)((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs);
			signed int *src=(signed int *)pSound;
			for (int i=0;i<to_fill/4/2;i++) {
				dst[i*2]=src[i];
				dst[i*2+1]=src[i];
			}
		} else {
			signed int *dst=(signed int *)((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs);
			signed int *src=(signed int *)pSound;
			for (int i=0;i<to_fill/4/4;i++) {
				dst[i*4]=src[i];
				dst[i*4+1]=src[i];
				dst[i*4+2]=src[i];
				dst[i*4+3]=src[i];
			}
		}
		lBytes-=to_fill;
		buffer_ana_subofs=0;
		
		buffer_ana_flag[buffer_ana_gen_ofs]=1;
        
        if ((mNeedSeek==2)&&(seek_needed==-1)) {
			mNeedSeek=3;
			buffer_ana_flag[buffer_ana_gen_ofs]|=2;
 		}
		
		buffer_ana_gen_ofs++;
		if (buffer_ana_gen_ofs==SOUND_BUFFER_NB) buffer_ana_gen_ofs=0;
		
		if (lBytes>=SOUND_BUFFER_SIZE_SAMPLE*2*2) {
			NSLog(@"*****************\n*****************\n***************");
		} else if (lBytes) {
			while (buffer_ana_flag[buffer_ana_gen_ofs]) {
				[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
				if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
					g_playing=0;
					return;
				}
			}
			if (soundQuality==1) {
				memcpy((char*)(buffer_ana[buffer_ana_gen_ofs]),((char*)pSound)+to_fill,lBytes);
			} else if (soundQuality==2) {
				signed int *dst=(signed int *)(char*)(buffer_ana[buffer_ana_gen_ofs]);
				signed int *src=(signed int *)(((char*)pSound)+to_fill/2);
				for (int i=0;i<lBytes/2/4;i++) {
					dst[i*2]=src[i];
					dst[i*2+1]=src[i];
				}
			} else {
				signed int *dst=(signed int *)(char*)(buffer_ana[buffer_ana_gen_ofs]);
				signed int *src=(signed int *)(((char*)pSound)+to_fill/4);
				for (int i=0;i<lBytes/4/4;i++) {
					dst[i*4]=src[i];
					dst[i*4+1]=src[i];
					dst[i*4+2]=src[i];
					dst[i*4+3]=src[i];
				}
			}
			buffer_ana_subofs=lBytes;
		}
	}
    
    if (mNeedSeek==1) {
        seek_needed=mNeedSeekTime;
        bGlobalSeekProgress=-1;
        mNeedSeek=2;
    }
}
int sexyd_updateseek(int progress) {  //called during seek. return 1 to stop play
	if (bGlobalShouldEnd||(!bGlobalIsPlaying)) return 1;
	bGlobalSeekProgress=progress;
	return 0;
}
void sexyd_update(unsigned char* pSound,long lBytes) {
    if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
		sexy_stop();
		return;
	}
	
	while (buffer_ana_flag[buffer_ana_gen_ofs]) {
		[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
		if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
			sexy_stop();
			return;
		}
	}
	
	int to_fill=SOUND_BUFFER_SIZE_SAMPLE*2*2-buffer_ana_subofs;
	
	if (lBytes<to_fill) {
		memcpy( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)pSound,lBytes);
		buffer_ana_subofs+=lBytes;
	} else {
		memcpy((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)pSound,to_fill);
		lBytes-=to_fill;
		buffer_ana_subofs=0;
		
		
		if (mNeedSeek==2) {
			mNeedSeek=3;
			buffer_ana_flag[buffer_ana_gen_ofs]=3;
 		} else buffer_ana_flag[buffer_ana_gen_ofs]=1;
		
		if (mNeedSeek==1) { //ask for seeking
			mNeedSeek=2;  //taken into account
			//NSLog(@"sexy seek : %d",mNeedSeekTime);
			sexy_seek(mNeedSeekTime);
			lBytes=0;
		}
		
		buffer_ana_gen_ofs++;
		if (buffer_ana_gen_ofs==SOUND_BUFFER_NB) buffer_ana_gen_ofs=0;
		
		if (lBytes>=SOUND_BUFFER_SIZE_SAMPLE*2*2) {
			//NSLog(@"*****************\n*****************\n***************");
		} else if (lBytes) {
			
			while (buffer_ana_flag[buffer_ana_gen_ofs]) {
				[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
				if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
					sexy_stop();
					return;
				}
			}
			memcpy((char*)(buffer_ana[buffer_ana_gen_ofs]),((char*)pSound)+to_fill,lBytes);
			buffer_ana_subofs=lBytes;
		}
	}
}

-(void)timThread {
    timThread_running=1;
    //    NSAutoreleasePool *pool =[[NSAutoreleasePool alloc] init];
    char *argv[1];
    argv[0]=tim_filepath;
    
    //timidity
    tim_init((char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"modizer.app/timidity"] UTF8String]);
    tim_init((char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"] UTF8String]);
    
    //tim_init((char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"modizer.app/timidity"] UTF8String]);
    tim_main(1, argv);
    //tim_close();
    //    [pool release];
    timThread_running=0;
}


// Handle main UADE thread (amiga emu)
-(void) uadeThread {
	//	printf("start thread\n");
	
	uadeThread_running=1;
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	int argc;
	char *argv[5];
	char *argv_buffer;
	
	if ([[NSThread currentThread] respondsToSelector:@selector(setThreadPriority)])	[[NSThread currentThread] setThreadPriority:SND_THREAD_PRIO];
	
	argc=5;
	argv_buffer=(char*)malloc(argc*32);
	for (int i=0;i<argc;i++) argv[i]=argv_buffer+32*i;
	strcpy(argv[0],"uadecore");
	strcpy(argv[1],"-i");
	strcpy(argv[2],"fd://0");
	strcpy(argv[3],"-o");
	strcpy(argv[4],"fd://0");
	uade_main(argc,(char**)argv);
	free(argv_buffer);
	
	//remove our pool and free the memory collected by it
    [pool release];
	
	uadeThread_running=0;
	//	printf("stop thread\n");
}
//invoked by secondary UADE thread, in charge of receiving sound data
int uade_audio_play(char *pSound,int lBytes,int song_end) {
	do {
		while (buffer_ana_flag[buffer_ana_gen_ofs]) {
			[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
			if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
				return 0;
			}
		}
		int to_fill=SOUND_BUFFER_SIZE_SAMPLE*2*2-buffer_ana_subofs;
		
		if (lBytes<to_fill) {
			memcpy( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)pSound,lBytes);
			buffer_ana_subofs+=lBytes;
			lBytes=0;
			if (song_end) {
				memset( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,0,SOUND_BUFFER_SIZE_SAMPLE*2*2-buffer_ana_subofs);
				buffer_ana_flag[buffer_ana_gen_ofs]=1|4;
				buffer_ana_gen_ofs++;
				if (buffer_ana_gen_ofs==SOUND_BUFFER_NB) buffer_ana_gen_ofs=0;
			}
		} else {
			memcpy((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)pSound,to_fill);
			lBytes-=to_fill;
			pSound+=to_fill;
			buffer_ana_subofs=0;
			
			buffer_ana_flag[buffer_ana_gen_ofs]=1;
			if (song_end) {
				buffer_ana_flag[buffer_ana_gen_ofs]=1|4;
			}
			
			buffer_ana_gen_ofs++;
			if (buffer_ana_gen_ofs==SOUND_BUFFER_NB) buffer_ana_gen_ofs=0;
		}
	} while (lBytes);
	
	
	return 0;
}
-(int) uade_playloop {
	struct uade_state *state=&UADEstate;
	uint16_t *sm;
	int i;
	int skip_first;
	uint32_t *u32ptr;
	char playerName[128];
	char formatName[128];
	char moduleName[128];
	
	uint8_t space[UADE_MAX_MESSAGE_SIZE];
	struct uade_msg *um = (struct uade_msg *) space;
	
	uint8_t sampledata[UADE_MAX_MESSAGE_SIZE];
	int left = 0;
	int what_was_left = 0;
	
	int subsong_end = 0;
	int next_song = 0;
	//	int ret;
	int tailbytes = 0;
	int playbytes;
	char *reason;
	
	int64_t subsong_bytes = 0;
	
	const int framesize = UADE_BYTES_PER_SAMPLE * UADE_CHANNELS;
	const int bytes_per_second = UADE_BYTES_PER_FRAME * state->config.frequency;
	
	enum uade_control_state controlstate = UADE_S_STATE;
	
	struct uade_ipc *ipc = &state->ipc;
	struct uade_song *us = state->song;
	struct uade_effect *ue = &state->effects;
	struct uade_config *uc = &state->config;
	
	uade_effect_reset_internals();
	
	uade_song_end_trigger=0;
	playerName[0]=0;
	formatName[0]=0;
	strcpy(moduleName,mod_name);
	skip_first=0;
	
	while (next_song == 0) {
		
		if (controlstate == UADE_S_STATE) {
			//handle option
			if (mUADE_OptChange) {
				switch (mUADE_OptChange) {
					case 1:
						uade_set_config_option(uc, UC_FORCE_LED, uc->led_state ? "off" : "on");
						//tprintf("\nForcing LED %s\n", (uc->led_state & 1) ? "ON" : "OFF");
						uade_send_filter_command(state);
						break;
					case 2:
						uade_effect_toggle(ue, UADE_EFFECT_NORMALISE);
						//tprintf("\nNormalise effect %s\n", uade_effect_is_enabled(ue, UADE_EFFECT_NORMALISE) ? "ON" : "OFF");
						break;
					case 3:
						uade_effect_toggle(ue, UADE_EFFECT_ALLOW);
						//tprintf("\nPostprocessing effects %s\n", uade_effect_is_enabled(ue, UADE_EFFECT_ALLOW) ? "ON" : "OFF");
						break;
					case 4:
						uade_effect_toggle(ue, UADE_EFFECT_PAN);
						//tprintf("\nPanning effect %s %s\n", uade_effect_is_enabled(ue, UADE_EFFECT_PAN) ? "ON" : "OFF", (uade_effect_is_enabled(ue, UADE_EFFECT_ALLOW) == 0 && uade_effect_is_enabled(ue, UADE_EFFECT_PAN) == 1) ? "(Remember to turn ON postprocessing!)" : "");
						break;
					case 5:
						uade_effect_toggle(ue, UADE_EFFECT_HEADPHONES);
						//tprintf("\nHeadphones effect %s %s\n", uade_effect_is_enabled(ue, UADE_EFFECT_HEADPHONES) ? "ON" : "OFF", (uade_effect_is_enabled(ue, UADE_EFFECT_ALLOW) == 0 && uade_effect_is_enabled(ue, UADE_EFFECT_HEADPHONES) == 1) ? "(Remember to turn ON postprocessing!)" : "");
						break;
					case 6:
						uade_effect_toggle(ue, UADE_EFFECT_GAIN);
						//tprintf("\nGain effect %s %s\n", uade_effect_is_enabled(ue, UADE_EFFECT_GAIN) ? "ON" : "OFF", (uade_effect_is_enabled(ue, UADE_EFFECT_ALLOW) == 0 && uade_effect_is_enabled(ue, UADE_EFFECT_GAIN)) ? "(Remember to turn ON postprocessing!)" : "");
						break;
				}
				mUADE_OptChange=0;
			}
			
			
			
			if (subsong_end && uade_song_end_trigger == 0) {
				if (uc->one_subsong == 0 && us->cur_subsong != -1 && us->max_subsong != -1) {
					if (moveToSubSong) {
						us->cur_subsong=moveToSubSongIndex;
					} else if (moveToPrevSubSong) {
						if (us->cur_subsong>us->min_subsong) us->cur_subsong--;
					} else us->cur_subsong++;
					
					if (us->cur_subsong > us->max_subsong) uade_song_end_trigger = 1;
					else {
						subsong_end = 0;
						subsong_bytes = 0;
						uade_change_subsong(state);
						mod_currentsub=us->cur_subsong;
						iCurrentTime=0;
						iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
						//Loop
						if (mLoopMode==1) iModuleLength=-1;
						mod_message_updated=1;
						
						if(moveToPrevSubSong||moveToNextSubSong||moveToSubSong) {
							mod_wantedcurrentsub=-1;
							what_was_left=0;
							tailbytes=0;
							if (moveToNextSubSong==2) {
								//[self iPhoneDrv_PlayWaitStop];
								//[self iPhoneDrv_PlayStart];
							} else {
								[self iPhoneDrv_PlayStop];
								[self iPhoneDrv_PlayStart];
							}
							skip_first=1;
						}
					}
					
					
					moveToPrevSubSong=0;
					moveToNextSubSong=0;
					moveToSubSong=0;
				} else {
					uade_song_end_trigger = 1;
				}
			}
			
			if ((us->cur_subsong != -1)&&(mod_wantedcurrentsub!=-1)&&(mod_wantedcurrentsub!=us->cur_subsong)) {
				subsong_end = 0;
				subsong_bytes = 0;
				us->cur_subsong=mod_wantedcurrentsub;
				mod_wantedcurrentsub=-1;
				if (us->cur_subsong>us->max_subsong) us->cur_subsong=us->max_subsong;
				if (us->cur_subsong<us->min_subsong) us->cur_subsong=us->min_subsong;
				mod_currentsub=us->cur_subsong;
				iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
				//Loop
				if (mLoopMode==1) iModuleLength=-1;
				mod_message_updated=1;
				uade_change_subsong(state);
				
			}
			
			if (uade_song_end_trigger) {
                // uade_song_end_trigger=0;
				next_song=1;
				//				printf("now exiting\n");
				if (uade_send_short_message(UADE_EXIT, ipc)) {
					printf("\nCan not send exit\n");
				}
				goto sendtoken;
			}
			
			left = uade_read_request(ipc);
			
		sendtoken:
			if (uade_send_short_message(UADE_COMMAND_TOKEN, ipc)) {
				printf("\nCan not send token\n");
				return 0;
			}
			
			controlstate = UADE_R_STATE;
			
			if (what_was_left||(subsong_end&&tailbytes)) {
				if (subsong_end) {
					/* We can only rely on 'tailbytes' amount which was determined
					 earlier when UADE_REPLY_SONG_END happened */
					playbytes = tailbytes;
					tailbytes = 0;
				} else {
					playbytes = what_was_left;
				}
				
				if (playbytes) {
					
					us->out_bytes += playbytes;
					subsong_bytes += playbytes;
					
					uade_effect_run(ue, (int16_t *) sampledata, playbytes / framesize);
					
					if (skip_first) skip_first=0;
					else {
						if (uade_audio_play((char*)sampledata, playbytes,subsong_end)) uade_song_end_trigger=2;
					}
				}
				
				/* FIX ME */
				if (uc->timeout != -1 && uc->use_timeouts) {
					if (uade_song_end_trigger == 0) {
						if (us->out_bytes / bytes_per_second >= uc->timeout) {
							printf("\nSong end (timeout %ds)\n", uc->timeout);
							uade_song_end_trigger = 1;
						}
					}
				}
				
				if (uc->subsong_timeout != -1 && uc->use_timeouts) {
					if (subsong_end == 0 && uade_song_end_trigger == 0) {
						if (subsong_bytes / bytes_per_second >= uc->subsong_timeout) {
							printf("\nSong end (subsong timeout %ds)\n", uc->subsong_timeout);
							subsong_end = 1;
						}
					}
				}
			}
			
		} else {
			if (moveToNextSubSong||moveToPrevSubSong||moveToSubSong) {
				subsong_end=1;
			}
			
			if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
				subsong_end=1;
			}
			
			/* receive state */
			if (uade_receive_message(um, sizeof(space), ipc) <= 0) {
				printf("\nCan not receive events from uade\n");
				return 0;
			}
			
			switch (um->msgtype) {
					
				case UADE_COMMAND_TOKEN:
					controlstate = UADE_S_STATE;
					break;
					
				case UADE_REPLY_DATA:
					sm = (uint16_t *) um->data;
					/*for (i = 0; i < um->size; i += 2) {
					 *sm = ntohs(*sm);
					 sm++;
					 }*/
					
					assert (left == um->size);
					assert (sizeof sampledata >= um->size);
					
					memcpy(sampledata, um->data, um->size);
					
					what_was_left = left;
					
					left = 0;
					break;
					
				case UADE_REPLY_FORMATNAME:
					uade_check_fix_string(um, 128);
					strcpy(formatName,(const char*)(um->data));
					if (1 + us->max_subsong - us->min_subsong==1) {
						sprintf(mod_message,"%s\n%s\n%s\nSubsong: 1",moduleName,formatName,playerName);
					}
					else {
						mod_minsub=us->min_subsong;
						mod_maxsub=us->max_subsong;
						mod_currentsub=us->cur_subsong;
						sprintf(mod_message,"%s\n%s\n%s\nSubsongs: %d",moduleName,formatName,playerName,1 + us->max_subsong - us->min_subsong);
						mod_subsongs=1 + us->max_subsong - us->min_subsong;
					}
					mod_message_updated=2;
					//printf("\nFormat name: %s\n", (uint8_t *) um->data);
					break;
					
				case UADE_REPLY_MODULENAME:
					uade_check_fix_string(um, 128);
					strcpy(moduleName,(const char*)(um->data));
					if (1 + us->max_subsong - us->min_subsong==1) {
						sprintf(mod_message,"%s\n%s\n%s\nSubsong: 1",moduleName,formatName,playerName);
					}
					else {
						mod_minsub=us->min_subsong;
						mod_maxsub=us->max_subsong;
						mod_currentsub=us->cur_subsong;
						sprintf(mod_message,"%s\n%s\n%s\nSubsongs: %d",moduleName,formatName,playerName,1 + us->max_subsong - us->min_subsong);
						mod_subsongs=1 + us->max_subsong - us->min_subsong;
					}
					mod_message_updated=2;
					//printf("\nModule name: %s\n", (uint8_t *) um->data);
					break;
					
				case UADE_REPLY_MSG:
					uade_check_fix_string(um, 512);
					//printf("UADE msg : %s\n",(const char*)(um->data));
					break;
					
				case UADE_REPLY_PLAYERNAME:
					uade_check_fix_string(um, 128);
					strcpy(playerName,(const char*)(um->data));
					if (1 + us->max_subsong - us->min_subsong==1) {
						sprintf(mod_message,"%s\n%s\n%s\nSubsong: 1",moduleName,formatName,playerName);
					}
					else {
						mod_minsub=us->min_subsong;
						mod_maxsub=us->max_subsong;
						mod_currentsub=us->cur_subsong;
						sprintf(mod_message,"%s\n%s\n%s\nSubsongs: %d",moduleName,formatName,playerName,1 + us->max_subsong - us->min_subsong);
						mod_subsongs=1 + us->max_subsong - us->min_subsong;
					}
					mod_message_updated=2;
					//printf("\nPlayer name: %s\n", (uint8_t *) um->data);
					break;
					
				case UADE_REPLY_SONG_END:
					if (um->size < 9) {
						printf("\nInvalid song end reply\n");
						exit(-1);
					}
					tailbytes = ntohl(((uint32_t *) um->data)[0]);
					if (!tailbytes)
						if (!what_was_left) what_was_left=2;
					/* next ntohl() is only there for a principle. it is not useful */
					if (ntohl(((uint32_t *) um->data)[1]) == 0) {
						/* normal happy song end. go to next subsong if any */
						subsong_end = 1;
						if (mSingleSubMode==0) moveToNextSubSong=2;
						
						//update song length
						[self setSongLengthfromMD5:mod_currentsub-mod_minsub+1 songlength:iCurrentTime];
						//printf("received happy song end %d\n",mod_wantedcurrentsub);
					} else {
						/* unhappy song end (error in the 68k side). skip to next song
						 ignoring possible subsongs */
						uade_song_end_trigger = 1;
						
						/* normal happy song end. go to next subsong if any */
						//subsong_end = 1;
						//moveToNextSubSong=2;
						
						[self setSongLengthfromMD5:mod_currentsub-mod_minsub+1 songlength:iCurrentTime];
						//printf("received unhappy song end %d\n",mod_wantedcurrentsub);
					}
					i = 0;
					reason = (char *) &um->data[8];
					while (reason[i] && i < (um->size - 8))
						i++;
					if (reason[i] != 0 || (i != (um->size - 9))) {
						printf("\nbroken reason string with song end notice\n");
						exit(-1);
					}
					//					printf("\nSong end (%s)\n", reason);
					break;
					
				case UADE_REPLY_SUBSONG_INFO:
					if (um->size != 12) {
						printf("\nsubsong info: too short a message\n");
						exit(-1);
					}
					
					u32ptr = (uint32_t *) um->data;
					us->min_subsong = ntohl(u32ptr[0]);
					us->max_subsong = ntohl(u32ptr[1]);
					us->cur_subsong = ntohl(u32ptr[2]);
					
					//printf("\nsubsong: %d from range [%d, %d]\n", us->cur_subsong, us->min_subsong, us->max_subsong);
					
					if (!(-1 <= us->min_subsong && us->min_subsong <= us->cur_subsong && us->cur_subsong <= us->max_subsong)) {
						int tempmin = us->min_subsong, tempmax = us->max_subsong;
						fprintf(stderr, "\nThe player is broken. Subsong info does not match.\n");
						us->min_subsong = tempmin <= tempmax ? tempmin : tempmax;
						us->max_subsong = tempmax >= tempmin ? tempmax : tempmin;
						if (us->cur_subsong > us->max_subsong)
							us->max_subsong = us->cur_subsong;
						else if (us->cur_subsong < us->min_subsong)
							us->min_subsong = us->cur_subsong;
					}
					
					if ((us->max_subsong - us->min_subsong) != 0) {
						//	printf("\nThere are %d subsongs in range [%d, %d].\n", 1 + us->max_subsong - us->min_subsong, us->min_subsong, us->max_subsong);
						if (1 + us->max_subsong - us->min_subsong==1) {
							sprintf(mod_message,"%s\n%s\n%s\nSubsong: 1",moduleName,formatName,playerName);
						}
						else {
							mod_minsub=us->min_subsong;
							mod_maxsub=us->max_subsong;
							mod_currentsub=us->cur_subsong;
							sprintf(mod_message,"%s\n%s\n%s\nSubsongs: %d",moduleName,formatName,playerName,1 + us->max_subsong - us->min_subsong);
							mod_subsongs=1 + us->max_subsong - us->min_subsong;
						}
						mod_message_updated=2;
					}
					
					//NSLog(@"playtime : %d",us->playtime);
					break;
					
				default:
					printf("\nExpected sound data. got %u.\n", (unsigned int) um->msgtype);
					return 0;
			}
		}
	}
	/*do {
	 ret = uade_receive_message(um, sizeof(space), ipc);
	 if (ret < 0) {
	 printf("\nCan not receive events (TOKEN) from uade.\n");
	 return 0;
	 }
	 if (ret == 0) {
	 printf("\nEnd of input after reboot.\n");
	 return 0;
	 }
	 } while (um->msgtype != UADE_COMMAND_TOKEN);*/
	return 0;
}

long src_callback(void *cb_data, float **data) {
    const char * result=usf_render(lzu_state->emu_state, lzu_sample_data, LZU_SAMPLE_SIZE, &lzu_sample_rate);
    if (result) return 0;
    src_short_to_float_array (lzu_sample_data, lzu_sample_data_float,LZU_SAMPLE_SIZE*2);
    *data=lzu_sample_data_float;
    return LZU_SAMPLE_SIZE;
}

-(void) generateSoundThread {
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	if ([[NSThread currentThread] respondsToSelector:@selector(setThreadPriority)]) [[NSThread currentThread] setThreadPriority:SND_THREAD_PRIO];
    
    
	while (1) {
		[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
		if (bGlobalIsPlaying) {
			bGlobalSoundGenInProgress=1;
			if ( !bGlobalEndReached && mPlayType) {
				int nbBytes=0;
                
                if (mPlayType==15) { //Special case : Timidity
                    int counter=0;
                    intr = 0;
                    [self timThread];
                    AudioQueueStop( mAudioQueue, FALSE );
					while ([self isEndReached]==NO) {
						[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
						counter++;
						if (counter*DEFAULT_WAIT_TIME_MS>2) break;
					}
					AudioQueueStop( mAudioQueue, TRUE );
					AudioQueueReset( mAudioQueue );
					mQueueIsBeingStopped = FALSE;
					bGlobalEndReached=1;
					bGlobalAudioPause=2;
                }
				
				if (mPlayType==12) {  //Special case : GSF
					int counter=0;
					[NSThread sleepForTimeInterval:0.1];  //TODO : check why it crashes in "release" target without this...
					gsf_loop();
					AudioQueueStop( mAudioQueue, FALSE );
					while ([self isEndReached]==NO) {
						[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
						counter++;
						if (counter*DEFAULT_WAIT_TIME_MS>2) break;
					}
					AudioQueueStop( mAudioQueue, TRUE );
					AudioQueueReset( mAudioQueue );
					mQueueIsBeingStopped = FALSE;
					bGlobalEndReached=1;
					bGlobalAudioPause=2;
				} else if (mPlayType==5) {  //Special case : SexyPSF
					int counter=0;
					[NSThread sleepForTimeInterval:0.1];  //TODO : check why it crashes in "release" target without this...
					sexy_execute();
					AudioQueueStop( mAudioQueue, FALSE );
					while ([self isEndReached]==NO) {
						[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
						counter++;
						if (counter*DEFAULT_WAIT_TIME_MS>2) break;
					}
					AudioQueueStop( mAudioQueue, TRUE );
					AudioQueueReset( mAudioQueue );
					mQueueIsBeingStopped = FALSE;
					bGlobalEndReached=1;
					bGlobalAudioPause=2;
					
					
					//bGlobalEndReached=1;
					//bGlobalAudioPause=2;
					//[self iPhoneDrv_PlayWaitStop];
					//AudioQueueStop( mAudioQueue, TRUE );
					
				} else if (mPlayType==6) {  //Special case : UADE
					int counter=0;
					[self uade_playloop];
					
					//NSLog(@"Waiting for end");
					while ([self isEndReached]==NO) {
						[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
						counter++;
						if (counter*DEFAULT_WAIT_TIME_MS>2) break;
					}
					AudioQueueStop( mAudioQueue, TRUE );
					AudioQueueReset( mAudioQueue );
					mQueueIsBeingStopped = FALSE;
					bGlobalEndReached=1;
					bGlobalAudioPause=2;
					//[self iPhoneDrv_PlayWaitStop];
					//AudioQueueStop( mAudioQueue, TRUE );
				} else if (mPlayType==11) {  //Special case : MDX
					int counter=0;
					mdx_play(mdx,pdx);
					
					//[self iPhoneDrv_PlayWaitStop];
					while ([self isEndReached]==NO) {
						[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
						counter++;
						if (counter*DEFAULT_WAIT_TIME_MS>2) break;
					}
					AudioQueueStop( mAudioQueue, TRUE );
					AudioQueueReset( mAudioQueue );
					mQueueIsBeingStopped = FALSE;
					bGlobalAudioPause=2;
					bGlobalEndReached=1;
				} else if (buffer_ana_flag[buffer_ana_gen_ofs]==0) {
					if (mNeedSeek==1) { //SEEK
						mNeedSeek=2;  //taken into account
						if (mPlayType==1) {   //GME
							bGlobalSeekProgress=-1;
							gme_seek(gme_emu,mNeedSeekTime);
							//gme_set_fade( gme_emu, iModuleLength-optGMEFadeOut,optGMEFadeOut ); //Fade 2s before end
						}
						if (mPlayType==2) { //MODPLUG
							bGlobalSeekProgress=-1;
							ModPlug_Seek(mp_file,mNeedSeekTime);
						}
						if (mPlayType==3) { //ADPLUG
							bGlobalSeekProgress=-1;
							adPlugPlayer->seek(mNeedSeekTime);
						}
						if (mPlayType==4) { //AOSDK : not supported
							mNeedSeek=0;
						}
						if (mPlayType==7) { //HVL
							bGlobalSeekProgress=-1;
							mNeedSeekTime=hvl_Seek(hvl_song,mNeedSeekTime);
						}
						if (mPlayType==8) { //SID : not supported
							mNeedSeek=0;
						}
						if (mPlayType==9) {//STSOUND
							if (ymMusicIsSeekable(ymMusic)==YMTRUE) {
								bGlobalSeekProgress=-1;
								ymMusicSeek(ymMusic,mNeedSeekTime);
							} else mNeedSeek=0;
						}
						if (mPlayType==10) {//SC68
							bGlobalSeekProgress=-1;
							api68_seek(sc68, mNeedSeekTime, NULL);
						}
						if (mPlayType==13) { //ASAP
							bGlobalSeekProgress=-1;
							ASAP_Seek(&asap, mNeedSeekTime);
						}
                        if (mPlayType==14) {
                            bGlobalSeekProgress=-1;
                            duh_end_sigrenderer(duh_player->dr);
                            duh_player->dr = duh_start_sigrenderer(duh, 0, 2/*nb channels*/, ((long long)mNeedSeekTime<<16)/1000);
                            if (!duh_player->dr) {
                                NSLog(@"big issue!!!!!!");
                            } else {
                                DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(duh_player->dr);
                                if (mLoopMode==1) dumb_it_set_loop_callback(itsr, &dumb_it_callback_loop, NULL);
                                else dumb_it_set_loop_callback(itsr, &dumb_it_callback_terminate, NULL);
                                dumb_it_set_xm_speed_zero_callback(itsr, &dumb_it_callback_terminate, NULL);
                            }
                        }
                        if (mPlayType==16) { //PMDMini : not supported
                            mNeedSeek=0;
                        }
                        if (mPlayType==17) { //VGM
                            SeekVGM(false,mNeedSeekTime*441/10);
                            mNeedSeek=0;
                        }
                        if (mPlayType==18) { //LAZYUSF : not supported
                            mNeedSeek=0;
                        }
					}
					if (moveToNextSubSong) {
						if (mod_currentsub<mod_maxsub) {
							mod_currentsub++;
							mod_message_updated=1;
							if (mPlayType==1) {//GME
								gme_start_track(gme_emu,mod_currentsub);
								sprintf(mod_name," %s",mod_filename);
								if (gme_track_info( gme_emu, &gme_info, mod_currentsub )==0) {
									strcpy(gmetype,gme_info->system);
									iModuleLength=gme_info->play_length;
									if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
									
									sprintf(mod_message,"Song:%s\nGame:%s\nAuthor:%s\nDumper:%s\nCopyright:%s\nTracks:%d\n%s",
                                            (gme_info->song?gme_info->song:" "),
                                            (gme_info->game?gme_info->game:" "),
                                            (gme_info->author?gme_info->author:" "),
                                            (gme_info->dumper?gme_info->dumper:" "),
                                            (gme_info->copyright?gme_info->copyright:" "),
                                            gme_track_count( gme_emu ),
                                            (gme_info->comment?gme_info->comment:" "));
									
									if (gme_info->song){
										if (gme_info->song[0]) sprintf(mod_name," %s/%s",(gme_info->game?gme_info->game:""),gme_info->song);
									}
									gme_free_info(gme_info);
								} else {
									strcpy(gmetype,"N/A");
									strcpy(mod_message,"N/A\n");
									iModuleLength=optGENDefaultLength;
								}
								//LOOP
								if (mLoopMode==1) iModuleLength=-1;
								
								mod_message_updated=2;
								
								if (iModuleLength>0) {
									if (iModuleLength>optGMEFadeOut) gme_set_fade( gme_emu, iModuleLength-optGMEFadeOut,optGMEFadeOut ); //Fade 1s before end
									else gme_set_fade( gme_emu, iModuleLength/2,iModuleLength/2 );
								} else gme_set_fade( gme_emu, 1<<30,optGMEFadeOut );
								if (moveToNextSubSong==2) {
									//[self iPhoneDrv_PlayWaitStop];
									//[self iPhoneDrv_PlayStart];
								} else {
									[self iPhoneDrv_PlayStop];
									[self iPhoneDrv_PlayStart];
								}
								iCurrentTime=0;
							}
							if (mPlayType==8) { //SID
								if (mSidEngineType==1) {
									sidEmuInitializeSong(*mSid1EmuEngine,*mSid1Tune, mod_currentsub);
								} else {
									mSidTune->selectSong(mod_currentsub);
									mSidEmuEngine->load(mSidTune);
								}
								
								if (moveToNextSubSong==2) {
									//[self iPhoneDrv_PlayWaitStop];
									//[self iPhoneDrv_PlayStart];
								} else {
									[self iPhoneDrv_PlayStop];
									[self iPhoneDrv_PlayStart];
								}
								iCurrentTime=0;
								iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
								if (mLoopMode) iModuleLength=-1;
								mod_message_updated=1;
							}
							if (mPlayType==10) {//SC68
								api68_music_info_t info;
								if (moveToNextSubSong==2) {
									//[self iPhoneDrv_PlayWaitStop];
									//[self iPhoneDrv_PlayStart];
								} else {
									[self iPhoneDrv_PlayStop];
									[self iPhoneDrv_PlayStart];
								}
								api68_play(sc68,mod_currentsub,1);
								api68_music_info( sc68, &info, mod_currentsub, NULL );
								iModuleLength=info.time_ms;
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
								if (mLoopMode) iModuleLength=-1;
								//NSLog(@"track : %d, time : %d, start : %d",mod_currentsub,info.time_ms,info.start_ms);
								iCurrentTime=0;
								mod_message_updated=1;
							}
                            if (mPlayType==13) {//ASAP
                                iModuleLength=asap.moduleInfo.durations[mod_currentsub];
                                ASAP_PlaySong(&asap, mod_currentsub, iModuleLength);
                                
                                
                                iCurrentTime=0;
                                numChannels=asap.moduleInfo.channels;
                                
                                if (moveToNextSubSong==2) {
                                    //[self iPhoneDrv_PlayWaitStop];
                                    //[self iPhoneDrv_PlayStart];
                                } else {
                                    [self iPhoneDrv_PlayStop];
                                    [self iPhoneDrv_PlayStart];
                                }
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                mod_message_updated=1;
                            }
						}
						moveToNextSubSong=0;
					}
					if (moveToSubSong) {
                        mod_currentsub=moveToSubSongIndex;
                        mod_message_updated=1;
                        if (mPlayType==1) {//GME
                            gme_start_track(gme_emu,mod_currentsub);
                            sprintf(mod_name," %s",mod_filename);
                            if (gme_track_info( gme_emu, &gme_info, mod_currentsub )==0) {
                                strcpy(gmetype,gme_info->system);
                                iModuleLength=gme_info->play_length;
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                
                                sprintf(mod_message,"Song:%s\nGame:%s\nAuthor:%s\nDumper:%s\nCopyright:%s\nTracks:%d\n%s",
                                        (gme_info->song?gme_info->song:" "),
                                        (gme_info->game?gme_info->game:" "),
                                        (gme_info->author?gme_info->author:" "),
                                        (gme_info->dumper?gme_info->dumper:" "),
                                        (gme_info->copyright?gme_info->copyright:" "),
                                        gme_track_count( gme_emu ),
                                        (gme_info->comment?gme_info->comment:" "));
                                
                                if (gme_info->song){
                                    if (gme_info->song[0]) sprintf(mod_name," %s/%s",(gme_info->game?gme_info->game:""),gme_info->song);
                                }
                                gme_free_info(gme_info);
                            } else {
                                strcpy(gmetype,"N/A");
                                strcpy(mod_message,"N/A\n");
                                iModuleLength=optGENDefaultLength;
                            }
                            //LOOP
                            if (mLoopMode==1) iModuleLength=-1;
                            
                            mod_message_updated=2;
                            
                            if (iModuleLength>0) {
                                if (iModuleLength>optGMEFadeOut) gme_set_fade( gme_emu, iModuleLength-optGMEFadeOut,optGMEFadeOut ); //Fade 1s before end
                                else gme_set_fade( gme_emu, iModuleLength/2, iModuleLength/2 );
                            } else gme_set_fade( gme_emu, 1<<30,optGMEFadeOut );
                            if (moveToSubSong==2) {
                                //[self iPhoneDrv_PlayWaitStop];
                                //[self iPhoneDrv_PlayStart];
                            } else {
                                [self iPhoneDrv_PlayStop];
                                [self iPhoneDrv_PlayStart];
                            }
                            iCurrentTime=0;
                        }
                        if (mPlayType==8) { //SID
                            if (mSidEngineType==1) {
                                sidEmuInitializeSong(*mSid1EmuEngine,*mSid1Tune, mod_currentsub);
                            } else {
                                mSidTune->selectSong(mod_currentsub);
                                mSidEmuEngine->load(mSidTune);
                            }
                            
                            if (moveToSubSong==2) {
                                //[self iPhoneDrv_PlayWaitStop];
                                //[self iPhoneDrv_PlayStart];
                            } else {
                                [self iPhoneDrv_PlayStop];
                                [self iPhoneDrv_PlayStart];
                            }
                            iCurrentTime=0;
                            iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
                            if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
                            if (mLoopMode) iModuleLength=-1;
                            mod_message_updated=1;
                        }
                        if (mPlayType==10) {//SC68
                            api68_music_info_t info;
                            if (moveToSubSong==2) {
                                //[self iPhoneDrv_PlayWaitStop];
                                //[self iPhoneDrv_PlayStart];
                            } else {
                                [self iPhoneDrv_PlayStop];
                                [self iPhoneDrv_PlayStart];
                            }
                            api68_play(sc68,mod_currentsub,1);
                            api68_music_info( sc68, &info, mod_currentsub, NULL );
                            iModuleLength=info.time_ms;
                            if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
                            if (mLoopMode) iModuleLength=-1;
                            //NSLog(@"track : %d, time : %d, start : %d",mod_currentsub,info.time_ms,info.start_ms);
                            iCurrentTime=0;
                            mod_message_updated=1;
                        }
                        if (mPlayType==13) {//ASAP
                            iModuleLength=asap.moduleInfo.durations[mod_currentsub];
                            ASAP_PlaySong(&asap, mod_currentsub, iModuleLength);
                            
                            
                            iCurrentTime=0;
                            numChannels=asap.moduleInfo.channels;
                            
                            if (moveToSubSong==2) {
                                //[self iPhoneDrv_PlayWaitStop];
                                //[self iPhoneDrv_PlayStart];
                            } else {
                                [self iPhoneDrv_PlayStop];
                                [self iPhoneDrv_PlayStart];
                            }
                            if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                            if (mLoopMode) iModuleLength=-1;
                            mod_message_updated=1;
                        }
						moveToSubSong=0;
					}
					if (moveToPrevSubSong) {
						moveToPrevSubSong=0;
						if (mod_currentsub>mod_minsub) {
							mod_currentsub--;
							mod_message_updated=1;
							if (mPlayType==1) {//GME
								gme_start_track(gme_emu,mod_currentsub);
								sprintf(mod_name," %s",mod_filename);
								if (gme_track_info( gme_emu, &gme_info, mod_currentsub )==0) {
									iModuleLength=gme_info->play_length;
									if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
									strcpy(gmetype,gme_info->system);
									sprintf(mod_message,"Song:%s\nGame:%s\nAuthor:%s\nDumper:%s\nCopyright:%s\nTracks:%d\n%s",
                                            (gme_info->song?gme_info->song:" "),
                                            (gme_info->game?gme_info->game:" "),
                                            (gme_info->author?gme_info->author:" "),
                                            (gme_info->dumper?gme_info->dumper:" "),
                                            (gme_info->copyright?gme_info->copyright:" "),
                                            gme_track_count( gme_emu ),
                                            (gme_info->comment?gme_info->comment:" "));
									
									if (gme_info->song){
										if (gme_info->song[0]) sprintf(mod_name," %s/%s",(gme_info->game?gme_info->game:""),gme_info->song);
									}
									
									
									gme_free_info(gme_info);
								} else {
									strcpy(gmetype,"N/A");
									strcpy(mod_message,"N/A\n");
									iModuleLength=optGENDefaultLength;
								}
								//LOOP
								if (mLoopMode==1) iModuleLength=-1;
								
								mod_message_updated=2;
								
								if (iModuleLength>0) {
									if (iModuleLength>optGMEFadeOut) gme_set_fade( gme_emu, iModuleLength-optGMEFadeOut,optGMEFadeOut ); //Fade 1s before end
									else gme_set_fade( gme_emu, iModuleLength/2,optGMEFadeOut); //Fade 1s before end
								} else gme_set_fade( gme_emu, 1<<30,optGMEFadeOut );
								[self iPhoneDrv_PlayStop];
								[self iPhoneDrv_PlayStart];
								iCurrentTime=0;
							}
							if (mPlayType==8) { //SID
								if (mSidEngineType==1) {
									sidEmuInitializeSong(*mSid1EmuEngine,*mSid1Tune, mod_currentsub);
								} else {
									mSidTune->selectSong(mod_currentsub);
									mSidEmuEngine->load(mSidTune);
								}
								
								[self iPhoneDrv_PlayStop];
								[self iPhoneDrv_PlayStart];
								iCurrentTime=0;
								iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
								if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
								if (mLoopMode) iModuleLength=-1;
								mod_message_updated=1;
							}
							if (mPlayType==10) {//SC68
								api68_music_info_t info;
								[self iPhoneDrv_PlayStop];
								[self iPhoneDrv_PlayStart];
								api68_play(sc68,mod_currentsub,1);
								api68_music_info( sc68, &info, mod_currentsub, NULL );
								iModuleLength=info.time_ms;
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
								if (mLoopMode) iModuleLength=-1;
								//NSLog(@"track : %d, time : %d, start : %d",mod_currentsub,info.time_ms,info.start_ms);
								iCurrentTime=0;
								mod_message_updated=1;
							}
                            if (mPlayType==13) {//ASAP
                                iModuleLength=asap.moduleInfo.durations[mod_currentsub];
                                ASAP_PlaySong(&asap, mod_currentsub, iModuleLength);
                                
                                
                                iCurrentTime=0;
                                numChannels=asap.moduleInfo.channels;
                                
                                [self iPhoneDrv_PlayStop];
                                [self iPhoneDrv_PlayStart];
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                
                                mod_message_updated=1;
                            }
						}
					}
					
					if (mPlayType==1) {  //GME
						if (gme_track_ended(gme_emu)) {
							//NSLog(@"Track ended : %d",iCurrentTime);
							if (mLoopMode==1) {
								gme_start_track(gme_emu,mod_currentsub);
								
								
								if (mSlowDevice) {
									gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE, buffer_ana[buffer_ana_gen_ofs] );
									/*									for (int i=SOUND_BUFFER_SIZE_SAMPLE/2-1;i>=0;i--) {
									 buffer_ana[buffer_ana_gen_ofs][i*4]=buffer_ana[buffer_ana_gen_ofs][i*4+2]=buffer_ana[buffer_ana_gen_ofs][i*2];
									 buffer_ana[buffer_ana_gen_ofs][i*4+1]=buffer_ana[buffer_ana_gen_ofs][i*4+3]=buffer_ana[buffer_ana_gen_ofs][i*2+1];
									 }*/
									signed int *dst=(signed int *)buffer_ana[buffer_ana_gen_ofs];
									signed int *src=(signed int *)buffer_ana[buffer_ana_gen_ofs];
									for (int i=SOUND_BUFFER_SIZE_SAMPLE/2-1;i>=0;i--) {
										dst[i*2]=src[i];
										dst[i*2+1]=src[i];
									}
									
									
								} else {
									gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE*2, buffer_ana[buffer_ana_gen_ofs] );
								}
								
								nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
							} else if (mChangeOfSong==0) {
								if ((mSingleSubMode==0)&&(mod_currentsub<mod_maxsub)) {
									//NSLog(@"time : %d song:%d",iCurrentTime,mod_currentsub);
                                    
									mod_currentsub++;
									gme_start_track(gme_emu,mod_currentsub);
									sprintf(mod_name," %s",mod_filename);
									if (gme_track_info( gme_emu, &gme_info, mod_currentsub )==0) {
										mChangeOfSong=1;
										mNewModuleLength=gme_info->play_length;
										if (mNewModuleLength<=0) mNewModuleLength=optGENDefaultLength;
										strcpy(gmetype,gme_info->system);
										
										sprintf(mod_message,"Song:%s\nGame:%s\nAuthor:%s\nDumper:%s\nCopyright:%s\nTracks:%d\n%s",
                                                (gme_info->song?gme_info->song:" "),
                                                (gme_info->game?gme_info->game:" "),
                                                (gme_info->author?gme_info->author:" "),
                                                (gme_info->dumper?gme_info->dumper:" "),
                                                (gme_info->copyright?gme_info->copyright:" "),
                                                gme_track_count( gme_emu ),
                                                (gme_info->comment?gme_info->comment:" "));
                                        
										if (gme_info->song){
											if (gme_info->song[0]) sprintf(mod_name," %s/%s",(gme_info->game?gme_info->game:""),gme_info->song);										}
										gme_free_info(gme_info);
									} else {
										strcpy(gmetype,"N/A");
										strcpy(mod_message,"N/A\n");
										mNewModuleLength=optGENDefaultLength;
									}
									//mod_message_updated=2;
									
									if (mNewModuleLength>0) {
										if (mNewModuleLength>optGMEFadeOut) gme_set_fade( gme_emu, mNewModuleLength-optGMEFadeOut,optGMEFadeOut ); //Fade 1s before end
										else gme_set_fade( gme_emu, mNewModuleLength/2, mNewModuleLength/2 ); //Fade 1s before end
									} else gme_set_fade( gme_emu, 1<<30,optGMEFadeOut );
									if (mSlowDevice) {
										gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE, buffer_ana[buffer_ana_gen_ofs] );
										/*for (int i=SOUND_BUFFER_SIZE_SAMPLE/2-1;i>=0;i--) {
										 buffer_ana[buffer_ana_gen_ofs][i*4]=buffer_ana[buffer_ana_gen_ofs][i*4+2]=buffer_ana[buffer_ana_gen_ofs][i*2];
										 buffer_ana[buffer_ana_gen_ofs][i*4+1]=buffer_ana[buffer_ana_gen_ofs][i*4+3]=buffer_ana[buffer_ana_gen_ofs][i*2+1];
										 }*/
										signed int *dst=(signed int *)buffer_ana[buffer_ana_gen_ofs];
										signed int *src=(signed int *)buffer_ana[buffer_ana_gen_ofs];
										for (int i=SOUND_BUFFER_SIZE_SAMPLE/2-1;i>=0;i--) {
											dst[i*2]=src[i];
											dst[i*2+1]=src[i];
										}
										
									} else {
										gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE*2, buffer_ana[buffer_ana_gen_ofs] );
									}
									
									nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
									
									//iCurrentTime=0;
									
								} else nbBytes=0;
							}
						}
						else {
							if (mSlowDevice) {
								gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE, buffer_ana[buffer_ana_gen_ofs] );
								/*for (int i=SOUND_BUFFER_SIZE_SAMPLE/2-1;i>=0;i--) {
								 buffer_ana[buffer_ana_gen_ofs][i*4]=buffer_ana[buffer_ana_gen_ofs][i*4+2]=buffer_ana[buffer_ana_gen_ofs][i*2];
								 buffer_ana[buffer_ana_gen_ofs][i*4+1]=buffer_ana[buffer_ana_gen_ofs][i*4+3]=buffer_ana[buffer_ana_gen_ofs][i*2+1];
								 }*/
								signed int *dst=(signed int *)buffer_ana[buffer_ana_gen_ofs];
								signed int *src=(signed int *)buffer_ana[buffer_ana_gen_ofs];
								for (int i=SOUND_BUFFER_SIZE_SAMPLE/2-1;i>=0;i--) {
									dst[i*2]=src[i];
									dst[i*2+1]=src[i];
								}
								
							} else {
								gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE*2, buffer_ana[buffer_ana_gen_ofs] );
							}
							nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
						}
					}
					if (mPlayType==4) { //AOSDK
						if ((*ao_types[ao_type].gen)((int16*)(buffer_ana[buffer_ana_gen_ofs]), SOUND_BUFFER_SIZE_SAMPLE)==AO_FAIL) {
							nbBytes=0;
							/*if ((*ao_types[ao_type].gen)((int16*)(&(buffer_ana[buffer_ana_gen_ofs][SOUND_BUFFER_SIZE_SAMPLE])), SOUND_BUFFER_SIZE_SAMPLE)==AO_FAIL) nbBytes=0;
							 else {
							 if (iCurrentTime<iModuleLength) nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
							 else nbBytes=0;
							 }*/
						} else {
							if ((iModuleLength==-1)||(iCurrentTime<iModuleLength)) nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
							else nbBytes=0;
						}
                        //                        nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
					}
					if (mPlayType==2) {  //MODPLUG
						int prev_ofs=buffer_ana_gen_ofs-1;
						if (prev_ofs<0) prev_ofs=SOUND_BUFFER_NB-1;
						genPattern[buffer_ana_gen_ofs]=ModPlug_GetCurrentPattern(mp_file);
						genRow[buffer_ana_gen_ofs]=ModPlug_GetCurrentRow(mp_file);
                        //NSLog(@"pat %d row %d",genPattern[buffer_ana_gen_ofs],genRow[buffer_ana_gen_ofs]);
                        /*						if ((genRow[buffer_ana_gen_ofs]!=genRow[prev_ofs])||(genPattern[buffer_ana_gen_ofs]!=genPattern[prev_ofs])) {
                         genCurOffset=0;
                         genCurOffsetCnt=0;
                         } else {
                         int tempo=ModPlug_GetCurrentTempo(mp_file);
                         int speed=ModPlug_GetCurrentSpeed(mp_file);
                         // compute length of current row
                         genCurOffsetCnt++;
                         genCurOffset=1000*genCurOffsetCnt*tempo/(3*25*speed);
                         }*/
                        //						genOffset[buffer_ana_gen_ofs]=genCurOffset;
						nbBytes = ModPlug_Read(mp_file,buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*2);
                        for (int i=0;i<numChannels;i++) {
                            int v=ModPlug_GetChannelVolume(mp_file,i);
                            genVolData[buffer_ana_gen_ofs*64+i]=(v>255?255:v);
                        }

					}
                    if (mPlayType==16) { //PMD
                        // render audio into sound buffer
                        // TODO does this work OK on mSlowDevices?
                        pmd_renderer(buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE);
                        nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                        // pmd_renderer gives no useful information on when song is done
                        // and will happily keep playing forever, so check song length against
                        // current playtime
                        if (iCurrentTime>=iModuleLength) {
                            AudioQueueStop( mAudioQueue, TRUE );
                            AudioQueueReset( mAudioQueue );
                            mQueueIsBeingStopped = FALSE;
                            bGlobalEndReached=1;
                            bGlobalAudioPause=2;
                        }
                    }
                    if (mPlayType==17) { //VGM
                        // render audio into sound buffer
                        // TODO does this work OK on mSlowDevices?
                        nbBytes=VGMFillBuffer((WAVE_16BS*)(buffer_ana[buffer_ana_gen_ofs]), SOUND_BUFFER_SIZE_SAMPLE)*2*2;
                    }
                    if (mPlayType==18) { //LAZYUSF
                        nbBytes=src_callback_read (src_state,src_ratio,SOUND_BUFFER_SIZE_SAMPLE, lzu_sample_converted_data_float)*2*2;
                        src_float_to_short_array (lzu_sample_converted_data_float,buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2) ;
//                        NSLog(@"nb: %d / %d",nbBytes,SOUND_BUFFER_SIZE_SAMPLE*2*2);
                        
                        if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                    }
					if (mPlayType==3) {  //ADPLUG
						if (opl_towrite) {
							int written=0;
							nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
							if (opl_towrite>=SOUND_BUFFER_SIZE_SAMPLE) {
								written=SOUND_BUFFER_SIZE_SAMPLE;
								opl->update((short int *)(buffer_ana[buffer_ana_gen_ofs]),SOUND_BUFFER_SIZE_SAMPLE);
								opl_towrite-=(SOUND_BUFFER_SIZE_SAMPLE);
							} else {
								written=opl_towrite;
								opl->update((short int *)(buffer_ana[buffer_ana_gen_ofs]),opl_towrite);
								opl_towrite=0;
							}
							if (!opl_towrite) {
								if (adPlugPlayer->update()) opl_towrite=(int)(PLAYBACK_FREQ*1.0f/adPlugPlayer->getrefresh());
								else {
									if (mLoopMode==1) {
										opl_towrite=(int)(PLAYBACK_FREQ*1.0f/adPlugPlayer->getrefresh());
										adPlugPlayer->seek(0);
									} else opl_towrite=0;
								}
								if ((written<SOUND_BUFFER_SIZE_SAMPLE)&&opl_towrite) {
									short int *dest=(short int *)(buffer_ana[buffer_ana_gen_ofs]);
									
									while ((written<SOUND_BUFFER_SIZE_SAMPLE)&&opl_towrite) {
										if (opl_towrite>(SOUND_BUFFER_SIZE_SAMPLE-written)) {
											opl->update(&dest[written*2],SOUND_BUFFER_SIZE_SAMPLE-written);
											opl_towrite-=SOUND_BUFFER_SIZE_SAMPLE-written;
											written=SOUND_BUFFER_SIZE_SAMPLE;
										} else {
											opl->update(&dest[written*2],opl_towrite);
											written+=opl_towrite;
											if (adPlugPlayer->update()) opl_towrite=(int)(PLAYBACK_FREQ*1.0f/adPlugPlayer->getrefresh());
											else {
												if (mLoopMode==1) {
													opl_towrite=(int)(PLAYBACK_FREQ*1.0f/adPlugPlayer->getrefresh());
													adPlugPlayer->seek(0);
												} else opl_towrite=0;
											}
										}
									}
								}
							}
						} else nbBytes=0;
					}
					if (mPlayType==7) {  //HVL
						if (hvl_sample_to_write) {
							int written=0;
							nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
							if (hvl_sample_to_write>=SOUND_BUFFER_SIZE_SAMPLE) {
								written=SOUND_BUFFER_SIZE_SAMPLE;
								hvl_mixchunk(hvl_song,SOUND_BUFFER_SIZE_SAMPLE,(int8*)(buffer_ana[buffer_ana_gen_ofs]),(int8*)(buffer_ana[buffer_ana_gen_ofs])+2,4);
								
								hvl_sample_to_write-=(SOUND_BUFFER_SIZE_SAMPLE);
							} else {
								written=hvl_sample_to_write;
								hvl_mixchunk(hvl_song,hvl_sample_to_write,(int8*)(buffer_ana[buffer_ana_gen_ofs]),(int8*)(buffer_ana[buffer_ana_gen_ofs])+2,4);
								
								hvl_sample_to_write=0;
							}
							if (!hvl_sample_to_write) {
								hvl_play_irq(hvl_song);
								if (hvl_song->ht_SongEndReached) {//end reached
									nbBytes=0;
								} else {
									hvl_sample_to_write=hvl_song->ht_Frequency/50/hvl_song->ht_SpeedMultiplier;
									
									if ((written<SOUND_BUFFER_SIZE_SAMPLE)&&hvl_sample_to_write) {
										int8 *dest=(int8 *)(buffer_ana[buffer_ana_gen_ofs]);
										
										while ((written<SOUND_BUFFER_SIZE_SAMPLE)&&hvl_sample_to_write) {
											if (hvl_sample_to_write>(SOUND_BUFFER_SIZE_SAMPLE-written)) {
												hvl_mixchunk(hvl_song,SOUND_BUFFER_SIZE_SAMPLE-written,&dest[written*4],&dest[written*4+2],4);
												hvl_sample_to_write-=SOUND_BUFFER_SIZE_SAMPLE-written;
												written=SOUND_BUFFER_SIZE_SAMPLE;
											} else {
												hvl_mixchunk(hvl_song,hvl_sample_to_write,&dest[written*4],&dest[written*4+2],4);
												written+=hvl_sample_to_write;
												hvl_play_irq(hvl_song);  //Check end ?
												if (hvl_song->ht_SongEndReached) {//end reached
													nbBytes=0;
													break;
												}
												hvl_sample_to_write=hvl_song->ht_Frequency/50/hvl_song->ht_SpeedMultiplier;
											}
										}
									}
								}
							}
						} else nbBytes=0;
						
					}
					if (mPlayType==8) { //SID
						if (mSidEngineType==1) {
							sidEmuFillBuffer(*mSid1EmuEngine,*mSid1Tune,buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*2);
							nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
						} else nbBytes=mSidEmuEngine->play(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*1)*2;
						if (mChangeOfSong==0) {
							if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (mLoopMode==0)&&(iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
								if ((mSingleSubMode==0)&&(mod_currentsub<mod_maxsub)) {
									nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
									mod_currentsub++;
									
									if (mSidEngineType==1) {
										sidEmuInitializeSong(*mSid1EmuEngine,*mSid1Tune, mod_currentsub);
									} else {
										mSidTune->selectSong(mod_currentsub);
										mSidEmuEngine->load(mSidTune);
									}
									
									mChangeOfSong=1;
									mNewModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
									if (mNewModuleLength<=0) mNewModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
									if (mLoopMode) mNewModuleLength=-1;
								} else {
									nbBytes=0;
								}
							}
						}
					}
					if (mPlayType==9) { //STSOUND
						int nbSample = SOUND_BUFFER_SIZE_SAMPLE;
						if (ymMusicComputeStereo((void*)ymMusic,(ymsample*)buffer_ana[buffer_ana_gen_ofs],nbSample)==YMTRUE) nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
						else nbBytes=0;
					}
					if (mPlayType==10) {//SC68
						nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
						int code = api68_process( sc68, buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE );
						if (code & API68_END) nbBytes=0;
						//if (code & API68_LOOP) nbBytes=0;
						//if (code & API68_CHANGE) nbBytes=0;
						
						if (mChangeOfSong==0) {
							if ((nbBytes==0)||( (iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
								if ((mSingleSubMode==0)&&(mod_currentsub<mod_maxsub)) {
									api68_music_info_t info;
									nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
									mod_currentsub++;
									api68_play(sc68,mod_currentsub,1);
									api68_music_info( sc68, &info, mod_currentsub, NULL );
									mChangeOfSong=1;
									mNewModuleLength=info.time_ms;
                                    if (mNewModuleLength<=0) mNewModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
									if (mLoopMode) mNewModuleLength=-1;
								} else nbBytes=0;
							}
						}
						if (code==API68_MIX_ERROR) nbBytes=0;
					}
					if (mPlayType==13) { //ASAP
						if (asap.moduleInfo.channels==2) {
							nbBytes= ASAP_Generate(&asap, (unsigned char *)buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE*2*2, ASAPSampleFormat_S16_L_E);
						} else {
							nbBytes= ASAP_Generate(&asap, (unsigned char *)buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE*2, ASAPSampleFormat_S16_L_E);
							short int *buff=(short int*)(buffer_ana[buffer_ana_gen_ofs]);
							for (int i=nbBytes*2-1;i>0;i--) {
								buff[i]=buff[i>>1];
							}
							nbBytes*=2;
						}
					}
                    if (mPlayType==14) {//DUMB
                        if (duh_player->dr) {
                            
                            if (mPatternDataAvail){
                                int prev_ofs=buffer_ana_gen_ofs-1;
                                if (prev_ofs<0) prev_ofs=SOUND_BUFFER_NB-1;
                                DUMB_IT_SIGRENDERER *duh_itsr=duh_get_it_sigrenderer(duh_player->dr);
                                
                                ModPlug_SeekOrder(mp_file,dumb_it_sr_get_current_order(duh_itsr));
                                genPattern[buffer_ana_gen_ofs]=ModPlug_GetCurrentOrder(mp_file);
                                //ModPlug_GetPatternOrder(mp_file, dumb_it_sr_get_current_order(duh_itsr)) ;
                                
                                genRow[buffer_ana_gen_ofs]=dumb_it_sr_get_current_row(duh_itsr);
                                
                                for (int i=0;i<numChannels;i++) {
                                    int v=dumb_it_sr_get_channel_volume(duh_itsr,i)*4;
                                    genVolData[buffer_ana_gen_ofs*64+i]=(v>255?255:v);
                                }

                                
                                //NSLog(@"pat %d row %d",genPattern[buffer_ana_gen_ofs],genRow[buffer_ana_gen_ofs]);
                            }
                            
                            long n = duh_render(duh_player->dr, 16/*bits_per_sample*/, 0/*bits_per_sample == 8 ? 1 : 0*/,
                                                duh_player->volume, 65536.0f / 44100, SOUND_BUFFER_SIZE_SAMPLE, buffer_ana[buffer_ana_gen_ofs]);
                            
                            nbBytes = n * (/*bits_per_sample*/16 >> 3) * 2/*stereo*/;
                        } else nbBytes=0;
                    }
					
					buffer_ana_flag[buffer_ana_gen_ofs]=1;
					if (mNeedSeek==2) {  //ask for a currentime update when this buffer will be played
						buffer_ana_flag[buffer_ana_gen_ofs]=buffer_ana_flag[buffer_ana_gen_ofs]|2;
						mNeedSeek=3;  //to avoid taking into account another time
					}
					
					if (nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2) {
                        short int *dest=buffer_ana[buffer_ana_gen_ofs];
                        short int lv,rv;
                        dest+=nbBytes/4;
                        if (nbBytes>=4) {
                            dest-=2;
                            lv=*dest++;
                            rv=*dest++;
                        } else {
                            lv=buffer_ana[(buffer_ana_gen_ofs+SOUND_BUFFER_NB-1)%SOUND_BUFFER_NB][SOUND_BUFFER_SIZE_SAMPLE-1];
                            rv=buffer_ana[(buffer_ana_gen_ofs+SOUND_BUFFER_NB-1)%SOUND_BUFFER_NB][SOUND_BUFFER_SIZE_SAMPLE-1];
                        }
                        for (int i=nbBytes/4;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                            *dest++=lv;
                            *dest++=rv;
                        }
						buffer_ana_flag[buffer_ana_gen_ofs]=buffer_ana_flag[buffer_ana_gen_ofs]|4; //end reached
						bGlobalEndReached=1;
					}
					if (mChangeOfSong==1) {
						buffer_ana_flag[buffer_ana_gen_ofs]=buffer_ana_flag[buffer_ana_gen_ofs]|8; //end reached but continue
						mChangeOfSong=2;
					}
					buffer_ana_gen_ofs++;
					if (buffer_ana_gen_ofs==SOUND_BUFFER_NB) buffer_ana_gen_ofs=0;
				}
			}
			bGlobalSoundGenInProgress=0;
		}
		if (bGlobalShouldEnd) break;
		
	}
	//Tell our callback what we've done
	//    [self performSelectorOnMainThread:@selector(loadComplete) withObject:fileName waitUntilDone:NO];
	
    //remove our pool and free the memory collected by it
    //    tim_close();
    [pool release];
}

//*****************************************
//Archive management
-(void) fex_extractToPath:(const char *)archivePath path:(const char *)extractPath {
	fex_type_t type;
	fex_t* fex;
	int arc_size;
	FILE *f;
	NSString *extractFilename,*extractPathFile;
	NSError *err;
	int idx;
	/* Determine file's type */
	if (fex_identify_file( &type, archivePath )) {
		NSLog(@"fex cannot determine type of %s",archivePath);
	}
	/* Only open files that fex can handle */
	if ( type != NULL ) {
		if (fex_open_type( &fex, archivePath, type )) {
			NSLog(@"cannot fex open : %s / type : %d",archivePath,type);
		} else{
			mdz_ArchiveFilesList=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*)); //TODO: free
            mdz_ArchiveFilesListAlias=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*)); //TODO: free
			idx=0;
			while ( !fex_done( fex ) ) {
                
                if ([self isAcceptedFile:[NSString stringWithFormat:@"%s",fex_name(fex)] no_aux_file:0]) {
                    
                    fex_stat(fex);
                    arc_size=fex_size(fex);
                    extractFilename=[NSString stringWithFormat:@"%s/%s",extractPath,fex_name(fex)];
                    extractPathFile=[extractFilename stringByDeletingLastPathComponent];
                    
                    //NSLog(@"file : %s, size : %dKo, output %@",fex_name(fex),arc_size/1024,extractFilename);
                    
                    
                    //1st create path if not existing yet
                    [mFileMngr createDirectoryAtPath:extractPathFile withIntermediateDirectories:TRUE attributes:nil error:&err];
                    [self addSkipBackupAttributeToItemAtPath:extractPathFile];
                    
                    //2nd extract file
                    f=fopen([extractFilename fileSystemRepresentation],"wb");
                    if (!f) {
                        NSLog(@"Cannot open %@ to extract %@",extractFilename,archivePath);
                    } else {
                        char *archive_data;
                        archive_data=(char*)malloc(32768); //32Ko buffer
                        while (arc_size) {
                            if (arc_size>32768) {
                                fex_read( fex, archive_data, 32768);
                                fwrite(archive_data,1,32768,f);
                                arc_size-=32768;
                            } else {
                                fex_read( fex, archive_data, arc_size );
                                fwrite(archive_data,1,arc_size,f);
                                arc_size=0;
                            }
                        }
                        free(archive_data);
                        fclose(f);
                        
                        NSString *tmp_filename=[NSString stringWithFormat:@"%s",fex_name(fex)];
                        
                        if ([self isAcceptedFile:[NSString stringWithFormat:@"%s",fex_name(fex)] no_aux_file:1]) {
                            mdz_ArchiveFilesList[idx]=(char*)malloc(strlen([tmp_filename fileSystemRepresentation])+1);
                            strcpy(mdz_ArchiveFilesList[idx],[tmp_filename fileSystemRepresentation]);
                            
                            NSRange r;
                            r.location=NSNotFound;
                            r = [tmp_filename rangeOfString:@".spc" options:NSCaseInsensitiveSearch];
                            if (r.location != NSNotFound) {
                                //if spc file, replace name by song name read in the file
                                //NSLog(@"found an spc: %@",tmp_filename);
                                archive_data=(char*)malloc(128);
                                f=fopen([extractFilename fileSystemRepresentation],"rb");
                                fread(archive_data,1,128,f);
                                fclose(f);
                                
                                //                                if (archive_data[0x23]==0x26) {
                                mdz_ArchiveFilesListAlias[idx]=(char*)malloc(32+1+4);
                                memcpy(mdz_ArchiveFilesListAlias[idx]+4,archive_data+0x2E,32);
                                mdz_ArchiveFilesListAlias[idx][32+4]=0;
                                mdz_ArchiveFilesListAlias[idx][0]=idx/100+'0';
                                mdz_ArchiveFilesListAlias[idx][1]=((idx/10)%10)+'0';
                                mdz_ArchiveFilesListAlias[idx][2]=((idx/1)%10)+'0';
                                mdz_ArchiveFilesListAlias[idx][3]='-';
                                /*                                } else {
                                 mdz_ArchiveFilesListAlias[idx]=(char*)malloc(strlen([tmp_filename fileSystemRepresentation])+1);
                                 strcpy(mdz_ArchiveFilesListAlias[idx],[tmp_filename fileSystemRepresentation]);
                                 }*/
                                //                                NSLog(@"name: %s",mdz_ArchiveFilesListAlias[idx]);
                                free(archive_data);
                            } else {
                                mdz_ArchiveFilesListAlias[idx]=(char*)malloc(strlen([[tmp_filename lastPathComponent] UTF8String])+1);
                                strcpy(mdz_ArchiveFilesListAlias[idx],[[tmp_filename lastPathComponent] UTF8String]);
                                //                                NSLog(@"name def: %s",mdz_ArchiveFilesListAlias[idx]);
                            }
                            
                            
                            idx++;
                        }
                        if (fex_next( fex )) {
                            NSLog(@"Error during fex scanning");
                            break;
                        }
                    }
                } else {
                    if (fex_next( fex )) {
                        NSLog(@"Error during fex scanning");
                        break;
                    }
                }
            }
			fex_close( fex );
		}
		fex = NULL;
	} else {
		//NSLog( @"Skipping unsupported archive: %s\n", archivePath );
	}
}

-(void) fex_extractSingleFileToPath:(const char *)archivePath path:(const char *)extractPath file_index:(int)index {
	fex_type_t type;
	fex_t* fex;
	int arc_size;
	FILE *f;
	NSString *extractFilename,*extractPathFile;
	NSError *err;
	int idx;
	/* Determine file's type */
	if (fex_identify_file( &type, archivePath )) {
		NSLog(@"fex cannot determine type of %s",archivePath);
	}
	/* Only open files that fex can handle */
	if ( type != NULL ) {
		if (fex_open_type( &fex, archivePath, type )) {
			NSLog(@"cannot fex open : %s / type : %d",archivePath,type);
		} else{
			mdz_ArchiveFilesList=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*)); //TODO: free
            mdz_ArchiveFilesListAlias=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*)); //TODO: free
			idx=0;
			while ( !fex_done( fex ) ) {
                
                if ([self isAcceptedFile:[NSString stringWithFormat:@"%s",fex_name(fex)] no_aux_file:1]) {
                    if (index==idx) {
                        fex_stat(fex);
                        arc_size=fex_size(fex);
                        extractFilename=[NSString stringWithFormat:@"%s/%s",extractPath,fex_name(fex)];
                        extractPathFile=[extractFilename stringByDeletingLastPathComponent];
                        //NSLog(@"file : %s, size : %dKo, output %@",fex_name(fex),arc_size/1024,extractFilename);
                        
                        
                        //1st create path if not existing yet
                        [mFileMngr createDirectoryAtPath:extractPathFile withIntermediateDirectories:TRUE attributes:nil error:&err];
                        [self addSkipBackupAttributeToItemAtPath:extractPathFile];
                        //2nd extract file
                        f=fopen([extractFilename fileSystemRepresentation],"wb");
                        if (!f) {
                            NSLog(@"Cannot open %@ to extract %@",extractFilename,archivePath);
                        } else {
                            char *archive_data;
                            archive_data=(char*)malloc(32768); //32Ko buffer
                            while (arc_size) {
                                if (arc_size>32768) {
                                    fex_read( fex, archive_data, 32768);
                                    fwrite(archive_data,1,32768,f);
                                    arc_size-=32768;
                                } else {
                                    fex_read( fex, archive_data, arc_size );
                                    fwrite(archive_data,1,arc_size,f);
                                    arc_size=0;
                                }
                            }
                            free(archive_data);
                            fclose(f);
                            NSString *tmp_filename=[NSString stringWithFormat:@"%s",fex_name(fex)];
                            
                            mdz_ArchiveFilesList[0]=(char*)malloc(strlen([tmp_filename fileSystemRepresentation])+1);
                            strcpy(mdz_ArchiveFilesList[0],[tmp_filename fileSystemRepresentation]);
                            mdz_ArchiveFilesListAlias[0]=(char*)malloc(strlen([[tmp_filename lastPathComponent] UTF8String])+1);
                            strcpy(mdz_ArchiveFilesListAlias[0],[[tmp_filename lastPathComponent] UTF8String]);
                            break;
                        }
                    } else idx++;
                    if (fex_next( fex )) {
                        NSLog(@"Error during fex scanning");
                        break;
                    }
                } else {
                    if (fex_next( fex )) {
                        NSLog(@"Error during fex scanning");
                        break;
                    }
                }
            }
			fex_close( fex );
		}
		fex = NULL;
	} else {
		//NSLog( @"Skipping unsupported archive: %s\n", archivePath );
	}
}

-(void) fex_scanarchive:(const char *)path {
	fex_type_t type;
	fex_t* fex;
	/* Determine file's type */
	if (fex_identify_file( &type, path )) {
		NSLog(@"fex cannot determine type of %s",path);
	}
	/* Only open files that fex can handle */
	if ( type != NULL ) {
		if (fex_open_type( &fex, path, type )) {
			NSLog(@"cannot fex open : %s / type : %d",path,type);
		} else{
			mdz_IsArchive=1;
			mdz_ArchiveFilesCnt=0;
			while ( !fex_done( fex ) ) {
                if ([self isAcceptedFile:[NSString stringWithFormat:@"%s",fex_name(fex)] no_aux_file:1]) {
                    mdz_ArchiveFilesCnt++;
                    //NSLog(@"file : %s",fex_name(fex));
                }
				if (fex_next( fex )) {
					NSLog(@"Error during fex scanning");
					break;
				}
			}
			fex_close( fex );
		}
		fex = NULL;
	} else {
		//NSLog( @"Skipping unsupported archive: %s\n", path );
	}
}

-(NSString*) fex_getfilename:(const char *)path index:(int)idx {
	fex_type_t type;
	fex_t* fex;
	/* Determine file's type */
	if (fex_identify_file( &type, path )) {
		NSLog(@"fex cannot determine type of %s",path);
	}
	/* Only open files that fex can handle */
	if ( type != NULL ) {
		if (fex_open_type( &fex, path, type )) {
			NSLog(@"cannot fex open : %s / type : %d",path,type);
		} else{
			while ( !fex_done( fex ) ) {
                if ([self isAcceptedFile:[NSString stringWithFormat:@"%s",fex_name(fex)] no_aux_file:1]) {
                    if (!idx) return [NSString stringWithFormat:@"%s",fex_name(fex)];
                    idx--;
                }
				if (fex_next( fex )) {
					NSLog(@"Error during fex scanning");
					break;
				}
			}
			fex_close( fex );
		}
		fex = NULL;
	} else {
		//NSLog( @"Skipping unsupported archive: %s\n", path );
	}
    return nil;
}
//*****************************************
//File loading & general functions
-(int) getSongLengthfromMD5:(int)track_nb {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int err;
	int songlength=-1;
	
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		
		sprintf(sqlStatement,"SELECT song_length FROM songlength WHERE id_md5=\"%s\" AND track_nb=%d",song_md5,track_nb);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				songlength=sqlite3_column_int(stmt, 0)*1000;
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d getSongLengthfromMD51",err);
		
	};
	sqlite3_close(db);
	
	if (songlength==-1) {
		//Try in user DB
		pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
		
		if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
			char sqlStatement[1024];
			sqlite3_stmt *stmt;
			
			sprintf(sqlStatement,"SELECT song_length FROM songlength_user WHERE id_md5=\"%s\" AND track_nb=%d",song_md5,track_nb);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					songlength=sqlite3_column_int(stmt, 0)*1000;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d getSongLengthfromMD52",err);
			
		};
		sqlite3_close(db);
	}
	
	pthread_mutex_unlock(&db_mutex);
	return songlength;
}
-(void) setSongLengthfromMD5:(int)track_nb songlength:(int)slength {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
		sprintf(sqlStatement,"DELETE FROM songlength_user WHERE id_md5=\"%s\" AND track_nb=%d",song_md5,track_nb);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d setSongLengthfromMD51",err);
		
		sprintf(sqlStatement,"INSERT INTO songlength_user (id_md5,track_nb,song_length) VALUES (\"%s\",%d,%d)",song_md5,track_nb,slength/1000);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d setSongLengthfromMD52",err);
		
	};
	sqlite3_close(db);
	
	pthread_mutex_unlock(&db_mutex);
}
-(void) getStilInfo:(char*)fullPath {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int err;
	
	strcpy(stil_info,"");
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		char tmppath[256];
		sqlite3_stmt *stmt;
		char *realPath=strstr(fullPath,"/HVSC");
		
		if (!realPath) {
			//try to find realPath with md5
			sprintf(sqlStatement,"SELECT filepath FROM hvsc_path WHERE id_md5=\"%s\"",song_md5);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					strcpy(tmppath,(const char*)sqlite3_column_text(stmt, 0));
					realPath=tmppath;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		} else realPath+=5;
		if (realPath) {
			sprintf(sqlStatement,"SELECT stil_info FROM stil WHERE fullpath=\"%s\"",realPath);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					strcpy(stil_info,(const char*)sqlite3_column_text(stmt, 0));
					while ((realPath=strstr(stil_info,"\\n"))) {
						*realPath='\n';
						realPath++;
						memmove(realPath,realPath+1,strlen(realPath));
					}
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	
	pthread_mutex_unlock(&db_mutex);
}

-(void) getStilAsmaInfo:(char*)fullPath {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int err;
	
	strcpy(stil_info,"");
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		char tmppath[256];
		sqlite3_stmt *stmt;
		char *realPath=strstr(fullPath,"/ASMA");
		
		if (!realPath) {
			//try to find realPath with md5
			sprintf(sqlStatement,"SELECT filepath FROM asma_path WHERE id_md5=\"%s\"",song_md5);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					strcpy(tmppath,(const char*)sqlite3_column_text(stmt, 0));
					realPath=tmppath;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		} else realPath+=5;
		if (realPath) {
			sprintf(sqlStatement,"SELECT stil_info FROM stil_asma WHERE fullpath=\"%s\"",realPath);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					strcpy(stil_info,(const char*)sqlite3_column_text(stmt, 0));
					while ((realPath=strstr(stil_info,"\\n"))) {
						*realPath='\n';
						realPath++;
						memmove(realPath,realPath+1,strlen(realPath));
					}
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	
	pthread_mutex_unlock(&db_mutex);
}


-(void) scanForPlayableFile:(NSString*)pathToScan {
	NSString *file;
	NSDirectoryEnumerator *dirEnum;
	NSDictionary *fileAttributes;
	NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
	NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
	NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
	NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
	NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
	NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
	NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_MODPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extDUMB=[SUPPORTED_FILETYPE_DUMB componentsSeparatedByString:@","];
	NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
	NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
	NSArray *filetype_extSEXYPSF=[SUPPORTED_FILETYPE_SEXYPSF componentsSeparatedByString:@","];
    NSArray *filetype_extLAZYUSF=[SUPPORTED_FILETYPE_LAZYUSF componentsSeparatedByString:@","];
	NSArray *filetype_extAOSDK=[SUPPORTED_FILETYPE_AOSDK componentsSeparatedByString:@","];
	NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
	NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
	NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
	NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
	NSMutableArray *filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extMDX count]+[filetype_extSID count]+
                                  [filetype_extSTSOUND count]+[filetype_extPMD count]+
								  [filetype_extSC68 count]+[filetype_extARCHIVE count]+[filetype_extUADE count]+[filetype_extMODPLUG count]+[filetype_extDUMB count]+
								  [filetype_extGME count]+[filetype_extADPLUG count]+[filetype_extSEXYPSF count]+[filetype_extLAZYUSF count]+
								  [filetype_extAOSDK count]+[filetype_extHVL count]+[filetype_extGSF count]+
								  [filetype_extASAP count]+[filetype_extWMIDI count]+[filetype_extVGM count]];
	
	int err;
	int local_nb_entries=0;
	
	NSRange r;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	
	[filetype_ext addObjectsFromArray:filetype_extMDX];
    [filetype_ext addObjectsFromArray:filetype_extPMD];
	[filetype_ext addObjectsFromArray:filetype_extSID];
	[filetype_ext addObjectsFromArray:filetype_extSTSOUND];
	[filetype_ext addObjectsFromArray:filetype_extSC68];
	[filetype_ext addObjectsFromArray:filetype_extARCHIVE];
	[filetype_ext addObjectsFromArray:filetype_extUADE];
	[filetype_ext addObjectsFromArray:filetype_extMODPLUG];
    [filetype_ext addObjectsFromArray:filetype_extDUMB];
	[filetype_ext addObjectsFromArray:filetype_extGME];
	[filetype_ext addObjectsFromArray:filetype_extADPLUG];
	[filetype_ext addObjectsFromArray:filetype_extSEXYPSF];
    [filetype_ext addObjectsFromArray:filetype_extLAZYUSF];
	[filetype_ext addObjectsFromArray:filetype_extAOSDK];
	[filetype_ext addObjectsFromArray:filetype_extHVL];
	[filetype_ext addObjectsFromArray:filetype_extGSF];
	[filetype_ext addObjectsFromArray:filetype_extASAP];
    [filetype_ext addObjectsFromArray:filetype_extVGM];
	[filetype_ext addObjectsFromArray:filetype_extWMIDI];
	
	// First check count for each section
	dirEnum = [mFileMngr enumeratorAtPath:pathToScan];
	while (file = [dirEnum nextObject]) {
		fileAttributes=[dirEnum fileAttributes];
		if ([fileAttributes objectForKey:NSFileType]==NSFileTypeDirectory) {
            //Dir : do nothing
		}
		if ([fileAttributes objectForKey:NSFileType]==NSFileTypeRegular) {
            //File : check if playable (looking at ext)
			NSString *extension = [[file pathExtension] uppercaseString];
			NSString *file_no_ext = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
			int found=0;
            
			if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
			else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
            
			if (found)  {
				local_nb_entries++;
            }
		}
	}
	
	if (local_nb_entries) {
        mdz_ArchiveFilesCnt=local_nb_entries;
        mdz_currentArchiveIndex=0;
        mdz_ArchiveFilesList=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*));
        mdz_ArchiveFilesListAlias=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*));
        
		// Second check count for each section
		dirEnum = [mFileMngr enumeratorAtPath:pathToScan];
		while (file = [dirEnum nextObject]) {
			fileAttributes=[dirEnum fileAttributes];
			if ([fileAttributes objectForKey:NSFileType]==NSFileTypeDirectory) {
				//dir : do nothing
			}
			if ([fileAttributes objectForKey:NSFileType]==NSFileTypeRegular) {
                //File : add to the list if playable
				NSString *extension = [[file pathExtension] uppercaseString];
				NSString *file_no_ext = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
				
				int found=0;
                
				if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
				else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                
                
				if (found)  {
                    mdz_ArchiveFilesList[mdz_currentArchiveIndex]=(char*)malloc([file length]+1);
                    strcpy(mdz_ArchiveFilesList[mdz_currentArchiveIndex],[file UTF8String]);
                    
                    mdz_ArchiveFilesListAlias[mdz_currentArchiveIndex]=(char*)malloc([file length]+1);
                    strcpy(mdz_ArchiveFilesListAlias[mdz_currentArchiveIndex],[file UTF8String]);
                    
                    mdz_currentArchiveIndex++;
				}
			}
		}
	}
    mdz_currentArchiveIndex=0;
	return;
}

-(int) isAcceptedFile:(NSString*)_filePath no_aux_file:(int)no_aux_file {
	NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@"."];
	NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
	NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
	NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
	NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
	NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_MODPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extDUMB=[SUPPORTED_FILETYPE_DUMB componentsSeparatedByString:@","];
	NSArray *filetype_extGME=(no_aux_file?[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_GME_EXT componentsSeparatedByString:@","]);
	NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
	NSArray *filetype_extSEXYPSF=(no_aux_file?[SUPPORTED_FILETYPE_SEXYPSF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_SEXYPSF_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extLAZYUSF=(no_aux_file?[SUPPORTED_FILETYPE_LAZYUSF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_LAZYUSF_EXT componentsSeparatedByString:@","]);
	NSArray *filetype_extAOSDK=(no_aux_file?[SUPPORTED_FILETYPE_AOSDK componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_AOSDK_EXT componentsSeparatedByString:@","]);
	NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
	NSArray *filetype_extGSF=(no_aux_file?[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_GSF_EXT componentsSeparatedByString:@","]);
	NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
	NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    NSArray *filetype_extCOVER=[SUPPORTED_FILETYPE_COVER componentsSeparatedByString:@","];
	
	NSString *extension;
    NSString *file_no_ext;
	NSString *filePath;
	int found=0;
    
    extension = [_filePath pathExtension];
    file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
    filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
    
    //    NSLog(@"check: %@",_filePath);
    mSingleFileType=1; //used to identify file which relies or not on another file (sample, psflib, ...)
	
    
	if (!found)
		for (int i=0;i<[filetype_extVGM count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=17;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=17;break;}
		}
    if (!found)
        for (int i=0;i<[filetype_extASAP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=13;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=13;break;}
        }
	if (!found)
		for (int i=0;i<[filetype_extGME count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_GME_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=1;break;
            }
			if ([file_no_ext caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_GME_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=1;break;
            }
		}
	if (!found)
		for (int i=0;i<[filetype_extSID count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=8;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=8;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extMDX count];i++) { //PDX might be required
			if ([extension caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
                mSingleFileType=0;
                found=11;break;
            }
			if ([file_no_ext caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
                mSingleFileType=0;
                found=11;break;
            }
		}
    if (!found)
		for (int i=0;i<[filetype_extPMD count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=16;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=16;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extADPLUG count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=3;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=3;break;}
		}
	
	if (!found)
		for (int i=0;i<[filetype_extSTSOUND count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=9;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=9;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extSC68 count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=10;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=10;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extSEXYPSF count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSEXYPSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_SEXYPSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=5;break;
            }
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSEXYPSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_SEXYPSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=5;break;
            }
		}
    if (!found)
        for (int i=0;i<[filetype_extLAZYUSF count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extLAZYUSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_LAZYUSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=18;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extLAZYUSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_LAZYUSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=18;break;
            }
        }
	if (!found)
		for (int i=0;i<[filetype_extAOSDK count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extAOSDK objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_AOSDK_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=4;break;
            }
			if ([file_no_ext caseInsensitiveCompare:[filetype_extAOSDK objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_AOSDK_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=4;break;
            }
		}
	if (!found)
		for (int i=0;i<[filetype_extGSF count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_GSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=12;break;
            }
			if ([file_no_ext caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_GSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=12;break;
            }
		}
    //tmp hack => redirect to timidity
	if (!found)
		for (int i=0;i<[filetype_extWMIDI count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=15;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=15;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extUADE count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {
                //check if require second file
                NSArray *singlefile=[SUPPORTED_FILETYPE_UADE_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                
                found=6;break;
            }
			if ([file_no_ext caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {
                //check if require second file
                NSArray *singlefile=[SUPPORTED_FILETYPE_UADE_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=6;break;
            }
		}
	if ((!found)||(mdz_defaultMODPLAYER==DEFAULT_MODPLUG))
		for (int i=0;i<[filetype_extMODPLUG count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=2;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=2;break;}
		}
	if ((!found)||(mdz_defaultMODPLAYER==DEFAULT_DUMB))
        for (int i=0;i<[filetype_extDUMB count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=14;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=14;break;}
        }
    if (!found) {
        for (int i=0;i<[filetype_extHVL count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=7;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=7;break;}
        }
    }
    
    //    NSLog(@"found: %d / single: %d",found,mSingleFileType);
    
    if (!no_aux_file) {
        if (!found)
            for (int i=0;i<[filetype_extCOVER count];i++) {
                if ([extension caseInsensitiveCompare:[filetype_extCOVER objectAtIndex:i]]==NSOrderedSame)    {found=99;break;}
                if ([file_no_ext caseInsensitiveCompare:[filetype_extCOVER objectAtIndex:i]]==NSOrderedSame) {found=99;break;}
                
            }
    }
    
    return found;
}

-(int) LoadModule:(NSString*)_filePath defaultMODPLAYER:(int)defaultMODPLAYER defaultSAPPLAYER:(int)defaultSAPPLAYER defaultVGMPLAYER:(int)defaultVGMPLAYER slowDevice:(int)slowDevice archiveMode:(int)archiveMode archiveIndex:(int)archiveIndex singleSubMode:(int)singleSubMode singleArcMode:(int)singleArcMode {
	NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
    NSArray *filetype_extLHA_ARCHIVE=[SUPPORTED_FILETYPE_LHA_ARCHIVE componentsSeparatedByString:@","];
	NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
	NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
	NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
	NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
	NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
	NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_MODPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extDUMB=[SUPPORTED_FILETYPE_DUMB componentsSeparatedByString:@","];
	NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
	NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
	NSArray *filetype_extSEXYPSF=[SUPPORTED_FILETYPE_SEXYPSF componentsSeparatedByString:@","];
    NSArray *filetype_extLAZYUSF=[SUPPORTED_FILETYPE_LAZYUSF componentsSeparatedByString:@","];
	NSArray *filetype_extAOSDK=[SUPPORTED_FILETYPE_AOSDK componentsSeparatedByString:@","];
	NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
	NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
	NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
	NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
	
	NSString *extension;
    NSString *file_no_ext;
	NSString *filePath;
	int found=0;
    
    mplayer_error_msg[0]=0;
	mSingleSubMode=singleSubMode;
    
    [self iPhoneDrv_LittlePlayStart];
	
	if (archiveMode==0) {
		extension = [_filePath pathExtension];
		file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
		filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
		
        mdz_IsArchive=0;
		mdz_defaultMODPLAYER=defaultMODPLAYER;
        mdz_defaultSAPPLAYER=defaultSAPPLAYER;
        mdz_defaultVGMPLAYER=defaultVGMPLAYER;
		mNeedSeek=0;
		mod_message_updated=0;
		mod_subsongs=1;
		mod_currentsub=mod_minsub=mod_maxsub=0;
		mod_wantedcurrentsub=-1;
		moveToPrevSubSong=0;
		moveToNextSubSong=0;
		moveToSubSong=0;
		moveToSubSongIndex=0;
		mSidEngineType=mAskedSidEngineType;
		iModuleLength=0;
        mPatternDataAvail=0;
		sprintf(mod_filename,"%s",[[[filePath lastPathComponent] stringByDeletingPathExtension] UTF8String]);
        
		
		mSlowDevice=slowDevice;
		
		if (mdz_ArchiveFilesCnt) {
			for (int i=0;i<mdz_ArchiveFilesCnt;i++) free(mdz_ArchiveFilesList[i]);
			free(mdz_ArchiveFilesList);
			mdz_ArchiveFilesList=NULL;
            for (int i=0;i<mdz_ArchiveFilesCnt;i++) free(mdz_ArchiveFilesListAlias[i]);
			free(mdz_ArchiveFilesListAlias);
			mdz_ArchiveFilesListAlias=NULL;
			mdz_ArchiveFilesCnt=0;
		}
		
		for (int i=0;i<[filetype_extARCHIVE count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extARCHIVE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extARCHIVE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
        for (int i=0;i<[filetype_extLHA_ARCHIVE count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extLHA_ARCHIVE objectAtIndex:i]]==NSOrderedSame) {found=2;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extLHA_ARCHIVE objectAtIndex:i]]==NSOrderedSame) {found=2;break;}
		}
		if (found) { //archived file
			mdz_IsArchive=0;
			mdz_ArchiveFilesCnt=0;
            
            strcpy(archive_filename,mod_filename);
            
			if (found==1) { //FEX
                [self fex_scanarchive:[filePath UTF8String]];
                if (mdz_ArchiveFilesCnt) {
                    FILE *f;
                    f = fopen([filePath UTF8String], "rb");
                    if (f == NULL) {
                        NSLog(@"Cannot open file %@",filePath);
                        return -1;
                    }
                    fseek(f,0L,SEEK_END);
                    mp_datasize=ftell(f);
                    fclose(f);
                    
                    //remove tmp dir
                    NSError *err;
                    NSString *tmpArchivePath=[NSString stringWithFormat:@"%@/tmp/tmpArchive",NSHomeDirectory()];
                    [mFileMngr removeItemAtPath:tmpArchivePath error:&err];
                    
                    //extract to tmp dir
                    [mFileMngr createDirectoryAtPath:tmpArchivePath withIntermediateDirectories:TRUE attributes:nil error:&err];
                    [self addSkipBackupAttributeToItemAtPath:tmpArchivePath];
                    
                    if (found==1) { //FEX
                        
                        //update singlefiletype flag
                        //[self isAcceptedFile:[NSString stringWithFormat:@"Documents/tmpArchive/%s",mdz_ArchiveFilesList[mdz_currentArchiveIndex] no_aux_file:1];
                        if (singleArcMode&&(archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)){
                            NSString *tmpstr=[self fex_getfilename:[filePath UTF8String] index:archiveIndex];
                            //NSLog(@"yo:%@",tmpstr);
                        }
                        
                        if (mSingleFileType&&singleArcMode&&(archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) {
                            mdz_ArchiveFilesCnt=1;
                            [self fex_extractSingleFileToPath :[filePath UTF8String] path:[tmpArchivePath UTF8String] file_index:archiveIndex];
                        } else {
                            
                            [self fex_extractToPath:[filePath UTF8String] path:[tmpArchivePath UTF8String]];
                        }
                    } else {
                    }
                    mdz_currentArchiveIndex=0;
                    if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) mdz_currentArchiveIndex=archiveIndex;
                    _filePath=[NSString stringWithFormat:@"tmp/tmpArchive/%s",mdz_ArchiveFilesList[mdz_currentArchiveIndex]];
                    extension = [_filePath pathExtension];
                    file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
                    filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
                    sprintf(mod_filename,"%s/%s",archive_filename,[[filePath lastPathComponent] UTF8String]);
                    
                    //NSLog(@"%@",_filePath);
                } else return -1;
			} else { //LHA
                int argc;
                char *argv[7];
                char argv_buffer[2048];
                
                //NSLog(@"opening %@",filePath);
                
                FILE *f;
				f = fopen([filePath UTF8String], "rb");
				if (f == NULL) {
					NSLog(@"Cannot open file %@",filePath);
					return -1;
				}
				fseek(f,0L,SEEK_END);
				mp_datasize=ftell(f);
				fclose(f);
				
				//remove tmp dir
				NSError *err;
				NSString *tmpArchivePath=[NSString stringWithFormat:@"%@/tmp/tmpArchive",NSHomeDirectory()];
				[mFileMngr removeItemAtPath:tmpArchivePath error:&err];
				
				//extract to tmp dir
                [mFileMngr createDirectoryAtPath:tmpArchivePath withIntermediateDirectories:TRUE attributes:nil error:&err];
                [self addSkipBackupAttributeToItemAtPath:tmpArchivePath];
                
                //NSLog(@"opening %@",tmpArchivePath);
                
                argc=6;
                /*argv=(char**)malloc(argc*sizeof(char*));
                 argv_buffer=(char*)malloc(4+2+3+3+[tmpArchivePath length]+4+[filePath length]+1);*/
                memset(argv_buffer,0,2048);
                
                
                argv[0]=argv_buffer;
                sprintf(argv[0],"lha");
                argv[1]=argv_buffer+4;
                sprintf(argv[1],"x");
                argv[2]=argv_buffer+4+2;
                sprintf(argv[2],"-f");
                argv[3]=argv_buffer+4+2+3;
                sprintf(argv[3],"-q");
                argv[4]=argv_buffer+4+2+3+3;
                sprintf(argv[4],"-w=%s",[tmpArchivePath UTF8String]);
                argv[5]=argv_buffer+4+2+3+3+[tmpArchivePath length]+4;
                sprintf(argv[5],"%s",[filePath UTF8String]);
                argv[6]=0;
                
                lha_main(argc,argv);
                /*free(argv_buffer);
                 free(argv);*/
                
                //now check how many files we can play
                [self scanForPlayableFile:tmpArchivePath];
                
                if (mdz_ArchiveFilesCnt) {
                    mdz_IsArchive=1;
                    if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) mdz_currentArchiveIndex=archiveIndex;
                    _filePath=[NSString stringWithFormat:@"tmp/tmpArchive/%s",mdz_ArchiveFilesList[mdz_currentArchiveIndex]];
                    extension = [_filePath pathExtension];
                    file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
                    filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
                    sprintf(mod_filename,"%s/%s",archive_filename,[[filePath lastPathComponent] UTF8String]);
                } else {
                    return -1;
                }
				
				//NSLog(@"%@",_filePath);
            }
		}
	} else {//archiveMode
		mNeedSeek=0;
		mod_message_updated=0;
		mod_subsongs=1;
		mod_currentsub=mod_minsub=mod_maxsub=0;
		mod_wantedcurrentsub=-1;
		moveToPrevSubSong=0;
		moveToNextSubSong=0;
		moveToSubSong=0;
		moveToSubSongIndex=0;
		mSidEngineType=mAskedSidEngineType;
		iModuleLength=0;
        mPatternDataAvail=0;
        
        if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) mdz_currentArchiveIndex=archiveIndex;
		
        _filePath=[NSString stringWithFormat:@"tmp/tmpArchive/%s",mdz_ArchiveFilesList[mdz_currentArchiveIndex]];
        extension = [_filePath pathExtension];
        file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
        filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
        sprintf(mod_filename,"%s/%s",archive_filename,[[filePath lastPathComponent] UTF8String]);
	}
    
    if (archiveIndex&&mdz_IsArchive) {
        if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) mdz_currentArchiveIndex=archiveIndex;
		
        _filePath=[NSString stringWithFormat:@"tmp/tmpArchive/%s",mdz_ArchiveFilesList[mdz_currentArchiveIndex]];
        extension = [_filePath pathExtension];
        file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
        filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
        sprintf(mod_filename,"%s/%s",archive_filename,[[filePath lastPathComponent] UTF8String]);
    }
	
	found=0;
	if (!found)
		for (int i=0;i<[filetype_extVGM count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=17;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=17;break;}
		}
    if (!found)
        for (int i=0;i<[filetype_extASAP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=13;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=13;break;}
        }
	if (!found||((mdz_defaultSAPPLAYER==DEFAULT_SAPGME)&&found==13)||((mdz_defaultVGMPLAYER==DEFAULT_VGMGME)&&found==17))
		for (int i=0;i<[filetype_extGME count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extSID count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=8;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=8;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extMDX count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {found=11;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {found=11;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extPMD count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=16;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=16;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extADPLUG count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=3;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=3;break;}
		}
	
	if (!found)
		for (int i=0;i<[filetype_extSTSOUND count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=9;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=9;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extSC68 count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=10;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=10;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extSEXYPSF count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSEXYPSF objectAtIndex:i]]==NSOrderedSame) {found=5;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSEXYPSF objectAtIndex:i]]==NSOrderedSame) {found=5;break;}
		}
    if (!found)
        for (int i=0;i<[filetype_extLAZYUSF count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extLAZYUSF objectAtIndex:i]]==NSOrderedSame) {found=18;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extLAZYUSF objectAtIndex:i]]==NSOrderedSame) {found=18;break;}
        }
	if (!found)
		for (int i=0;i<[filetype_extAOSDK count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extAOSDK objectAtIndex:i]]==NSOrderedSame) {found=4;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extAOSDK objectAtIndex:i]]==NSOrderedSame) {found=4;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extGSF count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {found=12;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {found=12;break;}
		}
    //tmp hack => redirect to timidity
	if (!found)
		for (int i=0;i<[filetype_extWMIDI count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=15;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=15;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extUADE count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {found=6;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {found=6;break;}
		}
	if ((!found)||(mdz_defaultMODPLAYER==DEFAULT_DUMB))
        for (int i=0;i<[filetype_extDUMB count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=14;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=14;break;}
        }
	if ((!found)||(mdz_defaultMODPLAYER==DEFAULT_MODPLUG))
		for (int i=0;i<[filetype_extMODPLUG count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=2;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=2;break;}
		}
    if (!found)
        for (int i=0;i<[filetype_extHVL count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=7;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=7;break;}
        }
	
    //    NSLog(@"file : %@\nfound:%d",filePath,found);
	
	if (found==1) {  //GME
		long sample_rate = (mSlowDevice?PLAYBACK_FREQ/2:PLAYBACK_FREQ); /* number of samples per second */
		int track = 0; /* index of track to play (0 = first) */
		gme_err_t err;
		mPlayType=1;
        
        gSpcSlowAPU=1;
        
		FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
		
		// Open music file in new emulator
        gme_emu=NULL;
		err=gme_open_file( [filePath UTF8String], &gme_emu, sample_rate );
		if (err) {
			NSLog(@"gme_open_file error: %s",err);
			return -1;
		} else {
			/* Register cleanup function and confirmation string as data */
			gme_set_user_data( gme_emu, my_data );
			gme_set_user_cleanup( gme_emu, my_cleanup );
            
            //is a m3u available ?
            NSString *tmpStr=[NSString stringWithFormat:@"%@.m3u",[filePath stringByDeletingPathExtension]];
            err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
            if (err) {
                NSString *tmpStr=[NSString stringWithFormat:@"%@.M3U",[filePath stringByDeletingPathExtension]];
                err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
            }
			
			/* Adjust equalizer for crisp, bassy sound */
			
            float treble,bass;
            treble=gme_eq.treble;
            bass=gme_eq.bass;
			gme_equalizer( gme_emu, &gme_eq );
            gme_eq.treble=treble;
            gme_eq.bass=bass;
			gme_set_equalizer( gme_emu, &gme_eq );
			gme_set_effects( gme_emu, &gme_fx);
			
			/**/
			int ignore_silence = strcmp(gmetype,"PC Engine") ? 1 : 0;
            //TODO: make it an option
			gme_ignore_silence(gme_emu,0);//ignore_silence);
			
			track=0;
			mod_subsongs=gme_track_count( gme_emu );
			mod_minsub=0;
			mod_maxsub=mod_subsongs-1;
			mod_currentsub=track;
			
			// Start track
			err=gme_start_track( gme_emu, track );
			if (err) {
				NSLog(@"gme_start_track error: %s",err);
				if (gme_emu) gme_delete( gme_emu );
				gme_emu=NULL;
				mPlayType=0;
				return -4;
			}
			
			sprintf(mod_name," %s",mod_filename);
			if (gme_track_info( gme_emu, &gme_info, mod_currentsub )==0) {
				iModuleLength=gme_info->play_length;
				strcpy(gmetype,gme_info->system);
				
				if (strcmp(gmetype,"NSF")==0) {//NSF
					//check length in database
					//iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
				}
				
				if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                
				sprintf(mod_message,"Song:%s\nGame:%s\nAuthor:%s\nDumper:%s\nCopyright:%s\nTracks:%d\n%s",
						(gme_info->song?gme_info->song:" "),
						(gme_info->game?gme_info->game:" "),
						(gme_info->author?gme_info->author:" "),
						(gme_info->dumper?gme_info->dumper:" "),
                        (gme_info->copyright?gme_info->copyright:" "),
						gme_track_count( gme_emu ),
						(gme_info->comment?gme_info->comment:" "));
				
				
				if (gme_info->song){
					if (gme_info->song[0]) sprintf(mod_name," %s/%s",(gme_info->game?gme_info->game:""),gme_info->song);
				}
				
				
				gme_free_info(gme_info);
			} else {
				strcpy(gmetype,"N/A");
				strcpy(mod_message,"N/A\n");
				iModuleLength=optGENDefaultLength;
				sprintf(mod_name," %s",mod_filename);
			}
			
			//Loop
			if (mLoopMode==1) iModuleLength=-1;
			if (iModuleLength>optGMEFadeOut) gme_set_fade( gme_emu, iModuleLength-optGMEFadeOut,optGMEFadeOut ); //Fade 1s before end
            //            else gme_set_fade( gme_emu, 1<<30);
			
			iCurrentTime=0;
			numChannels=gme_voice_count( gme_emu );
			
			mod_message_updated=2;
			return 0;
		}
	}
	if (found==3) {   //ADPLUG
		mPlayType=3;
		FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"ADplug Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
        // Allocate an OPL emu according to user choice : AUTO/Specified one
		// TODO
        if (mADPLUGopltype>2) mADPLUGopltype=0;
        switch (mADPLUGopltype) {
            case 0:opl=new CEmuopl(PLAYBACK_FREQ,TRUE,TRUE);  //Generic OPL
                opl->settype(Copl::TYPE_OPL2);
                break;
            case 1:
                opl=(CEmuopl*)(new CKemuopl(PLAYBACK_FREQ,TRUE,TRUE)); //Adlib OPL2/3
                break;
            case 2:
                opl=(CEmuopl*)(new CTemuopl(PLAYBACK_FREQ,TRUE,TRUE));  //Tatsuyuki
                break;
        }
        
        adplugDB = new CAdPlugDatabase;
        
        NSString *db_path = [[NSBundle mainBundle] resourcePath];
        
        adplugDB->load ([[NSString stringWithFormat:@"%@/adplug.db",db_path] UTF8String]);    // load user's database
        CAdPlug::set_database (adplugDB);
        
		adPlugPlayer = CAdPlug::factory([filePath UTF8String], opl);
		
		if (!adPlugPlayer) {
			//could not open.
			//let try the other lib below...
            delete adplugDB;
            adplugDB=NULL;
			delete opl;
            opl=NULL;
			mPlayType=0;
			for (int i=0;i<[filetype_extDUMB count];i++) { //Try Dumb if applicable
				if ([extension caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=14;break;}
				if ([file_no_ext caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=14;break;}
			}
			for (int i=0;i<[filetype_extUADE count];i++) { //Try UADE if applicable
				if ([extension caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {found=6;break;}
				if ([file_no_ext caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {found=6;break;}
			}
		} else {
			if (adPlugPlayer->update()) {
				opl_towrite=PLAYBACK_FREQ/adPlugPlayer->getrefresh();
			}
			
			std::string title=adPlugPlayer->gettitle();
			
			if (title.length()) sprintf(mod_name," %s",title.c_str());
			else sprintf(mod_name," %s",mod_filename);
			sprintf(mod_message,"%s",mod_name);
			
			if ((adPlugPlayer->getauthor()).length()>0)	sprintf(mod_message,"%sAuthor: %s\n", mod_message,adPlugPlayer->getauthor().c_str());
			if ((adPlugPlayer->getdesc()).length()>0) sprintf(mod_message,"%sDescription: %s\n",mod_message, adPlugPlayer->getdesc().c_str());
			/*		Author: %s\nDesc:%s\n",
			 (adPlugPlayer->getauthor()).c_str(),
			 (adPlugPlayer->getdesc()).c_str()];
			 */
			for (int i=0;i<adPlugPlayer->getinstruments();i++) {
				sprintf(mod_message,"%s%s\n", mod_message, adPlugPlayer->getinstrument(i).c_str());
			};
			
			iCurrentTime=0;
			iModuleLength=adPlugPlayer->songlength();
			
			//Loop
			if (mLoopMode==1) iModuleLength=-1;
            
			numChannels=9;
			
			return 0;
		}
	}
	
	if (found==5) {  //SexyPSF
		mPlayType=5;
		PSFINFO *pi;
		FILE *f;
		
		f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"SexyPSF Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
		
		NSString *fileDir=[filePath stringByDeletingLastPathComponent];
		pathdir=[fileDir UTF8String];
		
		
		if(!(pi=sexy_load((char*)[filePath UTF8String],pathdir,(mLoopMode==1)))) {
			if (sexypsf_missing_psflib) {
				/*UIAlertView *alertMissingLib=[[[UIAlertView alloc] initWithTitle:@"Warning" message:[NSString stringWithFormat:@"Missing file required for playback: %s.",sexypsf_psflib_str] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                 if (alertMissingLib) [alertMissingLib show];*/
                sprintf(mplayer_error_msg,"Missing PSFLIB: %s",sexypsf_psflib_str);
				mPlayType=0;
				//return -99;
                return -1;
			} else {
				NSLog(@"Error loading PSF");
				mPlayType=0;
				return -1;
			}
		}
		
		iModuleLength=pi->length;
		if (iModuleLength==0) {
			sexy_freepsfinfo(pi);
			sexy_stop();
			mPlayType=0;
			return -1;
		}
		
		iCurrentTime=0;
		numChannels=24;
		sprintf(mod_name,"");
		if (pi->title)
			if (pi->title[0]) sprintf(mod_name," %s",pi->title);
		
		if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
		
		sprintf(mod_message,"Game:\t%s\nTitle:\t%s\nArtist:\t%s\nYear:\t%s\nGenre:\t%s\nPSF By:\t%s\nCopyright:\t%s\n",
				(pi->game?pi->game:""),
				(pi->title?pi->title:""),
				(pi->artist?pi->artist:""),
				(pi->year?pi->year:""),
				(pi->genre?pi->genre:""),
				(pi->psfby?pi->psfby:""),
				(pi->copyright?pi->copyright:""));
		
		sexy_freepsfinfo(pi);
		
		//Loop
		if (mLoopMode==1) iModuleLength=-1;
		
		return 0;
	}
    
    if (found==18) {  //LAZYUSF
        mPlayType=18;
        FILE *f;
        
        f=fopen([filePath UTF8String],"rb");
        if (f==NULL) {
            NSLog(@"LAZYUSF Cannot open file %@",filePath);
            mPlayType=0;
            return -1;
        }
        fseek(f,0L,SEEK_END);
        mp_datasize=ftell(f);
        fclose(f);
        
        //Initi SRC samplerate converter
        int error;
        src_state=src_callback_new(src_callback,SRC_SINC_MEDIUM_QUALITY,2,&error,NULL);
        if (!src_state) {
            NSLog(@"Error while initializing SRC samplerate converter: %d",error);
            return -1;
        }
        /////////////////////////
        
        
        NSString *fileDir=[filePath stringByDeletingLastPathComponent];
        pathdir=[fileDir UTF8String];
        
        lzu_state = new usf_loader_state;
        lzu_state->emu_state = malloc( usf_get_state_size() );
        usf_clear( lzu_state->emu_state );
        
        usf_info_data=(corlett_t*)malloc(sizeof(corlett_t));
        memset(usf_info_data,0,sizeof(corlett_t));
        
        if ( psf_load( (char*)[filePath UTF8String], &psf_file_system, 0x21, usf_loader, lzu_state, usf_info, lzu_state ) < 0 ) {
            NSLog(@"Error loading USF");
            mPlayType=0;
            src_delete(src_state);
            return -1;
        }
        usf_set_compare( lzu_state->emu_state, lzu_state->enable_compare );
        usf_set_fifo_full( lzu_state->emu_state, lzu_state->enable_fifo_full );
        
        usf_render(lzu_state->emu_state, 0, 0, &lzu_sample_rate);
        src_ratio=PLAYBACK_FREQ/(double)lzu_sample_rate;
//        NSLog(@"init sr:%d %f",lzu_sample_rate,src_ratio);
        
        src_data.data_out=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*sizeof(float)*2);
        src_data.data_in=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*sizeof(float)*2);
        
        iModuleLength=-1;
        if (usf_info_data->inf_length) {
            iModuleLength=psfTimeToMS(usf_info_data->inf_length);
            //psfTimeToMS(usf_info_data->inf_fade);
        }
        
        iCurrentTime=0;
        numChannels=2;
        
        sprintf(mod_name,"");
        if (usf_info_data->inf_title)
            if (usf_info_data->inf_title[0]) sprintf(mod_name," %s",usf_info_data->inf_title);
        
        if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
        
        sprintf(mod_message,"Game:\t%s\nTitle:\t%s\nArtist:\t%s\nYear:\t%s\nGenre:\t%s\nUSF By:\t%s\nCopyright:\t%s\nTrack:\t%s\n",
                (usf_info_data->inf_game?usf_info_data->inf_game:""),
                (usf_info_data->inf_title?usf_info_data->inf_title:""),
                (usf_info_data->inf_artist?usf_info_data->inf_artist:""),
                (usf_info_data->inf_year?usf_info_data->inf_year:""),
                (usf_info_data->inf_genre?usf_info_data->inf_genre:""),
                (usf_info_data->inf_usfby?usf_info_data->inf_usfby:""),
                (usf_info_data->inf_copy?usf_info_data->inf_copy:""),
                (usf_info_data->inf_track?usf_info_data->inf_track:""));
        
        
        //Loop
        if (mLoopMode==1) iModuleLength=-1;
        
        lzu_sample_data=(int16_t*)malloc(LZU_SAMPLE_SIZE*2*2);
        lzu_sample_data_float=(float*)malloc(LZU_SAMPLE_SIZE*4*2);
        lzu_sample_converted_data_float=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4*2);
        
        
        return 0;
    }
    
    if (found==13) { //ASAP
        mPlayType=13;
        FILE *f;
        int song,duration;
        
        //		if (ASAP_IsOurFile([filePath UTF8String])==0) {
        //			NSLog(@"Incompatible with ASAP: %@",filePath);
        //			mPlayType=0;
        //			return -1;
        //		}
        
        f = fopen([filePath UTF8String], "rb");
        if (f == NULL) {
            NSLog(@"ASAP Cannot open file %@",filePath);
            mPlayType=0;
            return -1;
        }
        fseek(f,0L,SEEK_END);
        ASAP_module_len=ftell(f);
        mp_datasize=ASAP_module_len;
        ASAP_module=(unsigned char*)malloc(ASAP_module_len);
        fseek(f,0,SEEK_SET);
        fread(ASAP_module, 1, ASAP_module_len, f);
        fclose(f);
        
        if (!ASAP_Load(&asap, [filePath UTF8String], ASAP_module, ASAP_module_len)) {
            NSLog(@"Cannot ASAP_Load file %@",filePath);
            mPlayType=0;
            return -2;
        }
        
        md5_from_buffer(song_md5,33,(char*)ASAP_module,ASAP_module_len);
        song_md5[32]=0;
        
        song = asap.moduleInfo.defaultSong;
        duration = asap.moduleInfo.durations[song];
        ASAP_PlaySong(&asap, song, duration);
        mod_currentsub=song;
        
        sprintf(mod_message,"Author:%s\nTitle:%s\nSongs:%d\nChannels:%d\n",asap.moduleInfo.author,asap.moduleInfo.title,asap.moduleInfo.songs,asap.moduleInfo.channels);
        
        iModuleLength=duration;
        iCurrentTime=0;
        numChannels=asap.moduleInfo.channels;
        mod_minsub=0;
        mod_maxsub=asap.moduleInfo.songs-1;
        mod_subsongs=asap.moduleInfo.songs;
        
        sprintf(mod_name,"");
        if (asap.moduleInfo.title[0]) sprintf(mod_name," %s",asap.moduleInfo.title);
        if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
        
        stil_info[0]=0;
        [self getStilAsmaInfo:(char*)[filePath UTF8String]];
        
        sprintf(mod_message,"%s\n[STIL Information]\n%s\n",mod_message,stil_info);
        
        //Loop
        if (mLoopMode==1) iModuleLength=-1;
        
        return 0;
    }
    
	if (found==17) { //VGM
		mPlayType=17;
		FILE *f;
		int song,duration;
		
		f = fopen([filePath UTF8String], "rb");
		if (f == NULL) {
			NSLog(@"VGM Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
        
        VGMPlay_Init();
        // load configuration file here
        VGMPlay_Init2();
        if (!OpenVGMFile([filePath UTF8String]))
            if (!OpenOtherFile([filePath UTF8String])) {
                NSLog(@"Cannot OpenVGMFile file %@",filePath);
                mPlayType=0;
                return -2;
        }
        
        
		sprintf(mod_message,"Author:%ls\nGame:%ls\nSystem:%ls\nTitle:%ls\nRelease Date:%ls\nCreator:%ls\nNotes:%ls\n",
                GetTagStrEJ(VGMTag.strAuthorNameE,VGMTag.strAuthorNameJ),
                GetTagStrEJ(VGMTag.strGameNameE,VGMTag.strGameNameJ),
                GetTagStrEJ(VGMTag.strSystemNameE,VGMTag.strSystemNameJ),
                GetTagStrEJ(VGMTag.strTrackNameE,VGMTag.strTrackNameJ),
                VGMTag.strReleaseDate,
                VGMTag.strCreator,
                VGMTag.strNotes);
		
        
		iModuleLength=VGMHead.lngTotalSamples*10/441+2000;//ms
		iCurrentTime=0;
        numChannels=2;//asap.moduleInfo.channels;
		mod_minsub=0;
		mod_maxsub=0;
		mod_subsongs=1;
		
		sprintf(mod_name,"");
		if (GetTagStrEJ(VGMTag.strTrackNameE,VGMTag.strTrackNameJ)[0]) sprintf(mod_name," %ls",GetTagStrEJ(VGMTag.strTrackNameE,VGMTag.strTrackNameJ));
		if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
        
		//Loop
        if (mLoopMode==1) {
            iModuleLength=-1;
            VGMMaxLoop=2;
        }
        
		
		return 0;
	}
	if (found==12) {  //GSF
		mPlayType=12;
		FILE *f;
		char length_str[256], fade_str[256], title_str[256];
		char tmp_str[256];
		char *tag;
		
		
		soundLowPass = 1;
		soundEcho = 0;
		soundQuality = 2;//1:44Khz, 2:22Khz, 4:11Khz
		
		DetectSilence=1;
		silencelength=5;
		IgnoreTrackLength=0;
		DefaultLength=optGENDefaultLength;
		TrailingSilence=1000;
		playforever=0;
		
		
		decode_pos_ms = 0;
		seek_needed = -1;
		TrailingSilence=1000;
		
		f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"GSF Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
		
		
        
		gsf_libfile[0]=0;
        char tmp_file[2048];
        strcpy(tmp_file,(char*)[filePath UTF8String]);
		int res=GSFRun(tmp_file);
		if (!res) {
			NSLog(@"Error loading GSF file");
			mPlayType=0;
            if (gsf_libfile) {
                //missing lib
                sprintf(mplayer_error_msg,"Missing GSFLIB: %s",gsf_libfile);
            }
			return -1;
		}
		
		g_playing = 1;
		tag = (char*)malloc(50001);
		
		psftag_readfromfile((void*)tag, [filePath UTF8String]);
		
		//BOLD(); printf("Filename: "); NORMAL();
		//printf("%s\n", basename(argv[fi]));
		//BOLD(); printf("Channels: "); NORMAL();
		//printf("%d\n", sndNumChannels);
		//BOLD(); printf("Sample rate: "); NORMAL();
		//printf("%d\n", GSFsndSamplesPerSec);
		
		sprintf(mod_message,"\n");
		
		/*		sprintf(mod_message,"Game:\t%s\nTitle:\t%s\nArtist:\t%s\nYear:\t%s\nGenre:\t%s\nPSF By:\t%s\nCopyright:\t%s\n",
		 (pi->game?pi->game:""),
		 (pi->title?pi->title:""),
		 (pi->artist?pi->artist:""),
		 (pi->year?pi->year:""),
		 (pi->genre?pi->genre:""),
		 (pi->psfby?pi->psfby:""),
		 (pi->copyright?pi->copyright:""));
		 */
		
		if (!psftag_getvar(tag, "title", title_str, sizeof(title_str)-1)) {
			//BOLD(); printf("Title: "); NORMAL();
			sprintf(mod_message,"%sTitle:%s\n",mod_message,title_str);
			//printf("%s\n", title_str);
		}
		
		if (!psftag_getvar(tag, "artist", tmp_str, sizeof(tmp_str)-1)) {
			//BOLD(); printf("Artist: "); NORMAL();
			//printf("%s\n", tmp_str);
			sprintf(mod_message,"%sArtist:%s\n",mod_message,tmp_str);
		}
		
		if (!psftag_getvar(tag, "game", tmp_str, sizeof(tmp_str)-1)) {
			//BOLD(); printf("Game: "); NORMAL();
			//printf("%s\n", tmp_str);
			sprintf(mod_message,"%sGame:%s\n",mod_message,tmp_str);
		}
		
		if (!psftag_getvar(tag, "year", tmp_str, sizeof(tmp_str)-1)) {
			//BOLD(); printf("Year: "); NORMAL();
			//printf("%s\n", tmp_str);
			sprintf(mod_message,"%sYear:%s\n",mod_message,tmp_str);
		}
		
		if (!psftag_getvar(tag, "copyright", tmp_str, sizeof(tmp_str)-1)) {
			//BOLD(); printf("Copyright: "); NORMAL();
			//printf("%s\n", tmp_str);
			sprintf(mod_message,"%sCopyright:%s\n",mod_message,tmp_str);
		}
		
		if (!psftag_getvar(tag, "gsfby", tmp_str, sizeof(tmp_str)-1)) {
			//BOLD(); printf("GSF By: "); NORMAL();
			//printf("%s\n", tmp_str);
			sprintf(mod_message,"%sGSFby:%s\n",mod_message,tmp_str);
		}
		
		if (!psftag_getvar(tag, "tagger", tmp_str, sizeof(tmp_str)-1)) {
			//BOLD(); printf("Tagger: "); NORMAL();
			//printf("%s\n", tmp_str);
			sprintf(mod_message,"%sTagger:%s\n",mod_message,tmp_str);
		}
		
		if (!psftag_getvar(tag, "comment", tmp_str, sizeof(tmp_str)-1)) {
			//BOLD(); printf("Comment: "); NORMAL();
			//printf("%s\n", tmp_str);
			sprintf(mod_message,"%sComment:%s\n",mod_message,tmp_str);
		}
		
		if (!psftag_getvar(tag, "fade", fade_str, sizeof(fade_str)-1)) {
			FadeLength = LengthFromString(fade_str);
			//BOLD(); printf("Fade: "); NORMAL();
			//printf("%s (%d ms)\n", fade_str, FadeLength);
		}
		
		if (!psftag_raw_getvar(tag, "length", length_str, sizeof(length_str)-1)) {
			TrackLength = LengthFromString(length_str) + FadeLength;
			//BOLD(); printf("Length: "); NORMAL();
			//printf("%s (%d ms) ", length_str, TrackLength);
			if (IgnoreTrackLength) {
                //	printf("(ignored)");
				TrackLength = optGENDefaultLength;
			}
			//printf("\n");
		}
		else {
			TrackLength = optGENDefaultLength;
		}
		
		
		
		iModuleLength=TrackLength;
		
		iCurrentTime=0;
		numChannels=sndNumChannels;
		sprintf(mod_name,"");
		if (title_str)
			if (title_str[0]) sprintf(mod_name," %s",title_str);
		
		if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
		
        
		free(tag);
		
		//Loop
		if (mLoopMode==1) iModuleLength=-1;
		
		return 0;
	}
	if (found==4) {  //AOSDK
		mPlayType=4;
		uint32 filesig;
        
        
        FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"AOSDK Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		fseek(f,0,SEEK_END);
		mp_datasize=ftell(f);
		//rewind(f);
        fseek(f,0,SEEK_SET);
		ao_buffer=(unsigned char*)malloc(mp_datasize);
		fread(ao_buffer,mp_datasize,sizeof(char),f);
		fclose(f);
        
        //		NSLog(@"%s, %d",[filePath UTF8String],mp_datasize);
		
		NSString *fileDir=[filePath stringByDeletingLastPathComponent];
		pathdir=[fileDir UTF8String];
		
		// now try to identify the file
		ao_type = 0;
		filesig = ao_buffer[0]<<24 | ao_buffer[1]<<16 | ao_buffer[2]<<8 | ao_buffer[3];
		while (ao_types[ao_type].sig != 0xffffffff)	{
			if (filesig == ao_types[ao_type].sig) break;
			else ao_type++;
		}
		
		// now did we identify it above or just fall through?
		if (ao_types[ao_type].sig != 0xffffffff) {
			//printf("File identified as %s\n", ao_types[ao_type].name);
		} else {
			printf("ERROR: File is unknown, signature bytes are %02x %02x %02x %02x\n", ao_buffer[0], ao_buffer[1], ao_buffer[2], ao_buffer[3]);
			free(ao_buffer);
			mPlayType=0;
			return -1;
		}
		
		if ((*ao_types[ao_type].start)(ao_buffer, mp_datasize, (mLoopMode==1),optGENDefaultLength) != AO_SUCCESS) {
			free(ao_buffer);
			if (aopsf2_missing_psflib) {
				/*UIAlertView *alertMissingLib=[[[UIAlertView alloc] initWithTitle:@"Warning" message:[NSString stringWithFormat:@"Missing file required for playback: %s.",aopsf2_psflib_str] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                 if (alertMissingLib) [alertMissingLib show];*/
                sprintf(mplayer_error_msg,"Missing PSF2LIB: %s",sexypsf_psflib_str);
				mPlayType=0;
				//return -99;
                return -1;
			} else  {
				printf("ERROR: Engine rejected file!\n");
				mPlayType=0;
				return -1;
			}
		}
		
		(*ao_types[ao_type].fillinfo)(&ao_info);
		
		iModuleLength=ao_info.length_ms+ao_info.fade_ms;
        if (iModuleLength==0) iModuleLength=optGENDefaultLength;
        
        
		iCurrentTime=0;
		numChannels=24;
		if (ao_info.info[1][0]) {
            if (strcmp(ao_info.info[1],"n/a")) sprintf(mod_name," %s",ao_info.info[1]);
            else sprintf(mod_name," %s",mod_filename);
        }
		else sprintf(mod_name," %s",mod_filename);
		sprintf(mod_message,"%s%s\n%s%s\n%s%s\n%s%s\n%s%s\n%s%s\n%s%s\n",
				ao_info.title[2],ao_info.info[2],
				ao_info.title[3],ao_info.info[3],
				ao_info.title[4],ao_info.info[4],
				ao_info.title[5],ao_info.info[5],
				ao_info.title[6],ao_info.info[6],
				ao_info.title[7],ao_info.info[7],
				ao_info.title[8],ao_info.info[8]);
		
		//Loop
		if (mLoopMode==1) iModuleLength=-1;
		
		
		return 0;
	}
	if (found==8) {  //SID
		mPlayType=8;
		
		//First check that the file is available and get size
		FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"SID Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
		
		if (mSidEngineType==1) {
			
			mSid1EmuEngine = new emuEngine;
			// Set config
			struct emuConfig cfg;
			mSid1EmuEngine->getConfig(cfg);
			cfg.channels = SIDEMU_STEREO;
			cfg.volumeControl = SIDEMU_VOLCONTROL;
            if (mSIDFilterON) cfg.emulateFilter=TRUE;
            else cfg.emulateFilter=FALSE;

            switch (sid_forceClock) {
                case 0:
                    cfg.forceSongSpeed=FALSE;
                    break;
                case 1:
                    cfg.forceSongSpeed=TRUE;
                    cfg.clockSpeed=SIDTUNE_CLOCK_PAL;
                    break;
                case 2:
                    cfg.forceSongSpeed=TRUE;
                    cfg.clockSpeed=SIDTUNE_CLOCK_NTSC;
                    break;
            }
            
            switch (sid_forceModel) {
                case 0:
                    break;
                case 1:
                    cfg.mos8580=0;
                    break;
                case 2:
                    cfg.mos8580=1;
                    break;
            }
            
			mSid1EmuEngine->setConfig(cfg);
			
			
			// Load tune
			mSid1Tune=new sidTune([filePath UTF8String],true,0);
			
			if ((mSid1Tune==NULL)||(mSid1Tune->cachePtr==0)) {
				NSLog(@"SID SidTune init error");
				delete mSid1EmuEngine; mSid1EmuEngine=NULL;
				if (mSid1Tune) {delete mSid1Tune;mSid1Tune=NULL;}
				mPlayType=0;
				//try UADE	:sidmon1 or sidmon2
				found=6;
			} else {
				struct sidTuneInfo sidtune_info;
				mSid1Tune->getInfo(sidtune_info);
				
				if (sidtune_info.infoString[0][0]) sprintf(mod_name," %s",sidtune_info.infoString[0]);
				else sprintf(mod_name," %s",mod_filename);
				mod_subsongs=sidtune_info.songs;
				mod_minsub=1;//sidtune_info.startSong;
				mod_maxsub=sidtune_info.songs;
				mod_currentsub=sidtune_info.startSong;
				
				int tmp_md5_data_size=sidtune_info.c64dataLen+2*3+sizeof(sidtune_info.songSpeed)*sidtune_info.songs;
				char *tmp_md5_data=(char*)malloc(tmp_md5_data_size);
				memset(tmp_md5_data,0,tmp_md5_data_size);
				int ofs_md5_data=0;
				unsigned char tmp[2];
				memcpy(tmp_md5_data,mSid1Tune->cachePtr+mSid1Tune->fileOffset,sidtune_info.c64dataLen);
				ofs_md5_data+=sidtune_info.c64dataLen;
				// Include INIT and PLAY address.
				writeLEword(tmp,sidtune_info.initAddr);
				memcpy(tmp_md5_data+ofs_md5_data,tmp,2);
				ofs_md5_data+=2;
				writeLEword(tmp,sidtune_info.playAddr);
				memcpy(tmp_md5_data+ofs_md5_data,tmp,2);
				ofs_md5_data+=2;
				// Include number of songs.
				writeLEword(tmp,sidtune_info.songs);
				memcpy(tmp_md5_data+ofs_md5_data,tmp,2);
				ofs_md5_data+=2;
				
				// Include song speed for each song.
				for (unsigned int s = 1; s <= sidtune_info.songs; s++) {
					sidEmuInitializeSong(*mSid1EmuEngine,*mSid1Tune, s);
					mSid1Tune->getInfo(sidtune_info);
					if (sidtune_info.songSpeed==50) sidtune_info.songSpeed=0;
					memcpy(tmp_md5_data+ofs_md5_data,&sidtune_info.songSpeed,sizeof(sidtune_info.songSpeed));
					//NSLog(@"sp : %d %d %d",s,sidtune_info.songSpeed,sizeof(sidtune_info.songSpeed));
					ofs_md5_data+=sizeof(sidtune_info.songSpeed);
				}
				// Deal with PSID v2NG clock speed flags: Let only NTSC
				// clock speed change the MD5 fingerprint. That way the
				// fingerprint of a PAL-speed sidtune in PSID v1, v2, and
				// PSID v2NG format is the same.
				if ( sidtune_info.clockSpeed == SIDTUNE_CLOCK_NTSC ) {
					memcpy(tmp_md5_data+ofs_md5_data,&sidtune_info.clockSpeed,sizeof(sidtune_info.clockSpeed));
					ofs_md5_data+=sizeof(sidtune_info.clockSpeed);
				}
				
				md5_from_buffer(song_md5,33,tmp_md5_data,tmp_md5_data_size);
				song_md5[32]=0;
				free(tmp_md5_data);
				//NSLog(@"MD5: %s",song_md5);
				
				sidEmuInitializeSong(*mSid1EmuEngine,*mSid1Tune, mod_currentsub);
				mSid1Tune->getInfo(sidtune_info);
				iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
				if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
				
				if ((sidtune_info.sidModel==SIDTUNE_SIDMODEL_6581)&&(sid_forceModel==0)) {
                    mSid1EmuEngine->getConfig(cfg);
                    cfg.mos8580=0;
                    mSid1EmuEngine->setConfig(cfg);
				}
				
				if ((sidtune_info.sidModel == SIDTUNE_SIDMODEL_8580)&&(sid_forceModel==0)){
                    mSid1EmuEngine->getConfig(cfg);
                    cfg.mos8580=0;
                    mSid1EmuEngine->setConfig(cfg);
				} else {
					//mFilterSettings.distortion_enable = true;
					//mBuilder->filter(&mFilterSettings);
				}
				
				iCurrentTime=0;
				numChannels=4;
				
				
				mSid1EmuEngine->setVoiceVolume(1, 150, 255-150, 256);
				mSid1EmuEngine->setVoiceVolume(3, 150, 255-150, 256);
				mSid1EmuEngine->setVoiceVolume(2, 255-150, 150, 256);
				mSid1EmuEngine->setVoiceVolume(4, 255-150, 150, 256);
				
                stil_info[0]=0;
    			[self getStilInfo:(char*)[filePath UTF8String]];
				
				sprintf(mod_message,"");
				for (int i=0;i<sidtune_info.numberOfInfoStrings;i++)
					sprintf(mod_message,"%s%s\n",mod_message,sidtune_info.infoString[i]);
				
				sprintf(mod_message,"%s\n[STIL Information]\n%s\n",mod_message,stil_info);
				
				return 0;
			}
		} else {
			
			// Init SID emu engine
			mSidEmuEngine = new sidplay2;
			// Init ReSID
			mBuilder = new ReSIDBuilder("resid");
            //mBuilder = new ReSIDfpBuilder("residfp");
			// Set config
			sid2_config_t cfg = mSidEmuEngine->config();
			//cfg.optimisation = optSIDoptim;
			cfg.sidEmulation  = mBuilder;
			cfg.frequency= PLAYBACK_FREQ;
			cfg.emulateStereo = false;
			cfg.playback = sid2_stereo;
			cfg.sidSamples	  = true;
            
            switch (sid_forceClock) {
                case 0:
                    cfg.clockForced=FALSE;
                    break;
                case 1:
                    cfg.clockForced=TRUE;
                    cfg.clockSpeed=SID2_CLOCK_PAL;
                    break;
                case 2:
                    cfg.clockForced=TRUE;
                    cfg.clockSpeed=SID2_CLOCK_NTSC;
                    break;
            }
            
            switch (sid_forceModel) {
                case 0:
                    cfg.sidModel=SID2_MODEL_CORRECT;
                    break;
                case 1:
                    cfg.sidModel=SID2_MOS6581;
                    break;
                case 2:
                    cfg.sidModel=SID2_MOS8580;
                    break;
            }
            
          	// setup resid
            if (mBuilder) mBuilder->create(mSidEmuEngine->info().maxsids);
			if (mSIDFilterON) mBuilder->filter(true);
            else mBuilder->filter(false);
            mBuilder->bias(0);
            
			//mBuilder->filter((const sid_filter_t *)NULL);
            //mBuilder->filter(&mSIDFilter);
			//mBuilder->sampling(cfg.frequency);
            
			mSidEmuEngine->config(cfg);
			// Load tune
			mSidTune=new SidTune([filePath UTF8String],0,true);
			
			if ((mSidTune==NULL)||(mSidTune->cache.get()==0)) {
				NSLog(@"SID SidTune init error");
				delete mSidEmuEngine; mSidEmuEngine=NULL;
				delete mBuilder; mBuilder=NULL;
				if (mSidTune) {delete mSidTune;mSidTune=NULL;}
				mPlayType=0;
				//try UADE	:sidmon1 or sidmon2
				found=6;
			} else {
				SidTuneInfo sidtune_info;
				sidtune_info=mSidTune->getInfo();
				
				if (sidtune_info.infoString[0][0]) sprintf(mod_name," %s",sidtune_info.infoString[0]);
				else sprintf(mod_name," %s",mod_filename);
				mod_subsongs=sidtune_info.songs;
				mod_minsub=1;//sidtune_info.startSong;
				mod_maxsub=sidtune_info.songs;
				mod_currentsub=sidtune_info.startSong;
				
				int tmp_md5_data_size=sidtune_info.c64dataLen+2*3+sizeof(sidtune_info.songSpeed)*sidtune_info.songs;
				char *tmp_md5_data=(char*)malloc(tmp_md5_data_size);
				memset(tmp_md5_data,0,tmp_md5_data_size);
				int ofs_md5_data=0;
				unsigned char tmp[2];
				memcpy(tmp_md5_data,mSidTune->cache.get()+mSidTune->fileOffset,sidtune_info.c64dataLen);
				ofs_md5_data+=sidtune_info.c64dataLen;
				// Include INIT and PLAY address.
				writeLEword(tmp,sidtune_info.initAddr);
				memcpy(tmp_md5_data+ofs_md5_data,tmp,2);
				ofs_md5_data+=2;
				writeLEword(tmp,sidtune_info.playAddr);
				memcpy(tmp_md5_data+ofs_md5_data,tmp,2);
				ofs_md5_data+=2;
				// Include number of songs.
				writeLEword(tmp,sidtune_info.songs);
				memcpy(tmp_md5_data+ofs_md5_data,tmp,2);
				ofs_md5_data+=2;
				
				// Include song speed for each song.
				for (unsigned int s = 1; s <= sidtune_info.songs; s++)
				{
					mSidTune->selectSong(s);
					memcpy(tmp_md5_data+ofs_md5_data,&mSidTune->info.songSpeed,sizeof(mSidTune->info.songSpeed));
					//NSLog(@"sp : %d %d %d",s,mSidTune->info.songSpeed,sizeof(mSidTune->info.songSpeed));
					ofs_md5_data+=sizeof(mSidTune->info.songSpeed);
				}
				// Deal with PSID v2NG clock speed flags: Let only NTSC
				// clock speed change the MD5 fingerprint. That way the
				// fingerprint of a PAL-speed sidtune in PSID v1, v2, and
				// PSID v2NG format is the same.
				if ( mSidTune->info.clockSpeed == SIDTUNE_CLOCK_NTSC ) {
					memcpy(tmp_md5_data+ofs_md5_data,&mSidTune->info.clockSpeed,sizeof(mSidTune->info.clockSpeed));
					ofs_md5_data+=sizeof(mSidTune->info.clockSpeed);
					//myMD5.append(&info.clockSpeed,sizeof(info.clockSpeed));
				}
				md5_from_buffer(song_md5,33,tmp_md5_data,tmp_md5_data_size);
				song_md5[32]=0;
				free(tmp_md5_data);
				//NSLog(@"MD5: %s",song_md5);
				
				
				mSidTune->selectSong(mod_currentsub);
				iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
				if (!iModuleLength) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
				
				
				
                /*				if (sidtune_info.sidModel==SIDTUNE_SIDMODEL_6581) {
                 mBuilder->filter((sid_filter_t*)NULL);
                 }
                 
                 if (sidtune_info.sidModel == SIDTUNE_SIDMODEL_8580){
                 mBuilder->filter((sid_filter_t*)NULL);
                 } else {
                 mSIDFilter.distortion_enable = true;
                 mBuilder->filter(&mSIDFilter);
                 }*/
				
				if (mSidEmuEngine->load(mSidTune)==0) {
					iCurrentTime=0;
					numChannels=mSidEmuEngine->info().channels;
					
                    stil_info[0]=0;
					[self getStilInfo:(char*)[filePath UTF8String]];
					
					sprintf(mod_message,"");
					for (int i=0;i<sidtune_info.numberOfInfoStrings;i++)
						sprintf(mod_message,"%s%s\n",mod_message,sidtune_info.infoString[i]);
					sprintf(mod_message,"%s\n[STIL Information]\n%s\n",mod_message,stil_info);
					//Loop
					if (mLoopMode==1) iModuleLength=-1;
					return 0;
				}
			}
		}
	}
	if (found==6) {  //UADE
		int ret;
		
		mPlayType=6;
		// First check that the file is accessible and get the size
		FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"UADE Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fseek(f,0,SEEK_SET);
		char *tmp_md5_data=(char*)malloc(mp_datasize);
		fread(tmp_md5_data, 1, mp_datasize, f);
		md5_from_buffer(song_md5,33,tmp_md5_data,mp_datasize);
		song_md5[32]=0;
		free(tmp_md5_data);
		fclose(f);
		
		uadeThread_running=0;
		[NSThread detachNewThreadSelector:@selector(uadeThread) toTarget:self withObject:NULL];
		
		UADEstate.ipc.state = UADE_INITIAL_STATE;
		UADEstate.ipc.input= uade_ipc_set_input("fd://1");
		UADEstate.ipc.output = uade_ipc_set_output("fd://1");
		
		if (uade_send_string(UADE_COMMAND_CONFIG, UADEconfigname, &(UADEstate.ipc))) {
			printf("Can not send config name: %s\n",strerror(errno));
			mPlayType=0;
			return -4;
		}
		//try to determine player
		char modulename[PATH_MAX];
		char songname[PATH_MAX];
		strcpy(modulename, [filePath UTF8String]);
		UADEstate.song = NULL;
		UADEstate.ep = NULL;
		if (!uade_is_our_file(modulename, 0, &UADEstate)) {
			printf("Unknown format: %s\n", modulename);
			mPlayType=0;
			return -3;
		}
		
		//		printf("Player candidate: %s\n", UADEstate.ep->playername);
		
		if (strcmp(UADEstate.ep->playername, "custom") == 0) {
			strcpy(UADEplayername, modulename);
			modulename[0] = 0;
		} else {
			sprintf(UADEplayername, "%s/%s", UADEstate.config.basedir.name, UADEstate.ep->playername);
		}
		
		//		printf("Player name: %s\n", UADEplayername);
		
		if (strlen(UADEplayername) == 0) {
			printf("Error: an empty player name given\n");
			mPlayType=0;
			return -4;
		}
		
		strcpy(songname, modulename[0] ? modulename : UADEplayername);
		
		if (!uade_alloc_song(&UADEstate, songname)) {
			printf("Can not read %s: %s\n", songname,strerror(errno));
			mPlayType=0;
			return -5;
		}
		
		if (UADEstate.ep != NULL) uade_set_ep_attributes(&UADEstate);
		
		/* Now we have the final configuration in "uc". */
		
		UADEstate.config.no_postprocessing=mUADE_OptPOSTFX^1;
		UADEstate.config.headphones=mUADE_OptHEAD;
		UADEstate.config.gain_enable=mUADE_OptGAIN;
		UADEstate.config.gain=mUADE_OptGAINValue;
		UADEstate.config.normalise=mUADE_OptNORM;
		UADEstate.config.panning_enable=mUADE_OptPAN;
		UADEstate.config.panning=mUADE_OptPANValue;
        UADEstate.config.no_ep_end=(mLoopMode==1?1:0);
        //        UADEstate.config.use_ntsc=1;
		
		uade_set_effects(&UADEstate);
		
		//		printf("Song: %s (%zd bytes)\n",UADEstate.song->module_filename, UADEstate.song->bufsize);
		
		ret = uade_song_initialization(UADEscorename, UADEplayername, modulename, &UADEstate);
		if (ret) {
			if (ret == UADECORE_INIT_ERROR) {
				uade_unalloc_song(&UADEstate);
				mPlayType=0;
				return -6;
				//				goto cleanup;
				
			} else if (ret == UADECORE_CANT_PLAY) {
				printf("Uadecore refuses to play the song.\n");
				uade_unalloc_song(&UADEstate);
				mPlayType=0;
				return -7;
				//				continue;
			}
			
			printf("Unknown error from uade_song_initialization()\n");
			exit(1);
		}
		
		
		// song info
        strcpy(mod_filename,[[filePath lastPathComponent] UTF8String]);
        sprintf(mod_name," %s",mod_filename);
		sprintf(mod_message,"%s\n",mod_name);
		numChannels=4;
		iCurrentTime=0;
		iModuleLength=UADEstate.song->playtime;
		if (iModuleLength<0) iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
		
		//Loop
		if (mLoopMode==1) iModuleLength=-1;
		
		
		//		NSLog(@"playtime : %d",UADEstate.song->playtime);
		
		return 0;
	}
	
	if (found==2) {  //MODPLUG
		const char *modName;
		char *modMessage;
		mPlayType=2;
		
		FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"MODPLUG Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		rewind(f);
		mp_data=(char*)malloc(mp_datasize);
		fread(mp_data,mp_datasize,sizeof(char),f);
		fclose(f);
		
		[self getMPSettings];
		if (mLoopMode==1) mp_settings.mLoopCount=-1; //Should be like "infinite"
        else mp_settings.mLoopCount=0;
		[self updateMPSettings];
		
		mp_file=ModPlug_Load(mp_data,mp_datasize);
		if (mp_file==NULL) {
			free(mp_data); /* ? */
			NSLog(@"ModPlug_load error");
			mPlayType=0;
		} else {
			iModuleLength=ModPlug_GetLength(mp_file);
			iCurrentTime=0;
            mPatternDataAvail=1;
			
			numChannels=ModPlug_NumChannels(mp_file);
			numPatterns=ModPlug_NumPatterns(mp_file);
			
			modName=ModPlug_GetName(mp_file);
			if (!modName) {
				sprintf(mod_name," %s",mod_filename);
			} else if (modName[0]==0) {
				sprintf(mod_name," %s",mod_filename);
			} else {
				sprintf(mod_name," %s", modName);
			}
			
			
			numSamples=ModPlug_NumSamples(mp_file);
			numInstr=ModPlug_NumInstruments(mp_file);
			
			modMessage=ModPlug_GetMessage(mp_file);
			if (modMessage) sprintf(mod_message,"%s\n",modMessage);
			else {
				if ((numInstr==0)&&(numSamples==0)) sprintf(mod_message,"N/A\n");
				else sprintf(mod_message,"");
			}
			
			
/*			if (numInstr>0) {
				for (int i=1;i<=numInstr;i++) {
					ModPlug_InstrumentName(mp_file,i,str_name);
					sprintf(mod_message,"%s%s\n", mod_message,str_name);
				};
			}
			if (numSamples>0) {
				for (int i=1;i<=numSamples;i++) {
					ModPlug_SampleName(mp_file,i,str_name);
					sprintf(mod_message,"%s%s\n", mod_message,str_name);
				};
			}*/
			
			//Loop
			if (mLoopMode==1) {
                iModuleLength=-1;
            }
			
			return 0;
		}
	}
	if (found==7) {  //HVL
		mPlayType=7;
		
		FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"HVL Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
		
		if (mHVLinit==0) {
			hvl_InitReplayer();
			mHVLinit=1;
		}
		
		hvl_song=hvl_LoadTune((TEXT*)[filePath UTF8String],PLAYBACK_FREQ,1);
		
		if (hvl_song==NULL) {
			NSLog(@"HVL loadTune error");
			mPlayType=0;
			return -1;
		} else {
			
			
			
			if (hvl_song->ht_Name[0]) sprintf(mod_name," %s",hvl_song->ht_Name);
			else sprintf(mod_name," %s",[[filePath lastPathComponent] UTF8String]);
			
			mod_subsongs=hvl_song->ht_SubsongNr;
			mod_minsub=0;
			mod_maxsub=mod_subsongs-1;
			mod_currentsub=0;
			
			if (!hvl_InitSubsong( hvl_song,mod_currentsub )) {
				NSLog(@"HVL issue in initsubsong %d",mod_currentsub);
				hvl_FreeTune(hvl_song);
				mPlayType=0;
				return -2;
			}
			iModuleLength=hvl_GetPlayTime(hvl_song);
			iCurrentTime=0;
			
			numChannels=hvl_song->ht_Channels;
			
			if (hvl_song->ht_InstrumentNr==0) sprintf(mod_message,"N/A\n");
			else {
				sprintf(mod_message,"");
				for (int i=0;i<hvl_song->ht_InstrumentNr;i++) {
					sprintf(mod_message,"%s%s\n", mod_message,hvl_song->ht_Instruments[i].ins_Name);
				};
			}
			
			hvl_sample_to_write=hvl_song->ht_Frequency/50/hvl_song->ht_SpeedMultiplier;
			
			//Loop
			if (mLoopMode==1) iModuleLength=-1;
			
			
			return 0;
		}
	}
	if (found==9) {  //STSOUND
		mPlayType=9;
		
		FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"STSOUND Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
		
		ymMusic = ymMusicCreate();
		
		if (!ymMusicLoad(ymMusic,[filePath UTF8String])) {
			NSLog(@"STSOUND ymMusicLoad error");
			mPlayType=0;
			ymMusicDestroy(ymMusic);
			return -1;
		} else {
			ymMusicInfo_t info;
			ymMusicGetInfo(ymMusic,&info);
			
			ymMusicSetLoopMode(ymMusic,YMFALSE);
			ymMusicSetLowpassFiler(ymMusic,YMTRUE);
			ymMusicPlay(ymMusic);
			
			if (info.pSongName[0]) sprintf(mod_name," %s",info.pSongName);
			else sprintf(mod_name," %s",mod_filename);
			mod_subsongs=1;
			mod_minsub=0;
			mod_maxsub=0;
			mod_currentsub=0;
			
			iModuleLength=info.musicTimeInMs;
			iCurrentTime=0;
			
			numChannels=2;
			
			sprintf(mod_message,"Name.....: %s\nAuthor...: %s\nType.....: %s\nPlayer...: %s\nComment..: %s\n",info.pSongName,info.pSongAuthor,info.pSongType,info.pSongPlayer,info.pSongComment);
			
			//Loop
			if (mLoopMode==1) iModuleLength=-1;
			
			
			return 0;
		}
	}
	if (found==10) {  //SC68
		mPlayType=10;
		
		FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"SC68 Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
		
		if (api68_load_file(sc68,[filePath UTF8String])) {
			NSLog(@"SC68 api68_load_file error");
			mPlayType=0;
			return -1;
		} else {
			api68_music_info_t info;
			api68_music_info( sc68, &info, 1, NULL );
			if (info.title[0]) sprintf(mod_name," %s",info.title);
			else sprintf(mod_name," %s",mod_filename);
			
			mod_subsongs=info.tracks;
			mod_minsub=1;
			mod_maxsub=1+info.tracks-1;
			mod_currentsub=1;
			
			iModuleLength=info.time_ms;
			if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
			iCurrentTime=0;
			
			numChannels=2;
			
			sprintf(mod_message,"Title.....: %s\nAuthor...: %s\nComposer...: %s\nHardware...: %s\nConverter.....: %s\nRipper...: %s\n",
					info.title,info.author,info.composer,info.hwname,info.converter,info.ripper);
			
			return 0;
		}
	}
	if (found==11) {  //MDX
		mPlayType=11;
		
		FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"MDX Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
		
		if (mdx_load((char*)[filePath UTF8String],&mdx,&pdx,mSlowDevice,mLoopMode) ) {
			NSLog(@"MDX mdx_load error");
			mPlayType=0;
			return -1;
		} else {
			char *tmp_mod_name=(char*)mdx_get_title(mdx);
			if (tmp_mod_name) sprintf(mod_name," %s",tmp_mod_name);
			else sprintf(mod_name," %s",mod_filename);
			
			mod_subsongs=1;
			mod_minsub=1;
			mod_maxsub=1;
			mod_currentsub=1;
			
			iModuleLength=mdx_get_length( mdx,pdx);
			if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//MDX_DEFAULT_LENGTH;
			iCurrentTime=0;
			
			numChannels=mdx->tracks;
			
			if (tmp_mod_name) sprintf(mod_message,"Title.....: %s\n",tmp_mod_name);
			else sprintf(mod_message,"Title.....: N/A\n");
			
			if (tmp_mod_name) free(tmp_mod_name);
			
			if (mdx->pdx_name) {
				if (strlen(mdx->pdx_name) && (mdx->haspdx==0)) {
					sprintf(mod_message,"%sMissing PDX file: %s\n",mod_message,mdx->pdx_name);
                    NSString *alertMsg=[NSString stringWithFormat:NSLocalizedString(@"Missing PDX file: %s",@""),mdx->pdx_name];
                    UIAlertView *alertMissingPDX = [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"")
                                                                               message:alertMsg delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil] autorelease];
                    [alertMissingPDX show];
				}
			}
			
			//Loop
			if (mLoopMode==1) iModuleLength=-1;
			
			return 0;
		}
	}
    if (found==14) { //DUMB
        mPlayType=14;
        it_max_channels=0;
        
        FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"DUMB Cannot open file %@",filePath);
			mPlayType=0;
			return -1;
		}
		
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
        //Pattern display/modplug
		rewind(f);
		mp_data=(char*)malloc(mp_datasize);
		fread(mp_data,mp_datasize,1,f);
        //
		fclose(f);
        
        mod_subsongs=1;
		mod_minsub=1;
		mod_maxsub=1;
		mod_currentsub=1;
        
        /* Load file */
        typedef void (*func)(void);
        typedef DUH *(*dumb1)(const char *);
        typedef DUH *(*dumb2)(const char *, int);
        func dumbFormats[15] = {
            (func)&dumb_load_it, (func)&load_duh,
            (func)&dumb_load_xm, (func)&dumb_load_s3m,
            (func)&dumb_load_mod, (func)&dumb_load_stm,
            (func)&dumb_load_ptm, (func)&dumb_load_669,
            (func)&dumb_load_mtm, (func)&dumb_load_riff,
            (func)&dumb_load_asy, (func)&dumb_load_amf,
            (func)&dumb_load_okt, (func)&dumb_load_psm,
            (func)&dumb_load_old_psm };
        
        int i;
        for(i = 0; i < 15; i++) {
            dumbfile_open_memory(mp_data,mp_datasize);
            if(i==4||i==12) {
                duh = ((dumb2)dumbFormats[i])([filePath UTF8String],0);
            } else {
                duh = ((dumb1)dumbFormats[i])([filePath UTF8String]);
            }
            if(duh) {break;}
        }
        if(!duh) {
            free(mp_data);
            mp_data=NULL;
            return 1;
        }
        
        iModuleLength = (int)((LONG_LONG)duh_get_length(duh) * 1000 >> 16);
        const char *mod_title = duh_get_tag(duh, "TITLE");
        if (mod_title && mod_title[0]) {
            int i=0;
            while (mod_title[i]==' ') i++;  //only spaces?
            if (mod_title[i]) sprintf(mod_name," %s",mod_title); //no
            else sprintf(mod_name," %s",mod_filename); //yes
        }
        else sprintf(mod_name," %s",mod_filename);
        
        //NSLog(@"mod names: %s / %s",mod_title,mod_filename);
        
		iCurrentTime=0;
		numChannels=2;
		//Loop
		if (mLoopMode==1) iModuleLength=-1;
        
        duh_player = (DUH_PLAYER*)malloc(sizeof(DUH_PLAYER));
        if (!duh_player) {
            unload_duh(duh); duh=NULL;
            free(mp_data);
            mp_data=NULL;
            
            return -2;
        }
        duh_player->n_channels = 2;
        duh_player->volume = dumb_MastVol;
        
        duh_player->dr = duh_start_sigrenderer(duh, 0, 2/*nb channels*/, 0/*pos*/);
        if (!duh_player->dr) {
            free(duh_player);duh_player=NULL;
            unload_duh(duh); duh=NULL;
            free(mp_data);
            mp_data=NULL;
            
            return -3;
        }
        DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(duh_player->dr);
        if (mLoopMode==1) dumb_it_set_loop_callback(itsr, &dumb_it_callback_loop, NULL);
        else dumb_it_set_loop_callback(itsr, &dumb_it_callback_terminate, NULL);
        dumb_it_set_xm_speed_zero_callback(itsr, &dumb_it_callback_terminate, NULL);
        
        DUMB_IT_SIGDATA *itsd = duh_get_it_sigdata(duh);
        
        if (it_max_channels) numChannels=it_max_channels+1;
        else if (itsd->n_pchannels) numChannels = itsd->n_pchannels;
        
        sprintf(mod_message,"%s\n",[[filePath lastPathComponent] UTF8String]);
        const char * tag;
        tag = duh_get_tag(duh, "TRACKERVERSION");
        if (tag && *tag) sprintf(mod_message,"%sTracker version: %s\n",mod_message, tag);
        tag = duh_get_tag(duh, "FORMATVERSION");
        if (tag && *tag) sprintf(mod_message,"%sFormat version: %s\n",mod_message, tag);
        
        if (itsd->song_message) sprintf(mod_message,"%s%s\n",mod_message, itsd->song_message);
        
        if (itsd->n_instruments) {
            if (itsd->instrument)
                for (int i = 0, n = itsd->n_instruments; i < n; i++) sprintf(mod_message,"%s%s\n",mod_message,itsd->instrument[i].name);
        }
        if (itsd->n_samples) {
            if (itsd->sample)
                for (int i = 0, n = itsd->n_samples; i < n; i++) sprintf(mod_message,"%s%s\n",mod_message,itsd->sample[i].name);
        }
        
        //try to also load with modplug for pattern display
        mp_file=NULL;//ModPlug_LoadPat(mp_data,mp_datasize);
		if (mp_file==NULL) {
			free(mp_data);
		} else {
			mPatternDataAvail=1;
			numPatterns=ModPlug_NumPatterns(mp_file);
        }
        
		return 0;
    }
    if (found==15) { //timidity
		mPlayType=15;
        max_voices = voices = tim_max_voices;  //polyphony : MOVE TO SETTINGS
        set_current_resampler(tim_resampler);
        opt_reverb_control=tim_reverb;
        //set_current_resampler (RESAMPLE_LINEAR); //resample : MOVE TO SETTINGS
        
        if (mSlowDevice) {
            ios_play_mode.rate=22050;
        } else {
        }
        
		FILE *f=fopen([filePath UTF8String],"rb");
		if (f==NULL) {
			NSLog(@"timidity Cannot open file %@",filePath);
			mPlayType=0;
            
			return -1;
		}
		fseek(f,0L,SEEK_END);
		mp_datasize=ftell(f);
		fclose(f);
        
        
        mod_subsongs=1;
		mod_minsub=1;
		mod_maxsub=1;
		mod_currentsub=1;
        //		if (iModuleLength<=0) iModuleLength=MDX_DEFAULT_LENGTH;
        iModuleLength=-1;
        tim_midilength=-1;
        tim_lyrics_started=0;
        tim_pending_seek=-1;
		iCurrentTime=0;
        
		numChannels=2;
        
		sprintf(mod_name," %s",mod_filename);
		sprintf(mod_message,"Midi Infos:");
        
		//Loop
		if (mLoopMode==1) iModuleLength=-1;
        
        strcpy(tim_filepath,[filePath UTF8String]);
		      
		return 0;
	}
    if (found==16) { //PMD
        char tmp_mod_name[1024];
        tmp_mod_name[0] = 0;
        char tmp_mod_message[1024];
        tmp_mod_message[0] = 0;
        
        mPlayType=16;
        pmd_init();
        pmd_setrate(mSlowDevice?PLAYBACK_FREQ/2:PLAYBACK_FREQ); // 22kHz or 44.1kHz?
        
        FILE *f=fopen([filePath UTF8String],"rb");
        if (f==NULL) {
            NSLog(@"PMDMini Cannot open file %@",filePath);
            mPlayType=0;
            return -1;
        }
        
        fseek(f,0L,SEEK_END);
        mp_datasize=ftell(f);
        fclose(f);
        
        if (!pmd_is_pmd((char*)[filePath UTF8String])) {
            // not PMD; try AdPlug instead
            for (int i=0;i<[filetype_extADPLUG count];i++) {
                if ([extension caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=3;break;}
                if ([file_no_ext caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=3;break;}
			}
        } else {
            // doesn't actually play, just loads file into RAM & extracts data
            pmd_play((char*)[filePath UTF8String], (char*)[[filePath stringByDeletingLastPathComponent] UTF8String]);
            
            //iModuleLength=pmd_length_sec()*1000;
            // pmdmini's convenience pmd_get_length function returns seconds
            // w/ loss of precision; use pmdwinimport.h's getlength for ms
            int loop_length;
            getlength((char*)[filePath UTF8String], &iModuleLength, &loop_length);
            if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//PMD_DEFAULT_LENGTH;
            iCurrentTime=0;
            
            // these strings are SJIS
            pmd_get_title(tmp_mod_name);
            //printf("tmp_mod_name: %s",tmp_mod_name);
            if (tmp_mod_name[0]) sprintf(mod_name," %s",tmp_mod_name);
            else sprintf(mod_name," %s",mod_filename);
            
            pmd_get_compo(tmp_mod_message);
            if (tmp_mod_message[0]) sprintf(mod_message,"Title: %s\nComposer: %s",tmp_mod_name, tmp_mod_message);
            
            // PMD doesn't have subsongs
            mod_subsongs=1;
            mod_minsub=1;
            mod_maxsub=1;
            mod_currentsub=1;
            
            numChannels=pmd_get_tracks();
            
            //Loop
            if (mLoopMode==1) iModuleLength=-1;
            
            return 0;
        }
    }
    
	return 1;  //Could not find a lib to load module
}
//*****************************************
//Playback commands
-(void) selectPrevArcEntry {
	if (mdz_IsArchive&&mdz_ArchiveFilesCnt) {
		if (mdz_currentArchiveIndex) {
			[self Stop];
			mdz_currentArchiveIndex--;
		}
	}
}
-(void) selectNextArcEntry {
	if (mdz_IsArchive&&mdz_ArchiveFilesCnt) {
		if (mdz_currentArchiveIndex<(mdz_ArchiveFilesCnt-1)) {
			[self Stop];
			mdz_currentArchiveIndex++;
		}
	}
}
-(void) selectArcEntry:(int)arc_index{
	if (mdz_IsArchive&&mdz_ArchiveFilesCnt) {
		if ((arc_index>=0)&&(arc_index<mdz_ArchiveFilesCnt)) {
			[self Stop];
			mdz_currentArchiveIndex=arc_index;
		}
	}
}
-(void) playPrevSub{
	if (mod_subsongs<=1) return;
	moveToPrevSubSong=1;
}
-(void) playNextSub{
	if (mod_subsongs<=1) return;
	moveToNextSubSong=1;
}
-(void) playGoToSub:(int)sub_index{
	if (mod_subsongs<=1) return;
	if ((sub_index<mod_minsub)||(sub_index>mod_maxsub)) return;
	moveToSubSong=1;
	moveToSubSongIndex=sub_index;
}
-(void) Play {
	int counter=0;
	pthread_mutex_lock(&play_mutex);
	bGlobalSoundHasStarted=0;
	iCurrentTime=0;
	bGlobalAudioPause=0;
	[self iPhoneDrv_PlayStart];
	bGlobalEndReached=0;
	mChangeOfSong=0;
	bGlobalIsPlaying=1;
	//Ensure play has been taken into account
	//wait for sound generation thread to end
	if (1/*mSlowDevice*/) {
		while (bGlobalSoundHasStarted<SOUND_BUFFER_NB/2) {
			[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_UADE_MS];
			counter++;
			if (counter*DEFAULT_WAIT_TIME_UADE_MS>2) break;
		}
	}
	pthread_mutex_unlock(&play_mutex);
}
-(void) PlaySeek:(int)startPos subsong:(int)subsong {
    if (mPlayType!=15) { //hack for timidity : iModuleLength is unknown at this stage
        if (startPos>iModuleLength-SEEK_START_MARGIN_FROM_END) {
            startPos=iModuleLength-SEEK_START_MARGIN_FROM_END;
        }
    }
	if (startPos<0) startPos=0;
	
	switch (mPlayType) {
		case 1:  //GME
			if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
				
				gme_start_track(gme_emu,subsong);
				mod_currentsub=subsong;
				sprintf(mod_name," %s",mod_filename);
				if (gme_track_info( gme_emu, &gme_info, mod_currentsub )==0) {
					iModuleLength=gme_info->play_length;
					if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
					strcpy(gmetype,gme_info->system);
					sprintf(mod_message,"Song:%s\nGame:%s\nAuthor:%s\nDumper:%s\nCopyright:%s\nTracks:%d\n%s",
                            (gme_info->song?gme_info->song:" "),
                            (gme_info->game?gme_info->game:" "),
                            (gme_info->author?gme_info->author:" "),
                            (gme_info->dumper?gme_info->dumper:" "),
                            (gme_info->copyright?gme_info->copyright:" "),
                            gme_track_count( gme_emu ),
                            (gme_info->comment?gme_info->comment:" "));
					if (gme_info->song){
						if (gme_info->song[0]) sprintf(mod_name," %s/%s",(gme_info->game?gme_info->game:""),gme_info->song);
					}
					gme_free_info(gme_info);
				} else {
					strcpy(gmetype,"N/A");
					strcpy(mod_message,"N/A\n");
					iModuleLength=optGENDefaultLength;
				}
				//Loop
				if (mLoopMode==1) iModuleLength=-1;
				if (iModuleLength>optGMEFadeOut) gme_set_fade( gme_emu, iModuleLength-optGMEFadeOut ,optGMEFadeOut); //Fade 1s before end
				else gme_set_fade( gme_emu, 1<<30,optGMEFadeOut);
				mod_message_updated=2;
			}
			gme_seek(gme_emu,startPos);
			if (startPos) [self Seek:startPos];
            
			[self Play];
			iCurrentTime=startPos;
			break;
		case 2:  //MODPLUG
			if (startPos) [self Seek:startPos];
			[self Play];
			iCurrentTime=startPos;
			break;
		case 3:  //ADPLUG
			if (startPos) [self Seek:startPos];
			[self Play];
			iCurrentTime=startPos;
			break;
		case 4:  //AOSDK
			[self Play];
			if (startPos) [self Seek:startPos];
			break;
		case 5: //SexyPSF
			if (startPos) [self Seek:startPos];
			[self Play];
			break;
		case 6:  //UADE
			mod_wantedcurrentsub=subsong;
			if (startPos) [self Seek:startPos];
			[self Play];
			break;
		case 7://HVL/AHX
			mod_wantedcurrentsub=subsong;
			if (startPos) [self Seek:startPos];
			[self Play];
			break;
		case 8: //SID
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
				mod_currentsub=subsong;
			}
			if (mSidEngineType==1) {
				sidEmuInitializeSong(*mSid1EmuEngine,*mSid1Tune, mod_currentsub);
			} else {
				mSidTune->selectSong(mod_currentsub);
				mSidEmuEngine->load(mSidTune);
			}
			iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
			if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
			if (mLoopMode) iModuleLength=-1;
			
			mod_message_updated=1;
			if (startPos) [self Seek:startPos];
			[self Play];
			
			break;
		case 9:  //YM
			if (startPos) [self Seek:startPos];
			[self Play];
			break;
		case 10: //SC68
			if (startPos) [self Seek:startPos];
			if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
				mod_currentsub=subsong;
			}
			api68_music_info_t info;
			api68_play( sc68, mod_currentsub, 1);
			api68_music_info( sc68, &info, mod_currentsub, NULL );
			iModuleLength=info.time_ms;
			if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
			
			//Loop
			if (mLoopMode==1) iModuleLength=-1;
			
			
			//NSLog(@"track : %d, time : %d, start : %d",mod_currentsub,info.time_ms,info.start_ms);
			[self Play];
			break;
		case 11: //MDX
			[self Play];
			break;
		case 12: //GSF
			if (startPos) [self Seek:startPos];
			[self Play];
			break;
		case 13: //ASAP
			if (startPos) [self Seek:startPos];
			[self Play];
			break;
        case 14: //DUMB
			if (startPos) [self Seek:startPos];
			[self Play];
			break;
        case 15: //Timidity
			if (startPos) {
                //hack because midi length is unknow at this stage, preventing seek to work
                iCurrentTime=mNeedSeekTime=startPos;
                mNeedSeek=1;
            }//[self Seek:startPos];
			[self Play];
			break;
        case 16: //PMD
			[self Play];
			break;
        case 17: //VGM
            PlayVGM();
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case 18: //LAZYUSF
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
	}
}
-(void) Stop {
    
    if (mPlayType==15) { //Timidity
        intr = 1;
    }
    
	bGlobalIsPlaying=0;
	[self iPhoneDrv_PlayStop];
	
	//wait for sound generation thread to end
	while (bGlobalSoundGenInProgress) {
		[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
		//NSLog(@"Wait for end of thread");
	}
	bGlobalSeekProgress=0;
	bGlobalAudioPause=0;
	
	if (mPlayType==1) {
		gme_delete( gme_emu );
        gme_emu=NULL;
	}
	if (mPlayType==2) {
		if (mp_file) {
			ModPlug_Unload(mp_file);
		}
		if (mp_data) free(mp_data);
		mp_file=NULL;
	}
	if (mPlayType==3) {
		delete adPlugPlayer;
		adPlugPlayer=NULL;
		delete opl;
		opl=NULL;
        delete adplugDB;
        adplugDB=NULL;
	}
	if (mPlayType==4) {
		(*ao_types[ao_type].stop)();
		if (ao_buffer) free(ao_buffer);
	}
	if (mPlayType==5) { //SexyPSF
	}
	if (mPlayType==6) {  //UADE
		//		NSLog(@"Wait for end of UADE thread");
		while (uadeThread_running) {
			[NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
			
		}
		//		NSLog(@"ok");
		uade_unalloc_song(&UADEstate);
	}
	if (mPlayType==7) { //HVL
		hvl_FreeTune(hvl_song);
		hvl_song=NULL;
	}
	if (mPlayType==8) { //SID
		if (mSidTune) {
			delete mSidTune;
			mSidTune = NULL;
		}
		if (mSidEmuEngine) {
            sid2_config_t cfg = mSidEmuEngine->config();
            
            cfg.sidEmulation = NULL;
            mSidEmuEngine->config(cfg);
            delete mBuilder;
            mBuilder = NULL;
            
			delete mSidEmuEngine;
			mSidEmuEngine = NULL;
		}
		if (mSid1Tune) {
			delete mSid1Tune;
			mSid1Tune = NULL;
		}
		if (mSid1EmuEngine) {
			delete mSid1EmuEngine;
			mSid1EmuEngine = NULL;
		}
	}
	if (mPlayType==9) { //STSOUND
		ymMusicStop(ymMusic);
		ymMusicDestroy(ymMusic);
	}
	if (mPlayType==10) {//SC68
		api68_stop( sc68 );
		api68_close(sc68);
	}
	if (mPlayType==11) { //MDX
		mdx_close(mdx,pdx);
	}
	if (mPlayType==12) { //GSF
		GSFClose();
	}
	if (mPlayType==13) { //ASAP
        free(ASAP_module);
	}
    if (mPlayType==14) {
        if (mPatternDataAvail) {
            mPatternDataAvail=0;
            if (mp_file) {
                //ModPlug_UnloadPat(mp_file);
            }
            if (mp_data) free(mp_data);
            mp_file=NULL;
        }
        if (duh_player) {
            duh_end_sigrenderer(duh_player->dr);
            free(duh_player);duh_player=NULL;
        }
        if (duh) {
            unload_duh(duh); duh=NULL;
        }
    }
    if (mPlayType==15) { //VGM
        CloseVGMFile();
        VGMPlay_Deinit();
    }
    if (mPlayType==16) { //PMD
        pmd_stop();
    }
    if (mPlayType==17) { //VGM
        StopVGM();
    }
    if (mPlayType==18) { //LAZYUSF
        usf_shutdown(lzu_state->emu_state);
        free(lzu_sample_data);
        free(lzu_sample_data_float);
        free(lzu_sample_converted_data_float);
        free(usf_info_data);
        src_delete(src_state);
    }
    
}
-(void) Pause:(BOOL) paused {
    bGlobalAudioPause=(paused?1:0);
	//if (paused) [self iPhoneDrv_PlayStop];
	//else [self iPhoneDrv_PlayStart];
    if (paused) AudioQueuePause(mAudioQueue);// [self iPhoneDrv_PlayStop];
	else AudioQueueStart(mAudioQueue,NULL);//[self iPhoneDrv_PlayStart];
	
	
	mod_message_updated=1;
}
//*****************************************
//Playback infos
-(NSString*) getModMessage {
	NSString *modMessage;
	if ((mPlayType==1)||(mPlayType==4)||(mPlayType==5)||(mPlayType==11)||(mPlayType==12)||(mPlayType==16)||(mPlayType==17)||(mPlayType==18)) modMessage=[NSString stringWithCString:mod_message encoding:NSShiftJISStringEncoding];
	else {
		modMessage=[NSString stringWithCString:mod_message encoding:NSUTF8StringEncoding];
		if (modMessage==nil) modMessage=[NSString stringWithFormat:@"%s",mod_message];
	}
	if (modMessage==nil) return @"";
	return modMessage;
}
-(NSString*) getModName {
	NSString *modName;
	if ((mPlayType==1)||(mPlayType==4)||(mPlayType==5)||(mPlayType==11)||(mPlayType==12)||(mPlayType==16)||(mPlayType==17)||(mPlayType==18)) modName=[NSString stringWithCString:mod_name encoding:NSShiftJISStringEncoding];
	else {
		modName=[NSString stringWithCString:mod_name encoding:NSUTF8StringEncoding];
		if (modName==nil) modName=[NSString stringWithFormat:@"%s",mod_name];
	}
	if (modName==nil) return @"";
	return modName;
}
-(NSString*) getPlayerName {
	if (mPlayType==1) return @"Game Music Emulator";
	if (mPlayType==2) return @"OpenMPT";
	if (mPlayType==3) return @"Adplug";
	if (mPlayType==4) return @"Audio Overload";
	if (mPlayType==5) return @"SexyPSF";
	if (mPlayType==6) return @"UADE";
	if (mPlayType==7) return @"HVL";
	if (mPlayType==8) return (mSidEngineType==1?@"SIDPLAY1":@"SIDPLAY2/RESID");
	if (mPlayType==9) return @"STSOUND";
	if (mPlayType==10) return @"SC68";
	if (mPlayType==11) return @"MDX";
	if (mPlayType==12) return @"GSF";
	if (mPlayType==13) return @"ASAP";
    if (mPlayType==14) return @"DUMB";
    if (mPlayType==15) return @"Timidity";
    if (mPlayType==16) return @"PMDMini";
    if (mPlayType==17) return @"VGMPlay";
    if (mPlayType==18) return @"LazyUSF";
	return @"";
}
-(NSString*) getSubTitle:(int)subsong {
    NSString *result;
	if ((subsong<mod_minsub)||(subsong>mod_maxsub)) return @"";
	if (mPlayType==1) {
		if (gme_track_info( gme_emu, &gme_info, subsong )==0) {
			int sublen=gme_info->play_length;
			if (sublen<=0) sublen=optGENDefaultLength;
			
			if (gme_info->song){
				result=[NSString stringWithFormat:@"%d-%s %02d:%02d",subsong,gme_info->song,sublen/60000,(sublen/1000)%60];
			} else result=[NSString stringWithFormat:@"%d-%02d:%02d",subsong,gme_info->song,sublen/60000,(sublen/1000)%60];
			gme_free_info(gme_info);
            return result;
		}
	}
	return [NSString stringWithFormat:@"%d",subsong];
}
-(NSString*) getModType {
	if (mPlayType==1) {
		
		return [NSString stringWithFormat:@"%s",gmetype];
	}
	if (mPlayType==2) {
		switch ((unsigned int)ModPlug_GetModuleType(mp_file)) {
			case MOD_TYPE_MOD:return @"Amiga MODule";
			case MOD_TYPE_S3M:return @"Screamtracker 3";
			case MOD_TYPE_XM:return @"Fastracker 2";
			case MOD_TYPE_MED:return @"OctaMED";
			case MOD_TYPE_IT:return @"Impulse Tracker";
			case MOD_TYPE_669:return @"Composer/Unis 669";
			case MOD_TYPE_ULT:return @"UltraTracker";
			case MOD_TYPE_STM:return @"Screamtracker";
			case MOD_TYPE_FAR:return @"Farandole Composer";
			case MOD_TYPE_WAV:return @"wav";
			case MOD_TYPE_AMF:return @"amf";
			case MOD_TYPE_AMS:return @"ams";
			case MOD_TYPE_DSM:return @"dsm";
			case MOD_TYPE_MDL:return @"mdl";
			case MOD_TYPE_OKT:return @"OctaMED";
			case MOD_TYPE_MID:return @"mid";
			case MOD_TYPE_DMF:return @"dmf";
			case MOD_TYPE_DBM:return @"dbm";
			case MOD_TYPE_MT2:return @"mt2";
			case MOD_TYPE_AMF0:return @"amf0";
			case MOD_TYPE_PSM:return @"psm";
			case MOD_TYPE_J2B:return @"j2b";
			case MOD_TYPE_ABC:return @"abc";
			case MOD_TYPE_PAT:return @"pat";
			case MOD_TYPE_UMX:return @"umx";
			default:return @"???";
		}
	}
	if (mPlayType==3) return [NSString stringWithFormat:@"%s",(adPlugPlayer->gettype()).c_str()];
	if (mPlayType==4) return [NSString stringWithFormat:@"%s",ao_types[ao_type].name];
	if (mPlayType==5) return @"PSF";
	if (mPlayType==6) return [NSString stringWithFormat:@"%s",UADEstate.ep->playername];
	if (mPlayType==7) return (hvl_song->ht_ModType?@"HVL":@"AHX");
	if (mPlayType==8) return @"SID";
	if (mPlayType==9) return @"YM";
	if (mPlayType==10) {
		api68_music_info_t info;
		api68_music_info( sc68, &info, 1, NULL );
		
		return [NSString stringWithFormat:@"%s",info.replay];
	}
	if (mPlayType==11) {
		if (mdx->haspdx) return @"MDX/PDX";
		else return @"MDX";
	}
	if (mPlayType==12) return @"GSF";
	if (mPlayType==13) return @"ASAP";
	if (mPlayType==14) {
        const char * tag = duh_get_tag(duh, "FORMAT");
        if (tag && *tag) return [NSString stringWithFormat:@"%s",tag];
        else return @"mod?";
    }
    if (mPlayType==15) return @"MIDI";
    if (mPlayType==16) return @"PMD";
    if (mPlayType==17) return @"VGM";
    if (mPlayType==18) return @"USF";
	return @" ";
}
-(BOOL) isPlaying {
	if (bGlobalIsPlaying) return TRUE;
	else return FALSE;
	//if ([self isEndReached]) return FALSE;
	//else return TRUE;
}
-(int) isSeeking {
	if (bGlobalAudioPause) return 0;
	return bGlobalSeekProgress;
}
-(int) isPlayingTrackedMusic {
	if ((mPlayType==2)||(mPlayType==15)/*&&(bGlobalIsPlaying)*/) return 1;
	return 0;
}
-(BOOL) isEndReached{
	UInt32 i,datasize;
	datasize=sizeof(UInt32);
	AudioQueueGetProperty(mAudioQueue,kAudioQueueProperty_IsRunning,&i,&datasize);
	if (i==0) {
		return YES;
	}
	return NO;
}
//*****************************************
// Playback options

///////////////////////////
// SIDPLAY
///////////////////////////
-(void) optSIDFilter:(int)onoff {
    mSIDFilterON=onoff;
}

-(void) optSIDClock:(int)clockMode {
    sid_forceClock=clockMode;
}

-(void) optSIDModel:(int)modelMode {
    sid_forceModel=modelMode;
}

///////////////////////////
//SEXYPSF
///////////////////////////
-(void) optSEXYPSF:(int)reverb interpol:(int)interpol {
	sexyPSF_setReverb(reverb);
	sexyPSF_setInterpolation(interpol);
}

///////////////////////////
//GLOBAL
///////////////////////////
-(void) optGLOB_Panning:(int)onoff {
    mPanning=onoff;
}
-(void) optGLOB_PanningValue:(float)value {
    mPanningValue=(int)(value*128.0f);
}

///////////////////////////
//DUMB
///////////////////////////
-(void) optDUMB_MastVol:(float)value{
    if (value<0) value=0;
    if (value>1) value=1;
    dumb_MastVol=value*2;
    if (duh_player) {
        duh_player->volume = dumb_MastVol;
    }
}
-(void) optDUMB_Resampling:(int)value{
    if (value>3) value=3;
    if (value<0) value=0;
    dumb_resampling_quality=value;
}

///////////////////////////
//AOSDK
///////////////////////////
-(void) optAOSDK:(int)reverb interpol:(int)interpol {
	AOSDK_setReverb(reverb);
	AOSDK_setInterpolation(interpol);
}

-(void) optAOSDK_22KHZ:(int)value{
    aosdk_dsf_22khz=value;
}
-(void) optAOSDK_DSFDSP:(int)value{
    aosdk_dsfdsp=value;
}
-(void) optAOSDK_DSFEmuRatio:(int)value{
    switch (value) {
        default:
        case 0:aosdk_dsf_samplecycle_ratio=1;break;
        case 1:aosdk_dsf_samplecycle_ratio=5;break;
        case 2:aosdk_dsf_samplecycle_ratio=15;break;
        case 3:aosdk_dsf_samplecycle_ratio=30;break;
    }
}

-(void) optAOSDK_SSFDSP:(int)value{
    aosdk_ssfdsp=value;
}
-(void) optAOSDK_SSFEmuRatio:(int)value{
    switch (value) {
        default:
        case 0:aosdk_ssf_samplecycle_ratio=1;break;
        case 1:aosdk_ssf_samplecycle_ratio=5;break;
        case 2:aosdk_ssf_samplecycle_ratio=15;break;
        case 3:aosdk_ssf_samplecycle_ratio=30;break;
    }
}

///////////////////////////
// ADPLUG
///////////////////////////
-(void) optADPLUG:(int)opltype {
    mADPLUGopltype=opltype;
}

///////////////////////////
// GENERIC
///////////////////////////
-(void) optGEN_DefaultLength:(float_t)val {
	optGENDefaultLength=(int)(val*1000);
}

///////////////////////////
// TIMIDITY
///////////////////////////
-(void) optTIM_Poly:(int)val {
	tim_max_voices = val;
}
-(void) optTIM_Reverb:(int)val {
    if (val) tim_reverb=3;
    else tim_reverb=0;
}
-(void) optTIM_Chorus:(int)val {
    if (val) opt_chorus_control=1;
    else opt_chorus_control=0;
}
-(void) optTIM_LPFilter:(int)val {
    if (val) opt_lpf_def=1;
    else opt_lpf_def=0;
}
-(void) optTIM_Resample:(int)val {
    switch (val) {
        case 0:tim_resampler=RESAMPLE_NONE;
            break;
        case 1:tim_resampler=RESAMPLE_LINEAR;
            break;
        case 2:tim_resampler=RESAMPLE_CSPLINE;
            break;
        case 3:tim_resampler=RESAMPLE_GAUSS;
            break;
        case 4:tim_resampler=RESAMPLE_NEWTON;
            break;
        default:tim_resampler=RESAMPLE_NONE;
    }
    set_current_resampler(tim_resampler);
}
extern "C" int amplification;
extern "C" void adjust_master_volume(void);
extern "C" void adjust_amplification(void);
-(void) optTIM_Amplification:(int)val {
    amplification=val;
//    adjust_master_volume();
    adjust_amplification();
}

///////////////////////////
// GME
///////////////////////////
-(void) optGME_Fade:(int)fade {
    optGMEFadeOut=fade;
}

-(void) optGME_Ratio:(float)ratio isEnabled:(bool)enabled {
    optGMERatio = ratio;
    if(gme_emu) {
        if(!enabled) optGMERatio = 1;
        gme_set_tempo(gme_emu, optGMERatio);
    }
}

-(void) optGME_EQ:(double)treble bass:(double)bass {
    if (gme_emu) {
        gme_equalizer( gme_emu, &gme_eq );
        gme_eq.treble = treble;//-14; // -50.0 = muffled, 0 = flat, +5.0 = extra-crisp
        //bass
        //          def:80;  1 = full bass, 90 = average, 16000 = almost no bass
        //log10 ->  def:1,9;  0 - 4,2
        gme_eq.bass   = pow(10,4.2-bass);
        gme_set_equalizer( gme_emu, &gme_eq );
    }
}

-(void) optGME_FX:(int)enabled surround:(int)surround echo:(double)echo stereo:(double)stereo {
    if (gme_emu) {
        gme_fx.enabled=enabled;/* If 0, no effects are added */
        gme_fx.surround = surround; /* If 1, some channels are put in "back", using phase inversion */
        gme_fx.echo = echo;/* Amount of echo, where 0.0 = none, 1.0 = lots */
        gme_fx.stereo = stereo;/* Separation, where 0.0 = mono, 1.0 = hard left and right */
        gme_set_effects( gme_emu, &gme_fx);
    }
}

///////////////////////////
//UADE
///////////////////////////
-(void) optUADE_Led:(int)isOn {
	if (isOn!=mUADE_OptLED)	{
		mUADE_OptChange=1;
		mUADE_OptLED=isOn;
	}
}
-(void) optUADE_Norm:(int)isOn {
	mUADE_OptNORM=isOn;
	if (isOn) uade_effect_enable(&(UADEstate.effects), UADE_EFFECT_NORMALISE);
	else uade_effect_disable(&(UADEstate.effects), UADE_EFFECT_NORMALISE);
}
-(void) optUADE_PostFX:(int)isOn {
	mUADE_OptPOSTFX=isOn;
	if (isOn) uade_effect_enable(&(UADEstate.effects), UADE_EFFECT_ALLOW);
	else uade_effect_disable(&(UADEstate.effects), UADE_EFFECT_ALLOW);
}
-(void) optUADE_Pan:(int)isOn {
	mUADE_OptPAN=isOn;
	if (isOn) uade_effect_enable(&(UADEstate.effects), UADE_EFFECT_PAN);
	else uade_effect_disable(&(UADEstate.effects), UADE_EFFECT_PAN);
}
-(void) optUADE_Head:(int)isOn {
	mUADE_OptHEAD=isOn;
	if (isOn) uade_effect_enable(&(UADEstate.effects), UADE_EFFECT_HEADPHONES);
	else uade_effect_disable(&(UADEstate.effects), UADE_EFFECT_HEADPHONES);
}
-(void) optUADE_Gain:(int)isOn {
	mUADE_OptGAIN=isOn;
	if (isOn) uade_effect_enable(&(UADEstate.effects), UADE_EFFECT_GAIN);
	else uade_effect_disable(&(UADEstate.effects), UADE_EFFECT_GAIN);
}
-(void) optUADE_GainValue:(float)val {
	mUADE_OptGAINValue=val;
	uade_effect_gain_set_amount(&(UADEstate.effects), val);
}
-(void) optUADE_PanValue:(float)val {
	mUADE_OptPANValue=val;
	uade_effect_pan_set_amount(&(UADEstate.effects), val);
}

///////////////////////////
//MODPLUG
///////////////////////////
-(ModPlug_Settings*) getMPSettings {
	if (mPlayType==2) ModPlug_GetSettings(&mp_settings);
	return &mp_settings;
}
-(void) updateMPSettings {
	if (mPlayType==2) {
		ModPlug_SetSettings(mp_file,&mp_settings);
	}
}
-(void) setModPlugMasterVol:(float) mstVol {
	if ((mPlayType==2)&&(mp_file)) ModPlug_SetMasterVolume(mp_file,(int )(mstVol*256));
}

///////////////////////////
-(void) setLoopInf:(int)val {
	mLoopMode=val;
}
-(void) Seek:(int) seek_time {
	if ((mPlayType==4)||(mPlayType==6)||(mPlayType==8)
        ||(mPlayType==11)||(mPlayType==12)||(mPlayType==16)||mNeedSeek||(mPlayType==18)) return;
	
	if (mPlayType==9) {
		if (ymMusicIsSeekable(ymMusic)==YMFALSE) return;
	}
	
	if (seek_time>iModuleLength-SEEK_START_MARGIN_FROM_END) {
		seek_time=iModuleLength-SEEK_START_MARGIN_FROM_END;
		if (seek_time<0) seek_time=0;
	}
	
	iCurrentTime=mNeedSeekTime=seek_time;
	mNeedSeek=1;
}
//*****************************************
//Multisongs & archives
-(BOOL) isMultiSongs {
	if (mod_subsongs>1) return TRUE;
	return FALSE;
}
-(BOOL) isArchive {
	if (mdz_IsArchive) return TRUE;
	return FALSE;
}
-(int) getArcEntriesCnt {
    if (mdz_IsArchive==0) return 0;
	return mdz_ArchiveFilesCnt;
}
-(int) getArcIndex {
    if (mdz_IsArchive==0) return 0;
	return mdz_currentArchiveIndex;
}
-(NSString*) getArcEntryTitle:(int)arc_index {
	if ((arc_index>=0)&&(arc_index<mdz_ArchiveFilesCnt)) {
        //		return [NSString stringWithFormat:@"%s",mdz_ArchiveFilesList[arc_index]];
        return [NSString stringWithFormat:@"%s",mdz_ArchiveFilesListAlias[arc_index]];
	} else return @"";
	
}
-(int) getSongLength {
    return iModuleLength;
}
-(int) getCurrentTime {
    return iCurrentTime;
}
-(int) shouldUpdateInfos{
    return mod_message_updated;
}
-(void) setInfosUpdated {
    mod_message_updated=0;
}

-(int) getChannelVolume:(int)channel {
    int res;
    switch (mPlayType){
        case 2://Modplug
            res=ModPlug_GetChannelVolume( mp_file,channel);
            break;

        case 14://DUMB
        {
            DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer(duh_player->dr);
            res=dumb_it_sr_get_channel_volume(itsr,channel)*4;
        }
            break;
        default:res=0;
    }
    return res;
}

//*****************************************
@end
