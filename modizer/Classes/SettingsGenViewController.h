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

enum MDZ_SETTINGS_TYPE {
    MDZ_FAMILY=1,
    MDZ_SWITCH,
    MDZ_BOOLSWITCH,
    MDZ_SLIDER_CONTINUOUS,
    MDZ_SLIDER_DISCRETE,
    MDZ_TEXTBOX
};

enum MDZ_SETTINGS_FAMILY {
    MDZ_SETTINGS_ROOT=1,
    MDZ_SETTINGS_GLOBAL_PLAYER,
    MDZ_SETTINGS_GLOBAL_VISU,
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

enum MDZ_SETTINGS {
    MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER=1,
    GLOB_ForceMono,
    GLOB_Panning,
    GLOB_PanningValue,
    GLOB_DefaultLength,    
    GLOB_TitleFilename,
    GLOB_StatsUpload,
    GLOB_BackgroundMode,
    GLOB_PlayEnqueueAction,
    GLOB_EnqueueMode,
    GLOB_AfterDownloadAction,
    GLOB_DefaultMODPlayer,
    
    GLOB_CoverFlow,
    GLOB_PlayerViewOnPlay,
    
    MDZ_SETTINGS_FAMILY_GLOBAL_VISU,
    GLOB_FXLOD,
    GLOB_FXRandom,
    GLOB_FXAlpha,
    GLOB_FXMODPattern,
    GLOB_FXMIDIPattern,
    GLOB_FXPiano,
    GLOB_FXOscillo,
    GLOB_FXSpectrum,
    GLOB_FXBeat,
    GLOB_FX1,
    GLOB_FX2,
    GLOB_FX3,
    GLOB_FX4,
    GLOB_FX5,
    
    
    MDZ_SETTINGS_FAMILY_PLUGINS,
    
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
    
        MDZ_SETTINGS_FAMILY_UADE,
        UADE_Head,
        UADE_Led,
        UADE_Norm,
        UADE_PostFX,
        UADE_Pan,
            UADE_PanValue,    
        UADE_Gain,
            UADE_GainValue,
    
        MDZ_SETTINGS_FAMILY_TIMIDITY,
        TIM_Polyphony,
        TIM_Chorus,
        TIM_Reverb,
        TIM_Resample,
        TIM_LPFilter,
    
        MDZ_SETTINGS_FAMILY_SID,
        SID_Optim,
        SID_LibVersion,
        SID_Filter,
    
        MDZ_SETTINGS_FAMILY_DUMB,
        DUMB_MasterVolume,
        DUMB_Resampling,
    
        MDZ_SETTINGS_FAMILY_SEXYPSF,
        SEXYPSF_Reverb,
        SEXYPSF_Interpolation,
    
        MDZ_SETTINGS_FAMILY_AOSDK,
        AOSDK_Reverb,
        AOSDK_Interpolation,
        AOSDK_DSFEmuRatio,
        AOSDK_DSFDSP,
        AOSDK_DSF22KHZ,
        AOSDK_SSFEmuRatio,
        AOSDK_SSFDSP,
    
        MDZ_SETTINGS_FAMILY_ADPLUG,
        ADPLUG_OplType,
    
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
//common fields
    MDZ_SETTINGS_TYPE type;
    char *label;
    char *description;
    char family;
    char sub_family;
    union {
        t_setting_slider mdz_slider;
        t_setting_switch mdz_switch;
        t_setting_boolswitch mdz_boolswitch;
        t_setting_textbox mdz_textbox;
    } detail;
} t_settings;



@interface SettingsGenViewController : UIViewController {
    IBOutlet UITableView *tableView;
    int cur_settings_nb;
    int cur_settings_idx[MAX_SETTINGS];
@public
    IBOutlet DetailViewControllerIphone *detailViewController;
    char current_family;
}

@property (nonatomic,retain) IBOutlet UITableView *tableView;
@property (nonatomic,retain) IBOutlet DetailViewControllerIphone *detailViewController;

+ (void) loadSettings;

@end
