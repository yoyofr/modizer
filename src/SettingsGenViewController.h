//
//  SettingsGenViewController.h
//  modizer
//
//  Created by Yohann Magnien on 10/08/13.
//
//

#import <UIKit/UIKit.h>
#import "ModizerConstants.h"
#import "DetailViewControllerIphone.h"
#import "TPKeyboardAvoidingTableView.h"

#import "CMPopTipView.h"

#import "MSColorPicker/MSColorSelectionViewController.h"


#import "CFtpServer.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"

enum MDZ_SETTINGS_TYPE {
    MDZ_FAMILY=1,
    MDZ_SWITCH,
    MDZ_BOOLSWITCH,
    MDZ_SLIDER_CONTINUOUS,
    MDZ_SLIDER_DISCRETE,
    MDZ_SLIDER_DISCRETE_TIME,
    MDZ_SLIDER_DISCRETE_TIME_LONG,
    MDZ_TEXTBOX,
    MDZ_MSGBOX,
    MDZ_COLORPICKER,
    MDZ_BUTTON
};

enum MDZ_SETTINGS_SCOPE {
    SETTINGS_ALL=0,
    SETTINGS_GLOBAL,
    SETTINGS_VISU,
    SETTINGS_ADPLUG,
    SETTINGS_GBSPLAY,
    SETTINGS_GME,
    SETTINGS_GSF,
    SETTINGS_HC,
    SETTINGS_OMPT,
    SETTINGS_SID,
    SETTINGS_UADE,
    SETTINGS_TIMIDITY,
    SETTINGS_VGMPLAY,
    SETTINGS_VGMSTREAM,
    SETTINGS_XMP,
    SETTINGS_ONLINE,
    SETTINGS_NSFPLAY,
    SETTINGS_OSCILLO,
    SETTINGS_PIANOMIDI
};

enum MDZ_SETTINGS {
    MDZ_SETTINGS_FAMILY_ROOT=1,
    MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER,
    
    //GLOB_PlaybackFrequency,
    GLOB_SearchRegExp,
    GLOB_ResumeOnStart,
    GLOB_NoScreenAutoLock,
    GLOB_PBRATIO_ONOFF,
    GLOB_PBRATIO,
    GLOB_ForceMono,
    GLOB_Panning,
    GLOB_PanningValue,
    GLOB_DefaultLength,
    GLOB_AudioLatency,
    GLOB_ArcMultiDefaultAction,
    GLOB_ArcMultiPlayMode,
    GLOB_PlayEnqueueAction,
    GLOB_EnqueueMode,
    GLOB_AfterDownloadAction,    
    GLOB_BackgroundMode,
    GLOB_TruncateNameMode,
    GLOB_TitleFilename,
    GLOB_CoverFlow,
    GLOB_RecreateSamplesFolder,
    GLOB_StatsUpload,
    
    MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY,
    GLOB_Default2SFPlayer,
    GLOB_DefaultGBSPlayer,
    GLOB_DefaultKSSPlayer,
    GLOB_DefaultMIDIPlayer,
    GLOB_DefaultMODPlayer,
    GLOB_DefaultNSFPlayer,
    GLOB_DefaultPT3Player,
    GLOB_DefaultSAPPlayer,
    GLOB_DefaultSIDPlayer,
    GLOB_DefaultVGMPlayer,
    GLOB_DefaultYMPlayer,
    ADPLUG_PriorityOMPT,
    
    MDZ_SETTINGS_FAMILY_GLOBAL_FTP,
    FTP_STATUS,
    FTP_ONOFF,
    FTP_ANONYMOUS,
    FTP_USER,
    FTP_PASSWORD,
    FTP_PORT,
    
    MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE,
    ONLINE_MODLAND_CURRENT_URL,
    ONLINE_MODLAND_URL,
    ONLINE_MODLAND_URL_CUSTOM,
    ONLINE_HVSC_CURRENT_URL,
    ONLINE_HVSC_URL,
    ONLINE_HVSC_URL_CUSTOM,
    ONLINE_ASMA_CURRENT_URL,
    ONLINE_ASMA_URL,
    ONLINE_ASMA_URL_CUSTOM,
    
    MDZ_SETTINGS_FAMILY_PLUGINS,
    
        MDZ_SETTINGS_FAMILY_ADPLUG,
        ADPLUG_OplType,
        ADPLUG_StereoSurround,
    
        MDZ_SETTINGS_FAMILY_GBSPLAY,
        GBSPLAY_DefaultLength,
        GBSPLAY_Fadeouttime,
        GBSPLAY_HPFilterType,
        GBSPLAY_SilenceTimeout,
    
        MDZ_SETTINGS_FAMILY_GME,
        GME_FADEOUT,
        GME_STEREO_PANNING,
        GME_IGNORESILENCE,
        GME_EQ_ONOFF,
        GME_EQ_BASS,
        GME_EQ_TREBLE,
    
        MDZ_SETTINGS_FAMILY_GSF,
        GSF_SOUNDQUALITY,
        GSF_INTERPOLATION,
        GSF_LOWPASSFILTER,
        GSF_ECHO,
    
        MDZ_SETTINGS_FAMILY_HC,
        HC_ResampleQuality,
    
        MDZ_SETTINGS_FAMILY_NSFPLAY,
        NSFPLAY_DefaultLength,
        NSFPLAY_LowPass_Filter_Strength,  //LPF: 0-400 lowpass filter strength (0=off, 112=default, 400=full)
        NSFPLAY_HighPass_Filter_Strength, //HPF: 0-256 highpass filter strength (256=off, 164=default, 0=full)
        NSFPLAY_Region,  //REGION: 0=auto, 1/2=prefer NTSC/PAL, 3/4/5=force NTSC/PAL/DENDY
        NSFPLAY_VRC7_Patch,  /* 0 - VRC7 set by Nuke.KYT 3/15/2019 (dumped from VRC7 via special debug mode)
                              1 - VRC7 set by rainwarrior 8/01/2012 (used by Famitracker 0.4.0)
                              2 - VRC7 set by quietust 1/18/2004 (used by Famitracker 0.3.6)
                              3 - VRC7 set by Mitsutaka Okazaki 6/24/2001 (used by Famitracker 0.3.5 and prior)
                              4 - VRC7 set by Mitsutaka Okazaki 4/10/2004
                              5 - VRC7 set by kevtris 11/15/1999 (second set in vrcvii.txt)
                              6 - VRC7 set by kevtris 11/14/1999 (first set in vrcvii.txt)
                              7 - YM2413 set by Mitsutaka Okazaki 4/10/2004
                              8 - YMF281B set by Chabin 4/10/2004
                              9 - YMF281B set by plgDavid 2/28/2021*/
        NSFPLAY_ForceIRQ, //IRQ_ENABLE: forces IRQ capability to be enabled for all NSFs, not just NSF2.
        NSFPLAY_Quality,  //0-40 oversampling, default 10
    
        NSFPLAY_APU_OPTION0,    //unmute on reset
        NSFPLAY_APU_OPTION1,    //phase reset
        NSFPLAY_APU_OPTION2,    //nonlinear mixing
        NSFPLAY_APU_OPTION3,    //Swap duty 1/2
        NSFPLAY_APU_OPTION4,    //Initialize sweep unmute
        NSFPLAY_DMC_OPTION0,    //enable $4011 writes
        NSFPLAY_DMC_OPTION1,    //enable periodic noise
        NSFPLAY_DMC_OPTION2,    //unmute on reset
        NSFPLAY_DMC_OPTION3,    //anti clicking (suppresses $4011 pop without disabling its nonlinear mix)
        NSFPLAY_DMC_OPTION4,    //nonlinear mixer
        NSFPLAY_DMC_OPTION5,    //randomize noise on reset
        NSFPLAY_DMC_OPTION6,    //mute triangle on pitch 0 (prevents high frequency aliasing)
        NSFPLAY_DMC_OPTION7,    //randomize triangle on reset
        NSFPLAY_DMC_OPTION8,    //reverse bits of DPCM sample bytes

    
        NSFPLAY_FDS_OPTION0,    //(Hz) lowpass filter cutoff frequency (0=off, 2000=default)
        NSFPLAY_FDS_OPTION1,    //reset "phase" on $4085 write (works around timing issue in Bio Miracle Bokutte Upa)
        NSFPLAY_FDS_OPTION2,    //write protect $8000-DFFF (for some multi-expansion NSFs)
    
        NSFPLAY_MMC5_OPTION0,   //nonlinear mixing
        NSFPLAY_MMC5_OPTION1,   //phase reset
    
        NSFPLAY_N163_OPTION0,   //serial multiplexing (more accurate sound for 6+ channels)
        NSFPLAY_N163_OPTION1,   //Read-Only phase (for old NSFs that overwrite phase every frame)
        NSFPLAY_N163_OPTION2,   //Limit wavelength (for old NSFs that do not set the high bits)
    
        NSFPLAY_VRC7_OPTION0,   //Replace VRC7 with YM2413 (OPLL)
    
        MDZ_SETTINGS_FAMILY_OMPT,
        OMPT_MasterVolume,
        OMPT_Sampling,
        OMPT_StereoSeparation,
            
            
        MDZ_SETTINGS_FAMILY_SID,
        SID_Engine,
        SID_Interpolation,
        SID_Filter,
        SID_ForceLoop,
        SID_CLOCK,
        SID_MODEL,
        SID_SecondSIDOn,
        SID_SecondSIDAddress,
        SID_ThirdSIDOn,
        SID_ThirdSIDAddress,
    
        MDZ_SETTINGS_FAMILY_TIMIDITY,
        TIM_Maxloop,
        TIM_Polyphony,
        TIM_Amplification,
        TIM_Chorus,
        TIM_Reverb,
        TIM_Resample,
        TIM_LPFilter,
    
        MDZ_SETTINGS_FAMILY_UADE,
        UADE_Head,
        UADE_Led,
        UADE_Norm,
        UADE_PostFX,
        UADE_Pan,
            UADE_PanValue,
        UADE_Gain,
            UADE_GainValue,
        UADE_NTSC,
    
        MDZ_SETTINGS_FAMILY_VGMPLAY,
        VGMPLAY_Fadeouttime,
        VGMPLAY_Maxloop,
        VGMPLAY_PreferJTAG,
        VGMPLAY_YM2612Emulator,
        VGMPLAY_NUKEDOPN2_Option,
        VGMPLAY_YMF262Emulator,
        VGMPLAY_YM3812Emulator,
        VGMPLAY_QSoundEmulator,
        VGMPLAY_RF5C68Emulator,
    
        MDZ_SETTINGS_FAMILY_VGMSTREAM,
        VGMSTREAM_ResampleQuality,
        VGMSTREAM_Forceloop,
        VGMSTREAM_Maxloop,
        VGMSTREAM_Fadeouttime,
    
        MDZ_SETTINGS_FAMILY_XMP,
        XMP_Interpolation,
        XMP_MasterVolume,
        XMP_Amplification,
        XMP_StereoSeparation,
        //XMP_DSPLowPass,
        XMP_FLAGS_A500F,
    
    MDZ_SETTINGS_FAMILY_GLOBAL_VISU,
        MDZ_SETTINGS_FAMILY_OSCILLO,
        OSCILLO_ShowLabel,
        OSCILLO_LabelFontSize,
        OSCILLO_ShowGrid,
        OSCILLO_GRID_COLOR,
        OSCILLO_LINE_Width,
        OSCILLO_MONO_COLOR,
        OSCILLO_MULTI_COLORSET,
        OSCILLO_MULTI_RESETALL,
        OSCILLO_MULTI_COLOR01,
        OSCILLO_MULTI_COLOR02,
        OSCILLO_MULTI_COLOR03,
        OSCILLO_MULTI_COLOR04,
        OSCILLO_MULTI_COLOR05,
        OSCILLO_MULTI_COLOR06,
        OSCILLO_MULTI_COLOR07,
        OSCILLO_MULTI_COLOR08,
    GLOB_FXOscillo,
    GLOB_FXAlpha,
    GLOB_FXAlphaFS,
    GLOB_FXLOD,
    GLOB_FXFPS,
    GLOB_FXMSAA,
    GLOB_FXMODPattern,
    GLOB_FXMODPattern_CurrentLineMode,
    GLOB_FXMODPattern_Font,
    GLOB_FXMODPattern_FontSize,
        MDZ_SETTINGS_FAMILY_PIANOMIDI_COL,
        PIANOMIDI_MULTI_COLORSET,
        PIANOMIDI_MULTI_RESETALL,
        PIANOMIDI_MULTI_COLOR01,
        PIANOMIDI_MULTI_COLOR02,
        PIANOMIDI_MULTI_COLOR03,
        PIANOMIDI_MULTI_COLOR04,
        PIANOMIDI_MULTI_COLOR05,
        PIANOMIDI_MULTI_COLOR06,
        PIANOMIDI_MULTI_COLOR07,
        PIANOMIDI_MULTI_COLOR08,
        PIANOMIDI_MULTI_COLOR09,
        PIANOMIDI_MULTI_COLOR10,
        PIANOMIDI_MULTI_COLOR11,
        PIANOMIDI_MULTI_COLOR12,
        PIANOMIDI_MULTI_COLOR13,
        PIANOMIDI_MULTI_COLOR14,
        PIANOMIDI_MULTI_COLOR15,
        PIANOMIDI_MULTI_COLOR16,
        PIANOMIDI_MULTI_COLOR17,
        PIANOMIDI_MULTI_COLOR18,
        PIANOMIDI_MULTI_COLOR19,
        PIANOMIDI_MULTI_COLOR20,
        PIANOMIDI_MULTI_COLOR21,
        PIANOMIDI_MULTI_COLOR22,
        PIANOMIDI_MULTI_COLOR23,
        PIANOMIDI_MULTI_COLOR24,
        PIANOMIDI_MULTI_COLOR25,
        PIANOMIDI_MULTI_COLOR26,
        PIANOMIDI_MULTI_COLOR27,
        PIANOMIDI_MULTI_COLOR28,
        PIANOMIDI_MULTI_COLOR29,
        PIANOMIDI_MULTI_COLOR30,
        PIANOMIDI_MULTI_COLOR31,
        PIANOMIDI_MULTI_COLOR32,
    GLOB_FXPianoRoll,
    GLOB_FXPianoRollSpark,
    GLOB_FXPianoRollVoicesLabels,
    GLOB_FXPianoRollOctavesLabels,
    GLOB_FXMIDIPattern,
    GLOB_FXMIDICutLine,
    GLOB_FXMIDIBarStyle,
    GLOB_FXMIDIBarVibrato,
    GLOB_FXPiano,
    GLOB_FXPianoCutLine,
    GLOB_FXPianoColorMode,
    GLOB_FX3DSpectrum,
        
    GLOB_FXSpectrum,
    GLOB_FX2,
    GLOB_FX3,
    GLOB_FX4,
    GLOB_FX5,
    GLOB_FXRandom,
    
    MAX_SETTINGS
};

typedef struct {
    //boolswitch
    unsigned char switch_value;
} t_setting_boolswitch;

typedef struct {
    //switch
    unsigned char switch_value;
    unsigned char switch_value_nb;
    char **switch_labels;
} t_setting_switch;

typedef struct {
    //slider
    float slider_value;
    //unsigned int slider_value_nb;
    float slider_min_value;
    float slider_mid_value;
    float slider_max_value;
    char slider_digits; //100:percentage, 60:time, 1:1 digit, 2:2 digits
} t_setting_slider;

typedef struct {
    //textbox
    char *text;
    int max_width_char;    
} t_setting_textbox;

typedef struct {
    //textbox
    char *text;
} t_setting_msgbox;

typedef struct {
    //color
    int rgb;
} t_setting_color;



typedef struct {
//common fields
    MDZ_SETTINGS_TYPE type;
    const char *setting_id;
    char *label;
    char *description;
    unsigned short int family;
    unsigned short int sub_family;
    void (*callback)(id);
    SEL mdz_selector;
    union {
        t_setting_slider mdz_slider;
        t_setting_switch mdz_switch;
        t_setting_boolswitch mdz_boolswitch;
        t_setting_textbox mdz_textbox;
        t_setting_msgbox mdz_msgbox;
        t_setting_color mdz_color;
    } detail;
} t_settings;



@interface SettingsGenViewController : UIViewController <UINavigationControllerDelegate,UITextFieldDelegate,UIColorPickerViewControllerDelegate,MSColorSelectionViewControllerDelegate,UIPopoverPresentationControllerDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
    IBOutlet TPKeyboardAvoidingTableView *tableView;
    int cur_settings_nb;
    int cur_settings_idx[MAX_SETTINGS];
    
    CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
    
    NSMutableDictionary *dictActionBtn;
    
    WaitingView *waitingView,*waitingViewPlayer;
    NSTimer *repeatingTimer;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    //Color picker
    UIButton *currentColorPickerBtn;
    
    //FTP
    CFtpServer *ftpserver;
    CFtpServer::CUserEntry *pAnonymousUser;
    CFtpServer::CUserEntry *pUser;
    bool bServerRunning;
    
    bool darkMode;
    bool forceReloadCells;

@public
    IBOutlet DetailViewControllerIphone *detailViewController;
    int current_family;
}

@property (nonatomic,retain) IBOutlet TPKeyboardAvoidingTableView *tableView;
@property (nonatomic,retain) IBOutlet DetailViewControllerIphone *detailViewController;

@property (nonatomic, retain) CMPopTipView *popTipView;
@property (nonatomic, retain) WaitingView *waitingView,*waitingViewPlayer;

+ (void) loadSettings;
+ (void) restoreSettings;
+ (void) backupSettings;
+ (void) applyDefaultSettings;
+ (void) oscilloGenSystemColor:(int)_mode color_idx:(int)color_idx color_buffer:(unsigned int*)color_buffer;
+ (void) pianomidiGenSystemColor:(int)_mode color_idx:(int)color_idx color_buffer:(unsigned int*)color_buffer;

- (IBAction) goPlayer;
- (void) updateMiniPlayer;

@end
