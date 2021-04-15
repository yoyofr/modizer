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
    MDZ_TEXTBOX,
    MDZ_MSGBOX
};

enum MDZ_SETTINGS_SCOPE {
    SETTINGS_ALL=0,
    SETTINGS_GLOBAL,
    SETTINGS_VISU,
    SETTINGS_ADPLUG,
    SETTINGS_GME,
    SETTINGS_GSF,
    SETTINGS_HC,
    SETTINGS_MODPLUG,
    SETTINGS_SID,
    SETTINGS_UADE,
    SETTINGS_TIMIDITY,
    SETTINGS_VGMPLAY,
    SETTINGS_VGMSTREAM,
    SETTINGS_ONLINE
};

enum MDZ_SETTINGS {
    MDZ_SETTINGS_FAMILY_ROOT=1,
    MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER,
    GLOB_DefaultMODPlayer,
    GLOB_DefaultSAPPlayer,
    GLOB_DefaultVGMPlayer,
    GLOB_ForceMono,
    GLOB_Panning,
    GLOB_PanningValue,
    GLOB_DefaultLength,
    GLOB_CoverFlow,
    GLOB_PlayEnqueueAction,
    GLOB_EnqueueMode,
    GLOB_AfterDownloadAction,    
    GLOB_BackgroundMode,
    GLOB_TitleFilename,
    GLOB_PlayerViewOnPlay,
    GLOB_StatsUpload,
    
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
    
        MDZ_SETTINGS_FAMILY_GME,
        GME_FADEOUT,
        GME_RATIO_ONOFF,
        GME_RATIO,
        GME_IGNORESILENCE,
        GME_EQ_BASS,
        GME_EQ_TREBLE,
        GME_FX_ONOFF,
        GME_FX_SURROUND,
        GME_FX_ECHO,
        GME_FX_PANNING,
    
        MDZ_SETTINGS_FAMILY_GSF,
        GSF_SOUNDQUALITY,
        GSF_INTERPOLATION,
        GSF_LOWPASSFILTER,
        GSF_ECHO,
    
        MDZ_SETTINGS_FAMILY_HC,
        HC_ResampleQuality,
    
        MDZ_SETTINGS_FAMILY_OMPT,
        OMPT_MasterVolume,
        OMPT_Sampling,
        OMPT_StereoSeparation,
            
            
        MDZ_SETTINGS_FAMILY_SID,
        SID_Filter,
        SID_ForceLoop,
        SID_CLOCK,
        SID_MODEL,
        SID_SecondSIDOn,
        SID_SecondSIDAddress,
        SID_ThirdSIDOn,
        SID_ThirdSIDAddress,
    
    
        MDZ_SETTINGS_FAMILY_TIMIDITY,
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
        VGMPLAY_Maxloop,
        VGMPLAY_YM2612Emulator,
        VGMPLAY_PreferJTAG,
    
        MDZ_SETTINGS_FAMILY_VGMSTREAM,
        VGMSTREAM_ResampleQuality,
        VGMSTREAM_Forceloop,
        VGMSTREAM_Maxloop,
        VGMSTREAM_Fadeouttime,
    
        
    
    
    MDZ_SETTINGS_FAMILY_GLOBAL_VISU,
    GLOB_FXAlpha,
    GLOB_FXLOD,
    GLOB_FXFPS,
    GLOB_FXMSAA,
    GLOB_FXMODPattern,
    GLOB_FXMIDIPattern,
    GLOB_FXPiano,
        GLOB_FXPianoColorMode,
    GLOB_FX3DSpectrum,
    GLOB_FXOscillo,
    GLOB_FXSpectrum,
    GLOB_FXBeat,
    GLOB_FX1,
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
    float slider_max_value;
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
//common fields
    MDZ_SETTINGS_TYPE type;
    char *label;
    char *description;
    char family;
    char sub_family;
    void (*callback)(id);
    union {
        t_setting_slider mdz_slider;
        t_setting_switch mdz_switch;
        t_setting_boolswitch mdz_boolswitch;
        t_setting_textbox mdz_textbox;
        t_setting_msgbox mdz_msgbox;
    } detail;
} t_settings;



@interface SettingsGenViewController : UIViewController <UINavigationControllerDelegate,UITextFieldDelegate> {
    IBOutlet TPKeyboardAvoidingTableView *tableView;
    int cur_settings_nb;
    int cur_settings_idx[MAX_SETTINGS];
    
    WaitingView *waitingView;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    //FTP
    CFtpServer *ftpserver;
    CFtpServer::CUserEntry *pAnonymousUser;
    CFtpServer::CUserEntry *pUser;
    bool bServerRunning;
    
    bool darkMode;
    bool forceReloadCells;

@public
    IBOutlet DetailViewControllerIphone *detailViewController;
    char current_family;
}

@property (nonatomic,retain) IBOutlet TPKeyboardAvoidingTableView *tableView;
@property (nonatomic,retain) IBOutlet DetailViewControllerIphone *detailViewController;

+ (void) loadSettings;
+ (void) restoreSettings;
+ (void) backupSettings;
+ (void) applyDefaultSettings;
-(IBAction) goPlayer;
-(void) updateMiniPlayer;

@end
