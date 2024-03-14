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
    MDZ_TEXTBOX,
    MDZ_MSGBOX,
    MDZ_COLORPICKER
};

enum MDZ_SETTINGS_SCOPE {
    SETTINGS_ALL=0,
    SETTINGS_GLOBAL,
    SETTINGS_VISU,
    SETTINGS_ADPLUG,
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
    SETTINGS_OSCILLO
};

enum MDZ_SETTINGS {
    MDZ_SETTINGS_FAMILY_ROOT=1,
    MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER,
    
    //GLOB_PlaybackFrequency,
    GLOB_SearchRegExp,
    GLOB_ResumeOnStart,
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
    GLOB_RecreateSamplesFolder,
    GLOB_StatsUpload,
    
    MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY,
    GLOB_DefaultMODPlayer,
    GLOB_DefaultSAPPlayer,
    GLOB_DefaultSIDPlayer,
    GLOB_DefaultVGMPlayer,
    GLOB_DefaultNSFPlayer,
    GLOB_DefaultKSSPlayer,
    GLOB_DefaultMIDIPlayer,
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
    
        MDZ_SETTINGS_FAMILY_NSFPLAY,
        NSFPLAY_DefaultLength,
        NSFPLAY_N163_OPTION0,
        NSFPLAY_N163_OPTION1,
        NSFPLAY_N163_OPTION2,
    
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
        VGMPLAY_PreferJTAG,
        VGMPLAY_YMF262Emulator,
        VGMPLAY_YM2612Emulator,
        VGMPLAY_NUKEDOPN2_Option,
    
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
        OSCILLO_MONO_COLOR,
        OSCILLO_MULTI_COLOR01,
        OSCILLO_MULTI_COLOR02,
        OSCILLO_MULTI_COLOR03,
        OSCILLO_MULTI_COLOR04,
        OSCILLO_MULTI_COLOR05,
        OSCILLO_MULTI_COLOR06,
        OSCILLO_MULTI_COLOR07,
        OSCILLO_MULTI_COLOR08,        
    GLOB_FXAlpha,
    GLOB_FXLOD,
    GLOB_FXFPS,
    GLOB_FXMSAA,
    GLOB_FXMODPattern,
    GLOB_FXMODPattern_CurrentLineMode,
    GLOB_FXMODPattern_Font,
    GLOB_FXMODPattern_FontSize,
    GLOB_FXMIDIPattern,
    GLOB_FXPiano,
    GLOB_FXPianoColorMode,
    GLOB_FX3DSpectrum,
        
    GLOB_FXOscillo,
    GLOB_FXOscilloShowLabel,
    GLOB_FXOscilloShowGrid,
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
    //color
    int rgb;
} t_setting_color;



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
        t_setting_color mdz_color;
    } detail;
} t_settings;



@interface SettingsGenViewController : UIViewController <UINavigationControllerDelegate,UITextFieldDelegate,UIColorPickerViewControllerDelegate,MSColorSelectionViewControllerDelegate,UIPopoverPresentationControllerDelegate> {
    IBOutlet TPKeyboardAvoidingTableView *tableView;
    int cur_settings_nb;
    int cur_settings_idx[MAX_SETTINGS];
    
    NSMutableDictionary *dictActionBtn;
    
    WaitingView *waitingView;
    
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
    char current_family;
}

@property (nonatomic,retain) IBOutlet TPKeyboardAvoidingTableView *tableView;
@property (nonatomic,retain) IBOutlet DetailViewControllerIphone *detailViewController;

+ (void) loadSettings;
+ (void) restoreSettings;
+ (void) backupSettings;
+ (void) applyDefaultSettings;
+(void) oscilloGenSystemColor:(int)mode;

-(IBAction) goPlayer;
-(void) updateMiniPlayer;



@end
