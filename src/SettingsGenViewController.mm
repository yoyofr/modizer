//
//  SettingsGenViewController.m
//  modizer
//
//  Created by Yohann Magnien on 10/08/13.
//
//

extern volatile int mSlowDevice;

#import "SettingsGenViewController.h"
#import "MNEValueTrackingSlider.h"

#import "Reachability.h"
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/xattr.h>


@interface SettingsGenViewController ()
@end

@implementation SettingsGenViewController

@synthesize tableView,detailViewController;

volatile t_settings settings[MAX_SETTINGS];

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];
    else {
        UIAlertView *nofileplaying=[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
        [nofileplaying show];
    }
}

#pragma mark - Callback methods

//FTP
void optFTPSwitchChanged(id param) {
    [param FTPswitchChanged];
}

//ONLINE
void optONLINESwitchChanged(id param) {
    [SettingsGenViewController ONLINEswitchChanged];
}


//GLOBAL
-(void) optGLOBALChanged {
    [detailViewController settingsChanged:(int)SETTINGS_GLOBAL];
}
void optGLOBALChangedC(id param) {
    [param optGLOBALChanged];
}
//VISU
-(void) optVISUChanged {
    [detailViewController settingsChanged:(int)SETTINGS_VISU];
}
void optVISUChangedC(id param) {
    [param optVISUChanged];
}
//ADPLUG
-(void) optADPLUGChanged {
    [detailViewController settingsChanged:(int)SETTINGS_ADPLUG];
}
void optADPLUGChangedC(id param) {
    [param optADPLUGChanged];
}
//AOSDK
-(void) optAOSDKChanged {
    [detailViewController settingsChanged:(int)SETTINGS_AOSDK];
}
void optAOSDKChangedC(id param) {
    [param optAOSDKChanged];
}
//DUMB
-(void) optDUMBChanged {
    [detailViewController settingsChanged:(int)SETTINGS_DUMB];
}
void optDUMBChangedC(id param) {
    [param optDUMBChanged];
}
//GME
-(void) optGMEChanged {
    [detailViewController settingsChanged:(int)SETTINGS_GME];
}
void optGMEChangedC(id param) {
    [param optGMEChanged];
}
//MODPLUG
-(void) optMODPLUGChanged {
    [detailViewController settingsChanged:(int)SETTINGS_MODPLUG];
}
void optMODPLUGChangedC(id param) {
    [param optMODPLUGChanged];
}
//SEXYPSF
-(void) optSEXYPSFChanged {
    [detailViewController settingsChanged:(int)SETTINGS_SEXYPSF];
}
void optSEXYPSFChangedC(id param) {
    [param optSEXYPSFChanged];
}
//SID
-(void) optSIDChanged {
    [detailViewController settingsChanged:(int)SETTINGS_SID];
}
void optSIDChangedC(id param) {
    [param optSIDChanged];
}
//TIMIDITY
-(void) optTIMIDITYChanged {
    [detailViewController settingsChanged:(int)SETTINGS_TIMIDITY];
}
void optTIMIDITYChangedC(id param) {
    [param optTIMIDITYChanged];
}
//UADE
-(void) optUADEChanged {
    [detailViewController settingsChanged:(int)SETTINGS_UADE];
}
void optUADEChangedC(id param) {
    [param optUADEChanged];
}
//VGMPLAY
-(void) optVGMPLAYChanged {
    [detailViewController settingsChanged:(int)SETTINGS_VGMPLAY];
}
void optVGMPLAYChangedC(id param) {
    [param optVGMPLAYChanged];
}
//VGMSTREAM
-(void) optVGMSTREAMChanged {
    [detailViewController settingsChanged:(int)SETTINGS_VGMSTREAM];
}
void optVGMSTREAMChangedC(id param) {
    [param optVGMSTREAMChanged];
}

//LAZYUSF
-(void) optLAZYUSFChanged {
    [detailViewController settingsChanged:(int)SETTINGS_LAZYUSF];
}
void optLAZYUSFChangedC(id param) {
    [param optLAZYUSFChanged];
}
//GSF
-(void) optGSFChanged {
    [detailViewController settingsChanged:(int)SETTINGS_GSF];
}
void optGSFChangedC(id param) {
    [param optGSFChanged];
}

#pragma mark - Load/Init default settings

+ (void) restoreSettings {
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    NSString *str;

	for (int i=0;i<MAX_SETTINGS;i++)
        if (settings[i].label) {
        
            str=[NSString stringWithFormat:@"%s",settings[i].label];
            int j=settings[i].family;
            while (j!=MDZ_SETTINGS_FAMILY_ROOT) {
                str=[NSString stringWithFormat:@"%@/%s",str,settings[j].label];
                j=settings[j].family;
            }
            
            switch (settings[i].type) {
                case MDZ_BOOLSWITCH:
                    valNb=[prefs objectForKey:str];
                    if (valNb!=nil) settings[i].detail.mdz_boolswitch.switch_value=[valNb intValue];
                    break;
                case MDZ_SWITCH:
                    valNb=[prefs objectForKey:str];
                    if (valNb!=nil) settings[i].detail.mdz_switch.switch_value=[valNb intValue];
                    break;
                case MDZ_SLIDER_DISCRETE:
                    valNb=[prefs objectForKey:str];
                    if (valNb!=nil) settings[i].detail.mdz_slider.slider_value=[valNb intValue];
                    break;
                case MDZ_SLIDER_DISCRETE_TIME:
                    valNb=[prefs objectForKey:str];
                    if (valNb!=nil) settings[i].detail.mdz_slider.slider_value=[valNb intValue];
                    break;
                case MDZ_SLIDER_CONTINUOUS:
                    valNb=[prefs objectForKey:str];
                    if (valNb!=nil) settings[i].detail.mdz_slider.slider_value=[valNb floatValue];
                    break;
                case MDZ_TEXTBOX:
                    str=[prefs objectForKey:str];
                    if (str!=nil) {
                        if (settings[i].detail.mdz_textbox.text) free(settings[i].detail.mdz_textbox.text);
                        settings[i].detail.mdz_textbox.text=(char*)malloc(strlen([str UTF8String])+1);
                        strcpy(settings[i].detail.mdz_textbox.text,[str UTF8String]);
                    }
                    break;
                    //
                case MDZ_MSGBOX:
                    break;
                case MDZ_FAMILY:
                    break;
            }
        }
    [SettingsGenViewController ONLINEswitchChanged];
}

+ (void) backupSettings {
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    NSString *str;
    
	for (int i=0;i<MAX_SETTINGS;i++)
        if (settings[i].label) {
            
            str=[NSString stringWithFormat:@"%s",settings[i].label];
            int j=settings[i].family;
            while (j!=MDZ_SETTINGS_FAMILY_ROOT) {
                str=[NSString stringWithFormat:@"%@/%s",str,settings[j].label];
                j=settings[j].family;
            }
            
            switch (settings[i].type) {
                case MDZ_BOOLSWITCH:
                    valNb=[[NSNumber alloc] initWithInt:settings[i].detail.mdz_boolswitch.switch_value];
                    [prefs setObject:valNb forKey:str];
                    break;
                case MDZ_SWITCH:
                    valNb=[[NSNumber alloc] initWithInt:settings[i].detail.mdz_switch.switch_value];
                    [prefs setObject:valNb forKey:str];
                    break;
                case MDZ_SLIDER_DISCRETE:
                    valNb=[[NSNumber alloc] initWithInt:settings[i].detail.mdz_slider.slider_value];
                    [prefs setObject:valNb forKey:str];
                    break;
                case MDZ_SLIDER_DISCRETE_TIME:
                    valNb=[[NSNumber alloc] initWithInt:settings[i].detail.mdz_slider.slider_value];
                    [prefs setObject:valNb forKey:str];
                    break;
                case MDZ_SLIDER_CONTINUOUS:
                    valNb=[[NSNumber alloc] initWithFloat:settings[i].detail.mdz_slider.slider_value];
                    [prefs setObject:valNb forKey:str];
                    break;
                case MDZ_TEXTBOX:
                    if (settings[i].detail.mdz_textbox.text) [prefs setObject:[NSString stringWithFormat:@"%s",settings[i].detail.mdz_textbox.text] forKey:str];
                    else [prefs setObject:@"" forKey:str];
                    break;
                    //
                case MDZ_MSGBOX:
                    break;
                case MDZ_FAMILY:
                    break;
            }
        }
    [prefs synchronize];
}

+ (void) applyDefaultSettings {
    /////////////////////////////////////
    //GLOBAL Player
    /////////////////////////////////////
    settings[GLOB_ForceMono].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_Panning].detail.mdz_boolswitch.switch_value=1;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_value=0.7;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_value=SONG_DEFAULT_LENGTH/1000;
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_StatsUpload].detail.mdz_boolswitch.switch_value=1;
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value=2;
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value=2;
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value=0;
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value=1;
    settings[GLOB_CoverFlow].detail.mdz_boolswitch.switch_value=1;
    settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value=0;
    
    /////////////////////////////////////
    //GLOBAL FTP
    /////////////////////////////////////
    if (settings[FTP_STATUS].detail.mdz_msgbox.text) free(settings[FTP_STATUS].detail.mdz_msgbox.text);
    settings[FTP_STATUS].detail.mdz_msgbox.text=(char*)malloc(strlen("Inactive")+1);
    strcpy(settings[FTP_STATUS].detail.mdz_msgbox.text,"Inactive");
    
    settings[FTP_ONOFF].detail.mdz_switch.switch_value=0;
    settings[FTP_ANONYMOUS].detail.mdz_boolswitch.switch_value=1;
    
    if (settings[FTP_USER].detail.mdz_textbox.text) free(settings[FTP_USER].detail.mdz_textbox.text);
    settings[FTP_USER].detail.mdz_textbox.text=NULL;//(char*)"modizer";
    
    if (settings[FTP_PASSWORD].detail.mdz_textbox.text) free(settings[FTP_PASSWORD].detail.mdz_textbox.text);
    settings[FTP_PASSWORD].detail.mdz_textbox.text=NULL;//(char*)"modizer";
    
    if (settings[FTP_PORT].detail.mdz_textbox.text) free(settings[FTP_PORT].detail.mdz_textbox.text);
    settings[FTP_PORT].detail.mdz_textbox.text=(char*)malloc(strlen("21")+1);
    strcpy(settings[FTP_PORT].detail.mdz_textbox.text,"21");
    
    /////////////////////////////////////
    //GLOBAL ONLINE
    /////////////////////////////////////
    if (settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text);
    settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(MODLAND_HOST_DEFAULT)+1);
    strcpy(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text,MODLAND_HOST_DEFAULT);
    
    if (settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text);
    settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(HVSC_HOST_DEFAULT)+1);
    strcpy(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text,HVSC_HOST_DEFAULT);
    
    if (settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text);
    settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(ASMA_HOST_DEFAULT)+1);
    strcpy(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text,ASMA_HOST_DEFAULT);
    
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_value=0;
    settings[ONLINE_HVSC_URL].detail.mdz_boolswitch.switch_value=0;
    settings[ONLINE_ASMA_URL].detail.mdz_boolswitch.switch_value=0;
    
    if (settings[ONLINE_MODLAND_URL_CUSTOM].detail.mdz_textbox.text) free(settings[ONLINE_MODLAND_URL_CUSTOM].detail.mdz_textbox.text);
    settings[ONLINE_MODLAND_URL_CUSTOM].detail.mdz_textbox.text=NULL;
    
    if (settings[ONLINE_HVSC_URL_CUSTOM].detail.mdz_textbox.text) free(settings[ONLINE_HVSC_URL_CUSTOM].detail.mdz_textbox.text);
    settings[ONLINE_HVSC_URL_CUSTOM].detail.mdz_textbox.text=NULL;
    
    if (settings[ONLINE_ASMA_URL_CUSTOM].detail.mdz_textbox.text) free(settings[ONLINE_ASMA_URL_CUSTOM].detail.mdz_textbox.text);
    settings[ONLINE_ASMA_URL_CUSTOM].detail.mdz_textbox.text=NULL;
    
    /////////////////////////////////////
    //Visualizers
    /////////////////////////////////////
    settings[GLOB_FXRandom].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_FXAlpha].detail.mdz_slider.slider_value=0.7;
    settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXPiano].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value=1;
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=1;
    settings[GLOB_FX1].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
    settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
    settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
    
    if (mSlowDevice) settings[GLOB_FXLOD].detail.mdz_switch.switch_value=1;
    else settings[GLOB_FXLOD].detail.mdz_switch.switch_value=2;
    
    settings[GLOB_FXFPS].detail.mdz_switch.switch_value=1;
    
    settings[GLOB_FXMSAA].detail.mdz_switch.switch_value=0;
    
    /////////////////////////////////////
    //PLUGINS
    /////////////////////////////////////
    
    /////////////////////////////////////
    //MODPLUG
    /////////////////////////////////////
    settings[MODPLUG_MasterVolume].detail.mdz_slider.slider_value=0.5;
    settings[MODPLUG_Sampling].detail.mdz_switch.switch_value=2;
/*    settings[MODPLUG_Megabass].detail.mdz_boolswitch.switch_value=0;
    settings[MODPLUG_BassAmount].detail.mdz_slider.slider_value=0.7;
    settings[MODPLUG_BassRange].detail.mdz_slider.slider_value=0.3;
    settings[MODPLUG_Surround].detail.mdz_boolswitch.switch_value=0;
    settings[MODPLUG_SurroundDepth].detail.mdz_slider.slider_value=0.9;
    settings[MODPLUG_SurroundDelay].detail.mdz_slider.slider_value=0.8;
    settings[MODPLUG_Reverb].detail.mdz_boolswitch.switch_value=0;
    settings[MODPLUG_ReverbDepth].detail.mdz_slider.slider_value=0.8;
    settings[MODPLUG_ReverbDelay].detail.mdz_slider.slider_value=0.7;*/
    settings[MODPLUG_StereoSeparation].detail.mdz_slider.slider_value=0.5;
    
    /////////////////////////////////////
    //DUMB
    /////////////////////////////////////
    settings[DUMB_MasterVolume].detail.mdz_slider.slider_value=0.5;
    settings[DUMB_Resampling].detail.mdz_switch.switch_value=1;
    
    /////////////////////////////////////
    //TIMIDITY
    /////////////////////////////////////
    settings[TIM_Polyphony].detail.mdz_slider.slider_value=128;
    settings[TIM_Amplification].detail.mdz_slider.slider_value=100;
    settings[TIM_Chorus].detail.mdz_boolswitch.switch_value=1;
    settings[TIM_Reverb].detail.mdz_boolswitch.switch_value=1;
    settings[TIM_LPFilter].detail.mdz_boolswitch.switch_value=1;
    settings[TIM_Resample].detail.mdz_switch.switch_value=1;
    
    /////////////////////////////////////
    //GME
    /////////////////////////////////////
    settings[GME_FADEOUT].detail.mdz_slider.slider_value=1;
    settings[GME_RATIO].detail.mdz_slider.slider_value=1;
    settings[GME_RATIO_ONOFF].detail.mdz_slider.slider_value=1;
    settings[GME_IGNORESILENCE].detail.mdz_slider.slider_value=0;
    settings[GME_EQ_BASS].detail.mdz_slider.slider_value=4.2-1.9;
    settings[GME_EQ_TREBLE].detail.mdz_slider.slider_value=-14;
    settings[GME_FX_ONOFF].detail.mdz_boolswitch.switch_value=0;
    settings[GME_FX_SURROUND].detail.mdz_boolswitch.switch_value=0;
    settings[GME_FX_ECHO].detail.mdz_boolswitch.switch_value=0;
    settings[GME_FX_PANNING].detail.mdz_slider.slider_value=0;
    
    /////////////////////////////////////
    //GSF
    /////////////////////////////////////
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_value=2;
    settings[GSF_INTERPOLATION].detail.mdz_boolswitch.switch_value=1;
    settings[GSF_LOWPASSFILTER].detail.mdz_boolswitch.switch_value=1;
    settings[GSF_ECHO].detail.mdz_boolswitch.switch_value=0;
    
    /////////////////////////////////////
    //SID
    /////////////////////////////////////
    settings[SID_Filter].detail.mdz_boolswitch.switch_value=1;
    settings[SID_CLOCK].detail.mdz_switch.switch_value=0;
    settings[SID_MODEL].detail.mdz_switch.switch_value=0;
    
    /////////////////////////////////////
    //UADE
    /////////////////////////////////////
    settings[UADE_Head].detail.mdz_boolswitch.switch_value=0;
    settings[UADE_PostFX].detail.mdz_boolswitch.switch_value=1;
    settings[UADE_Led].detail.mdz_boolswitch.switch_value=0;
    settings[UADE_Norm].detail.mdz_boolswitch.switch_value=0;
    settings[UADE_Gain].detail.mdz_boolswitch.switch_value=0;
    settings[UADE_GainValue].detail.mdz_slider.slider_value=0.5;
    settings[UADE_Pan].detail.mdz_boolswitch.switch_value=1;
    settings[UADE_PanValue].detail.mdz_slider.slider_value=0.7;
    settings[UADE_NTSC].detail.mdz_boolswitch.switch_value=0;
    /////////////////////////////////////
    //SEXYPSF
    /////////////////////////////////////
    settings[SEXYPSF_Interpolation].detail.mdz_switch.switch_value=1;
    settings[SEXYPSF_Reverb].detail.mdz_switch.switch_value=1;
    
    /////////////////////////////////////
    //AOSDK
    /////////////////////////////////////
    settings[AOSDK_Interpolation].detail.mdz_switch.switch_value=2;
    settings[AOSDK_Reverb].detail.mdz_boolswitch.switch_value=1;
    settings[AOSDK_DSF22KHZ].detail.mdz_switch.switch_value=0;
    settings[AOSDK_DSFDSP].detail.mdz_boolswitch.switch_value=1;
    settings[AOSDK_DSFEmuRatio].detail.mdz_switch.switch_value=0;
    settings[AOSDK_SSFDSP].detail.mdz_boolswitch.switch_value=1;
    settings[AOSDK_SSFEmuRatio].detail.mdz_switch.switch_value=0;
    
    /////////////////////////////////////
    //ADPLUG
    /////////////////////////////////////
    settings[ADPLUG_OplType].detail.mdz_switch.switch_value=0;
    
    /////////////////////////////////////
    //VGMPLAY
    /////////////////////////////////////
    settings[VGMPLAY_Maxloop].detail.mdz_slider.slider_value=2;
    settings[VGMPLAY_YM2612Emulator].detail.mdz_slider.slider_value=0;
    settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value=0;
    
    /////////////////////////////////////
    //VGMSTREAM
    /////////////////////////////////////
    settings[VGMSTREAM_Forceloop].detail.mdz_boolswitch.switch_value=0;
    settings[VGMSTREAM_Maxloop].detail.mdz_slider.slider_value=2;
    settings[VGMSTREAM_ResampleQuality].detail.mdz_slider.slider_value=1;
    
    /////////////////////////////////////
    //LAZYUSF
    /////////////////////////////////////
    settings[LAZYUSF_ResampleQuality].detail.mdz_slider.slider_value=1;
    
    
}

+ (void) loadSettings {
    memset((char*)settings,0,sizeof(settings));
    /////////////////////////////////////
    //ROOT
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER].label=(char*)"Global";
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER].sub_family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_VISU].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_VISU].label=(char*)"Visualizers";
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_VISU].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_VISU].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_VISU].sub_family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].label=(char*)"Plugins";
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].sub_family=MDZ_SETTINGS_FAMILY_PLUGINS;
    
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_FTP].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_FTP].label=(char*)"FTP";
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_FTP].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_FTP].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_FTP].sub_family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE].label=(char*)"Online";
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE].sub_family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    
    
    
    /////////////////////////////////////
    //GLOBAL Player
    /////////////////////////////////////
    settings[GLOB_ForceMono].label=(char*)"Force Mono";
    settings[GLOB_ForceMono].description=NULL;
    settings[GLOB_ForceMono].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_ForceMono].sub_family=0;
    settings[GLOB_ForceMono].callback=&optGLOBALChangedC;
    settings[GLOB_ForceMono].type=MDZ_BOOLSWITCH;
    settings[GLOB_ForceMono].detail.mdz_boolswitch.switch_value=0;
    
    settings[GLOB_Panning].label=(char*)"Panning";
    settings[GLOB_Panning].description=NULL;
    settings[GLOB_Panning].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_Panning].sub_family=0;
    settings[GLOB_Panning].callback=&optGLOBALChangedC;
    settings[GLOB_Panning].type=MDZ_BOOLSWITCH;
    settings[GLOB_Panning].detail.mdz_boolswitch.switch_value=1;
    
    settings[GLOB_PanningValue].label=(char*)"Panning Value";
    settings[GLOB_PanningValue].description=NULL;
    settings[GLOB_PanningValue].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_PanningValue].sub_family=0;
    settings[GLOB_PanningValue].callback=&optGLOBALChangedC;
    settings[GLOB_PanningValue].type=MDZ_SLIDER_CONTINUOUS;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_value=0.7;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_min_value=0;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_max_value=1;
    
    
    settings[GLOB_DefaultLength].label=(char*)"Default Length(s)";
    settings[GLOB_DefaultLength].description=NULL;
    settings[GLOB_DefaultLength].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_DefaultLength].sub_family=0;
    settings[GLOB_DefaultLength].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultLength].type=MDZ_SLIDER_DISCRETE_TIME;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_value=SONG_DEFAULT_LENGTH/1000;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_min_value=10;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_max_value=600;
    
    settings[GLOB_DefaultMODPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultMODPlayer].label=(char*)"Default MOD player";
    settings[GLOB_DefaultMODPlayer].description=NULL;
    settings[GLOB_DefaultMODPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_DefaultMODPlayer].sub_family=0;
    settings[GLOB_DefaultMODPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value_nb=4;
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels[0]=(char*)"OMPT";
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels[1]=(char*)"DUMB";
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels[2]=(char*)"UADE";
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels[3]=(char*)"XMP";
    
    settings[GLOB_DefaultSAPPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultSAPPlayer].label=(char*)"Default SAP player";
    settings[GLOB_DefaultSAPPlayer].description=NULL;
    settings[GLOB_DefaultSAPPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_DefaultSAPPlayer].sub_family=0;
    settings[GLOB_DefaultSAPPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_labels[0]=(char*)"ASAP";
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_labels[1]=(char*)"GME";
    
    settings[GLOB_DefaultVGMPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultVGMPlayer].label=(char*)"Default VGM player";
    settings[GLOB_DefaultVGMPlayer].description=NULL;
    settings[GLOB_DefaultVGMPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_DefaultVGMPlayer].sub_family=0;
    settings[GLOB_DefaultVGMPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_labels[0]=(char*)"VGM";
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_labels[1]=(char*)"GME";
    
    settings[GLOB_TitleFilename].label=(char*)"Filename as title";
    settings[GLOB_TitleFilename].description=NULL;
    settings[GLOB_TitleFilename].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_TitleFilename].sub_family=0;
    settings[GLOB_TitleFilename].callback=&optGLOBALChangedC;
    settings[GLOB_TitleFilename].type=MDZ_BOOLSWITCH;
    settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value=0;
    
    settings[GLOB_StatsUpload].label=(char*)"Send statistics";
    settings[GLOB_StatsUpload].description=NULL;
    settings[GLOB_StatsUpload].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_StatsUpload].sub_family=0;
    settings[GLOB_StatsUpload].callback=&optGLOBALChangedC;
    settings[GLOB_StatsUpload].type=MDZ_BOOLSWITCH;
    settings[GLOB_StatsUpload].detail.mdz_boolswitch.switch_value=1;
    
    settings[GLOB_BackgroundMode].type=MDZ_SWITCH;
    settings[GLOB_BackgroundMode].label=(char*)"Background mode";
    settings[GLOB_BackgroundMode].description=NULL;
    settings[GLOB_BackgroundMode].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_BackgroundMode].sub_family=0;
    settings[GLOB_BackgroundMode].callback=&optGLOBALChangedC;
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value=1;
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels[1]=(char*)"Play";
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels[2]=(char*)"Full";
    
    settings[GLOB_EnqueueMode].type=MDZ_SWITCH;
    settings[GLOB_EnqueueMode].label=(char*)"Enqueue mode";
    settings[GLOB_EnqueueMode].description=NULL;
    settings[GLOB_EnqueueMode].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_EnqueueMode].sub_family=0;
    settings[GLOB_EnqueueMode].callback=&optGLOBALChangedC;
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value=2;
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels[0]=(char*)"First";
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels[1]=(char*)"Current";
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels[2]=(char*)"Last";
    
    settings[GLOB_PlayEnqueueAction].type=MDZ_SWITCH;
    settings[GLOB_PlayEnqueueAction].label=(char*)"Default Action";
    settings[GLOB_PlayEnqueueAction].description=NULL;
    settings[GLOB_PlayEnqueueAction].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_PlayEnqueueAction].sub_family=0;
    settings[GLOB_PlayEnqueueAction].callback=&optGLOBALChangedC;
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value=0;
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels[0]=(char*)"Play";
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels[1]=(char*)"Enqueue";
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels[2]=(char*)"Enq.&Play";
    
    settings[GLOB_AfterDownloadAction].type=MDZ_SWITCH;
    settings[GLOB_AfterDownloadAction].label=(char*)"Post download action";
    settings[GLOB_AfterDownloadAction].description=NULL;
    settings[GLOB_AfterDownloadAction].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_AfterDownloadAction].sub_family=0;
    settings[GLOB_AfterDownloadAction].callback=&optGLOBALChangedC;
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value=1;
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels[0]=(char*)"Nothing";
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels[1]=(char*)"Enqueue";
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels[2]=(char*)"Play";
    
    settings[GLOB_CoverFlow].label=(char*)"Coverflow";
    settings[GLOB_CoverFlow].description=NULL;
    settings[GLOB_CoverFlow].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_CoverFlow].sub_family=0;
    settings[GLOB_CoverFlow].callback=&optGLOBALChangedC;
    settings[GLOB_CoverFlow].type=MDZ_BOOLSWITCH;
    settings[GLOB_CoverFlow].detail.mdz_boolswitch.switch_value=1;
    
    settings[GLOB_PlayerViewOnPlay].label=(char*)"Player view on play";
    settings[GLOB_PlayerViewOnPlay].description=NULL;
    settings[GLOB_PlayerViewOnPlay].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_PlayerViewOnPlay].sub_family=0;
    settings[GLOB_PlayerViewOnPlay].callback=&optGLOBALChangedC;
    settings[GLOB_PlayerViewOnPlay].type=MDZ_BOOLSWITCH;
    settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value=0;
    
    /////////////////////////////////////
    //GLOBAL FTP
    /////////////////////////////////////
    settings[FTP_STATUS].label=(char*)"Server status";
    settings[FTP_STATUS].description=NULL;
    settings[FTP_STATUS].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_STATUS].sub_family=0;
    settings[FTP_STATUS].type=MDZ_MSGBOX;
    settings[FTP_STATUS].detail.mdz_msgbox.text=(char*)malloc(strlen("Inactive")+1);
    strcpy(settings[FTP_STATUS].detail.mdz_msgbox.text,"Inactive");
    
    
    settings[FTP_ONOFF].type=MDZ_SWITCH;
    settings[FTP_ONOFF].label=(char*)"FTP Server";
    settings[FTP_ONOFF].description=NULL;
    settings[FTP_ONOFF].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_ONOFF].callback=&optFTPSwitchChanged;
    settings[FTP_ONOFF].sub_family=0;
    settings[FTP_ONOFF].detail.mdz_switch.switch_value=0;
    settings[FTP_ONOFF].detail.mdz_switch.switch_value_nb=2;
    settings[FTP_ONOFF].detail.mdz_switch.switch_labels=(char**)malloc(settings[FTP_ONOFF].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[FTP_ONOFF].detail.mdz_switch.switch_labels[0]=(char*)"Stop";
    settings[FTP_ONOFF].detail.mdz_switch.switch_labels[1]=(char*)"Run";    
    
    settings[FTP_ANONYMOUS].label=(char*)"Authorize anonymous";
    settings[FTP_ANONYMOUS].description=NULL;
    settings[FTP_ANONYMOUS].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_ANONYMOUS].sub_family=0;
    settings[FTP_ANONYMOUS].type=MDZ_BOOLSWITCH;
    settings[FTP_ANONYMOUS].detail.mdz_boolswitch.switch_value=1;
    
    settings[FTP_USER].label=(char*)"User";
    settings[FTP_USER].description=NULL;
    settings[FTP_USER].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_USER].sub_family=0;
    settings[FTP_USER].type=MDZ_TEXTBOX;
    settings[FTP_USER].detail.mdz_textbox.text=NULL;//(char*)"modizer";
    
    settings[FTP_PASSWORD].label=(char*)"Password";
    settings[FTP_PASSWORD].description=NULL;
    settings[FTP_PASSWORD].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_PASSWORD].sub_family=0;
    settings[FTP_PASSWORD].type=MDZ_TEXTBOX;
    settings[FTP_PASSWORD].detail.mdz_textbox.text=NULL;//(char*)"modizer";
    
    settings[FTP_PORT].label=(char*)"Port";
    settings[FTP_PORT].description=NULL;
    settings[FTP_PORT].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_PORT].sub_family=0;
    settings[FTP_PORT].type=MDZ_TEXTBOX;
    settings[FTP_PORT].detail.mdz_textbox.text=(char*)malloc(strlen("21")+1);
    strcpy(settings[FTP_PORT].detail.mdz_textbox.text,"21");
    
    /////////////////////////////////////
    //GLOBAL ONLINE
    /////////////////////////////////////
    settings[ONLINE_MODLAND_CURRENT_URL].label=(char*)"MODLAND URL";
    settings[ONLINE_MODLAND_CURRENT_URL].description=NULL;
    settings[ONLINE_MODLAND_CURRENT_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_MODLAND_CURRENT_URL].sub_family=0;
    settings[ONLINE_MODLAND_CURRENT_URL].type=MDZ_MSGBOX;
    settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen("N/A")+1);
    strcpy(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text,"N/A");
    
    settings[ONLINE_HVSC_CURRENT_URL].label=(char*)"HVSC URL";
    settings[ONLINE_HVSC_CURRENT_URL].description=NULL;
    settings[ONLINE_HVSC_CURRENT_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_HVSC_CURRENT_URL].sub_family=0;
    settings[ONLINE_HVSC_CURRENT_URL].type=MDZ_MSGBOX;
    settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen("N/A")+1);
    strcpy(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text,"N/A");
    
    settings[ONLINE_ASMA_CURRENT_URL].label=(char*)"ASMA URL";
    settings[ONLINE_ASMA_CURRENT_URL].description=NULL;
    settings[ONLINE_ASMA_CURRENT_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_ASMA_CURRENT_URL].sub_family=0;
    settings[ONLINE_ASMA_CURRENT_URL].type=MDZ_MSGBOX;
    settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen("N/A")+1);
    strcpy(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text,"N/A");
    
    settings[ONLINE_MODLAND_URL].type=MDZ_SWITCH;
    settings[ONLINE_MODLAND_URL].label=(char*)"MODLAND Server";
    settings[ONLINE_MODLAND_URL].description=NULL;
    settings[ONLINE_MODLAND_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_MODLAND_URL].callback=&optONLINESwitchChanged;
    settings[ONLINE_MODLAND_URL].sub_family=0;
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_value=0;
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_value_nb=4;
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_labels=(char**)malloc(settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_labels[0]=(char*)"Default";
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_labels[1]=(char*)"Alt1";
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_labels[2]=(char*)"Alt2";
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_labels[3]=(char*)"Cust";
    
    settings[ONLINE_MODLAND_URL_CUSTOM].label=(char*)"Custom URL";
    settings[ONLINE_MODLAND_URL_CUSTOM].description=NULL;
    settings[ONLINE_MODLAND_URL_CUSTOM].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_MODLAND_URL_CUSTOM].callback=&optONLINESwitchChanged;
    settings[ONLINE_MODLAND_URL_CUSTOM].sub_family=0;
    settings[ONLINE_MODLAND_URL_CUSTOM].type=MDZ_TEXTBOX;
    settings[ONLINE_MODLAND_URL_CUSTOM].detail.mdz_textbox.text=NULL;
    
    settings[ONLINE_HVSC_URL].type=MDZ_SWITCH;
    settings[ONLINE_HVSC_URL].label=(char*)"HVSC Server";
    settings[ONLINE_HVSC_URL].description=NULL;
    settings[ONLINE_HVSC_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_HVSC_URL].callback=&optONLINESwitchChanged;
    settings[ONLINE_HVSC_URL].sub_family=0;
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_value=0;
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_value_nb=4;
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_labels=(char**)malloc(settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_labels[0]=(char*)"Default";
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_labels[1]=(char*)"Alt1";
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_labels[2]=(char*)"Alt2";
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_labels[3]=(char*)"Cust";
    
    settings[ONLINE_HVSC_URL_CUSTOM].label=(char*)"Custom URL";
    settings[ONLINE_HVSC_URL_CUSTOM].description=NULL;
    settings[ONLINE_HVSC_URL_CUSTOM].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_HVSC_URL_CUSTOM].sub_family=0;
    settings[ONLINE_HVSC_URL_CUSTOM].type=MDZ_TEXTBOX;
    settings[ONLINE_HVSC_URL_CUSTOM].detail.mdz_textbox.text=NULL;
    
    settings[ONLINE_ASMA_URL].type=MDZ_SWITCH;
    settings[ONLINE_ASMA_URL].label=(char*)"ASMA Server";
    settings[ONLINE_ASMA_URL].description=NULL;
    settings[ONLINE_ASMA_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_ASMA_URL].callback=&optONLINESwitchChanged;
    settings[ONLINE_ASMA_URL].sub_family=0;
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_value=0;
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_value_nb=4;
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_labels=(char**)malloc(settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_labels[0]=(char*)"Default";
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_labels[1]=(char*)"Alt1";
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_labels[2]=(char*)"Alt2";
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_labels[3]=(char*)"Cust";
    
    settings[ONLINE_ASMA_URL_CUSTOM].label=(char*)"Custom URL";
    settings[ONLINE_ASMA_URL_CUSTOM].description=NULL;
    settings[ONLINE_ASMA_URL_CUSTOM].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_ASMA_URL_CUSTOM].sub_family=0;
    settings[ONLINE_ASMA_URL_CUSTOM].type=MDZ_TEXTBOX;
    settings[ONLINE_ASMA_URL_CUSTOM].detail.mdz_textbox.text=NULL;
    
    [SettingsGenViewController ONLINEswitchChanged];
    
    /////////////////////////////////////
    //Visualizers
    /////////////////////////////////////
    settings[GLOB_FXRandom].label=(char*)"Random FX";
    settings[GLOB_FXRandom].description=NULL;
    settings[GLOB_FXRandom].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXRandom].sub_family=0;
    settings[GLOB_FXRandom].callback=&optVISUChangedC;
    settings[GLOB_FXRandom].type=MDZ_BOOLSWITCH;
    settings[GLOB_FXRandom].detail.mdz_boolswitch.switch_value=0;
    
    settings[GLOB_FXAlpha].label=(char*)"FX Transparency";
    settings[GLOB_FXAlpha].description=NULL;
    settings[GLOB_FXAlpha].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXAlpha].sub_family=0;
    settings[GLOB_FXRandom].callback=&optVISUChangedC;
    settings[GLOB_FXAlpha].type=MDZ_SLIDER_CONTINUOUS;
    settings[GLOB_FXAlpha].detail.mdz_slider.slider_value=0.7;
    settings[GLOB_FXAlpha].detail.mdz_slider.slider_min_value=0;
    settings[GLOB_FXAlpha].detail.mdz_slider.slider_max_value=1;
    
    settings[GLOB_FXBeat].label=(char*)"Beat FX";
    settings[GLOB_FXBeat].description=NULL;
    settings[GLOB_FXBeat].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXBeat].sub_family=0;
    settings[GLOB_FXBeat].type=MDZ_BOOLSWITCH;
    settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=0;
    
    settings[GLOB_FXOscillo].type=MDZ_SWITCH;
    settings[GLOB_FXOscillo].label=(char*)"Oscillo";
    settings[GLOB_FXOscillo].description=NULL;
    settings[GLOB_FXOscillo].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXOscillo].sub_family=0;
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXOscillo].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_labels[1]=(char*)"Split";
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_labels[2]=(char*)"Comb";
    
    settings[GLOB_FXSpectrum].type=MDZ_SWITCH;
    settings[GLOB_FXSpectrum].label=(char*)"2D Spectrum";
    settings[GLOB_FXSpectrum].description=NULL;
    settings[GLOB_FXSpectrum].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXSpectrum].sub_family=0;
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_labels[2]=(char*)"2";
    
    settings[GLOB_FXMODPattern].type=MDZ_SWITCH;
    settings[GLOB_FXMODPattern].label=(char*)"MOD Pattern";
    settings[GLOB_FXMODPattern].description=NULL;
    settings[GLOB_FXMODPattern].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMODPattern].sub_family=0;
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value_nb=7;
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[2]=(char*)"2";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[3]=(char*)"3";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[4]=(char*)"4";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[5]=(char*)"5";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[6]=(char*)"6";
    
    settings[GLOB_FXMIDIPattern].type=MDZ_SWITCH;
    settings[GLOB_FXMIDIPattern].label=(char*)"Note display";
    settings[GLOB_FXMIDIPattern].description=NULL;
    settings[GLOB_FXMIDIPattern].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMIDIPattern].sub_family=0;
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_labels[1]=(char*)"Hori";
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_labels[2]=(char*)"Vert";
    
    settings[GLOB_FXPiano].type=MDZ_SWITCH;
    settings[GLOB_FXPiano].label=(char*)"Piano mode";
    settings[GLOB_FXPiano].description=NULL;
    settings[GLOB_FXPiano].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXPiano].sub_family=0;
    settings[GLOB_FXPiano].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXPiano].detail.mdz_switch.switch_value_nb=5;
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXPiano].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels[2]=(char*)"2";
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels[3]=(char*)"3";
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels[4]=(char*)"4";
    
    settings[GLOB_FXPianoColorMode].type=MDZ_SWITCH;
    settings[GLOB_FXPianoColorMode].label=(char*)"Piano color mode";
    settings[GLOB_FXPianoColorMode].description=NULL;
    settings[GLOB_FXPianoColorMode].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXPianoColorMode].sub_family=0;
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value=1;
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_labels[0]=(char*)"Note";
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_labels[1]=(char*)"Instr";
    
    settings[GLOB_FX3DSpectrum].type=MDZ_SWITCH;
    settings[GLOB_FX3DSpectrum].label=(char*)"3D Spectrum";
    settings[GLOB_FX3DSpectrum].description=NULL;
    settings[GLOB_FX3DSpectrum].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX3DSpectrum].sub_family=0;
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=0;
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_labels[2]=(char*)"2";
    
    settings[GLOB_FX1].label=(char*)"FX1";
    settings[GLOB_FX1].description=NULL;
    settings[GLOB_FX1].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX1].sub_family=0;
    settings[GLOB_FX1].type=MDZ_BOOLSWITCH;
    settings[GLOB_FX1].detail.mdz_boolswitch.switch_value=0;
    
    settings[GLOB_FX2].type=MDZ_SWITCH;
    settings[GLOB_FX2].label=(char*)"FX2";
    settings[GLOB_FX2].description=NULL;
    settings[GLOB_FX2].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX2].sub_family=0;
    settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
    settings[GLOB_FX2].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FX2].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FX2].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FX2].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FX2].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FX2].detail.mdz_switch.switch_labels[2]=(char*)"2";
    
    settings[GLOB_FX3].type=MDZ_SWITCH;
    settings[GLOB_FX3].label=(char*)"FX3";
    settings[GLOB_FX3].description=NULL;
    settings[GLOB_FX3].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX3].sub_family=0;
    settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
    settings[GLOB_FX3].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FX3].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FX3].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FX3].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FX3].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FX3].detail.mdz_switch.switch_labels[2]=(char*)"2";
    
    settings[GLOB_FX4].label=(char*)"FX4";
    settings[GLOB_FX4].description=NULL;
    settings[GLOB_FX4].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX4].sub_family=0;
    settings[GLOB_FX4].type=MDZ_BOOLSWITCH;
    settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
    
    settings[GLOB_FX5].type=MDZ_SWITCH;
    settings[GLOB_FX5].label=(char*)"FX5";
    settings[GLOB_FX5].description=NULL;
    settings[GLOB_FX5].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX5].sub_family=0;
    settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
    settings[GLOB_FX5].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FX5].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FX5].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FX5].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FX5].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FX5].detail.mdz_switch.switch_labels[2]=(char*)"2";
    
    settings[GLOB_FXLOD].type=MDZ_SWITCH;
    settings[GLOB_FXLOD].label=(char*)"FX Level of details";
    settings[GLOB_FXLOD].description=NULL;
    settings[GLOB_FXLOD].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXLOD].sub_family=0;
    settings[GLOB_FXLOD].detail.mdz_switch.switch_value=2;
    settings[GLOB_FXLOD].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXLOD].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXLOD].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXLOD].detail.mdz_switch.switch_labels[0]=(char*)"Low";
    settings[GLOB_FXLOD].detail.mdz_switch.switch_labels[1]=(char*)"Med";
    settings[GLOB_FXLOD].detail.mdz_switch.switch_labels[2]=(char*)"High";
    
    settings[GLOB_FXMSAA].label=(char*)"MSAA";
    settings[GLOB_FXMSAA].description=NULL;
    settings[GLOB_FXMSAA].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMSAA].sub_family=0;
    settings[GLOB_FXMSAA].type=MDZ_BOOLSWITCH;
    settings[GLOB_FXMSAA].detail.mdz_boolswitch.switch_value=0;
    
    settings[GLOB_FXFPS].type=MDZ_SWITCH;
    settings[GLOB_FXFPS].label=(char*)"FX FPS";
    settings[GLOB_FXFPS].description=NULL;
    settings[GLOB_FXFPS].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXFPS].sub_family=0;
    settings[GLOB_FXFPS].detail.mdz_switch.switch_value=1;
    settings[GLOB_FXFPS].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_FXFPS].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXFPS].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXFPS].detail.mdz_switch.switch_labels[0]=(char*)"30";
    settings[GLOB_FXFPS].detail.mdz_switch.switch_labels[1]=(char*)"60";
    
    
    /////////////////////////////////////
    //PLUGINS
    /////////////////////////////////////
    
    /////////////////////////////////////
    //MODPLUG
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_MODPLUG].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_MODPLUG].label=(char*)"OpenMPT";
    settings[MDZ_SETTINGS_FAMILY_MODPLUG].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_MODPLUG].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_MODPLUG].sub_family=MDZ_SETTINGS_FAMILY_MODPLUG;
    
    settings[MODPLUG_MasterVolume].label=(char*)"Master Volume";
    settings[MODPLUG_MasterVolume].description=NULL;
    settings[MODPLUG_MasterVolume].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_MasterVolume].sub_family=0;
    settings[MODPLUG_MasterVolume].callback=&optMODPLUGChangedC;
    settings[MODPLUG_MasterVolume].type=MDZ_SLIDER_CONTINUOUS;
    settings[MODPLUG_MasterVolume].detail.mdz_slider.slider_value=0.5;
    settings[MODPLUG_MasterVolume].detail.mdz_slider.slider_min_value=0;
    settings[MODPLUG_MasterVolume].detail.mdz_slider.slider_max_value=1;
    
    settings[MODPLUG_Sampling].type=MDZ_SWITCH;
    settings[MODPLUG_Sampling].label=(char*)"Resampling";
    settings[MODPLUG_Sampling].description=NULL;
    settings[MODPLUG_Sampling].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_Sampling].sub_family=0;
    settings[MODPLUG_Sampling].callback=&optMODPLUGChangedC;
    settings[MODPLUG_Sampling].detail.mdz_switch.switch_value=2;
    settings[MODPLUG_Sampling].detail.mdz_switch.switch_value_nb=4;
    settings[MODPLUG_Sampling].detail.mdz_switch.switch_labels=(char**)malloc(settings[MODPLUG_Sampling].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[MODPLUG_Sampling].detail.mdz_switch.switch_labels[0]=(char*)"Near";
    settings[MODPLUG_Sampling].detail.mdz_switch.switch_labels[1]=(char*)"Lin";
    settings[MODPLUG_Sampling].detail.mdz_switch.switch_labels[2]=(char*)"Cub";
    settings[MODPLUG_Sampling].detail.mdz_switch.switch_labels[3]=(char*)"Win";
    
/*    settings[MODPLUG_Megabass].type=MDZ_BOOLSWITCH;
    settings[MODPLUG_Megabass].label=(char*)"Megabass";
    settings[MODPLUG_Megabass].description=NULL;
    settings[MODPLUG_Megabass].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_Megabass].sub_family=0;
    settings[MODPLUG_Megabass].callback=&optMODPLUGChangedC;
    settings[MODPLUG_Megabass].detail.mdz_boolswitch.switch_value=0;
    
    settings[MODPLUG_BassAmount].label=(char*)"Megabass Strength";
    settings[MODPLUG_BassAmount].description=NULL;
    settings[MODPLUG_BassAmount].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_BassAmount].sub_family=0;
    settings[MODPLUG_BassAmount].callback=&optMODPLUGChangedC;
    settings[MODPLUG_BassAmount].type=MDZ_SLIDER_CONTINUOUS;
    settings[MODPLUG_BassAmount].detail.mdz_slider.slider_value=0.7;
    settings[MODPLUG_BassAmount].detail.mdz_slider.slider_min_value=0;
    settings[MODPLUG_BassAmount].detail.mdz_slider.slider_max_value=1;
    
    settings[MODPLUG_BassRange].label=(char*)"Megabass Range";
    settings[MODPLUG_BassRange].description=NULL;
    settings[MODPLUG_BassRange].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_BassRange].sub_family=0;
    settings[MODPLUG_BassRange].callback=&optMODPLUGChangedC;
    settings[MODPLUG_BassRange].type=MDZ_SLIDER_CONTINUOUS;
    settings[MODPLUG_BassRange].detail.mdz_slider.slider_value=0.3;
    settings[MODPLUG_BassRange].detail.mdz_slider.slider_min_value=0;
    settings[MODPLUG_BassRange].detail.mdz_slider.slider_max_value=1;
    
    settings[MODPLUG_Surround].type=MDZ_BOOLSWITCH;
    settings[MODPLUG_Surround].label=(char*)"Surround";
    settings[MODPLUG_Surround].description=NULL;
    settings[MODPLUG_Surround].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_Surround].sub_family=0;
    settings[MODPLUG_Surround].callback=&optMODPLUGChangedC;
    settings[MODPLUG_Surround].detail.mdz_boolswitch.switch_value=0;
    
    settings[MODPLUG_SurroundDepth].label=(char*)"Surround Depth";
    settings[MODPLUG_SurroundDepth].description=NULL;
    settings[MODPLUG_SurroundDepth].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_SurroundDepth].sub_family=0;
    settings[MODPLUG_SurroundDepth].callback=&optMODPLUGChangedC;
    settings[MODPLUG_SurroundDepth].type=MDZ_SLIDER_CONTINUOUS;
    settings[MODPLUG_SurroundDepth].detail.mdz_slider.slider_value=0.9;
    settings[MODPLUG_SurroundDepth].detail.mdz_slider.slider_min_value=0;
    settings[MODPLUG_SurroundDepth].detail.mdz_slider.slider_max_value=1;
    
    settings[MODPLUG_SurroundDelay].label=(char*)"Surround Delay";
    settings[MODPLUG_SurroundDelay].description=NULL;
    settings[MODPLUG_SurroundDelay].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_SurroundDelay].sub_family=0;
    settings[MODPLUG_SurroundDelay].callback=&optMODPLUGChangedC;
    settings[MODPLUG_SurroundDelay].type=MDZ_SLIDER_CONTINUOUS;
    settings[MODPLUG_SurroundDelay].detail.mdz_slider.slider_value=0.8;
    settings[MODPLUG_SurroundDelay].detail.mdz_slider.slider_min_value=0;
    settings[MODPLUG_SurroundDelay].detail.mdz_slider.slider_max_value=1;
    
    settings[MODPLUG_Reverb].type=MDZ_BOOLSWITCH;
    settings[MODPLUG_Reverb].label=(char*)"Reverb";
    settings[MODPLUG_Reverb].description=NULL;
    settings[MODPLUG_Reverb].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_Reverb].sub_family=0;
    settings[MODPLUG_Reverb].callback=&optMODPLUGChangedC;
    settings[MODPLUG_Reverb].detail.mdz_boolswitch.switch_value=0;
    
    settings[MODPLUG_ReverbDepth].label=(char*)"Reverb Depth";
    settings[MODPLUG_ReverbDepth].description=NULL;
    settings[MODPLUG_ReverbDepth].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_ReverbDepth].sub_family=0;
    settings[MODPLUG_ReverbDepth].callback=&optMODPLUGChangedC;
    settings[MODPLUG_ReverbDepth].type=MDZ_SLIDER_CONTINUOUS;
    settings[MODPLUG_ReverbDepth].detail.mdz_slider.slider_value=0.8;
    settings[MODPLUG_ReverbDepth].detail.mdz_slider.slider_min_value=0;
    settings[MODPLUG_ReverbDepth].detail.mdz_slider.slider_max_value=1;
    
    settings[MODPLUG_ReverbDelay].label=(char*)"Reverb Delay";
    settings[MODPLUG_ReverbDelay].description=NULL;
    settings[MODPLUG_ReverbDelay].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_ReverbDelay].sub_family=0;
    settings[MODPLUG_ReverbDelay].callback=&optMODPLUGChangedC;
    settings[MODPLUG_ReverbDelay].type=MDZ_SLIDER_CONTINUOUS;
    settings[MODPLUG_ReverbDelay].detail.mdz_slider.slider_value=0.7;
    settings[MODPLUG_ReverbDelay].detail.mdz_slider.slider_min_value=0;
    settings[MODPLUG_ReverbDelay].detail.mdz_slider.slider_max_value=1;*/
    
    settings[MODPLUG_StereoSeparation].label=(char*)"Panning";
    settings[MODPLUG_StereoSeparation].description=NULL;
    settings[MODPLUG_StereoSeparation].family=MDZ_SETTINGS_FAMILY_MODPLUG;
    settings[MODPLUG_StereoSeparation].sub_family=0;
    settings[MODPLUG_StereoSeparation].callback=&optMODPLUGChangedC;
    settings[MODPLUG_StereoSeparation].type=MDZ_SLIDER_CONTINUOUS;
    settings[MODPLUG_StereoSeparation].detail.mdz_slider.slider_value=0.5;
    settings[MODPLUG_StereoSeparation].detail.mdz_slider.slider_min_value=0;
    settings[MODPLUG_StereoSeparation].detail.mdz_slider.slider_max_value=1;
    
    
    /////////////////////////////////////
    //DUMB
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_DUMB].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_DUMB].label=(char*)"Dumb";
    settings[MDZ_SETTINGS_FAMILY_DUMB].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_DUMB].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_DUMB].sub_family=MDZ_SETTINGS_FAMILY_DUMB;
    
    settings[DUMB_MasterVolume].label=(char*)"Master Volume";
    settings[DUMB_MasterVolume].description=NULL;
    settings[DUMB_MasterVolume].family=MDZ_SETTINGS_FAMILY_DUMB;
    settings[DUMB_MasterVolume].sub_family=0;
    settings[DUMB_MasterVolume].callback=&optDUMBChangedC;
    settings[DUMB_MasterVolume].type=MDZ_SLIDER_CONTINUOUS;
    settings[DUMB_MasterVolume].detail.mdz_slider.slider_value=1.0;
    settings[DUMB_MasterVolume].detail.mdz_slider.slider_min_value=0;
    settings[DUMB_MasterVolume].detail.mdz_slider.slider_max_value=1;
    
    settings[DUMB_Resampling].type=MDZ_SWITCH;
    settings[DUMB_Resampling].label=(char*)"Resampling";
    settings[DUMB_Resampling].description=NULL;
    settings[DUMB_Resampling].family=MDZ_SETTINGS_FAMILY_DUMB;
    settings[DUMB_Resampling].sub_family=0;
    settings[DUMB_Resampling].callback=&optDUMBChangedC;
    settings[DUMB_Resampling].detail.mdz_switch.switch_value=2;
    settings[DUMB_Resampling].detail.mdz_switch.switch_value_nb=4;
    settings[DUMB_Resampling].detail.mdz_switch.switch_labels=(char**)malloc(settings[DUMB_Resampling].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[DUMB_Resampling].detail.mdz_switch.switch_labels[0]=(char*)"Alias";
    settings[DUMB_Resampling].detail.mdz_switch.switch_labels[1]=(char*)"Lin";
    settings[DUMB_Resampling].detail.mdz_switch.switch_labels[2]=(char*)"Cubic";
    settings[DUMB_Resampling].detail.mdz_switch.switch_labels[3]=(char*)"FIR";
    
    /////////////////////////////////////
    //GME
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_GME].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GME].label=(char*)"GME";
    settings[MDZ_SETTINGS_FAMILY_GME].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GME].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_GME].sub_family=MDZ_SETTINGS_FAMILY_GME;
    
    settings[GME_FADEOUT].label=(char*)"Fade out";
    settings[GME_FADEOUT].description=NULL;
    settings[GME_FADEOUT].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_FADEOUT].sub_family=0;
    settings[GME_FADEOUT].callback=&optGMEChangedC;
    settings[GME_FADEOUT].type=MDZ_SLIDER_CONTINUOUS;
    settings[GME_FADEOUT].detail.mdz_slider.slider_value=1;
    settings[GME_FADEOUT].detail.mdz_slider.slider_min_value=0;
    settings[GME_FADEOUT].detail.mdz_slider.slider_max_value=5;
    
    settings[GME_RATIO_ONOFF].type=MDZ_BOOLSWITCH;
    settings[GME_RATIO_ONOFF].label=(char*)"Enable Playback Ratio";
    settings[GME_RATIO_ONOFF].description=NULL;
    settings[GME_RATIO_ONOFF].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_RATIO_ONOFF].sub_family=0;
    settings[GME_RATIO_ONOFF].callback=&optGMEChangedC;
    settings[GME_RATIO_ONOFF].detail.mdz_boolswitch.switch_value=0;
    
    settings[GME_IGNORESILENCE].type=MDZ_BOOLSWITCH;
    settings[GME_IGNORESILENCE].label=(char*)"Ignore Silence";
    settings[GME_IGNORESILENCE].description=NULL;
    settings[GME_IGNORESILENCE].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_IGNORESILENCE].sub_family=0;
    settings[GME_IGNORESILENCE].callback=&optGMEChangedC;
    settings[GME_IGNORESILENCE].detail.mdz_boolswitch.switch_value=0;
    
    settings[GME_RATIO].label=(char*)"Playback Ratio";
    settings[GME_RATIO].description=NULL;
    settings[GME_RATIO].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_RATIO].sub_family=0;
    settings[GME_RATIO].callback=&optGMEChangedC;
    settings[GME_RATIO].type=MDZ_SLIDER_CONTINUOUS;
    settings[GME_RATIO].detail.mdz_slider.slider_value=1;
    settings[GME_RATIO].detail.mdz_slider.slider_min_value=0.1;
    settings[GME_RATIO].detail.mdz_slider.slider_max_value=5;
    
    settings[GME_EQ_BASS].label=(char*)"Bass";
    settings[GME_EQ_BASS].description=NULL;
    settings[GME_EQ_BASS].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_EQ_BASS].sub_family=0;
    settings[GME_EQ_BASS].callback=&optGMEChangedC;
    settings[GME_EQ_BASS].type=MDZ_SLIDER_CONTINUOUS;
    settings[GME_EQ_BASS].detail.mdz_slider.slider_value=4.2-1.9;
    settings[GME_EQ_BASS].detail.mdz_slider.slider_min_value=0;
    settings[GME_EQ_BASS].detail.mdz_slider.slider_max_value=4.2;
    
    settings[GME_EQ_TREBLE].label=(char*)"Treble";
    settings[GME_EQ_TREBLE].description=NULL;
    settings[GME_EQ_TREBLE].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_EQ_TREBLE].sub_family=0;
    settings[GME_EQ_TREBLE].callback=&optGMEChangedC;
    settings[GME_EQ_TREBLE].type=MDZ_SLIDER_CONTINUOUS;
    settings[GME_EQ_TREBLE].detail.mdz_slider.slider_value=-14;
    settings[GME_EQ_TREBLE].detail.mdz_slider.slider_min_value=-50;
    settings[GME_EQ_TREBLE].detail.mdz_slider.slider_max_value=5;
    
    settings[GME_FX_ONOFF].type=MDZ_BOOLSWITCH;
    settings[GME_FX_ONOFF].label=(char*)"Post FX";
    settings[GME_FX_ONOFF].description=NULL;
    settings[GME_FX_ONOFF].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_FX_ONOFF].sub_family=0;
    settings[GME_FX_ONOFF].callback=&optGMEChangedC;
    settings[GME_FX_ONOFF].detail.mdz_boolswitch.switch_value=0;
    
    settings[GME_FX_SURROUND].type=MDZ_BOOLSWITCH;
    settings[GME_FX_SURROUND].label=(char*)"Surround";
    settings[GME_FX_SURROUND].description=NULL;
    settings[GME_FX_SURROUND].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_FX_SURROUND].sub_family=0;
    settings[GME_FX_SURROUND].callback=&optGMEChangedC;
    settings[GME_FX_SURROUND].detail.mdz_boolswitch.switch_value=0;
    
    settings[GME_FX_ECHO].type=MDZ_BOOLSWITCH;
    settings[GME_FX_ECHO].label=(char*)"Echo";
    settings[GME_FX_ECHO].description=NULL;
    settings[GME_FX_ECHO].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_FX_ECHO].sub_family=0;
    settings[GME_FX_ECHO].callback=&optGMEChangedC;
    settings[GME_FX_ECHO].detail.mdz_boolswitch.switch_value=0;
    
    settings[GME_FX_PANNING].label=(char*)"Panning";
    settings[GME_FX_PANNING].description=NULL;
    settings[GME_FX_PANNING].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_FX_PANNING].sub_family=0;
    settings[GME_FX_PANNING].callback=&optGMEChangedC;
    settings[GME_FX_PANNING].type=MDZ_SLIDER_CONTINUOUS;
    settings[GME_FX_PANNING].detail.mdz_slider.slider_value=0.5;
    settings[GME_FX_PANNING].detail.mdz_slider.slider_min_value=0;
    settings[GME_FX_PANNING].detail.mdz_slider.slider_max_value=1;
    
    /////////////////////////////////////
    //GSF
    /////////////////////////////////////
    
    settings[MDZ_SETTINGS_FAMILY_GSF].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GSF].label=(char*)"GSF";
    settings[MDZ_SETTINGS_FAMILY_GSF].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GSF].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_GSF].sub_family=MDZ_SETTINGS_FAMILY_GSF;
    
    settings[GSF_SOUNDQUALITY].type=MDZ_SWITCH;
    settings[GSF_SOUNDQUALITY].label=(char*)"Sound Quality";
    settings[GSF_SOUNDQUALITY].description=NULL;
    settings[GSF_SOUNDQUALITY].family=MDZ_SETTINGS_FAMILY_GSF;
    settings[GSF_SOUNDQUALITY].sub_family=0;
    settings[GSF_SOUNDQUALITY].callback=&optGSFChangedC;
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_value=2;
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_value_nb=3;
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_labels=(char**)malloc(settings[DUMB_Resampling].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_labels[0]=(char*)"11Khz";
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_labels[1]=(char*)"22Khz";
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_labels[2]=(char*)"44Khz";
    
    settings[GSF_INTERPOLATION].type=MDZ_BOOLSWITCH;
    settings[GSF_INTERPOLATION].label=(char*)"Interpolation";
    settings[GSF_INTERPOLATION].description=NULL;
    settings[GSF_INTERPOLATION].family=MDZ_SETTINGS_FAMILY_GSF;
    settings[GSF_INTERPOLATION].sub_family=0;
    settings[GSF_INTERPOLATION].callback=&optGSFChangedC;
    settings[GSF_INTERPOLATION].detail.mdz_boolswitch.switch_value=1;
    
    settings[GSF_LOWPASSFILTER].type=MDZ_BOOLSWITCH;
    settings[GSF_LOWPASSFILTER].label=(char*)"Lowpass Filter";
    settings[GSF_LOWPASSFILTER].description=NULL;
    settings[GSF_LOWPASSFILTER].family=MDZ_SETTINGS_FAMILY_GSF;
    settings[GSF_LOWPASSFILTER].sub_family=0;
    settings[GSF_LOWPASSFILTER].callback=&optGSFChangedC;
    settings[GSF_LOWPASSFILTER].detail.mdz_boolswitch.switch_value=1;
    
    settings[GSF_ECHO].type=MDZ_BOOLSWITCH;
    settings[GSF_ECHO].label=(char*)"Echo";
    settings[GSF_ECHO].description=NULL;
    settings[GSF_ECHO].family=MDZ_SETTINGS_FAMILY_GSF;
    settings[GSF_ECHO].sub_family=0;
    settings[GSF_ECHO].callback=&optGSFChangedC;
    settings[GSF_ECHO].detail.mdz_boolswitch.switch_value=0;
    
    
    /////////////////////////////////////
    //TIMIDITY
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].label=(char*)"Timidity";
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].sub_family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    
    settings[TIM_Polyphony].label=(char*)"Midi polyphony";
    settings[TIM_Polyphony].description=NULL;
    settings[TIM_Polyphony].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Polyphony].sub_family=0;
    settings[TIM_Polyphony].callback=&optTIMIDITYChangedC;
    settings[TIM_Polyphony].type=MDZ_SLIDER_DISCRETE;
    settings[TIM_Polyphony].detail.mdz_slider.slider_value=128;
    settings[TIM_Polyphony].detail.mdz_slider.slider_min_value=64;
    settings[TIM_Polyphony].detail.mdz_slider.slider_max_value=256;
    
    settings[TIM_Amplification].label=(char*)"Amplification";
    settings[TIM_Amplification].description=NULL;
    settings[TIM_Amplification].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Amplification].sub_family=0;
    settings[TIM_Amplification].callback=&optTIMIDITYChangedC;
    settings[TIM_Amplification].type=MDZ_SLIDER_DISCRETE;
    settings[TIM_Amplification].detail.mdz_slider.slider_value=100;
    settings[TIM_Amplification].detail.mdz_slider.slider_min_value=10;
    settings[TIM_Amplification].detail.mdz_slider.slider_max_value=400;
    
    settings[TIM_Chorus].type=MDZ_BOOLSWITCH;
    settings[TIM_Chorus].label=(char*)"Chorus";
    settings[TIM_Chorus].description=NULL;
    settings[TIM_Chorus].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Chorus].sub_family=0;
    settings[TIM_Chorus].callback=&optTIMIDITYChangedC;
    settings[TIM_Chorus].detail.mdz_boolswitch.switch_value=1;
    
    settings[TIM_Reverb].type=MDZ_BOOLSWITCH;
    settings[TIM_Reverb].label=(char*)"Reverb";
    settings[TIM_Reverb].description=NULL;
    settings[TIM_Reverb].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Reverb].sub_family=0;
    settings[TIM_Reverb].callback=&optTIMIDITYChangedC;
    settings[TIM_Reverb].detail.mdz_boolswitch.switch_value=1;
    
    settings[TIM_LPFilter].type=MDZ_BOOLSWITCH;
    settings[TIM_LPFilter].label=(char*)"LPFilter";
    settings[TIM_LPFilter].description=NULL;
    settings[TIM_LPFilter].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_LPFilter].sub_family=0;
    settings[TIM_LPFilter].callback=&optTIMIDITYChangedC;
    settings[TIM_LPFilter].detail.mdz_boolswitch.switch_value=1;
    
    settings[TIM_Resample].type=MDZ_SWITCH;
    settings[TIM_Resample].label=(char*)"Resampling";
    settings[TIM_Resample].description=NULL;
    settings[TIM_Resample].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Resample].sub_family=0;
    settings[TIM_Resample].callback=&optTIMIDITYChangedC;
    settings[TIM_Resample].detail.mdz_switch.switch_value=1;
    settings[TIM_Resample].detail.mdz_switch.switch_value_nb=5;
    settings[TIM_Resample].detail.mdz_switch.switch_labels=(char**)malloc(settings[TIM_Resample].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[TIM_Resample].detail.mdz_switch.switch_labels[0]=(char*)"None";
    settings[TIM_Resample].detail.mdz_switch.switch_labels[1]=(char*)"Line";
    settings[TIM_Resample].detail.mdz_switch.switch_labels[2]=(char*)"Spli";
    settings[TIM_Resample].detail.mdz_switch.switch_labels[3]=(char*)"Gaus";
    settings[TIM_Resample].detail.mdz_switch.switch_labels[4]=(char*)"Newt";
    
    /////////////////////////////////////
    //VGMPLAY
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_VGMPLAY].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_VGMPLAY].label=(char*)"VGMPlay";
    settings[MDZ_SETTINGS_FAMILY_VGMPLAY].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_VGMPLAY].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_VGMPLAY].sub_family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    
    
    settings[VGMPLAY_Maxloop].label=(char*)"Max loop";
    settings[VGMPLAY_Maxloop].description=NULL;
    settings[VGMPLAY_Maxloop].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_Maxloop].sub_family=0;
    settings[VGMPLAY_Maxloop].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_Maxloop].type=MDZ_SLIDER_DISCRETE;
    settings[VGMPLAY_Maxloop].detail.mdz_slider.slider_value=2;
    settings[VGMPLAY_Maxloop].detail.mdz_slider.slider_min_value=0;
    settings[VGMPLAY_Maxloop].detail.mdz_slider.slider_max_value=16;
    
    settings[VGMPLAY_YM2612Emulator].type=MDZ_SWITCH;
    settings[VGMPLAY_YM2612Emulator].label=(char*)"YM2612 Type";
    settings[VGMPLAY_YM2612Emulator].description=NULL;
    settings[VGMPLAY_YM2612Emulator].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_YM2612Emulator].sub_family=0;
    settings[VGMPLAY_YM2612Emulator].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_value=0;
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_value_nb=3;
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_labels=(char**)malloc(settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_labels[0]=(char*)"MAME";
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_labels[1]=(char*)"Nuked OPN2";
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_labels[2]=(char*)"Gens";
    
    settings[VGMPLAY_PreferJTAG].type=MDZ_BOOLSWITCH;
    settings[VGMPLAY_PreferJTAG].label=(char*)"Japanese Tag";
    settings[VGMPLAY_PreferJTAG].description=NULL;
    settings[VGMPLAY_PreferJTAG].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_PreferJTAG].sub_family=0;
    settings[VGMPLAY_PreferJTAG].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value=0;
    
    
    /////////////////////////////////////
    //VGMSTREAM
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_VGMSTREAM].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_VGMSTREAM].label=(char*)"VGMStream";
    settings[MDZ_SETTINGS_FAMILY_VGMSTREAM].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_VGMSTREAM].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_VGMSTREAM].sub_family=MDZ_SETTINGS_FAMILY_VGMSTREAM;
    
    settings[VGMSTREAM_Forceloop].type=MDZ_BOOLSWITCH;
    settings[VGMSTREAM_Forceloop].label=(char*)"Force loop";
    settings[VGMSTREAM_Forceloop].description=NULL;
    settings[VGMSTREAM_Forceloop].family=MDZ_SETTINGS_FAMILY_VGMSTREAM;
    settings[VGMSTREAM_Forceloop].sub_family=0;
    settings[VGMSTREAM_Forceloop].callback=&optVGMSTREAMChangedC;
    settings[VGMSTREAM_Forceloop].detail.mdz_boolswitch.switch_value=0;
    
    
    settings[VGMSTREAM_Maxloop].label=(char*)"Max loop";
    settings[VGMSTREAM_Maxloop].description=NULL;
    settings[VGMSTREAM_Maxloop].family=MDZ_SETTINGS_FAMILY_VGMSTREAM;
    settings[VGMSTREAM_Maxloop].sub_family=0;
    settings[VGMSTREAM_Maxloop].callback=&optVGMSTREAMChangedC;
    settings[VGMSTREAM_Maxloop].type=MDZ_SLIDER_DISCRETE;
    settings[VGMSTREAM_Maxloop].detail.mdz_slider.slider_value=2;
    settings[VGMSTREAM_Maxloop].detail.mdz_slider.slider_min_value=1;
    settings[VGMSTREAM_Maxloop].detail.mdz_slider.slider_max_value=32;
    
    settings[VGMSTREAM_ResampleQuality].type=MDZ_SWITCH;
    settings[VGMSTREAM_ResampleQuality].label=(char*)"Resampling";
    settings[VGMSTREAM_ResampleQuality].description=NULL;
    settings[VGMSTREAM_ResampleQuality].family=MDZ_SETTINGS_FAMILY_VGMSTREAM;
    settings[VGMSTREAM_ResampleQuality].sub_family=0;
    settings[VGMSTREAM_ResampleQuality].callback=&optVGMSTREAMChangedC;
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_value=1;
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_value_nb=5;
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels=(char**)malloc(settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels[0]=(char*)"Best";
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels[1]=(char*)"Med.";
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels[2]=(char*)"Fast";
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels[3]=(char*)"ZOH";
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels[4]=(char*)"Lin.";
    
    
    /////////////////////////////////////
    //LAZYUSF
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_LAZYUSF].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_LAZYUSF].label=(char*)"LAZYUSF";
    settings[MDZ_SETTINGS_FAMILY_LAZYUSF].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_LAZYUSF].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_LAZYUSF].sub_family=MDZ_SETTINGS_FAMILY_LAZYUSF;

    settings[LAZYUSF_ResampleQuality].type=MDZ_SWITCH;
    settings[LAZYUSF_ResampleQuality].label=(char*)"Resampling";
    settings[LAZYUSF_ResampleQuality].description=NULL;
    settings[LAZYUSF_ResampleQuality].family=MDZ_SETTINGS_FAMILY_LAZYUSF;
    settings[LAZYUSF_ResampleQuality].sub_family=0;
    settings[LAZYUSF_ResampleQuality].callback=&optLAZYUSFChangedC;
    settings[LAZYUSF_ResampleQuality].detail.mdz_switch.switch_value=1;
    settings[LAZYUSF_ResampleQuality].detail.mdz_switch.switch_value_nb=5;
    settings[LAZYUSF_ResampleQuality].detail.mdz_switch.switch_labels=(char**)malloc(settings[LAZYUSF_ResampleQuality].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[LAZYUSF_ResampleQuality].detail.mdz_switch.switch_labels[0]=(char*)"Best";
    settings[LAZYUSF_ResampleQuality].detail.mdz_switch.switch_labels[1]=(char*)"Med.";
    settings[LAZYUSF_ResampleQuality].detail.mdz_switch.switch_labels[2]=(char*)"Fast";
    settings[LAZYUSF_ResampleQuality].detail.mdz_switch.switch_labels[3]=(char*)"ZOH";
    settings[LAZYUSF_ResampleQuality].detail.mdz_switch.switch_labels[4]=(char*)"Lin.";
    
    
    /////////////////////////////////////
    //GME
    /////////////////////////////////////
    
    /////////////////////////////////////
    //SID
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_SID].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_SID].label=(char*)"SID";
    settings[MDZ_SETTINGS_FAMILY_SID].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_SID].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_SID].sub_family=MDZ_SETTINGS_FAMILY_SID;
    
    settings[SID_Filter].type=MDZ_BOOLSWITCH;
    settings[SID_Filter].label=(char*)"Filter";
    settings[SID_Filter].description=NULL;
    settings[SID_Filter].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_Filter].sub_family=0;
    settings[SID_Filter].callback=&optSIDChangedC;
    settings[SID_Filter].detail.mdz_boolswitch.switch_value=1;
    
    
    settings[SID_CLOCK].type=MDZ_SWITCH;
    settings[SID_CLOCK].label=(char*)"CLOCK";
    settings[SID_CLOCK].description=NULL;
    settings[SID_CLOCK].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_CLOCK].sub_family=0;
    settings[SID_CLOCK].callback=&optSIDChangedC;
    settings[SID_CLOCK].detail.mdz_switch.switch_value=0;
    settings[SID_CLOCK].detail.mdz_switch.switch_value_nb=3;
    settings[SID_CLOCK].detail.mdz_switch.switch_labels=(char**)malloc(settings[SID_CLOCK].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[SID_CLOCK].detail.mdz_switch.switch_labels[0]=(char*)"Auto";
    settings[SID_CLOCK].detail.mdz_switch.switch_labels[1]=(char*)"PAL";
    settings[SID_CLOCK].detail.mdz_switch.switch_labels[2]=(char*)"NTSC";
    
    settings[SID_MODEL].type=MDZ_SWITCH;
    settings[SID_MODEL].label=(char*)"Model";
    settings[SID_MODEL].description=NULL;
    settings[SID_MODEL].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_MODEL].sub_family=0;
    settings[SID_MODEL].callback=&optSIDChangedC;
    settings[SID_MODEL].detail.mdz_switch.switch_value=0;
    settings[SID_MODEL].detail.mdz_switch.switch_value_nb=3;
    settings[SID_MODEL].detail.mdz_switch.switch_labels=(char**)malloc(settings[SID_MODEL].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[SID_MODEL].detail.mdz_switch.switch_labels[0]=(char*)"Auto";
    settings[SID_MODEL].detail.mdz_switch.switch_labels[1]=(char*)"6581";
    settings[SID_MODEL].detail.mdz_switch.switch_labels[2]=(char*)"8580";
    
    
    /////////////////////////////////////
    //UADE
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_UADE].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_UADE].label=(char*)"UADE";
    settings[MDZ_SETTINGS_FAMILY_UADE].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_UADE].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_UADE].sub_family=MDZ_SETTINGS_FAMILY_UADE;
    
    settings[UADE_Head].type=MDZ_BOOLSWITCH;
    settings[UADE_Head].label=(char*)"Headphones";
    settings[UADE_Head].description=NULL;
    settings[UADE_Head].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_Head].sub_family=0;
    settings[UADE_Head].callback=&optUADEChangedC;
    settings[UADE_Head].detail.mdz_boolswitch.switch_value=0;
    
    settings[UADE_PostFX].type=MDZ_BOOLSWITCH;
    settings[UADE_PostFX].label=(char*)"Post FX";
    settings[UADE_PostFX].description=NULL;
    settings[UADE_PostFX].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_PostFX].sub_family=0;
    settings[UADE_PostFX].callback=&optUADEChangedC;
    settings[UADE_PostFX].detail.mdz_boolswitch.switch_value=1;
    
    
    settings[UADE_Led].type=MDZ_BOOLSWITCH;
    settings[UADE_Led].label=(char*)"LED";
    settings[UADE_Led].description=NULL;
    settings[UADE_Led].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_Led].sub_family=0;
    settings[UADE_Led].callback=&optUADEChangedC;
    settings[UADE_Led].detail.mdz_boolswitch.switch_value=0;
    
    settings[UADE_Norm].type=MDZ_BOOLSWITCH;
    settings[UADE_Norm].label=(char*)"Normalization";
    settings[UADE_Norm].description=NULL;
    settings[UADE_Norm].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_Norm].sub_family=0;
    settings[UADE_Norm].callback=&optUADEChangedC;
    settings[UADE_Norm].detail.mdz_boolswitch.switch_value=0;
    
    settings[UADE_Gain].type=MDZ_BOOLSWITCH;
    settings[UADE_Gain].label=(char*)"Gain";
    settings[UADE_Gain].description=NULL;
    settings[UADE_Gain].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_Gain].sub_family=0;
    settings[UADE_Gain].callback=&optUADEChangedC;
    settings[UADE_Gain].detail.mdz_boolswitch.switch_value=0;
    
    settings[UADE_GainValue].label=(char*)"Gain Value";
    settings[UADE_GainValue].description=NULL;
    settings[UADE_GainValue].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_GainValue].sub_family=0;
    settings[UADE_GainValue].callback=&optUADEChangedC;
    settings[UADE_GainValue].type=MDZ_SLIDER_CONTINUOUS;
    settings[UADE_GainValue].detail.mdz_slider.slider_value=0.5;
    settings[UADE_GainValue].detail.mdz_slider.slider_min_value=0;
    settings[UADE_GainValue].detail.mdz_slider.slider_max_value=1;
    
    settings[UADE_Pan].type=MDZ_BOOLSWITCH;
    settings[UADE_Pan].label=(char*)"Panning";
    settings[UADE_Pan].description=NULL;
    settings[UADE_Pan].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_Pan].sub_family=0;
    settings[UADE_Pan].callback=&optUADEChangedC;
    settings[UADE_Pan].detail.mdz_boolswitch.switch_value=1;
    
    settings[UADE_PanValue].label=(char*)"Panning Value";
    settings[UADE_PanValue].description=NULL;
    settings[UADE_PanValue].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_PanValue].sub_family=0;
    settings[UADE_PanValue].callback=&optUADEChangedC;
    settings[UADE_PanValue].type=MDZ_SLIDER_CONTINUOUS;
    settings[UADE_PanValue].detail.mdz_slider.slider_value=0.7;
    settings[UADE_PanValue].detail.mdz_slider.slider_min_value=0;
    settings[UADE_PanValue].detail.mdz_slider.slider_max_value=1;
    
    settings[UADE_NTSC].type=MDZ_BOOLSWITCH;
    settings[UADE_NTSC].label=(char*)"Force NTSC";
    settings[UADE_NTSC].description=NULL;
    settings[UADE_NTSC].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_NTSC].sub_family=0;
    settings[UADE_NTSC].callback=&optUADEChangedC;
    settings[UADE_NTSC].detail.mdz_boolswitch.switch_value=0;
    
    /////////////////////////////////////
    //SEXYPSF
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_SEXYPSF].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_SEXYPSF].label=(char*)"SexyPSF";
    settings[MDZ_SETTINGS_FAMILY_SEXYPSF].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_SEXYPSF].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_SEXYPSF].sub_family=MDZ_SETTINGS_FAMILY_SEXYPSF;
    
    settings[SEXYPSF_Interpolation].type=MDZ_SWITCH;
    settings[SEXYPSF_Interpolation].label=(char*)"Interpolation";
    settings[SEXYPSF_Interpolation].description=NULL;
    settings[SEXYPSF_Interpolation].family=MDZ_SETTINGS_FAMILY_SEXYPSF;
    settings[SEXYPSF_Interpolation].sub_family=0;
    settings[SEXYPSF_Interpolation].callback=&optSEXYPSFChangedC;
    settings[SEXYPSF_Interpolation].detail.mdz_switch.switch_value=1;
    settings[SEXYPSF_Interpolation].detail.mdz_switch.switch_value_nb=4;
    settings[SEXYPSF_Interpolation].detail.mdz_switch.switch_labels=(char**)malloc(settings[SEXYPSF_Interpolation].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[SEXYPSF_Interpolation].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[SEXYPSF_Interpolation].detail.mdz_switch.switch_labels[1]=(char*)"Lin";
    settings[SEXYPSF_Interpolation].detail.mdz_switch.switch_labels[2]=(char*)"Gau";
    settings[SEXYPSF_Interpolation].detail.mdz_switch.switch_labels[3]=(char*)"Cub";
    
    settings[SEXYPSF_Reverb].type=MDZ_SWITCH;
    settings[SEXYPSF_Reverb].label=(char*)"Reverb";
    settings[SEXYPSF_Reverb].description=NULL;
    settings[SEXYPSF_Reverb].family=MDZ_SETTINGS_FAMILY_SEXYPSF;
    settings[SEXYPSF_Reverb].sub_family=0;
    settings[SEXYPSF_Reverb].callback=&optSEXYPSFChangedC;
    settings[SEXYPSF_Reverb].detail.mdz_switch.switch_value=1;
    settings[SEXYPSF_Reverb].detail.mdz_switch.switch_value_nb=3;
    settings[SEXYPSF_Reverb].detail.mdz_switch.switch_labels=(char**)malloc(settings[SEXYPSF_Reverb].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[SEXYPSF_Reverb].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[SEXYPSF_Reverb].detail.mdz_switch.switch_labels[1]=(char*)"Fake";
    settings[SEXYPSF_Reverb].detail.mdz_switch.switch_labels[2]=(char*)"Real";
    
    /////////////////////////////////////
    //AOSDK
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_AOSDK].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_AOSDK].label=(char*)"AOSDK";
    settings[MDZ_SETTINGS_FAMILY_AOSDK].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_AOSDK].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_AOSDK].sub_family=MDZ_SETTINGS_FAMILY_AOSDK;
    
    settings[AOSDK_Interpolation].type=MDZ_SWITCH;
    settings[AOSDK_Interpolation].label=(char*)"Interpolation";
    settings[AOSDK_Interpolation].description=NULL;
    settings[AOSDK_Interpolation].family=MDZ_SETTINGS_FAMILY_AOSDK;
    settings[AOSDK_Interpolation].sub_family=0;
    settings[AOSDK_Interpolation].callback=&optAOSDKChangedC;
    settings[AOSDK_Interpolation].detail.mdz_switch.switch_value=2;
    settings[AOSDK_Interpolation].detail.mdz_switch.switch_value_nb=4;
    settings[AOSDK_Interpolation].detail.mdz_switch.switch_labels=(char**)malloc(settings[AOSDK_Interpolation].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[AOSDK_Interpolation].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[AOSDK_Interpolation].detail.mdz_switch.switch_labels[1]=(char*)"Lin";
    settings[AOSDK_Interpolation].detail.mdz_switch.switch_labels[2]=(char*)"Gau";
    settings[AOSDK_Interpolation].detail.mdz_switch.switch_labels[3]=(char*)"Cub";
    
    settings[AOSDK_Reverb].type=MDZ_BOOLSWITCH;
    settings[AOSDK_Reverb].label=(char*)"Reverb";
    settings[AOSDK_Reverb].description=NULL;
    settings[AOSDK_Reverb].family=MDZ_SETTINGS_FAMILY_AOSDK;
    settings[AOSDK_Reverb].sub_family=0;
    settings[AOSDK_Reverb].callback=&optAOSDKChangedC;
    settings[AOSDK_Reverb].detail.mdz_boolswitch.switch_value=1;
    
    settings[AOSDK_DSF22KHZ].type=MDZ_SWITCH;
    settings[AOSDK_DSF22KHZ].label=(char*)"Output";
    settings[AOSDK_DSF22KHZ].description=NULL;
    settings[AOSDK_DSF22KHZ].family=MDZ_SETTINGS_FAMILY_AOSDK;
    settings[AOSDK_DSF22KHZ].sub_family=0;
    settings[AOSDK_DSF22KHZ].callback=&optAOSDKChangedC;
    settings[AOSDK_DSF22KHZ].detail.mdz_switch.switch_value=0;
    settings[AOSDK_DSF22KHZ].detail.mdz_switch.switch_value_nb=2;
    settings[AOSDK_DSF22KHZ].detail.mdz_switch.switch_labels=(char**)malloc(settings[AOSDK_DSF22KHZ].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[AOSDK_DSF22KHZ].detail.mdz_switch.switch_labels[0]=(char*)"44Khz";
    settings[AOSDK_DSF22KHZ].detail.mdz_switch.switch_labels[1]=(char*)"22Khz";
    
    settings[AOSDK_DSFDSP].type=MDZ_BOOLSWITCH;
    settings[AOSDK_DSFDSP].label=(char*)"DSF DSP";
    settings[AOSDK_DSFDSP].description=NULL;
    settings[AOSDK_DSFDSP].family=MDZ_SETTINGS_FAMILY_AOSDK;
    settings[AOSDK_DSFDSP].sub_family=0;
    settings[AOSDK_DSFDSP].callback=&optAOSDKChangedC;
    settings[AOSDK_DSFDSP].detail.mdz_boolswitch.switch_value=1;
    
    settings[AOSDK_DSFEmuRatio].type=MDZ_SWITCH;
    settings[AOSDK_DSFEmuRatio].label=(char*)"DSF Emu Ratio";
    settings[AOSDK_DSFEmuRatio].description=NULL;
    settings[AOSDK_DSFEmuRatio].family=MDZ_SETTINGS_FAMILY_AOSDK;
    settings[AOSDK_DSFEmuRatio].sub_family=0;
    settings[AOSDK_DSFEmuRatio].callback=&optAOSDKChangedC;
    settings[AOSDK_DSFEmuRatio].detail.mdz_switch.switch_value=0;
    settings[AOSDK_DSFEmuRatio].detail.mdz_switch.switch_value_nb=4;
    settings[AOSDK_DSFEmuRatio].detail.mdz_switch.switch_labels=(char**)malloc(settings[AOSDK_DSFEmuRatio].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[AOSDK_DSFEmuRatio].detail.mdz_switch.switch_labels[0]=(char*)"1";
    settings[AOSDK_DSFEmuRatio].detail.mdz_switch.switch_labels[1]=(char*)"5";
    settings[AOSDK_DSFEmuRatio].detail.mdz_switch.switch_labels[2]=(char*)"15";
    settings[AOSDK_DSFEmuRatio].detail.mdz_switch.switch_labels[3]=(char*)"30";
    
    settings[AOSDK_SSFDSP].type=MDZ_BOOLSWITCH;
    settings[AOSDK_SSFDSP].label=(char*)"SSF DSP";
    settings[AOSDK_SSFDSP].description=NULL;
    settings[AOSDK_SSFDSP].family=MDZ_SETTINGS_FAMILY_AOSDK;
    settings[AOSDK_SSFDSP].sub_family=0;
    settings[AOSDK_SSFDSP].callback=&optAOSDKChangedC;
    settings[AOSDK_SSFDSP].detail.mdz_boolswitch.switch_value=1;
    
    settings[AOSDK_SSFEmuRatio].type=MDZ_SWITCH;
    settings[AOSDK_SSFEmuRatio].label=(char*)"SSF Emu Ratio";
    settings[AOSDK_SSFEmuRatio].description=NULL;
    settings[AOSDK_SSFEmuRatio].family=MDZ_SETTINGS_FAMILY_AOSDK;
    settings[AOSDK_SSFEmuRatio].sub_family=0;
    settings[AOSDK_SSFEmuRatio].callback=&optAOSDKChangedC;
    settings[AOSDK_SSFEmuRatio].detail.mdz_switch.switch_value=0;
    settings[AOSDK_SSFEmuRatio].detail.mdz_switch.switch_value_nb=4;
    settings[AOSDK_SSFEmuRatio].detail.mdz_switch.switch_labels=(char**)malloc(settings[AOSDK_SSFEmuRatio].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[AOSDK_SSFEmuRatio].detail.mdz_switch.switch_labels[0]=(char*)"1";
    settings[AOSDK_SSFEmuRatio].detail.mdz_switch.switch_labels[1]=(char*)"5";
    settings[AOSDK_SSFEmuRatio].detail.mdz_switch.switch_labels[2]=(char*)"15";
    settings[AOSDK_SSFEmuRatio].detail.mdz_switch.switch_labels[3]=(char*)"30";
    
    /////////////////////////////////////
    //ADPLUG
    /////////////////////////////////////
    settings[MDZ_SETTINGS_FAMILY_ADPLUG].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_ADPLUG].label=(char*)"ADPLUG";
    settings[MDZ_SETTINGS_FAMILY_ADPLUG].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_ADPLUG].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_ADPLUG].sub_family=MDZ_SETTINGS_FAMILY_ADPLUG;
    
    settings[ADPLUG_OplType].type=MDZ_SWITCH;
    settings[ADPLUG_OplType].label=(char*)"OPL Type";
    settings[ADPLUG_OplType].description=NULL;
    settings[ADPLUG_OplType].family=MDZ_SETTINGS_FAMILY_ADPLUG;
    settings[ADPLUG_OplType].sub_family=0;
    settings[ADPLUG_OplType].callback=&optADPLUGChangedC;
    settings[ADPLUG_OplType].detail.mdz_switch.switch_value=0;
    settings[ADPLUG_OplType].detail.mdz_switch.switch_value_nb=3;
    settings[ADPLUG_OplType].detail.mdz_switch.switch_labels=(char**)malloc(settings[ADPLUG_OplType].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[ADPLUG_OplType].detail.mdz_switch.switch_labels[0]=(char*)"Std";
    settings[ADPLUG_OplType].detail.mdz_switch.switch_labels[1]=(char*)"Adl";
    settings[ADPLUG_OplType].detail.mdz_switch.switch_labels[2]=(char*)"Tat";
    
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
        current_family=MDZ_SETTINGS_FAMILY_ROOT;
    }
    return self;
}


- (void)viewDidLoad
{
    [super viewDidLoad];
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
    self.navigationItem.rightBarButtonItem = item;
    
    //TODO: a faire dans le delegate
    //    if (current_family==MDZ_SETTINGS_FAMILY_ROOT) [self loadSettings];
    
    //Build current mapping
    cur_settings_nb=0;
    for (int i=0;i<MAX_SETTINGS;i++) {
        if (settings[i].family==current_family) {
            cur_settings_idx[cur_settings_nb++]=i;
            
        }
    }
    
    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
    
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}

- (void)viewWillAppear:(BOOL)animated {
    
    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    
    [self.tableView reloadData];
    
    [super viewWillAppear:animated];
}


- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return cur_settings_nb;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return 1;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    NSString *title=nil;
    return title;
}

- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    NSString *footer=(settings[cur_settings_idx[section]].description?[NSString stringWithFormat:@"%s",settings[cur_settings_idx[section]].description]:nil);
    return footer;
}

- (void)boolswitchChanged:(id)sender {
    int refresh=0;
    UISwitch *sw=(UISwitch*)sender;
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    if (settings[cur_settings_idx[indexPath.section]].detail.mdz_boolswitch.switch_value != sw.on) refresh=1;
    settings[cur_settings_idx[indexPath.section]].detail.mdz_boolswitch.switch_value=sw.on;
    
    if (settings[cur_settings_idx[indexPath.section]].callback) {
        settings[cur_settings_idx[indexPath.section]].callback(self);
    }    
    if (refresh) [tableView reloadData];
}
- (void)segconChanged:(id)sender {
    int refresh=0;
    
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    if (settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value !=[(UISegmentedControl*)sender selectedSegmentIndex]) refresh=1;
    settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value=[(UISegmentedControl*)sender selectedSegmentIndex];
    
    if (settings[cur_settings_idx[indexPath.section]].callback) {
        settings[cur_settings_idx[indexPath.section]].callback(self);
    }    
    if (refresh) [tableView reloadData];
}
- (void)sliderChanged:(id)sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    
    settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value=((MNEValueTrackingSlider*)sender).value;
    
    if (settings[cur_settings_idx[indexPath.section]].callback) {
        settings[cur_settings_idx[indexPath.section]].callback(self);
    }    
    //    if (OPTION(video_fskip)==10) [((MNEValueTrackingSlider*)sender) setValue:10 sValue:@"AUTO"];
    //    [tableView reloadData];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField{
    if (settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.text) {
        free(settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.text);
    }
    settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.text=NULL;
    
    if ([textField.text length]) {
        settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.text=(char*)malloc(strlen([textField.text UTF8String]+1));
        strcpy(settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.text,[textField.text UTF8String]);
    }
    
    
    switch (cur_settings_idx[textField.tag]) {
        case ONLINE_MODLAND_URL_CUSTOM:
            case ONLINE_HVSC_URL_CUSTOM:
            case ONLINE_ASMA_URL_CUSTOM:
            if (settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.text) {
                if (strncasecmp(settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.text,"HTTP://",7)==0) break; //HTTP
                if (strncasecmp(settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.text,"FTP://",6)==0) break; //FTP
                free(settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.text);
                settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.text=NULL;
                textField.text=@"";
                
                UIAlertView *alert = [[UIAlertView alloc] initWithTitle: NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"URL have to start with ftp:// or http://","") delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
                [alert show];
            }
    }
    
    [textField resignFirstResponder];
    
    if (settings[cur_settings_idx[textField.tag]].callback) {
        settings[cur_settings_idx[textField.tag]].callback(self);
        [self.tableView reloadData];
    }
    return YES;
}



- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    UILabel *topLabel;
    UITextField *msgLabel;
    
    UISwitch *switchview;
    UISegmentedControl *segconview;
    UITextField *txtfield;
    NSMutableArray *tmpArray;
    MNEValueTrackingSlider *sliderview;
    
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,50);
        
        [cell setBackgroundColor:[UIColor clearColor]];
        
        UIImage *image = [UIImage imageNamed:@"tabview_gradient50.png"];
        UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;
        //[imageView release];
        
        //
        // Create the label for the top row of text
        //
        topLabel = [[UILabel alloc] init];
        [cell.contentView addSubview:topLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        topLabel.tag = TOP_LABEL_TAG;
        topLabel.backgroundColor = [UIColor clearColor];
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0];
        topLabel.font = [UIFont boldSystemFontOfSize:14];
        topLabel.lineBreakMode=NSLineBreakByTruncatingMiddle;
        topLabel.opaque=TRUE;
        topLabel.numberOfLines=0;
        
        cell.accessoryView=nil;
        cell.accessoryType = UITableViewCellAccessoryNone;
    } else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
    }
    topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
    topLabel.highlightedTextColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0];
    
    topLabel.frame= CGRectMake(4,
                               0,
                               tabView.bounds.size.width*4/10,
                               50);
    
    
    
    topLabel.text=NSLocalizedString(([NSString stringWithFormat:@"%s",settings[cur_settings_idx[indexPath.section]].label]),@"");
    
    switch (settings[cur_settings_idx[indexPath.section]].type) {
        case MDZ_FAMILY:
            cell.accessoryView=nil;
            cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
            break;
        case MDZ_BOOLSWITCH:
            switchview = [[UISwitch alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,40)];
            [switchview addTarget:self action:@selector(boolswitchChanged:) forControlEvents:UIControlEventValueChanged];
            switchview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            cell.accessoryView = switchview;
            //[switchview release];
            switchview.on=settings[cur_settings_idx[indexPath.section]].detail.mdz_boolswitch.switch_value;
            break;
        case MDZ_SWITCH:{
            tmpArray=[[NSMutableArray alloc] init];
            for (int i=0;i<settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value_nb;i++) {
                [tmpArray addObject:[NSString stringWithFormat:@"%s",settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_labels[i]]];
            }
            segconview = [[UISegmentedControl alloc] initWithItems:tmpArray];
            segconview.frame=CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,30);
            segconview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            //            segconview.
            UIFont *font = [UIFont boldSystemFontOfSize:12.0f];
            NSDictionary *attributes = [NSDictionary dictionaryWithObject:font
                                                                   forKey:UITextAttributeFont];
            [segconview setTitleTextAttributes:attributes
                                      forState:UIControlStateNormal];
            
            [segconview addTarget:self action:@selector(segconChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = segconview;
            //[segconview release];
            segconview.selectedSegmentIndex=settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value;
        }
            break;
        case MDZ_SLIDER_CONTINUOUS:
            sliderview = [[MNEValueTrackingSlider alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,30)];
            sliderview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            sliderview.integerMode=0;
            [sliderview setMaximumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_max_value];
            [sliderview setMinimumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_min_value];
            [sliderview setContinuous:true];
            sliderview.value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
            [sliderview addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = sliderview;
            //[sliderview release];
            break;
        case MDZ_SLIDER_DISCRETE:
            sliderview = [[MNEValueTrackingSlider alloc] initWithFrame:CGRectMake(0,0+32,tabView.bounds.size.width*5.5f/10,30)];
            sliderview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            sliderview.integerMode=1;
            [sliderview setMaximumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_max_value];
            [sliderview setMinimumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_min_value];
            [sliderview setContinuous:true];
            sliderview.value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
            [sliderview addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = sliderview;
            //[sliderview release];
            break;
        case MDZ_SLIDER_DISCRETE_TIME:
            sliderview = [[MNEValueTrackingSlider alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,30)];
            sliderview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            sliderview.integerMode=2;
            [sliderview setMaximumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_max_value];
            [sliderview setMinimumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_min_value];
            [sliderview setContinuous:true];
            sliderview.value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
            [sliderview addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
            cell.accessoryView = sliderview;
            //[sliderview release];
            break;
        case MDZ_TEXTBOX:
            txtfield = [[UITextField alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,30)];
            txtfield.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            txtfield.borderStyle = UITextBorderStyleRoundedRect;
            txtfield.font = [UIFont systemFontOfSize:15];
            txtfield.autocorrectionType = UITextAutocorrectionTypeNo;
            txtfield.keyboardType = UIKeyboardTypeASCIICapable;
            txtfield.returnKeyType = UIReturnKeyDone;
            txtfield.clearButtonMode = UITextFieldViewModeWhileEditing;
            txtfield.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
            txtfield.delegate = self;
            txtfield.tag=indexPath.section;
            
            if (settings[cur_settings_idx[indexPath.section]].detail.mdz_textbox.text) txtfield.text=[NSString stringWithFormat:@"%s",settings[cur_settings_idx[indexPath.section]].detail.mdz_textbox.text];
            else txtfield.text=@"";
            cell.accessoryView = txtfield;
            //[txtfield release];
            break;
        case MDZ_MSGBOX:
            msgLabel = [[UITextField alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,30)];
            msgLabel.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            msgLabel.tag=indexPath.section;
            
            msgLabel.borderStyle = UITextBorderStyleRoundedRect;
            msgLabel.font = [UIFont systemFontOfSize:12];
            msgLabel.autocorrectionType = UITextAutocorrectionTypeNo;
            msgLabel.keyboardType = UIKeyboardTypeASCIICapable;
            msgLabel.returnKeyType = UIReturnKeyDone;
            msgLabel.clearButtonMode = UITextFieldViewModeWhileEditing;
            msgLabel.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
            msgLabel.delegate = self;
            msgLabel.enabled=FALSE;
            msgLabel.tag=indexPath.section;
            
            if (settings[cur_settings_idx[indexPath.section]].detail.mdz_msgbox.text) msgLabel.text=[NSString stringWithFormat:@"%s",settings[cur_settings_idx[indexPath.section]].detail.mdz_textbox.text];
            else msgLabel.text=@"";
            cell.accessoryView = msgLabel;
            //[msgLabel release];
            break;
    }
    
    
    return cell;
}

/*
 // Override to support conditional editing of the table view.
 - (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath
 {
 // Return NO if you do not want the specified item to be editable.
 return YES;
 }
 */

/*
 // Override to support editing the table view.
 - (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
 {
 if (editingStyle == UITableViewCellEditingStyleDelete) {
 // Delete the row from the data source
 [tabView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
 }
 else if (editingStyle == UITableViewCellEditingStyleInsert) {
 // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
 }
 }
 */

/*
 // Override to support rearranging the table view.
 - (void)tableView:(UITableView *)tabView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
 {
 }
 */

/*
 // Override to support conditional rearranging of the table view.
 - (BOOL)tableView:(UITableView *)tabView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
 {
 // Return NO if you do not want the item to be re-orderable.
 return YES;
 }
 */

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tabView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    SettingsGenViewController *settingsVC;
    
    if (settings[cur_settings_idx[indexPath.section]].type==MDZ_FAMILY) {
        settingsVC=[[SettingsGenViewController alloc] initWithNibName:@"SettingsViewController" bundle:[NSBundle mainBundle]];
        settingsVC->detailViewController=detailViewController;
        settingsVC.title=NSLocalizedString(([NSString stringWithFormat:@"%s",settings[cur_settings_idx[indexPath.section]].label]),@"");
        settingsVC->current_family=settings[cur_settings_idx[indexPath.section]].sub_family;
        [self.navigationController pushViewController:settingsVC animated:YES];
    }
}

#pragma mark - FTP and usefull methods

- (NSString *)getIPAddress {
	NSString *address = @"error";
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *temp_addr = NULL;
	int success = 0;
	
	// retrieve the current interfaces - returns 0 on success
	success = getifaddrs(&interfaces);
	if (success == 0)
	{
		// Loop through linked list of interfaces
		temp_addr = interfaces;
		while(temp_addr != NULL)
		{
			if(temp_addr->ifa_addr->sa_family == AF_INET)
			{
				// Check if interface is en0 which is the wifi connection on the iPhone
				if([[NSString stringWithUTF8String:temp_addr->ifa_name] isEqualToString:@"en0"])
				{
					// Get NSString from C String
					address = [NSString stringWithUTF8String:inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr)];
				}
			}
			
			temp_addr = temp_addr->ifa_next;
		}
	}
	
	// Free memory
	freeifaddrs(interfaces);
	
	return address;
}

-(bool)startFTPServer {
	int ftpport=0;
	sscanf(settings[FTP_PORT].detail.mdz_textbox.text,"%d",&ftpport);
	if (ftpport==0) return FALSE;
	
    if (!ftpserver) ftpserver = new CFtpServer();
    bServerRunning = false;
    
    ftpserver->SetMaxPasswordTries( 3 );
	ftpserver->SetNoLoginTimeout( 45 ); // seconds
	ftpserver->SetNoTransferTimeout( 90 ); // seconds
	ftpserver->SetDataPortRange( 1024, 4096 ); // data TCP-Port range = [100-999]
	ftpserver->SetCheckPassDelay( 0 ); // milliseconds. Bruteforcing protection.
	
	pUser = ftpserver->AddUser(settings[FTP_USER].detail.mdz_textbox.text,
							   settings[FTP_PASSWORD].detail.mdz_textbox.text,
							   [[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/"] UTF8String]);
	
    // Create anonymous user
	if (settings[FTP_ANONYMOUS].detail.mdz_boolswitch.switch_value) {
		pAnonymousUser = ftpserver->AddUser("anonymous",
                                            NULL,
                                            [[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/"] UTF8String]);
	}
	
	
    if( pUser ) {
		pUser->SetMaxNumberOfClient( 0 ); // Unlimited
		pUser->SetPrivileges( CFtpServer::READFILE | CFtpServer::WRITEFILE |
							 CFtpServer::LIST | CFtpServer::DELETEFILE | CFtpServer::CREATEDIR |
							 CFtpServer::DELETEDIR );
    }
	if( pAnonymousUser ) pAnonymousUser->SetPrivileges( CFtpServer::READFILE | CFtpServer::WRITEFILE |
													   CFtpServer::LIST | CFtpServer::DELETEFILE | CFtpServer::CREATEDIR |
													   CFtpServer::DELETEDIR );
    if (!ftpserver->StartListening( INADDR_ANY, ftpport )) return false;
    if (!ftpserver->StartAccepting()) return false;
    
    return true;
}

+(void) ONLINEswitchChanged {
    //MODLAND
    switch (settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_value) {
        case 0://default
            if (settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text);
            settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(MODLAND_HOST_DEFAULT)+1);
            strcpy(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text,MODLAND_HOST_DEFAULT);
            
            break;
        case 1://alt1
            if (settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text);
            settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(MODLAND_HOST_ALT1)+1);
            strcpy(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text,MODLAND_HOST_ALT1);
            
            break;
        case 2://alt2
            if (settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text);
            settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(MODLAND_HOST_ALT2)+1);
            strcpy(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text,MODLAND_HOST_ALT2);
            
            break;
        case 3://custom
            if (settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text);
            if (settings[ONLINE_MODLAND_URL_CUSTOM].detail.mdz_msgbox.text) {
                settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(settings[ONLINE_MODLAND_URL_CUSTOM].detail.mdz_msgbox.text)+1);
                strcpy(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text,settings[ONLINE_MODLAND_URL_CUSTOM].detail.mdz_msgbox.text);
            } else settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text=NULL;
            
            break;
    }
    //HVSC
    switch (settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_value) {
        case 0://default
            if (settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text);
            settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(HVSC_HOST_DEFAULT)+1);
            strcpy(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text,HVSC_HOST_DEFAULT);
            
            break;
        case 1://alt1
            if (settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text);
            settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(HVSC_HOST_ALT1)+1);
            strcpy(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text,HVSC_HOST_ALT1);
            
            break;
        case 2://alt2
            if (settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text);
            settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(HVSC_HOST_ALT2)+1);
            strcpy(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text,HVSC_HOST_ALT2);
            
            break;
        case 3://custom
            if (settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text);
            if (settings[ONLINE_HVSC_URL_CUSTOM].detail.mdz_msgbox.text) {
                settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(settings[ONLINE_HVSC_URL_CUSTOM].detail.mdz_msgbox.text)+1);
                strcpy(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text,settings[ONLINE_HVSC_URL_CUSTOM].detail.mdz_msgbox.text);
            } else settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text=NULL;
            
            break;
    }
    //ASMA
    switch (settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_value) {
        case 0://default
            if (settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text);
            settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(ASMA_HOST_DEFAULT)+1);
            strcpy(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text,ASMA_HOST_DEFAULT);
            
            break;
        case 1://alt1
            if (settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text);
            settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(ASMA_HOST_ALT1)+1);
            strcpy(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text,ASMA_HOST_ALT1);
            
            break;
        case 2://alt2
            if (settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text);
            settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(ASMA_HOST_ALT2)+1);
            strcpy(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text,ASMA_HOST_ALT2);
            
            break;
        case 3://custom
            if (settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text) free(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text);
            if (settings[ONLINE_ASMA_URL_CUSTOM].detail.mdz_msgbox.text) {
                settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen(settings[ONLINE_ASMA_URL_CUSTOM].detail.mdz_msgbox.text)+1);
                strcpy(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text,settings[ONLINE_ASMA_URL_CUSTOM].detail.mdz_msgbox.text);
            } else settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text=NULL;
            
            break;
    }
}

-(void) FTPswitchChanged {
	if (settings[FTP_ONOFF].detail.mdz_switch.switch_value) {
		if ([[Reachability reachabilityForLocalWiFi] currentReachabilityStatus]==ReachableViaWiFi) {
			if (!bServerRunning) { // Start the FTP Server
				if ([self startFTPServer]) {
					bServerRunning = true;
					
					NSString *ip = [self getIPAddress];
                    
					NSString *msg = [NSString stringWithFormat:@"Running on %@", ip];
                    if (settings[FTP_STATUS].detail.mdz_msgbox.text) {
                        free(settings[FTP_STATUS].detail.mdz_msgbox.text);
                    }
                    settings[FTP_STATUS].detail.mdz_msgbox.text=(char*)malloc(strlen([msg UTF8String])+1);
                    strcpy(settings[FTP_STATUS].detail.mdz_msgbox.text,[msg UTF8String]);
                    
                    // Disable idle timer to avoid wifi connection lost
                    [UIApplication sharedApplication].idleTimerDisabled=YES;
				} else {
					bServerRunning = false;
					UIAlertView *alert = [[UIAlertView alloc] initWithTitle: @"Error" message:@"Warning: Unable to start FTP Server." delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
					[alert show];
					settings[FTP_ONOFF].detail.mdz_switch.switch_value=0;
                    
                    ftpserver->StopListening();
                    // Delete users
                    ftpserver->DeleteUser(pAnonymousUser);
                    ftpserver->DeleteUser(pUser);
                    if (settings[FTP_STATUS].detail.mdz_msgbox.text) {
                        free(settings[FTP_STATUS].detail.mdz_msgbox.text);
                    }
                    settings[FTP_STATUS].detail.mdz_msgbox.text=(char*)malloc(strlen("Inactive")+1);
                    strcpy(settings[FTP_STATUS].detail.mdz_msgbox.text,"Inactive");
				}
			}
			
		} else {
			UIAlertView *alert = [[UIAlertView alloc] initWithTitle: @"Warning" message:@"FTP server can only run on a WIFI connection." delegate:nil cancelButtonTitle:@"Close" otherButtonTitles:nil];
			[alert show];
			settings[FTP_ONOFF].detail.mdz_switch.switch_value=0;
		}
	} else {
		if (bServerRunning) { // Stop FTP server
			// Stop the server
			ftpserver->StopListening();
			// Delete users
			ftpserver->DeleteUser(pAnonymousUser);
			ftpserver->DeleteUser(pUser);
			bServerRunning = false;
            if (settings[FTP_STATUS].detail.mdz_msgbox.text) {
                free(settings[FTP_STATUS].detail.mdz_msgbox.text);
            }
            settings[FTP_STATUS].detail.mdz_msgbox.text=(char*)malloc(strlen("Inactive")+1);
            strcpy(settings[FTP_STATUS].detail.mdz_msgbox.text,"Inactive");
            
            
            // Restart idle timer if battery mode is on (unplugged device)
            if ([[UIDevice currentDevice] batteryState] != UIDeviceBatteryStateUnplugged)
                [UIApplication sharedApplication].idleTimerDisabled=YES;
            else [UIApplication sharedApplication].idleTimerDisabled=NO;
		}
	}
	[tableView reloadData];
}

-(void) dealloc {
    if (bServerRunning) { // Stop FTP server
        // Stop the server
        ftpserver->StopListening();
        // Delete users
        ftpserver->DeleteUser(pAnonymousUser);
        ftpserver->DeleteUser(pUser);
        bServerRunning = false;
        if (settings[FTP_STATUS].detail.mdz_msgbox.text) {
            free(settings[FTP_STATUS].detail.mdz_msgbox.text);
        }
    }
    
    if (ftpserver) {
        delete ftpserver;
        ftpserver=NULL;
    }
    
    //[super dealloc];
}

@end
