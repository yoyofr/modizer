//
//  DetailViewController.h
//  modizer
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#define MAX_MENU_FX_STRING 20

#import "Queue.h"
#import "DBHelper.h"
//#import "HardwareClock.h"

#import "BButton.h"

#import "WaitingView.h"



//#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>

#import <UIKit/UIKit.h>

#import <MGLKit.h>

#import <ReplayKit/ReplayKit.h>

#import "OBSlider.h"

#import "ModizMusicPlayer.h"

//#import "AppDelegate_Phone.h"
#import "RootViewControllerPlaylist.h"

#import "SettingsGenViewController.h"

#import "VoicesViewController.h"

#import "GoogleAppHelper.h"

#import "EQViewController.h"

#import <UIKit/UIKit.h>

#import "CBAutoScrollLabel.h"

#import <UserNotifications/UserNotifications.h>

struct Resources;

@class OGLView;
@class EAGLContext;
@class CADisplayLink;
@class EQViewController;
@class VoicesViewController;

class CFont;
class CGLString;

enum {
    RS_NOT_RECORDING=0,
    RS_RECORDING,
    RS_RECORDING_FS,
    RS_RECORDING_AND_STOP,
    RS_RECORDING_AND_STOP_FS
};

@interface DetailViewControllerIphone : UIViewController <UINavigationControllerDelegate,UIGestureRecognizerDelegate, UIScrollViewDelegate,UITableViewDelegate,UITableViewDataSource,UIPopoverPresentationControllerDelegate,RPPreviewViewControllerDelegate,RPScreenRecorderDelegate,RPBroadcastControllerDelegate,RPBroadcastActivityViewControllerDelegate,
    UNUserNotificationCenterDelegate> {
    
    __weak IBOutlet MGLKView* m_oglView;
    MGLContext* m_oglContext;
    
    bool darkMode;
    bool forceReloadCells;
    
    int not_expected_version;
    
    UITableView *alertTableView;
    
    //EQ
    EQViewController *eqVC;
    bool bShowEQ;
    //Voices
    VoicesViewController *voicesVC;
    bool bShowVC;
    
    //Record screen
    bool bRSactive;
    int isRecordingScreen; //0: not recording, 1: record, 2: record & stop at song end
    
    bool statusbarHidden;
    
    WaitingView *waitingView;
    
    ModizMusicPlayer *mplayer;
    
    NSString *ratingImg[3];
    
    int curSongLength;
    
    int mOnlyCurrentEntry;
    int mOnlyCurrentSubEntry;

    int oglViewFullscreenChanged;
    int orientationHV;
    
    
    //st::HardwareClock m_clock;
    CADisplayLink *m_displayLink,*m_displayLink2;
    
    UIView *mInWasView;
    BOOL mInWasViewHidden;
    
    UIView *labelTopView;
    CBAutoScrollLabel *labelModuleName,*labelSub,*labelArtist;

    //Subsongs and Archive entries picker
    IBOutlet UIButton *btnChangeTime;
    IBOutlet BButton *btnShowArcList,*btnShowSubSong,*btnShowVoices,*btnRecordScreen;
    
    IBOutlet UILabel *labelTime,*labelModuleLength;
    IBOutlet UILabel *labelSeeking;
    IBOutlet UILabel *labelModuleSize,*labelNumChannels,*labelModuleType,*playlistPos,*labelLibName;
    IBOutlet UIButton *buttonLoopTitleSel,*buttonLoopList,*buttonLoopListSel,*buttonShuffle,*buttonShuffleSel,*buttonShuffleOneSel,*btnLoopInf;
    IBOutlet UIButton *backInfo,*infoZoom,*infoUnzoom;
    IBOutlet BButton *infoButton,*eqButton;
    IBOutlet UIButton *mainRating5,*mainRating5off;
    IBOutlet UIButton *btnAddToPl;
    IBOutlet UIToolbar *playBar,*pauseBar,*playBarSub,*pauseBarSub;
    
    IBOutlet OBSlider *sliderProgressModule;
    
    IBOutlet UITextView *textMessage;
    IBOutlet UIView *infoMsgView;
    IBOutlet UILabel *infoMsgLbl,*infoSecMsgLbl;
    IBOutlet UIView *detailView,*commandViewU,*mainView,*infoView;
    
    IBOutlet UIButton *oglButton;

    IBOutlet UIImageView *cover_view,*cover_viewBG;
    IBOutlet UIView *cover_viewAll;
    UIImageView *gifAnimation;
    
    int sliderProgressModuleEdit;
    int sliderProgressModuleChanged;
    int     module_waiting;
    NSTimer *repeatingTimer;
    
//    CFont *mFont,*mFontMenu;
//    NSString *mFontPath,*mFontMenuPath;
//    int mFontWidth,mFontHeight;
//    int mCurrentFontSize;
//    int mCurrentFontIdx;
    
//    CGLString *mText[512];
//    CGLString *mTextLine[512];
//    CGLString *viewTapInfoStr[MAX_MENU_FX_STRING];
//    CGLString *mHeader;
    
    int mDeviceType;
    CGFloat safe_bottom,safe_left,safe_right,safe_top;
    bool is_macOS;
    bool is_iPad;
    
    short int real_spectrumL[SPECTRUM_BANDS*2],oreal_spectrumL[SPECTRUM_BANDS];
    int real_spectrumIL[SPECTRUM_BANDS];
    short int img_spectrumL[SPECTRUM_BANDS*2];
    short int real_spectrumR[SPECTRUM_BANDS*2],oreal_spectrumR[SPECTRUM_BANDS];
    int real_spectrumIR[SPECTRUM_BANDS];
    short int img_spectrumR[SPECTRUM_BANDS*2];
    int real_spectrumSumL[SPECTRUM_BANDS][8],real_spectrumSumR[SPECTRUM_BANDS][8];
    
    bool clearAudioFXbuffer;
    
    t_plPlaylist_entry *mPlaylist;
    int    mPlaylist_pos,mPlaylist_size;
    
    int loadRequestInProgress;
    
    int mShuffle;
    int mShouldUpdateInfos;
    int mLoopMode;
    
    int mPlayingPosRestart;
    BOOL mIsPlaying;
    int mRestart,mRestart_sub,mRestart_arc;
    
    int mHasFocus;
    float mScaleFactor;
    int mPaused;
    
    UIImage *cover_img,*default_cover;
}

@property t_plPlaylist_entry *mPlaylist;
@property (retain,nonatomic) UIButton *btnAddToPl;
@property int mShuffle;
@property int mShouldUpdateInfos,mLoopMode;
@property bool bShowVC,bShowEQ;
@property bool bRSactive;
@property (retain,nonatomic) UIImage *cover_img,*default_cover;
@property (retain,nonatomic) WaitingView *waitingView;
@property (nonatomic, strong) NSTimer *mouseHideTimer;

@property int not_expected_version;

-(IBAction)pushedAddToPl;
-(IBAction)pushedRating5;

-(IBAction)changeLoopMode;
-(IBAction)shuffle;

-(IBAction)pushedLoopInf;

-(IBAction)playNext;
-(IBAction)playPrev;
-(IBAction)playNextSub;
-(IBAction)playPrevSub;
-(void) longPressNextSubArc:(UIGestureRecognizer *)gestureRecognizer;
-(void) longPressPrevSubArc:(UIGestureRecognizer *)gestureRecognizer;

- (void)restartCurrent;

- (IBAction)showPlaylist;

- (IBAction)showEQ;

-(IBAction)changeScreenRecorderFlag;

- (IBAction)showInfo;
- (IBAction)hideInfo;
- (IBAction)infoFullscreen;
- (IBAction)infoNormal;

- (void)showRating:(int)rating;
-(void) checkGLViewCanDisplay;

-(void)updateBarPos;
-(void)fixToolbarButtonIcons;

-(void)saveSettings;
-(void)loadSettings:(int)safe_mode;
-(int)checkFlagOnStartup;

-(BOOL)play_curEntry:(int)subsong;
-(int)play_nextEntry;
-(int)play_prevEntry;

-(void)play_restart;

-(void)play_listmodules:(t_playlist*)pl start_index:(int)index;
-(void)play_listmodules:(NSArray *)array start_index:(int)index path:(NSArray *)arrayFilepaths;

-(BOOL)play_module:(NSString *)filePath fname:(NSString *)fileName subsong:(int)subsong;

-(void) updateInfos: (NSTimer *) theTimer;
-(int) add_to_playlist:(NSString*)filePath fileName:(NSString*)fileName forcenoplay:(int)forcenoplay;
-(int) add_to_playlist:(NSArray *)filePaths fileNames:(NSArray*)fileNames forcenoplay:(int)forcenoplay;
-(void) remove_from_playlist:(int)index;

-(void) enterBackground;
-(void) enterForeground;


- (IBAction)sliderProgressValueChanged:(id)sender;
- (IBAction)sliderProgressEndChange:(id)sender;
- (IBAction)sliderProgressBeginChange:(id)sender;
- (IBAction)playPushed;
- (IBAction)pausePushed;

- (void) settingsChanged:(int)scope;

-(IBAction) oglButtonPushed;

@property (nonatomic, strong) UINavigationBarAppearance *previousAppearance API_AVAILABLE(ios(13.0));
@property (nonatomic) UIBarStyle previousBarStyle;

@property int mHasFocus,mPaused;
@property int mPlaylist_size,mPlaylist_pos,mDeviceType;
@property bool is_macOS,is_iPad;
@property BOOL mIsPlaying;
@property float mScaleFactor;
@property (nonatomic, retain) ModizMusicPlayer *mplayer;
//@property (nonatomic,retain) NSMutableArray *mPlaylistFilenames,*mPlaylistFilepaths;
@property (nonatomic,retain) UIView *mInWasView;

//@property (retain) CLLocationManager *locManager;
@property (nonatomic, retain) IBOutlet UIView *infoMsgView;
@property (nonatomic, retain) IBOutlet UILabel *infoMsgLbl,*infoSecMsgLbl;
@property (nonatomic, retain) IBOutlet UIButton *btnChangeTime;
@property (nonatomic, retain) IBOutlet UIView *mainView,*infoView;
@property (nonatomic, retain) IBOutlet UIButton *mainRating5,*mainRating5off;
@property (nonatomic, retain) IBOutlet UIButton *backInfo,*infoZoom,*infoUnzoom;
@property (nonatomic, retain) IBOutlet BButton *infoButton,*eqButton;
@property (nonatomic, retain) IBOutlet UIButton *oglButton;

@property (nonatomic, retain) NSTimer *repeatingTimer;

@property (nonatomic, retain) IBOutlet UIView *detailView,*commandViewU;

@property (nonatomic, retain) IBOutlet UILabel *labelNumChannels,*labelModuleType,*playlistPos,*labelLibName;
@property (nonatomic, retain) IBOutlet UILabel *labelTime,*labelModuleLength;
@property (nonatomic, retain) IBOutlet UILabel *labelSeeking;
@property (nonatomic, retain) IBOutlet UILabel *labelModuleSize;
@property (nonatomic, retain) IBOutlet BButton *btnShowArcList,*btnShowSubSong,*btnShowVoices,*btnRecordScreen;
@property (nonatomic, retain) IBOutlet UIButton *buttonLoopTitleSel,*buttonLoopList,*buttonLoopListSel,*buttonShuffle,*buttonShuffleSel,*buttonShuffleOneSel,*btnLoopInf;
@property (nonatomic, retain) IBOutlet UIToolbar *playBar,*pauseBar,*playBarSub,*pauseBarSub;

@property (nonatomic, retain) IBOutlet OBSlider *sliderProgressModule;
@property (nonatomic, retain) IBOutlet UITextView *textMessage;

@property (nonatomic, retain) IBOutlet UIImageView *cover_view,*cover_viewBG;
@property (nonatomic, retain) IBOutlet UIView *cover_viewAll;
@property (nonatomic, retain) UIImageView *gifAnimation;

@property int mOnlyCurrentEntry;
@property int mOnlyCurrentSubEntry;
@property int64_t seekRequested;

-(IBAction) changeTimeDisplay;

-(IBAction)showVoicesSelector:(id)sender;
-(IBAction)showSubSongSelector:(id)sender;
-(IBAction)showArcSelector:(id)sender;


- (void)titleTap:(UITapGestureRecognizer *)sender;

-(void) pushedRatingCommon:(signed char)rating;
-(void) hidePopup;
-(void) openPopup:(NSString *)msg secmsg:(NSString *)secmsg style:(int)style;
-(void) closePopup;
-(BOOL) play_loadArchiveModule;

-(NSString*) getCurrentModuleFilepath;

- (void) checkAvailableCovers:(int)index;

- (void)animationDidStop:(NSString *)animationID finished:(NSNumber *)finished context:(void *)context;

-(bool) isCancelPending;
-(void) resetCancelStatus;

-(void) cancelPushed;
-(void)showWaitingCancel;
-(void)hideWaitingCancel;
-(void)hideWaitingProgress;
-(void) showWaitingProgress;
-(void)showWaiting;
-(void)flushMainLoop;
-(void)updateWaitingDetail:(NSString *)text;
-(void)updateWaitingTitle:(NSString *)text;
-(void)hideWaiting;
-(void) setProgressWaiting:(NSNumber*)progress;
-(void) setCancelStatus:(bool)status;

- (UIViewController *)visibleViewController:(UIViewController *)rootViewController;

-(void) stop;
-(void) clearQueue;
-(void) seek:(NSNumber*)seekTime;
-(void) updMediaCenterProgress;
-(void) updMediaCenter;
-(void) cmdLike;
-(void) cmdDislike;

- (void)StartRecording;
- (void)StopRecording;

-(void) loadNewFileFailed:(NSString *)filePath fname:(NSString *)fileName arcidx:(int)arcidx subsong:(int)subsong;
-(void) loadNewFileCompleted:(NSString *)filePath fname:(NSString *)fileName arcidx:(int)arcidx subsong:(int)subsong;
-(int) requestLoadNewFile:(NSString *)filePath fname:(NSString *)fileName arcidx:(int)arcidx subsong:(int)subsong;

-(void) generateBGTexture;

-(void) jumpSeekFwd;
-(void) jumpSeekBwd;
-(void) oglViewSwitchFS;

-(void) mdPrevPreset;
-(void) mdNextPreset;
-(void) mdChangeFavoriteStatusPreset:(int)val;
-(void) mdSwitchLockStatusPreset;
-(void) mdOpenCloseMenu;
-(void) mdBackAction;
-(void) mdSwitchFPSHud;
-(void) mdInfoFX;
-(void) mdSwitchVolBars;
-(void) mdSwitchModPatternTheme:(int)val;
-(void) mdSwitchModPatternFont:(int)val;
-(void) mdSwitchModPatternFontSize:(int)val;
-(void) mdSwitchFixedBar;
-(void) mdSwitchSpectrumBloom:(int)val;
-(void) mdSwitchLandscapeBloom:(int)val;
-(void) mdShowMusicInfo;

-(void) mdTestAsyncLoad;

-(void) mdShiftMode:(int)active;

-(void) mdSetFX:(int)fxIdx value:(int)value;

-(void) checkNewCover;
-(void) sendNotifPlayedTitle;

-(void) switchFX:(int)fxNb change:(int)val;
-(void) changeSettingsValue:(int)settingsIdx value:(float)value;

-(void) refreshFXFSLabels;

-(void) oglButtonMessage;

-(void) cleanPlaylistAfterDelDir:(NSString*)fullPath;
-(void) cleanPlaylistAfterDelFile:(NSString*)fullPath;

@end
