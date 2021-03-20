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

int mod_total_length;


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

//SID2
#include "sidplayfp/sidplayfp.h"
#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidInfo.h"
#include "sidplayfp/SidTuneInfo.h"

#include "builders/residfp-builder/residfp.h"

static char **sidtune_title,**sidtune_name;




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
#define DEFAULT_XMP 3

#define DEFAULT_SAPASAP 0
#define DEFAULT_SAPGME 1

#define DEFAULT_VGMVGM 0
#define DEFAULT_VGMGME 1

static int mdz_IsArchive,mdz_ArchiveFilesCnt,mdz_currentArchiveIndex;
static int mdz_defaultMODPLAYER,mdz_defaultSAPPLAYER,mdz_defaultVGMPLAYER;
static char **mdz_ArchiveFilesList;
//static char **mdz_ArchiveFilesListAlias;

static NSFileManager *mFileMngr;

//GENERAL
static int mPanning,mPanningValue;


//2SF
static const XSFFile *xSFFile = nullptr;
static XSFPlayer *xSFPlayer = nullptr;
XSFConfig *xSFConfig = nullptr;
//SNSF
struct snsf_sound_out : public SNESSoundOut
{
    virtual ~snsf_sound_out() { }
    // Receives signed 16-bit stereo audio and a byte count
    virtual void write(const void * samples, unsigned long bytes)
    {
        /*sample_buffer.grow_size( bytes_in_buffer + bytes );
        memcpy( &sample_buffer[ bytes_in_buffer ], samples, bytes );
        bytes_in_buffer += bytes;*/
    }
};

auto xsfSampleBuffer = std::vector<uint8_t>(SOUND_BUFFER_SIZE_SAMPLE*2*2);

int optLAZYUSF_ResampleQuality=1;

extern "C" {
    //VGMPPLAY
    CHIPS_OPTION ChipOpts[0x02];
    bool EndPlay;
    
    //VGMSTREAM
#import "../libs/libvgmstream/vgmstream.h"
#import "../libs/libvgmstream/plugins.h"
    int mpg123_read(mpg123_handle *mh, unsigned char *out, size_t size, size_t *done);
    
    static mpg123_handle *mpg123h;
    static long mpg123_rate;
    static int mpg123_channel,mpg123_bytes;
    int optMPG123_resampleQuality=1;
    bool optMPG123_loopmode = false;
    double optMPG123_loop_count = 2.0f;
    static bool mMPG123_force_loop;
    static volatile int mMPG123_total_samples,mMPG123_seek_needed_samples,mMPG123_decode_pos_samples;
    
    
    static VGMSTREAM* vgmStream = NULL;
    static STREAMFILE* vgmFile = NULL;
    bool optVGMSTREAM_loopmode = false;
    double optVGMSTREAM_loop_count = 2.0f;
    int optVGMSTREAM_resampleQuality=1;
    
    static bool mVGMSTREAM_force_loop;
    static volatile int mVGMSTREAM_total_samples,mVGMSTREAM_seek_needed_samples,mVGMSTREAM_decode_pos_samples,mVGMSTREAM_totalinternal_samples;
    
    //xmp
#include "xmp.h"
    static xmp_context xmp_ctx;
    static struct xmp_module_info xmp_mi;
}

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
static sidplayfp *mSidEmuEngine;
sidbuilder *mBuilder;
//static ReSIDfpBuilder *mBuilder;
static SidTune *mSidTune;

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
extern "C" INT32 VGMMaxLoop,VGMMaxLoopM;


static const wchar_t* GetTagStrEJ(bool preferJTAG,const wchar_t* EngTag, const wchar_t* JapTag)
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
        if (! preferJTAG)
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
#include "internal/it.h"
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
static struct ASAP *asap;


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
extern char soundQuality;
extern int soundInterpolation;

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

#include "usf/misc.h"

int32_t lzu_sample_rate;
long lzu_currentSample,lzu_fadeStart,lzu_fadeLength;
#define LZU_SAMPLE_SIZE 1024
int16_t *lzu_sample_data;
float *lzu_sample_data_float;
float *lzu_sample_converted_data_float;




usf_loader_state * lzu_state;

static int16_t *vgm_sample_data;
static float *vgm_sample_data_float;
static float *vgm_sample_converted_data_float;
static char mmp_fileext[8];

static int16_t *mpg123_sample_data;
static float *mpg123_sample_data_float;
static float *mpg123_sample_converted_data_float;


//#include "corlett.h"
//extern corlett_t *usf_info_data;

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
#include "../utils/Resample/libsamplerate-0.1.8/src/samplerate.h"
}
#define SRC_DEFAULT_CONVERTER SRC_SINC_MEDIUM_QUALITY
double src_ratio;
SRC_STATE *src_state;
SRC_DATA src_data;

long src_callback_lazyusf(void *cb_data, float **data);
long src_callback_vgmstream(void *cb_data, float **data);
long src_callback_mpg123(void *cb_data, float **data);

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
#include "common/md5.h"
    
    
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
static volatile long mNeedSeek,mNeedSeekTime;

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
    
    auxfile = fopen([[NSString stringWithFormat:@"%@/%@",[NSString stringWithUTF8String:pathdir],[NSString stringWithUTF8String:filename]] UTF8String] , "rb");
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
    ModizMusicPlayer *mplayer=(__bridge ModizMusicPlayer*)data;
    if ( !  mplayer.mQueueIsBeingStopped ) {
        [mplayer iPhoneDrv_Update:mBuffer];
    }
}

/************************************************/
/* Handle phone calls interruptions             */
/************************************************/
void interruptionListenerCallback (void *inUserData,UInt32 interruptionState ) {
    ModizMusicPlayer *mplayer=(__bridge ModizMusicPlayer*)inUserData;
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
        ModizMusicPlayer *mplayer = (__bridge ModizMusicPlayer *) inUserData; // 6
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
                NSString *new_route = (__bridge NSString*)route;
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
@synthesize mCurrentSamples,mTgtSamples;
@synthesize mPlayType;
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
//AO stuff
@synthesize ao_buffer;
@synthesize ao_info;
//VGMPLAY stuff
@synthesize optVGMPLAY_maxloop,optVGMPLAY_ym2612emulator,optVGMPLAY_preferJapTag;
//Modplug stuff
@synthesize mp_settings;
@synthesize mp_file;
@synthesize mp_data;
@synthesize mVolume;
@synthesize numChannels,numPatterns,numSamples,numInstr,mPatternDataAvail;
@synthesize genRow,genPattern,/*genOffset,*/playRow,playPattern;//,playOffset;
@synthesize genVolData,playVolData;
//GSF
@synthesize optGSFsoundLowPass;
@synthesize optGSFsoundEcho;
@synthesize optGSFsoundQuality;//1:44Khz, 2:22Khz, 4:11Khz
@synthesize optGSFsoundInterpolation;
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


-(NSString*) wcharToNS:(const wchar_t*)wstr {
    return [[NSString alloc] initWithBytes:wstr length:wcslen(wstr)*sizeof(*wstr) encoding:NSUTF32LittleEndianStringEncoding];
}

-(NSString*) sjisToNS:(const char*)str {
    return [[NSString alloc] initWithBytes:str length:strlen(str)*sizeof(*str) encoding:NSShiftJISStringEncoding];
}


//*****************************************
//Internal playback functions
-(api68_t*)setupSc68 {
    api68_init_t init68;
    
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *path = [bundle resourcePath];
    NSMutableString *argData = [NSMutableString stringWithString:@"--sc68_data="];
    [argData appendString:path];
    //[argData appendString:@"/Replay"];    
    
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
                                (__bridge void*)self
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
                                         (__bridge void*)self                                                       // 5
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
        mCurrentSamples=0;
        
        mPanning=0;
        mPanningValue=64; //75%
        
        mdz_ArchiveFilesList=NULL;
//        mdz_ArchiveFilesListAlias=NULL;
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
        
        genVolData=(unsigned char*)malloc(SOUND_BUFFER_NB*SOUND_MAXMOD_CHANNELS);
        playVolData=(unsigned char*)malloc(SOUND_BUFFER_NB*SOUND_MAXMOD_CHANNELS);
        //playOffset=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        //
        
        
        //GME specific
        optGMEIgnoreSilence=0;
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
        mUADE_OptNTSC=0;
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
        
        mSidEmuEngine=NULL;
        mBuilder=NULL;
        mSidTune=NULL;
                
        sid_forceModel=0;
        sid_forceClock=0;
        //
        // SC68
        sc68 = [self setupSc68];
        //
        
        //ASAP
        asap = ASAP_New();
        //
        
        //VGMPLAY
        optVGMPLAY_maxloop = 2;
        optVGMPLAY_ym2612emulator=0;
        optVGMPLAY_preferJapTag=false;
        
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
    //[mFileMngr release];
    
    //[super dealloc];
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
                              (__bridge void*)self,
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
        memset(playVolData+i*SOUND_MAXMOD_CHANNELS,0,SOUND_MAXMOD_CHANNELS);
        memset(genVolData+i*SOUND_MAXMOD_CHANNELS,0,SOUND_MAXMOD_CHANNELS);
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
                memcpy(playVolData+buffer_ana_play_ofs*SOUND_MAXMOD_CHANNELS,genVolData+buffer_ana_play_ofs*SOUND_MAXMOD_CHANNELS,SOUND_MAXMOD_CHANNELS);
                //				playOffset[buffer_ana_play_ofs]=genOffset[buffer_ana_play_ofs];
            }
            if (mPlayType==MMP_TIMIDITY) {//Timidity
                memcpy(tim_notes_cpy[buffer_ana_play_ofs],tim_notes[buffer_ana_play_ofs],DEFAULT_VOICES*4);
                tim_voicenb_cpy[buffer_ana_play_ofs]=tim_voicenb[buffer_ana_play_ofs];
            }
            
            iCurrentTime+=1000.0f*SOUND_BUFFER_SIZE_SAMPLE/PLAYBACK_FREQ;
            
            if (mPlayType==MMP_SC68) {//SC68
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
    return (buffer_ana_play_ofs+1)%SOUND_BUFFER_NB;
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
    //NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    @autoreleasepool {
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
    //[pool release];
    }
    
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
                    printf("UADE: Song end (%s)\n", reason);
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

long src_callback_lazyusf(void *cb_data, float **data) {
    const char * result=usf_render(lzu_state->emu_state, lzu_sample_data, LZU_SAMPLE_SIZE, &lzu_sample_rate);
    if (result) return 0;
    lzu_currentSample+=LZU_SAMPLE_SIZE;
    if (iModuleLength>0)
    {
        if (lzu_currentSample>lzu_fadeStart) {
            int startSmpl=lzu_currentSample-lzu_fadeStart;
            if (startSmpl>LZU_SAMPLE_SIZE) startSmpl=LZU_SAMPLE_SIZE;
            int vol=lzu_fadeLength-(lzu_currentSample-lzu_fadeStart);
            if (vol<0) vol=0;
            for (int i=LZU_SAMPLE_SIZE-startSmpl;i<LZU_SAMPLE_SIZE;i++) {
                lzu_sample_data[i*2]=lzu_sample_data[i*2]*vol/lzu_fadeLength;
                lzu_sample_data[i*2+1]=lzu_sample_data[i*2+1]*vol/lzu_fadeLength;
                if (vol) vol--;
            }
        }
    }
    src_short_to_float_array (lzu_sample_data, lzu_sample_data_float,LZU_SAMPLE_SIZE*2);
    *data=lzu_sample_data_float;
    return LZU_SAMPLE_SIZE;
}

long src_callback_vgmstream(void *cb_data, float **data) {
    // render audio into sound buffer
    int16_t *sampleData;
    long sampleGenerated;
    int nbSamplesToRender=mVGMSTREAM_total_samples - mVGMSTREAM_decode_pos_samples;
    if (mVGMSTREAM_total_samples<0) {
        nbSamplesToRender=SOUND_BUFFER_SIZE_SAMPLE;
    }
    
    if (nbSamplesToRender >SOUND_BUFFER_SIZE_SAMPLE) {
        nbSamplesToRender = SOUND_BUFFER_SIZE_SAMPLE;
    }
    
    if (nbSamplesToRender<=0) return 0;
    
    sampleGenerated=nbSamplesToRender;
    
    int real_available_samples=mVGMSTREAM_totalinternal_samples-mVGMSTREAM_decode_pos_samples;
    if (mVGMSTREAM_total_samples<0) {
        real_available_samples=SOUND_BUFFER_SIZE_SAMPLE;
    }
    
    
    short int *snd_ptr;
    sampleData=vgm_sample_data;
    while (nbSamplesToRender) {
        if (nbSamplesToRender<=real_available_samples) {
            render_vgmstream(sampleData, nbSamplesToRender, vgmStream);
            mVGMSTREAM_decode_pos_samples+=nbSamplesToRender;
            nbSamplesToRender=0;
        } else {
            render_vgmstream(sampleData, real_available_samples, vgmStream);
            sampleData+=real_available_samples;
            mVGMSTREAM_decode_pos_samples+=real_available_samples;
            nbSamplesToRender-=real_available_samples;
            //end reached, looping from start
            reset_vgmstream(vgmStream);
            mVGMSTREAM_decode_pos_samples=0;
            real_available_samples=mVGMSTREAM_totalinternal_samples-mVGMSTREAM_decode_pos_samples;
        }
    }
    
    if (vgmStream->channels==1) {
        snd_ptr=vgm_sample_data;
        for (int i=sampleGenerated-1;i>=0;i--) {
            snd_ptr[i*2]=snd_ptr[i];
            snd_ptr[i*2+1]=snd_ptr[i];
        }
    }
    
    switch (vgmStream->channels) {
        case 1: //turno mono into stereo, L=R
            /*snd_ptr=vgm_sample_data;
            for (int i=nbSamplesToRender-1;i>=0;i--) {
                snd_ptr[i*2]=snd_ptr[i];
                snd_ptr[i*2+1]=snd_ptr[i];
            }
            mVGMSTREAM_decode_pos_samples+=nbSamplesToRender;*/
            break;
        case 2: //nothing to do
            break;
        case 3: //add 3rd channel in both L&R
            break;
        case 4: //assume order is L/R/L/R
            break;
        case 5:
            break;
        case 6:
            break;
        case 7:
            break;
        default:
            break;
    }
    
    
    src_short_to_float_array (vgm_sample_data, vgm_sample_data_float,sampleGenerated*2);
    *data=vgm_sample_data_float;
    
    return sampleGenerated;
}

    
long src_callback_mpg123(void *cb_data, float **data) {
    size_t done;
        // render audio into sound buffer
        int nbSamplesToRender=mMPG123_total_samples - mMPG123_decode_pos_samples;
        if (nbSamplesToRender >SOUND_BUFFER_SIZE_SAMPLE)
        {
            nbSamplesToRender = SOUND_BUFFER_SIZE_SAMPLE;
        }
        
        short int *snd_ptr;
        switch (mpg123_channel) {
            case 1:
                mpg123_read(mpg123h, (unsigned char *)mpg123_sample_data, nbSamplesToRender*mpg123_bytes, &done);
                snd_ptr=mpg123_sample_data;
                for (int i=nbSamplesToRender-1;i>=0;i--) {
                    snd_ptr[i*2]=snd_ptr[i];
                    snd_ptr[i*2+1]=snd_ptr[i];
                }
            break;
            case 2:
                mpg123_read(mpg123h, (unsigned char *)mpg123_sample_data, nbSamplesToRender*mpg123_bytes*2, &done);
            break;
            case 3:
            break;
            case 4:
            break;
            case 5:
            break;
            case 6:
            break;
            case 7:
            break;
            default:
            break;
        }
        mMPG123_decode_pos_samples+=nbSamplesToRender;
        
        src_short_to_float_array (mpg123_sample_data, mpg123_sample_data_float,nbSamplesToRender*2);
        *data=mpg123_sample_data_float;
        
        return nbSamplesToRender;
    }

    

-(void) generateSoundThread {
    //NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    @autoreleasepool {
            
    if ([[NSThread currentThread] respondsToSelector:@selector(setThreadPriority)]) [[NSThread currentThread] setThreadPriority:SND_THREAD_PRIO];
    
    
    while (1) {
        [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
        if (bGlobalIsPlaying) {
            bGlobalSoundGenInProgress=1;
            if ( !bGlobalEndReached && mPlayType) {
                int nbBytes=0;
                
                if (mPlayType==MMP_TIMIDITY) { //Special case : Timidity
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
                
                if (mPlayType==MMP_GSF) {  //Special case : GSF
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
                } else if (mPlayType==MMP_SEXYPSF) {  //Special case : SexyPSF
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
                    
                } else if (mPlayType==MMP_UADE) {  //Special case : UADE
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
                } else if (mPlayType==MMP_MDXPDX) {  //Special case : MDX
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
                        if (mPlayType==MMP_SIDPLAY) { //SID
                            long mSeekSamples=mNeedSeekTime*PLAYBACK_FREQ/1000;                            
                            bGlobalSeekProgress=-1;
                            if (mSeekSamples<mCurrentSamples) {
                                //reset
                                mSidTune->selectSong(mod_currentsub+1);
                                mSidEmuEngine->load(mSidTune);
                                mCurrentSamples=0;
                            }
                            mSidEmuEngine->fastForward( 100 * 32 );
                            while (mCurrentSamples+SOUND_BUFFER_SIZE_SAMPLE*32<=mSeekSamples) {
                                nbBytes=mSidEmuEngine->play(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*1)*2;
                                mCurrentSamples+=(nbBytes/4)*32;
                            }
                            mSidEmuEngine->fastForward( 100 );
                            
                            while (mCurrentSamples<mSeekSamples) {
                                nbBytes=mSidEmuEngine->play(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*1)*2;
                                mCurrentSamples+=(nbBytes/4);
                            }
                        }
                        if (mPlayType==MMP_GME) {   //GME
                            bGlobalSeekProgress=-1;
                            gme_seek(gme_emu,mNeedSeekTime);
                            //gme_set_fade( gme_emu, iModuleLength-optGMEFadeOut,optGMEFadeOut ); //Fade 2s before end
                        }
                        if (mPlayType==MMP_OPENMPT) { //MODPLUG
                            bGlobalSeekProgress=-1;
                            ModPlug_Seek(mp_file,mNeedSeekTime);
                        }
                        if (mPlayType==MMP_XMP) { //XMP
                            bGlobalSeekProgress=-1;
                            xmp_seek_time(xmp_ctx,mNeedSeekTime);
                        }
                        if (mPlayType==MMP_ADPLUG) { //ADPLUG
                            bGlobalSeekProgress=-1;
                            adPlugPlayer->seek(mNeedSeekTime);
                        }
                        if (mPlayType==MMP_AOSDK) { //AOSDK : not supported
                            mNeedSeek=0;
                        }
                        if (mPlayType==MMP_HVL) { //HVL
                            bGlobalSeekProgress=-1;
                            mNeedSeekTime=hvl_Seek(hvl_song,mNeedSeekTime);
                        }
                        if (mPlayType==MMP_STSOUND) {//STSOUND
                            if (ymMusicIsSeekable(ymMusic)==YMTRUE) {
                                bGlobalSeekProgress=-1;
                                ymMusicSeek(ymMusic,mNeedSeekTime);
                            } else mNeedSeek=0;
                        }
                        if (mPlayType==MMP_SC68) {//SC68
                            bGlobalSeekProgress=-1;
                            api68_seek(sc68, mNeedSeekTime, NULL);
                        }
                        if (mPlayType==MMP_ASAP) { //ASAP
                            bGlobalSeekProgress=-1;
                            ASAP_Seek(asap, mNeedSeekTime);
                        }
                        if (mPlayType==MMP_DUMB) {//DUMB
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
                        if (mPlayType==MMP_PMDMINI) { //PMDMini : not supported
                            mNeedSeek=0;
                        }
                        if (mPlayType==MMP_VGMPLAY) { //VGM
                            bGlobalSeekProgress=-1;
                            SeekVGM(false,mNeedSeekTime*441/10);
                            //mNeedSeek=0;
                        }
                        if (mPlayType==MMP_LAZYUSF) { //LAZYUSF
                            int seekSample=(double)mNeedSeekTime*(double)(lzu_sample_rate)/1000.0f;
                            bGlobalSeekProgress=-1;
                            if (lzu_currentSample >seekSample) {
                                usf_restart(lzu_state->emu_state);
                                lzu_currentSample=0;
                            }
                            
                            while (seekSample - lzu_currentSample > SOUND_BUFFER_SIZE_SAMPLE) {
                                const char * result=usf_render(lzu_state->emu_state, lzu_sample_data, LZU_SAMPLE_SIZE, &lzu_sample_rate);
                                lzu_currentSample+=LZU_SAMPLE_SIZE;
                            }
                            if (seekSample - lzu_currentSample > 0)
                            {
                                const char * result=usf_render(lzu_state->emu_state, lzu_sample_data, seekSample - lzu_currentSample, &lzu_sample_rate);
                                lzu_currentSample=seekSample;
                            }
                        }
                        if (mPlayType==MMP_2SF) { //2SF
                            
                            int seekSample=(double)mNeedSeekTime*(double)(xSFPlayer->GetSampleRate())/1000.0f;
                            bGlobalSeekProgress=-1;
                            if (xSFPlayer->currentSample >seekSample) {
                                xSFPlayer->Terminate();
                                xSFPlayer->Load();
                                xSFPlayer->SeekTop();
                            }
                            
                            while (seekSample - xSFPlayer->currentSample > SOUND_BUFFER_SIZE_SAMPLE) {
                                xSFPlayer->GenerateSamples(xsfSampleBuffer, 0, SOUND_BUFFER_SIZE_SAMPLE);
                                xSFPlayer->currentSample += SOUND_BUFFER_SIZE_SAMPLE;
                            }
                            if (seekSample - xSFPlayer->currentSample > 0)
                            {
                                xSFPlayer->GenerateSamples(xsfSampleBuffer, 0, seekSample - xSFPlayer->currentSample);
                                xSFPlayer->currentSample = seekSample;
                            }
                            
                            //mNeedSeek=0;
                        }
                        if (mPlayType==MMP_SNSF) { //SNSF
                            
                            /*int seekSample=(double)mNeedSeekTime*(double)(xSFPlayerSNSF->GetSampleRate())/1000.0f;
                            bGlobalSeekProgress=-1;
                            if (xSFPlayerSNSF->currentSample >seekSample) {
                                xSFPlayerSNSF->Terminate();
                                xSFPlayerSNSF->Load();
                                xSFPlayerSNSF->SeekTop();
                            }
                            
                            while (seekSample - xSFPlayerSNSF->currentSample > SOUND_BUFFER_SIZE_SAMPLE) {
                                xSFPlayerSNSF->GenerateSamples(xsfSampleBuffer, 0, SOUND_BUFFER_SIZE_SAMPLE);
                                xSFPlayerSNSF->currentSample += SOUND_BUFFER_SIZE_SAMPLE;
                            }
                            if (seekSample - xSFPlayerSNSF->currentSample > 0)
                            {
                                xSFPlayerSNSF->GenerateSamples(xsfSampleBuffer, 0, seekSample - xSFPlayerSNSF->currentSample);
                                xSFPlayerSNSF->currentSample = seekSample;
                            }*/
                            
                            //mNeedSeek=0;
                        }
                        if (mPlayType==MMP_VGMSTREAM) { //VGMSTREAM
                            mVGMSTREAM_seek_needed_samples=(double)mNeedSeekTime*(double)(vgmStream->sample_rate)/1000.0f;
                            bGlobalSeekProgress=-1;
                            if (mVGMSTREAM_decode_pos_samples>mVGMSTREAM_seek_needed_samples) {
                                reset_vgmstream(vgmStream);
                                mVGMSTREAM_decode_pos_samples=0;
                            }
                            while (mVGMSTREAM_decode_pos_samples<mVGMSTREAM_seek_needed_samples) {
                                int nbSamplesToRender=mVGMSTREAM_seek_needed_samples-mVGMSTREAM_decode_pos_samples;
                                if (nbSamplesToRender>SOUND_BUFFER_SIZE_SAMPLE) nbSamplesToRender=SOUND_BUFFER_SIZE_SAMPLE;
                                render_vgmstream(vgm_sample_data, nbSamplesToRender, vgmStream);
                                mVGMSTREAM_decode_pos_samples+=nbSamplesToRender;
                            }
                            
                            //mNeedSeek=0;
                        }
                        if (mPlayType==MMP_MPG123) { //MPG123
                            mMPG123_seek_needed_samples=(double)mNeedSeekTime*(double)(mpg123_rate)/1000.0f;
                            bGlobalSeekProgress=-1;
                            if (mMPG123_decode_pos_samples>mMPG123_seek_needed_samples) {
                                //reset_vgmstream(vgmStream);
                                mpg123_seek(mpg123h,0,SEEK_SET);
                                mMPG123_decode_pos_samples=0;
                            }
                            while (mMPG123_decode_pos_samples<mMPG123_seek_needed_samples) {
                                int nbSamplesToRender=mMPG123_seek_needed_samples-mMPG123_decode_pos_samples;
                                size_t done;
                                if (nbSamplesToRender>SOUND_BUFFER_SIZE_SAMPLE) nbSamplesToRender=SOUND_BUFFER_SIZE_SAMPLE;
                                mpg123_read(mpg123h, (unsigned char *)mpg123_sample_data, nbSamplesToRender*mpg123_bytes*mpg123_channel, &done);
                                mMPG123_decode_pos_samples+=nbSamplesToRender;
                            }
                            
                            //mNeedSeek=0;
                        }
                    }
                    if (moveToNextSubSong) {
                        if (mod_currentsub<mod_maxsub) {
                            mod_currentsub++;
                            mod_message_updated=1;
                            if (mPlayType==MMP_OPENMPT) {
                                openmpt_module_select_subsong(mp_file->mod, mod_currentsub);
                                iModuleLength=openmpt_module_get_duration_seconds( mp_file->mod )*1000;
                                iCurrentTime=0;
                                numChannels=openmpt_module_get_num_channels(mp_file->mod);  //should not change in a subsong
                                
                                if (moveToNextSubSong==2) {
                                    //[self iPhoneDrv_PlayWaitStop];
                                    //[self iPhoneDrv_PlayStart];
                                } else {
                                    [self iPhoneDrv_PlayStop];
                                    [self iPhoneDrv_PlayStart];
                                }
                                //if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                mod_message_updated=1;
                            }
                            if (mPlayType==MMP_GME) {//GME
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
                                        if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);
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
                            if (mPlayType==MMP_SIDPLAY) { //SID
                                mSidTune->selectSong(mod_currentsub+1);
                                mSidEmuEngine->load(mSidTune);
                            
                                if (moveToNextSubSong==2) {
                                    //[self iPhoneDrv_PlayWaitStop];
                                    //[self iPhoneDrv_PlayStart];
                                } else {
                                    [self iPhoneDrv_PlayStop];
                                    [self iPhoneDrv_PlayStart];
                                }
                                iCurrentTime=0;
                                mCurrentSamples=0;
                                iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
                                mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
                                if (mLoopMode) iModuleLength=-1;
                                mod_message_updated=1;
                            }
                            if (mPlayType==MMP_SC68) {//SC68
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
                            if (mPlayType==MMP_ASAP) {//ASAP
                                iModuleLength=asap->moduleInfo.durations[mod_currentsub];
                                ASAP_PlaySong(asap, mod_currentsub, iModuleLength);
                                
                                
                                iCurrentTime=0;
                                numChannels=asap->moduleInfo.channels;
                                
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
                        if (mPlayType==MMP_OPENMPT) {
                            openmpt_module_select_subsong(mp_file->mod, mod_currentsub);
                            iModuleLength=openmpt_module_get_duration_seconds( mp_file->mod )*1000;
                            iCurrentTime=0;
                            numChannels=openmpt_module_get_num_channels(mp_file->mod);  //should not change in a subsong
                            
                            if (moveToNextSubSong==2) {
                                //[self iPhoneDrv_PlayWaitStop];
                                //[self iPhoneDrv_PlayStart];
                            } else {
                                [self iPhoneDrv_PlayStop];
                                [self iPhoneDrv_PlayStart];
                            }
                            //if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                            if (mLoopMode) iModuleLength=-1;
                            mod_message_updated=1;
                        }
                        if (mPlayType==MMP_GME) {//GME
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
                                    if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);
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
                        if (mPlayType==MMP_SIDPLAY) { //SID
                            mSidTune->selectSong(mod_currentsub+1);
                            mSidEmuEngine->load(mSidTune);
                            
                            if (moveToSubSong==2) {
                                //[self iPhoneDrv_PlayWaitStop];
                                //[self iPhoneDrv_PlayStart];
                            } else {
                                [self iPhoneDrv_PlayStop];
                                [self iPhoneDrv_PlayStart];
                            }
                            iCurrentTime=0;
                            mCurrentSamples=0;
                            iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
                            if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
                            mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
                            if (mLoopMode) iModuleLength=-1;
                            mod_message_updated=1;
                        }
                        if (mPlayType==MMP_SC68) {//SC68
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
                        if (mPlayType==MMP_ASAP) {//ASAP
                            iModuleLength=asap->moduleInfo.durations[mod_currentsub];
                            ASAP_PlaySong(asap, mod_currentsub, iModuleLength);
                            
                            
                            iCurrentTime=0;
                            numChannels=asap->moduleInfo.channels;
                            
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
                            if (mPlayType==MMP_OPENMPT) {
                                openmpt_module_select_subsong(mp_file->mod, mod_currentsub);
                                iModuleLength=openmpt_module_get_duration_seconds( mp_file->mod )*1000;
                                iCurrentTime=0;
                                numChannels=openmpt_module_get_num_channels(mp_file->mod);  //should not change in a subsong
                                
                                if (moveToNextSubSong==2) {
                                    //[self iPhoneDrv_PlayWaitStop];
                                    //[self iPhoneDrv_PlayStart];
                                } else {
                                    [self iPhoneDrv_PlayStop];
                                    [self iPhoneDrv_PlayStart];
                                }
                                //if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                mod_message_updated=1;
                            }
                            if (mPlayType==MMP_GME) {//GME
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
                                        if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);
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
                            if (mPlayType==MMP_SIDPLAY) { //SID
                                mSidTune->selectSong(mod_currentsub+1);
                                mSidEmuEngine->load(mSidTune);
                                
                                [self iPhoneDrv_PlayStop];
                                [self iPhoneDrv_PlayStart];
                                iCurrentTime=0;
                                mCurrentSamples=0;
                                iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
                                mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
                                if (mLoopMode) iModuleLength=-1;
                                mod_message_updated=1;
                            }
                            if (mPlayType==MMP_SC68) {//SC68
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
                            if (mPlayType==MMP_ASAP) {//ASAP
                                iModuleLength=asap->moduleInfo.durations[mod_currentsub];
                                ASAP_PlaySong(asap, mod_currentsub, iModuleLength);
                                
                                
                                iCurrentTime=0;
                                numChannels=asap->moduleInfo.channels;
                                
                                [self iPhoneDrv_PlayStop];
                                [self iPhoneDrv_PlayStart];
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                
                                mod_message_updated=1;
                            }
                        }
                    }
                    
                    if (mPlayType==MMP_GME) {  //GME
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
                                            if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);										}
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
                    if (mPlayType==MMP_AOSDK) { //AOSDK
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
                    if (mPlayType==MMP_XMP) {  //XMP
                        if (xmp_play_buffer(xmp_ctx, buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE*2*2, 1) == 0) {
                            struct xmp_frame_info xmp_fi;
                            xmp_get_frame_info(xmp_ctx, &xmp_fi);
                            
                            nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                            
                            genPattern[buffer_ana_gen_ofs]=xmp_fi.pattern;
                            genRow[buffer_ana_gen_ofs]=xmp_fi.row;
                            
                            for (int i=0;i<numChannels;i++) {
                                int v=xmp_channel_vol(xmp_ctx, i,-1);
                                genVolData[buffer_ana_gen_ofs*SOUND_MAXMOD_CHANNELS+i]=(v>255?255:v);
                            }
                            
                        } else {
                            NSLog(@"XMP: end of song");
                            nbBytes=0;
                        }
                        
                    }
                    if (mPlayType==MMP_OPENMPT) {  //MODPLUG
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
                            genVolData[buffer_ana_gen_ofs*SOUND_MAXMOD_CHANNELS+i]=(v>255?255:v);
                        }
                        
                        if (mChangeOfSong==0) {
                            if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (mLoopMode==0)&&(iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if ((mSingleSubMode==0)&&(mod_currentsub<mod_maxsub)) {
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                    mod_currentsub++;
                                    
                                    openmpt_module_select_subsong(mp_file->mod, mod_currentsub);
                                    
                                    mChangeOfSong=1;
                                    mNewModuleLength=openmpt_module_get_duration_seconds(mp_file->mod)*1000;
                                    if (mLoopMode) mNewModuleLength=-1;
                                } else {
                                    nbBytes=0;
                                }
                            }
                        }
                        
                    }
                    if (mPlayType==MMP_PMDMINI) { //PMD
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
                    if (mPlayType==MMP_VGMPLAY) { //VGM
                        // render audio into sound buffer
                        // TODO does this work OK on mSlowDevices?
                        if (EndPlay) {
                            nbBytes=0;
                        } else nbBytes=VGMFillBuffer((WAVE_16BS*)(buffer_ana[buffer_ana_gen_ofs]), SOUND_BUFFER_SIZE_SAMPLE)*2*2;
                    }
                    if (mPlayType==MMP_VGMSTREAM) { //VGMSTREAM
                        
                        nbBytes=src_callback_read (src_state,src_ratio,SOUND_BUFFER_SIZE_SAMPLE, vgm_sample_converted_data_float)*2*2;
                        src_float_to_short_array (vgm_sample_converted_data_float,buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2) ;
                        if (mVGMSTREAM_total_samples>=0) {
                            if (mVGMSTREAM_decode_pos_samples>=mVGMSTREAM_total_samples) nbBytes=0;
                            if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                        }
                    }
                    if (mPlayType==MMP_MPG123) { //MPG123
                        
                        nbBytes=src_callback_read (src_state,src_ratio,SOUND_BUFFER_SIZE_SAMPLE, mpg123_sample_converted_data_float)*2*2;
                        src_float_to_short_array (mpg123_sample_converted_data_float,buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2) ;
                        if (mMPG123_decode_pos_samples>=mMPG123_total_samples) nbBytes=0;
                        //if (nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2) NSLog(@"nbBytes %d",nbBytes);
                        if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                    }
                    if (mPlayType==MMP_LAZYUSF) { //LAZYUSF
                        nbBytes=src_callback_read (src_state,src_ratio,SOUND_BUFFER_SIZE_SAMPLE, lzu_sample_converted_data_float)*2*2;
                        src_float_to_short_array (lzu_sample_converted_data_float,buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2) ;
                        
                        if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                    }
                    if (mPlayType==MMP_2SF) { //2SF
                        bool done;
                        unsigned int samplesWritten;
                        
                        done= xSFPlayer->FillBuffer(xsfSampleBuffer,samplesWritten);
                        if (done) nbBytes=0;
                        else nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                        memcpy((char*)(buffer_ana[buffer_ana_gen_ofs]),reinterpret_cast<char *>(&xsfSampleBuffer[0]),SOUND_BUFFER_SIZE_SAMPLE*2*2);
                        
                        //if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                        
                    }
                    if (mPlayType==MMP_SNSF) { //SNSF
                        /*bool done;
                        unsigned int samplesWritten;
                        
                        done= xSFPlayerSNSF->FillBuffer(xsfSampleBuffer,samplesWritten);
                        if (done) nbBytes=0;
                        else nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                        memcpy((char*)(buffer_ana[buffer_ana_gen_ofs]),reinterpret_cast<char *>(&xsfSampleBuffer[0]),SOUND_BUFFER_SIZE_SAMPLE*2*2);
                        */
                        //if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                        
                    }
                    if (mPlayType==MMP_ADPLUG) {  //ADPLUG
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
                    if (mPlayType==MMP_HVL) {  //HVL
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
                    if (mPlayType==MMP_SIDPLAY) { //SID
                        nbBytes=mSidEmuEngine->play(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*1)*2;
                        mCurrentSamples+=nbBytes/4;
                        if (mChangeOfSong==0) {
                            if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (mLoopMode==0)&&(iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if ((mSingleSubMode==0)&&(mod_currentsub<mod_maxsub)) {
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                    mod_currentsub++;
                                    
                                    mSidTune->selectSong(mod_currentsub+1);
                                    mSidEmuEngine->load(mSidTune);
                                    
                                    mChangeOfSong=1;
                                    mCurrentSamples=0;
                                    mNewModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
                                    if (mNewModuleLength<=0) mNewModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
                                    mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
                                    if (mLoopMode) mNewModuleLength=-1;
                                } else {
                                    nbBytes=0;
                                }
                            } else if (iModuleLength<0) {
                                if (mCurrentSamples>=mTgtSamples) {
                                    //loop
                                    mSidTune->selectSong(mod_currentsub+1);
                                    mSidEmuEngine->load(mSidTune);
                                    mCurrentSamples=0;
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                }
                            }
                        }
                        
                    }
                    if (mPlayType==MMP_STSOUND) { //STSOUND
                        int nbSample = SOUND_BUFFER_SIZE_SAMPLE;
                        if (ymMusicComputeStereo((void*)ymMusic,(ymsample*)buffer_ana[buffer_ana_gen_ofs],nbSample)==YMTRUE) nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                        else nbBytes=0;
                    }
                    if (mPlayType==MMP_SC68) {//SC68
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
                    if (mPlayType==MMP_ASAP) { //ASAP                        
                        if (asap->moduleInfo.channels==2) {
                            nbBytes= ASAP_Generate(asap, (unsigned char *)buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE*2*2, ASAPSampleFormat_S16_L_E);
                        } else {
                            nbBytes= ASAP_Generate(asap, (unsigned char *)buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE*2, ASAPSampleFormat_S16_L_E);
                            short int *buff=(short int*)(buffer_ana[buffer_ana_gen_ofs]);
                            for (int i=nbBytes-1;i>0;i--) {
                                buff[i]=buff[i>>1];
                            }
                            nbBytes*=2;
                        }
                        
                        if (mChangeOfSong==0) {
                            if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if ((mSingleSubMode==0)&&(mod_currentsub<mod_maxsub)) {
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                    mod_currentsub++;
                                    
                                    mNewModuleLength=asap->moduleInfo.durations[mod_currentsub];
                                    ASAP_PlaySong(asap, mod_currentsub, mNewModuleLength);
                                    
                                    mChangeOfSong=1;
                                    if (mNewModuleLength<=0) mNewModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
                                    if (mLoopMode) mNewModuleLength=-1;
                                } else nbBytes=0;
                            }
                        }
                        
                    }
                    if (mPlayType==MMP_DUMB) {//DUMB
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
                                    genVolData[buffer_ana_gen_ofs*SOUND_MAXMOD_CHANNELS+i]=(v>255?255:v);
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
    //[pool release];
    }
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
//            mdz_ArchiveFilesListAlias=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*)); //TODO: free
            idx=0;
            while ( !fex_done( fex ) ) {
                
                if ([self isAcceptedFile:[NSString stringWithUTF8String:fex_name(fex)] no_aux_file:0]) {
                    
                    fex_stat(fex);
                    arc_size=fex_size(fex);
                    extractFilename=[NSString stringWithFormat:@"%s/%@",extractPath,[NSString stringWithUTF8String:fex_name(fex)]];
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
                        archive_data=(char*)malloc(512*1024*1024); //buffer
                        while (arc_size) {
                            if (arc_size>512*1024*1024) {
                                fex_read( fex, archive_data, 512*1024*1024);
                                fwrite(archive_data,512*1024*1024,1,f);
                                arc_size-=512*1024*1024;
                            } else {
                                fex_read( fex, archive_data, arc_size );
                                fwrite(archive_data,arc_size,1,f);
                                arc_size=0;
                            }
                        }
                        free(archive_data);
                        fclose(f);
                        
                        NSString *tmp_filename=[NSString stringWithUTF8String:fex_name(fex)];
                        
                        if ([self isAcceptedFile:[NSString stringWithUTF8String:fex_name(fex)] no_aux_file:1]) {
                            mdz_ArchiveFilesList[idx]=(char*)malloc(strlen([tmp_filename fileSystemRepresentation])+1);
                            strcpy(mdz_ArchiveFilesList[idx],[tmp_filename fileSystemRepresentation]);
                            
/*                            NSRange r;
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
                                //                                NSLog(@"name: %s",mdz_ArchiveFilesListAlias[idx]);
                                free(archive_data);
                            } else {*/
//                                mdz_ArchiveFilesListAlias[idx]=(char*)malloc(strlen([[tmp_filename lastPathComponent] UTF8String])+1);
//                                strcpy(mdz_ArchiveFilesListAlias[idx],[[tmp_filename lastPathComponent] UTF8String]);
                                //                                NSLog(@"name def: %s",mdz_ArchiveFilesListAlias[idx]);
//                            }
                            
                            
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
//            mdz_ArchiveFilesListAlias=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*)); //TODO: free
            idx=0;
            while ( !fex_done( fex ) ) {
                
                if ([self isAcceptedFile:[NSString stringWithUTF8String:fex_name(fex)] no_aux_file:1]) {
                    if (index==idx) {
                        fex_stat(fex);
                        arc_size=fex_size(fex);
                        extractFilename=[NSString stringWithFormat:@"%s/%@",extractPath,[NSString stringWithUTF8String:fex_name(fex)]];
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
                            archive_data=(char*)malloc(32*1024*1024); //buffer
                            while (arc_size) {
                                if (arc_size>32*1024*1024) {
                                    fex_read( fex, archive_data, 32*1024*1024);
                                    fwrite(archive_data,32*1024*1024,1,f);
                                    arc_size-=32*1024*1024;
                                } else {
                                    fex_read( fex, archive_data, arc_size );
                                    fwrite(archive_data,arc_size,1,f);
                                    arc_size=0;
                                }
                            }
                            free(archive_data);
                            fclose(f);
                            NSString *tmp_filename=[NSString stringWithUTF8String:fex_name(fex)];
                            
                            mdz_ArchiveFilesList[0]=(char*)malloc(strlen([tmp_filename fileSystemRepresentation])+1);
                            strcpy(mdz_ArchiveFilesList[0],[tmp_filename fileSystemRepresentation]);
//                            mdz_ArchiveFilesListAlias[0]=(char*)malloc(strlen([[tmp_filename lastPathComponent] UTF8String])+1);
//                            strcpy(mdz_ArchiveFilesListAlias[0],[[tmp_filename lastPathComponent] UTF8String]);
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
                if ([self isAcceptedFile:[NSString stringWithUTF8String:fex_name(fex)] no_aux_file:1]) {
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
                if ([self isAcceptedFile:[NSString stringWithUTF8String:fex_name(fex)] no_aux_file:1]) {
                    if (!idx) {
                        NSString *result=[NSString stringWithUTF8String:fex_name(fex)];
                        fex_close( fex );
                        return result;
                    }
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
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
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
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
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
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
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
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else NSLog(@"ErrSQL : %d",err);
        
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
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extDUMB=[SUPPORTED_FILETYPE_DUMB componentsSeparatedByString:@","];
    NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extSEXYPSF=[SUPPORTED_FILETYPE_SEXYPSF componentsSeparatedByString:@","];
    NSArray *filetype_extLAZYUSF=[SUPPORTED_FILETYPE_LAZYUSF componentsSeparatedByString:@","];
    NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","];
    NSArray *filetype_extSNSF=[SUPPORTED_FILETYPE_SNSF componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSArray *filetype_extMPG123=[SUPPORTED_FILETYPE_MPG123 componentsSeparatedByString:@","];
    NSArray *filetype_extAOSDK=[SUPPORTED_FILETYPE_AOSDK componentsSeparatedByString:@","];
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    NSMutableArray *filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extMDX count]+[filetype_extSID count]+
                                  [filetype_extSTSOUND count]+[filetype_extPMD count]+
                                  [filetype_extSC68 count]+[filetype_extARCHIVE count]+[filetype_extUADE count]+[filetype_extMODPLUG count]+[filetype_extXMP count]+[filetype_extDUMB count]+
                                  [filetype_extGME count]+[filetype_extADPLUG count]+[filetype_extSEXYPSF count]+[filetype_extLAZYUSF count]+[filetype_ext2SF count]+[filetype_extSNSF count]+[filetype_extVGMSTREAM count]+[filetype_extMPG123 count]+
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
    [filetype_ext addObjectsFromArray:filetype_extXMP];
    [filetype_ext addObjectsFromArray:filetype_extDUMB];
    [filetype_ext addObjectsFromArray:filetype_extGME];
    [filetype_ext addObjectsFromArray:filetype_extADPLUG];
    [filetype_ext addObjectsFromArray:filetype_extSEXYPSF];
    [filetype_ext addObjectsFromArray:filetype_extLAZYUSF];
    [filetype_ext addObjectsFromArray:filetype_ext2SF];
    [filetype_ext addObjectsFromArray:filetype_extSNSF];
    [filetype_ext addObjectsFromArray:filetype_extVGMSTREAM];
    [filetype_ext addObjectsFromArray:filetype_extMPG123];
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
            NSString *extension;// = [[file pathExtension] uppercaseString];
            NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
            int found=0;
            
            NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
            extension = (NSString *)[temparray_filepath lastObject];
            [temparray_filepath removeLastObject];
            file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
            
            
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
//        mdz_ArchiveFilesListAlias=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*));
        
        // Second check count for each section
        dirEnum = [mFileMngr enumeratorAtPath:pathToScan];
        while (file = [dirEnum nextObject]) {
            fileAttributes=[dirEnum fileAttributes];
            if ([fileAttributes objectForKey:NSFileType]==NSFileTypeDirectory) {
                //dir : do nothing
            }
            if ([fileAttributes objectForKey:NSFileType]==NSFileTypeRegular) {
                //File : add to the list if playable
                NSString *extension;// = [[file pathExtension] uppercaseString];
                NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                int found=0;
                
                NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                extension = (NSString *)[temparray_filepath lastObject];
                [temparray_filepath removeLastObject];
                file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
                
                
                if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                
                
                if (found)  {
                    mdz_ArchiveFilesList[mdz_currentArchiveIndex]=(char*)malloc([file length]+1);
                    strcpy(mdz_ArchiveFilesList[mdz_currentArchiveIndex],[file UTF8String]);
                    
                    //mdz_ArchiveFilesListAlias[mdz_currentArchiveIndex]=(char*)malloc([file length]+1);
                    //strcpy(mdz_ArchiveFilesListAlias[mdz_currentArchiveIndex],[file UTF8String]);
                    
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
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extDUMB=[SUPPORTED_FILETYPE_DUMB componentsSeparatedByString:@","];
    NSArray *filetype_extGME=(no_aux_file?[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_GME_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extSEXYPSF=(no_aux_file?[SUPPORTED_FILETYPE_SEXYPSF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_SEXYPSF_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extLAZYUSF=(no_aux_file?[SUPPORTED_FILETYPE_LAZYUSF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_LAZYUSF_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_ext2SF=(no_aux_file?[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_2SF_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extSNSF=(no_aux_file?[SUPPORTED_FILETYPE_SNSF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_SNSF_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extAOSDK=(no_aux_file?[SUPPORTED_FILETYPE_AOSDK componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_AOSDK_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=(no_aux_file?[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_GSF_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSArray *filetype_extMPG123=[SUPPORTED_FILETYPE_MPG123 componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    NSArray *filetype_extCOVER=[SUPPORTED_FILETYPE_COVER componentsSeparatedByString:@","];
    
    NSString *extension;
    NSString *file_no_ext;
    NSString *filePath;
    int found=0;
    
    
    //extension = [_filePath pathExtension];
    //file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
    extension = (NSString *)[temparray_filepath lastObject];
    [temparray_filepath removeLastObject];
    file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
    
    
    //filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
    filePath=[self getFullFilePath:_filePath];
    
    mSingleFileType=1; //used to identify file which relies or not on another file (sample, psflib, ...)
    
    if (!found)
        for (int i=0;i<[filetype_extVGMSTREAM count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {found=MMP_VGMSTREAM;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {found=MMP_VGMSTREAM;break;}
        }
    if (!found)
    for (int i=0;i<[filetype_extMPG123 count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extMPG123 objectAtIndex:i]]==NSOrderedSame) {found=MMP_MPG123;break;}
        if ([file_no_ext caseInsensitiveCompare:[filetype_extMPG123 objectAtIndex:i]]==NSOrderedSame) {found=MMP_MPG123;break;}
    }
    if (!found)
        for (int i=0;i<[filetype_extVGM count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=MMP_VGMPLAY;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=MMP_VGMPLAY;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extASAP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=MMP_ASAP;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=MMP_ASAP;break;}
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
                found=MMP_GME;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_GME_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_GME;break;
            }
        }
    if (!found)
        for (int i=0;i<[filetype_extSID count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=MMP_SIDPLAY;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=MMP_SIDPLAY;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extMDX count];i++) { //PDX might be required
            if ([extension caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
                mSingleFileType=0;
                found=MMP_MDXPDX;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
                mSingleFileType=0;
                found=MMP_MDXPDX;break;
            }
        }
    if (!found)
        for (int i=0;i<[filetype_extPMD count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=MMP_PMDMINI;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=MMP_PMDMINI;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extADPLUG count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=MMP_ADPLUG;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=MMP_ADPLUG;break;}
        }
    
    if (!found)
        for (int i=0;i<[filetype_extSTSOUND count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=MMP_STSOUND;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=MMP_STSOUND;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extSC68 count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=MMP_SC68;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=MMP_SC68;break;}
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
                found=MMP_SEXYPSF;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSEXYPSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_SEXYPSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_SEXYPSF;break;
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
                found=MMP_LAZYUSF;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extLAZYUSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_LAZYUSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_LAZYUSF;break;
            }
        }
    if (!found)
        for (int i=0;i<[filetype_ext2SF count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_ext2SF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_2SF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_2SF;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_ext2SF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_2SF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_2SF;break;
            }
        }
    if (!found)
        for (int i=0;i<[filetype_extSNSF count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extSNSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_SNSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_SNSF;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extSNSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_SNSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_SNSF;break;
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
                found=MMP_AOSDK;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extAOSDK objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_AOSDK_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_AOSDK;break;
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
                found=MMP_GSF;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_GSF_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_GSF;break;
            }
        }
    //tmp hack => redirect to timidity
    if (!found)
        for (int i=0;i<[filetype_extWMIDI count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=MMP_TIMIDITY;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=MMP_TIMIDITY;break;}
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
                
                found=MMP_UADE;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {
                //check if require second file
                NSArray *singlefile=[SUPPORTED_FILETYPE_UADE_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_UADE;break;
            }
        }
    if ((!found)||(mdz_defaultMODPLAYER==DEFAULT_MODPLUG))
        for (int i=0;i<[filetype_extMODPLUG count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=MMP_OPENMPT;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=MMP_OPENMPT;break;}
        }
    if ((!found)||(mdz_defaultMODPLAYER==DEFAULT_XMP))
        for (int i=0;i<[filetype_extXMP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) {found=MMP_OPENMPT;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) {found=MMP_OPENMPT;break;}
        }
    if ((!found)||(mdz_defaultMODPLAYER==DEFAULT_DUMB))
        for (int i=0;i<[filetype_extDUMB count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=MMP_DUMB;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=MMP_DUMB;break;}
        }
    if (!found) {
        for (int i=0;i<[filetype_extHVL count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=MMP_HVL;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=MMP_HVL;break;}
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

-(int) mmp_gsfLoad:(NSString*)filePath {  //GSF
    mPlayType=MMP_GSF;
    FILE *f;
    char length_str[256], fade_str[256], title_str[256];
    char tmp_str[256];
    char *tag;
    
    
    soundLowPass = optGSFsoundLowPass;
    soundEcho = optGSFsoundEcho;
    soundQuality = optGSFsoundQuality;//1:44Khz, 2:22Khz, 4:11Khz
    soundInterpolation = optGSFsoundInterpolation;
    
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
    mod_name[0]=0;
    if (title_str)
        if (title_str[0]) sprintf(mod_name," %s",title_str);
    
    if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
    
    
    free(tag);
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    return 0;
}

-(int) mmp_mdxpdxLoad:(NSString*)filePath {  //MDX
    mPlayType=MMP_MDXPDX;
    
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
                UIAlertView *alertMissingPDX = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"")
                                                                           message:alertMsg delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil];
                [alertMissingPDX show];
            }
        }
        
        //Loop
        if (mLoopMode==1) iModuleLength=-1;
        
        return 0;
    }
}
-(int) mmp_sc68Load:(NSString*)filePath {  //SC68
    mPlayType=MMP_SC68;
    
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
-(int) mmp_stsoundLoad:(NSString*)filePath {  //STSOUND
    mPlayType=MMP_STSOUND;
    
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
    return 1;
}

-(void) sid_parseStilInfo {
    int idx=0;
    int parser_status=0;
    int parser_track_nb=0;
    int stil_info_len=strlen(stil_info);
    char tmp_str[1024];
    int tmp_str_idx;
    
    sidtune_name=(char**)malloc(mod_subsongs*sizeof(char*));
    memset(sidtune_name,0,sizeof(char*)*mod_subsongs);
    sidtune_title=(char**)malloc(mod_subsongs*sizeof(char*));
    memset(sidtune_title,0,sizeof(char*)*mod_subsongs);
    
    while (stil_info[idx]) {
        if ((stil_info[idx]=='(')&&(stil_info[idx+1]=='#')) {
            parser_status=1;
            parser_track_nb=0;
        }
        else {
            switch (parser_status) {
                case 1: // got a "(" before
                    if (stil_info[idx]=='#') parser_status=2;
                    else parser_status=0;
                    break;
                case 2: // got a "(#" before
                    if ((stil_info[idx]>='0')&&(stil_info[idx]<='9')) {
                        parser_track_nb=parser_track_nb*10+(stil_info[idx]-'0');
                        parser_status=2;
                    } else if (stil_info[idx]==')') {
                        parser_status=3;
                    }
                    break;
                case 3: // got a "(#<track_nb>)" before
                    if (stil_info[idx]=='N') parser_status=4;
                    if (stil_info[idx]=='T') parser_status=10;
                    break;
                    ///////////////////////////////////////////////
                    //"NAME: "
                    ///////////////////////////////////////////////
                case 4: // "N"
                    if (stil_info[idx]=='A') parser_status=5;
                    break;
                case 5: // "NA"
                    if (stil_info[idx]=='M') parser_status=6;
                    break;
                case 6: // "NAM"
                    if (stil_info[idx]=='E') parser_status=7;
                    break;
                case 7: // "NAME"
                    if (stil_info[idx]==':') parser_status=8;
                    break;
                case 8: // "NAME:"
                    if (stil_info[idx]==' ') {
                        parser_status=9;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                    }
                    break;
                case 9: // "NAME: "
                    if (stil_info[idx]==0x0A) {
                        parser_status=0;
                        if (parser_track_nb<=mod_subsongs) {
                            sidtune_name[parser_track_nb-1]=(char*)malloc(tmp_str_idx+1);
                            strcpy(sidtune_name[parser_track_nb-1],tmp_str);
                        }
                    } else {
                        tmp_str[tmp_str_idx]=stil_info[idx];
                        if (tmp_str_idx<1024) tmp_str_idx++;
                        tmp_str[tmp_str_idx]=0;
                    }
                    break;
                    ///////////////////////////////////////////////
                    //"TITLE: "
                    ///////////////////////////////////////////////
                case 10: // "T"
                    if (stil_info[idx]=='I') parser_status=11;
                    break;
                case 11: // "TI"
                    if (stil_info[idx]=='T') parser_status=12;
                    break;
                case 12: // "TIT"
                    if (stil_info[idx]=='L') parser_status=13;
                    break;
                case 13: // "TITL"
                    if (stil_info[idx]=='E') parser_status=14;
                    break;
                case 14: // "TITLE"
                    if (stil_info[idx]==':') parser_status=15;
                    break;
                case 15: // "TITLE:"
                    if (stil_info[idx]==' ') {
                        parser_status=16;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                    }
                    break;
                case 16: // "TITLE: "
                    if (stil_info[idx]==0x0A) {
                        parser_status=0;
                        if (parser_track_nb<=mod_subsongs) {
                            sidtune_title[parser_track_nb-1]=(char*)malloc(tmp_str_idx+1);
                            strcpy(sidtune_title[parser_track_nb-1],tmp_str);
                        }
                    } else {
                        tmp_str[tmp_str_idx]=stil_info[idx];
                        if (tmp_str_idx<1024) tmp_str_idx++;
                        tmp_str[tmp_str_idx]=0;
                    }
                    break;
                default:
                    break;
            }
        }
        idx++;
        
    }
}

/*
 * Load ROM dump from file.
 * Allocate the buffer if file exists, otherwise return 0.
 */
char* loadRom(const char* path, size_t romSize)
{
    char* buffer = 0;
    FILE *f=fopen(path,"rb");
    if (!f) return NULL;
    fseek(f,0L,SEEK_END);
    int fsize=ftell(f);
    fseek(f,0L,SEEK_SET);
    buffer=new char[fsize];
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    fread(buffer,1,fsize,f);
    
    fclose(f);
    return buffer;
}


-(int) mmp_sidplayLoad:(NSString*)filePath {  //SID
    mPlayType=MMP_SIDPLAY;
    
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
    
    /* new format: md5 on all file data*/
    int tmp_md5_data_size=mp_datasize;
    char *tmp_md5_data=(char*)malloc(tmp_md5_data_size);
    
    f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"SID Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    fread(tmp_md5_data,1,tmp_md5_data_size,f);
    fclose(f);
    
    md5_from_buffer(song_md5,33,tmp_md5_data,tmp_md5_data_size);
    song_md5[32]=0;
    free(tmp_md5_data);
    
    
            
    // Init SID emu engine
    mSidEmuEngine = new sidplayfp;
    
    NSString *c64_path = [[NSBundle mainBundle] resourcePath];
    
    char *kernal = loadRom([[c64_path stringByAppendingString:@"/kernal.c64"] UTF8String], 8192);
    char *basic = loadRom([[c64_path stringByAppendingString:@"/basic.c64"] UTF8String], 8192);
    char *chargen = loadRom([[c64_path stringByAppendingString:@"/chargen.c64"] UTF8String], 4096);

    mSidEmuEngine->setRoms((const uint8_t*)kernal, (const uint8_t*)basic, (const uint8_t*)chargen);

    delete [] kernal;
    delete [] basic;
    delete [] chargen;
    
    
    // Init ReSID
    mBuilder = new ReSIDfpBuilder("residfp");
    unsigned int maxsids = (mSidEmuEngine->info()).maxsids();
    mBuilder->create(maxsids);
    // Check if builder is ok
    if (!mBuilder->getStatus())
    {
        NSLog(@"issue in creating sid builder");
        return -1;
    }
    //mBuilder = new ReSIDfpBuilder("residfp");
    // Set config
    SidConfig cfg;// = mSidEmuEngine->config();
    cfg.frequency= PLAYBACK_FREQ;
    cfg.samplingMethod = SidConfig::INTERPOLATE;
    cfg.fastSampling = false;
    cfg.playback = SidConfig::STEREO;
    cfg.sidEmulation  = mBuilder;
    
    
    
    switch (sid_forceClock) {
        case 0:
            //cfg.clockForced=FALSE;
            cfg.forceC64Model=false;
            break;
        case 1:
            cfg.forceC64Model=true;
            cfg.defaultC64Model=SidConfig::PAL;
            break;
        case 2:
            cfg.forceC64Model=true;
            cfg.defaultC64Model=SidConfig::NTSC;
            break;
    }
    
    switch (sid_forceModel) {
        case 0:
            cfg.forceSidModel=false;
            break;
        case 1:
            cfg.defaultSidModel=SidConfig::MOS6581;
            cfg.forceSidModel=true;
            break;
        case 2:
            cfg.defaultSidModel=SidConfig::MOS8580;
            cfg.forceSidModel=true;
            break;
    }
    
    // setup resid
    if (mSIDFilterON) mBuilder->filter(true);
    else mBuilder->filter(false);
    
    mSidEmuEngine->config(cfg);
    // Load tune
    mSidTune=new SidTune([filePath UTF8String],0,true);
    
    if (mSidTune==NULL) {
        NSLog(@"SID SidTune init error");
        delete mSidEmuEngine; mSidEmuEngine=NULL;
        delete mBuilder; mBuilder=NULL;
        if (mSidTune) {delete mSidTune;mSidTune=NULL;}
        mPlayType=0;
    } else {
        const SidTuneInfo *sidtune_info;
        sidtune_info=mSidTune->getInfo();
        
        if (sidtune_info->infoString(0)[0]) sprintf(mod_name," %s",sidtune_info->infoString(0));
        else sprintf(mod_name," %s",mod_filename);
        mod_subsongs=sidtune_info->songs();
        mod_minsub=0;//sidtune_info.startSong;
        mod_maxsub=sidtune_info->songs()-1;
        mod_currentsub=sidtune_info->startSong()-1;
                       
        mSidTune->selectSong(mod_currentsub);
        iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
        if (!iModuleLength) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
        mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
        
        if (mSidEmuEngine->load(mSidTune)) {
            iCurrentTime=0;
            mCurrentSamples=0;
            numChannels=(mSidEmuEngine->info()).channels();
            
            stil_info[0]=0;
            [self getStilInfo:(char*)[filePath UTF8String]];
            
            sprintf(mod_message,"");
            for (int i=0;i<sidtune_info->numberOfInfoStrings();i++)
                sprintf(mod_message,"%s%s\n",mod_message,sidtune_info->infoString(i));
            
            sprintf(mod_message,"%s\n[STIL Information]\n%s\n",mod_message,stil_info);
            //Loop
            if (mLoopMode==1) iModuleLength=-1;
            
            
            //Parse STIL INFO for subsongs info
            [self sid_parseStilInfo];
            
            
            //////////////////////////////////
            //update DB with songlength
            //////////////////////////////////
            mod_total_length=0;
            for (int i=0;i<sidtune_info->songs(); i++) {
                int sid_subsong_length=[self getSongLengthfromMD5:i-mod_minsub+1];
                if (!sid_subsong_length) sid_subsong_length=optGENDefaultLength;//SID_DEFAULT_LENGTH;
                mod_total_length+=sid_subsong_length;
                
                short int playcount;
                signed char rating;
                int song_length;
                char channels_nb;
                int songs;
                NSString *filePathSid;
                NSString *fileName=[self getSubTitle:i];
                
                
                NSMutableArray *tmp_path=[NSMutableArray arrayWithArray:[filePath componentsSeparatedByString:@"/"]];
                for (;;) {
                    if ([(NSString *)[tmp_path firstObject] compare:@"Documents"]==NSOrderedSame) {
                        break;
                    }
                    [tmp_path removeObjectAtIndex:0];
                    if ([tmp_path count]==0) break;
                }
                filePathSid=[tmp_path componentsJoinedByString:@"/"];
                
                NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathSid,i];
                
                DBHelper::getFileStatsDBmod(fileName,filePathSubsong,&playcount,&rating,&song_length,&channels_nb,&songs);
                //NSLog(@"%@||%@||sl:%d||ra:%d",fileName,filePathSubsong,sid_subsong_length,rating);
                
                DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,playcount,rating,sid_subsong_length,(mSidEmuEngine->info()).channels(),sidtune_info->songs());
                
                if (i==sidtune_info->songs()-1) {// Global file stats update
                    fileName=[filePath lastPathComponent];
                    DBHelper::getFileStatsDBmod(fileName,filePathSid,&playcount,&rating,&song_length,&channels_nb,&songs);
                    
                    //NSLog(@"%@||%@||sl:%d||ra:%d",fileName,filePathSid,mod_total_length,rating);
                    
                    DBHelper::updateFileStatsDBmod(fileName,filePathSid,playcount,rating,mod_total_length,(mSidEmuEngine->info()).channels(),sidtune_info->songs());
                }
            }
            
            
            return 0;
        }
    }
    return 1;
}

-(int) mmp_hvlLoad:(NSString*)filePath {  //HVL
    mPlayType=MMP_HVL;
    
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

-(int) mmp_uadeLoad:(NSString*)filePath {  //UADE
    int ret;
    
    mPlayType=MMP_UADE;
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
    
    //printf("loading md5=%s\n",song_md5);
    
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
    
    //printf("Player candidate: %s\n", UADEstate.ep->playername);
    
    if (strcmp(UADEstate.ep->playername, "custom") == 0) {
        strcpy(UADEplayername, modulename);
        modulename[0] = 0;
    } else {
        sprintf(UADEplayername, "%s/%s", UADEstate.config.basedir.name, UADEstate.ep->playername);
    }
    
    //printf("Player name: %s\n", UADEplayername);
    
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
    
    UADEstate.config.use_ntsc=mUADE_OptNTSC;
    
    if (strcasecmp(song_md5,"a39585c86c7a521ba28075a102f32396")==0) { //Indianapolis 500 (cust)
        UADEstate.config.use_ntsc=1;
    }
    
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
-(int) mmp_sexypsfLoad:(NSString*)filePath {  //SexyPSF
    mPlayType=MMP_SEXYPSF;
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
-(int) mmp_aosdkLoad:(NSString*)filePath {  //AOSDK
    mPlayType=MMP_AOSDK;
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

-(int) mmp_xmpLoad:(NSString*)filePath {  //XMP
    mPlayType=MMP_XMP;
    
    FILE *f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"XMP Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);
    
    xmp_ctx = xmp_create_context();
    if (xmp_ctx==NULL) {
        NSLog(@"XMP: Cannot create context");
        return 1;
    }
    
    if (xmp_load_module(xmp_ctx, (char*)[filePath UTF8String]) < 0) {
        NSLog(@"XMP: error loading %s\n", [filePath UTF8String]);
        xmp_free_context(xmp_ctx);
        return 2;
    }
    
    if (xmp_start_player(xmp_ctx, 44100, 0) != 0) {
        NSLog(@"XMP: error loading %s\n", [filePath UTF8String]);
        xmp_release_module(xmp_ctx);
        xmp_free_context(xmp_ctx);
        return 3;
    }
    
    /* Show module data */
    
    xmp_get_module_info(xmp_ctx, &xmp_mi);
    
    sprintf(mod_name," %s",xmp_mi.mod->name);
    if (xmp_mi.mod->name[0]==0) {
        sprintf(mod_name," %s",mod_filename);
    }
    
    if (xmp_mi.comment) sprintf(mod_message,"%s\n", xmp_mi.comment);
    else {
        mod_message[0]=0;
        if (xmp_mi.mod->ins) {
            for (int i=0;i<xmp_mi.mod->ins;i++) {
                concatn(8192+MAX_STIL_DATA_LENGTH,mod_message,xmp_mi.mod->xxi[i].name);
                concatn(8192+MAX_STIL_DATA_LENGTH,mod_message,"\n");
            }
        }
        if (xmp_mi.mod->smp) {
            for (int i=0;i<xmp_mi.mod->smp;i++) {
                concatn(8192+MAX_STIL_DATA_LENGTH,mod_message,xmp_mi.mod->xxs[i].name);
                concatn(8192+MAX_STIL_DATA_LENGTH,mod_message,"\n");
            }
        }
    }
    
    /*NSLog(@"XMP num sequence: %d",xmp_mi.num_sequences);
     for (int i=0;i<xmp_mi.num_sequences;i++) {
     NSLog(@"XMP sequence %d duration: %d",i,xmp_mi.seq_data[i].duration);
     }
     */
    iModuleLength=xmp_mi.seq_data[0].duration;
    
    iCurrentTime=0;
    numChannels=xmp_mi.mod->chn;
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    
    
    return 0;
}

-(int) mmp_openmptLoad:(NSString*)filePath {  //MODPLUG
    const char *modName;
    char *modMessage;
    mPlayType=MMP_OPENMPT;
    
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
        
        
        mod_subsongs=openmpt_module_get_num_subsongs(mp_file->mod);
        mod_minsub=0;
        mod_maxsub=mod_subsongs-1;
        mod_currentsub=openmpt_module_get_selected_subsong( mp_file->mod );
        
        
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
    return 1;
}
-(int) mmp_timidityLoad:(NSString*)filePath { //timidity
    mPlayType=MMP_TIMIDITY;
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
-(int) mmp_vgmstreamLoad:(NSString*)filePath extension:(NSString*)extension{  //VGMSTREAM
    mPlayType=MMP_VGMSTREAM;
    FILE *f;
    
    //NSLog(@"yo: %@",filePath);
    
    f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"VGMSTREAM Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);
    
    //Initi SRC samplerate converter
    int error;
    src_state=src_callback_new(src_callback_vgmstream,optVGMSTREAM_resampleQuality,2,&error,NULL);
    if (!src_state) {
        NSLog(@"Error while initializing SRC samplerate converter: %d",error);
        return -1;
    }
    
    if (mLoopMode==1) mVGMSTREAM_force_loop = true;
    else mVGMSTREAM_force_loop = false;
    
    if ( optVGMSTREAM_loopmode)
    {
        mVGMSTREAM_force_loop = true;
        //NSLog(@"VGMStreamPlugin Force Loop");
    }
    
    
    vgmFile = open_stdio_streamfile([filePath UTF8String]);
    if (!vgmFile) {
        NSLog(@"Error open_stdio_streamfile %@",filePath);
        mPlayType=0;
        src_delete(src_state);
        return -1;
    }
    vgmStream = init_vgmstream_from_STREAMFILE(vgmFile);
    
    if (!vgmStream) {
        NSLog(@"Error init_vgmstream_from_STREAMFILE %@",filePath);
        mPlayType=0;
        close_streamfile(vgmFile);
        src_delete(src_state);
        return -1;
    }
    /////////////////////////
    
    vgmstream_cfg_t vcfg = {0};

    
#if 0
 /* song mofidiers */
    int play_forever;           /* keeps looping forever (needs loop points) */
    int ignore_loop;            /* ignores loops points */
    int force_loop;             /* enables full loops (0..samples) if file doesn't have loop points */
    int really_force_loop;      /* forces full loops even if file has loop points */
    int ignore_fade;            /*  don't fade after N loops */
    /* song processing */
    double loop_count;          /* target loops */
    double fade_delay;          /* fade delay after target loops */
    double fade_time;           /* fade period after target loops */
#endif
    vcfg.allow_play_forever = (mLoopMode==1?1:0);
    vcfg.play_forever =(mLoopMode==1?1:0);
    
    vcfg.loop_count = optVGMSTREAM_loop_count;
    vcfg.fade_time = 5.0f;
    vcfg.fade_delay = 0.0f;
    vcfg.ignore_fade = 0; //1;
    vcfg.force_loop = (mVGMSTREAM_force_loop?1:0);
    vcfg.ignore_loop = 0;//ignore_loop;
    numChannels=2;
    vgmstream_apply_config(vgmStream, &vcfg);
    vgmstream_set_play_forever(vgmStream,vcfg.play_forever);
    vgmstream_mixing_autodownmix(vgmStream, 2);
    vgmstream_mixing_enable(vgmStream, SOUND_BUFFER_SIZE_SAMPLE, NULL /*&input_channels*/, &numChannels);
    
    
    
    
    src_ratio=PLAYBACK_FREQ/(double)(vgmStream->sample_rate);
    
    if (vgmStream->channels <= 0)
    {
        close_vgmstream(vgmStream);
        vgmStream = NULL;
        NSLog(@"Error vgmStream->channels: %d",vgmStream->channels);
        close_streamfile(vgmFile);
        src_delete(src_state);
        return -1;
    }
    
    if (mLoopMode==1) {
        mVGMSTREAM_total_samples = -1;//get_vgmstream_play_samples(0, 0.0f, 0.0f, vgmStream);
        //mVGMSTREAM_totalinternal_samples = get_vgmstream_play_samples(1, 0.0f, 0.0f, vgmStream);
    } else {
        mVGMSTREAM_total_samples = get_vgmstream_play_samples(optVGMSTREAM_loop_count, 0.0f, 0.0f, vgmStream);
        mVGMSTREAM_totalinternal_samples = mVGMSTREAM_total_samples;
    }
    close_streamfile(vgmFile);
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    else iModuleLength=(double)mVGMSTREAM_total_samples*1000.0f/(double)(vgmStream->sample_rate);
    iCurrentTime=0;
    mVGMSTREAM_seek_needed_samples=-1;
    mVGMSTREAM_decode_pos_samples=0;
    
    numChannels=vgmStream->channels;
    sprintf(mod_name," %s",mod_filename);
    
    mod_message[0]=0;
    describe_vgmstream(vgmStream,mod_message,8192+MAX_STIL_DATA_LENGTH);
    vgm_sample_data=(int16_t*)malloc(SOUND_BUFFER_SIZE_SAMPLE*2*(numChannels>2?numChannels:2));
    vgm_sample_data_float=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4*(numChannels>2?numChannels:2));
    vgm_sample_converted_data_float=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4*(numChannels>2?numChannels:2));
    
    
    
    sprintf(mmp_fileext,"%s",[extension UTF8String] );
    return 0;
}
    
    /* Helper for v1 printing, get these strings their zero byte. */
    void safe_print(char *str_msg,char* name, char *data, size_t size)
    {
        char safe[31];
        if(size>30) return;
        
        memcpy(safe, data, size);
        safe[size] = 0;
        sprintf("%s: %s\n",str_msg, name, safe);
    }
    
    /* Print out ID3v1 info. */
    void print_v1(char *str_msg,mpg123_id3v1 *v1)
    {
        safe_print(str_msg+strlen(str_msg),"Title",   v1->title,   sizeof(v1->title));
        safe_print(str_msg+strlen(str_msg),"Artist",  v1->artist,  sizeof(v1->artist));
        safe_print(str_msg+strlen(str_msg),"Album",   v1->album,   sizeof(v1->album));
        safe_print(str_msg+strlen(str_msg),"Year",    v1->year,    sizeof(v1->year));
        safe_print(str_msg+strlen(str_msg),"Comment", v1->comment, sizeof(v1->comment));
        sprintf(str_msg+strlen(str_msg),"Genre: %i", v1->genre);
    }
    
    /* Split up a number of lines separated by \n, \r, both or just zero byte
     and print out each line with specified prefix. */
    void print_lines(char *str_msg,const char* prefix, mpg123_string *inlines)
    {
        size_t i;
        int hadcr = 0, hadlf = 0;
        char *lines = NULL;
        char *line  = NULL;
        size_t len = 0;
        
        if(inlines != NULL && inlines->fill)
        {
            lines = inlines->p;
            len   = inlines->fill;
        }
        else return;
        
        line = lines;
        for(i=0; i<len; ++i)
        {
            if(lines[i] == '\n' || lines[i] == '\r' || lines[i] == 0)
            {
                char save = lines[i]; /* saving, changing, restoring a byte in the data */
                if(save == '\n') ++hadlf;
                if(save == '\r') ++hadcr;
                if((hadcr || hadlf) && hadlf % 2 == 0 && hadcr % 2 == 0) line = "";
                
                if(line)
                {
                    lines[i] = 0;
                    sprintf(str_msg+strlen(str_msg),"%s%s\n", prefix, line);
                    line = NULL;
                    lines[i] = save;
                }
            }
            else
            {
                hadlf = hadcr = 0;
                if(line == NULL) line = lines+i;
            }
        }
    }
    
    /* Print out the named ID3v2  fields. */
    void print_v2(char *str_msg,mpg123_id3v2 *v2)
    {
        print_lines(str_msg+strlen(str_msg),"Title: ",   v2->title);
        print_lines(str_msg+strlen(str_msg),"Artist: ",  v2->artist);
        print_lines(str_msg+strlen(str_msg),"Album: ",   v2->album);
        print_lines(str_msg+strlen(str_msg),"Year: ",    v2->year);
        print_lines(str_msg+strlen(str_msg),"Comment: ", v2->comment);
        print_lines(str_msg+strlen(str_msg),"Genre: ",   v2->genre);
    }
    
    /* Print out all stored ID3v2 fields with their 4-character IDs. */
    void print_raw_v2(char *str_msg,mpg123_id3v2 *v2)
    {
        size_t i;
        for(i=0; i<v2->texts; ++i)
        {
            char id[5];
            char lang[4];
            memcpy(id, v2->text[i].id, 4);
            id[4] = 0;
            memcpy(lang, v2->text[i].lang, 3);
            lang[3] = 0;
            if(v2->text[i].description.fill)
            sprintf(str_msg+strlen(str_msg),"%s language(%s) description(%s)\n", id, lang, v2->text[i].description.p);
            else sprintf(str_msg+strlen(str_msg),"%s language(%s)\n", id, lang);
            
            print_lines(str_msg+strlen(str_msg)," ", &v2->text[i].text);
        }
        for(i=0; i<v2->extras; ++i)
        {
            char id[5];
            memcpy(id, v2->extra[i].id, 4);
            id[4] = 0;
            sprintf(str_msg+strlen(str_msg), "%s description(%s)\n",
                   id,
                   v2->extra[i].description.fill ? v2->extra[i].description.p : "" );
            print_lines(str_msg+strlen(str_msg)," ", &v2->extra[i].text);
        }
        for(i=0; i<v2->comments; ++i)
        {
            char id[5];
            char lang[4];
            memcpy(id, v2->comment_list[i].id, 4);
            id[4] = 0;
            memcpy(lang, v2->comment_list[i].lang, 3);
            lang[3] = 0;
            sprintf(str_msg+strlen(str_msg), "%s description(%s) language(%s): \n",
                   id,
                   v2->comment_list[i].description.fill ? v2->comment_list[i].description.p : "",
                   lang );
            print_lines(str_msg+strlen(str_msg)," ", &v2->comment_list[i].text);
        }
    }


-(int) mmp_mpg123Load:(NSString*)filePath extension:(NSString*)extension{  //VGMSTREAM
    int  encoding,err;
    mPlayType=MMP_MPG123;
    FILE *f;
    
    //NSLog(@"yo: %@",filePath);
    
    f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"MPG123 Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);

    
    //Initi SRC samplerate converter
    int error;
    src_state=src_callback_new(src_callback_mpg123,optMPG123_resampleQuality,2,&error,NULL);
    if (!src_state) {
        NSLog(@"Error while initializing SRC samplerate converter: %d",error);
        return -1;
    }
    
    if (mLoopMode==1) mVGMSTREAM_force_loop = true;
    else mVGMSTREAM_force_loop = false;
    
    if ( optVGMSTREAM_loopmode)
    {
        mVGMSTREAM_force_loop = true;
        //NSLog(@"VGMStreamPlugin Force Loop");
    }

    
    if (mpg123_init()) {
        NSLog(@"Error mpg123_init");
        mPlayType=0;
        src_delete(src_state);
        return -1;
    }
    mpg123h = mpg123_new(NULL, &err);
    if (! mpg123h) {
        NSLog(@"Error mpg123_new");
        mPlayType=0;
        src_delete(src_state);
        mpg123_delete(mpg123h);
        mpg123_exit();
        return -1;
    }
    
    /* open the file and get the decoding format */
    if (mpg123_open(mpg123h, [filePath UTF8String])) {
        NSLog(@"Error mpg123_open %@",filePath);
        mPlayType=0;
        src_delete(src_state);
        mpg123_delete(mpg123h);
        mpg123_exit();
        return -1;
    }
    
    mpg123_getformat(mpg123h, &mpg123_rate, &mpg123_channel, &encoding);
    mpg123_bytes = mpg123_encsize(encoding);
    
    NSLog(@"mpg123 info: %d %d %d",mpg123_rate,mpg123_bytes,mpg123_channel);
    
    src_ratio=PLAYBACK_FREQ/(double)(mpg123_rate);
        
    if (mpg123_channel <= 0)
        {
            NSLog(@"Error mpg123_channel: %d",mpg123_channel);
            src_delete(src_state);
            mpg123_close(mpg123h);
            mpg123_delete(mpg123h);
            mpg123_exit();
            return -1;
        }
    
    mpg123_seek(mpg123h,0,SEEK_END);
        /*if (mLoopMode==1) {
            mMPG123_total_samples = 0;
            //get_vgmstream_play_samples(0, 0.0f, 0.0f, vgmStream);
        } else mVGMSTREAM_total_samples = 0;
         */
    mMPG123_total_samples=mpg123_tell(mpg123h);
    mpg123_seek(mpg123h,0,SEEK_SET);
    //get_vgmstream_play_samples(optVGMSTREAM_loop_count, 0.0f, 0.0f, vgmStream);
    
        
    iModuleLength=(double)mMPG123_total_samples*1000.0f/(double)(mpg123_rate);
    iCurrentTime=0;
    mMPG123_seek_needed_samples=-1;
    mMPG123_decode_pos_samples=0;
        
        numChannels=mpg123_channel;
        if (numChannels>2) {
            NSLog(@"not managed yet, more than 2 channels (%d)",numChannels);
        }
        sprintf(mod_name," %s",mod_filename);
        mod_message[0]=0;
    
        mpg123_id3v1 *v1;
        mpg123_id3v2 *v2;
        int meta;
        mpg123_scan(mpg123h);
        meta = mpg123_meta_check(mpg123h);
        if(meta & MPG123_ID3 && mpg123_id3(mpg123h, &v1, &v2) == MPG123_OK) {
            if(v1 != NULL) {
                sprintf(mod_message+strlen(mod_message),"\n====      ID3v1       ====\n");
                print_v1(mod_message,v1);
            }
            //printf("\n====      ID3v2       ====\n");
            //if(v2 != NULL) print_v2(mod_message,v2);
            
            if(v2 != NULL) {
                sprintf(mod_message+strlen(mod_message),"\n==== ID3v2 Raw frames ====\n");
                print_raw_v2(mod_message,v2);
            }
        }
        //Loop
        if (mLoopMode==1) iModuleLength=-1;
        
        mpg123_sample_data=(int16_t*)malloc(SOUND_BUFFER_SIZE_SAMPLE*2*2);
        mpg123_sample_data_float=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4*2);
        mpg123_sample_converted_data_float=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4*2);
        
        sprintf(mmp_fileext,"%s",[extension UTF8String] );
        return 0;
    }


/*static int seek_needed;
 static double decode_pos_ms;
 static bool killThread = false;
 
 static const unsigned NumChannels = 2;
 static const unsigned BitsPerSample = 16;*/


-(int) mmp_2sfLoad:(NSString*)filePath extension:(NSString*)extension {  //2SF
    mPlayType=MMP_2SF;
    FILE *f;
    
    f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"2SF Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);
    
    //Create config
    xSFConfig = XSFConfig::Create();
    if (!xSFConfig) {
        NSLog(@"2SF Cannot initiate config");
        mPlayType=0;
        return 1;
    }
    xSFConfig->LoadConfig();
    
    //Create player
    
    xSFPlayer=nil;
    if (([extension caseInsensitiveCompare:@"NCSF"]==NSOrderedSame)||
        ([extension caseInsensitiveCompare:@"MININCSF"]==NSOrderedSame)) xSFPlayer=new XSFPlayer_NCSF([filePath UTF8String]);
    
    if (([extension caseInsensitiveCompare:@"2SF"]==NSOrderedSame)||
        ([extension caseInsensitiveCompare:@"MINI2SF"]==NSOrderedSame)) xSFPlayer=new XSFPlayer_2SF([filePath UTF8String]);
        
    if (!xSFPlayer) {
        NSLog(@"2SF Cannot initiate player");
        delete xSFConfig;
        mPlayType=0;
        return -2;
    }
    
    //Apply config to player
    xSFConfig->CopyConfigToMemory(xSFPlayer, true);
    if (mLoopMode==1) xSFConfig->playInfinitely=true;
    
    //Load file
    if (!xSFPlayer->Load()) {
        NSLog(@"XSF Cannot load file %@",filePath);
        delete xSFPlayer;
        delete xSFConfig;
        mPlayType=0;
        return -3;
    }
    xSFConfig->CopyConfigToMemory(xSFPlayer, false);
    xSFFile = xSFPlayer->GetXSFFile();
    
    sprintf(mod_name," %s",mod_filename);
    
    mod_message[0]=0;
    auto tags = xSFFile->GetAllTags();
    auto keys = tags.GetKeys();
    std::wstring info;
    for (unsigned x = 0, numTags = keys.size(); x < numTags; ++x)
    {
        if (x) concatn(8192+MAX_STIL_DATA_LENGTH,mod_message,"\n");
        concatn(8192+MAX_STIL_DATA_LENGTH,mod_message,(keys[x] + "=" + tags[keys[x]]).c_str());
    }
    int fadeInMS=xSFFile->GetFadeMS(xSFPlayer->fadeInMS);
    xSFPlayer->fadeInMS=fadeInMS;
    xSFPlayer->fadeSample=(long long)(fadeInMS)*(long long)(xSFPlayer->sampleRate)/1000;
    
    iCurrentTime=0;
    numChannels=2;
    iModuleLength=1000* (long long)(xSFPlayer->GetLengthInSamples()) / 44100;
    
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    sprintf(mmp_fileext,"%s",[extension UTF8String] );
    
    
    return 0;
}

-(int) mmp_snsfLoad:(NSString*)filePath extension:(NSString*)extension {  //SNSF
    mPlayType=MMP_SNSF;
    FILE *f;
    
    f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"SNSF Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);
/*
    //Create config
    xSFConfigSNSF = XSFConfigSNSF::Create();
    if (!xSFConfigSNSF) {
        NSLog(@"SNSF Cannot initiate config");
        mPlayType=0;
        return 1;
    }
    xSFConfigSNSF->LoadConfig();
    
    //Create player
    
    xSFPlayerSNSF=nil;
    if (([extension caseInsensitiveCompare:@"SNSF"]==NSOrderedSame)||
    ([extension caseInsensitiveCompare:@"MINISNSF"]==NSOrderedSame)) xSFPlayerSNSF=new XSFPlayer_SNSF([filePath UTF8String]);
    
    if (!xSFPlayerSNSF) {
        NSLog(@"SNSF Cannot initiate player");
        delete xSFConfigSNSF;
        mPlayType=0;
        return -2;
    }
    
    //Apply config to player
    xSFConfigSNSF->CopyConfigToMemory(xSFPlayerSNSF, true);
    if (mLoopMode==1) xSFConfigSNSF->playInfinitely=true;
    
    //Load file
    if (!xSFPlayerSNSF->Load()) {
        NSLog(@"SNSF Cannot load file %@",filePath);
        delete xSFPlayerSNSF;
        delete xSFConfigSNSF;
        mPlayType=0;
        return -3;
    }
    xSFConfigSNSF->CopyConfigToMemory(xSFPlayerSNSF, false);
    xSFFileSNSF = xSFPlayerSNSF->GetXSFFileSNSF();
    
    sprintf(mod_name," %s",mod_filename);
    
    mod_message[0]=0;
    auto tags = xSFFileSNSF->GetAllTags();
    auto keys = tags.GetKeys();
    std::wstring info;
    for (unsigned x = 0, numTags = keys.size(); x < numTags; ++x)
    {
        if (x) concatn(8192+MAX_STIL_DATA_LENGTH,mod_message,"\n");
        concatn(8192+MAX_STIL_DATA_LENGTH,mod_message,(keys[x] + "=" + tags[keys[x]]).c_str());
    }
    int fadeInMS=xSFFileSNSF->GetFadeMS(xSFPlayerSNSF->fadeInMS);
    xSFPlayerSNSF->fadeInMS=fadeInMS;
    xSFPlayerSNSF->fadeSample=(long long)(fadeInMS)*(long long)(xSFPlayerSNSF->sampleRate)/1000;
    
    iCurrentTime=0;
    numChannels=2;
    iModuleLength=1000* (long long)(xSFPlayerSNSF->GetLengthInSamples()) / 44100;
    
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    sprintf(mmp_fileext,"%s",[extension UTF8String] );
    
   */
    return 0;
}

-(int) mmp_lazyusfLoad:(NSString*)filePath {  //LAZYUSF
    mPlayType=MMP_LAZYUSF;
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
    src_state=src_callback_new(src_callback_lazyusf,optLAZYUSF_ResampleQuality,2,&error,NULL);
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
    
    //usf_set_hle_audio( lzu_state->emu_state, 1 );
    
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
    //NSLog(@"init sr:%d/%d %f",lzu_sample_rate,PLAYBACK_FREQ,src_ratio);
    
    src_data.data_out=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*sizeof(float)*2);
    src_data.data_in=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*sizeof(float)*2);
    
    iModuleLength=-1;
    if (usf_info_data->inf_length) {
        iModuleLength=psfTimeToMS(usf_info_data->inf_length)+psfTimeToMS(usf_info_data->inf_fade);
    }
    lzu_currentSample=0;
    lzu_fadeLength=psfTimeToMS(usf_info_data->inf_fade)*lzu_sample_rate/1000;
    lzu_fadeStart=psfTimeToMS(usf_info_data->inf_length)*lzu_sample_rate/1000;
    
    iCurrentTime=0;
    numChannels=2;
    
    sprintf(mod_name,"");
    if (usf_info_data->inf_title)
        if (usf_info_data->inf_title[0]) sprintf(mod_name," %s",usf_info_data->inf_title);
    
    if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
    
    sprintf(mod_message,"Game:\t%s\nTitle:\t%s\nArtist:\t%s\nYear:\t%s\nGenre:\t%s\nUSF By:\t%s\nCopyright:\t%s\nTrack:\t%s\nSample rate: %dHz\n",
            (usf_info_data->inf_game?usf_info_data->inf_game:""),
            (usf_info_data->inf_title?usf_info_data->inf_title:""),
            (usf_info_data->inf_artist?usf_info_data->inf_artist:""),
            (usf_info_data->inf_year?usf_info_data->inf_year:""),
            (usf_info_data->inf_genre?usf_info_data->inf_genre:""),
            (usf_info_data->inf_usfby?usf_info_data->inf_usfby:""),
            (usf_info_data->inf_copy?usf_info_data->inf_copy:""),
            (usf_info_data->inf_track?usf_info_data->inf_track:""),
            lzu_sample_rate);
    
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    lzu_sample_data=(int16_t*)malloc(LZU_SAMPLE_SIZE*2*2);
    lzu_sample_data_float=(float*)malloc(LZU_SAMPLE_SIZE*4*2);
    lzu_sample_converted_data_float=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4*2);
    
    
    
    return 0;
}

-(int) mmp_vgmplayLoad:(NSString*)filePath { //VGM
    mPlayType=MMP_VGMPLAY;
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
    ChipOpts[0].YM2612.EmuCore=optVGMPLAY_ym2612emulator;
    ChipOpts[1].YM2612.EmuCore=optVGMPLAY_ym2612emulator;
        
    VGMMaxLoop=optVGMPLAY_maxloop;
    if (mLoopMode==1) VGMMaxLoop=-1;
    
    VGMPlay_Init2();
    
    
    if (!OpenVGMFile([filePath UTF8String]))
        if (!OpenOtherFile([filePath UTF8String])) {
            NSLog(@"Cannot OpenVGMFile file %@",filePath);
            mPlayType=0;
            
            //StopVGM();
            //CloseVGMFile();
            VGMPlay_Deinit();
            
            return -2;
        }
    PlayVGM();
    
    sprintf(mod_message,"Author:%s\nGame:%s\nSystem:%s\nTitle:%s\nRelease Date:%s\nCreator:%s\nNotes:%s\n",
                [[self wcharToNS:GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strAuthorNameE,VGMTag.strAuthorNameJ)] UTF8String],
                [[self wcharToNS:GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strGameNameE,VGMTag.strGameNameJ)] UTF8String],
                [[self wcharToNS:GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strSystemNameE,VGMTag.strSystemNameJ)] UTF8String],
                [[self wcharToNS:GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strTrackNameE,VGMTag.strTrackNameJ)] UTF8String],
                [[self wcharToNS:VGMTag.strReleaseDate] UTF8String],
                [[self wcharToNS:VGMTag.strCreator] UTF8String],
                [[self wcharToNS:VGMTag.strNotes] UTF8String]);
    
    
    iModuleLength=(VGMHead.lngTotalSamples+VGMMaxLoopM*VGMHead.lngLoopSamples)*10/441;//ms
    //NSLog(@"VGM length %d",iModuleLength);
    iCurrentTime=0;
    numChannels=2;//asap.moduleInfo.channels;
    mod_minsub=0;
    mod_maxsub=0;
    mod_subsongs=1;
    
    sprintf(mod_name,"");
    if (GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strTrackNameE,VGMTag.strTrackNameJ)[0]) sprintf(mod_name," %s",[[self wcharToNS:GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strTrackNameE,VGMTag.strTrackNameJ)] UTF8String]);
    if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
    
    //Loop
    if (mLoopMode==1) {
        iModuleLength=-1;
    }
    
    return 0;
}

-(int) mmp_pmdminiLoad:(NSString*)filePath {
    char tmp_mod_name[1024];
    tmp_mod_name[0] = 0;
    char tmp_mod_message[1024];
    tmp_mod_message[0] = 0;
    
    mPlayType=MMP_PMDMINI;
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
        return 1;
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
        if (tmp_mod_name[0]) sprintf(mod_name," %s",[[self sjisToNS:tmp_mod_name] UTF8String]);
        else sprintf(mod_name," %s",mod_filename);
        
        pmd_get_compo(tmp_mod_message);
        if (tmp_mod_message[0]) sprintf(mod_message,"Title: %s\nComposer: %s",[[self sjisToNS:tmp_mod_name] UTF8String], [[self sjisToNS:tmp_mod_message] UTF8String]);
        
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

-(int) mmp_dumbLoad:(NSString*)filePath { //DUMB
    mPlayType=MMP_DUMB;
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

-(int) mmp_asapLoad:(NSString*)filePath { //ASAP
    mPlayType=MMP_ASAP;
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
    
    if (!ASAP_Load(asap, [filePath UTF8String], ASAP_module, ASAP_module_len)) {
        NSLog(@"Cannot ASAP_Load file %@",filePath);
        mPlayType=0;
        return -2;
    }
    
    md5_from_buffer(song_md5,33,(char*)ASAP_module,ASAP_module_len);
    song_md5[32]=0;
    
    song = asap->moduleInfo.defaultSong;
    duration = asap->moduleInfo.durations[song];
    ASAP_PlaySong(asap, song, duration);
    mod_currentsub=song;
    
    sprintf(mod_message,"Author:%s\nTitle:%s\nSongs:%d\nChannels:%d\n",asap->moduleInfo.author,asap->moduleInfo.title,asap->moduleInfo.songs,asap->moduleInfo.channels);
    
    iModuleLength=duration;
    iCurrentTime=0;
    numChannels=asap->moduleInfo.channels;
    mod_minsub=0;
    mod_maxsub=asap->moduleInfo.songs-1;
    mod_subsongs=asap->moduleInfo.songs;
    
    if (asap->moduleInfo.title[0]) sprintf(mod_name," %s",asap->moduleInfo.title);
    else sprintf(mod_name," %s",mod_filename);
    
    stil_info[0]=0;
    [self getStilAsmaInfo:(char*)[filePath UTF8String]];
    
    sprintf(mod_message,"%s\n[STIL Information]\n%s\n",mod_message,stil_info);
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    return 0;
}

-(int) mmp_adplugLoad:(NSString*)filePath {
    mPlayType=MMP_ADPLUG;
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
        /*        for (int i=0;i<[filetype_extDUMB count];i++) { //Try Dumb if applicable
         if ([extension caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=MMP_DUMB;break;}
         if ([file_no_ext caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=MMP_DUMB;break;}
         }
         for (int i=0;i<[filetype_extUADE count];i++) { //Try UADE if applicable
         if ([extension caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {found=MMP_UADE;break;}
         if ([file_no_ext caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {found=MMP_UADE;break;}
         }*/
        return -1;
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

-(int) mmp_gmeLoad:(NSString*)filePath {
    long sample_rate = (mSlowDevice?PLAYBACK_FREQ/2:PLAYBACK_FREQ); /* number of samples per second */
    int track = 0; /* index of track to play (0 = first) */
    gme_err_t err;
    mPlayType=MMP_GME;
    
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
        gme_ignore_silence(gme_emu,optGMEIgnoreSilence);
        
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
        
        //////////////////////////////////
        //update DB with songlength
        //////////////////////////////////
        mod_total_length=0;
        for (int i=0;i<mod_subsongs; i++) {
            if (gme_track_info( gme_emu, &gme_info, i )==0) {
                short int playcount;
                signed char rating;
                int song_length;
                char channels_nb;
                int songs;
                NSString *filePathGME;
                NSString *fileName;
                NSMutableArray *tmp_path;
                int gme_subsong_length=gme_info->play_length;
                if (gme_info->play_length<=0) gme_info->play_length=optGENDefaultLength;
                mod_total_length+=gme_info->play_length;
                
                channels_nb=gme_voice_count( gme_emu );
                
                gme_free_info(gme_info);
                
                fileName=[self getSubTitle:i];
                
                tmp_path=[NSMutableArray arrayWithArray:[filePath componentsSeparatedByString:@"/"]];
                for (;;) {
                    if ([(NSString *)[tmp_path firstObject] compare:@"Documents"]==NSOrderedSame) {
                        break;
                    }
                    [tmp_path removeObjectAtIndex:0];
                    if ([tmp_path count]==0) break;
                }
                filePathGME=[tmp_path componentsJoinedByString:@"/"];
                
                NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathGME,i];
                
                DBHelper::getFileStatsDBmod(fileName,filePathSubsong,&playcount,&rating,&song_length,&channels_nb,&songs);
                //NSLog(@"%@||%@||sl:%d||ra:%d",fileName,filePathSubsong,gme_subsong_length,rating);
                
                DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,playcount,rating,gme_subsong_length,gme_voice_count( gme_emu ),mod_subsongs);
                
                if (i==mod_subsongs-1) {// Global file stats update
                    fileName=[filePath lastPathComponent];
                    DBHelper::getFileStatsDBmod(fileName,filePathGME,&playcount,&rating,&song_length,&channels_nb,&songs);
                    
                    //NSLog(@"%@||%@||sl:%d||ra:%d",fileName,filePathGME,mod_total_length,rating);
                    
                    DBHelper::updateFileStatsDBmod(fileName,filePathGME,playcount,rating,mod_total_length,gme_voice_count( gme_emu ),mod_subsongs);
                }
            }
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
            
            sprintf(mod_name," %s",mod_filename);
            if (gme_info->song){
                if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);
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

static int mdz_ArchiveFiles_compare(const void *e1, const void *e2) {
    
    const char **pa = (const char**)e1;
    const char **pb = (const char**)e2;
    
    return strcasecmp(*pa,*pb);
}

-(NSString*) getFullFilePath:(NSString *)_filePath {
    NSString *fullFilePath;
    
    if ([_filePath length]>2) {
        if (([_filePath characterAtIndex:0]=='/')&&([_filePath characterAtIndex:1]=='/')) fullFilePath=[_filePath substringFromIndex:2];
        else fullFilePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
    } else fullFilePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
    return fullFilePath;
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
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extDUMB=[SUPPORTED_FILETYPE_DUMB componentsSeparatedByString:@","];
    NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extSEXYPSF=[SUPPORTED_FILETYPE_SEXYPSF componentsSeparatedByString:@","];
    NSArray *filetype_extLAZYUSF=[SUPPORTED_FILETYPE_LAZYUSF componentsSeparatedByString:@","];
    NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","];
    NSArray *filetype_extSNSF=[SUPPORTED_FILETYPE_SNSF componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSArray *filetype_extMPG123=[SUPPORTED_FILETYPE_MPG123 componentsSeparatedByString:@","];
    NSArray *filetype_extAOSDK=[SUPPORTED_FILETYPE_AOSDK componentsSeparatedByString:@","];
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    
    NSString *extension;
    NSString *file_no_ext;
    NSString *filePath;
    NSMutableArray *available_player=[NSMutableArray arrayWithCapacity:1];
    
    int found=0;
    
    mplayer_error_msg[0]=0;
    mSingleSubMode=singleSubMode;
    
    [self iPhoneDrv_LittlePlayStart];
    
    if (archiveMode==0) {
        //extension = [_filePath pathExtension];
        //file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
        
        NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
        extension = (NSString *)[temparray_filepath lastObject];
        [temparray_filepath removeLastObject];
        file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
        
        
        
        //filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
        filePath=[self getFullFilePath:_filePath];
        
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
        iModuleLength=0;
        mPatternDataAvail=0;
        
        sprintf(mod_filename,"%s",[[[filePath lastPathComponent] stringByDeletingPathExtension] UTF8String]);
        
        
        mSlowDevice=slowDevice;
        
        if (mdz_ArchiveFilesCnt) {
            for (int i=0;i<mdz_ArchiveFilesCnt;i++) free(mdz_ArchiveFilesList[i]);
            free(mdz_ArchiveFilesList);
            mdz_ArchiveFilesList=NULL;
            //for (int i=0;i<mdz_ArchiveFilesCnt;i++) free(mdz_ArchiveFilesListAlias[i]);
            //free(mdz_ArchiveFilesListAlias);
            //mdz_ArchiveFilesListAlias=NULL;
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
                            [self fex_extractSingleFileToPath:[filePath UTF8String] path:[tmpArchivePath UTF8String] file_index:archiveIndex];
                        } else {
                            
                            [self fex_extractToPath:[filePath UTF8String] path:[tmpArchivePath UTF8String]];
                        }
                    } else {
                    }
                    mdz_currentArchiveIndex=0;
                    
                    //sort the file list
                    if (mdz_ArchiveFilesCnt>1) qsort(mdz_ArchiveFilesList, mdz_ArchiveFilesCnt, sizeof(char*), &mdz_ArchiveFiles_compare);

                    
                    if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) mdz_currentArchiveIndex=archiveIndex;
                    _filePath=[NSString stringWithFormat:@"tmp/tmpArchive/%@",[NSString stringWithUTF8String:mdz_ArchiveFilesList[mdz_currentArchiveIndex]]];
                    //extension = [_filePath pathExtension];
                    //file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
                    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
                    extension = (NSString *)[temparray_filepath lastObject];
                    [temparray_filepath removeLastObject];
                    file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
                    
                    
                    //filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
                    filePath=[self getFullFilePath:_filePath];
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
                    
                    //sort the file list
                    if (mdz_ArchiveFilesCnt>1) qsort(mdz_ArchiveFilesList, mdz_ArchiveFilesCnt, sizeof(char*), &mdz_ArchiveFiles_compare);

                    
                    if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) mdz_currentArchiveIndex=archiveIndex;
                    _filePath=[NSString stringWithFormat:@"tmp/tmpArchive/%@",[NSString stringWithUTF8String:mdz_ArchiveFilesList[mdz_currentArchiveIndex]]];
                    //extension = [_filePath pathExtension];
                    //file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
                    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
                    extension = (NSString *)[temparray_filepath lastObject];
                    [temparray_filepath removeLastObject];
                    file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
                    
                    
                    //filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
                    filePath=[self getFullFilePath:_filePath];
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
        iModuleLength=0;
        mPatternDataAvail=0;
        
        if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) mdz_currentArchiveIndex=archiveIndex;
        
        _filePath=[NSString stringWithFormat:@"tmp/tmpArchive/%@",[NSString stringWithUTF8String:mdz_ArchiveFilesList[mdz_currentArchiveIndex]]];
        //extension = [_filePath pathExtension];
        //file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
        NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
        extension = (NSString *)[temparray_filepath lastObject];
        [temparray_filepath removeLastObject];
        file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
        
        
        //filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
        filePath=[self getFullFilePath:_filePath];
        sprintf(mod_filename,"%s/%s",archive_filename,[[filePath lastPathComponent] UTF8String]);
    }
    
    if (archiveIndex&&mdz_IsArchive) {
        if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) mdz_currentArchiveIndex=archiveIndex;
        
        _filePath=[NSString stringWithFormat:@"tmp/tmpArchive/%@",[NSString stringWithUTF8String:mdz_ArchiveFilesList[mdz_currentArchiveIndex]]];
        //extension = [_filePath pathExtension];
        //file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
        NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
        extension = (NSString *)[temparray_filepath lastObject];
        [temparray_filepath removeLastObject];
        file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
        
        
        //filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
        filePath=[self getFullFilePath:_filePath];
        sprintf(mod_filename,"%s/%s",archive_filename,[[filePath lastPathComponent] UTF8String]);
    }
    
    
    for (int i=0;i<[filetype_extVGM count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultVGMPLAYER==DEFAULT_VGMVGM) [available_player insertObject:[NSNumber numberWithInt:MMP_VGMPLAY] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_VGMPLAY]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultVGMPLAYER==DEFAULT_VGMVGM) [available_player insertObject:[NSNumber numberWithInt:MMP_VGMPLAY] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_VGMPLAY]];
            break;
        }
    }
    for (int i=0;i<[filetype_extASAP count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultSAPPLAYER==DEFAULT_SAPASAP) [available_player insertObject:[NSNumber numberWithInt:MMP_ASAP] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_ASAP]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultSAPPLAYER==DEFAULT_SAPASAP) [available_player insertObject:[NSNumber numberWithInt:MMP_ASAP] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_ASAP]];
            break;
        }
    }
    for (int i=0;i<[filetype_extGME count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {
            bool is_vgm=false;
            for (int j=0;j<[filetype_extVGM count];j++)
                if ([extension caseInsensitiveCompare:[filetype_extVGM objectAtIndex:j]]==NSOrderedSame) {
                    is_vgm=true;
                    break;
                }
            bool is_sap=false;
            for (int j=0;j<[filetype_extASAP count];j++)
                if ([extension caseInsensitiveCompare:[filetype_extASAP objectAtIndex:j]]==NSOrderedSame) {
                    is_sap=true;
                    break;
                }
            
            if ( (is_vgm && (mdz_defaultVGMPLAYER==DEFAULT_VGMGME)) ||
                (is_sap && (mdz_defaultSAPPLAYER==DEFAULT_SAPGME)) ) [available_player insertObject:[NSNumber numberWithInt:MMP_GME] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_GME]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {
            bool is_vgm=false;
            for (int j=0;j<[filetype_extVGM count];j++)
                if ([file_no_ext caseInsensitiveCompare:[filetype_extVGM objectAtIndex:j]]==NSOrderedSame) {
                    is_vgm=true;
                    break;
                }
            bool is_sap=false;
            for (int j=0;j<[filetype_extASAP count];j++)
                if ([file_no_ext caseInsensitiveCompare:[filetype_extASAP objectAtIndex:j]]==NSOrderedSame) {
                    is_sap=true;
                    break;
                }
            
            if ( (is_vgm && (mdz_defaultVGMPLAYER==DEFAULT_VGMGME)) ||
                (is_sap && (mdz_defaultSAPPLAYER==DEFAULT_SAPGME)) ) [available_player insertObject:[NSNumber numberWithInt:MMP_GME] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_GME]];
            break;
        }
    }
    
    for (int i=0;i<[filetype_extMDX count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_MDXPDX]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_MDXPDX]];
            break;
        }
    }
    
    for (int i=0;i<[filetype_extSTSOUND count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_STSOUND]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_STSOUND]];
            break;
        }
    }
    for (int i=0;i<[filetype_extSC68 count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_SC68]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_SC68]];
            break;
        }
    }
    for (int i=0;i<[filetype_extSEXYPSF count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extSEXYPSF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_SEXYPSF]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extSEXYPSF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_SEXYPSF]];
            break;
        }
    }
    for (int i=0;i<[filetype_extLAZYUSF count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extLAZYUSF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_LAZYUSF]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extLAZYUSF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_LAZYUSF]];
            break;
        }
    }
    for (int i=0;i<[filetype_ext2SF count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_ext2SF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_2SF]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_ext2SF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_2SF]];
            break;
        }
    }
    for (int i=0;i<[filetype_extSNSF count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extSNSF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_SNSF]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extSNSF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_SNSF]];
            break;
        }
    }
    for (int i=0;i<[filetype_extAOSDK count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extAOSDK objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_AOSDK]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extAOSDK objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_AOSDK]];
            break;
        }
    }
    for (int i=0;i<[filetype_extVGMSTREAM count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_VGMSTREAM]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_VGMSTREAM]];
            break;
        }
    }
    for (int i=0;i<[filetype_extMPG123 count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extMPG123 objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_MPG123]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extMPG123 objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_MPG123]];
            break;
        }
    }
    for (int i=0;i<[filetype_extGSF count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_GSF]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_GSF]];
            break;
        }
    }
    for (int i=0;i<[filetype_extWMIDI count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_TIMIDITY]];break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_TIMIDITY]];break;
        }
    }
    for (int i=0;i<[filetype_extXMP count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultMODPLAYER==DEFAULT_XMP) [available_player insertObject:[NSNumber numberWithInt:MMP_XMP] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_XMP]];
            found=MMP_XMP;
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultMODPLAYER==DEFAULT_XMP) [available_player insertObject:[NSNumber numberWithInt:MMP_XMP] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_XMP]];
            found=MMP_XMP;
            break;
        }
    }
    
    for (int i=0;i<[filetype_extUADE count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultMODPLAYER==DEFAULT_UADE) [available_player insertObject:[NSNumber numberWithInt:MMP_UADE] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_UADE]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultMODPLAYER==DEFAULT_UADE) [available_player insertObject:[NSNumber numberWithInt:MMP_UADE] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_UADE]];
            break;
        }
    }
    for (int i=0;i<[filetype_extDUMB count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultMODPLAYER==DEFAULT_DUMB) [available_player insertObject:[NSNumber numberWithInt:MMP_DUMB] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_DUMB]];
            found=MMP_DUMB;
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultMODPLAYER==DEFAULT_DUMB) [available_player insertObject:[NSNumber numberWithInt:MMP_DUMB] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_DUMB]];
            found=MMP_DUMB;
            break;
        }
    }
    for (int i=0;i<[filetype_extMODPLUG count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultMODPLAYER==DEFAULT_MODPLUG) [available_player insertObject:[NSNumber numberWithInt:MMP_OPENMPT] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_OPENMPT]];
            found=MMP_OPENMPT;
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultMODPLAYER==DEFAULT_MODPLUG) [available_player insertObject:[NSNumber numberWithInt:MMP_OPENMPT] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_OPENMPT]];
            found=MMP_OPENMPT;
            break;
        }
    }
    for (int i=0;i<[filetype_extHVL count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_HVL]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_HVL]];
            break;
        }
    }
    
    //****************************************************************************************
    //CONSTRAINTS
    // s3m (modules) -> try ADPLUG first and then others (ex: .s3m with adlib instrument only)
    // pmd -> try PMDMINI first and then ADPLUG
    // sid -> try sidplay first and the UADE (some sidmon1 or sidmon2 files not well named (sid instead of sid1/sid2)
    //
    //CONSEQUENCES in terms of LOADING ORDER
    // 1. PMDMINI
    // 2. ADPLUG
    // 3. SID
    // 3. The rest
    //****************************************************************************************
    
    for (int i=0;i<[filetype_extSID count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {
            [available_player insertObject:[NSNumber numberWithInt:MMP_SIDPLAY] atIndex:0];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {
            [available_player insertObject:[NSNumber numberWithInt:MMP_SIDPLAY] atIndex:0];
            break;
        }
    }
    
        
    for (int i=0;i<[filetype_extADPLUG count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {
            [available_player insertObject:[NSNumber numberWithInt:MMP_ADPLUG] atIndex:0];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {
            [available_player insertObject:[NSNumber numberWithInt:MMP_ADPLUG] atIndex:0];
            break;
        }
    }
    
    for (int i=0;i<[filetype_extPMD count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {
            [available_player insertObject:[NSNumber numberWithInt:MMP_PMDMINI] atIndex:0];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {
            [available_player insertObject:[NSNumber numberWithInt:MMP_PMDMINI] atIndex:0];
            break;
        }
    }

    
    //NSLog(@"Loading file:%@ ",filePath);
    //NSLog(@"Loading file:%@ ext:%@",file_no_ext,extension);
    
    mod_total_length=0;
    
    for (int i=0;i<[available_player count];i++) {
        int pl_idx=[((NSNumber*)[available_player objectAtIndex:i]) intValue];
        int successful_loading=0;
        //NSLog(@"pl_idx: %d",i);
        switch (pl_idx) {
            case MMP_TIMIDITY:
                if ([self mmp_timidityLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_VGMSTREAM:
                if ([self mmp_vgmstreamLoad:filePath extension:extension]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_MPG123:
                if ([self mmp_mpg123Load:filePath extension:extension]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_LAZYUSF:
                if ([self mmp_lazyusfLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_2SF:
                if ([self mmp_2sfLoad:filePath extension:extension]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_SNSF:
                if ([self mmp_snsfLoad:filePath extension:extension]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_VGMPLAY:
                if ([self mmp_vgmplayLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_PMDMINI:
                if ([self mmp_pmdminiLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_DUMB:
                if ([self mmp_dumbLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_ASAP:
                if ([self mmp_asapLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_GSF:
                if ([self mmp_gsfLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_MDXPDX:
                if ([self mmp_mdxpdxLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_SC68:
                if ([self mmp_sc68Load:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_STSOUND:
                if ([self mmp_stsoundLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_SIDPLAY:
                if ([self mmp_sidplayLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_HVL:
                if ([self mmp_hvlLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_XMP:
                if ([self mmp_xmpLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_UADE:
                if ([self mmp_uadeLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_SEXYPSF:
                if ([self mmp_sexypsfLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                else return 1;
                break;
            case MMP_AOSDK:
                if ([self mmp_aosdkLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_ADPLUG:
                if ([self mmp_adplugLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_OPENMPT:
                if ([self mmp_openmptLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_GME:
                if ([self mmp_gmeLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            default:
                NSLog(@"Unsupported player: %d",pl_idx); //Should never happen
                break;
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
    if (mPlayType!=MMP_TIMIDITY) { //hack for timidity : iModuleLength is unknown at this stage
        if (startPos>iModuleLength-SEEK_START_MARGIN_FROM_END) {
            startPos=iModuleLength-SEEK_START_MARGIN_FROM_END;
        }
    }
    if (startPos<0) startPos=0;
    
    switch (mPlayType) {
        case MMP_GME:  //GME
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
                        if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);
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
        case MMP_XMP:  //XMP
            if (startPos) [self Seek:startPos];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_OPENMPT:  //MODPLUG
            if (startPos) [self Seek:startPos];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_ADPLUG:  //ADPLUG
            if (startPos) [self Seek:startPos];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_AOSDK:  //AOSDK
            [self Play];
            if (startPos) [self Seek:startPos];
            break;
        case MMP_SEXYPSF: //SexyPSF
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_UADE:  //UADE
            mod_wantedcurrentsub=subsong;
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_HVL://HVL/AHX
            moveToSubSongIndex=1;
            moveToSubSongIndex=subsong;
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_SIDPLAY: //SID
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            mSidTune->selectSong(mod_currentsub+1);
            mSidEmuEngine->load(mSidTune);
            
            iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
            if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
            mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
            if (mLoopMode) iModuleLength=-1;
            
            mod_message_updated=1;
            if (startPos) [self Seek:startPos];
            [self Play];
            
            break;
        case MMP_STSOUND:  //YM
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_SC68: //SC68
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
        case MMP_MDXPDX: //MDX
            [self Play];
            break;
        case MMP_GSF: //GSF
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_ASAP: //ASAP
            moveToSubSong=1;
            moveToSubSongIndex=subsong;
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_DUMB: //DUMB
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_TIMIDITY: //Timidity
            if (startPos) {
                //hack because midi length is unknow at this stage, preventing seek to work
                iCurrentTime=mNeedSeekTime=startPos;
                mNeedSeek=1;
            }//[self Seek:startPos];
            [self Play];
            break;
        case MMP_PMDMINI: //PMD
            [self Play];
            break;
        case MMP_VGMPLAY: //VGM
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_LAZYUSF: //LAZYUSF
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_2SF: //2SF
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_SNSF: //SNSF
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_VGMSTREAM: //VGMSTREAM
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_MPG123: //MPG123
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
    }
}
-(void) Stop {
    
    if (mPlayType==MMP_TIMIDITY) { //Timidity
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
    
    if (mPlayType==MMP_GME) {
        gme_delete( gme_emu );
        gme_emu=NULL;
    }
    if (mPlayType==MMP_XMP) {
        xmp_end_player(xmp_ctx);
        xmp_release_module(xmp_ctx);
        xmp_free_context(xmp_ctx);
        xmp_ctx=NULL;
    }
    if (mPlayType==MMP_OPENMPT) {
        if (mp_file) {
            ModPlug_Unload(mp_file);
        }
        if (mp_data) free(mp_data);
        mp_file=NULL;
    }
    if (mPlayType==MMP_ADPLUG) {
        delete adPlugPlayer;
        adPlugPlayer=NULL;
        delete opl;
        opl=NULL;
        delete adplugDB;
        adplugDB=NULL;
    }
    if (mPlayType==MMP_AOSDK) {
        (*ao_types[ao_type].stop)();
        if (ao_buffer) free(ao_buffer);
    }
    if (mPlayType==MMP_SEXYPSF) { //SexyPSF
    }
    if (mPlayType==MMP_UADE) {  //UADE
        //		NSLog(@"Wait for end of UADE thread");
        while (uadeThread_running) {
            [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
            
        }
        //		NSLog(@"ok");
        uade_unalloc_song(&UADEstate);
    }
    if (mPlayType==MMP_HVL) { //HVL
        hvl_FreeTune(hvl_song);
        hvl_song=NULL;
    }
    if (mPlayType==MMP_SIDPLAY) { //SID
        if (mSidTune) {
            delete mSidTune;
            mSidTune = NULL;
        }
        if (mSidEmuEngine) {
            //sid2_config_t cfg = mSidEmuEngine->config();
            
            //cfg.sidEmulation = NULL;
            //mSidEmuEngine->config(cfg);
            delete mBuilder;
            mBuilder = NULL;
            
            delete mSidEmuEngine;
            mSidEmuEngine = NULL;
        }
        
        if (sidtune_title) {
            for (int i=0;i<mod_subsongs;i++)
                if (sidtune_title[i]) free(sidtune_title[i]);
            free(sidtune_title);
            sidtune_title=NULL;
        }
        if (sidtune_name) {
            for (int i=0;i<mod_subsongs;i++)
                if (sidtune_name[i]) free(sidtune_name[i]);
            free(sidtune_name);
            sidtune_name=NULL;
        }
    }
    if (mPlayType==MMP_STSOUND) { //STSOUND
        ymMusicStop(ymMusic);
        ymMusicDestroy(ymMusic);
    }
    if (mPlayType==MMP_SC68) {//SC68
        api68_stop( sc68 );
        api68_close(sc68);
    }
    if (mPlayType==MMP_MDXPDX) { //MDX
        mdx_close(mdx,pdx);
    }
    if (mPlayType==MMP_GSF) { //GSF
        GSFClose();
    }
    if (mPlayType==MMP_ASAP) { //ASAP
        free(ASAP_module);
    }
    if (mPlayType==MMP_DUMB) {
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
    if (mPlayType==MMP_TIMIDITY) { //VGM
    }
    if (mPlayType==MMP_PMDMINI) { //PMD
        pmd_stop();
    }
    if (mPlayType==MMP_VGMPLAY) { //VGM
        StopVGM();
        CloseVGMFile();
        VGMPlay_Deinit();
    }
    if (mPlayType==MMP_LAZYUSF) { //LAZYUSF
        usf_shutdown(lzu_state->emu_state);
        free(lzu_sample_data);
        free(lzu_sample_data_float);
        free(lzu_sample_converted_data_float);
        free(usf_info_data);
        if (src_state) src_delete(src_state);
        src_state=NULL;
    }
    if (mPlayType==MMP_2SF) { //2SF
        delete xSFPlayer;
        delete xSFConfig;
    }
    if (mPlayType==MMP_SNSF) { //SNSF
        /*delete xSFPlayerSNSF;
        delete xSFConfigSNSF;*/
    }
    if (mPlayType==MMP_VGMSTREAM) { //VGMSTREAM
        if (vgmStream != NULL)
        {
            close_vgmstream(vgmStream);
        }
        vgmStream = NULL;
        free(vgm_sample_data);
        free(vgm_sample_data_float);
        free(vgm_sample_converted_data_float);
        if (src_state) src_delete(src_state);
        src_state=NULL;
    }
    if (mPlayType==MMP_MPG123) { //MPG123
        if (mpg123h != NULL)
        {
            mpg123_close(mpg123h);
            mpg123_delete(mpg123h);
        }
        mpg123_exit();
        mpg123h = NULL;
        free(mpg123_sample_data);
        free(mpg123_sample_data_float);
        free(mpg123_sample_converted_data_float);
        if (src_state) src_delete(src_state);
        src_state=NULL;
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
    /*if ((mPlayType==MMP_GME)||(mPlayType==MMP_AOSDK)||(mPlayType==MMP_SEXYPSF)||(mPlayType==MMP_MDXPDX)||(mPlayType==MMP_GSF)||(mPlayType==MMP_PMDMINI)||(mPlayType==MMP_VGMPLAY)||(mPlayType==MMP_LAZYUSF)||(mPlayType==MMP_2SF)||(mPlayType==MMP_SNSF)) modMessage=[NSString stringWithCString:mod_message encoding:NSShiftJISStringEncoding];
    else*/ {
        modMessage=[NSString stringWithUTF8String:mod_message];
        //if (modMessage==nil) modMessage=[NSString stringWithUTF8String:mod_message];
    }
    if (modMessage==nil) return @"";
    return modMessage;
}
-(NSString*) getModName {
    NSString *modName;
    /*if ((mPlayType==MMP_GME)||(mPlayType==MMP_AOSDK)||(mPlayType==MMP_SEXYPSF)||(mPlayType==MMP_MDXPDX)||(mPlayType==MMP_GSF)||(mPlayType==MMP_PMDMINI)||(mPlayType==MMP_VGMPLAY)||(mPlayType==MMP_LAZYUSF)||(mPlayType==MMP_2SF)||(mPlayType==MMP_SNSF)) modName=[NSString stringWithCString:mod_name encoding:NSShiftJISStringEncoding];
    else*/ {
        modName=[NSString stringWithUTF8String:mod_name];
    }
    if (modName==nil) return @"";
    return modName;
}
-(NSString*) getPlayerName {
    if (mPlayType==MMP_GME) return @"Game Music Emulator";
    if (mPlayType==MMP_OPENMPT) return @"OpenMPT";
    if (mPlayType==MMP_XMP) return @"XMP";
    if (mPlayType==MMP_ADPLUG) return @"Adplug";
    if (mPlayType==MMP_AOSDK) return @"Audio Overload";
    if (mPlayType==MMP_SEXYPSF) return @"SexyPSF";
    if (mPlayType==MMP_UADE) return @"UADE";
    if (mPlayType==MMP_HVL) return @"HVL";
    if (mPlayType==MMP_SIDPLAY) return (@"SIDPLAY/RESIDFP");
    if (mPlayType==MMP_STSOUND) return @"STSOUND";
    if (mPlayType==MMP_SC68) return @"SC68";
    if (mPlayType==MMP_MDXPDX) return @"MDX";
    if (mPlayType==MMP_GSF) return @"GSF";
    if (mPlayType==MMP_ASAP) return @"ASAP";
    if (mPlayType==MMP_DUMB) return @"DUMB";
    if (mPlayType==MMP_TIMIDITY) return @"Timidity";
    if (mPlayType==MMP_PMDMINI) return @"PMDMini";
    if (mPlayType==MMP_VGMPLAY) return @"VGMPlay";
    if (mPlayType==MMP_LAZYUSF) return @"LazyUSF";
    if (mPlayType==MMP_2SF) return @"2SF";
    if (mPlayType==MMP_SNSF) return @"SNSF";
    if (mPlayType==MMP_VGMSTREAM) return @"VGMSTREAM";
    if (mPlayType==MMP_MPG123) return @"MPG123";
    return @"";
}
-(NSString*) getSubTitle:(int)subsong {
    NSString *result;
    if ((subsong<mod_minsub)||(subsong>mod_maxsub)) return @"";
    if (mPlayType==MMP_OPENMPT) {
        const char *ret=openmpt_module_get_subsong_name(mp_file->mod, subsong);
        if (ret) {
            result=[NSString stringWithFormat:@"%.3d-%@",subsong,[NSString stringWithUTF8String:ret]];
        } else result=[NSString stringWithFormat:@"%.3d",subsong];
        return result;
    }
    if (mPlayType==MMP_GME) {
        if (gme_track_info( gme_emu, &gme_info, subsong )==0) {
            int sublen=gme_info->play_length;
            if (sublen<=0) sublen=optGENDefaultLength;
            
            result=nil;
            if (gme_info->song){
                if (gme_info->song[0]) result=[NSString stringWithFormat:@"%.3d-%@",subsong,[NSString stringWithUTF8String:gme_info->song]];
            }
            if ((!result)&&(gme_info->game)) {
                if (gme_info->game[0]) result=[NSString stringWithFormat:@"%.3d-%@",subsong,[NSString stringWithUTF8String:gme_info->game]];
            }
            if (!result) result=[NSString stringWithFormat:@"%.3d",subsong];
            gme_free_info(gme_info);
            return result;
        }
    }
    if (mPlayType==MMP_SIDPLAY) {
        if (sidtune_name) {
            if (sidtune_name[subsong]) return [NSString stringWithFormat:@"%.3d-%@",subsong,[NSString stringWithUTF8String:sidtune_name[subsong]]];
        }
        if (sidtune_title) {
            if (sidtune_title[subsong]) return [NSString stringWithFormat:@"%.3d-%@",subsong,[NSString stringWithUTF8String:sidtune_title[subsong]]];
        }
        
        const SidTuneInfo *sidtune_info;
        sidtune_info=mSidTune->getInfo();
        if (sidtune_info->infoString(0)[0]) return [NSString stringWithFormat:@"%.3d-%@",subsong,[NSString stringWithUTF8String:sidtune_info->infoString(0)]];
        
        
        
        return [NSString stringWithFormat:@"%.3d",subsong];
    }
    return [NSString stringWithFormat:@"%.3d",subsong];
}
-(NSString*) getModType {
    if (mPlayType==MMP_GME) {
        
        return [NSString stringWithFormat:@"%s",gmetype];
    }
    if (mPlayType==MMP_XMP) {
        return [NSString stringWithFormat:@"%s",xmp_mi.mod->type];
    }
    if (mPlayType==MMP_OPENMPT) {
        char *str_type=(char*)ModPlug_GetModuleTypeLStr(mp_file);
        char *str_cont=(char*)ModPlug_GetModuleContainerLStr(mp_file);
        NSString *result=[NSString stringWithFormat:@"%s %s",str_type,str_cont];
        if (str_type) free(str_type);
        if (str_cont) free(str_cont);
        return result;
    }
    if (mPlayType==MMP_ADPLUG) return [NSString stringWithFormat:@"%s",(adPlugPlayer->gettype()).c_str()];
    if (mPlayType==MMP_AOSDK) return [NSString stringWithFormat:@"%s",ao_types[ao_type].name];
    if (mPlayType==MMP_SEXYPSF) return @"PSF";
    if (mPlayType==MMP_UADE) return [NSString stringWithFormat:@"%s",UADEstate.ep->playername];
    if (mPlayType==MMP_HVL) return (hvl_song->ht_ModType?@"HVL":@"AHX");
    if (mPlayType==MMP_SIDPLAY) return @"SID";
    if (mPlayType==MMP_STSOUND) return @"YM";
    if (mPlayType==MMP_SC68) {
        api68_music_info_t info;
        api68_music_info( sc68, &info, 1, NULL );
        
        return [NSString stringWithFormat:@"%s",info.replay];
    }
    if (mPlayType==MMP_MDXPDX) {
        if (mdx->haspdx) return @"MDX/PDX";
        else return @"MDX";
    }
    if (mPlayType==MMP_GSF) return @"GSF";
    if (mPlayType==MMP_ASAP) return @"ASAP";
    if (mPlayType==MMP_DUMB) {
        const char * tag = duh_get_tag(duh, "FORMAT");
        if (tag && *tag) return [NSString stringWithFormat:@"%s",tag];
        else return @"mod?";
    }
    if (mPlayType==MMP_TIMIDITY) return @"MIDI";
    if (mPlayType==MMP_PMDMINI) return @"PMD";
    if (mPlayType==MMP_VGMPLAY) return @"VGM";
    if (mPlayType==MMP_LAZYUSF) return @"USF";
    if (mPlayType==MMP_2SF) return [NSString stringWithFormat:@"%s",mmp_fileext];
    if (mPlayType==MMP_SNSF) return [NSString stringWithFormat:@"%s",mmp_fileext];
    if (mPlayType==MMP_VGMSTREAM) return [NSString stringWithFormat:@"%s",mmp_fileext];
    if (mPlayType==MMP_MPG123) return [NSString stringWithFormat:@"%s",mmp_fileext];
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
    if ((mPlayType==MMP_OPENMPT)||(mPlayType==MMP_TIMIDITY)/*&&(bGlobalIsPlaying)*/) return 1;
    return 0;
}
-(BOOL) isEndReached{
    UInt32 i,datasize;
    datasize=sizeof(UInt32);
    AudioQueueGetProperty(mAudioQueue,kAudioQueueProperty_IsRunning,&i,&datasize);
    if (i==0) {
        //NSLog(@"end reached");
        return YES;
    }
    //NSLog(@"end not reached");
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
//GSF
///////////////////////////
-(void) optGSF_UpdateParam {
    soundLowPass = optGSFsoundLowPass;
    soundEcho = optGSFsoundEcho;
    //soundQuality = optGSFsoundQuality;//1:44Khz, 2:22Khz, 4:11Khz
    //soundInterpolation = optGSFsoundInterpolation;
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
-(void) optGME_IgnoreSilence:(int)ignoreSilence {
    optGMEIgnoreSilence=ignoreSilence;
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
-(void) optUADE_NTSC:(int)isOn {
    mUADE_OptNTSC=isOn;
    //if (isOn) uade_effect_enable(&(UADEstate.effects), UADE_EFFECT_NORMALISE);
    //else uade_effect_disable(&(UADEstate.effects), UADE_EFFECT_NORMALISE);
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
//VGMPLAY
///////////////////////////
-(void) optVGMPLAY_MaxLoop:(unsigned int)val {
    optVGMPLAY_maxloop=val;
}
-(void) optVGMPLAY_YM2612emulator:(unsigned char)val {
    optVGMPLAY_ym2612emulator=val;
}
-(void) optVGMPLAY_PreferedJTag:(bool)val {
    optVGMPLAY_preferJapTag=val;
}

///////////////////////////
//VGMSTREAM
///////////////////////////
-(void) optVGMSTREAM_MaxLoop:(double)val {
    optVGMSTREAM_loop_count=val;
}
-(void) optVGMSTREAM_ForceLoop:(unsigned int)val {
    optVGMSTREAM_loopmode=val;
}
-(void) optVGMSTREAM_ResampleQuality:(unsigned int)val {
    optVGMSTREAM_resampleQuality=val;
}
///////////////////////////
//MPG123
///////////////////////////
-(void) optMPG123_MaxLoop:(double)val {
    optMPG123_loop_count=val;
}
-(void) optMPG123_ForceLoop:(unsigned int)val {
    optMPG123_loopmode=val;
}
-(void) optMPG123_ResampleQuality:(unsigned int)val {
    optMPG123_resampleQuality=val;
}
    

    
///////////////////////////
//LAZYUSF
///////////////////////////
-(void) optLAZYUSF_ResampleQuality:(unsigned int)val {
    optLAZYUSF_ResampleQuality=val;
}

///////////////////////////
//MODPLUG
///////////////////////////
-(ModPlug_Settings*) getMPSettings {
    if (mPlayType==MMP_OPENMPT) ModPlug_GetSettings(&mp_settings);
    return &mp_settings;
}
-(void) updateMPSettings {
    if (mPlayType==MMP_OPENMPT) {
        //IOS_OPENMPT_TODO  ModPlug_SetSettings(mp_file,&mp_settings);
        ModPlug_SetSettings(&mp_settings);
    }
}
-(void) setModPlugMasterVol:(float) mstVol {
    if ((mPlayType==MMP_OPENMPT)&&(mp_file)) ModPlug_SetMasterVolume(mp_file,(int )(mstVol*256));
}

///////////////////////////
-(void) setLoopInf:(int)val {
    mLoopMode=val;
}
-(void) Seek:(int) seek_time {
    if ((mPlayType==MMP_AOSDK)||(mPlayType==MMP_UADE)  ||(mPlayType==MMP_MDXPDX)||(mPlayType==MMP_GSF)||(mPlayType==MMP_PMDMINI)||mNeedSeek) return;
    
    if (mPlayType==MMP_STSOUND) {
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
        return [NSString stringWithUTF8String:mdz_ArchiveFilesList[arc_index]];
        //return [NSString stringWithFormat:@"%s",mdz_ArchiveFilesListAlias[arc_index]];
    } else return @"";
    
}

-(int) getGlobalLength {
    if (mod_total_length) return mod_total_length;
    return -1;
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
        case MMP_OPENMPT://Modplug
            res=ModPlug_GetChannelVolume( mp_file,channel);
            break;
        case MMP_XMP://XMP
            res=xmp_channel_vol(xmp_ctx, channel,-1);
            break;
        case MMP_DUMB://DUMB
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
