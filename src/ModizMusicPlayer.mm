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

int64_t iModuleLength;
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
#include "sidplayfp/SidConfig.h"

#include "builders/residfp-builder/residfp.h"
#include "builders/resid-builder/resid.h"




static char **sidtune_title,**sidtune_name;
signed char *m_voice_buff[SOUND_MAXVOICES_BUFFER_FX];
signed char *m_voice_buff_ana[SOUND_BUFFER_NB];
signed char *m_voice_buff_ana_cpy[SOUND_BUFFER_NB];
int m_voice_ChipID[SOUND_MAXVOICES_BUFFER_FX];
int m_voice_current_ptr[SOUND_MAXVOICES_BUFFER_FX];
int m_voice_systemColor[SOUND_VOICES_MAX_ACTIVE_CHIPS];
int m_voice_voiceColor[SOUND_MAXVOICES_BUFFER_FX];
signed char m_voice_current_system,m_voice_current_systemSub;
char m_voice_current_systemPairedOfs;
char mSIDSeekInProgress;
char m_voicesDataAvail;
char m_voicesStatus[SOUND_MAXMOD_CHANNELS];
int m_voicesForceOfs;






/* file types */
static char gmetype[64];

static uint32 ao_type;
static volatile int moveToPrevSubSong,moveToNextSubSong,mod_wantedcurrentsub,mChangeOfSong,mNewModuleLength,moveToSubSong,moveToSubSongIndex;
static int sampleVolume,mInterruptShoudlRestart;
//static volatile int genCurOffset,genCurOffsetCnt;
static char str_name[1024];
static char *stil_info;//[MAX_STIL_DATA_LENGTH];
char *mod_message;//[8192+MAX_STIL_DATA_LENGTH];
static char mod_name[256];
static char mod_filename[512];
static NSString *mod_title;
static char archive_filename[512];

static int mSingleFileType;

static char song_md5[33];

char mplayer_error_msg[1024];

static int mSingleSubMode;


#define DEFAULT_MODPLUG 0
#define DEFAULT_UADE 1
#define DEFAULT_XMP 2

#define DEFAULT_SAPASAP 0
#define DEFAULT_SAPGME 1

#define DEFAULT_VGMVGM 0
#define DEFAULT_VGMGME 1

static int mdz_IsArchive,mdz_ArchiveFilesCnt,mdz_currentArchiveIndex;
static int mdz_defaultMODPLAYER,mdz_defaultSAPPLAYER,mdz_defaultVGMPLAYER;

static char vgmplay_activeChips[SOUND_VOICES_MAX_ACTIVE_CHIPS];
static char vgmplay_activeChipsID[SOUND_VOICES_MAX_ACTIVE_CHIPS];
static char *vgmplay_activeChipsName[SOUND_VOICES_MAX_ACTIVE_CHIPS];
static char vgmplay_activeChipsNb;
static char **mdz_ArchiveFilesList;
//static char **mdz_ArchiveFilesListAlias;

static NSFileManager *mFileMngr;

//GENERAL
static int mPanning,mPanningValue;

//2SF
static const XSFFile *xSFFile = nullptr;
static XSFPlayer *xSFPlayer = nullptr;
XSFConfig *xSFConfig = nullptr;

auto xsfSampleBuffer = std::vector<uint8_t>(SOUND_BUFFER_SIZE_SAMPLE*2*2);

int optHC_ResampleQuality=1;

/////////////////////////////////
// V2M Player
/////////////////////////////////
#include "v2mplayer.h"
#include "libv2.h"
#include "v2mconv.h"
#include "sounddef.h"

int v2m_voices_mask;
static float *v2m_sample_data_float;

static V2MPlayer *v2m_player;
/////////////////////////////////


extern "C" {
    //VGMPPLAY
    extern CHIPS_OPTION ChipOpts[0x02];
    extern bool EndPlay;
    extern char* AppPaths[8];

/*
"SN76496", "YM2413", "YM2612", "YM2151", "SegaPCM", "RF5C68", "YM2203", "YM2608",
"YM2610", "YM3812", "YM3526", "Y8950", "YMF262", "YMF278B", "YMF271", "YMZ280B",
"RF5C164", "PWM", "AY8910", "GameBoy", "NES APU", "YMW258", "uPD7759", "OKIM6258",
"OKIM6295", "K051649", "K054539", "HuC6280", "C140", "K053260", "Pokey", "QSound",
"SCSP", "WSwan", "VSU", "SAA1099", "ES5503", "ES5506", "X1-010", "C352",
"GA20"*/
const UINT8 vgmCHN_COUNT[CHIP_COUNT] =
{    0x04, 0x09, 0x06, 0x08, 0x10, 0x08, 0x06, 0x10,
    0x0E, 0x09, 0x09, 0x09, 0x17, 0x2F, 0x0C, 0x08,
    0x08, 0x02, 0x03, 0x04, 0x05, 0x1C, 0x01, 0x01,
    0x04, 0x05, 0x08, 0x06, 0x18, 0x04, 0x04, 0x10,
    0x20, 0x04, 0x06, 0x06, 0x20, 0x20, 0x10, 0x20,
    0x04
};

const UINT8 vgmREALCHN_COUNT[CHIP_COUNT] =
{    0x04, 0x09, 0x06, 0x08, 0x10, 0x08, 0x06, 0x10,
    0x0E, 0x09, 0x09, 0x09, 0x12, 0x2A, 0x0C, 0x08,
    0x08, 0x02, 0x03, 0x04, 0x05, 0x1C, 0x01, 0x01,
    0x04, 0x05, 0x08, 0x06, 0x18, 0x04, 0x04, 0x10,
    0x20, 0x04, 0x06, 0x06, 0x20, 0x20, 0x10, 0x20,
    0x04
};

char vgmVRC7,vgm2610b;

//
// Get number of Voices / mute capacity
UINT8 vgmGetVoicesNb(UINT8 chipId) {
    if ((chipId==1)&&(vgmVRC7)) return 6;
    if ((chipId==8)&&(vgm2610b)) return 16;
    return vgmCHN_COUNT[chipId];
}

//
// Get number of Voices / data buffer
UINT8 vgmGetVoicesChannelsUsedNb(UINT8 chipId) {
    if ((chipId==1)&&(vgmVRC7)) return 6;
    if ((chipId==8)&&(vgm2610b)) return 16;
    return vgmREALCHN_COUNT[chipId];
}
    
    //VGMSTREAM
#import "../libs/libvgmstream/vgmstream.h"
#import "../libs/libvgmstream/plugins.h"
    
    static VGMSTREAM* vgmStream = NULL;
    static STREAMFILE* vgmFile = NULL;
    bool optVGMSTREAM_loopmode = false;
    int optVGMSTREAM_loop_count = 2.0f;
    int optVGMSTREAM_fadeouttime=5;
    int optVGMSTREAM_resampleQuality=1;
    
    static bool mVGMSTREAM_force_loop;
    static volatile int64_t mVGMSTREAM_total_samples,mVGMSTREAM_seek_needed_samples,mVGMSTREAM_decode_pos_samples,mVGMSTREAM_totalinternal_samples;
    
    //xmp
#include "xmp.h"
    static xmp_context xmp_ctx;
    static struct xmp_module_info *xmp_mi;
}


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
static int mSIDFilterON,mSIDForceLoop;
static int mSIDSecondSIDAddress,mSIDThirdSIDAddress;
static sidplayfp *mSidEmuEngine;
sidbuilder *mBuilder;
//static ReSIDfpBuilder *mBuilder;
static SidTune *mSidTune;

static int sid_forceClock;
static int sid_forceModel;
static char sid_engine,sid_interpolation;

//GME
static gme_info_t* gme_info;
static gme_equalizer_t gme_eq;
static gme_effects_t gme_fx;


int uade_song_end_trigger;


/****************/

void gsf_update(unsigned char *pSound,int lBytes);
extern "C" char gsf_libfile[1024];

extern "C" GD3_TAG VGMTag;
extern "C" VGM_HEADER VGMHead;
extern "C" INT32 VGMMaxLoop,VGMMaxLoopM;
extern "C" UINT32 FadeTime;

static int vgmGetChipsDetails(char *str,UINT8 ChipID, UINT8 SubType, UINT32 Clock)
{
    if (! Clock)
        return 0;
    
    if (ChipID == 0x00 && (Clock & 0x80000000))
        Clock &= ~0x40000000;
    if (Clock & 0x80000000)
    {
        Clock &= ~0x80000000;
        ChipID |= 0x80;
    }
    
    sprintf(str,"%s", GetAccurateChipName(ChipID, SubType));
    if (Clock & 0x40000000) return 2;
    return 1;
}

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
    
}

//ASAP
static unsigned char *ASAP_module;
static int ASAP_module_len;
static struct ASAP *asap;


extern "C" {
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


static int16_t *vgm_sample_data;
static float *vgm_sample_data_float;
static float *vgm_sample_converted_data_float;
static char mmp_fileext[8];

#import "hebios.h"

#import "../libs/libpsflib/psflib/psflib.h"
#import "../libs/libpsflib/psflib/psf2fs.h"

inline unsigned get_be16( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [0] << 8 |
            (unsigned) ((unsigned char const*) p) [1];
}

inline unsigned get_le32( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [3] << 24 |
            (unsigned) ((unsigned char const*) p) [2] << 16 |
            (unsigned) ((unsigned char const*) p) [1] <<  8 |
            (unsigned) ((unsigned char const*) p) [0];
}

inline unsigned get_be32( void const* p )
{
    return  (unsigned) ((unsigned char const*) p) [0] << 24 |
            (unsigned) ((unsigned char const*) p) [1] << 16 |
            (unsigned) ((unsigned char const*) p) [2] <<  8 |
            (unsigned) ((unsigned char const*) p) [3];
}

inline void set_le32( void* p, unsigned n )
{
    ((unsigned char*) p) [0] = (unsigned char) n;
    ((unsigned char*) p) [1] = (unsigned char) (n >> 8);
    ((unsigned char*) p) [2] = (unsigned char) (n >> 16);
    ((unsigned char*) p) [3] = (unsigned char) (n >> 24);
}

static void * psf_file_fopen( const char * uri );
static int psf_file_fseek( void * handle, int64_t offset, int whence );
static int psf_file_fclose( void * handle );
static size_t psf_file_fread( void * buffer, size_t size, size_t count, void * handle );
static int64_t psf_file_ftell( void * handle );


const psf_file_callbacks psf_file_system =
{
    "\\/|:",
    psf_file_fopen,
    psf_file_fread,
    psf_file_fseek,
    psf_file_fclose,
    psf_file_ftell
};

char *strclean(char *str) {
    if (str==NULL) return NULL;
    char *result=strdup(str);
    bool remove;
    int i=0;
    while (result[i]) {
        remove=false;
        switch (result[i]) {
            case ' ':remove=true; break;
            case '-':remove=true; break;
            case '_':remove=true; break;
            default:break;
        }
        if (remove) {
            int j=i;
            while (result[j]) {
                result[j]=result[j+1];
                j++;
            }
        }
        i++;
    }
    return result;
}

bool strcleancmp(char *str1,char *str2) {
    bool result=false;
    char *strtmp1,*strtmp2;
    strtmp1=strclean(str1);
    strtmp2=strclean(str2);
    if (strcasecmp(strtmp1,strtmp2)==0) result=true;
    if (strtmp1) free(strtmp1);
    if (strtmp2) free(strtmp2);
    return result;
}

static void * psf_file_fopen( const char * uri )
{
    FILE *f;
    f = fopen(uri, "r");
    if (!f) {
        //search file with case insensitive
        NSString *dir=[NSString stringWithFormat:@"%s",uri];
        dir=[dir stringByDeletingLastPathComponent];
        mFileMngr=[[NSFileManager alloc] init];
        NSError *error;
        NSArray *listFiles=[mFileMngr contentsOfDirectoryAtPath:dir error:&error];
        char *strtmp=(char*)uri;
        int i=0;
        while (uri[i]) {
            if (uri[i]=='/') strtmp=(char*)uri+i+1;
            i++;
        }
        for (int i=0;i<[listFiles count];i++) {
            //NSLog(@"file: %@",(NSString*)[listFiles objectAtIndex:i]);
            if (strcleancmp(strtmp,(char*)[(NSString*)[listFiles objectAtIndex:i] UTF8String])) {
                //NSLog(@"found");
                f=fopen([[NSString stringWithFormat:@"%@/%@",dir,(NSString*)[listFiles objectAtIndex:i]] UTF8String],"r");
                break;
            }
        }
        
    }
    return f;
}

static size_t psf_file_fread( void * buffer, size_t size, size_t count, void * handle )
{
    size_t bytes_read = fread(buffer,size,count,(FILE*)handle);
    return bytes_read / size;
}

static int psf_file_fseek( void * handle, int64_t offset, int whence )
{
    int result = fseek((FILE*)handle, offset, whence);
    return result;
}

static int psf_file_fclose( void * handle )
{
    fclose((FILE*)handle);
    return 0;
}

static int64_t psf_file_ftell( void * handle )
{
    int64_t pos = ftell((FILE*) handle);
    return pos;
}


#import "../libs/highlyexperimental/highlyexperimental/Core/psx.h"
#import "../libs/highlyexperimental/highlyexperimental/Core/bios.h"
#import "../libs/highlyexperimental/highlyexperimental/Core/iop.h"
#import "../libs/highlyexperimental/highlyexperimental/Core/r3000.h"

#import "../libs/highlytheoritical/highlytheoritical/sega.h"

#import "../libs/HighlyQuixotic/HighlyQuixotic/qsound.h"
extern "C" {
int HC_voicesMuteMask1;
int HC_voicesMuteMask2;
}


static int HC_type;
static uint8_t *HC_emulatorCore;
static void *HC_emulatorExtra;

struct psf_info_meta_state
{
    NSMutableDictionary * info;
    
    bool utf8;
    
    int tag_length_ms;
    int tag_fade_ms;
    
    float albumGain;
    float albumPeak;
    float trackGain;
    float trackPeak;
    float volume;
};

static int parse_time_crap(NSString * value)
{
    NSArray *crapFix = [value componentsSeparatedByString:@"\n"];
    NSArray *components = [[crapFix objectAtIndex:0] componentsSeparatedByString:@":"];

    float totalSeconds = 0;
    float multiplier = 1000;
    bool first = YES;
    for (id component in [components reverseObjectEnumerator]) {
        if (first) {
            first = NO;
            totalSeconds += [component floatValue] * multiplier;
        } else {
            totalSeconds += [component integerValue] * multiplier;
        }
        multiplier *= 60;
    }
                
    return totalSeconds;
}

static int psf_info_meta(void * context, const char * name, const char * value)
{
    struct psf_info_meta_state * state = ( struct psf_info_meta_state * ) context;
    
    NSString * tag = [NSString stringWithUTF8String:name];
    NSString * taglc = [tag lowercaseString];
    NSString * svalue = [NSString stringWithUTF8String:value];
    
    if ( svalue == nil )
        return 0;
    
    if ([taglc isEqualToString:@"game"])
    {
        taglc = @"album";
    }
    else if ([taglc isEqualToString:@"date"])
    {
        taglc = @"year";
    }
    
    if ([taglc hasPrefix:@"replaygain_"])
    {
        if ([taglc hasPrefix:@"replaygain_album_"])
        {
            if ([taglc hasSuffix:@"gain"])
                state->albumGain = [svalue floatValue];
            else if ([taglc hasSuffix:@"peak"])
                state->albumPeak = [svalue floatValue];
        }
        else if ([taglc hasPrefix:@"replaygain_track_"])
        {
            if ([taglc hasSuffix:@"gain"])
                state->trackGain = [svalue floatValue];
            else if ([taglc hasSuffix:@"peak"])
                state->trackPeak = [svalue floatValue];
        }
    }
    else if ([taglc isEqualToString:@"volume"])
    {
        state->volume = [svalue floatValue];
    }
    else if ([taglc isEqualToString:@"length"])
    {
        state->tag_length_ms = parse_time_crap(svalue);
    }
    else if ([taglc isEqualToString:@"fade"])
    {
        state->tag_fade_ms = parse_time_crap(svalue);
    }
    else if ([taglc isEqualToString:@"utf8"])
    {
        state->utf8 = true;
    }
    else if ([taglc isEqualToString:@"title"] ||
             [taglc isEqualToString:@"artist"] ||
             [taglc isEqualToString:@"album"] ||
             [taglc isEqualToString:@"year"] ||
             [taglc isEqualToString:@"genre"] ||
             [taglc isEqualToString:@"track"])
    {
        [state->info setObject:svalue forKey:taglc];
    }
    
    return 0;
}

struct psf1_load_state
{
    void * emu;
    bool first;
    unsigned refresh;
};

typedef struct {
    uint32_t pc0;
    uint32_t gp0;
    uint32_t t_addr;
    uint32_t t_size;
    uint32_t d_addr;
    uint32_t d_size;
    uint32_t b_addr;
    uint32_t b_size;
    uint32_t s_ptr;
    uint32_t s_size;
    uint32_t sp,fp,gp,ret,base;
} exec_header_t;

typedef struct {
    char key[8];
    uint32_t text;
    uint32_t data;
    exec_header_t exec;
    char title[60];
} psxexe_hdr_t;

static int psf1_info(void * context, const char * name, const char * value)
{
    struct psf1_load_state * state = ( struct psf1_load_state * ) context;
    
    NSString * sname = [[NSString stringWithUTF8String:name] lowercaseString];
    NSString * svalue = [NSString stringWithUTF8String:value];
    
    if ( !state->refresh && [sname isEqualToString:@"_refresh"] )
    {
        state->refresh = [svalue intValue];
    }
    
    return 0;
}


int psf1_loader(void * context, const uint8_t * exe, size_t exe_size,
                const uint8_t * reserved, size_t reserved_size)
{
    struct psf1_load_state * state = ( struct psf1_load_state * ) context;
    
    psxexe_hdr_t *psx = (psxexe_hdr_t *) exe;
    
    if ( exe_size < 0x800 ) return -1;
    if ( exe_size > UINT_MAX ) return -1;
    
    uint32_t addr = get_le32( &psx->exec.t_addr );
    uint32_t size = (uint32_t)exe_size - 0x800;
    
    addr &= 0x1fffff;
    if ( ( addr < 0x10000 ) || ( size > 0x1f0000 ) || ( addr + size > 0x200000 ) ) return -1;
    
    void * pIOP = psx_get_iop_state( state->emu );
    iop_upload_to_ram( pIOP, addr, exe + 0x800, size );
    
    if ( !state->refresh )
    {
        if (!strncasecmp((const char *) exe + 113, "Japan", 5)) state->refresh = 60;
        else if (!strncasecmp((const char *) exe + 113, "Europe", 6)) state->refresh = 50;
        else if (!strncasecmp((const char *) exe + 113, "North America", 13)) state->refresh = 60;
    }
    
    if ( state->first )
    {
        void * pR3000 = iop_get_r3000_state( pIOP );
        r3000_setreg(pR3000, R3000_REG_PC, get_le32( &psx->exec.pc0 ) );
        r3000_setreg(pR3000, R3000_REG_GEN+29, get_le32( &psx->exec.s_ptr ) );
        state->first = false;
    }
    
    return 0;
}


static int EMU_CALL virtual_readfile(void *context, const char *path, int offset, char *buffer, int length)
{
    return psf2fs_virtual_readfile(context, path, offset, buffer, length);
}

struct sdsf_loader_state
{
    uint8_t * data;
    size_t data_size;
};

int sdsf_loader(void * context, const uint8_t * exe, size_t exe_size,
                const uint8_t * reserved, size_t reserved_size)
{
    if ( exe_size < 4 ) return -1;
    
    struct sdsf_loader_state * state = ( struct sdsf_loader_state * ) context;
    
    uint8_t * dst = state->data;
    
    if ( state->data_size < 4 ) {
        state->data = dst = ( uint8_t * ) malloc( exe_size );
        state->data_size = exe_size;
        memcpy( dst, exe, exe_size );
        return 0;
    }
    
    uint32_t dst_start = get_le32( dst );
    uint32_t src_start = get_le32( exe );
    dst_start &= 0x7fffff;
    src_start &= 0x7fffff;
    size_t dst_len = state->data_size - 4;
    size_t src_len = exe_size - 4;
    if ( dst_len > 0x800000 ) dst_len = 0x800000;
    if ( src_len > 0x800000 ) src_len = 0x800000;
    
    if ( src_start < dst_start )
    {
        uint32_t diff = dst_start - src_start;
        state->data_size = dst_len + 4 + diff;
        state->data = dst = ( uint8_t * ) realloc( dst, state->data_size );
        memmove( dst + 4 + diff, dst + 4, dst_len );
        memset( dst + 4, 0, diff );
        dst_len += diff;
        dst_start = src_start;
        set_le32( dst, dst_start );
    }
    if ( ( src_start + src_len ) > ( dst_start + dst_len ) )
    {
        size_t diff = ( src_start + src_len ) - ( dst_start + dst_len );
        state->data_size = dst_len + 4 + diff;
        state->data = dst = ( uint8_t * ) realloc( dst, state->data_size );
        memset( dst + 4 + dst_len, 0, diff );
    }
    
    memcpy( dst + 4 + ( src_start - dst_start ), exe + 4, src_len );
    
    return 0;
}

struct qsf_loader_state
{
    uint8_t * key;
    uint32_t key_size;
    
    uint8_t * z80_rom;
    uint32_t z80_size;
    
    uint8_t * sample_rom;
    uint32_t sample_size;
};

static int upload_qsf_section( struct qsf_loader_state * state, const char * section, uint32_t start,
                          const uint8_t * data, uint32_t size )
{
    uint8_t ** array = NULL;
    uint32_t * array_size = NULL;
    uint32_t max_size = 0x7fffffff;
    
    if ( !strcmp( section, "KEY" ) ) { array = &state->key; array_size = &state->key_size; max_size = 11; }
    else if ( !strcmp( section, "Z80" ) ) { array = &state->z80_rom; array_size = &state->z80_size; }
    else if ( !strcmp( section, "SMP" ) ) { array = &state->sample_rom; array_size = &state->sample_size; }
    else return -1;
    
    if ( ( start + size ) < start ) return -1;
    
    uint32_t new_size = start + size;
    uint32_t old_size = *array_size;
    if ( new_size > max_size ) return -1;
    
    if ( new_size > old_size ) {
        *array = ( uint8_t * ) realloc( *array, new_size );
        *array_size = new_size;
        memset( (*array) + old_size, 0, new_size - old_size );
    }
    
    memcpy( (*array) + start, data, size );
    
    return 0;
}

static int qsf_loader(void * context, const uint8_t * exe, size_t exe_size,
                      const uint8_t * reserved, size_t reserved_size)
{
    struct qsf_loader_state * state = ( struct qsf_loader_state * ) context;
    
    for (;;) {
        char s[4];
        if ( exe_size < 11 ) break;
        memcpy( s, exe, 3 ); exe += 3; exe_size -= 3;
        s [3] = 0;
        uint32_t dataofs  = get_le32( exe ); exe += 4; exe_size -= 4;
        uint32_t datasize = get_le32( exe ); exe += 4; exe_size -= 4;
        if ( datasize > exe_size )
            return -1;
        
        if ( upload_qsf_section( state, s, dataofs, exe, datasize ) < 0 )
            return -1;
        
        exe += datasize;
        exe_size -= datasize;
    }
    
    return 0;
}

//SNSF
struct snsf_loader_state
{
    int base_set;
    uint32_t base;
    uint8_t * data;
    size_t data_size;
    uint8_t * sram;
    size_t sram_size;
    snsf_loader_state() : base_set(0), data(0), data_size(0), sram(0), sram_size(0) { }
    ~snsf_loader_state() { if (data) free(data); if (sram) free(sram); }
};
snsf_loader_state *snsf_rom;



int snsf_loader(void * context, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size)
{
    if ( exe_size < 8 ) return -1;

    struct snsf_loader_state * state = (struct snsf_loader_state *) context;

    unsigned char *iptr;
    unsigned isize;
    unsigned char *xptr;
    unsigned xofs = get_le32(exe + 0);
    unsigned xsize = get_le32(exe + 4);
    if ( xsize > exe_size - 8 ) return -1;
    if (!state->base_set)
    {
        state->base = xofs;
        state->base_set = 1;
    }
    else
    {
        xofs += state->base;
    }
    {
        iptr = state->data;
        isize = state->data_size;
        state->data = 0;
        state->data_size = 0;
    }
    if (!iptr)
    {
        unsigned rsize = xofs + xsize;
        {
            rsize -= 1;
            rsize |= rsize >> 1;
            rsize |= rsize >> 2;
            rsize |= rsize >> 4;
            rsize |= rsize >> 8;
            rsize |= rsize >> 16;
            rsize += 1;
        }
        iptr = (unsigned char *) malloc(rsize + 10);
        if (!iptr)
            return -1;
        memset(iptr, 0, rsize + 10);
        isize = rsize;
    }
    else if (isize < xofs + xsize)
    {
        unsigned rsize = xofs + xsize;
        {
            rsize -= 1;
            rsize |= rsize >> 1;
            rsize |= rsize >> 2;
            rsize |= rsize >> 4;
            rsize |= rsize >> 8;
            rsize |= rsize >> 16;
            rsize += 1;
        }
        xptr = (unsigned char *) realloc(iptr, xofs + rsize + 10);
        if (!xptr)
        {
            free(iptr);
            return -1;
        }
        iptr = xptr;
        isize = rsize;
    }
    memcpy(iptr + xofs, exe + 8, xsize);
    {
        state->data = iptr;
        state->data_size = isize;
    }

    // reserved section
    if (reserved_size >= 8)
    {
        unsigned rsvtype = get_le32(reserved + 0);
        unsigned rsvsize = get_le32(reserved + 4);

        if (rsvtype == 0)
        {
            // SRAM block
            if (reserved_size < 12 || rsvsize < 4)
            {
                printf("Reserve section (SRAM) is too short\n");
                return -1;
            }

            // check offset and size
            unsigned sram_offset = get_le32(reserved + 8);
            unsigned sram_patch_size = rsvsize - 4;
            if (sram_offset + sram_patch_size > 0x20000)
            {
                printf("SRAM size error\n");
                return -1;
            }

            if (!state->sram)
            {
                state->sram = (unsigned char *) malloc(0x20000);
                if (!state->sram)
                    return -1;
                memset(state->sram, 0, 0x20000);
            }

            // load SRAM data
            memcpy(state->sram + sram_offset, reserved + 12, sram_patch_size);

            // update SRAM size
            if (state->sram_size < sram_offset + sram_patch_size)
            {
                state->sram_size = sram_offset + sram_patch_size;
            }
        }
        else
        {
            printf("Unsupported reserve section type\n");
            return -1;
        }
    }

    return 0;
}




//LAZY USF
#ifdef ARCH_MIN_ARM_NEON
#include <arm_neon.h>
#endif

#include "usf/misc.h"

int32_t hc_sample_rate;
int64_t hc_currentSample,hc_fadeStart,hc_fadeLength;

int16_t *hc_sample_data;
float *hc_sample_data_float;
float *hc_sample_converted_data_float;

usf_loader_state * lzu_state;

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

int64_t src_callback_hc(void *cb_data, float **data);
int64_t src_callback_vgmstream(void *cb_data, float **data);

////////////////////

extern "C" {
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
static volatile int64_t mNeedSeek,mNeedSeekTime;

static int tim_open_output(void) {
    return 0;
} /* 0=success, 1=warning, -1=fatal error */

static void tim_close_output(void) {
}

static int tim_output_data(char *buf, int32 nbytes) {
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
        memcpy( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)buf,nbytes);
        
        buffer_ana_subofs+=nbytes;
        
        
        
    } else {
        memcpy((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)buf,to_fill);
        
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
            memcpy((char*)(buffer_ana[buffer_ana_gen_ofs]),((char*)buf)+to_fill,nbytes);
            
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
            *((int *)arg) = 4096;
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
@synthesize detailViewControllerIphone;
@synthesize mod_subsongs,mod_currentsub,mod_minsub,mod_maxsub,mLoopMode;
@synthesize mod_currentfile,mod_currentext;
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
//VGMPLAY stuff
@synthesize optVGMPLAY_maxloop,optVGMPLAY_ym2612emulator,optVGMPLAY_preferJapTag;
//Modplug stuff
@synthesize mp_settings;
@synthesize ompt_mod;
@synthesize mp_data;
@synthesize mVolume;
@synthesize numChannels,numPatterns,numSamples,numInstr,mPatternDataAvail,numVoicesChannels;
@synthesize m_voicesDataAvail;
@synthesize genRow,genPattern,/*genOffset,*/playRow,playPattern;//,playOffset;
@synthesize genVolData,playVolData;
//GSF
@synthesize optGSFsoundLowPass;
@synthesize optGSFsoundEcho;
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
        
        for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
            m_voice_current_ptr[i]=0;
            m_voice_buff[i]=(signed char*)calloc(1,SOUND_BUFFER_SIZE_SAMPLE);
        }
        for (int j=0;j<SOUND_BUFFER_NB;j++) {
            m_voice_buff_ana[j]=(signed char*)calloc(1,SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
            m_voice_buff_ana_cpy[j]=(signed char*)calloc(1,SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
        }
        memset(m_voice_ChipID,0,sizeof(int)*SOUND_MAXVOICES_BUFFER_FX);
        
        for (int i=0;i<SOUND_VOICES_MAX_ACTIVE_CHIPS;i++) {
            CGFloat hue=(250.0f/360.0f)+i*(30.0f/360.0f);
            while (hue>1.0) hue-=1.0f;
            while (hue<0.0) hue+=1.0f;
            UIColor *col=[UIColor colorWithHue:hue saturation:0.6f brightness:1.0f alpha:1.0f];
            CGFloat red,green,blue;
            //voicesChipColHalf[i]=[UIColor colorWithHue:0.8f-i*0.4f/(float)SOUND_VOICES_MAX_ACTIVE_CHIPS saturation:0.7f brightness:0.4f alpha:1.0f];
            [col getRed:&red green:&green blue:&blue alpha:NULL];
            m_voice_systemColor[i]=(((int)(red*255))<<16)|(((int)(green*255))<<8)|(((int)(blue*255))<<0);
            //printf("col %d: %02X %02X %02\n",i,(int)(red*255),(int)(green*255),(int)(blue*255));
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
        
        for (int i=0;i<SOUND_MAXMOD_CHANNELS;i++) m_voicesStatus[i]=1;
        
        stil_info=(char*)malloc(MAX_STIL_DATA_LENGTH);
        mod_message=(char*)malloc(MAX_STIL_DATA_LENGTH*2);

        
        mPanning=0;
        mPanningValue=64; //75%
        
        mdz_ArchiveFilesList=NULL;
//        mdz_ArchiveFilesListAlias=NULL;
        mdz_ArchiveFilesCnt=0;
        mdz_IsArchive=0;
        mdz_currentArchiveIndex=0;
        //Timidity
        
        
        //OMPT specific
        genPattern=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        genRow=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        //genOffset=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        playPattern=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        playRow=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        
        genVolData=(unsigned char*)malloc(SOUND_BUFFER_NB*SOUND_MAXMOD_CHANNELS);
        playVolData=(unsigned char*)malloc(SOUND_BUFFER_NB*SOUND_MAXMOD_CHANNELS);
        //playOffset=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        //
        
        //XMP
        xmp_ctx=NULL;
        xmp_mi=NULL;
        optXMP_InterpolationValue=XMP_INTERP_LINEAR;
        optXMP_AmpValue=1; //0 to 3
        optXMP_DSP=XMP_DSP_LOWPASS;
        optXMP_Flags=0;//XMP_FLAGS_A500;
        optXMP_MasterVol=100; //between 0 and 200
        optXMP_StereoSeparationVal=100; //default panning
        //
        
        
        //GME specific
        optGMEIgnoreSilence=0;
        optGMEFadeOut=1000;
        gme_eq.treble = -14; // -50.0 = muffled, 0 = flat, +5.0 = extra-crisp
        gme_eq.bass   = 80;  // 1 = full bass, 90 = average, 16000 = almost no bass
        gme_fx.enabled=0;
        gme_fx.echo = 0.0f;
        gme_fx.surround = 0.0f;
        gme_fx.stereo = 0.8f;
        
        
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
        
        //SID
                
        
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
        mSIDForceLoop=0;
        mSIDSecondSIDAddress=0;
        mSIDThirdSIDAddress=0;
        
        mSidEmuEngine=NULL;
        mBuilder=NULL;
        mSidTune=NULL;
                
        sid_forceModel=0;
        sid_forceClock=0;
        sid_engine=0;
        sid_interpolation=0;
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
        
        //HC
        bios_set_image(hebios, HEBIOS_SIZE);
        psx_init();
        sega_init();
        qsound_init();
        HC_emulatorCore=NULL;
        HC_emulatorExtra=NULL;
        
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
            memcpy(m_voice_buff_ana_cpy[buffer_ana_play_ofs],m_voice_buff_ana[buffer_ana_play_ofs],SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
            
            if (bGlobalEndReached && buffer_ana_flag[buffer_ana_play_ofs]&4) { //end reached
                //iCurrentTime=0;
                bGlobalAudioPause=2;
                bGlobalEndReached=0;
                for (int ii=0;ii<SOUND_BUFFER_NB;ii++) buffer_ana_flag[ii]&=0xFFFFFFFF^0x4;
            }
            
            if (mChangeOfSong && (buffer_ana_flag[buffer_ana_play_ofs]&8)) { //end reached but continue to play
                iCurrentTime=0;
                iModuleLength=mNewModuleLength;
                mChangeOfSong=0;
                mod_message_updated=1;
                for (int ii=0;ii<SOUND_BUFFER_NB;ii++) buffer_ana_flag[ii]&=0xFFFFFFFF^0x8;
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
    
    if (len<to_fill) {
        
        memcpy( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)data,len);
        
        buffer_ana_subofs+=len;
    } else {
        
        memcpy((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)data,to_fill);
        
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
            memcpy((char*)(buffer_ana[buffer_ana_gen_ofs]),((char*)data)+to_fill,len);
            
            buffer_ana_subofs=len;
        }
    }
}
void gsf_update(unsigned char* pSound,int lBytes) {
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
        memcpy( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)pSound,lBytes);
        
        buffer_ana_subofs+=lBytes;
        
    } else {
        memcpy((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)pSound,to_fill);
        
        lBytes-=to_fill;
        buffer_ana_subofs=0;
        
        buffer_ana_flag[buffer_ana_gen_ofs]=1;
        
        //copy voice data for oscillo view
        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
            for (int j=0;j<6;j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)];
            }
        }
        
        
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
            
            memcpy((char*)(buffer_ana[buffer_ana_gen_ofs]),((char*)pSound)+to_fill,lBytes);
            
            buffer_ana_subofs=lBytes;
        }
    }
    
    if (mNeedSeek==1) {
        seek_needed=mNeedSeekTime;
        bGlobalSeekProgress=-1;
        mNeedSeek=2;
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
                        
                        //copy voice data for oscillo view
                        if (numVoicesChannels) {
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)];
                                }
                            }
                            //printf("voice_ptr: %d\n",m_voice_current_ptr[0]>>8);
                        }
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

int64_t src_callback_hc(void *cb_data, float **data) {
    uint32_t howmany = SOUND_BUFFER_SIZE_SAMPLE;
    switch (HC_type) {
        case 1:
        case 2:
            if (psx_execute( HC_emulatorCore, 0x7fffffff, ( int16_t * ) hc_sample_data, &howmany, 0 )<0) return 0;
            break;
        case 0x11:
        case 0x12:
            if ( sega_execute( HC_emulatorCore, 0x7fffffff, ( int16_t * ) hc_sample_data, &howmany ) < 0 ) return 0;
            break;
        case 0x21:
            if (usf_render(lzu_state->emu_state, hc_sample_data, SOUND_BUFFER_SIZE_SAMPLE, &hc_sample_rate)) return 0;
            break;
        case 0x23:
            dwChannelMute=HC_voicesMuteMask1;
            howmany=snsf_gen(hc_sample_data,howmany)>>2;
            break;
        case 0x41:
            if ( qsound_execute( HC_emulatorCore, 0x7fffffff, ( int16_t * ) hc_sample_data, &howmany ) < 0 ) return 0;
            break;
    }
    hc_currentSample += howmany;

    if (iModuleLength>0) {
        if (hc_fadeLength&&(hc_fadeStart>0)&&(hc_currentSample>hc_fadeStart)) {
            int startSmpl=hc_currentSample-hc_fadeStart;
            if (startSmpl>SOUND_BUFFER_SIZE_SAMPLE) startSmpl=SOUND_BUFFER_SIZE_SAMPLE;
            int64_t vol=hc_fadeLength-(hc_currentSample-hc_fadeStart);
            if (vol<0) vol=0;
            for (int i=SOUND_BUFFER_SIZE_SAMPLE-startSmpl;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                hc_sample_data[i*2]=(int64_t)(hc_sample_data[i*2])*vol/hc_fadeLength;
                hc_sample_data[i*2+1]=(int64_t)(hc_sample_data[i*2+1])*vol/hc_fadeLength;
                if (vol) vol--;
            }
        }
    }
    src_short_to_float_array (hc_sample_data, hc_sample_data_float,SOUND_BUFFER_SIZE_SAMPLE*2);
    *data=hc_sample_data_float;
    return SOUND_BUFFER_SIZE_SAMPLE;
}

int64_t src_callback_vgmstream(void *cb_data, float **data) {
    // render audio into sound buffer
    int16_t *sampleData;
    int64_t sampleGenerated;
    int nbSamplesToRender=mVGMSTREAM_total_samples - mVGMSTREAM_decode_pos_samples;
    if (mVGMSTREAM_total_samples<0) {
        nbSamplesToRender=SOUND_BUFFER_SIZE_SAMPLE;
    }
    
    if (nbSamplesToRender >SOUND_BUFFER_SIZE_SAMPLE) {
        nbSamplesToRender = SOUND_BUFFER_SIZE_SAMPLE;
    }
    
    if (nbSamplesToRender<=0) return 0;
    
    sampleGenerated=nbSamplesToRender;
    
    int64_t real_available_samples=mVGMSTREAM_totalinternal_samples-mVGMSTREAM_decode_pos_samples;
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
                                  
                        //get the current viewcontroller from mainthread
                        __block UIViewController *vc;
                        NSOperationQueue *myQueue = [NSOperationQueue mainQueue];
                        [myQueue addOperationWithBlock:^{
                           // Background work
                            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                                // Main thread work (UI usually)
                                vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
                            }];
                        }];
                        
                        [myQueue waitUntilAllOperationsAreFinished];
                        
                        mdz_safe_execute_sel(vc,@selector(showWaiting),nil)
                        mdz_safe_execute_sel(vc,@selector(showWaitingCancel),nil)
                        mdz_safe_execute_sel(vc,@selector(updateWaitingTitle:),NSLocalizedString(@"Seeking",@""))
                        mdz_safe_execute_sel(vc,@selector(updateWaitingDetail:),@"")
                        mdz_safe_execute_sel(vc,@selector(flushMainLoop),nil)
                        
                        if (mPlayType==MMP_V2M) { //V2M
                            bGlobalSeekProgress=-1;
                            //v2m_player->Stop();
                            v2m_player->Play(mNeedSeekTime);
                            mCurrentSamples=mNeedSeekTime*PLAYBACK_FREQ/1000;
                        }
                        if (mPlayType==MMP_SIDPLAY) { //SID
                            int64_t mSeekSamples=mNeedSeekTime*PLAYBACK_FREQ/1000;
                            bGlobalSeekProgress=-1;
                            if (mSeekSamples<mCurrentSamples) {
                                //reset
                                mSidTune->selectSong(mod_currentsub+1);
                                mSidEmuEngine->load(mSidTune);
                                mCurrentSamples=0;
                            }
                            mSIDSeekInProgress=1;
                            mSidEmuEngine->fastForward( 100 * 32 );
                            while (mCurrentSamples+SOUND_BUFFER_SIZE_SAMPLE*32<=mSeekSamples) {
                                nbBytes=mSidEmuEngine->play(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*1)*2;
                                mCurrentSamples+=(nbBytes/4)*32;
                                
                                mdz_safe_execute_sel(vc,@selector(updateWaitingDetail:),([NSString stringWithFormat:@"%d%%",mCurrentSamples*100/mSeekSamples]))
                                NSInvocationOperation *invo = [[NSInvocationOperation alloc] initWithTarget:vc selector:@selector(isCancelPending) object:nil];
                                [invo start];
                                bool cancelSeek=false;
                                [invo.result getValue:&cancelSeek];
                                if (cancelSeek) {
                                    mdz_safe_execute_sel(vc,@selector(resetCancelStatus),nil);
                                    mSeekSamples=mCurrentSamples;
                                    iCurrentTime=mSeekSamples*1000/PLAYBACK_FREQ;
                                    mNeedSeekTime=iCurrentTime;
                                    //printf("stopped at: %d:%d\n",(((int)iCurrentTime/1000)/60),(((int)iCurrentTime/1000)%60));
                                    break;
                                }
                            }
                            mSidEmuEngine->fastForward( 100 );
                            
                            while (mCurrentSamples<mSeekSamples) {
                                nbBytes=mSidEmuEngine->play(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*1)*2;
                                mCurrentSamples+=(nbBytes/4);
                                
                                mdz_safe_execute_sel(vc,@selector(updateWaitingDetail:),([NSString stringWithFormat:@"%d%%",mCurrentSamples*100/mSeekSamples]))
                                
                                NSInvocationOperation *invo = [[NSInvocationOperation alloc] initWithTarget:vc selector:@selector(isCancelPending) object:nil];
                                [invo start];
                                bool result=false;
                                [invo.result getValue:&result];
                                if (result) {
                                    mdz_safe_execute_sel(vc,@selector(resetCancelStatus),nil);
                                    mSeekSamples=mCurrentSamples;
                                    iCurrentTime=mSeekSamples*1000/PLAYBACK_FREQ;
                                    mNeedSeekTime=iCurrentTime;
                                    break;
                                }
                            }
                            mSIDSeekInProgress=0;
                        }
                        if (mPlayType==MMP_GME) {   //GME
                            bGlobalSeekProgress=-1;
                            gme_seek(gme_emu,mNeedSeekTime);
                            //gme_set_fade( gme_emu, iModuleLength-optGMEFadeOut,optGMEFadeOut ); //Fade 2s before end
                        }
                        if (mPlayType==MMP_OPENMPT) { //MODPLUG
                            bGlobalSeekProgress=-1;
                            openmpt_module_set_position_seconds(openmpt_module_ext_get_module(ompt_mod), (double)(mNeedSeekTime)/1000.0f );
                            
                        }
                        if (mPlayType==MMP_XMP) { //XMP
                            bGlobalSeekProgress=-1;
                            xmp_seek_time(xmp_ctx,mNeedSeekTime);
                        }
                        if (mPlayType==MMP_ADPLUG) { //ADPLUG
                            bGlobalSeekProgress=-1;
                            adPlugPlayer->seek(mNeedSeekTime);
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
                        if (mPlayType==MMP_PMDMINI) { //PMDMini : not supported
                            mNeedSeek=0;
                        }
                        if (mPlayType==MMP_VGMPLAY) { //VGM
                            bGlobalSeekProgress=-1;
                            SeekVGM(false,mNeedSeekTime*441/10);
                            //mNeedSeek=0;
                        }
                        if (mPlayType==MMP_HC) { //HC
                            int seekSample=(double)mNeedSeekTime*(double)(hc_sample_rate)/1000.0f;
                            bGlobalSeekProgress=-1;
                            if (hc_currentSample >seekSample) {
                                //
                                // RESTART required
                                //
                                if (HC_type==0x21) usf_restart(lzu_state->emu_state);
                                else {
                                    [self MMP_HCClose];
                                    [self MMP_HCLoad:mod_currentfile];
                                }
                                hc_currentSample=0;
                            }
                            
                            //
                            // progress
                            //
                            while (seekSample - hc_currentSample > SOUND_BUFFER_SIZE_SAMPLE) {
                                uint32_t howmany = SOUND_BUFFER_SIZE_SAMPLE;
                                switch (HC_type) {
                                    case 1:
                                    case 2:
                                        psx_execute( HC_emulatorCore, 0x7fffffff, ( int16_t * ) hc_sample_data, &howmany, 0 );
                                        break;
                                    case 0x11:
                                    case 0x12:
                                        sega_execute( HC_emulatorCore, 0x7fffffff, 0, &howmany );
                                        break;
                                    case 0x21:
                                        usf_render(lzu_state->emu_state, hc_sample_data, howmany, &hc_sample_rate);
                                        break;
                                    case 0x23:
                                        snsf_gen(hc_sample_data, howmany);
                                        break;
                                    case 0x41:
                                        qsound_execute( HC_emulatorCore, 0x7fffffff, 0, &howmany );
                                        break;
                                }
                                hc_currentSample += howmany;
                                                                
                                mdz_safe_execute_sel(vc,@selector(updateWaitingDetail:),([NSString stringWithFormat:@"%d%%",hc_currentSample*100/seekSample]))
                                NSInvocationOperation *invo = [[NSInvocationOperation alloc] initWithTarget:vc selector:@selector(isCancelPending) object:nil];
                                [invo start];
                                bool result=false;
                                [invo.result getValue:&result];
                                if (result) {
                                    mdz_safe_execute_sel(vc,@selector(resetCancelStatus),nil);
                                    seekSample=hc_currentSample;
                                    iCurrentTime=seekSample*1000/PLAYBACK_FREQ;
                                    mNeedSeekTime=iCurrentTime;
                                    break;
                                }
                            }
                            //
                            // fine tune
                            //
                            if (seekSample - hc_currentSample > 0)
                            {
                                uint32_t howmany = seekSample - hc_currentSample;
                                switch (HC_type) {
                                    case 1:
                                    case 2:
                                        psx_execute( HC_emulatorCore, 0x7fffffff, ( int16_t * ) hc_sample_data, &howmany, 0 );
                                        break;
                                    case 0x11:
                                    case 0x12:
                                        sega_execute( HC_emulatorCore, 0x7fffffff, 0, &howmany );
                                        break;
                                    case 0x21:
                                        usf_render(lzu_state->emu_state, hc_sample_data, howmany, &hc_sample_rate);
                                        break;
                                    case 0x41:
                                        qsound_execute( HC_emulatorCore, 0x7fffffff, 0, &howmany );
                                        break;
                                }
                                hc_currentSample += howmany;
                                
                                mdz_safe_execute_sel(vc,@selector(updateWaitingDetail:),([NSString stringWithFormat:@"%d%%",hc_currentSample*100/seekSample]))
                                NSInvocationOperation *invo = [[NSInvocationOperation alloc] initWithTarget:vc selector:@selector(isCancelPending) object:nil];
                                [invo start];
                                bool result=false;
                                [invo.result getValue:&result];
                                if (result) {
                                    mdz_safe_execute_sel(vc,@selector(resetCancelStatus),nil);
                                    seekSample=hc_currentSample;
                                    iCurrentTime=seekSample*1000/PLAYBACK_FREQ;
                                    mNeedSeekTime=iCurrentTime;
                                    break;
                                }
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
                                                                                                
                                mdz_safe_execute_sel(vc,@selector(updateWaitingDetail:),([NSString stringWithFormat:@"%d%%",xSFPlayer->currentSample*100/seekSample]))
                                NSInvocationOperation *invo = [[NSInvocationOperation alloc] initWithTarget:vc selector:@selector(isCancelPending) object:nil];
                                [invo start];
                                bool result=false;
                                [invo.result getValue:&result];
                                if (result) {
                                    mdz_safe_execute_sel(vc,@selector(resetCancelStatus),nil);
                                    seekSample=xSFPlayer->currentSample;
                                    iCurrentTime=seekSample*1000/PLAYBACK_FREQ;
                                    mNeedSeekTime=iCurrentTime;
                                    break;
                                }
                            }
                            if (seekSample - xSFPlayer->currentSample > 0)
                            {
                                xSFPlayer->GenerateSamples(xsfSampleBuffer, 0, seekSample - xSFPlayer->currentSample);
                                xSFPlayer->currentSample = seekSample;
                                                                
                                mdz_safe_execute_sel(vc,@selector(updateWaitingDetail:),([NSString stringWithFormat:@"%d%%",xSFPlayer->currentSample*100/seekSample]))
                                NSInvocationOperation *invo = [[NSInvocationOperation alloc] initWithTarget:vc selector:@selector(isCancelPending) object:nil];
                                [invo start];
                                bool result=false;
                                [invo.result getValue:&result];
                                if (result) {
                                    mdz_safe_execute_sel(vc,@selector(resetCancelStatus),nil);
                                    seekSample=xSFPlayer->currentSample;
                                    iCurrentTime=seekSample*1000/PLAYBACK_FREQ;
                                    mNeedSeekTime=iCurrentTime;
                                    break;
                                }
                            }
                            
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
                                                                
                                mdz_safe_execute_sel(vc,@selector(updateWaitingDetail:),([NSString stringWithFormat:@"%d%%",mVGMSTREAM_decode_pos_samples*100/mVGMSTREAM_seek_needed_samples]))
                                NSInvocationOperation *invo = [[NSInvocationOperation alloc] initWithTarget:vc selector:@selector(isCancelPending) object:nil];
                                [invo start];
                                bool result=false;
                                [invo.result getValue:&result];
                                if (result) {
                                    mdz_safe_execute_sel(vc,@selector(resetCancelStatus),nil);
                                    mVGMSTREAM_seek_needed_samples=mVGMSTREAM_decode_pos_samples;
                                    iCurrentTime=mVGMSTREAM_seek_needed_samples*1000/PLAYBACK_FREQ;
                                    mNeedSeekTime=iCurrentTime;
                                    break;
                                }
                            }
                            
                            //mNeedSeek=0;
                        }
                        
                        mdz_safe_execute_sel(vc,@selector(hideWaitingCancel),nil)
                        mdz_safe_execute_sel(vc,@selector(hideWaiting),nil)
                    }
                    if (moveToNextSubSong) {
                        if (mod_currentsub<mod_maxsub) {
                            mod_currentsub++;
                            mod_message_updated=1;
                            if (mPlayType==MMP_XMP) {
                                xmp_set_position(xmp_ctx,mod_currentsub);
                                
                                iModuleLength=xmp_mi->seq_data[mod_currentsub].duration;
                                iCurrentTime=0;
                                
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
                            if (mPlayType==MMP_VGMSTREAM) {
                                //[self Stop];
                                if (vgmStream != NULL) close_vgmstream(vgmStream);
                                vgmStream = NULL;
                                mdz_safe_free(vgm_sample_data);
                                mdz_safe_free(vgm_sample_data_float);
                                mdz_safe_free(vgm_sample_converted_data_float);
                                if (src_state) src_delete(src_state);
                                src_state=NULL;
                                [self mmp_vgmstreamLoad:mod_currentfile extension:mod_currentext subsong:mod_currentsub];
                                
                                if (moveToNextSubSong==2) {
                                    //[self iPhoneDrv_PlayWaitStop];
                                    //[self iPhoneDrv_PlayStart];
                                } else {
                                    [self iPhoneDrv_PlayStop];
                                    [self iPhoneDrv_PlayStart];
                                }
                                //if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=2;
                            }
                            if (mPlayType==MMP_OPENMPT) {
                                openmpt_module_select_subsong(openmpt_module_ext_get_module(ompt_mod), mod_currentsub);
                                iModuleLength=openmpt_module_get_duration_seconds( openmpt_module_ext_get_module(ompt_mod) )*1000;
                                iCurrentTime=0;
                                numChannels=openmpt_module_get_num_channels(openmpt_module_ext_get_module(ompt_mod));  //should not change in a subsong
                                
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
                                api68_play(sc68,mod_currentsub,(mLoopMode?0:-1));
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
                                if (iModuleLength<1000) iModuleLength=1000;
                                ASAP_PlaySong(asap, mod_currentsub, iModuleLength);
                                
                                
                                iCurrentTime=0;
                                //numChannels=asap->moduleInfo.channels;
                                
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
                        if (mPlayType==MMP_XMP) {
                            xmp_set_position(xmp_ctx,mod_currentsub);
                            
                            iModuleLength=xmp_mi->seq_data[mod_currentsub].duration;
                            iCurrentTime=0;
                            
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
                        if (mPlayType==MMP_VGMSTREAM) {
                            //[self Stop];
                            if (vgmStream != NULL) close_vgmstream(vgmStream);
                            vgmStream = NULL;
                            mdz_safe_free(vgm_sample_data);
                            mdz_safe_free(vgm_sample_data_float);
                            mdz_safe_free(vgm_sample_converted_data_float);
                            if (src_state) src_delete(src_state);
                            src_state=NULL;
                            [self mmp_vgmstreamLoad:mod_currentfile extension:mod_currentext subsong:mod_currentsub];
                            
                            if (moveToNextSubSong==2) {
                                //[self iPhoneDrv_PlayWaitStop];
                                //[self iPhoneDrv_PlayStart];
                            } else {
                                [self iPhoneDrv_PlayStop];
                                [self iPhoneDrv_PlayStart];
                            }
                            //if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                            if (mLoopMode) iModuleLength=-1;
                            while (mod_message_updated) {
                                //wait
                                usleep(1);
                            }
                            mod_message_updated=2;
                        }
                        if (mPlayType==MMP_OPENMPT) {
                            openmpt_module_select_subsong(openmpt_module_ext_get_module(ompt_mod), mod_currentsub);
                            iModuleLength=openmpt_module_get_duration_seconds( openmpt_module_ext_get_module(ompt_mod) )*1000;
                            iCurrentTime=0;
                            //numChannels=openmpt_module_get_num_channels(openmpt_module_ext_get_module(ompt_mod));  //should not change in a subsong
                            
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
                            api68_play(sc68,mod_currentsub,(mLoopMode?0:-1));
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
                            if (iModuleLength<1000) iModuleLength=1000;
                            ASAP_PlaySong(asap, mod_currentsub, iModuleLength);
                            
                            
                            iCurrentTime=0;
                            //numChannels=8;//asap->moduleInfo.channels;
                            
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
                            if (mPlayType==MMP_XMP) {
                                xmp_set_position(xmp_ctx,mod_currentsub);
                                
                                iModuleLength=xmp_mi->seq_data[mod_currentsub].duration;
                                iCurrentTime=0;
                                
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
                            if (mPlayType==MMP_VGMSTREAM) {
                                //[self Stop];
                                if (vgmStream != NULL) close_vgmstream(vgmStream);
                                vgmStream = NULL;
                                mdz_safe_free(vgm_sample_data);
                                mdz_safe_free(vgm_sample_data_float);
                                mdz_safe_free(vgm_sample_converted_data_float);
                                if (src_state) src_delete(src_state);
                                src_state=NULL;
                                [self mmp_vgmstreamLoad:mod_currentfile extension:mod_currentext subsong:mod_currentsub];
                                
                                if (moveToNextSubSong==2) {
                                    //[self iPhoneDrv_PlayWaitStop];
                                    //[self iPhoneDrv_PlayStart];
                                } else {
                                    [self iPhoneDrv_PlayStop];
                                    [self iPhoneDrv_PlayStart];
                                }
                                //if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=2;
                            }
                            if (mPlayType==MMP_OPENMPT) {
                                openmpt_module_select_subsong(openmpt_module_ext_get_module(ompt_mod), mod_currentsub);
                                iModuleLength=openmpt_module_get_duration_seconds( openmpt_module_ext_get_module(ompt_mod) )*1000;
                                iCurrentTime=0;
                                //numChannels=openmpt_module_get_num_channels(openmpt_module_ext_get_module(ompt_mod));  //should not change in a subsong
                                
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
                                api68_play(sc68,mod_currentsub,(mLoopMode?0:-1));
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
                                if (iModuleLength<1000) iModuleLength=1000;
                                ASAP_PlaySong(asap, mod_currentsub, iModuleLength);
                                
                                
                                iCurrentTime=0;
                                //numChannels=8;//asap->moduleInfo.channels;
                                
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
                                gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE*2, buffer_ana[buffer_ana_gen_ofs] );
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
                                    gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE*2, buffer_ana[buffer_ana_gen_ofs] );
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                    //iCurrentTime=0;
                                    
                                } else nbBytes=0;
                            }
                        }
                        else {
                            gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE*2, buffer_ana[buffer_ana_gen_ofs] );
                            nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                            
                            if (m_voicesDataAvail) {
                                //copy voice data for oscillo view
                                for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                    for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)];
                                    }
                                }
                            }
                            //printf("voice_ptr: %d\n",m_voice_current_ptr[0]>>8);
                            
                            
                            
                        }
                    }
                    if (mPlayType==MMP_XMP) {  //XMP
                        if (m_voicesDataAvail) {
                            //reset to 0 buffer
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)]=0;
                                }
                            }
                        }
                        
                        if (xmp_play_buffer(xmp_ctx, buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE*2*2, 1) == 0) {
                            struct xmp_frame_info xmp_fi;
                            xmp_get_frame_info(xmp_ctx, &xmp_fi);
                            
                            nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                            
                            genPattern[buffer_ana_gen_ofs]=xmp_fi.pattern;
                            genRow[buffer_ana_gen_ofs]=xmp_fi.row;
                                                        
                            for (int i=0;i<numChannels;i++) {
                                int v=xmp_fi.channel_info[i].volume*4;
                                genVolData[buffer_ana_gen_ofs*SOUND_MAXMOD_CHANNELS+i]=(v>255?255:v);
                            }
                            
                            if (m_voicesDataAvail) {
                                //copy voice data for oscillo view
                                for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                    for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)];
                                    }
                                }
                            }
                        } else {
                            NSLog(@"XMP: end of song");
                            nbBytes=0;
                        }
                        
                    }
                    if (mPlayType==MMP_OPENMPT) {  //MODPLUG
                        int prev_ofs=buffer_ana_gen_ofs-1;
                        if (prev_ofs<0) prev_ofs=SOUND_BUFFER_NB-1;
                        
                        if (m_voicesDataAvail) {
                            //reset to 0 buffer
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)]=0;
                                }
                            }
                        }
                        
                        genPattern[buffer_ana_gen_ofs]=openmpt_module_get_current_pattern(openmpt_module_ext_get_module(ompt_mod));
                        genRow[buffer_ana_gen_ofs]=openmpt_module_get_current_row(openmpt_module_ext_get_module(ompt_mod));
                        
                        nbBytes=openmpt_module_read_interleaved_stereo(openmpt_module_ext_get_module(ompt_mod),PLAYBACK_FREQ,SOUND_BUFFER_SIZE_SAMPLE, buffer_ana[buffer_ana_gen_ofs] );
                        nbBytes*=4;
                        openmpt_module_get_current_pattern(openmpt_module_ext_get_module(ompt_mod));
                        for (int i=0;i<numChannels;i++) {
                            int v=openmpt_module_get_current_channel_vu_mono(openmpt_module_ext_get_module(ompt_mod),i)*255;
                            genVolData[buffer_ana_gen_ofs*SOUND_MAXMOD_CHANNELS+i]=(v>255?255:v);
                        }
                        
                        if (m_voicesDataAvail) {
                            //copy voice data for oscillo view
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)];
                                }
                            }
                        }
                        
                        if (mChangeOfSong==0) {
                            if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (mLoopMode==0)&&(iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if ((mSingleSubMode==0)&&(mod_currentsub<mod_maxsub)) {
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                    mod_currentsub++;
                                    
                                    openmpt_module_select_subsong(openmpt_module_ext_get_module(ompt_mod),mod_currentsub);
                                    
                                    mChangeOfSong=1;
                                    mNewModuleLength=openmpt_module_get_duration_seconds(openmpt_module_ext_get_module(ompt_mod))*1000;
                                    if (mLoopMode) mNewModuleLength=-1;
                                } else {
                                    nbBytes=0;
                                }
                            }
                        }
                        
                    }
                    if (mPlayType==MMP_PMDMINI) { //PMD
                        // render audio into sound buffer
                        pmd_renderer(buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE);
                        nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                        
                        // pmd_renderer gives no useful information on when song is done
                        // and will happily keep playing forever, so check song length against
                        // current playtime
                        if (iCurrentTime>=iModuleLength) {
                            nbBytes=0;
                            /*AudioQueueStop( mAudioQueue, TRUE );
                            AudioQueueReset( mAudioQueue );
                            mQueueIsBeingStopped = FALSE;
                            bGlobalEndReached=1;
                            bGlobalAudioPause=2;*/
                        }
                    }
                    if (mPlayType==MMP_VGMPLAY) { //VGM
                        // render audio into sound buffer
                        if (EndPlay) {
                            nbBytes=0;
                        } else {
                            nbBytes=VGMFillBuffer((WAVE_16BS*)(buffer_ana[buffer_ana_gen_ofs]), SOUND_BUFFER_SIZE_SAMPLE)*2*2;
                            
                            //copy voice data for oscillo view
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)];
                                }
                            }
                            //printf("vptr: %d\n",m_voice_current_ptr[0]>>8);
                        }
                    }
                    if (mPlayType==MMP_VGMSTREAM) { //VGMSTREAM
                        
                        nbBytes=src_callback_read (src_state,src_ratio,SOUND_BUFFER_SIZE_SAMPLE, vgm_sample_converted_data_float)*2*2;
                        src_float_to_short_array (vgm_sample_converted_data_float,buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2) ;
                        
                        if (mVGMSTREAM_total_samples>=0) {
                            if (mVGMSTREAM_decode_pos_samples>=mVGMSTREAM_total_samples) nbBytes=0;
                            if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                            
                            if ((!nbBytes) && (mod_subsongs>1) && (mod_currentsub<mod_maxsub) ) {
                                if (vgmStream != NULL) close_vgmstream(vgmStream);
                                vgmStream = NULL;
                                mdz_safe_free(vgm_sample_data);
                                mdz_safe_free(vgm_sample_data_float);
                                mdz_safe_free(vgm_sample_converted_data_float);
                                if (src_state) src_delete(src_state);
                                src_state=NULL;
                                mod_currentsub++;
                                [self mmp_vgmstreamLoad:mod_currentfile extension:mod_currentext subsong:mod_currentsub];
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=2;
                                
                                nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                            }
                        }
                    }
                    if (mPlayType==MMP_HC) { //Highly Complete
                        
                        //reset voice data for oscillo view
                        if (numVoicesChannels) {
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                    memset(m_voice_buff[j],0,SOUND_BUFFER_SIZE_SAMPLE);
                                }
                            }
                        }
                        nbBytes=src_callback_read (src_state,src_ratio,SOUND_BUFFER_SIZE_SAMPLE, hc_sample_converted_data_float)*2*2;
                        src_float_to_short_array (hc_sample_converted_data_float,buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2) ;
                        
                        //copy voice data for oscillo view
                        if (numVoicesChannels) {
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)];
                                }
                            }
                            //printf("voice_ptr: %d\n",m_voice_current_ptr[0]>>8);
                        }
                        if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                    }
                    if (mPlayType==MMP_2SF) { //2SF
                        bool done;
                        unsigned int samplesWritten;
                        
                        done=xSFPlayer->FillBuffer(xsfSampleBuffer,samplesWritten);
                        if (done) nbBytes=0;
                        else nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                        memcpy((char*)(buffer_ana[buffer_ana_gen_ofs]),reinterpret_cast<char *>(&xsfSampleBuffer[0]),SOUND_BUFFER_SIZE_SAMPLE*2*2);
                        
                        //copy voice data for oscillo view
                        if (numVoicesChannels) {
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)];
                                }
                            }
                            //printf("voice_ptr: %d\n",m_voice_current_ptr[0]>>8);
                        }
                        
                        //if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                        
                    }
                    if (mPlayType==MMP_V2M) { //V2M
                        
                        v2m_player->Render((float*) v2m_sample_data_float, SOUND_BUFFER_SIZE_SAMPLE);
                        mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                        //copy voice data for oscillo view
                        if (numVoicesChannels) {
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)];
                                }
                            }
                            //printf("voice_ptr: %d\n",m_voice_current_ptr[0]>>8);
                        }
                        
                        if (mCurrentSamples<mTgtSamples) {
                            nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                        } else {
                            //end reached
                            if (mLoopMode==1) {
                                //looping
                                v2m_player->Stop();
                                v2m_player->Play();
                                v2m_player->Render((float*) v2m_sample_data_float, SOUND_BUFFER_SIZE_SAMPLE);
                                nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                mCurrentSamples=0;
                            } else {
                                //stopping
                                nbBytes=0;
                            }
                        }
                            
#define LIMIT16(a) (a<-32768?-32768:(a>32767?32767:a))
                        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                            buffer_ana[buffer_ana_gen_ofs][i*2+0]=LIMIT16((32768*v2m_sample_data_float[i*2+0]));
                            buffer_ana[buffer_ana_gen_ofs][i*2+1]=LIMIT16((32768*v2m_sample_data_float[i*2+1]));
                        }
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
                                                
                        //copy voice data for oscillo view
                        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                            for (int j=0;j<(numVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?numVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>8))%(SOUND_BUFFER_SIZE_SAMPLE)];
                            }
                        }
                        
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
                                    mTgtSamples=mNewModuleLength*PLAYBACK_FREQ/1000;
                                    if (mLoopMode) mNewModuleLength=-1;
                                } else {
                                    nbBytes=0;
                                }
                            } else if (iModuleLength<0) {
                                if (mSIDForceLoop&&(mCurrentSamples>=mTgtSamples)) {
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
                                    api68_play(sc68,mod_currentsub,(mLoopMode?0:-1));
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
                                    if (mNewModuleLength<1000) mNewModuleLength=1000;
                                    ASAP_PlaySong(asap, mod_currentsub, mNewModuleLength);
                                    
                                    mChangeOfSong=1;
                                    if (mNewModuleLength<=0) mNewModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
                                    if (mLoopMode) mNewModuleLength=-1;
                                } else nbBytes=0;
                            }
                        }
                        
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
                        
                        bGlobalEndReached=1;
                    }
                    if (bGlobalEndReached) buffer_ana_flag[buffer_ana_gen_ofs]=buffer_ana_flag[buffer_ana_gen_ofs]|4; //end reached
                    if (mChangeOfSong) buffer_ana_flag[buffer_ana_gen_ofs]=buffer_ana_flag[buffer_ana_gen_ofs]|8; //end reached but continue
                    
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
-(void) fex_extractToPath:(const char *)archivePath path:(const char *)extractPath isRestarting:(bool)isRestarting {
    fex_type_t type;
    fex_t* fex;
    int arc_size;
    bool cancelExtract;
    FILE *f;
    NSString *extractFilename,*extractPathFile;
    NSError *err;
    int idx,idxAll;
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
            idx=0;idxAll=0;
            while ( !fex_done( fex ) ) {
                
                if ([self isAcceptedFile:[NSString stringWithUTF8String:fex_name(fex)] no_aux_file:0]) {
                    fex_stat(fex);
                    arc_size=fex_size(fex);
                    extractFilename=[NSString stringWithFormat:@"%s/%@",extractPath,[NSString stringWithUTF8String:fex_name(fex)]];
                    extractPathFile=[extractFilename stringByDeletingLastPathComponent];
                    
                    
                    if (!isRestarting) {
                        //1st create path if not existing yet
                        [mFileMngr createDirectoryAtPath:extractPathFile withIntermediateDirectories:TRUE attributes:nil error:&err];
                        [self addSkipBackupAttributeToItemAtPath:extractPathFile];
                    
                        //2nd extract file
                        f=fopen([extractFilename fileSystemRepresentation],"wb");
                        if (!f) {
                            NSLog(@"Cannot open %@ to extract %@",extractFilename,archivePath);
                        } else {
                            char *archive_data;
                            
                            //idxAll++;
                            UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
                            mdz_safe_execute_sel(vc,@selector(updateWaitingDetail:),([NSString stringWithFormat:@"%d/%d",idx,mdz_ArchiveFilesCnt]))
                            
                            [[NSRunLoop mainRunLoop] runUntilDate:[NSDate date]];
                            
                            NSInvocationOperation *invo = [[NSInvocationOperation alloc] initWithTarget:vc selector:@selector(isCancelPending) object:nil];
                            [invo start];
                            cancelExtract=false;
                            [invo.result getValue:&cancelExtract];
                            if (cancelExtract) {
                                mdz_safe_execute_sel(vc,@selector(resetCancelStatus),nil);
                                fclose(f);
                                break;
                            }
                            
                            archive_data=(char*)malloc(64*1024*1024); //buffer
                            while (arc_size) {
                                if (arc_size>64*1024*1024) {
                                    fex_read( fex, archive_data, 64*1024*1024);
                                    fwrite(archive_data,64*1024*1024,1,f);
                                    arc_size-=64*1024*1024;
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
                                
                                idx++;
                            }
                            if (fex_next( fex )) {
                                NSLog(@"Error during fex scanning");
                                break;
                            }
                        }
                    } else {
                        //restarting, skip extract
                        NSString *tmp_filename=[NSString stringWithUTF8String:fex_name(fex)];
                        
                        if ([self isAcceptedFile:[NSString stringWithUTF8String:fex_name(fex)] no_aux_file:1]) {
                            mdz_ArchiveFilesList[idx]=(char*)malloc(strlen([tmp_filename fileSystemRepresentation])+1);
                            strcpy(mdz_ArchiveFilesList[idx],[tmp_filename fileSystemRepresentation]);
                            
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

-(void) fex_extractSingleFileToPath:(const char *)archivePath path:(const char *)extractPath file_index:(int)index isRestarting:(bool)isRestarting {
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
                        
                        if (!isRestarting) {
                            //1st create path if not existing yet
                            [mFileMngr createDirectoryAtPath:extractPathFile withIntermediateDirectories:TRUE attributes:nil error:&err];
                            [self addSkipBackupAttributeToItemAtPath:extractPathFile];
                            //2nd extract file
                            f=fopen([extractFilename fileSystemRepresentation],"wb");
                            if (!f) {
                                NSLog(@"Cannot open %@ to extract %@",extractFilename,archivePath);
                            } else {
                                char *archive_data;
                                archive_data=(char*)malloc(64*1024*1024); //buffer
                                while (arc_size) {
                                    if (arc_size>64*1024*1024) {
                                        fex_read( fex, archive_data, 64*1024*1024);
                                        fwrite(archive_data,64*1024*1024,1,f);
                                        arc_size-=64*1024*1024;
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
                                break;
                            }
                        } else {
                            NSString *tmp_filename=[NSString stringWithUTF8String:fex_name(fex)];
                            
                            mdz_ArchiveFilesList[0]=(char*)malloc(strlen([tmp_filename fileSystemRepresentation])+1);
                            strcpy(mdz_ArchiveFilesList[0],[tmp_filename fileSystemRepresentation]);
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
    NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","];
    NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSArray *filetype_extHC=[SUPPORTED_FILETYPE_HC componentsSeparatedByString:@","];
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    NSMutableArray *filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extMDX count]+[filetype_extSID count]+
                                  [filetype_extSTSOUND count]+[filetype_extPMD count]+
                                  [filetype_extSC68 count]+[filetype_extARCHIVE count]+[filetype_extUADE count]+[filetype_extMODPLUG count]+[filetype_extXMP count]+
                                  [filetype_extGME count]+[filetype_extADPLUG count]+[filetype_ext2SF count]+[filetype_extV2M count]+[filetype_extVGMSTREAM count]+
                                  [filetype_extHC count]+[filetype_extHVL count]+[filetype_extGSF count]+
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
    [filetype_ext addObjectsFromArray:filetype_extGME];
    [filetype_ext addObjectsFromArray:filetype_extADPLUG];
    [filetype_ext addObjectsFromArray:filetype_ext2SF];
    [filetype_ext addObjectsFromArray:filetype_extV2M];
    [filetype_ext addObjectsFromArray:filetype_extVGMSTREAM];
    [filetype_ext addObjectsFromArray:filetype_extHC];
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
    NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extGME=(no_aux_file?[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_GME_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_ext2SF=(no_aux_file?[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_2SF_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
    NSArray *filetype_extHC=(no_aux_file?[SUPPORTED_FILETYPE_HC componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_HC_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=(no_aux_file?[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","]:[SUPPORTED_FILETYPE_GSF_EXT componentsSeparatedByString:@","]);
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
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
        for (int i=0;i<[filetype_extV2M count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extV2M objectAtIndex:i]]==NSOrderedSame) {found=MMP_V2M;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extV2M objectAtIndex:i]]==NSOrderedSame) {found=MMP_V2M;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extHC count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extHC objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_HC_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([extension caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_HC;break;
            }
            if ([file_no_ext caseInsensitiveCompare:[filetype_extHC objectAtIndex:i]]==NSOrderedSame) {
                //check if .miniXXX or .XXX
                NSArray *singlefile=[SUPPORTED_FILETYPE_HC_WITHEXTFILE componentsSeparatedByString:@","];
                for (int j=0;j<[singlefile count];j++)
                    if ([file_no_ext caseInsensitiveCompare:[singlefile objectAtIndex:j]]==NSOrderedSame) {
                        mSingleFileType=0;break;
                    }
                found=MMP_HC;break;
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
    soundQuality = 1;
    soundInterpolation = optGSFsoundInterpolation;
    
    DetectSilence=1;
    silencelength=5;
    IgnoreTrackLength=0;
    DefaultLength=optGENDefaultLength;
    TrailingSilence=1000;
    playforever=(mLoopMode?1:0);
    
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
    numChannels=6;
    numVoicesChannels=numChannels;
    for (int i=0;i<numVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[i/3];
    }
    m_voicesDataAvail=1;
    
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
    
    if (mdx_load((char*)[filePath UTF8String],&mdx,&pdx,mLoopMode) ) {
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
        
        ymMusicSetLoopMode(ymMusic,(mLoopMode==1?YMTRUE:YMFALSE));
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
                    if (strncmp(stil_info+idx,"NAME: ",strlen("NAME: "))==0) {
                        parser_status=4;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                        idx+=strlen("NAME: ")-1;
                    } else if (strncmp(stil_info+idx,"TITLE: ",strlen("TITLE: "))==0) {
                        parser_status=5;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                        idx+=strlen("TITLE: ")-1;
                    } else
                    break;
                case 4: // "NAME: "
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
                case 5: // "TITLE: "
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
    
            
    // Init ReSID
    if (sid_engine==0) mBuilder = new ReSIDBuilder("resid");
    else mBuilder = new ReSIDfpBuilder("residfp");
    unsigned int maxsids = (mSidEmuEngine->info()).maxsids();
    mBuilder->create(maxsids);
        
    // Check if builder is ok
    if (!mBuilder->getStatus()) {
        NSLog(@"issue in creating sid builder");
        return -1;
    }
    
    // setup resid
    if (mSIDFilterON) mBuilder->filter(true);
    else mBuilder->filter(false);
    
    // Set config
    SidConfig cfg = mSidEmuEngine->config();
    cfg.frequency= PLAYBACK_FREQ;
    if (sid_interpolation==0) {
        cfg.samplingMethod = SidConfig::INTERPOLATE;
        cfg.fastSampling = true;
    } else if (sid_interpolation==1) {
        cfg.samplingMethod = SidConfig::INTERPOLATE;
        cfg.fastSampling = false;
    } else {
        cfg.samplingMethod = SidConfig::RESAMPLE_INTERPOLATE;
        cfg.fastSampling = false;
    }
    cfg.playback = SidConfig::STEREO;
    cfg.sidEmulation  = mBuilder;
    cfg.secondSidAddress = mSIDSecondSIDAddress;
    cfg.thirdSidAddress = mSIDThirdSIDAddress;
    
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
    mSidEmuEngine->config(cfg);
    
    NSString *c64_path = [[NSBundle mainBundle] resourcePath];
    
    char *kernal = loadRom([[c64_path stringByAppendingString:@"/kernal.c64"] UTF8String], 8192);
    char *basic = loadRom([[c64_path stringByAppendingString:@"/basic.c64"] UTF8String], 8192);
    char *chargen = loadRom([[c64_path stringByAppendingString:@"/chargen.c64"] UTF8String], 4096);

    mSidEmuEngine->setRoms((const uint8_t*)kernal, (const uint8_t*)basic, (const uint8_t*)chargen);

    delete [] kernal;
    delete [] basic;
    delete [] chargen;
    
    
    
    // Load tune
    mSidTune=new SidTune([filePath UTF8String],0,true);
    
    if (mSidTune) {
        if (mSidTune->getStatus()==false) {
            NSLog(@"SID SidTune init error: wrong format");
            delete mSidTune;
            mSidTune=NULL;
        }
    }
    
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
            
            
            //cfg.secondSidAddress = 0xd420;
            //mSidEmuEngine->config(cfg);
            
            
            iCurrentTime=0;
            mCurrentSamples=0;
            numChannels=3*sidtune_info->sidChips();//(mSidEmuEngine->info()).channels();
                        
            if (sid_engine==1) {
                m_voicesDataAvail=1;
                numVoicesChannels=numChannels;
                for (int i=0;i<numVoicesChannels;i++) {
                    m_voice_voiceColor[i]=m_voice_systemColor[i/3];
                }
            } else m_voicesDataAvail=0;
            
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
                
                DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,playcount,rating,sid_subsong_length,numChannels,sidtune_info->songs());
                
                if (i==sidtune_info->songs()-1) {// Global file stats update
                    fileName=[filePath lastPathComponent];
                    DBHelper::getFileStatsDBmod(fileName,filePathSid,&playcount,&rating,&song_length,&channels_nb,&songs);
                    
                    //NSLog(@"%@||%@||sl:%d||ra:%d",fileName,filePathSid,mod_total_length,rating);
                    
                    DBHelper::updateFileStatsDBmod(fileName,filePathSid,playcount,rating,mod_total_length,numChannels,sidtune_info->songs());
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
    numVoicesChannels=numChannels;
    m_voicesDataAvail=1;
    HC_voicesMuteMask1=0xFF;
    for (int i=0;i<numVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[0];
    }    
    
    iCurrentTime=0;
    
    iModuleLength=UADEstate.song->playtime;
    if (iModuleLength<0) iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    //		NSLog(@"playtime : %d",UADEstate.song->playtime);
    
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
        NSLog(@"XMP: error initializing player");
        xmp_release_module(xmp_ctx);
        xmp_free_context(xmp_ctx);
        return 3;
    }
    
    xmp_set_player(xmp_ctx,XMP_PLAYER_AMP,optXMP_AmpValue);
    xmp_set_player(xmp_ctx,XMP_PLAYER_VOLUME,optXMP_MasterVol);
    xmp_set_player(xmp_ctx,XMP_PLAYER_MIX,optXMP_StereoSeparationVal);
    
    xmp_set_player(xmp_ctx,XMP_PLAYER_INTERP,optXMP_InterpolationValue);
    
    xmp_set_player(xmp_ctx,XMP_PLAYER_DSP,optXMP_DSP);
    xmp_set_player(xmp_ctx,XMP_PLAYER_CFLAGS,optXMP_Flags);

    /* Show module data */
    
    xmp_mi=(struct xmp_module_info*)calloc(1,sizeof(struct xmp_module_info));
    xmp_get_module_info(xmp_ctx, xmp_mi);
    
    sprintf(mod_name," %s",xmp_mi->mod->name);
    if (xmp_mi->mod->name[0]==0) {
        sprintf(mod_name," %s",mod_filename);
    }
    
    if (xmp_mi->comment) sprintf(mod_message,"%s\n", xmp_mi->comment);
    else {
        mod_message[0]=0;
        if (xmp_mi->mod->ins) {
            for (int i=0;i<xmp_mi->mod->ins;i++) {
                concatn(MAX_STIL_DATA_LENGTH*2,mod_message,xmp_mi->mod->xxi[i].name);
                concatn(MAX_STIL_DATA_LENGTH*2,mod_message,"\n");
            }
        }
        if (xmp_mi->mod->smp) {
            for (int i=0;i<xmp_mi->mod->smp;i++) {
                concatn(MAX_STIL_DATA_LENGTH*2,mod_message,xmp_mi->mod->xxs[i].name);
                concatn(MAX_STIL_DATA_LENGTH*2,mod_message,"\n");
            }
        }
    }
    
    /*NSLog(@"XMP num sequence: %d",xmp_mi->num_sequences);
     for (int i=0;i<xmp_mi->num_sequences;i++) {
     NSLog(@"XMP sequence %d duration: %d",i,xmp_mi->seq_data[i].duration);
     }
     */
    mod_subsongs=xmp_mi->num_sequences;
    mod_minsub=0;
    mod_maxsub=xmp_mi->num_sequences-1;
    
    iModuleLength=xmp_mi->seq_data[0].duration;
    
    iCurrentTime=0;
    numChannels=xmp_mi->mod->chn;
    numVoicesChannels=numChannels;
    m_voicesDataAvail=1;
    for (int i=0;i<numVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[0];
    }
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    mPatternDataAvail=2; //XMP style
    numPatterns=xmp_mi->mod->pat;
    
    ompt_patterns = (ModPlugNote**)calloc(1,sizeof(ModPlugNote*)*numPatterns);
    
    return 0;
}

static void libopenmpt_example_logfunc( const char * message, void * userdata ) {
    (void)userdata;
    if ( message ) {
        printf( "openmpt: %s\n", message );
    }
}

static int libopenmpt_example_errfunc( int error, void * userdata ) {
    (void)userdata;
    (void)error;
    return OPENMPT_ERROR_FUNC_RESULT_DEFAULT & ~OPENMPT_ERROR_FUNC_RESULT_LOG;
}

static void libopenmpt_example_print_error( const char * func_name, int mod_err, const char * mod_err_str ) {
    if ( !func_name ) {
        func_name = "unknown function";
    }
    if ( mod_err == OPENMPT_ERROR_OUT_OF_MEMORY ) {
        mod_err_str = openmpt_error_string( mod_err );
        if ( !mod_err_str ) {
            printf( "Error: %s\n", "OPENMPT_ERROR_OUT_OF_MEMORY" );
        } else {
            printf( "Error: %s\n", mod_err_str );
            openmpt_free_string( mod_err_str );
            mod_err_str = NULL;
        }
    } else {
        if ( !mod_err_str ) {
            mod_err_str = openmpt_error_string( mod_err );
            if ( !mod_err_str ) {
                printf( "Error: %s failed.\n", func_name );
            } else {
                printf( "Error: %s failed: %s\n", func_name, mod_err_str );
            }
            openmpt_free_string( mod_err_str );
            mod_err_str = NULL;
        }
        printf( "Error: %s failed: %s\n", func_name, mod_err_str );
    }
}


-(int) mmp_openmptLoad:(NSString*)filePath {  //MODPLUG
    char *modName;
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
    
    int mod_err = OPENMPT_ERROR_OK;
    const char * mod_err_str = NULL;
    ompt_mod_interactive=(openmpt_module_ext_interface_interactive*)malloc(sizeof(openmpt_module_ext_interface_interactive));
            
    ompt_mod=openmpt_module_ext_create_from_memory(mp_data, mp_datasize, &libopenmpt_example_logfunc, NULL, &libopenmpt_example_errfunc, NULL, &mod_err, &mod_err_str, NULL );
    if (!ompt_mod) {
        printf("openmpt error initializing ompt\n");
        mPlayType=0;
        free(mp_data);
        free(ompt_mod_interactive);
        return -1;
    }
    //openmpt_module_ext_interface_interactive *ompt_mod_interactive;
    
    if (!openmpt_module_ext_get_interface( ompt_mod, "interactive", ompt_mod_interactive, sizeof(openmpt_module_ext_interface_interactive))) {
        printf("openmpt error initializing interactive extension\n");
        mPlayType=0;
        free(mp_data);
        free(ompt_mod_interactive);
        return -1;
    }
    
    iModuleLength=(int)(openmpt_module_get_duration_seconds(openmpt_module_ext_get_module(ompt_mod))*1000);//  ModPlug_GetLength(mp_file);
    iCurrentTime=0;
    mPatternDataAvail=1; //OMPT/MODPLUG style
    
    numChannels=openmpt_module_get_num_channels(openmpt_module_ext_get_module(ompt_mod));// ModPlug_NumChannels(mp_file);
    numPatterns=openmpt_module_get_num_patterns(openmpt_module_ext_get_module(ompt_mod));// ModPlug_NumPatterns(mp_file);
    
    //LIBOPENMPT_API const char * openmpt_module_get_metadata_keys( openmpt_module * mod );
    //LIBOPENMPT_API const char * openmpt_module_get_metadata( openmpt_module * mod, const char * key );
    char *keys_list=(char*)openmpt_module_get_metadata_keys(openmpt_module_ext_get_module(ompt_mod));
    if (keys_list) {
        //printf("ompt metadata keys: %s\n",keys_list);
        free(keys_list);
    }
    
    modName=(char*)openmpt_module_get_metadata(openmpt_module_ext_get_module(ompt_mod),"title"); //ModPlug_GetName(mp_file);
    if (!modName) {
        sprintf(mod_name," %s",mod_filename);
    } else if (modName[0]==0) {
        sprintf(mod_name," %s",mod_filename);
    } else {
        sprintf(mod_name," %s", modName);
    }
    if (modName) free(modName);
    
    mod_subsongs=openmpt_module_get_num_subsongs(openmpt_module_ext_get_module(ompt_mod)); //mp_file->mod);
    mod_minsub=0;
    mod_maxsub=mod_subsongs-1;
    mod_currentsub=openmpt_module_get_selected_subsong(openmpt_module_ext_get_module(ompt_mod)); //mp_file->mod );
    
    numSamples=openmpt_module_get_num_samples(openmpt_module_ext_get_module(ompt_mod)); //ModPlug_NumSamples(mp_file);
    numInstr=openmpt_module_get_num_instruments(openmpt_module_ext_get_module(ompt_mod)); //ModPlug_NumInstruments(mp_file);
    
    modMessage=(char*)openmpt_module_get_metadata(openmpt_module_ext_get_module(ompt_mod),"message"); //ModPlug_GetMessage(mp_file);
    if (modMessage) sprintf(mod_message,"%s\n",modMessage);
    else {
        if ((numInstr==0)&&(numSamples==0)) sprintf(mod_message,"N/A\n");
        else sprintf(mod_message,"");
    }
    if (modMessage) free(modMessage);
    
    
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
        openmpt_module_set_repeat_count(openmpt_module_ext_get_module(ompt_mod),-1);
    }
    
    numVoicesChannels=numChannels;
    m_voicesDataAvail=1;
    for (int i=0;i<numVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[0];
    }
    
    ompt_patterns = (ModPlugNote**)calloc(1,sizeof(ModPlugNote*)*numPatterns);
    
    openmpt_module_set_render_param(openmpt_module_ext_get_module(ompt_mod),OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT,optOMPT_StereoSeparationVal);
    openmpt_module_set_render_param(openmpt_module_ext_get_module(ompt_mod),OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH,optOMPT_SamplingVal);
    openmpt_module_set_render_param(openmpt_module_ext_get_module(ompt_mod),OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL,optOMPT_MasterVol);

    return 0;
}

-(ModPlugNote*) xmp_getPattern:(int)pattern numrows:(unsigned int*)numrows {
    int c;
    int r;
    int numr,numc;
    ModPlugNote note;
    
    //safety checks
    if ((!xmp_ctx)||(!xmp_mi)) return NULL;
    
    if(pattern<0||pattern>=xmp_mi->mod->pat ){
        return NULL;
    }
    
    //compute rows number
    numr = xmp_mi->mod->xxp[pattern]->rows;
        
    if(numrows){
        *numrows = numr;
    }
    
    //allocate pattern if not already done
    if(!ompt_patterns[pattern]){
        ompt_patterns[pattern] = (ModPlugNote*)calloc(1,sizeof(ModPlugNote)*numr*numChannels);
        if(!ompt_patterns[pattern]) return NULL;
    }
    
    //fill pattern data
    numc=numChannels;
    for(r=0;r<numr;r++){
        for(c=0;c<numc;c++){
            //get track number / pattern & channels
            int track = xmp_mi->mod->xxp[pattern]->index[c];
            //get event / current row
            struct xmp_event *event = &xmp_mi->mod->xxt[track]->event[r];
            
            memset(&note,0,sizeof(ModPlugNote));
            
            note.Note = event->note;
            note.Instrument = event->ins;
            note.VolumeEffect = 0;
            note.Effect = event->fxt+1;
            note.Volume = event->vol;
            note.Parameter = event->fxp;
            memcpy(&ompt_patterns[pattern][r*numc+c],&note,sizeof(ModPlugNote));
        }
    }
    return ompt_patterns[pattern];
}

-(ModPlugNote*) ompt_getPattern:(int)pattern numrows:(unsigned int*)numrows {
    int c;
    int r;
    int numr;
    int numc;
    ModPlugNote note;
    
    if (mPlayType==MMP_XMP) return [self xmp_getPattern:pattern numrows:numrows];
    
    if(!ompt_mod) return NULL;
    if (!openmpt_module_ext_get_module(ompt_mod)) return NULL;
    if(numrows){
        *numrows = openmpt_module_get_pattern_num_rows(openmpt_module_ext_get_module(ompt_mod),pattern);
    }
    if(pattern<0||pattern>=openmpt_module_get_num_patterns(openmpt_module_ext_get_module(ompt_mod))){
        return NULL;
    }
    
    
    if(!ompt_patterns[pattern]){
        ompt_patterns[pattern] = (ModPlugNote*)malloc(sizeof(ModPlugNote)*openmpt_module_get_pattern_num_rows(openmpt_module_ext_get_module(ompt_mod),pattern)*openmpt_module_get_num_channels(openmpt_module_ext_get_module(ompt_mod)));
        if(!ompt_patterns[pattern]) return NULL;
        memset(ompt_patterns[pattern],0,sizeof(ModPlugNote)*openmpt_module_get_pattern_num_rows(openmpt_module_ext_get_module(ompt_mod),pattern)*openmpt_module_get_num_channels(openmpt_module_ext_get_module(ompt_mod)));
    }
    numr = openmpt_module_get_pattern_num_rows(openmpt_module_ext_get_module(ompt_mod),pattern);
    numc = openmpt_module_get_num_channels(openmpt_module_ext_get_module(ompt_mod));
    for(r=0;r<numr;r++){
        for(c=0;c<numc;c++){
            memset(&note,0,sizeof(ModPlugNote));
            note.Note = openmpt_module_get_pattern_row_channel_command(openmpt_module_ext_get_module(ompt_mod),pattern,r,c,OPENMPT_MODULE_COMMAND_NOTE);
            note.Instrument = openmpt_module_get_pattern_row_channel_command(openmpt_module_ext_get_module(ompt_mod),pattern,r,c,OPENMPT_MODULE_COMMAND_INSTRUMENT);
            note.VolumeEffect = openmpt_module_get_pattern_row_channel_command(openmpt_module_ext_get_module(ompt_mod),pattern,r,c,OPENMPT_MODULE_COMMAND_VOLUMEEFFECT);
            note.Effect = openmpt_module_get_pattern_row_channel_command(openmpt_module_ext_get_module(ompt_mod),pattern,r,c,OPENMPT_MODULE_COMMAND_EFFECT);
            note.Volume = openmpt_module_get_pattern_row_channel_command(openmpt_module_ext_get_module(ompt_mod),pattern,r,c,OPENMPT_MODULE_COMMAND_VOLUME);
            note.Parameter = openmpt_module_get_pattern_row_channel_command(openmpt_module_ext_get_module(ompt_mod),pattern,r,c,OPENMPT_MODULE_COMMAND_PARAMETER);
            memcpy(&ompt_patterns[pattern][r*numc+c],&note,sizeof(ModPlugNote));
        }
    }
    return ompt_patterns[pattern];
}

-(int) mmp_timidityLoad:(NSString*)filePath { //timidity
    mPlayType=MMP_TIMIDITY;
    max_voices = voices = tim_max_voices;  //polyphony : MOVE TO SETTINGS
    set_current_resampler(tim_resampler);
    opt_reverb_control=tim_reverb;
    //set_current_resampler (RESAMPLE_LINEAR); //resample : MOVE TO SETTINGS
    
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

-(int) mmp_vgmstreamLoad:(NSString*)filePath extension:(NSString*)extension subsong:(int)subindex{  //VGMSTREAM
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
    
    vgmFile->stream_index=subindex;
    
    vgmStream = init_vgmstream_from_STREAMFILE(vgmFile);
    close_streamfile(vgmFile);
    
    if (!vgmStream) {
        NSLog(@"Error init_vgmstream_from_STREAMFILE %@",filePath);
        mPlayType=0;
        src_delete(src_state);
        return -1;
    }
    /////////////////////////
    
    vgmstream_cfg_t vcfg = {0};
    vcfg.allow_play_forever = (mLoopMode==1?1:0);
    vcfg.play_forever =(mLoopMode==1?1:0);
    
    vcfg.loop_count = optVGMSTREAM_loop_count;
    vcfg.fade_time = optVGMSTREAM_fadeouttime;
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
        src_delete(src_state);
        return -1;
    }
    
    if (mLoopMode==1) {
        mVGMSTREAM_total_samples = -1;
        mVGMSTREAM_totalinternal_samples=-1;
    } else {
        mVGMSTREAM_total_samples = vgmstream_get_samples(vgmStream);
        mVGMSTREAM_totalinternal_samples = mVGMSTREAM_total_samples;
    }
    
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    else iModuleLength=(double)mVGMSTREAM_total_samples*1000.0f/(double)(vgmStream->sample_rate);
    iCurrentTime=0;
    mVGMSTREAM_seek_needed_samples=-1;
    mVGMSTREAM_decode_pos_samples=0;
    
    numChannels=vgmStream->channels;
    if (vgmStream->stream_name[0]) sprintf(mod_name," %s",vgmStream->stream_name);
    else sprintf(mod_name," %s",mod_filename);
    
    mod_message[0]=0;
    describe_vgmstream(vgmStream,mod_message,MAX_STIL_DATA_LENGTH*2);
    vgm_sample_data=(int16_t*)malloc(SOUND_BUFFER_SIZE_SAMPLE*2*(numChannels>2?numChannels:2));
    vgm_sample_data_float=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4*(numChannels>2?numChannels:2));
    vgm_sample_converted_data_float=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4*(numChannels>2?numChannels:2));
        
    mod_subsongs=vgmStream->num_streams;
    if (mod_subsongs>1) {
        mod_maxsub=vgmStream->num_streams;
        mod_minsub=1;
        mod_currentsub=vgmStream->stream_index;
        if (mod_currentsub<mod_minsub) mod_currentsub=mod_minsub;
    }
    
    return 0;
}
    
    
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
        if (x) concatn(MAX_STIL_DATA_LENGTH*2,mod_message,"\n");
        concatn(MAX_STIL_DATA_LENGTH*2,mod_message,(keys[x] + "=" + tags[keys[x]]).c_str());
    }
    int fadeInMS=xSFFile->GetFadeMS(xSFPlayer->fadeInMS);
    xSFPlayer->fadeInMS=fadeInMS;
    xSFPlayer->fadeSample=(int64_t)(fadeInMS)*(int64_t)(xSFPlayer->sampleRate)/1000;
    
    iCurrentTime=0;
    numChannels=16;
    numVoicesChannels=numChannels;
    m_voicesDataAvail=1;
    for (int i=0;i<numVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[0];
    }
    
    iModuleLength=1000* (int64_t)(xSFPlayer->GetLengthInSamples()) / 44100;
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    return 0;
}

static unsigned char* v2m_check_and_convert(unsigned char* tune, unsigned int* length)
{
    sdInit();

    if (tune[2] != 0 || tune[3] != 0)
    {
        printf("No valid input file\n");
        return NULL;
    }
    int version = CheckV2MVersion(tune, *length);
    if (version < 0)
    {
        printf("Failed to Check Version on input file\n");
        return NULL;
    }

    uint8_t* converted;
    int converted_length;
    ConvertV2M(tune, *length, &converted, &converted_length);
    *length = converted_length;
    free(tune);

    return converted;
}

-(int) mmp_v2mLoad:(NSString*)filePath extension:(NSString*)extension {  //SNSF
    mPlayType=MMP_V2M;
    FILE *f;
    
    f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"V2M Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    mp_data=(char*)malloc(mp_datasize);
    fseek(f,0L,SEEK_SET);
    fread(mp_data, 1, mp_datasize, f);
    fclose(f);
    
    //prepare the tune
    mp_data=(char*)v2m_check_and_convert((unsigned char*)mp_data,(unsigned int*)&mp_datasize);
    if (!mp_data) {
        NSLog(@"V2M: issue with file format %@",filePath);
        delete mp_data;
        mPlayType=0;
        return -2;
    }
    
    //Create player    
    v2m_player=new V2MPlayer;
    v2m_player->Init(1000);
    //Load tune
    v2m_player->Open(mp_data,PLAYBACK_FREQ);
    
    
    v2m_voices_mask=0xFFFFFFFF;
    v2m_player->Play(0);
    
    v2m_sample_data_float=(float*)calloc(1,sizeof(float)*2*SOUND_BUFFER_SIZE_SAMPLE);
    
    mod_subsongs=1;
    mod_minsub=0;
    mod_maxsub=0;
    mod_currentsub=0;
    
    sprintf(mod_name," %s",mod_filename);
    
    mod_message[0]=0;
    
    iCurrentTime=0;
    numChannels=16;
    numVoicesChannels=numChannels;
    m_voicesDataAvail=1;
    for (int i=0;i<numVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[0];
    }
    
    int32_t *timeline;
    int tllen;
    timeline=NULL;
    tllen=v2m_player->CalcPositions(&timeline);
    
    iModuleLength=timeline[2*(tllen-1)]+2000; // 2secs after last event //v2m_player->Length()*1000;
    mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
    mCurrentSamples=0;
    if (timeline) delete timeline;
    timeline=NULL;
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    return 0;
}

-(int) MMP_HCLoad:(NSString*)filePath {  //Highly Complete
    mPlayType=MMP_HC;
    FILE *f;
    
    HC_type=0;
    f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"HE Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);

    
    struct psf_info_meta_state info;
    info.info = [NSMutableDictionary dictionary];
    info.utf8 = false;
    info.tag_length_ms = 0;
    info.tag_fade_ms = 0;
    info.albumGain = 0;
    info.albumPeak = 0;
    info.trackGain = 0;
    info.trackPeak = 0;
    info.volume = 1;
    HC_type = psf_load( [filePath UTF8String], &psf_file_system, 0, 0, 0, psf_info_meta, &info, 0 );
    //NSLog(@"HC file type: %d",HC_type);
    if (HC_type <= 0) {
        return -1;
    }
    
    if (!info.tag_length_ms) {
        info.tag_length_ms = ( 2 * 60 + 30 ) * 1000;
        info.tag_fade_ms = 8000;
    }
            
    //Initi SRC samplerate converter
    int error;
    src_state=src_callback_new(src_callback_hc,optHC_ResampleQuality,2,&error,NULL);
    if (!src_state) {
        NSLog(@"Error while initializing SRC samplerate converter: %d",error);
        return -1;
    }
    /////////////////////////
    HC_voicesMuteMask1=0xFFFFFFFF;
    HC_voicesMuteMask2=0xFFFFFFFF;
    
    src_data.data_out=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*sizeof(float)*2);
    src_data.data_in=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*sizeof(float)*2);
    if (HC_type==1) { //PSF1
        hc_sample_rate=44100;
        HC_emulatorCore = ( uint8_t * ) calloc( 1,psx_get_state_size( 1 ) );
        psx_clear_state( HC_emulatorCore, 1 );
        struct psf1_load_state state;
        state.emu = HC_emulatorCore;
        state.first = true;
        state.refresh = 0;
        if ( psf_load( [filePath UTF8String], &psf_file_system, 1, psf1_loader, &state, psf1_info, &state, 1 ) <= 0 ) {
            NSLog(@"Error loading PSF1");
            mPlayType=0;
            src_delete(src_state);
            if (HC_emulatorCore) free(HC_emulatorCore);
            HC_emulatorCore=NULL;
            return -1;
        }
        if ( state.refresh ) psx_set_refresh( HC_emulatorCore, state.refresh );
        
        m_voicesDataAvail=1;
        numChannels=24;
        numVoicesChannels=numChannels;
        for (int i=0;i<numVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
        
        //help to behave more like real hardware, fix a few recent dumps
        void * pIOP = psx_get_iop_state( HC_emulatorCore );
        iop_set_compat( pIOP, IOP_COMPAT_HARSH );
    } else if (HC_type==2) { //PSF2
        hc_sample_rate=48000;
        HC_emulatorExtra = psf2fs_create();
        struct psf1_load_state state;
        state.refresh = 0;
        if ( psf_load( (char*)[filePath UTF8String], &psf_file_system, 2, psf2fs_load_callback, HC_emulatorExtra, psf1_info, &state, 1 ) <= 0 ) {
            NSLog(@"Error loading PSF2");
            mPlayType=0;
            src_delete(src_state);
            if (HC_emulatorExtra ) {
                psf2fs_delete( HC_emulatorExtra );
                HC_emulatorExtra = NULL;
            }
            return -1;
        }
        HC_emulatorCore = ( uint8_t * ) calloc( 1,psx_get_state_size( 2 ) );
        psx_clear_state( HC_emulatorCore, 2 );
        if ( state.refresh ) psx_set_refresh( HC_emulatorCore, state.refresh );
        psx_set_readfile( HC_emulatorCore, virtual_readfile, HC_emulatorExtra );
        
        m_voicesDataAvail=1;
        numChannels=48;
        numVoicesChannels=numChannels;
        for (int i=0;i<numVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[i/24];
        }
        
        //help to behave more like real hardware, fix a few recent dumps
        void * pIOP = psx_get_iop_state( HC_emulatorCore );
        iop_set_compat( pIOP, IOP_COMPAT_HARSH );
    } else if ( HC_type == 0x11 || HC_type == 0x12 ) { //DSL/SSF
        hc_sample_rate=44100;
        struct sdsf_loader_state state;
        memset( &state, 0, sizeof(state) );
        if ( psf_load( [filePath UTF8String], &psf_file_system, HC_type, sdsf_loader, &state, 0, 0, 0 ) <= 0 ) {
            NSLog(@"Error loading DSF/SSF");
            mPlayType=0;
            src_delete(src_state);
            return -1;
        }
        
        HC_emulatorCore = ( uint8_t * ) calloc( 1,sega_get_state_size( HC_type - 0x10 ) );
        
        sega_clear_state( HC_emulatorCore, HC_type - 0x10 );
        
        sega_enable_dry( HC_emulatorCore, 1 );
        sega_enable_dsp( HC_emulatorCore, 1 );
        
        sega_enable_dsp_dynarec( HC_emulatorCore, 0 );
        
        uint32_t start  = *(uint32_t*) state.data;
        size_t length = state.data_size;
        const size_t max_length = ( HC_type == 0x12 ) ? 0x800000 : 0x80000;
        if ( ( start + ( length - 4 ) ) > max_length ) {
            length = max_length - start + 4;
        }
        sega_upload_program( HC_emulatorCore, state.data, (uint32_t)length );
        
        free( state.data );
                        
        if (HC_type==0x11) {
            numChannels=32;
            numVoicesChannels=32;
            m_voicesDataAvail=1;
            for (int i=0;i<numVoicesChannels;i++) {
                m_voice_voiceColor[i]=m_voice_systemColor[0];
            }
        } else if (HC_type==0x12) {
            numChannels=64;
            numVoicesChannels=64;
            m_voicesDataAvail=1;
            for (int i=0;i<numVoicesChannels;i++) {
                m_voice_voiceColor[i]=m_voice_systemColor[0];
            }
        }
        
    } else if (HC_type == 0x21) { //USF
        lzu_state = new usf_loader_state;
        lzu_state->emu_state = malloc( usf_get_state_size() );
        usf_clear( lzu_state->emu_state );
        
        //usf_set_hle_audio( lzu_state->emu_state, 1 );
        
        usf_info_data=(corlett_t*)calloc(1,sizeof(corlett_t));
        
        if ( psf_load( (char*)[filePath UTF8String], &psf_file_system, 0x21, usf_loader, lzu_state, usf_info, lzu_state,1 ) < 0 ) {
            NSLog(@"Error loading USF");
            mPlayType=0;
            src_delete(src_state);
            mdz_safe_free(lzu_state->emu_state);
            mdz_safe_free(usf_info_data);
            delete lzu_state;
            lzu_state=NULL;
            return -1;
        }
        usf_set_compare( lzu_state->emu_state, lzu_state->enable_compare );
        usf_set_fifo_full( lzu_state->emu_state, lzu_state->enable_fifo_full );
        
        usf_render(lzu_state->emu_state, 0, 0, &hc_sample_rate);
        
        numChannels=2;
    } else if ( HC_type == 0x23 ) { //SNSF
        hc_sample_rate=32000;
        snsf_rom=new snsf_loader_state;
        if ( psf_load( (char*)[filePath UTF8String], &psf_file_system, 0x23, snsf_loader, snsf_rom, 0, 0, 0 ) < 0 ) {
            NSLog(@"Error loading SNSF");
            mPlayType=0;
            src_delete(src_state);
            delete snsf_rom;
            snsf_rom=NULL;
            return -1;
        }
                        
        snsf_start(snsf_rom->data, snsf_rom->data_size, snsf_rom->sram, snsf_rom->sram_size );
                
        numChannels=8;
        numVoicesChannels=8;
        m_voicesDataAvail=1;
        for (int i=0;i<numVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
    } else if ( HC_type == 0x41 ) { //QSF
        hc_sample_rate=24038;
        struct qsf_loader_state * state = ( struct qsf_loader_state * ) calloc( 1, sizeof( *state ) );
        
        HC_emulatorExtra = state;
        
        if ( psf_load( [filePath UTF8String], &psf_file_system, 0x41, qsf_loader, state, 0, 0, 0 ) <= 0 ) {
            NSLog(@"Error loading QSF");
            mPlayType=0;
            src_delete(src_state);
            free(HC_emulatorExtra);
            HC_emulatorExtra=NULL;
            return -1;
        }
        
        HC_emulatorCore = ( uint8_t * ) calloc( 1,qsound_get_state_size() );
        
        qsound_clear_state( HC_emulatorCore );
        
        if(state->key_size == 11) {
            uint8_t * ptr = state->key;
            uint32_t swap_key1 = get_be32( ptr +  0 );
            uint32_t swap_key2 = get_be32( ptr +  4 );
            uint32_t addr_key  = get_be16( ptr +  8 );
            uint8_t  xor_key   =        *( ptr + 10 );
            qsound_set_kabuki_key( HC_emulatorCore, swap_key1, swap_key2, addr_key, xor_key );
        } else {
            qsound_set_kabuki_key( HC_emulatorCore, 0, 0, 0, 0 );
        }
        qsound_set_z80_rom( HC_emulatorCore, state->z80_rom, state->z80_size );
        qsound_set_sample_rom( HC_emulatorCore, state->sample_rom, state->sample_size );
                
        m_voicesDataAvail=1;
        numChannels=19;
        numVoicesChannels=numChannels;
        for (int i=0;i<numVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
        
    }
    src_ratio=PLAYBACK_FREQ/(double)hc_sample_rate;
            
    iModuleLength=-1;
    if (info.tag_length_ms) {
        iModuleLength=info.tag_length_ms+info.tag_fade_ms;
    }
    iCurrentTime=0;
    
    
    if (HC_type==0x21) {
        sprintf(mod_name,"");
        if (usf_info_data->inf_title)
            if (usf_info_data->inf_title[0]) sprintf(mod_name," %s",usf_info_data->inf_title);
        
        if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
        
        sprintf(mod_message,"Game:\t%s\nTitle:\t%s\nArtist:\t%s\nYear:\t%s\nGenre:\t%s\nUSF By:\t%s\nCopyright:\t%s\nTrack:\t%s\nSample rate: %dHz\nLength: %ds\n",
                (usf_info_data->inf_game?usf_info_data->inf_game:""),
                (usf_info_data->inf_title?usf_info_data->inf_title:""),
                (usf_info_data->inf_artist?usf_info_data->inf_artist:""),
                (usf_info_data->inf_year?usf_info_data->inf_year:""),
                (usf_info_data->inf_genre?usf_info_data->inf_genre:""),
                (usf_info_data->inf_usfby?usf_info_data->inf_usfby:""),
                (usf_info_data->inf_copy?usf_info_data->inf_copy:""),
                (usf_info_data->inf_track?usf_info_data->inf_track:""),
                hc_sample_rate,
                iModuleLength/1000);
                        
        if (usf_info_data->inf_game && usf_info_data->inf_game[0]) mod_title=[NSString stringWithFormat:@"%s",usf_info_data->inf_game];
    } else {
        sprintf(mod_name,"");
        if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
        
        sprintf(mod_message,"\n");
        for (id key in info.info) {
            id value = info.info[key];
            NSString *tmpstr=[NSString stringWithFormat:@"%@:%@\n",key,value];
            strcat(mod_message,[tmpstr UTF8String]);
            
            if ([key isEqualToString:@"album"]&&([value length]>0)) mod_title=[NSString stringWithString:value];
        }
        strcat(mod_message,[[NSString stringWithFormat:@"Sample rate: %dHz\nLength: %ds\n",hc_sample_rate,iModuleLength/1000] UTF8String]);
    }
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    
    hc_currentSample=0;
    hc_fadeLength=(int64_t)(info.tag_fade_ms)*(int64_t)hc_sample_rate/1000;
    hc_fadeStart=(int64_t)(info.tag_length_ms)*(int64_t)hc_sample_rate/1000;
    
    hc_sample_data=(int16_t*)malloc(SOUND_BUFFER_SIZE_SAMPLE*2*2);
    hc_sample_data_float=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4*2);
    hc_sample_converted_data_float=(float*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4*2);
    
    return 0;
}

int vgmGetFileLength()
{
    UINT32 SmplCnt;
    UINT32 MSecCnt;
    INT32 LoopCnt;
    
    LoopCnt = VGMMaxLoopM;
    
    if (! LoopCnt && VGMHead.lngLoopSamples)
        return -1000;
    
    // Note: SmplCnt is ALWAYS 44.1 KHz, VGM's native sample rate
    SmplCnt = VGMHead.lngTotalSamples + VGMHead.lngLoopSamples * (LoopCnt - 0x01);
    
    MSecCnt = CalcSampleMSec(SmplCnt, 0x02);
    
    if (VGMHead.lngLoopSamples)
        MSecCnt += FadeTime + 0;//Options.PauseLp;
    else
        MSecCnt += 0;//Options.PauseNL;
    
    return MSecCnt;
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
    
    NSString *vgm_base_path = [[NSBundle mainBundle] resourcePath];
    AppPaths[0]=strdup([[NSString stringWithFormat:@"%@/",vgm_base_path] UTF8String]);
    // load configuration file here
    ChipOpts[0].YM2612.EmuCore=optVGMPLAY_ym2612emulator;
    ChipOpts[1].YM2612.EmuCore=optVGMPLAY_ym2612emulator;
    
    //ChipOpts[0].YM3812.EmuCore=optVGMPLAY_ym2612emulator;
    //ChipOpts[1].YM3812.EmuCore=optVGMPLAY_ym2612emulator;
        
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
    
    mod_message[0]=0;
    numChannels=0;
    numVoicesChannels=0;
    vgmplay_activeChipsNb=0;
    vgmVRC7=vgm2610b=0;
    
    
    UINT8 CurChip;
    UINT32 ChpClk;
    UINT8 ChpType;
    char strChip[16];
    char firstChip=1;
    for (CurChip = 0x00; CurChip < CHIP_COUNT; CurChip ++)
    {
        ChpClk = GetChipClock(&VGMHead, CurChip, &ChpType);
        if (ChpClk && GetChipClock(&VGMHead, 0x80 | CurChip, NULL)) ChpClk |= 0x40000000;
        if (ChpClk) {
            int chipNumber=vgmGetChipsDetails(strChip,CurChip, ChpType, ChpClk);
            
            if ((CurChip==1)&&(ChpClk&0x80000000)) vgmVRC7=1;
            if ((CurChip==8)&&(ChpClk&0x80000000)) vgm2610b=1;
            
            for (int i=0;i<chipNumber;i++) {
                vgmplay_activeChips[vgmplay_activeChipsNb]=CurChip;
                vgmplay_activeChipsID[vgmplay_activeChipsNb]=i;
                m_voice_ChipID[numChannels]=CurChip|(i<<8);
                                                
                vgmplay_activeChipsName[vgmplay_activeChipsNb]=strdup(strChip);
                numChannels+=vgmGetVoicesNb(CurChip);
                
                for (int j=numVoicesChannels;j<numVoicesChannels+vgmGetVoicesNb(CurChip);j++) {
                    m_voice_voiceColor[j]=m_voice_systemColor[vgmplay_activeChipsNb];
                }
                
                numVoicesChannels+=vgmGetVoicesChannelsUsedNb(CurChip);
                vgmplay_activeChipsNb++;
            }
                                    
            if (firstChip) {
                sprintf(mod_message+strlen(mod_message),"%dx%s",chipNumber,strChip);
                firstChip=0;
            } else sprintf(mod_message+strlen(mod_message),",%dx%s",chipNumber,strChip);
            /*    {    "SN76496", "YM2413", "YM2612", "YM2151", "SegaPCM", "RF5C68", "YM2203", "YM2608",
             "YM2610", "YM3812", "YM3526", "Y8950", "YMF262", "YMF278B", "YMF271", "YMZ280B",
             "RF5C164", "PWM", "AY8910", "GameBoy", "NES APU", "YMW258", "uPD7759", "OKIM6258",
             "OKIM6295", "K051649", "K054539", "HuC6280", "C140", "K053260", "Pokey", "QSound",
             "SCSP", "WSwan", "VSU", "SAA1099", "ES5503", "ES5506", "X1-010", "C352",
             "GA20"};*/
            
            
            strcpy(strChip,GetChipName(CurChip));
            if (strcmp(strChip,"SN76496")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM2413")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM2612")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM2151")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"SegaPCM")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM2203")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM2608")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM2610")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YMF262")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YMF278B")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM3812")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"PWM")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"AY8910")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"GameBoy")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"NES APU")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"uPD7759")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"OKIM6258")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"OKIM6295")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"K051649")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"HuC6280")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"C140")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"VSU")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"GA20")==0) {
                m_voicesDataAvail=1;
            }
        }
    }
        
    //if (!m_voicesDataAvail) numChannels=2;
                
    sprintf(mod_message+strlen(mod_message),"\nAuthor:%s\nGame:%s\nSystem:%s\nTitle:%s\nRelease Date:%s\nCreator:%s\nNotes:%s\n",
                [[self wcharToNS:GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strAuthorNameE,VGMTag.strAuthorNameJ)] UTF8String],
                [[self wcharToNS:GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strGameNameE,VGMTag.strGameNameJ)] UTF8String],
                [[self wcharToNS:GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strSystemNameE,VGMTag.strSystemNameJ)] UTF8String],
                [[self wcharToNS:GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strTrackNameE,VGMTag.strTrackNameJ)] UTF8String],
                [[self wcharToNS:VGMTag.strReleaseDate] UTF8String],
                [[self wcharToNS:VGMTag.strCreator] UTF8String],
                [[self wcharToNS:VGMTag.strNotes] UTF8String]);
    
    mod_title=[self wcharToNS:GetTagStrEJ(optVGMPLAY_preferJapTag,VGMTag.strGameNameE,VGMTag.strGameNameJ)];
    if (mod_title && ([mod_title length]==0)) mod_title=nil;
    
    //NSLog(@"loop: %d\n",VGMMaxLoopM);
    iModuleLength=vgmGetFileLength();//(VGMHead.lngTotalSamples+VGMMaxLoopM*VGMHead.lngLoopSamples)*10/441;//ms
    //NSLog(@"VGM length %d",iModuleLength);
    iCurrentTime=0;
    
    
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
    pmd_setrate(PLAYBACK_FREQ); // 22kHz or 44.1kHz?
    
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
        
        numChannels=2;//pmd_get_tracks();
        
        //Loop
        if (mLoopMode==1) iModuleLength=-1;
        
        return 0;
    }
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
    ASAP_MutePokeyChannels(asap,0); //all channels active by default
    
//    ASAPInfo_GetExtDescription
    
    sprintf(mod_message,"Author:%s\nTitle:%s\nSongs:%d\nChannels:%d\n",asap->moduleInfo.author,asap->moduleInfo.title,asap->moduleInfo.songs,asap->moduleInfo.channels);
    
    iModuleLength=(duration>=1000?duration:1000);
    iCurrentTime=0;
        
    numChannels=(asap->pokeys.extraPokeyMask?8:4);//asap->moduleInfo.channels;
    
    mod_minsub=0;
    mod_maxsub=asap->moduleInfo.songs-1;
    mod_subsongs=asap->moduleInfo.songs;
    
    if (asap->moduleInfo.title[0]) {
        sprintf(mod_name," %s",asap->moduleInfo.title);
        mod_title=[NSString stringWithFormat:@"%s",asap->moduleInfo.title];
    }
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
    int64_t sample_rate = PLAYBACK_FREQ; /* number of samples per second */
    int track = 0; /* index of track to play (0 = first) */
    gme_err_t err;
    mPlayType=MMP_GME;
    
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
        
        gme_equalizer( gme_emu, &gme_eq );
        gme_eq.treble=optGMEEQTreble;
        gme_eq.bass=optGMEEQBass;
        gme_set_equalizer( gme_emu, &gme_eq );
        
        gme_effects(gme_emu,&gme_fx);
        gme_fx.enabled=optGMEFXon;/* If 0, no effects are added */
        gme_fx.surround = optGMEFXSurround; /* If 1, some channels are put in "back", using phase inversion */
        gme_fx.echo = optGMEFXEcho;/* Amount of echo, where 0.0 = none, 1.0 = lots */
        gme_fx.stereo = optGMEFXStereo;/* Separation, where 0.0 = mono, 1.0 = hard left and right */
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
                        
            if (strcmp(gmetype,"Super Nintendo")==0) {//SPC
                m_voicesDataAvail=1;
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
            if (gme_info->game){
                if (gme_info->game[0]) mod_title=[NSString stringWithFormat:@"%s",gme_info->game];
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
        if (m_voicesDataAvail) numVoicesChannels=numChannels;
        
        for (int i=0;i<numVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
        
        
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

    

- (UIViewController *)visibleViewController:(UIViewController *)rootViewController
{
    if ([rootViewController isKindOfClass:[UITabBarController class]])
    {
        UIViewController *selectedViewController = ((UITabBarController *)rootViewController).selectedViewController;

        return [self visibleViewController:selectedViewController];
    }
    if ([rootViewController isKindOfClass:[UINavigationController class]])
    {
        UIViewController *lastViewController = [[((UINavigationController *)rootViewController) viewControllers] lastObject];

        return [self visibleViewController:lastViewController];
    }
    
    if (rootViewController.presentedViewController == nil)
    {
        return rootViewController;
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UINavigationController class]])
    {
        UINavigationController *navigationController = (UINavigationController *)rootViewController.presentedViewController;
        UIViewController *lastViewController = [[navigationController viewControllers] lastObject];

        return [self visibleViewController:lastViewController];
    }
    if ([rootViewController.presentedViewController isKindOfClass:[UITabBarController class]])
    {
        UITabBarController *tabBarController = (UITabBarController *)rootViewController.presentedViewController;
        UIViewController *selectedViewController = tabBarController.selectedViewController;

        return [self visibleViewController:selectedViewController];
    }

    UIViewController *presentedViewController = (UIViewController *)rootViewController.presentedViewController;

    return [self visibleViewController:presentedViewController];
}

-(int) LoadModule:(NSString*)_filePath defaultMODPLAYER:(int)defaultMODPLAYER defaultSAPPLAYER:(int)defaultSAPPLAYER defaultVGMPLAYER:(int)defaultVGMPLAYER archiveMode:(int)archiveMode archiveIndex:(int)archiveIndex singleSubMode:(int)singleSubMode singleArcMode:(int)singleArcMode detailVC:(DetailViewControllerIphone*)detailVC isRestarting:(bool)isRestarting{
    NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
    NSArray *filetype_extLHA_ARCHIVE=[SUPPORTED_FILETYPE_LHA_ARCHIVE componentsSeparatedByString:@","];
    NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
    NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
    NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
    NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
    NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
    NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","];
    NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSArray *filetype_extHC=[SUPPORTED_FILETYPE_HC componentsSeparatedByString:@","];
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
    
    detailViewControllerIphone=detailVC;
    
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
                //NSLog(@"scan done");
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
                    if (!isRestarting) {
                        [mFileMngr removeItemAtPath:tmpArchivePath error:&err];
                        //create tmp dir
                        [mFileMngr createDirectoryAtPath:tmpArchivePath withIntermediateDirectories:TRUE attributes:nil error:&err];
                        [self addSkipBackupAttributeToItemAtPath:tmpArchivePath];
                        
                    }
                    
                    
                    if (found==1) { //FEX
                        
                        //update singlefiletype flag
                        //[self isAcceptedFile:[NSString stringWithFormat:@"Documents/tmpArchive/%s",mdz_ArchiveFilesList[mdz_currentArchiveIndex] no_aux_file:1];
                        if (singleArcMode&&(archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)){
                            NSString *tmpstr=[self fex_getfilename:[filePath UTF8String] index:archiveIndex];
                            //NSLog(@"yo:%@",tmpstr);
                        }
                                                
                        UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
                        
                        mdz_safe_execute_sel(vc,@selector(showWaiting),nil)
                        mdz_safe_execute_sel(vc,@selector(showWaitingCancel),nil)
                        mdz_safe_execute_sel(vc,@selector(updateWaitingTitle:),NSLocalizedString(@"Extracting",@""))
                        mdz_safe_execute_sel(vc,@selector(updateWaitingDetail:),@"")
                        mdz_safe_execute_sel(vc,@selector(showWaiting),nil)
                        [[NSRunLoop mainRunLoop] runUntilDate:[NSDate date]];
                        
                        if (mSingleFileType&&singleArcMode&&(archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) {
                            mdz_ArchiveFilesCnt=1;
                            [self fex_extractSingleFileToPath:[filePath UTF8String] path:[tmpArchivePath UTF8String] file_index:archiveIndex isRestarting:isRestarting];
                        } else {
                            
                            [self fex_extractToPath:[filePath UTF8String] path:[tmpArchivePath UTF8String] isRestarting:isRestarting];
                        }
                        
                        mdz_safe_execute_sel(vc,@selector(hideWaitingCancel),nil)
                        mdz_safe_execute_sel(vc,@selector(hideWaiting),nil)
                        [[NSRunLoop mainRunLoop] runUntilDate:[NSDate date]];
                        
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
                
                
                
                NSError *err;
                NSString *tmpArchivePath=[NSString stringWithFormat:@"%@/tmp/tmpArchive",NSHomeDirectory()];
                
                if (!isRestarting) {
                    //remove tmp dir
                    [mFileMngr removeItemAtPath:tmpArchivePath error:&err];
                    //extract to tmp dir
                    [mFileMngr createDirectoryAtPath:tmpArchivePath withIntermediateDirectories:TRUE attributes:nil error:&err];
                    [self addSkipBackupAttributeToItemAtPath:tmpArchivePath];
                    argc=6;
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
                }
                                    
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
    for (int i=0;i<[filetype_extV2M count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extV2M objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_V2M]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extV2M objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_V2M]];
            break;
        }
    }
    for (int i=0;i<[filetype_extHC count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extHC objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_HC]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extHC objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_HC]];
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
    
    mod_currentfile=[NSString stringWithString:filePath];
    mod_currentext=[NSString stringWithString:extension];
    
    
    m_voicesDataAvail=0;
    m_voicesForceOfs=-1;
    for (int i=0;i<SOUND_MAXMOD_CHANNELS;i++) m_voicesStatus[i]=1;
    for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
        m_voice_current_ptr[i]=0;
        memset(m_voice_buff[i],0,SOUND_BUFFER_SIZE_SAMPLE);
    }
    for (int j=0;j<SOUND_BUFFER_NB;j++) {
        memset(m_voice_buff_ana[j],0,SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
        memset(m_voice_buff_ana_cpy[j],0,SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
    }
    memset(m_voice_ChipID,0,sizeof(int)*SOUND_MAXVOICES_BUFFER_FX);
    m_voice_current_system=0;
    mSIDSeekInProgress=0;
    sprintf(mmp_fileext,"%s",[extension UTF8String] );
    mod_title=nil;
    
    for (int i=0;i<[available_player count];i++) {
        int pl_idx=[((NSNumber*)[available_player objectAtIndex:i]) intValue];        
        //NSLog(@"pl_idx: %d",i);
        switch (pl_idx) {
            case MMP_TIMIDITY:
                if ([self mmp_timidityLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_VGMSTREAM:
                if ([self mmp_vgmstreamLoad:filePath extension:extension subsong:0]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_2SF:
                if ([self mmp_2sfLoad:filePath extension:extension]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_V2M:
                if ([self mmp_v2mLoad:filePath extension:extension]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_VGMPLAY:
                if ([self mmp_vgmplayLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
                break;
            case MMP_PMDMINI:
                if ([self mmp_pmdminiLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
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
            case MMP_HC:
                if ([self MMP_HCLoad:filePath]==0) return 0; //SUCCESSFULLY LOADED
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
    
    while (bGlobalSoundHasStarted<SOUND_BUFFER_NB/2) {
        [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_UADE_MS];
        counter++;
        if (counter*DEFAULT_WAIT_TIME_UADE_MS>2) break;
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
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            openmpt_module_select_subsong(openmpt_module_ext_get_module(ompt_mod), mod_currentsub);
            iModuleLength=openmpt_module_get_duration_seconds( openmpt_module_ext_get_module(ompt_mod) )*1000;
            if (mLoopMode) iModuleLength=-1;
            if (startPos) [self Seek:startPos];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_ADPLUG:  //ADPLUG
            if (startPos) [self Seek:startPos];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_HC:  //HE
            [self Play];
            if (startPos) [self Seek:startPos];
            break;
        case MMP_UADE:  //UADE
            mod_wantedcurrentsub=subsong;
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_HVL://HVL/AHX
            //moveToSubSong=1;
            //moveToSubSongIndex=subsong;
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            hvl_InitSubsong( hvl_song,mod_currentsub );
            iModuleLength=hvl_GetPlayTime(hvl_song);
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
            api68_play( sc68, mod_currentsub, (mLoopMode?0:-1));
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
            //moveToSubSong=1;
            //moveToSubSongIndex=;
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            iModuleLength=asap->moduleInfo.durations[mod_currentsub];
            if (iModuleLength<1000) iModuleLength=1000;
            ASAP_PlaySong(asap, mod_currentsub, iModuleLength);
            
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
        case MMP_2SF: //2SF
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_V2M: //V2M
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
        case MMP_VGMSTREAM: //VGMSTREAM
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            if (mod_subsongs>1) {
                if (vgmStream != NULL) close_vgmstream(vgmStream);
                vgmStream = NULL;
                mdz_safe_free(vgm_sample_data);
                mdz_safe_free(vgm_sample_data_float);
                mdz_safe_free(vgm_sample_converted_data_float);
                if (src_state) src_delete(src_state);
                src_state=NULL;
                [self mmp_vgmstreamLoad:mod_currentfile extension:mod_currentext subsong:mod_currentsub];
            }
            if (startPos) [self Seek:startPos];
            [self Play];
            break;
    }
}

-(void) MMP_HCClose {
    if (HC_emulatorCore) {
        if ( HC_type == 0x21 ) {
            usf_shutdown( HC_emulatorCore );
            free( HC_emulatorCore );
        } else {
            free( HC_emulatorCore );
        }
        HC_emulatorCore=NULL;
    }
    if ( HC_type == 2 && HC_emulatorExtra ) {
        psf2fs_delete( HC_emulatorExtra );
        HC_emulatorExtra = NULL;
    } else if (HC_type==0x21) {
        if (lzu_state) {
            if (lzu_state->emu_state) usf_shutdown(lzu_state->emu_state);
            mdz_safe_free(lzu_state->emu_state);
            delete lzu_state;
        }
        lzu_state=NULL;
        mdz_safe_free(usf_info_data);
    } else if ( HC_type==0x23) {
        if (snsf_rom) {
            snsf_term();
            delete snsf_rom;
            snsf_rom=NULL;
        }
    } else if ( HC_type == 0x41 && HC_emulatorExtra ) {
        struct qsf_loader_state * state = ( struct qsf_loader_state * ) HC_emulatorExtra;
        free( state->key );
        free( state->z80_rom );
        free( state->sample_rom );
        free( state );
        HC_emulatorExtra = nil;
    }

    mdz_safe_free(hc_sample_data);
    mdz_safe_free(hc_sample_data_float);
    mdz_safe_free(hc_sample_converted_data_float);
    
    if (src_state) src_delete(src_state);
    src_state=NULL;
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
        mdz_safe_free(xmp_mi);
    }
    if (mPlayType==MMP_OPENMPT) {
        if (ompt_patterns) {
            for (int i=0;i<numPatterns;i++) {
                if (ompt_patterns[i]) free(ompt_patterns[i]);
            }
            free(ompt_patterns);
        }
        ompt_patterns=NULL;
        
        if (ompt_mod) openmpt_module_ext_destroy(ompt_mod);
        ompt_mod=NULL;
        if (mp_data) free(mp_data);
        mp_data=NULL;
    }
    if (mPlayType==MMP_ADPLUG) {
        delete adPlugPlayer;
        adPlugPlayer=NULL;
        delete opl;
        opl=NULL;
        delete adplugDB;
        adplugDB=NULL;
    }
    if (mPlayType==MMP_HC) {
        [self MMP_HCClose];
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
    if (mPlayType==MMP_TIMIDITY) { //VGM
    }
    if (mPlayType==MMP_PMDMINI) { //PMD
        pmd_stop();
    }
    if (mPlayType==MMP_VGMPLAY) { //VGM
        StopVGM();
        CloseVGMFile();
        VGMPlay_Deinit();
        if (AppPaths[0]) free(AppPaths[0]);
        AppPaths[0]=NULL;
        for (int i=0;i<vgmplay_activeChipsNb;i++) {
            if (vgmplay_activeChipsName[i]) free(vgmplay_activeChipsName[i]);
            vgmplay_activeChipsName[i]=NULL;
        }
    }
    if (mPlayType==MMP_2SF) { //2SF
        if (xSFPlayer) delete xSFPlayer;
        xSFPlayer=NULL;
        if (xSFConfig) delete xSFConfig;
        xSFConfig=NULL;
    }
    if (mPlayType==MMP_V2M) { //V2M
        if (v2m_player) {
            v2m_player->Close();
            delete v2m_player;
            v2m_player=NULL;
        }
        if (mp_data) delete mp_data;
        mp_data=NULL;
        mdz_safe_free(v2m_sample_data_float);
    }
    if (mPlayType==MMP_VGMSTREAM) { //VGMSTREAM
        if (vgmStream != NULL) close_vgmstream(vgmStream);
        vgmStream = NULL;
        mdz_safe_free(vgm_sample_data);
        mdz_safe_free(vgm_sample_data_float);
        mdz_safe_free(vgm_sample_converted_data_float);
        if (src_state) src_delete(src_state);
        src_state=NULL;
    }
}

-(bool) isPaused {
    return (bGlobalAudioPause?true:false);
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
    NSString *modMessage=nil;
    if (mPlayType==MMP_MDXPDX) return [NSString stringWithCString:mod_message encoding:NSShiftJISStringEncoding];
    if (mod_message[0]) modMessage=[NSString stringWithUTF8String:mod_message];
    if (modMessage==nil) {
        modMessage=[NSString stringWithFormat:@"%s",mod_message];
        if (modMessage==nil) return @"";
    }
    return modMessage;
}

-(NSString*) getModFileTitle {
    //TODO: use title tag when available
    if (mod_title) return mod_title;
    return [NSString stringWithFormat:@"%s",mod_filename];
}

-(NSString*) getModName {
    NSString *modName;
    if (mPlayType==MMP_MDXPDX) return [NSString stringWithCString:mod_name encoding:NSShiftJISStringEncoding];
    modName=[NSString stringWithUTF8String:mod_name];
    
    if (modName==nil) return @"";
    return modName;
}

-(NSString*) getPlayerName {
    if (mPlayType==MMP_GME) return @"Game Music Emulator";
    if (mPlayType==MMP_OPENMPT) return @"OpenMPT";
    if (mPlayType==MMP_XMP) return @"XMP";
    if (mPlayType==MMP_ADPLUG) return @"Adplug";
    if (mPlayType==MMP_HC) return @"Highly Complete";
    if (mPlayType==MMP_UADE) return @"UADE";
    if (mPlayType==MMP_HVL) return @"HVL";
    if (mPlayType==MMP_SIDPLAY) return ((sid_engine?@"SIDPLAY/ReSIDFP":@"SIDPLAY/ReSID"));
    if (mPlayType==MMP_STSOUND) return @"STSOUND";
    if (mPlayType==MMP_SC68) return @"SC68";
    if (mPlayType==MMP_MDXPDX) return @"MDX";
    if (mPlayType==MMP_GSF) return @"GSF";
    if (mPlayType==MMP_ASAP) return @"ASAP";
    if (mPlayType==MMP_TIMIDITY) return @"Timidity";
    if (mPlayType==MMP_PMDMINI) return @"PMDMini";
    if (mPlayType==MMP_VGMPLAY) return @"VGMPlay";
    if (mPlayType==MMP_2SF) return @"2SF";
    if (mPlayType==MMP_V2M) return @"V2M";
    if (mPlayType==MMP_VGMSTREAM) return @"VGMSTREAM";
    return @"";
}
-(NSString*) getSubTitle:(int)subsong {
    NSString *result;
    if ((subsong<mod_minsub)||(subsong>mod_maxsub)) return @"";
    if (mPlayType==MMP_VGMSTREAM) {
        if (subsong==mod_currentsub) {
            if (vgmStream && vgmStream->stream_name[0]) result=[NSString stringWithFormat:@"%.3d-%s",subsong-mod_minsub+1,vgmStream->stream_name];
            else result=[NSString stringWithFormat:@"%.3d"];
        } else {
            VGMSTREAM* vgmStreamTmp = NULL;
            STREAMFILE* vgmFileTmp = NULL;
            
            vgmFileTmp = open_stdio_streamfile([mod_currentfile UTF8String]);
            if (!vgmFileTmp) {
                result=[NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
            } else {
                vgmFileTmp->stream_index=subsong;
                vgmStreamTmp = init_vgmstream_from_STREAMFILE(vgmFileTmp);
                close_streamfile(vgmFileTmp);
                if (vgmStreamTmp) {
                    if (vgmStreamTmp->stream_name[0]) result=[NSString stringWithFormat:@"%.3d-%s",subsong-mod_minsub+1,vgmStreamTmp->stream_name];
                    else result=[NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
                    close_vgmstream(vgmStreamTmp);
                } else result=[NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
            }
        }
        return result;
    } else if (mPlayType==MMP_OPENMPT) {
        const char *ret=openmpt_module_get_subsong_name(openmpt_module_ext_get_module(ompt_mod), subsong);
        if (ret) {
            result=[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:ret]];
        } else result=[NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
        return result;
    } else if (mPlayType==MMP_GME) {
        if (gme_track_info( gme_emu, &gme_info, subsong )==0) {
            int sublen=gme_info->play_length;
            if (sublen<=0) sublen=optGENDefaultLength;
            
            result=nil;
            if (gme_info->song){
                if (gme_info->song[0]) result=[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:gme_info->song]];
            }
            if ((!result)&&(gme_info->game)) {
                if (gme_info->game[0]) result=[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:gme_info->game]];
            }
            if (!result) result=[NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
            gme_free_info(gme_info);
            return result;
        }
    } else if (mPlayType==MMP_SIDPLAY) {
        if (sidtune_name) {
            if (sidtune_name[subsong]) return [NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:sidtune_name[subsong]]];
        }
        if (sidtune_title) {
            if (sidtune_title[subsong]) return [NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:sidtune_title[subsong]]];
        }
        
        const SidTuneInfo *sidtune_info;
        sidtune_info=mSidTune->getInfo();
        if (sidtune_info->infoString(0)[0]) return [NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:sidtune_info->infoString(0)]];
        
        return [NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
    }
    return [NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
}
-(NSString*) getModType {
    if (mPlayType==MMP_GME) {
        
        return [NSString stringWithFormat:@"%s",gmetype];
    }
    if (mPlayType==MMP_XMP) {
        return [NSString stringWithFormat:@"%s",xmp_mi->mod->type];
    }
    if (mPlayType==MMP_OPENMPT) {
        char *str_type=(char*)openmpt_module_get_metadata(openmpt_module_ext_get_module(ompt_mod),"type_long");//(char*)ModPlug_GetModuleTypeLStr(mp_file);
        char *str_cont=(char*)openmpt_module_get_metadata(openmpt_module_ext_get_module(ompt_mod),"container_long");//(char*)ModPlug_GetModuleContainerLStr(mp_file);
        NSString *result=[NSString stringWithFormat:@"%s %s",str_type,str_cont];
        if (str_type) free(str_type);
        if (str_cont) free(str_cont);
        return result;
    }
    if (mPlayType==MMP_ADPLUG) return [NSString stringWithFormat:@"%s",(adPlugPlayer->gettype()).c_str()];
    if (mPlayType==MMP_HC) {
        switch (HC_type) {
            case 1:
                return [NSString stringWithFormat:@"PS1"];
            case 2:
                return [NSString stringWithFormat:@"PS2"];
            case 0x11:
                return [NSString stringWithFormat:@"SSF"];
            case 0x12:
                return [NSString stringWithFormat:@"DSF"];
            case 0x21:
                return [NSString stringWithFormat:@"USF"];
            case 0x23:
                return [NSString stringWithFormat:@"SNSF"];
            case 0x41:
                return [NSString stringWithFormat:@"QSF"];
            default:
                return [NSString stringWithFormat:@"Unknown"];
        }
    }
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
    if (mPlayType==MMP_ASAP) return [NSString stringWithFormat:@"%s",ASAPInfo_GetExtDescription(mmp_fileext)];
    if (mPlayType==MMP_TIMIDITY) return @"MIDI";
    if (mPlayType==MMP_PMDMINI) return @"PMD";
    if (mPlayType==MMP_VGMPLAY) return @"VGM";
    if (mPlayType==MMP_2SF) return [NSString stringWithFormat:@"%s",mmp_fileext];
    if (mPlayType==MMP_V2M) return [NSString stringWithFormat:@"%s",mmp_fileext];
    if (mPlayType==MMP_VGMSTREAM) return [NSString stringWithFormat:@"%s",mmp_fileext];
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
-(void) optSIDSecondSIDAddress:(int)addr {
    mSIDSecondSIDAddress=addr;
}
-(void) optSIDThirdSIDAddress:(int)addr {
    mSIDThirdSIDAddress=addr;
}
-(void) optSIDForceLoop:(int)forceLoop {
    mSIDForceLoop=forceLoop;
}
-(void) optSIDFilter:(int)onoff {
    mSIDFilterON=onoff;
}

-(void) optSIDClock:(int)clockMode {
    sid_forceClock=clockMode;
}

-(void) optSIDModel:(int)modelMode {
    sid_forceModel=modelMode;
}

-(void) optSIDEngine:(char)engine {
    sid_engine=engine;
}

-(void) optSIDInterpolation:(char)mode {
    sid_interpolation=mode;
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
    optGMEEQTreble=treble;
    optGMEEQBass=bass;
    if (gme_emu) {
        gme_equalizer( gme_emu, &gme_eq );
        gme_eq.treble = optGMEEQTreble;//-14; // -50.0 = muffled, 0 = flat, +5.0 = extra-crisp
        //bass
        //          def:80;  1 = full bass, 90 = average, 16000 = almost no bass
        //log10 ->  def:1,9;  0 - 4,2
        gme_eq.bass   = pow(10,4.2-optGMEEQBass);
        gme_set_equalizer( gme_emu, &gme_eq );
    }
}


-(void) optGME_FX:(int)enabled surround:(int)surround echo:(double)echo stereo:(double)stereo {
    optGMEFXon=enabled;
    optGMEFXSurround=surround;
    optGMEFXEcho=echo;
    optGMEFXStereo=stereo;
    if (gme_emu) {
        gme_effects(gme_emu,&gme_fx);
        gme_fx.enabled=optGMEFXon;/* If 0, no effects are added */
        gme_fx.surround = optGMEFXSurround; /* If 1, some channels are put in "back", using phase inversion */
        gme_fx.echo = optGMEFXEcho;/* Amount of echo, where 0.0 = none, 1.0 = lots */
        gme_fx.stereo = optGMEFXStereo;/* Separation, where 0.0 = mono, 1.0 = hard left and right */
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
-(void) optVGMSTREAM_MaxLoop:(int)val {
    optVGMSTREAM_loop_count=val;
}
-(void) optVGMSTREAM_Fadeouttime:(int)val {
    optVGMSTREAM_fadeouttime=val;
}
-(void) optVGMSTREAM_ForceLoop:(unsigned int)val {
    optVGMSTREAM_loopmode=val;
}
-(void) optVGMSTREAM_ResampleQuality:(unsigned int)val {
    optVGMSTREAM_resampleQuality=val;
}
    
///////////////////////////
//HC
///////////////////////////
-(void) optHC_ResampleQuality:(unsigned int)val {
    optHC_ResampleQuality=val;
}

///////////////////////////
//XMP
///////////////////////////
-(void) optXMP_SetInterpolation:(int) mode {
    switch (mode) {
        case 0:
            optXMP_InterpolationValue=XMP_INTERP_NEAREST;
            break;
        case 1:
            optXMP_InterpolationValue=XMP_INTERP_LINEAR;
            break;
        case 2:
            optXMP_InterpolationValue=XMP_INTERP_SPLINE;
            break;
    }
    if (xmp_ctx) xmp_set_player(xmp_ctx,XMP_PLAYER_INTERP,optXMP_InterpolationValue);
    
}
-(void) optXMP_SetStereoSeparation:(int) value {
    optXMP_StereoSeparationVal=value;
    if (xmp_ctx) xmp_set_player(xmp_ctx,XMP_PLAYER_MIX,optXMP_StereoSeparationVal);
}
-(void) optXMP_SetAmp:(int) value {
    optXMP_AmpValue=value;
    if (xmp_ctx) xmp_set_player(xmp_ctx,XMP_PLAYER_AMP,optXMP_AmpValue);
}
-(void) optXMP_SetDSP:(bool) value {
    if (value) optXMP_DSP=XMP_DSP_LOWPASS;
    else optXMP_DSP=0;
    if (xmp_ctx) xmp_set_player(xmp_ctx,XMP_PLAYER_DSP,optXMP_DSP);
}
-(void) optXMP_SetFLAGS:(bool) value {
    if (value) optXMP_Flags=XMP_FLAGS_A500;
    else optXMP_Flags=0;
    if (xmp_ctx) xmp_set_player(xmp_ctx,XMP_PLAYER_CFLAGS,optXMP_Flags);
}
-(void) optXMP_SetMasterVol:(int) value {
    optXMP_MasterVol=value;
    if (xmp_ctx) xmp_set_player(xmp_ctx,XMP_PLAYER_VOLUME,optXMP_MasterVol);
}


///////////////////////////
//Openmpt
///////////////////////////
-(void) optOMPT_Sampling:(int) mode {
    int val;    
    switch(mode) {
        case 0: //internal default
            val=0;break;
        case 1://no interpolation (zero order hold)
            val=1;break;
        case 2: //linear interpolation
            val=2;break;
        case 3: //cubic interpolation
            val=4;break;
        case 4: //windowed sinc with 8 taps
            val=8;break;
    }
    optOMPT_SamplingVal=val;
    if (ompt_mod) openmpt_module_set_render_param(openmpt_module_ext_get_module(ompt_mod),OPENMPT_MODULE_RENDER_INTERPOLATIONFILTER_LENGTH,optOMPT_SamplingVal);
}
-(void) optOMPT_StereoSeparation:(float) val {
    optOMPT_StereoSeparationVal=val*100;
    if (ompt_mod) openmpt_module_set_render_param(openmpt_module_ext_get_module(ompt_mod),OPENMPT_MODULE_RENDER_STEREOSEPARATION_PERCENT,optOMPT_StereoSeparationVal);
}
-(void) optOMPT_MasterVol:(float) mstVol {
    optOMPT_MasterVol=(int32_t)(2000.0*log10((int )(mstVol*256)/128.0));
    if (ompt_mod) openmpt_module_set_render_param(openmpt_module_ext_get_module(ompt_mod),OPENMPT_MODULE_RENDER_MASTERGAIN_MILLIBEL,optOMPT_MasterVol);
}

///////////////////////////
-(void) setLoopInf:(int)val {
    mLoopMode=val;
}
-(void) Seek:(int) seek_time {
    if ((mPlayType==MMP_UADE)  ||(mPlayType==MMP_MDXPDX)||(mPlayType==MMP_PMDMINI)||mNeedSeek) return;
    
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

-(bool) isVoicesMutingSupported {
    switch (mPlayType) {
        case MMP_HC:
            if ((HC_type==1)||(HC_type==2)) return true;
            if ((HC_type==0x11)||(HC_type==0x12)) return true;
            if (HC_type==0x23) return true;
            if (HC_type==0x41) return true;
            return false;
        case MMP_2SF:
        case MMP_V2M:
        case MMP_GSF:
        case MMP_UADE:
        case MMP_OPENMPT:
        case MMP_XMP:
        case MMP_GME:
        case MMP_SIDPLAY:
        case MMP_ASAP:
        case MMP_VGMPLAY:
            return true;
        default: return false;
    }
}

-(NSString*) getVoicesName:(unsigned int)channel {
    if (channel>=SOUND_MAXMOD_CHANNELS) return nil;
    switch (mPlayType) {
        case MMP_2SF:
            return [NSString stringWithFormat:@"#%d-NDS",channel+1];
        case MMP_V2M:
            return [NSString stringWithFormat:@"#%d-V2",channel+1];
        case MMP_HC:
            if (HC_type==1) return [NSString stringWithFormat:@"#%d-SPU",channel+1];
            else if (HC_type==2) return [NSString stringWithFormat:@"#%d-SPU#%d",(channel%24)+1,(channel/24)+1];
            else if (HC_type==0x11) return [NSString stringWithFormat:@"#%d-SCSP",channel+1];
            else if (HC_type==0x12) return [NSString stringWithFormat:@"#%d-AICA",channel+1];
            else if (HC_type==0x23) return [NSString stringWithFormat:@"#%d-SPC700",channel+1];
            else if (HC_type==0x41) return [NSString stringWithFormat:@"#%d-QSOUND",channel+1];
            return @"";
        case MMP_OPENMPT: {
            NSString *result;
            char *strTmp=(char*)openmpt_module_get_channel_name(openmpt_module_ext_get_module(ompt_mod),channel);
            if (strTmp&&strTmp[0]) {
                result=[NSString stringWithFormat:@"#%d-%s",channel+1,strTmp];
                free(strTmp);
            } else result=[NSString stringWithFormat:@"#%d-OMPT",channel+1];
            return result;
        }
        case MMP_GSF:
            if (channel<4) return [NSString stringWithFormat:@"#%d-DMG",channel+1];
            else return [NSString stringWithFormat:@"#%d-DirectSnd",channel-4+1];
        case MMP_UADE:
            return [NSString stringWithFormat:@"#%d-PAULA",channel+1];
        case MMP_XMP:
            return [NSString stringWithFormat:@"#%d-XMP",channel+1];
        case MMP_ASAP:
            return [NSString stringWithFormat:@"#%d-POKEY#%d",(channel&3)+1,channel/4+1];
        case MMP_GME:
            return [NSString stringWithFormat:@"%s",gme_voice_name(gme_emu,channel)];
        case MMP_SIDPLAY:
            return [NSString stringWithFormat:@"#%d-SID#%d",(channel%3)+1,channel/3+1];
        case MMP_VGMPLAY:{
            int idx=0;
            for (int i=0;i<vgmplay_activeChipsNb;i++) {
                if ((channel>=idx)&&(channel<idx+vgmGetVoicesNb(vgmplay_activeChips[i]))) {
                    return [NSString stringWithFormat:@"#%d-%s#%d",channel-idx+1,GetChipName(vgmplay_activeChips[i]),vgmplay_activeChipsID[i]+1];
                }
                idx+=vgmGetVoicesNb(vgmplay_activeChips[i]);
            }
            return @"";
        }
        default:
            return [NSString stringWithFormat:@"#%d",channel+1];
            break;
    }
}

-(int) getSystemsNb {
    switch (mPlayType) {
        case MMP_HC:
            if (HC_type==1) return 1;
            else if (HC_type==2) return 2;
            else if ((HC_type==0x11)||(HC_type==0x12)) return 1;
            else if (HC_type==0x23) return 1;
            else if (HC_type==0x41) return 1;
            return 1;
        case MMP_GSF:
            return 2;
        case MMP_2SF:
        case MMP_V2M:
        case MMP_UADE:
        case MMP_OPENMPT:
        case MMP_XMP:
        case MMP_GME:
            return 1;
        case MMP_ASAP:
            return numChannels/4;
        case MMP_VGMPLAY:
            return vgmplay_activeChipsNb;
        case MMP_SIDPLAY:
            return numChannels/3; //number of sidchip active: voices/3, (3ch each)
        default:
            return 0;
    }
}

-(NSString*) getSystemsName:(int)systemIdx {
    switch (mPlayType) {
        case MMP_2SF:
            return @"NDS";
        case MMP_V2M:
            return @"V2";
        case MMP_HC:
            if (HC_type==1) return @"SPU";
            else if (HC_type==2) return [NSString stringWithFormat:@"SPU#%d",systemIdx + 1];
            else if (HC_type==0x11) return @"SCSP";
            else if (HC_type==0x12) return @"AICA";
            else if (HC_type==0x23) return @"SPC700";
            else if (HC_type==0x41) return @"QSOUND";
            return @"";
        case MMP_GSF:
            if (systemIdx==0) return @"DMG";
            return @"DirectSnd";
        case MMP_UADE:
            return @"PAULA";
        case MMP_OPENMPT:
            return @"OMPT";
        case MMP_XMP:
            return @"XMP";
        case MMP_ASAP:
            if (systemIdx==0) return @"POKEY#1";
            return @"POKEY#2";
        case MMP_GME:
            if (strcmp(gmetype,"Super Nintendo")==0) {//SPC
                return @"SPC700";
            }
            return [NSString stringWithFormat:@"%s",gmetype];
        case MMP_VGMPLAY:
            return [NSString stringWithFormat:@"%s#%d",vgmplay_activeChipsName[systemIdx],vgmplay_activeChipsID[systemIdx] + 1];
        case MMP_SIDPLAY:
            return [NSString stringWithFormat:@"Sid#%d",systemIdx + 1]; //number of sidchip active: voices/3, (3ch each)
        default:
            return 0;
    }
}

-(int) getSystemForVoice:(int)voiceIdx {
    switch (mPlayType) {
        case MMP_HC:
            if (HC_type==1) return 0;
            else if (HC_type==2) return voiceIdx/24;
            else if ((HC_type==0x11)||(HC_type==0x12)) return 0;
            else if (HC_type==0x23) return 0;
            else if (HC_type==0x41) return 0;
            return 0;
        case MMP_GSF:
            if (voiceIdx<4) return 0;
            return 1;
        case MMP_2SF:
        case MMP_V2M:
        case MMP_UADE:
        case MMP_OPENMPT:
        case MMP_XMP:
        case MMP_GME:
            return 0;
        case MMP_ASAP:
            return voiceIdx/4;
        case MMP_VGMPLAY: {
            int idx=0;
            for (int i=0;i<vgmplay_activeChipsNb;i++) {
                if (voiceIdx<idx+vgmGetVoicesNb(vgmplay_activeChips[i])) return i;
                idx+=vgmGetVoicesNb(vgmplay_activeChips[i]);
            }
            return 0;
        }
        case MMP_SIDPLAY:
            return voiceIdx/3;
        default:
            return 0;
    }
}

-(int) getSystemm_voicesStatus:(int)systemIdx {
    int tmp;
    switch (mPlayType) {
        case MMP_HC:
            tmp=0;
            if (HC_type==1) {
                for (int i=0;i<numChannels;i++) tmp+=(m_voicesStatus[i]?1:0);
                if (tmp==numChannels) return 2; //all active
                else if (tmp>0) return 1; //partially active
            } else if (HC_type==2) {
                for (int i=0;i<24;i++) tmp+=(m_voicesStatus[i+systemIdx*24]?1:0);
                if (tmp==24) return 2; //all active
                else if (tmp>0) return 1; //partially active
            } else if ((HC_type==0x11)||(HC_type==0x12)||(HC_type==0x23)||(HC_type==0x41)) { //scsp or aica or spc700 or qsound
                for (int i=0;i<numChannels;i++) tmp+=(m_voicesStatus[i]?1:0);
                if (tmp==numChannels) return 2; //all active
                else if (tmp>0) return 1; //partially active
            }
            return 0;
        case MMP_GSF:
            if (systemIdx==0) {
                tmp=0;
                for (int i=0;i<4;i++) {
                    tmp+=(m_voicesStatus[i]?1:0);
                }
                if (tmp==4) return 2; //all active
                else if (tmp>0) return 1; //partially active
                return 0; //all off
            } else {
                tmp=0;
                for (int i=4;i<6;i++) {
                    tmp+=(m_voicesStatus[i]?1:0);
                }
                if (tmp==2) return 2; //all active
                else if (tmp>0) return 1; //partially active
                return 0; //all off
            }
        case MMP_2SF:
        case MMP_V2M:
        case MMP_UADE:
        case MMP_OPENMPT:
        case MMP_XMP:
        case MMP_GME:
            tmp=0;
            for (int i=0;i<numChannels;i++) {
                tmp+=(m_voicesStatus[i]?1:0);
            }
            if (tmp==numChannels) return 2; //all active
            else if (tmp>0) return 1; //partially active
            return 0; //all off
        case MMP_ASAP:
            tmp=0;
            for (int i=0;i<4;i++) {
                tmp+=(m_voicesStatus[i+systemIdx*4]?1:0);
            }
            if (tmp==4) return 2; //all active
            else if (tmp>0) return 1; //partially active
            return 0; //all off
        case MMP_VGMPLAY: {
            int idx=0;
            //1st reach 1st voice of systemIdx
            for (int i=0;i<systemIdx;i++) {
                idx+=vgmGetVoicesNb(vgmplay_activeChips[i]);
            }
            //2nd check active voices/total
            tmp=0;
            for (int i=0;i<vgmGetVoicesNb(vgmplay_activeChips[systemIdx]);i++) {
                tmp+=(m_voicesStatus[idx+i]?1:0);
            }
            if (tmp==vgmGetVoicesNb(vgmplay_activeChips[systemIdx])) return 2; //all active
            else if (tmp>0) return 1; //partially active
            return 0; //all off
        }
        case MMP_SIDPLAY:
            tmp=0;
            for (int i=systemIdx*3;i<systemIdx*3+3;i++) {
                tmp+=(m_voicesStatus[i]?1:0);
            }
            if (tmp==3) return 2; //all active
            else if (tmp>0) return 1; //partially active
            return 0; //all off
        default:
            return 0;
    }
}

-(void) setSystemm_voicesStatus:(int)systemIdx active:(bool)active {
    switch (mPlayType) {
        case MMP_HC:
            if (HC_type==1) for (int i=0;i<numChannels;i++) [self setm_voicesStatus:active index:i]; //PSF, 24voices
            else if (HC_type==2) for (int i=0;i<24;i++) [self setm_voicesStatus:active index:(i+systemIdx*24)]; //PSF2, 48voices
            else if (HC_type==0x11) for (int i=0;i<numChannels;i++) [self setm_voicesStatus:active index:i]; //SCSP, 32voices
            else if (HC_type==0x12) for (int i=0;i<numChannels;i++) [self setm_voicesStatus:active index:i]; //AICA, 64voices
            else if (HC_type==0x23) for (int i=0;i<numChannels;i++) [self setm_voicesStatus:active index:i]; //SPC700, 8voices
            else if (HC_type==0x41) for (int i=0;i<numChannels;i++) [self setm_voicesStatus:active index:i]; //QSound, 19voices
            break;
        case MMP_GSF:
            if (systemIdx==0) for (int i=0;i<4;i++) [self setm_voicesStatus:active index:i];
            else for (int i=4;i<6;i++) [self setm_voicesStatus:active index:i];
            break;
        case MMP_2SF:
        case MMP_V2M:
        case MMP_UADE:
        case MMP_OPENMPT:
        case MMP_XMP:
        case MMP_GME:
            for (int i=0;i<numChannels;i++) [self setm_voicesStatus:active index:i];
            break;
        case MMP_ASAP:
            for (int i=0;i<4;i++) {
                [self setm_voicesStatus:active index:systemIdx*4+i];
            }
            break;
        case MMP_VGMPLAY: {
            int idx=0;
            //1st reach 1st voice of systemIdx
            for (int i=0;i<systemIdx;i++) {
                idx+=vgmGetVoicesNb(vgmplay_activeChips[i]);
            }
            //2nd update voice status
            for (int i=0;i<vgmGetVoicesNb(vgmplay_activeChips[systemIdx]);i++) {
                [self setm_voicesStatus:active index:i+idx];
            }
            break;
        }
        case MMP_SIDPLAY:
            for (int i=systemIdx*3;i<systemIdx*3+3;i++) {
                [self setm_voicesStatus:active index:i];
            }
            break;
        default:
            break;
    }
}


-(bool) getm_voicesStatus:(unsigned int)channel {
    if (channel>=SOUND_MAXMOD_CHANNELS) return false;
    return (m_voicesStatus[channel]?true:false);
}

-(void) setm_voicesStatus:(bool)active index:(unsigned int)channel {
    if (channel>=SOUND_MAXMOD_CHANNELS) return;
    m_voicesStatus[channel]=(active?1:0);
    switch (mPlayType) {
        case MMP_2SF:
            xSFPlayer->MuteChannels(channel,active);
            break;
        case MMP_V2M:
            if (active) v2m_voices_mask|=1<<channel;
            else v2m_voices_mask&=0xFFFFFFFF^(1<<channel);
            break;
        case MMP_UADE:
            if (active) HC_voicesMuteMask1|=1<<channel;
            else HC_voicesMuteMask1&=0xFFFFFFFF^(1<<channel);
            break;
        case MMP_HC:
            if ((HC_type==1)||(HC_type==2)) {//PSF1 and PSF2}
                if (active) {
                    if (channel<24) HC_voicesMuteMask1|=1<<channel;
                    else HC_voicesMuteMask2|=1<<(channel-24);
                } else {
                    if (channel<24) HC_voicesMuteMask1&=0xFFFFFFFF^(1<<channel);
                    else HC_voicesMuteMask2&=0xFFFFFFFF^(1<<(channel-24));
                }
            } else if ((HC_type==0x11)||(HC_type==0x12)) { //SSF/SCSP or DSF/AICA
                if (active) {
                    if (channel<32) HC_voicesMuteMask1|=1<<channel;
                    else HC_voicesMuteMask2|=1<<(channel-32);
                } else {
                    if (channel<32) HC_voicesMuteMask1&=0xFFFFFFFF^(1<<channel);
                    else HC_voicesMuteMask2&=0xFFFFFFFF^(1<<(channel-32));
                }
            } else if (HC_type==0x23) { //SNSF
                if (active) HC_voicesMuteMask1|=1<<channel;
                else HC_voicesMuteMask1&=0xFFFFFFFF^(1<<channel);
            } else if (HC_type==0x41) { //QSF
                if (active) HC_voicesMuteMask1|=1<<channel;
                else HC_voicesMuteMask1&=0xFFFFFFFF^(1<<channel);
            }
            break;
        case MMP_GSF:
            if (channel<4) GSFSoundChannelsEnable(1<<channel,active);
            else GSFSoundChannelsEnable(1<<(channel-4+8),active);
            break;
        case MMP_OPENMPT:
            ompt_mod_interactive->set_channel_mute_status(ompt_mod,channel,!active);
            break;
        case MMP_XMP:
            xmp_channel_mute(xmp_ctx,channel,!active);
            break;
        case MMP_GME:{
            //if some voices are muted, deactive silence detection as it could end current song
            bool isAnyVoicesMuted=false;
            for (int i=0;i<numChannels;i++) {
                if (m_voicesStatus[i]==0) {
                    isAnyVoicesMuted=true;
                    break;
                }
            }
            if (isAnyVoicesMuted) gme_ignore_silence(gme_emu,1);
            else gme_ignore_silence(gme_emu,optGMEIgnoreSilence);
            gme_mute_voice(gme_emu,channel,(active?0:1));
            break;
        }
        case MMP_SIDPLAY:
            mSidEmuEngine->mute(channel/3,channel%3,(active?0:1));   //(unsigned int sidNum, unsigned int voice, bool enable);
            break;
        case MMP_ASAP: {
            int mask=0;
            for (int i=0;i<numChannels;i++) if (m_voicesStatus[i]==0) mask|=1<<i;
            ASAP_MutePokeyChannels(asap,mask);
            break;
        }
        case MMP_VGMPLAY:{
            int idx=0;
            int mask=0;
            for (int i=0;i<vgmplay_activeChipsNb;i++) {
                if ((channel>=idx)&&(channel<idx+vgmGetVoicesNb(vgmplay_activeChips[i]))) {
                    //
                    switch (vgmplay_activeChips[i]) {
                        case 0: //SN76496: 4voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].SN76496.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].SN76496.ChnMute1|=1<<(channel-idx);
                            break;
                        case 1: //VRC7 or YM2413
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2413.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YM2413.ChnMute1|=1<<(channel-idx);
                            break;
                        case 2: //YM2612:  6voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2612.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YM2612.ChnMute1|=1<<(channel-idx);
                            if ((channel-idx)==5) {
                                //update dac channel as well, seems when dac channel 6 is linked to fm 4
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2612.ChnMute1&=0xFFFFFFFF^(1<<6);
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2612.ChnMute1|=1<<6;
                            }
                            break;
                        case 3: //YM2151: 8voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2151.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YM2151.ChnMute1|=1<<(channel-idx);
                            break;
                        case 4: //Sega PCM: 16voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].SegaPCM.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].SegaPCM.ChnMute1|=1<<(channel-idx);
                            break;
                        case 5: //RF5C68
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].RF5C68.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].RF5C68.ChnMute1|=1<<(channel-idx);
                            break;
                        case 6: {//YM2203
                            // chnmute1 3bits -> fff f:fm 3ch
                            // chnmute3, 3bits -> yyy y:ay 3ch
                            int voice=channel-idx;
                            if (voice<3) {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2203.ChnMute1&=0xFFFFFFFF^(1<<voice);
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2203.ChnMute1|=(1<<voice);
                            } else {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2203.ChnMute3&=0xFFFFFFFF^(1<<(voice-3));
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2203.ChnMute3|=(1<<(voice-3));
                            }
                            break;
                        }
                        case 7:{ //YM2608: 16voices:
                                 // chnmute1 & chnmute2, 13bits -> daaaaaaffffff   d:delta 1ch, a:adpcm 6ch, f:fm 6ch
                                 // chnmute3, 3bits -> yyy y:ay 3ch
                            int voice=channel-idx;
                            if (voice<6) {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute1&=0xFFFFFFFF^(1<<voice);
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute1|=(1<<voice);
                            } else if (voice<13) {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute2&=0xFFFFFFFF^(1<<(voice-6));
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute2|=(1<<(voice-6));
                            } else {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute3&=0xFFFFFFFF^(1<<(voice-13));
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute3|=(1<<(voice-13));
                            }                            
                            break;
                        }
                        case 8:{ //YM2610: 14voices (4 fm), YM2610b: 16voices (6 fm)
                                 // chnmute1 & chnmute2, 13bits -> daaaaaaffffff   d:delta 1ch, a:adpcm 6ch, f:fm 6ch
                                 // chnmute3, 3bits -> yyy y:ay 3ch
                            int voice=channel-idx;
                            if (!vgm2610b) {
                                if (voice<4) {
                                    switch (voice) {
                                        case 0:voice=1;break;
                                        case 1:voice=2;break;
                                        case 2:voice=4;break;
                                        case 3:voice=5;break;
                                    }
                                } else voice+=2;
                            }
                            if (voice<6) {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute1&=0xFFFFFFFF^(1<<voice);
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute1|=(1<<voice);
                            } else if (voice<13) {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute2&=0xFFFFFFFF^(1<<(voice-6));
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute2|=(1<<(voice-6));
                            } else {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute3&=0xFFFFFFFF^(1<<(voice-13));
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute3|=(1<<(voice-13));
                            }
                            break;
                        }
                        case 9: //YM3812: 9voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YM3812.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YM3812.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x0A: //YM3526
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YM3526.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YM3526.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x0B: //Y8950
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].Y8950.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].Y8950.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x0C: //YMF262
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YMF262.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YMF262.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x0D: //YMF278B:47voices fm: 18 channels + 5 drums pcm: 24 channels
                                    //  mute1: dddddffffffffffffffffff
                                    //  mute2: pppppppppppppppppppppppp
                            if (channel-idx<23) {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YMF278B.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                                else ChipOpts[vgmplay_activeChipsID[i]].YMF278B.ChnMute1|=(1<<(channel-idx));
                            } else {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YMF278B.ChnMute2&=0xFFFFFFFF^(1<<(channel-idx-23));
                                else ChipOpts[vgmplay_activeChipsID[i]].YMF278B.ChnMute2|=(1<<(channel-idx-23));
                            }
                            break;
                        case 0x0E: //YMF271
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YMF271.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YMF271.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x0F: //YMZ280B
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YMZ280B.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YMZ280B.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x10: //RF5C164
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].RF5C164.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].RF5C164.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x11: //PWM
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].PWM.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].PWM.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x12: //AY8910
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].AY8910.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].AY8910.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x13: //GameBoy
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].GameBoy.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].GameBoy.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x14: //NES APU: 5voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].NES.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].NES.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x15: //MultiPCM
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].MultiPCM.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].MultiPCM.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x16: //UPD7759
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].UPD7759.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].UPD7759.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x17: //OKIM6258: 1voice
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].OKIM6258.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].OKIM6258.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x18: //OKIM6295: 4voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].OKIM6295.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].OKIM6295.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x19: //K051649: 5voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].K051649.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].K051649.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1A: //K054539
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].K054539.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].K054539.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1B: //HuC6280
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].HuC6280.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].HuC6280.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1C: //C140: 24voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].C140.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].C140.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1D: //k053260
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].K053260.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].K053260.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1E: //pokey
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].Pokey.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].Pokey.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1F: //qsound
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].QSound.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].QSound.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x20: //SCSP
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].SCSP.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].SCSP.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x21: //WSAN
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].WSwan.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].WSwan.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x22: //VSU
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].VSU.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].VSU.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x23: //SAA1099
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].SAA1099.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].SAA1099.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x24: //ES5503
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].ES5503.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].ES5503.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x25: //ES5506
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].ES5506.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].ES5506.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x26: //X1_010
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].X1_010.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].X1_010.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x27: //C352
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].C352.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].C352.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x28: //X1_010
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].GA20.ChnMute1&=0xFFFFFFFF^(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].GA20.ChnMute1|=1<<(channel-idx);
                            break;
                        default:
                            break;
                    }
                    break;
                    //return [NSString stringWithFormat:@"#%d-%s#%d",channel-idx,GetChipName(vgmplay_activeChips[i]),i];
                }
                idx+=vgmGetVoicesNb(vgmplay_activeChips[i]);
            }
            RefreshMuting();
            break;
        }
        default:
            break;
    }
}


//*****************************************
@end
