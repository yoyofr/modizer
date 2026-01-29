//
//  DetailViewController.mm
//  modizer
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//
//#define PM_TEST_LOAD 64

#define FXSLOT_LPTOUCH_ACTIVTATION_DELAY 1.5  //in seconds
#define FXSLOT_LPTOUCH_CYCLING_DELAY 1.5  //in seconds, has to be <= FXSLOT_LPTOUCH_ACTIVTATION_DELAY

#define FX_AUTO_SCALING_DELAY_ZOOMIN_FAST 60 //frame delay before zooming in for FX auto scaling
#define FX_AUTO_SCALING_DELAY_ZOOMIN_SLOW 120 //frame delay before zooming in for FX auto scaling

#define PM_FRAMETIME_LIMIT (1000.0f/10.0f) // max allowed frame time in ms, if regularly above, PM will be deactivated
#define PM_FRAMETIME_LIMIT_WEAK 100 //Max slow frames allowed for 'weak' mode
#define PM_FRAMETIME_LIMIT_STRONG 10 //Max slow frames allowed for 'strong' mode

#define PM_PRESET_DISPLAY_TIMEOUT 10 //Display time in seconds of preset's name when in temporary display mode
#define FX_FS_SONGINFO_TIMEOUT 5 //Display time in seconds of song info data in fullscreen mode
//#define MDZ_FX_SONGINFO_MAXCHAR 80

#define FX_FS_GUIMESSAGE_TIMEOUT 2

#define UI_RADIO_INFO_HEIGHT 17+14+14

#define POPUP_STYLE_INFO 0
#define POPUP_STYLE_ALERT 1

#define SHOWINFO_SECTION1_SIZE 26
#define SHOWINFO_SECTION2_SIZE 44

#define FONTSIZE_PM_PRESET_INFO_LINE 18
#define FONTSIZE_SHOWINFO_FPS 24
#define FONTSIZE_SHOWINFO_DETAILS 16
#define FONTSIZE_FX_FS_INFO_LINE 18
#define FONTSIZE_FX_FS_INFO_LINE_DIVIDER 60 //42
#define FONTSIZE_GUIMSESSAGE 40

#define SHOWINFO_FPS_COLOR 0.2,1.0,0.1
#define SHOWINFO_CPU_COLOR 83.0/255.0,182.0/255.0,235.0/255.0

#define SHOWINFO_FXVIEW_COLOR 223.0/255.0,176.0/255.0,173.0/255.0
#define SHOWINFO_FXVIEWRES_COLOR 238.0/255.0,186.0/255.0,65.0/255.0

#define SHOWINFO_DEVICE_COLOR 223.0/255.0,176.0/255.0,173.0/255.0
#define SHOWINFO_DEVICERES_COLOR 238.0/255.0,186.0/255.0,65.0/255.0

#define SHOWINFO_PM_COLOR 223.0/255.0,176.0/255.0,173.0/255.0
#define SHOWINFO_PMRES_COLOR 238.0/255.0,186.0/255.0,65.0/255.0
#define SHOWINFO_PMAUDIO_COLOR 253.0/255.0,253.0/255.0,253.0/255.0

#define SHOWINFO_FXFRAME_COLOR 223.0/255.0,176.0/255.0,173.0/255.0
#define SHOWINFO_FXFRAMEINFO_COLOR 253.0/255.0,253.0/255.0,253.0/255.0

float varCheck[4];

extern unsigned int mdzRenderbuffer;

int mdz_pmMilkPermissiveEvalCode;//,mdz_pmBlurAfterAudio;
int mdz_pmTexturesSearchPathsNb;
const char *mdz_pmTexturesSearchPaths[5];

extern float camera_posX,camera_posY,camera_posZ;
extern float camera_lookX,camera_lookY,camera_lookZ;
float header_w;
float fontWidth;
int deactivateFStemp;

#define ASCII_MIDDOT "·"

#define PM_HorizontalSwipe_Threshold 120
#define PM_VerticalSwipe_Threshold 120

#define SELECTOR_TABVIEWCELL_HEIGHT 50
#define ARCSUB_MODE_NONE 0
#define ARCSUB_MODE_ARC 1
#define ARCSUB_MODE_SUB 2
#define ARCSUB_MODE_RADIO 3

static int current_selmode;
int MIDIFX_OFS;

#include <pthread.h>
extern pthread_mutex_t db_mutex,gl_mutex;
mach_port_t mdzMainThreadId;
volatile bool mdzRenderInProgress;

#import "SysMonitoring.h"

// Notification
#import <UserNotifications/UserNotifications.h>
#import "Notifications.h"

#import "DirParser.h"
FileNode *pmBundledPresetsFileNode;
FileNode *pmCustomPresetsFileNode;

extern BOOL nvdsp_EQ;

#if TARGET_OS_MACCATALYST
#import <Cocoa/Cocoa.h>
#endif

#import <mach/mach.h>
#import <mach/mach_host.h>
#import "FFTAccelerate.h"
static FFTAccelerate *fftAccel;
static float *fft_frequency,*fft_time,*fft_frequencyAvg;
static int *fft_freqAvgCount;

#import "AppDelegate_Phone.h"

#import "ModizFileHelper.h"

#import "RadioSource.h"

#define LOCATION_UPDATE_TIMING 1800 //in second : 30minutes
//#define NOTES_DISPLAY_LEFTMARGIN 30
int NOTES_DISPLAY_LEFTMARGIN=30;
int NOTES_DISPLAY_TOPMARGIN=30;


#include <sys/types.h>
#include <sys/sysctl.h>

#include "DBHelper.h"

#define DEBUG_INFOS 1
#define DEBUG_NO_SETTINGS 0

#include "ModizerTypes.h"

//#include "OGLView.h"
#include "RenderUtils.h"

extern unsigned int data_midifx_pal1[32];
extern unsigned int data_midifx_pal2[32];
extern unsigned int data_midifx_pal3[32];
extern unsigned int data_midifx_pal_custom[32];

extern unsigned int m_voice_oscillo_pal1[8];
extern unsigned int m_voice_oscillo_pal2[8];
extern unsigned int m_voice_oscillo_pal3[8];

#include <QuartzCore/CADisplayLink.h>
#import <QuartzCore/QuartzCore.h>

#import "UIImage+ImageEffects.h"

#import "UIImageResize.h"

#import "C64PICDecoder.h"
#import "C64PGGDecoder.h"
#import "C64PJJDecoder.h"

#import "DetailViewControllerIphone.h"
#import "RootViewControllerPlaylist.h"
#import "myTabBarController.h"
#import "CarPlayAndRemoteManagement.h"
#import <MediaPlayer/MediaPlayer.h>

#import "EQViewController.h"

//#import "../libs/libopenmpt/openmpt-trunk/include/modplug/include/libmodplug/modplug.h"

#include "ModizerVoicesData.h"

int fxSlot[FX_MAX];

#include "TextureUtils.h"
/*----------------------------------------------*/
#include <stdint.h>
#include <string>

#include <projectM-4/audio.h>
#include <projectM-4/projectM.h>
//#include <projectM-4/playlist.h>
#include <GLES3/gl3.h>

bool _pmIsInitialized;
bool _pmFirstInitDone;
double _fx_frame_time;

int _fx_frame_timeOverLimitCounter;
int _pm_shouldRestartAt;
int _pmCanvasWidth,_pmCanvasHeight;
projectm_handle _pm; //!< Pointer to the projectM instance used by the application.
//projectm_playlist_handle _pm_playlist; //!< Pointer to the projectM playlist manager instance.
MDZPlaylist *_mdzPM_playlist;
MDZFavorites *_mdzPM_Favorites;
bool _pmFavoritesChanged;
bool _pm_playlist_loadBundled,_pm_playlist_loadCustom;
NSString *pmCurPresetFile;
int _pm_display_name_countdown;

int _mdz_display_songinfo_countdown,_mdz_FS_display_cursorLine;
int _mdz_display_songinfo_char_count[6]={1,1,1,1,1,1};
float _mdz_FX_GuiMessage_fade,_mdz_FX_GuiMessage_fadeMax;
char _mdz_FX_GuiMessageStr[64];

NSString *_mdz_display_songinfo_title;
NSString *_mdz_display_songinfo_artist;
NSString *_mdz_display_songinfo_sub;

float _pm_display_scrollx=0;
int _pm_display_scroll_direction=1;
int _pm_display_scroll_pause=0;
bool _pmPresetUpdateDisplayInfo;
bool _pmPresetNewLoaded;
//
static int _pm_fps=60;
static int meshX=32,meshY=24;
float glScaleFactor=1.0;

static bool mBackground;
static bool mBackground_oglViewWasHidden;

//--------------------------------------------------
// ImGui
//--------------------------------------------------
#include "../utils/imgui/imgui.h"
#include "../utils/imgui/backends/imgui_impl_ios.h"
#include "../utils/imgui/backends/imgui_impl_opengl3.h"

extern NSMutableArray *mac_key_pressed,*mac_key_released;

#include "MDZFontAwesome.h"

ImGui_ImplIOS_UI *imGui_impl_ios;

extern float mdz_font_size[4];
extern ImFont  *font_menu;
extern ImFont  *font_tracker[FONT_TRACKER_NB];
extern ImFont  *font_trackerH[FONT_TRACKER_NB];
extern float font_trackerSize[FONT_TRACKER_NB][5];

//--------------------------------------------------

#include "PlayerMenu.h"

#import "gme.h"

#import "math.h"

#import "Font.h"
#import "GLString.h"

#import "timidity.h"

#import "AnimatedGif.h"

#import "TTFadeAnimator.h"

extern "C" signed char *m_voice_buff_ana_cpy[SOUND_BUFFER_NB];

/*extern "C" {
 int fix_fftr(short int f[], int m, int inverse);
 int fix_fft(short int  fr[], short int  fi[], short int  m, short int  inverse);
 }*/

static RootViewControllerPlaylist *nowplayingPL;

extern volatile t_settings settings[MAX_SETTINGS];

extern unsigned int tim_notes_cpy[SOUND_BUFFER_NB][DEFAULT_VOICES];
extern unsigned char tim_voicenb_cpy[SOUND_BUFFER_NB];
extern char mplayer_error_msg[1024];
float tim_midifx_note_range,tim_midifx_note_offset,tim_midifx_length;
bool tim_midifx_note_offset_reset;

float mScaleInfo[5];

float prollfx_note_range,prollfx_noteroll_offset,prollfx_length;
bool prollfx_note_offset_reset;

extern volatile int db_checked;

static int shouldRestart;
static int shouldUpdateCoverTexture;

//static volatile int locManager_isOn;
static BOOL mOglViewIsHidden;
static volatile int mSendStatTimer;
static NSDate *locationLastUpdate=nil;
static int mOglView1Tap=0;

float mDevice_hh,mDevice_ww;
static int mShouldHaveFocusAfterBackground,mLoadIssueMessage;
static int infoIsFullscreen=0;

static MPMediaItemArtwork *artwork;

static char voicesName[SOUND_MAXVOICES_BUFFER_FX*32];

//int texturePiano;

static volatile int mPopupAnimation=0;

static volatile int alertCannotPlay_displayed;

static int pmenu_fade=0;
static int pmenu_show=0;
extern int pMenu_fullscreenStatus;
static int oglv_corner_fade[4];

#define TOUCH_KEPT_THRESHOLD 10

#define max2(a,b) (a>b?a:b)
#define max4(a,b,c,d) max2(max2(a,b),max2(c,d))
#define max8(a,b,c,d,e,f,g,h) max2(max4(a,b,c,d),max4(e,f,g,h))

static int display_length_mode=0;

UIImage *backgroundImage;

static int updMPNowCnt=0;

@implementation DetailViewControllerIphone

@synthesize btnAddToPl,btnSaveFile,btnRadioPrevList;
@synthesize radioSource;
@synthesize mLoopMode;
@synthesize waitingView;
@synthesize cover_img,default_cover;
@synthesize bShowVC,bShowEQ;
@synthesize infoButton,eqButton;
@synthesize mShuffle,mShouldUpdateInfos;
@synthesize mOnlyCurrentSubEntry,mOnlyCurrentEntry;

@synthesize mDeviceType;
@synthesize is_macOS,is_iPad;
@synthesize cover_view,cover_viewBG,cover_viewAll,gifAnimation;
//@synthesize locManager;
@synthesize infoMsgView,infoMsgLbl,infoSecMsgLbl;
@synthesize mIsPlaying,mPaused,mplayer,mPlaylist;
@synthesize labelModuleLength, labelTime, labelModuleSize,textMessage,labelNumChannels,labelModuleType,labelSeeking,labelLibName;
@synthesize buttonLoopTitleSel,buttonLoopList,buttonLoopListSel,buttonShuffle,buttonShuffleSel,buttonShuffleOneSel,btnLoopInf;
@synthesize repeatingTimer;
@synthesize sliderProgressModule;
@synthesize detailView,commandViewU,playlistPos;
@synthesize playBar,pauseBar,playBarSub,pauseBarSub;

@synthesize mainView,infoView;
@synthesize mainRating5,mainRating5off;
@synthesize mHasFocus,mScaleFactor;
@synthesize backInfo;
@synthesize mPlaylist_pos,mPlaylist_size;

@synthesize oglButton;

@synthesize btnShowSubSong,btnShowArcList,btnShowVoices,btnRecordScreen;

@synthesize infoZoom,infoUnzoom;
@synthesize mInWasView;

@synthesize not_expected_version;

SysMonitoring *sysMonitor;
bool sysMonitorIsActive;

-(void) refreshCurrentVC {
    UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
    if ([vc respondsToSelector:@selector(refreshMiniplayer)]) [vc performSelector:@selector(refreshMiniplayer)];
    
    //also check if voices control UI is to be updated
    if (voicesVC) {
        [voicesVC resetVoicesButtons];
        [voicesVC recomputeFrames];
    }
}

-(void)didSelectRowInAlertSubController:(NSInteger)row {
    mPaused=0;
    //[self play_curEntry:(int)row+mplayer.mod_minsub];
    [mplayer playGoToSub:(int)row+mplayer.mod_minsub];
    clearAudioFXbuffer=true;
    _seekRequested=-1;
    
    double delayInSeconds = 0.5;
    dispatch_time_t delay = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
    
    dispatch_after(delay, dispatch_get_main_queue(), ^{
        // Code to run after 2 seconds
        if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
            [self sendNotifPlayedTitle];
        }
        
    });
}

-(void)didSelectRowInAlertRadioController:(NSInteger)row {
    if (row>0 ) [radioSource movePrev:row-1];
    else [radioSource startCurrent];
    if ([radioSource isInLibrary:0]) [btnSaveFile setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    else [btnSaveFile setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [self updRadioInfo];
}


-(void) cancelSubSel {
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
}

-(IBAction)changeScreenRecorderFlag {
    if (bRSactive) {
        if (isRecordingScreen!=RS_NOT_RECORDING) {
            [self StopRecording];
        }
    } else {
        if (isRecordingScreen==RS_NOT_RECORDING) {
            [self StartRecording];
        }
    }
}

-(IBAction)showVoicesSelector:(id)sender {
    if (bShowVC) {
        [voicesVC viewWillDisappear:YES];
        [voicesVC.view removeFromSuperview];
        [voicesVC removeFromParentViewController];
        voicesVC=nil;
    } else {
        voicesVC = [[VoicesViewController alloc]  initWithNibName:@"VoicesViewController" bundle:[NSBundle mainBundle]];
        //set new title
        voicesVC.title = NSLocalizedString(@"Voices control",@"Voices control");
        voicesVC.detailViewController=self;
        
        voicesVC.view.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y,mainView.frame.size.width,m_oglView.frame.size.height);
        // And push the window
        //[self.navigationController pushViewController:voicesVC animated:YES];
        
        [self addChildViewController:voicesVC];
        [self.view addSubview:voicesVC.view];
    }
}

-(IBAction)showSubSongSelector:(id)sender {
    UIViewController *controller = [[UIViewController alloc]init];
    CGRect rect,recttv;
    const NSInteger kAlertTableViewTag = 10001;
    
    current_selmode=ARCSUB_MODE_SUB;
    
    float rw,rh,rx,ry;
    if (self.view.traitCollection.horizontalSizeClass==UIUserInterfaceSizeClassCompact) {
        float estimated_height=SELECTOR_TABVIEWCELL_HEIGHT*(mplayer.mod_subsongs+1)+32;
        rx=0;
        ry=32;
        rw=self.view.frame.size.width;
        
        if (estimated_height<self.view.frame.size.height-50-ry) rh=estimated_height;
        else rh=self.view.frame.size.height-50-ry;
        rect = CGRectMake(rx, ry,rw,rh+50);
        recttv = CGRectMake(rx, ry,rw,rh);
    } else {
        float estimated_height=SELECTOR_TABVIEWCELL_HEIGHT*(mplayer.mod_subsongs+1)+16;
        
        rw=self.view.frame.size.width;
        if (estimated_height<self.view.frame.size.height*0.8f-100) rh=estimated_height;
        else rh=self.view.frame.size.height*0.8f-100;
        rect = CGRectMake(rw*0.15f, 0,rw*0.7f,rh+100);
        recttv = CGRectMake(0, 16,rw*0.7f,rh);
        
    }
    [controller setPreferredContentSize:rect.size];
    
    controller.modalPresentationStyle=UIModalPresentationPopover;
    
    UIView *containerView=[[UIView alloc] initWithFrame:recttv];
    alertTableView  = [[UITableView alloc] initWithFrame:containerView.bounds];
    containerView.backgroundColor = [UIColor clearColor];
    //containerView.layer.shadowColor = [[UIColor darkGrayColor] CGColor];
    //containerView.layer.shadowOffset = CGSizeMake(2.0,2.0);
    //containerView.layer.shadowOpacity = 1.0;
    //containerView.layer.shadowRadius = 2;
    
    alertTableView.layer.cornerRadius = 10;
    alertTableView.layer.masksToBounds = true;
    alertTableView.translatesAutoresizingMaskIntoConstraints = NO;
    [containerView addSubview:alertTableView];
    
    alertTableView.delegate = self;
    alertTableView.dataSource = self;
    alertTableView.tableFooterView = [[UIView alloc]initWithFrame:CGRectZero];
    alertTableView.rowHeight=SELECTOR_TABVIEWCELL_HEIGHT;
    alertTableView.sectionHeaderHeight=32;
    
    [alertTableView setSeparatorStyle:UITableViewCellSeparatorStyleSingleLine];
    [alertTableView setTag:kAlertTableViewTag];
    
    containerView.translatesAutoresizingMaskIntoConstraints = NO;
    [controller.view addSubview:containerView];// alertTableView];
    
    
    [controller.view bringSubviewToFront:containerView];//alertTableView];
    [controller.view setUserInteractionEnabled:YES];
    [alertTableView setUserInteractionEnabled:YES];
    [alertTableView setAllowsSelection:YES];
    
    BButton *cancel_btn= [[BButton alloc] initWithFrame:CGRectMake(0, 0, 200, 30)];
    [cancel_btn setType:BButtonTypePrimary];
    [cancel_btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
    [cancel_btn addTarget:self action:@selector(cancelSubSel) forControlEvents:UIControlEventTouchUpInside];
    [cancel_btn setTitle:NSLocalizedString(@"Cancel", @"Cancel Action") forState:UIControlStateNormal];
    cancel_btn.translatesAutoresizingMaskIntoConstraints = NO;
    [controller.view addSubview:cancel_btn];
    
    NSDictionary * viewsDic = NSDictionaryOfVariableBindings(cancel_btn, containerView);
    
    // Contraintes horizontales pour le containerView
    NSArray * hConstraintsContainer = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|[containerView]|"
                                                                              options:0
                                                                              metrics:nil
                                                                                views:viewsDic];
    [controller.view addConstraints:hConstraintsContainer];
    
    // Contraintes horizontales pour le bouton
    NSArray * hConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|-50-[cancel_btn]-50-|"
                                                                     options:0
                                                                     metrics:nil
                                                                       views:viewsDic];
    [controller.view addConstraints:hConstraints];
    
    // Contraintes verticales pour positionner containerView en haut et cancel_btn en bas
    NSArray * vConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-16-[containerView]-8-[cancel_btn(50)]-16-|"
                                                                     options:0
                                                                     metrics:nil
                                                                       views:viewsDic];
    [controller.view addConstraints:vConstraints];
    
    // Contraintes pour alertTableView à l'intérieur de containerView
    [NSLayoutConstraint activateConstraints:@[
        [alertTableView.topAnchor constraintEqualToAnchor:containerView.topAnchor],
        [alertTableView.bottomAnchor constraintEqualToAnchor:containerView.bottomAnchor],
        [alertTableView.leadingAnchor constraintEqualToAnchor:containerView.leadingAnchor],
        [alertTableView.trailingAnchor constraintEqualToAnchor:containerView.trailingAnchor]
    ]];
    
    [self presentViewController:controller animated:YES completion:^{
        NSIndexPath *indexPath = [NSIndexPath indexPathForRow:mplayer.mod_currentsub-mplayer.mod_minsub inSection:0];
        [alertTableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionMiddle];
    }];
    
    UIButton *btn=(UIButton*)sender;
    UIPopoverPresentationController *popoverctrl=controller.popoverPresentationController;
    popoverctrl.sourceView = btn;
    popoverctrl.sourceRect = CGRectMake(0, 0, btn.frame.size.width, btn.frame.size.height);
    if (self.view.traitCollection.horizontalSizeClass==UIUserInterfaceSizeClassCompact) {
        popoverctrl.backgroundColor=[UIColor blackColor];
    } else {
        popoverctrl.backgroundColor=[UIColor clearColor];
    }
    
    popoverctrl.delegate=self;
    //popoverctrl.permittedArrowDirections=UIPopoverArrowDirectionUp;
}

-(void)didSelectRowInAlertArcController:(NSInteger)row {
    [mplayer selectArcEntry:(int)row];
    
    [self showWaitingLoading];
    
    [self play_loadArchiveModule];
    [self hideWaiting];
    clearAudioFXbuffer=true;
    _seekRequested=-1;
}

-(void) cancelArcSel {
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
}

-(IBAction)showArcSelector:(id)sender {
    UIViewController *controller = [[UIViewController alloc]init];
    
    CGRect rect,recttv;
    const NSInteger kAlertTableViewTag = 10001;
    
    current_selmode=ARCSUB_MODE_ARC;
    
    float rw,rh,rx,ry;
    if (self.view.traitCollection.horizontalSizeClass==UIUserInterfaceSizeClassCompact) {
        float estimated_height=SELECTOR_TABVIEWCELL_HEIGHT*([mplayer getArcEntriesCnt]+1)+32;
        rx=0;
        ry=32;
        rw=self.view.frame.size.width;
        
        if (estimated_height<self.view.frame.size.height-50-ry) rh=estimated_height;
        else rh=self.view.frame.size.height-50-ry;
        rect = CGRectMake(rx, ry,rw,rh+50);
        recttv = CGRectMake(rx, ry,rw,rh);
        
    } else {
        float estimated_height=SELECTOR_TABVIEWCELL_HEIGHT*([mplayer getArcEntriesCnt]+1)+16;
        
        rw=self.view.frame.size.width;
        if (estimated_height<self.view.frame.size.height*0.8f-100) rh=estimated_height;
        else rh=self.view.frame.size.height*0.8f-100;
        rect = CGRectMake(rw*0.15f, 0,rw*0.7f,rh+100);
        recttv = CGRectMake(0, 16,rw*0.7f,rh);
        
    }
    [controller setPreferredContentSize:rect.size];
    
    controller.modalPresentationStyle=UIModalPresentationPopover;
    
    //alertTableView  = [[UITableView alloc] initWithFrame:recttv];
    
    UIView *containerView=[[UIView alloc] initWithFrame:recttv];
    //self.tableView = UITableView(frame: containerView.bounds, style: .plain)
    alertTableView  = [[UITableView alloc] initWithFrame:containerView.bounds];
    containerView.backgroundColor = [UIColor clearColor];
    //containerView.layer.shadowColor = [[UIColor darkGrayColor] CGColor];
    //containerView.layer.shadowOffset = CGSizeMake(2.0,2.0);
    //containerView.layer.shadowOpacity = 1.0;
    //containerView.layer.shadowRadius = 2;
    
    alertTableView.layer.cornerRadius = 10;
    alertTableView.layer.masksToBounds = true;
    alertTableView.translatesAutoresizingMaskIntoConstraints = NO;
    [containerView addSubview:alertTableView];
    
    alertTableView.delegate = self;
    alertTableView.dataSource = self;
    alertTableView.tableFooterView = [[UIView alloc]initWithFrame:CGRectZero];
    alertTableView.rowHeight=SELECTOR_TABVIEWCELL_HEIGHT;
    alertTableView.sectionHeaderHeight=32;
    
    [alertTableView setSeparatorStyle:UITableViewCellSeparatorStyleSingleLine];
    [alertTableView setTag:kAlertTableViewTag];
    
    containerView.translatesAutoresizingMaskIntoConstraints = NO;
    [controller.view addSubview:containerView];// alertTableView];
    
    
    [controller.view bringSubviewToFront:containerView];//alertTableView];
    [controller.view setUserInteractionEnabled:YES];
    [alertTableView setUserInteractionEnabled:YES];
    [alertTableView setAllowsSelection:YES];
    
    
    
    BButton *cancel_btn= [[BButton alloc] initWithFrame:CGRectMake(0, 0, 200, 30)];
    [cancel_btn setType:BButtonTypePrimary];
    [cancel_btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
    [cancel_btn addTarget:self action:@selector(cancelArcSel) forControlEvents:UIControlEventTouchUpInside];
    [cancel_btn setTitle:NSLocalizedString(@"Cancel", @"Cancel Action") forState:UIControlStateNormal];
    cancel_btn.translatesAutoresizingMaskIntoConstraints = NO;
    [controller.view addSubview:cancel_btn];
    
    NSDictionary * viewsDic = NSDictionaryOfVariableBindings(cancel_btn, containerView);
    
    // Contraintes horizontales pour le containerView
    NSArray * hConstraintsContainer = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|[containerView]|"
                                                                              options:0
                                                                              metrics:nil
                                                                                views:viewsDic];
    [controller.view addConstraints:hConstraintsContainer];
    
    // Contraintes horizontales pour le bouton
    NSArray * hConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|-50-[cancel_btn]-50-|"
                                                                     options:0
                                                                     metrics:nil
                                                                       views:viewsDic];
    [controller.view addConstraints:hConstraints];
    
    // Contraintes verticales pour positionner containerView en haut et cancel_btn en bas
    NSArray * vConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-16-[containerView]-8-[cancel_btn(50)]-16-|"
                                                                     options:0
                                                                     metrics:nil
                                                                       views:viewsDic];
    [controller.view addConstraints:vConstraints];
    
    // Contraintes pour alertTableView à l'intérieur de containerView
    [NSLayoutConstraint activateConstraints:@[
        [alertTableView.topAnchor constraintEqualToAnchor:containerView.topAnchor],
        [alertTableView.bottomAnchor constraintEqualToAnchor:containerView.bottomAnchor],
        [alertTableView.leadingAnchor constraintEqualToAnchor:containerView.leadingAnchor],
        [alertTableView.trailingAnchor constraintEqualToAnchor:containerView.trailingAnchor]
    ]];
    
    [self presentViewController:controller animated:YES completion:^{
        NSIndexPath *indexPath = [NSIndexPath indexPathForRow:[mplayer getArcIndex] inSection:0];
        [alertTableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionMiddle];
    }];
    
    UIButton *btn=(UIButton*)sender;
    UIPopoverPresentationController *popoverctrl=controller.popoverPresentationController;
    popoverctrl.sourceView = btn;
    popoverctrl.sourceRect = CGRectMake(0, 0, btn.frame.size.width, btn.frame.size.height);
    if (self.view.traitCollection.horizontalSizeClass==UIUserInterfaceSizeClassCompact) {
        popoverctrl.backgroundColor=[UIColor blackColor];
    } else {
        popoverctrl.backgroundColor=[UIColor clearColor];
    }
    
    popoverctrl.delegate=self;
    
    //popoverctrl.permittedArrowDirections=UIPopoverArrowDirectionUp;
}

-(void)stopLoopInfOnError {
    mLoopMode=0;
    [mplayer setLoopInf:0];
    [btnLoopInf setTitleColor:[UIColor colorWithRed:0.3f green:0.3f blue:0.3f alpha:1.0f] forState:UIControlStateNormal];
    buttonLoopList.hidden=NO;
    buttonLoopListSel.hidden=YES;
    buttonLoopTitleSel.hidden=YES;
}

-(void)setLoopInf:(int)mode {
    if (mode==1) {
        [mplayer setLoopInf:1];
        [btnLoopInf setTitleColor:[UIColor colorWithRed:0.3f green:0.5f blue:1.0f alpha:1.0f] forState:UIControlStateNormal];
        if ([mplayer isPlaying]) {
            int arcidx=[mplayer getArcIndex];
            if (arcidx) {
                int subIdx=mplayer.mod_currentsub;
                [self play_loadArchiveModule];
                if (subIdx) [mplayer playGoToSub:subIdx];
            } else {
                int subIdx=mplayer.mod_currentsub;
                [self play_curEntry:-1];
                if (subIdx) [mplayer playGoToSub:subIdx];
            }
        }
    } else  {
        [mplayer setLoopInf:0];
        [btnLoopInf setTitleColor:[UIColor colorWithRed:0.3f green:0.3f blue:0.3f alpha:1.0f] forState:UIControlStateNormal];
        if ([mplayer isPlaying]) {
            int arcidx=[mplayer getArcIndex];
            if (arcidx) {
                int subIdx=mplayer.mod_currentsub;
                [self play_loadArchiveModule];
                if (subIdx) [mplayer playGoToSub:subIdx];
            } else {
                int subIdx=mplayer.mod_currentsub;
                [self play_curEntry:-1];
                if (subIdx) [mplayer playGoToSub:subIdx];
            }
        }
    }
}


-(IBAction)pushedLoopInf {
    if (mplayer.mLoopMode==0) {
        [self setLoopInf:1];
    } else  {
        [self setLoopInf:0];
    }
}

-(bool) isProjectMAlone {
    bool ret=true;
    //Only Piano, Midi & MOD are using screen touches
    if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value||settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value) ret=false;
    if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value) ret=false;
    if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value) ret=false;
    
    return ret;
}

-(int) computeActiveFX {
    int active_idx=0;
    if (settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value) active_idx|=1<<FX_PROJECTM;
    if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value) active_idx|=1<<FX_OSCILLO;
    if (settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value) active_idx|=1<<FX_PIANOROLL;
    if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value) active_idx|=1<<FX_PIANO3D;
    if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value) active_idx|=1<<FX_MIDIPattern;
    if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value) active_idx|=1<<FX_MODPattern;
    if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value) active_idx|=1<<FX_2DSpectrum;
    if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value) active_idx|=1<<FX_3DSpectrum;
    if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value) active_idx|=1<<FX_3DLandscape;
    
    if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) active_idx|=1<<13;
    
    return active_idx;
}

-(IBAction) oglButtonPushed {
    if (mOglViewIsHidden) {
        mOglViewIsHidden=NO;
        // if no active FX, show menu
        if ([self computeActiveFX]==0) {
            pmenu_show=1;
            pmenu_fade=0;
        }
    }
    else {
        mOglViewIsHidden=YES;
    }
    [self checkGLViewCanDisplay];
}


- (void)showRating:(int)rating {
    if ([radioSource isActive]) {
        
    } else {
        if (rating) {
            mainRating5.hidden=FALSE;
            mainRating5off.hidden=TRUE;
        } else {
            mainRating5.hidden=TRUE;
            mainRating5off.hidden=FALSE;
        }
        // Update CarPlay buttons after rating change
        myTabBarController *tabBarController = (myTabBarController *)self.tabBarController;
        if (tabBarController && tabBarController.cpMngt) {
            [tabBarController.cpMngt refreshNowPlayingButtons];
        }
    }
}

-(void)updateStats:(NSString *)fileName filePath:(NSString *)filePath playcount_inc:(bool)playcount_inc {
    signed char rating;
    signed char avg_rating;
    short int playcount;
    
    filePath=[ModizFileHelper getFullCleanFilePath:filePath];
    
    if ([mplayer isArchive]) {
        //Update Global file stats
        fileName=[filePath lastPathComponent];
        DBHelper::getFileStatsDBmod(filePath,&playcount,&rating,&avg_rating);
        if (playcount_inc) playcount++;
        DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,rating,avg_rating,-1/*[mplayer getGlobalLength]*/,-1,-[mplayer getArcEntriesCnt]);
        
        //Update archive entry stats
        DBHelper::getFileStatsDBmod([NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],&playcount,&rating,&avg_rating);
        if (playcount_inc) playcount++;
        DBHelper::updateFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],playcount,rating,avg_rating,[mplayer getGlobalLength],-1,mplayer.mod_subsongs);
        
        //Update subsong entry if applicable
        if (mplayer.mod_subsongs>1) {
            DBHelper::getFileStatsDBmod([NSString stringWithFormat:@"%@@%d?%d",filePath,[mplayer getArcIndex],mplayer.mod_currentsub],&playcount,&rating,&avg_rating);
            if (playcount_inc) playcount++;
            DBHelper::updateFileStatsDBmod([NSString stringWithFormat:@"%@/%@",[mplayer getArcEntryTitle:[mplayer getArcIndex]],[mplayer getSubTitle:mplayer.mod_currentsub]],[NSString stringWithFormat:@"%@@%d?%d",filePath,[mplayer getArcIndex],mplayer.mod_currentsub],playcount,rating,avg_rating,[mplayer getSongLength],-1,1);
        }
    } else if ([mplayer isMultiSongs]){
        fileName=[filePath lastPathComponent];
        //Update Global file stats
        DBHelper::getFileStatsDBmod(filePath,&playcount,&rating,&avg_rating);
        if (playcount_inc) playcount++;
        DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,rating,avg_rating,[mplayer getGlobalLength],-1,mplayer.mod_subsongs);
        
        //Update subsong entry stats
        DBHelper::getFileStatsDBmod([NSString stringWithFormat:@"%@?%d",filePath,mplayer.mod_currentsub],&playcount,&rating,&avg_rating);
        if (playcount_inc) playcount++;
        DBHelper::updateFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],[NSString stringWithFormat:@"%@?%d",filePath,mplayer.mod_currentsub],playcount,rating,avg_rating,[mplayer getSongLength],-1,-1);
    } else {
        //Update Global file stats
        DBHelper::getFileStatsDBmod(filePath,&playcount,&rating,&avg_rating);
        if (playcount_inc) playcount++;
        DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,rating,avg_rating,[mplayer getGlobalLength],-1,mplayer.mod_subsongs);
        
    }
    
    NSDictionary *userInfo = @{
        @"fileName": fileName,
        @"filePath": filePath,
    };
    [[NSNotificationCenter defaultCenter] postNotificationName:MDZFileStatsChangedNotification
                                                        object:self
                                                      userInfo:userInfo];
}


- (void) pushedRatingMulti {
    signed char tmp_rating;
    __block NSString *filePath,*fileName;
    UIAlertController *msgAlert;
    UIAlertAction* userAction;
    UIAlertAction* cancelAction;
    
    filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
    fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
    
    tmp_rating=mPlaylist[mPlaylist_pos].mPlaylistRating;
    
    msgAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Favorites",@"")
                                                   message:[NSString stringWithFormat:NSLocalizedString(@"Please choose",@"")]
                                            preferredStyle:UIAlertControllerStyleActionSheet];
    //Cancel action
    cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
                                          handler:^(UIAlertAction * action) {
        if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
    }];
    [msgAlert addAction:cancelAction];
    
    bool isArchive=[mplayer isArchive];
    bool isMultiSongs=[mplayer isMultiSongs];
    
    //Check if a rating exist for current global file
    filePath=[ModizFileHelper getFullCleanFilePath:filePath];
    DBHelper::getFileStatsDBmod(filePath,NULL,&tmp_rating,NULL);
    
    if (tmp_rating) {
        //Remove current file action
        
        userAction = [UIAlertAction actionWithTitle:[NSString stringWithFormat:@"%@\n%@",NSLocalizedString(@"Remove current file",@""),[filePath lastPathComponent]] style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction * action) {
            
            //Update the rating at file level
            filePath=[ModizFileHelper getFullCleanFilePath:filePath];
            DBHelper::updateRatingDBmod(filePath,0);
            
            signed char tmp_rating=[self getCurrentRating];
            //update playlist
            self.mPlaylist[self.mPlaylist_pos].mPlaylistRating=tmp_rating;
            //update UI
            [self showRating:tmp_rating];
            
            if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
        }];
        
        [msgAlert addAction:userAction];
    } else {
        //Add current file action
        userAction = [UIAlertAction actionWithTitle:[NSString stringWithFormat:@"%@\n%@",NSLocalizedString(@"Add current file",@""),[filePath lastPathComponent]] style:UIAlertActionStyleDefault
                                            handler:^(UIAlertAction * action) {
            
            //Update the rating at file level
            filePath=[ModizFileHelper getFullCleanFilePath:filePath];
            DBHelper::updateRatingDBmod(filePath,5);
            //update playlist
            
            self.mPlaylist[self.mPlaylist_pos].mPlaylistRating=5;
            //update UI
            [self showRating:5];
            
            if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
        }];
        
        [msgAlert addAction:userAction];
    }
    
    if (isArchive) {
        
        //Check if a rating exist for current subsong
        filePath=[NSString stringWithFormat:@"%@@%d",[ModizFileHelper getFullCleanFilePath:filePath],[mplayer getArcIndex]];
        DBHelper::getFileStatsDBmod(filePath,NULL,&tmp_rating,NULL);
        
        if (tmp_rating) {
            //Remove current archive entry
            if ([mplayer isArchive]/*&&([mplayer getArcEntriesCnt]>1)*/) {
                userAction = [UIAlertAction actionWithTitle:[NSString stringWithFormat:@"%@\n%@",NSLocalizedString(@"Remove archive entry",@""),[mplayer getArcEntryTitle:[mplayer getArcIndex]]] style:UIAlertActionStyleDefault
                                                    handler:^(UIAlertAction * action) {
                    //Update the rating at file level
                    NSString *fpath=[ModizFileHelper getFullCleanFilePath:filePath];
                    fpath=[NSString stringWithFormat:@"%@@%d",fpath,[self.mplayer getArcIndex]];
                    DBHelper::updateRatingDBmod(fpath,0);
                    //recompute avg for global file
                    DBHelper::updateFileStatsAvgRatingDBmod(fpath);
                    
                    signed char tmp_rating=[self getCurrentRating];
                    //update playlist
                    self.mPlaylist[self.mPlaylist_pos].mPlaylistRating=tmp_rating;
                    //update UI
                    [self showRating:tmp_rating];
                    
                    
                    if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
                }];
                [msgAlert addAction:userAction];
            }
        } else {
            //Add current archive entry
            if ([mplayer isArchive]/*&&([mplayer getArcEntriesCnt]>1)*/) {
                userAction = [UIAlertAction actionWithTitle:[NSString stringWithFormat:@"%@\n%@",NSLocalizedString(@"Add archive entry",@""),[mplayer getArcEntryTitle:[mplayer getArcIndex]]] style:UIAlertActionStyleDefault
                                                    handler:^(UIAlertAction * action) {
                    //Update the rating at file level
                    NSString *fpath=[ModizFileHelper getFullCleanFilePath:filePath];
                    fpath=[NSString stringWithFormat:@"%@@%d",fpath,[self.mplayer getArcIndex]];
                    DBHelper::updateRatingDBmod(fpath,5);
                    //recompute avg for global file
                    DBHelper::updateFileStatsAvgRatingDBmod(fpath);
                    
                    //update playlist
                    self.mPlaylist[self.mPlaylist_pos].mPlaylistRating=5;
                    //update UI
                    [self showRating:5];
                    
                    
                    if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
                }];
                [msgAlert addAction:userAction];
            }
        }
    }
    
    if (isMultiSongs) {
        
        //Check if a rating exist for current subsong
        if (isArchive) {
            //embedded in an archive
            filePath=[NSString stringWithFormat:@"%@@%d?%d",[ModizFileHelper getFullCleanFilePath:filePath],[mplayer getArcIndex],mplayer.mod_currentsub];
            DBHelper::getFileStatsDBmod(filePath,NULL,&tmp_rating,NULL);
        } else {
            filePath=[NSString stringWithFormat:@"%@?%d",[ModizFileHelper getFullCleanFilePath:filePath],mplayer.mod_currentsub];
            DBHelper::getFileStatsDBmod(filePath,NULL,&tmp_rating,NULL);
        }
        
        if (tmp_rating) {
            //subsong has already a rating
            //propose to remove it
            userAction = [UIAlertAction actionWithTitle:[NSString stringWithFormat:@"%@\n%@",NSLocalizedString(@"Remove current subsong",@""),[mplayer getSubTitle:mplayer.mod_currentsub]] style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction * action) {
                NSString *fpath;
                //Update the rating at file level
                if (isArchive) {
                    fpath=[NSString stringWithFormat:@"%@@%d?%d",[ModizFileHelper getFullCleanFilePath:filePath],[self.mplayer getArcIndex],self.mplayer.mod_currentsub];
                } else {
                    fpath=[NSString stringWithFormat:@"%@?%d",[ModizFileHelper getFullCleanFilePath:filePath],self.mplayer.mod_currentsub];
                }
                
                DBHelper::updateRatingDBmod(fpath,0);
                //recompute avg for global file
                DBHelper::updateFileStatsAvgRatingDBmod(fpath);
                
                signed char tmp_rating=[self getCurrentRating];
                //update playlist
                self.mPlaylist[self.mPlaylist_pos].mPlaylistRating=tmp_rating;
                //update UI
                [self showRating:tmp_rating];
                
                if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
            }];
            
            [msgAlert addAction:userAction];
        } else {
            userAction = [UIAlertAction actionWithTitle:[NSString stringWithFormat:@"%@\n%@",NSLocalizedString(@"Add current subsong",@""),[mplayer getSubTitle:mplayer.mod_currentsub]] style:UIAlertActionStyleDefault
                                                handler:^(UIAlertAction * action) {
                NSString *fpath;
                //Update the rating at file level
                if (isArchive) {
                    fpath=[NSString stringWithFormat:@"%@@%d?%d",[ModizFileHelper getFullCleanFilePath:filePath],[self.mplayer getArcIndex],self.mplayer.mod_currentsub];
                } else {
                    fpath=[NSString stringWithFormat:@"%@?%d",[ModizFileHelper getFullCleanFilePath:filePath],self.mplayer.mod_currentsub];
                }
                
                DBHelper::updateRatingDBmod(fpath,5);
                //recompute avg for global file
                DBHelper::updateFileStatsAvgRatingDBmod(fpath);
                
                //update playlist
                self.mPlaylist[self.mPlaylist_pos].mPlaylistRating=5;
                //update UI
                [self showRating:5];
                
                if ([self respondsToSelector:@selector(updateMiniPlayer)]) [self performSelector:@selector(updateMiniPlayer)];
            }];
            [msgAlert addAction:userAction];
        }
    }
    [self presentViewController:msgAlert animated:YES completion:nil];
}


-(int) getCurrentRating {
    if (!mPlaylist || !mPlaylist_size) return 0;
    NSString *filePath,*fileName;
    filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
    fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
    
    return DBHelper::getRating(filePath,([mplayer isArchive]?[mplayer getArcIndex]:-1),([mplayer isMultiSongs]?mplayer.mod_currentsub:-1));;
}

//rating: -1 no force value
//         0 force negative, i.e. remove entry from favorites
//         5 force positive, i.e. add entry to favorites
-(void) pushedRatingCommon:(signed char)rating{
    signed char tmp_rating;
    short int playcount;
    NSString *filePath,*fileName;
    filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
    fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
    
    bool single_music_file=true;
    if ([mplayer isArchive]/*&&([mplayer getArcEntriesCnt]>1)*/) single_music_file=false;
    if (mplayer.mod_subsongs>1) single_music_file=false;
    
    if ((single_music_file==false)&&(rating==-1)) {
        [self pushedRatingMulti];
    } else {
        //recompose filePath
        filePath=[ModizFileHelper getFullCleanFilePath:filePath];
        if ([mplayer isArchive]) filePath=[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]];
        if ([mplayer isMultiSongs]) filePath=[NSString stringWithFormat:@"%@?%d",filePath,mplayer.mod_currentsub];
        
        DBHelper::getFileStatsDBmod(filePath,NULL,&tmp_rating,NULL);
        
        if (tmp_rating) {
            //remove
            if (rating!=5) { //check if a positive rating wasn't force
                DBHelper::updateRatingDBmod(filePath,0);
                mPlaylist[mPlaylist_pos].mPlaylistRating=0;
                [self showRating:0];
            }
        } else {
            //add
            if (rating!=0) { //check if a negative rating wasn't force
                DBHelper::updateRatingDBmod(filePath,5);
                mPlaylist[mPlaylist_pos].mPlaylistRating=5;
                [self showRating:5];
            }
        }
    }
    
    DBHelper::getFileStatsDBmod(filePath,&playcount,&tmp_rating,NULL);
    
    if (settings[GLOB_StatsUpload].detail.mdz_boolswitch.switch_value) {
        mSendStatTimer=0;
        [GoogleAppHelper SendStatistics:fileName path:filePath rating:tmp_rating playcount:playcount];
    }
}

#import "PlaylistCommonFunctions.h"

-(IBAction)pushedSaveFile {
    if ([radioSource isActive] && ![radioSource isInLibrary:0]) {
        NSString *name=nil;
        if (radioSource.mRadioSource==RS_COLLECTION_SNES) {
            name=mplayer.album;
        }
        
        if ([radioSource saveFileToLibrary:name]) {
            [self openPopup:NSLocalizedString(@"File saved in Library", @"") secmsg:@"" style:POPUP_STYLE_INFO];
            [btnSaveFile setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
        } else {
            [self openPopup:NSLocalizedString(@"File already in Library", @"") secmsg:@"" style:POPUP_STYLE_INFO];
        }
    }
}


-(IBAction)pushedAddToPl {
    //add to playlist
    [self addToPlaylistSelView:mPlaylist[mPlaylist_pos].mPlaylistFilepath label:mPlaylist[mPlaylist_pos].mPlaylistFilename showNowListening:false];
}

-(void)cmdLike{
    if (!mPlaylist_size) return;
    
    [self pushedRatingCommon:5];
}
-(void)cmdDislike{
    if (!mPlaylist_size) return;
    
    [self pushedRatingCommon:0];
}


-(IBAction)pushedRating5{
    if (!mPlaylist_size) return;
    [self pushedRatingCommon:-1];
}

static char note2charA[12]={'C','C','D','D','E','F','F','G','G','A','A','B'};
static char note2charB[12]={'-','#','-','#','-','-','#','-','#','-','#','-'};
static char dec2hex[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
static int currentPattern,currentRow,visibleChan,nextPattern,prevPattern;
static float modPatternLineSize,modPatternWindowSize;

static int _shiftModeOn;
static float oglTapX=0,oglTapY=0,movePx=0,movePy=0,movePxMOD=0,movePyMOD=0,movePxOld=0,movePyOld=0,movePxPM=0,movePyPM=0;
static float oglLPTapX=0,oglLPTapY=0,oglLPTapXstart=0,oglLPTapYstart=0;
static int fxLPselected=-1,fxTargetSlot=-1,fxLPselectedCpt=0;
static int oglLPTap=0;
static float movePxPMenu=0,movePyPMenu=0;
static float posMouseX=0,posMouseY=0;
static float moveWheelXPMenu,moveWheelYPMenu=0;
static float movePreWheelXPMenu,movePreWheelYPMenu=0;
static float startPx=0,startPy=0;
static float posPx=0,posPy=0;
static int movePMnomore=0;
static int panGesture1Tap,panGestureWheel,panGestureHover;
static float movePxMID=0,movePyMID=0,movePinchScaleFXMID=0;
static float movePxPRoll=0,movePyPRoll=0,movePinchScaleFXPRoll=0;
static float movePxFXPiano=0,movePyFXPiano=0,movePx2FXPiano=0,movePy2FXPiano=0,movePinchScaleFXPiano=0;
static float movePxFX3DSpectrum=0,movePyFX3DSpectrum=0,movePx2FX3DSpectrum=0,movePy2FX3DSpectrum=0,movePinchScaleFX3DSpectrum=0;
static float movePx2=0,movePy2=0,movePx2Old=0,movePy2Old=0;
static float movePinchScaleFXMOD=0;
static float movePinchScale,movePinchScaleOld,movePinchAngle;



- (void)settingsChanged:(int)scope {
    /////////////////////
    //GLOBAL
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_GLOBAL)) {
        //reset idle timer to settings value
        [[UIApplication sharedApplication] setIdleTimerDisabled:settings[GLOB_NoScreenAutoLock].detail.mdz_boolswitch.switch_value];
        
        [mplayer optGLOB_Panning:settings[GLOB_Panning].detail.mdz_boolswitch.switch_value];
        [mplayer optGLOB_PanningValue:settings[GLOB_PanningValue].detail.mdz_slider.slider_value];
        switch (settings[GLOB_ForceMono].detail.mdz_boolswitch.switch_value) {
            case 1:mplayer.optForceMono=1;break;
            case 0:mplayer.optForceMono=0;break;
        }
        
        if ((mPlaylist_pos>=0)&&(mPlaylist_pos<mPlaylist_size)) {
            NSString *fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
            if (settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value) {
                labelModuleName.text=[NSString stringWithFormat:@"%@ /%@",fileName,[mplayer getModName]];
            } else {
                if ([mplayer getModFileTitleOrNull]) labelModuleName.text=[NSString stringWithFormat:@"%@ /%@",[mplayer getModFileTitle],[mplayer getModName]];
                else labelModuleName.text=[NSString stringWithFormat:@"%@",[mplayer getModName]];
            }
        }
        
        [mplayer optGENPBRatio];
    }
    
    /////////////////////
    //VISU
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_VISU)) {
        [self checkGLViewCanDisplay];
        if (m_displayLink) m_displayLink.preferredFramesPerSecond = (settings[GLOB_FXFPS].detail.mdz_switch.switch_value?60:30); //60 or 30 fps depending on device speed iPhone
        
        fxSlot[FX_PROJECTM]=settings[PROJECTM_FXSLOT].detail.mdz_slider.slider_value;
        fxSlot[FX_OSCILLO]=settings[OSCILLO_FXSLOT].detail.mdz_slider.slider_value;
        fxSlot[FX_PIANOROLL]=settings[GLOB_FXPianoRollFXSLOT].detail.mdz_slider.slider_value;
        fxSlot[FX_PIANO3D]=settings[GLOB_FXPiano3DFXSLOT].detail.mdz_slider.slider_value;
        fxSlot[FX_MIDIPattern]=settings[GLOB_FXMIDIPatternFXSLOT].detail.mdz_slider.slider_value;
        fxSlot[FX_MODPattern]=settings[GLOB_FXMODPatternFXSLOT].detail.mdz_slider.slider_value;
        fxSlot[FX_2DSpectrum]=settings[GLOB_FXSpectrumFXSLOT].detail.mdz_slider.slider_value;
        fxSlot[FX_3DSpectrum]=settings[GLOB_FX3DSpectrumFXSLOT].detail.mdz_slider.slider_value;
        fxSlot[FX_3DLandscape]=settings[GLOB_FX3DLandscapeFXSLOT].detail.mdz_slider.slider_value;
        fxSlot[FX_COVER]=settings[GLOB_FXCoverFXSLOT].detail.mdz_slider.slider_value;
    }
    
    /////////////////////
    //OSCILLO
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_OSCILLO)) {
        [mplayer optUpdateSystemColor];
    }
    
    /////////////////////
    //PR&NS
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_PIANOMIDI)) {
        for (int i=0;i<32;i++) {
            data_midifx_pal_custom[i]=settings[PIANOMIDI_MULTI_COLOR01+i].detail.mdz_color.rgb;
        }
    }
    
    /////////////////////
    //ADPLUG
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_ADPLUG)) {
        [mplayer optADPLUG:settings[ADPLUG_OplType].detail.mdz_switch.switch_value stereosurround:settings[ADPLUG_StereoSurround].detail.mdz_switch.switch_value priorityOverMod:settings[ADPLUG_PriorityOMPT].detail.mdz_switch.switch_value];
    }
    
    /////////////////////
    //UADE
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_UADE)) {
        if (settings[UADE_Norm].detail.mdz_boolswitch.switch_value) {
            settings[UADE_PostFX].detail.mdz_boolswitch.switch_value=1;
        }
        if (settings[UADE_Pan].detail.mdz_boolswitch.switch_value) {
            settings[UADE_PostFX].detail.mdz_boolswitch.switch_value=1;
        }
        if (settings[UADE_Led].detail.mdz_boolswitch.switch_value) {
            settings[UADE_PostFX].detail.mdz_boolswitch.switch_value=1;
        }
        if (settings[UADE_Gain].detail.mdz_boolswitch.switch_value) {
            settings[UADE_PostFX].detail.mdz_boolswitch.switch_value=1;
        }
        [mplayer optUADE_Led:settings[UADE_Led].detail.mdz_boolswitch.switch_value];
        [mplayer optUADE_Norm:settings[UADE_Norm].detail.mdz_boolswitch.switch_value];
        [mplayer optUADE_Pan:settings[UADE_Pan].detail.mdz_boolswitch.switch_value];
        [mplayer optUADE_Head:settings[UADE_Head].detail.mdz_boolswitch.switch_value];
        [mplayer optUADE_PostFX:settings[UADE_PostFX].detail.mdz_boolswitch.switch_value];
        [mplayer optUADE_Gain:settings[UADE_Gain].detail.mdz_boolswitch.switch_value];
        [mplayer optUADE_PanValue:settings[UADE_PanValue].detail.mdz_slider.slider_value];
        [mplayer optUADE_GainValue:settings[UADE_GainValue].detail.mdz_slider.slider_value];
        [mplayer optUADE_NTSC:settings[UADE_NTSC].detail.mdz_boolswitch.switch_value];
    }
    
    /////////////////////
    //VGMPLAY
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_VGMPLAY)) {
        [mplayer optVGMPLAY_Update];
    }
    
    /////////////////////
    //SID
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_SID)) {
        [mplayer optSIDFilter:settings[SID_Filter].detail.mdz_boolswitch.switch_value];
        [mplayer optSIDEngine:(char)(settings[SID_Engine].detail.mdz_switch.switch_value)];
        [mplayer optSIDInterpolation:(char)(settings[SID_Interpolation].detail.mdz_switch.switch_value)];
        if (settings[SID_SecondSIDOn].detail.mdz_boolswitch.switch_value) {
            long x = strtol(settings[SID_SecondSIDAddress].detail.mdz_textbox.text, 0, 0);
            [mplayer optSIDSecondSIDAddress:x];
        } else [mplayer optSIDSecondSIDAddress:0];
        
        if (settings[SID_ThirdSIDOn].detail.mdz_boolswitch.switch_value) {
            long x = strtol(settings[SID_ThirdSIDAddress].detail.mdz_textbox.text, 0, 0);
            [mplayer optSIDThirdSIDAddress:x];
        } else [mplayer optSIDThirdSIDAddress:0];
        
        [mplayer optSIDForceLoop:settings[SID_ForceLoop].detail.mdz_boolswitch.switch_value];
        [mplayer optSIDClock:settings[SID_CLOCK].detail.mdz_boolswitch.switch_value];
        [mplayer optSIDModel:settings[SID_MODEL].detail.mdz_boolswitch.switch_value];
    }
    
    /////////////////////
    //GME
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_GME)) {
        [mplayer optGME_Update];
    }
    
    /////////////////////
    //GSF
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_GSF)) {
        switch (settings[GSF_SOUNDQUALITY].detail.mdz_switch.switch_value) {
            case 0:
                mplayer.optGSFsoundQuality=4; //11Khz
                break;
            case 1:
                mplayer.optGSFsoundQuality=2; //22Khz
                break;
            case 2:
                mplayer.optGSFsoundQuality=1; //44Khz
                break;
            default:
                mplayer.optGSFsoundQuality=1; //44Khz
                break;
        }
        mplayer.optGSFsoundInterpolation=settings[GSF_INTERPOLATION].detail.mdz_boolswitch.switch_value;
        mplayer.optGSFsoundLowPass =settings[GSF_LOWPASSFILTER].detail.mdz_boolswitch.switch_value;
        mplayer.optGSFsoundEcho=settings[GSF_ECHO].detail.mdz_boolswitch.switch_value;
        [mplayer optGSF_UpdateParam];
    }
    
    /////////////////////
    //NSFPLAY
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_NSFPLAY)) {
        [mplayer optNSFPLAY_UpdateParam];
        
    }
    
    /////////////////////
    //GBSPLAY
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_GBSPLAY)) {
        [mplayer optGBSPLAY_UpdateParam];
    }
    
    /////////////////////
    //TIMIDITY
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_TIMIDITY)) {
        [mplayer optTIM_Poly:settings[TIM_Polyphony].detail.mdz_slider.slider_value];
        [mplayer optTIM_Chorus:(int)(settings[TIM_Chorus].detail.mdz_boolswitch.switch_value)];
        [mplayer optTIM_Reverb:(int)(settings[TIM_Reverb].detail.mdz_boolswitch.switch_value)];
        [mplayer optTIM_Resample:(int)(settings[TIM_Resample].detail.mdz_switch.switch_value)];
        [mplayer optTIM_LPFilter:(int)(settings[TIM_LPFilter].detail.mdz_boolswitch.switch_value)];
        [mplayer optTIM_Amplification:(int)(settings[TIM_Amplification].detail.mdz_slider.slider_value)];
        [mplayer optTIM_PBRatio];
        
    }
    
    /////////////////////
    //VGMSTREAM
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_VGMSTREAM)) {
        [mplayer optVGMSTREAM_ForceLoop:settings[VGMSTREAM_Forceloop].detail.mdz_boolswitch.switch_value];
        [mplayer optVGMSTREAM_MaxLoop:(int)(settings[VGMSTREAM_Maxloop].detail.mdz_slider.slider_value)];
        [mplayer optVGMSTREAM_ResampleQuality:(int)(settings[VGMSTREAM_ResampleQuality].detail.mdz_switch.switch_value)];
    }
    
    /////////////////////
    //HC
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_HC)) {
        [mplayer optHC_ResampleQuality:(int)(settings[HC_ResampleQuality].detail.mdz_switch.switch_value)];
    }
    
    /////////////////////
    //OMPT
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_OMPT)) {
        [mplayer optOMPT_Sampling:settings[OMPT_Sampling].detail.mdz_switch.switch_value];
        [mplayer optOMPT_StereoSeparation:settings[OMPT_StereoSeparation].detail.mdz_slider.slider_value];
        [mplayer optOMPT_MasterVol:settings[OMPT_MasterVolume].detail.mdz_slider.slider_value];
        [mplayer optOMPT_AmigaFiltter];
        [mplayer optOMPT_Tempo];
    }
    
    /////////////////////
    //XMP
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_XMP)) {
        [mplayer optXMP_SetStereoSeparation:settings[XMP_StereoSeparation].detail.mdz_slider.slider_value];
        [mplayer optXMP_SetMasterVol:settings[XMP_MasterVolume].detail.mdz_slider.slider_value];
        [mplayer optXMP_SetInterpolation:settings[XMP_Interpolation].detail.mdz_switch.switch_value];
        [mplayer optXMP_SetAmp:settings[XMP_Amplification].detail.mdz_switch.switch_value];
        //[mplayer optXMP_SetDSP:settings[XMP_DSPLowPass].detail.mdz_boolswitch.switch_value];
        [mplayer optXMP_SetFLAGS:settings[XMP_FLAGS_A500F].detail.mdz_boolswitch.switch_value];
        [mplayer optXMP_SetTempo];
    }
    
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_PROJECTM)) {
        if (_pmIsInitialized && _pm ) {
            pmSoftReinit(false);
        }
        mdz_pmMilkPermissiveEvalCode=settings[PROJECTM_PermmissiveMode].detail.mdz_boolswitch.switch_value;
    }
}

-(void) checkGLViewCanDisplay{
    if (mOglViewIsHidden) {
        m_oglView.hidden=YES;
    } else {
        if ((infoView.hidden==YES)) {
            m_oglView.hidden=NO;
        }
    }
}

- (void)setShuffleMode:(int)mode {
    mShuffle=mode%3;
    if (mShuffle<0) mShuffle=0;
    switch (mShuffle) {
        case 0: //sequential mode
            [mplayer setArchiveSubShuffle:NO];
            buttonShuffle.hidden=NO;
            buttonShuffleSel.hidden=YES;
            buttonShuffleOneSel.hidden=YES;
            break;
        case 1: //random mode playing whole sub entries
            [mplayer setArchiveSubShuffle:TRUE];
            buttonShuffle.hidden=YES;
            buttonShuffleSel.hidden=YES;
            buttonShuffleOneSel.hidden=NO;
            break;
        case 2: //full random mode, only picking 1 entry / file
            [mplayer setArchiveSubShuffle:TRUE];
            buttonShuffle.hidden=YES;
            buttonShuffleSel.hidden=NO;
            buttonShuffleOneSel.hidden=YES;
            break;
    }
    
    // Update CarPlay buttons
    myTabBarController *tabBarController = (myTabBarController *)self.tabBarController;
    if (tabBarController && tabBarController.cpMngt) {
        [tabBarController.cpMngt refreshNowPlayingButtons];
    }
}

- (void)setLoopMode:(int)mode {
    mLoopMode=mode%3;
    if (mLoopMode<0) mLoopMode=0;
    switch (mLoopMode) {
        case 0:
            buttonLoopList.hidden=NO;
            buttonLoopListSel.hidden=YES;
            buttonLoopTitleSel.hidden=YES;
            break;
        case 1:
            buttonLoopList.hidden=YES;
            buttonLoopListSel.hidden=NO;
            buttonLoopTitleSel.hidden=YES;
            break;
        case 2:
            buttonLoopList.hidden=YES;
            buttonLoopListSel.hidden=YES;
            buttonLoopTitleSel.hidden=NO;
            break;
    }
    
    // Update CarPlay buttons
    myTabBarController *tabBarController = (myTabBarController *)self.tabBarController;
    if (tabBarController && tabBarController.cpMngt) {
        [tabBarController.cpMngt refreshNowPlayingButtons];
    }
}


- (IBAction)changeLoopMode {
    mLoopMode++;
    [self setLoopMode:mLoopMode];
}

- (IBAction)shuffle {
    mShuffle=(mShuffle+1)%3;
    [self setShuffleMode:mShuffle];
}

-(void) mdOpenCloseMenu {
    if (mOglViewIsHidden==YES) {
        mOglViewIsHidden=NO;
        [self checkGLViewCanDisplay];
    }
    
    if (pmenu_show==0) {
        pmenu_show=1;
        pmenu_fade=0;
    } else {
        pmenu_show=-0;
        //pmenu_fade=0;
    }
}
-(void) mdBackAction {
    if (pmenu_show) {
        PMenu::playerMenuBack();
    } else {
        //Ensure all keys are processed
        //        ImGuiIOSEvent imgui_event;
        //        imgui_event.event_type=IMGUI_IOS_Event_None;
        //        ImGui_ImplIOS_UpdateEvent(&imgui_event);
        //
        [self.navigationController popViewControllerAnimated:YES];
    }
}
-(void) mdTestAsyncLoad {
    int pos=[_mdzPM_playlist getPos];
    [_mdzPM_playlist loadASyncCurrentPreset:false];
}

-(void) mdPrevPreset {
    if (_mdzPM_playlist==nil) return;
    if (settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value) [_mdzPM_playlist last:false];
    else [_mdzPM_playlist last:false];
}
-(void) mdNextPreset {
    if ( _mdzPM_playlist==nil) return;
    [_mdzPM_playlist next:false];
}
-(void) mdInfoFX {
    if (_pmIsInitialized && _pm && settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value) {
        _pmPresetUpdateDisplayInfo=true;
        _pm_display_scroll_pause=_pm_fps*1.5;
    }
}
-(void) mdShiftMode:(int)active {
    _shiftModeOn=active;
}

-(void) mdSetFX:(int)fxIdx value:(int)value {
    switch (fxIdx) {
        case 0:
            [SettingsGenViewController setSettingsValue:GLOB_FXFullscreen value:value];
            [self oglUpdateFSStatus];
            break;
        case 1:
            [SettingsGenViewController setSettingsValue:PROJECTM_FXONOFF value:value];
            break;
        case 2:
            [SettingsGenViewController setSettingsValue:OSCILLO_FXMODE value:value];
            break;
        case 3:
            [SettingsGenViewController setSettingsValue:GLOB_FXPianoRoll value:value];
            break;
        case 4:
            [SettingsGenViewController setSettingsValue:GLOB_FXPiano3D value:value];
            break;
        case 5:
            [SettingsGenViewController setSettingsValue:GLOB_FXMIDIPattern value:value];
            break;
        case 6:
            [SettingsGenViewController setSettingsValue:GLOB_FXMODPattern value:value];
            break;
        case 7:
            [SettingsGenViewController setSettingsValue:GLOB_FXSpectrum value:value];
            break;
        case 8:
            [SettingsGenViewController setSettingsValue:GLOB_FX3DSpectrum value:value];
            break;
        case 9:
            [SettingsGenViewController setSettingsValue:GLOB_FX3DLandscape value:value];
            break;
    }
}

-(void) mdSwitchSpectrumBloom:(int)val {
    [SettingsGenViewController changeSettingsValue:GLOB_FX3DSpectrumBloom change:val];
    
    [self openPopup:NSLocalizedString(@"Spectrum 3D",@"") secmsg:[NSString stringWithFormat:@"Bloom set to %s",settings[GLOB_FX3DSpectrumBloom].detail.mdz_switch.switch_labels[settings[GLOB_FX3DSpectrumBloom].detail.mdz_switch.switch_value]] style:POPUP_STYLE_INFO];
}
-(void) mdSwitchLandscapeBloom:(int)val {
    [SettingsGenViewController changeSettingsValue:GLOB_FX3DLandscapeBloom change:val];
    
    [self openPopup:NSLocalizedString(@"3D Landscape",@"") secmsg:[NSString stringWithFormat:@"Bloom set to %s",settings[GLOB_FX3DLandscapeBloom].detail.mdz_switch.switch_labels[settings[GLOB_FX3DLandscapeBloom].detail.mdz_switch.switch_value]] style:POPUP_STYLE_INFO];
}
-(void) mdSwitchVolBars {
    [SettingsGenViewController changeSettingsValue:GLOB_FXMODPattern_VolBar change:1];
}
-(void) mdSwitchFixedBar {
    [SettingsGenViewController changeSettingsValue:GLOB_FXMODPattern_CurrentLineMode change:1];
}
-(void) mdSwitchModPatternTheme:(int)val {
    [SettingsGenViewController changeSettingsValue:GLOB_FXMODPattern_Theme change:val];
}
-(void) mdSwitchModPatternFont:(int)val {
    [SettingsGenViewController changeSettingsValue:GLOB_FXMODPattern_Font change:val];
}
-(void) mdSwitchModPatternFontSize:(int)val {
    [SettingsGenViewController changeSettingsValue:GLOB_FXMODPattern_FontSize change:val];
}

-(void) newGuiMessage:(NSString*)msg {
    if (msg==nil) return;
    snprintf(_mdz_FX_GuiMessageStr,64,"%s",[msg UTF8String]);
    int fps=(settings[GLOB_FXFPS].detail.mdz_switch.switch_value?60:30);
    _mdz_FX_GuiMessage_fade=fps*FX_FS_GUIMESSAGE_TIMEOUT;
    _mdz_FX_GuiMessage_fadeMax=_mdz_FX_GuiMessage_fade;
}

-(void) showSongInfo:(ImVec2)size frameToUpdate:(int)frameToUpdate {
    static int framecpt=0;
    char strLine[6][256];
    static int cursorCpt=0;
    float ww=size.x;
    float hh=size.y;
    
    float font_size=ww/FONTSIZE_FX_FS_INFO_LINE_DIVIDER;
    if (font_size<FONTSIZE_FX_FS_INFO_LINE) font_size=FONTSIZE_FX_FS_INFO_LINE;
    if (font_menu) ImGui::PushFont(font_menu,font_size*glScaleFactor);
    else ImGui::PushFont(nullptr);
    
    float textWidth=ImGui::CalcTextSize("ABCDEFGH").x/8.0;
    int MDZ_FX_SONGINFO_MAXCHAR=round(ww*glScaleFactor/(textWidth+0));
    
    if (MDZ_FX_SONGINFO_MAXCHAR>256) MDZ_FX_SONGINFO_MAXCHAR=256;
    
    int lineIdx=0;
    for (int i=0;i<6;i++) strLine[i][0]=0;
    
    if ((_mdz_display_songinfo_title!=nil)&&[_mdz_display_songinfo_title length]) {
        int strLen=(int)[_mdz_display_songinfo_title length];
        if (strLen<MDZ_FX_SONGINFO_MAXCHAR) {
            snprintf(strLine[lineIdx++],MDZ_FX_SONGINFO_MAXCHAR,"%sX",[_mdz_display_songinfo_title UTF8String]);
        } else {
            int split=strLen/2;
            snprintf(strLine[lineIdx++],MDZ_FX_SONGINFO_MAXCHAR,"%sX",[[_mdz_display_songinfo_title substringToIndex:split] UTF8String]);
            snprintf(strLine[lineIdx++],MDZ_FX_SONGINFO_MAXCHAR,"%sX",[[_mdz_display_songinfo_title substringFromIndex:split] UTF8String]);
        }
    }
    
    if ((_mdz_display_songinfo_sub!=nil)&&[_mdz_display_songinfo_sub length]) {
        int strLen=(int)[_mdz_display_songinfo_sub length];
        if (strLen<MDZ_FX_SONGINFO_MAXCHAR) {
            snprintf(strLine[lineIdx++],MDZ_FX_SONGINFO_MAXCHAR,"%sX",[_mdz_display_songinfo_sub UTF8String]);
        } else {
            int split=strLen/2;
            snprintf(strLine[lineIdx++],MDZ_FX_SONGINFO_MAXCHAR,"%sX",[[_mdz_display_songinfo_sub substringToIndex:split] UTF8String]);
            snprintf(strLine[lineIdx++],MDZ_FX_SONGINFO_MAXCHAR,"%sX",[[_mdz_display_songinfo_sub substringFromIndex:split] UTF8String]);
        }
    }
    
    if ((_mdz_display_songinfo_artist!=nil)&&[_mdz_display_songinfo_artist length]) {
        int strLen=(int)[_mdz_display_songinfo_artist length];
        if (strLen<MDZ_FX_SONGINFO_MAXCHAR) {
            snprintf(strLine[lineIdx++],MDZ_FX_SONGINFO_MAXCHAR,"%sX",[_mdz_display_songinfo_artist UTF8String]);
        } else {
            int split=strLen/2;
            snprintf(strLine[lineIdx++],MDZ_FX_SONGINFO_MAXCHAR,"%sX",[[_mdz_display_songinfo_artist substringToIndex:split] UTF8String]);
            snprintf(strLine[lineIdx++],MDZ_FX_SONGINFO_MAXCHAR,"%sX",[[_mdz_display_songinfo_artist substringFromIndex:split] UTF8String]);
        }
    }
    
    for (int j=0;j<frameToUpdate;j++) {
        if (_mdz_display_songinfo_char_count[0]<MDZ_FX_SONGINFO_MAXCHAR) {
            if (cursorCpt&1) _mdz_display_songinfo_char_count[0]++;
        }
        for (int i=1;i<6;i++) {
            if ( (_mdz_display_songinfo_char_count[i]<MDZ_FX_SONGINFO_MAXCHAR) &&
                ( (_mdz_display_songinfo_char_count[i-1]>=(strlen(strLine[i-1])+4)) ||
                 (_mdz_display_songinfo_char_count[i-1]==MDZ_FX_SONGINFO_MAXCHAR) ) )  {
                if (cursorCpt&1) _mdz_display_songinfo_char_count[i]++;
                if (strlen(strLine[i])>1) _mdz_FS_display_cursorLine=i;
            }
        }
        cursorCpt++;
    }
    
    bool allIsVisible=false;
    if ( (_mdz_display_songinfo_char_count[0]>=strlen(strLine[0])) &&
        (_mdz_display_songinfo_char_count[1]>=strlen(strLine[1])) &&
        (_mdz_display_songinfo_char_count[2]>=strlen(strLine[2])) &&
        (_mdz_display_songinfo_char_count[3]>=strlen(strLine[3])) &&
        (_mdz_display_songinfo_char_count[4]>=strlen(strLine[4])) &&
        (_mdz_display_songinfo_char_count[5]>=strlen(strLine[5])) ) {
        allIsVisible=true;
    }
    
    float alpha_val=(float)(_mdz_display_songinfo_countdown*4)/64.0;
    if (alpha_val>0.6) alpha_val=0.6;
    float alpha_txt=alpha_val*2;
    if (alpha_txt>1.0) alpha_txt=1.0;
    
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0,0,0,alpha_val));
    ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0,1.0,1.0,alpha_txt));
    
    
    
    float textHH=ImGui::GetTextLineHeightWithSpacing()/glScaleFactor;
    float pos_x,pos_y;
    ImVec2 str_size;
    
    ImVec2 str_size_max=ImVec2(0,0);
    for (int i=0;i<6;i++) {
        str_size=ImGui::CalcTextSize(strLine[i]);
        if (str_size.x>str_size_max.x) str_size_max=str_size;
    }
    
    lineIdx=0;
    for (int i=0;i<6;i++) strLine[i][0]=0;
    
    if ((_mdz_display_songinfo_title!=nil)&&[_mdz_display_songinfo_title length]) {
        int strLen=(int)[_mdz_display_songinfo_title length];
        if (strLen<MDZ_FX_SONGINFO_MAXCHAR) {
            snprintf(strLine[lineIdx],_mdz_display_songinfo_char_count[lineIdx],"%s",[_mdz_display_songinfo_title UTF8String]);
            lineIdx++;
        } else {
            int split=strLen/2;
            snprintf(strLine[lineIdx],_mdz_display_songinfo_char_count[lineIdx],"%s",[[_mdz_display_songinfo_title substringToIndex:split] UTF8String]);
            lineIdx++;
            snprintf(strLine[lineIdx],_mdz_display_songinfo_char_count[lineIdx],"%s",[[_mdz_display_songinfo_title substringFromIndex:split] UTF8String]);
            lineIdx++;
        }
    }
    
    if ((_mdz_display_songinfo_sub!=nil)&&[_mdz_display_songinfo_sub length]) {
        int strLen=(int)[_mdz_display_songinfo_sub length];
        if (strLen<MDZ_FX_SONGINFO_MAXCHAR) {
            snprintf(strLine[lineIdx],_mdz_display_songinfo_char_count[lineIdx],"%s",[_mdz_display_songinfo_sub UTF8String]);
            lineIdx++;
        } else {
            int split=strLen/2;
            snprintf(strLine[lineIdx],_mdz_display_songinfo_char_count[lineIdx],"%s",[[_mdz_display_songinfo_sub substringToIndex:split] UTF8String]);
            lineIdx++;
            snprintf(strLine[lineIdx],_mdz_display_songinfo_char_count[lineIdx],"%s",[[_mdz_display_songinfo_sub substringFromIndex:split] UTF8String]);
            lineIdx++;
        }
    }
    
    if ((_mdz_display_songinfo_artist!=nil)&&[_mdz_display_songinfo_artist length]) {
        int strLen=(int)[_mdz_display_songinfo_artist length];
        if (strLen<MDZ_FX_SONGINFO_MAXCHAR) {
            snprintf(strLine[lineIdx],_mdz_display_songinfo_char_count[lineIdx],"%s",[_mdz_display_songinfo_artist UTF8String]);
            lineIdx++;
        } else {
            int split=strLen/2;
            snprintf(strLine[lineIdx],_mdz_display_songinfo_char_count[lineIdx],"%s",[[_mdz_display_songinfo_artist substringToIndex:split] UTF8String]);
            lineIdx++;
            snprintf(strLine[lineIdx],_mdz_display_songinfo_char_count[lineIdx],"%s",[[_mdz_display_songinfo_artist substringFromIndex:split] UTF8String]);
            lineIdx++;
        }
    }
    
    str_size_max.x=str_size_max.x/glScaleFactor;
    float safe_adjust_top=0;
    float safe_adjust_bottom=0;
    float safe_adjust_left=0;
    float safe_adjust_right=8;
    if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) {
        safe_adjust_top=safe_top;
        safe_adjust_bottom=safe_bottom;
        safe_adjust_left=safe_left;
        safe_adjust_right=safe_right;
    }
    switch (settings[GLOB_FX_DISPLAYSONGINFO].detail.mdz_boolswitch.switch_value) {
        default:
        case 1: //TL
            pos_x=safe_adjust_left*glScaleFactor;
            pos_y=(safe_adjust_top+0*textHH)*glScaleFactor;
            break;
        case 2: //TR
            pos_x=(ww-str_size_max.x-safe_adjust_right)*glScaleFactor;
            if (coverAvailable) pos_x-=textHH*(float)lineIdx*glScaleFactor;
            pos_y=(safe_adjust_top+0*textHH)*glScaleFactor;
            break;
        case 3: //Center
            pos_x=round((ww-str_size_max.x)/2.0*glScaleFactor);
            if (coverAvailable) pos_x-=textHH*(float)lineIdx*glScaleFactor/2;
            pos_y=round((hh-textHH*(float)lineIdx)/2.0*glScaleFactor);
            break;
        case 4: //BL
            pos_x=safe_adjust_left*glScaleFactor;
            pos_y=(hh-textHH*(float)lineIdx-2*textHH-safe_adjust_bottom)*glScaleFactor;
            break;
        case 5: //BR
            pos_x=(ww-str_size_max.x-safe_adjust_right)*glScaleFactor;
            if (coverAvailable) pos_x-=textHH*(float)lineIdx*glScaleFactor;
            pos_y=(hh-textHH*(float)lineIdx-2*textHH-safe_adjust_bottom)*glScaleFactor;
            break;
    }
    ImGui::SetNextWindowPos(ImVec2(pos_x,pos_y));
    ImGui::SetNextWindowSize(ImVec2((str_size_max.x+4+(float)lineIdx*textHH)*glScaleFactor,((float)lineIdx+0.5)*textHH*glScaleFactor));
    ImGui::GetStyle().Alpha=1.0;
    ImGui::Begin("On screen music info",0,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoFocusOnAppearing);
    
    ImVec2 cur_pos=ImGui::GetCursorPos();
    
    for (int i=0;i<lineIdx;i++) {
        if (coverAvailable) {
            ImVec2 new_cur_pos=ImGui::GetCursorPos();
            new_cur_pos.x+=textHH*(float)lineIdx*glScaleFactor;
            ImGui::SetCursorPos(new_cur_pos);
        }
        
        if (_mdz_FS_display_cursorLine==i) {
            if ((framecpt%32)>=10) ImGui::Text("%s_",strLine[i]);
            else ImGui::Text("%s",strLine[i]);
        } else ImGui::Text("%s",strLine[i]);
    }
    
    if (coverAvailable) {
        ImGui::SetCursorPos(cur_pos);
        ImGui::Image(txtCoverImg, ImVec2(lineIdx*textHH*glScaleFactor,lineIdx*textHH*glScaleFactor));
    }
    
    
    ImGui::End();
    ImGui::PopFont();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    
    if (allIsVisible) {
        for (int j=0;j<frameToUpdate;j++) {
            if (_mdz_display_songinfo_countdown) _mdz_display_songinfo_countdown--;
            if (!_mdz_display_songinfo_countdown) {
                for (int i=0;i<6;i++) _mdz_display_songinfo_char_count[i]=1;
            }
        }
    }
    framecpt++;
}

-(void) showGuiMessage:(ImVec2)size frameToUpdate:(int)frameToUpdate {
    if (!_mdz_FX_GuiMessage_fade) return;
    if (!strlen(_mdz_FX_GuiMessageStr)) return;
    
    float alpha;
    
    ImGui::GetStyle().Alpha=1.0;
    
    float zoomfact=2.0-2.0*pow(_mdz_FX_GuiMessage_fade/_mdz_FX_GuiMessage_fadeMax,3.0);
    if (zoomfact<0) zoomfact=0;
    if (zoomfact>1) zoomfact=1;
    float font_size=round( FONTSIZE_GUIMSESSAGE*( 1 + 4.0*pow(sin(zoomfact*3.14159),2) ) );
    
    if (font_menu) ImGui::PushFont(font_menu,font_size*glScaleFactor);
    else ImGui::PushFont(nullptr);
    
    
    float posY=0;
    float posX;
    ImVec2 textSize=ImGui::CalcTextSize(_mdz_FX_GuiMessageStr);
    posX=round((size.x*glScaleFactor-textSize.x-16*glScaleFactor)/2.0);
    ImGui::SetNextWindowPos(ImVec2(posX,posY));
    ImGui::SetNextWindowSize(ImVec2((textSize.x+16*glScaleFactor),(textSize.y+6*glScaleFactor)));
    
    alpha=(float)(_mdz_FX_GuiMessage_fade)/30.0;
    if (alpha>1) alpha=1;
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(0,0,0,0));
    ImGui::Begin("GUI_msg",0,
                 ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoFocusOnAppearing
                 );
    
    if (font_menu) ImGui::PushFont(font_menu,(font_size+3)*glScaleFactor);
    else ImGui::PushFont(nullptr);
    
    ImGui::SetCursorPos(ImVec2((0*posX+(8-1)*glScaleFactor), (3-1)*glScaleFactor) );
    ImGui::TextColored(ImVec4(0.0,0.0,0.0,alpha),"%s",_mdz_FX_GuiMessageStr);
    
    ImGui::PopFont();
    
    
    ImGui::SetCursorPos(ImVec2((0*posX+8*glScaleFactor), 3*glScaleFactor) );
    ImGui::TextColored(ImVec4(1.0,1.0,1.0,alpha),"%s",_mdz_FX_GuiMessageStr);
    
    
    
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    
    ImGui::PopFont();
    
    
    for (int i=0;i<frameToUpdate;i++) {
        if (_mdz_FX_GuiMessage_fade>0) _mdz_FX_GuiMessage_fade--;
    }
}

-(void) mdChangeFavoriteStatusPreset:(int)val {
    if (_pmIsInitialized && _pm /*&& (pmenu_show==0)*/) {
        const char *title;
        title = [_mdzPM_playlist getCurPresetCleanTitle];
        if (title) {
            NSString *strName=[NSString stringWithUTF8String:title];
            
            bool added=false;
            if (val==1) {
                [_mdzPM_Favorites addFavoritePreset:strName];
                [_mdzPM_Favorites addFavStatusFor:strName bundleFN:pmBundledPresetsFileNode customFN:pmCustomPresetsFileNode];
                added=true;
            } else if (val==-1) [_mdzPM_Favorites remFavoritePreset:strName];
            else if (val==0) {
                if ([_mdzPM_Favorites isFavoritePreset:strName]) [_mdzPM_Favorites remFavoritePreset:strName];
                else {
                    [_mdzPM_Favorites addFavoritePreset:strName];
                    [_mdzPM_Favorites addFavStatusFor:strName bundleFN:pmBundledPresetsFileNode customFN:pmCustomPresetsFileNode];
                    added=true;
                }
            }
            if (added) {
                //[self newGuiMessage:NSLocalizedString(@"Added to favorites",@"")];
                [self newGuiMessage:[NSString stringWithFormat:@"%C",static_cast<unichar>(FA_STAR)]];
                
            } else {
                //[self newGuiMessage:NSLocalizedString(@"Removed from favorites",@"")];
                [self newGuiMessage:[NSString stringWithFormat:@"%C",static_cast<unichar>(FA_STAR_O)]];
            }
            _pmFavoritesChanged=true;
            _pmPresetUpdateDisplayInfo=true;
            _pm_display_scroll_pause=_pm_fps*1.5;
        }
    }
}


-(void) mdSwitchLockStatusPreset {
    if (_pmIsInitialized && _pm) {
        const char *title;
        int index=[_mdzPM_playlist getPos];
        title = [_mdzPM_playlist getCurPresetCleanTitle];
        if (title) {
            if (settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value) {
                projectm_set_preset_locked(_pm, false);
                settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value=0;
                //[self newGuiMessage:NSLocalizedString(@"Preset unlocked",@"")];
                [self newGuiMessage:[NSString stringWithFormat:@"%C",static_cast<unichar>(FA_UNLOCK)]];
            } else {
                projectm_set_preset_locked(_pm, true);
                settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value=1;
                //[self newGuiMessage:NSLocalizedString(@"Locked",@"")];
                [self newGuiMessage:[NSString stringWithFormat:@"%C",static_cast<unichar>(FA_LOCK)]];
            }
            _pmPresetUpdateDisplayInfo=true;
            _pm_display_scroll_pause=_pm_fps*1.5;
        }
        
        
    }
}
-(void) mdSwitchFPSHud {
    [SettingsGenViewController changeSettingsValue:GLOB_FXSHOWINFO change:1];
}


-(void) switchFX:(int)fxNb change:(int)val {
    if (mOglViewIsHidden==YES) {
        mOglViewIsHidden=NO;
        [self checkGLViewCanDisplay];
    }
    switch (fxNb) {
        case 1:
            [SettingsGenViewController changeSettingsValue:PROJECTM_FXONOFF change:val];
            break;
        case 2:
            [SettingsGenViewController changeSettingsValue:OSCILLO_FXMODE change:val];
            break;
        case 3:
            [SettingsGenViewController changeSettingsValue:GLOB_FXPianoRoll change:val];
            break;
        case 4:
            [SettingsGenViewController changeSettingsValue:GLOB_FXPiano3D change:val];
            break;
        case 5:
            [SettingsGenViewController changeSettingsValue:GLOB_FXMIDIPattern change:val];
            break;
        case 6:
            [SettingsGenViewController changeSettingsValue:GLOB_FXMODPattern change:val];
            break;
        case 7:
            [SettingsGenViewController changeSettingsValue:GLOB_FXSpectrum change:val];
            break;
        case 8:
            [SettingsGenViewController changeSettingsValue:GLOB_FX3DSpectrum change:val];
            break;
        case 9:
            [SettingsGenViewController changeSettingsValue:GLOB_FX3DLandscape change:val];
            break;
        case 0:
            break;
    }
    [self settingsChanged:SETTINGS_VISU];
}

-(void)oglUpdateFSStatus {
    pMenu_fullscreenStatus=settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value;
    
    oglViewFullscreenChanged=1;
    shouldUpdateCoverTexture=1;
    
    if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) {
        if (mOglViewIsHidden) {
            mOglViewIsHidden=NO;
            [self checkGLViewCanDisplay];
        }
    }
    [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
}

- (void)oglViewSwitchFS {
    settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=!(settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value);
    [self oglUpdateFSStatus];
}

- (IBAction)backPushed:(id)sender {
    [[self navigationController] setNavigationBarHidden:NO animated:NO];
    [[self navigationController] popViewControllerAnimated:YES];
}

- (IBAction)playPushed {
    mPaused=0;
    if (mIsPlaying) {
        self.pauseBarSub.hidden=YES;
        self.playBarSub.hidden=YES;
        self.pauseBar.hidden=YES;
        self.playBar.hidden=YES;
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) {
            self.pauseBarSub.hidden=NO;
        } else {
            self.pauseBar.hidden=NO;
        }
        
        [self updateBarPos];
        [mplayer Pause:NO];
    } else {
        [self play_curEntry:-1];
    }
    return;
}
- (IBAction)pausePushed {
    mPaused=1;
    if (mIsPlaying) {
        self.pauseBarSub.hidden=YES;
        self.playBarSub.hidden=YES;
        self.pauseBar.hidden=YES;
        self.playBar.hidden=YES;
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) {
            self.playBarSub.hidden=NO;
        } else {
            self.playBar.hidden=NO;
        }
        [self updateBarPos];
        [mplayer Pause:YES];
    }
    return;
}

- (IBAction)sliderProgressBeginChange:(id)sender {
    sliderProgressModuleEdit=1;
}

- (IBAction)sliderProgressValueChanged:(id)sender {
    int slider_time;
    //sliderProgressModuleChanged=1;
    
    if (curSongLength>0) slider_time=(int)(sliderProgressModule.value*(float)(curSongLength-1));
    
    if (display_length_mode&&(curSongLength>0)) {
        labelTime.text=[NSString stringWithFormat:@"-%.2d:%.2d", ((curSongLength-slider_time)/1000)/60,((curSongLength-slider_time)/1000)%60];
    } else {
        labelTime.text=[NSString stringWithFormat:@"%.2d:%.2d", (slider_time/1000)/60,(slider_time/1000)%60];
    }
    return;
}

- (IBAction)sliderProgressEndChange:(id)sender {
    int64_t curTime=0;
    if (curSongLength>0) curTime=(int64_t)(sliderProgressModule.value*(float)(curSongLength-1));
    else return;
    
    if (mPaused) [self playPushed];
    
    [mplayer Seek:curTime];
    _seekRequested=curTime;
    
    if (display_length_mode&&(curSongLength>0)) labelTime.text=[NSString stringWithFormat:@"-%.2d:%.2d", ((curSongLength-[mplayer getCurrentTime])/1000)/60,((curSongLength-[mplayer getCurrentTime])/1000)%60];
    else labelTime.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getCurrentTime]/1000)/60,([mplayer getCurrentTime]/1000)%60];
    sliderProgressModuleChanged=0;
    sliderProgressModuleEdit=0;
    return;
}


-(void) jumpSeekFwd {
    int64_t itime=[mplayer getCurrentTime];
    itime+=10000;
    [mplayer Seek:itime];
    _seekRequested=itime;
}

-(void) jumpSeekBwd {
    int64_t itime=[mplayer getCurrentTime];
    itime-=10000;
    if (itime<0) itime=0;
    [mplayer Seek:itime];
    _seekRequested=itime;
}

-(void) seek:(NSNumber*)seekTime {
    int64_t curTime;
    if (curSongLength>0) curTime=[seekTime intValue];//(int)(sliderProgressModule.value*(float)(curSongLength-1));
    else return;
    
    MDZILog("seek %d",(int)(curTime/1000));
    
    if (mPaused) [self playPushed];
    
    [mplayer Seek:curTime];
    _seekRequested=curTime;
    
    if (display_length_mode&&(curSongLength>0)) labelTime.text=[NSString stringWithFormat:@"-%.2d:%.2d", ((curSongLength-[mplayer getCurrentTime])/1000)/60,((curSongLength-[mplayer getCurrentTime])/1000)%60];
    else labelTime.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getCurrentTime]/1000)/60,([mplayer getCurrentTime]/1000)%60];
    //    sliderProgressModuleChanged=0;
    //    sliderProgressModuleEdit=0;
    
    return;
}

-(void) updMediaCenterProgress {
    //MPNowPlayingInfoCenter *infoCenter = [MPNowPlayingInfoCenter defaultCenter];
    [self updMediaCenter];
}
-(void) updMediaCenter {
    static bool no_reetrant=false;
    if (no_reetrant) return;
    no_reetrant=true;
    MPNowPlayingInfoCenter *infoCenter = [MPNowPlayingInfoCenter defaultCenter];
    
    MPRemoteCommandCenter *cmdCenter=[MPRemoteCommandCenter sharedCommandCenter];
    
    if (artwork==nil) {
        if (cover_img) artwork=[[MPMediaItemArtwork alloc] initWithImage:cover_img];
        else artwork=[[MPMediaItemArtwork alloc] initWithImage:default_cover];
    }
    
    if (mPlaylist_size) {
        NSString *artist=mplayer.artist;
        NSString *album=mplayer.album;
        NSString *title;
        
        if ([mplayer getModFileTitle]) title=[NSString stringWithFormat:@"%@ /%@",[mplayer getModFileTitle],[mplayer getModName]];
        else title=[NSString stringWithFormat:@"%@",[mplayer getModName]];
        
        //if (is_macOS) {
        if (mIsPlaying) {
            if (mPaused) infoCenter.playbackState=MPNowPlayingPlaybackStatePaused;
            else infoCenter.playbackState=MPNowPlayingPlaybackStatePlaying;
        } else infoCenter.playbackState=MPNowPlayingPlaybackStateStopped;
        //}
        
        if (mIsPlaying) {
            if (mPaused) {
                [cmdCenter.playCommand setEnabled:YES];
                [cmdCenter.pauseCommand setEnabled:NO];
            } else {
                [cmdCenter.playCommand setEnabled:NO];
                [cmdCenter.pauseCommand setEnabled:YES];
            }
        } else {
            [cmdCenter.playCommand setEnabled:YES];
            [cmdCenter.pauseCommand setEnabled:NO];
        }
        
        // Update shuffle and repeat state for CarPlay
        if (mShuffle) {
            cmdCenter.changeShuffleModeCommand.currentShuffleType = MPShuffleTypeItems;
        } else {
            cmdCenter.changeShuffleModeCommand.currentShuffleType = MPShuffleTypeOff;
        }
        
        switch (mLoopMode) {
            case 0:
                cmdCenter.changeRepeatModeCommand.currentRepeatType = MPRepeatTypeOff;
                break;
            case 1:
                cmdCenter.changeRepeatModeCommand.currentRepeatType = MPRepeatTypeAll;
                break;
            case 2:
                cmdCenter.changeRepeatModeCommand.currentRepeatType = MPRepeatTypeOne;
                break;
        }
        
        infoCenter.nowPlayingInfo = [NSMutableDictionary dictionaryWithObjectsAndKeys:
                                     title,
                                     MPMediaItemPropertyTitle,
                                     artist,
                                     MPMediaItemPropertyArtist,
                                     artwork,
                                     MPMediaItemPropertyArtwork,
                                     artist,
                                     MPMediaItemPropertyAlbumArtist,
                                     album,
                                     MPMediaItemPropertyAlbumTitle,
                                     [NSNumber numberWithInt:MPNowPlayingInfoMediaTypeAudio],
                                     MPNowPlayingInfoPropertyMediaType,
                                     
                                     [NSNumber numberWithUnsignedInteger:mPlaylist_pos],
                                     MPNowPlayingInfoPropertyPlaybackQueueIndex,
                                     [NSNumber numberWithUnsignedInteger:mPlaylist_size],
                                     MPNowPlayingInfoPropertyPlaybackQueueCount,
                                     
                                     [NSNumber numberWithFloat:(float)([mplayer getSongLength])/1000],
                                     MPMediaItemPropertyPlaybackDuration,
                                     [NSNumber numberWithFloat:(float)([mplayer getCurrentTime])/1000],
                                     MPNowPlayingInfoPropertyElapsedPlaybackTime,
                                     (mPaused?@0:@1),
                                     MPNowPlayingInfoPropertyPlaybackRate,
                                     
                                     //[NSNumber numberWithUnsignedInteger:mPlaylist_pos],
                                     //MPMediaItemPropertyPersistentID,
                                     nil];
    } else {
        
        if (is_macOS) {
            infoCenter.playbackState=MPNowPlayingPlaybackStateStopped;
        }
        
        if (mIsPlaying) {
            [cmdCenter.playCommand setEnabled:YES];
            [cmdCenter.pauseCommand setEnabled:NO];
        }
        
        infoCenter.nowPlayingInfo = [NSDictionary dictionaryWithObjectsAndKeys:
                                     artwork,
                                     MPMediaItemPropertyArtwork,
                                     @0,
                                     MPNowPlayingInfoPropertyPlaybackQueueIndex,
                                     @0,
                                     MPNowPlayingInfoPropertyPlaybackQueueCount,
                                     
                                     @0,
                                     MPMediaItemPropertyPlaybackDuration,
                                     @0,
                                     MPNowPlayingInfoPropertyElapsedPlaybackTime,
                                     @0,
                                     MPNowPlayingInfoPropertyPlaybackRate,
                                     
                                     nil];
    }
    
    no_reetrant=false;
}


-(IBAction) changeTimeDisplay {
    display_length_mode^=1;
}


//define the targetmethod
-(void) updateInfos: (NSTimer *) theTimer {
    static bool noReEntrant=false;
    static int noProgressCnt=0;
    
    if (noReEntrant) return;
    noReEntrant=true;
    
    static int last_itime=0;
    bool noProgress=false;
    int itime=[mplayer getCurrentTime];
    if (itime==last_itime) {
        noProgressCnt++;
        if (noProgressCnt>2) noProgress=true; //5 is 1 second
    } else noProgressCnt=0;
    last_itime=itime;
    
    /*
     Issue in loading file
     */
    if (mplayer.mLoadModuleStatus<0) {
        //remove playlist entry when issue detected after loading, i.e. UADE failing
        mLoadIssueMessage=3;
        mplayer.mLoadModuleStatus=0;
        
        if (mplayer_error_msg[0]==0) snprintf(mplayer_error_msg,sizeof(mplayer_error_msg),"%s",[mPlaylist[mPlaylist_pos].mPlaylistFilepath UTF8String]);
        
        [self remove_from_playlist:mPlaylist_pos];
        
        [self refreshCurrentVC];
        ///////////////////////////////////////////////////
        // Update miniplayer
        ///////////////////////////////////////////////////
        UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
        mdz_safe_execute_sel(vc,@selector(updateMiniPlayer),nil)
        
        if ((alertCannotPlay_displayed==0)&&(mLoadIssueMessage)) {
            NSString *alertMsg;
            alertCannotPlay_displayed=1;
            [self openPopup:NSLocalizedString(@"File cannot be played. Skipping to next playable file.",@"") secmsg:[NSString stringWithFormat:@"%s",mplayer_error_msg] style:POPUP_STYLE_ALERT];
            
            [self play_curEntry:-1];
            
        } else {
            
            [self play_curEntry:-1];
        }
        noReEntrant=false;
        return;
    }
    /*
     Check if stat should be sent to server
     */
    if (mSendStatTimer) {
        mSendStatTimer--;
        if (mSendStatTimer==0) {
            short int playcount;
            signed char tmp_rating,avg_rating;
            DBHelper::getFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilepath,&playcount,&tmp_rating,&avg_rating);
            [GoogleAppHelper SendStatistics:mPlaylist[mPlaylist_pos].mPlaylistFilename path:mPlaylist[mPlaylist_pos].mPlaylistFilepath rating:tmp_rating playcount:playcount];
        }
    }
    
    
    /*
     Should we update the file infos ?
     */
    int mpl_upd=[mplayer shouldUpdateInfos];
    
    /*
     If there is an upcoming song end, do not update infos
     */
    //MDZILog("time update %.1f %.1f noprog:%d",(float)itime/1000.0,(float)curSongLength/1000.0,noProgress);
    bool delayUpdate=false;
    if (mpl_upd) {
        if (curSongLength>3000) {
            if ((itime>curSongLength-3000)&&(itime<curSongLength-100)) {
                
                if (!mPaused && !noProgress) {
                    delayUpdate=true;
                    //MDZFLog("pending update %.1f %.1f",(float)itime/1000.0,(float)curSongLength/1000.0);
                } else {
                    if (mPaused) MDZFLog("paused");
                    if (noProgress) MDZFLog("no progress");
                }
            }
        }
    }
    
    /*
     update infos
     */
    if ( !delayUpdate && (mpl_upd) || mShouldUpdateInfos ) {
        
        ///////////////////////////////////////////////////
        // Update miniplayer
        ///////////////////////////////////////////////////
        UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
        mdz_safe_execute_sel(vc,@selector(updateMiniPlayer),nil)
        
        
        /////////////////////////////////////////////////////////////////////////////
        //update rating
        /////////////////////////////////////////////////////////////////////////////
        mPlaylist[mPlaylist_pos].mPlaylistRating=[self getCurrentRating];
        
        [self showRating:mPlaylist[mPlaylist_pos].mPlaylistRating];
        
        
        if ((mpl_upd!=3)||(mShouldUpdateInfos)) {
            short int playcount=0;
            signed char tmp_rating;
            NSString *fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
            NSString *filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
            
            [self updateStats:fileName filePath:filePath playcount_inc:false];
            
            mShouldUpdateInfos=0;
        }
        if (mpl_upd>=2) {
            if (mpl_upd==2) {
                
                labelArtist.text=mplayer.artist;
                
                if (labelArtist.text && [labelArtist.text length]) {
                    labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,22);
                    labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,18);
                } else {
                    labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,40);
                    labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,0);
                }
                
                if (settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value==0) {
                    {
                        if ([mplayer getModFileTitleOrNull]) labelModuleName.text=[NSString stringWithFormat:@"%@ /%@",[mplayer getModFileTitle],[mplayer getModName]];
                        else labelModuleName.text=[NSString stringWithFormat:@"%@",[mplayer getModName]];
                        
                        [self refreshFXFSLabels];
                    }
                }
            }
            if (infoView.hidden==FALSE) {
                textMessage.text=[NSString stringWithFormat:@"%@",[mplayer getModMessage]];
            }
            if (mpl_upd==3) {
                if (infoView.hidden==FALSE) [textMessage scrollRangeToVisible:NSMakeRange([textMessage.text length], 0)];
            }
        }
        [mplayer setInfosUpdated];
        if ((mplayer.mod_subsongs>1)/*&&(mOnlyCurrentSubEntry==0)*/) {
            int mpl_arcCnt=[mplayer getArcEntriesCnt];
            if (mpl_arcCnt) {
                playlistPos.text=[NSString stringWithFormat:@"%d of %d/arc %d of %d/sub %d(%d,%d)",mPlaylist_pos+1,mPlaylist_size,
                                  [mplayer getArcIndex]+1,mpl_arcCnt,
                                  mplayer.mod_currentsub-mplayer.mod_minsub+1,1,mplayer.mod_subsongs];
            } else {
                playlistPos.text=[NSString stringWithFormat:@"%d of %d/sub %d(%d,%d)",mPlaylist_pos+1,mPlaylist_size,mplayer.mod_currentsub-mplayer.mod_minsub+1,1,mplayer.mod_subsongs];
            }
            //[pvSubSongSel reloadAllComponents];
            
            current_selmode=ARCSUB_MODE_NONE;
            [self dismissViewControllerAnimated:YES completion:nil];
            
            if (btnShowSubSong.hidden==true) {
                if (mOnlyCurrentSubEntry==0) btnShowSubSong.hidden=false;
            }
            
            
        } else {
            int mpl_arcCnt=[mplayer getArcEntriesCnt];
            if (mpl_arcCnt) {
                playlistPos.text=[NSString stringWithFormat:@"%d of %d/arc %d of %d",mPlaylist_pos+1,mPlaylist_size,
                                  [mplayer getArcIndex]+1,mpl_arcCnt];
            } else playlistPos.text=[NSString stringWithFormat:@"%d of %d",mPlaylist_pos+1,mPlaylist_size];
            btnShowSubSong.hidden=true;
        }
        self.pauseBarSub.hidden=YES;
        self.playBarSub.hidden=YES;
        self.pauseBar.hidden=YES;
        self.playBar.hidden=YES;
        if (mIsPlaying&& (mplayer.bGlobalAudioPause==0) ) {
            if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.pauseBarSub.hidden=NO;
            else self.pauseBar.hidden=NO;
            mPaused=0;
        } else {
            if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.playBarSub.hidden=NO;
            else self.playBar.hidden=NO;
            mPaused=1;
        }
        [self updateBarPos];
        
        curSongLength=[mplayer getSongLength];
        
        if (curSongLength<0) {
            if (display_length_mode) display_length_mode=0;
            sliderProgressModule.enabled=FALSE;
            labelModuleLength.text=@"--:--";
        } else {
            
            sliderProgressModule.enabled=TRUE;
            labelModuleLength.text=[NSString stringWithFormat:@"%.2d:%.2d", (curSongLength/1000)/60,(curSongLength/1000)%60];
        }
    }
    
    /*
     Have we gone too far ?
     */
    
    if ((curSongLength>0)&&(itime>curSongLength)) // if gone too far, limit
        itime=curSongLength;
    
    
    /*
     If slider isn't being updated, update UI elements / progress
     */
    if (!sliderProgressModuleEdit) {
        
        if (_seekRequested) {
            static int64_t last_time_diff=0xFFFFFFFFFFFFFF;
            int64_t time_diff=abs(itime-_seekRequested);
            if (time_diff<1000) {
                _seekRequested=-1;
                last_time_diff=0xFFFFFFFFFFFFFF;
            } else {
                int64_t ctime=_seekRequested;
                labelTime.text=NSLocalizedString(@"Seeking", @"");
                if ( (time_diff<last_time_diff) && (itime<=_seekRequested) ) ctime=itime;
                if (time_diff<last_time_diff) last_time_diff=time_diff;
                
                sliderProgressModule.value=(float)(ctime)/(float)(curSongLength);
            }
        }
        
        if (_seekRequested==-1) {
            
            if (noProgress && mIsPlaying && !mPaused) {
                labelTime.text=NSLocalizedString(@"Buffering", @"");
            } else {
                labelTime.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getCurrentTime]/1000)/60,([mplayer getCurrentTime]/1000)%60];
                
                if (curSongLength>0) {
                    if (display_length_mode) labelTime.text=[NSString stringWithFormat:@"-%.2d:%.2d", ((curSongLength-itime)/1000)/60,((curSongLength-itime)/1000)%60];
                    //                MDZILog("itime: %d",int(itime));
                    sliderProgressModule.value=(float)(itime)/(float)(curSongLength);
                    
                } else {
                }
            }
        }
    }
    
    /*
     If a seek is in progress, update UI accordongly
     */
    int seekinprogress=[mplayer isSeeking];
    if (seekinprogress) {
        labelSeeking.hidden=FALSE;
        labelSeeking.text=NSLocalizedString(@"Seeking",@"");
    } else {
        labelSeeking.hidden=TRUE;
    }
    
    /*
     Is the end reached and file has ended ?
     */
    if (/*(mPaused==0)&&*/(mplayer.bGlobalAudioPause==2)&&[mplayer isEndReached]) {//mod ended
        //have to update the pause button
        //mSendStatTimer=0;
        mIsPlaying=FALSE;
        mPaused=1;
        self.pauseBarSub.hidden=YES;
        self.playBarSub.hidden=YES;
        self.pauseBar.hidden=YES;
        self.playBar.hidden=YES;
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.playBarSub.hidden=NO;
        else self.playBar.hidden=NO;
        
        [self updateBarPos];
        
        //check if video recording is in progress and if should stop
        if (isRecordingScreen!=RS_NOT_RECORDING) {
            if ((isRecordingScreen==RS_RECORDING_AND_STOP)||(isRecordingScreen==RS_RECORDING_AND_STOP_FS)) [self StopRecording];
        }
        
        //and go to next entry if playlist
        if ((mLoopMode==2)||(mplayer.mLoopMode==1))  [self play_curEntry:-1];
        else {
            if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0)) {
                if ([mplayer selectNextArcEntry]<0) [self play_nextEntry];
                else {
                    [self showWaitingLoading];
                    if ([self play_loadArchiveModule]==FALSE) [self play_nextEntry];
                    [self hideWaiting];
                }
            } else [self play_nextEntry];
        }
        noReEntrant=false;
        return;
    } else {
    }
    
    /*
     Update MediaCenter every 5 calls
     */
    if (updMPNowCnt==0) {
        [self updMediaCenterProgress];
        updMPNowCnt=5;
    } else updMPNowCnt--;
    noReEntrant=false;
    return;
}

int qsort_ComparePlEntries(const void *entryA, const void *entryB) {
    NSString *strA,*strB;
    NSComparisonResult res;
    strA=((t_plPlaylist_entry*)entryA)->mPlaylistFilename;
    strB=((t_plPlaylist_entry*)entryB)->mPlaylistFilename;
    res=[strA localizedCaseInsensitiveCompare:strB];
    if (res==NSOrderedAscending) return -1;
    if (res==NSOrderedSame) return 0;
    return 1; //NSOrderedDescending
}

int qsort_ComparePlEntriesRev(const void *entryA, const void *entryB) {
    NSString *strA,*strB;
    NSComparisonResult res;
    strA=((t_plPlaylist_entry*)entryA)->mPlaylistFilename;
    strB=((t_plPlaylist_entry*)entryB)->mPlaylistFilename;
    res=[strB localizedCaseInsensitiveCompare:strA];
    if (res==NSOrderedAscending) return -1;
    if (res==NSOrderedSame) return 0;
    return 1; //NSOrderedDescending
}

- (void) updRadioInfo {
    if ([radioSource getHistorySize]>0) [btnRadioPrevList setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    else [btnRadioPrevList setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    
    if (!bShowRadio) return;
    NSString *title_str;
    NSString *msg_str;
    NSMutableAttributedString *msg_strAttr;
    
    NSDictionary *baseAttributes = @{
        NSForegroundColorAttributeName:[UIColor colorWithRed:0.94 green:0.89 blue:1.0 alpha:1.0],
        NSFontAttributeName:[UIFont systemFontOfSize:14 weight:UIFontWeightRegular],
        NSBackgroundColorAttributeName:[UIColor clearColor]
    };
    NSDictionary *attributesDataNext = @{
        NSForegroundColorAttributeName:[UIColor colorWithWhite:1.0 alpha:1],
        NSFontAttributeName:[UIFont systemFontOfSize:14 weight:UIFontWeightMedium],
    };
    NSRange rangeData;
    int pos;
    
    NSString *nextEntry;
    if ([radioSource queueSize]>1) nextEntry=[radioSource getQueueLabel:1];
    else {
        nextEntry=@"loading...";
        NSTimer *checkAgain;
        checkAgain=[NSTimer scheduledTimerWithTimeInterval: 0.50f target:self selector:@selector(updRadioInfo) userInfo:nil repeats: NO];
    }
    msg_str=[NSString stringWithFormat:NSLocalizedString(@"Up next: %@",@""),nextEntry];
    title_str=[NSString stringWithFormat:@"%@ - %@",NSLocalizedString(@"Radio mode",@""),[radioSource radioSourceName]];
        
    msg_strAttr=[[NSMutableAttributedString alloc] initWithString:msg_str attributes:baseAttributes];
    pos=(int)[msg_str rangeOfString:@"Up next: "].location+(int)[@"Up next: " length];
    rangeData = NSMakeRange(pos,[nextEntry length]);
    [msg_strAttr setAttributes:attributesDataNext range:rangeData];
    
    radioTitle.text=title_str;
    radioInfo.attributedText=msg_strAttr;
}

-(IBAction) radioShowPrevList:(id)sender {
    
    if ([radioSource getHistorySize]==0) return;
    
    UIViewController *controller = [[UIViewController alloc]init];
    CGRect rect,recttv;
    const NSInteger kAlertTableViewTag = 10001;
    
    current_selmode=ARCSUB_MODE_RADIO;
    
    int entries=[radioSource getHistorySize];
    
    float rw,rh,rx,ry;
    if (self.view.traitCollection.horizontalSizeClass==UIUserInterfaceSizeClassCompact) {
        float estimated_height=SELECTOR_TABVIEWCELL_HEIGHT*(entries+1+1)+32;
        rx=0;
        ry=32;
        rw=self.view.frame.size.width;
        
        if (estimated_height<self.view.frame.size.height-50-ry) rh=estimated_height;
        else rh=self.view.frame.size.height-50-ry;
        rect = CGRectMake(rx, ry,rw,rh+50);
        recttv = CGRectMake(rx, ry,rw,rh);
    } else {
        float estimated_height=SELECTOR_TABVIEWCELL_HEIGHT*(entries+1)+16;
        
        rw=self.view.frame.size.width;
        if (estimated_height<self.view.frame.size.height*0.8f-100) rh=estimated_height;
        else rh=self.view.frame.size.height*0.8f-100;
        rect = CGRectMake(rw*0.15f, 0,rw*0.7f,rh+100);
        recttv = CGRectMake(0, 16,rw*0.7f,rh);
        
    }
    [controller setPreferredContentSize:rect.size];
    
    controller.modalPresentationStyle=UIModalPresentationPopover;
    
    UIView *containerView=[[UIView alloc] initWithFrame:recttv];
    alertTableView  = [[UITableView alloc] initWithFrame:containerView.bounds];
    containerView.backgroundColor = [UIColor clearColor];
    
    alertTableView.layer.cornerRadius = 10;
    alertTableView.layer.masksToBounds = true;
    alertTableView.translatesAutoresizingMaskIntoConstraints = NO;
    [containerView addSubview:alertTableView];
    
    alertTableView.delegate = self;
    alertTableView.dataSource = self;
    alertTableView.tableFooterView = [[UIView alloc]initWithFrame:CGRectZero];
    alertTableView.rowHeight=SELECTOR_TABVIEWCELL_HEIGHT;
    alertTableView.sectionHeaderHeight=32;
    
    [alertTableView setSeparatorStyle:UITableViewCellSeparatorStyleSingleLine];
    [alertTableView setTag:kAlertTableViewTag];
    
    containerView.translatesAutoresizingMaskIntoConstraints = NO;
    [controller.view addSubview:containerView];// alertTableView];
    
    
    [controller.view bringSubviewToFront:containerView];//alertTableView];
    [controller.view setUserInteractionEnabled:YES];
    [alertTableView setUserInteractionEnabled:YES];
    [alertTableView setAllowsSelection:YES];
    
    BButton *cancel_btn= [[BButton alloc] initWithFrame:CGRectMake(0, 0, 200, 30)];
    [cancel_btn setType:BButtonTypeGray];
    [cancel_btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
    [cancel_btn addTarget:self action:@selector(cancelSubSel) forControlEvents:UIControlEventTouchUpInside];
    [cancel_btn setTitle:NSLocalizedString(@"Close", @"") forState:UIControlStateNormal];
    cancel_btn.translatesAutoresizingMaskIntoConstraints = NO;
    [controller.view addSubview:cancel_btn];
    
    NSDictionary * viewsDic = NSDictionaryOfVariableBindings(cancel_btn, containerView);
    
    // Contraintes horizontales pour le containerView
    NSArray * hConstraintsContainer = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|[containerView]|"
                                                                              options:0
                                                                              metrics:nil
                                                                                views:viewsDic];
    [controller.view addConstraints:hConstraintsContainer];
    
    // Contraintes horizontales pour le bouton
    NSArray * hConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|-50-[cancel_btn]-50-|"
                                                                     options:0
                                                                     metrics:nil
                                                                       views:viewsDic];
    [controller.view addConstraints:hConstraints];
    
    // Contraintes verticales pour positionner containerView en haut et cancel_btn en bas
    NSArray * vConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-16-[containerView]-8-[cancel_btn(40)]-16-|"
                                                                     options:0
                                                                     metrics:nil
                                                                       views:viewsDic];
    [controller.view addConstraints:vConstraints];
    
    // Contraintes pour alertTableView à l'intérieur de containerView
    [NSLayoutConstraint activateConstraints:@[
        [alertTableView.topAnchor constraintEqualToAnchor:containerView.topAnchor],
        [alertTableView.bottomAnchor constraintEqualToAnchor:containerView.bottomAnchor],
        [alertTableView.leadingAnchor constraintEqualToAnchor:containerView.leadingAnchor],
        [alertTableView.trailingAnchor constraintEqualToAnchor:containerView.trailingAnchor]
    ]];
    
    [self presentViewController:controller animated:YES completion:^{
//        NSIndexPath *indexPath = [NSIndexPath indexPathForRow:0 inSection:0];
//        [alertTableView selectRowAtIndexPath:indexPath animated:YES scrollPosition:UITableViewScrollPositionMiddle];
    }];
    
    UIButton *btn=(UIButton*)sender;
    UIPopoverPresentationController *popoverctrl=controller.popoverPresentationController;
    popoverctrl.sourceView = btn;
    popoverctrl.sourceRect = CGRectMake(0, 0, btn.frame.size.width, btn.frame.size.height);
    if (self.view.traitCollection.horizontalSizeClass==UIUserInterfaceSizeClassCompact) {
        popoverctrl.backgroundColor=[UIColor blackColor];
    } else {
        popoverctrl.backgroundColor=[UIColor clearColor];
    }
    
    popoverctrl.delegate=self;
    //popoverctrl.permittedArrowDirections=UIPopoverArrowDirectionUp;
}

- (void) showRadioPopup {
    if (bShowRadio) {
        [radioTitle removeFromSuperview];
        [radioInfo removeFromSuperview];
        [radioView removeFromSuperview];
        bShowRadio=false;
    } else {
        radioView = [[UIView alloc] init];
        radioView.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y,mainView.frame.size.width,UI_RADIO_INFO_HEIGHT);
        radioView.opaque=false;
        radioView.backgroundColor=[UIColor colorWithWhite:0 alpha:0.7];
        [self.mainView addSubview:radioView];
        radioView.layer.zPosition=m_oglView.layer.zPosition+0.1;
        
        radioTitle=[[UILabel alloc] initWithFrame:CGRectMake(2,2,radioView.frame.size.width-2,14)];
        radioTitle.textColor=[UIColor colorWithRed:0.92 green:0.85 blue:1.0 alpha:1.0];
        radioTitle.backgroundColor=[UIColor clearColor];
        
        radioTitle.font=[UIFont systemFontOfSize:17 weight:UIFontWeightMedium];
        [radioView addSubview:radioTitle];
        
        radioInfo=[[UITextView alloc] initWithFrame:CGRectMake(2,14,radioView.frame.size.width-2,radioView.frame.size.height-14)];
        radioInfo.textColor=[UIColor colorWithRed:1.0 green:1.0 blue:1.0 alpha:1.0];
        radioInfo.backgroundColor=[UIColor clearColor];
        radioInfo.font=[UIFont systemFontOfSize:14];
        radioInfo.editable=NO;
        radioInfo.selectable=NO;
        [radioView addSubview:radioInfo];
        
        bShowRadio=true;
        
        [self updRadioInfo];
    }
}

- (IBAction)showPlaylist {
    t_playlist* temp_playlist;
    temp_playlist=(t_playlist*)calloc(1,sizeof(t_playlist));
    
    if (mPlaylist_size) { //display current queue
        for (int i=0;i<mPlaylist_size;i++) {
            temp_playlist->entries[i].label=[[NSString alloc] initWithString:mPlaylist[i].mPlaylistFilename];
            temp_playlist->entries[i].fullpath=[[NSString alloc ] initWithString:mPlaylist[i].mPlaylistFilepath];
            
            temp_playlist->entries[i].ratings=mPlaylist[i].mPlaylistRating;
            temp_playlist->entries[i].playcounts=-1;
        }
        temp_playlist->nb_entries=mPlaylist_size;
        temp_playlist->playlist_name=NSLocalizedString(@"Now playing",@"");
        temp_playlist->playlist_id=nil;
        
        nowplayingPL = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
        //set new title
        nowplayingPL.title = temp_playlist->playlist_name;
        ((RootViewControllerPlaylist*)nowplayingPL)->show_playlist=1;
        
        // Set new directory
        ((RootViewControllerPlaylist*)nowplayingPL)->browse_depth = 1;
        ((RootViewControllerPlaylist*)nowplayingPL)->detailViewController=self;
        ((RootViewControllerPlaylist*)nowplayingPL)->playlist=temp_playlist;
        ((RootViewControllerPlaylist*)nowplayingPL)->mDetailPlayerMode=1;
        ((RootViewControllerPlaylist*)nowplayingPL)->integrated_playlist=INTEGRATED_PLAYLIST_NOWPLAYING;
        ((RootViewControllerPlaylist*)nowplayingPL)->currentPlayedEntry=mPlaylist_pos+1;
        
        // And push the window
        [self.navigationController pushViewController:nowplayingPL animated:YES];
    }
    
}

int recording=0;
- (IBAction)showEQ {
    if (bShowEQ) {
        [eqVC viewWillDisappear:YES];
        [eqVC.view removeFromSuperview];
        [eqVC removeFromParentViewController];
    } else {
        eqVC = [[EQViewController alloc]  initWithNibName:@"EQViewController" bundle:[NSBundle mainBundle]];
        //set new title
        eqVC.title = @"Equalizer";
        eqVC.detailViewController=self;
        
        eqVC.view.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y,mainView.frame.size.width,m_oglView.frame.size.height);
        
        [self addChildViewController:eqVC];
        [self.view addSubview:eqVC.view];
    }
}


- (IBAction)infoFullscreen {
    infoIsFullscreen=1;
    infoZoom.hidden=YES;
    infoUnzoom.hidden=NO;
    [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
}
- (IBAction)infoNormal {
    infoIsFullscreen=0;
    infoZoom.hidden=NO;
    infoUnzoom.hidden=YES;
    mainView.hidden=NO;
    [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
}

- (IBAction)showInfo {
    if (infoView.hidden==NO) {
        [self hideInfo];
        return;
    }
    textMessage.text=[NSString stringWithFormat:@"%@",[mplayer getModMessage]];
    
    infoView.hidden=NO;
}

- (IBAction)hideInfo {
    
    infoView.hidden=YES;
    
}

- (void)restartCurrent {
    if ([mplayer isArchive]&&(mplayer.mod_subsongs<=1)) {
        [mplayer selectArcEntry:[mplayer getArcIndex]];
        
        [self showWaitingLoading];
        
        [self play_loadArchiveModule];
        
        [self hideWaiting];
        [self refreshCurrentVC];
    } else {
        //restart
        if (mplayer.mod_subsongs>1) [mplayer playGoToSub:mplayer.mod_currentsub];
        else [self play_curEntry:-1];
        if (mPaused) [self playPushed];
        [self refreshCurrentVC];
    }
}

- (IBAction)playPrevSub {
    static bool no_reentrant=false;
    if (mShuffle==1) {
        [self playPrev];
        return;
    }
    if (no_reentrant) return;
    no_reentrant=true;
    if ([mplayer getCurrentTime]>=MIN_DELAY_PREV_ENTRY) {//if more than MIN_DELAY_PREV_ENTRY milliseconds are elapsed, restart current track
        [self restartCurrent];
        clearAudioFXbuffer=true;
        _seekRequested=-1;
        
        if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
            [self sendNotifPlayedTitle];
        }
        no_reentrant=false;
        return;
    }
    
    if ([mplayer isArchive]&&(mplayer.mod_subsongs<=1)&&(mOnlyCurrentEntry==0)) {
        //if archive and no subsongs => change archive index
        if ([mplayer selectPrevArcEntry]<0) [self playPrev];
        else {
            [self showWaitingLoading];
            [self play_loadArchiveModule];
            [self hideWaiting];
        }
        clearAudioFXbuffer=true;
        _seekRequested=-1;
        
        if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
            [self sendNotifPlayedTitle];
        }
        if (mPaused) [self playPushed];
        [self refreshCurrentVC];
    } else {
        if ((mplayer.mod_subsongs>1)&&(mOnlyCurrentSubEntry==0)) {
            //Subsongs, try previous one
            if ([mplayer playPrevSub]<0) {
                //reach end
                if ([mplayer isArchive]) {
                    //archive, try previous one
                    if ([mplayer selectPrevArcEntry]<0) [self playPrev];
                    else {
                        [self showWaitingLoading];
                        [self play_loadArchiveModule];
                        [self hideWaiting];
                    }
                } else [self playPrev];
            }
            clearAudioFXbuffer=true;
            _seekRequested=-1;
            
            if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
                [self sendNotifPlayedTitle];
            }
        } else [self playPrev];
        if (mPaused) [self playPushed];
        [self refreshCurrentVC];
    }
    no_reentrant=false;
}

#pragma mark - UNUserNotificationCenterDelegate

// This method is called when a notification arrives while the app is in the foreground
- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions))completionHandler {
    
    //MDZDLog("Notification received in foreground: %@", notification.request.content.title);
    
    //    if (is_macOS) completionHandler(UNNotificationPresentationOptionBanner);
    //    else
    //    {
    //            completionHandler(UNNotificationPresentationOptionBanner);
    //    }
}

- (UNNotificationAttachment *)createAttachmentFromUIImage:(UIImage *)image {
    // Convert UIImage to file and create attachment
    if (!image) {
        return nil;
    }
    
    // Convert UIImage to NSData (PNG format)
    NSData *imageData = UIImagePNGRepresentation(image);
    // Alternative: Use JPEG with compression
    // NSData *imageData = UIImageJPEGRepresentation(image, 1.0);
    
    if (!imageData) {
        MDZELog("Failed to convert UIImage to NSData");
        return nil;
    }
    
    // Create temporary file URL
    NSString *tempDirectory = NSTemporaryDirectory();
    NSString *fileName = [NSString stringWithFormat:@"MDZNotif%@.png", [[NSUUID UUID] UUIDString]];
    NSURL *fileURL = [NSURL fileURLWithPath:[tempDirectory stringByAppendingPathComponent:fileName]];
    
    // Write image data to file
    NSError *writeError = nil;
    [imageData writeToURL:fileURL options:NSAtomicWrite error:&writeError];
    
    if (writeError) {
        MDZELog("Error writing image to file: %@", writeError.localizedDescription);
        return nil;
    }
    
    // Create attachment from file URL
    NSError *attachmentError = nil;
    UNNotificationAttachment *attachment =
    [UNNotificationAttachment attachmentWithIdentifier:@"uiimage"
                                                   URL:fileURL
                                               options:nil
                                                 error:&attachmentError];
    
    if (attachmentError) {
        MDZELog("Error creating attachment: %@", attachmentError.localizedDescription);
        return nil;
    }
    
    return attachment;
}

- (void)removeNotificationWithIdentifier:(NSString *)identifier {
    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    
    // Remove from notification center (delivered notifications)
    [center removeDeliveredNotificationsWithIdentifiers:@[identifier]];
    
    // Also remove from pending notifications (scheduled but not yet delivered)
    [center removePendingNotificationRequestsWithIdentifiers:@[identifier]];
    
    //MDZDLog("Notification removed: %@", identifier);
}

- (void)sendNotifPlayedTitle {
    // Configure the notification's payload.
    NSString *identifier=[NSString stringWithFormat:@"NewTitle%@",[[NSUUID UUID] UUIDString]];
    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    center.delegate=self;
    
    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
    content.title = [NSString localizedUserNotificationStringForKey:@"Playing" arguments:nil];
    //content.subtitle = [NSString localizedUserNotificationStringForKey:@"Subtitle" arguments:nil];
    //content.badge = [NSNumber numberWithInt:1];
    content.body = [NSString stringWithFormat:NSLocalizedString(@"%@",@""),labelModuleName.text];
    //content.sound = [UNNotificationSound defaultSound];
    content.interruptionLevel=UNNotificationInterruptionLevelActive;
    if (is_macOS) content.interruptionLevel=UNNotificationInterruptionLevelActive;
    
    // Add image attachment
    if (cover_img) {
        UNNotificationAttachment *attachment = [self createAttachmentFromUIImage:cover_img];
        if (attachment) {
            content.attachments = @[attachment];
        }
    }
    
    
    // Deliver the notification in five seconds.
    UNTimeIntervalNotificationTrigger* trigger = [UNTimeIntervalNotificationTrigger
                                                  triggerWithTimeInterval:0.1 repeats:NO];
    
    UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:identifier
                                                                          content:content trigger:trigger];
    
    // Schedule the notification.
    
    float notif_duration=settings[GLOB_NotificationDuration].detail.mdz_slider.slider_value;
    if (notif_duration<1) notif_duration=1;
    if (notif_duration>10) notif_duration=10;
    notif_duration=round(notif_duration * NSEC_PER_SEC);
    
    //    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    [center addNotificationRequest:request withCompletionHandler:^(NSError * _Nullable error) {
        if (!error) {
            // Schedule removal after 5 seconds
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(notif_duration)),
                           dispatch_get_main_queue(), ^{
                [self removeNotificationWithIdentifier:identifier];
            });
        }
    }];
    
}

- (IBAction)playNextSub {
    static bool no_reentrant=false;
    if (no_reentrant) return;
    no_reentrant=true;
    
    
    if (mShuffle==1) {
        [self playNext];
        clearAudioFXbuffer=true;
        _seekRequested=-1;
        
        no_reentrant=false;
        return;
    }
    
    //if archive and no subsongs => change archive index
    if ([mplayer isArchive]&&(mplayer.mod_subsongs<=1)&&(mOnlyCurrentEntry==0)) {
        if ([mplayer selectNextArcEntry]<0) [self playNext];
        else {
            [self showWaitingLoading];
            [self play_loadArchiveModule];
            [self hideWaiting];
            [self refreshCurrentVC];
        }
    } else {
        if ((mplayer.mod_subsongs>1)&&(mOnlyCurrentSubEntry==0)) { //subsongs
            if ([mplayer playNextSub]<0) { //end reached
                if ([mplayer isArchive]) {
                    //it is an archive, select next entry
                    if ([mplayer selectNextArcEntry]<0) [self playNext];
                    else {
                        [self showWaitingLoading];
                        [self play_loadArchiveModule];
                        [self hideWaiting];
                        [self refreshCurrentVC];
                    }
                } else [self playNext]; //not an archive, next entry
            } else {
                clearAudioFXbuffer=true;
                _seekRequested=-1;
                
                if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
                    [self sendNotifPlayedTitle];
                }
            }
        } else [self playNext]; //not an archive, next entry
        if (mPaused) [self playPushed];
        [self refreshCurrentVC];
    }
    no_reentrant=false;
}

-(void) longPressNextSubArc:(UIGestureRecognizer *)gestureRecognizer {
    static bool no_reentrant=false;
    if (no_reentrant) return;
    no_reentrant=true;
    if ([gestureRecognizer state]==UIGestureRecognizerStateBegan) {
        if ([mplayer isArchive]) {
            if ([mplayer selectNextArcEntry]<0) [self playNext];
            else {
                [self showWaitingLoading];
                [self play_loadArchiveModule];
                [self hideWaiting];
                [self refreshCurrentVC];
            }
        }
        clearAudioFXbuffer=true;
        _seekRequested=-1;
        
        if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
            [self sendNotifPlayedTitle];
        }
    }
    no_reentrant=false;
}

-(void) longPressPrevSubArc:(UIGestureRecognizer *)gestureRecognizer {
    static bool no_reentrant=false;
    if (no_reentrant) return;
    no_reentrant=true;
    if ([gestureRecognizer state]==UIGestureRecognizerStateBegan) {
        if ([mplayer isArchive]) {
            if ([mplayer selectPrevArcEntry]<0) [self playPrev];
            else {
                [self showWaitingLoading];
                [self play_loadArchiveModule];
                [self hideWaiting];
                [self refreshCurrentVC];
            }
        }
        clearAudioFXbuffer=true;
        _seekRequested=-1;
        
        if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
            [self sendNotifPlayedTitle];
        }
    }
    no_reentrant=false;
}

-(void) stop {
    [mplayer Stop];
    [repeatingTimer invalidate];
    repeatingTimer = nil;
    mPaused=TRUE;
    mIsPlaying=0;
    
    [self updMediaCenter];
}

-(void) clearQueue {
    for (int i=0;i<mPlaylist_size;i++) {
        mPlaylist[i].mPlaylistFilename=nil;
        mPlaylist[i].mPlaylistFilepath=nil;
    }
    mPlaylist_size=0;
}

- (IBAction)playNext {
    static bool no_reentrant=false;
    if (no_reentrant) return;
    no_reentrant=true;
    if ([self play_nextEntry]) {
        clearAudioFXbuffer=true;
        _seekRequested=-1;
    }
    
    no_reentrant=false;
}

- (IBAction)playPrev {
    static bool no_reentrant=false;
    if (no_reentrant) return;
    no_reentrant=true;
    if ([mplayer getCurrentTime]>=MIN_DELAY_PREV_ENTRY) {//if more than MIN_DELAY_PREV_ENTRY milliseconds are elapsed, restart current track
        [self play_curEntry:-1];
        clearAudioFXbuffer=true;
        _seekRequested=-1;
        
    } else if ([self play_prevEntry]) {
        clearAudioFXbuffer=true;
        _seekRequested=-1;
    }
    
    no_reentrant=false;
}
-(BOOL)play_curEntry:(int)subsong {
    NSString *fileName;
    NSString *filePath;
    mIsPlaying=FALSE;
    
    if (mPlaylist_size==0) {
        if (repeatingTimer) [repeatingTimer invalidate];
        repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
        [mplayer Stop];
        mPaused=1;
        if (mHasFocus) [[self navigationController] popViewControllerAnimated:YES];
        
        if ([radioSource isActive]) {
            [radioSource moveNext:TRUE];
        }
        return FALSE;
    }
    
    if (mPlaylist_pos>mPlaylist_size-1) mPlaylist_pos=0;
    
    fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
    filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
    mPlaylist[mPlaylist_pos].mPlaylistCount++;
    
    [self requestLoadNewFile:filePath fname:fileName arcidx:-1 subsong:subsong];
    return true;
    /*
     if ([self play_module:filePath fname:fileName subsong:subsong]==FALSE) {
     [self remove_from_playlist:mPlaylist_pos];
     
     [self hideWaiting];
     [self refreshCurrentVC];
     ///////////////////////////////////////////////////
     // Update miniplayer
     ///////////////////////////////////////////////////
     UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
     mdz_safe_execute_sel(vc,@selector(updateMiniPlayer),nil)
     
     if ((alertCannotPlay_displayed==0)&&(mLoadIssueMessage)) {
     NSString *alertMsg;
     alertCannotPlay_displayed=1;
     [self openPopup:NSLocalizedString(@"File cannot be played. Skipping to next playable file.",@"") secmsg:[NSString stringWithFormat:@"%s",mplayer_error_msg] style:POPUP_STYLE_ALERT];
     
     [self play_curEntry:-1];
     
     } else {
     
     [self play_curEntry:-1];
     }
     return FALSE;
     }
     
     [self hideWaiting];
     [self refreshCurrentVC];
     
     return TRUE;
     */
}

-(int)play_prevEntry {
    if (mPlaylist_size==0) {
        if (repeatingTimer) [repeatingTimer invalidate];
        repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
        [mplayer Stop];
        mPaused=1;
        if (mHasFocus) [[self navigationController] popViewControllerAnimated:YES];
        return 0;
    }
    if (mShuffle) {
        int i;
        int minval;
        minval=mPlaylist[0].mPlaylistCount;
        for (i=0;i<mPlaylist_size;i++) if (mPlaylist[i].mPlaylistCount<minval) minval=mPlaylist[i].mPlaylistCount;
        
        mPlaylist_pos=arc4random()%(mPlaylist_size);
        i=0;
        while ((i<mPlaylist_size)&&(mPlaylist[mPlaylist_pos].mPlaylistCount>minval)) {
            i++;
            mPlaylist_pos++; if (mPlaylist_pos>=mPlaylist_size) mPlaylist_pos=0;
        }
        [self play_curEntry:-1];
        return 1;
    } else {
        if (mPlaylist_pos>0) mPlaylist_pos--;
        else if (mLoopMode==1) mPlaylist_pos=mPlaylist_size-1;
        [self play_curEntry:-1];
        return 1;
    }
    return 0;
}

-(int)play_nextEntry {
    if ([radioSource isActive]) {
        [radioSource moveNext:FALSE];
        if ([radioSource isInLibrary:0]) [btnSaveFile setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
        else [btnSaveFile setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        [self updRadioInfo];
        return 1;
    } else {
        
        if (mPlaylist_size==0) {
            if (repeatingTimer) [repeatingTimer invalidate];
            repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
            [mplayer Stop];
            mPaused=1;
            if (mHasFocus) [[self navigationController] popViewControllerAnimated:YES];
            return 0;
        }
        if (mShuffle) {
            int i;
            int minval;
            minval=mPlaylist[0].mPlaylistCount;
            for (i=0;i<mPlaylist_size;i++) if (mPlaylist[i].mPlaylistCount<minval) minval=mPlaylist[i].mPlaylistCount;
            
            mPlaylist_pos=arc4random()%(mPlaylist_size);
            i=0;
            while ((i<mPlaylist_size)&&(mPlaylist[mPlaylist_pos].mPlaylistCount>minval)) {
                i++;
                mPlaylist_pos++; if (mPlaylist_pos>=mPlaylist_size) mPlaylist_pos=0;
            }
            [self play_curEntry:-1];
            
            return 1;
        } else if (mPlaylist_pos<mPlaylist_size-1) {
            mPlaylist_pos++;
            [self play_curEntry:-1];
            
            return 1;
        } else if (mLoopMode==1) {
            mPlaylist_pos=0;
            [self play_curEntry:-1];
            
            return 1;
        }
    }
    return 0;
}

-(void)play_randomEntry {
    
}

-(void)play_listmodules:(NSArray *)array start_index:(int)index path:(NSArray *)arrayFilepaths {
    int limitPl=0;
    mRestart=0;
    mRestart_sub=-1;
    mRestart_arc=0;
    mPlayingPosRestart=0;
    
    //if not radio mode, stop radio
    if (![(NSString*)[arrayFilepaths objectAtIndex:0] containsString:@"tmp/tmpRadio"]) {
        [radioSource stop];
        
        if (bShowRadio) [self showRadioPopup];
    }
    
    if ([array count]>=MAX_PL_ENTRIES) {
        NSString *msg_str=[NSString stringWithFormat:NSLocalizedString(@"Too much entries! Playlist will be limited to %d first entries.",@""),MAX_PL_ENTRIES];
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:msg_str];
        limitPl=1;
        //        return;
    }
    
    if (mPlaylist_size) {
        for (int i=0;i<mPlaylist_size;i++) {
            mPlaylist[i].mPlaylistFilename=nil;
            mPlaylist[i].mPlaylistFilepath=nil;
        }
    }
    
    mPlaylist_size=(limitPl?MAX_PL_ENTRIES:[array count]);
    for (int i=0;i<mPlaylist_size;i++) {
        mPlaylist[i].mPlaylistFilename=[[NSString alloc] initWithString:[array objectAtIndex:i]];
        mPlaylist[i].mPlaylistFilepath=[[NSString alloc] initWithString:[arrayFilepaths objectAtIndex:i]];
        
        mPlaylist[i].mPlaylistRating=-1;//rating;
        mPlaylist[i].mPlaylistCount=0;
        mPlaylist[i].cover_flag=-1;
    }
    
    mPlaylist_pos=index;
    if (mShuffle) {
        int i;
        int minval;
        minval=mPlaylist[0].mPlaylistCount;
        for (i=0;i<mPlaylist_size;i++) if (mPlaylist[i].mPlaylistCount<minval) minval=mPlaylist[i].mPlaylistCount;
        
        mPlaylist_pos=arc4random()%(mPlaylist_size);
        i=0;
        while ((i<mPlaylist_size)&&(mPlaylist[mPlaylist_pos].mPlaylistCount>minval)) {
            i++;
            mPlaylist_pos++; if (mPlaylist_pos>=mPlaylist_size) mPlaylist_pos=0;
        }
    }
    
    [self play_curEntry:-1];
    clearAudioFXbuffer=true;
    _seekRequested=-1;
    
    //    if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
    //        [self sendNotifPlayedTitle];
    //    }
    
    [self refreshCurrentVC];
}

-(void)play_listmodules:(t_playlist*)pl start_index:(int)index {
    int limitPl=0;
    mRestart=0;
    mRestart_sub=-1;
    mRestart_arc=0;
    mPlayingPosRestart=0;
    
    //if not radio mode, stop radio
    if (![pl->entries[0].fullpath containsString:@"tmp/tmpRadio"]) {
        [radioSource stop];
        if (bShowRadio) [self showRadioPopup];
    }
    
    if (pl->nb_entries>=MAX_PL_ENTRIES) {
        NSString *msg_str=[NSString stringWithFormat:NSLocalizedString(@"Too much entries! Playlist will be limited to %d first entries.",@""),MAX_PL_ENTRIES];
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:msg_str];
        limitPl=1;
        //        return;
    }
    
    if (mPlaylist_size) {
        for (int i=0;i<mPlaylist_size;i++) {
            mPlaylist[i].mPlaylistFilename=nil;
            mPlaylist[i].mPlaylistFilepath=nil;
        }
    }
    
    mPlaylist_size=(limitPl?MAX_PL_ENTRIES:pl->nb_entries);
    for (int i=0;i<mPlaylist_size;i++) {
        mPlaylist[i].mPlaylistFilename=[[NSString alloc] initWithString:pl->entries[i].label];
        mPlaylist[i].mPlaylistFilepath=[[NSString alloc] initWithString:pl->entries[i].fullpath];
        
        mPlaylist[i].mPlaylistRating=pl->entries[i].ratings;
        mPlaylist[i].mPlaylistCount=0;
        mPlaylist[i].cover_flag=-1;
    }
    
    mPlaylist_pos=(index>0?index:0);
    
    if (mShuffle&&(index<0)) {
        int i;
        int minval;
        minval=mPlaylist[0].mPlaylistCount;
        for (i=0;i<mPlaylist_size;i++) if (mPlaylist[i].mPlaylistCount<minval) minval=mPlaylist[i].mPlaylistCount;
        
        mPlaylist_pos=arc4random()%(mPlaylist_size);
        i=0;
        while ((i<mPlaylist_size)&&(mPlaylist[mPlaylist_pos].mPlaylistCount>minval)) {
            i++;
            mPlaylist_pos++; if (mPlaylist_pos>=mPlaylist_size) mPlaylist_pos=0;
        }
    }
    
    [self play_curEntry:-1];
    
    clearAudioFXbuffer=true;
    _seekRequested=-1;
    
    //    if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
    //        [self sendNotifPlayedTitle];
    //    }
    
    [self refreshCurrentVC];
}

-(void)play_restart {
    for (int i=0;i<mPlaylist_size;i++) {
        mPlaylist[i].mPlaylistCount=0;
    }
    //[playlistTabView reloadData];
    
    // Skip auto-restart if launched from a shortcut (shortcut will load its own playlist)
    // Check both UserDefaults and flag file
    BOOL launchedFromShortcut = [[NSUserDefaults standardUserDefaults] boolForKey:@"LaunchedFromShortcut"];
    
    if (launchedFromShortcut) {
        MDZILog("Skipping auto-restart - launched from Shortcut");
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"LaunchedFromShortcut"];
        [[NSUserDefaults standardUserDefaults] synchronize];
        mRestart=0;
        return;
    }
    
    //if (segcont_resumeLaunch.selectedSegmentIndex==0) return;
    if (mPlaylist_size>0) mRestart=1;
    else mRestart=0;
    
    if ([self play_curEntry:-1]) {
        //    self.tabBarController.selectedViewController = self; //detailViewController;
    }
}

-(int) add_to_playlist:(NSArray *)filePaths fileNames:(NSArray*)fileNames forcenoplay:(int)forcenoplay{
    int added_pos;
    int playLaunched=0;
    int add_entries_nb=[fileNames count];
    
    //if not radio mode, stop radio
    if ([radioSource isActive]) {
        [self stop];
        [self clearQueue];
        [radioSource stop];
        if (bShowRadio) [self showRadioPopup];
    }
    
    if (mPlaylist_size+add_entries_nb>=MAX_PL_ENTRIES) {
        NSString *msg_str=[NSString stringWithFormat:NSLocalizedString(@"Too much entries! Playlist will be limited to %d first entries.",@""),MAX_PL_ENTRIES];
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:msg_str];
        return 0;
    }
    
    if (mPlaylist_size) { //already in a playlist : append to it
        if (settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value==0) {
            
            for (int i=mPlaylist_size-1;i>=0;i--) {
                mPlaylist[i+add_entries_nb]=mPlaylist[i];
            }
            for (int i=0;i<add_entries_nb;i++) {
                mPlaylist[i].mPlaylistFilename=[[NSString alloc] initWithString:[fileNames objectAtIndex:i]];
                mPlaylist[i].mPlaylistFilepath=[[NSString alloc] initWithString:[filePaths objectAtIndex:i]];
                mPlaylist[i].mPlaylistRating=-1;
                mPlaylist[i].cover_flag=-1;
            }
            added_pos=0;
            mPlaylist_pos+=add_entries_nb;
        } else if ((settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value==1)&&(mPlaylist_pos<mPlaylist_size-1)) { //after current
            
            for (int i=mPlaylist_size-1;i>mPlaylist_pos;i--) {
                mPlaylist[i+add_entries_nb]=mPlaylist[i];
            }
            
            for (int i=0;i<add_entries_nb;i++) {
                mPlaylist[i+mPlaylist_pos+1].mPlaylistFilename=[[NSString alloc] initWithString:[fileNames objectAtIndex:i]];
                mPlaylist[i+mPlaylist_pos+1].mPlaylistFilepath=[[NSString alloc] initWithString:[filePaths objectAtIndex:i]];
                mPlaylist[i+mPlaylist_pos+1].mPlaylistRating=-1;
                mPlaylist[i+mPlaylist_pos+1].cover_flag=-1;
            }
            added_pos=mPlaylist_pos+1;
        } else { //last
            for (int i=0;i<add_entries_nb;i++) {
                mPlaylist[i+mPlaylist_size].mPlaylistFilename=[[NSString alloc] initWithString:[fileNames objectAtIndex:i]];
                mPlaylist[i+mPlaylist_size].mPlaylistFilepath=[[NSString alloc] initWithString:[filePaths objectAtIndex:i]];
                mPlaylist[i+mPlaylist_size].mPlaylistRating=-1;
                mPlaylist[i+mPlaylist_size].cover_flag=-1;
            }
            added_pos=mPlaylist_size;
        }
        mPlaylist_size+=add_entries_nb;
        //TODO To optimize
        for (int i=added_pos;i<add_entries_nb;i++) {
            mPlaylist[i].mPlaylistCount=0; //new entry
        }
        
        
        mShouldUpdateInfos=1;
    } else { //create a new playlist with downloaded song
        for (int i=0;i<add_entries_nb;i++) {
            mPlaylist[i].mPlaylistFilename=[[NSString alloc] initWithString:[fileNames objectAtIndex:i]];
            mPlaylist[i].mPlaylistFilepath=[[NSString alloc] initWithString:[filePaths objectAtIndex:i]];
            mPlaylist[i].mPlaylistCount=0;
            mPlaylist[i].mPlaylistRating=-1;
            mPlaylist[i].cover_flag=-1;
        }
        mPlaylist_size=add_entries_nb;
        mPlaylist_pos=0;
        added_pos=0;
        [self play_curEntry:-1];
        playLaunched=1;
    }
    if ((playLaunched==0)&&(!forcenoplay)&&(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==2)) {//Enqueue & play
        mPlaylist_pos=added_pos;
        [self play_curEntry:-1];
        playLaunched=1;
    }
    
    /*    NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
     if (mPlaylist_size) [self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
     [myindex autorelease];*/
    
    [self refreshCurrentVC];
    
    return playLaunched;
}


-(int) add_to_playlist:(NSString*)filePath fileName:(NSString*)fileName forcenoplay:(int)forcenoplay{
    int added_pos;
    int playLaunched=0;
    short int playcount=0;
    signed char rating=0;
    signed char avg_rating;
    
    
    //if not radio mode, stop radio
    if ([radioSource isActive]) {
        [self stop];
        [self clearQueue];
        [radioSource stop];
        if (bShowRadio) [self showRadioPopup];
    }
    
    
    if (mPlaylist_size>=MAX_PL_ENTRIES) {
        NSString *msg_str=[NSString stringWithFormat:NSLocalizedString(@"Too much entries! Playlist will be limited to %d first entries.",@""),MAX_PL_ENTRIES];
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:msg_str];
        return 0;
    }
    if (mPlaylist_size) { //already in a playlist : append to it
        if (settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value==0) {
            for (int i=mPlaylist_size-1;i>=0;i--) {
                mPlaylist[i+1]=mPlaylist[i];
            }
            mPlaylist[0].mPlaylistFilename=[[NSString alloc] initWithString:fileName];
            mPlaylist[0].mPlaylistFilepath=[[NSString alloc] initWithString:filePath];
            mPlaylist[0].cover_flag=-1;
            
            added_pos=0;
            mPlaylist_pos++;
            
        } else if ((settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value==1)&&(mPlaylist_pos<mPlaylist_size-1)) { //after current
            for (int i=mPlaylist_size-1;i>mPlaylist_pos;i--) {
                mPlaylist[i+1]=mPlaylist[i];
            }
            
            mPlaylist[mPlaylist_pos+1].mPlaylistFilename=[[NSString alloc] initWithString:fileName];
            mPlaylist[mPlaylist_pos+1].mPlaylistFilepath=[[NSString alloc] initWithString:filePath];
            mPlaylist[mPlaylist_pos+1].cover_flag=-1;
            added_pos=mPlaylist_pos+1;
        } else { //last
            mPlaylist[mPlaylist_size].mPlaylistFilename=[[NSString alloc] initWithString:fileName];
            mPlaylist[mPlaylist_size].mPlaylistFilepath=[[NSString alloc] initWithString:filePath];
            mPlaylist[mPlaylist_size].cover_flag=-1;
            added_pos=mPlaylist_size;
        }
        mPlaylist_size++;
        
        //TODO To optimize
        mPlaylist[added_pos].mPlaylistCount=0;
        rating=0;
        DBHelper::getFileStatsDBmod(mPlaylist[added_pos].mPlaylistFilepath,&playcount,&rating,&avg_rating);
        mPlaylist[added_pos].mPlaylistRating=rating;
        
        mShouldUpdateInfos=1;
    } else { //create a new playlist with downloaded song
        mPlaylist[0].mPlaylistFilename=[[NSString alloc] initWithString:fileName];
        mPlaylist[0].mPlaylistFilepath=[[NSString alloc] initWithString:filePath];
        mPlaylist[0].cover_flag=-1;
        added_pos=0;
        mPlaylist_size=1;
        mPlaylist[added_pos].mPlaylistCount=0;
        rating=0;
        DBHelper::getFileStatsDBmod(mPlaylist[added_pos].mPlaylistFilepath,&playcount,&rating,&avg_rating);
        mPlaylist[added_pos].mPlaylistRating=rating;
        
        mPlaylist_pos=0;
        
        [self play_curEntry:-1];
        playLaunched=1;
        clearAudioFXbuffer=true;
        _seekRequested=-1;
        
        //        if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
        //            [self sendNotifPlayedTitle];
        //        }
    }
    if ((!forcenoplay)&&(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==2)) {//Enqueue & play
        mPlaylist_pos=added_pos;
        [self play_curEntry:-1];
        playLaunched=1;
        clearAudioFXbuffer=true;
        _seekRequested=-1;
        
        //        if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
        //            [self sendNotifPlayedTitle];
        //        }
    }
    
    [self refreshCurrentVC];
    
    //NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
    /*    if (mPlaylist_size) [self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];*/
    //[myindex autorelease];
    return playLaunched;
}

-(void) remove_from_playlist:(int)index {//remove entry
    if (mPlaylist_size) { //ensure playlist is not empty
        //[mPlaylist[index].mPlaylistFilename autorelease];
        //[mPlaylist[index].mPlaylistFilepath autorelease];
        
        for (int i=index;i<mPlaylist_size-1;i++) {
            mPlaylist[i]=mPlaylist[i+1];
        }
        
        mPlaylist_size--;
        if ((index<mPlaylist_pos)||(mPlaylist_pos==mPlaylist_size)) mPlaylist_pos--;
        
        //playlistPos.text=[NSString stringWithFormat:@"%d on %d",mPlaylist_pos,mPlaylist_size];
        mShouldUpdateInfos=1;
    }
    
    if (mPlaylist_size) {
        /*        NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
         [self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
         [myindex autorelease];*/
    }
}

-(NSString*) getCurrentModuleFilepath {
    if (mPlaylist_size==0) return nil;
    return mPlaylist[mPlaylist_pos].mPlaylistFilepath;
}



-(BOOL) play_loadArchiveModule {
    short int playcount=0;
    char signed avg_rating;
    int retcode;
    NSString *fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
    NSString *filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
    
    if (!filePath) return FALSE;
    
    if (settings[GLOB_ResumeOnStart].detail.mdz_boolswitch.switch_value==0) mPlayingPosRestart=0;
    
    
    mOnlyCurrentEntry=0;
    mOnlyCurrentSubEntry=0;
    //mSendStatTimer=0;
    
    // if already playing, stop
    if (repeatingTimer) {
        [repeatingTimer invalidate];
        repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
        [mplayer Stop];
    }
    mShouldUpdateInfos=1;
    
    
    //ensure any settings changes to be taken into account before loading next file
    //[self settingsChanged:(int)SETTINGS_ALL];
    
    if (mShuffle==1) {
        mOnlyCurrentSubEntry|=2;
        mOnlyCurrentEntry|=2;
    }
    // load module
    
    if ((retcode=[mplayer LoadModule:filePath archiveMode:1 archiveIndex:-1 singleSubMode:mOnlyCurrentSubEntry  singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart shuffle:mShuffle])) {
        //error while loading
        
        if ( [mplayer isArchive] &&
            !mOnlyCurrentSubEntry &&
            !mOnlyCurrentEntry ) {
            
            do {
                if ([mplayer selectNextArcEntry]<0) break;
                
                mRestart_arc=[mplayer getArcIndex];
                retcode=[mplayer LoadModule:filePath archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart shuffle:mShuffle];
            } while (retcode!=0);
        }
        
        if (retcode) {
            MDZELog("Issue in LoadModule(archive) %@",filePath);
            if (retcode==-99) mLoadIssueMessage=0;
            else mLoadIssueMessage=1;
            return FALSE;
        }
    }
    
    
    if (mShuffle==1) {
        if ([mplayer isArchive]) {
            [mplayer Stop]; //deallocate relevant items
            mRestart_arc=arc4random()%[mplayer getArcEntriesCnt];
            
            if ((retcode=[mplayer LoadModule:filePath archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry  singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart shuffle:mShuffle])) {
                //error while loading
                if ( [mplayer isArchive] &&
                    !mOnlyCurrentSubEntry &&
                    !mOnlyCurrentEntry ) {
                    
                    do {
                        if ([mplayer selectNextArcEntry]<0) break;
                        mRestart_arc=[mplayer getArcIndex];
                        retcode=[mplayer LoadModule:filePath archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart shuffle:mShuffle];
                    } while (retcode);
                    
                }
                
                if (retcode) {
                    MDZELog("Issue in LoadModule(archive) %@",filePath);
                    if (retcode==-99) mLoadIssueMessage=0;
                    else mLoadIssueMessage=2;
                    return FALSE;
                }
            }
            
            //            [self hideWaiting];
        }
    }
    
    //fix issue with OMPT settings reset after load
    //[self settingsChanged:(int)SETTINGS_ALL];
    
    [self checkForCover:filePath];
    
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
    
    if ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0)) btnShowSubSong.hidden=false;
    else btnShowSubSong.hidden=true;
    if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0)) btnShowArcList.hidden=false;
    else btnShowArcList.hidden=true;
    if ([mplayer isVoicesMutingSupported]) btnShowVoices.hidden=false;
    else btnShowVoices.hidden=true;
    
    
    alertCannotPlay_displayed=0;
    //Visualization stuff
    [self reinitVisuVars];
    
    //Is OGLView visible ?
    [self checkGLViewCanDisplay];
    
    //update playback buttons
    self.pauseBarSub.hidden=YES;
    self.playBarSub.hidden=YES;
    self.pauseBar.hidden=YES;
    self.playBar.hidden=YES;
    if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.pauseBarSub.hidden=NO;
    else self.pauseBar.hidden=NO;
    [self updateBarPos];
    
    
    //random mode & mutli song ?
    if (mShuffle) {
        if ([mplayer isMultiSongs]) {
            if (mShuffle==1) mOnlyCurrentSubEntry=1;
            mRestart_sub=arc4random()%(mplayer.mod_subsongs)+mplayer.mod_minsub;
        }
        [mplayer PlaySeek:mPlayingPosRestart subsong:(mRestart_sub>=0?mRestart_sub:mplayer.mod_currentsub)];
    } else {
        [mplayer PlaySeek:mPlayingPosRestart subsong:mplayer.mod_currentsub];
    }
    sliderProgressModule.value=0;
    mIsPlaying=YES;
    mPaused=0;
    
    //Update song info if required
    labelModuleName.hidden=NO;
    labelArtist.text=mplayer.artist;
    
    if (labelArtist.text && [labelArtist.text length]) {
        labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,22);
        labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,18);
    } else {
        labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,40);
        labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,0);
    }

    
    if (settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value) {
        labelModuleName.text=[NSString stringWithFormat:@"%@ /%@",fileName,[mplayer getModName]];
    } else {
        if ([mplayer getModFileTitleOrNull]) labelModuleName.text=[NSString stringWithFormat:@"%@ /%@",[mplayer getModFileTitle],[mplayer getModName]];
        else labelModuleName.text=[NSString stringWithFormat:@"%@",[mplayer getModName]];
    }
    [self refreshFXFSLabels];
    
    labelModuleSize.text=[NSString stringWithFormat:NSLocalizedString(@"Size: %dKB",@""), mplayer.mp_datasize>>10];
    if ([mplayer getSongLength]>0) {
        if (display_length_mode) labelTime.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getSongLength]/1000)/60,([mplayer getSongLength]/1000)%60];
        sliderProgressModule.enabled=YES;
        labelModuleLength.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getSongLength]/1000)/60,([mplayer getSongLength]/1000)%60];
    } else {
        if (display_length_mode) display_length_mode=0;
        sliderProgressModule.enabled=FALSE;
        labelModuleLength.text=@"--:--";
    }
    
    //Update current entry stats
    //increase playcount by one if not restarting
    signed char tmp_rating;
    DBHelper::getFileStatsDBmod([NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],&playcount,&tmp_rating,&avg_rating);
    if (!mRestart) playcount++;
    mPlaylist[mPlaylist_pos].mPlaylistRating=tmp_rating;
    DBHelper::updateFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],playcount,tmp_rating,avg_rating,-1,-1,-1);
    
    [self showRating:tmp_rating];
    
    //set volume (if applicable)
    [mplayer optOMPT_MasterVol:settings[OMPT_MasterVolume].detail.mdz_slider.slider_value];
    
    
    labelTime.text=@"00:00";
    if (mplayer.numChannels) {
        if (mplayer.numChannels==1) labelNumChannels.text=[NSString stringWithFormat:@"1 channel"];
        else labelNumChannels.text=[NSString stringWithFormat:@"%d channels",mplayer.numChannels];
    } else labelNumChannels.text=[NSString stringWithFormat:@""];
    
    labelModuleType.text=[NSString stringWithFormat:@"Format: %@",[mplayer getModType]];
    labelLibName.text=[NSString stringWithFormat:@"Player: %@",[mplayer getPlayerName]];
    textMessage.text=[NSString stringWithFormat:@"\n%@",[mplayer getModMessage]];
    
    [textMessage scrollRangeToVisible:NSMakeRange(0, 1)];
    
    
    [self updMediaCenter];
    
    //Activate timer for play infos
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.20f target:self selector:@selector(updateInfos:) userInfo:nil repeats: YES]; //5 times/second
    
    if (nowplayingPL) {
        NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
        [nowplayingPL.tableView reloadData];
        nowplayingPL.currentPlayedEntry=mPlaylist_pos;
        [nowplayingPL.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos+1] animated:YES scrollPosition:UITableViewScrollPositionMiddle];
    }
    
    clearAudioFXbuffer=true;
    _seekRequested=-1;
    
    if (settings[GLOB_Notification].detail.mdz_switch.switch_value==2) {
        [self sendNotifPlayedTitle];
    }
    return TRUE;
}

- (void)titleTap:(UITapGestureRecognizer *)sender {
    [self openPopup:labelModuleName.text secmsg:mPlaylist[mPlaylist_pos].mPlaylistFilepath style:POPUP_STYLE_INFO];
}

//for archive cover
-(NSString*) getFullFilePath:(NSString *)_filePath {
    NSString *fullFilePath;
    
    if ([_filePath length]>2) {
        if (([_filePath characterAtIndex:0]=='/')&&([_filePath characterAtIndex:1]=='/')) fullFilePath=[_filePath substringFromIndex:2];
        else fullFilePath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:_filePath];
    } else fullFilePath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:_filePath];
    return fullFilePath;
}

-(void) checkForCover:(NSString *)filePath {
    NSString *pathFolderImgPNG,*pathCoverImgPNG,*pathFileImgPNG,
    *pathFolderImgJPG,*pathCoverImgJPG,*pathFolderImgJPEG,
    *pathCoverImgJPEG,*pathFileImgJPG,*pathFileImgJPEG,
    *pathFolderImgGIF,*pathCoverImgGIF,*pathFileImgGIF,
    *pathFolderImgWEBP,*pathCoverImgWEBP,*pathFileImgWEBP,
    *pathFileImgPIC,*pathFileImgPGG,*pathFileImgPJJ;
    
    bool partialPath=false;
    if ([[filePath substringToIndex:[@"Documents/" length]] isEqualToString:@"Documents/"]) {
        partialPath=true;
    }
    
    if (partialPath) {
        pathFolderImgPNG=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@/folder.png",[filePath stringByDeletingLastPathComponent]];
        pathFolderImgJPG=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@/folder.jpg",[filePath stringByDeletingLastPathComponent]];
        pathFolderImgJPEG=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@/folder.jpeg",[filePath stringByDeletingLastPathComponent]];
        pathFolderImgGIF=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@/folder.gif",[filePath stringByDeletingLastPathComponent]];
        pathFolderImgWEBP=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@/folder.webp",[filePath stringByDeletingLastPathComponent]];
        pathCoverImgPNG=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@/cover.png",[filePath stringByDeletingLastPathComponent]];
        pathCoverImgJPG=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@/cover.jpg",[filePath stringByDeletingLastPathComponent]];
        pathCoverImgJPEG=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@/cover.jpeg",[filePath stringByDeletingLastPathComponent]];
        pathCoverImgGIF=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@/cover.gif",[filePath stringByDeletingLastPathComponent]];
        pathCoverImgWEBP=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@/cover.webp",[filePath stringByDeletingLastPathComponent]];
        pathFileImgPNG=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@.png",[filePath stringByDeletingPathExtension]];
        pathFileImgJPG=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@.jpg",[filePath stringByDeletingPathExtension]];
        pathFileImgJPEG=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@.jpeg",[filePath stringByDeletingPathExtension]];
        pathFileImgGIF=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@.gif",[filePath stringByDeletingPathExtension]];
        pathFileImgWEBP=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@.webp",[filePath stringByDeletingPathExtension]];
        pathFileImgPIC=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@.pic",[filePath stringByDeletingPathExtension]];
        pathFileImgPGG=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@.pgg",[filePath stringByDeletingPathExtension]];
        pathFileImgPJJ=[[ModizFileHelper getAppHomeDirectory] stringByAppendingFormat:@"/%@.pjj",[filePath stringByDeletingPathExtension]];
    } else {
        pathFolderImgPNG=[NSString stringWithFormat:@"%@/folder.png",[filePath stringByDeletingLastPathComponent]];
        pathFolderImgJPG=[NSString stringWithFormat:@"%@/folder.jpg",[filePath stringByDeletingLastPathComponent]];
        pathFolderImgJPEG=[NSString stringWithFormat:@"%@/folder.jpeg",[filePath stringByDeletingLastPathComponent]];
        pathFolderImgGIF=[NSString stringWithFormat:@"%@/folder.gif",[filePath stringByDeletingLastPathComponent]];
        pathFolderImgWEBP=[NSString stringWithFormat:@"%@/folder.webp",[filePath stringByDeletingLastPathComponent]];
        pathCoverImgPNG=[NSString stringWithFormat:@"%@/cover.png",[filePath stringByDeletingLastPathComponent]];
        pathCoverImgJPG=[NSString stringWithFormat:@"%@/cover.jpg",[filePath stringByDeletingLastPathComponent]];
        pathCoverImgJPEG=[NSString stringWithFormat:@"%@/cover.jpeg",[filePath stringByDeletingLastPathComponent]];
        pathCoverImgGIF=[NSString stringWithFormat:@"%@/cover.gif",[filePath stringByDeletingLastPathComponent]];
        pathCoverImgWEBP=[NSString stringWithFormat:@"%@/cover.webp",[filePath stringByDeletingLastPathComponent]];
        pathFileImgPNG=[NSString stringWithFormat:@"%@.png",[filePath stringByDeletingPathExtension]];
        pathFileImgJPG=[NSString stringWithFormat:@"%@.jpg",[filePath stringByDeletingPathExtension]];
        pathFileImgJPEG=[NSString stringWithFormat:@"%@.jpeg",[filePath stringByDeletingPathExtension]];
        pathFileImgGIF=[NSString stringWithFormat:@"%@.gif",[filePath stringByDeletingPathExtension]];
        pathFileImgWEBP=[NSString stringWithFormat:@"%@.webp",[filePath stringByDeletingPathExtension]];
        pathFileImgPIC=[NSString stringWithFormat:@"%@.pic",[filePath stringByDeletingPathExtension]];
        pathFileImgPGG=[NSString stringWithFormat:@"%@.pgg",[filePath stringByDeletingPathExtension]];
        pathFileImgPJJ=[NSString stringWithFormat:@"%@.pjj",[filePath stringByDeletingPathExtension]];
    }
    
    coverAvailable=false;
    cover_img=nil;
    
    //    cover_img=[UIImage imageWithData:[NSData dataWithContentsOfFile:pathFolderImgPNG]];
    if (gifAnimation) [gifAnimation removeFromSuperview];
    gifAnimation=nil;
    
    if ([mplayer artworkImage]) {
        cover_img=[mplayer artworkImage];
    }
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFileImgJPG]) cover_img=[UIImage imageWithContentsOfFile:pathFileImgJPG];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFileImgJPEG]) cover_img=[UIImage imageWithContentsOfFile:pathFileImgJPEG];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFileImgPNG]) cover_img=[UIImage imageWithContentsOfFile:pathFileImgPNG];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFileImgWEBP]) cover_img=[UIImage imageWithContentsOfFile:pathFileImgWEBP];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFileImgGIF]) {
        cover_img=[UIImage imageWithContentsOfFile:pathFileImgGIF];
        if (cover_img) {
            NSURL* firstUrl = [NSURL fileURLWithPath:pathFileImgGIF];
            gifAnimation = [AnimatedGif getAnimationForGifAtUrl: firstUrl];
            
            gifAnimation.frame=CGRectMake(0, 0,cover_view.frame.size.width,cover_view.frame.size.height);
            [gifAnimation layoutSubviews];
            [cover_view addSubview:gifAnimation];
        }
    }
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFileImgPIC]) {
        NSData *picData = [NSData dataWithContentsOfFile:pathFileImgPIC];
        if (picData) {
            UIImage *img = [C64PICDecoder imageFromPICData:picData];
            if (img) cover_img = img;
        }
    }
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFileImgPGG]) {
        NSData *picData = [NSData dataWithContentsOfFile:pathFileImgPGG];
        if (picData) {
            UIImage *img = [C64PGGDecoder imageFromPGGData:picData];
            if (img) cover_img = img;
        }
    }
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFileImgPJJ]) {
        NSData *picData = [NSData dataWithContentsOfFile:pathFileImgPJJ];
        if (picData) {
            UIImage *img = [C64PJJDecoder imageFromPJJData:picData];
            if (img) cover_img = img;
        }
    }
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFolderImgJPG]) cover_img=[UIImage imageWithContentsOfFile:pathFolderImgJPG];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathCoverImgJPG]) cover_img=[UIImage imageWithContentsOfFile:pathCoverImgJPG];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFolderImgJPEG]) cover_img=[UIImage imageWithContentsOfFile:pathFolderImgJPEG];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathCoverImgJPEG]) cover_img=[UIImage imageWithContentsOfFile:pathCoverImgJPEG];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFolderImgPNG]) cover_img=[UIImage imageWithContentsOfFile:pathFolderImgPNG];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathCoverImgPNG]) cover_img=[UIImage imageWithContentsOfFile:pathCoverImgPNG];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFolderImgWEBP]) cover_img=[UIImage imageWithContentsOfFile:pathFolderImgWEBP];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathCoverImgWEBP]) cover_img=[UIImage imageWithContentsOfFile:pathCoverImgWEBP];
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathFolderImgGIF]) {
        cover_img=[UIImage imageWithContentsOfFile:pathFolderImgGIF];
        if (cover_img) {
            NSURL* firstUrl = [NSURL fileURLWithPath:pathFolderImgGIF];
            gifAnimation= [AnimatedGif getAnimationForGifAtUrl: firstUrl];
            
            gifAnimation.frame=CGRectMake(0, 0,cover_view.frame.size.width,cover_view.frame.size.height);
            [gifAnimation layoutSubviews];
            [cover_view addSubview:gifAnimation];
        }
    }
    if ((cover_img==nil)&&[[NSFileManager defaultManager] fileExistsAtPath:pathCoverImgGIF]) {
        cover_img=[UIImage imageWithContentsOfFile:pathCoverImgGIF];
        if (cover_img) {
            NSURL* firstUrl = [NSURL fileURLWithPath:pathCoverImgGIF];
            gifAnimation= [AnimatedGif getAnimationForGifAtUrl: firstUrl];
            
            gifAnimation.frame=CGRectMake(0, 0,cover_view.frame.size.width,cover_view.frame.size.height);
            [gifAnimation layoutSubviews];
            [cover_view addSubview:gifAnimation];
        }
    }
    
    
    if ((cover_img==nil)&&[mplayer isArchive]) {//archive mode, check tmp folder
        NSString *file,*cpath;
        NSURL *fileURL;
        NSArray *filetype_ext=[SUPPORTED_FILETYPE_COVER componentsSeparatedByString:@","];
        NSFileManager *fileMngr=[[NSFileManager alloc] init];
        
        BOOL isDir;
        NSArray *dirContent;
        
        cpath=[NSString stringWithFormat:@"%@/tmpArchive",NSTemporaryDirectory()];
        
        //List all entries
        NSURL *directoryURL = [NSURL fileURLWithPath:cpath];
        NSDirectoryEnumerator *directoryEnumerator =
        [fileMngr enumeratorAtURL:directoryURL
       includingPropertiesForKeys:@[NSURLPathKey, NSURLNameKey, NSURLIsDirectoryKey]
                          options:NSDirectoryEnumerationSkipsHiddenFiles
                     errorHandler:nil];
        
        /*for (NSURL *fileURL in directoryEnumerator) {
         [dirContent addObject:fileURL];
         }*/
        dirContent=[directoryEnumerator allObjects];
        
        NSArray *sortedDirContent = [dirContent sortedArrayUsingComparator:^(id obj1, id obj2) {
            
            NSString *str1;//[(NSString *)obj1 lastPathComponent];
            NSString *str2;//[(NSString *)obj2 lastPathComponent];
            
            
            [(NSURL*)obj1 getResourceValue:&str1 forKey:NSURLPathKey error:nil];
            [(NSURL*)obj2 getResourceValue:&str2 forKey:NSURLPathKey error:nil];
            
            return [str1 caseInsensitiveCompare:str2];
        }];
        
        int file_idx=0;
        int file_cnt=[sortedDirContent count];
        for (fileURL in sortedDirContent) {
            
            NSNumber *isDirectory = nil;
            [fileURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
            [fileURL getResourceValue:&file forKey:NSURLPathKey error:nil];
            
            isDir=[isDirectory boolValue];
            
            if (!isDir) {
                NSString *extension = [[file pathExtension] uppercaseString];
                
                if ([filetype_ext indexOfObject:extension]!=NSNotFound) {
                    cover_img=[UIImage imageWithContentsOfFile:file];
                    break;
                }
            }
        }
        //[fileMngr release];
    }
    
    if (cover_img==nil) {
        cover_img=[self getArchiveCover:[self getFullFilePath:filePath]];
    }
    
    if (cover_img==nil) {
        cover_img=default_cover;
    } else coverAvailable=true;
    
    if (cover_img) {
        if (mScaleFactor!=1) cover_img = [[UIImage alloc] initWithCGImage:cover_img.CGImage scale:mScaleFactor orientation:UIImageOrientationUp];
        
        cover_view.image=cover_img;
        //cover_view.hidden=FALSE;
        
        cover_viewBG.image=[cover_img applyLightEffect];
        //cover_viewBG.hidden=FALSE;
        cover_viewAll.hidden=FALSE;
    } else {
        //cover_view.hidden=TRUE;
        //cover_viewBG.hidden=TRUE;
        cover_viewAll.hidden=TRUE;
    }
    
    shouldUpdateCoverTexture=1;
}

-(void) cancelPushed {
    if (_seekRequested>=0) {
        _seekRequested=[mplayer getCurrentSamplesPos];
    }
    if (loadRequestInProgress) {
        mplayer.extractPendingCancel=true;
        [waitingView hideCancel];
        [waitingView resetCancelStatus];
        [waitingView hideProgress];
        [waitingView setDetail:NSLocalizedString(@"Cancelling...",@"")];
    }
}

-(void) loadNewFileFailed:(NSString *)filePath fname:(NSString *)fileName arcidx:(int)arcidx subsong:(int)subsong {
    MDZELog("load failed: %@ %@ %d %d",filePath,fileName,arcidx,subsong);
    loadRequestInProgress=0;
    
    UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
    //mdz_safe_execute_sel(vc,@selector(hideWaiting),nil)
    [self hideWaiting];
    
    [self remove_from_playlist:mPlaylist_pos];
    
    [self hideWaiting];
    [self refreshCurrentVC];
    ///////////////////////////////////////////////////
    // Update miniplayer
    ///////////////////////////////////////////////////
    vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
    mdz_safe_execute_sel(vc,@selector(updateMiniPlayer),nil)
    
    if ((alertCannotPlay_displayed==0)&&(mLoadIssueMessage)) {
        NSString *alertMsg;
        alertCannotPlay_displayed=1;
        [self openPopup:NSLocalizedString(@"File cannot be played. Skipping to next playable file.",@"") secmsg:[NSString stringWithFormat:@"%s",mplayer_error_msg] style:POPUP_STYLE_ALERT];
        
        [self play_curEntry:-1];
        
    } else {
        
        [self play_curEntry:-1];
    }
}

-(void) loadNewFileCompleted:(NSString *)filePath fname:(NSString *)fileName arcidx:(int)arcidx subsong:(int)subsong {
#if DEBUG_MODIZER
    MDZDLog("load completed: %@ %@ %d %d",filePath,fileName,arcidx,subsong);
#endif
    //UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
    //mdz_safe_execute_sel(vc,@selector(hideWaiting),nil)
    [self hideWaiting];
    
    loadRequestInProgress=0;
    
    //fix issue with OMPT settings reset after load
    //[self settingsChanged:(int)SETTINGS_ALL];
    
    [self checkForCover:filePath];
    
    mPlaylist[mPlaylist_pos].cover_flag=-1;
    
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
    
    if ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0)) btnShowSubSong.hidden=false;
    else btnShowSubSong.hidden=true;
    if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0)) btnShowArcList.hidden=false;
    else btnShowArcList.hidden=true;
    if ([mplayer isVoicesMutingSupported]) btnShowVoices.hidden=false;
    else btnShowVoices.hidden=true;
    
    alertCannotPlay_displayed=0;
    //Visualization stuff
    [self reinitVisuVars];
    
    //Is OGLView visible ?
    [self checkGLViewCanDisplay];
    
    //Restart management
    if (mRestart) {
        //mRestart=0;
        self.pauseBarSub.hidden=YES;
        self.playBarSub.hidden=YES;
        self.pauseBar.hidden=YES;
        self.playBar.hidden=YES;
        
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.playBarSub.hidden=NO;
        else self.playBar.hidden=NO;
        
        [self updateBarPos];
        [mplayer PlaySeek:mPlayingPosRestart subsong:(mRestart_sub>=0?mRestart_sub:mplayer.mod_currentsub)];
        if (mPlayingPosRestart) {
            mPlayingPosRestart=0;
        } else sliderProgressModule.value=0;
        [mplayer Pause:YES];
        mIsPlaying=YES;
        mPaused=1;
    } else {
        //random mode & multi song ?
        if ((mShuffle==1)&&(subsong<0)) {
            /*            if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)) {
             mOnlyCurrentSubEntry=1;
             }*/
            if ([mplayer isMultiSongs]) {
                mOnlyCurrentSubEntry|=2;
                mRestart_sub=arc4random()%(mplayer.mod_subsongs)+mplayer.mod_minsub;
            }
        } else if (mShuffle==2) {
            if (!(mOnlyCurrentSubEntry&1))
                if ([mplayer isMultiSongs]) mRestart_sub=arc4random()%(mplayer.mod_subsongs)+mplayer.mod_minsub;
        }
        self.pauseBarSub.hidden=YES;
        self.playBarSub.hidden=YES;
        self.pauseBar.hidden=YES;
        self.playBar.hidden=YES;
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.pauseBarSub.hidden=NO;
        else self.pauseBar.hidden=NO;
        [self updateBarPos];
        //mRestart=0;
        [mplayer PlaySeek:mPlayingPosRestart subsong:(mRestart_sub>=0?mRestart_sub:mplayer.mod_currentsub)];
        if (mPlayingPosRestart) {
            mPlayingPosRestart=0;
        } else sliderProgressModule.value=0;
        mIsPlaying=YES;
        mPaused=0;
        
    }
    
    mRestart_sub=0;
    mRestart_arc=0;
    //set volume (if applicable)
    [mplayer optOMPT_MasterVol:settings[OMPT_MasterVolume].detail.mdz_slider.slider_value];
    
    //Update song info if required
    labelArtist.text=mplayer.artist;
    
    if (labelArtist.text && [labelArtist.text length]) {
        labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,22);
        labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,18);
    } else {
        labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,40);
        labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,0);
    }

    
    labelModuleName.hidden=NO;
    if (settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value) labelModuleName.text=[NSString stringWithString:fileName];
    else {
        if ([mplayer getModFileTitleOrNull]) labelModuleName.text=[NSString stringWithFormat:@"%@ /%@",[mplayer getModFileTitle],[mplayer getModName]];
        else labelModuleName.text=[NSString stringWithFormat:@"%@",[mplayer getModName]];
    }
    
    [self refreshFXFSLabels];
    
    labelModuleSize.text=[NSString stringWithFormat:NSLocalizedString(@"Size: %dKB",@""), mplayer.mp_datasize>>10];
    if ([mplayer getSongLength]>0) {
        if (display_length_mode) labelTime.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getSongLength]/1000)/60,([mplayer getSongLength]/1000)%60];
        sliderProgressModule.enabled=YES;
        labelModuleLength.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getSongLength]/1000)/60,([mplayer getSongLength]/1000)%60];
    } else {
        if (display_length_mode) display_length_mode=0;
        sliderProgressModule.enabled=FALSE;
        labelModuleLength.text=@"--:--";
    }
    
    [self updateStats:fileName filePath:filePath playcount_inc:(mRestart==0)];
    if (!mRestart) {//UPDATE Google Application
        if (settings[GLOB_StatsUpload].detail.mdz_boolswitch.switch_value) mSendStatTimer=5*10;//Wait 5s
        
    }
    [self showRating:[self getCurrentRating]];
    
    labelTime.text=@"00:00";
    if (mplayer.numChannels) {
        if (mplayer.numChannels==1) labelNumChannels.text=[NSString stringWithFormat:@"1 channel"];
        else labelNumChannels.text=[NSString stringWithFormat:@"%d channels",mplayer.numChannels];
    } else labelNumChannels.text=[NSString stringWithFormat:@""];
    
    labelModuleType.text=[NSString stringWithFormat:@"Format: %@",[mplayer getModType]];
    labelLibName.text=[NSString stringWithFormat:@"Player: %@",[mplayer getPlayerName]];
    textMessage.text=[NSString stringWithFormat:@"\n%@",[mplayer getModMessage]];
    
    [textMessage scrollRangeToVisible:NSMakeRange(0, 1)];
    
    
    //    MPNowPlayingInfoCenter *infoCenter = [MPNowPlayingInfoCenter defaultCenter];
    //
    //
    if (cover_img) artwork=[[MPMediaItemArtwork alloc] initWithImage:cover_img];
    else artwork=[[MPMediaItemArtwork alloc] initWithImage:default_cover];
    //
    updMPNowCnt=0;
    
    //Activate timer for play infos
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.1f target:self selector:@selector(updateInfos:) userInfo:nil repeats: YES]; //10 times/second
    
    if (nowplayingPL) {
        NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
        [nowplayingPL.tableView reloadData];
        nowplayingPL.currentPlayedEntry=mPlaylist_pos;
        [nowplayingPL.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos+1] animated:YES scrollPosition:UITableViewScrollPositionMiddle];
    }
    mRestart=0;
    
    clearAudioFXbuffer=true;
    _seekRequested=-1;
    
    if (mPaused==false) {
        if (settings[GLOB_Notification].detail.mdz_switch.switch_value>0) {
            [self sendNotifPlayedTitle];
        }
    }
}

-(int) requestLoadNewFile:(NSString *)filePath fname:(NSString *)fileName arcidx:(int)arcidx subsong:(int)subsong {
    int arcidx_filepath=-1;
    int subsong_filepath=-1;
    NSString *filePathClean=[ModizFileHelper getFullCleanFilePath:filePath arcidx_ptr:&arcidx_filepath subsong_ptr:&subsong_filepath];
    if (filePathClean==nil) return -1;
    
    _seekRequested=-1;
    
    mOnlyCurrentSubEntry=0;
    mOnlyCurrentEntry=0;
    if (arcidx_filepath>=0) {
        arcidx=arcidx_filepath;
        if (settings[GLOB_ArcMultiPlayMode].detail.mdz_switch.switch_value==0) mOnlyCurrentEntry|=1;
        
    }
    if (subsong_filepath>=0) {
        subsong=subsong_filepath;
        if (settings[GLOB_ArcMultiPlayMode].detail.mdz_switch.switch_value==0) mOnlyCurrentSubEntry|=1;
    }
    filePath=filePathClean;
    
    if (mRestart) {
        arcidx=mRestart_arc;
        subsong=mRestart_sub;
    } else {
        mRestart_arc=arcidx;
        mRestart_sub=subsong;
    }
    
    //UIViewController *vc = [self visibleViewController:[UIApplication sharedApplication].keyWindow.rootViewController];
    //mdz_safe_execute_sel(vc,@selector(showWaitingProgress),nil)
    //mdz_safe_execute_sel(vc,@selector(showWaitingLoading),nil)
    [self showWaitingProgress];
    [self showWaitingLoading];
    
    mSendStatTimer=0;
    shouldRestart=0;
    
    if (loadRequestInProgress) return -3; //no reentrant
    if (!filePath) return -1;
    if (!fileName) return -1;
    
    loadRequestInProgress=1;
    
    if (settings[GLOB_ResumeOnStart].detail.mdz_boolswitch.switch_value==0) mPlayingPosRestart=0;
    
    // if already playing, stop
    if (repeatingTimer) {
        [repeatingTimer invalidate];
        repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
        [mplayer Stop];
    }
    mShouldUpdateInfos=1;
    // load module
    
    //ensure any settings changes to be taken into account before loading next file
    //[self settingsChanged:(int)SETTINGS_ALL];
    
    if (mShuffle==1) {
        mOnlyCurrentSubEntry|=2;
        mOnlyCurrentEntry|=2;
    }
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        int retcode;
        if ((retcode=[mplayer LoadModule:filePath archiveMode:0 archiveIndex:arcidx singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart  shuffle:mShuffle])) {
            
            //error while loading
            //if it is an archive, try to load next entry until end or valid one reached
            if ( [mplayer isArchive] &&
                !mOnlyCurrentSubEntry &&
                !mOnlyCurrentEntry ) {
                
                do {
                    if ([mplayer selectNextArcEntry]<0) break;
                    mRestart_arc=[mplayer getArcIndex];
                    retcode=[mplayer LoadModule:filePath archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart shuffle:mShuffle];
                    if ([mplayer getArcIndex]>=[mplayer getArcEntriesCnt]-1) break;
                } while (retcode);
            }
            
            if (retcode) {
                //Wasn't able to load, fail
                MDZELog("Issue in LoadModule %@",filePath);
                mRestart=0;
                mRestart_sub=0;
                mRestart_arc=0;
                if (mplayer_error_msg[0]==0) snprintf(mplayer_error_msg,sizeof(mplayer_error_msg),"%s",[filePath UTF8String]);
                if (retcode==-99) mLoadIssueMessage=0;
                else mLoadIssueMessage=3;
                
                [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                    [self loadNewFileFailed:filePath fname:fileName arcidx:arcidx subsong:subsong];
                }];
                return;
            }
        }
        
        //loaded successfully, check if shuffle is active
        if ((mShuffle==1)&&[mplayer isArchive]) {
            [mplayer Stop]; //deallocate relevant items
            
            if (!(mOnlyCurrentEntry&1)) mRestart_arc=arc4random()%[mplayer getArcEntriesCnt]; //do not shuffle if arc entry was part of filename
            
            if ((retcode=[mplayer LoadModule:filePath archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry  singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart shuffle:mShuffle])) {
                //error while loading
                
                do {
                    if ([mplayer selectNextArcEntry]<0) {
                        retcode=-1;
                        break;
                    } else {
                        mRestart_arc=[mplayer getArcIndex];
                        retcode=[mplayer LoadModule:filePath archiveMode:1 archiveIndex:arcidx singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart shuffle:mShuffle];
                    }
                } while (retcode);
                
                if (retcode) {
                    MDZELog("Issue in LoadModule(archive) %@",filePath);
                    if (retcode==-99) mLoadIssueMessage=0;
                    else mLoadIssueMessage=4;
                    
                    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                        [self loadNewFileFailed:filePath fname:fileName arcidx:arcidx subsong:subsong];
                    }];
                    return;
                }
            }
        }
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            [self loadNewFileCompleted:filePath fname:fileName arcidx:arcidx subsong:subsong];
        }];
    });
    
}



-(BOOL)play_module:(NSString *)filePath fname:(NSString *)fileName subsong:(int)subsong {
    short int playcount=0;
    int retcode;
    NSString *filePathTmp;
    
    mSendStatTimer=0;
    shouldRestart=0;
    
    if (!filePath) return FALSE;
    if (!fileName) return FALSE;
    
    if (settings[GLOB_ResumeOnStart].detail.mdz_boolswitch.switch_value==0) mPlayingPosRestart=0;
    
    // if already playing, stop
    if (repeatingTimer) {
        [repeatingTimer invalidate];
        repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
        [mplayer Stop];
    }
    mShouldUpdateInfos=1;
    // load module
    
    const char *tmp_str=[filePath UTF8String];
    char tmp_str_copy[1024];
    int found_arc=0;
    int arc_index=-1;
    int found_sub=0;
    int sub_index=-1;
    int i=0;
    mOnlyCurrentEntry=0;
    mOnlyCurrentSubEntry=0;
    filePathTmp=NULL;
    while (tmp_str[i]) {
        if (found_arc) {
            arc_index=arc_index*10+tmp_str[i]-'0';
            mOnlyCurrentEntry|=1;
        }
        if (found_sub) {
            sub_index=sub_index*10+tmp_str[i]-'0';
            mOnlyCurrentSubEntry|=1;
        }
        if (tmp_str[i]=='@') {
            found_arc=1;
            arc_index=0;
            memcpy(tmp_str_copy,tmp_str,i);
            tmp_str_copy[i]=0;
            if (!filePathTmp) filePathTmp=[NSString stringWithUTF8String:tmp_str_copy];
        }
        if (tmp_str[i]=='?') {
            found_sub=1;
            sub_index=0;
            memcpy(tmp_str_copy,tmp_str,i);
            tmp_str_copy[i]=0;
            if (!filePathTmp) filePathTmp=[NSString stringWithUTF8String:tmp_str_copy];
        }
        i++;
    }
    if ((found_arc==0)&&(found_sub==0)) filePathTmp=[NSString stringWithString:filePath];
    
    if (mRestart) {
        
    } else {
        mRestart_arc=arc_index;
        mRestart_sub=sub_index;
        
        if (subsong>=0) mRestart_sub=subsong;
    }
    
    //ensure any settings changes to be taken into account before loading next file
    //[self settingsChanged:(int)SETTINGS_ALL];
    
    if (mShuffle==1) {
        mOnlyCurrentSubEntry|=2;
        mOnlyCurrentEntry|=2;
    }
    
    
    if ((retcode=[mplayer LoadModule:filePathTmp archiveMode:0 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart  shuffle:mShuffle])) {
        
        //error while loading
        //if it is an archive, try to load next entry until end or valid one reached
        if ( [mplayer isArchive] &&
            !mOnlyCurrentSubEntry &&
            !mOnlyCurrentEntry ) {
            
            do {
                if ([mplayer selectNextArcEntry]<0) break;
                mRestart_arc=[mplayer getArcIndex];
                retcode=[mplayer LoadModule:filePathTmp archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart shuffle:mShuffle];
                if ([mplayer getArcIndex]>=[mplayer getArcEntriesCnt]-1) break;
            } while (retcode);
        }
        
        if (retcode) {
            MDZELog("Issue in LoadModule %@",filePathTmp);
            mRestart=0;
            mRestart_sub=0;
            mRestart_arc=0;
            if (mplayer_error_msg[0]==0) snprintf(mplayer_error_msg,sizeof(mplayer_error_msg),"%s",[filePathTmp UTF8String]);
            if (retcode==-99) mLoadIssueMessage=0;
            else mLoadIssueMessage=3;
            return FALSE;
        }
    }
    
    if (mShuffle==1) {
        if ([mplayer isArchive]) {
            [mplayer Stop]; //deallocate relevant items
            
            if (!(mOnlyCurrentEntry&1)) mRestart_arc=arc4random()%[mplayer getArcEntriesCnt]; //do not shuffle if arc entry was part of filename
            
            if ((retcode=[mplayer LoadModule:filePath archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry  singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart shuffle:mShuffle])) {
                //error while loading
                
                do {
                    if ([mplayer selectNextArcEntry]<0) {
                        retcode=-1;
                        break;
                    } else {
                        mRestart_arc=[mplayer getArcIndex];
                        retcode=[mplayer LoadModule:filePathTmp archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry detailVC:self isRestarting:(bool)mRestart shuffle:mShuffle];
                    }
                } while (retcode);
                
                if (retcode) {
                    MDZELog("Issue in LoadModule(archive) %@",filePath);
                    if (retcode==-99) mLoadIssueMessage=0;
                    else mLoadIssueMessage=4;
                    
                    return FALSE;
                }
            }
            
            //            [self hideWaiting];
        }
    }
    
    
    
    //fix issue with OMPT settings reset after load
    //[self settingsChanged:(int)SETTINGS_ALL];
    
    [self checkForCover:filePathTmp];
    
    mPlaylist[mPlaylist_pos].cover_flag=-1;
    
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
    
    if ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0)) btnShowSubSong.hidden=false;
    else btnShowSubSong.hidden=true;
    if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0)) btnShowArcList.hidden=false;
    else btnShowArcList.hidden=true;
    if ([mplayer isVoicesMutingSupported]) btnShowVoices.hidden=false;
    else btnShowVoices.hidden=true;
    
    alertCannotPlay_displayed=0;
    //Visualization stuff
    [self reinitVisuVars];
    
    
    //Is OGLView visible ?
    [self checkGLViewCanDisplay];
    
    //Restart management
    if (mRestart) {
        //mRestart=0;
        self.pauseBarSub.hidden=YES;
        self.playBarSub.hidden=YES;
        self.pauseBar.hidden=YES;
        self.playBar.hidden=YES;
        
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.playBarSub.hidden=NO;
        else self.playBar.hidden=NO;
        
        [self updateBarPos];
        [mplayer PlaySeek:mPlayingPosRestart subsong:(mRestart_sub>=0?mRestart_sub:mplayer.mod_currentsub)];
        if (mPlayingPosRestart) {
            mPlayingPosRestart=0;
        } else sliderProgressModule.value=0;
        [mplayer Pause:YES];
        mIsPlaying=YES;
        mPaused=1;
    } else {
        //random mode & multi song ?
        if ((mShuffle==1)&&(found_sub==0)) {
            /*            if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)) {
             mOnlyCurrentSubEntry=1;
             }*/
            if ([mplayer isMultiSongs]) {
                mOnlyCurrentSubEntry|=2;
                mRestart_sub=arc4random()%(mplayer.mod_subsongs)+mplayer.mod_minsub;
            }
        } else if (mShuffle==2) {
            if (!(mOnlyCurrentSubEntry&1))
                if ([mplayer isMultiSongs]) mRestart_sub=arc4random()%(mplayer.mod_subsongs)+mplayer.mod_minsub;
        }
        self.pauseBarSub.hidden=YES;
        self.playBarSub.hidden=YES;
        self.pauseBar.hidden=YES;
        self.playBar.hidden=YES;
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.pauseBarSub.hidden=NO;
        else self.pauseBar.hidden=NO;
        [self updateBarPos];
        //mRestart=0;
        [mplayer PlaySeek:mPlayingPosRestart subsong:(mRestart_sub>=0?mRestart_sub:mplayer.mod_currentsub)];
        if (mPlayingPosRestart) {
            mPlayingPosRestart=0;
        } else sliderProgressModule.value=0;
        mIsPlaying=YES;
        mPaused=0;
        
    }
    
    mRestart_sub=0;
    mRestart_arc=0;
    //set volume (if applicable)
    [mplayer optOMPT_MasterVol:settings[OMPT_MasterVolume].detail.mdz_slider.slider_value];
    
    
    
    
    
    //Update song info if required
    labelArtist.text=mplayer.artist;
    
    if (labelArtist.text && [labelArtist.text length]) {
        labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,22);
        labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,18);
    } else {
        labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,40);
        labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,0);
    }

    
    labelModuleName.hidden=NO;
    if (settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value) labelModuleName.text=[NSString stringWithString:fileName];
    else {
        if ([mplayer getModFileTitleOrNull]) labelModuleName.text=[NSString stringWithFormat:@"%@ /%@",[mplayer getModFileTitle],[mplayer getModName]];
        else labelModuleName.text=[NSString stringWithFormat:@"%@",[mplayer getModName]];
    }
    
    [self refreshFXFSLabels];
    
    labelModuleSize.text=[NSString stringWithFormat:NSLocalizedString(@"Size: %dKB",@""), mplayer.mp_datasize>>10];
    if ([mplayer getSongLength]>0) {
        if (display_length_mode) labelTime.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getSongLength]/1000)/60,([mplayer getSongLength]/1000)%60];
        sliderProgressModule.enabled=YES;
        labelModuleLength.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getSongLength]/1000)/60,([mplayer getSongLength]/1000)%60];
    } else {
        if (display_length_mode) display_length_mode=0;
        sliderProgressModule.enabled=FALSE;
        labelModuleLength.text=@"--:--";
    }
    
    [self updateStats:fileName filePath:filePath playcount_inc:(mRestart==0)];
    if (!mRestart) {//UPDATE Google Application
        if (settings[GLOB_StatsUpload].detail.mdz_boolswitch.switch_value) mSendStatTimer=5*10;//Wait 5s
        
    }
    [self showRating:[self getCurrentRating]];
    
    
    labelTime.text=@"00:00";
    if (mplayer.numChannels) {
        if (mplayer.numChannels==1) labelNumChannels.text=[NSString stringWithFormat:@"1 channel"];
        else labelNumChannels.text=[NSString stringWithFormat:@"%d channels",mplayer.numChannels];
    } else labelNumChannels.text=[NSString stringWithFormat:@""];
    
    labelModuleType.text=[NSString stringWithFormat:@"Format: %@",[mplayer getModType]];
    labelLibName.text=[NSString stringWithFormat:@"Player: %@",[mplayer getPlayerName]];
    textMessage.text=[NSString stringWithFormat:@"\n%@",[mplayer getModMessage]];
    
    [textMessage scrollRangeToVisible:NSMakeRange(0, 1)];
    
    
    //    MPNowPlayingInfoCenter *infoCenter = [MPNowPlayingInfoCenter defaultCenter];
    //
    if (cover_img) artwork=[[MPMediaItemArtwork alloc] initWithImage:cover_img];
    else artwork=[[MPMediaItemArtwork alloc] initWithImage:default_cover];
    updMPNowCnt=0;
    
    //Activate timer for play infos
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.1f target:self selector:@selector(updateInfos:) userInfo:nil repeats: YES]; //10 times/second
    
    if (nowplayingPL) {
        NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
        [nowplayingPL.tableView reloadData];
        nowplayingPL.currentPlayedEntry=mPlaylist_pos;
        [nowplayingPL.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos+1] animated:YES scrollPosition:UITableViewScrollPositionMiddle];
    }
    mRestart=0;
    
    return TRUE;
}



#pragma mark -
#pragma mark Rotation support

//- (void)willAnimateSecondHalfOfRotationFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation duration:(NSTimeInterval)duration {
//- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
/*- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
 if ((fromInterfaceOrientation==UIInterfaceOrientationPortrait)||(fromInterfaceOrientation==UIInterfaceOrientationPortraitUpsideDown)) {
 } else {
 
 }
 }*/

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
    //    return UIInterfaceOrientationMaskAllButUpsideDown;
}

- (BOOL)shouldAutorotate {
    return YES;
}

// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
- (BOOL)mdzUpdateUI:(UIInterfaceOrientation)interfaceOrientation {
    float width;
    float height;
    
    orientationHV=interfaceOrientation;
    
    //MDZILog("safe: b%f t%f l%f r%f orientation: %d",safe_bottom,safe_top,safe_left,safe_right,(int)interfaceOrientation);
    
    if ((interfaceOrientation==UIInterfaceOrientationPortrait)||(interfaceOrientation==UIInterfaceOrientationPortraitUpsideDown)) {
        //        waitingView.transform=CGAffineTransformMakeRotation(interfaceOrientation==UIInterfaceOrientationPortrait?0:M_PI);
        //waitingView.frame=CGRectMake(mDevice_ww/2-60,mDevice_hh/2-60,120,110);
        
        if (!deactivateFStemp && settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) {
            if (mHasFocus) {
                statusbarHidden=YES;
                [self setNeedsStatusBarAppearanceUpdate];
            }
            [self.navigationController setNavigationBarHidden:YES animated:YES];
            mainView.frame = CGRectMake(0.0, 0.0, mDevice_ww, mDevice_hh);
            m_oglView.frame = CGRectMake(0.0, 0.0, mDevice_ww, mDevice_hh);
            //cover_viewBG.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh);//-230+80+44-safe_bottom);
            cover_viewAll.frame = m_oglView.frame;//CGRectMake(0, 0, mDevice_ww, mDevice_hh);//-230+80+44-safe_bottom);
            
            cover_view.frame = CGRectMake(cover_viewAll.frame.size.width/20,
                                          cover_viewAll.frame.size.height/20,
                                          cover_viewAll.frame.size.width-2*cover_viewAll.frame.size.width/20,
                                          cover_viewAll.frame.size.height-2*cover_viewAll.frame.size.height/20);
            
            if (bShowRadio) { radioView.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y+safe_top,mainView.frame.size.width,UI_RADIO_INFO_HEIGHT);
                radioTitle.frame=CGRectMake(2,2,radioView.frame.size.width-2,14);
                radioInfo.frame=CGRectMake(2,14,radioView.frame.size.width-2,radioView.frame.size.height-14);
            }
        } else {
            if (mHasFocus) {
                statusbarHidden=NO;
                [self setNeedsStatusBarAppearanceUpdate];
            }
            [self.navigationController setNavigationBarHidden:NO animated:YES];
            
            float yofs;//=self.navigationItem.titleView.frame.size.height;
            //if (is_macOS) yofs+=70+42;
            CGPoint pt;
            if (is_macOS||is_iPad) {
                pt=[self.view convertPoint:self.view.frame.origin toView:[UIApplication sharedApplication].keyWindow.rootViewController.view];
            } else {
                pt=[self.view convertPoint:self.mainView.frame.origin toView:[UIApplication sharedApplication].keyWindow.rootViewController.view];
            }
            
            //MDZILog("yo: %f",pt.y);
            yofs=pt.y;
            
            // Use self.view.safeAreaInsets for more reliable results, especially on iOS 15
            safe_bottom=self.view.safeAreaInsets.bottom;
            safe_top=self.view.safeAreaInsets.top;
            safe_left=self.view.safeAreaInsets.left;
            safe_right=self.view.safeAreaInsets.right;
            //            if (safe_bottom>0) safe_bottom+=20;
            //MDZILog("safe: %f %f %f %f",safe_bottom,safe_top,safe_left,safe_right);
            
            if (is_macOS||is_iPad) {
                mainView.frame = CGRectMake(safe_left, 0, mDevice_ww-safe_left-safe_right, mDevice_hh-yofs);
                m_oglView.frame = CGRectMake(0, 80, mDevice_ww-safe_left-safe_right, mDevice_hh-yofs-80-44-safe_bottom);
                oglButton.frame = CGRectMake(0, 80, mDevice_ww-safe_left-safe_right, mDevice_hh-yofs-80-44-safe_bottom);
            } else{
                mainView.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh-yofs);
                m_oglView.frame = CGRectMake(safe_left, 80, mDevice_ww-safe_left-safe_right, mDevice_hh-yofs-80-44-safe_bottom);
                oglButton.frame = CGRectMake(safe_left, 80, mDevice_ww-safe_left-safe_right, mDevice_hh-yofs-80-44-safe_bottom);
                if (gifAnimation) gifAnimation.frame = CGRectMake(0, 0,cover_view.frame.size.width,cover_view.frame.size.height);
            }
            
            if (bShowRadio) { radioView.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y,mainView.frame.size.width,UI_RADIO_INFO_HEIGHT);
                radioTitle.frame=CGRectMake(2,2,radioView.frame.size.width-2,14);
                radioInfo.frame=CGRectMake(2,14,radioView.frame.size.width-2,radioView.frame.size.height-14);
            }
            
            cover_viewAll.frame = m_oglView.frame;//CGRectMake(0, 0, mDevice_ww, mDevice_hh-230+80+44-safe_bottom);
            
            cover_view.frame = CGRectMake(cover_viewAll.frame.size.width/20,
                                          cover_viewAll.frame.size.height/20,
                                          cover_viewAll.frame.size.width-2*cover_viewAll.frame.size.width/20,
                                          cover_viewAll.frame.size.height-2*cover_viewAll.frame.size.height/20);
            
            
            if (gifAnimation) gifAnimation.frame = CGRectMake(0, 0,cover_view.frame.size.width,cover_view.frame.size.height);
            
            
            if (bShowEQ) eqVC.view.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y,mainView.frame.size.width,m_oglView.frame.size.height);
            if (bShowVC) voicesVC.view.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y,mainView.frame.size.width,m_oglView.frame.size.height);
            
            if (infoIsFullscreen) infoView.frame = CGRectMake(0, 0, m_oglView.frame.size.width, m_oglView.frame.size.height+m_oglView.frame.origin.y);
            else infoView.frame = m_oglView.frame;//CGRectMake(mainView.frame.origin.x, 80, mainView.frame.size.width, mDevice_hh-230-safe_bottom);
            
            //commandViewU.frame = CGRectMake(2, 48, mDevice_ww-4, 32);
            commandViewU.frame = CGRectMake(0, 0, mDevice_ww-safe_left-safe_right, 32+48);
            
            buttonLoopTitleSel.frame = CGRectMake(10,0+48,32,32);
            buttonLoopList.frame = CGRectMake(10,0+48,32,32);
            buttonLoopListSel.frame = CGRectMake(10,0+48,32,32);
            buttonShuffle.frame = CGRectMake(50,0+48,32,32);
            buttonShuffleSel.frame = CGRectMake(50,0+48,32,32);
            buttonShuffleOneSel.frame = CGRectMake(50,0+48,32,32);
            
            btnLoopInf.frame = CGRectMake(88,48+3,28,28);
            
            btnShowSubSong.frame = CGRectMake(mDevice_ww-safe_left-safe_right-36,0+48,32,32);
            btnShowArcList.frame = CGRectMake(mDevice_ww-safe_left-safe_right-36*2,0+48,32,32);
            btnShowVoices.frame =  CGRectMake(mDevice_ww-safe_left-safe_right-36*3,0+48,32,32);
            btnRecordScreen.frame =CGRectMake(mDevice_ww-safe_left-safe_right-36*4,0+48,32,32);
            btnAddToPl.frame =     CGRectMake(mDevice_ww-safe_left-safe_right-36*5,0+48,32,32);
            btnSaveFile.frame =    CGRectMake(mDevice_ww-safe_left-safe_right-36*5,0+48,32,32);
            btnRadioPrevList.frame =    CGRectMake(mDevice_ww-safe_left-safe_right-36*6,0+48,32,32);


            mainRating5.frame = CGRectMake(130+2,3+48+4,20,20);
            mainRating5off.frame = CGRectMake(130+2,3+48+4,20,20);
            
            
            if ([radioSource isActive]) {
                btnSaveFile.hidden=FALSE;
                btnRadioPrevList.hidden=FALSE;
                btnAddToPl.hidden=TRUE;
                mainRating5.hidden=TRUE;
                mainRating5off.hidden=TRUE;
            } else {
                btnSaveFile.hidden=TRUE;
                btnRadioPrevList.hidden=TRUE;
                btnAddToPl.hidden=FALSE;
                [self showRating:[self getCurrentRating]];
            }
            
            infoButton.frame = CGRectMake(mDevice_ww-safe_left-safe_right-40,4,36,36);
            eqButton.frame = CGRectMake(mDevice_ww-safe_left-safe_right-40-40,4,36,36);
            
            playlistPos.frame = CGRectMake((mDevice_ww-safe_left-safe_right)/2-90-20,0,180,20);
            labelModuleLength.frame=CGRectMake(2,0,45,20);
            labelTime.frame=CGRectMake(2,24,45,20);
            btnChangeTime.frame=CGRectMake(2,24,45,20);
            sliderProgressModule.frame = CGRectMake(48,23-6,mDevice_ww-safe_left-safe_right-48-40-40-4,23);
        }
    } else{
        //        waitingView.transform=CGAffineTransformMakeRotation(interfaceOrientation==UIInterfaceOrientationLandscapeLeft?-M_PI_2:M_PI_2);
        //waitingView.frame=CGRectMake(mDevice_hh/2-60,mDevice_ww/2-60,120,110);
        
        if (!deactivateFStemp && settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) {
            if (mHasFocus) {
                statusbarHidden=YES;
                [self setNeedsStatusBarAppearanceUpdate];
            }
            [self.navigationController setNavigationBarHidden:YES animated:YES];
            
            
            mainView.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww);
            m_oglView.frame = CGRectMake(0.0, 0.0, mDevice_hh, mDevice_ww);  //ipad
            //cover_viewBG.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww);//-82+82-safe_bottom-yofs);
            cover_viewAll.frame = m_oglView.frame;//CGRectMake(0.0, 0, mDevice_hh, mDevice_ww);//-82+82-safe_bottom-yofs);
            
            cover_view.frame = CGRectMake(cover_viewAll.frame.size.width/20,
                                          cover_viewAll.frame.size.height/20,
                                          cover_viewAll.frame.size.width-2*cover_viewAll.frame.size.width/20,
                                          cover_viewAll.frame.size.height-2*cover_viewAll.frame.size.height/20);
            
            if (bShowRadio) { radioView.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y+safe_top,mainView.frame.size.width,UI_RADIO_INFO_HEIGHT);
                radioTitle.frame=CGRectMake(2,2,radioView.frame.size.width-2,14);
                radioInfo.frame=CGRectMake(2,14,radioView.frame.size.width-2,radioView.frame.size.height-14);
            }
        } else {
            if (mHasFocus) {
                statusbarHidden=NO;
                [self setNeedsStatusBarAppearanceUpdate];
            }
            [self.navigationController setNavigationBarHidden:NO animated:YES];
            
            //                int yofs=self.navigationItem.titleView.frame.size.height;
            //                if (is_macOS) yofs+=104;
            //                else yofs+=12;
            
            float yofs;
            CGPoint pt;
            if (is_macOS||is_iPad) {
                pt=[self.view convertPoint:self.view.frame.origin toView:[UIApplication sharedApplication].keyWindow.rootViewController.view];
            } else {
                pt=[self.view convertPoint:self.mainView.frame.origin toView:[UIApplication sharedApplication].keyWindow.rootViewController.view];
            }
            yofs=pt.y;
            
#if TARGET_OS_MACCATALYST
            if (is_macOS) {
                yofs-=28;
            }
#endif
            
            // Use self.view.safeAreaInsets for more reliable results, especially on iOS 15
            safe_bottom=self.view.safeAreaInsets.bottom;
            safe_top=self.view.safeAreaInsets.top;
            safe_left=self.view.safeAreaInsets.left;
            safe_right=self.view.safeAreaInsets.right;
            //                if (safe_bottom>0) safe_bottom+=20;
            //                MDZILog("safe: %f %f %f %f",safe_bottom,safe_top,safe_left,safe_right);
            
            if (is_macOS||is_iPad) {
                mainView.frame = CGRectMake(safe_left, 0, mDevice_hh-safe_left-safe_right, mDevice_ww-yofs);
                m_oglView.frame = CGRectMake(0, 82, mDevice_hh-safe_left-safe_right, mDevice_ww-82-safe_bottom-yofs);
                oglButton.frame = CGRectMake(0, 82, mDevice_hh-safe_left-safe_right, mDevice_ww-82-safe_bottom-yofs);
                
            } else {
                mainView.frame = CGRectMake(safe_left, 28, mDevice_hh-safe_left-safe_right, mDevice_ww-yofs);
                m_oglView.frame = CGRectMake(0, 82, mDevice_hh-safe_left-safe_right, mDevice_ww-82-0*safe_bottom-yofs);
                oglButton.frame = CGRectMake(0, 82, mDevice_hh-safe_left-safe_right, mDevice_ww-82-0*safe_bottom-yofs);
                
            }
            
            if (bShowRadio) {
                radioView.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y,mainView.frame.size.width,UI_RADIO_INFO_HEIGHT);
                radioTitle.frame=CGRectMake(2,2,radioView.frame.size.width-2,14);
                radioInfo.frame=CGRectMake(2,14,radioView.frame.size.width-2,radioView.frame.size.height-14);
            }
            
            cover_viewAll.frame = m_oglView.frame;//CGRectMake(0.0, 0, mDevice_hh, mDevice_ww-82+82-safe_bottom-yofs);
            cover_view.frame = CGRectMake(cover_viewAll.frame.size.width/20,
                                          cover_viewAll.frame.size.height/20,
                                          cover_viewAll.frame.size.width-2*cover_viewAll.frame.size.width/20,
                                          cover_viewAll.frame.size.height-2*cover_viewAll.frame.size.height/20);
            if (gifAnimation) {
                gifAnimation.frame = CGRectMake(0, 0,cover_view.frame.size.width,cover_view.frame.size.height);
            }
            
            if (bShowEQ) eqVC.view.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y,mainView.frame.size.width,m_oglView.frame.size.height);
            if (bShowVC) voicesVC.view.frame=CGRectMake(mainView.frame.origin.x,m_oglView.frame.origin.y,mainView.frame.size.width,m_oglView.frame.size.height);
            
            if (infoIsFullscreen) infoView.frame = CGRectMake(0, 0, m_oglView.frame.size.width, m_oglView.frame.size.height+m_oglView.frame.origin.y);
            else infoView.frame = m_oglView.frame;// CGRectMake(mainView.frame.origin.x, 82, mainView.frame.size.width, mDevice_ww-82-30-0*safe_bottom);
            
            int xofs=mDevice_hh-(24*5+36*3+8)-safe_left-safe_right;
            yofs=10;
            //commandViewU.frame = CGRectMake(mDevice_hh-72-40-31-20-4, 8, 40+72+31+20, 32+32);
            commandViewU.frame = CGRectMake(0, 0, mainView.frame.size.width, 32+44+8);
            
            buttonLoopTitleSel.frame = CGRectMake(xofs+2,yofs+0,40,32);
            buttonLoopList.frame = CGRectMake(xofs+2,yofs+0,40,32);
            buttonLoopListSel.frame = CGRectMake(xofs+2,yofs+0,40,32);
            buttonShuffle.frame = CGRectMake(xofs+42,yofs+0,40,32);
            buttonShuffleSel.frame = CGRectMake(xofs+42,yofs+0,40,32);
            buttonShuffleOneSel.frame = CGRectMake(xofs+42,yofs+0,40,32);
            btnLoopInf.frame = CGRectMake(xofs+80,yofs+-12,35,57);
            
            mainRating5.frame = CGRectMake(xofs+6+2,yofs+42+2,20,20);
            mainRating5off.frame = CGRectMake(xofs+6+2,yofs+42+2,20,20);
            
            if ([radioSource isActive]) {
                btnSaveFile.hidden=FALSE;
                btnRadioPrevList.hidden=FALSE;
                btnAddToPl.hidden=TRUE;
                mainRating5.hidden=TRUE;
                mainRating5off.hidden=TRUE;
            } else {
                btnSaveFile.hidden=TRUE;
                btnRadioPrevList.hidden=TRUE;
                btnAddToPl.hidden=FALSE;
                [self showRating:[self getCurrentRating]];
            }
            
            btnShowSubSong.frame =  CGRectMake(xofs+6+24*5+4+36*2,yofs+40,32,32);
            btnShowArcList.frame =  CGRectMake(xofs+6+24*5+4+36,yofs+40,32,32);
            btnShowVoices.frame =   CGRectMake(xofs+6+24*5+4,yofs+40,32,32);
            btnRecordScreen.frame = CGRectMake(xofs+6+24*5+4-36,yofs+40,32,32);
            btnAddToPl.frame =      CGRectMake(xofs+6+24*5+4-36*2,yofs+40,32,32);
            btnSaveFile.frame =     CGRectMake(xofs+6+24*5+4-36*2,yofs+40,32,32);
            btnRadioPrevList.frame =     CGRectMake(xofs+6+24*5+4-36*3,yofs+40,32,32);
            
            infoButton.frame = CGRectMake(mDevice_hh-44-safe_left-safe_right,4,40,40);
            eqButton.frame = CGRectMake(mDevice_hh-44-44-safe_left-safe_right,4,40,40);
            
            playlistPos.frame = CGRectMake((mDevice_hh-200-safe_left-safe_right)/2-90,0,180,20);
            
            labelModuleLength.frame=CGRectMake(2,0,45,20);
            labelTime.frame=CGRectMake(2,20,45,20);
            btnChangeTime.frame=CGRectMake(2,17,45,20);
            
            sliderProgressModule.frame = CGRectMake(48,16-3,mDevice_hh-(24*5+36*3+10+48)-safe_left-safe_right,23);
        }
    }
    [self updateBarPos];
    
    
    return YES;
}


-(void)updateBarPos {
    if ((orientationHV==UIInterfaceOrientationPortrait)||(orientationHV==UIInterfaceOrientationPortraitUpsideDown)) {
        float y_ofs;
        if (is_macOS||is_iPad) {
            
            y_ofs=m_oglView.frame.origin.y+m_oglView.frame.size.height;
            
            playBar.frame = CGRectMake(0, y_ofs, mDevice_ww-safe_left-safe_right, 44);
            pauseBar.frame = CGRectMake(0, y_ofs, mDevice_ww-safe_left-safe_right, 44);
            playBarSub.frame = CGRectMake(0, y_ofs, mDevice_ww-safe_left-safe_right, 44);
            pauseBarSub.frame = CGRectMake(0, y_ofs, mDevice_ww-safe_left-safe_right, 44);
        } else {
            y_ofs=m_oglView.frame.origin.y+m_oglView.frame.size.height;
            
            playBar.frame = CGRectMake(0, y_ofs, mDevice_ww, 44);
            pauseBar.frame = CGRectMake(0, y_ofs, mDevice_ww, 44);
            playBarSub.frame = CGRectMake(0, y_ofs, mDevice_ww, 44);
            pauseBarSub.frame = CGRectMake(0, y_ofs, mDevice_ww, 44);
            
        }
    } else {
        int xofs=24*5+36*3+10;
        float y_ofs=40;
        
        playBar.frame = CGRectMake(0, y_ofs, mDevice_hh-xofs-safe_left-safe_right, 44); //mDevice_hh-(playBar.hidden?0:375)
        pauseBar.frame = CGRectMake(0, y_ofs, mDevice_hh-xofs-safe_left-safe_right, 44);
        playBarSub.frame =  CGRectMake(0, y_ofs, mDevice_hh-xofs-safe_left-safe_right, 44);
        pauseBarSub.frame =  CGRectMake(0, y_ofs, mDevice_hh-xofs-safe_left-safe_right, 44);
    }
    
}



#pragma mark -
#pragma mark View lifecycle



/**************************************************/
/**************************************************/
/**************************************************/
/* User Defined Variables */
GLfloat angle;

GLuint txtBGImage,txtCoverImg;
float txtCoverImgRatio;
bool coverAvailable;
//GLsizei txtBGImageWidth,txtBGImageHeight;

/**************************************************/

//return 1 if flag is not ok
-(int)checkFlagOnStartup{
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSNumber *valNb;
    int retcode=0;
    
    [prefs synchronize];
    
    valNb=[prefs objectForKey:@"ModizerRunningForeGround"];if (DEBUG_NO_SETTINGS) valNb=nil;
    if (valNb == nil) retcode=1;
    else if ([valNb intValue]!=0) retcode=1;
    
    valNb=[[NSNumber alloc] initWithInt:1];
    [prefs setObject:valNb forKey:@"ModizerRunningForeGround"];
    
    return retcode;
}

-(void) updateAllSettingsAfterChange {
    /////////////////////////////////////
    //update according to settings
    if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) {
        settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=0;
        [self oglViewSwitchFS];
    }
    
    if (infoIsFullscreen) {
        infoZoom.hidden=YES;
        infoUnzoom.hidden=NO;
    } else {
        infoZoom.hidden=NO;
        infoUnzoom.hidden=YES;
    }
    
    mLoopMode--;
    [self changeLoopMode];
    [mplayer setLoopInf:mplayer.mLoopMode^1];
    [self pushedLoopInf];
    
    mShuffle--;
    if (mShuffle<0) mShuffle=2;
    [self shuffle];
    
    //update settings according toi what was loaded
    
    [self settingsChanged:(int)SETTINGS_ALL];
    
}

-(void)loadSettings:(int)safe_mode{
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSNumber *valNb;
    
    MDZILog("Load Settings, safe mode %d",safe_mode);
    
    [prefs synchronize];
    
    not_expected_version=0;
    valNb=[prefs objectForKey:@"VERSION_MAJOR"];if (safe_mode) valNb=nil;
    if (valNb == nil) {
        //should not happen on released version
        not_expected_version=0;
    } else {
        if ([valNb intValue]!=VERSION_MAJOR) {
            not_expected_version=1;
            
            MDZILog("not expected version maj");
        }
    }
    valNb=[prefs objectForKey:@"VERSION_MINOR"];if (safe_mode) valNb=nil;
    if (valNb == nil) {
        //should not happen on released version
        not_expected_version=0;
    } else {
        if ([valNb intValue]!=VERSION_MINOR) {
            not_expected_version=1;
            
            MDZILog("not expected version min");
        }
    }
    if (not_expected_version) {
        //do some stuff, like reset/upgrade DB, ...
    }
    
    valNb=[prefs objectForKey:@"ViewFX"];if (safe_mode) valNb=nil;
    if (valNb == nil) mOglViewIsHidden=YES;
    else mOglViewIsHidden = [valNb boolValue];
    
    
    ////////////////////////////////////
    // Internal stuff
    /////////////////////////
    valNb=[prefs objectForKey:@"InfoFullscreen"];if (safe_mode) valNb=nil;
    if (valNb == nil) infoIsFullscreen = 0;
    else infoIsFullscreen = [valNb intValue];
    valNb=[prefs objectForKey:@"OGLFullscreen"];if (safe_mode) valNb=nil;
    //    if (valNb == nil) oglViewFullscreen = 0;
    //    else oglViewFullscreen = [valNb intValue];
    valNb=[prefs objectForKey:@"LoopMode"];if (safe_mode) valNb=nil;
    if (valNb == nil) mLoopMode = 0;
    else mLoopMode = [valNb intValue];
    valNb=[prefs objectForKey:@"LoopModeInf"];if (safe_mode) valNb=nil;
    if (valNb == nil) [mplayer setLoopInf:0];
    else [mplayer setLoopInf:([valNb intValue]?1:0)];
    valNb=[prefs objectForKey:@"Shuffle"];if (safe_mode) valNb=nil;
    if (valNb == nil) mShuffle = 0;
    else mShuffle = [valNb intValue];
    
    
    
    valNb=[prefs objectForKey:@"IsPlaying"];if (safe_mode) valNb=nil;
    if (valNb == nil) mIsPlaying=FALSE;
    else mIsPlaying= [valNb boolValue];
    valNb=[prefs objectForKey:@"PlayingPos"];if (safe_mode) valNb=nil;
    if (valNb == nil) mPlayingPosRestart=0;
    else mPlayingPosRestart= [valNb intValue];
    
    valNb=[prefs objectForKey:@"PlaylistSize"];if (safe_mode) valNb=nil;
    if (valNb == nil) mPlaylist_size=0;
    else mPlaylist_size= [valNb intValue];
    valNb=[prefs objectForKey:@"PlaylistPos"];if (safe_mode) valNb=nil;
    if (valNb == nil) mPlaylist_pos=0;
    else mPlaylist_pos= [valNb intValue];
    
    valNb=[prefs objectForKey:@"Subsongs"];if (safe_mode) valNb=nil;
    if (valNb != nil) {
        if ([valNb intValue]>1) {
            valNb=[prefs objectForKey:@"Cur_subsong"];if (safe_mode) valNb=nil;
            if (valNb != nil) mRestart_sub=[valNb intValue];
        }
    }
    
    valNb=[prefs objectForKey:@"ArchiveCnt"];if (safe_mode) valNb=nil;
    if (valNb != nil) {
        if ([valNb intValue]>0) {
            valNb=[prefs objectForKey:@"ArchiveIndex"];if (safe_mode) valNb=nil;
            if (valNb != nil) mRestart_arc=[valNb intValue];
        }
    }
    
    valNb=[prefs objectForKey:@"ProjectM_playlist_index"];if (safe_mode) valNb=nil;
    if (valNb != nil) {
        int idx=[valNb intValue];
        _pm_shouldRestartAt=idx;
    }
    
    if (mPlaylist_size) {
        gzFile f;
        f=gzopen([[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"Documents/modizer.plnow"] UTF8String],"rb");
        if (f) {
            int fsize=1<<16;
            char *fdata=(char *)malloc(fsize);
            char str[1024];
            if (fdata) {
                int available_bytes=gzread(f,fdata,fsize);
                int ofs=0;
                int ofs_str=0;
                for (int i=0;i<mPlaylist_size;i++) {
                    while (fdata[ofs]) {
                        str[ofs_str++]=fdata[ofs];
                        if (ofs_str>=1024) break;
                        ofs++;
                        if (ofs>=fsize) {//refill
                            available_bytes=gzread(f,fdata,fsize);
                            if (!available_bytes) {
                                MDZELog("error while uncompressing modizer.plnow @ %d/%d",i,mPlaylist_size);
                                mPlaylist_size=0;
                                break;
                            }
                            ofs=0;
                        }
                    }
                    if (ofs_str>=1024) {
                        MDZELog("error while uncompressing modizer.plnow @ %d/%d",i,mPlaylist_size);
                        mPlaylist_size=0;
                        break;
                    }
                    str[ofs_str]=0;
                    ofs_str=0;
                    mPlaylist[i].mPlaylistFilename=[NSString stringWithUTF8String:str];
                    
                    //move to 1st char of string
                    ofs++;
                    if (ofs>=fsize) { //refill
                        available_bytes=gzread(f,fdata,fsize);
                        if (!available_bytes) {
                            MDZELog("error while uncompressing modizer.plnow @ %d/%d",i,mPlaylist_size);
                            mPlaylist_size=0;
                            break;
                        }
                        ofs=0;
                    }
                    
                    while (fdata[ofs]) {
                        str[ofs_str++]=fdata[ofs];
                        if (ofs_str>=1024) break;
                        ofs++;
                        if (ofs>=fsize) { //refill
                            available_bytes=gzread(f,fdata,fsize);
                            if (!available_bytes) {
                                MDZELog("error while uncompressing modizer.plnow @ %d/%d",i,mPlaylist_size);
                                mPlaylist_size=0;
                                break;
                            }
                            ofs=0;
                        }
                    }
                    if (ofs_str>=1024) {
                        MDZELog("error while uncompressing modizer.plnow @ %d/%d",i,mPlaylist_size);
                        mPlaylist_size=0;
                        break;
                    }
                    str[ofs_str]=0;
                    ofs_str=0;
                    mPlaylist[i].mPlaylistFilepath=[NSString stringWithUTF8String:str];
                    //move to 1st char of string
                    ofs++;
                    if (ofs>=fsize) { //refill
                        available_bytes=gzread(f,fdata,fsize);
                        if (!available_bytes) {
                            MDZELog("error while uncompressing modizer.plnow @ %d/%d",i,mPlaylist_size);
                            mPlaylist_size=0;
                            break;
                        }
                        ofs=0;
                    }
                }
            }
            free(fdata);
            gzclose(f);
        } else {
            mRestart_sub=0;
            mRestart_arc=0;
            mPlaylist_pos=0;
            mPlaylist_size=0;
        }
    }
    if (not_expected_version) {
        
    }
    
    [self updateAllSettingsAfterChange];
}

-(void)saveSettings{
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSNumber *valNb;
    
    valNb=[[NSNumber alloc] initWithInt:VERSION_MAJOR];
    [prefs setObject:valNb forKey:@"VERSION_MAJOR"];
    valNb=[[NSNumber alloc] initWithInt:(VERSION_MINOR)];
    [prefs setObject:valNb forKey:@"VERSION_MINOR"];
    
    ///////////////////////////////////
    // Internal stuff
    ///////////////////////////////////////
    valNb=[[NSNumber alloc] initWithInt:(int)mOglViewIsHidden];
    [prefs setObject:valNb forKey:@"ViewFX"];
    
    valNb=[[NSNumber alloc] initWithInt:infoIsFullscreen];
    [prefs setObject:valNb forKey:@"InfoFullscreen"];
    //    valNb=[[NSNumber alloc] initWithInt:oglViewFullscreen];
    //    [prefs setObject:valNb forKey:@"OGLFullscreen"];
    
    
    valNb=[[NSNumber alloc] initWithInt:mLoopMode];
    [prefs setObject:valNb forKey:@"LoopMode"];
    valNb=[[NSNumber alloc] initWithInt:mplayer.mLoopMode];
    [prefs setObject:valNb forKey:@"LoopModeInf"];
    valNb=[[NSNumber alloc] initWithInt:mShuffle];
    [prefs setObject:valNb forKey:@"Shuffle"];
    
    
    valNb=[[NSNumber alloc] initWithBool:mIsPlaying];
    [prefs setObject:valNb forKey:@"IsPlaying"];
    
    valNb=[[NSNumber alloc] initWithInt:mPlaylist_size];
    [prefs setObject:valNb forKey:@"PlaylistSize"];
    
    valNb=[[NSNumber alloc] initWithInt:mPlaylist_pos];
    [prefs setObject:valNb forKey:@"PlaylistPos"];
    
    valNb=[[NSNumber alloc] initWithInt:[mplayer getCurrentTime]];
    [prefs setObject:valNb forKey:@"PlayingPos"];
    
    if (mPlaylist_size) {
        gzFile f;
        f=gzopen([[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"Documents/modizer.plnow"] UTF8String],"wb");
        if (f) {
            for (int i=0;i<mPlaylist_size;i++) {
                const char *str=[mPlaylist[i].mPlaylistFilename UTF8String];
                gzwrite(f,str,(int)(strlen(str)+1));
                str=[mPlaylist[i].mPlaylistFilepath UTF8String];
                gzwrite(f,str,(int)(strlen(str)+1));
            }
            gzclose(f);
        }
    }
    
    valNb=[[NSNumber alloc] initWithInt:mplayer.mod_subsongs];
    [prefs setObject:valNb forKey:@"Subsongs"];
    valNb=[[NSNumber alloc] initWithInt:mplayer.mod_currentsub];
    [prefs setObject:valNb forKey:@"Cur_subsong"];
    
    valNb=[[NSNumber alloc] initWithInt:[mplayer getArcEntriesCnt]];
    [prefs setObject:valNb forKey:@"ArchiveCnt"];
    valNb=[[NSNumber alloc] initWithInt:[mplayer getArcIndex]];
    [prefs setObject:valNb forKey:@"ArchiveIndex"];
    
    if (_mdzPM_playlist!=nil) {
        if ([_mdzPM_playlist getSize]) {
            int index=[_mdzPM_playlist getPos];
            valNb=[[NSNumber alloc] initWithInt:index];
            [prefs setObject:valNb forKey:@"ProjectM_playlist_index"];
        }
    }
    
    //Synchronise pref
    [prefs synchronize];
    
    // Save PM playlist if available
    [_mdzPM_playlist savePlaylist];
    
    // Save PM Favorites
    [_mdzPM_Favorites saveFavorites];
    
}


- (NSString *)machine
{
    size_t size;
    
    // Set 'oldp' parameter to NULL to get the size of the data
    // returned so we can allocate appropriate amount of space
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    
    // Allocate the space to store name
    char *name = (char*)malloc(size);
    
    // Get the platform name
    sysctlbyname("hw.machine", name, &size, NULL, 0);
    
    // Place name into a string
    NSString *machine = [[NSString alloc] initWithFormat:@"%s",name];
    
    // Done with this
    free(name);
    
    return machine;
}

-(UIImage*) getArchiveCover:(NSString *)filepath {
    UIImage *res_image=nil;
    const char *path=[filepath UTF8String];
    struct archive *a;
    struct archive_entry *entry;
    int r;
    
    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);
    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, [filepath UTF8String], 10240); // Note 1
    if (r == ARCHIVE_OK) {
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            NSString *strFilename=[ModizFileHelper getCorrectFileName:[filepath UTF8String] archive:a entry:entry];
            bool found_img=false;
            if ([[strFilename pathExtension] caseInsensitiveCompare:@"PNG"]==NSOrderedSame) {
                //PNG detected
                found_img=true;
            }
            if ([[strFilename pathExtension] caseInsensitiveCompare:@"JPG"]==NSOrderedSame) {
                //JPG detected
                found_img=true;
            }
            if ([[strFilename pathExtension] caseInsensitiveCompare:@"JPEG"]==NSOrderedSame) {
                //JPEG detected
                found_img=true;
            }
            if ([[strFilename pathExtension] caseInsensitiveCompare:@"WEBP"]==NSOrderedSame) {
                //WEBP detected
                found_img=true;
            }
            if ([[strFilename pathExtension] caseInsensitiveCompare:@"GIF"]==NSOrderedSame) {
                //GIF detected
                found_img=true;
            }
            
            if (found_img) {
                char *data_ptr;
                int data_size=archive_entry_size(entry);
                data_ptr=(char*)malloc(data_size);
                if (data_ptr) {
                    r=archive_read_data(a,data_ptr,data_size);
                    if (r==ARCHIVE_OK) res_image=[UIImage imageWithData:[NSData dataWithBytes:data_ptr length:data_size]];
                    free(data_ptr);
                    break;
                }
            }
        }
    }
    r = archive_read_free(a);  // Note 3
    
    return res_image;
}

-(int) scanArchiveForCover:(const char *)path {
    struct archive *a;
    struct archive_entry *entry;
    int r;
    int ret=0;
    
    a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);
    archive_read_support_format_all(a);
    r = archive_read_open_filename(a, path, 10240); // Note 1
    if (r == ARCHIVE_OK) {
        while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            NSString *strFilename=[ModizFileHelper getCorrectFileName:path archive:a entry:entry];
            if ([[strFilename pathExtension] caseInsensitiveCompare:@"PNG"]==NSOrderedSame) {
                //PNG detected
                ret=1;
                break;
            }
            if ([[strFilename pathExtension] caseInsensitiveCompare:@"JPG"]==NSOrderedSame) {
                //JPG detected
                ret=2;
                break;
            }
            if ([[strFilename pathExtension] caseInsensitiveCompare:@"JPEG"]==NSOrderedSame) {
                //JPEG detected
                ret=3;
                break;
            }
            if ([[strFilename pathExtension] caseInsensitiveCompare:@"WEBP"]==NSOrderedSame) {
                //WEBP detected
                ret=4;
                break;
            }
            if ([[strFilename pathExtension] caseInsensitiveCompare:@"GIF"]==NSOrderedSame) {
                //GIF detected
                ret=5;
                break;
            }
        }
        r = archive_read_free(a);  // Note 3
    }
    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"

-(void)orientationDidChange:(NSNotification*)notification
{
    UIDeviceOrientation op=[[UIDevice currentDevice]orientation];
    UIInterfaceOrientation o = [[UIApplication sharedApplication] statusBarOrientation];
    o = [[UIApplication sharedApplication] statusBarOrientation];
    
    switch (o) {
        case UIInterfaceOrientationLandscapeLeft:
            orientationHV=(int)UIInterfaceOrientationLandscapeLeft;
            break;
        case UIInterfaceOrientationLandscapeRight:
            orientationHV=(int)UIInterfaceOrientationLandscapeRight;
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            orientationHV=(int)UIInterfaceOrientationPortraitUpsideDown;
            break;
        default:
            orientationHV=(int)UIInterfaceOrientationPortrait;
            break;
    }
    
    /*switch (op) {
     case UIDeviceOrientationPortrait:            // Device oriented vertically, home button on the bottom
     orientationHV=(int)UIInterfaceOrientationPortrait;
     break;
     case UIDeviceOrientationPortraitUpsideDown:  // Device oriented vertically, home button on the top
     orientationHV=(int)UIInterfaceOrientationPortraitUpsideDown;
     break;
     case UIDeviceOrientationLandscapeLeft:       // Device oriented horizontally, home button on the right
     orientationHV=(int)UIInterfaceOrientationLandscapeRight;
     break;
     case UIDeviceOrientationLandscapeRight:      // Device oriented horizontally, home button on the left
     orientationHV=(int)UIInterfaceOrientationLandscapeLeft;
     break;
     case UIDeviceOrientationFaceUp:              // Device oriented flat, face up
     orientationHV=(int)UIInterfaceOrientationPortrait;
     break;
     case UIDeviceOrientationFaceDown:             // Device oriented flat, face down
     orientationHV=(int)UIInterfaceOrientationPortrait;
     break;
     default:
     orientationHV=(int)UIInterfaceOrientationPortrait;
     }*/
    
    /*if(Orientation==UIDeviceOrientationLandscapeLeft || Orientation==UIDeviceOrientationLandscapeRight)
     {
     }
     else if(Orientation==UIDeviceOrientationPortrait)
     {
     }*/
}


-(void) traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (alertTableView) [alertTableView reloadData];
}
-(void) presentContextOGL {
    MGLLayer *oglLayer = (MGLLayer *)m_oglView.layer;
    
    [m_oglContext present:oglLayer];
}
-(void) setContextOGL {
    MGLLayer *oglLayer = (MGLLayer *)m_oglView.layer;
    [MGLContext setCurrentContext:m_oglContext forLayer:oglLayer];
    glViewport(0, 0, m_oglView.frame.size.width*glScaleFactor, m_oglView.frame.size.height*glScaleFactor);
}
-(void) setupOGLView {
    //Ensure the view doesn't have width or height = 0
    if ( (m_oglView.frame.size.height==0) || (m_oglView.frame.size.width==0) ) {
        m_oglView.frame = CGRectMake(0,0,64,64);
    }
    MGLLayer *oglLayer = (MGLLayer *)m_oglView.layer;
    // Set the layer's scale factor as you wish
    //    oglLayer.retainedBacking = YES;
    oglLayer.contentsScale = [[UIScreen mainScreen] scale];
    glScaleFactor=[[UIScreen mainScreen] scale];
    
    //to avoid flickering issue / rest of UI widgets
    oglLayer.drawsAsynchronously = YES;
    oglLayer.shouldRasterize = NO;
    oglLayer.opaque=YES;
    oglLayer.allowsGroupOpacity=NO;
    
    
    // Create OpenGL context
    m_oglContext = [[MGLContext alloc] initWithAPI:kMGLRenderingAPIOpenGLES3];
    if (!m_oglContext || ![MGLContext setCurrentContext:m_oglContext]) {
        MDZELog("no OGL context!!");
    }
    m_oglView.context = m_oglContext;
    
    // Configure renderbuffers created by the view
    m_oglView.drawableColorFormat = MGLDrawableColorFormatRGBA8888;
    m_oglView.drawableDepthFormat = MGLDrawableDepthFormat24;
    m_oglView.drawableStencilFormat = MGLDrawableStencilFormatNone;
    // Enable multisampling
    m_oglView.drawableMultisample = MGLDrawableMultisampleNone;
    
    
    
}

void pmSoftReinit(bool forceReloadPlaylist) {
    if (!_pm) return;
    
    const char *curPresetLocalPath=NULL;
    int curPresetType=NULL;
    if ([_mdzPM_playlist getSize]) {
        curPresetLocalPath=[_mdzPM_playlist getCurFullpath];
        curPresetType=[_mdzPM_playlist getCurType];
    }
    
    //projectm_playlist_set_shuffle(_pm_playlist, settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value);
    [_mdzPM_playlist setShuffle:settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value];
    
    meshX=round(settings[PROJECTM_MeshSizeX].detail.mdz_slider.slider_value/2)*2;
    if (meshX<8) meshX=8;if (meshX>128) meshX=128;
    meshY=round(settings[PROJECTM_MeshSizeY].detail.mdz_slider.slider_value/2)*2;
    if (meshX<8) meshX=8;if (meshX>128) meshX=128;
    
    projectm_set_mesh_size(_pm, meshX, meshY);
    projectm_set_aspect_correction(_pm, settings[PROJECTM_AspectRatio].detail.mdz_boolswitch.switch_value);
    projectm_set_preset_locked(_pm, settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value);
    
    // Preset display settings
    projectm_set_preset_duration(_pm, settings[PROJECTM_TimeBetweenPreset].detail.mdz_slider.slider_value);//15.0);
    if (settings[PROJECTM_BlendPresets].detail.mdz_boolswitch.switch_value) projectm_set_soft_cut_duration(_pm, settings[PROJECTM_BlendTime].detail.mdz_slider.slider_value);//2.0);
    else projectm_set_soft_cut_duration(_pm, 0);//2.0);
    projectm_set_hard_cut_enabled(_pm, settings[PROJECTM_HardCutEnabled].detail.mdz_boolswitch.switch_value);
    
    projectm_set_hard_cut_duration(_pm, settings[PROJECTM_HardCutMinTime].detail.mdz_slider.slider_value);
    projectm_set_hard_cut_sensitivity(_pm, settings[PROJECTM_HardCutSensitivity].detail.mdz_slider.slider_value);
    projectm_set_beat_sensitivity(_pm, settings[PROJECTM_BeatSensitivity].detail.mdz_slider.slider_value);
    
    if (forceReloadPlaylist ||
        (_pm_playlist_loadBundled!=settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value) ||
        (_pm_playlist_loadCustom!=settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value)) {
        
        
        if (settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value && (_pm_playlist_loadCustom!=settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value) ) {
            //parse again custom dir
            updatePresetCustomDirStructure();
        }
        
        [_mdzPM_playlist clear];
        
        _pm_playlist_loadBundled=settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value;
        _pm_playlist_loadCustom=settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value;
        
        if (_pm_playlist_loadBundled) [_mdzPM_playlist addItems:[pmBundledPresetsFileNode getSelectedPlaylist]];
        if (_pm_playlist_loadCustom) [_mdzPM_playlist addItems:[pmCustomPresetsFileNode getSelectedPlaylist]];
        
        //try to restart from same preset
        bool found_pos=false;
        if (curPresetLocalPath) found_pos=[_mdzPM_playlist setPosForPreset:curPresetLocalPath type:curPresetType];
        if (!found_pos) {
            //Wasn't able to keep same preset, have to restart
            if (_mdzPM_playlist.shuffle) [_mdzPM_playlist next:false];
            else [_mdzPM_playlist setPos:0 cut:false];
        }
    }
}

void updatePresetCustomDirStructure() {
    DirParser *dirParser=[[DirParser alloc] init];
    dirParser.includeHiddenFiles = NO;
    dirParser.maxDepth = 5;
    dirParser.filterExt = @[@"milk", @"milkz"];
    
    NSError *error;
    
    NSString *canonicalHomePath;
    [[[NSURL fileURLWithPath:[ModizFileHelper getAppHomeDirectory]] URLByResolvingSymlinksInPath] getResourceValue:&canonicalHomePath forKey:NSURLCanonicalPathKey error:nil];
    NSString *dirPath = [NSString stringWithFormat:@"%@/Documents%s/presets",canonicalHomePath,PM_ROOT_FOLDER_CUSTOM];
    
    pmCustomPresetsFileNode=nil;
    pmCustomPresetsFileNode=[dirParser parseFastDirectoryAtPath:dirPath type:MDZ_PLAYLIST_FNODE_Custom error:&error];
    if (error) {
        MDZELog("Cannot parse projectm custom presets");
        pmBundledPresetsFileNode=nil;
    }
}

void buildPresetDirStructure() {
    DirParser *dirParser=[[DirParser alloc] init];
    dirParser.includeHiddenFiles = NO;
    dirParser.maxDepth = 5;
    dirParser.filterExt = @[@"milk",@"milkz"];
    
    NSString *canonicalHomePath;
    
    NSString *pmBundleDir = [NSString stringWithFormat:@"%@/projectm/assets/presets",[[NSBundle mainBundle] resourcePath]];
    
    [[[NSURL fileURLWithPath:[ModizFileHelper getAppHomeDirectory]] URLByResolvingSymlinksInPath] getResourceValue:&canonicalHomePath forKey:NSURLCanonicalPathKey error:nil];
    NSString *pmCustomDir = [NSString stringWithFormat:@"%@/Documents%s/presets",canonicalHomePath,PM_ROOT_FOLDER_CUSTOM];
    
    //NSString *pmCustomDir = [NSString stringWithFormat:@"%@/Documents%s/presets",[ModizFileHelper getAppHomeDirectory],PM_ROOT_FOLDER_CUSTOM];
    
    NSError *error=nil;
    
    pmBundledPresetsFileNode=nil;
    pmBundledPresetsFileNode=[dirParser parseFastDirectoryAtPath:pmBundleDir type:MDZ_PLAYLIST_FNODE_Bundle error:&error];
    if (error) {
        MDZELog("Cannot parse projectm blunded presets");
        pmBundledPresetsFileNode=nil;
    }
    
    pmCustomPresetsFileNode=nil;
    pmCustomPresetsFileNode=[dirParser parseFastDirectoryAtPath:pmCustomDir type:MDZ_PLAYLIST_FNODE_Custom error:&error];
    
    if (error) {
        MDZELog("Cannot parse projectm custom presets");
        pmBundledPresetsFileNode=nil;
    }
}

#ifdef PM_TEST_LOAD
void pm_perfTest() {
    START_PROFILE
    [_mdzPM_playlist setPos:0 cut:true];
    char strTmp[16];
    for (int i=0;i<PM_TEST_LOAD;i++) {
        [_mdzPM_playlist next:false];
        snprintf(strTmp,16,"load %d",i);
        CHECK_PROFILE(strTmp)
    }
    END_PROFILE
}
#endif

- (void)pmRelease {
    if ((!_pmIsInitialized)||(_pm==NULL)) return;
    projectm_destroy(_pm);
    _pm=NULL;
    _pm_shouldRestartAt=[_mdzPM_playlist getPos];
    
    _mdzPM_playlist=nil;
    _mdzPM_Favorites=nil;
    
    _pmIsInitialized=false;
}

- (void)pmInit {
    
    mdz_pmMilkPermissiveEvalCode=settings[PROJECTM_PermmissiveMode].detail.mdz_boolswitch.switch_value;
    
    _pm = projectm_create();
    if (!_pm) {
        MDZELog("cannot create projectM instance");
        return;
    }
    
    // Allocate Playlist
    _mdzPM_playlist=[[MDZPlaylist alloc] init:_pm name:@"PM Default Playlist"];
    // Try to load existing save
    [_mdzPM_playlist loadPlaylist];
    
    // Allocate Favorites
    _mdzPM_Favorites=[[MDZFavorites alloc] init];
    [_mdzPM_Favorites loadFavorites];
    _pmFavoritesChanged=false;
    
    MDZDLog("loaded pl, entries nb: %d",[_mdzPM_playlist getSize]);
    if ([_mdzPM_playlist getSize]) {
        [_mdzPM_playlist updateFileNodeStatus:pmBundledPresetsFileNode];
        [_mdzPM_playlist updateFileNodeStatus:pmCustomPresetsFileNode];
        [_mdzPM_Favorites updateFileNodeStatus:pmBundledPresetsFileNode type:0];
        [_mdzPM_Favorites updateFileNodeStatus:pmCustomPresetsFileNode type:1];
    }
    
    [_mdzPM_playlist setShuffle:settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value];
    
    _pm_fps=settings[GLOB_FXFPS].detail.mdz_switch.switch_value?60:30;
    
    
    
    if (_pmCanvasWidth<512) _pmCanvasWidth=512;
    if (_pmCanvasHeight<512) _pmCanvasHeight=512;
    
    projectm_set_window_size(_pm, _pmCanvasWidth, _pmCanvasHeight);
    projectm_set_fps(_pm, _pm_fps);
    
    meshX=round(settings[PROJECTM_MeshSizeX].detail.mdz_slider.slider_value/2)*2;
    if (meshX<8) meshX=8;if (meshX>128) meshX=128;
    meshY=round(settings[PROJECTM_MeshSizeY].detail.mdz_slider.slider_value/2)*2;
    if (meshX<8) meshX=8;if (meshX>128) meshX=128;
    
    projectm_set_mesh_size(_pm, meshX, meshY);
    projectm_set_aspect_correction(_pm, settings[PROJECTM_AspectRatio].detail.mdz_boolswitch.switch_value);
    projectm_set_preset_locked(_pm, settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value);
    
    // Preset display settings
    projectm_set_preset_duration(_pm, settings[PROJECTM_TimeBetweenPreset].detail.mdz_slider.slider_value);//15.0);
    if (settings[PROJECTM_BlendPresets].detail.mdz_boolswitch.switch_value) projectm_set_soft_cut_duration(_pm, settings[PROJECTM_BlendTime].detail.mdz_slider.slider_value);//2.0);
    else projectm_set_soft_cut_duration(_pm, 0);//2.0);
    projectm_set_hard_cut_enabled(_pm, settings[PROJECTM_HardCutEnabled].detail.mdz_boolswitch.switch_value);
    
    projectm_set_hard_cut_duration(_pm, settings[PROJECTM_HardCutMinTime].detail.mdz_slider.slider_value);
    projectm_set_hard_cut_sensitivity(_pm, settings[PROJECTM_HardCutSensitivity].detail.mdz_slider.slider_value);
    projectm_set_beat_sensitivity(_pm, settings[PROJECTM_BeatSensitivity].detail.mdz_slider.slider_value);
    
    
    NSString *pmBundleDirText = [NSString stringWithFormat:@"%@/projectm/assets/textures",[[NSBundle mainBundle] resourcePath]];
    NSString *pmCustomDirText = [NSString stringWithFormat:@"%@/Documents%s/textures",[ModizFileHelper getAppHomeDirectory],PM_ROOT_FOLDER_CUSTOM];
    NSString *pmCustomDirSprites = [NSString stringWithFormat:@"%@/Documents%s/sprites",[ModizFileHelper getAppHomeDirectory],PM_ROOT_FOLDER_CUSTOM];
    
    mdz_pmTexturesSearchPathsNb=0;
    mdz_pmTexturesSearchPaths[mdz_pmTexturesSearchPathsNb++]=[pmBundleDirText UTF8String];
    mdz_pmTexturesSearchPaths[mdz_pmTexturesSearchPathsNb++]=[pmCustomDirText UTF8String];
    mdz_pmTexturesSearchPaths[mdz_pmTexturesSearchPathsNb++]=[pmCustomDirSprites UTF8String];
    
    projectm_set_texture_search_paths(_pm, (const char **)mdz_pmTexturesSearchPaths,mdz_pmTexturesSearchPathsNb);
    
    _pm_playlist_loadBundled=settings[PROJECTM_BundledPresets].detail.mdz_boolswitch.switch_value;
    _pm_playlist_loadCustom=settings[PROJECTM_CustomPresets].detail.mdz_boolswitch.switch_value;
    
    if ([_mdzPM_playlist getSize]==0) {
        //empty playlist, initiate with available and active presets
        if (_pm_playlist_loadBundled) [_mdzPM_playlist addItems:[pmBundledPresetsFileNode getSelectedPlaylist]];
        if (_pm_playlist_loadCustom) [_mdzPM_playlist addItems:[pmCustomPresetsFileNode getSelectedPlaylist]];
    }
    
    _pmPresetUpdateDisplayInfo=false;
    _pm_display_name_countdown=0;
    
    //    [_mdzPM_playlist loadIdlePreset];
    if ((_pm_shouldRestartAt>=0) &&(_pm_shouldRestartAt<[_mdzPM_playlist getSize])) {
        //        MDZILog("restart pm preset idx: %d",_pm_shouldRestartAt);
        [_mdzPM_playlist setPos:_pm_shouldRestartAt cut:true];
    } else {
        [_mdzPM_playlist setPos:0 cut:true];
    }
    //reset idx
    _pm_shouldRestartAt=-1;
    _pmIsInitialized=true;
    _pmFirstInitDone=true;
    
#ifdef PM_TEST_LOAD
    pm_perfTest();
#endif
}

- (void) reinitVisuVars {
    //MDZILog("reset var");
    movePx=movePy=movePxOld=movePyOld=0;
    startPx=startPy=0;
    posPx=posPy=0;
    movePx2=movePy2=movePx2Old=movePy2Old=0;
    movePinchScale=movePinchScaleOld=0;
    movePinchAngle=0;
    movePinchScaleFXMOD=0;
    sliderProgressModuleEdit=0;
    sliderProgressModuleChanged=0;
    modPatternLineSize	=0;
}

- (void) buildCommandBar:(UIToolbar *)bar isPause:(bool)isPause isSub:(bool)isSub {
    
    // When creating your bar button items with SF Symbols:
    UIImageSymbolConfiguration *config = [UIImageSymbolConfiguration configurationWithPointSize:22.0
                                                                                         weight:UIImageSymbolWeightThin
                                                                                          scale:UIImageSymbolScaleLarge];
    
    
    UIImage *playPauseImage;
    if (!isPause) playPauseImage = [UIImage systemImageNamed:@"play.fill" withConfiguration:config];
    else playPauseImage = [UIImage systemImageNamed:@"pause.fill" withConfiguration:config];
    UIImage *nextImage = [UIImage systemImageNamed:@"forward.end.fill" withConfiguration:config];
    UIImage *prevImage = [UIImage systemImageNamed:@"backward.end.fill" withConfiguration:config];
    UIImage *nextSubImage=nil;
    UIImage *prevSubImage=nil;
    if (isSub) {
        nextSubImage = [UIImage systemImageNamed:@"forward.fill" withConfiguration:config];
        prevSubImage = [UIImage systemImageNamed:@"backward.fill" withConfiguration:config];
    }
    
    
    UIBarButtonItem *itemPlayPause = [[UIBarButtonItem alloc] initWithImage:playPauseImage style:UIBarButtonItemStylePlain target:self action:(isPause?@selector(pausePushed):@selector(playPushed))];
    
    UIBarButtonItem *itemPrev = [[UIBarButtonItem alloc] initWithImage:prevImage style:UIBarButtonItemStylePlain target:self action:@selector(playPrev)];
    
    UIBarButtonItem *itemNext = [[UIBarButtonItem alloc] initWithImage:nextImage style:UIBarButtonItemStylePlain target:self action:@selector(playNext)];
    
    UIBarButtonItem *itemPrevSub=nil;
    UIBarButtonItem *itemNextSub=nil;
    if (isSub) {
        itemPrevSub = [[UIBarButtonItem alloc] initWithImage:prevSubImage style:UIBarButtonItemStylePlain target:self action:@selector(playPrevSub)];
        itemNextSub = [[UIBarButtonItem alloc] initWithImage:nextSubImage style:UIBarButtonItemStylePlain target:self action:@selector(playNextSub)];
        
        UILongPressGestureRecognizer *longPressPaPrevSGesture = [[UILongPressGestureRecognizer alloc]
                                                                 initWithTarget:self
                                                                 action:@selector(longPressPrevSubArc:)];
        UILongPressGestureRecognizer *longPressPaNextSGesture = [[UILongPressGestureRecognizer alloc]
                                                                 initWithTarget:self
                                                                 action:@selector(longPressNextSubArc:)];
        
        if ([[itemPrevSub valueForKey:@"view"] respondsToSelector:@selector(addGestureRecognizer:)]) {
            [[itemPrevSub valueForKey:@"view"] addGestureRecognizer:longPressPaPrevSGesture];
        }
        if ([[itemNextSub valueForKey:@"view"] respondsToSelector:@selector(addGestureRecognizer:)]) {
            [[itemNextSub valueForKey:@"view"] addGestureRecognizer:longPressPaNextSGesture];
        }
    }
    
    UIBarButtonItem *flexSpace = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace primaryAction:NULL];
    
    NSArray *buttonItems;
    if (isSub) {
        buttonItems = [NSArray arrayWithObjects:flexSpace,itemPrev,flexSpace,itemPrevSub,flexSpace,itemPlayPause,flexSpace,itemNextSub,flexSpace,itemNext,flexSpace,nil];
    } else {
        buttonItems = [NSArray arrayWithObjects:flexSpace,itemPrev,flexSpace,itemPlayPause,flexSpace,itemNext,flexSpace,nil];
    }
    [bar setItems:buttonItems];
    
    //to avoid flickering issue / metal view
    bar.layer.opaque = YES;
    bar.layer.shouldRasterize = YES;
    bar.layer.rasterizationScale = mScaleFactor;
    bar.layer.drawsAsynchronously = YES;
}

- (void) buildCommandBars {
    [self buildCommandBar:pauseBar isPause:true isSub:false];
    [self buildCommandBar:playBar isPause:false isSub:false];
    [self buildCommandBar:pauseBarSub isPause:true isSub:true];
    [self buildCommandBar:playBarSub isPause:false isSub:true];
}

-(UIWindow*) getWindow {
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]]) {
            UIWindowScene *wscene=(UIWindowScene *)scene;
            for (UIWindow *win in wscene.windows) {
                if (win.keyWindow) {
                    return win;
                }
            }
        }
    }
    //fallback
    return [UIApplication sharedApplication].windows.firstObject;
}

-(void) getScreenSize:(float*)scaleFactor width:(float *)devWW height:(float*)devHH {
    CGSize screenSize;
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]]) {
            UIWindowScene *wscene=(UIWindowScene *)scene;
            for (UIWindow *win in wscene.windows) {
                if (win.keyWindow) {
                    screenSize=win.screen.bounds.size;
                    safe_bottom=win.safeAreaInsets.bottom;
                    safe_top=win.safeAreaInsets.top;
                    safe_left=win.safeAreaInsets.left;
                    safe_right=win.safeAreaInsets.right;
                }
            }
        }
    }
    if (scaleFactor) *scaleFactor=[[UIScreen mainScreen] scale];
    if (devWW) *devWW=screenSize.width;
    if (devHH) *devHH=screenSize.height;
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    START_PROFILE
    
    [super viewDidLoad];
    
    mScaleInfo[0]=0;
    mScaleInfo[1]=0;
    mScaleInfo[2]=FX_AUTO_SCALING_DELAY_ZOOMIN_SLOW;
    mScaleInfo[3]=FX_AUTO_SCALING_DELAY_ZOOMIN_SLOW;
    mScaleInfo[4]=FX_AUTO_SCALING_DELAY_ZOOMIN_SLOW;
    //    if (safe_bottom>0) safe_bottom+=20;
    mScaleFactor=1.0f;
    is_iPad=false;
    is_macOS=false;
#if TARGET_OS_MACCATALYST
    is_macOS=true;
    mDeviceType=DEVICE_MACOS;
#endif
    if ([NSProcessInfo processInfo].isiOSAppOnMac) {
        is_macOS=true;
        mDeviceType=DEVICE_MACOS;
    }
    
    
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        if (!is_macOS) {
            mDeviceType=DEVICE_IPAD; //ipad
            is_iPad=true;
        }
        else mDeviceType=DEVICE_MACOS;
        UIScreen* mainscr = [UIScreen mainScreen];
        
        //UIWindow *win=[UIApplication sharedApplication].keyWindow;
        UIWindow *win;
        //win=[UIApplication sharedApplication].windows.firstObject;
        win=[self getWindow];
        
        
        //if (mainscr.bounds.size.height>mainscr.bounds.size.width) {
        if (win.bounds.size.height>win.bounds.size.width) {
            mDevice_hh=win.bounds.size.height;
            mDevice_ww=win.bounds.size.width;
            orientationHV=UIInterfaceOrientationPortrait; //(int)[[UIDevice currentDevice]orientation];
        } else {
            mDevice_ww=win.bounds.size.height;
            mDevice_hh=win.bounds.size.width;
            orientationHV=UIInterfaceOrientationLandscapeLeft; //(int)[[UIDevice currentDevice]orientation];
        }
        
        mScaleFactor=mainscr.scale;
        if (mScaleFactor>=2) {
            if (!is_macOS) mDeviceType=DEVICE_IPAD_RETINA;
        }
    } else {
        mDeviceType=DEVICE_IPHONE; //iphone
        mDevice_hh=480;
        mDevice_ww=320;
        UIScreen* mainscr = [UIScreen mainScreen];
        UIWindow *win;
        //win=[UIApplication sharedApplication].windows.firstObject;
        win=[self getWindow];
        
        
        if (win.bounds.size.height>win.bounds.size.width) {
            mDevice_hh=win.bounds.size.height;
            mDevice_ww=win.bounds.size.width;
            orientationHV=UIInterfaceOrientationPortrait; //(int)[[UIDevice currentDevice]orientation];
        } else {
            mDevice_ww=win.bounds.size.height;
            mDevice_hh=win.bounds.size.width;
            orientationHV=UIInterfaceOrientationLandscapeLeft; //(int)[[UIDevice currentDevice]orientation];
        }
        mScaleFactor=mainscr.scale;
        
        if (mScaleFactor>=2) mDeviceType=DEVICE_IPHONE_RETINA;
        
    }
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(orientationDidChange:) name:UIDeviceOrientationDidChangeNotification object:nil];
    
    
    _fx_frame_time=0;
    _fx_frame_timeOverLimitCounter=0;
    deactivateFStemp=0;
    
    //reset timer
    tgtFrameStartTime=0;
    //update displayLink refresh
    mBackground=false;
    
    sysMonitor=[[SysMonitoring alloc] init];
    sysMonitorIsActive=false;
    
    CHECK_PROFILE("step1")
    //--------------------------------//
    // OpenGL
    //--------------------------------//
    [self setupOGLView];
    [self setContextOGL];
    m_nAverageFps=0;
    //--------------------------------//
    // Texture for background view
    //--------------------------------//
    txtBGImage=0;
    txtCoverImg=0;
    coverAvailable=false;
    glGenTextures(1, &txtBGImage);
    glGenTextures(1, &txtCoverImg);
    
    CHECK_PROFILE("openGL")
    
    //
    //opengl stuff
    //Init shaders
    if (RenderUtils::RenderInit()) {
        MDZDLog("render init OK");
    }  else MDZELog("!!render init KO!!");
    
    CHECK_PROFILE("Renders")
    
    //--------------------------------//
    // ImGui init
    //--------------------------------//
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplIOS_Init();
    ImGui_ImplOpenGL3_Init();
    imGui_impl_ios=[[ImGui_ImplIOS_UI alloc] init];
    [imGui_impl_ios initTF:m_oglView];
    
    CHECK_PROFILE("ImGUI")
    
    //--------------------------------//
    mSendStatTimer=0;
    loadRequestInProgress=0;
    //NSLocale* locale = [NSLocale autoupdatingCurrentLocale];
    //located_country=[NSString stringWithString:locale.countryCode];
    
    self.navigationController.delegate = self;
    
    statusbarHidden=false;
    alertTableView=nil;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    //Font
    //    mFont=NULL;
    //    mFontPath=NULL;
    //    mFontMenu=NULL;
    //    mFontMenuPath=NULL;
    
    commandViewU.backgroundColor=[UIColor colorWithRed:0 green:0 blue:0 alpha:0.9f];
    
    labelModuleName=[[CBAutoScrollLabel alloc] init];
    //labelModuleName.backgroundColor=[UIColor blackColor];
    labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,22);
    labelModuleName.textColor = [UIColor colorWithRed:0.95 green:0.95 blue:0.99 alpha:1.0];
    [labelModuleName setFont:[UIFont systemFontOfSize:17]];
    labelModuleName.textColor = [UIColor whiteColor];
    labelModuleName.labelSpacing = 35; // distance between start and end labels
    labelModuleName.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    labelModuleName.scrollSpeed = 30; // pixels per second
    labelModuleName.textAlignment = NSTextAlignmentCenter; // centers text when no auto-scrolling is applied
    labelModuleName.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    
    labelArtist=[[CBAutoScrollLabel alloc] init];
    //labelArtist.backgroundColor=[UIColor blackColor];
    labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,18);
    labelArtist.textColor = [UIColor colorWithRed:0.85 green:0.85 blue:0.90 alpha:1.0];
    [labelArtist setFont:[UIFont systemFontOfSize:12]];
    labelArtist.textColor = [UIColor whiteColor];
    labelArtist.labelSpacing = 35; // distance between start and end labels
    labelArtist.pauseInterval = 3.7; // seconds of pause before scrolling starts again
    labelArtist.scrollSpeed = 30; // pixels per second
    labelArtist.textAlignment = NSTextAlignmentCenter; // centers text when no auto-scrolling is applied
    labelArtist.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
    
    labelModuleName.userInteractionEnabled = YES;
    UITapGestureRecognizer *tapGesture =
    [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(titleTap:)];
    [labelModuleName addGestureRecognizer:tapGesture];
    
    labelModuleName.text=@"No file selected";
    labelArtist.text=@"";
    
    labelContainer = [[UIView alloc] init];
    [labelContainer addSubview:labelModuleName];
    [labelContainer addSubview:labelArtist];

    labelContainer.frame=CGRectMake(0,0,self.view.frame.size.width-128,40);
    self.navigationItem.titleView = labelContainer;

    //self.navigationItem.titleView=labelModuleName;
    //self.navigationItem.title=@"No file selected";
    //    self.navigationItem.backBarButtonItem.title=@"dd";
    
    
    mLoadIssueMessage=0;
    curSongLength=0;
    
    repeatingTimer=nil;
    
    CHECK_PROFILE("various1")
    
    default_cover=[UIImage imageNamed:@"AppStore512.png"];
    //[default_cover retain];
    artwork=nil;
    
    cover_view.layer.shadowColor = [UIColor blackColor].CGColor;
    cover_view.layer.shadowOffset = CGSizeMake(1, 2);
    cover_view.layer.shadowOpacity = 1;
    cover_view.layer.shadowRadius = 2.0;
    cover_view.clipsToBounds = NO;
    
    //Voices control
    bShowVC=false;
    
    //EQ
    eqVC=nil;
    bShowEQ=false;
    
    [sliderProgressModule setThumbImage:[UIImage imageNamed:@"slider.png" ] forState:UIControlStateNormal];
    
    CHECK_PROFILE("various2a")
    
    shouldRestart=1;
    
    gifAnimation=nil;
    cover_view.contentMode=UIViewContentModeScaleAspectFit;
    cover_viewBG.contentMode=UIViewContentModeScaleToFill;
    cover_viewAll.contentMode=UIViewContentModeScaleToFill;
    
    //build various bars
    [self buildCommandBars];
    
    mPlaylist=(t_plPlaylist_entry*)calloc(MAX_PL_ENTRIES,sizeof(t_plPlaylist_entry));
    
    CHECK_PROFILE("various2b")
    
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"music.note.list"] style:UIBarButtonItemStylePlain target:self action:@selector(showPlaylist)];
    self.navigationItem.rightBarButtonItem = item;
    
    mHasFocus=0;
    mShouldUpdateInfos=0;
    mPaused=1;
    
    
    //reset idle timer to settings value
    [[UIApplication sharedApplication] setIdleTimerDisabled:settings[GLOB_NoScreenAutoLock].detail.mdz_boolswitch.switch_value];
    
    isRecordingScreen=RS_NOT_RECORDING;
    
    CHECK_PROFILE("various2")
    
    
    
    CHECK_PROFILE("various3")
    /* iPhone Simulator == i386
     iPhone == iPhone1,1             //Slow
     3G iPhone == iPhone1,2          //Slow
     3GS iPhone == iPhone2,1
     4 iPhone == iPhone3,1
     1st Gen iPod == iPod1,1         //Slow
     2nd Gen iPod == iPod2,1
     3rd Gen iPod == iPod3,1
     */
    
    NSString *strMachine=[self machine];
    
    self.hidesBottomBarWhenPushed = YES;
    
    CHECK_PROFILE("various4")
    
    //    [[infoButton layer] setCornerRadius:10.0];
    /* Popup stuff */
    [[infoMsgView layer] setCornerRadius:5.0];
    [[infoMsgView layer] setBorderWidth:2.0];
    [[infoMsgView layer] setBorderColor:[[UIColor colorWithRed: 0.95f green: 0.95f blue: 0.95f alpha: 1.0f] CGColor]];   //Adding Border color.
    infoMsgView.hidden=YES;
    
    /**/
    
    ratingImg[0] = @"heart-empty.png";
    ratingImg[1] = @"heart-half-filled.png";
    ratingImg[2] = @"heart-filled.png";
    
    //    for (int i=0;i<MAX_MENU_FX_STRING;i++) viewTapInfoStr[i]=NULL;
    
    
    mPlaylist_size=0;
    mIsPlaying=FALSE;
    oglViewFullscreenChanged=0;
    mOglViewIsHidden=YES;
    
    infoZoom.hidden=NO;
    infoUnzoom.hidden=YES;
    
    mRestart=0;
    mRestart_sub=0;
    
    [sliderProgressModule.layer setCornerRadius:8.0];
    [labelSeeking.layer setCornerRadius:8.0];
    [labelTime.layer setCornerRadius:8.0];
    //[commandViewU.layer setCornerRadius:8.0];
    
    CHECK_PROFILE("various5")
    
    textMessage.font = [UIFont fontWithName:@"Courier-Bold" size:14];
    textMessage.editable=NO;
    
    
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
    
    //init pattern notes buffer
    //    for (int i=0;i<128;i++) {
    //        mText[i]=nil;
    //    }
    //    for (int i=0;i<128;i++) {
    //        mTextLine[i]=nil;
    //    }
    
    //    mHeader=nil;
    CHECK_PROFILE("various6")
    mplayer = [[ModizMusicPlayer alloc] initMusicPlayer];
    
    CHECK_PROFILE("musicplayer")
    
    m_oglView.multipleTouchEnabled = true;
    
    // Create gesture recognizer
    UITapGestureRecognizer *glViewOneFingerOneTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(glViewOneFingerOneTap:)];
    // Set required taps and number of touches
    [glViewOneFingerOneTap setNumberOfTapsRequired:1];
    [glViewOneFingerOneTap setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewOneFingerOneTap];
    
    
    // Create gesture recognizer
    UITapGestureRecognizer *glViewTwoFingersTouch = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(glViewTwoFingersTouch:)];
    // Set required taps and number of touches
    [glViewTwoFingersTouch setNumberOfTapsRequired:2];
    [glViewTwoFingersTouch setNumberOfTouchesRequired:2];
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewTwoFingersTouch];
    
    UILongPressGestureRecognizer *glViewLongPressTouch = [[UILongPressGestureRecognizer alloc]
                                                          initWithTarget:self
                                                          action:@selector(glViewLongPresstouch:)];
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewLongPressTouch];
    
    // Create gesture recognizer
    UIPanGestureRecognizer *glViewPanGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(glViewPanGesture:)];
    // Set required taps and number of touches
    [glViewPanGesture setMinimumNumberOfTouches:1];
    [glViewPanGesture setMaximumNumberOfTouches:1];
    
    glViewPanGesture.allowedScrollTypesMask = UIScrollTypeMaskAll;
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewPanGesture];
    
    // Create gesture recognizer
    UIPanGestureRecognizer *glViewPan2Gesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(glViewPan2Gesture:)];
    // Set required taps and number of touches
    [glViewPan2Gesture setMinimumNumberOfTouches:2];
    [glViewPan2Gesture setMaximumNumberOfTouches:2];
    
    glViewPan2Gesture.allowedScrollTypesMask = UIScrollTypeMaskAll;
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewPan2Gesture];
    
    // Create gesture recognizer
    UIPinchGestureRecognizer *glViewPinchGesture = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(glViewPinchGesture:)];
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewPinchGesture];
    
    UIHoverGestureRecognizer *glHoverGesture = [[UIHoverGestureRecognizer alloc] initWithTarget:self action:@selector(glViewHoverGesture:)];
    [m_oglView addGestureRecognizer:glHoverGesture];
    
    
    //BButton
    [btnShowVoices setStyle:BButtonStyleBootstrapV2];
    [btnShowArcList setStyle:BButtonStyleBootstrapV2];
    [btnShowSubSong setStyle:BButtonStyleBootstrapV2];
    [btnRecordScreen setStyle:BButtonStyleBootstrapV2];
    [btnSaveFile setStyle:BButtonStyleBootstrapV2];
    [btnRadioPrevList setStyle:BButtonStyleBootstrapV2];
    [btnAddToPl setStyle:BButtonStyleBootstrapV2];
    
    [btnShowVoices setType:BButtonTypeInverse];
    [btnShowArcList setType:BButtonTypeInverse];
    [btnShowSubSong setType:BButtonTypeInverse];
    [btnRecordScreen setType:BButtonTypeInverse];
    [btnSaveFile setType:BButtonTypeInverse];
    [btnRadioPrevList setType:BButtonTypeInverse];
    [btnAddToPl setType:BButtonTypeInverse];
    
    [btnShowVoices addAwesomeIcon:FAIconMusic beforeTitle:YES];
    [btnShowArcList addAwesomeIcon:FAIconArchive beforeTitle:YES];
    [btnShowSubSong addAwesomeIcon:FAIconStackOverflow beforeTitle:YES];
    [btnRecordScreen addAwesomeIcon:FAIconVideoCamera beforeTitle:YES];
    [btnSaveFile addAwesomeIcon:FAIconDownload beforeTitle:YES];
    [btnRadioPrevList addAwesomeIcon:FAIconHistory beforeTitle:YES];
    [btnAddToPl addAwesomeIcon:FAIconPlus beforeTitle:YES];
    
    btnShowVoices.hidden=false;
    btnRecordScreen.hidden=false;
    btnRecordScreen.enabled=true;
    btnRecordScreen.selected=false;
    btnSaveFile.hidden=false;
    btnSaveFile.enabled=true;
    btnSaveFile.selected=false;
    btnRadioPrevList.hidden=false;
    btnRadioPrevList.enabled=true;
    btnRadioPrevList.selected=false;
    bRSactive=false;
    
    
    [infoButton setStyle:BButtonStyleBootstrapV2];
    [infoButton setType:BButtonTypeInverse];
    [infoButton addAwesomeIcon:FAIconInfoCircle beforeTitle:YES];
    
    [eqButton setStyle:BButtonStyleBootstrapV2];
    [eqButton setType:BButtonTypeInverse];
    [eqButton setTitleColor:(nvdsp_EQ?[UIColor whiteColor]:[UIColor grayColor]) forState:UIControlStateNormal];
    [eqButton addAwesomeIcon:FAIconSliders beforeTitle:YES];
    
    CHECK_PROFILE("various7")
    
    //Visualization
    /* Set Starting Angle To Zero */
    angle=0.0f;
    /* Create Our Empty Texture */
    
    tim_midifx_note_range=DEFAULT_VISIBLE_MIDI_NOTES*mDevice_ww/640;
    if (tim_midifx_note_range>MAX_VISIBLE_MIDI_NOTES) tim_midifx_note_range=MAX_VISIBLE_MIDI_NOTES;
    if (tim_midifx_note_range<MIN_VISIBLE_MIDI_NOTES) tim_midifx_note_range=MIN_VISIBLE_MIDI_NOTES;
    
    movePinchScaleFXMID=(DEFAULT_VISIBLE_MIDI_NOTES-tim_midifx_note_range)/64.0;
    movePinchScaleFXMOD=0;
    
    tim_midifx_note_offset_reset=true;
    tim_midifx_length=MAX_MIDIFX_LENGTH;
    
    prollfx_note_range=DEFAULT_VISIBLE_MIDI_NOTES*mDevice_ww/640;
    if (prollfx_note_range>MAX_VISIBLE_MIDI_NOTES) prollfx_note_range=MAX_VISIBLE_MIDI_NOTES;
    if (prollfx_note_range<MIN_VISIBLE_MIDI_NOTES) prollfx_note_range=MIN_VISIBLE_MIDI_NOTES;
    
    movePinchScaleFXPRoll=(DEFAULT_VISIBLE_MIDI_NOTES-prollfx_note_range)/64.0;
    
    prollfx_note_offset_reset=true;
    prollfx_length=MAX_MIDIFX_LENGTH;
    
    clearAudioFXbuffer=true;
    _seekRequested=-1;
    
    CHECK_PROFILE("various8")
    PMenu::playerMenuInit();
    
    CHECK_PROFILE("PMenu")
    //
    //Init colors
    [SettingsGenViewController pianomidiGenSystemColor:0 color_idx:-1 color_buffer:data_midifx_pal1];
    [SettingsGenViewController pianomidiGenSystemColor:1 color_idx:-1 color_buffer:data_midifx_pal2];
    [SettingsGenViewController pianomidiGenSystemColor:2 color_idx:-1 color_buffer:data_midifx_pal3];
    
    [SettingsGenViewController oscilloGenSystemColor:0 color_idx:-1 color_buffer:m_voice_oscillo_pal1];
    [SettingsGenViewController oscilloGenSystemColor:1 color_idx:-1 color_buffer:m_voice_oscillo_pal2];
    [SettingsGenViewController oscilloGenSystemColor:2 color_idx:-1 color_buffer:m_voice_oscillo_pal3];
    
    //init play/pause status for button
    self.pauseBar.hidden=YES;
    self.playBar.hidden=NO;
    self.pauseBarSub.hidden=YES;
    self.playBarSub.hidden=YES;
    
    // Fix truncated system icons in toolbar buttons
    
    [self updateBarPos];
    mplayer.bGlobalAudioPause=1;
    //init mod player var
    
    
    //FFT
    for (int i=0;i<SPECTRUM_BANDS;i++)
        for (int j=0;j<8;j++) {
            real_spectrumSumL[i][j]=real_spectrumSumR[i][j]=0;
        }
    fftAccel = new FFTAccelerate(SOUND_BUFFER_SIZE_SAMPLE);
    
    fft_frequency = (float *)malloc(sizeof(float)*SOUND_BUFFER_SIZE_SAMPLE);
    fft_frequencyAvg = (float *)malloc(sizeof(float)*SPECTRUM_BANDS);
    fft_freqAvgCount = (int *)malloc(sizeof(int)*SPECTRUM_BANDS);
    fft_time = (float *)malloc(sizeof(float)*SOUND_BUFFER_SIZE_SAMPLE);
    
    memset(fxSlot,0,sizeof(fxSlot));
    
    _pm_shouldRestartAt=-1;
    CHECK_PROFILE("various9")
    if ([self checkFlagOnStartup]) {
        [self loadSettings:1];
        mShouldUpdateInfos=1;
    } else [self loadSettings:0];
    
    CHECK_PROFILE("load settings")
    //---------------------------------
    //---------------------------------
    mdzRenderInProgress=false;
    _pmIsInitialized=false;
    _pmFirstInitDone=false;
    
    _pmPresetNewLoaded=false;
    float _pmScaleFactor=1<<settings[PROJECTM_Quality].detail.mdz_switch.switch_value;
    _pmCanvasWidth=m_oglView.frame.size.width*glScaleFactor/_pmScaleFactor;
    _pmCanvasHeight=m_oglView.frame.size.height*glScaleFactor/_pmScaleFactor;
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
        //--------------------------------//
        // Build ProjectM presets directories structure
        //--------------------------------//
        {
            START_PROFILE
            buildPresetDirStructure();
            CHECK_PROFILE("parsed bundled and custom folders")
            //--------------------------------//
            // ProjectM
            //--------------------------------//
            [self pmInit];
            CHECK_PROFILE("pmInit")
            END_PROFILE
        }
        
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            
        }];
    });
    
    for (int i=0;i<mPlaylist_size;i++) mPlaylist[i].cover_flag=-1;
    
    [self.view bringSubviewToFront:infoMsgView];
    
    //update displayLink refresh
    //    m_displayLink=nil;
    m_displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(doFrame)];
    m_displayLink.preferredFramesPerSecond = (settings[GLOB_FXFPS].detail.mdz_switch.switch_value?60:30); //60 or 30 fps depending on device speed iPhone
    [m_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    
    //------------------------------------------------------
    
    modPatternWindowSize=0;
    modPatternLineSize=0;
    visibleChan=SOUND_MAXMOD_CHANNELS;
    
    modpat_curTheme=modpat_themesList[(settings[GLOB_FXMODPattern_Theme].detail.mdz_switch.switch_value)&modpat_themesNb];
    
    _shiftModeOn=0;
    
    pmenu_fade=0;
    pmenu_show=0;
    memset(oglv_corner_fade,0,sizeof(oglv_corner_fade));
    
    _mdz_display_songinfo_countdown=0;
    _mdz_FX_GuiMessageStr[0]=0;
    _mdz_FX_GuiMessage_fade=0;
    
    
    radioSource = [[RadioSource alloc] init];
    radioSource.detailVC=self;
    radioView=nil;
    bShowRadio=false;
    
    //    [super viewDidLoad];
    END_PROFILE
    
}

#if TARGET_OS_MACCATALYST

- (void)mouseDidMove:(UIGestureRecognizer *)gesture {
    // Montrer le curseur
    [NSCursor unhide];
    
    // Annuler le timer précédent
    [self.mouseHideTimer invalidate];
    
    // Créer un nouveau timer pour cacher après 2 secondes d'inactivité
    self.mouseHideTimer = [NSTimer scheduledTimerWithTimeInterval:2.0
                                                           target:self
                                                         selector:@selector(hideCursor)
                                                         userInfo:nil
                                                          repeats:NO];
}


- (BOOL)isFullscreen {
    if (@available(iOS 13.0, *)) {
        UIWindowScene *windowScene = (UIWindowScene *)self.view.window.windowScene;
        if (windowScene) {
            // Sur Catalyst, utiliser directement les dimensions de la fenêtre
            // et vérifier via la titlebar visibility
            
            CGRect windowFrame = windowScene.coordinateSpace.bounds;
            
            // Méthode 1: Vérifier si on a une titlebar cachée (iOS 16+)
            //            if (@available(iOS 16.0, *)) {
            //                if (windowScene.titlebar) {
            //                    BOOL titlebarHidden = (windowScene.titlebar.titleVisibility == UITitlebarTitleVisibilityHidden);
            //                    MDZILog("fs: titlebar hidden=%d", titlebarHidden);
            //                    return titlebarHidden;
            //                }
            //            }
            
            // Méthode 2: Comparer avec la taille maximale disponible
            // En plein écran, la fenêtre devrait être > 1500pts de large sur un écran standard
            BOOL likelyFullscreen = (windowFrame.size.width > 1500.0 &&
                                     windowFrame.size.height > 1000.0);
            
            //            MDZILog("fs: window=%f x %f, likely fullscreen=%d",windowFrame.size.width, windowFrame.size.height, likelyFullscreen);
            
            return likelyFullscreen;
        }
    }
    return NO;
}

- (void)hideCursor {
    if ([self isFullscreen]) [NSCursor hide];
}

#endif

- (void)dealloc {
    [waitingView removeFromSuperview];
    //[waitingView release];
    
    //if (locationLastUpdate) [locationLastUpdate release];
    
    [repeatingTimer invalidate];
    repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
    
    [mplayer Stop];
    //[mplayer release];
    
    for (int i=0;i<mPlaylist_size;i++) {
        //[mPlaylist[i].mPlaylistFilename autorelease];
        //[mPlaylist[i].mPlaylistFilepath autorelease];
    }
    free(mPlaylist);
    mPlaylist_size=0;
    
    //FFT
    delete(fftAccel);
    free(fft_frequency);
    free(fft_frequencyAvg);
    free(fft_freqAvgCount);
    free(fft_time);
    
    
    PMenu::playerMenuShutdown();
    
    //[super dealloc];
}


-(void) enterBackground {
    mBackground=true;
    mBackground_oglViewWasHidden=m_oglView.hidden;
    m_oglView.hidden=true;
    if (m_displayLink) {
        [m_displayLink invalidate];
        m_displayLink=nil;
    }
    if (mHasFocus) {
        mShouldHaveFocusAfterBackground=1;
        //[self viewWillDisappear:NO];
    } else mShouldHaveFocusAfterBackground=0;
    
    if (labelModuleName) [labelModuleName setPaused:true];
    
    //Deactivate updateInfos timer
    //[repeatingTimer invalidate];
    //repeatingTimer = nil;
    
    //Release ProjectM
    //[self pmRelease];
}

-(void) enterForeground {
    if (mShouldHaveFocusAfterBackground) {
        //[self viewWillAppear:YES];
    }
    //reset timer
    tgtFrameStartTime=0;
    mBackground=false;
    m_oglView.hidden=mBackground_oglViewWasHidden;
    
    //Reactivate updateInfos timer
    //if ([mplayer isPlaying]) repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.1f target:self selector:@selector(updateInfos:) userInfo:nil repeats: YES];
    
    if (labelModuleName) [labelModuleName setPaused:false];
    
    //Init ProjectM
    //if (_pmFirstInitDone && (_pmIsInitialized==false)) [self pmInit];
    
    //Build displaylink if needed
    if (_pmFirstInitDone && (m_displayLink==nil)) {
        m_displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(doFrame)];
        m_displayLink.preferredFramesPerSecond = (settings[GLOB_FXFPS].detail.mdz_switch.switch_value?60:30); //60 or 30 fps depending on device speed iPhone
        [m_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    }
}

- (void)viewWillLayoutSubviews {
    // Update safe area insets from the view itself, which is more reliable than keyWindow
    // especially on iOS 15 where keyWindow may not have correct insets during initial layout
    safe_bottom = self.view.safeAreaInsets.bottom;
    safe_top = self.view.safeAreaInsets.top;
    safe_left = self.view.safeAreaInsets.left;
    safe_right = self.view.safeAreaInsets.right;
    
    //    if (safe_bottom>0) safe_bottom+=20;
    
    UIWindow *win;//=[UIApplication sharedApplication].keyWindow;
    win=[self getWindow];
    
    // Update device dimensions and orientation based on current view bounds
    // This ensures we always have the correct dimensions when layout is triggered
    CGSize viewSize = win.bounds.size;// self.view.bounds.size;
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        if (viewSize.height>viewSize.width) {
            mDevice_hh=viewSize.height+(!deactivateFStemp && settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value?0:68);
            mDevice_ww=viewSize.width;
            orientationHV=UIInterfaceOrientationPortrait;
        } else {
            mDevice_ww=viewSize.height+(!deactivateFStemp && settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value?0:68);
            mDevice_hh=viewSize.width;
            orientationHV=UIInterfaceOrientationLandscapeLeft;
        }
    } else {
        // iPhone
        if (viewSize.height>viewSize.width) {
            mDevice_hh=viewSize.height;
            mDevice_ww=viewSize.width;
            orientationHV=UIInterfaceOrientationPortrait;
        } else {
            mDevice_ww=viewSize.height;
            mDevice_hh=viewSize.width;
            orientationHV=UIInterfaceOrientationLandscapeLeft;
        }
    }
    
    [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
    
    shouldUpdateCoverTexture=1;
    
    //AppDelegate_Phone *app_delegate=(AppDelegate_Phone *)[[UIApplication sharedApplication] delegate];
    //CGRect frame = [[app_delegate modizerWin] frame];
    
    [super viewWillLayoutSubviews];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    //    if (mOglViewIsHidden==NO) {
    //        //YOYOFR HACK to remove one day (maybe after switch to Metal ?)
    //        //on macos, when switching to full screen size, a lag appears if opengl view is displayed
    //        //so remove it for 1s
    //        mOglViewIsHidden=YES;
    //        [self checkGLViewCanDisplay];
    //
    //        NSTimeInterval delayInSeconds = 0.1;
    //        dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
    //        dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
    //            mOglViewIsHidden=NO;
    //            [self checkGLViewCanDisplay];
    //        });
    //    }
    
    
    labelContainer.frame=CGRectMake(0,0,size.width-128,40);
    if (labelArtist.text && [labelArtist.text length]) {
        labelModuleName.frame=CGRectMake(0,0,size.width-128,22);
        labelArtist.frame=CGRectMake(0,22,size.width-128,18);
    } else {
        labelModuleName.frame=CGRectMake(0,0,size.width-128,40);
        labelArtist.frame=CGRectMake(0,22,size.width-128,0);
    }
    
    
    // Update device dimensions for both iPad AND iPhone
    // This was previously only updating for iPad, causing rotation issues on iPhone (especially iOS 15.5)
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        if (size.height>size.width) {
            mDevice_hh=size.height+(!deactivateFStemp && settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value?0:68);
            mDevice_ww=size.width;
            orientationHV=UIInterfaceOrientationPortrait; //(int)[[UIDevice currentDevice]orientation];
        } else {
            mDevice_ww=size.height+(!deactivateFStemp && settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value?0:68);
            mDevice_hh=size.width;
            orientationHV=UIInterfaceOrientationLandscapeLeft; //(int)[[UIDevice currentDevice]orientation];
        }
    } else {
        // iPhone: update dimensions based on new size
        if (size.height>size.width) {
            mDevice_hh=size.height;
            mDevice_ww=size.width;
            orientationHV=UIInterfaceOrientationPortrait;
        } else {
            mDevice_ww=size.height;
            mDevice_hh=size.width;
            orientationHV=UIInterfaceOrientationLandscapeLeft;
        }
    }
    
    [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
    
    //Should recompute bg texture after resize
    shouldUpdateCoverTexture=1;
    
    //[waitingView setNeedsLayout]
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleLightContent;
}

- (BOOL)prefersStatusBarHidden {
    return statusbarHidden;
}


- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    // Check if we're in sidebar mode
    // Find TabBarController via window's root view controller
    BOOL isInSidebarMode = NO;
    UIViewController *tabBarVC = nil;
    
    // Get the window's root view controller
    UIWindow *window = self.view.window;
    if (window == nil) {
        // Try to get the key window
        for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
            if ([scene isKindOfClass:[UIWindowScene class]]) {
                UIWindowScene *windowScene = (UIWindowScene *)scene;
                for (UIWindow *w in windowScene.windows) {
                    if (w.isKeyWindow) {
                        window = w;
                        break;
                    }
                }
            }
            if (window) break;
        }
    }
    
    tabBarVC = window.rootViewController;
    
    if (tabBarVC) {
    }
    
    if (tabBarVC != nil && [tabBarVC isKindOfClass:[UITabBarController class]]) {
        if (@available(iOS 18.0, *)) {
            if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad) {
                tabBarVC.traitOverrides.horizontalSizeClass = UIUserInterfaceSizeClassCompact;
            }
        }
        // Check if we're in sidebar mode
        if ([tabBarVC respondsToSelector:@selector(catalystSplitViewController)]) {
            id splitVC = [tabBarVC performSelector:@selector(catalystSplitViewController)];
            if (splitVC != nil) {
                isInSidebarMode = YES;
                // Hide the mini-player when showing the full player
                if ([tabBarVC respondsToSelector:@selector(hideSharedMiniPlayer)]) {
                    [tabBarVC performSelector:@selector(hideSharedMiniPlayer)];
                }
            } else {
            }
        }
    } else {
    }
    
    // Only set delegate if NOT in sidebar mode
    // In sidebar mode, the tab bar controller manages navigation delegate
    if (!isInSidebarMode) {
        self.navigationController.delegate = self;
    }
    
    deactivateFStemp=0;
    
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        //ipad
        UIScreen* mainscr = [UIScreen mainScreen];
        
        UIWindow *win;//=[UIApplication sharedApplication].keyWindow;
        win=[self getWindow];
        
        //if (mainscr.bounds.size.height>mainscr.bounds.size.width) {
        if (win.bounds.size.height>win.bounds.size.width) {
            mDevice_hh=win.bounds.size.height;
            mDevice_ww=win.bounds.size.width;
            orientationHV=UIInterfaceOrientationPortrait; //(int)[[UIDevice currentDevice]orientation];
        } else {
            mDevice_ww=win.bounds.size.height;//-(is_macOS?60:0);
            mDevice_hh=win.bounds.size.width;
            orientationHV=UIInterfaceOrientationLandscapeLeft; //(int)[[UIDevice currentDevice]orientation];
        }
    } else {
        //iphone
        mDevice_hh=480;
        mDevice_ww=320;
        UIScreen* mainscr = [UIScreen mainScreen];
        UIWindow *win;//=[UIApplication sharedApplication].keyWindow;
        win=[self getWindow];
        if (win.bounds.size.height>win.bounds.size.width) {
            mDevice_hh=win.bounds.size.height;
            mDevice_ww=win.bounds.size.width;
            orientationHV=UIInterfaceOrientationPortrait; //(int)[[UIDevice currentDevice]orientation];
        } else {
            mDevice_ww=win.bounds.size.height;
            mDevice_hh=win.bounds.size.width;
            orientationHV=UIInterfaceOrientationLandscapeLeft; //(int)[[UIDevice currentDevice]orientation];
        }
        mScaleFactor=mainscr.scale;
        
        //if (mScaleFactor>=2) mDeviceType=2;
    }
    
    // Use self.view.safeAreaInsets for more reliable results, especially on iOS 15
    // where keyWindow may not have correct safe area insets at this point in the lifecycle
    safe_bottom=self.view.safeAreaInsets.bottom;
    safe_top=self.view.safeAreaInsets.top;
    safe_left=self.view.safeAreaInsets.left;
    safe_right=self.view.safeAreaInsets.right;
    
    
    //    if (safe_bottom>0) safe_bottom+=20;
    
    
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (alertTableView) [alertTableView reloadData];
    
    alertCannotPlay_displayed=0;
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent animated:YES];
    
    labelContainer.frame=CGRectMake(0,0,self.view.frame.size.width-128,40);
    if (labelArtist.text && [labelArtist.text length]) {
        labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,22);
        labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,18);
    } else {
        labelModuleName.frame=CGRectMake(0,0,self.view.frame.size.width-128,40);
        labelArtist.frame=CGRectMake(0,22,self.view.frame.size.width-128,0);
    }
    
    
    //eq
    eqVC=nil;
    [eqButton setTitleColor:(nvdsp_EQ?[UIColor whiteColor]:[UIColor grayColor]) forState:UIControlStateNormal];
    [eqButton setTitleColor:(nvdsp_EQ?[UIColor whiteColor]:[UIColor grayColor]) forState:UIControlStateHighlighted];
    
    [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateNormal];
    [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateHighlighted];
    
    [btnAddToPl setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    
    [btnSaveFile setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    if ([radioSource getHistorySize]>0) [btnRadioPrevList setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    else [btnRadioPrevList setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    
    if (mPlaylist_size) {
        for (int i=0;i<mPlaylist_size;i++) {  //reset rating to force resynchro (for ex, user delted an entry in 'favorites' list, thus reseting the rating for a given file
            mPlaylist[i].mPlaylistRating=-1;
        }
        
        //update rating (-1 => get current value from DB)
        mPlaylist[mPlaylist_pos].mPlaylistRating=[self getCurrentRating];
        
        //Check rating for current entry
        
        [self showRating:mPlaylist[mPlaylist_pos].mPlaylistRating];
        //update playlist
        /*        NSIndexPath *myindex=[[[NSIndexPath alloc] initWithIndex:0] autorelease];
         [self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:FALSE scrollPosition:UITableViewScrollPositionMiddle];*/
    }
    [self checkNewCover];
    
    //update play/pause bars...
    
    self.pauseBarSub.hidden=YES;
    self.playBarSub.hidden=YES;
    self.pauseBar.hidden=YES;
    self.playBar.hidden=YES;
    if (mPaused) {
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.playBarSub.hidden=NO;
        else self.playBar.hidden=NO;
    } else {
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.pauseBarSub.hidden=NO;
        else self.pauseBar.hidden=NO;
    }
    
    //get ogl context & bind
    [self setContextOGL];
    
    [self updateBarPos];
    //Hack to allow UIToolbar to be set up correctly
    if (((UIInterfaceOrientation)orientationHV==UIInterfaceOrientationPortrait) || ((UIInterfaceOrientation)orientationHV==UIInterfaceOrientationPortraitUpsideDown) ) {
        [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
    } else {
        [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
    }
    
    self.previousAppearance = self.navigationController.navigationBar.standardAppearance;
    // Set black appearance
    UINavigationBarAppearance *appearance = [[UINavigationBarAppearance alloc] init];
    [appearance configureWithOpaqueBackground];
    appearance.backgroundColor = [UIColor blackColor];
    appearance.titleTextAttributes = @{NSForegroundColorAttributeName: [UIColor whiteColor]};
    
    self.navigationController.navigationBar.standardAppearance = appearance;
    self.navigationController.navigationBar.scrollEdgeAppearance = appearance;
    self.navigationController.navigationBar.compactAppearance = appearance;
    
    MIDIFX_OFS=(settings[GLOB_FXFPS].detail.mdz_switch.switch_value?MIDIFX_OFS_60FPS:MIDIFX_OFS_30FPS);
    
    _pm_fps=settings[GLOB_FXFPS].detail.mdz_switch.switch_value?60:30;
    if (settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==2) _pmPresetUpdateDisplayInfo=true; //Force a (re)display
    
    movePxMID=movePyMID=0;
    movePxPRoll=movePyPRoll=0;
    movePMnomore=0;
    
    panGestureHover=panGestureWheel=panGesture1Tap=0;
    
    tgtFrameCnt=0;
    
    //clean key status as changing VC loses track of key pressed/released
    [mac_key_released removeAllObjects];
    [mac_key_pressed removeAllObjects];
    ImGui_ImplIOS_ResetKeyMouse();
    
    //Display reminder / access to FX view
    [self oglButtonMessage];
    
    //Displaylink: update FPS
    if (m_displayLink) {
        m_displayLink.preferredFramesPerSecond = (settings[GLOB_FXFPS].detail.mdz_switch.switch_value?60:30); //60 or 30 fps depending on device speed iPhone
    }
    
    //check if in radio mode or not
    if ([radioSource isActive]) {
        UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"dot.radiowaves.up.forward"] style:UIBarButtonItemStylePlain target:self action:@selector(showRadioPopup)];
        self.navigationItem.rightBarButtonItem = item;
        [self updRadioInfo];
    } else {
        UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:@"music.note.list"] style:UIBarButtonItemStylePlain target:self action:@selector(showPlaylist)];
        self.navigationItem.rightBarButtonItem = item;
    }
    if ([radioSource isActive]) {
        if ([radioSource isInLibrary:0]) [btnSaveFile setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
        else [btnSaveFile setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
        [self updRadioInfo];
    }
}

- (void)oglButtonMessage {
    //reset attributes
    oglButton.titleLabel.alpha=1.0;
    oglButton.transform = CGAffineTransformIdentity; // Reset transform first
    [UIView animateWithDuration:1.0 delay:1.0 options:0
                     animations:^{
        //fade out & zoom
        self.oglButton.titleLabel.alpha=0.0;
        self.oglButton.transform = CGAffineTransformMakeScale(1.5, 1.5); // Zoom to 150%
    } completion:^(BOOL finished) {
        //reset transfo
        self.oglButton.transform = CGAffineTransformIdentity; // Reset transform first
    }];
}

- (void)checkNewCover {
    if (mPlaylist_size) {
        NSString *filePathTmp=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
        const char *tmp_str=[mPlaylist[mPlaylist_pos].mPlaylistFilepath UTF8String];
        char tmp_str_copy[1024];
        int i=0;
        while (tmp_str[i]) {
            if (tmp_str[i]=='@') {
                memcpy(tmp_str_copy,tmp_str,i);
                tmp_str_copy[i]=0;
                filePathTmp=[NSString stringWithFormat:@"%s",tmp_str_copy];
                break;
            }
            if (tmp_str[i]=='?') {
                memcpy(tmp_str_copy,tmp_str,i);
                tmp_str_copy[i]=0;
                filePathTmp=[NSString stringWithFormat:@"%s",tmp_str_copy];
                break;
            }
            i++;
        }
        
        [self checkForCover:filePathTmp];
    }
}


- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    // Check if we're in sidebar mode
    BOOL isInSidebarMode = NO;
    UIViewController *tabBarVC = nil;
    
    // Get window's root view controller
    UIWindow *window = self.view.window;
    if (window == nil) {
        // Try to get the key window
        for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
            if ([scene isKindOfClass:[UIWindowScene class]]) {
                UIWindowScene *windowScene = (UIWindowScene *)scene;
                for (UIWindow *w in windowScene.windows) {
                    if (w.isKeyWindow) {
                        window = w;
                        break;
                    }
                }
            }
            if (window) break;
        }
    }
    
    tabBarVC = window.rootViewController;
    
    if (tabBarVC != nil && [tabBarVC isKindOfClass:[UITabBarController class]]) {
        if (@available(iOS 18.0, *)) {
            if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad) {
                tabBarVC.traitOverrides.horizontalSizeClass = UIUserInterfaceSizeClassRegular;
            }
        }
        // Check if we're in sidebar mode
        if ([tabBarVC respondsToSelector:@selector(catalystSplitViewController)]) {
            id splitVC = [tabBarVC performSelector:@selector(catalystSplitViewController)];
            if (splitVC != nil) {
                isInSidebarMode = YES;
                // Don't show the mini-player here - let the appearing view controller handle it
                // This prevents duplicate mini-players when navigating back
            }
        }
    }
    
    // Only set delegate if NOT in sidebar mode
    // In sidebar mode, the tab bar controller manages navigation delegate
    if (!isInSidebarMode) {
        self.navigationController.delegate = self;
    }
    
    //    if (m_displayLink) [m_displayLink invalidate];
    
    [[self navigationController] setNavigationBarHidden:NO animated:NO];
    
    if (self.previousAppearance) {
        self.navigationController.navigationBar.standardAppearance = self.previousAppearance;
        self.navigationController.navigationBar.scrollEdgeAppearance = self.previousAppearance;
        self.navigationController.navigationBar.compactAppearance = self.previousAppearance;
    }
    //    [[[self navigationController] navigationBar] setBackgroundColor:[UIColor systemBackgroundColor]];
    statusbarHidden=NO;
    //    [self setNeedsStatusBarAppearanceUpdate];
}

- (UIImage *)imageFromView:(UIView *)view {
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithBounds:view.layer.bounds];
    UIImage *image= [renderer imageWithActions:^(UIGraphicsImageRendererContext*_Nonnull myContext){
        [view.layer renderInContext: myContext.CGContext];
    }];
    
    return image;
}

-(void) generateCoverTexture {
    
    if (cover_img) {
        if (txtCoverImg) {
            //glDeleteTextures(1,&txtBGImage);
        }
        
        CGSize sizeOfImage = [cover_img size];
        CGFloat scaleOfImage = [cover_img scale];
        CGSize pixelSizeOfImage = CGSizeMake(scaleOfImage * sizeOfImage.width, scaleOfImage * sizeOfImage.height);
        
        //create context
        GLubyte * textureData = (GLubyte *)malloc(pixelSizeOfImage.width * pixelSizeOfImage.height * 4 * sizeof(GLubyte));
        CGContextRef tmpContext = CGBitmapContextCreate(textureData, pixelSizeOfImage.width, pixelSizeOfImage.height, 8, pixelSizeOfImage.width * 4, CGImageGetColorSpace(backgroundImage.CGImage), kCGImageAlphaPremultipliedLast);
        
        //draw image into context
        CGContextDrawImage(tmpContext, CGRectMake(0.0, 0.0, pixelSizeOfImage.width, pixelSizeOfImage.height), cover_img.CGImage);
        
        //        txtCoverImgWidth=pixelSizeOfImage.width;
        //        txtCoverImgHeight=pixelSizeOfImage.height;
        
        glBindTexture(GL_TEXTURE_2D, txtCoverImg);
        
        //create texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixelSizeOfImage.width, pixelSizeOfImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
        
        txtCoverImgRatio=pixelSizeOfImage.width/pixelSizeOfImage.height;
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        //        glGenerateMipmap(GL_TEXTURE_2D);
        //glActiveTexture(GL_TEXTURE0);
        
        free(textureData);
        CGContextRelease(tmpContext);
    }
}


-(void) generateBGTexture {
    backgroundImage=[self imageFromView:self.cover_viewAll];
    //backgroundImage=self.cover_img;
    if (backgroundImage) {
        if (txtBGImage) {
            //glDeleteTextures(1,&txtbackgroundImage);
        }
        
        CGSize sizeOfImage = [backgroundImage size];
        CGFloat scaleOfImage = [backgroundImage scale];
        CGSize pixelSizeOfImage = CGSizeMake(scaleOfImage * sizeOfImage.width, scaleOfImage * sizeOfImage.height);
        
        //create context
        GLubyte * textureData = (GLubyte *)malloc(pixelSizeOfImage.width * pixelSizeOfImage.height * 4 * sizeof(GLubyte));
        CGContextRef tmpContext = CGBitmapContextCreate(textureData, pixelSizeOfImage.width, pixelSizeOfImage.height, 8, pixelSizeOfImage.width * 4, CGImageGetColorSpace(backgroundImage.CGImage), kCGImageAlphaPremultipliedLast);
        
        //draw image into context
        CGContextDrawImage(tmpContext, CGRectMake(0.0, 0.0, pixelSizeOfImage.width, pixelSizeOfImage.height), backgroundImage.CGImage);
        
        //        txtBGImageWidth=pixelSizeOfImage.width;
        //        txtBGImageHeight=pixelSizeOfImage.height;
        
        glBindTexture(GL_TEXTURE_2D, txtBGImage);
        
        //create texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixelSizeOfImage.width, pixelSizeOfImage.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        //        glGenerateMipmap(GL_TEXTURE_2D);
        
        //glActiveTexture(GL_TEXTURE0);
        
        
        free(textureData);
        CGContextRelease(tmpContext);
    }
}

- (void)viewDidAppear:(BOOL)animated {
    mHasFocus=1;

    nowplayingPL=nil;

    //Force an update of song info to display in gl view
    [self refreshFXFSLabels];

    //
    [super viewDidAppear:animated];
}

void updateSettingsSelectedSlot() {
    //update fxslot settings
    settings[PROJECTM_FXSLOT].detail.mdz_slider.slider_value=fxSlot[FX_PROJECTM];
    settings[OSCILLO_FXSLOT].detail.mdz_slider.slider_value=fxSlot[FX_OSCILLO];
    settings[GLOB_FXPianoRollFXSLOT].detail.mdz_slider.slider_value=fxSlot[FX_PIANOROLL];
    settings[GLOB_FXPiano3DFXSLOT].detail.mdz_slider.slider_value=fxSlot[FX_PIANO3D];
    settings[GLOB_FXMIDIPatternFXSLOT].detail.mdz_slider.slider_value=fxSlot[FX_MIDIPattern];
    settings[GLOB_FXMODPatternFXSLOT].detail.mdz_slider.slider_value=fxSlot[FX_MODPattern];
    settings[GLOB_FXSpectrumFXSLOT].detail.mdz_slider.slider_value=fxSlot[FX_2DSpectrum];
    settings[GLOB_FX3DSpectrumFXSLOT].detail.mdz_slider.slider_value=fxSlot[FX_3DSpectrum];
    settings[GLOB_FX3DLandscapeFXSLOT].detail.mdz_slider.slider_value=fxSlot[FX_3DLandscape];
    settings[GLOB_FXCoverFXSLOT].detail.mdz_slider.slider_value=fxSlot[FX_COVER];
    //
}

-(void) glViewLongPresstouch:(UILongPressGestureRecognizer *)gestureRecognizer {
    static float oglLPTapXold=0,oglLPTapYold=0;
    CGPoint pt=[gestureRecognizer locationInView:m_oglView];
    oglLPTapX=pt.x;
    oglLPTapY=pt.y;
    
    switch (gestureRecognizer.state) {
        case UIGestureRecognizerStateBegan:
            oglLPTap=1;
            oglLPTapXstart=oglLPTapX;
            oglLPTapYstart=oglLPTapY;
            oglLPTapXold=oglLPTapX;
            oglLPTapYold=oglLPTapY;
            break;
        case UIGestureRecognizerStateChanged:
            oglLPTap=2;
            //reset delay counter if moving enough
            if ( (abs(oglLPTapXold-oglLPTapX)>8.0) || (abs(oglLPTapYold-oglLPTapY)>8.0) ) {
                fxLPselectedCpt=0;
                oglLPTapXold=oglLPTapX;
                oglLPTapYold=oglLPTapY;
            }
            break;
        default:
            if (fxLPselected>=0) {
                fxSlot[fxLPselected]=fxTargetSlot;
                
                //update fxslot settings
                updateSettingsSelectedSlot();
            }
            oglLPTap=0;
            fxLPselected=-1;
            break;
    }
}


-(void) glViewTwoFingersTouch:(UITapGestureRecognizer *)gestureRecognizer {
    //MDZILog("touch 2 fingers");
    
    switch (gestureRecognizer.state) {
        case UIGestureRecognizerStateBegan:
//            MDZILog("reset wheel 2 fingers touch start");
            moveWheelXPMenu=0;
            moveWheelYPMenu=0;
            break;
        case UIGestureRecognizerStateChanged:
            break;
        default:
            mOglView1Tap=1;
            break;
    }
    
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
//    MDZILog("reset wheel touches began");
    moveWheelXPMenu=0;
    moveWheelYPMenu=0;
}

-(void) glViewOneFingerOneTap:(UITapGestureRecognizer *)gestureRecognizer {
    moveWheelXPMenu=0;
    moveWheelYPMenu=0;
//    MDZILog("reset wheel onetap");
    switch (gestureRecognizer.state) {
        case UIGestureRecognizerStateBegan:
            //Stop wheel base move if still active
            
            
            break;
        case UIGestureRecognizerStateChanged:
            break;
        default:
            mOglView1Tap=1;
            break;
    }
    
    CGPoint pt=[gestureRecognizer locationInView:m_oglView];
    oglTapX=pt.x;
    oglTapY=pt.y;
}

-(void) glViewPanGesture:(UIPanGestureRecognizer *)gestureRecognizer {
    if (_shiftModeOn) {
        [self glViewPan2Gesture:gestureRecognizer];
    } else {
        static CGPoint last_pt;
        CGPoint pt=[gestureRecognizer translationInView:m_oglView];
        CGPoint start_pt=[gestureRecognizer locationInView:m_oglView];
        movePx=pt.x;
        movePy=pt.y;
        switch (gestureRecognizer.state) {
            case UIGestureRecognizerStateBegan:
                last_pt=pt;
                startPx=start_pt.x;
                startPy=start_pt.y;
                
                posPx=start_pt.x;
                posPy=start_pt.y;
                
                //Stop wheel base move if still active
//                panGestureWheel=1;
                moveWheelXPMenu=0;
                moveWheelYPMenu=0;
                
//                MDZILog("reset wheel Pan start");
                
                panGesture1Tap=1;
                movePxOld=movePx;
                movePyOld=movePy;
                //Also reset tracking variables related to "swipe" like gesture
                movePxPM=0;movePyPM=0;
                movePxPMenu=0;movePyPMenu=0;
                movePMnomore=0;
                movePreWheelXPMenu=0;
                movePreWheelYPMenu=0;
                break;
            case UIGestureRecognizerStateChanged:
                panGesture1Tap=2;
                if (pmenu_show) {
                    movePxPMenu+=pt.x-last_pt.x;
                    movePyPMenu+=pt.y-last_pt.y;
                    posPx=start_pt.x;
                    posPy=start_pt.y;
                }
//                panGestureWheel=2;
                if (pmenu_show) {
                    movePreWheelXPMenu=pt.x-last_pt.x;
                    movePreWheelYPMenu=pt.y-last_pt.y;
                }
                last_pt=pt;
                break;
            default:
                if (pmenu_show) {
//                    MDZILog("Wheel last: %f %f",movePreWheelXPMenu,movePreWheelYPMenu);
//                    movePreWheelXPMenu=pt.x-last_pt.x;
//                    movePreWheelYPMenu=pt.y-last_pt.y;
//                    last_pt=pt;
                    
                    
                    if (is_macOS) {
//                        moveWheelXPMenu=movePreWheelXPMenu*5;
//                        moveWheelYPMenu=movePreWheelYPMenu*5;
                    } else {
                        moveWheelXPMenu=movePreWheelXPMenu*10;
                        moveWheelYPMenu=movePreWheelYPMenu*10;
                        posPx=start_pt.x;
                        posPy=start_pt.y;
                    }
                }
                panGesture1Tap=0;
                //Also reset tracking variables related to "swipe" like gesture
                movePxPM=0;movePyPM=0;
                movePxPMenu=0;movePyPMenu=0;
                movePMnomore=0;
//                panGestureWheel=0;
                break;
        }
    }
    
#if TARGET_OS_MACCATALYST
    // Montrer le curseur
        [NSCursor unhide];
        
        // Annuler le timer précédent
        [self.mouseHideTimer invalidate];
        
        // Créer un nouveau timer pour cacher après 2 secondes d'inactivité
        self.mouseHideTimer = [NSTimer scheduledTimerWithTimeInterval:2.0
                                                               target:self
                                                             selector:@selector(hideCursor)
                                                             userInfo:nil
                                                              repeats:NO];
#endif
}

-(void) glViewHoverGesture:(UIHoverGestureRecognizer *)gestureRecognizer {
    CGPoint pt=[gestureRecognizer locationInView:m_oglView];
    posMouseX=round(pt.x);
    posMouseY=round(pt.y);
    
    moveWheelXPMenu=0;
    moveWheelYPMenu=0;
//    MDZILog("reset wheel Hover");
    
    switch (gestureRecognizer.state) {
        case UIGestureRecognizerStateBegan:
            panGestureHover=1;
            
            break;
        case UIGestureRecognizerStateChanged:
            panGestureHover=2;
            break;
        default:
            panGestureHover=0;
            break;
    }
    
#if TARGET_OS_MACCATALYST
    // Montrer le curseur
        [NSCursor unhide];
        
        // Annuler le timer précédent
        [self.mouseHideTimer invalidate];
        
        // Créer un nouveau timer pour cacher après 2 secondes d'inactivité
        self.mouseHideTimer = [NSTimer scheduledTimerWithTimeInterval:2.0
                                                               target:self
                                                             selector:@selector(hideCursor)
                                                             userInfo:nil
                                                              repeats:NO];
#endif
}

-(void) glViewPan2Gesture:(UIPanGestureRecognizer *)gestureRecognizer {
//    if (!_shiftModeOn) {
//        [self glViewPanGesture:gestureRecognizer];
//        return;
//    }
    static CGPoint last_pt;
    CGPoint pt=[gestureRecognizer translationInView:m_oglView];
    CGPoint start_pt=[gestureRecognizer locationInView:m_oglView];
    movePx2=pt.x;
    movePy2=pt.y;
    
    CGPoint velocity = [gestureRecognizer velocityInView:gestureRecognizer.view];
    movePinchAngle = atan2(velocity.y, velocity.x);
    
    switch (gestureRecognizer.state) {
        case UIGestureRecognizerStateBegan:
            startPx=start_pt.x;
            startPy=start_pt.y;
            posPx=start_pt.x;
            posPy=start_pt.y;
            movePx=0;
            movePy=0;
            last_pt=pt;
            
            movePx2Old=movePx2;
            movePy2Old=movePy2;
            
            panGestureWheel=1;
            moveWheelXPMenu=0;
            moveWheelYPMenu=0;
            
            //MDZILog("reset wheel Pan2 start");
            
            break;
        case UIGestureRecognizerStateChanged:
            panGestureWheel=2;
            if (pmenu_show) {
                moveWheelXPMenu=pt.x-last_pt.x;
                moveWheelYPMenu=pt.y-last_pt.y;
                //MDZILog("pan2 set pt %f %f",moveWheelXPMenu,moveWheelYPMenu);
            }
            last_pt=pt;
            break;
        default:
            //MDZILog("other");
            panGestureWheel=0;
            break;
    }
}

- (CGFloat)angleForPinchGesture:(UIPinchGestureRecognizer *)gesture
                          view:(UIView *)view
{
    if (gesture.numberOfTouches < 2) {
        return 0;
    }

    CGPoint p1 = [gesture locationOfTouch:0 inView:view];
    CGPoint p2 = [gesture locationOfTouch:1 inView:view];

    CGFloat dx = p2.x - p1.x;
    CGFloat dy = p2.y - p1.y;
    CGFloat angle = atan2(dy, dx); // radians

    return angle;
}


-(void) glViewPinchGesture:(UIPinchGestureRecognizer *)gestureRecognizer {
    CGFloat scale=gestureRecognizer.scale;
    movePinchScale=scale;
    movePinchAngle=[self angleForPinchGesture:gestureRecognizer view:gestureRecognizer.view] * 180.0 / M_PI;
    
    CGPoint start_pt=[gestureRecognizer locationInView:m_oglView];
    
    if (gestureRecognizer.state==UIGestureRecognizerStateBegan) {
        startPx=start_pt.x;
        startPy=start_pt.y;
        
        movePinchScaleOld=movePinchScale;
        
//        MDZILog("reset wheel Pinch start");
        moveWheelXPMenu=0;
        moveWheelYPMenu=0;
    }
}

// fps calculation
static float m_nFps; // current FPS
static CFTimeInterval lastFrameStartTime;
static CFTimeInterval tgtFrameStartTime;
static int m_nAverageFps; // the average FPS
static int m_nAverageFpsCounter;
static float m_nAverageFpsSum;
int tgtFrameCnt;

static void calcFps()
{
    CFTimeInterval thisFrameStartTime = CFAbsoluteTimeGetCurrent();
    float deltaTimeInSeconds = thisFrameStartTime - lastFrameStartTime;
    m_nFps = (deltaTimeInSeconds == 0) ? 0: 1.0 / (deltaTimeInSeconds);

    m_nAverageFpsCounter++;
    m_nAverageFpsSum+=m_nFps;
    if (m_nAverageFpsCounter >= 20) // calculate average FPS
    {
        m_nAverageFps = round(m_nAverageFpsSum/m_nAverageFpsCounter);
        m_nAverageFpsCounter = 0;
        m_nAverageFpsSum = 0;
    }
    lastFrameStartTime = thisFrameStartTime;
}

extern "C" int current_sample;

void menuInterpolValue(float &curValue,float startValue,float tgtValue) {
    float diff=tgtValue-curValue;
    if (diff>0) {
        float incr=round(diff/20.0)+2;
        curValue+=incr;
        if (curValue>tgtValue) curValue=tgtValue;
        if (curValue<0) curValue=0;
    } else if (diff<0) {
        diff=curValue-startValue;
        float decr=round(diff/20.0)-2;
        if (decr>=0) decr=-2;
        curValue+=decr;
        if (curValue<tgtValue) curValue=tgtValue;
        if (curValue>startValue) curValue=startValue;
    }
}

- (void)frameTooSlow {
    settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value=false;
    
    UIAlertController *msgAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Warning",@"")
                                   message:NSLocalizedString(@"FX too slow.\nProjectM FX has been deactivated.\nPlease consider choosing less complex presets and/or reduce resolution.",@"")
                                   preferredStyle:UIAlertControllerStyleAlert];
    
    UIAlertAction* moveToNext = [UIAlertAction actionWithTitle:NSLocalizedString(@"Move to next",@"") style:UIAlertActionStyleDefault
       handler:^(UIAlertAction * action) {
        [_mdzPM_playlist next:true];
        settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value=true;
        }];
    [msgAlert addAction:moveToNext];
    
    UIAlertAction* remAndNext = [UIAlertAction actionWithTitle:NSLocalizedString(@"Remove preset and move to next",@"") style:UIAlertActionStyleDefault
       handler:^(UIAlertAction * action) {
        [_mdzPM_playlist removeCurEntry];
        [_mdzPM_playlist loadCurEntry];
        settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value=true;
        }];
    [msgAlert addAction:remAndNext];
    
    UIAlertAction* closeAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Ok",@"") style:UIAlertActionStyleDefault
       handler:^(UIAlertAction * action) {
        }];
    [msgAlert addAction:closeAction];
    
    [self presentViewController:msgAlert animated:YES completion:nil];
}

-(void) doFramePM:(ImVec2)winSize isSlot:(bool)isSlot {
    float ww=winSize.x;
    float hh=winSize.y;
    if (!_pmIsInitialized) return; //PRojectM might still be initializing and calling some opengl stuff from background thread
    
    if (settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value) {
        //PM is active
        
        //check if it is alone before processing inputs, to avoid mixing inputs with other FX
        //bool isPMalone=[self isProjectMAlone];
        
        if (movePMnomore==0) {
            if (movePxPM>PM_HorizontalSwipe_Threshold) {
                movePxPM=0;
                movePyPM=0;
                movePMnomore=1;
                if ([_mdzPM_playlist getSize]) {
                    if (settings[PROJECTM_AutoSwitchPresetsMode].detail.mdz_switch.switch_value) [_mdzPM_playlist last:false];
                    else [_mdzPM_playlist last:false];
                }
            } else if (movePxPM<-PM_HorizontalSwipe_Threshold) {
                movePxPM=0;
                movePyPM=0;
                movePMnomore=1;
                
                if ([_mdzPM_playlist getSize]) {
                    [_mdzPM_playlist next:false];
                }
            }
            
            if (movePyPM>PM_VerticalSwipe_Threshold) {
                //----------------------
                //Swipe down: lock/unlock
                //----------------------
                movePxPM=0;
                movePyPM=0;
                movePMnomore=1;
                if (_pmIsInitialized && _pm) {
                    [self mdSwitchLockStatusPreset];
                }
            } else if (movePyPM<-PM_VerticalSwipe_Threshold) {
                //----------------------
                //Swipe up: favorite -> like/unlike
                //----------------------
                movePxPM=0;
                movePyPM=0;
                movePMnomore=1;
                if (_pmIsInitialized && _pm) {
                    [self mdChangeFavoriteStatusPreset:0];
                }
            }
        }
    }
    
    mdzMainThreadId = pthread_mach_thread_np(pthread_self());
    
    /*-------------------------------------------------------------------------------*/
    /*  ProjectM render */
    /*-------------------------------------------------------------------------------*/
    if (_pmIsInitialized && _pm && settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value) {
        size_t currentMeshX{0};
        size_t currentMeshY{0};
        
        meshX=round(settings[PROJECTM_MeshSizeX].detail.mdz_slider.slider_value/2)*2;
        if (meshX<8) meshX=8;if (meshX>128) meshX=128;
        meshY=round(settings[PROJECTM_MeshSizeY].detail.mdz_slider.slider_value/2)*2;
        if (meshY<6) meshY=6;if (meshY>96) meshY=96;
        
        projectm_get_mesh_size(_pm, &currentMeshX, &currentMeshY);
        if (currentMeshX != meshX || currentMeshY != meshY) {
            projectm_set_mesh_size(_pm, meshX, meshY);
        }
        
        size_t canvasWidth;
        size_t canvasHeight;
        projectm_get_window_size(_pm, &canvasWidth, &canvasHeight);
        
        float _pmScaleFactor=1<<settings[PROJECTM_Quality].detail.mdz_switch.switch_value;;
        _pmCanvasWidth=ww*glScaleFactor/_pmScaleFactor;
        _pmCanvasHeight=hh*glScaleFactor/_pmScaleFactor;
        
        if ((_pmCanvasWidth!=canvasWidth) || (_pmCanvasHeight!=canvasHeight)) {
            projectm_set_window_size(_pm, _pmCanvasWidth, _pmCanvasHeight);
            MDZILog("set win size: %d %d",_pmCanvasWidth,_pmCanvasHeight);
        }
        
       
        int sample_count=(settings[GLOB_FXFPS].detail.mdz_switch.switch_value?735:735*2);
        projectm_pcm_add_int16(_pm,(const int16_t*)pmBuffer,sample_count,PROJECTM_STEREO);
        
//        mdz_pmBlurAfterAudio=settings[PROJECTM_BlurAfterAudioMode].detail.mdz_boolswitch.switch_value;
        
        if ( (_pmCanvasWidth==(ww*glScaleFactor)) && (_pmCanvasHeight==(hh*glScaleFactor)) && !isSlot) {
            //Max Quality, screen resolution
            //Render directly to screen
            projectm_opengl_render_frame(_pm);
        } else {
            //reduced Quality
            //Render to a texture and then display it
            RenderUtils::startRenderToTexture(_pmCanvasWidth,_pmCanvasHeight);
            projectm_opengl_render_frame_fbo(_pm,mdzRenderbuffer);
            RenderUtils::endRenderToTextureBasic(ww*glScaleFactor,hh*glScaleFactor,1.0);
        }
        
        projectm_set_fps(_pm, (m_nAverageFps>0?m_nAverageFps:60));
    }
    /*-------------------------------------------------------------------------------*/
}

- (void)showGUICorners:(ImVec2)winsize frameToUpdate:(int)frameToUpdate{
    float ww=winsize.x;
    float hh=winsize.y;
    
    float cur_winSizeX=ww/4;
    float cur_winSizeY=hh/4;
    float alpha;
    
    ImGui::GetStyle().Alpha=1.0;
    if (font_menu) ImGui::PushFont(font_menu,FONTSIZE_GUIMSESSAGE*glScaleFactor);
    else ImGui::PushFont(nullptr);
    
    char strButton[32];
    float posX,posY;
    for (int i=0;i<4;i++) {
        switch (i) {
            case 0:
                posX=0*glScaleFactor;posY=(hh-cur_winSizeY)*glScaleFactor;
                snprintf(strButton,32,"%s",[[NSString stringWithFormat:@"%C",static_cast<unichar>(FA_CHEVRON_LEFT)] UTF8String]);
                break;
            case 1:
                posX=(ww-cur_winSizeX)*glScaleFactor;posY=(hh-cur_winSizeY)*glScaleFactor;
                snprintf(strButton,32,"%s",[[NSString stringWithFormat:@"%C",static_cast<unichar>(FA_CHEVRON_RIGHT)] UTF8String]);
                break;
            case 2:
                posX=(ww-cur_winSizeX)*glScaleFactor;posY=0*glScaleFactor;
                snprintf(strButton,32,"%s",[[NSString stringWithFormat:@"%C",static_cast<unichar>(FA_MICROCHIP)] UTF8String]);
                break;
            case 3:
                posX=0*glScaleFactor;posY=0*glScaleFactor;
                snprintf(strButton,32,"%s",[[NSString stringWithFormat:@"%C",static_cast<unichar>(FA_INFO)] UTF8String]);
                break;
        }
        ImGui::SetNextWindowPos(ImVec2(posX,posY));
        ImGui::SetNextWindowSize(ImVec2(cur_winSizeX*glScaleFactor,cur_winSizeY*glScaleFactor));
        alpha=(float)(oglv_corner_fade[i])/30.0*0.5;
        if (alpha>1) alpha=1;
        ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0,0,0,alpha));
        ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0,1.0,1.0,alpha));
        char strId[8];
        snprintf(strId,8,"TapWin%d",i);
        ImGui::Begin(strId,0,
                     ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoFocusOnAppearing
                     );
        float text_width=ImGui::CalcTextSize(strButton).x;
        float text_height=ImGui::GetTextLineHeightWithSpacing();
        
        ImGui::SetCursorPos(ImVec2( (cur_winSizeX*glScaleFactor-text_width)/2, (cur_winSizeY*glScaleFactor-text_height)/2 ) );
        
        ImGui::Text("%s",strButton);
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }
    ImGui::PopFont();
    
    for (int j=0;j<frameToUpdate;j++) {
        for (int i=0;i<4;i++) {
            if (oglv_corner_fade[i]>0) oglv_corner_fade[i]--;
        }
    }
}

- (void)showInfoData:(ImVec2)winsize frameToUpdate:(int)frameToUpdate{
    float ww=winsize.x;
    float hh=winsize.y;
    if (!sysMonitorIsActive) {
        [sysMonitor startMonitoring];
        sysMonitorIsActive=true;
    }
    float cpuUsage=sysMonitor.cpuUsage;
    
    float winsizeX,winsizeY;
    static float cur_winSizeX=0;
    static float cur_winSizeY=0;

    static float startX=0,startY=0;
    static int switchPrevValue=0;
    if (switchPrevValue!=settings[GLOB_FXSHOWINFO].detail.mdz_switch.switch_value) {
        switchPrevValue=settings[GLOB_FXSHOWINFO].detail.mdz_switch.switch_value;
        startX=cur_winSizeX;
        startY=cur_winSizeY;
    }
    switch (settings[GLOB_FXSHOWINFO].detail.mdz_switch.switch_value) {
        case 0:winsizeX=0;winsizeY=0;break;
        case 1:winsizeX=68;winsizeY=SHOWINFO_SECTION1_SIZE;break;
        case 2:winsizeX=80;winsizeY=SHOWINFO_SECTION2_SIZE;break;
        case 3:winsizeX=80;winsizeY=hh;break;
        default:winsizeX=0;winsizeY=0;break;
    }
    
    if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) {
        if (winsizeY>(hh-safe_top-safe_bottom)) winsizeY=(hh-safe_top-safe_bottom);
    }
    
    for (int i=0;i<frameToUpdate;i++) {
        menuInterpolValue(cur_winSizeX,startX,winsizeX);
        menuInterpolValue(cur_winSizeY,startY,winsizeY);
    }
    if ( (cur_winSizeX!=0) || (cur_winSizeY!=0) ) {
        char strTmp[32];
        float posx,posy;
        ImVec2 sizeText;
        posx=0;
        posy=0;
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) {
            posx+=safe_right;
            posy+=safe_top;
        }
        
        ImGui::SetNextWindowPos(ImVec2((ww-cur_winSizeX-posx)*glScaleFactor,posy*glScaleFactor));
        ImGui::SetNextWindowSize(ImVec2(cur_winSizeX*glScaleFactor,cur_winSizeY*glScaleFactor));
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0,0,0,0.5));
        ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(0,0,0,0));
        
        float txtAlpha=(SHOWINFO_SECTION1_SIZE-(cur_winSizeY-0))/(SHOWINFO_SECTION1_SIZE);
        if (txtAlpha<0) txtAlpha=0;
        if (txtAlpha>1) txtAlpha=1;
        txtAlpha=1-txtAlpha;
        float txtAlphaX=cur_winSizeX/68.0;
        if (txtAlphaX>1) txtAlphaX=1;
        txtAlphaX*=txtAlphaX;
        txtAlpha*=txtAlphaX;

        
        ImGui::GetStyle().Alpha=1.0;
        if (font_menu) ImGui::PushFont(font_menu,FONTSIZE_SHOWINFO_FPS*glScaleFactor);
        else ImGui::PushFont(nullptr);
        
        ImGui::Begin("Info",0,
                     ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoFocusOnAppearing
                     );
        
        //FPS
        posy=0;
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_FPS_COLOR,txtAlpha));
        ImGui::SetCursorPos(ImVec2(2,posy));
        ImGui::Text("FPS");
        snprintf(strTmp,32,"%d",m_nAverageFps);
        sizeText=ImGui::CalcTextSize(strTmp);
        posx=sizeText.x+8;
        posy=0;
        ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
        ImGui::Text("%s",strTmp);
        posy+=sizeText.y+2;
        if (cur_winSizeY>SHOWINFO_SECTION1_SIZE) {
            txtAlpha=(SHOWINFO_SECTION2_SIZE-SHOWINFO_SECTION1_SIZE-(cur_winSizeY-SHOWINFO_SECTION1_SIZE))/(SHOWINFO_SECTION2_SIZE-SHOWINFO_SECTION1_SIZE);
            if (txtAlpha<0) txtAlpha=0;
            if (txtAlpha>1) txtAlpha=1;
            txtAlpha=1-txtAlpha;
            txtAlpha*=txtAlphaX;
            
            //smaller font
            if (font_menu) ImGui::PushFont(font_menu,FONTSIZE_SHOWINFO_DETAILS*glScaleFactor);
            else ImGui::PushFont(nullptr);
            //CPU
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_CPU_COLOR,txtAlpha));
            ImGui::SetCursorPos(ImVec2(2,posy));
            ImGui::Text("CPU");
            snprintf(strTmp,32,"%.2f%%",cpuUsage);
            sizeText=ImGui::CalcTextSize(strTmp);
            posx=sizeText.x+8;
            ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
            ImGui::Text("%s",strTmp);
            ImGui::PopStyleColor();
            posy+=sizeText.y+2;
            
            if (cur_winSizeY>SHOWINFO_SECTION2_SIZE) {
                txtAlpha=(hh-SHOWINFO_SECTION2_SIZE-(cur_winSizeY-SHOWINFO_SECTION2_SIZE))/(hh-SHOWINFO_SECTION2_SIZE);
                if (txtAlpha<0) txtAlpha=0;
                if (txtAlpha>1) txtAlpha=1;
                txtAlpha=1-txtAlpha;
                txtAlpha*=txtAlphaX;
                
                float devWW,devHH;
                CGSize screenSize;
                for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
                    if ([scene isKindOfClass:[UIWindowScene class]]) {
                        UIWindowScene *wscene=(UIWindowScene *)scene;
                        for (UIWindow *win in wscene.windows) {
                            if (win.keyWindow) {
                                screenSize=win.screen.bounds.size;
                            }
                        }
                    }
                }
                
                devWW=screenSize.width*glScaleFactor;
                devHH=screenSize.height*glScaleFactor;
                
                posy+=sizeText.y+6;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_DEVICE_COLOR,txtAlpha));
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("Device");
                posy+=sizeText.y+4;
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_DEVICERES_COLOR,txtAlpha));
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("R");
                snprintf(strTmp,32,"%.0fx%.0f",devWW,devHH);
                sizeText=ImGui::CalcTextSize(strTmp);
                posx=sizeText.x+8;
                ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                ImGui::Text("%s",strTmp);
                posy+=sizeText.y+2;
                ImGui::PopStyleColor();
                
                posy+=sizeText.y+6;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_FXVIEW_COLOR,txtAlpha));
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("FX View");
                posy+=sizeText.y+4;
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_FXVIEWRES_COLOR,txtAlpha));
                //Resolution
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("R");
                snprintf(strTmp,32,"%.0fx%.0f",ww*glScaleFactor,hh*glScaleFactor);
                sizeText=ImGui::CalcTextSize(strTmp);
                posx=sizeText.x+8;
                ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                ImGui::Text("%s",strTmp);
                posy+=sizeText.y+2;
                ImGui::PopStyleColor();
                
                posy+=sizeText.y+6;
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_PM_COLOR,txtAlpha));
                //ProjectM info
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("ProjectM");
                posy+=sizeText.y+4;
                //Internal PM resolution
                ImGui::PopStyleColor();
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_PMRES_COLOR,txtAlpha));
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("R");
                snprintf(strTmp,32,"%dx%d",_pmCanvasWidth,_pmCanvasHeight);
                sizeText=ImGui::CalcTextSize(strTmp);
                posx=sizeText.x+8;
                ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                ImGui::Text("%s",strTmp);
                posy+=sizeText.y+4;
                
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("Mesh");
                snprintf(strTmp,32,"%.0fx%.0f",
                         settings[PROJECTM_MeshSizeX].detail.mdz_slider.slider_value,
                         settings[PROJECTM_MeshSizeY].detail.mdz_slider.slider_value);
                sizeText=ImGui::CalcTextSize(strTmp);
                posx=sizeText.x+8;
                ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                ImGui::Text("%s",strTmp);
                posy+=sizeText.y+4;
                
                posy+=sizeText.y+4;
                //PM audio data
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_PMAUDIO_COLOR,txtAlpha));
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("Bass");
                
                float bassAttr,midAttr,trebAttr,volAttr;
                bassAttr=midAttr=trebAttr=volAttr=0;;
                if (_pm) projectm_get_audio_vars(_pm,&bassAttr,&midAttr,&trebAttr,&volAttr);

                
                snprintf(strTmp,32,"%1.2f",bassAttr);
                sizeText=ImGui::CalcTextSize(strTmp);
                posx=sizeText.x+8;
                ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                ImGui::Text("%s",strTmp);
                posy+=sizeText.y+2;
                
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("Mid");
                snprintf(strTmp,32,"%1.2f",midAttr);
                sizeText=ImGui::CalcTextSize(strTmp);
                posx=sizeText.x+8;
                ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                ImGui::Text("%s",strTmp);
                posy+=sizeText.y+2;
                
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("Treb");
                snprintf(strTmp,32,"%1.2f",trebAttr);
                sizeText=ImGui::CalcTextSize(strTmp);
                posx=sizeText.x+8;
                ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                ImGui::Text("%s",strTmp);
                posy+=sizeText.y+2;
                
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("Vol");
                snprintf(strTmp,32,"%1.2f",volAttr);
                sizeText=ImGui::CalcTextSize(strTmp);
                posx=sizeText.x+8;
                ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                ImGui::Text("%s",strTmp);
                posy+=sizeText.y+2;
                
                posy+=sizeText.y+6;
                //FX Frame info
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_FXFRAME_COLOR,txtAlpha));
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("FX Frame");
                posy+=sizeText.y+4;
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_FXFRAMEINFO_COLOR,txtAlpha));
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("Exec");
                snprintf(strTmp,32,"%.1fms",_fx_frame_time);
                sizeText=ImGui::CalcTextSize(strTmp);
                posx=sizeText.x+8;
                ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                ImGui::Text("%s",strTmp);
                posy+=sizeText.y+2;
                
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("Slow");
                snprintf(strTmp,32,"%d",_fx_frame_timeOverLimitCounter);
                sizeText=ImGui::CalcTextSize(strTmp);
                posx=sizeText.x+8;
                ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                ImGui::Text("%s",strTmp);
                posy+=sizeText.y+2;
                
#ifndef NDEBUG
                posy+=sizeText.y+6;
                //Debug info
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_FXFRAME_COLOR,txtAlpha));
                ImGui::SetCursorPos(ImVec2(2,posy));
                ImGui::Text("Debug info");
                posy+=sizeText.y+4;
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(SHOWINFO_FXFRAMEINFO_COLOR,txtAlpha));
                for (int i=0;i<4;i++) {
                    snprintf(strTmp,32,"%.3f",varCheck[i]);
                    sizeText=ImGui::CalcTextSize(strTmp);
                    posx=sizeText.x+8;
                    ImGui::SetCursorPos(ImVec2(cur_winSizeX*glScaleFactor-posx,posy));
                    ImGui::Text("%s",strTmp);
                    posy+=sizeText.y+2;
                }
#endif
                
                ImGui::PopStyleColor();
                
            }
            ImGui::PopFont();
        }
        ImGui::PopStyleColor();
        
        
        ImGui::End();
        ImGui::PopFont();
        
        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
    }
}

-(void) mdShowMusicInfo {
    [self refreshFXFSLabels];
}

-(void) refreshFXFSLabels {
    if ([mplayer isPlaying]) {
        
        int fps=(settings[GLOB_FXFPS].detail.mdz_switch.switch_value?60:30);
        _mdz_display_songinfo_countdown=fps*FX_FS_SONGINFO_TIMEOUT;
        
        _mdz_display_songinfo_char_count[0]=1;
        _mdz_display_songinfo_char_count[1]=1;
        _mdz_display_songinfo_char_count[2]=1;
        
        _mdz_FS_display_cursorLine=0;
        
        _mdz_display_songinfo_title=[mplayer getModFileTitle];
        _mdz_display_songinfo_artist=[mplayer artist];
        if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)) {
            //archive with multiple files
            if (mplayer.mod_subsongs>1) {
                //and also subsongs
                _mdz_display_songinfo_sub=[NSString stringWithFormat:@"(%d/%d)(%d/%d) %@",[mplayer getArcIndex]+1,[mplayer getArcEntriesCnt],mplayer.mod_currentsub-mplayer.mod_minsub+1,mplayer.mod_subsongs,[mplayer getModName]];
            } else {
                //no subsong
                _mdz_display_songinfo_sub=[NSString stringWithFormat:@"(%d/%d) %@",[mplayer getArcIndex]+1,[mplayer getArcEntriesCnt],[mplayer getModName]];
            }
        } else {
            if (mplayer.mod_subsongs>1) {
                //subsongs
                _mdz_display_songinfo_sub=[NSString stringWithFormat:@"(%d/%d) %@",mplayer.mod_currentsub-mplayer.mod_minsub+1,mplayer.mod_subsongs,[mplayer getModName]];
            } else {
                //no subsong
                _mdz_display_songinfo_sub=[NSString stringWithFormat:@"%@",[mplayer getModName]];
            }
        }
    }
}

- (void) doFx2DSpectrum:(ImVec4)fxSize {
    float x=fxSize.x;
    float y=fxSize.y;
    float ww=fxSize.z;
    float hh=fxSize.w;
    int nb_spectrum_bands;
    
    switch (settings[GLOB_FXLOD].detail.mdz_switch.switch_value) {
        case 2:
            nb_spectrum_bands=SPECTRUM_BANDS;
            break;
        case 1:
            nb_spectrum_bands=SPECTRUM_BANDS/2;
            break;
        case 0:
            nb_spectrum_bands=SPECTRUM_BANDS/4;
            break;
    }
    
    RenderUtils::DrawSpectrum2D(x,y,ww,hh,real_spectrumL,real_spectrumR,
                                settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value,nb_spectrum_bands,glScaleFactor,
                                0/*settings[GLOB_BLOOMFX].detail.mdz_boolswitch.switch_value*/);
}

- (void) doFx3DSpectrum:(ImVec4)fxSize {
    float x=fxSize.x;
    float y=fxSize.y;
    float ww=fxSize.z;
    float hh=fxSize.w;
    int nb_spectrum_bands;
    
    static float spectrum_posx=0;
    static float spectrum_posy=0;
    static float spectrum_posz=0;
    static float spectrum_rotx=0;
    static float spectrum_roty=0;
    
    switch (settings[GLOB_FXLOD].detail.mdz_switch.switch_value) {
        case 2:
            nb_spectrum_bands=SPECTRUM_BANDS;
            break;
        case 1:
            nb_spectrum_bands=SPECTRUM_BANDS/2;
            break;
        case 0:
            nb_spectrum_bands=SPECTRUM_BANDS/4;
            break;
    }
    
    if (movePinchScaleFX3DSpectrum<-9.0/4) movePinchScaleFX3DSpectrum=-9.0/4;
    if (movePinchScaleFX3DSpectrum>9.0/4) movePinchScaleFX3DSpectrum=9.0/4;
    spectrum_rotx=movePxFX3DSpectrum*0.5f;
    spectrum_roty=movePyFX3DSpectrum*0.25f;
    if (movePx2FX3DSpectrum>400) movePx2FX3DSpectrum=400;
    if (movePx2FX3DSpectrum<-400) movePx2FX3DSpectrum=-400;
    if (movePy2FX3DSpectrum>400) movePy2FX3DSpectrum=400;
    if (movePy2FX3DSpectrum<-400) movePy2FX3DSpectrum=-400;
    spectrum_posx=movePx2FX3DSpectrum*0.05;
    spectrum_posy=-movePy2FX3DSpectrum*0.05;
    spectrum_posz=movePinchScaleFX3DSpectrum*10*4;
    
    int mirror=0;
    if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value) mirror=0;
    if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value) mirror=0;
    RenderUtils::DrawSpectrum3DBar(x,y,ww,hh,real_spectrumL,real_spectrumR,angle,
                                   settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value,nb_spectrum_bands,mirror,glScaleFactor,settings[GLOB_FX3DSpectrumBloom].detail.mdz_switch.switch_value,spectrum_rotx,spectrum_roty,spectrum_posx,spectrum_posy,spectrum_posz);
}

- (void) doFx3DLandscape:(ImVec4)fxSize {
    float x=fxSize.x;
    float y=fxSize.y;
    float ww=fxSize.z;
    float hh=fxSize.w;
    int nb_spectrum_bands;
    
    switch (settings[GLOB_FXLOD].detail.mdz_switch.switch_value) {
        case 2:
            nb_spectrum_bands=SPECTRUM_BANDS;
            break;
        case 1:
            nb_spectrum_bands=SPECTRUM_BANDS/2;
            break;
        case 0:
            nb_spectrum_bands=SPECTRUM_BANDS/4;
            break;
    }
    
    if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value<4){
        RenderUtils::DrawSpectrum3D(x,y,ww,hh,real_spectrumL,real_spectrumR,angle,settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value,nb_spectrum_bands,settings[GLOB_FX3DLandscapeBloom].detail.mdz_boolswitch.switch_value,glScaleFactor);
    } else if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value<6) { RenderUtils::DrawSpectrumLandscape3D(x,y,ww,hh,real_spectrumL,real_spectrumR,angle,settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value-3,nb_spectrum_bands,settings[GLOB_FX3DLandscapeBloom].detail.mdz_boolswitch.switch_value,glScaleFactor);
    } else {
        RenderUtils::DrawSpectrum3DMorph(x,y,ww,hh,real_spectrumL,real_spectrumR,angle,settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value-5,nb_spectrum_bands,settings[GLOB_FX3DLandscapeBloom].detail.mdz_boolswitch.switch_value,glScaleFactor);
    }
}

- (void) doFxCover:(ImVec4)fxSize {
    float x=fxSize.x;
    float y=fxSize.y;
    float ww=fxSize.z;
    float hh=fxSize.w;
    
    if (settings[GLOB_FXCoverFillMode].detail.mdz_switch.switch_value==0){
        RenderUtils::DrawTexture(ww, hh, txtCoverImg, 1.0f,1);
    } else {
        RenderUtils::DrawTexture(ww, hh, txtCoverImg, 1.0f,1,txtCoverImgRatio);
    }
}


- (void) doFxMidiPattern:(ImVec4)fxSize {
    float x=fxSize.x;
    float y=fxSize.y;
    float ww=fxSize.z;
    float hh=fxSize.w;
    int playerpos;
    
    /*******************************************************/
    /* Compute midiFX display scrolling */
    /*******************************************************/
    if ( ([mplayer isMidiLikeDataAvailable]||mplayer.mPatternDataAvail)&&
        settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value ) {
        float note_fx_linewidth;
        float noteroll_fx_keywidth;
        //scroll  & get current note bar width
        
        
        
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==2) {
            //vertical
            tim_midifx_note_offset+=-movePxMID;
            
            note_fx_linewidth=ww/tim_midifx_note_range;
            
            tim_midifx_length-=movePyMID*1.2f*(tim_midifx_length)/(MAX_MIDIFX_LENGTH)*(tim_midifx_length)/(MAX_MIDIFX_LENGTH)*(tim_midifx_length)/(MAX_MIDIFX_LENGTH);
            
            movePyMID=0;
            if (tim_midifx_length>MAX_MIDIFX_LENGTH) tim_midifx_length=MAX_MIDIFX_LENGTH;
            if (tim_midifx_length<=MIDIFX_OFS) tim_midifx_length=MIDIFX_OFS+1;
        } else {
            //horizontal
            tim_midifx_note_offset+=movePyMID;
            
            note_fx_linewidth=hh/tim_midifx_note_range;
            
            tim_midifx_length+=movePxMID*1.2f*(tim_midifx_length)/(MAX_MIDIFX_LENGTH)*(tim_midifx_length)/(MAX_MIDIFX_LENGTH)*(tim_midifx_length)/(MAX_MIDIFX_LENGTH);
            movePxMID=0;
            if (tim_midifx_length>MAX_MIDIFX_LENGTH) tim_midifx_length=MAX_MIDIFX_LENGTH;
            if (tim_midifx_length<=MIDIFX_OFS) tim_midifx_length=MIDIFX_OFS+1;
        }
        
        movePxMID=0;
        movePyMID=0;
        
        if (tim_midifx_note_offset<0) {
            tim_midifx_note_offset=0;
        }
        if ( (tim_midifx_note_offset/note_fx_linewidth+tim_midifx_note_range)>=MAX_MIDI_NOTES ) {
            tim_midifx_note_offset=(MAX_MIDI_NOTES-tim_midifx_note_range)*note_fx_linewidth;
        }
        
        float visible_wkeys_range=(tim_midifx_note_range*7.0/12.0);
        noteroll_fx_keywidth=(float)(ww)/visible_wkeys_range;
        
        if (tim_midifx_note_offset_reset) {
            tim_midifx_note_offset_reset=false;
            tim_midifx_note_offset=note_fx_linewidth*(128 - tim_midifx_note_range)/2;
            if (tim_midifx_note_offset<0) tim_midifx_note_offset=0;
            
        }
        
        //compute current center
        float note_visible_center=tim_midifx_note_offset/note_fx_linewidth+(tim_midifx_note_range/2);
        
        //update visible notes range
        if (movePinchScaleFXMID<((DEFAULT_VISIBLE_MIDI_NOTES-MAX_VISIBLE_MIDI_NOTES)/64.0f)) movePinchScaleFXMID=((DEFAULT_VISIBLE_MIDI_NOTES-MAX_VISIBLE_MIDI_NOTES)/64.0f);
        if (movePinchScaleFXMID>((DEFAULT_VISIBLE_MIDI_NOTES-MIN_VISIBLE_MIDI_NOTES)/64.0f)) movePinchScaleFXMID=(DEFAULT_VISIBLE_MIDI_NOTES-MIN_VISIBLE_MIDI_NOTES)/64.0f;
        tim_midifx_note_range=DEFAULT_VISIBLE_MIDI_NOTES-movePinchScaleFXMID*64.0f;
        
        if  (tim_midifx_note_range<MIN_VISIBLE_MIDI_NOTES) {
            tim_midifx_note_range=MIN_VISIBLE_MIDI_NOTES;
        }
        if (tim_midifx_note_range>MAX_VISIBLE_MIDI_NOTES) tim_midifx_note_range=MAX_VISIBLE_MIDI_NOTES;
        
        //update bar width
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==2) {
            //vert
            note_fx_linewidth=ww/tim_midifx_note_range;
        } else {
            //horiz
            note_fx_linewidth=hh/tim_midifx_note_range;
        }
        visible_wkeys_range=(tim_midifx_note_range*7.0/12.0);
        noteroll_fx_keywidth=(float)(ww)/visible_wkeys_range;
        
        //recompute offset to get same center
        tim_midifx_note_offset=(note_visible_center-(tim_midifx_note_range/2))*note_fx_linewidth;
        
        if (tim_midifx_note_offset<0) {
            tim_midifx_note_offset=0;
        }
        if ( (tim_midifx_note_offset/note_fx_linewidth+tim_midifx_note_range)>=MAX_MIDI_NOTES ) {
            tim_midifx_note_offset=(MAX_MIDI_NOTES-tim_midifx_note_range)*note_fx_linewidth;
        }
        
    }
    
    
    playerpos=[mplayer getCurrentGenBufferIdx];
            
    //max rendering size
    float maxLength=(settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value-1?ww:hh);
    //used to store min & max rendering pos
    mScaleInfo[0]=maxLength;
    mScaleInfo[1]=0;
    
    RenderUtils::DrawMidiFX(x,y,ww,hh,settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value-1,tim_midifx_note_range,tim_midifx_note_offset,tim_midifx_length,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value,mScaleFactor,mScaleInfo);

    if (settings[GLOB_FXMIDIPatternAutoScale].detail.mdz_boolswitch.switch_value) {
        //rendered size
        float sizeFx=mScaleInfo[1]-mScaleInfo[0]+1;
        //too large
        if ( sizeFx>=maxLength ) {
            movePinchScaleFXMID-=2.0*1.0/64.0;
            //tim_midifx_note_offset+=1;
        } else {
            //too low / bass
            if ( (mScaleInfo[0]<0) ) {
                float diff=-mScaleInfo[0]/4;
                if (diff>8) diff=8;
                if (diff<0.4) diff=0.4;
                tim_midifx_note_offset-=diff;
            }
            //too high / treble
            if ( (mScaleInfo[1]>maxLength) ) {
                float diff=(mScaleInfo[1]-maxLength)/4;
                if (diff>8) diff=8;
                if (diff<0.4) diff=0.4;
                tim_midifx_note_offset+=diff;
            }
            
            //too small
            if (sizeFx<=maxLength*0.9) mScaleInfo[2]+=1;
            else mScaleInfo[2]=0;
            if ( (mScaleInfo[2]>FX_AUTO_SCALING_DELAY_ZOOMIN_FAST)) {
                if (movePinchScaleFXMID<((DEFAULT_VISIBLE_MIDI_NOTES-MIN_VISIBLE_MIDI_NOTES)/64.0f)) {
                    movePinchScaleFXMID+=1.0*1.0/64.0;
                    //tim_midifx_note_offset-=2.0;
                }
            }
        }
    }
}

- (void) doFxPiano3D:(ImVec4)fxSize {
    float x=fxSize.x;
    float y=fxSize.y;
    float ww=fxSize.z;
    float hh=fxSize.w;
    static float piano_posx=0;
    static float piano_posy=0;
    static float piano_posz=0;
    static float piano_rotx=0;
    static float piano_roty=0;
    
    switch (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value) {
        case 1:
            RenderUtils::DrawPiano3D(x,y,ww,hh,1,0,0,0,0,0,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value);
            break;
        case 2:
            RenderUtils::DrawPiano3DWithNotesWall(x,y,ww,hh,1,0,0,0,0,0,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value,settings[GLOB_FXLOD].detail.mdz_switch.switch_value);
            break;
        case 3:
            if (movePinchScaleFXPiano<-0/4) movePinchScaleFXPiano=-0/4;
            if (movePinchScaleFXPiano>9.0/4) movePinchScaleFXPiano=9.0/4;
            piano_rotx=movePyFXPiano;
            piano_roty=movePxFXPiano;
            piano_posx=movePx2FXPiano*0.05;
            piano_posy=-movePy2FXPiano*0.05;
            piano_posz=movePinchScaleFXPiano*100*4;
            RenderUtils::DrawPiano3D(x,y,ww,hh,0,piano_posx,piano_posy,piano_posz,piano_rotx,piano_roty,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value);
            break;
        case 4:
            if (movePinchScaleFXPiano<-0.8/4) movePinchScaleFXPiano=-0.8/4;
            if (movePinchScaleFXPiano>14.0/4) movePinchScaleFXPiano=14.0/4;
            piano_rotx=movePyFXPiano;
            piano_roty=movePxFXPiano;
            piano_posx=movePx2FXPiano*0.05;
            piano_posy=-movePy2FXPiano*0.05;
            piano_posz=movePinchScaleFXPiano*100*4;
            RenderUtils::DrawPiano3DWithNotesWall(x,y,ww,hh,0,piano_posx,piano_posy,piano_posz,piano_rotx,piano_roty,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value,settings[GLOB_FXLOD].detail.mdz_switch.switch_value);
            break;
    }
}

- (void) doFxPianoRoll:(ImVec4)fxSize {
    float x=fxSize.x;
    float y=fxSize.y;
    float ww=fxSize.z;
    float hh=fxSize.w;
    
    /*******************************************************/
    /* Compute pianoroll display scrolling */
    /*******************************************************/
    if ( ([mplayer isMidiLikeDataAvailable]||mplayer.mPatternDataAvail)&&
        settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value ) {
        float noteroll_fx_keywidth;
        //scroll  & get current note bar width
        
        prollfx_noteroll_offset+=-movePxPRoll;
        
        movePxPRoll=0;
        movePyPRoll=0;
        
        float visible_wkeys_range=(prollfx_note_range*7.0/12.0);
        noteroll_fx_keywidth=(float)(ww)/visible_wkeys_range;
        if (prollfx_noteroll_offset<0) {
            prollfx_noteroll_offset=0;
        }
        if ( (prollfx_noteroll_offset>(MAX_MIDI_NOTES-prollfx_note_range)*noteroll_fx_keywidth*7.0/12.0) ) {
            prollfx_noteroll_offset=(MAX_MIDI_NOTES-prollfx_note_range)*noteroll_fx_keywidth*7.0/12.0;
        }
        
        if (prollfx_note_offset_reset) {
            prollfx_note_offset_reset=false;
            prollfx_noteroll_offset=noteroll_fx_keywidth*(128 - prollfx_note_range)/2.0*7.0/12.0;
            if (prollfx_noteroll_offset<0) prollfx_noteroll_offset=0;
        }
        
        //compute current center
        float noteroll_visible_center=prollfx_noteroll_offset*12.0/7.0/noteroll_fx_keywidth+(prollfx_note_range/2);
        
        //update visible notes range
        if (movePinchScaleFXPRoll<((DEFAULT_VISIBLE_MIDI_NOTES-MAX_VISIBLE_MIDI_NOTES)/64.0f)) movePinchScaleFXPRoll=((DEFAULT_VISIBLE_MIDI_NOTES-MAX_VISIBLE_MIDI_NOTES)/64.0f);
        if (movePinchScaleFXPRoll>((DEFAULT_VISIBLE_MIDI_NOTES-MIN_VISIBLE_MIDI_NOTES*1.5)/64.0f)) movePinchScaleFXPRoll=(DEFAULT_VISIBLE_MIDI_NOTES-MIN_VISIBLE_MIDI_NOTES*1.5)/64.0f;
        prollfx_note_range=DEFAULT_VISIBLE_MIDI_NOTES-movePinchScaleFXPRoll*64.0f;
        
        if  (prollfx_note_range<MIN_VISIBLE_MIDI_NOTES) {
            prollfx_note_range=MIN_VISIBLE_MIDI_NOTES;
        }
        if (prollfx_note_range>MAX_VISIBLE_MIDI_NOTES) prollfx_note_range=MAX_VISIBLE_MIDI_NOTES;
        
        //update bar width
        visible_wkeys_range=(prollfx_note_range*7.0/12.0);
        noteroll_fx_keywidth=(float)(ww)/visible_wkeys_range;
        
        //recompute offset to get same center
        prollfx_noteroll_offset=(noteroll_visible_center-(prollfx_note_range/2))*7.0/12.0*noteroll_fx_keywidth;
        if (prollfx_noteroll_offset<0) {
            prollfx_noteroll_offset=0;
        }
        if ( (prollfx_noteroll_offset>(MAX_MIDI_NOTES-prollfx_note_range)*noteroll_fx_keywidth*7.0/12.0) ) {
            prollfx_noteroll_offset=(MAX_MIDI_NOTES-prollfx_note_range)*noteroll_fx_keywidth*7.0/12.0;
        }
    }
    
    memset(voicesName,0,sizeof(voicesName));
    for (int i=0;i<[mplayer getNumChannels];i++) {
        snprintf(voicesName+i*32,31,"%s",[[mplayer getVoicesName:i onlyMidi:true] UTF8String]);
    }
    
    //max rendering size
    float maxLength=ww;
    //used to store min & max rendering pos
    float oldScaleMin,oldScaleMax;
    oldScaleMin=mScaleInfo[0];
    oldScaleMax=mScaleInfo[1];
    if (mScaleInfo[4]>=FX_AUTO_SCALING_DELAY_ZOOMIN_FAST) {
        mScaleInfo[0]=maxLength;
        mScaleInfo[1]=0;
        mScaleInfo[4]=0;
    }
    
    int delay_threshold=FX_AUTO_SCALING_DELAY_ZOOMIN_FAST;
    
    switch (settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value) {
        case 1:
            delay_threshold=FX_AUTO_SCALING_DELAY_ZOOMIN_SLOW;
            RenderUtils::DrawPianoRollFX(x,y,ww,hh,settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value-1,prollfx_note_range,prollfx_noteroll_offset,prollfx_length,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value,mScaleFactor,(char*)voicesName,mScaleInfo);
            break;
        case 2:
            RenderUtils::DrawPianoRollSynthesiaFX(x,y,ww,hh,settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value-1,prollfx_note_range,prollfx_noteroll_offset,prollfx_length,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value,mScaleFactor,(char*)voicesName,mScaleInfo);
            break;
    }
    
    varCheck[0]=mScaleInfo[0];
    varCheck[1]=mScaleInfo[1];
    varCheck[2]=mScaleInfo[3];
    varCheck[3]=mScaleInfo[4];
    
    
    if (settings[GLOB_FXPianoRollFXAutoScale].detail.mdz_boolswitch.switch_value) {
        if ( (oldScaleMin>mScaleInfo[0]) || (oldScaleMax<mScaleInfo[1]) ) {
            //change of min or max, reset counter
            mScaleInfo[4]=0;
        } else mScaleInfo[4]++;
        
        //rendered size
        float sizeFx=mScaleInfo[1]-mScaleInfo[0]+1;
        //too large to fit
        if ( sizeFx>=maxLength ) {
            movePinchScaleFXPRoll-=2.0*1.0/64.0;
            mScaleInfo[4]=FX_AUTO_SCALING_DELAY_ZOOMIN_FAST;
            //tim_midifx_note_offset+=1;
        } else {
            //too low / bass
            if ( (mScaleInfo[0]<0) ) {
                float diff=-mScaleInfo[0]/4;
                if (diff>8) diff=8;
                if (diff<0.4) diff=0.4;
                prollfx_noteroll_offset-=diff;
                mScaleInfo[4]=FX_AUTO_SCALING_DELAY_ZOOMIN_FAST;
            }
            //too high / treble
            if ( (mScaleInfo[1]>maxLength) ) {
                float diff=(mScaleInfo[1]-maxLength)/4;
                if (diff>8) diff=8;
                if (diff<0.4) diff=0.4;
                prollfx_noteroll_offset+=diff;
                mScaleInfo[4]=FX_AUTO_SCALING_DELAY_ZOOMIN_FAST;
            }
            
            //too small, should be zoomed
            if (sizeFx<=maxLength*0.8) {
                mScaleInfo[3]+=1;
            }
            else {
                mScaleInfo[3]=0;
            }
            
            if ( (mScaleInfo[3]>delay_threshold)) {
                if (movePinchScaleFXPRoll<((DEFAULT_VISIBLE_MIDI_NOTES-MIN_VISIBLE_MIDI_NOTES*1.5)/64.0f)) {
                    movePinchScaleFXPRoll+=1.0*1.0/64.0;
                } else mScaleInfo[3]=0;
                mScaleInfo[4]=FX_AUTO_SCALING_DELAY_ZOOMIN_FAST;
            }
        }
    }
}

- (void) doFxModPatterns:(ImVec4)fxSize {
    float x=fxSize.x;
    float y=fxSize.y;
    float ww=fxSize.z;
    float hh=fxSize.w;
    
#define MAX_STR_DATA_SIZE 66*SOUND_MAXMOD_CHANNELS+1
    char str_data[MAX_STR_DATA_SIZE];
    unsigned int cnote,cinst,ceff,cparam,cvol,endChan;
    int numRows,numRowsP,numRowsN;
    int i,j,k,l,note_avail,idx,startRow;
    int linestodraw,midline;
    ModPlugNote *currentNotes,*prevNotes,*nextNotes,*readNotes;
    
    //LIBOMPT or LIBXMP
    //DISPLAY MOD PATTERNS
    
    /*******************************************************/
    /* Compute pattern display scrolling */
    /*******************************************************/
    if ((mplayer.mPatternDataAvail)&&(settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value)) {//pattern display
        if (modPatternLineSize<=modPatternWindowSize) movePxMOD=0;
        else {
            if (modPatternWindowSize-movePxMOD*glScaleFactor>=modPatternLineSize) movePxMOD=-(modPatternLineSize-modPatternWindowSize)/glScaleFactor;
            else if (movePxMOD>0) movePxMOD=0;
        }
    }
    
    //------------------------------------------------
    // Select current mod pattern themes
    //------------------------------------------------
    modpat_curTheme=modpat_themesList[(settings[GLOB_FXMODPattern_Theme].detail.mdz_switch.switch_value)%modpat_themesNb];
    
    int display_note_mode=(settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value-1);
    if (display_note_mode>=3) display_note_mode-=3;
    
    float fontSize=16;
    switch (settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value) {
        case 0: //
            fontSize=mdz_font_size[0];
            break;
        case 1: //
            fontSize=mdz_font_size[1];
            break;
        case 2: //
            fontSize=mdz_font_size[2];
            break;
        case 3: //
            fontSize=mdz_font_size[3];
            break;
    }
    
    if (settings[GLOB_FXMODPattern_BGAlpha].detail.mdz_slider.slider_value>0) {
        //Darken the background
        RenderUtils::FillArea(0, 0, ww, hh, ww, hh,glScaleFactor, settings[GLOB_FXMODPattern_BGAlpha].detail.mdz_slider.slider_value*255.0);
    }
    
    ImGui::GetStyle().Alpha=1.0f;
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(0,0,0,0));
    
    int cur_font=settings[GLOB_FXMODPattern_Font].detail.mdz_switch.switch_value;
    if (cur_font>=FONT_TRACKER_NB) cur_font=FONT_TRACKER_NB-1;
    
    float font_ofsX,font_ofsY;
    if (font_tracker[cur_font]) { ImGui::PushFont(font_tracker[cur_font],fontSize*glScaleFactor);
        font_ofsX=font_trackerSize[cur_font][3]*fontSize/FONT_BASE_SIZEF*font_trackerSize[cur_font][2];
        font_ofsY=font_trackerSize[cur_font][4]*fontSize/FONT_BASE_SIZEF*font_trackerSize[cur_font][2];
    }
    else {
        ImGui::PushFont(nullptr);
        font_ofsX=0;
        font_ofsY=0;
    }
    //                ImGui::Begin("ModPattern",0,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoFocusOnAppearing);
    
    //ImGui::SetCursorPos(ImVec2(0,0));
    
    //Compute how many lines to draw
    float lineHeight=(fontSize+1)*glScaleFactor;//ImGui::GetTextLineHeight();
    
    linestodraw=((float)hh*glScaleFactor-lineHeight-4.0*glScaleFactor+lineHeight-1)/lineHeight;
    //linestodraw=round((hh*glScaleFactor-NOTES_DISPLAY_TOPMARGIN+lineHeight/mScaleFactor+3)/(lineHeight/mScaleFactor+4)); //draw even if halfed for last line
    //int limit_midline=round((hh*glScaleFactor-NOTES_DISPLAY_TOPMARGIN)/(lineHeight/mScaleFactor+4)); //draw even if halfed for last line
    int limit_midline=((float)hh*glScaleFactor-lineHeight-4.0*glScaleFactor)/lineHeight;
    midline=0;//linestodraw>>1;
    
    
    //Get access to notes data / patterns
    int *pat,*row;
    int playerpos;
    pat=[mplayer playPattern];
    row=[mplayer playRow];
    playerpos=[mplayer getCurrentPlayedBufferIdx];
    currentPattern=pat[playerpos];
    currentRow=row[playerpos];
    
    if ( (currentPattern>=0)&&(currentRow>=0) ) {
        
        currentNotes=[mplayer ompt_getPattern:currentPattern numrows:(unsigned int*)(&numRows)];
        prevNotes=nil;
        nextNotes=nil;
        prevPattern=[mplayer prevPattern][playerpos];
        if (prevPattern>=0) prevNotes=[mplayer ompt_getPattern:prevPattern numrows:(unsigned int*)(&numRowsP)];
        
        nextPattern=[mplayer nextPattern][playerpos];
        if (nextPattern>=0) nextNotes=[mplayer ompt_getPattern:nextPattern numrows:(unsigned int*)(&numRowsN)];
        
        if (settings[GLOB_FXMODPattern_CurrentLineMode].detail.mdz_switch.switch_value) midline=linestodraw>>1;
        else {
            midline=currentRow;
            if (midline>=limit_midline) midline=limit_midline-1;
        }
        
        endChan=mplayer.numChannels;
        startRow=currentRow-midline;
        
        int channelVolumeData[SOUND_MAXMOD_CHANNELS];
        unsigned char *volData=mplayer.playVolData;
        for (int i=0;i<endChan;i++) {
            channelVolumeData[i]=volData[playerpos*SOUND_MAXMOD_CHANNELS+i];
        }
        
        idx=startRow*mplayer.numChannels;
        
        if (fontWidth==0) fontWidth=ImGui::CalcTextSize("ABCDEFGH").x/8.0;
        RenderUtils::DrawChanLayout(x,y,ww,hh,display_note_mode,endChan,((int)(movePxMOD)),fontWidth/glScaleFactor,fontSize+1,glScaleFactor);
        
        if (settings[GLOB_FXMODPattern_VolBar].detail.mdz_boolswitch.switch_value) {
            RenderUtils::DrawChanLayoutAfter(x,y,ww,hh,display_note_mode,channelVolumeData,endChan,((int)(movePxMOD)),fontWidth/mScaleFactor,fontSize+1,0,midline,mScaleFactor);
        } else {
            RenderUtils::DrawChanLayoutAfter(x,y,ww,hh,display_note_mode,NULL,endChan,((int)(movePxMOD)),fontWidth/mScaleFactor,fontSize+1,0,midline,mScaleFactor);
        }
        
        
        if (currentNotes) {
            l=0;
            
            //1st win with line nb
            char str_prefix[4];
            ImVec2 cursorPos;
            float startx=(ImGui::CalcTextSize("9999").x);
            modPatternWindowSize=ww*glScaleFactor-startx;
            
            
            ImGui::SetNextWindowPos(ImVec2(x*glScaleFactor,y*glScaleFactor));
            ImGui::SetNextWindowSize(ImVec2(startx,hh*glScaleFactor));
            ImGui::Begin("ModPatternWin_LinesCol",0,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
                         ImGuiWindowFlags_NoScrollbar|
                         ImGuiWindowFlags_NoFocusOnAppearing);
            
            int colR,colG,colB;
            float color_div;
            
            for (i=startRow;i<startRow+linestodraw;i++) {
                
                str_prefix[3]=0;
                str_prefix[0]=' ';
                if ((i<0)&&prevNotes) {
                    str_prefix[1]=dec2hex[((numRowsP+i)>>4)&0xF];
                    str_prefix[2]=dec2hex[(numRowsP+i)&0xF];
                    color_div=0.7;
                } else if (i<numRows) {
                    str_prefix[1]=dec2hex[(i>>4)&0xF];
                    str_prefix[2]=dec2hex[i&0xF];
                    color_div=1;
                } else if (nextNotes) {
                    str_prefix[1]=dec2hex[((i-numRows)>>4)&0xF];
                    str_prefix[2]=dec2hex[(i-numRows)&0xF];
                    color_div=0.7;
                }
                cursorPos=ImVec2((font_ofsX)*mScaleFactor-fontWidth/3.0f,
                                 (i-startRow+1)*lineHeight+(4.0+font_ofsY)*glScaleFactor);
                
                if ((i==currentRow)&&(modpat_curTheme->theme_flag&MDZ_THEMEFLAG_HighlightZoom)) {
                    if (i&1) {
                        colR=modpat_curTheme->lineNb_col1H[0]*color_div;
                        colG=modpat_curTheme->lineNb_col1H[1]*color_div;
                        colB=modpat_curTheme->lineNb_col1H[2]*color_div;
                    } else {
                        colR=modpat_curTheme->lineNb_col2H[0]*color_div;
                        colG=modpat_curTheme->lineNb_col2H[1]*color_div;
                        colB=modpat_curTheme->lineNb_col2H[2]*color_div;
                    }
                } else {
                    if (i&1) {
                        colR=modpat_curTheme->lineNb_col1[0]*color_div;
                        colG=modpat_curTheme->lineNb_col1[1]*color_div;
                        colB=modpat_curTheme->lineNb_col1[2]*color_div;
                    } else {
                        colR=modpat_curTheme->lineNb_col2[0]*color_div;
                        colG=modpat_curTheme->lineNb_col2[1]*color_div;
                        colB=modpat_curTheme->lineNb_col2[2]*color_div;
                    }
                }
                
                if ((i==currentRow)&&(modpat_curTheme->theme_flag&MDZ_THEMEFLAG_HighlightZoom)) {
                    ImGui::SetCursorPos(cursorPos);
                    ImGui::TextAttrZoom(1.4f,"{#%02X%02X%02X}%s",colR,colG,colB,str_prefix);
                } else {
                    ImGui::SetCursorPos(cursorPos);
                    ImGui::TextAttr("{#%02X%02X%02X}%s",colR,colG,colB,str_prefix);
                }
                
            }
            ImGui::End();
            //2nd win with pattern
            ImGui::SetNextWindowPos(ImVec2(startx+x*glScaleFactor,y*glScaleFactor));
            ImGui::SetNextWindowSize(ImVec2(ww*glScaleFactor-startx,hh*glScaleFactor));
            ImGui::Begin("ModPatternWin2",0,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
                         ImGuiWindowFlags_NoScrollbar|
                         ImGuiWindowFlags_NoFocusOnAppearing);
            
            for (i=startRow;i<startRow+linestodraw;i++) {
                note_avail=0;
                if (i<0) {
                    color_div=0.7;
                    if ((prevNotes)&&((numRowsP+i)>=0)) {
                        if (numRowsP+i>=0) {
                            note_avail=1;
                            idx=(numRowsP+i)*mplayer.numChannels;
                            readNotes=prevNotes;
                        }
                    }
                } else if (currentNotes&&(i<numRows)) {
                    color_div=1;
                    note_avail=1;
                    idx=i*mplayer.numChannels;
                    readNotes=currentNotes;
                } else {
                    color_div=0.7;
                    if ((nextNotes)&&((i-numRows)<numRowsN)) {
                        note_avail=1;
                        idx=(i-numRows)*mplayer.numChannels;
                        readNotes=nextNotes;
                    }
                }
                k=0;
                if (note_avail) {
                    bool highlight=false;
                    if ((i==currentRow)&&(modpat_curTheme->theme_flag&MDZ_THEMEFLAG_HighlightZoom)) highlight=true;
                    switch (display_note_mode) {
                        case 0: //all infos
                            for (j=0;j<endChan;j++)  {
                                cnote=readNotes[idx].Note;
                                cinst=readNotes[idx].Instrument;
                                ceff=readNotes[idx].Effect;
                                cparam=readNotes[idx].Parameter;
                                cvol=readNotes[idx].Volume;
                                
                                if (highlight) {
                                    colR=modpat_curTheme->note_colH[0]*color_div;
                                    colG=modpat_curTheme->note_colH[1]*color_div;
                                    colB=modpat_curTheme->note_colH[2]*color_div;
                                } else {
                                    colR=modpat_curTheme->note_col[0]*color_div;
                                    colG=modpat_curTheme->note_col[1]*color_div;
                                    colB=modpat_curTheme->note_col[2]*color_div;
                                }
                                str_data[k++]='{';str_data[k++]='#';
                                str_data[k++]=dec2hex[(colR>>4)&0xF];str_data[k++]=dec2hex[colR&0xF];
                                str_data[k++]=dec2hex[(colG>>4)&0xF];str_data[k++]=dec2hex[colG&0xF];
                                str_data[k++]=dec2hex[(colB>>4)&0xF];str_data[k++]=dec2hex[colB&0xF];
                                str_data[k++]='}';
                                
                                if (cnote) {
                                    str_data[k++]=note2charA[(cnote-13)%12];
                                    str_data[k++]=note2charB[(cnote-13)%12];
                                    int note=(cnote-13)/12; if (note<0) note=0; if (note>15) note=15;
                                    str_data[k++]=dec2hex[note];
                                } else {
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                }
                                
                                if (highlight) {
                                    colR=modpat_curTheme->instrument_colH[0]*color_div;
                                    colG=modpat_curTheme->instrument_colH[1]*color_div;
                                    colB=modpat_curTheme->instrument_colH[2]*color_div;
                                } else {
                                    colR=modpat_curTheme->instrument_col[0]*color_div;
                                    colG=modpat_curTheme->instrument_col[1]*color_div;
                                    colB=modpat_curTheme->instrument_col[2]*color_div;
                                }
                                str_data[k++]='{';str_data[k++]='#';
                                str_data[k++]=dec2hex[(colR>>4)&0xF];str_data[k++]=dec2hex[colR&0xF];
                                str_data[k++]=dec2hex[(colG>>4)&0xF];str_data[k++]=dec2hex[colG&0xF];
                                str_data[k++]=dec2hex[(colB>>4)&0xF];str_data[k++]=dec2hex[colB&0xF];
                                str_data[k++]='}';
                                
                                if (cinst) {
                                    str_data[k++]=dec2hex[(cinst>>4)&0xF];
                                    str_data[k++]=dec2hex[cinst&0xF];
                                } else {
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                }
                                
                                if (highlight) {
                                    colR=modpat_curTheme->volume_colH[0]*color_div;
                                    colG=modpat_curTheme->volume_colH[1]*color_div;
                                    colB=modpat_curTheme->volume_colH[2]*color_div;
                                } else {
                                    colR=modpat_curTheme->volume_col[0]*color_div;
                                    colG=modpat_curTheme->volume_col[1]*color_div;
                                    colB=modpat_curTheme->volume_col[2]*color_div;
                                }
                                str_data[k++]='{';str_data[k++]='#';
                                str_data[k++]=dec2hex[(colR>>4)&0xF];str_data[k++]=dec2hex[colR&0xF];
                                str_data[k++]=dec2hex[(colG>>4)&0xF];str_data[k++]=dec2hex[colG&0xF];
                                str_data[k++]=dec2hex[(colB>>4)&0xF];str_data[k++]=dec2hex[colB&0xF];
                                str_data[k++]='}';
                                
                                if (cvol) {
                                    str_data[k++]=dec2hex[(cvol>>4)&0xF];
                                    str_data[k++]=dec2hex[cvol&0xF];
                                } else {
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                }
                                
                                if (highlight) {
                                    colR=modpat_curTheme->effect_colH[0]*color_div;
                                    colG=modpat_curTheme->effect_colH[1]*color_div;
                                    colB=modpat_curTheme->effect_colH[2]*color_div;
                                } else {
                                    colR=modpat_curTheme->effect_col[0]*color_div;
                                    colG=modpat_curTheme->effect_col[1]*color_div;
                                    colB=modpat_curTheme->effect_col[2]*color_div;
                                }
                                str_data[k++]='{';str_data[k++]='#';
                                str_data[k++]=dec2hex[(colR>>4)&0xF];str_data[k++]=dec2hex[colR&0xF];
                                str_data[k++]=dec2hex[(colG>>4)&0xF];str_data[k++]=dec2hex[colG&0xF];
                                str_data[k++]=dec2hex[(colB>>4)&0xF];str_data[k++]=dec2hex[colB&0xF];
                                str_data[k++]='}';
                                
                                if (ceff) {
                                    str_data[k++]='A'+ceff;
                                } else {
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                }
                                
                                if (highlight) {
                                    colR=modpat_curTheme->param_colH[0]*color_div;
                                    colG=modpat_curTheme->param_colH[1]*color_div;
                                    colB=modpat_curTheme->param_colH[2]*color_div;
                                } else {
                                    colR=modpat_curTheme->param_col[0]*color_div;
                                    colG=modpat_curTheme->param_col[1]*color_div;
                                    colB=modpat_curTheme->param_col[2]*color_div;
                                }
                                str_data[k++]='{';str_data[k++]='#';
                                str_data[k++]=dec2hex[(colR>>4)&0xF];str_data[k++]=dec2hex[colR&0xF];
                                str_data[k++]=dec2hex[(colG>>4)&0xF];str_data[k++]=dec2hex[colG&0xF];
                                str_data[k++]=dec2hex[(colB>>4)&0xF];str_data[k++]=dec2hex[colB&0xF];
                                str_data[k++]='}';
                                
                                if (cparam) {
                                    str_data[k++]=dec2hex[(cparam>>4)&0xF];
                                    str_data[k++]=dec2hex[cparam&0xF];
                                } else {
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                }
                                str_data[k++]=' ';
                                idx++;
                            }
                            break;
                        case 1: //note + instru
                            for (j=0;j<endChan;j++)  {
                                cnote=readNotes[idx].Note;
                                cinst=readNotes[idx].Instrument;
                                
                                if (highlight) {
                                    colR=modpat_curTheme->note_colH[0]*color_div;
                                    colG=modpat_curTheme->note_colH[1]*color_div;
                                    colB=modpat_curTheme->note_colH[2]*color_div;
                                } else {
                                    colR=modpat_curTheme->note_col[0]*color_div;
                                    colG=modpat_curTheme->note_col[1]*color_div;
                                    colB=modpat_curTheme->note_col[2]*color_div;
                                }
                                str_data[k++]='{';str_data[k++]='#';
                                str_data[k++]=dec2hex[(colR>>4)&0xF];str_data[k++]=dec2hex[colR&0xF];
                                str_data[k++]=dec2hex[(colG>>4)&0xF];str_data[k++]=dec2hex[colG&0xF];
                                str_data[k++]=dec2hex[(colB>>4)&0xF];str_data[k++]=dec2hex[colB&0xF];
                                str_data[k++]='}';
                                
                                if (cnote) {
                                    str_data[k++]=note2charA[(cnote-13)%12];
                                    str_data[k++]=note2charB[(cnote-13)%12];
                                    int note=(cnote-13)/12; if (note<0) note=0; if (note>15) note=15;
                                    str_data[k++]=dec2hex[note];
                                } else {
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                }
                                
                                if (highlight) {
                                    colR=modpat_curTheme->instrument_colH[0]*color_div;
                                    colG=modpat_curTheme->instrument_colH[1]*color_div;
                                    colB=modpat_curTheme->instrument_colH[2]*color_div;
                                } else {
                                    colR=modpat_curTheme->instrument_col[0]*color_div;
                                    colG=modpat_curTheme->instrument_col[1]*color_div;
                                    colB=modpat_curTheme->instrument_col[2]*color_div;
                                }
                                str_data[k++]='{';str_data[k++]='#';
                                str_data[k++]=dec2hex[(colR>>4)&0xF];str_data[k++]=dec2hex[colR&0xF];
                                str_data[k++]=dec2hex[(colG>>4)&0xF];str_data[k++]=dec2hex[colG&0xF];
                                str_data[k++]=dec2hex[(colB>>4)&0xF];str_data[k++]=dec2hex[colB&0xF];
                                str_data[k++]='}';
                                
                                if (cinst) {
                                    str_data[k++]=dec2hex[(cinst>>4)&0xF];
                                    str_data[k++]=dec2hex[cinst&0xF];
                                } else {
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                }
                                str_data[k++]=' ';
                                idx++;
                            }
                            break;
                        case 2: //only note
                            for (j=0;j<endChan;j++)  {
                                cnote=readNotes[idx].Note;
                                
                                if (highlight) {
                                    colR=modpat_curTheme->note_colH[0]*color_div;
                                    colG=modpat_curTheme->note_colH[1]*color_div;
                                    colB=modpat_curTheme->note_colH[2]*color_div;
                                } else {
                                    colR=modpat_curTheme->note_col[0]*color_div;
                                    colG=modpat_curTheme->note_col[1]*color_div;
                                    colB=modpat_curTheme->note_col[2]*color_div;
                                }
                                str_data[k++]='{';str_data[k++]='#';
                                str_data[k++]=dec2hex[(colR>>4)&0xF];str_data[k++]=dec2hex[colR&0xF];
                                str_data[k++]=dec2hex[(colG>>4)&0xF];str_data[k++]=dec2hex[colG&0xF];
                                str_data[k++]=dec2hex[(colB>>4)&0xF];str_data[k++]=dec2hex[colB&0xF];
                                str_data[k++]='}';
                                
                                if (cnote) {
                                    str_data[k++]=note2charA[(cnote-13)%12];
                                    str_data[k++]=note2charB[(cnote-13)%12];
                                    int note=(cnote-13)/12; if (note<0) note=0; if (note>15) note=15;
                                    str_data[k++]=dec2hex[note];
                                } else {
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                    str_data[k++]=ASCII_MIDDOT[0];str_data[k++]=ASCII_MIDDOT[1];
                                }
                                str_data[k++]=' ';
                                idx++;
                            }
                            break;
                    }
                    str_data[k]=0;
                    //mText[l++] = new CGLString(str_data, mFont,mScaleFactor);
                    
                } else {
                    //mText[l++] = NULL;
                    str_data[k]=0;
                }
                
                cursorPos.y=(i-startRow+1)*lineHeight+(4.0+font_ofsY)*glScaleFactor;
                cursorPos.x=font_ofsX*glScaleFactor;
                
                if ((i==currentRow)&&(modpat_curTheme->theme_flag&MDZ_THEMEFLAG_HighlightZoom)) {
                    ImGui::SetCursorPos(cursorPos);
                    ImGui::TextAttrZoom(1.4f,"%s",str_data);
                } else {
                    ImGui::SetCursorPos(cursorPos);
                    ImGui::TextAttr("%s",str_data);
                }
                
                
                
            }
            ImGui::SetScrollX(-movePxMOD*glScaleFactor);
            ImGui::End();
            
            //3rd win: draw header
            ImGui::SetNextWindowPos(ImVec2(startx+x*glScaleFactor,y*glScaleFactor));
            ImGui::SetNextWindowSize(ImVec2(ww*glScaleFactor-startx,lineHeight));
            ImGui::Begin("ModPatternWin3",0,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
                         ImGuiWindowFlags_NoScrollbar|
                         ImGuiWindowFlags_NoFocusOnAppearing);
            
            memset(str_data,32,11*mplayer.numChannels);//visibleChan);
            str_data[11*mplayer.numChannels]=0; //11 chars max / channel
            float xofs=0;
            int str_size=1;
            
            switch (display_note_mode) {
                case 0:
                    for (j=0;j<endChan;j++) {
                        str_data[(j-0)*11+4]='0'+(j+1)/10;
                        str_data[(j-0)*11+5]='0'+(j+1)%10;
                    }
                    //str_data[(endChan-1-0)*11+9]=0;
                    str_data[11*endChan]=0;
                    str_size=11*endChan;
                    xofs=0;
                    break;
                case 1:
                    for (j=0;j<endChan;j++) {
                        str_data[(j-0)*6+2]='0'+(j+1)/10;
                        str_data[(j-0)*6+3]='0'+(j+1)%10;
                    }
                    str_data[6*endChan]=0;
                    str_size=6*endChan;
                    xofs=0.5;
                    break;
                case 2:
                    for (j=0;j<endChan;j++) {
                        str_data[(j-0)*4+1]='0'+(j+1)/10;
                        str_data[(j-0)*4+2]='0'+(j+1)%10;
                    }
                    str_data[4*endChan]=0;
                    str_size=4*endChan;
                    xofs=0.5;
                    break;
            }
            header_w=ImGui::CalcTextSize(str_data).x;
            fontWidth=round(header_w/str_size);
            xofs*=fontWidth;
            
            if (note_avail) modPatternLineSize=header_w;
            
            ImGui::SetCursorPos(ImVec2(-xofs+font_ofsX*glScaleFactor,(4.0+font_ofsY/2.0)*glScaleFactor));
            
            colR=modpat_curTheme->header_col[0];
            colG=modpat_curTheme->header_col[1];
            colB=modpat_curTheme->header_col[2];
            
            ImGui::TextAttr("{#%02X%02X%02X}%s",colR,colG,colB,str_data);
            
            ImGui::SetScrollX(-movePxMOD*glScaleFactor);
            ImGui::End();
        }
    }
    ImGui::PopFont();
    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
}

- (void) doFxOscillo:(ImVec4)fxSize {
    short int **snd_buffer;
    int cur_pos;
    snd_buffer=[mplayer buffer_ana_cpy];
    cur_pos=[mplayer getCurrentPlayedBufferIdx];
    float x=fxSize.x;
    float y=fxSize.y;
    float ww=fxSize.z;
    float hh=fxSize.w;
    
    switch (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value) {
        case 1:
            if ([mplayer m_voicesDataAvail]) {
                if (settings[OSCILLO_ShowLabel].detail.mdz_boolswitch.switch_value) {
                    memset(voicesName,0,sizeof(voicesName));
                    for (int i=0;i<[mplayer getNumChannels];i++) {
                        snprintf(voicesName+i*32,31,"%s",[[mplayer getVoicesName:i onlyMidi:false] UTF8String]);
                    }
                    RenderUtils::DrawOscilloMultiple(x,y,ww,hh,m_voice_buff_ana_cpy,cur_pos,([mplayer getNumChannels]<SOUND_MAXVOICES_BUFFER_FX?[mplayer getNumChannels]:SOUND_MAXVOICES_BUFFER_FX),1,mScaleFactor,settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value,
                                                     0/*settings[GLOB_BLOOMFX].detail.mdz_boolswitch.switch_value*/,
                                                     (char*)voicesName,settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value);
                } else {
                    RenderUtils::DrawOscilloMultiple(x,y,ww,hh,m_voice_buff_ana_cpy,cur_pos,([mplayer getNumChannels]<SOUND_MAXVOICES_BUFFER_FX?[mplayer getNumChannels]:SOUND_MAXVOICES_BUFFER_FX),1,mScaleFactor,settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value,
                                                     0/*settings[GLOB_BLOOMFX].detail.mdz_boolswitch.switch_value*/,
                                                     NULL,settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value);
                }
            } else {
                if (settings[OSCILLO_ShowLabel].detail.mdz_boolswitch.switch_value) {
                    memset(voicesName,0,sizeof(voicesName));
                    snprintf(voicesName+0*32,31,"Left");
                    snprintf(voicesName+1*32,31,"Right");
                    RenderUtils::DrawOscilloMultiple(x,y,ww,hh,(signed char **)snd_buffer,cur_pos,2,1,mScaleFactor,settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value,
                                                     0,
                                                     (char*)voicesName,settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value,1);
                } else {
                    RenderUtils::DrawOscilloMultiple(x,y,ww,hh,(signed char **)snd_buffer,cur_pos,2,1,mScaleFactor,settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value,
                                                     0,
                                                     NULL,settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value,1);
                }
                
                
            }
            break;
        case 2:
            if ([mplayer m_voicesDataAvail]) {
                char voicesName[SOUND_MAXVOICES_BUFFER_FX*32];
                
                if (settings[OSCILLO_ShowLabel].detail.mdz_boolswitch.switch_value) {
                    memset(voicesName,0,sizeof(voicesName));
                    for (int i=0;i<[mplayer getNumChannels];i++) {
                        snprintf(voicesName+i*32,31,"%s",[[mplayer getVoicesName:i onlyMidi:false] UTF8String]);
                    }
                    
                    RenderUtils::DrawOscilloMultiple(x,y,ww,hh,m_voice_buff_ana_cpy,cur_pos,([mplayer getNumChannels]<SOUND_MAXVOICES_BUFFER_FX?[mplayer getNumChannels]:SOUND_MAXVOICES_BUFFER_FX),2,mScaleFactor,settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value,
                                                     0/*settings[GLOB_BLOOMFX].detail.mdz_boolswitch.switch_value*/,
                                                     (char*)voicesName,settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value);
                } else {
                    RenderUtils::DrawOscilloMultiple(x,y,ww,hh,m_voice_buff_ana_cpy,cur_pos,([mplayer getNumChannels]<SOUND_MAXVOICES_BUFFER_FX?[mplayer getNumChannels]:SOUND_MAXVOICES_BUFFER_FX),2,mScaleFactor,settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value,
                                                     0/*settings[GLOB_BLOOMFX].detail.mdz_boolswitch.switch_value*/,
                                                     NULL,settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value);
                }
            } else {
                RenderUtils::DrawOscilloMultiple(x,y,ww,hh,(signed char **)snd_buffer,cur_pos,2,1,mScaleFactor,settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value,
                                                 0/*settings[GLOB_BLOOMFX].detail.mdz_boolswitch.switch_value*/,
                                                 NULL,settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value,1);
            }
            break;
        case 3:
            RenderUtils::DrawOscilloMultiple(x,y,ww,hh,(signed char **)snd_buffer,cur_pos,2,1,mScaleFactor,settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value,
                                             0/*settings[GLOB_BLOOMFX].detail.mdz_boolswitch.switch_value*/,
                                             NULL,settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value,1);
            break;
        case 4:
            RenderUtils::DrawOscilloMultiple(x,y,ww,hh,(signed char **)snd_buffer,cur_pos,2,2,mScaleFactor,settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value,
                                             0/*settings[GLOB_BLOOMFX].detail.mdz_boolswitch.switch_value*/,
                                             NULL,settings[OSCILLO_ShowGrid].detail.mdz_boolswitch.switch_value,1);
            break;
    }
}

bool isFXinRangeLPtouch(float x,float y,float w,float h) {
    if ( (oglLPTapXstart>=x) && (oglLPTapXstart<=(x+w)) &&
        (oglLPTapYstart>=y) && (oglLPTapYstart<=(y+h)) ) return YES;
    return NO;
}

bool isFXinRangeLPtouchCurrent(float x,float y,float w,float h) {
    if ( (oglLPTapX>=x) && (oglLPTapX<=(x+w)) &&
        (oglLPTapY>=y) && (oglLPTapY<=(y+h)) ) return YES;
    return NO;
}

bool isOverlappingSlots(int slot1,int slot2) {
    switch (slot1) {
        case 0: //full
            return true;
        case 1: //left
            if ((slot2==0)||(slot2==1)||(slot2==3)||(slot2==4)||(slot2==5)||(slot2==7)) return true;
            return false;
        case 2: //right
            if ((slot2==0)||(slot2==2)||(slot2==3)||(slot2==4)||(slot2==6)||(slot2==8)) return true;
            return false;
        case 3: //bottom
            if ((slot2==0)||(slot2==1)||(slot2==2)||(slot2==3)||(slot2==5)||(slot2==6)) return true;
            return false;
        case 4: //top
            if ((slot2==0)||(slot2==1)||(slot2==2)||(slot2==4)||(slot2==7)||(slot2==8)) return true;
            return false;
        case 5: //bottom left
            if ((slot2==0)||(slot2==1)||(slot2==3)||(slot2==5)) return true;
            return false;
        case 6: //bottom right
            if ((slot2==0)||(slot2==2)||(slot2==3)||(slot2==6)) return true;
            return false;
        case 7: //top left
            if ((slot2==0)||(slot2==1)||(slot2==4)||(slot2==7)) return true;
            return false;
        case 8: //top right
            if ((slot2==0)||(slot2==2)||(slot2==4)||(slot2==8)) return true;
            return false;
    }
    return false;
}

void initViewPortData(int fxidx,float &x,float &y,float &w,float &h,float ww,float hh) {
    switch (fxSlot[fxidx]) {
        default:
        case 0: //full
            x=0; y=0;
            w=ww; h=hh;
            break;
        case 1: //split vertically, left
            x=0; y=0;
            w=ww/2; h=hh;
            break;
        case 2: //split vertically, right
            x=ww/2; y=0;
            w=ww/2; h=hh;
            break;
        case 3: //split horizontally, bottom
            x=0; y=hh/2;
            w=ww; h=hh/2;
            break;
        case 4: //split horizontally, top
            x=0; y=0;
            w=ww; h=hh/2;
            break;
        case 5: //split vert&hor, bottom left
            x=0; y=hh/2;
            w=ww/2; h=hh/2;
            break;
        case 6: //split vert&hor, bottom right
            x=ww/2; y=hh/2;
            w=ww/2; h=hh/2;
            break;
        case 7: //split vert&hor, top left
            x=0; y=0;
            w=ww/2; h=hh/2;
            break;
        case 8: //split vert&hor, top right
            x=ww/2; y=0;
            w=ww/2; h=hh/2;
            break;
    }
    //is a long press touch in progress
    //and is the current FX not selected
    if (oglLPTap && (fxLPselected!=fxidx)) {
        //is the org touch point in range of the FX used slot
        if (isFXinRangeLPtouch(x,y,w,h)) {
            //is there already a FX selected
            if (fxLPselected==-1) {
                //no: select current one
                fxLPselectedCpt=0;
                fxLPselected=fxidx;
            } else if (isFXinRangeLPtouchCurrent(x,y,w,h)) {
                //yes: check if we should swap with current one
                if ( (fxLPselected!=fxidx)&&(fxLPselectedCpt>(settings[GLOB_FXFPS].detail.mdz_switch.switch_value?60:30)*FXSLOT_LPTOUCH_ACTIVTATION_DELAY) ) {
                    fxLPselected=fxidx;
                    fxLPselectedCpt=FXSLOT_LPTOUCH_ACTIVTATION_DELAY-FXSLOT_LPTOUCH_CYCLING_DELAY;
                } else fxLPselectedCpt++;
            }
        }
    }
    
    if (fxLPselected==fxidx) {
        w=ww/2; h=hh/2;
        x=oglLPTapX-w/2;
        y=hh-oglLPTapY-h/2;
        y=h-y;
    }
}

void drawTgtSlotPattern(int fxIdx,float x,float y,float w,float h,float ww,float hh) {
    if ( (fxLPselected==fxIdx) && (fxTargetSlot>=0) ) {
        //highlight tgt slot
        static int cptA=0;
        int alpha=255-128;
        int col=96+16*sin(cptA*0.03);
        if (col<0) col=0; if (col>255) col=255;
        glViewport(0, 0, ww*glScaleFactor, hh*glScaleFactor);
        RenderUtils::FillAreaPattern(x, y,
                                     w, h, ww, hh,glScaleFactor, fxTargetSlot, alpha,col,col,col);
        cptA++;
    }
}

- (void)doFrame {
    static int no_reentrant=0;
    static int framecpt=0;
    uint ww,hh;
    int nb_spectrum_bands;
    
    float fxalpha;
    int frameToUpdate=0;
    int shouldGoToSettings=0;
    
    if (!_pmIsInitialized) return; //PRojectM might still be initializing and calling some opengl stuff from background thread
    
    if (no_reentrant) {
        MDZELog("reentering doFrame");
        return;
    }
    no_reentrant=1;
    
    framecpt++;
    
    if (shouldUpdateCoverTexture) {
        // Generate new texture / current background view
        [self generateBGTexture];
        [self generateCoverTexture];
        shouldUpdateCoverTexture=0;
    }
    
    if (mOglViewIsHidden) m_oglView.hidden=YES;
    
    //check if view is really visible
    bool isVisible=false;
    if (self.view.window) isVisible=true;
    
    //get ogl view dimension
    ww=m_oglView.frame.size.width;
    hh=m_oglView.frame.size.height;
    
    CFTimeInterval _fx_start_time=CFAbsoluteTimeGetCurrent();
    CFTimeInterval curFrameStartTime=CFAbsoluteTimeGetCurrent();
    if (tgtFrameStartTime==0) {
        tgtFrameStartTime=curFrameStartTime;
        frameToUpdate=1;
    }
    else {
        double time_diff=curFrameStartTime-tgtFrameStartTime;
        double fps_to_draw=time_diff*(settings[GLOB_FXFPS].detail.mdz_switch.switch_value?60:30);
        frameToUpdate=round(fps_to_draw);
        if (frameToUpdate<1) frameToUpdate=1;
        tgtFrameStartTime=curFrameStartTime;
    }
    
    //tgtFrameCnt=0;
    int frameToUpdateTmp=frameToUpdate;
    while (frameToUpdateTmp) {
        RenderUtils::UpdateDataMidiFX(tim_notes_cpy[[mplayer getCurrentGenBufferIdx]],clearAudioFXbuffer,mPaused);
        RenderUtils::UpdateDataPiano(tim_notes_cpy[[mplayer getCurrentGenBufferIdx]],clearAudioFXbuffer,mPaused);
        frameToUpdateTmp--;
    }
    clearAudioFXbuffer=false;
    
    calcFps();
    
    if (mBackground || !mHasFocus) {
        no_reentrant=0;
        return;
    }
    
    if (self.mainView.hidden||m_oglView.hidden||(isVisible==false)) {
        no_reentrant=0;
        return;
    }
    
    //    if (!mFont || !mFontMenu ) {
    //        no_reentrant=0;
    //        return;
    //    }
    fxalpha=settings[GLOB_FXAlpha].detail.mdz_slider.slider_value;
    if (settings[GLOB_FX3DSpectrumBloom].detail.mdz_switch.switch_value) {
        //if bloom is on, fx alpha should be minimum 0.9f
        //fxalpha=fmax(fxalpha,0.9f);
    }
    //m_oglView.alpha=fxalpha;
    
    
    if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) {
        //cover_viewBG.layer.zPosition=MAXFLOAT-10;
        //cover_view.layer.zPosition=MAXFLOAT-9;
        cover_viewAll.layer.zPosition=MAXFLOAT-8;
        m_oglView.layer.zPosition=MAXFLOAT-7;
        if (radioView) radioView.layer.zPosition=m_oglView.layer.zPosition+0.1;
    } else {
        //cover_viewBG.layer.zPosition=0;
        //cover_view.layer.zPosition=1;
        cover_viewAll.layer.zPosition=0;
        m_oglView.layer.zPosition=3;
        if (radioView) radioView.layer.zPosition=m_oglView.layer.zPosition+0.1;
    }
    
    mdzRenderInProgress=true;
    pthread_mutex_lock(&gl_mutex);
    [self setContextOGL];
    glClearColor(0.0f, 0.0f , 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    /*------------------------------------------------*/
    // Feed buffer for ProjectM
    if ([mplayer isPlaying]){
        short int **snd_buffer;
        int cur_pos,prev_pos;
        snd_buffer=[mplayer buffer_ana_cpy];
        cur_pos=[mplayer getCurrentPlayedBufferIdx];
        short int *curBuffer=snd_buffer[cur_pos];
        
        int sample_count=(settings[GLOB_FXFPS].detail.mdz_switch.switch_value?735:735*2);
        
        pmBufferPosWrite=0;
        
        if ([mplayer isPaused]) {
            for (int i=0;i<sample_count;i++) {
                pmBuffer[pmBufferPosWrite++]=0;
                pmBuffer[pmBufferPosWrite++]=0;
                if (pmBufferPosWrite>=PM_BUFFER_SIZE*2) pmBufferPosWrite=0;
            }
        } else {
            int posBuff=0;
            for (int i=0;i<sample_count;i++) {
                pmBuffer[pmBufferPosWrite++]=curBuffer[posBuff*2];
                pmBuffer[pmBufferPosWrite++]=curBuffer[posBuff*2+1];
                if (pmBufferPosWrite>=PM_BUFFER_SIZE*2) pmBufferPosWrite=0;
                posBuff++;
                if (posBuff>=SOUND_BUFFER_SIZE_SAMPLE) {
                    posBuff=0;
                    cur_pos++;
                    if (cur_pos>=SOUND_BUFFER_NB) cur_pos=0;
                    curBuffer=snd_buffer[cur_pos];
                }
            }
        }
    }
    /*----------------------------------------------------*/
    
    //-----------------------------------
    // ImGui
    //-----------------------------------
    
    [imGui_impl_ios updateEvent];
    
    ImGuiIOSEvent imgui_event;
    imgui_event.event_type=IMGUI_IOS_Event_None;
    if (panGestureHover) {
        static float oldPosX=-1,oldPosY=-1;
        if ( (oldPosX!=posMouseX) || (oldPosY!=posMouseY) ) {
            imgui_event.event_type=IMGUI_IOS_Event_MouseMove;
            imgui_event.pos_x=posMouseX*glScaleFactor;
            imgui_event.pos_y=posMouseY*glScaleFactor;
        }
        oldPosX=posMouseX;oldPosY=posMouseY;
    }
    if (mOglView1Tap) {
        imgui_event.event_type=IMGUI_IOS_Event_Tap_1;
        imgui_event.pos_x=oglTapX*glScaleFactor;
        imgui_event.pos_y=oglTapY*glScaleFactor;
        //projectm_touch(_pm, imgui_event.pos_x,imgui_event.pos_y, 1, PROJECTM_TOUCH_TYPE_RANDOM);
    }
    if (panGesture1Tap) {
        
        imgui_event.event_type=IMGUI_IOS_Event_MouseDrag;
        imgui_event.pos_x=(movePx+startPx)*glScaleFactor;
        imgui_event.pos_y=(movePy+startPy)*glScaleFactor;
        //projectm_touch_drag(_pm, imgui_event.pos_x,imgui_event.pos_y, 1);
    }
    if ( moveWheelYPMenu||moveWheelXPMenu ) {
        imgui_event.event_type=IMGUI_IOS_Event_MouseWheel;
        imgui_event.pos_x=posPx*glScaleFactor;
        imgui_event.pos_y=posPy*glScaleFactor;
        imgui_event.wheel_x=moveWheelXPMenu/200.0;
        imgui_event.wheel_y=moveWheelYPMenu/200.0;
        
        //MDZILog("send wheel move %f %f at pos %f %f",imgui_event.wheel_x,imgui_event.wheel_y,imgui_event.pos_x,imgui_event.pos_y);
        
        //if (panGestureWheel==0) {
        moveWheelXPMenu=moveWheelXPMenu*0.94f;
        moveWheelYPMenu=moveWheelYPMenu*0.94f;
        if (fabs(moveWheelXPMenu)<2.0f) moveWheelXPMenu=0;
        if (fabs(moveWheelYPMenu)<2.0f) moveWheelYPMenu=0;
        //}
    }
    
    ImGui_ImplIOS_NewFrame(ww*glScaleFactor,hh*glScaleFactor,1);
    ImGui_ImplIOS_UpdateEvent(&imgui_event);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    
    //ensure no VAO is bound
    glBindVertexArray(0);
    //also unbind the array buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    RenderUtils::DrawTexture(ww, hh, txtBGImage, 1.0f-fxalpha,1);
    
    if (pmenu_show) {
        //movePxPMenu+=movePx-movePxOld;
        //movePyPMenu+=movePy-movePyOld;
    }
    /*
     1:left
     2:right
     3:bottom
     4:top
     5:bottom left
     6:bottom right
     7:top left
     8:top right
     */
    bool mdz_ui_touch_zone[9];
    memset(mdz_ui_touch_zone,0,sizeof(mdz_ui_touch_zone));
    
    //if ((movePx-movePxOld)||(movePy-movePyOld)) {
        mdz_ui_touch_zone[0]=true; //always true for fullscreen
        if (startPx<ww/2) {
            //left
            mdz_ui_touch_zone[1]=true;
            if (startPy>hh/2) {
                //bottom
                mdz_ui_touch_zone[3]=true;
                //bottom left
                mdz_ui_touch_zone[5]=true;
            }
            else {
                //top
                mdz_ui_touch_zone[4]=true;
                //top left
                mdz_ui_touch_zone[7]=true;
            }
        } else {
            //right
            mdz_ui_touch_zone[2]=true;
            if (startPy>hh/2) {
                //bottom
                mdz_ui_touch_zone[3]=true;
                //bottom right
                mdz_ui_touch_zone[6]=true;
            }
            else {
                //top
                mdz_ui_touch_zone[4]=true;
                //top right
                mdz_ui_touch_zone[8]=true;
            }
        }
    //}
    if ((pmenu_show==0)&&[SettingsGenViewController isFXActive:PROJECTM_FXONOFF] && mdz_ui_touch_zone[fxSlot[FX_PROJECTM]]) {
        movePxPM+=movePx-movePxOld;
        movePyPM+=movePy-movePyOld;
    }
    
    if ((pmenu_show==0)&&[SettingsGenViewController isFXActive:GLOB_FXPiano3D] && mdz_ui_touch_zone[fxSlot[FX_PIANO3D]]) {
        movePxFXPiano+=movePx-movePxOld;
        movePyFXPiano+=movePy-movePyOld;
        movePx2FXPiano+=movePx2-movePx2Old;
        movePy2FXPiano+=movePy2-movePy2Old;
        movePinchScaleFXPiano+=movePinchScale-movePinchScaleOld;
    }
    
    if ((pmenu_show==0)&&[SettingsGenViewController isFXActive:GLOB_FX3DSpectrum] && mdz_ui_touch_zone[fxSlot[FX_3DSpectrum]]) {
        movePxFX3DSpectrum+=movePx-movePxOld;
        movePyFX3DSpectrum+=movePy-movePyOld;
        movePx2FX3DSpectrum+=movePx2-movePx2Old;
        movePy2FX3DSpectrum+=movePy2-movePy2Old;
        movePinchScaleFX3DSpectrum+=movePinchScale-movePinchScaleOld;
    }
    
    if ((pmenu_show==0)&&[SettingsGenViewController isFXActive:GLOB_FXMIDIPattern] && mdz_ui_touch_zone[fxSlot[FX_MIDIPattern]]) {
        movePxMID+=movePx-movePxOld;
        movePyMID+=movePy-movePyOld;
        movePinchScaleFXMID+=movePinchScale-movePinchScaleOld;
    }
    
    if ((pmenu_show==0)&&[SettingsGenViewController isFXActive:GLOB_FXPianoRoll] && mdz_ui_touch_zone[fxSlot[FX_PIANOROLL]]) {
        movePxPRoll+=movePx-movePxOld;
        movePyPRoll+=movePy-movePyOld;
        movePinchScaleFXPRoll+=movePinchScale-movePinchScaleOld;
    }
    
    if ((pmenu_show==0)&&[SettingsGenViewController isFXActive:GLOB_FXMODPattern] && mdz_ui_touch_zone[fxSlot[FX_MODPattern]]) {
        movePxMOD+=movePx-movePxOld;
        movePyMOD+=movePy-movePyOld;
        movePinchScaleFXMOD+=movePinchScale-movePinchScaleOld;
        
        if (movePinchScaleFXMOD<-0.4) {
            movePinchScaleFXMOD=0;
            //MDZILog("angle %f",movePinchAngle);
            if ( (fabs(movePinchAngle)>45)&&(fabs(movePinchAngle)<45+90) ) {
                if (settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value>0) settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value--;
                
            } else {
                
                settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value++;
                if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value>=settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value_nb) settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value_nb-1;
            }
        } else if (movePinchScaleFXMOD>0.4) {
            movePinchScaleFXMOD=0;
            //MDZILog("angle %f",movePinchAngle);
            if ( (fabs(movePinchAngle)>45)&&(fabs(movePinchAngle)<45+90) ) {
                settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value++;
                if (settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value>=settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value_nb) settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value=settings[GLOB_FXMODPattern_FontSize].detail.mdz_switch.switch_value_nb-1;
            } else {
                if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value>1) settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value--;
            }
        }
    }
    
    movePinchScaleOld=movePinchScale;
    movePxOld=movePx;
    movePyOld=movePy;
    movePx2Old=movePx2;
    movePy2Old=movePy2;
    
    //check for click
    if (mOglView1Tap) {
        mOglView1Tap=0;
        //MDZILog("ww %d hh %d oglTapX %f oglTapY %f",ww,hh,oglTapX,oglTapY);
        if ( (pmenu_show==0) && (oglTapX<=ww*1/4) && (oglTapY>=hh*3/4) ) {
            //tapping down left corner and not in menu, move to next ProjecTM preset
            oglv_corner_fade[0]=30;
            [self mdPrevPreset];
        } else if ( (pmenu_show==0) && (oglTapX>=ww*3/4) && (oglTapY>=hh*3/4) ) {
            //tapping down right corner and not in menu, move to next ProjecTM preset
            oglv_corner_fade[1]=30;
            [self mdNextPreset];
        } else if ( (pmenu_show==0) && (oglTapX>=ww*3/4) && (oglTapY<=hh*1/4) ) {
            //tapping upper right corner and not in menu, activate showinfo panel
            oglv_corner_fade[2]=30;
            [SettingsGenViewController changeSettingsValue:GLOB_FXSHOWINFO change:1];
        }  else if ( (pmenu_show==0) && (oglTapX<=ww*1/4) && (oglTapY<=hh*1/4) ) {
            //tapping upper left corner and not in menu, display music info
            //also display if needed preset info
            [self refreshFXFSLabels];
            oglv_corner_fade[3]=30;
            _pmPresetUpdateDisplayInfo=true;
        } else {
            //Activate menu if tap on the rest of the gl view
            if (pmenu_show==0) {
                pmenu_fade=0;
                pmenu_show=1;
            }
        }
    }
    
    //update spectrum data
    if (
        (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value)||
        (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value)||
        (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value)  ) {
            //compute new spectrum data
            if ([mplayer isPlaying]){
                //FFT: build audio buffer
                short int **snd_buffer;
                int cur_pos;
                snd_buffer=[mplayer buffer_ana_cpy];
                cur_pos=[mplayer getCurrentPlayedBufferIdx];
                short int *curBuffer=snd_buffer[cur_pos];
                // COMPUTE FFT
#define SOUND_BUFFER_SIZE_SAMPLE_SPECTRUM 512
                /////////////////////////////////////////
                //Number of Samples for input(time domain)/output(frequency domain)
                int numSamples = SOUND_BUFFER_SIZE_SAMPLE_SPECTRUM;
                //Fill Input Array with Left channel
                for (int i=0; i<numSamples; i++) {
                    fft_time[i]=(float)curBuffer[i*2]/32768.0f;
                }
                memset(fft_frequencyAvg,0,sizeof(float)*SPECTRUM_BANDS);
                memset(fft_freqAvgCount,0,sizeof(int)*SPECTRUM_BANDS);
                fftAccel->doFFTReal(fft_time, fft_frequency, numSamples);
                
                int lowfreq,highfreq,tmpfreq;
                
                double Xfactor=powl(10.l,log10l(SOUND_BUFFER_SIZE_SAMPLE_SPECTRUM/2)/(double)(SPECTRUM_BANDS-1));
                highfreq=1;
                for (int i=0;i<SPECTRUM_BANDS;i++) {
                    //lowfreq=1.l*powl(Xfactor,i+6);
                    lowfreq=highfreq;
                    //highfreq=1.l*powl(Xfactor,i+1+6);
                    highfreq+=1.0;
                    tmpfreq=1.l*powl(Xfactor,i);
                    if (highfreq<tmpfreq) highfreq=tmpfreq;
                    
                    if (highfreq>=numSamples/2) highfreq=numSamples/2-1;
                    
                    //sum=0;
                    for (int k=lowfreq;k<highfreq;k++) {
                        fft_frequencyAvg[i]=max(fft_frequencyAvg[i],fft_frequency[k]);
                        //sum+=fft_frequency[k];
                    }
                    
                    fft_frequencyAvg[i]=20.0f*log10(fft_frequencyAvg[i])+60;
                    
                    //fft_frequencyAvg[i]=20.0f*log10(sum)+60;
                    
                    if (fft_frequencyAvg[i]<0) fft_frequencyAvg[i]=0;
                    
                }
                
                for (int i=0;i<SPECTRUM_BANDS;i++) {
                    float t=64.0f*fft_frequencyAvg[i];
                    //if (t>oreal_spectrumL[i]) oreal_spectrumL[i]=t;
                    //else oreal_spectrumL[i]=oreal_spectrumL[i]*SPECTRUM_DECREASE_RATE;
                    oreal_spectrumL[i]=t;
                }
                //Fill Input Array with Right channel
                for (int i=0; i<numSamples; i++) {
                    fft_time[i]=(float)curBuffer[i*2+1]/32768.0f;
                }
                memset(fft_frequencyAvg,0,sizeof(float)*SPECTRUM_BANDS);
                memset(fft_freqAvgCount,0,sizeof(int)*SPECTRUM_BANDS);
                fftAccel->doFFTReal(fft_time, fft_frequency, numSamples);
                
                highfreq=1;
                for (int i=0;i<SPECTRUM_BANDS;i++) {
                    //lowfreq=1.l*powl(Xfactor,i+6);
                    lowfreq=highfreq;
                    //highfreq=1.l*powl(Xfactor,i+1+6);
                    highfreq+=1.0;
                    tmpfreq=1.l*powl(Xfactor,i);
                    if (highfreq<tmpfreq) highfreq=tmpfreq;
                    
                    
                    if (highfreq>=numSamples/2) highfreq=numSamples/2-1;
                    
                    //sum=0;
                    for (int k=lowfreq;k<highfreq;k++) {
                        fft_frequencyAvg[i]=max(fft_frequencyAvg[i],fft_frequency[k]);
                        //sum+=fft_frequency[k];
                    }
                    
                    fft_frequencyAvg[i]=20.0f*log10(fft_frequencyAvg[i])+60;
                    
                    //fft_frequencyAvg[i]=20.0f*log10(sum)+60;
                    
                    if (fft_frequencyAvg[i]<0) fft_frequencyAvg[i]=0;
                    
                }
                
                for (int i=0;i<SPECTRUM_BANDS;i++) {
                    float t=64.0f*(fft_frequencyAvg[i]);///fft_freqAvgCount[idx];
                    //if (t>oreal_spectrumR[i]) oreal_spectrumR[i]=t;
                    //else oreal_spectrumR[i]=oreal_spectrumR[i]*SPECTRUM_DECREASE_RATE;
                    oreal_spectrumR[i]=t;
                }
                
                
                // COMPUTE FINAL FFT & BEAT DETECTION
                int newSpecL,newSpecR,sumL,sumR;
                for (int i=0;i<SPECTRUM_BANDS;i++) {
                    newSpecL=oreal_spectrumL[i];
                    newSpecR=oreal_spectrumR[i];
                    //SUM THE LAST 8 FFT & COMPUTE AVERAGE
                    sumL=newSpecL;
                    sumR=newSpecR;
                    for (int j=0;j<7;j++) {
                        real_spectrumSumL[i][j]=real_spectrumSumL[i][j+1];
                        sumL+=real_spectrumSumL[i][j];
                        real_spectrumSumR[i][j]=real_spectrumSumR[i][j+1];
                        sumR+=real_spectrumSumR[i][j];
                    }
                    real_spectrumSumL[i][7]=newSpecL;
                    real_spectrumSumR[i][7]=newSpecR;
                    sumL>>=3;sumR>>=3;
                }
                /////////////////////////////////////////
            }
        }
    
    int detail_lvl=settings[GLOB_FXLOD].detail.mdz_switch.switch_value;
    int decrease_factor=(settings[GLOB_FXFPS].detail.mdz_switch.switch_value?3:2);
    int increase_factor=(settings[GLOB_FXFPS].detail.mdz_switch.switch_value?2:1);
    int tgtL,tgtR;
    switch (detail_lvl) {
        case 2:
            nb_spectrum_bands=SPECTRUM_BANDS;
            
            for (int i=0;i<SPECTRUM_BANDS;i++) {
                if (real_spectrumL[i]<oreal_spectrumL[i]) real_spectrumL[i]+=((oreal_spectrumL[i]-real_spectrumL[i])>>increase_factor)+1;
                else if (real_spectrumL[i]>oreal_spectrumL[i]) real_spectrumL[i]+=((oreal_spectrumL[i]-real_spectrumL[i])>>decrease_factor)-1;
                if (real_spectrumR[i]<oreal_spectrumR[i]) real_spectrumR[i]+=((oreal_spectrumR[i]-real_spectrumR[i])>>increase_factor)+1;
                else if (real_spectrumR[i]>oreal_spectrumR[i]) real_spectrumR[i]+=((oreal_spectrumR[i]-real_spectrumR[i])>>decrease_factor)-1;
            }
            break;
        case 1:
            nb_spectrum_bands=SPECTRUM_BANDS/2;
            for (int i=0;i<SPECTRUM_BANDS/2;i++) {
                tgtL=max2(oreal_spectrumL[i*2],oreal_spectrumL[i*2+1]);
                tgtR=max2(oreal_spectrumR[i*2],oreal_spectrumR[i*2+1]);
                
                if (real_spectrumL[i]<tgtL) real_spectrumL[i]+=((tgtL-real_spectrumL[i])>>increase_factor)+1;
                else if (real_spectrumL[i]>tgtL) real_spectrumL[i]+=((tgtL-real_spectrumL[i])>>decrease_factor)-1;
                if (real_spectrumR[i]<tgtR) real_spectrumR[i]+=((tgtR-real_spectrumR[i])>>increase_factor)+1;
                else if (real_spectrumR[i]>tgtR) real_spectrumR[i]+=((tgtR-real_spectrumR[i])>>decrease_factor)-1;
                
            }
            break;
            
        case 0:
            nb_spectrum_bands=SPECTRUM_BANDS/4;
            for (int i=0;i<SPECTRUM_BANDS/4;i++) {
                tgtL=max4(oreal_spectrumL[i*4],oreal_spectrumL[i*4+1],oreal_spectrumL[i*4+2],oreal_spectrumL[i*4+3]);
                tgtR=max4(oreal_spectrumR[i*4],oreal_spectrumR[i*4+1],oreal_spectrumR[i*4+2],oreal_spectrumR[i*4+3]);
                
                if (real_spectrumL[i]<tgtL) real_spectrumL[i]+=((tgtL-real_spectrumL[i])>>increase_factor)+1;
                else if (real_spectrumL[i]>tgtL) real_spectrumL[i]+=((tgtL-real_spectrumL[i])>>decrease_factor)-1;
                if (real_spectrumR[i]<tgtR) real_spectrumR[i]+=((tgtR-real_spectrumR[i])>>increase_factor)+1;
                else if (real_spectrumR[i]>tgtR) real_spectrumR[i]+=((tgtR-real_spectrumR[i])>>decrease_factor)-1;
                
            }
            break;
    }
    angle+=(float)4.0f;
    
    
    if ([mplayer isPlaying]) {
                float x;
                float y;
                float w;
                float h;
        
        //  7 4 8
        //  1 0 2
        //  5 3 6
        
        //Compute distance for each slot to determine the closest one
        fxTargetSlot=-1;
        ImVec2 slotsLocations[9];
        ImVec4 slotsPos[9];
        if (oglLPTap) {
            float distance=ww*ww+hh*hh;
            //full
            slotsLocations[0].x=ww/2;slotsLocations[0].y=hh/2;
            slotsPos[0].x=0;slotsPos[0].y=0;
            slotsPos[0].z=ww;slotsPos[0].w=hh;
            //left
            slotsLocations[1].x=ww/4;slotsLocations[1].y=hh/2;
            slotsPos[1].x=0;slotsPos[1].y=0;
            slotsPos[1].z=ww/2;slotsPos[1].w=hh;
            //right
            slotsLocations[2].x=ww*3/4;slotsLocations[2].y=hh/2;
            slotsPos[2].x=ww/2;slotsPos[2].y=0;
            slotsPos[2].z=ww/2;slotsPos[2].w=hh;
            //bottom
            slotsLocations[3].x=ww/2;slotsLocations[3].y=hh*3/4;
            slotsPos[3].x=0;slotsPos[3].y=0;
            slotsPos[3].z=ww;slotsPos[3].w=hh/2;
            //top
            slotsLocations[4].x=ww/2;slotsLocations[4].y=hh*1/4;
            slotsPos[4].x=0;slotsPos[4].y=hh/2;
            slotsPos[4].z=ww;slotsPos[4].w=hh/2;
            //bottom left
            slotsLocations[5].x=ww/4;slotsLocations[5].y=hh*3/4;
            slotsPos[5].x=0;slotsPos[5].y=0;
            slotsPos[5].z=ww/2;slotsPos[5].w=hh/2;
            //bottom right
            slotsLocations[6].x=ww*3/4;slotsLocations[6].y=hh*3/4;
            slotsPos[6].x=ww/2;slotsPos[6].y=0;
            slotsPos[6].z=ww/2;slotsPos[6].w=hh/2;
            //top left
            slotsLocations[7].x=ww/4;slotsLocations[7].y=hh*1/4;
            slotsPos[7].x=0;slotsPos[7].y=hh/2;
            slotsPos[7].z=ww/2;slotsPos[7].w=hh/2;
            //top right
            slotsLocations[8].x=ww*3/4;slotsLocations[8].y=hh*1/4;
            slotsPos[8].x=ww/2;slotsPos[8].y=hh/2;
            slotsPos[8].z=ww/2;slotsPos[8].w=hh/2;
            for (int i=0;i<9;i++) {
                float tmp=(slotsLocations[i].x-oglLPTapX)*(slotsLocations[i].x-oglLPTapX)+(slotsLocations[i].y-oglLPTapY)*(slotsLocations[i].y-oglLPTapY);
                if (tmp<distance) {
                    distance=tmp;
                    fxTargetSlot=i;
                }
            }
        }
        
        bool isModPatternExclusive=false;
        if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value && mplayer.mPatternDataAvail &&
            settings[GLOB_FXMODPatternHideOther].detail.mdz_boolswitch.switch_value) isModPatternExclusive=true;
        
        for (int pass=0;pass<2;pass++) {
            /*-------------------------------------------------------------------------------*/
            /*  ProjectM render */
            /*-------------------------------------------------------------------------------*/
            if ( ((pass==0) && (fxLPselected!=FX_PROJECTM)) ||
                ((pass==1) && (fxLPselected==FX_PROJECTM)) )
            if (settings[PROJECTM_FXONOFF].detail.mdz_switch.switch_value) {
                drawTgtSlotPattern(FX_PROJECTM,slotsPos[fxTargetSlot].x, slotsPos[fxTargetSlot].y,
                                   slotsPos[fxTargetSlot].z, slotsPos[fxTargetSlot].w, ww, hh);
                
                bool isSlot=false;
                initViewPortData(FX_PROJECTM,x,y,w,h,ww,hh);
                glViewport(x*mScaleFactor, (h!=hh?h-y:y)*mScaleFactor, w*mScaleFactor, h*mScaleFactor);
                if ((w<ww)||(h<hh)) isSlot=true;
                [self doFramePM:ImVec2(w,h) isSlot:isSlot];
            }
            
            //-------------------------------------
            // Cover
            //-------------------------------------
            if ( ((pass==0) && (fxLPselected!=FX_COVER)) ||
                ((pass==1) && (fxLPselected==FX_COVER)) )
            if (settings[GLOB_FXCover].detail.mdz_boolswitch.switch_value) {
                drawTgtSlotPattern(FX_COVER,slotsPos[fxTargetSlot].x, slotsPos[fxTargetSlot].y,
                                   slotsPos[fxTargetSlot].z, slotsPos[fxTargetSlot].w, ww, hh);
                
                initViewPortData(FX_COVER,x,y,w,h,ww,hh);
                glViewport(x*mScaleFactor, (h!=hh?h-y:y)*mScaleFactor, w*mScaleFactor, h*mScaleFactor);
                [self doFxCover:ImVec4(x,y,w,h)];
            }
            
            
            /*-------------------------------------------------------------------------------*/
            
            
            //-------------------------------------
            // Landscape 3D
            //-------------------------------------
            if ( ((pass==0) && (fxLPselected!=FX_3DLandscape)) ||
                ((pass==1) && (fxLPselected==FX_3DLandscape)) )
            if (settings[GLOB_FX3DLandscape].detail.mdz_switch.switch_value) {
                drawTgtSlotPattern(FX_3DLandscape,slotsPos[fxTargetSlot].x, slotsPos[fxTargetSlot].y,
                                   slotsPos[fxTargetSlot].z, slotsPos[fxTargetSlot].w, ww, hh);
                
                initViewPortData(FX_3DLandscape,x,y,w,h,ww,hh);
                glViewport(x*mScaleFactor, (h!=hh?h-y:y)*mScaleFactor, w*mScaleFactor, h*mScaleFactor);
                [self doFx3DLandscape:ImVec4(x,y,w,h)];
            }
            
            //-------------------------------------
            // Spectrum 3D
            //-------------------------------------
            if ( ((pass==0) && (fxLPselected!=FX_3DSpectrum)) ||
                ((pass==1) && (fxLPselected==FX_3DSpectrum)) )
            if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value) {
                drawTgtSlotPattern(FX_3DSpectrum,slotsPos[fxTargetSlot].x, slotsPos[fxTargetSlot].y,
                                   slotsPos[fxTargetSlot].z, slotsPos[fxTargetSlot].w, ww, hh);
                
                initViewPortData(FX_3DSpectrum,x,y,w,h,ww,hh);
                glViewport(x*mScaleFactor, (h!=hh?h-y:y)*mScaleFactor, w*mScaleFactor, h*mScaleFactor);
                [self doFx3DSpectrum:ImVec4(x,y,w,h)];
            }
            
            //-------------------------------------
            // Piano 3D
            //-------------------------------------
            if ( ((pass==0) && (fxLPselected!=FX_PIANO3D)) ||
                ((pass==1) && (fxLPselected==FX_PIANO3D)) )
            if (settings[GLOB_FXPiano3D].detail.mdz_switch.switch_value) {
                if (!isModPatternExclusive || !isOverlappingSlots(fxSlot[FX_PIANO3D],fxSlot[FX_MODPattern])) {
                    drawTgtSlotPattern(FX_PIANO3D,slotsPos[fxTargetSlot].x, slotsPos[fxTargetSlot].y,
                                       slotsPos[fxTargetSlot].z, slotsPos[fxTargetSlot].w, ww, hh);
                    
                    initViewPortData(FX_PIANO3D,x,y,w,h,ww,hh);
                    glViewport(x*mScaleFactor, (h!=hh?h-y:y)*mScaleFactor, w*mScaleFactor, h*mScaleFactor);
                    [self doFxPiano3D:ImVec4(x,y,w,h)];
                }
            }
            //-------------------------------------
            // Spectrum 2D
            //-------------------------------------
            if ( ((pass==0) && (fxLPselected!=FX_2DSpectrum)) ||
                ((pass==1) && (fxLPselected==FX_2DSpectrum)) )
            if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value) {
                drawTgtSlotPattern(FX_2DSpectrum,slotsPos[fxTargetSlot].x, slotsPos[fxTargetSlot].y,
                                   slotsPos[fxTargetSlot].z, slotsPos[fxTargetSlot].w, ww, hh);
                
                initViewPortData(FX_2DSpectrum,x,y,w,h,ww,hh);
                glViewport(x*mScaleFactor, (h!=hh?h-y:y)*mScaleFactor, w*mScaleFactor, h*mScaleFactor);
                [self doFx2DSpectrum:ImVec4(x,y,w,h)];
            }
            
            //-------------------------------------
            // Piano Roll
            //-------------------------------------
            if ( ((pass==0) && (fxLPselected!=FX_PIANOROLL)) ||
                ((pass==1) && (fxLPselected==FX_PIANOROLL)) )
            if (settings[GLOB_FXPianoRoll].detail.mdz_switch.switch_value) {
                
                if (!isModPatternExclusive || !isOverlappingSlots(fxSlot[FX_PIANOROLL],fxSlot[FX_MODPattern])) {
                    drawTgtSlotPattern(FX_PIANOROLL,slotsPos[fxTargetSlot].x, slotsPos[fxTargetSlot].y,
                                       slotsPos[fxTargetSlot].z, slotsPos[fxTargetSlot].w, ww, hh);
                    
                    initViewPortData(FX_PIANOROLL,x,y,w,h,ww,hh);
                    glViewport(x*mScaleFactor, (h!=hh?h-y:y)*mScaleFactor, w*mScaleFactor, h*mScaleFactor);
                    [self doFxPianoRoll:ImVec4(x,y,w,h)];
                }
            }
            
            //-------------------------------------
            // Midi patterns
            //-------------------------------------
            if ( ((pass==0) && (fxLPselected!=FX_MIDIPattern)) ||
                ((pass==1) && (fxLPselected==FX_MIDIPattern)) )
            if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value) {
                
                if (!isModPatternExclusive || !isOverlappingSlots(fxSlot[FX_MIDIPattern],fxSlot[FX_MODPattern])) {
                    drawTgtSlotPattern(FX_MIDIPattern,slotsPos[fxTargetSlot].x, slotsPos[fxTargetSlot].y,
                                       slotsPos[fxTargetSlot].z, slotsPos[fxTargetSlot].w, ww, hh);
                    
                    initViewPortData(FX_MIDIPattern,x,y,w,h,ww,hh);
                    glViewport(x*mScaleFactor, (h!=hh?h-y:y)*mScaleFactor, w*mScaleFactor, h*mScaleFactor);
                    [self doFxMidiPattern:ImVec4(x,y,w,h)];
                }
            }
            
            //-------------------------------------
            // Mod patterns
            //-------------------------------------
            if ( ((pass==0) && (fxLPselected!=FX_MODPattern)) ||
                ((pass==1) && (fxLPselected==FX_MODPattern)) )
            if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value && mplayer.mPatternDataAvail) {
                drawTgtSlotPattern(FX_MODPattern,slotsPos[fxTargetSlot].x, slotsPos[fxTargetSlot].y,
                                   slotsPos[fxTargetSlot].z, slotsPos[fxTargetSlot].w, ww, hh);
                
                initViewPortData(FX_MODPattern,x,y,w,h,ww,hh);
                glViewport(x*mScaleFactor, (h!=hh?h-y:y)*mScaleFactor, w*mScaleFactor, h*mScaleFactor);
                [self doFxModPatterns:ImVec4(x,y,w,h)];
            }
            
            //-------------------------------------
            // Oscillo
            //-------------------------------------
            if ( ((pass==0) && (fxLPselected!=FX_OSCILLO)) ||
                ((pass==1) && (fxLPselected==FX_OSCILLO)) )
            if (settings[OSCILLO_FXMODE].detail.mdz_switch.switch_value) {
                drawTgtSlotPattern(FX_OSCILLO,slotsPos[fxTargetSlot].x, slotsPos[fxTargetSlot].y,
                                   slotsPos[fxTargetSlot].z, slotsPos[fxTargetSlot].w, ww, hh);
                
                initViewPortData(FX_OSCILLO,x,y,w,h,ww,hh);
                glViewport(x*mScaleFactor, (h!=hh?h-y:y)*mScaleFactor, w*mScaleFactor, h*mScaleFactor);
                [self doFxOscillo:ImVec4(x,y,w,h)];
            }
        }
        glViewport(0, 0, ww*mScaleFactor, hh*mScaleFactor);
    }
    
    //-------------------------------------
    // Song info
    //-------------------------------------
    if (settings[GLOB_FX_DISPLAYSONGINFO].detail.mdz_boolswitch.switch_value && _mdz_display_songinfo_countdown) [self showSongInfo:ImVec2(ww,hh) frameToUpdate:frameToUpdate];
    
    //-------------------------------------
    // ProjectM preset name display
    //-------------------------------------
    if ((settings[PROJECTM_FXONOFF].detail.mdz_boolswitch.switch_value) && ((settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value)||([_mdzPM_playlist getSize]==0))) {
        if (_pmIsInitialized && _pm) {
            //float x,y,w,h;
            
            //            MDZILog("safe %f %f",safe_top,safe_bottom);
            
            if (_pmPresetNewLoaded) {
                _pmPresetUpdateDisplayInfo=true;
                _pmPresetNewLoaded=false;
            }
            
            if (_pmPresetUpdateDisplayInfo) {
                _pmPresetUpdateDisplayInfo=false;
                _pm_display_scrollx=0;
                _pm_display_scroll_direction=1;
                _pm_display_name_countdown=_pm_fps*PM_PRESET_DISPLAY_TIMEOUT;
            }
            
            //if not limited, reset countdown
            if ((settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==2) || ([_mdzPM_playlist getSize]==0)) {
                _pm_display_name_countdown=_pm_fps*PM_PRESET_DISPLAY_TIMEOUT;
            }
            NSString *pmInfoStr;
            if ( [_mdzPM_playlist size] && [_mdzPM_Favorites isFavoritePreset:[NSString stringWithUTF8String:[_mdzPM_playlist getCurPresetCleanTitle]]] ) {
                pmInfoStr=[NSString stringWithFormat:@"%C%C %s   ",
                           static_cast<unichar>(FA_STAR),
                           static_cast<unichar>((settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value?FA_LOCK:FA_UNLOCK)),
                           [_mdzPM_playlist getCurLabel]];
            } else {
                pmInfoStr=[NSString stringWithFormat:@"%C %s   ",
                           static_cast<unichar>((settings[PROJECTM_LockPreset].detail.mdz_boolswitch.switch_value?FA_LOCK:FA_UNLOCK)),
                           [_mdzPM_playlist getCurLabel]];
            }
            const char *pmPresetStr=[pmInfoStr UTF8String];
            if (pmPresetStr&&_pm_display_name_countdown) {
                float alpha_val=(float)(_pm_display_name_countdown*4)/255.0;
                if (alpha_val>0.8) alpha_val=0.8;
                
                if (font_menu) ImGui::PushFont(font_menu,FONTSIZE_PM_PRESET_INFO_LINE*glScaleFactor);
                else ImGui::PushFont(nullptr);
                
                float textHH=ImGui::GetTextLineHeightWithSpacing()/glScaleFactor+6.0;
                
                float pos_y=hh-textHH;
                if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) pos_y-=safe_bottom;
                pos_y*=glScaleFactor;
                ImGui::SetNextWindowPos(ImVec2(0,pos_y));
                ImGui::SetNextWindowSize(ImVec2(ww*glScaleFactor,textHH*glScaleFactor));
                ImGui::GetStyle().Alpha=alpha_val;
                ImGui::Begin("On screen info",0,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoFocusOnAppearing);
                ImVec2 pmPresetStr_size=ImGui::CalcTextSize(pmPresetStr);
                
                //if fullscreen or landscape orientation, add padding to cope with safe/zone / rounded borders
                if ( (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) || (ww>hh) ) {
                    ImGui::Text("     %s     ",pmPresetStr);
                } else {
                    ImGui::Text("%s",pmPresetStr);
                }
                
                ImGui::SetScrollX(_pm_display_scrollx);
                ImGui::End();
                ImGui::PopFont();
                
                for (int j=0;j<frameToUpdate;j++) {
                    if ((settings[PROJECTM_ShowPresetLabel].detail.mdz_switch.switch_value==1)&&_pm_display_name_countdown) _pm_display_name_countdown--;
                    
                    if (_pm_display_scroll_pause) _pm_display_scroll_pause--;
                    else {
                        if (_pm_display_scroll_direction==1) {
                            if (m_oglView.frame.size.width*glScaleFactor+_pm_display_scrollx<pmPresetStr_size.x) _pm_display_scrollx+=3;
                            else {
                                if (_pm_display_scrollx>0) {
                                    _pm_display_scroll_direction=-1;
                                    _pm_display_scroll_pause=_pm_fps*1.5;
                                }
                            }
                        } else {
                            if (_pm_display_scrollx>0) _pm_display_scrollx-=3;
                            else {
                                _pm_display_scrollx=0;
                                _pm_display_scroll_direction=1;
                                _pm_display_scroll_pause=_pm_fps*1.5;
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (pmenu_show) {
        if (pmenu_fade<255) {
            pmenu_fade+=32*frameToUpdate;//48;
            /*            pmenu_fade+=(255-pmenu_fade)/3;*/
            if (pmenu_fade>255) pmenu_fade=255;
        }
    } else {
        if (pmenu_fade>0) {
            pmenu_fade-=32*frameToUpdate;//48;
            /*            pmenu_fade-=(255+32-pmenu_fade)/3;*/
            if (pmenu_fade<0) pmenu_fade=0;
        }
    }
    
    if (pmenu_fade) {
        float fadelev=sin(pmenu_fade*3.14159/2/256);
        if (fadelev<0) fadelev=0;
        if (fadelev>1.0f) fadelev=1.0f;
        
        //specific case for fullscreen switch change
        bool isFullscreen=settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value;
        
        int ret=PMenu::playerShowMenu(ww,hh,safe_top,safe_bottom,safe_left,safe_right,glScaleFactor,fadelev,movePxPMenu,movePyPMenu,pmenu_show);
        
        //update fxslot settings
        updateSettingsSelectedSlot();
        
        
        movePxPMenu=movePyPMenu=0;
        if (ret<0) {
            mOglViewIsHidden=YES;
            pmenu_show=0;
            //pmenu_fade=0;
            if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) {
                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=0;
                oglViewFullscreenChanged=1;
                [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
            }
        } else if (ret==0) {
            pmenu_show=0;
            pmenu_fade=0;
        } else if (ret>0) {
            if (ret>=2) shouldGoToSettings=ret; //Visu or submenu
        }
        
        //specific case for fullscreen switch change
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value!=isFullscreen) {
            shouldUpdateCoverTexture=1;
            oglViewFullscreenChanged=1;
            [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
        }
    }
    
    [self showGUICorners:ImVec2(ww,hh) frameToUpdate:frameToUpdate];
    [self showInfoData:ImVec2(ww,hh) frameToUpdate:frameToUpdate];
    
    //    if (_mdzPM_playlist.lastFailed) {
    //        [self newGuiMessage:[NSString stringWithFormat:@"%C",static_cast<unichar>(FA_EXCLAMATION_TRIANGLE)]];
    //        //_mdzPM_playlist.lastFailed=false;
    //    }
    [self showGuiMessage:ImVec2(ww,hh) frameToUpdate:frameToUpdate];
    
    //-----------------------------------
    // ImGui
    //-----------------------------------
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    [self presentContextOGL];
    pthread_mutex_unlock(&gl_mutex);
    mdzRenderInProgress=false;
    
    CFTimeInterval _fx_last_time=CFAbsoluteTimeGetCurrent();
    _fx_frame_time=1000.0f*(double)(_fx_last_time-_fx_start_time);
    
    if (_fx_frame_time<PM_FRAMETIME_LIMIT) {
        if (_fx_frame_timeOverLimitCounter) _fx_frame_timeOverLimitCounter--;
    } else {
        _fx_frame_timeOverLimitCounter++;
        int limitSlowFrame=0;
        switch (settings[GLOB_FX_LIMIT_SLOWFX].detail.mdz_switch.switch_value) {
            case 0:limitSlowFrame=0;break;
            case 1:limitSlowFrame=PM_FRAMETIME_LIMIT_WEAK;break;
            default:
            case 2:limitSlowFrame=PM_FRAMETIME_LIMIT_STRONG;break;
        }
        if (limitSlowFrame) {
            if (_fx_frame_timeOverLimitCounter>limitSlowFrame) [self frameTooSlow];
        }
    }
    
    no_reentrant=0;
    
    if (shouldGoToSettings) {
        //if fullscreen, go back to window so that navbar appears for settings screen
        if (settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value) {
            oglViewFullscreenChanged=1;
            deactivateFStemp=1;
            [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
            
        }
        
        SettingsGenViewController *settingsVC=[[SettingsGenViewController alloc] initWithNibName:@"SettingsViewController" bundle:[NSBundle mainBundle]];
        settingsVC->detailViewController=self;
        switch (shouldGoToSettings) {
            case 2: //Visualization
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_GLOBAL_VISU;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            case 3+FX_OSCILLO:
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_OSCILLO;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            case 3+FX_PROJECTM:
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_PROJECTM;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            case 3+FX_2DSpectrum:
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_FXPSpectrum;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            case 3+FX_3DSpectrum:
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_FXP3DSpectrum;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            case 3+FX_3DLandscape:
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_FXP3DLandscape;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            case 3+FX_PIANO3D:
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_FXPiano3D;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            case 3+FX_PIANOROLL:
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_FXPianoRoll;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            case 3+FX_MIDIPattern:
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_FXMIDIPattern;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            case 3+FX_MODPattern:
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_FXMODPattern;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            case 3+FX_COVER:
                settingsVC->current_family=MDZ_SETTINGS_FAMILY_FXPCover;
                settingsVC.title=NSLocalizedString(([NSString stringWithUTF8String:settings[settingsVC->current_family].label]),@"");
                break;
            default: //Root
                settingsVC.title=NSLocalizedString(@"General Settings",@"");
                //settingsVC->current_family=MDZ_SETTINGS_FAMILY_ROOT;
                break;
        }
        
        //        settingsVC.view.frame=self.view.frame;
        SettingsGenViewController *childController=settingsVC;
        if ([childController respondsToSelector:@selector(setEdgesForExtendedLayout:)]) {
            childController.edgesForExtendedLayout = UIRectEdgeNone;
            childController.extendedLayoutIncludesOpaqueBars = NO;
        }
        if ([childController isKindOfClass:[UITableViewController class]]) {
            ((UITableViewController *)childController).tableView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentAutomatic;
        } else if ([childController.view isKindOfClass:[UIScrollView class]]) {
            ((UIScrollView *)childController.view).contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentAutomatic;
        }
        
        [self.navigationController pushViewController:settingsVC animated:YES];
    }
}


- (void)viewDidDisappear:(BOOL)animated {
    mHasFocus=0;
    
    //Displaylink: update FPS
    if (m_displayLink) {
        m_displayLink.preferredFramesPerSecond = 5; //reduce to 5fps when not visible
    }
    
    [super viewDidDisappear:animated];
}


- (void)viewDidUnload {
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}
#pragma mark -
#pragma mark Memory management


- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

/* POPUP functions */
-(void) hidePopup {
    infoMsgView.hidden=YES;
    mPopupAnimation=0;
}

-(void) openPopup:(NSString *)msg secmsg:(NSString*)secmsg style:(int)style{
    CGRect frame;
    
    UIColor *bgcol;
    switch (style){
        case POPUP_STYLE_INFO://info
            bgcol=[UIColor colorWithRed:(float)(0x00)/255.0f green:(float)(0x02)/255.0f blue:(float)(0x41)/255.0f alpha:1.0];
            break;
        case POPUP_STYLE_ALERT://alert
            bgcol=[UIColor colorWithRed:(float)(0xB0)/255.0f green:(float)(0x02)/255.0f blue:(float)(0x00)/255.0f alpha:1.0];
            break;
    }
    infoMsgView.backgroundColor=bgcol;
    
    infoMsgLbl.text=[NSString stringWithString:msg];
    infoSecMsgLbl.text=[NSString stringWithString:secmsg];
    
    if (mPopupAnimation) return;
    
    mPopupAnimation=1;
    frame=infoMsgView.frame;
    frame.origin.y=self.view.frame.size.height;
    infoMsgView.frame=frame;
    infoMsgView.hidden=NO;
    
    [UIView animateWithDuration:0.4 delay:0.0 options:0
                     animations:^{
        CGRect frame;
        frame=self.infoMsgView.frame;
        frame.origin.y=self.view.frame.size.height-144;
        self.infoMsgView.frame=frame;
        } completion:^(BOOL finished) {
            [self closePopup];
        }];
}
-(void) closePopup {
    [UIView animateWithDuration:0.4 delay:2.4 options:0
                     animations:^{
        CGRect frame;
        frame=self.infoMsgView.frame;
        frame.origin.y=self.view.frame.size.height;
        self.infoMsgView.frame=frame;
        } completion:^(BOOL finished) {
            [self hidePopup];
        }];
}


#pragma mark - Table view data source

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    UILabel *myLabel = [[UILabel alloc] init];
    NSString *lbl;
    switch (current_selmode) {
        case ARCSUB_MODE_ARC:
            lbl=NSLocalizedString(@"Choose a song",@"");
            break;
        case ARCSUB_MODE_SUB:
            lbl=NSLocalizedString(@"Choose a subsong",@"");
            break;
        case ARCSUB_MODE_RADIO:
            lbl=NSLocalizedString(@"Radio history",@"");
            break;
    }
    
    
    [myLabel setText:lbl];
    [myLabel setTextAlignment:NSTextAlignmentCenter];
    
    myLabel.backgroundColor = [UIColor blackColor];
    myLabel.textColor = [UIColor whiteColor];
    myLabel.font = [UIFont fontWithName:@"Gotham-Bold" size:17.0f];
    return myLabel;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    switch (current_selmode) {
        case ARCSUB_MODE_ARC:
            return [mplayer getArcEntriesCnt];
        case ARCSUB_MODE_SUB:
            return mplayer.mod_subsongs;
        case ARCSUB_MODE_RADIO:
            return [radioSource getHistorySize]+1;
    }
    return 0;
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    return nil;
}

- (NSInteger)tableView:(UITableView *)tabView sectionForSectionIndexTitle:(NSString *)title {
    return NSNotFound;
}

- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
    const NSInteger TOP_LABEL_TAG = 1001;
    UILabel *topLabel;
    
    
    if (forceReloadCells) {
        while ([tabView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        forceReloadCells=false;
    }
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,SELECTOR_TABVIEWCELL_HEIGHT);
        
        [cell setBackgroundColor:[UIColor clearColor]];
        
        UIBackgroundConfiguration *backgroundConfig = [UIBackgroundConfiguration listGroupedCellConfiguration];
        backgroundConfig.backgroundColor = [UIColor systemGroupedBackgroundColor];
        cell.backgroundConfiguration = backgroundConfig;
        
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
        topLabel.font = [UIFont systemFontOfSize:15 weight:MDZ_UIFONT_WEIGHT];
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;
        topLabel.opaque=TRUE;
        topLabel.numberOfLines=0;
        
        cell.accessoryView=nil;
        cell.accessoryType = UITableViewCellAccessoryNone;
    } else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;
    }
    
    float margin=MDZ_TABVIEW_SEPARATOR_MARGIN;
    cell.layoutMargins = UIEdgeInsetsMake(0, margin, 0, margin);
    cell.separatorInset = UIEdgeInsetsMake(0, margin, 0, margin);
    
    if (darkMode) {
        topLabel.textColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:1.0 green:1.0 blue:1.0 alpha:1.0];
        
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
    }
    topLabel.frame= CGRectMake(4,
                               0,
                               tabView.bounds.size.width-8,
                               SELECTOR_TABVIEWCELL_HEIGHT);
    
    switch (current_selmode) {
        case ARCSUB_MODE_ARC:
            topLabel.text=[NSString stringWithFormat:@"%@",[mplayer getArcEntryTitle:(int)(indexPath.row)]];
            break;
        case ARCSUB_MODE_SUB:
            topLabel.text=[NSString stringWithFormat:@"%@",[mplayer getSubTitle:(int)(indexPath.row+mplayer.mod_minsub)]];
            break;
        case ARCSUB_MODE_RADIO:
            if (indexPath.row==0) {
                topLabel.text=[NSString stringWithFormat:@"%@",[radioSource getQueueLabel:0]];
            } else {
                topLabel.text=[NSString stringWithFormat:@"%@",[radioSource getHistoryLabel:(int)(indexPath.row-1)]];
            }
            break;
        default:
            topLabel.text=@"N/A";
            break;
    }
    
    return cell;
}

// Override to support editing the table view.
- (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    
}
/*- (NSIndexPath *)tableView:(UITableView *)tableView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath {
 return proposedDestinationIndexPath;
 }*/
// Override to support rearranging the table view.
/*- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
 
 }*/
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return NO;
}
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return NO;
}



#pragma mark Table view delegate

- (void)tableView:(UITableView *)tabView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    // Navigation logic may go here. Create and push another view controller.
    switch (current_selmode) {
        case ARCSUB_MODE_ARC:
            [self didSelectRowInAlertArcController:indexPath.row];
            break;
        case ARCSUB_MODE_SUB:
            [self didSelectRowInAlertSubController:indexPath.row];
            break;
        case ARCSUB_MODE_RADIO:
            [self didSelectRowInAlertRadioController:indexPath.row];
            break;
        default:
            break;
    }
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
}

-(void) cleanPlaylistAfterDelDir:(NSString*)fullPath {
    //////////////////////////////////////////////////////////////////////////////////////:
    //nowplaying playlist, remove a dir (multiple entries)
    //////////////////////////////////////////////////////////////////////////////////////:
    int strLen=(int)[fullPath length];
    int j=0;
    bool isplaying=[self mIsPlaying];
    bool removeCurrentPos=false;
    while (j<mPlaylist_size) {
        NSString *strTmp=[ModizFileHelper getFullPathForFilePath:mPlaylist[j].mPlaylistFilepath];
        if ([strTmp length]>=strLen) {
            strTmp=[strTmp substringToIndex:strLen];
            //MDZILog("cleanPlaylistAfterDelDir: %@ | %@\n",strTmp,fullPath);
            if ([strTmp isEqualToString:fullPath]) {
                if (j==mPlaylist_pos) [self stop];
                
                mPlaylist[j].mPlaylistFilename=nil;
                mPlaylist[j].mPlaylistFilepath=nil;
                for (int i=j;i<mPlaylist_size-1;i++) {
                    mPlaylist[i].mPlaylistFilename=mPlaylist[i+1].mPlaylistFilename;
                    mPlaylist[i].mPlaylistFilepath=mPlaylist[i+1].mPlaylistFilepath;
                    mPlaylist[i].mPlaylistRating=mPlaylist[i+1].mPlaylistRating;
                    mPlaylist[i].mPlaylistCount=mPlaylist[i+1].mPlaylistCount;
                    mPlaylist[i].cover_flag=mPlaylist[i+1].cover_flag;
                }
                mPlaylist_size--;
                if (mPlaylist_pos>=mPlaylist_size) mPlaylist_pos--;
                if ((j)<=mPlaylist_pos) mPlaylist_pos--;
                if (mPlaylist_pos<0) mPlaylist_pos=0;
                mShouldUpdateInfos=1;
            } else j++;
        } else j++;
    }
    if (removeCurrentPos && mPlaylist_size && isplaying) {
        [self play_curEntry:-1];
    }
    if (mPlaylist_size==0) [self stop];
}
-(void) cleanPlaylistAfterDelFile:(NSString*)fullPath {
    //////////////////////////////////////////////////////////////////////////////////////:
    //nowplaying playlist, remove an entry
    //////////////////////////////////////////////////////////////////////////////////////:
    
    int j=0;
    
    bool isplaying=[self mIsPlaying];
    bool removeCurrentPos=false;
    
    while (j<mPlaylist_size) {
        NSString *strTmp=[ModizFileHelper getFullPathForFilePath:mPlaylist[j].mPlaylistFilepath];
        //MDZILog("cleanPlaylistAfterDelFile: %@ | %@\n",strTmp,fullPath);
        if ([strTmp isEqualToString:fullPath]) {
            if (j==mPlaylist_pos) {
                removeCurrentPos=true;
            }
            
            mPlaylist[j].mPlaylistFilename=nil;
            mPlaylist[j].mPlaylistFilepath=nil;
            for (int i=j;i<mPlaylist_size-1;i++) {
                mPlaylist[i].mPlaylistFilename=mPlaylist[i+1].mPlaylistFilename;
                mPlaylist[i].mPlaylistFilepath=mPlaylist[i+1].mPlaylistFilepath;
                mPlaylist[i].mPlaylistRating=mPlaylist[i+1].mPlaylistRating;
                mPlaylist[i].mPlaylistCount=mPlaylist[i+1].mPlaylistCount;
                mPlaylist[i].cover_flag=mPlaylist[i+1].cover_flag;
            }
            
            mPlaylist_size--;
            if (mPlaylist_pos>=mPlaylist_size) mPlaylist_pos--;
            if ((j)<=mPlaylist_pos) mPlaylist_pos--;
            if (mPlaylist_pos<0) mPlaylist_pos=0;
            mShouldUpdateInfos=1;
            
            
        } else j++;
    }
    if (removeCurrentPos && mPlaylist_size && isplaying) {
        [self play_curEntry:-1];
    }
    if (mPlaylist_size==0) [self stop];
}


/*
 -(void)ShowBroadcasting {
 [RPBroadcastActivityViewController loadBroadcastActivityViewControllerWithHandler:^(RPBroadcastActivityViewController * broadcastActivityViewController, NSError *error) {
 
 broadcastActivityViewController.delegate = self;
 
 [self presentViewController:broadcastActivityViewController animated:YES completion:nil];
 
 }];
 }*/

- (void)StartRecording {
    UIAlertController *msgAlert;
    UIAlertAction* userAction;
    UIAlertAction* cancelAction;
    
    msgAlert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Screen recording",@"")
                                                   message:[NSString stringWithFormat:NSLocalizedString(@"Please choose",@"")]
                                            preferredStyle:UIAlertControllerStyleActionSheet];
    //Cancel action
    cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel
                                          handler:^(UIAlertAction * action) {
    }];
    [msgAlert addAction:cancelAction];
    
    //Start recording action
    userAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Record",@"") style:UIAlertActionStyleDefault
                                        handler:^(UIAlertAction * action) {
        RPScreenRecorder* recorder =  RPScreenRecorder.sharedRecorder;
        recorder.delegate = self;
        
        [recorder startRecordingWithHandler:^(NSError *error) {
            if(error) {
                isRecordingScreen=RS_NOT_RECORDING;
                MDZELog("Error= %@",error.localizedDescription);
            } else {
                bRSactive=true;
                isRecordingScreen=RS_RECORDING;
                
                //remove idle timer
                [[UIApplication sharedApplication] setIdleTimerDisabled:true];
            }
            [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateNormal];
            [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateHighlighted];
        }];
    }];
    [msgAlert addAction:userAction];
    
    //Restart recording action
    userAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Restart & record",@"") style:UIAlertActionStyleDefault
                                        handler:^(UIAlertAction * action) {
        RPScreenRecorder* recorder =  RPScreenRecorder.sharedRecorder;
        recorder.delegate = self;
        
        [self restartCurrent];
        [recorder startRecordingWithHandler:^(NSError *error) {
            if(error) {
                isRecordingScreen=RS_NOT_RECORDING;
                MDZELog("Error= %@",error.localizedDescription);
            } else {
                isRecordingScreen=RS_RECORDING;
                bRSactive=true;
                
                //remove idle timer
                [[UIApplication sharedApplication] setIdleTimerDisabled:true];
            }
            [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateNormal];
            [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateHighlighted];
        }];
    }];
    [msgAlert addAction:userAction];
    
    //Restart & stop recording action
    userAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Restart, record & stop",@"") style:UIAlertActionStyleDefault
                                        handler:^(UIAlertAction * action) {
        RPScreenRecorder* recorder =  RPScreenRecorder.sharedRecorder;
        recorder.delegate = self;
        
        [self restartCurrent];
        [recorder startRecordingWithHandler:^(NSError *error) {
            if(error) {
                isRecordingScreen=RS_NOT_RECORDING;
                MDZELog("Error= %@",error.localizedDescription);
            } else {
                isRecordingScreen=RS_RECORDING_AND_STOP;
                bRSactive=true;
                
                //remove idle timer
                [[UIApplication sharedApplication] setIdleTimerDisabled:true];
            }
            [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateNormal];
            [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateHighlighted];
        }];
    }];
    [msgAlert addAction:userAction];
    
    //Restart & FS recording action
    userAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Restart, fullscreen & record",@"") style:UIAlertActionStyleDefault
                                        handler:^(UIAlertAction * action) {
        RPScreenRecorder* recorder =  RPScreenRecorder.sharedRecorder;
        recorder.delegate = self;
        
        settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=1;
        oglViewFullscreenChanged=1;
        [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
        
        [recorder startRecordingWithHandler:^(NSError *error) {
            if(error) {
                isRecordingScreen=RS_NOT_RECORDING;
                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=0;
                oglViewFullscreenChanged=0;
                MDZELog("Error= %@",error.localizedDescription);
            } else {
                bRSactive=true;
                isRecordingScreen=RS_RECORDING_FS;
                [self restartCurrent];
                
                //remove idle timer
                [[UIApplication sharedApplication] setIdleTimerDisabled:true];
            }
            [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateNormal];
            [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateHighlighted];
        }];
    }];
    [msgAlert addAction:userAction];
    
    //Restart, FS & stop recording action
    userAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Restart, fullscreen, record & stop",@"") style:UIAlertActionStyleDefault
                                        handler:^(UIAlertAction * action) {
        RPScreenRecorder* recorder =  RPScreenRecorder.sharedRecorder;
        recorder.delegate = self;
        
        settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=1;
        oglViewFullscreenChanged=1;
        [self mdzUpdateUI:(UIInterfaceOrientation)orientationHV];
        
        [recorder startRecordingWithHandler:^(NSError *error) {
            if(error) {
                isRecordingScreen=RS_NOT_RECORDING;
                settings[GLOB_FXFullscreen].detail.mdz_boolswitch.switch_value=0;
                oglViewFullscreenChanged=0;
                MDZELog("Error= %@",error.localizedDescription);
            } else {
                bRSactive=true;
                isRecordingScreen=RS_RECORDING_AND_STOP_FS;
                [self restartCurrent];
                
                //remove idle timer
                [[UIApplication sharedApplication] setIdleTimerDisabled:true];
            }
            [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateNormal];
            [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateHighlighted];
        }];
    }];
    [msgAlert addAction:userAction];
    
    [self showAlert:msgAlert];
    
    
}

- (void)StopRecording
{
    RPScreenRecorder* recorder = RPScreenRecorder.sharedRecorder;
    
    
    [recorder stopRecordingWithHandler:^(RPPreviewViewController * previewViewController,
                                         NSError * error) {
        isRecordingScreen=RS_NOT_RECORDING;
        bRSactive=false;
        //reset idle timer to settings value
        [[UIApplication sharedApplication] setIdleTimerDisabled:settings[GLOB_NoScreenAutoLock].detail.mdz_boolswitch.switch_value];
        
        [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateNormal];
        [btnRecordScreen setTitleColor:(bRSactive?[UIColor redColor]:[UIColor grayColor]) forState:UIControlStateHighlighted];
        if(error)
        {
            MDZELog("Error= %@",error.localizedDescription);
        }
        
        if(previewViewController)
        {
            [self pausePushed];
            previewViewController.previewControllerDelegate = self;
            previewViewController.modalPresentationStyle = UIModalPresentationFullScreen;
            [self presentViewController:previewViewController animated:YES completion:nil];
        }
        
    }];
}

- (void)screenRecorder:(RPScreenRecorder *)screenRecorder
didStopRecordingWithError:(NSError *)error
 previewViewController:(RPPreviewViewController *)previewViewController {
    
    isRecordingScreen=false;
    //reset idle timer to settings value
    [[UIApplication sharedApplication] setIdleTimerDisabled:settings[GLOB_NoScreenAutoLock].detail.mdz_boolswitch.switch_value];
    
    
    if(error) {
        MDZELog("Error= %@",error.localizedDescription);
    }
    
}

#pragma mark RPPreview ViewController Delegate

/* @abstract Called when the view controller is finished. */
- (void)previewControllerDidFinish:(RPPreviewViewController *)previewController {
    [self dismissViewControllerAnimated:YES completion:nil];
}

/* @abstract Called when the view controller is finished and returns a set of activity types that the user has completed on the recording. The built in activity types are listed in UIActivity.h. */
- (void)previewController:(RPPreviewViewController *)previewController didFinishWithActivityTypes:(NSSet <NSString *> *)activityTypes {
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES;
}

@end
