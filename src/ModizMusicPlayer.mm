//
//  ModizMusicPlayer.mm
//  modizer
//
//  Created by Yohann Magnien on 12/06/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//
extern void *LoadingProgressObserverContext;

#import <AVFAudio/AVAudioSession.h>

#include <pthread.h>
#include <sqlite3.h>
#include <sys/xattr.h>

extern pthread_mutex_t db_mutex;
extern pthread_mutex_t play_mutex;

int64_t iModuleLength;
double iCurrentTime;
int mod_message_updated;
int64_t mCurrentSamples,mTgtSamples,mFadeSamplesStart;

int mod_total_length;


#import "ModizMusicPlayer.h"
#import "ModizFileHelper.h"

#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

int PLAYBACK_FREQ=DEFAULT_PLAYBACK_FREQ;

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


//PxTone & Organya
#include "pxtnService.h"
#include "pxtnError.h"
// organya
extern "C" void unload_org(void);
extern "C" int org_play(const char *fn, char *buf);
extern "C" int org_getoutputtime(void);
extern "C" int org_currentpos();
extern "C" int org_gensamples(char *pixel_sample_buffer,int samplesNb);
extern "C" int org_getlength(void);

bool pixel_organya_mode;
pxtnService*    pixel_pxtn;
pxtnDescriptor*    pixel_desc;
char*            pixel_fileBuffer;
uint32_t        pixel_fileBufferLen;
uint64_t organya_mute_mask;
char org_filename[1024];
#define PIXEL_BUF_SIZE 1024

//WEBSID
extern "C" {
#include "../libs/websid/websid/src/system.h"
#include "../libs/websid/websid/src/vic.h"
#include "../libs/websid/websid/src/core.h"
#include "../libs/websid/websid/src/memory.h"
}
#include "../libs/websid/websid/src/filter6581.h"
#include "../libs/websid/websid/src/sid.h"
extern "C" void     sidWriteMem(uint16_t addr, uint8_t value);

#include "../libs/websid/websid/src/Postprocess.h"
using websid::Postprocess;

#include "../libs/websid/websid/src/SidSnapshot.h"
using websid::SidSnapshot;

#include "../libs/websid/websid/src/loaders.h"

static char*            websid_fileBuffer;
static uint32_t        websid_fileBufferLen;
static FileLoader*    websid_loader;
static char websid_skip_silence_loop = 10;
static char websid_sound_started;
static int16_t** websid_scope_buffers;
static const char **websid_info;

//GBSPLAY
extern "C" {
#include "gbs_player.h"
#include "gbs_internal.h"
struct gbs *gbs;
}

//SID2
#include "sidplayfp/sidplayfp.h"
#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidInfo.h"
#include "sidplayfp/SidTuneInfo.h"
#include "sidplayfp/SidConfig.h"

#include "builders/residfp-builder/residfp.h"
#include "builders/resid-builder/resid.h"

void* m_sid_chipId[MAXSID_CHIPS];

/* EUPMINI
 * global data
 *
 */
typedef struct {
    char    title[32]; // オフセット/offset 000h タイトルは半角32文字，全角で16文字以内の文字列で指定します。The title is specified as a character string of 32 half-width characters or less and 16 full-width characters or less.
    char    artist[8]; // オフセット/offset 020h
    char    dummy[44]; // オフセット/offset 028h
    char    trk_name[32][16]; // オフセット/offset 084h 512 = 32 * 16
    char    short_trk_name[32][8]; // オフセット/offset 254h 256 = 32 * 8
    char    trk_mute[32]; // オフセット/offset 354h
    char    trk_port[32]; // オフセット/offset 374h
    char    trk_midi_ch[32]; // オフセット/offset 394h
    char    trk_key_bias[32]; // オフセット/offset 3B4h
    char    trk_transpose[32]; // オフセット/offset 3D4h
    char    trk_play_filter[32][7]; // オフセット/offset 3F4h 224 = 32 * 7
    // ＦＩＬＴＥＲ：形式１ ベロシティフィルター/Format 1 velocity filter ＦＩＬＴＥＲ：形式２ ボリュームフィルター/FILTER: Format 2 volume filter ＦＩＬＴＥＲ：形式３ パンポットフィルター/FILTER: Format 3 panpot filter have 7 typed parameters 1 byte sized each parameter
    // ＦＩＬＴＥＲ：形式４ ピッチベンドフィルター/Format 4 pitch bend filter has 7 typed parameters some parameters are 2 bytes sized
    char    instruments_name[128][4]; // オフセット/offset 4D4h 512 = 128 * 4
    char    fm_midi_ch[6]; // オフセット/offset 6D4h
    char    pcm_midi_ch[8]; // オフセット/offset 6DAh
    char    fm_file_name[8]; // オフセット/offset 6E2h
    char    pcm_file_name[8]; // オフセット/offset 6EAh
    char    reserved[260]; // オフセット/offset 6F2h 260 = (32 * 8) + 4 ??? ＦＩＬＴＥＲ：形式４ ピッチベンドフィルター/Format 4 pitch bend filter additional space ???
    char    appli_name[8]; // オフセット/offset 7F6h
    char    appli_version[2]; // オフセット/offset 7FEh
    int32_t size; // オフセット/offset 800h
    char    signature; // オフセット/offset 804h
    char    first_tempo; // オフセット/offset 805h
} EUPHEAD;

typedef struct {
    char    trk_mute[32];
    char    trk_port[32];
    char    trk_midi_ch[32];
    char    trk_key_bias[32];
    char    trk_transpose[32];
    char    fm_midi_ch[6];
    char    pcm_midi_ch[8];
    int32_t size;
    char    signature;
    char    first_tempo;
    char*   eupData;
} EUPINFO;

struct pcm_struct eup_pcm;
int eup_mutemask;
uint8_t *eup_buf;
EUPPlayer *eup_player;
EUP_TownsEmulator *eup_dev;
EUPHEAD eup_header;
char eup_filename[1024];

#define PATH_DELIMITER      ";"
#define PATH_DELIMITER_CHAR ';'

static std::string const downcase(std::string const &s)
{
    std::string r;
    for (std::string::const_iterator i = s.begin(); i != s.end(); i++) {
        r += (*i >= 'A' && *i <= 'Z') ? (*i - 'A' + 'a') : *i;
    }
    return r;
}

static std::string const upcase(std::string const &s)
{
    std::string r;
    for (std::string::const_iterator i = s.begin(); i != s.end(); i++) {
        r += (*i >= 'a' && *i <= 'z') ? (*i - 'a' + 'A') : *i;
    }
    return r;
}

static FILE *openFile_inPath(std::string const &filename, std::string const &path)
{
    FILE *f = nullptr;
    std::vector<std::string> fn2;
    fn2.push_back(filename);
    fn2.push_back(downcase(filename));
    fn2.push_back(upcase(filename));
    
    std::string::const_iterator i = path.begin();
    while (i != path.end()) {
        std::string dir{};
        while (i != path.end() && *i != PATH_DELIMITER_CHAR) {
            dir += *i++;
        }
        if (i != path.end() && *i == PATH_DELIMITER_CHAR) {
            i++;
        }
        for (std::vector<std::string>::const_iterator j = fn2.begin(); j != fn2.end(); j++) {
            std::string const filename(dir /*+ "/"*/ +*j);
            //std::cerr << "trying " << filename << std::endl;
            f = fopen(filename.c_str(), "rb");
            if (f != nullptr) {
                //std::cerr << "loading " << filename << std::endl;
                return f;
            }
        }
    }
    if (f == nullptr) {
        fprintf(stderr, "error finding %s\n", filename.c_str());
        std::fflush(stderr);
    }
    
    return f;
}

uint8_t *EUPPlayer_readFile(EUPPlayer *player,
                            TownsAudioDevice *device,
                            std::string const &nameOfEupFile,EUPHEAD *eup_headerptr) {
    // ヘッダ読み込み用バッファ
    EUPHEAD eupHeader;
    // EUP情報領域
    EUPINFO eupInfo;
    
    // とりあえず, TOWNS emu のみに対応.
    
    uint8_t *eupbuf = nullptr;
    player->stopPlaying();
    
    {
        FILE *f = fopen(nameOfEupFile.c_str(), "rb");
        if (f == nullptr) {
            perror(nameOfEupFile.c_str());
            return nullptr;
        }
        
        struct stat statbufEupFile;
        if (fstat(fileno(f), &statbufEupFile) != 0) {
            perror(nameOfEupFile.c_str());
            fclose(f);
            return nullptr;
        }
        
        if (statbufEupFile.st_size < 2048 + 6 + 6) {
            fprintf(stderr, "%s: too short file.\n", nameOfEupFile.c_str());
            std::fflush(stderr);
            fclose(f);
            return nullptr;
        }
        
        size_t eupRead = fread(&eupHeader, 1, sizeof(EUPHEAD), f);
        if (eupRead != sizeof(EUPHEAD)) {
            fprintf(stderr, "EUP file does not follow specification.\n");
            std::fflush(stderr);
            fclose(f);
            return nullptr;
        }
        else {
            int trk;
            
            // データー領域の確保
            eupInfo.eupData = nullptr/*new uint8_t[eupHeader.size]*/;
            //if (eupInfo.eupData == nullptr break;
            
            eupInfo.first_tempo = eupHeader.first_tempo;
            //player->tempo(eupInfo.first_tempo + 30);
            //fprintf(stderr, "(eupInfo.first_tempo + 30) is %u.\n", static_cast<unsigned char>(eupInfo.first_tempo + 30u));
            
            player->tempo(eupInfo.first_tempo + 30);
            
            // ヘッダ情報のコピー
            for (trk = 0; trk < 32; trk++) {
                eupInfo.trk_mute[trk] = eupHeader.trk_mute[trk];
                eupInfo.trk_port[trk] = eupHeader.trk_port[trk];
                eupInfo.trk_midi_ch[trk] = eupHeader.trk_midi_ch[trk];
                eupInfo.trk_key_bias[trk] = eupHeader.trk_key_bias[trk];
                eupInfo.trk_transpose[trk] = eupHeader.trk_transpose[trk];
                
                player->mapTrack_toChannel(trk, eupInfo.trk_midi_ch[trk]);
                //fprintf(stderr, "eupInfo.trk_midi_ch[%d] is %d.\n", trk, eupInfo.trk_midi_ch[trk]);
            }
            
            
            // FM/PCM may be arbitrarily mapped to the available 14 slots
            for (int n = 0; n < 6; n++) {
                eupInfo.fm_midi_ch[n] = eupHeader.fm_midi_ch[n];
                device->assignFmDeviceToChannel(eupInfo.fm_midi_ch[n], n);
            }
            for (int n = 0; n < 8; n++) {
                eupInfo.pcm_midi_ch[n] = eupHeader.pcm_midi_ch[n];
                device->assignPcmDeviceToChannel(eupInfo.pcm_midi_ch[n]);
            }
            
            eupInfo.signature = eupHeader.signature;
            //eupInfo.first_tempo = eupHeader.first_tempo;
            eupInfo.size = eupHeader.size;
        }
        
        eupbuf = new uint8_t[statbufEupFile.st_size];
        fseek(f, 0, SEEK_SET); // seek to start
        eupRead = fread(eupbuf, 1, statbufEupFile.st_size, f);
        if (eupRead != statbufEupFile.st_size) {
            fprintf(stderr, "EUP not fully read: %zu instead of %lu.\n", eupRead, statbufEupFile.st_size);
            fclose(f);
            delete eupbuf;
            return nullptr;
        }
        fclose(f);
    }
    
    //player->tempo(eupInfo.first_tempo + 30);
    
    char nameBuf[1024];
    char *fmbpmbdir = std::getenv("HOME");
    if (nullptr != fmbpmbdir)
    {
        std::strcpy(nameBuf, fmbpmbdir);
        std::strcat(nameBuf, "/.eupplay/");
    }
    else
    {
        nameBuf[0] = 0;
    }
    std::string fmbPath(nameBuf);
    std::string pmbPath(nameBuf);
    std::string eupDir(nameOfEupFile.substr(0, nameOfEupFile.rfind("/") + 1)/* + "/"*/);
    fmbPath = eupDir + PATH_DELIMITER + fmbPath;
    //std::cerr << "fmbPath is " << fmbPath << std::endl;
    pmbPath = eupDir + PATH_DELIMITER + pmbPath;
    //std::cerr << "pmbPath is " << pmbPath << std::endl;
    
    {
        uint8_t instrument[] = {
            ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // name
            17, 33, 10, 17,                         // detune / multiple
            25, 10, 57, 0,                          // output level
            154, 152, 218, 216,                     // key scale / attack rate
            15, 12, 7, 12,                          // amon / decay rate
            0, 5, 3, 5,                             // sustain rate
            38, 40, 70, 40,                         // sustain level / release rate
            20,                                     // feedback / algorithm
            0xc0,                                   // pan, LFO
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };
        for (int n = 0; n < 128; n++) {
            device->setFmInstrumentParameter(n, instrument);
        }
    }
    /* FMB */
    {
        char fn0[16];
        memcpy(fn0, eupHeader.fm_file_name/*eupbuf + 0x6e2*/, 8); /* verified 0x06e2 offset */
        fn0[8] = '\0';
        std::string fn1(std::string(fn0) + ".fmb");
        if (fn1!=".fmb") {
            FILE *f = openFile_inPath(fn1, fmbPath);
            if (f != nullptr) {
                struct stat statbuf1;
                fstat(fileno(f), &statbuf1);
                uint8_t buf1[statbuf1.st_size];
                size_t fmbRead = fread(buf1, 1, statbuf1.st_size, f);
                if (fmbRead != statbuf1.st_size) {
                    fprintf(stderr, "FMB not fully read: %zu instead of %lu.\n", fmbRead, statbuf1.st_size);
                }
                fclose(f);
                for (int n = 0; n < (statbuf1.st_size - 8) / 48; n++) {
                    device->setFmInstrumentParameter(n, buf1 + 8 + 48 * n);
                }
                
            } else {
                delete eupbuf;
                return nullptr;
            }
        }
    }
    /* PMB */
    {
        char fn0[16];
        memcpy(fn0, eupHeader.pcm_file_name/*eupbuf + 0x6ea*/, 8); /* verified 0x06ea offset */
        fn0[8] = '\0';
        std::string fn1(std::string(fn0) + ".pmb");
        if (fn1!=".pmb") {
            FILE *f = openFile_inPath(fn1, pmbPath);
            if (f != nullptr) {
                struct stat statbuf1;
                fstat(fileno(f), &statbuf1);
                uint8_t buf1[statbuf1.st_size];
                size_t pmbRead = fread(buf1, 1, statbuf1.st_size, f);
                if (pmbRead != statbuf1.st_size) {
                    fprintf(stderr, "PMB not fully read: %zu instead of %lu.\n", pmbRead, statbuf1.st_size);
                }
                fclose(f);
                device->setPcmInstrumentParameters(buf1, statbuf1.st_size);
            } else {
                delete eupbuf;
                return nullptr;
            }
        }
    }
    
    if (eup_headerptr) memcpy((char*)eup_headerptr,(char*)(&eupHeader),sizeof(EUPHEAD));
    
    return eupbuf;
}

int EUPPlayer_ResetReload(){
    delete eup_buf;
    delete eup_dev;
    eup_dev = new EUP_TownsEmulator;
    eup_dev->rate(streamAudioRate); /* Hz */
    eup_player->outputDevice(eup_dev);
    eup_buf = EUPPlayer_readFile(eup_player, eup_dev, eup_filename,&eup_header);
    if (eup_buf == nullptr) {
        fprintf(stderr, "%s: read failed\n", eup_filename);
        std::fflush(stderr);
        return -1;
    }
    eup_player->startPlaying(eup_buf + 2048 + 6);
    return 0;
}




static char **sidtune_title,**sidtune_name,**sidtune_artist;
signed char *m_voice_buff[SOUND_MAXVOICES_BUFFER_FX];
signed int *m_voice_buff_accumul_temp[SOUND_MAXVOICES_BUFFER_FX];
unsigned char *m_voice_buff_accumul_temp_cnt[SOUND_MAXVOICES_BUFFER_FX];
int m_voice_buff_adjustement;
unsigned char m_voice_channel_mapping[256]; //used for timidity to map channels used to voices
unsigned char m_channel_voice_mapping[256]; //used for timidity to map channels used to voices

signed char *m_voice_buff_ana[SOUND_BUFFER_NB];
signed char *m_voice_buff_ana_cpy[SOUND_BUFFER_NB];
int m_voice_ChipID[SOUND_MAXVOICES_BUFFER_FX];
int64_t m_voice_current_ptr[SOUND_MAXVOICES_BUFFER_FX];
int64_t m_voice_prev_current_ptr[SOUND_MAXVOICES_BUFFER_FX];
int m_voice_systemColor[SOUND_VOICES_MAX_ACTIVE_CHIPS];
int m_voice_voiceColor[SOUND_MAXVOICES_BUFFER_FX];
signed char m_voice_current_system,m_voice_current_systemSub;
char m_voice_current_systemPairedOfs;
char mSIDSeekInProgress;
char m_voicesDataAvail;
char m_voicesStatus[SOUND_MAXMOD_CHANNELS];
int m_voicesForceOfs;
int m_voice_current_samplerate;
double m_voice_current_rateratio;
int m_voice_current_sample;
int m_genNumVoicesChannels;
int m_genMasterVol;
int64_t generic_mute_mask;

//SID
int sid_v4;
//


// GME M3U reader
#include "../../libs/libGME/gme/M3u_Playlist.h"
M3u_Playlist m3uReader;


/* file types */
static char gmetype[64];

static uint32 ao_type;
static volatile int mod_wantedcurrentsub,mNewModuleLength,moveToSubSong,moveToSubSongIndex;
static int sampleVolume,mInterruptShoudlRestart;
//static volatile int genCurOffset,genCurOffsetCnt;
static char str_name[1024];
static char *stil_info;//[MAX_STIL_DATA_LENGTH];
char *mod_message;//[8192+MAX_STIL_DATA_LENGTH];
static char mod_name[1024];
static char mod_filename[1024];
static NSString *mod_title;
static char archive_filename[1024];

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

#define DEFAULT_NSFNSFPLAY 0
#define DEFAULT_NSFGME 1

#define DEFAULT_GBSGBSPLAY 0
#define DEFAULT_GBSGME 1

#define DEFAULT_KSSLIBKSS 0
#define DEFAULT_KSSGME 1

static bool mdz_ShufflePlayMode;
static int mdz_IsArchive,mdz_ArchiveFilesCnt,mdz_currentArchiveIndex;
static int *mdz_ArchiveEntryPlayed;
static int *mdz_SubsongPlayed;

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

static float *v2m_sample_data_float;

static V2MPlayer *v2m_player;
/////////////////////////////////


extern "C" {
//PT3
#include "ayumi.h"
#include "pt3player.h"
#include "load_text.h"
unsigned char pt3_ayreg[14];
struct ayumi pt3_ay[10];
int pt3_numofchips=0;
int pt3_volume = 10000;
struct ay_data pt3_t;
short *pt3_sptr;
int pt3_lastsamp=0;
int pt3_isr_step=1;
int pt3_lastleng=0;
short *pt3_tmpbuf[10];
int pt3_frame[10];
int pt3_sample[10];
int pt3_fast=0;
int pt3_mute[10];
char* pt3_music_buf;
int pt3_music_size;

void pt3_update_ayumi_state(struct ayumi* ay, uint8_t* r, int ch) {
    func_getregs(r, ch);
    ayumi_set_tone(ay, 0, (r[1] << 8) | r[0]);
    ayumi_set_tone(ay, 1, (r[3] << 8) | r[2]);
    ayumi_set_tone(ay, 2, (r[5] << 8) | r[4]);
    ayumi_set_noise(ay, r[6]);
    ayumi_set_mixer(ay, 0, r[7] & 1, (r[7] >> 3) & 1, r[8] >> 4);
    ayumi_set_mixer(ay, 1, (r[7] >> 1) & 1, (r[7] >> 4) & 1, r[9] >> 4);
    ayumi_set_mixer(ay, 2, (r[7] >> 2) & 1, (r[7] >> 5) & 1, r[10] >> 4);
    ayumi_set_volume(ay, 0, r[8] & 0xf);
    ayumi_set_volume(ay, 1, r[9] & 0xf);
    ayumi_set_volume(ay, 2, r[10] & 0xf);
    ayumi_set_envelope(ay, (r[12] << 8) | r[11]);
    if (r[13] != 255) {
        ayumi_set_envelope_shape(ay, r[13]);
    }
    
    for (int i=0; i<9; i++) {
        if (i % 3 == 0 && pt3_mute[i] && ch == i / 3) {
            ayumi_set_volume(ay, 0, 0);
            ayumi_set_mixer(ay, 0, r[7] & 1, (r[7] >> 3) & 1, 0);
        }
        if (i % 3 == 1 && pt3_mute[i] && ch == i / 3) {
            ayumi_set_volume(ay, 1, 0);
            ayumi_set_mixer(ay, 1, (r[7] >> 1) & 1, (r[7] >> 4) & 1, 0);
        }
        if (i % 3 == 2 && pt3_mute[i] && ch == i / 3) {
            ayumi_set_volume(ay, 2, 0);
            ayumi_set_mixer(ay, 2, (r[7] >> 2) & 1, (r[7] >> 5) & 1, 0);
        }
    }
}


static int pt3_renday(short *snd, int leng, struct ayumi* ay, struct ay_data* t, int ch,int loop_allowed)
{
    pt3_isr_step = t->sample_rate / t->frame_rate;
    int ret=0;
    int isr_counter = 0;
    int16_t *buf = (short*)snd;
    int i = 0;
    if (pt3_fast) pt3_isr_step /= 4;
    pt3_lastleng=leng;
    while (leng>0)
    {
        if (1) {
            if (pt3_sample[ch] >= pt3_isr_step) {
                ret=func_play_tick(ch);
                if (buf) pt3_update_ayumi_state(ay,pt3_ayreg,ch);
                pt3_sample[ch] = 0;
                pt3_frame[ch]++;
            }
            if (buf) {
                ayumi_process(ay,ch);
                if (t->dc_filter_on) {
                    ayumi_remove_dc(ay);
                }
                
                buf[i] = (short) (ay->left * pt3_volume);
                buf[i+1] = (short) (ay->right * pt3_volume);
            }
        }
        
        pt3_sample[ch]++;
        leng-=4;
        i+=2;
        if (ret&&(!loop_allowed)) break;
    }
    return ret;
}

//KSS
#include "kssplay.h"
KSSPLAY *kssplay;
KSS *kss;

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
    0x04, 0x05, 0x08, 0x06, 0x18, 0x04, 0x04, 0x10+3,
    0x20, 0x04, 0x06, 0x06, 0x20, 0x20, 0x10, 0x20,
    0x04
};

const UINT8 vgmREALCHN_COUNT[CHIP_COUNT] =
{    0x04, 0x09, 0x06, 0x08, 0x10, 0x08, 0x06, 0x10,
    0x0E, 0x09, 0x06, 0x09, 0x12+5, 0x2A, 0x0C, 0x08,
    0x08, 0x02, 0x03, 0x04, 0x05, 0x1C, 0x01, 0x01,
    0x04, 0x05, 0x08, 0x06, 0x18, 0x04, 0x04, 0x10+3,
    0x20, 0x04, 0x06, 0x06, 0x20, 0x20, 0x10, 0x20,
    0x04
};

char vgmVRC7,vgm2610b;

static char vgm_voice_name[32];
char *vgmGetVoiceDetailName(UINT8 chipdId,UINT8 channel) {
    vgm_voice_name[0]=0;
    switch (chipdId) {
        case 0x00: //SN76496
            sprintf(vgm_voice_name,"PSG %d",channel+1);
            break;
        case 0x01: //YM2413
            sprintf(vgm_voice_name,"FM %d",channel+1);
            break;
        case 0x02: //YM2612
            sprintf(vgm_voice_name,"FM %d",channel+1);
            break;
        case 0x06: //YM2203
            if (channel<3) sprintf(vgm_voice_name,"FM %d",channel+1);
            else sprintf(vgm_voice_name,"SSG %d",channel-3+1);
            break;
        case 0x07: //YM2608
            if (channel<6) sprintf(vgm_voice_name,"FM %d",channel+1);
            else if (channel<6+6) sprintf(vgm_voice_name,"Rhythm %d",channel-6+1);
            else if (channel<6+6+1) sprintf(vgm_voice_name,"ADPCM");
            else sprintf(vgm_voice_name,"SSG %d",channel-6-6-1+1);
            break;
        case 0x08: //YM2610
            if (vgm2610b) {
                if (channel<6) sprintf(vgm_voice_name,"FM %d",channel+1);
                else if (channel<6+6) sprintf(vgm_voice_name,"ADPCM-A %d",channel-6+1);
                else if (channel<6+6+1) sprintf(vgm_voice_name,"ADPCM-B");
                else sprintf(vgm_voice_name,"SSG %d",channel-6-6-1+1);
            } else {
                if (channel<4) sprintf(vgm_voice_name,"FM %d",channel+1);
                else if (channel<4+6) sprintf(vgm_voice_name,"ADPCM-A %d",channel-4+1);
                else if (channel<4+6+1) sprintf(vgm_voice_name,"ADPCM-B");
                else sprintf(vgm_voice_name,"SSG %d",channel-4-6-1+1);
            }
            break;
        case 0x27: //C352
            sprintf(vgm_voice_name,"PCM %d",channel+1);
            break;
        case 0x28: //IREM GA20
            sprintf(vgm_voice_name,"PCM %d",channel+1);
            break;
            
    }
    return vgm_voice_name;
}
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
#import "../libs/libvgmstream/vgmstream-master/src/vgmstream.h"
#import "../libs/libvgmstream/vgmstream-master/src/base/plugins.h"

static VGMSTREAM* vgmStream = NULL;
static STREAMFILE* vgmFile = NULL;
bool optVGMSTREAM_loopmode = false;
int optVGMSTREAM_loop_count = 2.0f;
int optVGMSTREAM_fadeouttime=5;
int optVGMSTREAM_resampleQuality=1;

static bool mVGMSTREAM_force_loop;
static volatile int64_t mVGMSTREAM_total_samples,mVGMSTREAM_decode_pos_samples,mVGMSTREAM_totalinternal_samples;

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
//ATARIAUDIO
SndhFile atariSndh;
uint32_t atariSndh_hash;
uint32_t *atariWaveData;
//STSOUND
static YMMUSIC *ymMusic;
//SC68
//static api68_t *sc68;
static sc68_t *sc68;

extern "C" {

int getNumberTraceStreams() {
    int track=sc68_cntl(sc68,SC68_GET_TRACK);
    int num =  sc68_used_channels(sc68, track);
    return num;
}

short **getScopeBuffers() {
    return NULL;
}

}

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
static double gme_eq_treble_orig,gme_eq_bass_orig;



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
static int optNSFPLAYDefaultLength=SONG_DEFAULT_LENGTH;

bool tim_force_soundfont;
char tim_force_soundfont_path[1024];
char tim_config_file_path[1024];
int mdz_tim_only_precompute;

static char tim_filepath[1024];
static char tim_filepath_orgfile[1024];
static char tim_filepath_filename[256];
static volatile int tim_finished;
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
int64_t seek_tgtSamples;

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

int64_t src_callback_hc(void *cb_data, float **data);
int64_t src_callback_vgmstream(void *cb_data, float **data);

////////////////////

extern "C" {
int MDXshoudlReset;
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
    
    int diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
    if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
    
    while ((buffer_ana_flag[buffer_ana_gen_ofs])||(diff_ana_ofs>=SOUND_BUFFER_NB/2)) {
        [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
        if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
            g_playing=0;
            return 0;
        }
        diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
        if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
    }
    
    int to_fill=SOUND_BUFFER_SIZE_SAMPLE*2*2-buffer_ana_subofs;
    if (nbytes<to_fill) {
        memcpy( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)buf,nbytes);
        
        buffer_ana_subofs+=nbytes;
        
    } else {
        memcpy((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)buf,to_fill);
        
        nbytes-=to_fill;
        buffer_ana_subofs=0;
        
        
        for (int j=0;j<m_genNumVoicesChannels;j++) {
            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE-1)];
                m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE-1)]=0;
            }
            m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;//m_voice_current_ptr[j];
            if ((m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE) m_voice_prev_current_ptr[j]-=(SOUND_BUFFER_SIZE_SAMPLE)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
            
            memset(m_voice_buff_accumul_temp[j],0,SOUND_BUFFER_SIZE_SAMPLE*sizeof(int)*2);
            memset(m_voice_buff_accumul_temp_cnt[j],0,SOUND_BUFFER_SIZE_SAMPLE*2);            
        }
        
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

//NSFPLAY
#include "nsfplay.h"
xgm::NSFPlayer *nsfPlayer;
xgm::NSFPlayerConfig *nsfPlayerConfig;
xgm::NSF *nsfData;
//NSFPLAY

#define MODIZ_MAX_CHIPS 8 //has to be the max / each lib
#define MODIZ_CHIP_NAME_MAX_CHAR 32
#define MODIZ_VOICE_NAME_MAX_CHAR 32

#define NES_MAX_CHIPS 8
enum {
    NES_OFF=0,
    NES_APU,
    NES_FDS,
    NES_FME7,
    NES_MMC5,
    NES_N106,
    NES_VRC6,
    NES_VRC7
};
#define KSS_MAX_CHIPS 5
enum {
    KSS_OFF=0,
    KSS_Y8950,
    KSS_YM2413,
    KSS_SN76489,
    KSS_PSG,
    KSS_SCC
};

char modizChipsetType[MODIZ_MAX_CHIPS];
char modizChipsetStartVoice[MODIZ_MAX_CHIPS];
char modizChipsetVoicesCount[MODIZ_MAX_CHIPS];
char modizChipsetName[MODIZ_MAX_CHIPS][MODIZ_CHIP_NAME_MAX_CHAR];
char modizVoicesName[SOUND_MAXVOICES_BUFFER_FX][MODIZ_VOICE_NAME_MAX_CHAR];
char modizChipsetCount;




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
#if 0
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
#endif
/*************************************************/
/* Audio property listener                       */
/*************************************************/
#if 0
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
#endif



@implementation ModizMusicPlayer
@synthesize artist,album;
@synthesize extractPendingCancel;
@synthesize detailViewControllerIphone;
@synthesize mod_subsongs,mod_currentsub,mod_minsub,mod_maxsub,mLoopMode;
@synthesize mod_currentfile,mod_currentext;
@synthesize mPlayType;
@synthesize mp_datasize;
@synthesize optForceMono;
//Adplug stuff
@synthesize adPlugPlayer,adplugDB;
@synthesize opl;
@synthesize opl_towrite;
@synthesize mADPLUGopltype,mADPLUGstereosurround,mADPLUGPriorityOverMod;
//GME stuff
@synthesize gme_emu;
//SID
//VGMPLAY stuff
//Modplug stuff
@synthesize mp_settings;
@synthesize ompt_mod;
@synthesize mp_data;
@synthesize mVolume;
@synthesize mLoadModuleStatus;
@synthesize numChannels,numPatterns,numSamples,numInstr,mPatternDataAvail;
@synthesize m_voicesDataAvail;
@synthesize genRow,genPattern,playRow,playPattern,nextPattern,prevPattern;
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
-(sc68_t*)setupSc68 {
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *path = [bundle resourcePath];
    NSMutableString *argData = [NSMutableString stringWithString:@"--sc68-user-path="];
    [argData appendString:path];
    //[argData appendString:@"/Replay"];
    
    char *t = (char *)[argData UTF8String];
    char *args[] = { (char*)"sc68", t, NULL };
    
    sc68_init_t init68;
    
    init68.argc = 2;
    init68.argv = args;
    
    memset(&init68,0,sizeof(init68));
    init68.argc = 2;
    init68.argv = args;
    init68.debug_set_mask = 0;
    init68.debug_clr_mask = 0;
    init68.msg_handler = NULL;//(sc68_msg_t) msgfct;
    init68.flags.no_load_config = 0;
    init68.flags.no_save_config = 1;
    
    sc68_t* api=NULL;
    
    if (sc68_init(&init68)) {
        //issues
        return NULL;
    } else {
        // Create an emulator instance.
        // You should consider using sc68_create_t struct.
        api = sc68_create(0);
    }
    //
    return api;
}

-(void) handleInterruption:(NSNotification *)notification {
    AVAudioSessionInterruptionType interruptionType = (AVAudioSessionInterruptionType)[[[notification userInfo] objectForKey:AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
    if (AVAudioSessionInterruptionTypeBegan == interruptionType) {
        
        mInterruptShoudlRestart=0;
        if ([self isPlaying] && (self.bGlobalAudioPause==0)) {
            [self Pause:YES];
            mInterruptShoudlRestart=1;
        }
        
    }
    else if (AVAudioSessionInterruptionTypeEnded == interruptionType) {
        // if the interruption was removed, and the app had been playing, resume playback
        if (mInterruptShoudlRestart) {
            //check if headphone is used?
            /*CFStringRef newRoute;
             UInt32 size = sizeof(CFStringRef);
             AudioSessionGetProperty(kAudioSessionProperty_AudioRoute,&size,&newRoute);
             if (newRoute) {
             if (CFStringCompare(newRoute,CFSTR("Headphone"),NULL)==kCFCompareEqualTo) {  //
             mInterruptShoudlRestart=0;
             }
             }*/
            
            if (mInterruptShoudlRestart) [self Pause:NO];
            mInterruptShoudlRestart=0;
        }
    }
}

- (void)handleRouteChange:(NSNotification *)notification
{
    UInt8 reasonValue = [[notification.userInfo valueForKey:AVAudioSessionRouteChangeReasonKey] intValue];
    AVAudioSessionRouteDescription *routeDescription = [notification.userInfo valueForKey:AVAudioSessionRouteChangePreviousRouteKey];
    
    //NSLog(@"Route change:");
    switch (reasonValue) {
        case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
            //NSLog(@"     NewDeviceAvailable");
            break;
        case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
            //NSLog(@"     OldDeviceUnavailable");
            [self Pause:YES];
            break;
        case AVAudioSessionRouteChangeReasonCategoryChange:
            //NSLog(@"     CategoryChange");
            //NSLog(@" New Category: %@", [[AVAudioSession sharedInstance] category]);
            break;
        case AVAudioSessionRouteChangeReasonOverride:
            //NSLog(@"     Override");
            break;
        case AVAudioSessionRouteChangeReasonWakeFromSleep:
            //NSLog(@"     WakeFromSleep");
            break;
        case AVAudioSessionRouteChangeReasonNoSuitableRouteForCategory:
            //NSLog(@"     NoSuitableRouteForCategory");
            break;
        default:
            //NSLog(@"     ReasonUnknown");
            break;
    }
    
    //NSLog(@"Previous route:\n");
    //NSLog(@"%@", routeDescription);
}



-(id) initMusicPlayer {
    mFileMngr=[[NSFileManager alloc] init];
    
    PLAYBACK_FREQ=DEFAULT_PLAYBACK_FREQ;
    /*switch (settings[GLOB_PlaybackFrequency].detail.mdz_switch.switch_value) {
     case 0: //44,1Khz
     PLAYBACK_FREQ=44100;
     break;
     case 1: //48Khz
     PLAYBACK_FREQ=48000;
     break;
     }*/
    
    if ((self = [super init])) {
        NSError *audioSessionError = nil;
        AVAudioSession *session = [AVAudioSession sharedInstance];
        
        [session setCategory:AVAudioSessionCategoryPlayback error:&audioSessionError];
        if (audioSessionError) {
            NSLog(@"Audio session setCategory Error %ld, %@",
                  (long)audioSessionError.code, audioSessionError.localizedDescription);
        }
        
        NSTimeInterval bufferDuration = SOUND_BUFFER_SIZE_SAMPLE*2.0f/PLAYBACK_FREQ;
        [session setPreferredIOBufferDuration:bufferDuration error:&audioSessionError];
        if (audioSessionError) {
            NSLog(@"Error %ld, %@",
                  (long)audioSessionError.code, audioSessionError.localizedDescription);
        }
        
        double sampleRate = PLAYBACK_FREQ;
        [session setPreferredSampleRate:sampleRate error:&audioSessionError];
        if (audioSessionError) {
            NSLog(@"Error %ld, %@",
                  (long)audioSessionError.code, audioSessionError.localizedDescription);
        }
        
        // Register for Route Change notifications
        [[NSNotificationCenter defaultCenter] addObserver: self
                                                 selector: @selector(handleRouteChange:)
                                                     name: AVAudioSessionRouteChangeNotification
                                                   object: session];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(handleInterruption:)
                                                     name:AVAudioSessionInterruptionNotification object:session];
        
        [session setActive:YES error:&audioSessionError];
        
        // Get current values
        sampleRate = session.sampleRate;
        bufferDuration = session.IOBufferDuration;
        
        [session setPreferredSampleRate:sampleRate error:&audioSessionError];
        if (audioSessionError) {
            NSLog(@"Error %ld, %@",
                  (long)audioSessionError.code, audioSessionError.localizedDescription);
        }
        
        buffer_ana_flag=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        buffer_ana=(short int**)malloc(SOUND_BUFFER_NB*sizeof(short int *));
        buffer_ana_cpy=(short int**)malloc(SOUND_BUFFER_NB*sizeof(short int *));
        buffer_ana_subofs=0;
        for (int i=0;i<SOUND_BUFFER_NB;i++) {
            buffer_ana[i]=(short int *)malloc(SOUND_BUFFER_SIZE_SAMPLE*2*2);
            buffer_ana_cpy[i]=(short int *)malloc(SOUND_BUFFER_SIZE_SAMPLE*2*2);
            buffer_ana_flag[i]=0;
        }
        
        for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
            m_voice_current_ptr[i]=0;
            m_voice_prev_current_ptr[i]=0;
            m_voice_buff[i]=(signed char*)calloc(1,SOUND_BUFFER_SIZE_SAMPLE*4*4);
            
            m_voice_buff_accumul_temp[i]=(signed int*)calloc(1,SOUND_BUFFER_SIZE_SAMPLE*sizeof(int)*2);
            m_voice_buff_accumul_temp_cnt[i]=(unsigned char*)calloc(1,SOUND_BUFFER_SIZE_SAMPLE*2);
        }
        
        
        for (int j=0;j<SOUND_BUFFER_NB;j++) {
            m_voice_buff_ana[j]=(signed char*)calloc(1,SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
            m_voice_buff_ana_cpy[j]=(signed char*)calloc(1,SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
        }
        memset(m_voice_ChipID,0,sizeof(int)*SOUND_MAXVOICES_BUFFER_FX);
        
        /*for (int i=0;i<SOUND_VOICES_MAX_ACTIVE_CHIPS;i++) {
         CGFloat hue=(240.0f/360.0f)+i*(70.0f/360.0f);
         while (hue>1.0) hue-=1.0f;
         while (hue<0.0) hue+=1.0f;
         UIColor *col=[UIColor colorWithHue:hue saturation:0.5f brightness:1.0f alpha:1.0f];
         CGFloat red,green,blue;
         //voicesChipColHalf[i]=[UIColor colorWithHue:0.8f-i*0.4f/(float)SOUND_VOICES_MAX_ACTIVE_CHIPS saturation:0.7f brightness:0.4f alpha:1.0f];
         [col getRed:&red green:&green blue:&blue alpha:NULL];
         m_voice_systemColor[i]=(((int)(red*255))<<16)|(((int)(green*255))<<8)|(((int)(blue*255))<<0);
         printf("col %d: %02X%02X%02\n",i,(int)(red*255),(int)(green*255),(int)(blue*255));
         }*/
        
        m_genNumVoicesChannels=0;
        [self optUpdateSystemColor];
                
        //Global
        bGlobalShouldEnd=0;
        bGlobalSoundGenInProgress=0;
        optForceMono=0;
        mod_subsongs=0;
        mod_message_updated=0;
        bGlobalAudioPause=0;
        bGlobalIsPlaying=0;
        mVolume=1.0f;
        mLoadModuleStatus=0;
        mLoopMode=0;
        mCurrentSamples=0;
        m_voice_current_sample=0;
        
        for (int i=0;i<SOUND_MAXMOD_CHANNELS;i++) m_voicesStatus[i]=1;
        
        stil_info=(char*)malloc(MAX_STIL_DATA_LENGTH);
        mod_message=(char*)malloc(MAX_STIL_DATA_LENGTH*2);
        
        
        mPanning=0;
        mPanningValue=0; //0% full stereo
        
        mdz_SubsongPlayed=NULL;
        mdz_ArchiveEntryPlayed=NULL;
        
        mdz_ArchiveFilesList=NULL;
        //        mdz_ArchiveFilesListAlias=NULL;
        mdz_ArchiveFilesCnt=0;
        mdz_IsArchive=0;
        mdz_ShufflePlayMode=false;
        mdz_currentArchiveIndex=0;
        //Timidity
        
        
        //OMPT specific
        genPattern=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        genRow=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        //genOffset=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        playPattern=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        playRow=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        prevPattern=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        nextPattern=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        genPrevPattern=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        genNextPattern=(int*)malloc(SOUND_BUFFER_NB*sizeof(int));
        
        genVolData=(unsigned char*)malloc(SOUND_BUFFER_NB*SOUND_MAXMOD_CHANNELS);
        playVolData=(unsigned char*)malloc(SOUND_BUFFER_NB*SOUND_MAXMOD_CHANNELS);
        
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
        //
        // ADPLUG specific
        mADPLUGopltype=0;
        mADPLUGstereosurround=1;
        mADPLUGPriorityOverMod=0;
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
        
        //KSS specific
        kssplay=NULL;
        kss=NULL;
        
        //HVL specific
        mHVLinit=0;
        
        //WEBSID
        websid_scope_buffers=NULL;
        
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
        // PT3
        for (int ch=0; ch<10; ch++) {
            pt3_tmpbuf[ch] = (short*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4);
        }
        
        //ASAP
        asap = ASAP_New();
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
        
        //NSProgress for archive extract
        extractProgress = nil;
        
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
    free(prevPattern);
    free(nextPattern);
    free(genPrevPattern);
    free(genNextPattern);
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
    if (err) return 0;
    /* ... and its associated buffers */
    mBuffers = (AudioQueueBufferRef*)malloc( sizeof(AudioQueueBufferRef) * SOUND_BUFFER_NB );
    for (int i=0; i<SOUND_BUFFER_NB; i++) {
        AudioQueueBufferRef mBuffer;
        err = AudioQueueAllocateBuffer( mAudioQueue, SOUND_BUFFER_SIZE_SAMPLE*2*2, &mBuffer );
        if (err) return 0;
        mBuffers[i]=mBuffer;
    }
    /* Set initial playback volume */
    err = AudioQueueSetParameter( mAudioQueue, kAudioQueueParam_Volume, mVolume );
    if (err) return 0;
    
    
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
        playRow[i]=playPattern[i]=prevPattern[i]=nextPattern[i]=genRow[i]=genPattern[i]=genPrevPattern[i]=genNextPattern[i]=0;
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

-(BOOL) iPhoneDrv_PlayRestart {
    UInt32 err;
    UInt32 i;
    
    mQueueIsBeingStopped = TRUE;
    AudioQueueStop( mAudioQueue, TRUE );
    AudioQueueReset( mAudioQueue );
    mQueueIsBeingStopped = FALSE;
    
    buffer_ana_gen_ofs=buffer_ana_play_ofs=0;
    buffer_ana_subofs=0;
    //    genCurOffset=0;genCurOffsetCnt=0;
    for (int i=0;i<SOUND_BUFFER_NB;i++) {
        buffer_ana_flag[i]=0;
        playRow[i]=playPattern[i]=prevPattern[i]=nextPattern[i]=genRow[i]=genPattern[i]=genPrevPattern[i]=genNextPattern[i]=0;
        memset(playVolData+i*SOUND_MAXMOD_CHANNELS,0,SOUND_MAXMOD_CHANNELS);
        memset(genVolData+i*SOUND_MAXMOD_CHANNELS,0,SOUND_MAXMOD_CHANNELS);
        memset(buffer_ana[i],0,SOUND_BUFFER_SIZE_SAMPLE*2*2);
        memset(buffer_ana_cpy[i],0,SOUND_BUFFER_SIZE_SAMPLE*2*2);
        memset(tim_notes[i],0,DEFAULT_VOICES*4);
        memset(tim_notes_cpy[i],0,DEFAULT_VOICES*4);
        memset(m_voice_buff_ana[i],0,SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
        memset(m_voice_buff_ana_cpy[i],0,SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
        tim_voicenb[i]=tim_voicenb_cpy[i]=0;
    }
    
    sampleVolume=0;
    for (i=0; i<SOUND_BUFFER_NB; i++) {
        memset(mBuffers[i]->mAudioData,0,SOUND_BUFFER_SIZE_SAMPLE*2*2);
        mBuffers[i]->mAudioDataByteSize = SOUND_BUFFER_SIZE_SAMPLE*2*2;
        AudioQueueEnqueueBuffer( mAudioQueue, mBuffers[i], 0, NULL);
        //        [self iPhoneDrv_FillAudioBuffer:mBuffers[i]];
        
    }
    bGlobalAudioPause=0;
    /* Start the Audio Queue! */
    //AudioQueuePrime( mAudioQueue, 0,NULL );
    err = AudioQueueStart( mAudioQueue, NULL );
    
    return 1;
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
                //iCurrentTime=mNeedSeekTime;
                mNeedSeek=0;
                bGlobalSeekProgress=0;
            }
            
            //take into account audio latency for output device
            
            int tgt_ofs=(buffer_ana_play_ofs-[self getLatencyInBuffer:settings[GLOB_AudioLatency].detail.mdz_slider.slider_value])%SOUND_BUFFER_NB;//-;
            if (tgt_ofs<0) tgt_ofs+=SOUND_BUFFER_NB;
            
            if (mPatternDataAvail) {//Modplug
                playPattern[buffer_ana_play_ofs]=genPattern[tgt_ofs];
                playRow[buffer_ana_play_ofs]=genRow[tgt_ofs];
                prevPattern[buffer_ana_play_ofs]=genPrevPattern[tgt_ofs];
                nextPattern[buffer_ana_play_ofs]=genNextPattern[tgt_ofs];
                memcpy(playVolData+buffer_ana_play_ofs*SOUND_MAXMOD_CHANNELS,genVolData+tgt_ofs*SOUND_MAXMOD_CHANNELS,SOUND_MAXMOD_CHANNELS);
                //				playOffset[buffer_ana_play_ofs]=genOffset[buffer_ana_play_ofs];
            }
            if (mPlayType==MMP_TIMIDITY) {//Timidity
                memcpy(tim_notes_cpy[buffer_ana_play_ofs],tim_notes[tgt_ofs],DEFAULT_VOICES*4);
                tim_voicenb_cpy[buffer_ana_play_ofs]=tim_voicenb[tgt_ofs];
            }
            
            iCurrentTime+=1000.0f*SOUND_BUFFER_SIZE_SAMPLE/PLAYBACK_FREQ;
            
            if (mPlayType==MMP_SC68) {//SC68
                iCurrentTime=sc68_cntl(sc68,SC68_GET_POS);
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

            memcpy(buffer_ana_cpy[buffer_ana_play_ofs],buffer_ana[tgt_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*2);
            
            memcpy(m_voice_buff_ana_cpy[buffer_ana_play_ofs],m_voice_buff_ana[tgt_ofs],SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
            
            if (bGlobalEndReached && buffer_ana_flag[buffer_ana_play_ofs]&4) { //end reached
                //iCurrentTime=0;
                bGlobalAudioPause=2;
                bGlobalEndReached=0;
                for (int ii=0;ii<SOUND_BUFFER_NB;ii++) buffer_ana_flag[ii]&=~0x4;
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
        //buffer_ana_play_ofs++;
        //if (buffer_ana_play_ofs==SOUND_BUFFER_NB) buffer_ana_play_ofs=0;
    }
    return 0;
}

-(int) getLatencyInBuffer:(double)latency {
    if (latency==0) {
        AVAudioSession *session=[AVAudioSession sharedInstance];
        latency=session.outputLatency;
    }
    double buffer_length=(double)(SOUND_BUFFER_SIZE_SAMPLE)/(double)(PLAYBACK_FREQ);

    int buffer_to_compensate=int(latency/buffer_length);
    
    //NSLog(@"buffer to compensate: %d",buffer_to_compensate);
    
    if (buffer_to_compensate>=SOUND_BUFFER_NB/2) buffer_to_compensate=SOUND_BUFFER_NB/2; //take some contingency / some fx reading ahead buffer, such as oscilloMultiple
    //if (buffer_to_compensate<2) buffer_to_compensate=2; //take some contingency / avoid having FX using voice_data that can be updated in the middle of the FX
    
    //NSLog(@"buffer to compensate adjusted: %d",buffer_to_compensate);
    
    return buffer_to_compensate;
}

-(int) getCurrentPlayedBufferIdx {
    /*int adj=-1-[self getLatencyInBuffer];
    adj=-1;*/
    
    int idx=(buffer_ana_play_ofs+1);//-[self getLatencyInBuffer]);
    while (idx<0) idx+=SOUND_BUFFER_NB;
    return idx%SOUND_BUFFER_NB;
}

-(int) getCurrentGenBufferIdx {
    int idx=(buffer_ana_play_ofs-1);
    if (idx<0) idx+=SOUND_BUFFER_NB;
    return idx%SOUND_BUFFER_NB;
}

void mdx_update(unsigned char *data,int len,int end_reached) {
    if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
        mdx_stop();
        return;
    }
    
    int diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
    if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
    
    while ((buffer_ana_flag[buffer_ana_gen_ofs])||(diff_ana_ofs>=SOUND_BUFFER_NB/2)) {
        [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
        if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
            mdx_stop();
            return;
        }
        diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
        if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
    }
    int to_fill=SOUND_BUFFER_SIZE_SAMPLE*2*2-buffer_ana_subofs;
    if (len<to_fill) {
        memcpy( (char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)data,len);
        
        for (int j=0;j<m_genNumVoicesChannels;j++) {
            for (int i=buffer_ana_subofs/4;i<(buffer_ana_subofs+len)/4;i++) {
                m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)];
                //m_voice_buff[j][(i+m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*2-1)]=0;
            }
            m_voice_prev_current_ptr[j]+=(len>>2)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;//m_voice_current_ptr[j];
            if ((m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=(SOUND_BUFFER_SIZE_SAMPLE*2*4)) m_voice_prev_current_ptr[j]-=(SOUND_BUFFER_SIZE_SAMPLE*2*4)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
        }
        
        buffer_ana_subofs+=len;
    } else {
        memcpy((char*)(buffer_ana[buffer_ana_gen_ofs])+buffer_ana_subofs,(char*)data,to_fill);
        
        for (int j=0;j<m_genNumVoicesChannels;j++) {
            for (int i=buffer_ana_subofs/4;i<(buffer_ana_subofs+to_fill)/4;i++) {
                m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)];
                //m_voice_buff[j][(i+m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*2-1)]=0;
            }
            m_voice_prev_current_ptr[j]+=(to_fill>>2)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;//m_voice_current_ptr[j];
            if ((m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=(SOUND_BUFFER_SIZE_SAMPLE*2*4)) m_voice_prev_current_ptr[j]-=(SOUND_BUFFER_SIZE_SAMPLE*2*4)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
        }
        
        
        len-=to_fill;
        buffer_ana_subofs=0;
        
        buffer_ana_flag[buffer_ana_gen_ofs]=1;
        
        
        
        if ((mNeedSeek==2)&&(seek_needed==-1)) {
            mNeedSeek=3;
            buffer_ana_flag[buffer_ana_gen_ofs]|=2;
        }
        
        if (end_reached) {
            buffer_ana_flag[buffer_ana_gen_ofs]|=4;
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
    
    if (mNeedSeek==1) {
        seek_needed=mNeedSeekTime;
        bGlobalSeekProgress=-1;
        mNeedSeek=2;
        
        if (seek_needed<decode_pos_ms) {
            MDXshoudlReset=1;
            decode_pos_ms=0;
        }
    }
    
    decode_pos_ms += ((SOUND_BUFFER_SIZE_SAMPLE) * 1000)/(float)PLAYBACK_FREQ;
    if (seek_needed!=-1) {
        if (decode_pos_ms>=seek_needed) {
            seek_needed=-1;
        }
    }
    
}

extern "C" {
    
    static void gbs_stepcallback(struct gbs *gbs, cycles_t cycles, const struct gbs_channel_status chan[], void *priv) {
        sound_step(cycles, chan);
    }
    
    long gbs_mdz_nextsubsong(struct gbs* const gbs, void *priv) {
        return false;
    }

    
    void gbs_update(unsigned char* pSound,int lBytes) {
        if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
            g_playing=0;
            return;
        }
        
        if (mNeedSeek==2) {
            if (seek_tgtSamples>mCurrentSamples) {
                mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                return;
            } else {
                seek_needed=-1;
            }
        }
        
        int diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
        if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
        
        while ((buffer_ana_flag[buffer_ana_gen_ofs])||(diff_ana_ofs>=SOUND_BUFFER_NB/2)) {
            [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
            if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
                g_playing=0;
                return;
            }
            diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
            if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
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
            for (int j=0;j<4;j++) {
//                int val=m_voice_buff[j][(-1+(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)];
//                
//                for (int i=(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT);i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
//                    m_voice_buff[j][(i+(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)]=val;
//                }
                
                for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                    m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][i];
                }
            }
            
            
            mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
            iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
            
            
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
    }
}


void gsf_update(unsigned char* pSound,int lBytes) {
    if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
        g_playing=0;
        return;
    }
    
    int diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
    if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
    
    while ((buffer_ana_flag[buffer_ana_gen_ofs])||(diff_ana_ofs>=SOUND_BUFFER_NB/2)) {
        [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
        if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
            g_playing=0;
            return;
        }
        diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
        if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
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
            for (int j=0;j<6;j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE-1)];
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
    
    //tim_init((char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"modizer.app/timidity"] UTF8String]);
    
    if (tim_force_soundfont) {
        //NSLog(@"forcing with: %s",tim_force_soundfont_path);
        tim_init(tim_force_soundfont_path);
        snprintf(tim_config_file_path,sizeof(tim_config_file_path),"%s/timidity.cfg",tim_force_soundfont_path);
    } else {
        //timidity
        tim_init((char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"modizer.app/timidity"] UTF8String]);
        tim_init((char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"] UTF8String]);
        snprintf(tim_config_file_path,sizeof(tim_config_file_path),"%s",(char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/timidity.cfg"] UTF8String]);
    }
    
    mdz_tim_only_precompute=0;
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
        int diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
        if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
        
        while ((buffer_ana_flag[buffer_ana_gen_ofs])||(diff_ana_ofs>=SOUND_BUFFER_NB/2)) {
            [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
            if (bGlobalShouldEnd||(!bGlobalIsPlaying)) {
                return 0;
            }
            diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
            if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
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
            
            //copy voice data for oscillo view
            //NSLog(@"uade voice: %d",m_voice_current_ptr[0]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
            
            for (int j=0;j<4;j++) {
                for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*4*4-1)];
                    //m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)]=0;
                }
                m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                if (m_voice_prev_current_ptr[j]>=(SOUND_BUFFER_SIZE_SAMPLE*4*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)) m_voice_prev_current_ptr[j]-=(SOUND_BUFFER_SIZE_SAMPLE*4*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
            }
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
                    }  else us->cur_subsong++;
                    
                    if (us->cur_subsong > us->max_subsong) uade_song_end_trigger = 1;
                    else {
                        subsong_end = 0;
                        subsong_bytes = 0;
                        uade_change_subsong(state);
                        mod_currentsub=us->cur_subsong;
                        
                        iCurrentTime=0;
                        mNeedSeek=0;bGlobalSeekProgress=0;
                        iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
                        
                        //Loop
                        if (mLoopMode==1) iModuleLength=-1;
                        mod_message_updated=1;
                        
                        if(moveToSubSong) {
                            mod_wantedcurrentsub=-1;
                            what_was_left=0;
                            tailbytes=0;
                            
                            [self iPhoneDrv_PlayRestart];
                            
                            skip_first=1;
                        }
                    }
                    
                    
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
            if (moveToSubSong) {
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
            
            int prev_mod_subsongs=mod_subsongs;
            
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
                        if (mSingleSubMode==0) {
                            if (mod_currentsub<mod_maxsub) [self playNextSub]; //TODO: check if it works
                        }
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
                    if (strcmp(reason,"module check failed")==0) mLoadModuleStatus=-1;
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
            
            if ((prev_mod_subsongs!=mod_subsongs)&&(mod_subsongs>1)) {
                [self initSubSongPlayed];
                //printf("initsubsong / %d subsongs\n",mod_subsongs);
                if (mdz_ShufflePlayMode) {
                    [self playNextSub];
                }
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
                
                for (int jj=0;jj<m_genNumVoicesChannels;jj++) {
                    m_voice_buff[jj][i]=(int)(m_voice_buff[jj][i])*vol/hc_fadeLength;
                }
                
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
    
    if (nbSamplesToRender<=0) {
        memset(vgm_sample_data,0,SOUND_BUFFER_SIZE_SAMPLE*2*2);
        src_short_to_float_array (vgm_sample_data, vgm_sample_data_float,SOUND_BUFFER_SIZE_SAMPLE*2);
        *data=vgm_sample_data_float;
        return SOUND_BUFFER_SIZE_SAMPLE;
    }
    
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
        int donotstop=0;
        
        if ([[NSThread currentThread] respondsToSelector:@selector(setThreadPriority)]) [[NSThread currentThread] setThreadPriority:SND_THREAD_PRIO];
        
        //int cptyo=0;
        while (1) {
            
//            if (cptyo++==1024)  {
//                cptyo=0;
//                AVAudioSession *session=[AVAudioSession sharedInstance];
//                NSLog(@"latency: %f / %d",session.outputLatency*1000,[self getLatencyInBuffer:0]);
//                
//            }
            [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
            if (bGlobalIsPlaying) {
                
                int diff_ana_ofs=(buffer_ana_gen_ofs-buffer_ana_play_ofs);
                if (diff_ana_ofs<0) diff_ana_ofs+=SOUND_BUFFER_NB;
                                                                
                bGlobalSoundGenInProgress=1;
                if ( !bGlobalEndReached && mPlayType) {
                    int nbBytes=0;
                    
                    if (mPlayType==MMP_TIMIDITY) { //Special case : Timidity
                        int counter=0;
                        intr = 0;
                        tim_finished=0;
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
                        tim_finished=1;
                    } else if (mPlayType==MMP_GSF) {  //Special case : GSF
                        int counter=0;
                        [NSThread sleepForTimeInterval:0.1];  //TODO : check why it crashes in "release" target without this...
                        intr = 0;
                        tim_finished=0;
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
                        tim_finished=1;
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
                        while (1) {
                            mdx_play(mdx,pdx);
                            if (!MDXshoudlReset) break;
                            MDXshoudlReset=0;
                            decode_pos_ms=0;
                        }
                        
                        
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
                    } else if (mPlayType==MMP_GBS) {  //Special case : GBS
                        int counter=0;
                        int quit=0;
                        intr = 0;
                        tim_finished=0;
                        while (g_playing&&(!quit)) {
                            if (mNeedSeek==1) {
                                seek_needed=mNeedSeekTime;
                                seek_tgtSamples=mNeedSeekTime*PLAYBACK_FREQ/1000;
                                bGlobalSeekProgress=-1;
                                mNeedSeek=2;
                                
                                if (seek_tgtSamples<mCurrentSamples) {
                                    if (m3uReader.size()) gbs_init(gbs,m3uReader[mod_currentsub].track);
                                    else  gbs_init(gbs,mod_currentsub);
                                    mCurrentSamples=0;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                }
                            }
                            if (!step_emulation(gbs)) {
                                quit = 1;
                                //break;
                            }
                            if (quit/*||( (iModuleLength>0)&&(iCurrentTime>iModuleLength))*/) {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) {
                                        //stop
                                    } else {
                                        moveToSubSong=2;
                                        quit = 0;
                                    }
                                }
                            }
                            if (moveToSubSong) {
                                mod_currentsub=moveToSubSongIndex;
                                
                                if (m3uReader.size()) {
                                    gbs_init(gbs,m3uReader[mod_currentsub].track);
                                    iModuleLength=m3uReader[mod_currentsub].length;
                                    if (iModuleLength<=0) {
                                        const struct gbs_status *status = gbs_get_status(gbs);
                                        iModuleLength=(status->subsong_len)*1000/1024;
                                    }
                                    if (m3uReader[mod_currentsub].name) mod_title=[NSString stringWithUTF8String:m3uReader[mod_currentsub-mod_minsub].name];
                                    gbs_configure(gbs,m3uReader[mod_currentsub].track,iModuleLength/1000, 
                                                  settings[GBSPLAY_SilenceTimeout].detail.mdz_slider.slider_value,1,
                                                  settings[GBSPLAY_Fadeouttime].detail.mdz_slider.slider_value); //silence timeout, subsong gap, fadeout);
                                } else {
                                    gbs_init(gbs,mod_currentsub);
                                    const struct gbs_status *status = gbs_get_status(gbs);
                                    iModuleLength=(status->subsong_len)*1000/1024;
                                    mod_title=[NSString stringWithUTF8String:status->songtitle];
                                    gbs_configure(gbs,mod_currentsub,iModuleLength/1000,
                                                  settings[GBSPLAY_SilenceTimeout].detail.mdz_slider.slider_value,1,
                                                  settings[GBSPLAY_Fadeouttime].detail.mdz_slider.slider_value); //silence timeout, subsong gap, fadeout);
                                }
                                const struct gbs_metadata *metadata=gbs_get_metadata(gbs);
                                snprintf(mod_message,MAX_STIL_DATA_LENGTH*2,"Title..........: %s\nName...........:%s\nAuthor.........: %s\nCopyright......: %s\n",
                                        [mod_title UTF8String],mod_name,metadata->author,metadata->copyright);
                                
                                
                                mCurrentSamples=0;
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                [self iPhoneDrv_PlayRestart];
                                
                                
                                
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                
                                mod_message_updated=2;
                                
                                moveToSubSong=0;
                            }
                            if (intr) break;
                        }
                        
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
                        tim_finished=1;
                    } else if ((buffer_ana_flag[buffer_ana_gen_ofs]==0)&&(diff_ana_ofs<((SOUND_BUFFER_NB/2)-1))) {
                        
                        //NSLog(@"gen %d play %d diff %d",buffer_ana_gen_ofs,buffer_ana_play_ofs,diff_ana_ofs);
                        
                        if (mNeedSeek==1) { //SEEK
                            mNeedSeek=2;  //taken into account
                            
                            dispatch_sync(dispatch_get_main_queue(), ^(void){
                                //Run UI Updates
                                [detailViewControllerIphone showWaitingCancel];
                                [detailViewControllerIphone showWaitingProgress];
                                [detailViewControllerIphone showWaiting];
                                [detailViewControllerIphone updateWaitingTitle:NSLocalizedString(@"Seeking",@"")];
                                [detailViewControllerIphone updateWaitingDetail:@""];
                            });
                            
                            if (mPlayType==MMP_KSS) { //KSS
                                int64_t mStartPosSamples;
                                bGlobalSeekProgress=-1;
                                
                                int64_t mSeekSamples=mNeedSeekTime*PLAYBACK_FREQ/1000;
                                
                                if (mCurrentSamples > mSeekSamples) {
                                    mCurrentSamples=0;
                                    if (m3uReader.size()) {
                                        KSSPLAY_reset(kssplay, m3uReader[mod_currentsub].track, 0);
                                        iModuleLength=m3uReader[mod_currentsub].length;
                                        if (iModuleLength<=0) {
                                            if (kss->info) iModuleLength=kss->info[mod_currentsub].time_in_ms;
                                            else iModuleLength=optGENDefaultLength;
                                        }
                                        if (iModuleLength<1000) iModuleLength=1000;
                                    } else {
                                        KSSPLAY_reset(kssplay, mod_currentsub, 0);
                                        if (kss->info) {
                                            iModuleLength=kss->info[mod_currentsub].time_in_ms;
                                        } else iModuleLength=optGENDefaultLength;
                                        if (iModuleLength<1000) iModuleLength=1000;
                                    }
                                }
                                mStartPosSamples=mCurrentSamples;
                                
                                while ((mSeekSamples - mCurrentSamples) > SOUND_BUFFER_SIZE_SAMPLE) {
                                    KSSPLAY_calc_silent(kssplay, SOUND_BUFFER_SIZE_SAMPLE);
                                    //KSSPLAY_calc(kssplay, buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE);
                                    
                                    mCurrentSamples += SOUND_BUFFER_SIZE_SAMPLE;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                                if ((mSeekSamples - mCurrentSamples) > 0)
                                {
                                    KSSPLAY_calc_silent(kssplay, mSeekSamples - mCurrentSamples);
                                    //KSSPLAY_calc(kssplay, buffer_ana[buffer_ana_gen_ofs], mSeekSamples - mCurrentSamples);
                                    
                                    mCurrentSamples=mSeekSamples;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                }
                            }
                            if (mPlayType==MMP_V2M) { //V2M
                                bGlobalSeekProgress=-1;
                                //v2m_player->Stop();
                                v2m_player->Play(mNeedSeekTime);
                                mCurrentSamples=mNeedSeekTime*PLAYBACK_FREQ/1000;
                            }
                            if (mPlayType==MMP_WEBSID) { //WEBSID
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=mNeedSeekTime*PLAYBACK_FREQ/1000;
                                bGlobalSeekProgress=-1;
                                
                                uint8_t is_simple_sid_mode = !FileLoader::isExtendedSidFile();
                                uint8_t speed =    FileLoader::getCurrentSongSpeed();
                                
                                if (mSeekSamples<mCurrentSamples) {
                                    //reset
                                    websid_loader->initTune(PLAYBACK_FREQ,mod_currentsub);
                                    Postprocess::init(SOUND_BUFFER_SIZE_SAMPLE, PLAYBACK_FREQ);
                                    SidSnapshot::init(SOUND_BUFFER_SIZE_SAMPLE, SOUND_BUFFER_SIZE_SAMPLE);
                                    mCurrentSamples=0;
                                    m_voice_current_sample=0;
                                }
                                mStartPosSamples=mCurrentSamples;
                                
                                mSIDSeekInProgress=1;
                                while (mCurrentSamples+SOUND_BUFFER_SIZE_SAMPLE<=mSeekSamples) {
                                    Core::runOneFrame(is_simple_sid_mode, speed, buffer_ana[buffer_ana_gen_ofs],
                                                      websid_scope_buffers, SOUND_BUFFER_SIZE_SAMPLE);
                                    mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    Postprocess::applyStereoEnhance(buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE);
                                    SidSnapshot::record();
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                                while (mCurrentSamples<mSeekSamples) {
                                    Core::runOneFrame(is_simple_sid_mode, speed, buffer_ana[buffer_ana_gen_ofs],
                                                      websid_scope_buffers, SOUND_BUFFER_SIZE_SAMPLE/*mSeekSamples*/);
                                    mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE/*mSeekSamples*/;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    Postprocess::applyStereoEnhance(buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE);
                                    SidSnapshot::record();
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                                mSIDSeekInProgress=0;
                            }
                            if (mPlayType==MMP_SIDPLAY) { //SID
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=mNeedSeekTime*PLAYBACK_FREQ/1000;
                                bGlobalSeekProgress=-1;
                                if (mSeekSamples<mCurrentSamples) {
                                    //reset
                                    mSidTune->selectSong(mod_currentsub+1);
                                    mSidEmuEngine->load(mSidTune);
                                    mCurrentSamples=0;
                                    m_voice_current_sample=0;
                                }
                                mStartPosSamples=mCurrentSamples;
                                
                                mSIDSeekInProgress=1;
                                mSidEmuEngine->fastForward( 100 * 32 );
                                while (mCurrentSamples+SOUND_BUFFER_SIZE_SAMPLE*32<=mSeekSamples) {
                                    nbBytes=mSidEmuEngine->play(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*1)*2;
                                    mCurrentSamples+=(nbBytes/4)*32;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                                
                                
                                mSidEmuEngine->fastForward( 100 );
                                while (mCurrentSamples<mSeekSamples) {
                                    nbBytes=mSidEmuEngine->play(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*1)*2;
                                    mCurrentSamples+=(nbBytes/4);
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                                mSIDSeekInProgress=0;
                                                                
                            }
                            if (mPlayType==MMP_GME) {   //GME
                                bGlobalSeekProgress=-1;
                                gme_seek(gme_emu,mNeedSeekTime);
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
                            if (mPlayType==MMP_ATARISOUND) {//ATARISOUND
                                int64_t mStartPosSamples;
                                int mSeekSamples=(double)mNeedSeekTime*(double)(PLAYBACK_FREQ)/1000.0f;
                                bGlobalSeekProgress=-1;
                                if (mCurrentSamples > mSeekSamples) {
                                    atariSndh.InitSubSong(mod_currentsub);
                                    mCurrentSamples=0;
                                }
                                mStartPosSamples=mCurrentSamples;
                                
                                while (mSeekSamples - mCurrentSamples > SOUND_BUFFER_SIZE_SAMPLE) {
                                    atariSndh.AudioRender(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE);
                                    mCurrentSamples += SOUND_BUFFER_SIZE_SAMPLE;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                                if (mSeekSamples - mCurrentSamples > 0)
                                {
                                    atariSndh.AudioRender(buffer_ana[buffer_ana_gen_ofs],mSeekSamples-mCurrentSamples);
                                    mCurrentSamples = mSeekSamples;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                            }
                            if (mPlayType==MMP_SC68) {//SC68
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=(double)mNeedSeekTime*(double)(PLAYBACK_FREQ)/1000.0f;
                                bGlobalSeekProgress=-1;
                                if (mCurrentSamples > mSeekSamples) {
                                    sc68_play(sc68,mod_currentsub,(mLoopMode?SC68_INF_LOOP:0));
                                    mCurrentSamples=0;
                                }
                                mStartPosSamples=mCurrentSamples;
                                
                                while (mSeekSamples - mCurrentSamples > SOUND_BUFFER_SIZE_SAMPLE) {
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE;//*2*2;
                                    int code = sc68_process(sc68, buffer_ana[buffer_ana_gen_ofs], &nbBytes);
                                    
                                    mCurrentSamples += SOUND_BUFFER_SIZE_SAMPLE;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                                if (mSeekSamples - mCurrentSamples > 0)
                                {
                                    nbBytes=(mSeekSamples-mCurrentSamples)/4;
                                    int code = sc68_process(sc68, buffer_ana[buffer_ana_gen_ofs], &nbBytes);
                                    
                                    mCurrentSamples = mSeekSamples;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                            }
                            if (mPlayType==MMP_PT3) {//PT3
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=mNeedSeekTime*PLAYBACK_FREQ/1000;
                                bGlobalSeekProgress=-1;
                                
                                if (mSeekSamples<mCurrentSamples) {
                                    mCurrentSamples=0;
                                    for (int ch=0; ch<pt3_numofchips; ch++) {
                                        func_restart_music(ch);
                                        pt3_frame[ch] = 0;
                                        pt3_sample[ch] = 0;
                                    }
                                }
                                mStartPosSamples=mCurrentSamples;
                                
                                while (1) {
                                    int nbBytes=(mSeekSamples-mCurrentSamples)*4;
                                    if (nbBytes>SOUND_BUFFER_SIZE_SAMPLE*4) nbBytes=SOUND_BUFFER_SIZE_SAMPLE*4;
                                    for (int ch=0; ch<pt3_numofchips; ch++) {
                                        pt3_renday(NULL, nbBytes, &pt3_ay[ch], &pt3_t, ch,0);
                                    }
                                    mCurrentSamples+=nbBytes>>2;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    if (mCurrentSamples>=mSeekSamples) break;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                            }
                            if (mPlayType==MMP_ASAP) { //ASAP
                                bGlobalSeekProgress=-1;
                                ASAP_Seek(asap, mNeedSeekTime);
                            }
                            if (mPlayType==MMP_PMDMINI) { //PMDMini : not supported
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=(double)mNeedSeekTime*(double)(PLAYBACK_FREQ)/1000.0f;
                                bGlobalSeekProgress=-1;
                                if (mCurrentSamples >mSeekSamples) {
                                    pmd_stop();
                                    // doesn't actually play, just loads file into RAM & extracts data
                                    char *arg[4];
                                    arg[0]=NULL;
                                    arg[1]=(char*)[mod_currentfile UTF8String];
                                    arg[2]=NULL;
                                    arg[3]=NULL;
                                    pmd_play(arg, (char*)[[mod_currentfile stringByDeletingLastPathComponent] UTF8String]);
                                    
                                    iCurrentTime=0;
                                    mCurrentSamples=0;
                                }
                                mStartPosSamples=mCurrentSamples;
                                
                                while (mSeekSamples - mCurrentSamples > 0) {
                                    int64_t sample_to_skip=mSeekSamples - mCurrentSamples;
                                    if (sample_to_skip>SOUND_BUFFER_SIZE_SAMPLE) sample_to_skip=SOUND_BUFFER_SIZE_SAMPLE;
                                    
                                    pmd_renderer(buffer_ana[buffer_ana_gen_ofs], sample_to_skip);
                                    
                                    mCurrentSamples+=sample_to_skip;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                                
                            }
                            if (mPlayType==MMP_NSFPLAY) { //NSFPlay
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=(double)mNeedSeekTime*(double)(PLAYBACK_FREQ)/1000.0f;
                                bGlobalSeekProgress=-1;
                                if (mCurrentSamples >mSeekSamples) {
                                    //nsfPlayer->SetSong(mod_currentsub);
                                    nsfPlayer->Reset();
                                    
                                    iCurrentTime=0;
                                    mCurrentSamples=0;
                                }
                                mStartPosSamples=mCurrentSamples;
                                
                                while (mCurrentSamples<mSeekSamples) {
                                    int64_t sample_to_skip=mSeekSamples - mCurrentSamples;
                                    if (sample_to_skip>SOUND_BUFFER_SIZE_SAMPLE) sample_to_skip=SOUND_BUFFER_SIZE_SAMPLE;
                                    nsfPlayer->Skip(sample_to_skip);
                                    
                                    mCurrentSamples+=sample_to_skip;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                                
                            }
                            if (mPlayType==MMP_VGMPLAY) { //VGM
                                bGlobalSeekProgress=-1;
                                SeekVGM(false,mNeedSeekTime*441/10);
                                //mNeedSeek=0;
                            }
                            if (mPlayType==MMP_PIXEL) {
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=(double)mNeedSeekTime*(double)(PLAYBACK_FREQ)/1000.0f;
                                bGlobalSeekProgress=-1;
                                if (mCurrentSamples >mSeekSamples) {
                                    
                                    if (!pixel_organya_mode) {
                                        pixel_pxtn->clear();
                                        pixel_pxtn->init();
                                        
                                        pixel_pxtn->set_destination_quality(2, PLAYBACK_FREQ);
                                        pixel_desc->set_memory_r(pixel_fileBuffer, pixel_fileBufferLen);
                                        pixel_pxtn->read(pixel_desc);
                                        pixel_pxtn->tones_ready();
                                        
                                        
                                        pxtnVOMITPREPARATION prep = {0};
                                        //prep.flags |= pxtnVOMITPREPFLAG_loop; // don't loop
                                        prep.start_pos_float = 0;
                                        prep.master_volume = 1; //(volume / 100.0f);
                                        
                                        pixel_pxtn->moo_preparation(&prep);
                                    } else {
                                        org_play(org_filename, pixel_fileBuffer);
                                    }
                                    mCurrentSamples=0;
                                }
                                mStartPosSamples=mCurrentSamples;
                                
                                while (mSeekSamples - mCurrentSamples > 0) {
                                    int64_t sample_to_skip=mSeekSamples - mCurrentSamples;
                                    if (sample_to_skip>SOUND_BUFFER_SIZE_SAMPLE) sample_to_skip=SOUND_BUFFER_SIZE_SAMPLE;
                                    if (pixel_organya_mode) org_gensamples((char*)(buffer_ana[buffer_ana_gen_ofs]), sample_to_skip);
                                    else pixel_pxtn->Moo(NULL, sample_to_skip*2*2);
                                    
                                    mCurrentSamples+=sample_to_skip;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                            }
                            if (mPlayType==MMP_EUP) { //EUP
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=(double)mNeedSeekTime*(double)(PLAYBACK_FREQ)/1000.0f;
                                bGlobalSeekProgress=-1;
                                if (mCurrentSamples >mSeekSamples) {
                                    eup_player->stopPlaying();
                                    EUPPlayer_ResetReload();
                                    mCurrentSamples=0;
                                }
                                mStartPosSamples=mCurrentSamples;
                                
                                while (mSeekSamples - mCurrentSamples > 0) {
                                    eup_player->nextTick(true);
                                    
                                    mCurrentSamples+=eup_pcm.write_pos/2;
                                    iCurrentTime=mCurrentSamples*1000/PLAYBACK_FREQ;
                                    
                                    eup_pcm.write_pos=0;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mCurrentSamples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mCurrentSamples;
                                        break;
                                    }
                                }
                            }
                            if (mPlayType==MMP_HC) { //HC
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=(double)mNeedSeekTime*(double)(hc_sample_rate)/1000.0f;
                                bGlobalSeekProgress=-1;
                                if (hc_currentSample>mSeekSamples) {
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
                                mStartPosSamples=hc_currentSample;
                                
                                //
                                // progress
                                //
                                while (mSeekSamples - hc_currentSample > SOUND_BUFFER_SIZE_SAMPLE) {
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
                                    hc_currentSample += SOUND_BUFFER_SIZE_SAMPLE;
                                    iCurrentTime=hc_currentSample*1000/hc_sample_rate;
                                    
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(hc_currentSample-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=hc_currentSample;
                                        break;
                                    }
                                }
                                //
                                // fine tune
                                //
                                if (mSeekSamples - hc_currentSample > 0)
                                {
                                    uint32_t howmany = (mSeekSamples - hc_currentSample);
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
                                        case 0x23: //to confirm it is ok
                                            snsf_gen(hc_sample_data, howmany);
                                            break;
                                        case 0x41:
                                            qsound_execute( HC_emulatorCore, 0x7fffffff, 0, &howmany );
                                            break;
                                    }
                                    hc_currentSample = mSeekSamples;
                                    iCurrentTime=hc_currentSample*1000/hc_sample_rate;
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(hc_currentSample-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=hc_currentSample;
                                        break;
                                    }
                                }
                            }
                            if (mPlayType==MMP_2SF) { //2SF
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=(double)mNeedSeekTime*(double)(xSFPlayer->GetSampleRate())/1000.0f;
                                bGlobalSeekProgress=-1;
                                if (xSFPlayer->currentSample >mSeekSamples) {
                                    xSFPlayer->Terminate();
                                    xSFPlayer->Load();
                                    xSFPlayer->SeekTop();
                                }
                                mStartPosSamples=xSFPlayer->currentSample;
                                
                                while (mSeekSamples - xSFPlayer->currentSample > SOUND_BUFFER_SIZE_SAMPLE) {
                                    xSFPlayer->GenerateSamples(xsfSampleBuffer, 0, SOUND_BUFFER_SIZE_SAMPLE);
                                    xSFPlayer->currentSample += SOUND_BUFFER_SIZE_SAMPLE;
                                    iCurrentTime=(xSFPlayer->currentSample)*1000/(xSFPlayer->GetSampleRate());
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(xSFPlayer->currentSample-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=xSFPlayer->currentSample;
                                        break;
                                    }
                                }
                                if (mSeekSamples - xSFPlayer->currentSample > 0)
                                {
                                    xSFPlayer->GenerateSamples(xsfSampleBuffer, 0, mSeekSamples - xSFPlayer->currentSample);
                                    xSFPlayer->currentSample = mSeekSamples;
                                    iCurrentTime=(xSFPlayer->currentSample)*1000/(xSFPlayer->GetSampleRate());
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(xSFPlayer->currentSample-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=xSFPlayer->currentSample;
                                        break;
                                    }
                                }
                                
                                //mNeedSeek=0;
                            }
                            if (mPlayType==MMP_VGMSTREAM) { //VGMSTREAM
                                int64_t mStartPosSamples;
                                int64_t mSeekSamples=(double)mNeedSeekTime*(double)(vgmStream->sample_rate)/1000.0f;
                                bGlobalSeekProgress=-1;
                                if (mVGMSTREAM_decode_pos_samples>mSeekSamples) {
                                    reset_vgmstream(vgmStream);
                                    mVGMSTREAM_decode_pos_samples=0;
                                }
                                mStartPosSamples=mVGMSTREAM_decode_pos_samples;
                                
                                while (mVGMSTREAM_decode_pos_samples<mSeekSamples) {
                                    int nbSamplesToRender=mSeekSamples-mVGMSTREAM_decode_pos_samples;
                                    if (nbSamplesToRender>SOUND_BUFFER_SIZE_SAMPLE) nbSamplesToRender=SOUND_BUFFER_SIZE_SAMPLE;
                                    render_vgmstream(vgm_sample_data, nbSamplesToRender, vgmStream);
                                    
                                    mVGMSTREAM_decode_pos_samples+=nbSamplesToRender;
                                    iCurrentTime=mVGMSTREAM_decode_pos_samples*1000/(vgmStream->sample_rate);
                                    
                                    dispatch_sync(dispatch_get_main_queue(), ^(void){
                                        //Run UI Updates
                                        [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithFloat: (float)(mVGMSTREAM_decode_pos_samples-mStartPosSamples)/(mSeekSamples-mStartPosSamples)]];
                                    });
                                    if ([detailViewControllerIphone isCancelPending]) {
                                        [detailViewControllerIphone resetCancelStatus];
                                        mSeekSamples=mVGMSTREAM_decode_pos_samples;
                                        break;
                                    }
                                }
                            }
                            
                            dispatch_sync(dispatch_get_main_queue(), ^(void){
                                [detailViewControllerIphone hideWaiting];
                                [detailViewControllerIphone hideWaitingCancel];
                            });
                        }
                        if (moveToSubSong) {
                            mod_currentsub=moveToSubSongIndex;
                            mod_message_updated=1;
                            if (mPlayType==MMP_KSS) {
                                if (m3uReader.size()) {
                                    KSSPLAY_reset(kssplay, m3uReader[mod_currentsub].track, 0);
                                    iModuleLength=m3uReader[mod_currentsub].length;
                                    if (iModuleLength<=0) {
                                        if (kss->info) iModuleLength=kss->info[mod_currentsub].time_in_ms;
                                        else iModuleLength=optGENDefaultLength;
                                    }
                                } else {
                                    KSSPLAY_reset(kssplay, mod_currentsub, 0);
                                    if (kss->info) {
                                        iModuleLength=kss->info[mod_currentsub].time_in_ms;
                                    } else iModuleLength=optGENDefaultLength;
                                }
                                
                                if (m3uReader.size()) {
                                    if (m3uReader[mod_currentsub].name) mod_title=[NSString stringWithUTF8String:m3uReader[mod_currentsub-mod_minsub].name];
                                } else {
                                    if (kss->info) {
                                        if (kss->info[mod_currentsub].title[0]) mod_title=[NSString stringWithUTF8String:kss->info[mod_currentsub].title];
                                    }
                                }
                                
                                mCurrentSamples=0;
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                [self iPhoneDrv_PlayRestart];
                                
                                if (mLoopMode) iModuleLength=-1;
                                if (iModuleLength>0) mFadeSamplesStart=(int64_t)(iModuleLength-1000)*PLAYBACK_FREQ/1000; //1s
                                else mFadeSamplesStart=1<<30;
                                
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=2;
                            } else if (mPlayType==MMP_HVL) {
                                hvl_InitSubsong( hvl_song,mod_currentsub );
                                
                                iModuleLength=hvl_GetPlayTime(hvl_song);
                                if (mLoopMode) iModuleLength=-1;
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                [self iPhoneDrv_PlayRestart];
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=1;
                            } else if (mPlayType==MMP_ADPLUG) {
                                adPlugPlayer->rewind(mod_currentsub);
                                
                                iModuleLength=adPlugPlayer->songlength();
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                if (adPlugPlayer->update()) opl_towrite=(int)(PLAYBACK_FREQ*1.0f/adPlugPlayer->getrefresh());
                                
                                [self iPhoneDrv_PlayRestart];
                                
                                //if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=1;
                            } else if (mPlayType==MMP_XMP) {
                                xmp_set_position(xmp_ctx,mod_currentsub);
                                
                                iModuleLength=xmp_mi->seq_data[mod_currentsub].duration;
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                [self iPhoneDrv_PlayRestart];
                                
                                //if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=1;
                            } else if (mPlayType==MMP_VGMSTREAM) {
                                [self vgmStream_ChangeToSub:mod_currentsub];
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                if (moveToSubSong==1) [self iPhoneDrv_PlayRestart];
                                
                                if (mLoopMode) iModuleLength=-1;
                                
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=2;
                            } else if (mPlayType==MMP_OPENMPT) {
                                openmpt_module_select_subsong(openmpt_module_ext_get_module(ompt_mod), mod_currentsub);
                                iModuleLength=openmpt_module_get_duration_seconds( openmpt_module_ext_get_module(ompt_mod) )*1000;
                                iCurrentTime=0;
                                //numChannels=openmpt_module_get_num_channels(openmpt_module_ext_get_module(ompt_mod));  //should not change in a subsong
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                if (moveToSubSong==1) [self iPhoneDrv_PlayRestart];
                                
                                //if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=1;
                            } else if (mPlayType==MMP_GME) {//GME
                                gme_start_track(gme_emu,mod_currentsub);
                                //sprintf(mod_name," %s",mod_filename);
                                if (gme_track_info( gme_emu, &gme_info, mod_currentsub )==0) {
                                    strcpy(gmetype,gme_info->system);
                                    iModuleLength=gme_info->play_length;
                                    if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                    
                                    sprintf(mod_message,"Song.......: %s\nGame.......: %s\nAuthor.....: %s\nDumper.....: %s\nCopyright..: %s\nTracks......: %d\n%s",
                                            (gme_info->song?gme_info->song:" "),
                                            (gme_info->game?gme_info->game:" "),
                                            (gme_info->author?gme_info->author:" "),
                                            (gme_info->dumper?gme_info->dumper:" "),
                                            (gme_info->copyright?gme_info->copyright:" "),
                                            gme_track_count( gme_emu ),
                                            (gme_info->comment?gme_info->comment:" "));
                                    
//                                    if (gme_info->song){
//                                        if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);
//                                    }
                                    if (gme_info->song){
                                        if (gme_info->song[0]) mod_title=[NSString stringWithCString:gme_info->song encoding:NSShiftJISStringEncoding];
                                        else mod_title=NULL;
                                        //if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);
                                    }
                                    gme_free_info(gme_info);
                                } else {
                                    strcpy(gmetype,"N/A");
                                    strcpy(mod_message,"N/A\n");
                                    iModuleLength=optGENDefaultLength;
                                }
                                //LOOP
                                if (mLoopMode==1) iModuleLength=-1;
                                
                                if (iModuleLength>0) {
                                    if (iModuleLength>settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000) gme_set_fade_msecs( gme_emu, iModuleLength-settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000,settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000 ); //Fade 1s before end
                                    else gme_set_fade_msecs( gme_emu, iModuleLength/2, iModuleLength/2 );
                                } else gme_set_fade_msecs( gme_emu, 1<<30,settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000 );
                                if (moveToSubSong==1) [self iPhoneDrv_PlayRestart];
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=2;
                            } else if (mPlayType==MMP_WEBSID) { //WEBSID
                                websid_loader->initTune(PLAYBACK_FREQ, mod_currentsub);
                                Postprocess::init(SOUND_BUFFER_SIZE_SAMPLE, PLAYBACK_FREQ);
                                SidSnapshot::init(SOUND_BUFFER_SIZE_SAMPLE, SOUND_BUFFER_SIZE_SAMPLE);
                                // testcase: Baroque_Music_64_BASIC (takes 100sec before playing - without this speedup)
                                websid_skip_silence_loop = 10;
                                websid_sound_started=0;
                                
                                iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
                                if (iModuleLength<0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
                                else if (iModuleLength==0) iModuleLength=1000;
                                mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
                                
                                if (mLoopMode) iModuleLength=-1;
                                
                                if (moveToSubSong==1) [self iPhoneDrv_PlayRestart];
                                
                                iCurrentTime=0;
                                mCurrentSamples=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                mod_name[0]=0;
                                if (sidtune_name) {
                                    if (sidtune_name[mod_currentsub]) snprintf(mod_name,sizeof(mod_name),[[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_name[mod_currentsub]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
                                }
                                if (sidtune_title) {
                                    if (sidtune_title[mod_currentsub]) {
                                        if (mod_name[0]==0) snprintf(mod_name,sizeof(mod_name),[[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_title[mod_currentsub]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
                                        else strcat(mod_name, [[[NSString stringWithFormat:@" / %s",sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
                                    }
                                }
                                if (sidtune_artist) {
                                    if (sidtune_artist[mod_currentsub]) {
                                        artist=[NSString stringWithUTF8String:sidtune_artist[mod_currentsub]];
                                    }
                                }
                                if (mod_name[0]==0) {
                                    if (websid_info[4][0]) snprintf(mod_name,sizeof(mod_name),[[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:websid_info[4]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
                                }
                                
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=2;
                            } else if (mPlayType==MMP_SIDPLAY) { //SID
                                mSidTune->selectSong(mod_currentsub+1);
                                mSidEmuEngine->load(mSidTune);
                                
                                if (moveToSubSong==1) [self iPhoneDrv_PlayRestart];
                                
                                iCurrentTime=0;
                                mCurrentSamples=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                m_voice_current_sample=0;
                                iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
                                if (iModuleLength<0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
                                else if (iModuleLength==0) iModuleLength=1000;
                                
                                mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
                                if (mLoopMode) iModuleLength=-1;
                                
                                if (sidtune_name) {
                                    if (sidtune_name[mod_currentsub]) mod_title=[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_name[mod_currentsub]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
                                    else mod_title=NULL;
                                } else mod_title=NULL;
                                if (sidtune_title) {
                                    if (sidtune_title[mod_currentsub]) {
                                        if (mod_title==NULL) mod_title=[NSString stringWithFormat:@"%@",[[NSString stringWithUTF8String:sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]];
                                        else mod_title=[NSString stringWithFormat:@"%@ - %@",mod_title,[[NSString stringWithUTF8String:sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]];
                                    }
                                }
                                
                                if (sidtune_artist) {
                                    if (sidtune_artist[mod_currentsub]) {
                                        artist=[NSString stringWithUTF8String:sidtune_artist[mod_currentsub]];
                                    }
                                }
                                                                
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=2;
                            } else if (mPlayType==MMP_SC68) {//SC68
                                if (moveToSubSong==1) [self iPhoneDrv_PlayRestart];
                                sc68_play(sc68,mod_currentsub,(mLoopMode?SC68_INF_LOOP:0));
                                sc68_process(sc68, buffer_ana[buffer_ana_gen_ofs], 0); //to apply the track change
                                sc68_music_info_t info;
                                sc68_music_info(sc68,&info,SC68_CUR_TRACK,0);
                                iModuleLength=info.trk.time_ms;
                                
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
                                if (mLoopMode) iModuleLength=-1;
                                //NSLog(@"track : %d, time : %d, start : %d",mod_currentsub,info.time_ms,info.start_ms);
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                if (info.title[0]) snprintf(mod_name,sizeof(mod_name)," %s",info.title);
                                else snprintf(mod_name,sizeof(mod_name)," %s",mod_filename);
                                
                                if (info.artist[0]) {
                                    artist=[NSString stringWithUTF8String:info.artist];
                                }
                                
                                
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=2;
                            } else if (mPlayType==MMP_ATARISOUND) {//ATARISOUND
                                atariSndh.InitSubSong(mod_currentsub);
                                SndhFile::SubSongInfo info;
                                atariSndh.GetSubsongInfo(mod_currentsub,info);
                                
                                if (info.musicSubTitle) sprintf(mod_message,"Title..........: %s\nSubsong title..: %s\nArtist.........: %s\nYear.........: %s\nRipper.........: %s\nConverter......: %s\n",
                                                                info.musicTitle,info.musicSubTitle,info.musicAuthor,info.year,info.ripper,info.converter);
                                else sprintf(mod_message,"Title.....: %s\nArtist....: %s\nYear......: %s\nRipper....: %s\nConverter.: %s\n",
                                             info.musicTitle,info.musicAuthor,info.year,info.ripper,info.converter);
                                
                                if (info.musicSubTitle) sprintf(mod_name," %s",info.musicSubTitle);
                                else if (info.musicTitle) sprintf(mod_name," %s",info.musicTitle);
                                
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                mCurrentSamples=0;
                                m_voice_current_sample=0;
                                iModuleLength=info.playerTickCount*1000/info.playerTickRate;
                                if (iModuleLength<=0) {
                                    unsigned int frames;
                                    int flags;
                                    if (timedb68_get(atariSndh_hash,mod_currentsub-1,&frames,&flags)>=0) {
                                        iModuleLength=frames*1000/info.playerTickRate;
                                    }
                                }
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                
                                if (moveToSubSong==1) [self iPhoneDrv_PlayRestart];
                                
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=2;
                            } else if (mPlayType==MMP_PT3) {//PT3
                                //subsong not supported
                            } else if (mPlayType==MMP_NSFPLAY) { //NSFPLAY
                                if (m3uReader.size()) {
                                    nsfPlayer->SetSong(m3uReader[mod_currentsub].track);
                                } else nsfPlayer->SetSong(mod_currentsub);
                                
                                nsfPlayer->Reset();
                                
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                mCurrentSamples=0;
                                
                                if (m3uReader.size()) iModuleLength=m3uReader[mod_currentsub-mod_minsub].length;
                                else iModuleLength=nsfPlayer->GetLength();
                                if (iModuleLength<=0) iModuleLength=optNSFPLAYDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                
                                if (moveToSubSong==1) [self iPhoneDrv_PlayRestart];
                                
                                // song info
                                if (m3uReader.size()) {
                                    mod_title=[NSString stringWithUTF8String:m3uReader[mod_currentsub-mod_minsub].name];
                                } else mod_title=[NSString stringWithUTF8String:nsfData->GetTitleString("%L",mod_currentsub)];
                                                                                                
                                while (mod_message_updated) {
                                    usleep(1);
                                }
                                mod_message_updated=2;
                            } else if (mPlayType==MMP_ASAP) {//ASAP
                                iModuleLength=ASAPInfo_GetDuration(ASAP_GetInfo(asap),mod_currentsub);
                                if (iModuleLength<1000) iModuleLength=1000;
                                ASAP_PlaySong(asap, mod_currentsub, iModuleLength);
                                
                                iCurrentTime=0;
                                mNeedSeek=0;bGlobalSeekProgress=0;
                                
                                if (moveToSubSong==1) [self iPhoneDrv_PlayRestart];
                                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                                if (mLoopMode) iModuleLength=-1;
                                
                                while (mod_message_updated) {
                                    //wait
                                    usleep(1);
                                }
                                mod_message_updated=1;
                            }
                            moveToSubSong=0;
                        }
                        
                        if (mPlayType==MMP_GME) {  //GME
                            nbBytes=0;
                            m_voice_current_system=-1;
                            if (gme_track_ended(gme_emu)) {
                                //NSLog(@"Track ended : %d",iCurrentTime);
                                if (mLoopMode==1) {
                                    gme_start_track(gme_emu,mod_currentsub);
                                    gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE*2, buffer_ana[buffer_ana_gen_ofs] );
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                } else if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                            } else {
                                gme_play( gme_emu, SOUND_BUFFER_SIZE_SAMPLE*2, buffer_ana[buffer_ana_gen_ofs] );
                                nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                
                                if (m_voicesDataAvail) {
                                    
                                    //copy voice data for oscillo view
                                    for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                            m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)];
                                            //m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)]=0;
                                        }
                                        m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                        
                                    }
                                }
                                
                            }
                        }
                        if (mPlayType==MMP_XMP) {  //XMP
                            if (m_voicesDataAvail) {
                                //reset to 0 buffer
                                //for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++)  memset(m_voice_buff[j],0,SOUND_BUFFER_SIZE_SAMPLE);
                            }
                            
                            if (xmp_play_buffer(xmp_ctx, buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE*2*2, 1) == 0) {
                                struct xmp_frame_info xmp_fi;
                                xmp_get_frame_info(xmp_ctx, &xmp_fi);
                                
                                nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                
                                genPattern[buffer_ana_gen_ofs]=xmp_fi.pattern;
                                genRow[buffer_ana_gen_ofs]=xmp_fi.row;
                                genPrevPattern[buffer_ana_gen_ofs]=-1;
                                genNextPattern[buffer_ana_gen_ofs]=-1;
                                
                                for (int i=0;i<numChannels;i++) {
                                    int v=xmp_fi.channel_info[i].volume*4;
                                    genVolData[buffer_ana_gen_ofs*SOUND_MAXMOD_CHANNELS+i]=(v>255?255:v);
                                }
                                
                                if (m_voicesDataAvail) {
                                    //copy voice data for oscillo view
                                    for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                            m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)];
                                            m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)]=0;
                                        }
                                        m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                        if (m_voice_prev_current_ptr[j]>=(SOUND_BUFFER_SIZE_SAMPLE*2*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)) m_voice_prev_current_ptr[j]-=(SOUND_BUFFER_SIZE_SAMPLE*2*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                                    }
                                }
                            } else {
                                //NSLog(@"XMP: end of song");
                                nbBytes=0;
                            }
                            
                        }
                        if (mPlayType==MMP_OPENMPT) {  //MODPLUG
                            int prev_ofs=buffer_ana_gen_ofs-1;
                            if (prev_ofs<0) prev_ofs=SOUND_BUFFER_NB-1;
                            
                            if (m_voicesDataAvail) {
                                //reset to 0 buffer
                                for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++)  memset(m_voice_buff[j],0,SOUND_BUFFER_SIZE_SAMPLE);
                            }
                            
                            genPattern[buffer_ana_gen_ofs]=openmpt_module_get_current_pattern(openmpt_module_ext_get_module(ompt_mod));
                            genRow[buffer_ana_gen_ofs]=openmpt_module_get_current_row(openmpt_module_ext_get_module(ompt_mod));
                            
                            int order_idx=-1;
                            int current_order=openmpt_module_get_current_order(openmpt_module_ext_get_module(ompt_mod));
                            if (current_order>0) order_idx=openmpt_module_get_order_pattern(openmpt_module_ext_get_module(ompt_mod),current_order-1);
                            genPrevPattern[buffer_ana_gen_ofs]=order_idx;
                            
                            if (current_order<openmpt_module_get_num_orders(openmpt_module_ext_get_module(ompt_mod))) order_idx=openmpt_module_get_order_pattern(openmpt_module_ext_get_module(ompt_mod),current_order+1);
                            genNextPattern[buffer_ana_gen_ofs]=order_idx;
                            
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
                                    for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE-1)];
                                    }
                                }
                            }
                            
                            if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (mLoopMode==0)&&(iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
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
                                
                                if (m_genNumVoicesChannels) {
                                    for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                        memset(m_voice_buff[j],0,SOUND_BUFFER_SIZE_SAMPLE);
                                        m_voice_current_ptr[j]=0;
                                    }
                                }
                                
                                nbBytes=VGMFillBuffer((WAVE_16BS*)(buffer_ana[buffer_ana_gen_ofs]), SOUND_BUFFER_SIZE_SAMPLE)*2*2;
                                
                                
                                if (m_genNumVoicesChannels) {
                                    for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                        
                                        for (int i=(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT);i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                            m_voice_buff[j][i]=m_voice_buff[j][(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)-1];
                                        }
                                    }
                                }
                                
                                //NSLog(@"prev ptr: %d",m_voice_prev_current_ptr[5]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                                //copy voice data for oscillo view
                                for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                    for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)];
                                        m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)]=0;
                                    }
                                    m_voice_prev_current_ptr[j]=0;
                                    //                                    m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                    //                                    if ((m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4*2) m_voice_prev_current_ptr[j]-=SOUND_BUFFER_SIZE_SAMPLE*4*2<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                }
                                
                            }
                        }
                        if (mPlayType==MMP_VGMSTREAM) { //VGMSTREAM
                            
                            
                            nbBytes=src_callback_read (src_state,src_ratio,SOUND_BUFFER_SIZE_SAMPLE, vgm_sample_converted_data_float)*2*2;
                            
                            src_float_to_short_array (vgm_sample_converted_data_float,buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2) ;
                            
                            if (mVGMSTREAM_total_samples>=0) {
                                if (mVGMSTREAM_decode_pos_samples>=mVGMSTREAM_total_samples) nbBytes=0;
                                //if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                            }
                            
                            
                            if (nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2) {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                            }
                        }
                        if (mPlayType==MMP_HC) { //Highly Complete
                            
                            //reset voice data for oscillo view if not SNSF
                            if (m_genNumVoicesChannels&&(HC_type!=0x23)&&(HC_type!=0x41)&&(HC_type!=0x1)&&(HC_type!=0x2)) {
                                for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                    memset(m_voice_buff[j],0,SOUND_BUFFER_SIZE_SAMPLE);
                                    m_voice_current_ptr[j]=0;
                                }
                            }
                            
                            nbBytes=src_callback_read (src_state,src_ratio,SOUND_BUFFER_SIZE_SAMPLE, hc_sample_converted_data_float)*2*2;
                            
                            
                            src_float_to_short_array (hc_sample_converted_data_float,buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2) ;
                            
                            //copy voice data for oscillo view
                            if (m_genNumVoicesChannels) {
                                if ((HC_type==0x23)||(HC_type==0x41)||(HC_type==0x1)||(HC_type==0x2))  { //SNSF, QSF, PSF, PSF2
                                    for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j]
                                            [(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)];
                                            m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)]=0;
                                        }
                                        m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                        if ((m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*4*2) m_voice_prev_current_ptr[j]=m_voice_prev_current_ptr[j]-((SOUND_BUFFER_SIZE_SAMPLE*2*4)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                                    }
                                } else {
                                    for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE-1)];
                                        }
                                        m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                        if ((m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE) m_voice_prev_current_ptr[j]=m_voice_prev_current_ptr[j]-((SOUND_BUFFER_SIZE_SAMPLE)<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                                    }
                                }
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
                            if (m_genNumVoicesChannels) {
                                for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                    for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+0*(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE-1)];
                                    }
                                }
                                
                            }
                            
                            //if ((iModuleLength!=-1)&&(iCurrentTime>iModuleLength)) nbBytes=0;
                            
                        }
                        if (mPlayType==MMP_V2M) { //V2M
                            
                            v2m_player->Render((float*) v2m_sample_data_float, SOUND_BUFFER_SIZE_SAMPLE);
                            mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                            //copy voice data for oscillo view
                            if (m_genNumVoicesChannels) {
                                for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                    for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE-1)];
                                    }
                                }
                                
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
                            memcpy(m_voice_prev_current_ptr,m_voice_current_ptr,sizeof(m_voice_prev_current_ptr));
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
                                    if (!opl_towrite) {
                                        if (mLoopMode==1) {
                                            opl_towrite=(int)(PLAYBACK_FREQ*1.0f/adPlugPlayer->getrefresh());
                                            adPlugPlayer->seek(0);
                                        }
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
                                                    } else {
                                                        opl_towrite=0;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                
                                if (m_genNumVoicesChannels) {
                                    for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                        for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2-1)];
                                            m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2-1)]=0;
                                        }
                                    }
                                }
                                
                                
                            } else {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=0;
                                
                            }
                        }
                        if (mPlayType==MMP_HVL) {  //HVL
                            memcpy(m_voice_prev_current_ptr,m_voice_current_ptr,sizeof(m_voice_prev_current_ptr));
                            
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
                            
                            if (m_genNumVoicesChannels) {
                                for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                    for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2-1)];
                                        m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2-1)]=0;
                                    }
                                }
                            }
                            
                        }
                        
                        if (mPlayType==MMP_KSS) {
                            
                            if (m_genNumVoicesChannels) {
                                for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                    memset(m_voice_buff[j],0,SOUND_BUFFER_SIZE_SAMPLE);
                                    m_voice_current_ptr[j]=0;
                                }
                            }
                            
                            KSSPLAY_calc(kssplay, buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE);
                            mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                            
                            /* If looped, start fadeout */
                            if ( (KSSPLAY_get_loop_count(kssplay) > kssOptLoopNb) && (KSSPLAY_get_fade_flag(kssplay) == KSSPLAY_FADE_NONE) ) {
                                KSSPLAY_fade_start(kssplay, 5000); //5s fadeout when looping
                            } else if ( (mCurrentSamples >= mFadeSamplesStart) && (KSSPLAY_get_fade_flag(kssplay) == KSSPLAY_FADE_NONE) ) {
                                KSSPLAY_fade_start(kssplay, 1000); //1s fadeout when not looping
                            }
                            
                            /* If the fade is ended or the play is stopped, break */
                            if ( (KSSPLAY_get_fade_flag(kssplay) == KSSPLAY_FADE_END) || (KSSPLAY_get_stop_flag(kssplay)) ) nbBytes=0;
                            else nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                            
                            //NSLog(@"ptr:%d",m_voice_current_ptr[1]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                            
                            if (m_genNumVoicesChannels) {
                                for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                    
                                    if (m_voice_current_ptr[j])
                                        for (int i=(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT);i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                            m_voice_buff[j][i]=m_voice_buff[j][(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)-1];
                                        }
                                }
                            }
                            for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][i];
                                }
                            }
                            
                            if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (mLoopMode==0)&&(iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                            }
                        }
                        
                        if (mPlayType==MMP_WEBSID) {
                            uint8_t is_simple_sid_mode = !FileLoader::isExtendedSidFile();
                            uint8_t speed =    FileLoader::getCurrentSongSpeed();
                            
                            // limit "skipping" so as not to make the browser unresponsive
                            for (uint16_t i = 0; i < websid_skip_silence_loop; i++)
                            {
                                Core::runOneFrame(is_simple_sid_mode, speed, buffer_ana[buffer_ana_gen_ofs],
                                                  websid_scope_buffers, SOUND_BUFFER_SIZE_SAMPLE);
                                mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                                
                                if (websid_sound_started || SID::isAudible())
                                {
                                    websid_sound_started = 1;
                                    break;
                                }
                            }
                            Postprocess::applyStereoEnhance(buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE);
                            SidSnapshot::record();
                            
                            nbBytes=SOUND_BUFFER_SIZE_SAMPLE*4;
                            
                            //copy voice data for oscillo view
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=websid_scope_buffers[j][i]>>8;
                                }
                            }
                            
                            if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (mLoopMode==0)&&(iModuleLength>0)&&(mCurrentSamples>mTgtSamples)) ) {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                
                                nbBytes=0;
                                memset(m_voice_buff_ana[buffer_ana_gen_ofs],0,SOUND_BUFFER_SIZE_SAMPLE*SOUND_MAXVOICES_BUFFER_FX);
                                
                            } else if (iModuleLength<0) {
                                if (mSIDForceLoop&&(mCurrentSamples>=mTgtSamples)) {
                                    //loop
                                    websid_loader->initTune(PLAYBACK_FREQ,mod_currentsub);
                                    Postprocess::init(SOUND_BUFFER_SIZE_SAMPLE, PLAYBACK_FREQ);
                                    SidSnapshot::init(SOUND_BUFFER_SIZE_SAMPLE, SOUND_BUFFER_SIZE_SAMPLE);
                                    mCurrentSamples=0;
                                    m_voice_current_sample=0;
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                }
                            }
                        }
                        
                        
                        if (mPlayType==MMP_SIDPLAY) { //SID
                            
                            m_voice_current_sample=0;
                            nbBytes=mSidEmuEngine->play(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE*2*1)*2;
                            mCurrentSamples+=nbBytes/4;
                            //m_voice_current_sample+=nbBytes/4;
                            //copy voice data for oscillo view
                            
                            for (int j=0;j<m_genNumVoicesChannels;j++) {
                                for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*2-1)];
                                }
                                m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                if ((m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=(SOUND_BUFFER_SIZE_SAMPLE*2)) m_voice_prev_current_ptr[j]-=(SOUND_BUFFER_SIZE_SAMPLE*2<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                            }
                            
                            if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (mLoopMode==0)&&(iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                            } else if (iModuleLength<0) {
                                if (mSIDForceLoop&&(mCurrentSamples>=mTgtSamples)) {
                                    //loop
                                    mSidTune->selectSong(mod_currentsub+1);
                                    mSidEmuEngine->load(mSidTune);
                                    mCurrentSamples=0;
                                    m_voice_current_sample=0;
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                }
                            }
                            
                        }
                        if (mPlayType==MMP_STSOUND) { //STSOUND
                            int nbSample = SOUND_BUFFER_SIZE_SAMPLE;
                            if (ymMusicComputeStereo((void*)ymMusic,(ymsample*)buffer_ana[buffer_ana_gen_ofs],nbSample)==YMTRUE) { nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                            } else {
                                nbBytes=0;
                            }
                            
                            //copy voice data for oscillo view
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<m_genNumVoicesChannels;j++) {
                                    m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][i];
                                    m_voice_buff[j][i]=0;
                                }
                            }
                        }
                        if (mPlayType==MMP_PIXEL) {
                            if (pixel_organya_mode) {
                                if (!org_gensamples((char*)(buffer_ana[buffer_ana_gen_ofs]), SOUND_BUFFER_SIZE_SAMPLE)) {
                                    nbBytes=0;
                                } else {
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                    mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                                    
                                    for (int j=0;j<m_genNumVoicesChannels;j++) {
                                        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)];
                                        }
                                        m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                        if ((m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=(SOUND_BUFFER_SIZE_SAMPLE*4*2)) m_voice_prev_current_ptr[j]-=(SOUND_BUFFER_SIZE_SAMPLE*4*2<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                                    }
                                    
                                }
                            } else {
                                if (!pixel_pxtn->Moo(buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE*2*2)) {
                                    nbBytes=0;
                                }
                                else {
                                    nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                    mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                                    
                                    //copy voice data for oscillo view
                                    
                                    for (int j=0;j<m_genNumVoicesChannels;j++) {
                                        for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE*4*2-1)];
                                        }
                                        m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                        if ((m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=(SOUND_BUFFER_SIZE_SAMPLE*4*2)) m_voice_prev_current_ptr[j]-=(SOUND_BUFFER_SIZE_SAMPLE*4*2<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                                    }
                                }
                            }
                            
                        }
                        if (mPlayType==MMP_EUP) { //EUP
                            int nbSample = SOUND_BUFFER_SIZE_SAMPLE*2; //stereo
                            if (eup_player->isPlaying()) {
                                int smpl_available=0;
                                if (eup_pcm.read_pos<=eup_pcm.write_pos) smpl_available=eup_pcm.write_pos-eup_pcm.read_pos;
                                else smpl_available=streamAudioBufferSamples-eup_pcm.read_pos+eup_pcm.write_pos;
                                
                                while (smpl_available<nbSample) {
                                    if (eup_player->isPlaying()) eup_player->nextTick();
                                    else {
                                        nbBytes=0;
                                        break;
                                    }
                                    
                                    if (eup_pcm.read_pos<=eup_pcm.write_pos) smpl_available=eup_pcm.write_pos-eup_pcm.read_pos;
                                    else smpl_available=streamAudioBufferSamples-eup_pcm.read_pos+eup_pcm.write_pos;
                                }
                                
                                for (int j=0;j<nbSample;j++) {
                                    buffer_ana[buffer_ana_gen_ofs][j]=((int16_t *) (eup_pcm.buffer))[eup_pcm.read_pos++];
                                    if (eup_pcm.read_pos>=streamAudioBufferSamples) eup_pcm.read_pos=0;
                                }
                                
                                
                                //copy voice data for oscillo view
                                for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                    int64_t voice_data_ofs=(m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                                    
                                    for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                        m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][voice_data_ofs&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)];
                                        m_voice_buff[j][voice_data_ofs&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)]=0;
                                        voice_data_ofs++;
                                    }
                                    m_voice_prev_current_ptr[j]+=SOUND_BUFFER_SIZE_SAMPLE<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                                    if ((m_voice_prev_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=(SOUND_BUFFER_SIZE_SAMPLE*2*4)) m_voice_prev_current_ptr[j]-=(SOUND_BUFFER_SIZE_SAMPLE*2*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
                                }
                                
                                
                                nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                            } else nbBytes=0;
                        }
                        if (mPlayType==MMP_ATARISOUND) { //ATARISOUND
                            if (m_genNumVoicesChannels) {
                                for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                    memset(m_voice_buff[j],0,SOUND_BUFFER_SIZE_SAMPLE);
                                    m_voice_current_ptr[j]=0;
                                }
                            }
                            
                            int retAtari=atariSndh.AudioRender(buffer_ana[buffer_ana_gen_ofs],SOUND_BUFFER_SIZE_SAMPLE,atariWaveData);
                            nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                            mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                            for (int i=SOUND_BUFFER_SIZE_SAMPLE-1;i>=0;i--) {
                                buffer_ana[buffer_ana_gen_ofs][i*2+1]=buffer_ana[buffer_ana_gen_ofs][i];
                                buffer_ana[buffer_ana_gen_ofs][i*2]=buffer_ana[buffer_ana_gen_ofs][i];
                                
                                //copy voice data for oscillo view
                                m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+0]=(signed char)(atariWaveData[i]&0xFF);
                                m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+1]=(signed char)((atariWaveData[i]>>8)&0xFF);
                                m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+2]=(signed char)((atariWaveData[i]>>16)&0xFF);
                                m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+3]=(signed char)((atariWaveData[i]>>24)&0xFF);
                            }
                            //if (loopCnt&&(mLoopMode==0)) nbBytes=0;
                            
                            if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                            }
                            
                        }
                        if (mPlayType==MMP_NSFPLAY) {
                            
                            for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                memset(m_voice_buff[j],0,SOUND_BUFFER_SIZE_SAMPLE);
                                m_voice_current_ptr[j]=0;
                            }
                            
                            
                            nbBytes=nsfPlayer->Render(buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE);
                            nbBytes*=4;
                            mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                            
                            if (nsfPlayer->IsDetected()||nsfPlayer->IsStopped()) {
                                //end reached
                                [self setSongLengthfromMD5:mod_currentsub-mod_minsub+1 songlength:iCurrentTime];
                                
                                if (iModuleLength<0) {
                                    if (nsfPlayer->IsDetected()||nsfPlayer->IsStopped()) {
                                        //loop
                                        //reset NSF
                                        nsfPlayer->Reset();
                                        mCurrentSamples=0;
                                        m_voice_current_sample=0;
                                        nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                    }
                                } else nbBytes=0;
                            }
                            
                            //copy voice data for oscillo view
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<m_genNumVoicesChannels;j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE-1)];
                                }
                            }
                            
                            if ((nbBytes==0)||( (iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                            } else {
                                
                            }
                        }
                        if (mPlayType==MMP_PT3) { //PT3
                            nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                            
                            if (m_genNumVoicesChannels) {
                                for (int j=0;j<(m_genNumVoicesChannels<SOUND_MAXVOICES_BUFFER_FX?m_genNumVoicesChannels:SOUND_MAXVOICES_BUFFER_FX);j++) {
                                    memset(m_voice_buff[j],0,SOUND_BUFFER_SIZE_SAMPLE);
                                    m_voice_current_ptr[j]=0;
                                }
                            }
                            
                            for (int ch = 0; ch<pt3_numofchips; ch++) {
                                if (pt3_renday(pt3_tmpbuf[ch], SOUND_BUFFER_SIZE_SAMPLE*4, &pt3_ay[ch], &pt3_t, ch,mLoopMode)) {
                                    //printf("PT3: loop/end point reached\n");
                                    if (mLoopMode) nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                                    else nbBytes=0;
                                }
                            }
                            mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                            for (int j=0; j<SOUND_BUFFER_SIZE_SAMPLE*2; j++) { //stereo = 16bit x 2
                                int tv=0;
                                for (int ch = 0; ch < pt3_numofchips; ch++) { //collecting
                                    tv += *(int16_t*)(pt3_tmpbuf[ch]+j);
                                    //m_voice_buff_ana[buffer_ana_gen_ofs][ch*SOUND_MAXVOICES_BUFFER_FX+j]=
                                }
                                buffer_ana[buffer_ana_gen_ofs][j] = (short)(tv/pt3_numofchips);
                            }
                            
                            //copy voice data for oscillo view
                            for (int i=0;i<SOUND_BUFFER_SIZE_SAMPLE;i++) {
                                for (int j=0;j<pt3_numofchips*3;j++) { m_voice_buff_ana[buffer_ana_gen_ofs][i*SOUND_MAXVOICES_BUFFER_FX+j]=m_voice_buff[j][(i+(m_voice_current_ptr[j]>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT))&(SOUND_BUFFER_SIZE_SAMPLE-1)];
                                }
                            }
                            
                            
                        }
                        if (mPlayType==MMP_SC68) {//SC68
                            nbBytes=SOUND_BUFFER_SIZE_SAMPLE;//*2*2;
                            int code = sc68_process(sc68, buffer_ana[buffer_ana_gen_ofs], &nbBytes);
                            nbBytes=SOUND_BUFFER_SIZE_SAMPLE*2*2;
                            
                            mCurrentSamples+=SOUND_BUFFER_SIZE_SAMPLE;
                            
                            if (code==SC68_ERROR) nbBytes=0;
                            if (code & SC68_END) nbBytes=0;
                            //if (code & API68_LOOP) nbBytes=0;
                            //if (code & API68_CHANGE) nbBytes=0;
                            
                            if ((nbBytes==0)||( (iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                            }
                            
                        }
                        if (mPlayType==MMP_ASAP) { //ASAP
                            if (ASAPInfo_GetChannels(ASAP_GetInfo(asap))==2) {
                                nbBytes= ASAP_Generate(asap, (unsigned char *)buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE*2*2, ASAPSampleFormat_S16_L_E);
                            } else {
                                nbBytes= ASAP_Generate(asap, (unsigned char *)buffer_ana[buffer_ana_gen_ofs], SOUND_BUFFER_SIZE_SAMPLE*2, ASAPSampleFormat_S16_L_E);
                                short int *buff=(short int*)(buffer_ana[buffer_ana_gen_ofs]);
                                for (int i=nbBytes-1;i>0;i--) {
                                    buff[i]=buff[i>>1];
                                }
                                nbBytes*=2;
                            }
                            
                            if ((nbBytes<SOUND_BUFFER_SIZE_SAMPLE*2*2)||( (iModuleLength>0)&&(iCurrentTime>iModuleLength)) ) {
                                if (mSingleSubMode==0) {
                                    if ([self playNextSub]<0) nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                    else {
                                        nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
                                        donotstop=1;
                                        moveToSubSong=2;
                                    }
                                } else nbBytes=(nbBytes==SOUND_BUFFER_SIZE_SAMPLE*2*2?nbBytes-4:nbBytes);
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
                            if (donotstop) donotstop=0;
                            else bGlobalEndReached=1;
                        }
                        if (bGlobalEndReached) buffer_ana_flag[buffer_ana_gen_ofs]=buffer_ana_flag[buffer_ana_gen_ofs]|4; //end reached
                        
                        buffer_ana_gen_ofs++;
                        if (buffer_ana_gen_ofs==SOUND_BUFFER_NB) buffer_ana_gen_ofs=0;
                    }
                }
                bGlobalSoundGenInProgress=0;
            }
            if (bGlobalShouldEnd) break;
            
        }
        //NSLog(@"out");
        //Tell our callback what we've done
        //    [self performSelectorOnMainThread:@selector(loadComplete) withObject:fileName waitUntilDone:NO];
        
        //remove our pool and free the memory collected by it
        //    tim_close();
        //[pool release];
    }
}

//*****************************************
//Archive management

- (void) observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                       context:(void *)context
{
    if (context == LoadingProgressObserverContext) {
        NSProgress *progress = object;
        
        if ([progress isCancelled]) {
            //NSLog(@"modizemusicplayer extract cancelled");
            extractDone=true;
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{

            }];
        }
#if DEBUG_MODIZER
        //NSLog(@"extract progress: %lf",progress.fractionCompleted);
#endif
        if (progress.fractionCompleted>=1.0f) extractDone=true;
        
        dispatch_async(dispatch_get_main_queue(), ^(void){
            //Run UI Updates
//            UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
//            mdz_safe_execute_sel(vc,@selector(setProgressWaiting:),[NSNumber numberWithDouble:progress.fractionCompleted])
            [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithDouble:progress.fractionCompleted]];
            if (progress.fractionCompleted>=1.0f) {
                [detailViewControllerIphone setProgressWaiting:[NSNumber numberWithDouble:1]];
                //mdz_safe_execute_sel(vc,@selector(hideWaiting),nil)
                //mdz_safe_execute_sel(vc,@selector(hideWaitingProgress),nil)
                //mdz_safe_execute_sel(vc,@selector(setProgressWaiting),progress.fractionCompleted)
            }
        });
                            
//        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
//            UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
//            mdz_safe_execute_sel(vc,@selector(setProgressWaiting:),[NSNumber numberWithDouble:progress.fractionCompleted])
//            //mdz_safe_execute_sel(vc,@selector(hideWaiting),nil)
//            //[self.waitingView setProgress:progress.fractionCompleted];
//            if (progress.fractionCompleted>=1.0f) {
//                mdz_safe_execute_sel(vc,@selector(hideWaiting),nil)
//                mdz_safe_execute_sel(vc,@selector(hideWaitingProgress),nil)
//                //mdz_safe_execute_sel(vc,@selector(setProgressWaiting),progress.fractionCompleted)
////                [self hideWaiting];
////                [self hideWaitingProgress];
////                [self.tableView setUserInteractionEnabled:true];
//            }
//        }];
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}


-(int) extractToPath:(const char *)archivePath path:(const char *)extractPath isRestarting:(bool)isRestarting {
    int ret=0;
    [ModizFileHelper scanarchive:archivePath filesList_ptr:&mdz_ArchiveFilesList filesCount_ptr:&mdz_ArchiveFilesCnt];
        
    if (!isRestarting) {
        extractProgress = [NSProgress progressWithTotalUnitCount:1];
        extractProgress.cancellable = YES;
        extractProgress.pausable = NO;
        extractDone=false;
        extractPendingCancel=false;
        
        [ModizFileHelper extractToPath:archivePath path:extractPath caller:self progress:extractProgress context:LoadingProgressObserverContext];
        while (extractDone==false) {
            [NSThread sleepForTimeInterval:0.01f];
            if ([self extractPendingCancel]) {
                extractPendingCancel=false;
                [extractProgress cancel];
                ret=-1;
            }
        }
    }
    return ret;
#if 0
    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    int r;
    ssize_t size;
    
    bool cancelExtract;
    FILE *f;
    NSString *extractFilename,*extractPathFile;
    NSError *err;
    int idx,idxAll;
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);
    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, archivePath, 16384);
    
    if (r==ARCHIVE_OK) {
        mdz_ArchiveFilesList=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*)); //TODO: free
        //            mdz_ArchiveFilesListAlias=(char**)malloc(mdz_ArchiveFilesCnt*sizeof(char*)); //TODO: free
        idx=0;idxAll=0;
        for (;;) {
            r = archive_read_next_header(a, &entry);
            
            if (r == ARCHIVE_EOF) break;
            if (r != ARCHIVE_OK) {
                NSLog(@"archive_read_next_header() %s", archive_error_string(a));
                break;
            }
            
            NSString *tmp_filename=[ModizFileHelper getCorrectFileName:archivePath archive:a entry:entry];
            
            if ([ModizFileHelper isAcceptedFile:tmp_filename no_aux_file:0]) {
                
                extractFilename=[NSString stringWithFormat:@"%s/%@",extractPath,tmp_filename];
                extractPathFile=[extractFilename stringByDeletingLastPathComponent];
                
                if (!isRestarting) {
                    //1st create path if not existing yet
                    [mFileMngr createDirectoryAtPath:extractPathFile withIntermediateDirectories:TRUE attributes:nil error:&err];
                    [self addSkipBackupAttributeToItemAtPath:extractPathFile];
                    
                    //2nd extract file
                    f=fopen([extractFilename fileSystemRepresentation],"wb");
                    if (!f) {
                        NSLog(@"Cannot open %@ to extract %@",extractFilename,extractPathFile);
                    } else {
                        const void *buff;
                        size_t size;
                        la_int64_t offset;
                        
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
                        
                        for (;;) {
                            r = archive_read_data_block(a, &buff, &size, &offset);
                            if (r == ARCHIVE_EOF) break;
                            if (r < ARCHIVE_OK) break;
                            fwrite(buff,size,1,f);
                        }
                        
                        fclose(f);
                        
                        if ([ModizFileHelper isAcceptedFile:tmp_filename no_aux_file:1]) {
                            mdz_ArchiveFilesList[idx]=(char*)malloc(strlen([tmp_filename fileSystemRepresentation])+1);
                            strcpy(mdz_ArchiveFilesList[idx],[tmp_filename fileSystemRepresentation]);
                            
                            idx++;
                        }
                    }
                } else {
                    //restarting, skip extract
                    
                    if ([ModizFileHelper isAcceptedFile:tmp_filename no_aux_file:1]) {
                        mdz_ArchiveFilesList[idx]=(char*)malloc(strlen([tmp_filename fileSystemRepresentation])+1);
                        strcpy(mdz_ArchiveFilesList[idx],[tmp_filename fileSystemRepresentation]);
                        
                        idx++;
                    }
                }
            }
        }
    }
    r = archive_read_free(a);
#endif
}

-(NSString*) getArcEntryFilename:(const char *)path index:(int)idx {
    struct archive *a = archive_read_new();
    struct archive_entry *entry;
    int r;
    NSString *result=NULL;
    
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);
    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, path, 16384);
    
    if (r==ARCHIVE_OK) {
        for (;;) {
            r = archive_read_next_header(a, &entry);
            if (r!=ARCHIVE_OK) break;
            NSString *tmp_filename=[ModizFileHelper getCorrectFileName:path archive:a entry:entry];
            
            if ([ModizFileHelper isAcceptedFile:tmp_filename no_aux_file:1]) {
                if (!idx) {
                    result=tmp_filename;
                    break;
                }
                idx--;
            }
        }
    }
        
    r = archive_read_free(a);
    return result;
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
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
    NSMutableArray *filetype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE];
    NSMutableArray *filetype_extAMIGA=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE_AMIGA];
    
    int err;
    int local_nb_entries=0;
    
    NSRange r;
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    
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
            //[temparray_filepath removeLastObject];
            file_no_ext=[temparray_filepath firstObject];
            
            
            if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
            else if ([filetype_extAMIGA indexOfObject:file_no_ext]!=NSNotFound) found=1;
            
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
                //[temparray_filepath removeLastObject];
                file_no_ext=[temparray_filepath firstObject];
                
                
                if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                else if ([filetype_extAMIGA indexOfObject:file_no_ext]!=NSNotFound) found=1;
                
                
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

-(void) mmp_updateDBStatsAtLoad {
    if (mdz_ArchiveFilesCnt) {
        //update file
        NSString *fpath=[NSString stringWithFormat:@"%@@%d",[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath],mdz_currentArchiveIndex];
        DBHelper::updateFileStatsDBmod([self getArcEntryTitle:mdz_currentArchiveIndex],fpath,-1,-1,-1,iModuleLength,numChannels,mod_subsongs);
        
        //if only 1 archive entry, update it as well
        if (mdz_ArchiveFilesCnt==1) DBHelper::updateFileStatsDBmod([[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath] lastPathComponent],[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath],-1,-1,-1,iModuleLength,numChannels,mod_subsongs);
        else { //archive with multiple entries, reset stats if needed
            DBHelper::updateFileStatsDBmod([[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath] lastPathComponent],[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath],-1,-1,-1,0,0,-mdz_ArchiveFilesCnt);
        }
    } else { //not an archive
        DBHelper::updateFileStatsDBmod([[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath] lastPathComponent],[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath],-1,-1,-1,iModuleLength,numChannels,mod_subsongs);
    }
}

-(void) mmp_updateDBStatsAtLoadSubsong:(int)mod_total_length {
    if (mdz_ArchiveFilesCnt) {
        //update file
        NSString *fpath=[NSString stringWithFormat:@"%@@%d",[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath],mdz_currentArchiveIndex];
        DBHelper::updateFileStatsDBmod([self getArcEntryTitle:mdz_currentArchiveIndex],fpath,-1,-1,-1,mod_total_length,numChannels,mod_subsongs);
        
        //if only 1 archive entry, update it as well
        if (mdz_ArchiveFilesCnt==1) DBHelper::updateFileStatsDBmod([[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath] lastPathComponent],[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath],-1,-1,-1,mod_total_length,numChannels,mod_subsongs);
        else { //archive with multiple entries, reset stats if needed
            DBHelper::updateFileStatsDBmod([[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath] lastPathComponent],[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath],-1,-1,-1,0,0,-mdz_ArchiveFilesCnt);
        }
    } else { //not an archive
        DBHelper::updateFileStatsDBmod([[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath] lastPathComponent],[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath],-1,-1,-1,mod_total_length,numChannels,mod_subsongs);
    }
}


-(int) mmp_gsfLoad:(NSString*)filePath {  //GSF
    mPlayType=MMP_GSF;
    FILE *f;
    char length_str[256], fade_str[256], title_str[256], game_str[256];
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
    
    sprintf(mod_message,"\n");
    
    if (!psftag_getvar(tag, "title", title_str, sizeof(title_str)-1)) {
        //BOLD(); printf("Title: "); NORMAL();
        sprintf(mod_message,"%sTitle......: %s\n",mod_message,title_str);
        //printf("%s\n", title_str);
    }
    
    if (!psftag_getvar(tag, "artist", tmp_str, sizeof(tmp_str)-1)) {
        //BOLD(); printf("Artist: "); NORMAL();
        //printf("%s\n", tmp_str);
        sprintf(mod_message,"%sArtist.....: %s\n",mod_message,tmp_str);
        artist=[NSString stringWithUTF8String:tmp_str];
    }
    
    if (!psftag_getvar(tag, "game", game_str, sizeof(tmp_str)-1)) {
        //BOLD(); printf("Game: "); NORMAL();
        //printf("%s\n", tmp_str);
        sprintf(mod_message,"%sGame.......: %s\n",mod_message,game_str);
        album=[NSString stringWithFormat:@"%s",game_str];
    }
    
    if (!psftag_getvar(tag, "year", tmp_str, sizeof(tmp_str)-1)) {
        //BOLD(); printf("Year: "); NORMAL();
        //printf("%s\n", tmp_str);
        sprintf(mod_message,"%sYear.......: %s\n",mod_message,tmp_str);
    }
    
    if (!psftag_getvar(tag, "copyright", tmp_str, sizeof(tmp_str)-1)) {
        //BOLD(); printf("Copyright: "); NORMAL();
        //printf("%s\n", tmp_str);
        sprintf(mod_message,"%sCopyright..: %s\n",mod_message,tmp_str);
    }
    
    if (!psftag_getvar(tag, "gsfby", tmp_str, sizeof(tmp_str)-1)) {
        //BOLD(); printf("GSF By: "); NORMAL();
        //printf("%s\n", tmp_str);
        sprintf(mod_message,"%sGSFby......: %s\n",mod_message,tmp_str);
    }
    
    if (!psftag_getvar(tag, "tagger", tmp_str, sizeof(tmp_str)-1)) {
        //BOLD(); printf("Tagger: "); NORMAL();
        //printf("%s\n", tmp_str);
        sprintf(mod_message,"%sTagger.....: %s\n",mod_message,tmp_str);
    }
    
    if (!psftag_getvar(tag, "comment", tmp_str, sizeof(tmp_str)-1)) {
        //BOLD(); printf("Comment: "); NORMAL();
        //printf("%s\n", tmp_str);
        sprintf(mod_message,"%sComment....: %s\n",mod_message,tmp_str);
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
    m_genNumVoicesChannels=numChannels;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[i/4];
    }
    m_voicesDataAvail=1;
    
    mod_name[0]=0;
    if (game_str[0]) sprintf(mod_name," %s",game_str);
    
    if (title_str[0]) mod_title=[NSString stringWithUTF8String:title_str];
    
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
        if (tmp_mod_name) snprintf(mod_name,sizeof(mod_name)," %s",tmp_mod_name);
        else snprintf(mod_name,sizeof(mod_name)," %s",mod_filename);
        
        decode_pos_ms = 0;
        seek_needed = -1;
        MDXshoudlReset=0;
        
        mod_subsongs=1;
        mod_minsub=1;
        mod_maxsub=1;
        mod_currentsub=1;
        
        iModuleLength=mdx_get_length( mdx,pdx);
        if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//MDX_DEFAULT_LENGTH;
        iCurrentTime=0;
        
        numChannels=(mdx->haspdx?9:8);//mdx->tracks;
        
        NSString *filePathDoc=[ModizFileHelper getFilePathFromDocuments:filePath];
        
        [self mmp_updateDBStatsAtLoad];
        
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
        
        m_voicesDataAvail=1;
        m_genNumVoicesChannels=numChannels;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[i/8];
        }
        
        return 0;
    }
}
-(int) mmp_AtariSoundLoad:(NSString*)filePath {  //ATARISOUND
    mPlayType=MMP_ATARISOUND;
    
    FILE *f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"SC68 Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    mp_data=(char*)malloc(mp_datasize);
    if (!mp_data) {
        NSLog(@"AtariSound: cannot allocate %dKo to load file\n",mp_datasize>>10);
        fclose(f);
        return  -1;
    }
    fseek(f,0L,SEEK_SET);
    fread(mp_data, 1, mp_datasize, f);
    fclose(f);
    
    
    
    
    if (!(atariSndh.Load(mp_data,mp_datasize,PLAYBACK_FREQ))) {
        NSLog(@"AtariSound load error");
        mPlayType=0;
        free(mp_data);
        return -1;
    } else {
        SndhFile::SubSongInfo info;
        uint8_t *sndhData=(uint8_t *)atariSndh.GetRawData();
        int sndhDataLen=atariSndh.GetRawDataSize();
        
        //compute hash same way sc68 does to use their timedb table
        //
        atariSndh_hash=0;
        for (int i=0;i<8+24;i++) {
            atariSndh_hash += sndhData[i];
            atariSndh_hash += atariSndh_hash << 10;
            atariSndh_hash ^= atariSndh_hash >> 6;
        }
        for (int i=0;i<sndhDataLen;i++) {
            atariSndh_hash += sndhData[i];
            atariSndh_hash += atariSndh_hash << 10;
            atariSndh_hash ^= atariSndh_hash >> 6;
        }
        
        
        
        mod_subsongs=atariSndh.GetSubsongCount();
        mod_minsub=1;
        mod_maxsub=1+mod_subsongs-1;
        mod_currentsub=atariSndh.GetDefaultSubsong();
        
        numChannels=4;
        
        if (mod_currentsub<mod_minsub) mod_currentsub=mod_minsub;
        if (mod_currentsub>mod_maxsub) mod_currentsub=mod_maxsub;
        atariSndh.GetSubsongInfo(mod_currentsub, info);
        
        atariSndh.InitSubSong(mod_subsongs);
        
        if (info.musicSubTitle) sprintf(mod_name," %s",info.musicSubTitle);
        else if (info.musicTitle) sprintf(mod_name," %s",info.musicTitle);
        
        iModuleLength=info.playerTickCount*1000/info.playerTickRate;
        
        if (iModuleLength<=0) {
            unsigned int frames;
            int flags;
            if (timedb68_get(atariSndh_hash,mod_currentsub-1,&frames,&flags)>=0) {
                iModuleLength=frames*1000/info.playerTickRate;
            }
        }
        
        if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
        iCurrentTime=0;
        
        //////////////////////////////////
        //update DB with songlength
        //////////////////////////////////
        for (int i=0;i<mod_subsongs; i++) {
            atariSndh.GetSubsongInfo(i+1, info);
            
            int subsong_length=info.playerTickCount*1000/info.playerTickRate;
            
            if (subsong_length<=0) {
                unsigned int frames;
                int flags;
                if (timedb68_get(atariSndh_hash,mod_currentsub-1,&frames,&flags)>=0) {
                    subsong_length=frames*1000/info.playerTickRate;
                }
            }
            
            if (subsong_length<=0) subsong_length=optGENDefaultLength;
            mod_total_length+=subsong_length;
            
            int song_length;
            NSString *filePathMain;
            NSString *fileName=[self getSubTitle:i];
            
            filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
            if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
            
            NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
            DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,subsong_length,numChannels,mod_subsongs);
            
            
            if (i==mod_subsongs-1) {// Global file stats update
                [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
            }
        }
        
        if (info.musicSubTitle) sprintf(mod_message,"Title..........: %s\nSubsong title..: %s\nArtist.........: %s\nYear.........: %s\nRipper.........: %s\nConverter......: %s\n",
                                        info.musicTitle,info.musicSubTitle,info.musicAuthor,info.year,info.ripper,info.converter);
        else sprintf(mod_message,"Title.....: %s\nArtist....: %s\nYear......: %s\nRipper....: %s\nConverter.: %s\n",
                     info.musicTitle,info.musicAuthor,info.year,info.ripper,info.converter);
        
        
        artist=[NSString stringWithUTF8String:info.musicAuthor];
        
        atariWaveData=(uint32_t*)malloc(SOUND_BUFFER_SIZE_SAMPLE*4);
        m_voicesDataAvail=1;
        m_genNumVoicesChannels=numChannels;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[i/3];
        }
        
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
    
    
    if (sc68_load_uri(sc68,[filePath UTF8String])) {
        NSLog(@"SC68 api68_load_file error");
        mPlayType=0;
        return -1;
    } else {
        sc68_music_info_t info;
        
        // Set track and loop (optionnal).
        int ret=sc68_play(sc68,1/*SC68_DEF_TRACK*/,(mLoopMode?SC68_INF_LOOP:0));
        ret=sc68_process(sc68, buffer_ana[buffer_ana_gen_ofs], 0); //to apply the track change
        sc68_music_info(sc68,&info,SC68_CUR_TRACK,0);
        ret=sc68_play(sc68,info.dsk.track,(mLoopMode?SC68_INF_LOOP:0)); //change for default track
        sc68_music_info(sc68,&info,SC68_CUR_TRACK,0);
        ret=sc68_process(sc68, buffer_ana[buffer_ana_gen_ofs], 0); //to apply the track change
        if (info.title[0]) sprintf(mod_name," %s",info.title);
        else sprintf(mod_name," %s",mod_filename);
        
        mod_subsongs=info.tracks;
        mod_minsub=1;
        mod_maxsub=1+info.tracks-1;
        mod_currentsub=sc68_cntl(sc68,SC68_GET_TRACK);
        numChannels=2;
        
        
        if (mod_currentsub<mod_minsub) mod_currentsub=mod_minsub;
        if (mod_currentsub>mod_maxsub) mod_currentsub=mod_maxsub;
        
        iModuleLength=info.trk.time_ms;
        
        if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
        iCurrentTime=0;
        
        //////////////////////////////////
        //update DB with songlength
        //////////////////////////////////
        for (int i=0;i<mod_subsongs; i++) {
            sc68_music_info_t info_sub;
            sc68_music_info( sc68, &info_sub, i+1, NULL );
            
            int subsong_length=info_sub.trk.time_ms;
            if (subsong_length<=0) subsong_length=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
            mod_total_length+=subsong_length;
            
            NSString *filePathMain;
            NSString *fileName=[self getSubTitle:i];
            
            filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
            if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
            
            NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
            DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,subsong_length,numChannels,mod_subsongs);
            
            if (i==mod_subsongs-1) {// Global file stats update
                [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
            }
        }
        
        
        sprintf(mod_message,"Title.....: %s\nArtist....: %s\nFormat....: %s\nHardware..: %s\nConverter.: %s\nRipper....: %s\n",
                info.title,info.artist,info.format,info.dsk.hw,info.converter,info.ripper);
        artist=[NSString stringWithUTF8String:info.artist];
        
        return 0;
    }
    
}

-(int) mmp_eupLoad:(NSString*)filePath{
    mPlayType=MMP_EUP;
    g_playing = 0;
    FILE *f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"EUP Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);
    
    
    eup_dev = new EUP_TownsEmulator;
    eup_player = new EUPPlayer();
    /* signed 16 bit sample, little endian */
    eup_dev->rate(streamAudioRate); /* Hz */
    
    eup_player->outputDevice(eup_dev);
    
    /* re-inizialize pcm struct */
    memset(&eup_pcm, 0, sizeof(eup_pcm));
    eup_pcm.on = true;
    eup_pcm.read_pos = 0;
    
    
    eup_buf = EUPPlayer_readFile(eup_player, eup_dev, [filePath UTF8String],&eup_header);
    if (eup_buf == nullptr) {
        fprintf(stderr, "%s: read failed\n", [filePath UTF8String]);
        std::fflush(stderr);
        return -1;
    }
    
    eup_player->startPlaying(eup_buf + 2048 + 6);
    
    //compute length
    mCurrentSamples=0;
    if (eup_player->isPlaying()) {
        while (1) {
            if (eup_player->isPlaying()) eup_player->nextTick(true);
            else break;
            
            mCurrentSamples+=eup_pcm.write_pos/2;
            eup_pcm.write_pos=0;
            if (mCurrentSamples>streamAudioRate*60*30) break; //if more than 30min, exit
        }
        
    }
    if (mCurrentSamples>streamAudioRate*60*30) iModuleLength=-1;
    else iModuleLength=mCurrentSamples*1000/streamAudioRate;
    
    eup_player->stopPlaying();
    
    strcpy(eup_filename,[filePath UTF8String]);
    
    //Reset player
    EUPPlayer_ResetReload();
    
    mCurrentSamples=0;
    eup_mutemask=0;
    
    mod_subsongs=1;
    mod_minsub=1;
    mod_maxsub=1;
    mod_currentsub=1;
    if (mod_currentsub<mod_minsub) mod_currentsub=mod_minsub;
    if (mod_currentsub>mod_maxsub) mod_currentsub=mod_maxsub;
    if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
    
    iCurrentTime=0;
    
    // song info
    snprintf(mod_name,sizeof(mod_name)," %s",[[[filePath lastPathComponent] stringByDeletingPathExtension] UTF8String]);
    mod_title=[NSString stringWithUTF8String:mod_name];
    
    const char *title=[[self sjisToNS:eup_header.title] UTF8String];
    if (title[0]) strncpy(mod_name,title,sizeof(mod_name)-1);
    
    snprintf(mod_message,MAX_STIL_DATA_LENGTH*2,"Title.....: %s\nArtist....: %s\nAppli....: %s\n",[[self sjisToNS:eup_header.title] UTF8String],[[self sjisToNS:eup_header.artist] UTF8String],[[self sjisToNS:eup_header.appli_name] UTF8String]);
    artist=[NSString stringWithUTF8String:[[self sjisToNS:eup_header.artist] UTF8String]];
    
    numChannels=6+8;
    m_voicesDataAvail=1;
    m_genNumVoicesChannels=numChannels;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
        if (i<6) m_voice_voiceColor[i]=m_voice_systemColor[0];
        else m_voice_voiceColor[i]=m_voice_systemColor[1];
    }
    
    [self mmp_updateDBStatsAtLoad];
    
    if (mLoopMode) iModuleLength=-1;
    
    return 0;
}

typedef struct {
    const char md5[33];
    const char *option_id;
    int val_option;
} nsfplay_fixes_t;

-(int) mmp_nsfplayLoad:(NSString*)filePath {  //NSFPLAY
    mPlayType=MMP_NSFPLAY;
    
    FILE *f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"NSFPlay Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);
    
    
    nsfPlayer=new xgm::NSFPlayer();
    
    nsfPlayerConfig=new xgm::NSFPlayerConfig();
    (*nsfPlayerConfig)["RATE"]=44100;
    (*nsfPlayerConfig)["MASTER_VOLUME"]=256;
    
    (*nsfPlayerConfig)["LPF"]=settings[NSFPLAY_LowPass_Filter_Strength].detail.mdz_slider.slider_value;
    (*nsfPlayerConfig)["HPF"]=settings[NSFPLAY_HighPass_Filter_Strength].detail.mdz_slider.slider_value;
        
    (*nsfPlayerConfig)["REGION"]=settings[NSFPLAY_Region].detail.mdz_switch.switch_value;
    (*nsfPlayerConfig)["IRQ_ENABLE"]=settings[NSFPLAY_ForceIRQ].detail.mdz_boolswitch.switch_value;    
    (*nsfPlayerConfig)["QUALITY"]=settings[NSFPLAY_Quality].detail.mdz_slider.slider_value;
    
    (*nsfPlayerConfig)["APU1_OPTION0"]=settings[NSFPLAY_APU_OPTION0].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU1_OPTION1"]=settings[NSFPLAY_APU_OPTION1].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU1_OPTION2"]=settings[NSFPLAY_APU_OPTION2].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU1_OPTION3"]=settings[NSFPLAY_APU_OPTION3].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU1_OPTION4"]=settings[NSFPLAY_APU_OPTION4].detail.mdz_boolswitch.switch_value;
    
    (*nsfPlayerConfig)["APU2_OPTION0"]=settings[NSFPLAY_DMC_OPTION0].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU2_OPTION1"]=settings[NSFPLAY_DMC_OPTION1].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU2_OPTION2"]=settings[NSFPLAY_DMC_OPTION2].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU2_OPTION3"]=settings[NSFPLAY_DMC_OPTION3].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU2_OPTION4"]=settings[NSFPLAY_DMC_OPTION4].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU2_OPTION5"]=settings[NSFPLAY_DMC_OPTION5].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU2_OPTION6"]=settings[NSFPLAY_DMC_OPTION6].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU2_OPTION7"]=settings[NSFPLAY_DMC_OPTION7].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["APU2_OPTION8"]=settings[NSFPLAY_DMC_OPTION8].detail.mdz_boolswitch.switch_value;    
    
    (*nsfPlayerConfig)["N163_OPTION0"]=settings[NSFPLAY_N163_OPTION0].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["N163_OPTION1"]=settings[NSFPLAY_N163_OPTION1].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["N163_OPTION2"]=settings[NSFPLAY_N163_OPTION2].detail.mdz_boolswitch.switch_value;
    
    (*nsfPlayerConfig)["FDS_OPTION0"]=atoi(settings[NSFPLAY_FDS_OPTION0].detail.mdz_textbox.text);
    (*nsfPlayerConfig)["FDS_OPTION1"]=settings[NSFPLAY_FDS_OPTION1].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["FDS_OPTION2"]=settings[NSFPLAY_FDS_OPTION2].detail.mdz_boolswitch.switch_value;
    
    (*nsfPlayerConfig)["MMC5_OPTION0"]=settings[NSFPLAY_MMC5_OPTION0].detail.mdz_boolswitch.switch_value;
    (*nsfPlayerConfig)["MMC5_OPTION1"]=settings[NSFPLAY_MMC5_OPTION1].detail.mdz_boolswitch.switch_value;
    
    (*nsfPlayerConfig)["VRC7_PATCH"]=settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_value;
    (*nsfPlayerConfig)["VRC7_OPTION0"]=settings[NSFPLAY_VRC7_OPTION0].detail.mdz_boolswitch.switch_value;
        
    nsfPlayer->SetConfig(nsfPlayerConfig);
    
    nsfData=new xgm::NSF();
    
    if (mLoopMode) nsfData->SetDefaults(/*nsfData->default_playtime*/optNSFPLAYDefaultLength,0,1<<16);
    else nsfData->SetDefaults(/*nsfData->default_playtime*/optNSFPLAYDefaultLength,nsfData->default_fadetime,nsfData->default_loopnum);
    
    //check if a playlist exists
    const char *plfile=[[[filePath stringByDeletingPathExtension] stringByAppendingString:@".m3u"] UTF8String];
    f=fopen(plfile,"rb");
    if (!f) {
        plfile=[[[filePath stringByDeletingPathExtension] stringByAppendingString:@".M3U"] UTF8String];
        f=fopen(plfile,"rb");
    }
    m3uReader.clear();
    if (f) {
        fclose(f);
        m3uReader.load(plfile);
    }
    
    nsfData->LoadFile([filePath UTF8String]);
    nsfPlayer->Load(nsfData);
    
    nsfPlayer->SetPlayFreq(44100);
    nsfPlayer->SetChannels(2);
    
    if (m3uReader.size()) {
        
        mod_subsongs=m3uReader.size();
        mod_minsub=0;
        mod_maxsub=m3uReader.size()-1;
        mod_currentsub=0;
    } else {
        mod_subsongs=nsfData->GetSongNum();
        mod_minsub=0;
        mod_maxsub=mod_subsongs-1;
        mod_currentsub=nsfData->GetSong();
    }
    
    if (m3uReader.size()) {
        nsfPlayer->SetSong(m3uReader[mod_currentsub-mod_minsub].track);
    } else nsfPlayer->SetSong(mod_currentsub-mod_minsub);
    nsfPlayer->Reset();
    
    
    if (mod_currentsub<mod_minsub) mod_currentsub=mod_minsub;
    if (mod_currentsub>mod_maxsub) mod_currentsub=mod_maxsub;
    
    iModuleLength=nsfData->GetLength();
    if (mLoopMode) iModuleLength=-1;
    
    iCurrentTime=0;
    
    //////////////////////////////////
    //update DB with songlength
    //////////////////////////////////
    mod_total_length=0;
    
    for (int i=0;i<mod_subsongs; i++) {
        int song_length=0;
        NSString *filePathMain;
        NSString *fileName;
        NSMutableArray *tmp_path;
        
        if (m3uReader.size()) song_length=m3uReader[i].length;
        if (song_length<=0) {
            //set subsong
            nsfPlayer->SetSong(i+mod_minsub);
            
            song_length=nsfPlayer->GetLength();
        }
        mod_total_length+=song_length;
        
        fileName=[self getSubTitle:i];
        
        filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
        if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
        
        NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
        DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,song_length,numChannels,mod_subsongs);
        
        if (i==mod_subsongs-1) {// Global file stats update
            [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
        }
    }
    
    if (m3uReader.size()) {
        nsfPlayer->SetSong(m3uReader[mod_currentsub-mod_minsub].track);
    } else nsfPlayer->SetSong(mod_currentsub-mod_minsub);
    
    // song info
    if (m3uReader.size()) {
        mod_title=[NSString stringWithUTF8String:m3uReader[mod_currentsub-mod_minsub].name];
    } else mod_title=[NSString stringWithUTF8String:nsfData->GetTitleString("%L",mod_currentsub)];
    
    const char *nsfe_name=nsfData->GetTitleString("%T",mod_currentsub);
    if (nsfe_name[0]) snprintf(mod_name,sizeof(mod_name)," %s",nsfe_name);
    else snprintf(mod_name,sizeof(mod_name)," %s",[[[mod_currentfile lastPathComponent] stringByDeletingPathExtension] UTF8String]);
        
    artist=[NSString stringWithUTF8String:nsfData->artist];
    
    numChannels=0;
    memset(modizChipsetType,0,sizeof(modizChipsetType));
    modizChipsetCount=0;
    modizChipsetType[modizChipsetCount]=NES_APU;
    modizChipsetStartVoice[modizChipsetCount]=0;
    modizChipsetVoicesCount[modizChipsetCount]=5;
    
    snprintf(modizChipsetName[modizChipsetCount],16,"RP2A03");
    for (int i=numChannels;i<numChannels+5;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
    }
    numChannels+=5;
    modizChipsetCount++;
    
    snprintf(modizVoicesName[0],MODIZ_VOICE_NAME_MAX_CHAR,"Square 0");
    snprintf(modizVoicesName[1],MODIZ_VOICE_NAME_MAX_CHAR,"Square 1");
    snprintf(modizVoicesName[2],MODIZ_VOICE_NAME_MAX_CHAR,"Triangle");
    snprintf(modizVoicesName[3],MODIZ_VOICE_NAME_MAX_CHAR,"Noise");
    snprintf(modizVoicesName[4],MODIZ_VOICE_NAME_MAX_CHAR,"DMC");
    
    
    snprintf(mod_message,MAX_STIL_DATA_LENGTH*2,"Title....: %s\nArtist...: %s\nCopyright: %s\n",nsfData->GetTitleString("%T",-1),nsfData->artist,nsfData->copyright);
    if (nsfData->ripper) {
        strcat(mod_message,"Ripper...: ");
        strcat(mod_message,nsfData->ripper);
        strcat(mod_message,"\n");
    }
    
    if (nsfData->use_fds) {
        strcat(mod_message,"Mapper...: FDS\n");
        modizChipsetStartVoice[modizChipsetCount]=numChannels;
        modizChipsetType[modizChipsetCount]=NES_FDS;
        modizChipsetVoicesCount[modizChipsetCount]=1;
        snprintf(modizChipsetName[modizChipsetCount],16,"FDS");
        for (int i=numChannels;i<numChannels+modizChipsetVoicesCount[modizChipsetCount];i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
        }
        snprintf(modizVoicesName[numChannels],MODIZ_VOICE_NAME_MAX_CHAR,"FDS");
        numChannels+=modizChipsetVoicesCount[modizChipsetCount];
        modizChipsetCount++;
        
        
    }
    if (nsfData->use_fme7) {
        strcat(mod_message,"Mapper...: FME7\n");
        modizChipsetStartVoice[modizChipsetCount]=numChannels;
        modizChipsetType[modizChipsetCount]=NES_FME7;
        modizChipsetVoicesCount[modizChipsetCount]=modizChipsetVoicesCount[modizChipsetCount];
        snprintf(modizChipsetName[modizChipsetCount],16,"5B");
        for (int i=numChannels;i<numChannels+3;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
            snprintf(modizVoicesName[i],MODIZ_VOICE_NAME_MAX_CHAR,"5B:%d",i);
        }
        numChannels+=modizChipsetVoicesCount[modizChipsetCount];
        modizChipsetCount++;
    }
    if (nsfData->use_mmc5) {
        strcat(mod_message,"Mapper...: MMC5\n");
        modizChipsetStartVoice[modizChipsetCount]=numChannels;
        modizChipsetType[modizChipsetCount]=NES_MMC5;
        modizChipsetVoicesCount[modizChipsetCount]=3;
        snprintf(modizChipsetName[modizChipsetCount],16,"MMC5");
        for (int i=numChannels;i<numChannels+modizChipsetVoicesCount[modizChipsetCount];i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
        }
        snprintf(modizVoicesName[numChannels+0],MODIZ_VOICE_NAME_MAX_CHAR,"MMC5:S0");
        snprintf(modizVoicesName[numChannels+1],MODIZ_VOICE_NAME_MAX_CHAR,"MMC5:S1");
        snprintf(modizVoicesName[numChannels+2],MODIZ_VOICE_NAME_MAX_CHAR,"MMC5:PCM");
        numChannels+=modizChipsetVoicesCount[modizChipsetCount];
        modizChipsetCount++;
    }
    if (nsfData->use_n106) {
        strcat(mod_message,"Mapper...: N106\n");
        modizChipsetStartVoice[modizChipsetCount]=numChannels;
        modizChipsetType[modizChipsetCount]=NES_N106;
        modizChipsetVoicesCount[modizChipsetCount]=8;
        snprintf(modizChipsetName[modizChipsetCount],16,"N163");
        for (int i=numChannels;i<numChannels+modizChipsetVoicesCount[modizChipsetCount];i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
            snprintf(modizVoicesName[i],MODIZ_VOICE_NAME_MAX_CHAR,"N163:%d",i);
            
        }
        numChannels+=modizChipsetVoicesCount[modizChipsetCount];
        modizChipsetCount++;
    }
    if (nsfData->use_vrc6) {
        strcat(mod_message,"Mapper...: VRC6\n");
        modizChipsetStartVoice[modizChipsetCount]=numChannels;
        modizChipsetType[modizChipsetCount]=NES_VRC6;
        modizChipsetVoicesCount[modizChipsetCount]=3;
        snprintf(modizChipsetName[modizChipsetCount],16,"VRC6");
        for (int i=numChannels;i<numChannels+modizChipsetVoicesCount[modizChipsetCount];i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
        }
        snprintf(modizVoicesName[numChannels+0],MODIZ_VOICE_NAME_MAX_CHAR,"VRC6:S0");
        snprintf(modizVoicesName[numChannels+1],MODIZ_VOICE_NAME_MAX_CHAR,"VRC6:S1");
        snprintf(modizVoicesName[numChannels+2],MODIZ_VOICE_NAME_MAX_CHAR,"VRC6:SAW");
        numChannels+=modizChipsetVoicesCount[modizChipsetCount];
        modizChipsetCount++;
    }
    if (nsfData->use_vrc7) {
        strcat(mod_message,"Mapper...: VRC7\n");
        modizChipsetStartVoice[modizChipsetCount]=numChannels;
        modizChipsetType[modizChipsetCount]=NES_VRC7;
        modizChipsetVoicesCount[modizChipsetCount]=9;
        snprintf(modizChipsetName[modizChipsetCount],16,"VRC7");
        for (int i=numChannels;i<numChannels+modizChipsetVoicesCount[modizChipsetCount];i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
            snprintf(modizVoicesName[i],MODIZ_VOICE_NAME_MAX_CHAR,"VRC7:%d",i);
        }
        numChannels+=modizChipsetVoicesCount[modizChipsetCount];
        modizChipsetCount++;
    }
    
    if (nsfData->text) {
        strcat(mod_message,"\n");
        strcat(mod_message,nsfData->text);
        strcat(mod_message,"\n");
    }
    
    m_voicesDataAvail=1;
    m_genNumVoicesChannels=numChannels;
    
    return 0;
}

-(int) mmp_pt3Load:(NSString*)filePath {  //PT3
    mPlayType=MMP_PT3;
    
    FILE *f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"PT3 Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);
    
    
    memset(&pt3_t, 0, sizeof(struct ay_data));
    pt3_t.sample_rate = 44100;
    pt3_t.eqp_stereo_on = 1;
    pt3_t.dc_filter_on = 1;
    pt3_t.is_ym = 1;
    pt3_t.pan[0]=0.1;
    pt3_t.pan[1]=0.5;
    pt3_t.pan[2]=0.9;
    pt3_t.clock_rate = 1750000;
    pt3_t.frame_rate = 50;
    pt3_t.note_table = -1;
    
    
    
    for (int i=0;i<10;i++)
        pt3_mute[i]=0;
    
    load_text_file("playpt3.txt", &pt3_t);
    forced_notetable=pt3_t.note_table;
    
    pt3_numofchips=0;
    
    pt3_music_size=mp_datasize;
    pt3_music_buf=(char*)malloc(mp_datasize+1);
    if (!pt3_music_buf) return -2;
    f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"PT3 Cannot open file %@",filePath);
        mPlayType=0;
        free(pt3_music_buf);
        return -1;
    }
    
    fread(pt3_music_buf,1,pt3_music_size,f);
    pt3_music_buf[pt3_music_size]=0;
    fclose(f);
    
    // song info
    sprintf(mod_name," %s",[[[filePath lastPathComponent] stringByDeletingPathExtension] UTF8String]);
    mod_title=[NSString stringWithUTF8String:mod_name];
    strncpy(mod_message,pt3_music_buf,99);
    mod_message[99]=0;
    
    artist=[NSString stringWithFormat:@"%s",""];
    
    int num = func_setup_music((uint8_t*)pt3_music_buf, pt3_music_size, pt3_numofchips, 1);
    
    pt3_numofchips+=num;
    printf("Number of chips: %i\n",num);
    
    
    for (int ch=0; ch<pt3_numofchips; ch++) {
        if (!ayumi_configure(&pt3_ay[ch], pt3_t.is_ym, pt3_t.clock_rate, pt3_t.sample_rate)) {
            printf("ayumi_configure error (wrong sample rate?)\n");
            return 1;
        }
        ayumi_set_pan(&pt3_ay[ch], 0, pt3_t.pan[0], pt3_t.eqp_stereo_on);
        ayumi_set_pan(&pt3_ay[ch], 1, pt3_t.pan[1], pt3_t.eqp_stereo_on);
        ayumi_set_pan(&pt3_ay[ch], 2, pt3_t.pan[2], pt3_t.eqp_stereo_on);
        pt3_frame[ch]=0;
        pt3_sample[ch]=0;
        printf("Ayumi #%i configured\n",ch);
    }
    
    //    ayumi_play(pt3_ay, &pt3_t);
    
    
    mod_subsongs=1;
    mod_minsub=1;
    mod_maxsub=1;
    mod_currentsub=1;
    mCurrentSamples=0;
    if (mod_currentsub<mod_minsub) mod_currentsub=mod_minsub;
    if (mod_currentsub>mod_maxsub) mod_currentsub=mod_maxsub;
    
    iModuleLength=0;
    while (1) {
        if (pt3_renday(NULL, SOUND_BUFFER_SIZE_SAMPLE*4, &pt3_ay[0], &pt3_t, 0,0)) break;
        iModuleLength+=SOUND_BUFFER_SIZE_SAMPLE;
    }
    iModuleLength=iModuleLength*1000/PLAYBACK_FREQ;
    
    for (int ch=0; ch<pt3_numofchips; ch++) {
        func_restart_music(ch);
        pt3_frame[ch] = 0;
        pt3_sample[ch] = 0;
    }
    
    if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
    
    iCurrentTime=0;
    
    numChannels=pt3_numofchips*3;
    m_voicesDataAvail=1;
    m_genNumVoicesChannels=numChannels;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[i/3];
    }
    
    [self mmp_updateDBStatsAtLoad];
    
    //Loop
    if (mLoopMode) iModuleLength=-1;
    return 0;
}

-(int) mmp_gbsLoad:(NSString*)filePath {  //GBS
    mPlayType=MMP_GBS;
    
    FILE *f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"GBS Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);
    
    
    int argc;
    char *argv[3];
    argv[0]=strdup("gbsplay");
    argv[1]=strdup([filePath UTF8String]);
    argv[2]=NULL;
    
    argc = 2;
    gbs = common_init(argc, argv);
    if (!gbs) {
        NSLog(@"GBS Cannot open file %@",filePath);
        mPlayType=0;
        return -2;
    }
    gbs_set_nextsubsong_cb(gbs, gbs_mdz_nextsubsong,NULL);
    
    [self optGBSPLAY_UpdateParam];
        
    free(argv[0]);
    free(argv[1]);
    
    /* init additional callbacks */
    if (sound_step)
        gbs_set_step_callback(gbs, gbs_stepcallback, NULL);

    /* precalculate lookup tables */
//    precalc_notes();
//    precalc_vols();
    
    //Get metadata from GBS file
    const struct gbs_metadata *metadata=gbs_get_metadata(gbs);
    const struct gbs_status *status = gbs_get_status(gbs);
    
    //check if a m3u playlist exists
    const char *plfile=[[[filePath stringByDeletingPathExtension] stringByAppendingString:@".m3u"] UTF8String];
    f=fopen(plfile,"rb");
    if (!f) {
        plfile=[[[filePath stringByDeletingPathExtension] stringByAppendingString:@".M3U"] UTF8String];
        f=fopen(plfile,"rb");
    }
    m3uReader.clear();
    if (f) {
        fclose(f);
        m3uReader.load(plfile);
    }
    
    if (m3uReader.size()) {
        mod_subsongs=m3uReader.size();
        mod_minsub=0;
        mod_maxsub=m3uReader.size()-1;
        mod_currentsub=0;
    } else {
        mod_subsongs=status->songs;
        mod_minsub=0;
        mod_maxsub=mod_subsongs-1;
        mod_currentsub=status->defaultsong-1;
        if (mod_currentsub<mod_minsub) mod_currentsub=mod_minsub;
        if (mod_currentsub>mod_maxsub) mod_currentsub=mod_maxsub;
    }
    
    if (mod_subsongs>1) {
        mod_total_length=0;
        for (int i=0;i<mod_subsongs;i++) {
            int subsong_length=0;
            if (m3uReader.size()) subsong_length=m3uReader[i].length;
            if (subsong_length<=0) {
                gbs_init(gbs, mod_currentsub);
                status = gbs_get_status(gbs);
                subsong_length=(status->subsong_len)*1000/1024;
            }
            
            mod_total_length+=subsong_length;
            
            int song_length;
            NSString *filePathMain;
            NSString *fileName=[self getSubTitle:i];
            
            filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
            if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
            
            NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
            DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,subsong_length,numChannels,mod_subsongs);
            
            if (i==mod_subsongs-1) {// Global file stats update
                [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
            }
        }
    } else {
        [self mmp_updateDBStatsAtLoad];
    }
    
    snprintf(mod_name,sizeof(mod_name)," %s",mod_filename);
    
    if (m3uReader.info().title) {
        if (m3uReader.info().title[0]) snprintf(mod_name,sizeof(mod_name)," %s",m3uReader.info().title);
    } else if (metadata->title[0]) snprintf(mod_name,sizeof(mod_name)," %s",metadata->title);
    
    if (m3uReader.info().artist) {
        if (m3uReader.info().artist[0]) artist=[NSString stringWithCString:m3uReader.info().artist encoding:NSShiftJISStringEncoding];
    } else artist=[NSString stringWithUTF8String:metadata->author];
    
    
    if (m3uReader.size()) {
        gbs_init(gbs, m3uReader[mod_currentsub].track);
        iModuleLength=m3uReader[mod_currentsub].length;
        if (iModuleLength<=0) {
            status = gbs_get_status(gbs);
            iModuleLength=(status->subsong_len)*1000/1024;
        }
        if (m3uReader[mod_currentsub].name) mod_title=[NSString stringWithUTF8String:m3uReader[mod_currentsub-mod_minsub].name];
        gbs_configure(gbs,m3uReader[mod_currentsub].track,iModuleLength/1000,
                      settings[GBSPLAY_SilenceTimeout].detail.mdz_slider.slider_value,1,
                      settings[GBSPLAY_Fadeouttime].detail.mdz_slider.slider_value); //silence timeout, subsong gap, fadeout
    } else {
        gbs_init(gbs, mod_currentsub);
        status = gbs_get_status(gbs);
        iModuleLength=(status->subsong_len)*1000/1024;
        
        mod_title=[NSString stringWithUTF8String:status->songtitle];
        
        gbs_configure(gbs,mod_currentsub,iModuleLength/1000,
                      settings[GBSPLAY_SilenceTimeout].detail.mdz_slider.slider_value,1,
                      settings[GBSPLAY_Fadeouttime].detail.mdz_slider.slider_value); //silence timeout, subsong gap, fadeout);
        
    }    
    snprintf(mod_message,MAX_STIL_DATA_LENGTH*2,"Title..........: %s\nName...........:%s\nAuthor.........: %s\nCopyright......: %s\n",
            [mod_title UTF8String],mod_name,metadata->author,metadata->copyright);
    
    
    
    if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
    
    iCurrentTime=0;
    mCurrentSamples=0;
    
    numChannels=4;
    m_voicesDataAvail=1;
    m_genNumVoicesChannels=numChannels;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[i/3];
    }
    modizChipsetCount=0;
    modizChipsetStartVoice[modizChipsetCount]=0;
    modizChipsetVoicesCount[modizChipsetCount]=numChannels;
    snprintf(modizChipsetName[modizChipsetCount],16,"PSG");
    snprintf(modizVoicesName[0],MODIZ_VOICE_NAME_MAX_CHAR,"Square 0");
    snprintf(modizVoicesName[1],MODIZ_VOICE_NAME_MAX_CHAR,"Square 1");
    snprintf(modizVoicesName[2],MODIZ_VOICE_NAME_MAX_CHAR,"Wave");
    snprintf(modizVoicesName[3],MODIZ_VOICE_NAME_MAX_CHAR,"Noise");
    modizChipsetCount++;
    
    //Loop
    if (mLoopMode) iModuleLength=-1;
    
    seek_needed=-1;
    g_playing=1;
    return 0;
}

-(int) mmp_pixelLoad:(NSString*)filePath {  //PxTone Collage & Organya
    mPlayType=MMP_PIXEL;
    
    FILE *f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"PT3 Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    pixel_fileBufferLen=mp_datasize;
    pixel_fileBuffer=(char*)malloc(pixel_fileBufferLen);
    if (!pixel_fileBuffer) {
        NSLog(@"cannot allocate pixel_fileBuffer");
        fclose(f);
        return -1;
    }
    fseek(f,0L,SEEK_SET);
    fread(pixel_fileBuffer,1,pixel_fileBufferLen,f);
    fclose(f);
    
    pixel_organya_mode=false;
    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[filePath lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
    NSString *extension = (NSString *)[temparray_filepath lastObject];
    if ([extension isEqualToString:@"ORG"]) pixel_organya_mode=true;
    
    
    bool success = false;
    pixel_pxtn = new pxtnService();
    pixel_desc = new pxtnDescriptor();
    
    if (pixel_organya_mode) {
        organya_mute_mask=0;
        strcpy(org_filename,[filePath UTF8String]);
        success = !org_play(org_filename, pixel_fileBuffer);
    } else {
        // pxtone
        if (pixel_pxtn->init() == pxtnOK) {
            if (pixel_pxtn->set_destination_quality(2, PLAYBACK_FREQ)) {
                if (pixel_desc->set_memory_r(pixel_fileBuffer, pixel_fileBufferLen) && (pixel_pxtn->read(pixel_desc) == pxtnOK) && (pixel_pxtn->tones_ready() == pxtnOK)) {
                    success = true;
                    
                    pxtnVOMITPREPARATION prep = {0};
                    //prep.flags |= pxtnVOMITPREPFLAG_loop; // don't loop
                    prep.start_pos_float = 0;
                    prep.master_volume = 1; //(volume / 100.0f);
                    
                    if (!pixel_pxtn->moo_preparation(&prep)) {
                        success = false;
                    }
                } else {
                    pixel_pxtn->evels->Release();
                }
            }
        }
    }
    if (!success) {
        free(pixel_fileBuffer);
        pixel_fileBuffer=NULL;
        if (pixel_pxtn) delete pixel_pxtn;
        pixel_pxtn=NULL;
        if (pixel_desc) delete pixel_desc;
        pixel_desc=NULL;
        return -2;
    }
    
    // song info
    snprintf(mod_name,sizeof(mod_name)," %s",[[[filePath lastPathComponent] stringByDeletingPathExtension] UTF8String]);
    mod_title=[NSString stringWithUTF8String:mod_name];
    
    if (!pixel_organya_mode) {
        const char *name=pixel_pxtn->text->get_name_buf(NULL);
        const char *comment=pixel_pxtn->text->get_comment_buf(NULL);
        if (name) snprintf(mod_name,sizeof(mod_name)," %s",[[self sjisToNS:name] UTF8String]);
        snprintf(mod_message,MAX_STIL_DATA_LENGTH*2,"Title.....: %s\nComment...: %s\nVoices....:%d\n",name,comment,pixel_pxtn->Woice_Num());
        for (int i=0;i<pixel_pxtn->Woice_Num();i++) {
            strcat(mod_message,pixel_pxtn->Woice_Get(i)->get_name_buf(NULL));
            strcat(mod_message,"\n");
        }
        
        iModuleLength=(int64_t)(pixel_pxtn->moo_get_total_sample())*1000/PLAYBACK_FREQ;
    } else {
        iModuleLength=org_getlength();
    }
    artist=[NSString stringWithFormat:@"%s",""];
    
    mod_subsongs=1;
    mod_minsub=1;
    mod_maxsub=1;
    mod_currentsub=1;
    mCurrentSamples=0;
    if (mod_currentsub<mod_minsub) mod_currentsub=mod_minsub;
    if (mod_currentsub>mod_maxsub) mod_currentsub=mod_maxsub;
    
    if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
    
    iCurrentTime=0;
    
    if (!pixel_organya_mode) {
        numChannels=pixel_pxtn->Unit_Num();
        m_voicesDataAvail=1;
        m_genNumVoicesChannels=numChannels;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
    } else {
        numChannels=16;
        m_voicesDataAvail=1;
        m_genNumVoicesChannels=numChannels;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
    }
    
    [self mmp_updateDBStatsAtLoad];
    
    if (mLoopMode) iModuleLength=-1;
    
    return 0;
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
        
        generic_mute_mask=0;
        numChannels=3;
        m_voicesDataAvail=1;
        m_genNumVoicesChannels=numChannels;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
        
        sprintf(mod_message,"Name.....: %s\nAuthor...: %s\nType.....: %s\nPlayer...: %s\nComment..: %s\n",info.pSongName,info.pSongAuthor,info.pSongType,info.pSongPlayer,info.pSongComment);
        artist=[NSString stringWithUTF8String:info.pSongAuthor];
        
        
        [self mmp_updateDBStatsAtLoad];
        
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
    sidtune_artist=(char**)malloc(mod_subsongs*sizeof(char*));
    memset(sidtune_artist,0,sizeof(char*)*mod_subsongs);
    
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
                    } else if (strncmp(stil_info+idx,"ARTIST: ",strlen("ARTIST: "))==0) {
                        parser_status=6;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                        idx+=strlen("ARTIST: ")-1;
                    }
                    break;
                case 4: // "NAME: "
                    if (stil_info[idx]==0x0A) {
                        parser_status=3;
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
                        parser_status=3;
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
                case 6: // "ARTIST: "
                    if (stil_info[idx]==0x0A) {
                        parser_status=3;
                        if (parser_track_nb<=mod_subsongs) {
                            sidtune_artist[parser_track_nb-1]=(char*)malloc(tmp_str_idx+1);
                            strcpy(sidtune_artist[parser_track_nb-1],tmp_str);
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

-(int) mmp_websidLoad:(NSString*)filePath {  //SID
    mPlayType=MMP_WEBSID;
    
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
    websid_fileBufferLen=mp_datasize;
    websid_fileBuffer=(char*)malloc(websid_fileBufferLen);
    
    f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"SID Cannot open file %@",filePath);
        mPlayType=0;
        free(websid_fileBuffer);
        return -1;
    }
    fread(websid_fileBuffer,1,websid_fileBufferLen,f);
    fclose(f);
    
    md5_from_buffer(song_md5,33,websid_fileBuffer,websid_fileBufferLen);
    
    // Compute! Sidplayer file (stereo files not supported)
    uint32_t is_mus=0;
    if ([[[filePath pathExtension] lowercaseString] isEqualToString:@"mus"]) is_mus=1;
    if ([[[filePath pathExtension] lowercaseString] isEqualToString:@"str"]) is_mus=1;
    
    websid_loader = FileLoader::getInstance(is_mus, (void*)websid_fileBuffer, websid_fileBufferLen);
    
    if (!websid_loader) return -2;    // error
    
    NSString *c64_path = [[NSBundle mainBundle] resourcePath];
    
    char *kernal = loadRom([[c64_path stringByAppendingString:@"/kernal.c64"] UTF8String], 8192);
    char *basic = loadRom([[c64_path stringByAppendingString:@"/basic.c64"] UTF8String], 8192);
    char *chargen = loadRom([[c64_path stringByAppendingString:@"/chargen.c64"] UTF8String], 4096);
    
    uint32_t result = websid_loader->load((uint8_t*)websid_fileBuffer, websid_fileBufferLen, [[filePath lastPathComponent] UTF8String], (uint8_t*)basic, (uint8_t*)chargen, (uint8_t*)kernal, 0);
    
    delete kernal;
    delete basic;
    delete chargen;
    
    if (!result) {
        if (!websid_scope_buffers) {
            websid_scope_buffers = (int16_t**)calloc(sizeof(int16_t*), MAX_SIDS*4);
            for (int i= 0; i<MAX_SIDS*4; i++) {
                websid_scope_buffers[i] = (int16_t*)calloc(sizeof(int16_t), SOUND_BUFFER_SIZE_SAMPLE + 1);
            }
        }
        
        switch (sid_forceModel) {
            case 0:
                //cfg.forceSidModel=false;
                break;
            case 1:
                SID::setSID6581(true);
                //cfg.defaultSidModel=SidConfig::MOS6581;
                //cfg.forceSidModel=true;
                break;
            case 2:
                SID::setSID6581(false);
                //cfg.defaultSidModel=SidConfig::MOS8580;
                //cfg.forceSidModel=true;
                break;
        }
        
        
        //PLAYBACK_FREQ / vicFramesPerSecond();
        uint8_t is_ntsc=FileLoader::getNTSCMode();
        switch (sid_forceClock) {
            case 0:
                //cfg.forceC64Model=false;
                break;
            case 1:
                //cfg.forceC64Model=true;
                //cfg.defaultC64Model=SidConfig::PAL;
                is_ntsc=0;
                break;
            case 2:
                //cfg.forceC64Model=true;
                //cfg.defaultC64Model=SidConfig::NTSC;
                is_ntsc=1;
                break;
        }
        
        Core::resetTimings(PLAYBACK_FREQ, is_ntsc);
        
        websid_info=websid_loader->getInfoStrings();
        // song specific infos
        // 0: load_addr;
        // 1: play_speed;
        // 2: max_sub_song;
        // 3: actual_sub_song;
        // 4: song_name;
        // 5: song_author;
        // 6: song_copyright;
        // 7: md5 of sid file (removed)
        
        
        mod_subsongs=*((uint8_t*)(websid_info[2]));
        mod_minsub=0;//sidtune_info.startSong;
        mod_maxsub=mod_subsongs-1;
        mod_currentsub=*((uint8_t*)(websid_info[3]));
        
        
        websid_loader->initTune(PLAYBACK_FREQ, mod_currentsub);
        Postprocess::init(SOUND_BUFFER_SIZE_SAMPLE, PLAYBACK_FREQ);
        SidSnapshot::init(SOUND_BUFFER_SIZE_SAMPLE, SOUND_BUFFER_SIZE_SAMPLE);
        // testcase: Baroque_Music_64_BASIC (takes 100sec before playing - without this speedup)
        websid_skip_silence_loop = 10;
        websid_sound_started=0;
        
        
        iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
        if (iModuleLength<0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
        else if (iModuleLength==0) iModuleLength=1000;
        mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
        //Loop
        if (mLoopMode) iModuleLength=-1;
        
        numChannels=SID::getNumberUsedChips() * 4;
        m_genNumVoicesChannels=numChannels;
        m_voicesDataAvail=1;
        
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[i/4];
        }
        
        
        stil_info[0]=0;
        [self getStilInfo:(char*)[filePath UTF8String]];
        
        //Parse STIL INFO for subsongs info
        [self sid_parseStilInfo];
        
        //////////////////////////////////
        //update DB with songlength
        //////////////////////////////////
        for (int i=0;i<mod_subsongs; i++) {
            int sid_subsong_length=[self getSongLengthfromMD5:i-mod_minsub+1];
            if (sid_subsong_length<0) sid_subsong_length=optGENDefaultLength;//SID_DEFAULT_LENGTH;
            else if (sid_subsong_length==0) sid_subsong_length=1000;
            mod_total_length+=sid_subsong_length;
            
            NSString *filePathMain;
            NSString *fileName=[self getSubTitle:i];
            
            
            filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
            if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
            
            NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
            DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,sid_subsong_length,numChannels,mod_subsongs);
            
            if (i==mod_subsongs-1) {// Global file stats update
                [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
            }
        }
        
        //if sid file, assuming 2nd infoString is artist
        if (strcasecmp([[filePath pathExtension] UTF8String],"sid")==0) {
            if (websid_info[5][0]) {
                artist=[NSString stringWithUTF8String:websid_info[5]];
            }
        }
        
        if (sidtune_artist) {
            if (sidtune_artist[mod_currentsub]) {
                artist=[NSString stringWithUTF8String:sidtune_artist[mod_currentsub]];
            }
        }
        
        sprintf(mod_message,"");
        sprintf(mod_message,"Chip: %s\n",(SID::isSID6581()?"6581":"8580"));
        
        sprintf(mod_message,"%s\n[STIL Information]\n%s\n",mod_message,stil_info);
        
        if (websid_info[4][0]) snprintf(mod_name,sizeof(mod_name)," %s",websid_info[4]);
        else snprintf(mod_name,sizeof(mod_name)," %s",mod_filename);
        
        mod_name[0]=0;
        if (sidtune_name) {
            if (sidtune_name[mod_currentsub]) snprintf(mod_name,sizeof(mod_name),[[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_name[mod_currentsub]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
        }
        if (sidtune_title) {
            if (sidtune_title[mod_currentsub]) {
                if (mod_name[0]==0) snprintf(mod_name,sizeof(mod_name),[[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_title[mod_currentsub]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
                else strcat(mod_name, [[[NSString stringWithFormat:@" / %s",sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
            }
        }
        if (mod_name[0]==0) {
            if (websid_info[4][0]) snprintf(mod_name,sizeof(mod_name),[[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:websid_info[4]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
        }
        
        
    } else {
        free(websid_fileBuffer);
        websid_fileBuffer=NULL;
        return -3;
    }
    return 0;
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
    
    sid_v4=0;
    memset(m_sid_chipId,0,sizeof(m_sid_chipId));
    /* new format: md5 on all file data*/
    int tmp_md5_data_size=mp_datasize;
    char *tmp_md5_data=(char*)malloc(tmp_md5_data_size);
    
    f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"SID Cannot open file %@",filePath);
        mPlayType=0;
        free(tmp_md5_data);
        return -1;
    }
    fread(tmp_md5_data,1,tmp_md5_data_size,f);
    fclose(f);
    
    md5_from_buffer(song_md5,33,tmp_md5_data,tmp_md5_data_size);
    free(tmp_md5_data);
    
    // Init SID emu engine
    mSidEmuEngine = new sidplayfp;
    
    
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
    cfg.digiBoost = true;
    
    cfg.defaultSidModel=SidConfig::MOS6581;
    cfg.defaultC64Model=SidConfig::PAL;
    cfg.ciaModel=SidConfig::MOS6526;
    cfg.powerOnDelay=0;
    
    cfg.secondSidAddress = mSIDSecondSIDAddress;
    cfg.thirdSidAddress = mSIDThirdSIDAddress;
    
    switch (sid_forceClock) {
        case 0:
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
    
    
    // Init ReSID
    if (sid_engine==0) {
        mBuilder = new ReSIDBuilder("resid");
        ((ReSIDBuilder*)mBuilder)->bias(0.5f);
    } else {
        mBuilder = new ReSIDfpBuilder("residfp");
        ((ReSIDfpBuilder*)mBuilder)->filter6581Curve(0.5f);
        ((ReSIDfpBuilder*)mBuilder)->filter8580Curve(0.5f);
    }
    cfg.sidEmulation  = mBuilder;
    
    // Check if builder is ok
    if (!mBuilder->getStatus()) {
        NSLog(@"issue in creating sid builder");
        return -1;
    }
    
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
        
        
        
        mod_subsongs=sidtune_info->songs();
        mod_minsub=0;//sidtune_info.startSong;
        mod_maxsub=sidtune_info->songs()-1;
        mod_currentsub=sidtune_info->startSong()-1;
        
        mSidTune->selectSong(mod_currentsub-mod_minsub);
        
        iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
        if (iModuleLength<0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
        else if (iModuleLength==0) iModuleLength=1000;
        mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
        
        if (mSidEmuEngine->load(mSidTune)) {
            
            
            //cfg.secondSidAddress = 0xd420;
            //mSidEmuEngine->config(cfg);
            
            
            iCurrentTime=0;
            mCurrentSamples=0;
            numChannels=4*sidtune_info->sidChips();//(mSidEmuEngine->info()).channels();
            
            //if (sid_engine==1){
            m_voicesDataAvail=1;
            m_genNumVoicesChannels=numChannels;
            for (int i=0;i<m_genNumVoicesChannels;i++) {
                m_voice_voiceColor[i]=m_voice_systemColor[i/4];
            }
            //} else m_voicesDataAvail=0;
            
            stil_info[0]=0;
            [self getStilInfo:(char*)[filePath UTF8String]];
            
            //if sid file, assuming 2nd infoString is artist
            if (strcasecmp([[filePath pathExtension] UTF8String],"sid")==0) {
                if (sidtune_info->numberOfInfoStrings()>=2) {
                    artist=[NSString stringWithFormat:@"%s",sidtune_info->infoString(1)];
                }
            }
            
            sprintf(mod_message,"");
            
            if (cfg.forceSidModel) {
                if (cfg.defaultSidModel==SidConfig::MOS8580) {
                    //cfg.leftVolume*=3;
                    //cfg.rightVolume*=3;
                    mSidEmuEngine->config(cfg);
                }
            } else {
                //boost volume for 8580
                if (sidtune_info->sidModel(0)==SidTuneInfo::SIDMODEL_8580) {
                    //cfg.leftVolume*=3;
                    //cfg.rightVolume*=3;
                    mSidEmuEngine->config(cfg);
                }
            }
            
            if (cfg.forceSidModel) sprintf(mod_message,"Chip: %s\n",(cfg.defaultSidModel==SidConfig::MOS8580?"8580":"6581"));
            else sprintf(mod_message,"Chip: %s\n",(sidtune_info->sidModel(0)==SidTuneInfo::SIDMODEL_8580?"8580":"6581"));
            for (int i=0;i<sidtune_info->numberOfInfoStrings();i++)
                sprintf(mod_message,"%s%s\n",mod_message,sidtune_info->infoString(i));
            
            for (int i=0;i<sidtune_info->numberOfCommentStrings();i++)
                sprintf(mod_message,"%s%s\n",mod_message,sidtune_info->commentString(i));
            
            sprintf(mod_message,"%s\n[STIL Information]\n%s\n",mod_message,stil_info);
            //Loop
            if (mLoopMode==1) iModuleLength=-1;
            
            
            //Parse STIL INFO for subsongs info
            [self sid_parseStilInfo];
            
            mod_name[0]=0;
//            if (sidtune_name) {
//                if (sidtune_name[mod_currentsub]) snprintf(mod_name,sizeof(mod_name)," %s",[[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_name[mod_currentsub]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
//            }
//            if (sidtune_title) {
//                if (sidtune_title[mod_currentsub]) {
//                    if (mod_name[0]==0) snprintf(mod_name,sizeof(mod_name)," %s",[[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_title[mod_currentsub]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
//                    else strcat(mod_name, [[NSString stringWithFormat:@" / %@",[[NSString stringWithUTF8String:sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]] UTF8String]);
//                }
//            }
            if (mod_name[0]==0) {
                if (sidtune_info->infoString(0)[0]) snprintf(mod_name,sizeof(mod_name)," %s",[[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_info->infoString(0)]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
                else snprintf(mod_name,sizeof(mod_name)," %s",mod_filename);
            }
            
            if (sidtune_name) {
                if (sidtune_name[mod_currentsub]) mod_title=[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_name[mod_currentsub]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
                else mod_title=NULL;
            } else mod_title=NULL;
            if (sidtune_title) {
                if (sidtune_title[mod_currentsub]) {
                    if (mod_title==NULL) mod_title=[NSString stringWithFormat:@"%@",[[NSString stringWithUTF8String:sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]];
                    else mod_title=[NSString stringWithFormat:@"%@ - %@",mod_title,[[NSString stringWithUTF8String:sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]];
                }
            }
            
            
            
            //////////////////////////////////
            //update DB with songlength
            //////////////////////////////////
            for (int i=0;i<sidtune_info->songs(); i++) {
                int sid_subsong_length=[self getSongLengthfromMD5:i-mod_minsub+1];
                if (sid_subsong_length<0) sid_subsong_length=optGENDefaultLength;//SID_DEFAULT_LENGTH;
                else if (sid_subsong_length==0) sid_subsong_length=1000;
                mod_total_length+=sid_subsong_length;
                
                NSString *filePathMain;
                NSString *fileName=[self getSubTitle:i];
                
                
                filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
                if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
                
                NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
                DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,sid_subsong_length,numChannels,mod_subsongs);
                
                if (i==mod_subsongs-1) {// Global file stats update
                    [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
                }
            }
            
            
            return 0;
        }
    }
    return 1;
}

-(int) mmp_s98Load:(NSString*)filePath {  //S98
    return -1;
}

-(int) mmp_kssLoad:(NSString*)filePath {  //KSS
    mPlayType=MMP_KSS;
    
    FILE *f=fopen([filePath UTF8String],"rb");
    if (f==NULL) {
        NSLog(@"HSS Cannot open file %@",filePath);
        mPlayType=0;
        return -1;
    }
    
    fseek(f,0L,SEEK_END);
    mp_datasize=ftell(f);
    fclose(f);
    
    if ((kss = KSS_load_file((char*)[filePath UTF8String])) == NULL) {
        NSLog(@"KSS Cannot load file %@",filePath);
        return -2;
    }
    
    //check if a playlist exists
    const char *plfile=[[[filePath stringByDeletingPathExtension] stringByAppendingString:@".m3u"] UTF8String];
    f=fopen(plfile,"rb");
    if (!f) {
        plfile=[[[filePath stringByDeletingPathExtension] stringByAppendingString:@".M3U"] UTF8String];
        f=fopen(plfile,"rb");
    }
    m3uReader.clear();
    if (f) {
        fclose(f);
        m3uReader.load(plfile);
    }
    
    if (m3uReader.size()) {
        mod_subsongs=m3uReader.size();
        mod_minsub=0;
        mod_maxsub=m3uReader.size()-1;
        mod_currentsub=0;
    } else {
        mod_subsongs=kss->trk_max-kss->trk_min+1;
        mod_minsub=kss->trk_min;
        mod_maxsub=kss->trk_max;
        mod_currentsub=mod_minsub;
    }
    
    
    /* INIT KSSPLAY */
    kssplay = KSSPLAY_new(PLAYBACK_FREQ, 2, 16);
    KSSPLAY_set_data(kssplay, kss);
    
    kssOptLoopNb=1;
    
    sprintf(mod_name," %s",mod_filename);
    if (kss->title[0]) sprintf(mod_name," %s",kss->title);
    if (m3uReader.info().title)
        if (m3uReader.info().title[0]) sprintf(mod_name," %s",m3uReader.info().title);
    
    if (m3uReader.size()) {
        if (m3uReader[mod_currentsub].name) mod_title=[NSString stringWithUTF8String:m3uReader[mod_currentsub-mod_minsub].name];
    } else {
        if (kss->info) {
            if (kss->info[mod_currentsub].title[0]) mod_title=[NSString stringWithUTF8String:kss->info[mod_currentsub].title];
        }
    }
    
    if (m3uReader.info().artist)
        if (m3uReader.info().artist[0]) artist=[NSString stringWithCString:m3uReader.info().artist encoding:NSShiftJISStringEncoding];
    
    if (m3uReader.size()) {
        KSSPLAY_reset(kssplay, m3uReader[mod_currentsub].track, 0);
        iModuleLength=m3uReader[mod_currentsub].length;
        if (iModuleLength<=0) {
            if (kss->info) iModuleLength=kss->info[mod_currentsub].time_in_ms;
            else iModuleLength=optGENDefaultLength;
        }
        if (iModuleLength<1000) iModuleLength=1000;
    } else {
        KSSPLAY_reset(kssplay, mod_currentsub, 0);
        if (kss->info) {
            iModuleLength=kss->info[mod_currentsub].time_in_ms;
        }
        else iModuleLength=optGENDefaultLength;
        if (iModuleLength<1000) iModuleLength=1000;
    }
    
    KSSPLAY_set_device_quality(kssplay, EDSC_PSG, 1);
    KSSPLAY_set_device_quality(kssplay, EDSC_SCC, 1);
    KSSPLAY_set_device_quality(kssplay, EDSC_OPLL, 1);
    
    KSSPLAY_set_master_volume(kssplay, 64);
    
    KSSPLAY_set_device_pan(kssplay, EDSC_PSG, -32);
    KSSPLAY_set_device_pan(kssplay, EDSC_SCC,  32);
    kssplay->opll_stereo = 1;
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 0, 1);
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 1, 2);
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 2, 1);
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 3, 2);
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 4, 1);
    KSSPLAY_set_channel_pan(kssplay, EDSC_OPLL, 5, 2);
    
    numChannels=0;
    memset(modizChipsetType,0,sizeof(modizChipsetType));
    modizChipsetCount=0;
    mod_message[0]=0;
    
    if (kssplay->kss->msx_audio && !kssplay->device_mute[KSS_DEVICE_OPL]) {
        strcat(mod_message,"Y8950,");
        modizChipsetStartVoice[modizChipsetCount]=numChannels;
        modizChipsetType[modizChipsetCount]=KSS_Y8950;
        modizChipsetVoicesCount[modizChipsetCount]=15;
        snprintf(modizChipsetName[modizChipsetCount],16,"Y8950");
        for (int i=numChannels;i<numChannels+modizChipsetVoicesCount[modizChipsetCount];i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
            snprintf(modizVoicesName[i],MODIZ_VOICE_NAME_MAX_CHAR,"Y8950:%d",i-numChannels+1);
        }
        numChannels+=modizChipsetVoicesCount[modizChipsetCount];
        modizChipsetCount++;
    }
    if (kssplay->kss->fmpac && !kssplay->device_mute[KSS_DEVICE_OPLL]) {
        strcat(mod_message,"YM2413,");
        modizChipsetStartVoice[modizChipsetCount]=numChannels;
        modizChipsetType[modizChipsetCount]=KSS_YM2413;
        modizChipsetVoicesCount[modizChipsetCount]=14;
        snprintf(modizChipsetName[modizChipsetCount],16,"YM2413");
        for (int i=numChannels;i<numChannels+modizChipsetVoicesCount[modizChipsetCount];i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
            snprintf(modizVoicesName[i],MODIZ_VOICE_NAME_MAX_CHAR,"YM2413:%d",i-numChannels+1);
        }
        numChannels+=modizChipsetVoicesCount[modizChipsetCount];
        modizChipsetCount++;
    }
    if (!kssplay->device_mute[KSS_DEVICE_PSG]) {
        if (kssplay->kss->sn76489) {
            strcat(mod_message,"SN76489,");
            modizChipsetStartVoice[modizChipsetCount]=numChannels;
            modizChipsetType[modizChipsetCount]=KSS_SN76489;
            modizChipsetVoicesCount[modizChipsetCount]=4;
            snprintf(modizChipsetName[modizChipsetCount],16,"SN76489");
            for (int i=numChannels;i<numChannels+modizChipsetVoicesCount[modizChipsetCount];i++) {
                m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
                snprintf(modizVoicesName[i],MODIZ_VOICE_NAME_MAX_CHAR,"SN76489:%d",i-numChannels+1);
            }
            
            numChannels+=modizChipsetVoicesCount[modizChipsetCount];
            modizChipsetCount++;
        } else {
            strcat(mod_message,"PSG,");
            modizChipsetStartVoice[modizChipsetCount]=numChannels;
            modizChipsetType[modizChipsetCount]=KSS_PSG;
            modizChipsetVoicesCount[modizChipsetCount]=3;
            snprintf(modizChipsetName[modizChipsetCount],16,"PSG");
            for (int i=numChannels;i<numChannels+modizChipsetVoicesCount[modizChipsetCount];i++) {
                m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
                snprintf(modizVoicesName[i],MODIZ_VOICE_NAME_MAX_CHAR,"PSG:%d",i-numChannels+1);
            }
            numChannels+=modizChipsetVoicesCount[modizChipsetCount];
            modizChipsetCount++;
        }
    }
    if (!kssplay->device_mute[KSS_DEVICE_SCC]) {
        strcat(mod_message,"Konami SCC,");
        modizChipsetStartVoice[modizChipsetCount]=numChannels;
        modizChipsetType[modizChipsetCount]=KSS_SCC;
        modizChipsetVoicesCount[modizChipsetCount]=5;
        snprintf(modizChipsetName[modizChipsetCount],16,"SCC");
        for (int i=numChannels;i<numChannels+modizChipsetVoicesCount[modizChipsetCount];i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[modizChipsetCount];
            snprintf(modizVoicesName[i],MODIZ_VOICE_NAME_MAX_CHAR,"SCC:%d",i-numChannels+1);
        }
        numChannels+=modizChipsetVoicesCount[modizChipsetCount];
        modizChipsetCount++;
    }
    
    //remove the last ','
    if (strlen(mod_message)) mod_message[strlen(mod_message)-1]=0;
    
    generic_mute_mask=0;
    m_voicesDataAvail=1;
    m_genNumVoicesChannels=numChannels;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[0];
    }
    
    if (mod_subsongs>1) {
        mod_total_length=0;
        for (int i=0;i<mod_subsongs;i++) {
            int subsong_length=0;
            if (m3uReader.size()) subsong_length=m3uReader[i].length;
            if (subsong_length<=0) {
                if (kss->info) subsong_length=kss->info[i].time_in_ms;
                else subsong_length=optGENDefaultLength;
            }
            if (subsong_length<1000) subsong_length=1000;
            
            mod_total_length+=subsong_length;
            
            int song_length;
            NSString *filePathMain;
            NSString *fileName=[self getSubTitle:i];
            
            filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
            if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
            
            NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
            DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,subsong_length,numChannels,mod_subsongs);
            
            if (i==mod_subsongs-1) {// Global file stats update
                [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
            }
        }
    } else {
        [self mmp_updateDBStatsAtLoad];
    }
    
    KSSPLAY_get_MGStext(kssplay,mod_message+strlen(mod_message),MAX_STIL_DATA_LENGTH*2-strlen(mod_message));
    
    if (mLoopMode) iModuleLength=-1;
    iCurrentTime=0;
    mCurrentSamples=0;
    
    if (iModuleLength>0) mFadeSamplesStart=(int64_t)(iModuleLength-1000)*PLAYBACK_FREQ/1000; //1s
    else mFadeSamplesStart=1<<30;
    
    return 0;
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
        numChannels=hvl_song->ht_Channels;
        iModuleLength=hvl_GetPlayTime(hvl_song);
        iCurrentTime=0;
        
        
        if (mod_subsongs>1) NSLog(@"hvl multisub");
        
        
        //////////////////////////////////
        //update DB with songlength
        //////////////////////////////////
        if (mod_subsongs) {
            for (int i=0;i<mod_subsongs; i++) {
                if (!hvl_InitSubsong( hvl_song,i )) {
                    NSLog(@"HVL issue in initsubsong %d",i);
                    hvl_FreeTune(hvl_song);
                    mPlayType=0;
                    return -2;
                }
                
                int subsong_length=hvl_GetPlayTime(hvl_song);
                mod_total_length+=subsong_length;
                
                short int playcount;
                signed char rating,avg_rating;
                int song_length;
                char channels_nb;
                int songs;
                NSString *filePathMain;
                NSString *fileName=[self getSubTitle:i];
                
                filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
                if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
                
                NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
                DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,subsong_length,numChannels,mod_subsongs);
                
                if (i==mod_subsongs-1) {// Global file stats update
                    [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
                }
            }
        } else {
            mod_subsongs=1;
            [self mmp_updateDBStatsAtLoad];
        }
        
        if (!hvl_InitSubsong( hvl_song,mod_currentsub )) {
            NSLog(@"HVL issue in initsubsong %d",mod_currentsub);
            hvl_FreeTune(hvl_song);
            mPlayType=0;
            return -2;
        }
        
        
        //Loop
        if (mLoopMode==1) iModuleLength=-1;
        
        
        m_genNumVoicesChannels=numChannels;
        m_voicesDataAvail=1;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
        
        
        if (hvl_song->ht_InstrumentNr==0) sprintf(mod_message,"N/A\n");
        else {
            sprintf(mod_message,"");
            for (int i=0;i<hvl_song->ht_InstrumentNr;i++) {
                sprintf(mod_message,"%s%s\n", mod_message,hvl_song->ht_Instruments[i].ins_Name);
            };
        }
        
        hvl_sample_to_write=hvl_song->ht_Frequency/50/hvl_song->ht_SpeedMultiplier;
        
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
    fclose(f);
    
    md5_from_buffer(song_md5,33,tmp_md5_data,mp_datasize);
    free(tmp_md5_data);
    
    
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
    UADEstate.config.frequency=PLAYBACK_FREQ;
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
    //printf("mod filename: %s\n",mod_filename);
    
    sprintf(mod_name," %s",mod_filename);
    sprintf(mod_message,"%s\n",mod_name);
    numChannels=4;
    m_genNumVoicesChannels=numChannels;
    m_voicesDataAvail=1;
    HC_voicesMuteMask1=0xFF;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[0];
    }
    
    iCurrentTime=0;
    
    iModuleLength=UADEstate.song->playtime;
    if (iModuleLength<0) iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
    
    
    
    [self mmp_updateDBStatsAtLoad];
    
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
        xmp_ctx=NULL;
        return 2;
    }
    
    if (xmp_start_player(xmp_ctx, 44100, 0) != 0) {
        NSLog(@"XMP: error initializing player");
        xmp_release_module(xmp_ctx);
        xmp_free_context(xmp_ctx);
        xmp_ctx=NULL;
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
    
    //////////////////////////////////
    //update DB with songlength
    //////////////////////////////////
    for (int i=0;i<mod_subsongs; i++) {
        int subsong_length=xmp_mi->seq_data[i].duration;
        mod_total_length+=subsong_length;
        
        NSString *filePathMain;
        NSString *fileName=[self getSubTitle:i];
        
        filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
        if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
        
        NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
        DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,subsong_length,numChannels,mod_subsongs);
        
        if (i==mod_subsongs-1) {// Global file stats update
            [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
        }
    }
    
    iCurrentTime=0;
    numChannels=xmp_mi->mod->chn;
    m_genNumVoicesChannels=numChannels;
    m_voicesDataAvail=1;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
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
    
    iModuleLength=(int)(openmpt_module_get_duration_seconds(openmpt_module_ext_get_module(ompt_mod))*1000);
    iCurrentTime=0;
    mPatternDataAvail=1; //OMPT/MODPLUG style
    
    numChannels=openmpt_module_get_num_channels(openmpt_module_ext_get_module(ompt_mod));
    numPatterns=openmpt_module_get_num_patterns(openmpt_module_ext_get_module(ompt_mod));
    
    //LIBOPENMPT_API const char * openmpt_module_get_metadata_keys( openmpt_module * mod );
    //LIBOPENMPT_API const char * openmpt_module_get_metadata( openmpt_module * mod, const char * key );
    char *keys_list=(char*)openmpt_module_get_metadata_keys(openmpt_module_ext_get_module(ompt_mod));
    if (keys_list) {
        //printf("ompt metadata keys: %s\n",keys_list);
        free(keys_list);
    }
    
    modName=(char*)openmpt_module_get_metadata(openmpt_module_ext_get_module(ompt_mod),"title");
    if (!modName) {
        sprintf(mod_name," %s",mod_filename);
    } else if (modName[0]==0) {
        sprintf(mod_name," %s",mod_filename);
    } else {
        sprintf(mod_name," %s", modName);
    }
    mdz_safe_free(modName);
    
    char *artistStr=(char*)openmpt_module_get_metadata(openmpt_module_ext_get_module(ompt_mod),"artist");
    if (!artistStr) {
    } else if (artistStr[0]==0) {
    } else {
        artist=[NSString stringWithUTF8String:artistStr];
    }
    if (artistStr) free(artistStr);
    
    mod_subsongs=openmpt_module_get_num_subsongs(openmpt_module_ext_get_module(ompt_mod)); //mp_file->mod);
    mod_minsub=0;
    mod_maxsub=mod_subsongs-1;
    mod_currentsub=openmpt_module_get_selected_subsong(openmpt_module_ext_get_module(ompt_mod)); //mp_file->mod );
    //////////////////////////////////
    //update DB with songlength
    //////////////////////////////////
    for (int i=0;i<mod_subsongs; i++) {
        openmpt_module_select_subsong(openmpt_module_ext_get_module(ompt_mod), i);
        int subsong_length=openmpt_module_get_duration_seconds( openmpt_module_ext_get_module(ompt_mod) )*1000;
        mod_total_length+=subsong_length;
        
        NSString *filePathMain;
        NSString *fileName=[self getSubTitle:i];
        
        filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
        if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
        
        NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
        DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,subsong_length,numChannels,mod_subsongs);
        
        if (i==mod_subsongs-1) {// Global file stats update
            [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
        }
    }
    openmpt_module_select_subsong(openmpt_module_ext_get_module(ompt_mod), mod_currentsub);
    
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
    
    m_genNumVoicesChannels=numChannels;
    m_voicesDataAvail=1;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
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
    
    //check if a soundfont exist in the same dir
    NSError *error;
    NSArray *dirContent;//
    NSString *file,*cpath;
    BOOL isDir;
    BOOL fileExist;
    
    tim_force_soundfont=false;
    
    cpath=[filePath stringByDeletingLastPathComponent];
    //test if a sf2 exist with same name
    file=[[filePath stringByDeletingPathExtension] stringByAppendingString:@".sf2"];
    fileExist=[mFileMngr fileExistsAtPath:file isDirectory:&isDir];
    if (fileExist&&(!isDir)) {
        snprintf(tim_force_soundfont_path,sizeof(tim_force_soundfont_path)-1,"%s",[cpath UTF8String]);
        f=fopen([[cpath stringByAppendingString:@"/timidity.cfg"] UTF8String],"wb");
        if (f) {
            tim_force_soundfont=true;
            fprintf(f,"soundfont \"%s\"\n",[file UTF8String]);
            fclose(f);
        }
    } else {
        dirContent=[mFileMngr contentsOfDirectoryAtPath:cpath error:&error];
        for (file in dirContent) {
            [mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
            if (!isDir) {
                if ([[[file pathExtension] lowercaseString] isEqualToString:@"sf2"]) {
                    //sf2 soundfont found
                    snprintf(tim_force_soundfont_path,sizeof(tim_force_soundfont_path)-1,"%s",[cpath UTF8String]);
                    f=fopen([[cpath stringByAppendingString:@"/timidity.cfg"] UTF8String],"wb");
                    if (f) {
                        tim_force_soundfont=true;
                        fprintf(f,"soundfont \"%s\"\n",[file UTF8String]);
                        fclose(f);
                    }
                }
            }
        }
    }
    
    
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
    
    numChannels=64;
    
    strcpy(tim_filepath,[filePath UTF8String]);
    if (mdz_ArchiveFilesCnt) {
        strcpy(tim_filepath_orgfile,[[NSString stringWithFormat:@"%@@%d",mod_loadmodule_filepath,mdz_currentArchiveIndex] UTF8String]);
        strcpy(tim_filepath_filename,[[self getArcEntryTitle:mdz_currentArchiveIndex] UTF8String]);
    }
    else {
        strcpy(tim_filepath_orgfile,[mod_loadmodule_filepath UTF8String]);
        strcpy(tim_filepath_filename,[[mod_loadmodule_filepath lastPathComponent] UTF8String]);
    }
    
    tim_finished=1;
    
    //preplay the file to get the number of channels in m_genNumVoicesChannels
    {
        char *argv[1];
        argv[0]=tim_filepath;
        
        //tim_init((char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"modizer.app/timidity"] UTF8String]);
        
        if (tim_force_soundfont) {
            //NSLog(@"forcing with: %s",tim_force_soundfont_path);
            tim_init(tim_force_soundfont_path);
            snprintf(tim_config_file_path,sizeof(tim_config_file_path),"%s/timidity.cfg",tim_force_soundfont_path);
        } else {
            //timidity
            tim_init((char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"modizer.app/timidity"] UTF8String]);
            tim_init((char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"] UTF8String]);
            snprintf(tim_config_file_path,sizeof(tim_config_file_path),"%s",(char*)[[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/timidity.cfg"] UTF8String]);
        }
        
        mdz_tim_only_precompute=1;
        tim_main(1, argv);
    }
    
    iModuleLength=tim_midilength;
    numChannels=m_genNumVoicesChannels;
    
    [self mmp_updateDBStatsAtLoad];
    
    m_voicesDataAvail=1;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[0];
    }
    
    sprintf(mod_name," %s",mod_filename);
    sprintf(mod_message,"Midi Infos:");
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    
    
    return 0;
}

-(void) vgmStream_ChangeToSub:(int) subsong {
    if (vgmStream) close_vgmstream(vgmStream);
    vgmFile->stream_index=subsong;
    vgmStream = init_vgmstream_from_STREAMFILE(vgmFile);
    if (!vgmStream) {
        iModuleLength=0;
        
        mVGMSTREAM_total_samples = 0;
        mVGMSTREAM_totalinternal_samples = 0;
        mVGMSTREAM_decode_pos_samples=0;
        
        numChannels=0;
        mod_message[0]=0;
        describe_vgmstream(vgmStream,mod_message,MAX_STIL_DATA_LENGTH*2);
        
        return;
    }
    
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
    
    mVGMSTREAM_decode_pos_samples=0;
    
    numChannels=vgmStream->channels;
    if (vgmStream->stream_name[0]) sprintf(mod_name," %s",vgmStream->stream_name);
    else sprintf(mod_name," %s",mod_filename);
    
    mod_message[0]=0;
    describe_vgmstream(vgmStream,mod_message,MAX_STIL_DATA_LENGTH*2);
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
    
    vgmFile->stream_index=0;
    
    vgmStream = init_vgmstream_from_STREAMFILE(vgmFile);
    
    if (!vgmStream) {
        NSLog(@"Error init_vgmstream_from_STREAMFILE %@",filePath);
        mPlayType=0;
        src_delete(src_state);
        close_streamfile(vgmFile);
        vgmFile=NULL;
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
    
    if (vgmStream->channels <= 0) {
        NSLog(@"Error vgmStream->channels: %d",vgmStream->channels);
        close_vgmstream(vgmStream);
        vgmStream = NULL;
        src_delete(src_state);
        close_streamfile(vgmFile);
        vgmFile=NULL;
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
    
    //////////////////////////////////
    //update DB with songlength
    //////////////////////////////////
    for (int i=0;i<mod_subsongs; i++) {
        //change subsong
        vgmFile->stream_index=i;
        VGMSTREAM* vgmStreamTmp = init_vgmstream_from_STREAMFILE(vgmFile);
        if (vgmStreamTmp) {
            int subsong_length= vgmstream_get_samples(vgmStreamTmp);
            mod_total_length+=(double)subsong_length*1000.0f/(double)(vgmStream->sample_rate);
            
            close_vgmstream(vgmStreamTmp);
            
            NSString *filePathMain;
            NSString *fileName=[self getSubTitle:i];
            
            filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
            if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
            
            NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
            DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,subsong_length,numChannels,mod_subsongs);
            
            if (i==mod_subsongs-1) {// Global file stats update
                [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
            }
        }
    }
    vgmFile->stream_index=mod_currentsub;
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
        if (keys[x]=="artist") artist=[NSString stringWithUTF8String:tags[keys[x]].c_str()];
        else if (keys[x]=="game") album=[NSString stringWithUTF8String:tags[keys[x]].c_str()];
        else if (keys[x]=="title") mod_title=[NSString stringWithUTF8String:tags[keys[x]].c_str()];
    }
    if (album) sprintf(mod_name," %s",[album UTF8String]);
    
    int fadeInMS=xSFFile->GetFadeMS(xSFPlayer->fadeInMS);
    xSFPlayer->fadeInMS=fadeInMS;
    xSFPlayer->fadeSample=(int64_t)(fadeInMS)*(int64_t)(xSFPlayer->sampleRate)/1000;
    
    iCurrentTime=0;
    numChannels=16;
    m_genNumVoicesChannels=numChannels;
    m_voicesDataAvail=1;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
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

-(int) mmp_v2mLoad:(NSString*)filePath extension:(NSString*)extension {  //V2M
    mPlayType=MMP_V2M;
    FILE *f;
    
    if ([[extension uppercaseString] isEqualToString:@"V2MZ"]) {
        //GZIP compressed
        f=fopen([filePath UTF8String],"rb");
        if (f==NULL) {
            NSLog(@"V2M Cannot open file %@",filePath);
            mPlayType=0;
            return -1;
        }
        fseek(f,-4,SEEK_END);
        fread(&mp_datasize,1,4,f);
        mp_data=(char*)malloc(mp_datasize);
        fclose(f);
        
        gzFile gzf;
        gzf=gzopen([filePath UTF8String],"rb");
        if (gzf==NULL) {
            NSLog(@"V2M Cannot open file %@",filePath);
            mPlayType=0;
            return -1;
        }
        gzread(gzf,mp_data,mp_datasize);
        gzclose(gzf);
    } else {
        //Normal V2M file
        f=fopen([filePath UTF8String],"rb");
        if (f==NULL) {
            NSLog(@"V2M Cannot open file %@",filePath);
            mPlayType=0;
            return -1;
        }
        fseek(f,0,SEEK_END);
        mp_datasize=ftell(f);
        mp_data=(char*)malloc(mp_datasize);
        fseek(f,0,SEEK_SET);
        fread(mp_data,1,mp_datasize,f);
        fclose(f);
    }
    
    if (!mp_datasize) {
        NSLog(@"V2M: issue with file format %@",filePath);
        mPlayType=0;
        return -1;
    }
    
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
    
    
    generic_mute_mask=0xFFFFFFFF;
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
    m_genNumVoicesChannels=numChannels;
    m_voicesDataAvail=1;
    for (int i=0;i<m_genNumVoicesChannels;i++) {
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
    
    [self mmp_updateDBStatsAtLoad];
    
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
        info.tag_length_ms = optGENDefaultLength;
        info.tag_fade_ms = 3000;
    }
    iModuleLength=info.tag_length_ms+info.tag_fade_ms;
    iCurrentTime=0;
    
    
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
    
    if (HC_type==1) { //PSF1
        hc_sample_rate=44100;
        m_voice_current_samplerate=hc_sample_rate;
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
        m_genNumVoicesChannels=numChannels;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
        
        //help to behave more like real hardware, fix a few recent dumps
        void * pIOP = psx_get_iop_state( HC_emulatorCore );
        iop_set_compat( pIOP, IOP_COMPAT_HARSH );
    } else if (HC_type==2) { //PSF2
        hc_sample_rate=48000;
        m_voice_current_samplerate=hc_sample_rate;
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
        m_genNumVoicesChannels=numChannels;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[i/24];
        }
        
        //help to behave more like real hardware, fix a few recent dumps
        void * pIOP = psx_get_iop_state( HC_emulatorCore );
        iop_set_compat( pIOP, IOP_COMPAT_HARSH );
    } else if ( HC_type == 0x11 || HC_type == 0x12 ) { //DSF/SSF
        hc_sample_rate=44100;
        m_voice_current_samplerate=hc_sample_rate;
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
            m_genNumVoicesChannels=32;
            m_voicesDataAvail=1;
            for (int i=0;i<m_genNumVoicesChannels;i++) {
                m_voice_voiceColor[i]=m_voice_systemColor[0];
            }
        } else if (HC_type==0x12) {
            numChannels=64;
            m_genNumVoicesChannels=64;
            m_voicesDataAvail=1;
            for (int i=0;i<m_genNumVoicesChannels;i++) {
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
            usf_shutdown(lzu_state->emu_state);
            
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
        
        m_voice_current_samplerate=hc_sample_rate;
        
        numChannels=2;
    } else if ( HC_type == 0x23 ) { //SNSF
        hc_sample_rate=32000;
        m_voice_current_samplerate=hc_sample_rate;
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
        m_genNumVoicesChannels=8;
        m_voicesDataAvail=1;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
    } else if ( HC_type == 0x41 ) { //QSF
        hc_sample_rate=24038;
        m_voice_current_samplerate=hc_sample_rate;
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
        m_genNumVoicesChannels=numChannels;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
        
    }
    src_ratio=PLAYBACK_FREQ/(double)hc_sample_rate;
    
    if (HC_type==0x21) {
        sprintf(mod_name,"");
        
        if (usf_info_data->inf_title)
            if (usf_info_data->inf_title[0]) sprintf(mod_name," %s",usf_info_data->inf_title);
        
        if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
        
        sprintf(mod_message,"Game.......:\t%s\nTitle......:\t%s\nArtist.....:\t%s\nYear.......:\t%s\nGenre......:\t%s\nRipper.....:\t%s\nCopyright..:\t%s\nTrack......:\t%s\nSample rate: %dHz\nLength: %ds\n",
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
        
        if (usf_info_data->inf_game && usf_info_data->inf_game[0]) mod_title=[NSString stringWithUTF8String:(const char*)(usf_info_data->inf_game)];
        
        if (usf_info_data->inf_artist) artist=[NSString stringWithUTF8String:usf_info_data->inf_artist];
        if (usf_info_data->inf_game) album=[NSString stringWithUTF8String:usf_info_data->inf_game];
    } else {
        sprintf(mod_name,"");
        if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
        
        sprintf(mod_message,"\n");
        const char *padding="...........";
        int tgtstrlen=11;
        for (id key in info.info) {
            
            id value = info.info[key];
            int padLen=tgtstrlen-[key length];
            NSString *tmpstr=[NSString stringWithFormat:@"%@%*.*s: %@\n",key,padLen,padLen,padding,value];
            strcat(mod_message,[tmpstr UTF8String]);
            
            if ([key isEqualToString:@"album"]&&([value length]>0)) {
                mod_title=[NSString stringWithString:value];
                album=[NSString stringWithString:value];
            }
            if ([key isEqualToString:@"artist"]&&([value length]>0)) {
                artist=[NSString stringWithString:value];
            }
        }
        //strcat(mod_message,[[NSString stringWithFormat:@"Sample rate: %dHz\nLength.....: %ds\n",hc_sample_rate,iModuleLength/1000] UTF8String]);
    }
    
    [self mmp_updateDBStatsAtLoad];
    
    //Loop
    if (mLoopMode==1) iModuleLength=-1;
    
    m_genNumVoicesChannels=numChannels;
    
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
    ChipOpts[0].YM2612.EmuCore=settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_value;
    ChipOpts[1].YM2612.EmuCore=settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_value;
    
    ChipOpts[0].YM3812.EmuCore=settings[VGMPLAY_YM3812Emulator].detail.mdz_switch.switch_value;
    ChipOpts[1].YM3812.EmuCore=settings[VGMPLAY_YM3812Emulator].detail.mdz_switch.switch_value;
    
    ChipOpts[0].YMF262.EmuCore=settings[VGMPLAY_YMF262Emulator].detail.mdz_switch.switch_value;
    ChipOpts[1].YMF262.EmuCore=settings[VGMPLAY_YMF262Emulator].detail.mdz_switch.switch_value;
    
    ChipOpts[0].YM2612.SpecialFlags|=(settings[VGMPLAY_NUKEDOPN2_Option].detail.mdz_switch.switch_value<<3);
    ChipOpts[1].YM2612.SpecialFlags|=(settings[VGMPLAY_NUKEDOPN2_Option].detail.mdz_switch.switch_value<<3);
    
    ChipOpts[0].QSound.EmuCore=settings[VGMPLAY_QSoundEmulator].detail.mdz_switch.switch_value;
    ChipOpts[1].QSound.EmuCore=settings[VGMPLAY_QSoundEmulator].detail.mdz_switch.switch_value;
    
    VGMMaxLoop=settings[VGMPLAY_Maxloop].detail.mdz_slider.slider_value;
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
    m_genNumVoicesChannels=0;
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
                
                for (int j=m_genNumVoicesChannels;j<m_genNumVoicesChannels+vgmGetVoicesNb(CurChip);j++) {
                    m_voice_voiceColor[j]=m_voice_systemColor[vgmplay_activeChipsNb];
                }
                
                m_genNumVoicesChannels+=vgmGetVoicesChannelsUsedNb(CurChip);
                vgmplay_activeChipsNb++;
            }
            
            if (firstChip) {
                sprintf(mod_message+strlen(mod_message),"Chipsets....: %dx%s",chipNumber,strChip);
                firstChip=0;
            } else sprintf(mod_message+strlen(mod_message),",%dx%s",chipNumber,strChip);
            
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
            } else if (strcmp(strChip,"RF5C68")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM2203")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM2608")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM2610")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM3812")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YM3526")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YMF262")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YMF278B")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YMZ280B")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"PWM")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"AY8910")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"GameBoy")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"NES APU")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"YMW258")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"uPD7759")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"OKIM6258")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"OKIM6295")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"K051649")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"K054539")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"HuC6280")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"C140")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"K053260")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"VSU")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"C352")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"GA20")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"QSound")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"Y8950")==0) {
                m_voicesDataAvail=1;
            } else if (strcmp(strChip,"WSwan")==0) {
                m_voicesDataAvail=1;
            }
            
            /*
             "YMF271","RF5C164", "Pokey","SCSP","SAA1099", "ES5503", "ES5506", "X1-010"*/
            
            
        }
    }
    
    if (m_voicesDataAvail) m_genNumVoicesChannels=numChannels;
    else m_genNumVoicesChannels=0;
    
    sprintf(mod_message+strlen(mod_message),"\nAuthor......: %s\nGame........: %s\nSystem......: %s\nTitle.......: %s\nRelease Date: %s\nCreator.....: %s\nNotes.......: %s\n",
            [[self wcharToNS:GetTagStrEJ((bool)(settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value),VGMTag.strAuthorNameE,VGMTag.strAuthorNameJ)] UTF8String],
            [[self wcharToNS:GetTagStrEJ((bool)(settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value),VGMTag.strGameNameE,VGMTag.strGameNameJ)] UTF8String],
            [[self wcharToNS:GetTagStrEJ((bool)(settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value),VGMTag.strSystemNameE,VGMTag.strSystemNameJ)] UTF8String],
            [[self wcharToNS:GetTagStrEJ((bool)(settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value),VGMTag.strTrackNameE,VGMTag.strTrackNameJ)] UTF8String],
            [[self wcharToNS:VGMTag.strReleaseDate] UTF8String],
            [[self wcharToNS:VGMTag.strCreator] UTF8String],
            [[self wcharToNS:VGMTag.strNotes] UTF8String]);
    
    artist=[self wcharToNS:GetTagStrEJ((bool)(settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value),VGMTag.strAuthorNameE,VGMTag.strAuthorNameJ)];
    album=[self wcharToNS:GetTagStrEJ((bool)(settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value),VGMTag.strGameNameE,VGMTag.strGameNameJ)];
    if ([album length]==0) album=[filePath lastPathComponent];
    
    mod_title=[self wcharToNS:GetTagStrEJ((bool)(settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value),VGMTag.strGameNameE,VGMTag.strGameNameJ)];
    if (mod_title && ([mod_title length]==0)) mod_title=nil;
    
    //NSLog(@"loop: %d\n",VGMMaxLoopM);
    iModuleLength=vgmGetFileLength();//(VGMHead.lngTotalSamples+VGMMaxLoopM*VGMHead.lngLoopSamples)*10/441;//ms
    //NSLog(@"VGM length %d",iModuleLength);
    iCurrentTime=0;
    
    
    mod_minsub=0;
    mod_maxsub=0;
    mod_subsongs=1;
    
    sprintf(mod_name,"");
    if (GetTagStrEJ((bool)(settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value),VGMTag.strTrackNameE,VGMTag.strTrackNameJ)[0]) sprintf(mod_name," %s",[[self wcharToNS:GetTagStrEJ((bool)(settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value),VGMTag.strTrackNameE,VGMTag.strTrackNameJ)] UTF8String]);
    if (mod_name[0]==0) sprintf(mod_name," %s",mod_filename);
    
    [self mmp_updateDBStatsAtLoad];
    
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
    pmd_init((char*)[[filePath stringByDeletingLastPathComponent] UTF8String]);
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
        char *arg[4];
        arg[0]=NULL;
        arg[1]=(char*)[filePath UTF8String];
        arg[2]=NULL;
        arg[3]=NULL;
        pmd_play(arg, (char*)[[filePath stringByDeletingLastPathComponent] UTF8String]);
        
        //getlength((char*)[filePath UTF8String], &iModuleLength, &loop_length);
        iModuleLength=pmd_loop_msec();
        if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//PMD_DEFAULT_LENGTH;
        iCurrentTime=0;
        
        // these strings are SJIS
        pmd_get_title(tmp_mod_name);
        
        //printf("tmp_mod_name: %s",tmp_mod_name);
        if (tmp_mod_name[0]) sprintf(mod_name," %s",[[self sjisToNS:tmp_mod_name] UTF8String]);
        else sprintf(mod_name," %s",mod_filename);
        
        pmd_get_compo(tmp_mod_message);
        if (tmp_mod_message[0]) sprintf(mod_message,"Title: %s\nComposer: %s",[[self sjisToNS:tmp_mod_name] UTF8String], [[self sjisToNS:tmp_mod_message] UTF8String]);
        
        if (tmp_mod_message[0]) {
            artist=[self sjisToNS:tmp_mod_message];
        }
        if (tmp_mod_message[0]) {
            album=[filePath lastPathComponent];
        }
        
        // PMD doesn't have subsongs
        mod_subsongs=1;
        mod_minsub=1;
        mod_maxsub=1;
        mod_currentsub=1;
        
        numChannels=2;//pmd_get_tracks();
        
        [self mmp_updateDBStatsAtLoad];
        
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
    
    mod_minsub=0;
    mod_maxsub=ASAPInfo_GetSongs(ASAP_GetInfo(asap))-1;
    mod_subsongs=mod_maxsub+1;
    
    mod_currentsub = ASAPInfo_GetDefaultSong(ASAP_GetInfo(asap));
    mod_currentsub = ASAPInfo_GetDefaultSong(ASAP_GetInfo(asap));
    
    duration = ASAPInfo_GetDuration(ASAP_GetInfo(asap),mod_currentsub);
    
    //////////////////////////////////
    //update DB with songlength
    //////////////////////////////////
    for (int i=0;i<mod_subsongs; i++) {
        int subsong_length=ASAPInfo_GetDuration(ASAP_GetInfo(asap),i);
        mod_total_length+=subsong_length;
        
        NSString *filePathMain;
        NSString *fileName=[self getSubTitle:i];
        
        filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
        if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
        
        NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
        DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,subsong_length,numChannels,mod_subsongs);
        
        if (i==mod_subsongs-1) {// Global file stats update
            [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
        }
    }
    
    ASAP_PlaySong(asap, mod_currentsub, duration);
    ASAP_MutePokeyChannels(asap,0); //all channels active by default
    
    //    ASAPInfo_GetExtDescription
    
    sprintf(mod_message,"Author:%s\nTitle:%s\nSongs:%d\nChannels:%d\n",ASAPInfo_GetAuthor(ASAP_GetInfo(asap)),ASAPInfo_GetTitle(ASAP_GetInfo(asap)),ASAPInfo_GetSongs(ASAP_GetInfo(asap)),ASAPInfo_GetChannels(ASAP_GetInfo(asap))*4);
    
    artist=[NSString stringWithUTF8String:ASAPInfo_GetAuthor(ASAP_GetInfo(asap))];
    
    iModuleLength=(duration>=1000?duration:1000);
    iCurrentTime=0;
    
    numChannels=(ASAPInfo_GetChannels(ASAP_GetInfo(asap))*4);
    
    
    if (ASAPInfo_GetTitle(ASAP_GetInfo(asap))[0]) {
        sprintf(mod_name," %s",ASAPInfo_GetTitle(ASAP_GetInfo(asap)));
        mod_title=[NSString stringWithUTF8String:(const char*)ASAPInfo_GetTitle(ASAP_GetInfo(asap))];
    }
    else sprintf(mod_name," %s",mod_filename);
    
    stil_info[0]=0;
    [self getStilAsmaInfo:(char*)[filePath UTF8String]];
    
    sprintf(mod_message,"%s\n[STIL Information]\n%s\n",mod_message,stil_info);
    
    //Loop
    if (mLoopMode==1) {
        iModuleLength=-1;
    }
    
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
    if (mADPLUGopltype>3) mADPLUGopltype=0;
    switch (mADPLUGopltype) {
        case 0:
            if (mADPLUGstereosurround==1) { //surround
                COPLprops a, b;
                a.use16bit = b.use16bit = true;
                a.stereo = b.stereo = false;
                a.opl = new CWemuopl(PLAYBACK_FREQ, a.use16bit, a.stereo);
                b.opl = new CWemuopl(PLAYBACK_FREQ, b.use16bit, b.stereo);
                opl = new CSurroundopl(&a, &b, true);
            } else opl = new CWemuopl(PLAYBACK_FREQ, true, true);
            break;
        case 1: //Satoh
            if (mADPLUGstereosurround==1) { //surround
                COPLprops a, b;
                a.use16bit = b.use16bit = true;
                a.stereo = b.stereo = false;
                a.opl = new CEmuopl(PLAYBACK_FREQ, a.use16bit, a.stereo);
                b.opl = new CEmuopl(PLAYBACK_FREQ, b.use16bit, b.stereo);
                opl = new CSurroundopl(&a, &b, true);
            } else opl = new CEmuopl(PLAYBACK_FREQ, true, true);
            break;
        case 2:  //Ken
            if (mADPLUGstereosurround==1) { //surround
                COPLprops a, b;
                a.use16bit = b.use16bit = true;
                a.stereo = b.stereo = false;
                a.opl = new CKemuopl(PLAYBACK_FREQ, a.use16bit, a.stereo);
                b.opl = new CKemuopl(PLAYBACK_FREQ, b.use16bit, b.stereo);
                opl = new CSurroundopl(&a, &b, true);
            } else opl = new CKemuopl(PLAYBACK_FREQ, true, true);
            break;
        case 3: //Nuked
            if (mADPLUGstereosurround==1) { //surround
                COPLprops a, b;
                a.use16bit = b.use16bit = true;
                a.stereo = b.stereo = true; //Nuked works only in stereo
                a.opl = new CNemuopl(PLAYBACK_FREQ);
                b.opl = new CNemuopl(PLAYBACK_FREQ);
                opl = new CSurroundopl(&a, &b, true);
            } else opl = new CNemuopl(PLAYBACK_FREQ);
            break;
    }
    
    adplugDB = new CAdPlugDatabase;
    
    NSString *db_path = [[NSBundle mainBundle] resourcePath];
    
    adplugDB->load ([[NSString stringWithFormat:@"%@/adplug.db",db_path] UTF8String]);    // load user's database
    CAdPlug::set_database (adplugDB);
    
    //CAdPlug::printAllExtensionSupported();
    
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
        if (adPlugPlayer->update()) opl_towrite=(int)(PLAYBACK_FREQ*1.0f/adPlugPlayer->getrefresh());
        
        std::string title=adPlugPlayer->gettitle();
        
        if (title.length()) sprintf(mod_name," %s",title.c_str());
        else sprintf(mod_name," %s",mod_filename);
        mod_message[0]=0;
        
        if ((adPlugPlayer->gettype().length()>0)) snprintf(mod_message,MAX_STIL_DATA_LENGTH*2,"%sType.......: %s\n", mod_message,adPlugPlayer->gettype().c_str());
        
        
        if ((adPlugPlayer->getauthor()).length()>0)	{
            snprintf(mod_message,MAX_STIL_DATA_LENGTH*2,"%sAuthor.....: %s\n", mod_message,adPlugPlayer->getauthor().c_str());
            artist=[NSString stringWithUTF8String:adPlugPlayer->getauthor().c_str()];
        }
        
        if ((adPlugPlayer->getdesc()).length()>0) snprintf(mod_message,MAX_STIL_DATA_LENGTH*2,"%sDescription: %s\n",mod_message, adPlugPlayer->getdesc().c_str());
        
        for (int i=0;i<adPlugPlayer->getinstruments();i++) {
            snprintf(mod_message,MAX_STIL_DATA_LENGTH*2,"%s%s\n", mod_message, adPlugPlayer->getinstrument(i).c_str());
        };
        
        iCurrentTime=0;
        iModuleLength=adPlugPlayer->songlength();
        
        numChannels=18; //OPL3
        
        m_voicesDataAvail=1;
        m_voice_current_system=1;
        if (m_voicesDataAvail) m_genNumVoicesChannels=numChannels;
        for (int i=0;i<m_genNumVoicesChannels;i++) {
            m_voice_voiceColor[i]=m_voice_systemColor[0];
        }
        
        mod_subsongs=adPlugPlayer->getsubsongs();
        mod_minsub=0;
        mod_maxsub=mod_subsongs-1;
        mod_currentsub=adPlugPlayer->getsubsong();
        
        generic_mute_mask=0;
        
        //////////////////////////////////
        //update DB with songlength
        //////////////////////////////////
        if (mod_subsongs>1) {
            for (int i=0;i<mod_subsongs; i++) {
                int subsong_length=adPlugPlayer->songlength(i);
                mod_total_length+=subsong_length;
                
                NSString *filePathMain;
                NSString *fileName=[self getSubTitle:i];
                
                filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
                if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
                
                NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
                DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,subsong_length,numChannels,mod_subsongs);
                
                if (i==mod_subsongs-1) {// Global file stats update
                    [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
                }
            }
        } else {
            [self mmp_updateDBStatsAtLoad];
        }
        
        
        
        
        
        //Loop
        if (mLoopMode==1) iModuleLength=-1;
        
        return 0;
    }
}

-(int) mmp_gmeLoad:(NSString*)filePath {
    int64_t sample_rate = PLAYBACK_FREQ; /* number of samples per second */
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
        bool let_libkss_takeover=false;
        if (gme_track_info( gme_emu, &gme_info, 0 )==0) {
            //if ((strcmp(gme_info->system,"MSX")==0)||(strcmp(gme_info->system,"MSX + FM Sound")==0)) let_libkss_takeover=true;
            gme_free_info(gme_info);
            gme_info=NULL;
        }
        if (let_libkss_takeover) {
            mPlayType=0;
            if (gme_emu) gme_delete( gme_emu );
            gme_emu=NULL;
            return -10;
        }
        /* Register cleanup function and confirmation string as data */
        gme_set_user_data( gme_emu, my_data );
        gme_set_user_cleanup( gme_emu, my_cleanup );
        
        gme_enable_accuracy(gme_emu,1);
        
        //get initial value from emu core
        gme_equalizer( gme_emu, &gme_eq );
        gme_eq_treble_orig=gme_eq.treble;
        gme_eq_bass_orig=gme_eq.bass;
        
        //is a m3u available ?
        NSString *tmpStr=[NSString stringWithFormat:@"%@.m3u",[filePath stringByDeletingPathExtension]];
        err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
        if (err) {
            NSString *tmpStr=[NSString stringWithFormat:@"%@.M3U",[filePath stringByDeletingPathExtension]];
            err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
        }
        
        [self optGME_Update]; //update EQ & Stereo
                        
        /**/
        gme_ignore_silence(gme_emu,!settings[GME_IGNORESILENCE].detail.mdz_boolswitch.switch_value);
        
        mod_subsongs=gme_track_count( gme_emu );
        mod_minsub=0;
        mod_maxsub=mod_subsongs-1;
        mod_currentsub=0;
        
        numChannels=gme_voice_count( gme_emu );
        
        // Start track
        err=gme_start_track( gme_emu, mod_currentsub );
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
        for (int i=0;i<mod_subsongs; i++) {
            if (gme_track_info( gme_emu, &gme_info, i )==0) {
                NSString *filePathMain;
                NSString *fileName;
                NSMutableArray *tmp_path;
                int gme_subsong_length=gme_info->play_length;
                if (gme_info->play_length<=0) gme_info->play_length=optGENDefaultLength;
                mod_total_length+=gme_info->play_length;
                
                gme_free_info(gme_info);
                
                fileName=[self getSubTitle:i];
                //NSLog(@"%@",mod_loadmodule_filepath);
                filePathMain=[ModizFileHelper getFilePathFromDocuments:mod_loadmodule_filepath];
                if (mdz_ArchiveFilesCnt) filePathMain=[NSString stringWithFormat:@"%@@%d",filePathMain,mdz_currentArchiveIndex];
                
                NSString *filePathSubsong=[NSString stringWithFormat:@"%@?%d",filePathMain,i];
                DBHelper::updateFileStatsDBmod(fileName,filePathSubsong,-1,-1,-1,gme_subsong_length,gme_voice_count( gme_emu ),mod_subsongs);
                
                if (i==mod_subsongs-1) {// Global file stats update
                    [self mmp_updateDBStatsAtLoadSubsong:mod_total_length];
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
                        
            if (gme_info->song){
                if (gme_info->song[0]) mod_title=[NSString stringWithCString:gme_info->song encoding:NSShiftJISStringEncoding];
                else mod_title=NULL;
                //if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);
            }
            
            sprintf(mod_name," %s",mod_filename);
            if (gme_info->game){
                if (gme_info->game[0]) {
                    //mod_title=[NSString stringWithCString:gme_info->game encoding:NSShiftJISStringEncoding];
                    album=[NSString stringWithCString:gme_info->game encoding:NSShiftJISStringEncoding];
                    snprintf(mod_name+1,sizeof(mod_name),[album UTF8String]);
                }
            }
            
            if (gme_info->author) {
                artist=[NSString stringWithCString:gme_info->author encoding:NSShiftJISStringEncoding];
            }
            
            if ([album length]==0) album=[filePath lastPathComponent];
            
            gme_free_info(gme_info);
        } else {
            mod_title=[NSString stringWithFormat:@"%.3d",mod_currentsub-mod_minsub+1];
            strcpy(gmetype,"N/A");
            strcpy(mod_message,"N/A\n");
            iModuleLength=optGENDefaultLength;
            sprintf(mod_name," %s",mod_filename);
        }
        
        //Loop
        if (mLoopMode==1) iModuleLength=-1;
        if (iModuleLength>0) {
            if (iModuleLength>settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000) gme_set_fade_msecs( gme_emu, iModuleLength-settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000,settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000 ); //Fade 1s before end
            else gme_set_fade_msecs( gme_emu, iModuleLength/2, iModuleLength/2 );
        } else gme_set_fade_msecs( gme_emu, 1<<30,settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000 );
        //            else gme_set_fade_msecs( gme_emu, 1<<30);
        
        iCurrentTime=0;
        
        if (m_voicesDataAvail) {
            m_genNumVoicesChannels=numChannels;
        }
        
        for (int i=0;i<m_genNumVoicesChannels;i++) {
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

extern NSURL *icloudURL;
extern bool icloud_available;


-(NSString*) getFullFilePath:(NSString *)_filePath {
    NSString *fullFilePath;
    
    if ([_filePath length]>2) {
        if (([_filePath characterAtIndex:0]=='/')&&([_filePath characterAtIndex:1]=='/')) fullFilePath=[_filePath substringFromIndex:2];
        else {
            if (icloud_available && ([_filePath containsString:[icloudURL path]])) fullFilePath=[NSString stringWithString:_filePath];
            else fullFilePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
        }
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

-(void) updateCurSubSongPlayed:(int)idx {
    int maxid=0;
    if (!mdz_SubsongPlayed) return;
    
    for (int i=0;i<mod_subsongs;i++) {
        if (maxid<mdz_SubsongPlayed[i]) maxid=mdz_SubsongPlayed[i];
    }
    mdz_SubsongPlayed[idx]=maxid+1;
    
    //printf("sub data:\n");
    //for (int i=0;i<mod_subsongs;i++) printf("%d ",mdz_SubsongPlayed[i]);
    //printf("\n");
}

-(void) initSubSongPlayed {
    if (mdz_SubsongPlayed) free(mdz_SubsongPlayed);
    mdz_SubsongPlayed=NULL;
    
    if (mod_subsongs>1) {
        mdz_SubsongPlayed=(int*)malloc(mod_subsongs*sizeof(int));
        memset(mdz_SubsongPlayed,0,mod_subsongs*sizeof(int));
        //[self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
    }
}

-(int) LoadModule:(NSString*)_filePath archiveMode:(int)archiveMode archiveIndex:(int)archiveIndex singleSubMode:(int)singleSubMode singleArcMode:(int)singleArcMode detailVC:(DetailViewControllerIphone*)detailVC isRestarting:(bool)isRestarting shuffle:(bool)shuffle{
    NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
    NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
    NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
    NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
    NSArray *filetype_extATARISOUND=[SUPPORTED_FILETYPE_ATARISOUND componentsSeparatedByString:@","];
    NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
    NSArray *filetype_extPT3=[SUPPORTED_FILETYPE_PT3 componentsSeparatedByString:@","];
    NSArray *filetype_extGBS=[SUPPORTED_FILETYPE_GBS componentsSeparatedByString:@","];
    NSArray *filetype_extNSFPLAY=[SUPPORTED_FILETYPE_NSFPLAY componentsSeparatedByString:@","];
    NSArray *filetype_extPIXEL=[SUPPORTED_FILETYPE_PIXEL componentsSeparatedByString:@","];
    NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
    NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_OMPT componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_ext2SF=[SUPPORTED_FILETYPE_2SF componentsSeparatedByString:@","];
    NSArray *filetype_extV2M=[SUPPORTED_FILETYPE_V2M componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSArray *filetype_extHC=[SUPPORTED_FILETYPE_HC componentsSeparatedByString:@","];
    NSArray *filetype_extEUP=[SUPPORTED_FILETYPE_EUP componentsSeparatedByString:@","];
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extS98=[SUPPORTED_FILETYPE_S98 componentsSeparatedByString:@","];
    NSArray *filetype_extKSS=[SUPPORTED_FILETYPE_KSS componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    char mdz_defaultMODPLAYER,mdz_defaultSAPPLAYER,mdz_defaultVGMPLAYER,mdz_defaultNSFPLAYER;
    char mdz_defaultKSSPLAYER,mdz_defaultMIDIPLAYER,mdz_defaultSIDPLAYER,mdz_defaultGBSPLAYER;

    NSString *extension;
    NSString *file_no_ext;
    NSString *filePath;
    NSMutableArray *available_player=[NSMutableArray arrayWithCapacity:1];
    
    int found=0;
    volatile static bool no_reentrant=false;
    
    if (no_reentrant) {
        return 2;
    }
    
    no_reentrant=true;
    
    mod_loadmodule_filepath=[NSString stringWithString:_filePath];
    
    detailViewControllerIphone=detailVC;
    
    mplayer_error_msg[0]=0;
    mSingleSubMode=singleSubMode;
    
    [self iPhoneDrv_LittlePlayStart];
    
    mdz_defaultMODPLAYER=settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value;
    mdz_defaultSAPPLAYER=settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value;
    mdz_defaultVGMPLAYER=settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value;
    mdz_defaultNSFPLAYER=settings[GLOB_DefaultNSFPlayer].detail.mdz_switch.switch_value;
    mdz_defaultGBSPLAYER=settings[GLOB_DefaultGBSPlayer].detail.mdz_switch.switch_value;
    mdz_defaultKSSPLAYER=settings[GLOB_DefaultKSSPlayer].detail.mdz_switch.switch_value;
    mdz_defaultMIDIPLAYER=settings[GLOB_DefaultMIDIPlayer].detail.mdz_switch.switch_value;
    mdz_defaultSIDPLAYER=settings[GLOB_DefaultSIDPlayer].detail.mdz_switch.switch_value;
    
    if (archiveMode==0) {
        //extension = [_filePath pathExtension];
        //file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
        
        NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
        extension = (NSString *)[temparray_filepath lastObject];
        //[temparray_filepath removeLastObject];
        file_no_ext=[temparray_filepath firstObject];
        
        
        
        //filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
        filePath=[self getFullFilePath:_filePath];
        
        
        
        mdz_IsArchive=0;
        mNeedSeek=0;
        mod_message_updated=0;
        mod_subsongs=1;
        mod_currentsub=mod_minsub=mod_maxsub=0;
        mod_wantedcurrentsub=-1;
        moveToSubSong=0;
        moveToSubSongIndex=0;
        iModuleLength=0;
        mPatternDataAvail=0;
        
        sprintf(mod_filename,"%s",[[[filePath lastPathComponent] stringByDeletingPathExtension] UTF8String]);
        
        
        if (mdz_ArchiveFilesCnt) {
            for (int i=0;i<mdz_ArchiveFilesCnt;i++) free(mdz_ArchiveFilesList[i]);
            free(mdz_ArchiveFilesList);
            mdz_ArchiveFilesList=NULL;
            mdz_ArchiveFilesCnt=0;
            if (mdz_ArchiveEntryPlayed) free(mdz_ArchiveEntryPlayed);
            mdz_ArchiveEntryPlayed=NULL;
        }
        
        for (int i=0;i<[filetype_extARCHIVE count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extARCHIVE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            //if ([file_no_ext caseInsensitiveCompare:[filetype_extARCHIVE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
        if (found) { //might be an archived file
            mdz_IsArchive=0;
            mdz_ArchiveFilesCnt=0;
            
            strcpy(archive_filename,mod_filename);
            
            if (found==1) { //Archive
                                
                [ModizFileHelper scanarchive:[filePath UTF8String] filesList_ptr:nil filesCount_ptr:&mdz_ArchiveFilesCnt];
                
                //NSLog(@"scan done");
                if (mdz_ArchiveFilesCnt) {
                    mdz_IsArchive=1;
                    
                    FILE *f;
                    f = fopen([filePath UTF8String], "rb");
                    if (f == NULL) {
                        NSLog(@"Cannot open file %@",filePath);
                        no_reentrant=false;
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
                    
                    
                    if (found==1) { //Archive
                        if ([self extractToPath:[filePath UTF8String] path:[tmpArchivePath UTF8String] isRestarting:isRestarting]) {
                            //issue
                            if (mdz_ArchiveFilesCnt) {
                                for (int i=0;i<mdz_ArchiveFilesCnt;i++) {
                                    mdz_safe_free(mdz_ArchiveFilesList[i])
                                }
                                mdz_safe_free(mdz_ArchiveFilesList)
                                mdz_ArchiveFilesCnt=0;
                            }
                            no_reentrant=false;
                            return -2;
                        }
                    } else {
                    }
                    mdz_currentArchiveIndex=0;
                    
                    //sort the file list
                    if (mdz_ArchiveFilesCnt>1) qsort(mdz_ArchiveFilesList, mdz_ArchiveFilesCnt, sizeof(char*), &mdz_ArchiveFiles_compare);
                    
                    if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) mdz_currentArchiveIndex=archiveIndex;
                    else if (mdz_ShufflePlayMode) mdz_currentArchiveIndex=arc4random()%mdz_ArchiveFilesCnt;
                    
                    _filePath=[NSString stringWithFormat:@"tmp/tmpArchive/%@",[NSString stringWithUTF8String:mdz_ArchiveFilesList[mdz_currentArchiveIndex]]];
                    //extension = [_filePath pathExtension];
                    //file_no_ext = [[_filePath lastPathComponent] stringByDeletingPathExtension];
                    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
                    extension = (NSString *)[temparray_filepath lastObject];
                    //[temparray_filepath removeLastObject];
                    file_no_ext=[temparray_filepath firstObject];
                    
                    
                    //filePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
                    filePath=[self getFullFilePath:_filePath];
                    sprintf(mod_filename,"%s/%s",archive_filename,[[filePath lastPathComponent] UTF8String]);
                    
                    //NSLog(@"%@",_filePath);
                    
                } else {
                    //no file found, try as regular file
                    found=0;
                    //no_reentrant=false;
                    //return -1;
                }
            }
        }
        
        if (mdz_IsArchive && mdz_ArchiveFilesCnt) {
            mdz_ArchiveEntryPlayed=(int*)malloc(mdz_ArchiveFilesCnt*sizeof(int));
            memset(mdz_ArchiveEntryPlayed,0,mdz_ArchiveFilesCnt*sizeof(int));
        }
    } else {//archiveMode
        mNeedSeek=0;
        mod_message_updated=0;
        mod_subsongs=1;
        mod_currentsub=mod_minsub=mod_maxsub=0;
        mod_wantedcurrentsub=-1;
        moveToSubSong=0;
        moveToSubSongIndex=0;
        iModuleLength=0;
        mPatternDataAvail=0;
        
        if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) {
            mdz_currentArchiveIndex=archiveIndex;
        }
        
    }
    
    if (mdz_IsArchive&&mdz_ArchiveFilesCnt) {
        
        //check if specific entry is to select
        if (archiveIndex) {
            if ((archiveIndex>=0)&&(archiveIndex<mdz_ArchiveFilesCnt)) {
                mdz_currentArchiveIndex=archiveIndex;
            }
        }
        //increase play counter
        int max_idx=0;
        for (int j=0;j<mdz_ArchiveFilesCnt;j++) {
            if (max_idx<mdz_ArchiveEntryPlayed[j]) max_idx=mdz_ArchiveEntryPlayed[j];
        }
        mdz_ArchiveEntryPlayed[mdz_currentArchiveIndex]=max_idx+1;
        
        //init file references
        _filePath=[NSString stringWithFormat:@"tmp/tmpArchive/%@",[NSString stringWithUTF8String:mdz_ArchiveFilesList[mdz_currentArchiveIndex]]];
        NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[_filePath lastPathComponent] componentsSeparatedByString:@"."]];
        extension = (NSString *)[temparray_filepath lastObject];
        file_no_ext=[temparray_filepath firstObject];
        
        filePath=[self getFullFilePath:_filePath];
        sprintf(mod_filename,"%s/%s",archive_filename,[[filePath lastPathComponent] UTF8String]);
    }
    
    if (filePath==NULL) {
        no_reentrant=false;
        return -1;
    }
    
    for (int i=0;i<[filetype_extNSFPLAY count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultNSFPLAYER==DEFAULT_NSFNSFPLAY) [available_player addObject:[NSNumber numberWithInt:MMP_NSFPLAY]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:i]]==NSOrderedSame) {
        //            if (mdz_defaultNSFPLAYER==DEFAULT_NSFNSFPLAY) [available_player addObject:[NSNumber numberWithInt:MMP_NSFPLAY]];
        //            break;
        //        }
    }
    
    
    for (int i=0;i<[filetype_extVGM count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultVGMPLAYER==DEFAULT_VGMVGM) [available_player insertObject:[NSNumber numberWithInt:MMP_VGMPLAY] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_VGMPLAY]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {
        //            if (mdz_defaultVGMPLAYER==DEFAULT_VGMVGM) [available_player insertObject:[NSNumber numberWithInt:MMP_VGMPLAY] atIndex:0];
        //            else [available_player addObject:[NSNumber numberWithInt:MMP_VGMPLAY]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extASAP count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultSAPPLAYER==DEFAULT_SAPASAP) [available_player insertObject:[NSNumber numberWithInt:MMP_ASAP] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_ASAP]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {
        //            if (mdz_defaultSAPPLAYER==DEFAULT_SAPASAP) [available_player insertObject:[NSNumber numberWithInt:MMP_ASAP] atIndex:0];
        //            else [available_player addObject:[NSNumber numberWithInt:MMP_ASAP]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extGBS count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extGBS objectAtIndex:i]]==NSOrderedSame) {
            if (mdz_defaultGBSPLAYER==DEFAULT_GBSGBSPLAY) [available_player addObject:[NSNumber numberWithInt:MMP_GBS]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extGBS objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_GBS]];
        //            break;
        //        }
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
            bool is_nsf=false;
            for (int j=0;j<[filetype_extNSFPLAY count];j++)
                if ([extension caseInsensitiveCompare:[filetype_extNSFPLAY objectAtIndex:j]]==NSOrderedSame) {
                    is_nsf=true;
                    break;
                }
            bool is_kss=false;
            if ([extension caseInsensitiveCompare:@"KSS"]==NSOrderedSame) {
                is_kss=true;
            }
            bool is_gbs=false;
            if ([extension caseInsensitiveCompare:@"GBS"]==NSOrderedSame) {
                is_gbs=true;
            }
            if ( (is_vgm && (mdz_defaultVGMPLAYER==DEFAULT_VGMGME)) ||
                (is_sap && (mdz_defaultSAPPLAYER==DEFAULT_SAPGME)) ||
                (is_kss && (mdz_defaultKSSPLAYER==DEFAULT_KSSGME)) ||
                (is_nsf && (mdz_defaultNSFPLAYER==DEFAULT_NSFGME)) ||
                (is_gbs && (mdz_defaultGBSPLAYER==DEFAULT_GBSGME)) ) [available_player insertObject:[NSNumber numberWithInt:MMP_GME] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_GME]];
            break;
        }
    }
    
    for (int i=0;i<[filetype_extMDX count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_MDXPDX]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_MDXPDX]];
        //            break;
        //        }
    }
    
    for (int i=0;i<[filetype_extSTSOUND count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_STSOUND]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_STSOUND]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extATARISOUND count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extATARISOUND objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_ATARISOUND]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extATARISOUND objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_ATARISOUND]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extSC68 count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_SC68]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_SC68]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extPT3 count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extPT3 objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_PT3]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extPT3 objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_PT3]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extPIXEL count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extPIXEL objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_PIXEL]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extPIXEL objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_PIXEL]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_ext2SF count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_ext2SF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_2SF]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_ext2SF objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_2SF]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extV2M count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extV2M objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_V2M]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extV2M objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_V2M]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extHC count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extHC objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_HC]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extHC objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_HC]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extEUP count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extEUP objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_EUP]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extEUP objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_EUP]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extVGMSTREAM count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_VGMSTREAM]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_VGMSTREAM]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extGSF count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_GSF]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_GSF]];
        //            break;
        //        }
    }
    for (int i=0;i<[filetype_extWMIDI count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_TIMIDITY]];break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_TIMIDITY]];break;
        //        }
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
    
    for (int i=0;i<[filetype_extHVL count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_HVL]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_HVL]];
        //            break;
        //        }
    }
    
    for (int i=0;i<[filetype_extS98 count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extS98 objectAtIndex:i]]==NSOrderedSame) {
            [available_player addObject:[NSNumber numberWithInt:MMP_S98]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extS98 objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player addObject:[NSNumber numberWithInt:MMP_S98]];
        //            break;
        //        }
    }
    
    for (int i=0;i<[filetype_extKSS count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extKSS objectAtIndex:i]]==NSOrderedSame) {
            if ((mdz_defaultKSSPLAYER==DEFAULT_KSSLIBKSS)||([extension caseInsensitiveCompare:@"KSS"]!=NSOrderedSame)) [available_player insertObject:[NSNumber numberWithInt:MMP_KSS] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_KSS]];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extKSS objectAtIndex:i]]==NSOrderedSame) {
        //            if ((mdz_defaultKSSPLAYER==DEFAULT_KSSLIBKSS)||([extension caseInsensitiveCompare:@"KSS"]!=NSOrderedSame)) [available_player insertObject:[NSNumber numberWithInt:MMP_KSS] atIndex:0];
        //            else [available_player addObject:[NSNumber numberWithInt:MMP_KSS]];
        //            break;
        //        }
    }
    bool is_potential_mod=false;
    for (int i=0;i<[filetype_extXMP count];i++) {
        if ( ([extension caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) ||
            ([file_no_ext caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) ) {
            is_potential_mod=true;
        }
    }
    for (int i=0;i<[filetype_extMODPLUG count];i++) {
        if ( ([extension caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) ||
            ([file_no_ext caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) ) {
            is_potential_mod=true;
        }
    }
    for (int i=0;i<[filetype_extUADE count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {
            if ((mdz_defaultMODPLAYER==DEFAULT_UADE)&&is_potential_mod) [available_player insertObject:[NSNumber numberWithInt:MMP_UADE] atIndex:0];
            else [available_player addObject:[NSNumber numberWithInt:MMP_UADE]];
            break;
        }
        if ([file_no_ext caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {
            if ((mdz_defaultMODPLAYER==DEFAULT_UADE)&&is_potential_mod) [available_player insertObject:[NSNumber numberWithInt:MMP_UADE] atIndex:0];
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
    // 4. The rest
    //****************************************************************************************
    
    for (int i=0;i<[filetype_extSID count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {
            [available_player insertObject:[NSNumber numberWithInt:(mdz_defaultSIDPLAYER?MMP_WEBSID:MMP_SIDPLAY)] atIndex:0];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player insertObject:[NSNumber numberWithInt:(mdz_defaultSIDPLAYER?MMP_WEBSID:MMP_SIDPLAY)] atIndex:0];
        //            break;
        //        }
    }
    
    for (int i=0;i<[filetype_extADPLUG count];i++) {
        bool is_mid=false;
        if ([@"MID" caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {
            is_mid=true;
        }
        
        if (is_mid) {
            if (mdz_defaultMIDIPLAYER==1) { //Priority over Timidity
                if ([extension caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {
                    [available_player insertObject:[NSNumber numberWithInt:MMP_ADPLUG] atIndex:0];
                    break;
                }
                //                if ([file_no_ext caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {
                //                    [available_player insertObject:[NSNumber numberWithInt:MMP_ADPLUG] atIndex:0];
                //                    break;
                //                }
            } else {
                //let Timidity plays
            }
        } else {
            if ([extension caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {
                if (mADPLUGPriorityOverMod) [available_player insertObject:[NSNumber numberWithInt:MMP_ADPLUG] atIndex:0];
                else [available_player addObject:[NSNumber numberWithInt:MMP_ADPLUG]];
                break;
            }
            //            if ([file_no_ext caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {
            //                if (mADPLUGPriorityOverMod) [available_player insertObject:[NSNumber numberWithInt:MMP_ADPLUG] atIndex:0];
            //                else [available_player addObject:[NSNumber numberWithInt:MMP_ADPLUG]];
            //                break;
            //            }
        }
    }
    
    for (int i=0;i<[filetype_extPMD count];i++) {
        if ([extension caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {
            [available_player insertObject:[NSNumber numberWithInt:MMP_PMDMINI] atIndex:0];
            break;
        }
        //        if ([file_no_ext caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {
        //            [available_player insertObject:[NSNumber numberWithInt:MMP_PMDMINI] atIndex:0];
        //            break;
        //        }
    }
    
    //NSLog(@"Loading file:%@ ",filePath);
    //NSLog(@"Loading file:%@ ext:%@",file_no_ext,extension);
    
    mod_total_length=0;
    
    mod_currentfile=[NSString stringWithString:filePath];
    mod_currentext=[NSString stringWithString:extension];
    
    artist=@"";
    album=[filePath lastPathComponent];
    mod_title=nil;
    
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
    m_voice_current_sample=0;
    sprintf(mmp_fileext,"%s",[extension UTF8String] );
    mod_title=nil;
    
    
    /*if (mdz_IsArchive && mdz_ArchiveFilesCnt) {
     printf("current play stat:\n");
     for (int i=0;i<mdz_ArchiveFilesCnt;i++) printf("%d ",mdz_ArchiveEntryPlayed[i]);
     printf("\n");
     }*/
    
    int retval=1;
    
    for (int i=0;i<[available_player count];i++) {
        int pl_idx=[((NSNumber*)[available_player objectAtIndex:i]) intValue];
        if (!retval) break;
        //NSLog(@"pl_idx: %d",i);
        switch (pl_idx) {
            case MMP_TIMIDITY:
                retval=[self mmp_timidityLoad:filePath];
                break;
            case MMP_VGMSTREAM:
                retval=[self mmp_vgmstreamLoad:filePath extension:extension];
                break;
            case MMP_2SF:
                retval=[self mmp_2sfLoad:filePath extension:extension];
                break;
            case MMP_V2M:
                retval=[self mmp_v2mLoad:filePath extension:extension];
                break;
            case MMP_VGMPLAY:
                retval=[self mmp_vgmplayLoad:filePath];
                break;
            case MMP_PMDMINI:
                retval=[self mmp_pmdminiLoad:filePath];
                break;
            case MMP_ASAP:
                retval=[self mmp_asapLoad:filePath];
                break;
            case MMP_GSF:
                retval=[self mmp_gsfLoad:filePath];
                break;
            case MMP_MDXPDX:
                retval=[self mmp_mdxpdxLoad:filePath];
                break;
            case MMP_SC68:
                retval=[self mmp_sc68Load:filePath];
                break;
            case MMP_STSOUND:
                retval=[self mmp_stsoundLoad:filePath];
                break;
            case MMP_ATARISOUND:
                retval=[self mmp_AtariSoundLoad:filePath];
                break;
            case MMP_PT3:
                retval=[self mmp_pt3Load:filePath];
                break;
            case MMP_GBS:
                retval=[self mmp_gbsLoad:filePath];
                break;
            case MMP_NSFPLAY:
                retval=[self mmp_nsfplayLoad:filePath];
                break;
            case MMP_PIXEL:
                retval=[self mmp_pixelLoad:filePath];
                break;
            case MMP_EUP:
                retval=[self mmp_eupLoad:filePath];
                break;
            case MMP_SIDPLAY:
                retval=[self mmp_sidplayLoad:filePath];
                break;
            case MMP_WEBSID:
                retval=[self mmp_websidLoad:filePath];
                break;
            case MMP_HVL:
                retval=[self mmp_hvlLoad:filePath];
                break;
            case MMP_S98:
                retval=[self mmp_s98Load:filePath];
                break;
            case MMP_KSS:
                retval=[self mmp_kssLoad:filePath];
                break;
            case MMP_XMP:
                retval=[self mmp_xmpLoad:filePath];
                break;
            case MMP_UADE:
                retval=[self mmp_uadeLoad:filePath];
                break;
            case MMP_HC:
                retval=[self MMP_HCLoad:filePath];
                break;
            case MMP_ADPLUG:
                retval=[self mmp_adplugLoad:filePath];
                break;
            case MMP_OPENMPT:
                retval=[self mmp_openmptLoad:filePath];
                break;
            case MMP_GME:
                retval=[self mmp_gmeLoad:filePath];
                break;
            default:
                //Could not find a lib to load module
                NSLog(@"Unsupported player: %d",pl_idx); //Should never happen
                break;
        }
    }
    [self initSubSongPlayed];
    no_reentrant=false;
    mLoadModuleStatus=retval;
    
    //clear oscillo data ptr
    for (int i=0;i<SOUND_MAXVOICES_BUFFER_FX;i++) {
        m_voice_current_ptr[i]=0;
        m_voice_prev_current_ptr[i]=0;
        memset(m_voice_buff[i],0,SOUND_BUFFER_SIZE_SAMPLE*2*4);
        
        memset(m_voice_buff_accumul_temp[i],0,SOUND_BUFFER_SIZE_SAMPLE*sizeof(int)*2);
        memset(m_voice_buff_accumul_temp_cnt[i],0,SOUND_BUFFER_SIZE_SAMPLE*2);
    }
    
    return retval;
}
//*****************************************
//Playback commands

-(void) setArchiveSubShuffle:(bool)shuffle {
    mdz_ShufflePlayMode=shuffle;
}
-(int) selectPrevArcEntry {
    if (mdz_IsArchive&&mdz_ArchiveFilesCnt) {
        if (! mdz_ShufflePlayMode) {
            if (mdz_currentArchiveIndex>0) {
                [self Stop];
                mdz_currentArchiveIndex--;
            } else return -1; //end reached
        } else {
            int playedCnt=0;
            int max_idx=0;
            int next_entry=-1;
            //cancel current index
            mdz_ArchiveEntryPlayed[mdz_currentArchiveIndex]=0;
            for (int i=0;i<mdz_ArchiveFilesCnt;i++) {
                if (mdz_ArchiveEntryPlayed[i]) playedCnt++;
                if (max_idx<mdz_ArchiveEntryPlayed[i]) {
                    max_idx=mdz_ArchiveEntryPlayed[i];
                    next_entry=i;
                }
            }
            if (playedCnt>0) {
                [self Stop];
                //select randomly prev played one
                mdz_currentArchiveIndex=next_entry;
                mdz_ArchiveEntryPlayed[mdz_currentArchiveIndex]--;
            } else return -1;
        }
        
    }
    return mdz_currentArchiveIndex;
}
-(int) selectNextArcEntry {
    if (mdz_IsArchive&&mdz_ArchiveFilesCnt) {
        if (! mdz_ShufflePlayMode) {
            if (mdz_currentArchiveIndex<(mdz_ArchiveFilesCnt-1)) {
                [self Stop];
                mdz_currentArchiveIndex++;
            } else return -1; //end reached
        } else {
            int playedCnt=0;
            for (int i=0;i<mdz_ArchiveFilesCnt;i++) if (mdz_ArchiveEntryPlayed[i]) playedCnt++;
            if (playedCnt<mdz_ArchiveFilesCnt) {
                [self Stop];
                //select randomly next unplayed one
                mdz_currentArchiveIndex=arc4random()%mdz_ArchiveFilesCnt;
                while (mdz_ArchiveEntryPlayed[mdz_currentArchiveIndex]) {
                    mdz_currentArchiveIndex++;
                    if (mdz_currentArchiveIndex>=mdz_ArchiveFilesCnt) mdz_currentArchiveIndex=0;
                }
            } else return -1;
        }
    }
    return mdz_currentArchiveIndex;
}
-(void) selectArcEntry:(int)arc_index{
    if (mdz_IsArchive&&mdz_ArchiveFilesCnt) {
        if ((arc_index>=0)&&(arc_index<mdz_ArchiveFilesCnt)) {
            [self Stop];
            mdz_currentArchiveIndex=arc_index;
        }
    }
}
-(int) playPrevSub{
    if (mod_subsongs<=1) return -1;
    
    if (! mdz_ShufflePlayMode) {
        //Sequencial
        if (mod_currentsub>mod_minsub) moveToSubSongIndex=mod_currentsub-1;
        else return -1;
    } else {
        //Random
        int playedCnt=0;
        int max_idx=0;
        int next_entry=-1;
        //cancel current index
        mdz_SubsongPlayed[mod_currentsub-mod_minsub]=0;
        for (int i=0;i<mod_subsongs;i++) {
            if (mdz_SubsongPlayed[i]) playedCnt++;
            if (max_idx<mdz_SubsongPlayed[i]) {
                max_idx=mdz_SubsongPlayed[i];
                next_entry=i;
            }
        }
        if (playedCnt>0) {
            //[self Stop];
            //select randomly prev played one
            moveToSubSongIndex=next_entry+mod_minsub;
            mdz_SubsongPlayed[next_entry]--;
        } else return -1;
    }
    
    moveToSubSong=1;
    [self updateCurSubSongPlayed:moveToSubSongIndex-mod_minsub];
    return moveToSubSongIndex;
}

-(int) playNextSub{
    if (mod_subsongs<=1) return -1;
    //moveToNextSubSong=1;
    
    if (! mdz_ShufflePlayMode) {
        //Sequencial
        if (mod_currentsub<mod_maxsub) moveToSubSongIndex=mod_currentsub+1;
        else return -1;
    } else {
        //Random
        int playedCnt=0;
        int nextsubidx=0;
        for (int i=0;i<mod_subsongs;i++) if (mdz_SubsongPlayed[i]) playedCnt++;
        if (playedCnt<mod_subsongs) {
            //select randomly next unplayed one
            nextsubidx=(arc4random()%mod_subsongs);
            while (mdz_SubsongPlayed[nextsubidx]) {
                nextsubidx++;
                if (nextsubidx>=mod_subsongs) nextsubidx=0;
            }
        } else return -1;
        moveToSubSongIndex=nextsubidx+mod_minsub;
    }
    moveToSubSong=1;
    [self updateCurSubSongPlayed:moveToSubSongIndex-mod_minsub];
    return moveToSubSongIndex;
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
    mNeedSeek=0;bGlobalSeekProgress=0;
    [self iPhoneDrv_PlayStart];
    bGlobalEndReached=0;
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
-(void) PlaySeek:(int64_t)startPos subsong:(int)subsong {
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
                //sprintf(mod_name," %s",mod_filename);
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
//                    if (gme_info->song){
//                        if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);
//                    }
                    if (gme_info->song){
                        if (gme_info->song[0]) mod_title=[NSString stringWithCString:gme_info->song encoding:NSShiftJISStringEncoding];
                        else mod_title=NULL;
                        //if (gme_info->song[0]) sprintf(mod_name," %s",gme_info->song);
                    }
                    gme_free_info(gme_info);
                } else {
                    mod_title=[NSString stringWithFormat:@"%.3d",mod_currentsub-mod_minsub+1];
                    strcpy(gmetype,"N/A");
                    strcpy(mod_message,"N/A\n");
                    iModuleLength=optGENDefaultLength;
                }
                //Loop
                if (mLoopMode==1) iModuleLength=-1;
                if (iModuleLength>settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000) gme_set_fade_msecs( gme_emu, iModuleLength-settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000 ,settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000); //Fade 1s before end
                else gme_set_fade_msecs( gme_emu, 1<<30,settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000);
                mod_message_updated=2;
            }
            gme_seek(gme_emu,startPos);
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_XMP:  //XMP
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
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
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_ADPLUG:  //ADPLUG
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            adPlugPlayer->rewind(mod_currentsub-mod_minsub);
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_HC:  //HE
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_EUP:  //EUP
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_UADE:  //UADE
            mod_wantedcurrentsub=subsong;
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_wantedcurrentsub-mod_minsub];
            [self Play];
            break;
        case MMP_KSS:  //KSS
            
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            if (m3uReader.size()) {
                KSSPLAY_reset(kssplay, m3uReader[mod_currentsub].track, 0);
                iModuleLength=m3uReader[mod_currentsub].length;
                if (iModuleLength<=0) {
                    if (kss->info) iModuleLength=kss->info[mod_currentsub].time_in_ms;
                    else iModuleLength=optGENDefaultLength;
                }
                if (iModuleLength<1000) iModuleLength=1000;
                if (mLoopMode) iModuleLength=-1;
                if (iModuleLength>0) mFadeSamplesStart=(int64_t)(iModuleLength-1000)*PLAYBACK_FREQ/1000; //1s
                else mFadeSamplesStart=1<<30;
            } else {
                KSSPLAY_reset(kssplay, mod_currentsub, 0);
                if (kss->info) {
                    iModuleLength=kss->info[mod_currentsub].time_in_ms;
                } else iModuleLength=optGENDefaultLength;
                if (iModuleLength<1000) iModuleLength=1000;
                
                if (mLoopMode) iModuleLength=-1;
                if (iModuleLength>0) mFadeSamplesStart=(int64_t)(iModuleLength-1000)*PLAYBACK_FREQ/1000; //1s
                else mFadeSamplesStart=1<<30;
            }
            
            if (m3uReader.size()) {
                if (m3uReader[mod_currentsub].name) mod_title=[NSString stringWithUTF8String:m3uReader[mod_currentsub-mod_minsub].name];
            } else {
                if (kss->info) {
                    if (kss->info[mod_currentsub].title[0]) mod_title=[NSString stringWithUTF8String:kss->info[mod_currentsub].title];
                }
            }
            
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
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
            if (mLoopMode==1) iModuleLength=-1;
            
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_WEBSID: //SID
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            websid_loader->initTune(PLAYBACK_FREQ,mod_currentsub);
            Postprocess::init(SOUND_BUFFER_SIZE_SAMPLE, PLAYBACK_FREQ);
            SidSnapshot::init(SOUND_BUFFER_SIZE_SAMPLE, SOUND_BUFFER_SIZE_SAMPLE);
            websid_skip_silence_loop = 10;
            websid_sound_started=0;
            
            iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
            if (iModuleLength<0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
            else if (iModuleLength==0) iModuleLength=1000;
            mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
            if (mLoopMode) iModuleLength=-1;
                        
            if (sidtune_name) {
                if (sidtune_name[mod_currentsub]) mod_title=[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_name[mod_currentsub]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
                else mod_title=NULL;
            } else mod_title=NULL;
            if (sidtune_title) {
                if (sidtune_title[mod_currentsub]) {
                    if (mod_title==NULL) mod_title=[NSString stringWithFormat:@"%@",[[NSString stringWithUTF8String:sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]];
                    else mod_title=[NSString stringWithFormat:@"%@ - %@",mod_title,[[NSString stringWithUTF8String:sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]];
                }
            }
                        
            if (sidtune_artist) {
                if (sidtune_artist[mod_currentsub]) {
                    artist=[NSString stringWithUTF8String:sidtune_artist[mod_currentsub]];
                }
            }
            
            
            /*while (mod_message_updated) {
             //wait
             usleep(1);
             }*/
            
            mod_message_updated=2;
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            
            break;
        case MMP_SIDPLAY: //SID
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            mSidTune->selectSong(mod_currentsub+1);
            mSidEmuEngine->load(mSidTune);
            
            iModuleLength=[self getSongLengthfromMD5:mod_currentsub-mod_minsub+1];
            if (iModuleLength<0) iModuleLength=optGENDefaultLength;//SID_DEFAULT_LENGTH;
            else if (iModuleLength==0) iModuleLength=1000;
            
            mTgtSamples=iModuleLength*PLAYBACK_FREQ/1000;
            if (mLoopMode) iModuleLength=-1;
            
            if (sidtune_name) {
                if (sidtune_name[mod_currentsub]) mod_title=[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_name[mod_currentsub]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
                else mod_title=NULL;
            } else mod_title=NULL;
            if (sidtune_title) {
                if (sidtune_title[mod_currentsub]) {
                    if (mod_title==NULL) mod_title=[NSString stringWithFormat:@"%@",[[NSString stringWithUTF8String:sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]];
                    else mod_title=[NSString stringWithFormat:@"%@ - %@",mod_title,[[NSString stringWithUTF8String:sidtune_title[mod_currentsub]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]];
                }
            }
            
            if (sidtune_artist) {
                if (sidtune_artist[mod_currentsub]) {
                    artist=[NSString stringWithUTF8String:sidtune_artist[mod_currentsub]];
                }
            }
            
            if (mod_name[0]==0) {
                const SidTuneInfo *sidtune_info;
                sidtune_info=mSidTune->getInfo();
                if (sidtune_info->infoString(0)[0]) snprintf(mod_name,sizeof(mod_name),[[[NSString stringWithFormat:@"%@",[NSString stringWithUTF8String:sidtune_info->infoString(0)]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"] UTF8String]);
            }
            
            
            mod_message_updated=2;
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            
            break;
        case MMP_STSOUND:  //YM
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_NSFPLAY:{
            mod_currentsub=subsong;
            if (m3uReader.size()) {
                nsfPlayer->SetSong(m3uReader[mod_currentsub-mod_minsub].track);
            } else nsfPlayer->SetSong(mod_currentsub-mod_minsub);
            nsfPlayer->Reset();
            
            
            // song info
            if (m3uReader.size()) {
                mod_title=[NSString stringWithUTF8String:m3uReader[mod_currentsub-mod_minsub].name];
            } else mod_title=[NSString stringWithUTF8String:nsfData->GetTitleString("%L",mod_currentsub)];
            
            if (m3uReader.size()) iModuleLength=m3uReader[mod_currentsub-mod_minsub].length;
            else iModuleLength=nsfPlayer->GetLength();
            
            if (iModuleLength<=0) iModuleLength=optNSFPLAYDefaultLength;
            if (mLoopMode) iModuleLength=-1;
            
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        }
        case MMP_ATARISOUND:  //SNDH
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
        {
            atariSndh.InitSubSong(mod_currentsub);
            SndhFile::SubSongInfo info;
            atariSndh.GetSubsongInfo(mod_currentsub,info);
            
            iModuleLength=info.playerTickCount*1000/info.playerTickRate;
            if (iModuleLength<=0) {
                unsigned int frames;
                int flags;
                if (timedb68_get(atariSndh_hash,mod_currentsub-1,&frames,&flags)>=0) {
                    iModuleLength=frames*1000/info.playerTickRate;
                }
            }
            if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
            if (mLoopMode) iModuleLength=-1;
            
            if (info.musicSubTitle) sprintf(mod_message,"Title..........: %s\nSubsong title..: %s\nArtist.........: %s\nYear.........: %s\nRipper.........: %s\nConverter......: %s\n",
                                            info.musicTitle,info.musicSubTitle,info.musicAuthor,info.year,info.ripper,info.converter);
            else sprintf(mod_message,"Title.....: %s\nArtist....: %s\nYear......: %s\nRipper....: %s\nConverter.: %s\n",
                         info.musicTitle,info.musicAuthor,info.year,info.ripper,info.converter);
            
            if (info.musicSubTitle) sprintf(mod_name," %s",info.musicSubTitle);
            else if (info.musicTitle) sprintf(mod_name," %s",info.musicTitle);
            
            mod_message_updated=2;
        }
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_PT3:  //PT3
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_GBS:  //GBS
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            if (m3uReader.size()) {
                gbs_init(gbs, m3uReader[mod_currentsub].track);
                iModuleLength=m3uReader[mod_currentsub].length;
                if (iModuleLength<=0) {
                    const struct gbs_status *status = gbs_get_status(gbs);
                    iModuleLength=(int64_t)(status->subsong_len)*1000/1024;
                }
                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                
                gbs_configure(gbs,m3uReader[mod_currentsub].track,iModuleLength/1000,
                              settings[GBSPLAY_SilenceTimeout].detail.mdz_slider.slider_value,1,
                              settings[GBSPLAY_Fadeouttime].detail.mdz_slider.slider_value); //silence timeout, subsong gap, fadeout);
                
                if (mLoopMode) iModuleLength=-1;
                if (m3uReader[mod_currentsub].name) mod_title=[NSString stringWithUTF8String:m3uReader[mod_currentsub-mod_minsub].name];
            } else {
                gbs_init(gbs, mod_currentsub);
                const struct gbs_status *status = gbs_get_status(gbs);
                iModuleLength=(int64_t)(status->subsong_len)*1000/1024;
                if (iModuleLength<=0) iModuleLength=optGENDefaultLength;
                
                gbs_configure(gbs,mod_currentsub,iModuleLength/1000,
                              settings[GBSPLAY_SilenceTimeout].detail.mdz_slider.slider_value,1,
                              settings[GBSPLAY_Fadeouttime].detail.mdz_slider.slider_value); //silence timeout, subsong gap, fadeout);
                
                if (mLoopMode) iModuleLength=-1;
                mod_title=[NSString stringWithUTF8String:status->songtitle];
            }
            {
                const struct gbs_metadata *metadata=gbs_get_metadata(gbs);
                snprintf(mod_message,MAX_STIL_DATA_LENGTH*2,"Title..........: %s\nName...........:%s\nAuthor.........: %s\nCopyright......: %s\n",
                     [mod_title UTF8String],mod_name,metadata->author,metadata->copyright);
            }
            
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_PIXEL:  //PxTone & Orgaya
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            iCurrentTime=startPos;
            break;
        case MMP_SC68: //SC68
            if (startPos) [self Seek:startPos];
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            
            sc68_play( sc68, mod_currentsub, (mLoopMode?SC68_INF_LOOP:0));
            sc68_music_info_t info;
            sc68_process(sc68, buffer_ana[buffer_ana_gen_ofs], 0); //to apply the track change
            sc68_music_info(sc68,&info,SC68_CUR_TRACK,0);
            iModuleLength=info.trk.time_ms;
            
            if (iModuleLength<=0) iModuleLength=optGENDefaultLength;//SC68_DEFAULT_LENGTH;
            
            //Loop
            if (mLoopMode==1) iModuleLength=-1;
            
            
            //NSLog(@"track : %d, time : %d, start : %d",mod_currentsub,info.time_ms,info.start_ms);
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_MDXPDX: //MDX
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_GSF: //GSF
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_ASAP: //ASAP
            //moveToSubSong=1;
            //moveToSubSongIndex=;
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
            }
            iModuleLength=ASAPInfo_GetDuration(ASAP_GetInfo(asap),mod_currentsub);
            if (iModuleLength<1000) iModuleLength=1000;
            ASAP_PlaySong(asap, mod_currentsub, iModuleLength);
            
            if (mLoopMode) iModuleLength=-1;
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_TIMIDITY: //Timidity
            if (startPos) {
                //hack because midi length is unknow at this stage, preventing seek to work
                iCurrentTime=mNeedSeekTime=startPos;
                mNeedSeek=1;
            }//[self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_PMDMINI: //PMD
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_VGMPLAY: //VGM
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_2SF: //2SF
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_V2M: //V2M
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
        case MMP_VGMSTREAM: //VGMSTREAM
            if ((subsong!=-1)&&(subsong>=mod_minsub)&&(subsong<=mod_maxsub)) {
                mod_currentsub=subsong;
                [self vgmStream_ChangeToSub:mod_currentsub];
            }
            
            if (startPos) [self Seek:startPos];
            [self updateCurSubSongPlayed:mod_currentsub-mod_minsub];
            [self Play];
            break;
    }
}

-(void) MMP_HCClose {
    mdz_safe_free(HC_emulatorCore);
    
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
    bGlobalIsPlaying=0;
    [self iPhoneDrv_PlayStop];
    
    if (mPlayType==MMP_TIMIDITY) { //Timidity
        intr = 1;
        while (!tim_finished) {
            [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
        }
    }
    
    /*bGlobalIsPlaying=0;
     [self iPhoneDrv_PlayStop];*/
    
    //wait for sound generation thread to end
    while (bGlobalSoundGenInProgress) {
        [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
        //NSLog(@"Wait for end of thread");
    }
    bGlobalSeekProgress=0;
    bGlobalAudioPause=0;
    
    if (mPlayType==MMP_GME) {
        if (gme_emu) {
            gme_delete( gme_emu );
            gme_emu=NULL;
        }
    }
    if (mPlayType==MMP_XMP) {
        if (xmp_ctx) xmp_end_player(xmp_ctx);
        if (xmp_ctx) xmp_release_module(xmp_ctx);
        if (xmp_ctx) xmp_free_context(xmp_ctx);
        xmp_ctx=NULL;
        mdz_safe_free(xmp_mi);
        
        if (ompt_patterns) {
            for (int i=0;i<numPatterns;i++) {
                if (ompt_patterns[i]) free(ompt_patterns[i]);
            }
            free(ompt_patterns);
        }
        ompt_patterns=NULL;
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
        mdz_safe_free(mp_data);
        mdz_safe_free(ompt_mod_interactive)
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
    if (mPlayType==MMP_NSFPLAY) {
        if (nsfPlayer) delete nsfPlayer;
        nsfPlayer=NULL;
        if (nsfPlayerConfig) delete nsfPlayerConfig;
        nsfPlayerConfig=NULL;
        if (nsfData) delete nsfData;
        nsfData=NULL;
        m3uReader.clear();
    }
    if (mPlayType==MMP_PIXEL) {
        
        if (pixel_organya_mode) unload_org();
        if (pixel_pxtn) pixel_pxtn->clear();
        
        free(pixel_fileBuffer);
        pixel_fileBuffer=NULL;
        if (pixel_pxtn) delete pixel_pxtn;
        pixel_pxtn=NULL;
        if (pixel_desc) delete pixel_desc;
        pixel_desc=NULL;
    }
    if (mPlayType==MMP_EUP) {
        if (eup_player) {
            eup_player->stopPlaying();
            delete eup_player;
            delete eup_dev;
            delete eup_buf;
            eup_player=NULL;
            eup_dev=NULL;
            eup_buf=NULL;
        }
    }
    if (mPlayType==MMP_UADE) {  //UADE
        //		NSLog(@"Wait for end of UADE thread");
        while (uadeThread_running) {
            [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
            
        }
        //		NSLog(@"ok");
        uade_unalloc_song(&UADEstate);
    }
    if (mPlayType==MMP_KSS) { //KSS
        if (kssplay) KSSPLAY_delete(kssplay);
        kssplay=NULL;
        if (kss) KSS_delete(kss);
        kss=NULL;
        m3uReader.clear();
    }
    if (mPlayType==MMP_HVL) { //HVL
        hvl_FreeTune(hvl_song);
        hvl_song=NULL;
    }
    if (mPlayType==MMP_WEBSID) { //SID
        if (websid_scope_buffers) {
            
            for (int i= 0; i<MAX_SIDS*4; i++) {
                free(websid_scope_buffers[i]);
            }
            free(websid_scope_buffers);
            websid_scope_buffers=NULL;
        }
        
        if (websid_fileBuffer) free(websid_fileBuffer);
        websid_fileBuffer=NULL;
        
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
        if (sidtune_artist) {
            for (int i=0;i<mod_subsongs;i++)
                if (sidtune_artist[i]) free(sidtune_artist[i]);
            free(sidtune_artist);
            sidtune_artist=NULL;
        }
    }
    
    if (mPlayType==MMP_SIDPLAY) { //SID
        if (mSidEmuEngine) {
            mSidEmuEngine->stop();
            mSidEmuEngine->load(NULL);
            //sid2_config_t cfg = mSidEmuEngine->config();
            //cfg.sidEmulation = NULL;
            //mSidEmuEngine->config(cfg);
            delete mBuilder;
            mBuilder = NULL;
            
            delete mSidEmuEngine;
            mSidEmuEngine = NULL;
            
            if (mSidTune) {
                delete mSidTune;
                mSidTune = NULL;
            }
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
        if (sidtune_artist) {
            for (int i=0;i<mod_subsongs;i++)
                if (sidtune_artist[i]) free(sidtune_artist[i]);
            free(sidtune_artist);
            sidtune_artist=NULL;
        }
    }
    if (mPlayType==MMP_STSOUND) { //STSOUND
        ymMusicStop(ymMusic);
        ymMusicDestroy(ymMusic);
    }
    if (mPlayType==MMP_ATARISOUND) { //ATARISOUND
        if (mp_data) free(mp_data);
        if (atariWaveData) free(atariWaveData);
        atariWaveData=NULL;
        mp_data=NULL;
        atariSndh.Unload();
    }
    if (mPlayType==MMP_PT3) { //PT3
        if (pt3_music_buf) free(pt3_music_buf);
        pt3_music_buf=NULL;
    }
    if (mPlayType==MMP_GBS) { //GBS
        /* stop sound */
        intr = 1;
        while (!tim_finished) {
            [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
        }
        if (gbs) common_cleanup(gbs);
        gbs=NULL;
    }
    if (mPlayType==MMP_SC68) {//SC68
        sc68_stop(sc68);
        sc68_close(sc68);
    }
    if (mPlayType==MMP_MDXPDX) { //MDX
        if (mdx) {
            mdx_close(mdx,pdx);
            mdx=NULL;pdx=NULL;
        }
    }
    if (mPlayType==MMP_GSF) { //GSF
        intr = 1;
        while (!tim_finished) {
            [NSThread sleepForTimeInterval:DEFAULT_WAIT_TIME_MS];
        }
        GSFClose();
    }
    if (mPlayType==MMP_ASAP) { //ASAP
        free(ASAP_module);
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
        if (vgmFile) close_streamfile(vgmFile);
        vgmFile=NULL;
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
    if ((mPlayType==MMP_KSS)||(mPlayType==MMP_GME)||(mPlayType==MMP_MDXPDX)||(mPlayType==MMP_PIXEL)||(mPlayType==MMP_NSFPLAY)) return [NSString stringWithCString:mod_message encoding:NSShiftJISStringEncoding];
    if (mod_message[0]) modMessage=[NSString stringWithUTF8String:mod_message];
    if (modMessage==nil) {
        modMessage=[NSString stringWithFormat:@"%s",mod_message];
        if (modMessage==nil) return @"";
    }
    return modMessage;
}

-(NSString*) getModFileTitle {
    //TODO: use title tag when available
    if (mod_title&&([mod_title length]>0)) return mod_title;
    return [[NSString stringWithUTF8String:mod_filename] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
}

-(NSString*) getModFileTitleOrNull {
    if ([mod_title length]==0) return NULL;
    return mod_title;
}

-(NSString*) getModName {
    NSString *modName;
    if  ( (mPlayType==MMP_KSS)||(mPlayType==MMP_GME)||(mPlayType==MMP_MDXPDX)||(mPlayType==MMP_NSFPLAY) ) return [[NSString stringWithCString:mod_name encoding:NSShiftJISStringEncoding] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
    modName=[[NSString stringWithUTF8String:mod_name] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
    
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
    if (mPlayType==MMP_EUP) return @"EUPMini";
    if (mPlayType==MMP_HVL) return @"HVL";
    if (mPlayType==MMP_KSS) return @"LIBKSS";
    if (mPlayType==MMP_WEBSID) return @"WebSID";
    if (mPlayType==MMP_SIDPLAY) return ((sid_engine?@"SIDPLAY/ReSIDFP":@"SIDPLAY/ReSID"));
    if (mPlayType==MMP_STSOUND) return @"STSOUND";
    if (mPlayType==MMP_ATARISOUND) return @"ATARISOUND";
    if (mPlayType==MMP_PT3) return @"PT3 Player";
    if (mPlayType==MMP_GBS) return @"GBSPlay";
    if (mPlayType==MMP_NSFPLAY) return @"NSFPLAY";
    if (mPlayType==MMP_PIXEL) return @"PxTone/Organya";
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
            else result=[[NSString stringWithFormat:@"%.3d",subsong] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
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
                    if (vgmStreamTmp->stream_name[0]) result=[[NSString stringWithFormat:@"%.3d-%s",subsong-mod_minsub+1,vgmStreamTmp->stream_name] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
                    else result=[NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
                    close_vgmstream(vgmStreamTmp);
                } else result=[NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
            }
        }
        return result;
    } else if (mPlayType==MMP_OPENMPT) {
        const char *ret=openmpt_module_get_subsong_name(openmpt_module_ext_get_module(ompt_mod), subsong);
        if (ret) {
            result=[[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:ret]]  stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
            free((char*)ret);
        } else result=[NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
        return result;
    } else if (mPlayType==MMP_GME) {
        if (gme_track_info( gme_emu, &gme_info, subsong )==0) {
            int sublen=gme_info->play_length;
            if (sublen<=0) sublen=optGENDefaultLength;
            
            result=nil;
            if (gme_info->song){
                if (gme_info->song[0]) result=[[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithCString:gme_info->song encoding:NSShiftJISStringEncoding]]  stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
            }
            if ((!result)&&(gme_info->game)) {
                if (gme_info->game[0]) result=[[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithCString:gme_info->game encoding:NSShiftJISStringEncoding]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
            }
            if (!result) result=[NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
            gme_free_info(gme_info);
            return result;
        }
    } else if (mPlayType==MMP_WEBSID) {
        NSString *subtitle=NULL;
        if (sidtune_name) {
            if (sidtune_name[subsong]) subtitle=[[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:sidtune_name[subsong]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
        }
        if (sidtune_title) {
            if (sidtune_title[subsong]) {
                if (!subtitle) subtitle=[[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:sidtune_title[subsong]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
                else [subtitle stringByAppendingFormat:@"|%@",[[NSString stringWithUTF8String:sidtune_title[subsong]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]];
            }
        }
        if (subtitle) return subtitle;
        
        if (websid_info[4][0]) return [[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:websid_info[4]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
        return [NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
        
    } else if (mPlayType==MMP_SIDPLAY) {
        NSString *subtitle=NULL;
        if (sidtune_name) {
            if (sidtune_name[subsong]) subtitle=[[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:sidtune_name[subsong]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
        }
        if (sidtune_title) {
            if (sidtune_title[subsong]) {
                if (!subtitle) subtitle=[[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:sidtune_title[subsong]]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
                else [subtitle stringByAppendingFormat:@"|%@",[[NSString stringWithUTF8String:sidtune_title[subsong]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"]];
            }
        }
        if (subtitle) return subtitle;
        
        const SidTuneInfo *sidtune_info;
        sidtune_info=mSidTune->getInfo();
        if (sidtune_info->infoString(0)[0]) return [[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:sidtune_info->infoString(0)]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
        return [NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
        
    } else if (mPlayType==MMP_KSS) {
        if (m3uReader.size()-1>=subsong) {
            return [[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithCString:m3uReader[subsong].name encoding:NSShiftJISStringEncoding] ] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
        }
        if (kss->info) {
            return [[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithCString:kss->info[subsong].title encoding:NSShiftJISStringEncoding] ] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
        }
    } else if (mPlayType==MMP_GBS) {
        if (m3uReader.size()-1>=subsong) {
            return [[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithCString:m3uReader[subsong].name encoding:NSShiftJISStringEncoding] ] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
        }
        return [NSString stringWithFormat:@"%.3d",subsong-mod_minsub+1];
    }else if (mPlayType==MMP_ATARISOUND) {
        SndhFile::SubSongInfo info;
        atariSndh.GetSubsongInfo(subsong,info);
        const char *str=info.musicSubTitle;
        if (!str) str=info.musicTitle;
        return [[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithUTF8String:str]] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
    } else if (mPlayType==MMP_NSFPLAY) {
        if (m3uReader.size()-1>=subsong) {
            return [[NSString stringWithFormat:@"%.3d-%@",subsong-mod_minsub+1,[NSString stringWithCString:m3uReader[subsong].name encoding:NSShiftJISStringEncoding] ] stringByReplacingOccurrencesOfString:@"\"" withString:@"'"];
        }
        return [NSString stringWithFormat:@"%.3d-%s",subsong-mod_minsub+1,nsfData->GetTitleString("%L",subsong)];
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
    if (mPlayType==MMP_EUP) return [NSString stringWithFormat:@"Euphony"];
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
    if (mPlayType==MMP_KSS) return @"KSS";
    if (mPlayType==MMP_HVL) return (hvl_song->ht_ModType?@"HVL":@"AHX");
    if (mPlayType==MMP_WEBSID) return @"SID";
    if (mPlayType==MMP_SIDPLAY) return @"SID";
    if (mPlayType==MMP_STSOUND) return @"YM";
    if (mPlayType==MMP_ATARISOUND) return @"SNDH";
    if (mPlayType==MMP_PT3) return @"PT3";
    if (mPlayType==MMP_GBS) return @"GBS";
    if (mPlayType==MMP_NSFPLAY) {
        if (nsfData->is_nsfe) return @"NSFe";
        return @"NSF";
    }
    if (mPlayType==MMP_PIXEL) {
        if (pixel_organya_mode) return @"Organya";
        else return @"PxTone";
    }
    if (mPlayType==MMP_SC68) {
        sc68_music_info_t info;
        sc68_music_info(sc68,&info,SC68_CUR_TRACK,0);
        
        return [NSString stringWithFormat:@"%s",info.format];
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
    mPanningValue=128-(int)(value*128.0f);
}

///////////////////////////
//GSF
///////////////////////////
-(void) optGSF_UpdateParam {
    soundLowPass = optGSFsoundLowPass;
    soundEcho = optGSFsoundEcho;
}

///////////////////////////
//GBSPlay
///////////////////////////
-(void) optGBSPLAY_UpdateParam {
    if (gbs==NULL) return;
    switch (settings[GBSPLAY_HPFilterType].detail.mdz_switch.switch_value) {
        case 0:gbs_set_filter(gbs,FILTER_OFF);break;
        case 1:gbs_set_filter(gbs,FILTER_DMG);break;
        case 2:gbs_set_filter(gbs,FILTER_CGB);break;
    }
    gbs_set_default_length(gbs,settings[GBSPLAY_DefaultLength].detail.mdz_slider.slider_value);
}
///////////////////////////
//NSFPlay
///////////////////////////
-(void) optNSFPLAY_DefaultLength:(float_t)val {
    optNSFPLAYDefaultLength=(int)(val*1000);
}

-(void) optNSFPLAY_UpdateParam {
    if (nsfPlayerConfig) {
        (*nsfPlayerConfig)["LPF"]=settings[NSFPLAY_LowPass_Filter_Strength].detail.mdz_slider.slider_value;
        (*nsfPlayerConfig)["HPF"]=settings[NSFPLAY_HighPass_Filter_Strength].detail.mdz_slider.slider_value;
            
        (*nsfPlayerConfig)["REGION"]=settings[NSFPLAY_Region].detail.mdz_switch.switch_value;
        (*nsfPlayerConfig)["IRQ_ENABLE"]=settings[NSFPLAY_ForceIRQ].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["QUALITY"]=settings[NSFPLAY_Quality].detail.mdz_slider.slider_value;
        
        (*nsfPlayerConfig)["APU1_OPTION0"]=settings[NSFPLAY_APU_OPTION0].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU1_OPTION1"]=settings[NSFPLAY_APU_OPTION1].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU1_OPTION2"]=settings[NSFPLAY_APU_OPTION2].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU1_OPTION3"]=settings[NSFPLAY_APU_OPTION3].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU1_OPTION4"]=settings[NSFPLAY_APU_OPTION4].detail.mdz_boolswitch.switch_value;
        
        (*nsfPlayerConfig)["APU2_OPTION0"]=settings[NSFPLAY_DMC_OPTION0].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU2_OPTION1"]=settings[NSFPLAY_DMC_OPTION1].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU2_OPTION2"]=settings[NSFPLAY_DMC_OPTION2].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU2_OPTION3"]=settings[NSFPLAY_DMC_OPTION3].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU2_OPTION4"]=settings[NSFPLAY_DMC_OPTION4].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU2_OPTION5"]=settings[NSFPLAY_DMC_OPTION5].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU2_OPTION6"]=settings[NSFPLAY_DMC_OPTION6].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU2_OPTION7"]=settings[NSFPLAY_DMC_OPTION7].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["APU2_OPTION8"]=settings[NSFPLAY_DMC_OPTION8].detail.mdz_boolswitch.switch_value;
        
        (*nsfPlayerConfig)["N163_OPTION0"]=settings[NSFPLAY_N163_OPTION0].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["N163_OPTION1"]=settings[NSFPLAY_N163_OPTION1].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["N163_OPTION2"]=settings[NSFPLAY_N163_OPTION2].detail.mdz_boolswitch.switch_value;
        
        (*nsfPlayerConfig)["FDS_OPTION0"]=atoi(settings[NSFPLAY_FDS_OPTION0].detail.mdz_textbox.text);
        (*nsfPlayerConfig)["FDS_OPTION1"]=settings[NSFPLAY_FDS_OPTION1].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["FDS_OPTION2"]=settings[NSFPLAY_FDS_OPTION2].detail.mdz_boolswitch.switch_value;
        
        (*nsfPlayerConfig)["MMC5_OPTION0"]=settings[NSFPLAY_MMC5_OPTION0].detail.mdz_boolswitch.switch_value;
        (*nsfPlayerConfig)["MMC5_OPTION1"]=settings[NSFPLAY_MMC5_OPTION1].detail.mdz_boolswitch.switch_value;
        
        (*nsfPlayerConfig)["VRC7_PATCH"]=settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_value;
        (*nsfPlayerConfig)["VRC7_OPTION0"]=settings[NSFPLAY_VRC7_OPTION0].detail.mdz_boolswitch.switch_value;
        
        nsfPlayer->Notify(-1);
    }
}


///////////////////////////
// ADPLUG
///////////////////////////
-(void) optADPLUG:(int)opltype stereosurround:(int)stereosurround priorityOverMod:(int)priorityOverMod{
    mADPLUGopltype=opltype;
    mADPLUGstereosurround=stereosurround;
    mADPLUGPriorityOverMod=priorityOverMod;
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
-(void) optGME_Ratio:(float)ratio isEnabled:(bool)enabled {
    optGMERatio = ratio;
    if(gme_emu) {
        if(!enabled) optGMERatio = 1;
        gme_set_tempo(gme_emu, optGMERatio);
    }
}


-(void) optGME_Update {
    if (gme_emu) {
        if (settings[GME_EQ_ONOFF].detail.mdz_boolswitch.switch_value) {
            gme_equalizer( gme_emu, &gme_eq );
            gme_eq.treble = settings[GME_EQ_TREBLE].detail.mdz_slider.slider_value;//-14; // -50.0 = muffled, 0 = flat, +5.0 = extra-crisp
            
            gme_eq.bass   = pow(10,4.2-settings[GME_EQ_BASS].detail.mdz_slider.slider_value);
            //bass
            //          def:80;  1 = full bass, 90 = average, 16000 = almost no bass
            //log10 ->  def:1,9;  0 - 4,2
            
            gme_set_equalizer( gme_emu, &gme_eq );
        } else {
            //restore initial value
            gme_eq.treble=gme_eq_treble_orig;
            gme_eq.bass=gme_eq_bass_orig;
            gme_set_equalizer( gme_emu, &gme_eq );
        }
                
        gme_set_stereo_depth(gme_emu,settings[GME_STEREO_PANNING].detail.mdz_slider.slider_value);
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
/////
///////////////////////
///
-(void) optUpdateSystemColor {
    for (int i=0;i<SOUND_VOICES_MAX_ACTIVE_CHIPS;i++) {
        m_voice_systemColor[i]=settings[OSCILLO_MULTI_COLOR01+i].detail.mdz_color.rgb;
    }
    for (int i=0;i<m_genNumVoicesChannels;i++) {
        m_voice_voiceColor[i]=m_voice_systemColor[[self getSystemForVoice:i]];
    }
}

///////////////////////////
-(void) setLoopInf:(int)val {
    mLoopMode=val;
}
-(void) Seek:(int64_t) seek_time {
    NSLog(@"mdz need seek: %lld - %d - %lld",mNeedSeekTime,[self isSeeking],iModuleLength);
    if ([self isSeeking]) return;
    
    if ((mPlayType==MMP_UADE) ||mNeedSeek) return;
    
    if (mPlayType==MMP_STSOUND) {
        if (ymMusicIsSeekable(ymMusic)==YMFALSE) return;
    }
    
    if (seek_time>iModuleLength-SEEK_START_MARGIN_FROM_END) {
        seek_time=iModuleLength-SEEK_START_MARGIN_FROM_END;
        if (seek_time<0) seek_time=0;
    }
    
    mNeedSeekTime=seek_time;
    iCurrentTime=mNeedSeekTime;
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

-(bool) isArchiveFullyPlayed{
    int ret=0;
    for (int i=0;i<mdz_ArchiveFilesCnt;i++) if (mdz_ArchiveEntryPlayed[i]) ret++;
    if (ret==mdz_ArchiveFilesCnt) return true;
    return false;
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

-(int) getNumChannels {
    if (numChannels!=m_genNumVoicesChannels) {
        numChannels=m_genNumVoicesChannels; //update if it has changed (ex: Timidity++)
        mod_message_updated=2;
    }
    return m_genNumVoicesChannels;
}

-(bool) isVoicesMutingSupported {
    switch (mPlayType) {
        case MMP_HC:
            if ((HC_type==1)||(HC_type==2)) return true;
            if ((HC_type==0x11)||(HC_type==0x12)) return true;
            if (HC_type==0x23) return true;
            if (HC_type==0x41) return true;
            return false;
        case MMP_KSS:
        case MMP_NSFPLAY:
        case MMP_PIXEL:
        case MMP_EUP:
        case MMP_PT3:
        case MMP_GBS:
        case MMP_2SF:
        case MMP_V2M:
        case MMP_GSF:
        case MMP_UADE:
        case MMP_OPENMPT:
        case MMP_XMP:
        case MMP_GME:
        case MMP_ADPLUG:
        case MMP_MDXPDX:
        case MMP_HVL:
        case MMP_STSOUND:
        case MMP_WEBSID:
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
        case MMP_TIMIDITY:
            return [NSString stringWithFormat:@"%d-%s",channel+1,channel_instrum_name(m_voice_channel_mapping[channel])];
        case MMP_KSS:
        case MMP_GBS:
        case MMP_NSFPLAY: {
            return [NSString stringWithFormat:@"%s",modizVoicesName[channel]];
        }
        case MMP_2SF:
            return [NSString stringWithFormat:@"#%d-NDS",channel+1];
        case MMP_V2M:
            return [NSString stringWithFormat:@"#%d-V2",channel+1];
        case MMP_PIXEL: {
            if (pixel_organya_mode) {
                return [NSString stringWithFormat:@"#%d-Organya",channel+1];
            } else {
                const char *name=pixel_pxtn->Unit_Get(channel)->get_name_buf(NULL);
                if (name) return [NSString stringWithFormat:@"#%d-%@",channel+1,[self sjisToNS:name]];
                else return [NSString stringWithFormat:@"#%d",channel+1];
            }
        }
        case MMP_ATARISOUND:
            if (channel<3) return [NSString stringWithFormat:@"#%d-YM2149F",channel+1];
            else return [NSString stringWithFormat:@"#%d-DMA",channel+1];
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
        case MMP_EUP:
            if (channel<6) return [NSString stringWithFormat:@"%d-FM",channel+1];
            else return [NSString stringWithFormat:@"%d-PCM",channel-5];
        case MMP_GSF:
            if (channel<4) return [NSString stringWithFormat:@"#%d-DMG",channel+1];
            else return [NSString stringWithFormat:@"#%d-DirectSnd",channel-4+1];
        case MMP_UADE:
            return [NSString stringWithFormat:@"#%d-PAULA",channel+1];
        case MMP_XMP:
            return [NSString stringWithFormat:@"#%d-XMP",channel+1];
        case MMP_HVL:
            if (hvl_song->ht_ModType) return [NSString stringWithFormat:@"#%d-HVL",channel+1];
            return [NSString stringWithFormat:@"#%d-AHX",channel+1];
        case MMP_MDXPDX:
            if (channel<8) return [NSString stringWithFormat:@"#%d-YM2151",channel+1];
            else return [NSString stringWithFormat:@"PCM"];
        case MMP_ADPLUG:
            return [NSString stringWithFormat:@"#%d-OPL3",channel+1];
        case MMP_STSOUND:
            return [NSString stringWithFormat:@"#%d-YM2149",channel+1];
        case MMP_PT3:
            return [NSString stringWithFormat:@"AY#%d %c",channel/3,(channel%3)+'A'];
        case MMP_ASAP:
            return [NSString stringWithFormat:@"#%d-POKEY#%d",(channel&3)+1,channel/4+1];
        case MMP_GME:
            return [NSString stringWithFormat:@"%s",gme_voice_name(gme_emu,channel)];
        case MMP_WEBSID:
            return [NSString stringWithFormat:@"#%d-SID#%d",(channel%4)+1,channel/4+1];
        case MMP_SIDPLAY:
            return [NSString stringWithFormat:@"#%d-SID#%d",(channel%4)+1,channel/4+1];
        case MMP_VGMPLAY:{
            int idx=0;
            for (int i=0;i<vgmplay_activeChipsNb;i++) {
                if ((channel>=idx)&&(channel<idx+vgmGetVoicesNb(vgmplay_activeChips[i]))) {
                    vgmGetVoiceDetailName(vgmplay_activeChips[i],channel-idx);
                    if (vgm_voice_name[0]) return [NSString stringWithFormat:@"#%d-%s",channel-idx+1,vgm_voice_name];
                    else return [NSString stringWithFormat:@"#%d-%s#%d",channel-idx+1,GetChipName(vgmplay_activeChips[i]),vgmplay_activeChipsID[i]+1];
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
        case MMP_KSS:
        case MMP_GBS:
        case MMP_NSFPLAY:
            return modizChipsetCount;
        case MMP_HC:
            if (HC_type==1) return 1;
            else if (HC_type==2) return 2;
            else if ((HC_type==0x11)||(HC_type==0x12)) return 1;
            else if (HC_type==0x23) return 1;
            else if (HC_type==0x41) return 1;
            return 1;
        case MMP_GSF:
            return 2;
        case MMP_EUP:
            return 2;
        case MMP_PIXEL:
        case MMP_ATARISOUND:
        case MMP_2SF:
        case MMP_ADPLUG:
        case MMP_HVL:
        case MMP_STSOUND:
        case MMP_V2M:
        case MMP_UADE:
        case MMP_OPENMPT:
        case MMP_XMP:
        case MMP_GME:
            return 1;
        case MMP_MDXPDX:
            if (numChannels==8) return 1;
            else return 2;
        case MMP_PT3:
            return pt3_numofchips;
        case MMP_ASAP:
            return numChannels/4;
        case MMP_VGMPLAY:
            return vgmplay_activeChipsNb;
        case MMP_WEBSID:
        case MMP_SIDPLAY:
            return numChannels/4; //number of sidchip active: voices/3, (3ch each)
        default:
            return 0;
    }
}

-(NSString*) getSystemsName:(int)systemIdx {
    switch (mPlayType) {
        case MMP_KSS:
        case MMP_GBS:
        case MMP_NSFPLAY:
            return [NSString stringWithFormat:@"%s",modizChipsetName[systemIdx]];
        case MMP_2SF:
            return @"NDS";
        case MMP_V2M:
            return @"V2";
        case MMP_PIXEL:
            if (pixel_organya_mode) return @"Organya";
            else return @"PxTone";
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
        case MMP_EUP:
            if (systemIdx==0) return @"YM2612";
            else return @"RF5c68";
        case MMP_UADE:
            return @"PAULA";
        case MMP_OPENMPT:
            return @"OMPT";
        case MMP_HVL:
            if (hvl_song->ht_ModType) return @"HVL";
            else return @"AHX";
        case MMP_MDXPDX:
            if (systemIdx==0) return @"YM2151";
            else return @"PCM";
        case MMP_ADPLUG:
            return @"OPL3";
        case MMP_STSOUND:
            return @"YM2149";
        case MMP_XMP:
            return @"XMP";
        case MMP_ASAP:
            if (systemIdx==0) return @"POKEY#1";
            return @"POKEY#2";
        case MMP_PT3:
            return [NSString stringWithFormat:@"AY#%d",systemIdx];
        case MMP_GME:
            if (strcmp(gmetype,"Super Nintendo")==0) {//SPC
                return @"SPC700";
            }
            return [NSString stringWithFormat:@"%s",gmetype];
        case MMP_VGMPLAY:
            return [NSString stringWithFormat:@"%s#%d",vgmplay_activeChipsName[systemIdx],vgmplay_activeChipsID[systemIdx] + 1];
        case MMP_WEBSID:
        case MMP_SIDPLAY:
            return [NSString stringWithFormat:@"Sid#%d",systemIdx + 1]; //number of sidchip active
        default:
            return 0;
    }
}

-(int) getSystemForVoice:(int)voiceIdx {
    switch (mPlayType) {
        case MMP_NSFPLAY:
        case MMP_GBS:
        case MMP_KSS:
            for (int i=0;i<modizChipsetCount;i++) {
                if ((voiceIdx>=modizChipsetStartVoice[i])&&(voiceIdx<modizChipsetStartVoice[i]+modizChipsetVoicesCount[i])) return i;
            }
            return 0;
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
        case MMP_EUP:
            if (voiceIdx<6) return 0;
            return 1;
        case MMP_PIXEL:
        case MMP_2SF:
        case MMP_V2M:
        case MMP_UADE:
        case MMP_OPENMPT:
        case MMP_XMP:
        case MMP_GME:
        case MMP_ADPLUG:
        case MMP_HVL:
        case MMP_STSOUND:
            return 0;
        case MMP_MDXPDX:
            if (voiceIdx<8) return 0;
            return 1;
        case MMP_ASAP:
            return voiceIdx/4;
        case MMP_PT3:
            return voiceIdx/3;
        case MMP_VGMPLAY: {
            int idx=0;
            for (int i=0;i<vgmplay_activeChipsNb;i++) {
                if (voiceIdx<idx+vgmGetVoicesNb(vgmplay_activeChips[i])) return i;
                idx+=vgmGetVoicesNb(vgmplay_activeChips[i]);
            }
            return 0;
        }
        case MMP_WEBSID:
        case MMP_SIDPLAY:
            return voiceIdx/4;
        default:
            return 0;
    }
}

-(int) getSystemm_voicesStatus:(int)systemIdx {
    int tmp;
    switch (mPlayType) {
        case MMP_KSS:
        case MMP_GBS:
        case MMP_NSFPLAY:
            tmp=0;
            for (int i=modizChipsetStartVoice[systemIdx];i<modizChipsetStartVoice[systemIdx]+modizChipsetVoicesCount[systemIdx];i++) tmp+=(m_voicesStatus[i]?1:0);
            if (tmp==modizChipsetVoicesCount[systemIdx]) return 2; //all active
            else if (tmp>0) return 1; //partially active
            return 0; //all off
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
        case MMP_EUP:
            if (systemIdx==0) {
                tmp=0;
                for (int i=0;i<6;i++) {
                    tmp+=(m_voicesStatus[i]?1:0);
                }
                if (tmp==6) return 2; //all active
                else if (tmp>0) return 1; //partially active
                return 0; //all off
            } else {
                tmp=0;
                for (int i=6;i<14;i++) {
                    tmp+=(m_voicesStatus[i]?1:0);
                }
                if (tmp==8) return 2; //all active
                else if (tmp>0) return 1; //partially active
                return 0; //all off
            }
        case MMP_PIXEL:
        case MMP_2SF:
        case MMP_V2M:
        case MMP_UADE:
        case MMP_OPENMPT:
        case MMP_XMP:
        case MMP_GME:
        case MMP_ADPLUG:
        case MMP_HVL:
        case MMP_STSOUND:
            tmp=0;
            for (int i=0;i<numChannels;i++) {
                tmp+=(m_voicesStatus[i]?1:0);
            }
            if (tmp==numChannels) return 2; //all active
            else if (tmp>0) return 1; //partially active
            return 0; //all off
        case MMP_MDXPDX:
            if (systemIdx==0) { //MDX
                tmp=0;
                for (int i=0;i<8;i++) {
                    tmp+=(m_voicesStatus[i]?1:0);
                }
                if (tmp==8) return 2; //all active
                else if (tmp>0) return 1; //partially active
                return 0; //all off
            } else { //PDX
                if (m_voicesStatus[8]) return 2;
                else return 0;
            }
        case MMP_ASAP:
            tmp=0;
            for (int i=0;i<4;i++) {
                tmp+=(m_voicesStatus[i+systemIdx*4]?1:0);
            }
            if (tmp==4) return 2; //all active
            else if (tmp>0) return 1; //partially active
            return 0; //all off
        case MMP_PT3:
            tmp=0;
            for (int i=systemIdx*3;i<systemIdx*3+3;i++) {
                if (pt3_mute[i]) tmp++;
            }
            if (tmp==3) return 0; //all off
            if (tmp==0) return 2; //all on
            return 1; //partially off
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
        case MMP_WEBSID:
        case MMP_SIDPLAY:
            tmp=0;
            for (int i=systemIdx*4;i<systemIdx*4+4;i++) {
                tmp+=(m_voicesStatus[i]?1:0);
            }
            if (tmp==4) return 2; //all active
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
        case MMP_EUP:
            if (systemIdx==0) for (int i=0;i<6;i++) [self setm_voicesStatus:active index:i];
            else for (int i=6;i<14;i++) [self setm_voicesStatus:active index:i];
            break;
        case MMP_KSS:
        case MMP_GBS:
        case MMP_NSFPLAY:
            for (int i=modizChipsetStartVoice[systemIdx];i<modizChipsetStartVoice[systemIdx]+modizChipsetVoicesCount[systemIdx];i++) [self setm_voicesStatus:active index:i];
            break;
        case MMP_PIXEL:
        case MMP_2SF:
        case MMP_V2M:
        case MMP_UADE:
        case MMP_OPENMPT:
        case MMP_XMP:
        case MMP_GME:
        case MMP_ADPLUG:
        case MMP_HVL:
        case MMP_STSOUND:
            for (int i=0;i<numChannels;i++) [self setm_voicesStatus:active index:i];
            break;
        case MMP_MDXPDX:
            if (systemIdx==0) {//MDX
                for (int i=0;i<8;i++) [self setm_voicesStatus:active index:i];
            } else {
                [self setm_voicesStatus:active index:8];
            }
            break;
        case MMP_ASAP:
            for (int i=0;i<4;i++) {
                [self setm_voicesStatus:active index:systemIdx*4+i];
            }
            break;
        case MMP_PT3:
            for (int i=0;i<3;i++) {
                [self setm_voicesStatus:active index:systemIdx*3+i];
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
        case MMP_WEBSID:
        case MMP_SIDPLAY:
            for (int i=systemIdx*4;i<systemIdx*4+4;i++) {
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
        case MMP_KSS: {
            int chipIdx=[self getSystemForVoice:channel];
            int voiceIdx=channel-modizChipsetStartVoice[chipIdx];
            switch (modizChipsetType[chipIdx]) {
                case KSS_Y8950:
                    if (active) generic_mute_mask&=~((int64_t)1<<voiceIdx);
                    else generic_mute_mask|=((int64_t)1<<voiceIdx);
                    break;
                case KSS_YM2413:
                    if (active) generic_mute_mask&=~((int64_t)1<<(voiceIdx+15));
                    else generic_mute_mask|=((int64_t)1<<(voiceIdx+15));
                    break;
                case KSS_SN76489:
                    if (active) generic_mute_mask&=~((int64_t)1<<(voiceIdx+15+14));
                    else generic_mute_mask|=((int64_t)1<<(voiceIdx+15+14));
                    break;
                case KSS_PSG:
                    if (active) generic_mute_mask&=~((int64_t)1<<(voiceIdx+15+14+4));
                    else generic_mute_mask|=((int64_t)1<<(voiceIdx+15+14+4));
                    break;
                case KSS_SCC:
                    if (active) generic_mute_mask&=~((int64_t)1<<(voiceIdx+15+14+4+3));
                    else generic_mute_mask|=((int64_t)1<<(voiceIdx+15+14+4+3));
                    break;
            }
        }
            break;
        case MMP_GBS:
            gbs_toggle_setmute(gbs,channel,!active); //YOYOFR
            break;
        case MMP_NSFPLAY: {
            int chipIdx=[self getSystemForVoice:channel];
            int voiceIdx=channel-modizChipsetStartVoice[chipIdx];
            int current_mask=(*nsfPlayerConfig)["MASK"];
            //NSLog(@"chip %d voice %d mask %08X",chipIdx,voiceIdx,current_mask);
            switch (modizChipsetType[chipIdx]) {
                case NES_APU:
                    if (active) current_mask&=~(1<<voiceIdx);
                    else current_mask|=(1<<voiceIdx);
                    break;
                case NES_FDS:
                    if (active) current_mask&=~(1<<5);
                    else current_mask|=(1<<5);
                    break;
                case NES_MMC5:
                    if (active) current_mask&=~(1<<(voiceIdx+6));
                    else current_mask|=(1<<(voiceIdx+6));
                    break;
                case NES_FME7:
                    if (active) current_mask&=~(1<<(voiceIdx+9));
                    else current_mask|=(1<<(voiceIdx+9));
                    break;
                case NES_VRC6:
                    if (active) current_mask&=~(1<<(voiceIdx+12));
                    else current_mask|=(1<<(voiceIdx+12));
                    break;
                case NES_VRC7:
                    if (active) current_mask&=~(1<<(voiceIdx+15));
                    else current_mask|=(1<<(voiceIdx+15));
                    break;
                case NES_N106:
                    if (active) current_mask&=~(1<<(voiceIdx+21));
                    else current_mask|=(1<<(voiceIdx+21));
                    break;
            }
            (*nsfPlayerConfig)["MASK"]=current_mask;
            nsfPlayer->Notify(-1);
        }
            break;
        case MMP_MDXPDX:
        case MMP_ADPLUG:
        case MMP_HVL:
        case MMP_STSOUND:
            if (active) generic_mute_mask&=~(1<<channel);
            else generic_mute_mask|=(1<<channel);
            break;
        case MMP_2SF:
            xSFPlayer->MuteChannels(channel,active);
            break;
        case MMP_V2M:
            if (active) generic_mute_mask|=1<<channel;
            else generic_mute_mask&=~(1<<channel);
            break;
        case MMP_PIXEL:
            if (pixel_organya_mode) {
                if (!active) organya_mute_mask|=1<<channel;
                else organya_mute_mask&=~1<<channel;
            } else {
                if (!active) pixel_pxtn->mute_mask|=1<<channel;
                else pixel_pxtn->mute_mask&=~1<<channel;
            }
            break;
        case MMP_UADE:
            if (active) HC_voicesMuteMask1|=1<<channel;
            else HC_voicesMuteMask1&=~(1<<channel);
            break;
        case MMP_HC:
            if ((HC_type==1)||(HC_type==2)) {//PSF1 and PSF2}
                if (active) {
                    if (channel<24) HC_voicesMuteMask1|=1<<channel;
                    else HC_voicesMuteMask2|=1<<(channel-24);
                } else {
                    if (channel<24) HC_voicesMuteMask1&=~(1<<channel);
                    else HC_voicesMuteMask2&=~(1<<(channel-24));
                }
            } else if ((HC_type==0x11)||(HC_type==0x12)) { //SSF/SCSP or DSF/AICA
                if (active) {
                    if (channel<32) HC_voicesMuteMask1|=1<<channel;
                    else HC_voicesMuteMask2|=1<<(channel-32);
                } else {
                    if (channel<32) HC_voicesMuteMask1&=~(1<<channel);
                    else HC_voicesMuteMask2&=~(1<<(channel-32));
                }
            } else if (HC_type==0x23) { //SNSF
                if (active) HC_voicesMuteMask1|=1<<channel;
                else HC_voicesMuteMask1&=~(1<<channel);
            } else if (HC_type==0x41) { //QSF
                if (active) HC_voicesMuteMask1|=1<<channel;
                else HC_voicesMuteMask1&=~(1<<channel);
            }
            break;
        case MMP_GSF:
            if (channel<4) GSFSoundChannelsEnable(1<<channel,active);
            else GSFSoundChannelsEnable(1<<(channel-4+8),active);
            break;
        case MMP_EUP: {
            int new_eup_mutemask=0;
            for (int i=0;i<14;i++) if (!(m_voicesStatus[i])) new_eup_mutemask|=1<<i;
            eup_mutemask=new_eup_mutemask;
            break;
        }
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
            else gme_ignore_silence(gme_emu,!settings[GME_IGNORESILENCE].detail.mdz_boolswitch.switch_value);
            gme_mute_voice(gme_emu,channel,(active?0:1));
            break;
        }
        case MMP_WEBSID:
        case MMP_SIDPLAY:
            //check consistency with channel 4
        {
            int system_idx=channel/4;
            int sid_channel=channel%4;
            int tmp=0;
            for (int i=0;i<3;i++) if (m_voicesStatus[system_idx*4+i]) tmp++;
            if (tmp) { //at least 1 voice active, 4th channel is on
                m_voicesStatus[system_idx*4+3]=1;
            } else { //all main 3 voices are off, 4th channel is off
                m_voicesStatus[system_idx*4+3]=0;
            }
            
            if ((channel%4)==3) { //it is a 4th channel, override value
                active=m_voicesStatus[channel];
            }
            if (mPlayType==MMP_SIDPLAY) mSidEmuEngine->mute(system_idx,sid_channel,(active?0:1));   //(unsigned int sidNum,
            else if (mPlayType==MMP_WEBSID) {
                SID::setMute(system_idx, sid_channel, (active?0:1));
                SID::setMute(system_idx, 3, (m_voicesStatus[system_idx*4+3]?0:1));
            }
            //SID::setMute(system_idx, 3, (m_voicesStatus[system_idx*4+3]?0:1));
        }
            if ((channel%4)<3) mSidEmuEngine->mute(channel/4,channel%4,(active?0:1));   //(unsigned int sidNum, unsigned int voice, bool enable);
            break;
        case MMP_PT3: {
            pt3_mute[channel]=(active?0:1);
            break;
        }
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
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].SN76496.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].SN76496.ChnMute1|=1<<(channel-idx);
                            break;
                        case 1: //VRC7 or YM2413
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2413.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YM2413.ChnMute1|=1<<(channel-idx);
                            break;
                        case 2: //YM2612:  6voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2612.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YM2612.ChnMute1|=1<<(channel-idx);
                            if ((channel-idx)==5) {
                                //update dac channel as well, seems when dac channel 6 is linked to fm 4
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2612.ChnMute1&=~(1<<6);
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2612.ChnMute1|=1<<6;
                            }
                            break;
                        case 3: //YM2151: 8voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2151.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YM2151.ChnMute1|=1<<(channel-idx);
                            break;
                        case 4: //Sega PCM: 16voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].SegaPCM.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].SegaPCM.ChnMute1|=1<<(channel-idx);
                            break;
                        case 5: //RF5C68
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].RF5C68.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].RF5C68.ChnMute1|=1<<(channel-idx);
                            break;
                        case 6: {//YM2203
                            // chnmute1 3bits -> fff f:fm 3ch
                            // chnmute3, 3bits -> yyy y:ay 3ch
                            int voice=channel-idx;
                            if (voice<3) {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2203.ChnMute1&=~(1<<voice);
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2203.ChnMute1|=(1<<voice);
                            } else {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2203.ChnMute3&=~(1<<(voice-3));
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2203.ChnMute3|=(1<<(voice-3));
                            }
                            break;
                        }
                        case 7:{ //YM2608: 16voices:
                            // chnmute1 & chnmute2, 13bits -> daaaaaaffffff   d:delta 1ch, a:adpcm 6ch, f:fm 6ch
                            // chnmute3, 3bits -> yyy y:ay 3ch
                            int voice=channel-idx;
                            if (voice<6) {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute1&=~(1<<voice);
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute1|=(1<<voice);
                            } else if (voice<13) {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute2&=~(1<<(voice-6));
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute2|=(1<<(voice-6));
                            } else {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute3&=~(1<<(voice-13));
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2608.ChnMute3|=(1<<(voice-13));
                            }
                            break;
                        }
                        case 8:{ //YM2610: 14voices (4 fm), YM2610b: 16voices (6 fm)
                            // chnmute1 & chnmute2, 13bits -> daaaaaaffffff   d:delta 1ch, a:adpcm 6ch, f:fm 6ch
                            // chnmute3, 3bits -> yyy y:ay 3ch
                            int voice=channel-idx;
                            if (vgm2610b==0) {
                                if (voice<4) {
                                    switch (voice) {
                                        case 0:voice=1;break;
                                        case 1:voice=2;break;
                                        case 2:voice=4;break;
                                        case 3:voice=5;break;
                                    }
                                } else voice+=2; //adpcm start at 6
                            }
                            
                            if (voice<6) { //FM
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute1&=~(1<<voice);
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute1|=(1<<voice);
                            } else if (voice<6+7) {  //ADPCM & Delta
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute2&=~(1<<(voice-6));
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute2|=(1<<(voice-6));
                            } else { //AY chip
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute3&=~(1<<(voice-13));
                                else ChipOpts[vgmplay_activeChipsID[i]].YM2610.ChnMute3|=(1<<(voice-13));
                            }
                            break;
                        }
                        case 9: //YM3812: 9voices+5perc
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YM3812.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YM3812.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x0A: //YM3526
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YM3526.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YM3526.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x0B: //Y8950
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].Y8950.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].Y8950.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x0C: //YMF262
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YMF262.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YMF262.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x0D: //YMF278B:47voices:23(ymf262)+24pcm(wave table) fm: 18 channels + 5 drums pcm: 24 channels
                            //  mute1: dddddffffffffffffffffff
                            //  mute2: pppppppppppppppppppppppp
                            if (channel-idx<23) {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YMF278B.ChnMute1&=~(1<<(channel-idx));
                                else ChipOpts[vgmplay_activeChipsID[i]].YMF278B.ChnMute1|=(1<<(channel-idx));
                            } else {
                                if (active) ChipOpts[vgmplay_activeChipsID[i]].YMF278B.ChnMute2&=~(1<<(channel-idx-23));
                                else ChipOpts[vgmplay_activeChipsID[i]].YMF278B.ChnMute2|=(1<<(channel-idx-23));
                            }
                            break;
                        case 0x0E: //YMF271
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YMF271.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YMF271.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x0F: //YMZ280B
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].YMZ280B.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].YMZ280B.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x10: //RF5C164
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].RF5C164.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].RF5C164.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x11: //PWM
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].PWM.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].PWM.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x12: //AY8910
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].AY8910.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].AY8910.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x13: //GameBoy
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].GameBoy.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].GameBoy.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x14: //NES APU: 5voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].NES.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].NES.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x15: //MultiPCM
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].MultiPCM.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].MultiPCM.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x16: //UPD7759
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].UPD7759.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].UPD7759.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x17: //OKIM6258: 1voice
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].OKIM6258.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].OKIM6258.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x18: //OKIM6295: 4voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].OKIM6295.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].OKIM6295.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x19: //K051649: 5voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].K051649.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].K051649.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1A: //K054539
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].K054539.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].K054539.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1B: //HuC6280
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].HuC6280.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].HuC6280.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1C: //C140: 24voices
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].C140.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].C140.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1D: //k053260
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].K053260.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].K053260.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1E: //pokey
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].Pokey.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].Pokey.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x1F: //qsound
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].QSound.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].QSound.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x20: //SCSP
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].SCSP.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].SCSP.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x21: //WSWAN
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].WSwan.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].WSwan.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x22: //VSU
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].VSU.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].VSU.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x23: //SAA1099
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].SAA1099.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].SAA1099.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x24: //ES5503
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].ES5503.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].ES5503.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x25: //ES5506
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].ES5506.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].ES5506.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x26: //X1_010
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].X1_010.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].X1_010.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x27: //C352
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].C352.ChnMute1&=~(1<<(channel-idx));
                            else ChipOpts[vgmplay_activeChipsID[i]].C352.ChnMute1|=1<<(channel-idx);
                            break;
                        case 0x28: //X1_010
                            if (active) ChipOpts[vgmplay_activeChipsID[i]].GA20.ChnMute1&=~(1<<(channel-idx));
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
