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

enum MDZ_SETTINGS_TYPE {
    MDZ_FAMILY=1,
    MDZ_SWITCH,
    MDZ_BOOLSWITCH,
    MDZ_SLIDER_CONTINUOUS,
    MDZ_SLIDER_DISCRETE,
    MDZ_TEXTBOX,
    MDZ_MSGBOX
};

enum MDZ_SETTINGS_SCOPE {
    SETTINGS_ALL=0,
    SETTINGS_GLOBAL,
    SETTINGS_VISU,
    SETTINGS_ADPLUG,
    SETTINGS_AOSDK,
    SETTINGS_DUMB,
    SETTINGS_GME,
    SETTINGS_MODPLUG,
    SETTINGS_SEXYPSF,
    SETTINGS_SID,
    SETTINGS_UADE,
    SETTINGS_TIMIDITY
};

/*
enum MDZ_SETTINGS_FAMILY {
    MDZ_SETTINGS_ROOT=1,
    MDZ_SETTINGS_GLOBAL_PLAYER,
    MDZ_SETTINGS_GLOBAL_VISU,
    MDZ_SETTINGS_GLOBAL_FTP,
    MDZ_SETTINGS_PLUGINS,
    MDZ_SETTINGS_MODPLUG,
    MDZ_SETTINGS_UADE,
    MDZ_SETTINGS_TIMIDITY,
    MDZ_SETTINGS_SID,
    MDZ_SETTINGS_DUMB,
    MDZ_SETTINGS_SEXYPSF,
    MDZ_SETTINGS_AOSDK,
    MDZ_SETTINGS_ADPLUG
};
 */

enum MDZ_SETTINGS {
    MDZ_SETTINGS_FAMILY_ROOT=1,
    MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER,
    GLOB_DefaultMODPlayer,
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
    
    MDZ_SETTINGS_FAMILY_PLUGINS,
    
        MDZ_SETTINGS_FAMILY_ADPLUG,
        ADPLUG_OplType,
    
        MDZ_SETTINGS_FAMILY_AOSDK,
        AOSDK_Reverb,
        AOSDK_Interpolation,
        AOSDK_DSFEmuRatio,
        AOSDK_DSFDSP,
        AOSDK_DSF22KHZ,
        AOSDK_SSFEmuRatio,
        AOSDK_SSFDSP,
    
        MDZ_SETTINGS_FAMILY_DUMB,
        DUMB_MasterVolume,
        DUMB_Resampling,
    
    
        MDZ_SETTINGS_FAMILY_GME,
        GME_FADEOUT,
        GME_EQ_BASS,
        GME_EQ_TREBLE,
        GME_FX_ONOFF,
        GME_FX_SURROUND,
        GME_FX_ECHO,
        GME_FX_PANNING,
    
        MDZ_SETTINGS_FAMILY_MODPLUG,
        MODPLUG_MasterVolume,
        MODPLUG_Sampling,
        MODPLUG_StereoSeparation,
        MODPLUG_Megabass,
            MODPLUG_BassAmount,
            MODPLUG_BassRange,
        MODPLUG_Reverb,
            MODPLUG_ReverbDepth,
            MODPLUG_ReverbDelay,
        MODPLUG_Surround,
            MODPLUG_SurroundDepth,
            MODPLUG_SurroundDelay,
    
        MDZ_SETTINGS_FAMILY_SEXYPSF,
        SEXYPSF_Reverb,
        SEXYPSF_Interpolation,
    
    
        MDZ_SETTINGS_FAMILY_SID,
        SID_Optim,
        SID_LibVersion,
        SID_Filter,
        SID_CLOCK,
        SID_MODEL,
    
    
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
    
    
    MDZ_SETTINGS_FAMILY_GLOBAL_VISU,
    GLOB_FXAlpha,
    GLOB_FXLOD,
    GLOB_FXMSAA,
    GLOB_FXMODPattern,
    GLOB_FXMODPatternVolume,
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
    unsigned int slider_value_nb;
    float slider_min_value;
    float slider_max_value;
} t_setting_slider;

typedef struct {
    //textbox
    char *text;
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



@interface SettingsGenViewController : UIViewController <UITextFieldDelegate> {
    IBOutlet TPKeyboardAvoidingTableView *tableView;
    int cur_settings_nb;
    int cur_settings_idx[MAX_SETTINGS];
    
    //FTP
    CFtpServer *ftpserver;
    CFtpServer::CUserEntry *pAnonymousUser;
    CFtpServer::CUserEntry *pUser;
    bool bServerRunning;
@public
    IBOutlet DetailViewControllerIphone *detailViewController;
    char current_family;
}

@property (nonatomic,retain) IBOutlet TPKeyboardAvoidingTableView *tableView;
@property (nonatomic,retain) IBOutlet DetailViewControllerIphone *detailViewController;

+ (void) loadSettings;
+ (void) restoreSettings;
+ (void) applyDefaultSettings;

@end
