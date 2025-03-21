//  SettingsGenViewController.m
//  modizer
//
//  Created by Yohann Magnien on 10/08/13.
//
//



#include <iostream>

#import "SettingsGenViewController.h"
//#import "MNEValueTrackingSlider.h"
#import "OBSlider.h"

#import "Reachability.h"
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/xattr.h>

#import "TTFadeAnimator.h"

#define STRINGIZE(x) #x
#define STRINGIZE2(x) STRINGIZE(x)


@interface SettingsGenViewController ()
@end

@implementation SettingsGenViewController

@synthesize tableView,detailViewController;
@synthesize waitingView,waitingViewPlayer;

volatile t_settings settings[MAX_SETTINGS];

#include "MiniPlayerImplementTableView.h"

-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer {
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    
    NSIndexPath *indexPath = [self.tableView indexPathForRowAtPoint:p];
    if (indexPath != nil) {
        if ((gestureRecognizer.state==UIGestureRecognizerStateBegan)||(gestureRecognizer.state==UIGestureRecognizerStateChanged)) {
            //                    NSLog(@"long press on table view at %d/%d", indexPath.section,indexPath.row);
            int crow=indexPath.row;
            int csection=indexPath.section;
            if (csection>=0) {
                //display popup
                
                //get info
                NSError *err;
                NSDictionary *dict;
                
                if (settings[cur_settings_idx[indexPath.section]].description) {
                    NSString *str=NSLocalizedString(([NSString stringWithUTF8String:settings[cur_settings_idx[indexPath.section]].description]),@"");
                    
                    if (self.popTipView == nil) {
                        self.popTipView = [[CMPopTipView alloc] initWithMessage:str];
                        self.popTipView.delegate = self;
                        self.popTipView.backgroundColor = [UIColor lightGrayColor];
                        self.popTipView.textColor = [UIColor darkTextColor];
                        self.popTipView.textAlignment=UITextAlignmentLeft;
                        self.popTipView.titleAlignment=UITextAlignmentLeft;
                        
                        [self.popTipView presentPointingAtView:[[self.tableView cellForRowAtIndexPath:indexPath] viewWithTag:BOTTOM_LABEL_TAG]  inView:self.view animated:YES];
                        popTipViewRow=crow;
                        popTipViewSection=csection;
                    } else {
                        if ((popTipViewRow!=crow)||(popTipViewSection!=csection)||([str compare:self.popTipView.message]!=NSOrderedSame)) {
                            self.popTipView.message=str;
                            [self.popTipView presentPointingAtView:[[self.tableView cellForRowAtIndexPath:indexPath] viewWithTag:BOTTOM_LABEL_TAG] inView:self.view animated:YES];
                            popTipViewRow=crow;
                            popTipViewSection=csection;
                        }
                    }
                }
            }
        } else {
            //hide popup
            if (popTipView!=nil) {
                [self.popTipView dismissAnimated:YES];
                popTipView=nil;
            }
        }
    }
}

-(void)handleTapPress:(UILongPressGestureRecognizer *)gestureRecognizer {
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    
    NSIndexPath *indexPath = [self.tableView indexPathForRowAtPoint:p];
    if (indexPath != nil) {
        //if ((gestureRecognizer.state==UIGestureRecognizerStateBegan)||(gestureRecognizer.state==UIGestureRecognizerStateChanged)) {
        //                    NSLog(@"long press on table view at %d/%d", indexPath.section,indexPath.row);
        int crow=indexPath.row;
        int csection=indexPath.section;
        if (csection>=0) {
            //display popup
            
            //get info
            NSError *err;
            NSDictionary *dict;
            
            if (settings[cur_settings_idx[indexPath.section]].description) {
                NSString *str=NSLocalizedString(([NSString stringWithUTF8String:settings[cur_settings_idx[indexPath.section]].description]),@"");
                
                if (self.popTipView == nil) {
                    self.popTipView = [[CMPopTipView alloc] initWithMessage:str];
                    self.popTipView.delegate = self;
                    self.popTipView.backgroundColor = [UIColor lightGrayColor];
                    self.popTipView.textColor = [UIColor darkTextColor];
                    
                    [self.popTipView presentPointingAtView:[self.tableView cellForRowAtIndexPath:indexPath] inView:self.view animated:YES];
                    popTipViewRow=crow;
                    popTipViewSection=csection;
                } else {
                    if ((popTipViewRow!=crow)||(popTipViewSection!=csection)||([str compare:self.popTipView.message]!=NSOrderedSame)) {
                        self.popTipView.message=str;
                        [self.popTipView presentPointingAtView:[self.tableView cellForRowAtIndexPath:indexPath] inView:self.view animated:YES];
                        popTipViewRow=crow;
                        popTipViewSection=csection;
                    }
                }
            }
            // }
        } else {
            //hide popup
            if (popTipView!=nil) {
                [self.popTipView dismissAnimated:YES];
                popTipView=nil;
            }
        }
    }
}


-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:YES];
    else {
        UIAlertView *nofileplaying=[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"")
                                                              message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
        [nofileplaying show];
    }
}

#pragma mark - Callback methods

//OSCILLO Colors
-(void) optOSCILLOColorChanged {
    [detailViewController settingsChanged:(int)SETTINGS_OSCILLO];
}
void optOSCILLOColorChangedC(id param) {
    [param optOSCILLOColorChanged];
}

//PR & NS Colors
-(void) optPIANOMIDIColorChanged {
    [detailViewController settingsChanged:(int)SETTINGS_PIANOMIDI];
}
void optPIANOMIDIColorChangedC(id param) {
    [param optPIANOMIDIColorChanged];
}

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
//GME
-(void) optGMEChanged {
    [detailViewController settingsChanged:(int)SETTINGS_GME];
}
void optGMEChangedC(id param) {
    [param optGMEChanged];
}
//OMPT
-(void) optOMPTChanged {
    [detailViewController settingsChanged:(int)SETTINGS_OMPT];
}
void optOMPTChangedC(id param) {
    [param optOMPTChanged];
}
//XMP
-(void) optXMPChanged {
    [detailViewController settingsChanged:(int)SETTINGS_XMP];
}
void optXMPChangedC(id param) {
    [param optXMPChanged];
}
//GBSPLAY
-(void) optGBSPLAYChanged {
    [detailViewController settingsChanged:(int)SETTINGS_GBSPLAY];
}
void optGBSPLAYChangedC(id param) {
    [param optGBSPLAYChanged];
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

//HC
-(void) optHCChanged {
    [detailViewController settingsChanged:(int)SETTINGS_HC];
}
void optHCChangedC(id param) {
    [param optHCChanged];
}
//GSF
-(void) optGSFChanged {
    [detailViewController settingsChanged:(int)SETTINGS_GSF];
}
void optGSFChangedC(id param) {
    [param optGSFChanged];
}

//NSFPLAY
-(void) optNSFPLAYChanged {
    [detailViewController settingsChanged:(int)SETTINGS_NSFPLAY];
}
void optNSFPLAYChangedC(id param) {
    [param optNSFPLAYChanged];
}


#pragma mark - Load/Init default settings

+ (void) restoreSettings {
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSNumber *valNb;
    NSString *str;
    
    [prefs synchronize];
    
    for (int i=0;i<MAX_SETTINGS;i++)
        if (settings[i].label) {
            
            //if (settings[i].setting_id) NSLog(@"id:%s",settings[i].setting_id);
            
            str=[NSString stringWithFormat:@"%s",settings[i].setting_id];
            
            /*str=[NSString stringWithFormat:@"%s",settings[i].label];
             int j=settings[i].family;
             while (j!=MDZ_SETTINGS_FAMILY_ROOT) {
             str=[NSString stringWithFormat:@"%s/%@",settings[j].label,str];
             j=settings[j].family;
             }*/
            
            switch (settings[i].type) {
                case MDZ_BOOLSWITCH:
                    valNb=[prefs objectForKey:str];
                    if ((valNb!=nil)&&([valNb isKindOfClass:[NSNumber class]])) settings[i].detail.mdz_boolswitch.switch_value=[valNb intValue];
                    break;
                case MDZ_SWITCH:
                    valNb=[prefs objectForKey:str];
                    if ((valNb!=nil)&&([valNb isKindOfClass:[NSNumber class]])) settings[i].detail.mdz_switch.switch_value=[valNb intValue];
                    if (settings[i].detail.mdz_switch.switch_value<0) settings[i].detail.mdz_switch.switch_value=0;
                    if (settings[i].detail.mdz_switch.switch_value>settings[i].detail.mdz_switch.switch_value_nb) settings[i].detail.mdz_switch.switch_value=settings[i].detail.mdz_switch.switch_value_nb-1;
                    break;
                case MDZ_SLIDER_DISCRETE:
                    valNb=[prefs objectForKey:str];
                    if ((valNb!=nil)&&([valNb isKindOfClass:[NSNumber class]])) settings[i].detail.mdz_slider.slider_value=[valNb intValue];
                    if (settings[i].detail.mdz_slider.slider_value<settings[i].detail.mdz_slider.slider_min_value) settings[i].detail.mdz_slider.slider_value=settings[i].detail.mdz_slider.slider_min_value;
                    if (settings[i].detail.mdz_slider.slider_value>settings[i].detail.mdz_slider.slider_max_value) settings[i].detail.mdz_slider.slider_value=settings[i].detail.mdz_slider.slider_max_value;
                    break;
                case MDZ_SLIDER_DISCRETE_TIME:
                    valNb=[prefs objectForKey:str];
                    if ((valNb!=nil)&&([valNb isKindOfClass:[NSNumber class]])) settings[i].detail.mdz_slider.slider_value=[valNb intValue];
                    if (settings[i].detail.mdz_slider.slider_value<settings[i].detail.mdz_slider.slider_min_value) settings[i].detail.mdz_slider.slider_value=settings[i].detail.mdz_slider.slider_min_value;
                    if (settings[i].detail.mdz_slider.slider_value>settings[i].detail.mdz_slider.slider_max_value) settings[i].detail.mdz_slider.slider_value=settings[i].detail.mdz_slider.slider_max_value;
                    break;
                case MDZ_SLIDER_DISCRETE_TIME_LONG:
                    valNb=[prefs objectForKey:str];
                    if ((valNb!=nil)&&([valNb isKindOfClass:[NSNumber class]])) settings[i].detail.mdz_slider.slider_value=[valNb intValue];
                    if (settings[i].detail.mdz_slider.slider_value<settings[i].detail.mdz_slider.slider_min_value) settings[i].detail.mdz_slider.slider_value=settings[i].detail.mdz_slider.slider_min_value;
                    if (settings[i].detail.mdz_slider.slider_value>settings[i].detail.mdz_slider.slider_max_value) settings[i].detail.mdz_slider.slider_value=settings[i].detail.mdz_slider.slider_max_value;
                    break;
                case MDZ_SLIDER_CONTINUOUS:
                    valNb=[prefs objectForKey:str];
                    if ((valNb!=nil)&&([valNb isKindOfClass:[NSNumber class]])) settings[i].detail.mdz_slider.slider_value=[valNb floatValue];
                    if (settings[i].detail.mdz_slider.slider_value<settings[i].detail.mdz_slider.slider_min_value) settings[i].detail.mdz_slider.slider_value=settings[i].detail.mdz_slider.slider_min_value;
                    if (settings[i].detail.mdz_slider.slider_value>settings[i].detail.mdz_slider.slider_max_value) settings[i].detail.mdz_slider.slider_value=settings[i].detail.mdz_slider.slider_max_value;
                    break;
                case MDZ_TEXTBOX:
                    str=[prefs objectForKey:str];
                    if ((str!=nil)&&([str isKindOfClass:[NSString class]])) {
                        if (settings[i].detail.mdz_textbox.text) free(settings[i].detail.mdz_textbox.text);
                        settings[i].detail.mdz_textbox.text=(char*)malloc(strlen([str UTF8String])+1);
                        strcpy(settings[i].detail.mdz_textbox.text,[str UTF8String]);
                    }
                    break;
                    //
                case MDZ_MSGBOX:
                    break;
                case MDZ_COLORPICKER:
                    valNb=[prefs objectForKey:str];
                    if ((valNb!=nil)&&([valNb isKindOfClass:[NSNumber class]])) settings[i].detail.mdz_color.rgb=[valNb intValue];
                    if (settings[i].detail.mdz_color.rgb<0) settings[i].detail.mdz_color.rgb=0;
                    if (settings[i].detail.mdz_color.rgb>0xFFFFFF) settings[i].detail.mdz_color.rgb=0xFFFFFF;
                    break;
                case MDZ_FAMILY:
                    break;
                case MDZ_BUTTON:
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
            
            str=[NSString stringWithFormat:@"%s",settings[i].setting_id];
            
            //NSLog(@"backup settings: %@",str);
            /*str=[NSString stringWithFormat:@"%s",settings[i].label];
             int j=settings[i].family;
             while (j!=MDZ_SETTINGS_FAMILY_ROOT) {
             str=[NSString stringWithFormat:@"%s/%@",settings[j].label,str];
             j=settings[j].family;
             }*/
            //NSLog(@"got: %@",str);
            
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
                case MDZ_SLIDER_DISCRETE_TIME_LONG:
                    valNb=[[NSNumber alloc] initWithInt:settings[i].detail.mdz_slider.slider_value];
                    [prefs setObject:valNb forKey:str];
                    break;
                case MDZ_SLIDER_CONTINUOUS:
                    valNb=[[NSNumber alloc] initWithFloat:settings[i].detail.mdz_slider.slider_value];
                    [prefs setObject:valNb forKey:str];
                    break;
                case MDZ_TEXTBOX:
                    if (settings[i].detail.mdz_textbox.text) [prefs setObject:[NSString stringWithUTF8String:settings[i].detail.mdz_textbox.text] forKey:str];
                    else [prefs setObject:@"" forKey:str];
                    break;
                    //
                case MDZ_COLORPICKER:
                    valNb=[[NSNumber alloc] initWithInt:settings[i].detail.mdz_color.rgb];
                    [prefs setObject:valNb forKey:str];
                    break;
                case MDZ_MSGBOX:
                    break;
                case MDZ_FAMILY:
                    break;
                case MDZ_BUTTON:
                    break;
            }
        }
    [prefs synchronize];
}


+(void) oscilloGenSystemColor:(int)_mode color_idx:(int)color_idx color_buffer:(unsigned int*)color_buffer; {
    float start_pos,mul_factor,sat;
    
    switch (_mode%3) {
        case 2:
            start_pos=240.0f/360.0f;
            mul_factor=(1.45f/SOUND_VOICES_MAX_ACTIVE_CHIPS);
            sat=0.7f;
            if ((color_idx<0)||(color_idx==0)) settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb=0x00FF00;
            break;
        case 1:
            start_pos=130.0f/360.0f;
            mul_factor=(1.7f/SOUND_VOICES_MAX_ACTIVE_CHIPS);
            sat=0.7f;
            if ((color_idx<0)||(color_idx==0)) settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb=0x00FF00;
            break;
        case 0:
            start_pos=240.0f/360.0f;
            mul_factor=(9.5f/SOUND_VOICES_MAX_ACTIVE_CHIPS);
            sat=0.6f;
            if ((color_idx<0)||(color_idx==0)) settings[OSCILLO_MONO_COLOR].detail.mdz_color.rgb=0x00FF00;
            break;
        default:
            break;
    }
    
    if ((color_idx<0)||(color_idx==1)) settings[OSCILLO_GRID_COLOR].detail.mdz_color.rgb=0x404040;
    
    for (int i=0;i<SOUND_VOICES_MAX_ACTIVE_CHIPS;i++) {
        CGFloat hue=start_pos+i*mul_factor;
        while (hue>1.0) hue-=1.0f;
        while (hue<0.0) hue+=1.0f;
        UIColor *col=[UIColor colorWithHue:hue saturation:sat brightness:1.0f alpha:1.0f];
        CGFloat red,green,blue;
        [col getRed:&red green:&green blue:&blue alpha:NULL];
        //CIColor *cicol=[CIColor colorWithCGColor:col.CGColor];
        if ((color_idx<0)||(color_idx==(i+2))) {
            if (color_buffer) color_buffer[i]=(((int)(red*255))<<16)|(((int)(green*255))<<8)|(((int)(blue*255))<<0);
            else settings[OSCILLO_MULTI_COLOR01+i].detail.mdz_color.rgb=(((int)(red*255))<<16)|(((int)(green*255))<<8)|(((int)(blue*255))<<0);
        }
    }
}


+(void) pianomidiGenSystemColor:(int)_mode color_idx:(int)color_idx color_buffer:(unsigned int*)color_buffer{
    float start_pos,mul_factor,sat;
    
    switch (_mode%3) {
        case 0:
            start_pos=220.0f/360.0f;
            mul_factor=(0.4f/SOUND_VOICES_MAX_ACTIVE_CHIPS);
            sat=1.0f;
            break;
        case 1:
            start_pos=240.0f/360.0f;
            mul_factor=3.3/32.0;//(9.9f/SOUND_VOICES_MAX_ACTIVE_CHIPS);
            sat=1.0f;
            break;
        case 2:
            start_pos=120.0f/360.0f;
            mul_factor=(1.7f/SOUND_VOICES_MAX_ACTIVE_CHIPS);
            sat=1.0f;
            break;
        default:
            break;
    }
    
    for (int i=0;i<32;i++) {
        CGFloat hue=start_pos+i*mul_factor;
        
        if (_mode==0) {
            hue=start_pos+(i&7)*mul_factor+(float)(i/8)*21.0/360.0;
            sat=1-(float)(i/8)*0.1;
        }
        
        while (hue>1.0) hue-=1.0f;
        while (hue<0.0) hue+=1.0f;
        UIColor *col;
        col=[UIColor colorWithHue:hue saturation:sat brightness:1.0f alpha:1.0f];
        CGFloat red,green,blue;
        [col getRed:&red green:&green blue:&blue alpha:NULL];
        CIColor *cicol=[CIColor colorWithCGColor:col.CGColor];
        if ((color_idx<0)||(color_idx==i)) {
            if (color_buffer) color_buffer[i]=(((int)(red*255))<<16)|(((int)(green*255))<<8)|(((int)(blue*255))<<0);
            else settings[PIANOMIDI_MULTI_COLOR01+i].detail.mdz_color.rgb=(((int)(red*255))<<16)|(((int)(green*255))<<8)|(((int)(blue*255))<<0);
        }
    }
}



+ (void) applyDefaultSettings {
    /////////////////////////////////////
    //GLOBAL Player
    /////////////////////////////////////
    settings[GLOB_ForceMono].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_PBRATIO].detail.mdz_slider.slider_value=1;
    settings[GLOB_PBRATIO_ONOFF].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_Panning].detail.mdz_boolswitch.switch_value=1;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_value=0.7;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_value=SONG_DEFAULT_LENGTH/1000;
    settings[GLOB_Fadeouttime].detail.mdz_slider.slider_value=3;
    settings[GLOB_SilenceDetection].detail.mdz_slider.slider_value=3;
    
    
    settings[GLOB_AudioLatency].detail.mdz_slider.slider_value=0;
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultSIDPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultYMPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultNSFPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultPT3Player].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultGBSPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultKSSPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_Default2SFPlayer].detail.mdz_switch.switch_value=0;
    settings[GLOB_DefaultMIDIPlayer].detail.mdz_switch.switch_value=0;
    //settings[GLOB_PlaybackFrequency].detail.mdz_switch.switch_value=0;
    
    settings[GLOB_ArcMultiDefaultAction].detail.mdz_switch.switch_value=0;
    settings[GLOB_ArcMultiPlayMode].detail.mdz_switch.switch_value=0;
    
    settings[GLOB_SearchRegExp].detail.mdz_boolswitch.switch_value=1;
    settings[GLOB_ResumeOnStart].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_NoScreenAutoLock].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value=1;
    settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_StatsUpload].detail.mdz_boolswitch.switch_value=1;
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value=2;
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value=2;
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value=0;
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value=1;
    settings[GLOB_CoverFlow].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_RecreateSamplesFolder].detail.mdz_boolswitch.switch_value=1;
    
    /////////////////////////////////////
    //GLOBAL FTP
    /////////////////////////////////////
    if (settings[FTP_STATUS].detail.mdz_msgbox.text) free(settings[FTP_STATUS].detail.mdz_msgbox.text);
    settings[FTP_STATUS].detail.mdz_msgbox.text=(char*)malloc(strlen("Inactive")+1);
    strcpy(settings[FTP_STATUS].detail.mdz_msgbox.text,"Inactive");
    
    settings[FTP_ONOFF].detail.mdz_switch.switch_value=0;
    settings[FTP_ANONYMOUS].detail.mdz_boolswitch.switch_value=1;
    
    if (settings[FTP_USER].detail.mdz_textbox.text) free(settings[FTP_USER].detail.mdz_textbox.text);
    settings[FTP_USER].detail.mdz_textbox.text=NULL;
    
    if (settings[FTP_PASSWORD].detail.mdz_textbox.text) free(settings[FTP_PASSWORD].detail.mdz_textbox.text);
    settings[FTP_PASSWORD].detail.mdz_textbox.text=NULL;
    
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
    settings[GLOB_FXAlphaFS].detail.mdz_slider.slider_value=0.8;
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=0;
    settings[OSCILLO_ShowLabel].detail.mdz_boolswitch.switch_value=1;
    settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value=1;
    
    settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value=1;
    settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value=1;
    
    [SettingsGenViewController oscilloGenSystemColor:0 color_idx:-1 color_buffer:NULL];
    settings[OSCILLO_MULTI_COLORSET].detail.mdz_switch.switch_value=0;
    
    settings[OSCILLO_ShowLabel].detail.mdz_boolswitch.switch_value=1;
    settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value=1;
    
    
    
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value=1;
    
    [SettingsGenViewController pianomidiGenSystemColor:0 color_idx:-1 color_buffer:NULL];
    settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_value=0;
    
    settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value=1;
    settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_boolswitch.switch_value=1;
    settings[GLOB_FXPianoRollOctavesLabels].detail.mdz_boolswitch.switch_value=0;
    
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_value=2;
    settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value=1;
    settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_value=2;
    settings[GLOB_FXPiano].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_value=0;
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value=1;
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=0;
    settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
    settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
    settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
    settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
    
    settings[GLOB_FXLOD].detail.mdz_switch.switch_value=2;
    
    settings[GLOB_FXFPS].detail.mdz_switch.switch_value=1;
    
    settings[GLOB_FXMSAA].detail.mdz_switch.switch_value=0;
    
    /////////////////////////////////////
    //PLUGINS
    /////////////////////////////////////
    
    /////////////////////////////////////
    //OPENMPT
    /////////////////////////////////////
    settings[OMPT_MasterVolume].detail.mdz_slider.slider_value=0.5;
    settings[OMPT_Sampling].detail.mdz_switch.switch_value=0;
    settings[OMPT_StereoSeparation].detail.mdz_slider.slider_value=0.5;
    
    /////////////////////////////////////
    //TIMIDITY
    /////////////////////////////////////
    settings[TIM_Maxloop].detail.mdz_slider.slider_value=0;
    settings[TIM_Polyphony].detail.mdz_slider.slider_value=128;
    settings[TIM_Amplification].detail.mdz_slider.slider_value=100;
    settings[TIM_Chorus].detail.mdz_boolswitch.switch_value=1;
    settings[TIM_Reverb].detail.mdz_boolswitch.switch_value=1;
    settings[TIM_LPFilter].detail.mdz_boolswitch.switch_value=1;
    settings[TIM_Resample].detail.mdz_switch.switch_value=1;
    
    /////////////////////////////////////
    //GBSPLAY
    /////////////////////////////////////
    settings[GBSPLAY_HPFilterType].detail.mdz_switch.switch_value=1;
    
    /////////////////////////////////////
    //GME
    /////////////////////////////////////
    settings[GME_IGNORESILENCE].detail.mdz_slider.slider_value=0;
    settings[GME_EQ_ONOFF].detail.mdz_boolswitch.switch_value=0;
    settings[GME_EQ_BASS].detail.mdz_slider.slider_value=4.2-1.9;
    settings[GME_EQ_TREBLE].detail.mdz_slider.slider_value=-14;
    settings[GME_STEREO_PANNING].detail.mdz_slider.slider_value=0.7;
    
    /////////////////////////////////////
    //GSF
    /////////////////////////////////////
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_value=2;
    settings[GSF_INTERPOLATION].detail.mdz_boolswitch.switch_value=1;
    settings[GSF_LOWPASSFILTER].detail.mdz_boolswitch.switch_value=1;
    settings[GSF_ECHO].detail.mdz_boolswitch.switch_value=0;
    
    /////////////////////////////////////
    //NSFPLAY
    /////////////////////////////////////
    settings[NSFPLAY_DefaultLength].detail.mdz_slider.slider_value=SONG_DEFAULT_LENGTH/1000;
    settings[NSFPLAY_Quality].detail.mdz_slider.slider_value=10;
    settings[NSFPLAY_LowPass_Filter_Strength].detail.mdz_slider.slider_value=112;
    settings[NSFPLAY_HighPass_Filter_Strength].detail.mdz_slider.slider_value=164;
    settings[NSFPLAY_Region].detail.mdz_switch.switch_value=0;
    settings[NSFPLAY_ForceIRQ].detail.mdz_boolswitch.switch_value=0;
    
    settings[NSFPLAY_APU_OPTION0].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_APU_OPTION1].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_APU_OPTION2].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_APU_OPTION3].detail.mdz_boolswitch.switch_value=0;
    settings[NSFPLAY_APU_OPTION4].detail.mdz_boolswitch.switch_value=0;
    
    settings[NSFPLAY_DMC_OPTION0].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_DMC_OPTION1].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_DMC_OPTION2].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_DMC_OPTION3].detail.mdz_boolswitch.switch_value=0;
    settings[NSFPLAY_DMC_OPTION4].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_DMC_OPTION5].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_DMC_OPTION6].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_DMC_OPTION7].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_DMC_OPTION8].detail.mdz_boolswitch.switch_value=0;
    
    settings[NSFPLAY_MMC5_OPTION0].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_MMC5_OPTION1].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_N163_OPTION0].detail.mdz_boolswitch.switch_value=1;
    settings[NSFPLAY_N163_OPTION1].detail.mdz_boolswitch.switch_value=0;
    settings[NSFPLAY_N163_OPTION2].detail.mdz_boolswitch.switch_value=0;
    if (settings[NSFPLAY_FDS_OPTION0].detail.mdz_msgbox.text) free(settings[NSFPLAY_FDS_OPTION0].detail.mdz_msgbox.text);
    settings[NSFPLAY_FDS_OPTION0].detail.mdz_msgbox.text=(char*)malloc(strlen("2000")+1);
    strcpy(settings[NSFPLAY_FDS_OPTION0].detail.mdz_msgbox.text,"2000");
    settings[NSFPLAY_FDS_OPTION1].detail.mdz_boolswitch.switch_value=0;
    settings[NSFPLAY_FDS_OPTION2].detail.mdz_boolswitch.switch_value=0;
    settings[NSFPLAY_VRC7_OPTION0].detail.mdz_boolswitch.switch_value=0;
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_value=0;
    
    
    
    
    
    /////////////////////////////////////
    //SID
    /////////////////////////////////////
    settings[SID_Engine].detail.mdz_boolswitch.switch_value=1;
    settings[SID_Interpolation].detail.mdz_switch.switch_value=2;
    settings[SID_Filter].detail.mdz_boolswitch.switch_value=1;
    settings[SID_SecondSIDOn].detail.mdz_switch.switch_value=0;
    settings[SID_ThirdSIDOn].detail.mdz_switch.switch_value=0;
    //0xD420-0xD7FF  or  0xDE00-0xDFFF
    if (settings[SID_SecondSIDAddress].detail.mdz_msgbox.text) free(settings[SID_SecondSIDAddress].detail.mdz_msgbox.text);
    settings[SID_SecondSIDAddress].detail.mdz_msgbox.text=(char*)malloc(strlen("0xd420")+1);
    strcpy(settings[SID_SecondSIDAddress].detail.mdz_msgbox.text,"0xd420");
    if (settings[SID_ThirdSIDAddress].detail.mdz_msgbox.text) free(settings[SID_ThirdSIDAddress].detail.mdz_msgbox.text);
    settings[SID_ThirdSIDAddress].detail.mdz_msgbox.text=(char*)malloc(strlen("0xd440")+1);
    strcpy(settings[SID_ThirdSIDAddress].detail.mdz_msgbox.text,"0xd440");
    
    settings[SID_ForceLoop].detail.mdz_boolswitch.switch_value=0;
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
    //ADPLUG
    /////////////////////////////////////
    settings[ADPLUG_OplType].detail.mdz_switch.switch_value=0;
    settings[ADPLUG_StereoSurround].detail.mdz_switch.switch_value=1;
    settings[ADPLUG_PriorityOMPT].detail.mdz_switch.switch_value=1;
    
    /////////////////////////////////////
    //VGMPLAY
    /////////////////////////////////////
    settings[VGMPLAY_Maxloop].detail.mdz_slider.slider_value=2;
    settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value=0;
    settings[VGMPLAY_YMF262Emulator].detail.mdz_switch.switch_value=0;
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_value=0;
    settings[VGMPLAY_YM3812Emulator].detail.mdz_switch.switch_value=0;
    settings[VGMPLAY_QSoundEmulator].detail.mdz_switch.switch_value=0;
    settings[VGMPLAY_RF5C68Emulator].detail.mdz_switch.switch_value=0;
    settings[VGMPLAY_NUKEDOPN2_Option].detail.mdz_switch.switch_value=0;
    
    
    /////////////////////////////////////
    //VGMSTREAM
    /////////////////////////////////////
    settings[VGMSTREAM_Forceloop].detail.mdz_boolswitch.switch_value=0;
    settings[VGMSTREAM_Maxloop].detail.mdz_slider.slider_value=2;
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_value=1;
    
    
    /////////////////////////////////////
    //HC
    /////////////////////////////////////
    settings[HC_ResampleQuality].detail.mdz_switch.switch_value=0;
    settings[HC_MainEnabled].detail.mdz_boolswitch.switch_value=1;
    settings[HC_ReverbEnabled].detail.mdz_boolswitch.switch_value=1;
    
    /////////////////////////////////////
    //XMP
    /////////////////////////////////////
    settings[XMP_Interpolation].detail.mdz_switch.switch_value=1;
    settings[XMP_MasterVolume].detail.mdz_slider.slider_value=100;
    settings[XMP_Amplification].detail.mdz_switch.switch_value=1;
    settings[XMP_StereoSeparation].detail.mdz_slider.slider_value=100;
    //settings[XMP_DSPLowPass].detail.mdz_boolswitch.switch_value=1;
    settings[XMP_FLAGS_A500F].detail.mdz_boolswitch.switch_value=0;
    
    
}

#define SETTINGS_ID_DEF(x) settings[x].setting_id=STRINGIZE2(x);
+ (void) loadSettings {
    memset((char*)settings,0,sizeof(settings));
    /////////////////////////////////////
    //ROOT
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER)
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER].label=(char*)"Global";
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER].sub_family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY)
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY].label=(char*)"Default Players";
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY].sub_family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_GLOBAL_VISU)
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_VISU].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_VISU].label=(char*)"Visualizers";
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_VISU].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_VISU].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_VISU].sub_family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_OSCILLO)
    settings[MDZ_SETTINGS_FAMILY_OSCILLO].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_OSCILLO].label=(char*)"Oscillo settings";
    settings[MDZ_SETTINGS_FAMILY_OSCILLO].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_OSCILLO].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[MDZ_SETTINGS_FAMILY_OSCILLO].sub_family=MDZ_SETTINGS_FAMILY_OSCILLO;
    
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_PIANOMIDI_COL)
    settings[MDZ_SETTINGS_FAMILY_PIANOMIDI_COL].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_PIANOMIDI_COL].label=(char*)"PR & NS colors settings";
    settings[MDZ_SETTINGS_FAMILY_PIANOMIDI_COL].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_PIANOMIDI_COL].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[MDZ_SETTINGS_FAMILY_PIANOMIDI_COL].sub_family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    
    
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_PLUGINS)
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].label=(char*)"Plugins";
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_PLUGINS].sub_family=MDZ_SETTINGS_FAMILY_PLUGINS;
    
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_GLOBAL_FTP)
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_FTP].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_FTP].label=(char*)"FTP";
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_FTP].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_FTP].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_FTP].sub_family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE)
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE].label=(char*)"Online";
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE].family=MDZ_SETTINGS_FAMILY_ROOT;
    settings[MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE].sub_family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    
    
    
    /////////////////////////////////////
    //GLOBAL Player
    /////////////////////////////////////
    ///
    
    SETTINGS_ID_DEF(GLOB_PBRATIO_ONOFF)
    settings[GLOB_PBRATIO_ONOFF].type=MDZ_BOOLSWITCH;
    settings[GLOB_PBRATIO_ONOFF].label=(char*)"Enable playback ratio";
    settings[GLOB_PBRATIO_ONOFF].description=(char*)"Only for plugins supporting this feature (GME, NSFPLAY, LIBVGM)";
    settings[GLOB_PBRATIO_ONOFF].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_PBRATIO_ONOFF].sub_family=0;
    settings[GLOB_PBRATIO_ONOFF].callback=&optGLOBALChangedC;
    
    SETTINGS_ID_DEF(GLOB_PBRATIO)
    settings[GLOB_PBRATIO].label=(char*)"Playback ratio";
    settings[GLOB_PBRATIO].description=NULL;
    settings[GLOB_PBRATIO].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_PBRATIO].sub_family=0;
    settings[GLOB_PBRATIO].callback=&optGLOBALChangedC;
    settings[GLOB_PBRATIO].type=MDZ_SLIDER_CONTINUOUS;
    settings[GLOB_PBRATIO].detail.mdz_slider.slider_digits=20;
    settings[GLOB_PBRATIO].detail.mdz_slider.slider_min_value=0.2;
    settings[GLOB_PBRATIO].detail.mdz_slider.slider_max_value=3;
    
    SETTINGS_ID_DEF(GLOB_ForceMono)
    settings[GLOB_ForceMono].setting_id=STRINGIZE2(GLOB_ForceMono);
    settings[GLOB_ForceMono].label=(char*)"Force Mono";
    settings[GLOB_ForceMono].description=NULL;
    settings[GLOB_ForceMono].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_ForceMono].sub_family=0;
    settings[GLOB_ForceMono].callback=&optGLOBALChangedC;
    settings[GLOB_ForceMono].type=MDZ_BOOLSWITCH;
    
    SETTINGS_ID_DEF(GLOB_Panning)
    settings[GLOB_Panning].label=(char*)"Panning (Stereo separation)";
    settings[GLOB_Panning].description=NULL;
    settings[GLOB_Panning].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_Panning].sub_family=0;
    settings[GLOB_Panning].callback=&optGLOBALChangedC;
    settings[GLOB_Panning].type=MDZ_BOOLSWITCH;
    
    SETTINGS_ID_DEF(GLOB_PanningValue)
    settings[GLOB_PanningValue].label=(char*)"Panning value";
    settings[GLOB_PanningValue].description=NULL;
    settings[GLOB_PanningValue].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_PanningValue].sub_family=0;
    settings[GLOB_PanningValue].callback=&optGLOBALChangedC;
    settings[GLOB_PanningValue].type=MDZ_SLIDER_CONTINUOUS;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_digits=100;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_min_value=0;
    settings[GLOB_PanningValue].detail.mdz_slider.slider_max_value=1;
    
    SETTINGS_ID_DEF(GLOB_Fadeouttime)
    settings[GLOB_Fadeouttime].label=(char*)"Fade out time";
    settings[GLOB_Fadeouttime].description=NULL;
    settings[GLOB_Fadeouttime].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_Fadeouttime].sub_family=0;
    settings[GLOB_Fadeouttime].callback=&optGLOBALChangedC;
    settings[GLOB_Fadeouttime].type=MDZ_SLIDER_DISCRETE;
    settings[GLOB_Fadeouttime].detail.mdz_slider.slider_digits=0;
    settings[GLOB_Fadeouttime].detail.mdz_slider.slider_min_value=0;
    settings[GLOB_Fadeouttime].detail.mdz_slider.slider_max_value=10;
    
    SETTINGS_ID_DEF(GLOB_SilenceDetection)
    settings[GLOB_SilenceDetection].label=(char*)"Silence Detection";
    settings[GLOB_SilenceDetection].description=(char*)"Stop song if silent for more than selected number of seconds. 0: off";
    settings[GLOB_SilenceDetection].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_SilenceDetection].sub_family=0;
    settings[GLOB_SilenceDetection].callback=&optGLOBALChangedC;
    settings[GLOB_SilenceDetection].type=MDZ_SLIDER_DISCRETE;
    settings[GLOB_SilenceDetection].detail.mdz_slider.slider_digits=0;
    settings[GLOB_SilenceDetection].detail.mdz_slider.slider_min_value=0;
    settings[GLOB_SilenceDetection].detail.mdz_slider.slider_max_value=10;
    
    SETTINGS_ID_DEF(GLOB_DefaultLength)
    settings[GLOB_DefaultLength].label=(char*)"Default Length";
    settings[GLOB_DefaultLength].description=NULL;
    settings[GLOB_DefaultLength].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_DefaultLength].sub_family=0;
    settings[GLOB_DefaultLength].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultLength].type=MDZ_SLIDER_DISCRETE_TIME_LONG;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_digits=60;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_min_value=10;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_mid_value=60*10;
    settings[GLOB_DefaultLength].detail.mdz_slider.slider_max_value=60*60;
    
    SETTINGS_ID_DEF(GLOB_AudioLatency)
    settings[GLOB_AudioLatency].type=MDZ_SWITCH;
    settings[GLOB_AudioLatency].label=(char*)"Audio latency in ms";
    settings[GLOB_AudioLatency].description=(char*)"0=auto, delay rounded to a multiple of audio buffer size";
    settings[GLOB_AudioLatency].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_AudioLatency].sub_family=0;
    settings[GLOB_AudioLatency].callback=&optGLOBALChangedC;
    settings[GLOB_AudioLatency].type=MDZ_SLIDER_DISCRETE;
    settings[GLOB_AudioLatency].detail.mdz_slider.slider_digits=0;
    settings[GLOB_AudioLatency].detail.mdz_slider.slider_min_value=0;
    settings[GLOB_AudioLatency].detail.mdz_slider.slider_max_value=1000*(SOUND_BUFFER_NB/2)*SOUND_BUFFER_SIZE_SAMPLE/DEFAULT_PLAYBACK_FREQ;
    
    SETTINGS_ID_DEF(GLOB_ResumeOnStart)
    settings[GLOB_ResumeOnStart].label=(char*)"Resume position on launch";
    settings[GLOB_ResumeOnStart].description=NULL;
    settings[GLOB_ResumeOnStart].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_ResumeOnStart].sub_family=0;
    settings[GLOB_ResumeOnStart].callback=&optGLOBALChangedC;
    settings[GLOB_ResumeOnStart].type=MDZ_BOOLSWITCH;
    
    SETTINGS_ID_DEF(GLOB_NoScreenAutoLock)
    settings[GLOB_NoScreenAutoLock].label=(char*)"Disable screen auto lock";
    settings[GLOB_NoScreenAutoLock].description=NULL;
    settings[GLOB_NoScreenAutoLock].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_NoScreenAutoLock].sub_family=0;
    settings[GLOB_NoScreenAutoLock].callback=&optGLOBALChangedC;
    settings[GLOB_NoScreenAutoLock].type=MDZ_BOOLSWITCH;
    
    SETTINGS_ID_DEF(GLOB_SearchRegExp)
    settings[GLOB_SearchRegExp].label=(char*)"Search: simplified regexp ('.'->'\\.', '*'->'.*')";
    settings[GLOB_SearchRegExp].description=NULL;
    settings[GLOB_SearchRegExp].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_SearchRegExp].sub_family=0;
    settings[GLOB_SearchRegExp].callback=&optGLOBALChangedC;
    settings[GLOB_SearchRegExp].type=MDZ_BOOLSWITCH;
    
    SETTINGS_ID_DEF(GLOB_ArcMultiDefaultAction)
    settings[GLOB_ArcMultiDefaultAction].type=MDZ_SWITCH;
    settings[GLOB_ArcMultiDefaultAction].label=(char*)"Multi default action";
    settings[GLOB_ArcMultiDefaultAction].description=(char*)"Default action when selecting an archive or a multisong file";
    settings[GLOB_ArcMultiDefaultAction].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_ArcMultiDefaultAction].sub_family=0;
    settings[GLOB_ArcMultiDefaultAction].callback=&optGLOBALChangedC;
    settings[GLOB_ArcMultiDefaultAction].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_ArcMultiDefaultAction].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_ArcMultiDefaultAction].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_ArcMultiDefaultAction].detail.mdz_switch.switch_labels[0]=(char*)"Play";
    settings[GLOB_ArcMultiDefaultAction].detail.mdz_switch.switch_labels[1]=(char*)"Browse";
    
    SETTINGS_ID_DEF(GLOB_ArcMultiPlayMode)
    settings[GLOB_ArcMultiPlayMode].type=MDZ_SWITCH;
    settings[GLOB_ArcMultiPlayMode].label=(char*)"Archive/multi play mode";
    settings[GLOB_ArcMultiPlayMode].description=(char*)"Play only selected entry or full archive/multisong file";
    settings[GLOB_ArcMultiPlayMode].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_ArcMultiPlayMode].sub_family=0;
    settings[GLOB_ArcMultiPlayMode].callback=&optGLOBALChangedC;
    settings[GLOB_ArcMultiPlayMode].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_ArcMultiPlayMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_ArcMultiPlayMode].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_ArcMultiPlayMode].detail.mdz_switch.switch_labels[0]=(char*)"Entry";
    settings[GLOB_ArcMultiPlayMode].detail.mdz_switch.switch_labels[1]=(char*)"Full";
    
    SETTINGS_ID_DEF(GLOB_TruncateNameMode)
    settings[GLOB_TruncateNameMode].type=MDZ_SWITCH;
    settings[GLOB_TruncateNameMode].label=(char*)"Long name truncate";
    settings[GLOB_TruncateNameMode].description=NULL;
    settings[GLOB_TruncateNameMode].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_TruncateNameMode].sub_family=0;
    settings[GLOB_TruncateNameMode].callback=&optGLOBALChangedC;
    settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_labels[0]=(char*)"Head";
    settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_labels[1]=(char*)"Mid.";
    settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_labels[2]=(char*)"Tail";
    
    SETTINGS_ID_DEF(GLOB_TitleFilename)
    settings[GLOB_TitleFilename].label=(char*)"Filename as title";
    settings[GLOB_TitleFilename].description=NULL;
    settings[GLOB_TitleFilename].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_TitleFilename].sub_family=0;
    settings[GLOB_TitleFilename].callback=&optGLOBALChangedC;
    settings[GLOB_TitleFilename].type=MDZ_BOOLSWITCH;
    settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(GLOB_StatsUpload)
    settings[GLOB_StatsUpload].label=(char*)"Send statistics";
    settings[GLOB_StatsUpload].description=NULL;
    settings[GLOB_StatsUpload].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_StatsUpload].sub_family=0;
    settings[GLOB_StatsUpload].callback=&optGLOBALChangedC;
    settings[GLOB_StatsUpload].type=MDZ_BOOLSWITCH;
    settings[GLOB_StatsUpload].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(GLOB_BackgroundMode)
    settings[GLOB_BackgroundMode].type=MDZ_SWITCH;
    settings[GLOB_BackgroundMode].label=(char*)"Background mode";
    settings[GLOB_BackgroundMode].description=(char*)"Off=no background process, Partial=keep playing music, Full=keep app on in background";
    settings[GLOB_BackgroundMode].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_BackgroundMode].sub_family=0;
    settings[GLOB_BackgroundMode].callback=&optGLOBALChangedC;
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_BackgroundMode].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels[1]=(char*)"Partial";
    settings[GLOB_BackgroundMode].detail.mdz_switch.switch_labels[2]=(char*)"Full";
    
    SETTINGS_ID_DEF(GLOB_EnqueueMode)
    settings[GLOB_EnqueueMode].type=MDZ_SWITCH;
    settings[GLOB_EnqueueMode].label=(char*)"Enqueue position";
    settings[GLOB_EnqueueMode].description=NULL;
    settings[GLOB_EnqueueMode].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_EnqueueMode].sub_family=0;
    settings[GLOB_EnqueueMode].callback=&optGLOBALChangedC;
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels[0]=(char*)"First";
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels[1]=(char*)"Current";
    settings[GLOB_EnqueueMode].detail.mdz_switch.switch_labels[2]=(char*)"Last";
    
    SETTINGS_ID_DEF(GLOB_PlayEnqueueAction)
    settings[GLOB_PlayEnqueueAction].type=MDZ_SWITCH;
    settings[GLOB_PlayEnqueueAction].label=(char*)"Default Action";
    settings[GLOB_PlayEnqueueAction].description=NULL;
    settings[GLOB_PlayEnqueueAction].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_PlayEnqueueAction].sub_family=0;
    settings[GLOB_PlayEnqueueAction].callback=&optGLOBALChangedC;
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels[0]=(char*)"Play";
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels[1]=(char*)"Enqueue";
    settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_labels[2]=(char*)"Enq.&Play";
    
    SETTINGS_ID_DEF(GLOB_AfterDownloadAction)
    settings[GLOB_AfterDownloadAction].type=MDZ_SWITCH;
    settings[GLOB_AfterDownloadAction].label=(char*)"Post download action";
    settings[GLOB_AfterDownloadAction].description=NULL;
    settings[GLOB_AfterDownloadAction].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_AfterDownloadAction].sub_family=0;
    settings[GLOB_AfterDownloadAction].callback=&optGLOBALChangedC;
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels[0]=(char*)"Nothing";
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels[1]=(char*)"Enqueue";
    settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_labels[2]=(char*)"Play";
    
    SETTINGS_ID_DEF(GLOB_CoverFlow)
    settings[GLOB_CoverFlow].label=(char*)"Coverflow";
    settings[GLOB_CoverFlow].description=NULL;
    settings[GLOB_CoverFlow].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_CoverFlow].sub_family=0;
    settings[GLOB_CoverFlow].callback=&optGLOBALChangedC;
    settings[GLOB_CoverFlow].type=MDZ_BOOLSWITCH;
    settings[GLOB_CoverFlow].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(GLOB_RecreateSamplesFolder)
    settings[GLOB_RecreateSamplesFolder].label=(char*)"Auto. restore Samples folder";
    settings[GLOB_RecreateSamplesFolder].description=NULL;
    settings[GLOB_RecreateSamplesFolder].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER;
    settings[GLOB_RecreateSamplesFolder].sub_family=0;
    settings[GLOB_RecreateSamplesFolder].callback=&optGLOBALChangedC;
    settings[GLOB_RecreateSamplesFolder].type=MDZ_BOOLSWITCH;
    settings[GLOB_RecreateSamplesFolder].detail.mdz_boolswitch.switch_value=1;
    
    
    /////////////////////////////////////
    //GLOBAL PLAYER PRIORITY
    /////////////////////////////////////
    
    SETTINGS_ID_DEF(GLOB_DefaultMODPlayer)
    settings[GLOB_DefaultMODPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultMODPlayer].label=(char*)"MOD player";
    settings[GLOB_DefaultMODPlayer].description=NULL;
    settings[GLOB_DefaultMODPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_DefaultMODPlayer].sub_family=0;
    settings[GLOB_DefaultMODPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels[0]=(char*)"OMPT";
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels[1]=(char*)"UADE";
    settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_labels[2]=(char*)"XMP";
    
    SETTINGS_ID_DEF(GLOB_DefaultSAPPlayer)
    settings[GLOB_DefaultSAPPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultSAPPlayer].label=(char*)"SAP player";
    settings[GLOB_DefaultSAPPlayer].description=NULL;
    settings[GLOB_DefaultSAPPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_DefaultSAPPlayer].sub_family=0;
    settings[GLOB_DefaultSAPPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_labels[0]=(char*)"ASAP";
    settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_labels[1]=(char*)"GME";
    
    SETTINGS_ID_DEF(GLOB_DefaultSIDPlayer)
    settings[GLOB_DefaultSIDPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultSIDPlayer].label=(char*)"SID player";
    settings[GLOB_DefaultSIDPlayer].description=NULL;
    settings[GLOB_DefaultSIDPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_DefaultSIDPlayer].sub_family=0;
    settings[GLOB_DefaultSIDPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultSIDPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultSIDPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultSIDPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultSIDPlayer].detail.mdz_switch.switch_labels[0]=(char*)"sidplayfp";
    settings[GLOB_DefaultSIDPlayer].detail.mdz_switch.switch_labels[1]=(char*)"WebSID";
    
    SETTINGS_ID_DEF(GLOB_DefaultVGMPlayer)
    settings[GLOB_DefaultVGMPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultVGMPlayer].label=(char*)"VGM player";
    settings[GLOB_DefaultVGMPlayer].description=NULL;
    settings[GLOB_DefaultVGMPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_DefaultVGMPlayer].sub_family=0;
    settings[GLOB_DefaultVGMPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_labels[0]=(char*)"VGM";
    settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_labels[1]=(char*)"GME";
    
    SETTINGS_ID_DEF(GLOB_DefaultYMPlayer)
    settings[GLOB_DefaultYMPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultYMPlayer].label=(char*)"YM player";
    settings[GLOB_DefaultYMPlayer].description=NULL;
    settings[GLOB_DefaultYMPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_DefaultYMPlayer].sub_family=0;
    settings[GLOB_DefaultYMPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultYMPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultYMPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultYMPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultYMPlayer].detail.mdz_switch.switch_labels[0]=(char*)"ZXTune";
    settings[GLOB_DefaultYMPlayer].detail.mdz_switch.switch_labels[1]=(char*)"STSound";
    
    SETTINGS_ID_DEF(GLOB_DefaultNSFPlayer)
    settings[GLOB_DefaultNSFPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultNSFPlayer].label=(char*)"NSF player";
    settings[GLOB_DefaultNSFPlayer].description=NULL;
    settings[GLOB_DefaultNSFPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_DefaultNSFPlayer].sub_family=0;
    settings[GLOB_DefaultNSFPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultNSFPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultNSFPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultNSFPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultNSFPlayer].detail.mdz_switch.switch_labels[0]=(char*)"NSFPLAY";
    settings[GLOB_DefaultNSFPlayer].detail.mdz_switch.switch_labels[1]=(char*)"GME";
    
    SETTINGS_ID_DEF(GLOB_DefaultPT3Player)
    settings[GLOB_DefaultPT3Player].type=MDZ_SWITCH;
    settings[GLOB_DefaultPT3Player].label=(char*)"PT3 player";
    settings[GLOB_DefaultPT3Player].description=NULL;
    settings[GLOB_DefaultPT3Player].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_DefaultPT3Player].sub_family=0;
    settings[GLOB_DefaultPT3Player].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultPT3Player].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultPT3Player].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultPT3Player].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultPT3Player].detail.mdz_switch.switch_labels[0]=(char*)"ZXTune";
    settings[GLOB_DefaultPT3Player].detail.mdz_switch.switch_labels[1]=(char*)"PT3Player";
    
    SETTINGS_ID_DEF(GLOB_DefaultGBSPlayer)
    settings[GLOB_DefaultGBSPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultGBSPlayer].label=(char*)"GBS player";
    settings[GLOB_DefaultGBSPlayer].description=NULL;
    settings[GLOB_DefaultGBSPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_DefaultGBSPlayer].sub_family=0;
    settings[GLOB_DefaultGBSPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultGBSPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultGBSPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultGBSPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultGBSPlayer].detail.mdz_switch.switch_labels[0]=(char*)"GBSPLAY";
    settings[GLOB_DefaultGBSPlayer].detail.mdz_switch.switch_labels[1]=(char*)"GME";
    
    SETTINGS_ID_DEF(GLOB_DefaultKSSPlayer)
    settings[GLOB_DefaultKSSPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultKSSPlayer].label=(char*)"KSS player";
    settings[GLOB_DefaultKSSPlayer].description=NULL;
    settings[GLOB_DefaultKSSPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_DefaultKSSPlayer].sub_family=0;
    settings[GLOB_DefaultKSSPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultKSSPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultKSSPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultKSSPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultKSSPlayer].detail.mdz_switch.switch_labels[0]=(char*)"LIBKSS";
    settings[GLOB_DefaultKSSPlayer].detail.mdz_switch.switch_labels[1]=(char*)"GME";
    
    SETTINGS_ID_DEF(GLOB_Default2SFPlayer)
    settings[GLOB_Default2SFPlayer].type=MDZ_SWITCH;
    settings[GLOB_Default2SFPlayer].label=(char*)"2SF player";
    settings[GLOB_Default2SFPlayer].description=NULL;
    settings[GLOB_Default2SFPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_Default2SFPlayer].sub_family=0;
    settings[GLOB_Default2SFPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_Default2SFPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_Default2SFPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_Default2SFPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_Default2SFPlayer].detail.mdz_switch.switch_labels[0]=(char*)"XSF";
    settings[GLOB_Default2SFPlayer].detail.mdz_switch.switch_labels[1]=(char*)"VIO2SF";
    
    SETTINGS_ID_DEF(GLOB_DefaultMIDIPlayer)
    settings[GLOB_DefaultMIDIPlayer].type=MDZ_SWITCH;
    settings[GLOB_DefaultMIDIPlayer].label=(char*)"MIDI player";
    settings[GLOB_DefaultMIDIPlayer].description=NULL;
    settings[GLOB_DefaultMIDIPlayer].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[GLOB_DefaultMIDIPlayer].sub_family=0;
    settings[GLOB_DefaultMIDIPlayer].callback=&optGLOBALChangedC;
    settings[GLOB_DefaultMIDIPlayer].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_DefaultMIDIPlayer].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_DefaultMIDIPlayer].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_DefaultMIDIPlayer].detail.mdz_switch.switch_labels[0]=(char*)"Timidity";
    settings[GLOB_DefaultMIDIPlayer].detail.mdz_switch.switch_labels[1]=(char*)"AdPlug";
    
    SETTINGS_ID_DEF(ADPLUG_PriorityOMPT)
    settings[ADPLUG_PriorityOMPT].type=MDZ_SWITCH;
    settings[ADPLUG_PriorityOMPT].label=(char*)"Adplug priority for module";
    settings[ADPLUG_PriorityOMPT].description=NULL;
    settings[ADPLUG_PriorityOMPT].family=MDZ_SETTINGS_FAMILY_GLOBAL_PLAYER_PRIORITY;
    settings[ADPLUG_PriorityOMPT].sub_family=0;
    settings[ADPLUG_PriorityOMPT].callback=&optADPLUGChangedC;
    settings[ADPLUG_PriorityOMPT].detail.mdz_switch.switch_value_nb=2;
    settings[ADPLUG_PriorityOMPT].detail.mdz_switch.switch_labels=(char**)malloc(settings[ADPLUG_PriorityOMPT].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[ADPLUG_PriorityOMPT].detail.mdz_switch.switch_labels[0]=(char*)"OMPT";
    settings[ADPLUG_PriorityOMPT].detail.mdz_switch.switch_labels[1]=(char*)"ADPlug";
    
    
    /////////////////////////////////////
    //GLOBAL FTP
    /////////////////////////////////////
    ///
    SETTINGS_ID_DEF(FTP_STATUS)
    settings[FTP_STATUS].label=(char*)"Server status";
    settings[FTP_STATUS].description=NULL;
    settings[FTP_STATUS].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_STATUS].sub_family=0;
    settings[FTP_STATUS].type=MDZ_MSGBOX;
    settings[FTP_STATUS].detail.mdz_msgbox.text=(char*)malloc(strlen("Inactive")+1);
    strcpy(settings[FTP_STATUS].detail.mdz_msgbox.text,"Inactive");
    
    SETTINGS_ID_DEF(FTP_ONOFF)
    settings[FTP_ONOFF].type=MDZ_SWITCH;
    settings[FTP_ONOFF].label=(char*)"FTP Server";
    settings[FTP_ONOFF].description=NULL;
    settings[FTP_ONOFF].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_ONOFF].callback=&optFTPSwitchChanged;
    settings[FTP_ONOFF].sub_family=0;
    settings[FTP_ONOFF].detail.mdz_switch.switch_value_nb=2;
    settings[FTP_ONOFF].detail.mdz_switch.switch_labels=(char**)malloc(settings[FTP_ONOFF].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[FTP_ONOFF].detail.mdz_switch.switch_labels[0]=(char*)"Stop";
    settings[FTP_ONOFF].detail.mdz_switch.switch_labels[1]=(char*)"Run";
    
    SETTINGS_ID_DEF(FTP_ANONYMOUS)
    settings[FTP_ANONYMOUS].label=(char*)"Authorize anonymous";
    settings[FTP_ANONYMOUS].description=NULL;
    settings[FTP_ANONYMOUS].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_ANONYMOUS].sub_family=0;
    settings[FTP_ANONYMOUS].type=MDZ_BOOLSWITCH;
    settings[FTP_ANONYMOUS].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(FTP_USER)
    settings[FTP_USER].label=(char*)"User";
    settings[FTP_USER].description=NULL;
    settings[FTP_USER].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_USER].sub_family=0;
    settings[FTP_USER].type=MDZ_TEXTBOX;
    settings[FTP_USER].detail.mdz_textbox.text=NULL;
    
    SETTINGS_ID_DEF(FTP_PASSWORD)
    settings[FTP_PASSWORD].label=(char*)"Password";
    settings[FTP_PASSWORD].description=NULL;
    settings[FTP_PASSWORD].family=MDZ_SETTINGS_FAMILY_GLOBAL_FTP;
    settings[FTP_PASSWORD].sub_family=0;
    settings[FTP_PASSWORD].type=MDZ_TEXTBOX;
    settings[FTP_PASSWORD].detail.mdz_textbox.text=NULL;
    
    SETTINGS_ID_DEF(FTP_PORT)
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
    SETTINGS_ID_DEF(ONLINE_MODLAND_CURRENT_URL)
    settings[ONLINE_MODLAND_CURRENT_URL].label=(char*)"MODLAND URL";
    settings[ONLINE_MODLAND_CURRENT_URL].description=NULL;
    settings[ONLINE_MODLAND_CURRENT_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_MODLAND_CURRENT_URL].sub_family=0;
    settings[ONLINE_MODLAND_CURRENT_URL].type=MDZ_MSGBOX;
    settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen("N/A")+1);
    strcpy(settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text,"N/A");
    
    SETTINGS_ID_DEF(ONLINE_HVSC_CURRENT_URL)
    settings[ONLINE_HVSC_CURRENT_URL].label=(char*)"HVSC URL";
    settings[ONLINE_HVSC_CURRENT_URL].description=NULL;
    settings[ONLINE_HVSC_CURRENT_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_HVSC_CURRENT_URL].sub_family=0;
    settings[ONLINE_HVSC_CURRENT_URL].type=MDZ_MSGBOX;
    settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen("N/A")+1);
    strcpy(settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text,"N/A");
    
    SETTINGS_ID_DEF(ONLINE_ASMA_CURRENT_URL)
    settings[ONLINE_ASMA_CURRENT_URL].label=(char*)"ASMA URL";
    settings[ONLINE_ASMA_CURRENT_URL].description=NULL;
    settings[ONLINE_ASMA_CURRENT_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_ASMA_CURRENT_URL].sub_family=0;
    settings[ONLINE_ASMA_CURRENT_URL].type=MDZ_MSGBOX;
    settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text=(char*)malloc(strlen("N/A")+1);
    strcpy(settings[ONLINE_ASMA_CURRENT_URL].detail.mdz_msgbox.text,"N/A");
    
    SETTINGS_ID_DEF(ONLINE_MODLAND_URL)
    settings[ONLINE_MODLAND_URL].type=MDZ_SWITCH;
    settings[ONLINE_MODLAND_URL].label=(char*)"MODLAND Server";
    settings[ONLINE_MODLAND_URL].description=NULL;
    settings[ONLINE_MODLAND_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_MODLAND_URL].callback=&optONLINESwitchChanged;
    settings[ONLINE_MODLAND_URL].sub_family=0;
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_value_nb=4;
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_labels=(char**)malloc(settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_labels[0]=(char*)"Default";
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_labels[1]=(char*)"Alt1";
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_labels[2]=(char*)"Alt2";
    settings[ONLINE_MODLAND_URL].detail.mdz_switch.switch_labels[3]=(char*)"Cust";
    
    SETTINGS_ID_DEF(ONLINE_MODLAND_URL_CUSTOM)
    settings[ONLINE_MODLAND_URL_CUSTOM].label=(char*)"MODLAND cust.URL";
    settings[ONLINE_MODLAND_URL_CUSTOM].description=NULL;
    settings[ONLINE_MODLAND_URL_CUSTOM].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_MODLAND_URL_CUSTOM].callback=&optONLINESwitchChanged;
    settings[ONLINE_MODLAND_URL_CUSTOM].sub_family=0;
    settings[ONLINE_MODLAND_URL_CUSTOM].type=MDZ_TEXTBOX;
    settings[ONLINE_MODLAND_URL_CUSTOM].detail.mdz_textbox.text=NULL;
    
    SETTINGS_ID_DEF(ONLINE_HVSC_URL)
    settings[ONLINE_HVSC_URL].type=MDZ_SWITCH;
    settings[ONLINE_HVSC_URL].label=(char*)"HVSC Server";
    settings[ONLINE_HVSC_URL].description=NULL;
    settings[ONLINE_HVSC_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_HVSC_URL].callback=&optONLINESwitchChanged;
    settings[ONLINE_HVSC_URL].sub_family=0;
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_value_nb=4;
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_labels=(char**)malloc(settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_labels[0]=(char*)"Default";
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_labels[1]=(char*)"Alt1";
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_labels[2]=(char*)"Alt2";
    settings[ONLINE_HVSC_URL].detail.mdz_switch.switch_labels[3]=(char*)"Cust";
    
    SETTINGS_ID_DEF(ONLINE_HVSC_URL_CUSTOM)
    settings[ONLINE_HVSC_URL_CUSTOM].label=(char*)"HVSC cust.URL";
    settings[ONLINE_HVSC_URL_CUSTOM].description=NULL;
    settings[ONLINE_HVSC_URL_CUSTOM].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_HVSC_URL_CUSTOM].sub_family=0;
    settings[ONLINE_HVSC_URL_CUSTOM].type=MDZ_TEXTBOX;
    settings[ONLINE_HVSC_URL_CUSTOM].detail.mdz_textbox.text=NULL;
    
    SETTINGS_ID_DEF(ONLINE_ASMA_URL)
    settings[ONLINE_ASMA_URL].type=MDZ_SWITCH;
    settings[ONLINE_ASMA_URL].label=(char*)"ASMA Server";
    settings[ONLINE_ASMA_URL].description=NULL;
    settings[ONLINE_ASMA_URL].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_ASMA_URL].callback=&optONLINESwitchChanged;
    settings[ONLINE_ASMA_URL].sub_family=0;
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_value_nb=4;
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_labels=(char**)malloc(settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_labels[0]=(char*)"Default";
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_labels[1]=(char*)"Alt1";
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_labels[2]=(char*)"Alt2";
    settings[ONLINE_ASMA_URL].detail.mdz_switch.switch_labels[3]=(char*)"Cust";
    
    SETTINGS_ID_DEF(ONLINE_ASMA_URL_CUSTOM)
    settings[ONLINE_ASMA_URL_CUSTOM].label=(char*)"ASMA cust.URL";
    settings[ONLINE_ASMA_URL_CUSTOM].description=NULL;
    settings[ONLINE_ASMA_URL_CUSTOM].family=MDZ_SETTINGS_FAMILY_GLOBAL_ONLINE;
    settings[ONLINE_ASMA_URL_CUSTOM].sub_family=0;
    settings[ONLINE_ASMA_URL_CUSTOM].type=MDZ_TEXTBOX;
    settings[ONLINE_ASMA_URL_CUSTOM].detail.mdz_textbox.text=NULL;
    
    [SettingsGenViewController ONLINEswitchChanged];
    
    /////////////////////////////////////
    //Visualizers
    /////////////////////////////////////
    SETTINGS_ID_DEF(GLOB_FXRandom)
    settings[GLOB_FXRandom].label=(char*)"Random FX";
    settings[GLOB_FXRandom].description=NULL;
    settings[GLOB_FXRandom].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXRandom].sub_family=0;
    settings[GLOB_FXRandom].callback=&optVISUChangedC;
    settings[GLOB_FXRandom].type=MDZ_BOOLSWITCH;
    settings[GLOB_FXRandom].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(GLOB_FXAlpha)
    settings[GLOB_FXAlpha].label=(char*)"FX Alpha";
    settings[GLOB_FXAlpha].description=NULL;
    settings[GLOB_FXAlpha].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXAlpha].sub_family=0;
    settings[GLOB_FXAlpha].callback=&optVISUChangedC;
    settings[GLOB_FXAlpha].type=MDZ_SLIDER_CONTINUOUS;
    settings[GLOB_FXAlpha].detail.mdz_slider.slider_digits=100;
    settings[GLOB_FXAlpha].detail.mdz_slider.slider_min_value=0;
    settings[GLOB_FXAlpha].detail.mdz_slider.slider_max_value=1;
    
    SETTINGS_ID_DEF(GLOB_FXAlphaFS)
    settings[GLOB_FXAlphaFS].label=(char*)"FX Alpha Fullscreen";
    settings[GLOB_FXAlphaFS].description=NULL;
    settings[GLOB_FXAlphaFS].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXAlphaFS].sub_family=0;
    settings[GLOB_FXAlphaFS].callback=&optVISUChangedC;
    settings[GLOB_FXAlphaFS].type=MDZ_SLIDER_CONTINUOUS;
    settings[GLOB_FXAlphaFS].detail.mdz_slider.slider_digits=100;
    settings[GLOB_FXAlphaFS].detail.mdz_slider.slider_min_value=0;
    settings[GLOB_FXAlphaFS].detail.mdz_slider.slider_max_value=1;
    
    SETTINGS_ID_DEF(GLOB_FXOscillo)
    settings[GLOB_FXOscillo].type=MDZ_SWITCH;
    settings[GLOB_FXOscillo].label=(char*)"Oscillo FX";
    settings[GLOB_FXOscillo].description=NULL;
    settings[GLOB_FXOscillo].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXOscillo].sub_family=0;
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_value_nb=4;
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXOscillo].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_labels[1]=(char*)"Multi 1";
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_labels[2]=(char*)"Multi 2";
    settings[GLOB_FXOscillo].detail.mdz_switch.switch_labels[3]=(char*)"Stereo";
    
    SETTINGS_ID_DEF(GLOB_FXSpectrum)
    settings[GLOB_FXSpectrum].type=MDZ_SWITCH;
    settings[GLOB_FXSpectrum].label=(char*)"2D Spectrum";
    settings[GLOB_FXSpectrum].description=NULL;
    settings[GLOB_FXSpectrum].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXSpectrum].sub_family=0;
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FXSpectrum].detail.mdz_switch.switch_labels[2]=(char*)"2";
    
    SETTINGS_ID_DEF(GLOB_FXMODPattern)
    settings[GLOB_FXMODPattern].type=MDZ_SWITCH;
    settings[GLOB_FXMODPattern].label=(char*)"MOD Pattern";
    settings[GLOB_FXMODPattern].description=NULL;
    settings[GLOB_FXMODPattern].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMODPattern].sub_family=0;
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value_nb=7;
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[2]=(char*)"2";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[3]=(char*)"3";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[4]=(char*)"4";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[5]=(char*)"5";
    settings[GLOB_FXMODPattern].detail.mdz_switch.switch_labels[6]=(char*)"6";
    
    SETTINGS_ID_DEF(GLOB_FXMODPattern_CurrentLineMode)
    settings[GLOB_FXMODPattern_CurrentLineMode].type=MDZ_SWITCH;
    settings[GLOB_FXMODPattern_CurrentLineMode].label=(char*)"MOD Current Line";
    settings[GLOB_FXMODPattern_CurrentLineMode].description=NULL;
    settings[GLOB_FXMODPattern_CurrentLineMode].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMODPattern_CurrentLineMode].sub_family=0;
    settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_labels[0]=(char*)"Follow";
    settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_labels[1]=(char*)"Fixed";
    
    SETTINGS_ID_DEF(GLOB_FXMODPattern_Font)
    settings[GLOB_FXMODPattern_Font].type=MDZ_SWITCH;
    settings[GLOB_FXMODPattern_Font].label=(char*)"MOD Pattern Font";
    settings[GLOB_FXMODPattern_Font].description=NULL;
    settings[GLOB_FXMODPattern_Font].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMODPattern_Font].sub_family=0;
    settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value_nb=5;
    settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_labels[0]=(char*)"Amiga";
    settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_labels[1]=(char*)"C64";
    settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_labels[2]=(char*)"GB";
    settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_labels[3]=(char*)"04b";
    settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_labels[4]=(char*)"Trk";
    
    SETTINGS_ID_DEF(GLOB_FXMODPattern_FontSize)
    settings[GLOB_FXMODPattern_FontSize].type=MDZ_SWITCH;
    settings[GLOB_FXMODPattern_FontSize].label=(char*)"MOD Pattern Font Size";
    settings[GLOB_FXMODPattern_FontSize].description=NULL;
    settings[GLOB_FXMODPattern_FontSize].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMODPattern_FontSize].sub_family=0;
    settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value_nb=4;
    settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_labels[0]=(char*)"10";
    settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_labels[1]=(char*)"16";
    settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_labels[2]=(char*)"24";
    settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_labels[3]=(char*)"32";
    
    SETTINGS_ID_DEF(GLOB_FXPianoRoll)
    settings[GLOB_FXPianoRoll].type=MDZ_SWITCH;
    settings[GLOB_FXPianoRoll].label=(char*)"Piano Roll FX";
    settings[GLOB_FXPianoRoll].description=(char*)"Activate Piano roll FX, standard or 'Synthesia' like";
    settings[GLOB_FXPianoRoll].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXPianoRoll].sub_family=0;
    settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_labels[1]=(char*)"Std";
    settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_labels[2]=(char*)"Syn.";
    
    SETTINGS_ID_DEF(GLOB_FXPianoRollSpark)
    settings[GLOB_FXPianoRollSpark].type=MDZ_SWITCH;
    settings[GLOB_FXPianoRollSpark].label=(char*)"PR/Spark FX";
    settings[GLOB_FXPianoRollSpark].description=(char*)"Activate spark FX, colored or white";
    settings[GLOB_FXPianoRollSpark].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXPianoRollSpark].sub_family=0;
    settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_labels[1]=(char*)"Col.";
    settings[GLOB_FXPianoRollSpark].detail.mdz_switch.switch_labels[2]=(char*)"White";
    
    SETTINGS_ID_DEF(GLOB_FXPianoRollVoicesLabels)
    settings[GLOB_FXPianoRollVoicesLabels].label=(char*)"PR/Voices labels";
    settings[GLOB_FXPianoRollVoicesLabels].description=(char*)"Display voices label";
    settings[GLOB_FXPianoRollVoicesLabels].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXPianoRollVoicesLabels].sub_family=0;
    settings[GLOB_FXPianoRollVoicesLabels].type=MDZ_BOOLSWITCH;
    settings[GLOB_FXPianoRollVoicesLabels].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(GLOB_FXPianoRollOctavesLabels)
    settings[GLOB_FXPianoRollOctavesLabels].label=(char*)"PR/Octaves labels";
    settings[GLOB_FXPianoRollOctavesLabels].description=(char*)"Display octaves labels";
    settings[GLOB_FXPianoRollOctavesLabels].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXPianoRollOctavesLabels].sub_family=0;
    settings[GLOB_FXPianoRollOctavesLabels].type=MDZ_BOOLSWITCH;
    settings[GLOB_FXPianoRollOctavesLabels].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(GLOB_FXMIDIPattern)
    settings[GLOB_FXMIDIPattern].type=MDZ_SWITCH;
    settings[GLOB_FXMIDIPattern].label=(char*)"Notes scroller FX";
    settings[GLOB_FXMIDIPattern].description=NULL;
    settings[GLOB_FXMIDIPattern].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMIDIPattern].sub_family=0;
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_labels[1]=(char*)"Hori";
    settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_labels[2]=(char*)"Vert";
    
    SETTINGS_ID_DEF(GLOB_FXMIDICutLine)
    settings[GLOB_FXMIDICutLine].type=MDZ_SWITCH;
    settings[GLOB_FXMIDICutLine].label=(char*)"NS/bars mode";
    settings[GLOB_FXMIDICutLine].description=NULL;
    settings[GLOB_FXMIDICutLine].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMIDICutLine].sub_family=0;
    settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_labels[0]=(char*)"Cut";
    settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_labels[1]=(char*)"Shaded";
    settings[GLOB_FXMIDICutLine].detail.mdz_switch.switch_labels[2]=(char*)"Full";
    
    SETTINGS_ID_DEF(GLOB_FXMIDIBarStyle)
    settings[GLOB_FXMIDIBarStyle].type=MDZ_SWITCH;
    settings[GLOB_FXMIDIBarStyle].label=(char*)"NS/bars style";
    settings[GLOB_FXMIDIBarStyle].description=NULL;
    settings[GLOB_FXMIDIBarStyle].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMIDIBarStyle].sub_family=0;
    settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_labels[0]=(char*)"Flat";
    settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_labels[1]=(char*)"Box";
    settings[GLOB_FXMIDIBarStyle].detail.mdz_switch.switch_labels[2]=(char*)"Piston";
    
    SETTINGS_ID_DEF(GLOB_FXMIDIBarVibrato)
    settings[GLOB_FXMIDIBarVibrato].type=MDZ_SWITCH;
    settings[GLOB_FXMIDIBarVibrato].label=(char*)"Vibrato display";
    settings[GLOB_FXMIDIBarVibrato].description=NULL;
    settings[GLOB_FXMIDIBarVibrato].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMIDIBarVibrato].sub_family=0;
    settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_labels[1]=(char*)"Fixed";
    settings[GLOB_FXMIDIBarVibrato].detail.mdz_switch.switch_labels[2]=(char*)"Anim";
    
    SETTINGS_ID_DEF(GLOB_FXPiano)
    settings[GLOB_FXPiano].type=MDZ_SWITCH;
    settings[GLOB_FXPiano].label=(char*)"Piano FX";
    settings[GLOB_FXPiano].description=NULL;
    settings[GLOB_FXPiano].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXPiano].sub_family=0;
    settings[GLOB_FXPiano].detail.mdz_switch.switch_value_nb=5;
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXPiano].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels[2]=(char*)"2";
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels[3]=(char*)"3";
    settings[GLOB_FXPiano].detail.mdz_switch.switch_labels[4]=(char*)"4";
    
    SETTINGS_ID_DEF(GLOB_FXPianoCutLine)
    settings[GLOB_FXPianoCutLine].type=MDZ_SWITCH;
    settings[GLOB_FXPianoCutLine].label=(char*)"Piano bars mode";
    settings[GLOB_FXPianoCutLine].description=NULL;
    settings[GLOB_FXPianoCutLine].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXPianoCutLine].sub_family=0;
    settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_labels[0]=(char*)"Cut";
    settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_labels[1]=(char*)"Shaded";
    settings[GLOB_FXPianoCutLine].detail.mdz_switch.switch_labels[2]=(char*)"Full";
    
    SETTINGS_ID_DEF(GLOB_FXPianoColorMode)
    settings[GLOB_FXPianoColorMode].type=MDZ_SWITCH;
    settings[GLOB_FXPianoColorMode].label=(char*)"Piano color mode";
    settings[GLOB_FXPianoColorMode].description=NULL;
    settings[GLOB_FXPianoColorMode].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXPianoColorMode].sub_family=0;
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_labels[0]=(char*)"Note";
    settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_labels[1]=(char*)"Instr";
    
    SETTINGS_ID_DEF(GLOB_FX3DSpectrum)
    settings[GLOB_FX3DSpectrum].type=MDZ_SWITCH;
    settings[GLOB_FX3DSpectrum].label=(char*)"3D Spectrum";
    settings[GLOB_FX3DSpectrum].description=NULL;
    settings[GLOB_FX3DSpectrum].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX3DSpectrum].sub_family=0;
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_labels[2]=(char*)"2";
    
    SETTINGS_ID_DEF(GLOB_FX2)
    settings[GLOB_FX2].type=MDZ_SWITCH;
    settings[GLOB_FX2].label=(char*)"FX2";
    settings[GLOB_FX2].description=NULL;
    settings[GLOB_FX2].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX2].sub_family=0;
    settings[GLOB_FX2].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FX2].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FX2].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FX2].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FX2].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FX2].detail.mdz_switch.switch_labels[2]=(char*)"2";
    
    SETTINGS_ID_DEF(GLOB_FX3)
    settings[GLOB_FX3].type=MDZ_SWITCH;
    settings[GLOB_FX3].label=(char*)"FX3";
    settings[GLOB_FX3].description=NULL;
    settings[GLOB_FX3].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX3].sub_family=0;
    settings[GLOB_FX3].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FX3].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FX3].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FX3].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FX3].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FX3].detail.mdz_switch.switch_labels[2]=(char*)"2";
    
    SETTINGS_ID_DEF(GLOB_FX4)
    settings[GLOB_FX4].label=(char*)"FX4";
    settings[GLOB_FX4].description=NULL;
    settings[GLOB_FX4].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX4].sub_family=0;
    settings[GLOB_FX4].type=MDZ_BOOLSWITCH;
    settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(GLOB_FX5)
    settings[GLOB_FX5].type=MDZ_SWITCH;
    settings[GLOB_FX5].label=(char*)"FX5";
    settings[GLOB_FX5].description=NULL;
    settings[GLOB_FX5].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FX5].sub_family=0;
    settings[GLOB_FX5].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FX5].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FX5].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FX5].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GLOB_FX5].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[GLOB_FX5].detail.mdz_switch.switch_labels[2]=(char*)"2";
    
    SETTINGS_ID_DEF(GLOB_FXLOD)
    settings[GLOB_FXLOD].type=MDZ_SWITCH;
    settings[GLOB_FXLOD].label=(char*)"FX Level of details";
    settings[GLOB_FXLOD].description=NULL;
    settings[GLOB_FXLOD].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXLOD].sub_family=0;
    settings[GLOB_FXLOD].detail.mdz_switch.switch_value_nb=3;
    settings[GLOB_FXLOD].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXLOD].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXLOD].detail.mdz_switch.switch_labels[0]=(char*)"Low";
    settings[GLOB_FXLOD].detail.mdz_switch.switch_labels[1]=(char*)"Med";
    settings[GLOB_FXLOD].detail.mdz_switch.switch_labels[2]=(char*)"High";
    
    SETTINGS_ID_DEF(GLOB_FXMSAA)
    settings[GLOB_FXMSAA].label=(char*)"MSAA";
    settings[GLOB_FXMSAA].description=NULL;
    settings[GLOB_FXMSAA].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXMSAA].sub_family=0;
    settings[GLOB_FXMSAA].type=MDZ_BOOLSWITCH;
    settings[GLOB_FXMSAA].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(GLOB_FXFPS)
    settings[GLOB_FXFPS].type=MDZ_SWITCH;
    settings[GLOB_FXFPS].label=(char*)"FX FPS";
    settings[GLOB_FXFPS].description=NULL;
    settings[GLOB_FXFPS].family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
    settings[GLOB_FXFPS].sub_family=0;
    settings[GLOB_FXFPS].detail.mdz_switch.switch_value_nb=2;
    settings[GLOB_FXFPS].detail.mdz_switch.switch_labels=(char**)malloc(settings[GLOB_FXFPS].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GLOB_FXFPS].detail.mdz_switch.switch_labels[0]=(char*)"30";
    settings[GLOB_FXFPS].detail.mdz_switch.switch_labels[1]=(char*)"60";
    
    /////////////////////////////////////
    //OSCILLO
    /////////////////////////////////////
    ///
    SETTINGS_ID_DEF(OSCILLO_ShowLabel)
    settings[OSCILLO_ShowLabel].type=MDZ_BOOLSWITCH;
    settings[OSCILLO_ShowLabel].label=(char*)"Oscillo show labels";
    settings[OSCILLO_ShowLabel].description=NULL;
    settings[OSCILLO_ShowLabel].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_ShowLabel].sub_family=0;
    
    SETTINGS_ID_DEF(OSCILLO_LabelFontSize)
    settings[OSCILLO_LabelFontSize].type=MDZ_SWITCH;
    settings[OSCILLO_LabelFontSize].label=(char*)"Oscillo labels font size";
    settings[OSCILLO_LabelFontSize].description=NULL;
    settings[OSCILLO_LabelFontSize].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_LabelFontSize].sub_family=0;
    settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value_nb=3;
    settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_labels=(char**)malloc(settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_labels[0]=(char*)"10";
    settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_labels[1]=(char*)"16";
    settings[OSCILLO_LabelFontSize].detail.mdz_switch.switch_labels[2]=(char*)"24";
    
    SETTINGS_ID_DEF(OSCILLO_ShowGrid)
    settings[OSCILLO_ShowGrid].type=MDZ_BOOLSWITCH;
    settings[OSCILLO_ShowGrid].label=(char*)"Oscillo show grid";
    settings[OSCILLO_ShowGrid].description=NULL;
    settings[OSCILLO_ShowGrid].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_ShowGrid].sub_family=0;
    
    SETTINGS_ID_DEF(OSCILLO_GRID_COLOR)
    settings[OSCILLO_GRID_COLOR].label=(char*)"Grid color";
    settings[OSCILLO_GRID_COLOR].description=NULL;
    settings[OSCILLO_GRID_COLOR].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_GRID_COLOR].sub_family=0;
    settings[OSCILLO_GRID_COLOR].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_GRID_COLOR].type=MDZ_COLORPICKER;
    
    SETTINGS_ID_DEF(OSCILLO_LINE_Width)
    settings[OSCILLO_LINE_Width].type=MDZ_SWITCH;
    settings[OSCILLO_LINE_Width].label=(char*)"Oscillo line width";
    settings[OSCILLO_LINE_Width].description=NULL;
    settings[OSCILLO_LINE_Width].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_LINE_Width].sub_family=0;
    settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value_nb=3;
    settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_labels=(char**)malloc(settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_labels[0]=(char*)"Thin";
    settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_labels[1]=(char*)"Medium";
    settings[OSCILLO_LINE_Width].detail.mdz_switch.switch_labels[2]=(char*)"Thick";
    
    SETTINGS_ID_DEF(OSCILLO_MONO_COLOR)
    settings[OSCILLO_MONO_COLOR].label=(char*)"Mono color";
    settings[OSCILLO_MONO_COLOR].description=NULL;
    settings[OSCILLO_MONO_COLOR].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MONO_COLOR].sub_family=0;
    settings[OSCILLO_MONO_COLOR].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MONO_COLOR].type=MDZ_COLORPICKER;

    SETTINGS_ID_DEF(OSCILLO_MULTI_COLORSET)
    settings[OSCILLO_MULTI_COLORSET].type=MDZ_SWITCH;
    settings[OSCILLO_MULTI_COLORSET].label=(char*)"Colors set";
    settings[OSCILLO_MULTI_COLORSET].description=(char*)"Choose between 2 standard sets or customized colors";;
    settings[OSCILLO_MULTI_COLORSET].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MULTI_COLORSET].sub_family=0;
    settings[OSCILLO_MULTI_COLORSET].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MULTI_COLORSET].detail.mdz_switch.switch_value_nb=4;
    settings[OSCILLO_MULTI_COLORSET].detail.mdz_switch.switch_labels=(char**)malloc(settings[OSCILLO_MULTI_COLORSET].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[OSCILLO_MULTI_COLORSET].detail.mdz_switch.switch_labels[0]=(char*)"Set 1";
    settings[OSCILLO_MULTI_COLORSET].detail.mdz_switch.switch_labels[1]=(char*)"Set 2";
    settings[OSCILLO_MULTI_COLORSET].detail.mdz_switch.switch_labels[2]=(char*)"Set 3";
    settings[OSCILLO_MULTI_COLORSET].detail.mdz_switch.switch_labels[3]=(char*)"Cust";
    
    SETTINGS_ID_DEF(OSCILLO_MULTI_RESETALL)
    settings[OSCILLO_MULTI_RESETALL].label=(char*)"Reset all";
    settings[OSCILLO_MULTI_RESETALL].description=(char*)"Reset all custom colors";
    settings[OSCILLO_MULTI_RESETALL].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MULTI_RESETALL].sub_family=0;
    settings[OSCILLO_MULTI_RESETALL].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MULTI_RESETALL].mdz_selector=@selector(resetAllColor:);
    settings[OSCILLO_MULTI_RESETALL].type=MDZ_BUTTON;
    
    SETTINGS_ID_DEF(OSCILLO_MULTI_COLOR01)
    settings[OSCILLO_MULTI_COLOR01].label=(char*)"Multi color | System 1";
    settings[OSCILLO_MULTI_COLOR01].description=NULL;
    settings[OSCILLO_MULTI_COLOR01].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MULTI_COLOR01].sub_family=0;
    settings[OSCILLO_MULTI_COLOR01].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MULTI_COLOR01].type=MDZ_COLORPICKER;
    
    SETTINGS_ID_DEF(OSCILLO_MULTI_COLOR02)
    settings[OSCILLO_MULTI_COLOR02].label=(char*)"Multi color | System 2";
    settings[OSCILLO_MULTI_COLOR02].description=NULL;
    settings[OSCILLO_MULTI_COLOR02].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MULTI_COLOR02].sub_family=0;
    settings[OSCILLO_MULTI_COLOR02].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MULTI_COLOR02].type=MDZ_COLORPICKER;
    
    SETTINGS_ID_DEF(OSCILLO_MULTI_COLOR03)
    settings[OSCILLO_MULTI_COLOR03].label=(char*)"Multi color | System 3";
    settings[OSCILLO_MULTI_COLOR03].description=NULL;
    settings[OSCILLO_MULTI_COLOR03].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MULTI_COLOR03].sub_family=0;
    settings[OSCILLO_MULTI_COLOR03].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MULTI_COLOR03].type=MDZ_COLORPICKER;
    
    SETTINGS_ID_DEF(OSCILLO_MULTI_COLOR04)
    settings[OSCILLO_MULTI_COLOR04].label=(char*)"Multi color | System 4";
    settings[OSCILLO_MULTI_COLOR04].description=NULL;
    settings[OSCILLO_MULTI_COLOR04].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MULTI_COLOR04].sub_family=0;
    settings[OSCILLO_MULTI_COLOR04].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MULTI_COLOR04].type=MDZ_COLORPICKER;
    
    SETTINGS_ID_DEF(OSCILLO_MULTI_COLOR05)
    settings[OSCILLO_MULTI_COLOR05].label=(char*)"Multi color | System 5";
    settings[OSCILLO_MULTI_COLOR05].description=NULL;
    settings[OSCILLO_MULTI_COLOR05].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MULTI_COLOR05].sub_family=0;
    settings[OSCILLO_MULTI_COLOR05].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MULTI_COLOR05].type=MDZ_COLORPICKER;
    
    SETTINGS_ID_DEF(OSCILLO_MULTI_COLOR06)
    settings[OSCILLO_MULTI_COLOR06].label=(char*)"Multi color | System 6";
    settings[OSCILLO_MULTI_COLOR06].description=NULL;
    settings[OSCILLO_MULTI_COLOR06].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MULTI_COLOR06].sub_family=0;
    settings[OSCILLO_MULTI_COLOR06].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MULTI_COLOR06].type=MDZ_COLORPICKER;
    
    SETTINGS_ID_DEF(OSCILLO_MULTI_COLOR07)
    settings[OSCILLO_MULTI_COLOR07].label=(char*)"Multi color | System 7";
    settings[OSCILLO_MULTI_COLOR07].description=NULL;
    settings[OSCILLO_MULTI_COLOR07].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MULTI_COLOR07].sub_family=0;
    settings[OSCILLO_MULTI_COLOR07].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MULTI_COLOR07].type=MDZ_COLORPICKER;
    
    SETTINGS_ID_DEF(OSCILLO_MULTI_COLOR08)
    settings[OSCILLO_MULTI_COLOR08].label=(char*)"Multi color | System 8";
    settings[OSCILLO_MULTI_COLOR08].description=NULL;
    settings[OSCILLO_MULTI_COLOR08].family=MDZ_SETTINGS_FAMILY_OSCILLO;
    settings[OSCILLO_MULTI_COLOR08].sub_family=0;
    settings[OSCILLO_MULTI_COLOR08].callback=&optOSCILLOColorChangedC;
    settings[OSCILLO_MULTI_COLOR08].type=MDZ_COLORPICKER;
    
    
    /////////////////////////////////////
    //PR & NS Colors
    /////////////////////////////////////
    ///
    
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLORSET)
    settings[PIANOMIDI_MULTI_COLORSET].type=MDZ_SWITCH;
    settings[PIANOMIDI_MULTI_COLORSET].label=(char*)"Colors set";
    settings[PIANOMIDI_MULTI_COLORSET].description=(char*)"Choose between 2 standard sets or customized colors";
    settings[PIANOMIDI_MULTI_COLORSET].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLORSET].sub_family=0;
    settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_value_nb=4;
    settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_labels=(char**)malloc(settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_labels[0]=(char*)"Set 1";
    settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_labels[1]=(char*)"Set 2";
    settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_labels[2]=(char*)"Set 3";
    settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_labels[3]=(char*)"Cust.";
    
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_RESETALL)
    settings[PIANOMIDI_MULTI_RESETALL].label=(char*)"Reset all";
    settings[PIANOMIDI_MULTI_RESETALL].description=(char*)"Reset all custom colors";
    settings[PIANOMIDI_MULTI_RESETALL].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_RESETALL].sub_family=0;
    settings[PIANOMIDI_MULTI_RESETALL].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_RESETALL].mdz_selector=@selector(resetAllColor:);
    settings[PIANOMIDI_MULTI_RESETALL].type=MDZ_BUTTON;
    
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR01)
    settings[PIANOMIDI_MULTI_COLOR01].label=(char*)"Custom color 01";
    settings[PIANOMIDI_MULTI_COLOR01].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR01].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR01].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR01].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR01].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR02)
    settings[PIANOMIDI_MULTI_COLOR02].label=(char*)"Custom color 02";
    settings[PIANOMIDI_MULTI_COLOR02].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR02].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR02].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR02].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR02].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR03)
    settings[PIANOMIDI_MULTI_COLOR03].label=(char*)"Custom color 03";
    settings[PIANOMIDI_MULTI_COLOR03].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR03].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR03].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR03].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR03].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR04)
    settings[PIANOMIDI_MULTI_COLOR04].label=(char*)"Custom color 04";
    settings[PIANOMIDI_MULTI_COLOR04].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR04].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR04].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR04].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR04].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR05)
    settings[PIANOMIDI_MULTI_COLOR05].label=(char*)"Custom color 05";
    settings[PIANOMIDI_MULTI_COLOR05].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR05].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR05].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR05].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR05].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR06)
    settings[PIANOMIDI_MULTI_COLOR06].label=(char*)"Custom color 06";
    settings[PIANOMIDI_MULTI_COLOR06].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR06].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR06].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR06].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR06].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR07)
    settings[PIANOMIDI_MULTI_COLOR07].label=(char*)"Custom color 07";
    settings[PIANOMIDI_MULTI_COLOR07].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR07].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR07].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR07].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR07].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR08)
    settings[PIANOMIDI_MULTI_COLOR08].label=(char*)"Custom color 08";
    settings[PIANOMIDI_MULTI_COLOR08].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR08].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR08].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR08].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR08].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR09)
    settings[PIANOMIDI_MULTI_COLOR09].label=(char*)"Custom color 09";
    settings[PIANOMIDI_MULTI_COLOR09].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR09].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR09].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR09].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR09].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR10)
    settings[PIANOMIDI_MULTI_COLOR10].label=(char*)"Custom color 10";
    settings[PIANOMIDI_MULTI_COLOR10].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR10].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR10].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR10].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR10].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR11)
    settings[PIANOMIDI_MULTI_COLOR11].label=(char*)"Custom color 11";
    settings[PIANOMIDI_MULTI_COLOR11].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR11].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR11].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR11].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR11].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR12)
    settings[PIANOMIDI_MULTI_COLOR12].label=(char*)"Custom color 12";
    settings[PIANOMIDI_MULTI_COLOR12].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR12].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR12].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR12].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR12].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR13)
    settings[PIANOMIDI_MULTI_COLOR13].label=(char*)"Custom color 13";
    settings[PIANOMIDI_MULTI_COLOR13].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR13].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR13].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR13].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR13].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR14)
    settings[PIANOMIDI_MULTI_COLOR14].label=(char*)"Custom color 14";
    settings[PIANOMIDI_MULTI_COLOR14].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR14].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR14].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR14].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR14].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR15)
    settings[PIANOMIDI_MULTI_COLOR15].label=(char*)"Custom color 15";
    settings[PIANOMIDI_MULTI_COLOR15].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR15].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR15].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR15].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR15].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR16)
    settings[PIANOMIDI_MULTI_COLOR16].label=(char*)"Custom color 16";
    settings[PIANOMIDI_MULTI_COLOR16].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR16].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR16].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR16].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR16].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR17)
    settings[PIANOMIDI_MULTI_COLOR17].label=(char*)"Custom color 17";
    settings[PIANOMIDI_MULTI_COLOR17].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR17].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR17].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR17].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR17].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR18)
    settings[PIANOMIDI_MULTI_COLOR18].label=(char*)"Custom color 18";
    settings[PIANOMIDI_MULTI_COLOR18].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR18].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR18].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR18].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR18].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR19)
    settings[PIANOMIDI_MULTI_COLOR19].label=(char*)"Custom color 19";
    settings[PIANOMIDI_MULTI_COLOR19].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR19].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR19].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR19].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR19].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR20)
    settings[PIANOMIDI_MULTI_COLOR20].label=(char*)"Custom color 20";
    settings[PIANOMIDI_MULTI_COLOR20].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR20].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR20].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR20].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR20].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR21)
    settings[PIANOMIDI_MULTI_COLOR21].label=(char*)"Custom color 21";
    settings[PIANOMIDI_MULTI_COLOR21].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR21].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR21].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR21].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR21].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR22)
    settings[PIANOMIDI_MULTI_COLOR22].label=(char*)"Custom color 22";
    settings[PIANOMIDI_MULTI_COLOR22].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR22].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR22].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR22].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR22].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR23)
    settings[PIANOMIDI_MULTI_COLOR23].label=(char*)"Custom color 23";
    settings[PIANOMIDI_MULTI_COLOR23].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR23].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR23].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR23].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR23].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR24)
    settings[PIANOMIDI_MULTI_COLOR24].label=(char*)"Custom color 24";
    settings[PIANOMIDI_MULTI_COLOR24].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR24].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR24].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR24].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR24].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR25)
    settings[PIANOMIDI_MULTI_COLOR25].label=(char*)"Custom color 25";
    settings[PIANOMIDI_MULTI_COLOR25].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR25].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR25].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR25].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR25].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR26)
    settings[PIANOMIDI_MULTI_COLOR26].label=(char*)"Custom color 26";
    settings[PIANOMIDI_MULTI_COLOR26].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR26].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR26].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR26].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR26].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR27)
    settings[PIANOMIDI_MULTI_COLOR27].label=(char*)"Custom color 27";
    settings[PIANOMIDI_MULTI_COLOR27].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR27].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR27].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR27].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR27].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR28)
    settings[PIANOMIDI_MULTI_COLOR28].label=(char*)"Custom color 28";
    settings[PIANOMIDI_MULTI_COLOR28].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR28].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR28].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR28].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR28].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR29)
    settings[PIANOMIDI_MULTI_COLOR29].label=(char*)"Custom color 29";
    settings[PIANOMIDI_MULTI_COLOR29].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR29].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR29].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR29].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR29].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR30)
    settings[PIANOMIDI_MULTI_COLOR30].label=(char*)"Custom color 30";
    settings[PIANOMIDI_MULTI_COLOR30].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR30].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR30].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR30].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR30].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR31)
    settings[PIANOMIDI_MULTI_COLOR31].label=(char*)"Custom color 31";
    settings[PIANOMIDI_MULTI_COLOR31].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR31].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR31].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR31].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR31].type=MDZ_COLORPICKER;
    SETTINGS_ID_DEF(PIANOMIDI_MULTI_COLOR32)
    settings[PIANOMIDI_MULTI_COLOR32].label=(char*)"Custom color 32";
    settings[PIANOMIDI_MULTI_COLOR32].description=NULL;
    settings[PIANOMIDI_MULTI_COLOR32].family=MDZ_SETTINGS_FAMILY_PIANOMIDI_COL;
    settings[PIANOMIDI_MULTI_COLOR32].sub_family=0;
    settings[PIANOMIDI_MULTI_COLOR32].callback=&optPIANOMIDIColorChangedC;
    settings[PIANOMIDI_MULTI_COLOR32].type=MDZ_COLORPICKER;
    
    /////////////////////////////////////
    //PLUGINS
    /////////////////////////////////////
    
    /////////////////////////////////////
    //OMPT
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_OMPT)
    settings[MDZ_SETTINGS_FAMILY_OMPT].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_OMPT].label=(char*)"OpenMPT";
    settings[MDZ_SETTINGS_FAMILY_OMPT].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_OMPT].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_OMPT].sub_family=MDZ_SETTINGS_FAMILY_OMPT;
    
    SETTINGS_ID_DEF(OMPT_MasterVolume)
    settings[OMPT_MasterVolume].label=(char*)"Master volume";
    settings[OMPT_MasterVolume].description=NULL;
    settings[OMPT_MasterVolume].family=MDZ_SETTINGS_FAMILY_OMPT;
    settings[OMPT_MasterVolume].sub_family=0;
    settings[OMPT_MasterVolume].callback=&optOMPTChangedC;
    settings[OMPT_MasterVolume].type=MDZ_SLIDER_CONTINUOUS;
    settings[OMPT_MasterVolume].detail.mdz_slider.slider_digits=100;
    settings[OMPT_MasterVolume].detail.mdz_slider.slider_min_value=0;
    settings[OMPT_MasterVolume].detail.mdz_slider.slider_max_value=1;
    
    SETTINGS_ID_DEF(OMPT_Sampling)
    settings[OMPT_Sampling].type=MDZ_SWITCH;
    settings[OMPT_Sampling].label=(char*)"Interpolation";
    settings[OMPT_Sampling].description=NULL;
    settings[OMPT_Sampling].family=MDZ_SETTINGS_FAMILY_OMPT;
    settings[OMPT_Sampling].sub_family=0;
    settings[OMPT_Sampling].callback=&optOMPTChangedC;
    settings[OMPT_Sampling].detail.mdz_switch.switch_value_nb=5;
    settings[OMPT_Sampling].detail.mdz_switch.switch_labels=(char**)malloc(settings[OMPT_Sampling].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[OMPT_Sampling].detail.mdz_switch.switch_labels[0]=(char*)"Def.";
    settings[OMPT_Sampling].detail.mdz_switch.switch_labels[1]=(char*)"Near.";
    settings[OMPT_Sampling].detail.mdz_switch.switch_labels[2]=(char*)"Lin.";
    settings[OMPT_Sampling].detail.mdz_switch.switch_labels[3]=(char*)"Cub.";
    settings[OMPT_Sampling].detail.mdz_switch.switch_labels[4]=(char*)"Win.";
    
    SETTINGS_ID_DEF(OMPT_StereoSeparation)
    settings[OMPT_StereoSeparation].label=(char*)"Panning (Stereo separation)";
    settings[OMPT_StereoSeparation].description=NULL;
    settings[OMPT_StereoSeparation].family=MDZ_SETTINGS_FAMILY_OMPT;
    settings[OMPT_StereoSeparation].sub_family=0;
    settings[OMPT_StereoSeparation].callback=&optOMPTChangedC;
    settings[OMPT_StereoSeparation].type=MDZ_SLIDER_CONTINUOUS;
    settings[OMPT_StereoSeparation].detail.mdz_slider.slider_digits=100;
    settings[OMPT_StereoSeparation].detail.mdz_slider.slider_min_value=0;
    settings[OMPT_StereoSeparation].detail.mdz_slider.slider_max_value=1;
    
    
    
    /////////////////////////////////////
    //GME
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_GME)
    settings[MDZ_SETTINGS_FAMILY_GME].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GME].label=(char*)"GME";
    settings[MDZ_SETTINGS_FAMILY_GME].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GME].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_GME].sub_family=MDZ_SETTINGS_FAMILY_GME;
    
    SETTINGS_ID_DEF(GME_IGNORESILENCE)
    settings[GME_IGNORESILENCE].type=MDZ_BOOLSWITCH;
    settings[GME_IGNORESILENCE].label=(char*)"Silence detection";
    settings[GME_IGNORESILENCE].description=(char*)"Will skip song if nothing is played for specified duration (0=off)";
    settings[GME_IGNORESILENCE].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_IGNORESILENCE].sub_family=0;
    settings[GME_IGNORESILENCE].callback=&optGMEChangedC;
    settings[GME_IGNORESILENCE].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(GME_EQ_ONOFF)
    settings[GME_EQ_ONOFF].type=MDZ_BOOLSWITCH;
    settings[GME_EQ_ONOFF].label=(char*)"Equalizer";
    settings[GME_EQ_ONOFF].description=NULL;
    settings[GME_EQ_ONOFF].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_EQ_ONOFF].sub_family=0;
    settings[GME_EQ_ONOFF].callback=&optGMEChangedC;
    
    SETTINGS_ID_DEF(GME_EQ_BASS)
    settings[GME_EQ_BASS].label=(char*)"Bass";
    settings[GME_EQ_BASS].description=NULL;
    settings[GME_EQ_BASS].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_EQ_BASS].sub_family=0;
    settings[GME_EQ_BASS].callback=&optGMEChangedC;
    settings[GME_EQ_BASS].type=MDZ_SLIDER_CONTINUOUS;
    settings[GME_EQ_BASS].detail.mdz_slider.slider_digits=1;
    settings[GME_EQ_BASS].detail.mdz_slider.slider_min_value=0;
    settings[GME_EQ_BASS].detail.mdz_slider.slider_max_value=4.2;
    
    SETTINGS_ID_DEF(GME_EQ_TREBLE)
    settings[GME_EQ_TREBLE].label=(char*)"Treble";
    settings[GME_EQ_TREBLE].description=NULL;
    settings[GME_EQ_TREBLE].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_EQ_TREBLE].sub_family=0;
    settings[GME_EQ_TREBLE].callback=&optGMEChangedC;
    settings[GME_EQ_TREBLE].type=MDZ_SLIDER_CONTINUOUS;
    settings[GME_EQ_TREBLE].detail.mdz_slider.slider_digits=1;
    settings[GME_EQ_TREBLE].detail.mdz_slider.slider_min_value=-50;
    settings[GME_EQ_TREBLE].detail.mdz_slider.slider_max_value=5;
    
    SETTINGS_ID_DEF(GME_STEREO_PANNING)
    settings[GME_STEREO_PANNING].label=(char*)"Panning (Stereo separation)";
    settings[GME_STEREO_PANNING].description=NULL;
    settings[GME_STEREO_PANNING].family=MDZ_SETTINGS_FAMILY_GME;
    settings[GME_STEREO_PANNING].sub_family=0;
    settings[GME_STEREO_PANNING].callback=&optGMEChangedC;
    settings[GME_STEREO_PANNING].type=MDZ_SLIDER_CONTINUOUS;
    settings[GME_STEREO_PANNING].detail.mdz_slider.slider_digits=100;
    settings[GME_STEREO_PANNING].detail.mdz_slider.slider_min_value=0;
    settings[GME_STEREO_PANNING].detail.mdz_slider.slider_max_value=1;
    
    /////////////////////////////////////
    //GSF
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_GSF)
    settings[MDZ_SETTINGS_FAMILY_GSF].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GSF].label=(char*)"GSF";
    settings[MDZ_SETTINGS_FAMILY_GSF].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GSF].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_GSF].sub_family=MDZ_SETTINGS_FAMILY_GSF;
    
    SETTINGS_ID_DEF(GSF_SOUNDQUALITY)
    settings[GSF_SOUNDQUALITY].type=MDZ_SWITCH;
    settings[GSF_SOUNDQUALITY].label=(char*)"Quality";
    settings[GSF_SOUNDQUALITY].description=NULL;
    settings[GSF_SOUNDQUALITY].family=MDZ_SETTINGS_FAMILY_GSF;
    settings[GSF_SOUNDQUALITY].sub_family=0;
    settings[GSF_SOUNDQUALITY].callback=&optGSFChangedC;
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_value_nb=3;
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_labels=(char**)malloc(settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_labels[0]=(char*)"11Khz";
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_labels[1]=(char*)"22Khz";
    settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_labels[2]=(char*)"44Khz";
    
    SETTINGS_ID_DEF(GSF_INTERPOLATION)
    settings[GSF_INTERPOLATION].type=MDZ_BOOLSWITCH;
    settings[GSF_INTERPOLATION].label=(char*)"Interpolation";
    settings[GSF_INTERPOLATION].description=NULL;
    settings[GSF_INTERPOLATION].family=MDZ_SETTINGS_FAMILY_GSF;
    settings[GSF_INTERPOLATION].sub_family=0;
    settings[GSF_INTERPOLATION].callback=&optGSFChangedC;
    settings[GSF_INTERPOLATION].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(GSF_LOWPASSFILTER)
    settings[GSF_LOWPASSFILTER].type=MDZ_BOOLSWITCH;
    settings[GSF_LOWPASSFILTER].label=(char*)"LP filter";
    settings[GSF_LOWPASSFILTER].description=NULL;
    settings[GSF_LOWPASSFILTER].family=MDZ_SETTINGS_FAMILY_GSF;
    settings[GSF_LOWPASSFILTER].sub_family=0;
    settings[GSF_LOWPASSFILTER].callback=&optGSFChangedC;
    settings[GSF_LOWPASSFILTER].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(GSF_ECHO)
    settings[GSF_ECHO].type=MDZ_BOOLSWITCH;
    settings[GSF_ECHO].label=(char*)"Echo";
    settings[GSF_ECHO].description=NULL;
    settings[GSF_ECHO].family=MDZ_SETTINGS_FAMILY_GSF;
    settings[GSF_ECHO].sub_family=0;
    settings[GSF_ECHO].callback=&optGSFChangedC;
    settings[GSF_ECHO].detail.mdz_boolswitch.switch_value=0;
    
    /////////////////////////////////////
    //NSFPLAY
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_NSFPLAY)
    settings[MDZ_SETTINGS_FAMILY_NSFPLAY].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_NSFPLAY].label=(char*)"NSFPLAY";
    settings[MDZ_SETTINGS_FAMILY_NSFPLAY].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_NSFPLAY].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_NSFPLAY].sub_family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    
    SETTINGS_ID_DEF(NSFPLAY_DefaultLength)
    settings[NSFPLAY_DefaultLength].label=(char*)"Default length";
    settings[NSFPLAY_DefaultLength].description=NULL;
    settings[NSFPLAY_DefaultLength].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_DefaultLength].sub_family=0;
    settings[NSFPLAY_DefaultLength].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_DefaultLength].type=MDZ_SLIDER_DISCRETE_TIME_LONG;
    settings[NSFPLAY_DefaultLength].detail.mdz_slider.slider_digits=60;
    settings[NSFPLAY_DefaultLength].detail.mdz_slider.slider_min_value=10;
    settings[NSFPLAY_DefaultLength].detail.mdz_slider.slider_mid_value=60*10;
    settings[NSFPLAY_DefaultLength].detail.mdz_slider.slider_max_value=60*60;
    
    SETTINGS_ID_DEF(NSFPLAY_Quality)
    settings[NSFPLAY_Quality].label=(char*)"Quality";
    settings[NSFPLAY_Quality].description=(char*)"Oversampling. Min=0, default: 10";
    settings[NSFPLAY_Quality].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_Quality].sub_family=0;
    settings[NSFPLAY_Quality].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_Quality].type=MDZ_SLIDER_DISCRETE;
    settings[NSFPLAY_Quality].detail.mdz_slider.slider_digits=0;
    settings[NSFPLAY_Quality].detail.mdz_slider.slider_min_value=0;
    settings[NSFPLAY_Quality].detail.mdz_slider.slider_max_value=40;
    
    SETTINGS_ID_DEF(NSFPLAY_LowPass_Filter_Strength)
    settings[NSFPLAY_LowPass_Filter_Strength].label=(char*)"LP filter";
    settings[NSFPLAY_LowPass_Filter_Strength].description=(char*)"Min=off, default: 112";
    settings[NSFPLAY_LowPass_Filter_Strength].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_LowPass_Filter_Strength].sub_family=0;
    settings[NSFPLAY_LowPass_Filter_Strength].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_LowPass_Filter_Strength].type=MDZ_SLIDER_DISCRETE;
    settings[NSFPLAY_LowPass_Filter_Strength].detail.mdz_slider.slider_digits=0;
    settings[NSFPLAY_LowPass_Filter_Strength].detail.mdz_slider.slider_min_value=0;
    settings[NSFPLAY_LowPass_Filter_Strength].detail.mdz_slider.slider_max_value=400;
    
    SETTINGS_ID_DEF(NSFPLAY_HighPass_Filter_Strength)
    settings[NSFPLAY_HighPass_Filter_Strength].label=(char*)"HP filter";
    settings[NSFPLAY_HighPass_Filter_Strength].description=(char*)"Max=off, default: 164";
    settings[NSFPLAY_HighPass_Filter_Strength].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_HighPass_Filter_Strength].sub_family=0;
    settings[NSFPLAY_HighPass_Filter_Strength].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_HighPass_Filter_Strength].type=MDZ_SLIDER_DISCRETE;
    settings[NSFPLAY_HighPass_Filter_Strength].detail.mdz_slider.slider_digits=0;
    settings[NSFPLAY_HighPass_Filter_Strength].detail.mdz_slider.slider_min_value=0;
    settings[NSFPLAY_HighPass_Filter_Strength].detail.mdz_slider.slider_max_value=256;
    
    SETTINGS_ID_DEF(NSFPLAY_Region)
    settings[NSFPLAY_Region].type=MDZ_SWITCH;
    settings[NSFPLAY_Region].label=(char*)"Region";
    settings[NSFPLAY_Region].description=(char*)"1/2/3=NTSC/PAL/DENDY\n4/5/6=force NTSC/PAL/DENDY";
    settings[NSFPLAY_Region].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_Region].sub_family=0;
    settings[NSFPLAY_Region].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_Region].detail.mdz_switch.switch_value_nb=7;
    settings[NSFPLAY_Region].detail.mdz_switch.switch_labels=(char**)malloc(settings[NSFPLAY_Region].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[NSFPLAY_Region].detail.mdz_switch.switch_labels[0]=(char*)"Auto";
    settings[NSFPLAY_Region].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[NSFPLAY_Region].detail.mdz_switch.switch_labels[2]=(char*)"2";
    settings[NSFPLAY_Region].detail.mdz_switch.switch_labels[3]=(char*)"3";
    settings[NSFPLAY_Region].detail.mdz_switch.switch_labels[4]=(char*)"4";
    settings[NSFPLAY_Region].detail.mdz_switch.switch_labels[5]=(char*)"5";
    settings[NSFPLAY_Region].detail.mdz_switch.switch_labels[6]=(char*)"6";
    
    SETTINGS_ID_DEF(NSFPLAY_ForceIRQ)
    settings[NSFPLAY_ForceIRQ].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_ForceIRQ].label=(char*)"Force IRQ";
    settings[NSFPLAY_ForceIRQ].description=(char*)"Forces IRQ capability for all NSFs, not just NSF2";
    settings[NSFPLAY_ForceIRQ].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_ForceIRQ].sub_family=0;
    settings[NSFPLAY_ForceIRQ].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_APU_OPTION0)
    settings[NSFPLAY_APU_OPTION0].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_APU_OPTION0].label=(char*)"APU unmute on reset";
    settings[NSFPLAY_APU_OPTION0].description=NULL;
    settings[NSFPLAY_APU_OPTION0].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_APU_OPTION0].sub_family=0;
    settings[NSFPLAY_APU_OPTION0].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_APU_OPTION1)
    settings[NSFPLAY_APU_OPTION1].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_APU_OPTION1].label=(char*)"APU phase reset";
    settings[NSFPLAY_APU_OPTION1].description=NULL;
    settings[NSFPLAY_APU_OPTION1].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_APU_OPTION1].sub_family=0;
    settings[NSFPLAY_APU_OPTION1].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_APU_OPTION2)
    settings[NSFPLAY_APU_OPTION2].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_APU_OPTION2].label=(char*)"APU non linear mixing";
    settings[NSFPLAY_APU_OPTION2].description=NULL;
    settings[NSFPLAY_APU_OPTION2].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_APU_OPTION2].sub_family=0;
    settings[NSFPLAY_APU_OPTION2].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_APU_OPTION3)
    settings[NSFPLAY_APU_OPTION3].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_APU_OPTION3].label=(char*)"APU swap duty 1/2";
    settings[NSFPLAY_APU_OPTION3].description=NULL;
    settings[NSFPLAY_APU_OPTION3].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_APU_OPTION3].sub_family=0;
    settings[NSFPLAY_APU_OPTION3].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_APU_OPTION4)
    settings[NSFPLAY_APU_OPTION4].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_APU_OPTION4].label=(char*)"APU Initialize sweep unmute";
    settings[NSFPLAY_APU_OPTION4].description=NULL;
    settings[NSFPLAY_APU_OPTION4].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_APU_OPTION4].sub_family=0;
    settings[NSFPLAY_APU_OPTION4].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_DMC_OPTION0)
    settings[NSFPLAY_DMC_OPTION0].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_DMC_OPTION0].label=(char*)"DMC enable $4011 writes";
    settings[NSFPLAY_DMC_OPTION0].description=NULL;
    settings[NSFPLAY_DMC_OPTION0].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_DMC_OPTION0].sub_family=0;
    settings[NSFPLAY_DMC_OPTION0].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_DMC_OPTION1)
    settings[NSFPLAY_DMC_OPTION1].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_DMC_OPTION1].label=(char*)"DMC enable periodic noise";
    settings[NSFPLAY_DMC_OPTION1].description=NULL;
    settings[NSFPLAY_DMC_OPTION1].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_DMC_OPTION1].sub_family=0;
    settings[NSFPLAY_DMC_OPTION1].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_DMC_OPTION2)
    settings[NSFPLAY_DMC_OPTION2].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_DMC_OPTION2].label=(char*)"DMC unmute on reset";
    settings[NSFPLAY_DMC_OPTION2].description=NULL;
    settings[NSFPLAY_DMC_OPTION2].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_DMC_OPTION2].sub_family=0;
    settings[NSFPLAY_DMC_OPTION2].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_DMC_OPTION3)
    settings[NSFPLAY_DMC_OPTION3].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_DMC_OPTION3].label=(char*)"DMC anti clicking";
    settings[NSFPLAY_DMC_OPTION3].description=NULL;
    settings[NSFPLAY_DMC_OPTION3].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_DMC_OPTION3].sub_family=0;
    settings[NSFPLAY_DMC_OPTION3].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_DMC_OPTION4)
    settings[NSFPLAY_DMC_OPTION4].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_DMC_OPTION4].label=(char*)"DMC non linear mixer";
    settings[NSFPLAY_DMC_OPTION4].description=NULL;
    settings[NSFPLAY_DMC_OPTION4].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_DMC_OPTION4].sub_family=0;
    settings[NSFPLAY_DMC_OPTION4].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_DMC_OPTION5)
    settings[NSFPLAY_DMC_OPTION5].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_DMC_OPTION5].label=(char*)"DMC randomize noise on reset";
    settings[NSFPLAY_DMC_OPTION5].description=NULL;
    settings[NSFPLAY_DMC_OPTION5].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_DMC_OPTION5].sub_family=0;
    settings[NSFPLAY_DMC_OPTION5].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_DMC_OPTION6)
    settings[NSFPLAY_DMC_OPTION6].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_DMC_OPTION6].label=(char*)"DMC mute triangle on pitch 0";
    settings[NSFPLAY_DMC_OPTION6].description=(char*)"prevents high frequency aliasing";
    settings[NSFPLAY_DMC_OPTION6].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_DMC_OPTION6].sub_family=0;
    settings[NSFPLAY_DMC_OPTION6].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_DMC_OPTION7)
    settings[NSFPLAY_DMC_OPTION7].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_DMC_OPTION7].label=(char*)"DMC rand. triangle";
    settings[NSFPLAY_DMC_OPTION7].description=(char*)"randomize triangle on reset";
    settings[NSFPLAY_DMC_OPTION7].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_DMC_OPTION7].sub_family=0;
    settings[NSFPLAY_DMC_OPTION7].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_DMC_OPTION8)
    settings[NSFPLAY_DMC_OPTION8].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_DMC_OPTION8].label=(char*)"DMC reverse bits";
    settings[NSFPLAY_DMC_OPTION8].description=(char*)"reverse bits of DPCM sample bytes";
    settings[NSFPLAY_DMC_OPTION8].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_DMC_OPTION8].sub_family=0;
    settings[NSFPLAY_DMC_OPTION8].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_MMC5_OPTION0)
    settings[NSFPLAY_MMC5_OPTION0].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_MMC5_OPTION0].label=(char*)"MMC5 non linear mixing";
    settings[NSFPLAY_MMC5_OPTION0].description=NULL;
    settings[NSFPLAY_MMC5_OPTION0].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_MMC5_OPTION0].sub_family=0;
    settings[NSFPLAY_MMC5_OPTION0].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_MMC5_OPTION1)
    settings[NSFPLAY_MMC5_OPTION1].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_MMC5_OPTION1].label=(char*)"MMC5 Phase reset";
    settings[NSFPLAY_MMC5_OPTION1].description=(char*)"Reset phase counter after write $5003 and $5007";
    settings[NSFPLAY_MMC5_OPTION1].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_MMC5_OPTION1].sub_family=0;
    settings[NSFPLAY_MMC5_OPTION1].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_FDS_OPTION0)
    settings[NSFPLAY_FDS_OPTION0].label=(char*)"FDS LP filter in Hz";
    settings[NSFPLAY_FDS_OPTION0].description=(char*)"Cutoff frequency (0Hz=off, 2000Hz=default)";
    settings[NSFPLAY_FDS_OPTION0].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_FDS_OPTION0].sub_family=0;
    settings[NSFPLAY_FDS_OPTION0].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_FDS_OPTION0].type=MDZ_TEXTBOX;
    settings[NSFPLAY_FDS_OPTION0].detail.mdz_textbox.text=(char*)malloc(strlen("2000")+1);
    settings[NSFPLAY_FDS_OPTION0].detail.mdz_textbox.max_width_char=6;
    strcpy(settings[NSFPLAY_FDS_OPTION0].detail.mdz_textbox.text,"2000");
    
    SETTINGS_ID_DEF(NSFPLAY_FDS_OPTION1)
    settings[NSFPLAY_FDS_OPTION1].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_FDS_OPTION1].label=(char*)"FDS reset \"phase\"";
    settings[NSFPLAY_FDS_OPTION1].description=(char*)"$4085 \"resets\" modular phase";
    settings[NSFPLAY_FDS_OPTION1].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_FDS_OPTION1].sub_family=0;
    settings[NSFPLAY_FDS_OPTION1].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_FDS_OPTION2)
    settings[NSFPLAY_FDS_OPTION2].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_FDS_OPTION2].label=(char*)"FDS write protect";
    settings[NSFPLAY_FDS_OPTION2].description=(char*)"write protect $8000-DFFF";
    settings[NSFPLAY_FDS_OPTION2].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_FDS_OPTION2].sub_family=0;
    settings[NSFPLAY_FDS_OPTION2].callback=&optNSFPLAYChangedC;
    
    SETTINGS_ID_DEF(NSFPLAY_VRC7_Patch)
    settings[NSFPLAY_VRC7_Patch].type=MDZ_SWITCH;
    settings[NSFPLAY_VRC7_Patch].label=(char*)"VRC7 Patches";
    settings[NSFPLAY_VRC7_Patch].description=(char*)"0 - VRC7 set by Nuke.KYT 3/15/2019\n\
1 - VRC7 set by rainwarrior 8/01/2012\n\
2 - VRC7 set by quietust 1/18/2004\n\
3 - VRC7 set by Mitsutaka Okazaki\n\
4 - VRC7 set by Mitsutaka Okazaki\n\
5 - VRC7 set by kevtris 11/15/1999\n\
6 - VRC7 set by kevtris 11/14/1999\n\
7 - YM2413 set by Mitsutaka Okazaki 4/10/2004\n\
8 - YMF281B set by Chabin 4/10/2004\n\
9 - YMF281B set by plgDavid 2/28/2021";
    settings[NSFPLAY_VRC7_Patch].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_VRC7_Patch].sub_family=0;
    settings[NSFPLAY_VRC7_Patch].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_value_nb=10;
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels=(char**)malloc(settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels[0]=(char*)"0";
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels[2]=(char*)"2";
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels[3]=(char*)"3";
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels[4]=(char*)"4";
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels[5]=(char*)"5";
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels[6]=(char*)"6";
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels[7]=(char*)"7";
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels[8]=(char*)"8";
    settings[NSFPLAY_VRC7_Patch].detail.mdz_switch.switch_labels[9]=(char*)"9";
    
    SETTINGS_ID_DEF(NSFPLAY_VRC7_OPTION0)
    settings[NSFPLAY_VRC7_OPTION0].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_VRC7_OPTION0].label=(char*)"VRC7 Replace with OPLL";
    settings[NSFPLAY_VRC7_OPTION0].description=NULL;
    settings[NSFPLAY_VRC7_OPTION0].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_VRC7_OPTION0].sub_family=0;
    settings[NSFPLAY_VRC7_OPTION0].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_VRC7_OPTION0].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(NSFPLAY_N163_OPTION0)
    settings[NSFPLAY_N163_OPTION0].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_N163_OPTION0].label=(char*)"N163 Serial Multiplex mixing";
    settings[NSFPLAY_N163_OPTION0].description=NULL;
    settings[NSFPLAY_N163_OPTION0].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_N163_OPTION0].sub_family=0;
    settings[NSFPLAY_N163_OPTION0].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_N163_OPTION0].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(NSFPLAY_N163_OPTION1)
    settings[NSFPLAY_N163_OPTION1].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_N163_OPTION1].label=(char*)"N163 Write Protect Phase";
    settings[NSFPLAY_N163_OPTION1].description=NULL;
    settings[NSFPLAY_N163_OPTION1].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_N163_OPTION1].sub_family=0;
    settings[NSFPLAY_N163_OPTION1].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_N163_OPTION1].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(NSFPLAY_N163_OPTION2)
    settings[NSFPLAY_N163_OPTION2].type=MDZ_BOOLSWITCH;
    settings[NSFPLAY_N163_OPTION2].label=(char*)"N163 Limit Wavelength";
    settings[NSFPLAY_N163_OPTION2].description=NULL;
    settings[NSFPLAY_N163_OPTION2].family=MDZ_SETTINGS_FAMILY_NSFPLAY;
    settings[NSFPLAY_N163_OPTION2].sub_family=0;
    settings[NSFPLAY_N163_OPTION2].callback=&optNSFPLAYChangedC;
    settings[NSFPLAY_N163_OPTION2].detail.mdz_boolswitch.switch_value=0;
    
    /////////////////////////////////////
    //TIMIDITY
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_TIMIDITY)
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].label=(char*)"Timidity";
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_TIMIDITY].sub_family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    
    SETTINGS_ID_DEF(TIM_Maxloop)
    settings[TIM_Maxloop].label=(char*)"Max loop";
    settings[TIM_Maxloop].description=NULL;
    settings[TIM_Maxloop].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Maxloop].sub_family=0;
    settings[TIM_Maxloop].callback=&optTIMIDITYChangedC;
    settings[TIM_Maxloop].type=MDZ_SLIDER_DISCRETE;
    settings[TIM_Maxloop].detail.mdz_slider.slider_digits=0;
    settings[TIM_Maxloop].detail.mdz_slider.slider_min_value=0;
    settings[TIM_Maxloop].detail.mdz_slider.slider_max_value=16;
    
    SETTINGS_ID_DEF(TIM_Polyphony)
    settings[TIM_Polyphony].label=(char*)"Midi polyphony";
    settings[TIM_Polyphony].description=NULL;
    settings[TIM_Polyphony].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Polyphony].sub_family=0;
    settings[TIM_Polyphony].callback=&optTIMIDITYChangedC;
    settings[TIM_Polyphony].type=MDZ_SLIDER_DISCRETE;
    settings[TIM_Polyphony].detail.mdz_slider.slider_digits=0;
    settings[TIM_Polyphony].detail.mdz_slider.slider_min_value=64;
    settings[TIM_Polyphony].detail.mdz_slider.slider_max_value=256;
    
    SETTINGS_ID_DEF(TIM_Amplification)
    settings[TIM_Amplification].label=(char*)"Amplification";
    settings[TIM_Amplification].description=NULL;
    settings[TIM_Amplification].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Amplification].sub_family=0;
    settings[TIM_Amplification].callback=&optTIMIDITYChangedC;
    settings[TIM_Amplification].type=MDZ_SLIDER_DISCRETE;
    settings[TIM_Amplification].detail.mdz_slider.slider_digits=0;
    settings[TIM_Amplification].detail.mdz_slider.slider_min_value=10;
    settings[TIM_Amplification].detail.mdz_slider.slider_max_value=400;
    
    SETTINGS_ID_DEF(TIM_Chorus)
    settings[TIM_Chorus].type=MDZ_BOOLSWITCH;
    settings[TIM_Chorus].label=(char*)"Chorus";
    settings[TIM_Chorus].description=NULL;
    settings[TIM_Chorus].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Chorus].sub_family=0;
    settings[TIM_Chorus].callback=&optTIMIDITYChangedC;
    settings[TIM_Chorus].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(TIM_Reverb)
    settings[TIM_Reverb].type=MDZ_BOOLSWITCH;
    settings[TIM_Reverb].label=(char*)"Reverb";
    settings[TIM_Reverb].description=NULL;
    settings[TIM_Reverb].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Reverb].sub_family=0;
    settings[TIM_Reverb].callback=&optTIMIDITYChangedC;
    settings[TIM_Reverb].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(TIM_LPFilter)
    settings[TIM_LPFilter].type=MDZ_BOOLSWITCH;
    settings[TIM_LPFilter].label=(char*)"LP filter";
    settings[TIM_LPFilter].description=NULL;
    settings[TIM_LPFilter].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_LPFilter].sub_family=0;
    settings[TIM_LPFilter].callback=&optTIMIDITYChangedC;
    settings[TIM_LPFilter].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(TIM_Resample)
    settings[TIM_Resample].type=MDZ_SWITCH;
    settings[TIM_Resample].label=(char*)"Resampling";
    settings[TIM_Resample].description=NULL;
    settings[TIM_Resample].family=MDZ_SETTINGS_FAMILY_TIMIDITY;
    settings[TIM_Resample].sub_family=0;
    settings[TIM_Resample].callback=&optTIMIDITYChangedC;
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
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_VGMPLAY)
    settings[MDZ_SETTINGS_FAMILY_VGMPLAY].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_VGMPLAY].label=(char*)"LIBVGM";
    settings[MDZ_SETTINGS_FAMILY_VGMPLAY].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_VGMPLAY].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_VGMPLAY].sub_family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    
    SETTINGS_ID_DEF(VGMPLAY_Maxloop)
    settings[VGMPLAY_Maxloop].label=(char*)"Max loop";
    settings[VGMPLAY_Maxloop].description=NULL;
    settings[VGMPLAY_Maxloop].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_Maxloop].sub_family=0;
    settings[VGMPLAY_Maxloop].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_Maxloop].type=MDZ_SLIDER_DISCRETE;
    settings[VGMPLAY_Maxloop].detail.mdz_slider.slider_digits=0;
    settings[VGMPLAY_Maxloop].detail.mdz_slider.slider_min_value=1;
    settings[VGMPLAY_Maxloop].detail.mdz_slider.slider_max_value=16;
    
    SETTINGS_ID_DEF(VGMPLAY_PreferJTAG)
    settings[VGMPLAY_PreferJTAG].type=MDZ_BOOLSWITCH;
    settings[VGMPLAY_PreferJTAG].label=(char*)"Japanese Tag";
    settings[VGMPLAY_PreferJTAG].description=NULL;
    settings[VGMPLAY_PreferJTAG].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_PreferJTAG].sub_family=0;
    settings[VGMPLAY_PreferJTAG].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(VGMPLAY_YM2612Emulator)
    settings[VGMPLAY_YM2612Emulator].type=MDZ_SWITCH;
    settings[VGMPLAY_YM2612Emulator].label=(char*)"YM2612 emu";
    settings[VGMPLAY_YM2612Emulator].description=NULL;
    settings[VGMPLAY_YM2612Emulator].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_YM2612Emulator].sub_family=0;
    settings[VGMPLAY_YM2612Emulator].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_value_nb=3;
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_labels=(char**)malloc(settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_labels[0]=(char*)"MAME";
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_labels[1]=(char*)"Nuked OPN2";
    settings[VGMPLAY_YM2612Emulator].detail.mdz_switch.switch_labels[2]=(char*)"Gens";
    
    SETTINGS_ID_DEF(VGMPLAY_YM3812Emulator)
    settings[VGMPLAY_YM3812Emulator].type=MDZ_SWITCH;
    settings[VGMPLAY_YM3812Emulator].label=(char*)"YM3812 emu";
    settings[VGMPLAY_YM3812Emulator].description=NULL;
    settings[VGMPLAY_YM3812Emulator].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_YM3812Emulator].sub_family=0;
    settings[VGMPLAY_YM3812Emulator].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_YM3812Emulator].detail.mdz_switch.switch_value_nb=2;
    settings[VGMPLAY_YM3812Emulator].detail.mdz_switch.switch_labels=(char**)malloc(settings[VGMPLAY_YM3812Emulator].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[VGMPLAY_YM3812Emulator].detail.mdz_switch.switch_labels[0]=(char*)"AdlibEmu";
    settings[VGMPLAY_YM3812Emulator].detail.mdz_switch.switch_labels[1]=(char*)"MAME";
    //settings[VGMPLAY_YM3812Emulator].detail.mdz_switch.switch_labels[2]=(char*)"Nuked OPL3";
    
    SETTINGS_ID_DEF(VGMPLAY_QSoundEmulator)
    settings[VGMPLAY_QSoundEmulator].type=MDZ_SWITCH;
    settings[VGMPLAY_QSoundEmulator].label=(char*)"QSound emu";
    settings[VGMPLAY_QSoundEmulator].description=NULL;
    settings[VGMPLAY_QSoundEmulator].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_QSoundEmulator].sub_family=0;
    settings[VGMPLAY_QSoundEmulator].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_QSoundEmulator].detail.mdz_switch.switch_value_nb=2;
    settings[VGMPLAY_QSoundEmulator].detail.mdz_switch.switch_labels=(char**)malloc(settings[VGMPLAY_QSoundEmulator].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[VGMPLAY_QSoundEmulator].detail.mdz_switch.switch_labels[0]=(char*)"CTR";
    settings[VGMPLAY_QSoundEmulator].detail.mdz_switch.switch_labels[1]=(char*)"MAME";
    
    SETTINGS_ID_DEF(VGMPLAY_RF5C68Emulator)
    settings[VGMPLAY_RF5C68Emulator].type=MDZ_SWITCH;
    settings[VGMPLAY_RF5C68Emulator].label=(char*)"RF5C68 emu";
    settings[VGMPLAY_RF5C68Emulator].description=NULL;
    settings[VGMPLAY_RF5C68Emulator].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_RF5C68Emulator].sub_family=0;
    settings[VGMPLAY_RF5C68Emulator].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_RF5C68Emulator].detail.mdz_switch.switch_value_nb=2;
    settings[VGMPLAY_RF5C68Emulator].detail.mdz_switch.switch_labels=(char**)malloc(settings[VGMPLAY_RF5C68Emulator].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[VGMPLAY_RF5C68Emulator].detail.mdz_switch.switch_labels[0]=(char*)"MAME";
    settings[VGMPLAY_RF5C68Emulator].detail.mdz_switch.switch_labels[1]=(char*)"GENS";
    
    SETTINGS_ID_DEF(VGMPLAY_NUKEDOPN2_Option)
    settings[VGMPLAY_NUKEDOPN2_Option].type=MDZ_SWITCH;
    settings[VGMPLAY_NUKEDOPN2_Option].label=(char*)"Nuked OPN2 Type";
    settings[VGMPLAY_NUKEDOPN2_Option].description=NULL;
    settings[VGMPLAY_NUKEDOPN2_Option].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_NUKEDOPN2_Option].sub_family=0;
    settings[VGMPLAY_NUKEDOPN2_Option].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_NUKEDOPN2_Option].detail.mdz_switch.switch_value_nb=4;
    settings[VGMPLAY_NUKEDOPN2_Option].detail.mdz_switch.switch_labels=(char**)malloc(settings[VGMPLAY_NUKEDOPN2_Option].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[VGMPLAY_NUKEDOPN2_Option].detail.mdz_switch.switch_labels[0]=(char*)"Filt.";
    settings[VGMPLAY_NUKEDOPN2_Option].detail.mdz_switch.switch_labels[1]=(char*)"ASIC";
    settings[VGMPLAY_NUKEDOPN2_Option].detail.mdz_switch.switch_labels[2]=(char*)"Disc.";
    settings[VGMPLAY_NUKEDOPN2_Option].detail.mdz_switch.switch_labels[3]=(char*)"MD1";
    
    SETTINGS_ID_DEF(VGMPLAY_YMF262Emulator)
    settings[VGMPLAY_YMF262Emulator].type=MDZ_SWITCH;
    settings[VGMPLAY_YMF262Emulator].label=(char*)"YMF262 emu";
    settings[VGMPLAY_YMF262Emulator].description=NULL;
    settings[VGMPLAY_YMF262Emulator].family=MDZ_SETTINGS_FAMILY_VGMPLAY;
    settings[VGMPLAY_YMF262Emulator].sub_family=0;
    settings[VGMPLAY_YMF262Emulator].callback=&optVGMPLAYChangedC;
    settings[VGMPLAY_YMF262Emulator].detail.mdz_switch.switch_value_nb=3;
    settings[VGMPLAY_YMF262Emulator].detail.mdz_switch.switch_labels=(char**)malloc(settings[VGMPLAY_YMF262Emulator].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[VGMPLAY_YMF262Emulator].detail.mdz_switch.switch_labels[0]=(char*)"AdlibEmu";
    settings[VGMPLAY_YMF262Emulator].detail.mdz_switch.switch_labels[1]=(char*)"MAME";
    settings[VGMPLAY_YMF262Emulator].detail.mdz_switch.switch_labels[2]=(char*)"Nuked";
    
    /////////////////////////////////////
    //VGMSTREAM
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_VGMSTREAM)
    settings[MDZ_SETTINGS_FAMILY_VGMSTREAM].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_VGMSTREAM].label=(char*)"VGMStream";
    settings[MDZ_SETTINGS_FAMILY_VGMSTREAM].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_VGMSTREAM].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_VGMSTREAM].sub_family=MDZ_SETTINGS_FAMILY_VGMSTREAM;
    
    SETTINGS_ID_DEF(VGMSTREAM_Forceloop)
    settings[VGMSTREAM_Forceloop].type=MDZ_BOOLSWITCH;
    settings[VGMSTREAM_Forceloop].label=(char*)"Force loop";
    settings[VGMSTREAM_Forceloop].description=NULL;
    settings[VGMSTREAM_Forceloop].family=MDZ_SETTINGS_FAMILY_VGMSTREAM;
    settings[VGMSTREAM_Forceloop].sub_family=0;
    settings[VGMSTREAM_Forceloop].callback=&optVGMSTREAMChangedC;
    settings[VGMSTREAM_Forceloop].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(VGMSTREAM_Maxloop)
    settings[VGMSTREAM_Maxloop].label=(char*)"Max loop";
    settings[VGMSTREAM_Maxloop].description=NULL;
    settings[VGMSTREAM_Maxloop].family=MDZ_SETTINGS_FAMILY_VGMSTREAM;
    settings[VGMSTREAM_Maxloop].sub_family=0;
    settings[VGMSTREAM_Maxloop].callback=&optVGMSTREAMChangedC;
    settings[VGMSTREAM_Maxloop].type=MDZ_SLIDER_DISCRETE;
    settings[VGMSTREAM_Maxloop].detail.mdz_slider.slider_digits=0;
    settings[VGMSTREAM_Maxloop].detail.mdz_slider.slider_min_value=1;
    settings[VGMSTREAM_Maxloop].detail.mdz_slider.slider_max_value=32;
    
    SETTINGS_ID_DEF(VGMSTREAM_ResampleQuality)
    settings[VGMSTREAM_ResampleQuality].label=(char*)"Resampling";
    settings[VGMSTREAM_ResampleQuality].description=NULL;
    settings[VGMSTREAM_ResampleQuality].family=MDZ_SETTINGS_FAMILY_VGMSTREAM;
    settings[VGMSTREAM_ResampleQuality].sub_family=0;
    settings[VGMSTREAM_ResampleQuality].callback=&optVGMSTREAMChangedC;
    settings[VGMSTREAM_ResampleQuality].type=MDZ_SWITCH;
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_value_nb=5;
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels=(char**)malloc(settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels[0]=(char*)"Best";
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels[1]=(char*)"Med.";
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels[2]=(char*)"Fast";
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels[3]=(char*)"ZOH";
    settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_labels[4]=(char*)"Lin.";
    
    
    /////////////////////////////////////
    //HC
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_HC)
    settings[MDZ_SETTINGS_FAMILY_HC].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_HC].label=(char*)"Highly Complete";
    settings[MDZ_SETTINGS_FAMILY_HC].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_HC].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_HC].sub_family=MDZ_SETTINGS_FAMILY_HC;
    
    SETTINGS_ID_DEF(HC_ResampleQuality)
    settings[HC_ResampleQuality].type=MDZ_SWITCH;
    settings[HC_ResampleQuality].label=(char*)"Resampling";
    settings[HC_ResampleQuality].description=NULL;
    settings[HC_ResampleQuality].family=MDZ_SETTINGS_FAMILY_HC;
    settings[HC_ResampleQuality].sub_family=0;
    settings[HC_ResampleQuality].callback=&optHCChangedC;
    settings[HC_ResampleQuality].detail.mdz_switch.switch_value_nb=5;
    settings[HC_ResampleQuality].detail.mdz_switch.switch_labels=(char**)malloc(settings[HC_ResampleQuality].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[HC_ResampleQuality].detail.mdz_switch.switch_labels[0]=(char*)"Best";
    settings[HC_ResampleQuality].detail.mdz_switch.switch_labels[1]=(char*)"Med.";
    settings[HC_ResampleQuality].detail.mdz_switch.switch_labels[2]=(char*)"Fast";
    settings[HC_ResampleQuality].detail.mdz_switch.switch_labels[3]=(char*)"ZOH";
    settings[HC_ResampleQuality].detail.mdz_switch.switch_labels[4]=(char*)"Lin.";
    
    SETTINGS_ID_DEF(HC_MainEnabled)
    settings[HC_MainEnabled].type=MDZ_BOOLSWITCH;
    settings[HC_MainEnabled].label=(char*)"PS1/PS2 Main";
    settings[HC_MainEnabled].description=NULL;
    settings[HC_MainEnabled].family=MDZ_SETTINGS_FAMILY_HC;
    settings[HC_MainEnabled].sub_family=0;
    settings[HC_MainEnabled].callback=&optHCChangedC;
    
    SETTINGS_ID_DEF(HC_ReverbEnabled)
    settings[HC_ReverbEnabled].type=MDZ_BOOLSWITCH;
    settings[HC_ReverbEnabled].label=(char*)"PS1/PS2 Reverb";
    settings[HC_ReverbEnabled].description=NULL;
    settings[HC_ReverbEnabled].family=MDZ_SETTINGS_FAMILY_HC;
    settings[HC_ReverbEnabled].sub_family=0;
    settings[HC_ReverbEnabled].callback=&optHCChangedC;
    
    
    /////////////////////////////////////
    //GME
    /////////////////////////////////////
    
    /////////////////////////////////////
    //SID
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_SID)
    settings[MDZ_SETTINGS_FAMILY_SID].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_SID].label=(char*)"SID";
    settings[MDZ_SETTINGS_FAMILY_SID].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_SID].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_SID].sub_family=MDZ_SETTINGS_FAMILY_SID;
    
    SETTINGS_ID_DEF(SID_Engine)
    settings[SID_Engine].type=MDZ_SWITCH;
    settings[SID_Engine].label=(char*)"Engine";
    settings[SID_Engine].description=NULL;
    settings[SID_Engine].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_Engine].sub_family=0;
    settings[SID_Engine].callback=&optSIDChangedC;
    settings[SID_Engine].detail.mdz_switch.switch_value_nb=2;
    settings[SID_Engine].detail.mdz_switch.switch_labels=(char**)malloc(settings[SID_Engine].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[SID_Engine].detail.mdz_switch.switch_labels[0]=(char*)"ReSID";
    settings[SID_Engine].detail.mdz_switch.switch_labels[1]=(char*)"ReSIDFP";
    
    SETTINGS_ID_DEF(SID_Interpolation)
    settings[SID_Interpolation].type=MDZ_SWITCH;
    settings[SID_Interpolation].label=(char*)"Interpolation";
    settings[SID_Interpolation].description=NULL;
    settings[SID_Interpolation].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_Interpolation].sub_family=0;
    settings[SID_Interpolation].callback=&optSIDChangedC;
    settings[SID_Interpolation].detail.mdz_switch.switch_value_nb=3;
    settings[SID_Interpolation].detail.mdz_switch.switch_labels=(char**)malloc(settings[SID_Interpolation].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[SID_Interpolation].detail.mdz_switch.switch_labels[0]=(char*)"Fast";
    settings[SID_Interpolation].detail.mdz_switch.switch_labels[1]=(char*)"Med";
    settings[SID_Interpolation].detail.mdz_switch.switch_labels[2]=(char*)"Best";
    
    SETTINGS_ID_DEF(SID_Filter)
    settings[SID_Filter].type=MDZ_BOOLSWITCH;
    settings[SID_Filter].label=(char*)"Filter";
    settings[SID_Filter].description=NULL;
    settings[SID_Filter].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_Filter].sub_family=0;
    settings[SID_Filter].callback=&optSIDChangedC;
    settings[SID_Filter].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(SID_SecondSIDOn)
    settings[SID_SecondSIDOn].type=MDZ_BOOLSWITCH;
    settings[SID_SecondSIDOn].label=(char*)"Force 2nd SID";
    settings[SID_SecondSIDOn].description=NULL;
    settings[SID_SecondSIDOn].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_SecondSIDOn].sub_family=0;
    settings[SID_SecondSIDOn].callback=&optSIDChangedC;
    settings[SID_SecondSIDOn].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(SID_ThirdSIDOn)
    settings[SID_ThirdSIDOn].type=MDZ_BOOLSWITCH;
    settings[SID_ThirdSIDOn].label=(char*)"Force 3rd SID";
    settings[SID_ThirdSIDOn].description=NULL;
    settings[SID_ThirdSIDOn].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_ThirdSIDOn].sub_family=0;
    settings[SID_ThirdSIDOn].callback=&optSIDChangedC;
    settings[SID_ThirdSIDOn].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(SID_SecondSIDAddress)
    settings[SID_SecondSIDAddress].label=(char*)"Address 2nd";
    settings[SID_SecondSIDAddress].description=(char*)"0xD420-0xD7FF or 0xDE00-0xDFFF";
    settings[SID_SecondSIDAddress].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_SecondSIDAddress].sub_family=0;
    settings[SID_SecondSIDAddress].type=MDZ_TEXTBOX;
    settings[SID_SecondSIDAddress].detail.mdz_textbox.text=(char*)malloc(strlen("0xD420")+1);
    settings[SID_SecondSIDAddress].detail.mdz_textbox.max_width_char=6;
    strcpy(settings[SID_SecondSIDAddress].detail.mdz_textbox.text,"0xD420");
    
    SETTINGS_ID_DEF(SID_ThirdSIDAddress)
    settings[SID_ThirdSIDAddress].label=(char*)"Address 3rd";
    settings[SID_ThirdSIDAddress].description=(char*)"0xD420-0xD7FF or 0xDE00-0xDFFF";
    settings[SID_ThirdSIDAddress].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_ThirdSIDAddress].sub_family=0;
    settings[SID_ThirdSIDAddress].type=MDZ_TEXTBOX;
    settings[SID_ThirdSIDAddress].detail.mdz_textbox.text=(char*)malloc(strlen("0xD440")+1);
    settings[SID_ThirdSIDAddress].detail.mdz_textbox.max_width_char=6;
    strcpy(settings[SID_ThirdSIDAddress].detail.mdz_textbox.text,"0xD440");
    
    SETTINGS_ID_DEF(SID_ForceLoop)
    settings[SID_ForceLoop].type=MDZ_BOOLSWITCH;
    settings[SID_ForceLoop].label=(char*)"Force loop";
    settings[SID_ForceLoop].description=NULL;
    settings[SID_ForceLoop].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_ForceLoop].sub_family=0;
    settings[SID_ForceLoop].callback=&optSIDChangedC;
    settings[SID_ForceLoop].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(SID_CLOCK)
    settings[SID_CLOCK].type=MDZ_SWITCH;
    settings[SID_CLOCK].label=(char*)"Clock";
    settings[SID_CLOCK].description=NULL;
    settings[SID_CLOCK].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_CLOCK].sub_family=0;
    settings[SID_CLOCK].callback=&optSIDChangedC;
    settings[SID_CLOCK].detail.mdz_switch.switch_value_nb=3;
    settings[SID_CLOCK].detail.mdz_switch.switch_labels=(char**)malloc(settings[SID_CLOCK].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[SID_CLOCK].detail.mdz_switch.switch_labels[0]=(char*)"Auto";
    settings[SID_CLOCK].detail.mdz_switch.switch_labels[1]=(char*)"PAL";
    settings[SID_CLOCK].detail.mdz_switch.switch_labels[2]=(char*)"NTSC";
    
    SETTINGS_ID_DEF(SID_MODEL)
    settings[SID_MODEL].type=MDZ_SWITCH;
    settings[SID_MODEL].label=(char*)"Model";
    settings[SID_MODEL].description=NULL;
    settings[SID_MODEL].family=MDZ_SETTINGS_FAMILY_SID;
    settings[SID_MODEL].sub_family=0;
    settings[SID_MODEL].callback=&optSIDChangedC;
    settings[SID_MODEL].detail.mdz_switch.switch_value_nb=3;
    settings[SID_MODEL].detail.mdz_switch.switch_labels=(char**)malloc(settings[SID_MODEL].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[SID_MODEL].detail.mdz_switch.switch_labels[0]=(char*)"Auto";
    settings[SID_MODEL].detail.mdz_switch.switch_labels[1]=(char*)"6581";
    settings[SID_MODEL].detail.mdz_switch.switch_labels[2]=(char*)"8580";
    
    
    /////////////////////////////////////
    //UADE
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_UADE)
    settings[MDZ_SETTINGS_FAMILY_UADE].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_UADE].label=(char*)"UADE";
    settings[MDZ_SETTINGS_FAMILY_UADE].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_UADE].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_UADE].sub_family=MDZ_SETTINGS_FAMILY_UADE;
    
    SETTINGS_ID_DEF(UADE_Head)
    settings[UADE_Head].type=MDZ_BOOLSWITCH;
    settings[UADE_Head].label=(char*)"Headphones";
    settings[UADE_Head].description=NULL;
    settings[UADE_Head].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_Head].sub_family=0;
    settings[UADE_Head].callback=&optUADEChangedC;
    settings[UADE_Head].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(UADE_PostFX)
    settings[UADE_PostFX].type=MDZ_BOOLSWITCH;
    settings[UADE_PostFX].label=(char*)"Post FX";
    settings[UADE_PostFX].description=NULL;
    settings[UADE_PostFX].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_PostFX].sub_family=0;
    settings[UADE_PostFX].callback=&optUADEChangedC;
    settings[UADE_PostFX].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(UADE_Led)
    settings[UADE_Led].type=MDZ_BOOLSWITCH;
    settings[UADE_Led].label=(char*)"LED";
    settings[UADE_Led].description=NULL;
    settings[UADE_Led].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_Led].sub_family=0;
    settings[UADE_Led].callback=&optUADEChangedC;
    settings[UADE_Led].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(UADE_Norm)
    settings[UADE_Norm].type=MDZ_BOOLSWITCH;
    settings[UADE_Norm].label=(char*)"Normalization";
    settings[UADE_Norm].description=NULL;
    settings[UADE_Norm].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_Norm].sub_family=0;
    settings[UADE_Norm].callback=&optUADEChangedC;
    settings[UADE_Norm].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(UADE_Gain)
    settings[UADE_Gain].type=MDZ_BOOLSWITCH;
    settings[UADE_Gain].label=(char*)"Gain";
    settings[UADE_Gain].description=NULL;
    settings[UADE_Gain].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_Gain].sub_family=0;
    settings[UADE_Gain].callback=&optUADEChangedC;
    settings[UADE_Gain].detail.mdz_boolswitch.switch_value=0;
    
    SETTINGS_ID_DEF(UADE_GainValue)
    settings[UADE_GainValue].label=(char*)"Gain value";
    settings[UADE_GainValue].description=NULL;
    settings[UADE_GainValue].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_GainValue].sub_family=0;
    settings[UADE_GainValue].callback=&optUADEChangedC;
    settings[UADE_GainValue].type=MDZ_SLIDER_CONTINUOUS;
    settings[UADE_GainValue].detail.mdz_slider.slider_value=0.5;
    settings[UADE_GainValue].detail.mdz_slider.slider_digits=100;
    settings[UADE_GainValue].detail.mdz_slider.slider_min_value=0;
    settings[UADE_GainValue].detail.mdz_slider.slider_max_value=1;
    
    SETTINGS_ID_DEF(UADE_Pan)
    settings[UADE_Pan].type=MDZ_BOOLSWITCH;
    settings[UADE_Pan].label=(char*)"Panning (Stereo separation)";
    settings[UADE_Pan].description=NULL;
    settings[UADE_Pan].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_Pan].sub_family=0;
    settings[UADE_Pan].callback=&optUADEChangedC;
    settings[UADE_Pan].detail.mdz_boolswitch.switch_value=1;
    
    SETTINGS_ID_DEF(UADE_PanValue)
    settings[UADE_PanValue].label=(char*)"Panning value";
    settings[UADE_PanValue].description=NULL;
    settings[UADE_PanValue].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_PanValue].sub_family=0;
    settings[UADE_PanValue].callback=&optUADEChangedC;
    settings[UADE_PanValue].type=MDZ_SLIDER_CONTINUOUS;
    settings[UADE_PanValue].detail.mdz_slider.slider_digits=100;
    settings[UADE_PanValue].detail.mdz_slider.slider_min_value=0;
    settings[UADE_PanValue].detail.mdz_slider.slider_max_value=1;
    
    SETTINGS_ID_DEF(UADE_NTSC)
    settings[UADE_NTSC].type=MDZ_BOOLSWITCH;
    settings[UADE_NTSC].label=(char*)"Force NTSC";
    settings[UADE_NTSC].description=NULL;
    settings[UADE_NTSC].family=MDZ_SETTINGS_FAMILY_UADE;
    settings[UADE_NTSC].sub_family=0;
    settings[UADE_NTSC].callback=&optUADEChangedC;
    settings[UADE_NTSC].detail.mdz_boolswitch.switch_value=0;
    
    /////////////////////////////////////
    //ADPLUG
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_ADPLUG)
    settings[MDZ_SETTINGS_FAMILY_ADPLUG].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_ADPLUG].label=(char*)"ADPLUG";
    settings[MDZ_SETTINGS_FAMILY_ADPLUG].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_ADPLUG].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_ADPLUG].sub_family=MDZ_SETTINGS_FAMILY_ADPLUG;
    
    SETTINGS_ID_DEF(ADPLUG_OplType)
    settings[ADPLUG_OplType].type=MDZ_SWITCH;
    settings[ADPLUG_OplType].label=(char*)"OPL Type";
    settings[ADPLUG_OplType].description=NULL;
    settings[ADPLUG_OplType].family=MDZ_SETTINGS_FAMILY_ADPLUG;
    settings[ADPLUG_OplType].sub_family=0;
    settings[ADPLUG_OplType].callback=&optADPLUGChangedC;
    settings[ADPLUG_OplType].detail.mdz_switch.switch_value_nb=4;
    settings[ADPLUG_OplType].detail.mdz_switch.switch_labels=(char**)malloc(settings[ADPLUG_OplType].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[ADPLUG_OplType].detail.mdz_switch.switch_labels[0]=(char*)"Woody";
    settings[ADPLUG_OplType].detail.mdz_switch.switch_labels[1]=(char*)"Satoh";
    settings[ADPLUG_OplType].detail.mdz_switch.switch_labels[2]=(char*)"Ken";
    settings[ADPLUG_OplType].detail.mdz_switch.switch_labels[3]=(char*)"Nuked";
    
    SETTINGS_ID_DEF(ADPLUG_StereoSurround)
    settings[ADPLUG_StereoSurround].type=MDZ_SWITCH;
    settings[ADPLUG_StereoSurround].label=(char*)"Harmonic";
    settings[ADPLUG_StereoSurround].description=NULL;
    settings[ADPLUG_StereoSurround].family=MDZ_SETTINGS_FAMILY_ADPLUG;
    settings[ADPLUG_StereoSurround].sub_family=0;
    settings[ADPLUG_StereoSurround].callback=&optADPLUGChangedC;
    settings[ADPLUG_StereoSurround].detail.mdz_switch.switch_value_nb=2;
    settings[ADPLUG_StereoSurround].detail.mdz_switch.switch_labels=(char**)malloc(settings[ADPLUG_StereoSurround].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[ADPLUG_StereoSurround].detail.mdz_switch.switch_labels[0]=(char*)"Stereo";
    settings[ADPLUG_StereoSurround].detail.mdz_switch.switch_labels[1]=(char*)"Surround";
    
    /////////////////////////////////////
    //GBSPLAY
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_GBSPLAY)
    settings[MDZ_SETTINGS_FAMILY_GBSPLAY].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_GBSPLAY].label=(char*)"GBSPLAY";
    settings[MDZ_SETTINGS_FAMILY_GBSPLAY].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_GBSPLAY].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_GBSPLAY].sub_family=MDZ_SETTINGS_FAMILY_GBSPLAY;
    
    
    
    SETTINGS_ID_DEF(GBSPLAY_HPFilterType)
    settings[GBSPLAY_HPFilterType].type=MDZ_SWITCH;
    settings[GBSPLAY_HPFilterType].label=(char*)"Highpass filter type";
    settings[GBSPLAY_HPFilterType].description=NULL;
    settings[GBSPLAY_HPFilterType].family=MDZ_SETTINGS_FAMILY_GBSPLAY;
    settings[GBSPLAY_HPFilterType].sub_family=0;
    settings[GBSPLAY_HPFilterType].callback=&optGBSPLAYChangedC;
    settings[GBSPLAY_HPFilterType].detail.mdz_switch.switch_value_nb=3;
    settings[GBSPLAY_HPFilterType].detail.mdz_switch.switch_labels=(char**)malloc(settings[GBSPLAY_HPFilterType].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[GBSPLAY_HPFilterType].detail.mdz_switch.switch_labels[0]=(char*)"Off";
    settings[GBSPLAY_HPFilterType].detail.mdz_switch.switch_labels[1]=(char*)"DMG";
    settings[GBSPLAY_HPFilterType].detail.mdz_switch.switch_labels[2]=(char*)"CGB";
    
    
    /////////////////////////////////////
    //XMP
    /////////////////////////////////////
    SETTINGS_ID_DEF(MDZ_SETTINGS_FAMILY_XMP)
    settings[MDZ_SETTINGS_FAMILY_XMP].type=MDZ_FAMILY;
    settings[MDZ_SETTINGS_FAMILY_XMP].label=(char*)"XMP";
    settings[MDZ_SETTINGS_FAMILY_XMP].description=NULL;
    settings[MDZ_SETTINGS_FAMILY_XMP].family=MDZ_SETTINGS_FAMILY_PLUGINS;
    settings[MDZ_SETTINGS_FAMILY_XMP].sub_family=MDZ_SETTINGS_FAMILY_XMP;
    
    SETTINGS_ID_DEF(XMP_MasterVolume)
    settings[XMP_MasterVolume].label=(char*)"Master volume";
    settings[XMP_MasterVolume].description=NULL;
    settings[XMP_MasterVolume].family=MDZ_SETTINGS_FAMILY_XMP;
    settings[XMP_MasterVolume].sub_family=0;
    settings[XMP_MasterVolume].callback=&optXMPChangedC;
    settings[XMP_MasterVolume].type=MDZ_SLIDER_DISCRETE;
    settings[XMP_MasterVolume].detail.mdz_slider.slider_digits=0;
    settings[XMP_MasterVolume].detail.mdz_slider.slider_min_value=0;
    settings[XMP_MasterVolume].detail.mdz_slider.slider_max_value=200;
    
    SETTINGS_ID_DEF(XMP_StereoSeparation)
    settings[XMP_StereoSeparation].label=(char*)"Stereo separation";
    settings[XMP_StereoSeparation].description=NULL;
    settings[XMP_StereoSeparation].family=MDZ_SETTINGS_FAMILY_XMP;
    settings[XMP_StereoSeparation].sub_family=0;
    settings[XMP_StereoSeparation].callback=&optXMPChangedC;
    settings[XMP_StereoSeparation].type=MDZ_SLIDER_DISCRETE;
    settings[XMP_StereoSeparation].detail.mdz_slider.slider_digits=0;
    settings[XMP_StereoSeparation].detail.mdz_slider.slider_min_value=0;
    settings[XMP_StereoSeparation].detail.mdz_slider.slider_max_value=100;
    
    SETTINGS_ID_DEF(XMP_Interpolation)
    settings[XMP_Interpolation].type=MDZ_SWITCH;
    settings[XMP_Interpolation].label=(char*)"Interpolation";
    settings[XMP_Interpolation].description=NULL;
    settings[XMP_Interpolation].family=MDZ_SETTINGS_FAMILY_XMP;
    settings[XMP_Interpolation].sub_family=0;
    settings[XMP_Interpolation].callback=&optXMPChangedC;
    settings[XMP_Interpolation].detail.mdz_switch.switch_value_nb=3;
    settings[XMP_Interpolation].detail.mdz_switch.switch_labels=(char**)malloc(settings[XMP_Interpolation].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[XMP_Interpolation].detail.mdz_switch.switch_labels[0]=(char*)"Near.";
    settings[XMP_Interpolation].detail.mdz_switch.switch_labels[1]=(char*)"Lin.";
    settings[XMP_Interpolation].detail.mdz_switch.switch_labels[2]=(char*)"Spl.";
    
    SETTINGS_ID_DEF(XMP_Amplification)
    settings[XMP_Amplification].type=MDZ_SWITCH;
    settings[XMP_Amplification].label=(char*)"Amplification";
    settings[XMP_Amplification].description=NULL;
    settings[XMP_Amplification].family=MDZ_SETTINGS_FAMILY_XMP;
    settings[XMP_Amplification].sub_family=0;
    settings[XMP_Amplification].callback=&optXMPChangedC;
    settings[XMP_Amplification].detail.mdz_switch.switch_value_nb=4;
    settings[XMP_Amplification].detail.mdz_switch.switch_labels=(char**)malloc(settings[XMP_Amplification].detail.mdz_switch.switch_value_nb*sizeof(char*));
    settings[XMP_Amplification].detail.mdz_switch.switch_labels[0]=(char*)"0";
    settings[XMP_Amplification].detail.mdz_switch.switch_labels[1]=(char*)"1";
    settings[XMP_Amplification].detail.mdz_switch.switch_labels[2]=(char*)"2";
    settings[XMP_Amplification].detail.mdz_switch.switch_labels[3]=(char*)"3";
    
    /*settings[XMP_DSPLowPass].type=MDZ_BOOLSWITCH;
     settings[XMP_DSPLowPass].label=(char*)"Lowpass filter";
     settings[XMP_DSPLowPass].description=NULL;
     settings[XMP_DSPLowPass].family=MDZ_SETTINGS_FAMILY_XMP;
     settings[XMP_DSPLowPass].sub_family=0;
     settings[XMP_DSPLowPass].callback=&optXMPChangedC;
     settings[XMP_DSPLowPass].detail.mdz_boolswitch.switch_value=1;*/
    
    SETTINGS_ID_DEF(XMP_FLAGS_A500F)
    settings[XMP_FLAGS_A500F].type=MDZ_BOOLSWITCH;
    settings[XMP_FLAGS_A500F].label=(char*)"Amiga 500 Filter";
    settings[XMP_FLAGS_A500F].description=NULL;
    settings[XMP_FLAGS_A500F].family=MDZ_SETTINGS_FAMILY_XMP;
    settings[XMP_FLAGS_A500F].sub_family=0;
    settings[XMP_FLAGS_A500F].callback=&optXMPChangedC;
    settings[XMP_FLAGS_A500F].detail.mdz_boolswitch.switch_value=0;
    
    [SettingsGenViewController applyDefaultSettings];
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

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////


- (void)viewDidLoad
{
    [super viewDidLoad];
    
    dictActionBtn=[NSMutableDictionary dictionaryWithCapacity:64];
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    popTipViewRow=-1;
    popTipViewSection=-1;
    
    UILongPressGestureRecognizer *lpgr = [[UILongPressGestureRecognizer alloc]
                                          initWithTarget:self action:@selector(handleLongPress:)];
    lpgr.minimumPressDuration = 1.0; //seconds
    lpgr.delegate = self;
    [self.tableView addGestureRecognizer:lpgr];
    
    self.navigationController.delegate = self;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    /////////////////////////////////////
    // Waiting view
    /////////////////////////////////////
    waitingView = [[WaitingView alloc] init];
    waitingView.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingView];
    waitingView.hidden=TRUE;
    
    NSDictionary *views = NSDictionaryOfVariableBindings(waitingView);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingView(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingView(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    waitingViewPlayer = [[WaitingView alloc] init];
    waitingViewPlayer.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingViewPlayer];
    
    views = NSDictionaryOfVariableBindings(waitingViewPlayer);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
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

-(void) traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    [self.tableView reloadData];
}

- (void)viewDidDisappear:(BOOL)animated {
    [repeatingTimer invalidate];
    repeatingTimer = nil;
    
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView removeObserver:self
                                          forKeyPath:observedSelector
                                             context:LoadingProgressObserverContext];
    [super viewDidDisappear:animated];
    
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleDefault;
}

- (void)viewWillAppear:(BOOL)animated {
    
    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    
    self.navigationController.delegate = self;
    
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    
    if ([detailViewController mPlaylist_size]>0) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    } else {
        wasMiniPlayerOn=false;
        [self hideMiniPlayer];
    }
    
    [self hideWaiting];
    
    [waitingViewPlayer resetCancelStatus];
    waitingViewPlayer.hidden=detailViewController.waitingView.hidden;
    waitingViewPlayer.btnStopCurrentAction.hidden=detailViewController.waitingView.btnStopCurrentAction.hidden;
    waitingViewPlayer.progressView.progress=detailViewController.waitingView.progressView.progress;
    waitingViewPlayer.progressView.hidden=detailViewController.waitingView.progressView.hidden;
    waitingViewPlayer.lblTitle.text=[NSString stringWithString:detailViewController.waitingView.lblTitle.text];
    waitingViewPlayer.lblDetail.text=[NSString stringWithString:detailViewController.waitingView.lblDetail.text];
    
    //    [waitingViewPlayer.progressView setObservedProgress:detailViewController.mplayer.extractProgress];
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView addObserver:self
                                       forKeyPath:observedSelector
                                          options:NSKeyValueObservingOptionInitial
                                          context:LoadingProgressObserverContext];
    
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.20f target:self selector:@selector(updateLoadingInfos:) userInfo:nil repeats: YES]; //5 times/second
    
    
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
    NSString *footer=nil;
    return footer;
}

- (void)boolswitchChanged:(UISwitch*)sender {
    int refresh=0;
    UISwitch *sw=(UISwitch*)sender;
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    if (settings[cur_settings_idx[indexPath.section]].detail.mdz_boolswitch.switch_value != sw.on) refresh=1;
    settings[cur_settings_idx[indexPath.section]].detail.mdz_boolswitch.switch_value=sw.on;
    
    if (settings[cur_settings_idx[indexPath.section]].callback) {
        settings[cur_settings_idx[indexPath.section]].callback(self);
    }
    if (refresh) [tableView reloadData];
}

-(void) resetAllColor:(id)sender {
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[((UIButton*)sender).description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    
    if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"PIANOMIDI_MULTI_RESETALL")==0) {
        
        [SettingsGenViewController pianomidiGenSystemColor:(settings[PIANOMIDI_MULTI_COLORSET].detail.mdz_switch.switch_value) color_idx:-1 color_buffer:NULL];
        [detailViewController settingsChanged:(int)SETTINGS_PIANOMIDI];
        [tableView reloadData];
    }
    if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_MULTI_RESETALL")==0) {
        [SettingsGenViewController oscilloGenSystemColor:(settings[OSCILLO_MULTI_COLORSET].detail.mdz_switch.switch_value) color_idx:-1 color_buffer:NULL];
        [detailViewController settingsChanged:(int)SETTINGS_OSCILLO];
        [tableView reloadData];
    }
}

-(void) resetColor:(id)sender {
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[((UIButton*)sender).description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    //Check OSCILLO part
    int colidx=-1;
    if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_MONO_COLOR")==0) colidx=0;
    else if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_GRID_COLOR")==0) colidx=1;
    else if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_MULTI_COLOR01")==0) colidx=2;
    else if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_MULTI_COLOR02")==0) colidx=3;
    else if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_MULTI_COLOR03")==0) colidx=4;
    else if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_MULTI_COLOR04")==0) colidx=5;
    else if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_MULTI_COLOR05")==0) colidx=6;
    else if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_MULTI_COLOR06")==0) colidx=7;
    else if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_MULTI_COLOR07")==0) colidx=8;
    else if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,"OSCILLO_MULTI_COLOR08")==0) colidx=9;
    
    
    if (colidx>=0) {
        [SettingsGenViewController oscilloGenSystemColor:0 color_idx:colidx color_buffer:NULL];
        [detailViewController settingsChanged:(int)SETTINGS_OSCILLO];
        [tableView reloadData];
        return;
    }
    
    //Check PR&NS part
    for (int i=0;i<32;i++) {
        char str_tmp[32];
        snprintf(str_tmp,32,"PIANOMIDI_MULTI_COLOR%.2d",i+1);
        if (strcmp(settings[cur_settings_idx[indexPath.section]].setting_id,str_tmp)==0) {
            colidx=i;
            break;
        }
    }
    
    if (colidx>=0) {
        [SettingsGenViewController pianomidiGenSystemColor:0 color_idx:colidx color_buffer:NULL];
        [detailViewController settingsChanged:(int)SETTINGS_PIANOMIDI];
        [tableView reloadData];
        return;
    }
}


-(void)colorPickerSelected:(id)sender {
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[((UIButton*)sender).description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    CGFloat r,g,b,a;
    r=g=b=a=0;
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"14.0")) {
        if (@available(iOS 14.0, *)) {
            UIColor *col=((UIColorWell*)sender).selectedColor;
            CIColor *cicol=[CIColor colorWithCGColor:col.CGColor];
            r=cicol.red;
            g=cicol.green;
            b=cicol.blue;
        }
    } else {
        [((UIButton*)sender).backgroundColor getRed:&r green:&g blue:&b alpha:&a];
        
    }
    
    settings[cur_settings_idx[indexPath.section]].detail.mdz_color.rgb=((unsigned char)(r*255)<<16)|((unsigned char)(g*255)<<8)|((unsigned char)(b*255)<<0);
    
    if (settings[cur_settings_idx[indexPath.section]].callback) {
        settings[cur_settings_idx[indexPath.section]].callback(self);
    }
}

- (void)colorPickerOpen:(UIButton*)sender {
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"14.0")) {
        if (@available(iOS 14.0, *)) {
        }
    } else {
        MSColorSelectionViewController *colorSelectionController = [[MSColorSelectionViewController alloc] init];
        UINavigationController *navCtrl = [[UINavigationController alloc] initWithRootViewController:colorSelectionController];
        
        navCtrl.modalPresentationStyle = UIModalPresentationPopover;
        navCtrl.popoverPresentationController.delegate = self;
        navCtrl.popoverPresentationController.sourceView = sender;
        navCtrl.popoverPresentationController.sourceRect = sender.bounds;
        navCtrl.preferredContentSize = [colorSelectionController.view systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
        
        currentColorPickerBtn=sender;
        
        colorSelectionController.delegate = self;
        colorSelectionController.color = sender.backgroundColor;
        
        navCtrl.view.backgroundColor=[UIColor lightGrayColor];
        
        if (self.traitCollection.horizontalSizeClass == UIUserInterfaceSizeClassCompact) {
            UIBarButtonItem *doneBtn = [[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"Done", ) style:UIBarButtonItemStyleDone target:self action:@selector(ms_dismissViewController:)];
            colorSelectionController.navigationItem.rightBarButtonItem = doneBtn;
        }
        
        [self presentViewController:navCtrl animated:YES completion:nil];
        
        
    }
}

-(void) updateSettingValueFromSliderValue:(int)settings_id slider_value:(float)value {
    float ret;
    float min=settings[settings_id].detail.mdz_slider.slider_min_value;
    float mid=settings[settings_id].detail.mdz_slider.slider_mid_value;
    float max=settings[settings_id].detail.mdz_slider.slider_max_value;
    
    if (value<=0.5) {
        ret=2*value;
        ret=ret*(mid-min)+min;
    } else {
        ret=2*(value-0.5);
        ret=ret*(max-mid)+mid;
    }
    
    settings[settings_id].detail.mdz_slider.slider_value=ret;
}
-(float) getSliderValueFromSettingValue:(int)settings_id {
    float ret;
    float min=settings[settings_id].detail.mdz_slider.slider_min_value;
    float mid=settings[settings_id].detail.mdz_slider.slider_mid_value;
    float max=settings[settings_id].detail.mdz_slider.slider_max_value;
    float value=settings[settings_id].detail.mdz_slider.slider_value;
    if (value<=mid) {
        ret=(value-min)/(mid-min)*0.5;
    } else {
        ret=(value-mid)/(max-mid)*0.5+0.5;
    }
    if (ret>1) ret=1;
    if (ret<0) ret=0;
    return ret;
}

- (void)segconChanged:(UISegmentedControl*)sender {
    int refresh=0;
    
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    if (settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value !=[(UISegmentedControl*)sender selectedSegmentIndex]) refresh=1;
    settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value=[(UISegmentedControl*)sender selectedSegmentIndex];
    
    if (settings[cur_settings_idx[indexPath.section]].callback) {
        settings[cur_settings_idx[indexPath.section]].callback(self);
    }
    if (refresh) [tableView reloadData];
}

-(void)sliderUpdateLabelValue:(UILabel *)lblValue digits:(char)digits value:(double)value {
    switch (digits) {
        case 0:
            lblValue.text=[NSString stringWithFormat:@"%.0lf",value];
            break;
        case 1:
            lblValue.text=[NSString stringWithFormat:@"%.1lf",value];
            break;
        case 2:
            lblValue.text=[NSString stringWithFormat:@"%.2lf",value];
            break;
        case 20:
            lblValue.text=[NSString stringWithFormat:@"%.2lf",round(value*20)/20];
            break;
        case 60:
            lblValue.text=[NSString stringWithFormat:@"%2d:%.2d",(int)(value/60),(int)(value)%60];
            break;
        case 100:
            lblValue.text=[NSString stringWithFormat:@"%2d%%",(int)(value*100)];
            break;
        default:
            lblValue.text=[NSString stringWithFormat:@"%.2lf",value];
            break;
    }
}

- (void)sliderChanged:(OBSlider*)sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    //settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value=((OBSlider*)sender).value;
    
//    if (settings[cur_settings_idx[indexPath.section]].callback) {
//        settings[cur_settings_idx[indexPath.section]].callback(self);
//    }
    
    UIView *masterView=[sender superview];
    UIView *v;
    for (v in [masterView subviews]) {
        if ([v isKindOfClass:[UILabel class]]) {
            UILabel *lblValue=(UILabel *)v;
            
            [self sliderUpdateLabelValue:lblValue digits:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits value:((OBSlider*)sender).value];
        }
    }
}

- (void)sliderTouchUp:(OBSlider*)sender {
    static bool noreentrant=false;
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    if ((settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits==0)||
        (settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits==60) ){
        
        settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value=round(((OBSlider*)sender).value);
    
        ((OBSlider*)sender).value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
    } else if (settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits==100 ){
        
        settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value=round(((OBSlider*)sender).value*100)/100;
        
        ((OBSlider*)sender).value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
    }else if (settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits==20 ){
        
        settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value=round(((OBSlider*)sender).value*settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits)/settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits;
        
        ((OBSlider*)sender).value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
    } else if ( settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits<9 ){
        
        settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value=round(((OBSlider*)sender).value*pow(10,settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits))/pow(10,settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits);
        
        ((OBSlider*)sender).value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
    } else {
        settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value=((OBSlider*)sender).value;
        ((OBSlider*)sender).value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
    }
    
    
    
    if (settings[cur_settings_idx[indexPath.section]].callback) {
        settings[cur_settings_idx[indexPath.section]].callback(self);
    }
}

- (void)longTimeSliderChanged:(OBSlider*)sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    [self updateSettingValueFromSliderValue:cur_settings_idx[indexPath.section] slider_value:((OBSlider*)sender).value];
    //settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value=((OBSlider*)sender).value;
    
//    if (settings[cur_settings_idx[indexPath.section]].callback) {
//        settings[cur_settings_idx[indexPath.section]].callback(self);
//    }
    
    UIView *masterView=[sender superview];
    UIView *v;
    for (v in [masterView subviews]) {
        if ([v isKindOfClass:[UILabel class]]) {
            UILabel *lblValue=(UILabel *)v;
            
            [self sliderUpdateLabelValue:lblValue digits:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits value:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value];
        }
    }
}

- (void)longTimeSliderTouchUp:(OBSlider*)sender {
    static bool noreentrant=false;
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    if ((settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits==0)||
        (settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits==60)||
        (settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits==100) ){
        settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value=round(settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value);
        ((OBSlider*)sender).value=[self getSliderValueFromSettingValue:cur_settings_idx[indexPath.section]];
    }
    
    if (settings[cur_settings_idx[indexPath.section]].callback) {
        settings[cur_settings_idx[indexPath.section]].callback(self);
    }
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
        //[self.tableView reloadData];
    }
    [self.tableView reloadData];
    return YES;
}

- (void)textFieldTextChanged:(UITextField *)textField {
    if (settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.max_width_char) {
        if (textField.text.length<=settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.max_width_char) return;
        
        NSInteger adaptedLength = settings[cur_settings_idx[textField.tag]].detail.mdz_textbox.max_width_char;
        NSRange range = NSMakeRange(0, adaptedLength);
        textField.text = [textField.text substringWithRange:range];
    }
}

- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    const NSInteger BTN_TAG = 1003;
    UILabel *topLabel,*bottomLabel;
    UITextField *msgLabel;
    
    UISwitch *switchview;
    UISegmentedControl *segconview;
    UITextField *txtfield;
    NSMutableArray *tmpArray;
    OBSlider *sliderview;
    UIButton *btn;
    UITapGestureRecognizer *tapLabelDesc;
    
    if (forceReloadCells) {
        while ([tabView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        forceReloadCells=false;
    }
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,50);
        
        [cell setBackgroundColor:[UIColor clearColor]];
        
        NSString *imgName=(darkMode?@"tabview_gradient50Black.png":@"tabview_gradient50.png");
        UIImage *image = [UIImage imageNamed:imgName];
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
        topLabel.font = [UIFont boldSystemFontOfSize:14];
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;;
        topLabel.opaque=TRUE;
        topLabel.numberOfLines=0;
        topLabel.userInteractionEnabled=true;
        
        bottomLabel = [[UILabel alloc] init];
        [cell.contentView addSubview:bottomLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        bottomLabel.tag = BOTTOM_LABEL_TAG;
        bottomLabel.backgroundColor = [UIColor clearColor];
        bottomLabel.font = [UIFont systemFontOfSize:12];
        bottomLabel.lineBreakMode=NSLineBreakByTruncatingTail;
        bottomLabel.opaque=TRUE;
        
        tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
        tapLabelDesc.delegate = self;
        
        [bottomLabel addGestureRecognizer:tapLabelDesc];
        bottomLabel.userInteractionEnabled=true;
                
        cell.accessoryView=nil;
        cell.accessoryType = UITableViewCellAccessoryNone;
        
        btn=[UIButton buttonWithType:UIButtonTypeCustom];
        [cell.contentView addSubview:btn];
        btn.tag=BTN_TAG;
    } else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        btn = (UIButton *)[cell viewWithTag:BTN_TAG];
        
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;
    }
    
    if (darkMode) {
        topLabel.textColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:1 green:1 blue:1 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.6 green:0.6 blue:0.6 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0];
    }
    
    btn.hidden=TRUE;
    
    if (settings[cur_settings_idx[indexPath.section]].description) {
        topLabel.frame= CGRectMake(4,
                                   0,
                                   tabView.bounds.size.width/**4/10*/,
                                   50);
        bottomLabel.frame= CGRectMake(4,
                                      38,
                                      tabView.bounds.size.width/**4/10*/,
                                      12);
        
        topLabel.text=NSLocalizedString(([NSString stringWithUTF8String:settings[cur_settings_idx[indexPath.section]].label]),@"");
        
        if (strstr(settings[cur_settings_idx[indexPath.section]].description,"\n")) {
            NSString *str=NSLocalizedString(([NSString stringWithUTF8String:settings[cur_settings_idx[indexPath.section]].description]),@"");
            str=(NSString *)[[str componentsSeparatedByString:@"\n"] firstObject];
            bottomLabel.text=[str stringByAppendingString:@"..."];
        }
        else bottomLabel.text=NSLocalizedString(([NSString stringWithUTF8String:settings[cur_settings_idx[indexPath.section]].description]),@"");
    } else {
        topLabel.frame= CGRectMake(4,
                                   0,
                                   tabView.bounds.size.width*4/10,
                                   50);
        bottomLabel.frame= CGRectMake(4,
                                      0,
                                      tabView.bounds.size.width*4/10,
                                      0);
        
        topLabel.text=NSLocalizedString(([NSString stringWithUTF8String:settings[cur_settings_idx[indexPath.section]].label]),@"");
        bottomLabel.text=@"";
    }
    
    tapLabelDesc=[[bottomLabel gestureRecognizers] firstObject];
    if (tapLabelDesc) [topLabel removeGestureRecognizer:tapLabelDesc];
    
    switch (settings[cur_settings_idx[indexPath.section]].type) {
        case MDZ_FAMILY:
            cell.accessoryView=nil;
            cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
            break;
        case MDZ_BOOLSWITCH:
            switchview = [[UISwitch alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,40)];
            [switchview addTarget:self action:@selector(boolswitchChanged:) forControlEvents:UIControlEventValueChanged];
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[switchview.description componentsSeparatedByString:@";"] firstObject]];
            switchview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            cell.accessoryView = switchview;
            //[switchview release];
            switchview.on=settings[cur_settings_idx[indexPath.section]].detail.mdz_boolswitch.switch_value;
            
            if (settings[cur_settings_idx[indexPath.section]].description) {
                tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
                tapLabelDesc.delegate = self;
                [topLabel addGestureRecognizer:tapLabelDesc];
            }
            break;
        case MDZ_SWITCH:{
            tmpArray=[[NSMutableArray alloc] init];
            for (int i=0;i<settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value_nb;i++) {
                [tmpArray addObject:NSLocalizedString(([NSString stringWithUTF8String:settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_labels[i]]),@"")];
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
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[segconview.description componentsSeparatedByString:@";"] firstObject]];
            cell.accessoryView = segconview;
            segconview.selectedSegmentIndex=settings[cur_settings_idx[indexPath.section]].detail.mdz_switch.switch_value;
            
            if (settings[cur_settings_idx[indexPath.section]].description) {
                tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
                tapLabelDesc.delegate = self;
                [topLabel addGestureRecognizer:tapLabelDesc];
            }
            
        }
            break;
        case MDZ_SLIDER_CONTINUOUS:{
            UIView *accview=[[UIView alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,50)];
            accview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            UILabel *lblValue=[[UILabel alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,12)];
            lblValue.font=[[lblValue font] fontWithSize:12];
            
            
            sliderview = [[OBSlider alloc] initWithFrame:CGRectMake(0,12,tabView.bounds.size.width*5.5f/10,30)];
            sliderview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            sliderview.continuous=true;
            [sliderview setMaximumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_max_value];
            [sliderview setMinimumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_min_value];
            [sliderview setContinuous:true];
            sliderview.value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
            [sliderview addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
            [sliderview addTarget:self action:@selector(sliderTouchUp:) forControlEvents:UIControlEventTouchUpInside];
            [sliderview addTarget:self action:@selector(sliderTouchUp:) forControlEvents:UIControlEventTouchUpOutside];
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[sliderview.description componentsSeparatedByString:@";"] firstObject]];
            
            [accview addSubview:lblValue];
            [accview addSubview:sliderview];
            cell.accessoryView = accview;
            
            [self sliderUpdateLabelValue:lblValue digits:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits value:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value];
            
            if (settings[cur_settings_idx[indexPath.section]].description) {
                tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
                tapLabelDesc.delegate = self;
                [topLabel addGestureRecognizer:tapLabelDesc];
            }
            break;
        }
        case MDZ_SLIDER_DISCRETE: {
            
            UIView *accview=[[UIView alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,50)];
            accview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            UILabel *lblValue=[[UILabel alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,12)];
            lblValue.font=[[lblValue font] fontWithSize:12];
            
            sliderview = [[OBSlider alloc] initWithFrame:CGRectMake(0,12,tabView.bounds.size.width*5.5f/10,30)];
            sliderview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            [sliderview setMaximumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_max_value];
            [sliderview setMinimumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_min_value];
            [sliderview setContinuous:true];
            sliderview.value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
            [sliderview addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
            [sliderview addTarget:self action:@selector(sliderTouchUp:) forControlEvents:UIControlEventTouchUpInside];
            [sliderview addTarget:self action:@selector(sliderTouchUp:) forControlEvents:UIControlEventTouchUpOutside];
            
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[sliderview.description componentsSeparatedByString:@";"] firstObject]];
            
            [accview addSubview:lblValue];
            [accview addSubview:sliderview];
            cell.accessoryView = accview;
            
            [self sliderUpdateLabelValue:lblValue digits:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits value:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value];
            
            if (settings[cur_settings_idx[indexPath.section]].description) {
                tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
                tapLabelDesc.delegate = self;
                [topLabel addGestureRecognizer:tapLabelDesc];
            }
            break;
        }
        case MDZ_SLIDER_DISCRETE_TIME:{
            UIView *accview=[[UIView alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,50)];
            accview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            UILabel *lblValue=[[UILabel alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,12)];
            lblValue.font=[[lblValue font] fontWithSize:12];
            
            sliderview = [[OBSlider alloc] initWithFrame:CGRectMake(0,12,tabView.bounds.size.width*5.5f/10,30)];
            sliderview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            [sliderview setMaximumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_max_value];
            [sliderview setMinimumValue:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_min_value];
            [sliderview setContinuous:true];
            
            sliderview.value=settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value;
            
            [sliderview addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
            [sliderview addTarget:self action:@selector(sliderTouchUp:) forControlEvents:UIControlEventTouchUpInside];
            [sliderview addTarget:self action:@selector(sliderTouchUp:) forControlEvents:UIControlEventTouchUpOutside];
            
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[sliderview.description componentsSeparatedByString:@";"] firstObject]];
            
            [accview addSubview:lblValue];
            [accview addSubview:sliderview];
            cell.accessoryView = accview;
            
            [self sliderUpdateLabelValue:lblValue digits:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits value:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value];
            
            if (settings[cur_settings_idx[indexPath.section]].description) {
                tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
                tapLabelDesc.delegate = self;
                [topLabel addGestureRecognizer:tapLabelDesc];
            }
            break;
        }
        case MDZ_SLIDER_DISCRETE_TIME_LONG:{
            UIView *accview=[[UIView alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,50)];
            accview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            UILabel *lblValue=[[UILabel alloc] initWithFrame:CGRectMake(0,0,tabView.bounds.size.width*5.5f/10,12)];
            lblValue.font=[[lblValue font] fontWithSize:12];
            
            sliderview = [[OBSlider alloc] initWithFrame:CGRectMake(0,12,tabView.bounds.size.width*5.5f/10,30)];
            sliderview.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleLeftMargin;
            [sliderview setMaximumValue:1];
            [sliderview setMinimumValue:0];
            [sliderview setContinuous:true];
            
            sliderview.value=[self getSliderValueFromSettingValue:cur_settings_idx[indexPath.section]];
            
            [sliderview addTarget:self action:@selector(longTimeSliderChanged:) forControlEvents:UIControlEventValueChanged];
            [sliderview addTarget:self action:@selector(longTimeSliderTouchUp:) forControlEvents:UIControlEventTouchUpInside];
            [sliderview addTarget:self action:@selector(longTimeSliderTouchUp:) forControlEvents:UIControlEventTouchUpOutside];
            
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[sliderview.description componentsSeparatedByString:@";"] firstObject]];
            
            [accview addSubview:lblValue];
            [accview addSubview:sliderview];
            cell.accessoryView = accview;
            
            [self sliderUpdateLabelValue:lblValue digits:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_digits value:settings[cur_settings_idx[indexPath.section]].detail.mdz_slider.slider_value];
            
            if (settings[cur_settings_idx[indexPath.section]].description) {
                tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
                tapLabelDesc.delegate = self;
                [topLabel addGestureRecognizer:tapLabelDesc];
            }
            break;
        }
        case MDZ_TEXTBOX: {
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
            
            if (settings[cur_settings_idx[indexPath.section]].detail.mdz_textbox.text) txtfield.text=[NSString stringWithUTF8String:settings[cur_settings_idx[indexPath.section]].detail.mdz_textbox.text];
            else txtfield.text=@"";
            
            UIFont *font = [UIFont systemFontOfSize:15];
            NSDictionary *userAttributes = @{NSFontAttributeName: font,
                                             NSForegroundColorAttributeName: [UIColor blackColor]};
            CGSize textSize = [txtfield.text sizeWithAttributes: userAttributes];
            textSize.width*=2;
            if (textSize.width<tabView.bounds.size.width*2.5f/10) textSize.width=tabView.bounds.size.width*2.5f/10;
            if (textSize.width>tabView.bounds.size.width*5.5f/10) textSize.width=tabView.bounds.size.width*5.5f/10;
            txtfield.frame=CGRectMake(0,0,textSize.width,30);
            
            [txtfield addTarget:self
                         action:@selector(textFieldTextChanged:)
               forControlEvents:UIControlEventEditingChanged];
            
            cell.accessoryView = txtfield;
            
            if (settings[cur_settings_idx[indexPath.section]].description) {
                tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
                tapLabelDesc.delegate = self;
                [topLabel addGestureRecognizer:tapLabelDesc];
            }
            break;
        }
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
            
            if (settings[cur_settings_idx[indexPath.section]].detail.mdz_msgbox.text) msgLabel.text=[NSString stringWithUTF8String:settings[cur_settings_idx[indexPath.section]].detail.mdz_textbox.text];
            else msgLabel.text=@"";
            cell.accessoryView = msgLabel;
            
            if (settings[cur_settings_idx[indexPath.section]].description) {
                tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
                tapLabelDesc.delegate = self;
                [topLabel addGestureRecognizer:tapLabelDesc];
            }
            break;
        case MDZ_COLORPICKER:{
            
            //UIControlState
            [btn setTitle:@"Reset" forState:UIControlStateNormal];
            [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
            if (darkMode) {
                btn.backgroundColor=[UIColor darkGrayColor];
                btn.layer.borderColor = [UIColor grayColor].CGColor;
            }
            else {
                btn.backgroundColor=[UIColor lightGrayColor];
                btn.layer.borderColor = [UIColor grayColor].CGColor;
            }
            
            btn.layer.borderWidth = 0.5f;
            btn.layer.cornerRadius = 10.0f;
            
            
            [btn addTarget:self action: @selector(resetColor:) forControlEvents: UIControlEventTouchUpInside];
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[btn.description componentsSeparatedByString:@";"] firstObject]];
            
            int icon_posx=tabView.bounds.size.width-2-tabView.safeAreaInsets.right-tabView.safeAreaInsets.left;
            icon_posx-=32+32+60;
            btn.frame = CGRectMake(icon_posx,14,60,28);
            btn.titleLabel.font=[btn.titleLabel.font fontWithSize:14];
            
            btn.enabled=YES;
            btn.hidden=NO;
            
            if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"14.0")) {
                if (@available(iOS 14.0, *)) {
                    int rgb=settings[cur_settings_idx[indexPath.section]].detail.mdz_color.rgb;
                    UIColorWell *colorWell= [[UIColorWell alloc] initWithFrame:CGRectMake(0,0,40,40)];
                    colorWell.supportsAlpha=false;
                    
                    [colorWell addTarget:self action:@selector(colorPickerSelected:) forControlEvents:UIControlEventValueChanged];
                    
                    colorWell.selectedColor=[UIColor colorWithRed:((rgb>>16)&0xFF)*1.0f/255.0f
                                                            green:((rgb>>8)&0xFF)*1.0f/255.0f
                                                             blue:((rgb>>0)&0xFF)*1.0f/255.0f
                                                            alpha:1];
                    
                    [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[colorWell.description componentsSeparatedByString:@";"] firstObject]];
                    cell.accessoryView = colorWell;
                }
            } else {
                int rgb=settings[cur_settings_idx[indexPath.section]].detail.mdz_color.rgb;
                UIButton *colorBtn=[UIButton buttonWithType:UIButtonTypeCustom];
                colorBtn.layer.borderColor = [UIColor whiteColor].CGColor;
                colorBtn.layer.borderWidth = 1.0f;
                colorBtn.layer.cornerRadius = 6.0f;
                
                [colorBtn setFrame:CGRectMake(0,0,40,40)];
                [colorBtn setBackgroundColor:[UIColor colorWithRed:((rgb>>16)&0xFF)*1.0f/255.0f
                                                             green:((rgb>>8)&0xFF)*1.0f/255.0f
                                                              blue:((rgb>>0)&0xFF)*1.0f/255.0f
                                                             alpha:1]];
                [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[colorBtn.description componentsSeparatedByString:@";"] firstObject]];
                
                [colorBtn addTarget:self action:@selector(colorPickerOpen:) forControlEvents:UIControlEventTouchUpInside];
                
                
                cell.accessoryView = colorBtn;
            }
            
            if (settings[cur_settings_idx[indexPath.section]].description) {
                tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
                tapLabelDesc.delegate = self;
                [topLabel addGestureRecognizer:tapLabelDesc];
            }
            break;
        }
        case MDZ_BUTTON:
            //UIControlState
            cell.accessoryView=nil;
            if (settings[cur_settings_idx[indexPath.section]].mdz_selector) {
                [btn setTitle:@"Reset" forState:UIControlStateNormal];
                [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
                if (darkMode) {
                    btn.backgroundColor=[UIColor darkGrayColor];
                    btn.layer.borderColor = [UIColor grayColor].CGColor;
                }
                else {
                    btn.backgroundColor=[UIColor lightGrayColor];
                    btn.layer.borderColor = [UIColor grayColor].CGColor;
                }
                
                btn.layer.borderWidth = 0.5f;
                btn.layer.cornerRadius = 10.0f;
                
                
                [btn addTarget:self action:settings[cur_settings_idx[indexPath.section]].mdz_selector forControlEvents: UIControlEventTouchUpInside];
                [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[btn.description componentsSeparatedByString:@";"] firstObject]];
                
                int icon_posx=tabView.bounds.size.width-2-tabView.safeAreaInsets.right-tabView.safeAreaInsets.left;
                icon_posx-=32+32+60;
                btn.frame = CGRectMake(icon_posx,14,60,28);
                btn.titleLabel.font=[btn.titleLabel.font fontWithSize:14];
                
                btn.enabled=YES;
                btn.hidden=NO;
                
                if (settings[cur_settings_idx[indexPath.section]].description) {
                    tapLabelDesc = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapPress:)];
                    tapLabelDesc.delegate = self;
                    [topLabel addGestureRecognizer:tapLabelDesc];
                }
                break;
        }
    }
    
    bottomLabel.frame= CGRectMake(4,
                                  38,
                                  tabView.bounds.size.width-cell.accessoryView.frame.size.width-16,
                                  12);
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
        settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[cur_settings_idx[indexPath.section]].label]),@"");
        settingsVC->current_family=settings[cur_settings_idx[indexPath.section]].sub_family;
        settingsVC.view.frame=self.view.frame;
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
    [waitingView removeFromSuperview];
    [waitingViewPlayer removeFromSuperview];
    waitingView=nil;
    waitingViewPlayer=nil;
    
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

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    }
}


#pragma mark - UINavigationControllerDelegate

- (id <UIViewControllerAnimatedTransitioning>)navigationController:(UINavigationController *)navigationController
                                   animationControllerForOperation:(UINavigationControllerOperation)operation
                                                fromViewController:(UIViewController *)fromVC
                                                  toViewController:(UIViewController *)toVC
{
    return [[TTFadeAnimator alloc] init];
}

#pragma mark - UIColorPickerViewControllerDelegate

//IOS 15+
- (void)colorPickerViewController:(UIColorPickerViewController *)viewController
                   didSelectColor:(UIColor *)color
                     continuously:(BOOL)continuously {
    
}

//iOS 14, deprecated from iOS 15
- (void)colorPickerViewControllerDidSelectColor:(UIColorPickerViewController *)viewController {
    
}

#pragma mark - MSColorViewDelegate

- (void)colorViewController:(MSColorSelectionViewController *)colorViewCntroller didChangeColor:(UIColor *)color
{
    currentColorPickerBtn.backgroundColor = color;
    //UIButton *colorBtn=(UIButton*)(navCtrl.popoverPresentationController.sourceView);
}

#pragma mark - Private

- (void)ms_dismissViewController:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
    [self colorPickerSelected:currentColorPickerBtn];
}

#pragma mark CMPopTipViewDelegate methods
- (void)popTipViewWasDismissedByUser:(CMPopTipView *)_popTipView {
    // User can tap CMPopTipView to dismiss it
    self.popTipView = nil;
}

#pragma mark - LoadingView related stuff

- (void) cancelPushed {
    detailViewController.mplayer.extractPendingCancel=true;
    [detailViewController setCancelStatus:true];
    [detailViewController hideWaitingCancel];
    [detailViewController hideWaitingProgress];
    [detailViewController updateWaitingDetail:NSLocalizedString(@"Cancelling...",@"")];
        
    [self hideWaitingCancel];
    [self hideWaitingProgress];
    [self updateWaitingDetail:NSLocalizedString(@"Cancelling...",@"")];
}

-(void) updateLoadingInfos: (NSTimer *) theTimer {
    [waitingViewPlayer.progressView setProgress:detailViewController.waitingView.progressView.progress animated:YES];
}


- (void) observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                       context:(void *)context
{
    if (context==LoadingProgressObserverContext){
        WaitingView *wv = object;
        if (wv.hidden==false) {
            [waitingViewPlayer resetCancelStatus];
            waitingViewPlayer.hidden=detailViewController.waitingView.hidden;
            waitingViewPlayer.btnStopCurrentAction.hidden=detailViewController.waitingView.btnStopCurrentAction.hidden;
            waitingViewPlayer.progressView.progress=detailViewController.waitingView.progressView.progress;
            waitingViewPlayer.progressView.hidden=detailViewController.waitingView.progressView.hidden;
            waitingViewPlayer.lblTitle.text=[NSString stringWithString:detailViewController.waitingView.lblTitle.text];
            waitingViewPlayer.lblDetail.text=[NSString stringWithString:detailViewController.waitingView.lblDetail.text];
        }
        waitingViewPlayer.hidden=wv.hidden;
        
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

@end
