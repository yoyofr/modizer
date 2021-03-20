//
//  DetailViewController.m
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//



#define SELECTOR_TABVIEWCELL_HEIGHT 50
#define ARCSUB_MODE_NONE 0
#define ARCSUB_MODE_ARC 1
#define ARCSUB_MODE_SUB 2
static int current_selmode;

extern BOOL nvdsp_EQ;

#import <mach/mach.h>
#import <mach/mach_host.h>
#import "FFTAccelerate.h"
static FFTAccelerate *fftAccel;
static float *fft_frequency,*fft_time,*fft_frequencyAvg;
static int *fft_freqAvgCount;


#define LOCATION_UPDATE_TIMING 1800 //in second : 30minutes
#define NOTES_DISPLAY_LEFTMARGIN 30
#define NOTES_DISPLAY_TOPMARGIN 30


#include <sys/types.h>
#include <sys/sysctl.h>


#include "TextureUtils.h"
#include "DBHelper.h"

#define DEBUG_INFOS 0
#define DEBUG_NO_SETTINGS 0

#include "OGLView.h"
#include "RenderUtils.h"
#include "FXFluid.h"

#include <QuartzCore/CADisplayLink.h>
#import <OpenGLES/EAGLDrawable.h>
#include <OpenGLES/ES1/glext.h>
#import <QuartzCore/QuartzCore.h>

#import "UIImage+ImageEffects.h"

#import "UIImageResize.h"

#import "DetailViewControllerIphone.h"
#import "RootViewControllerPlaylist.h"
#import <MediaPlayer/MediaPlayer.h>

#import "EQViewController.h"

//#import "modplug.h"
//#import "../../libopenmpt/libmodplug/modplug.h"
#import "../libs/libopenmpt/openmpt-trunk/include/modplug/include/libmodplug/modplug.h"


#import "gme.h"

#import "math.h"

#import "Font.h"
#import "GLString.h"

#import "timidity.h"

#import "AnimatedGif.h"

/*extern "C" {
    int fix_fftr(short int f[], int m, int inverse);
    int fix_fft(short int  fr[], short int  fi[], short int  m, short int  inverse);
}*/

static RootViewControllerPlaylist *nowplayingPL;


static t_playlist* temp_playlist;

extern volatile t_settings settings[MAX_SETTINGS];

extern int tim_notes_cpy[SOUND_BUFFER_NB][DEFAULT_VOICES];
extern unsigned char tim_voicenb_cpy[SOUND_BUFFER_NB];
extern char mplayer_error_msg[1024];
int tim_midifx_note_range,tim_midifx_note_offset;

extern volatile int db_checked;

static int shouldRestart;

//static volatile int locManager_isOn;
static int coverflow_plsize,coverflow_pos,coverflow_needredraw;
static BOOL mOglViewIsHidden;
static volatile int mSendStatTimer;
static NSDate *locationLastUpdate=nil;
static double located_lat=999,located_lon=999;
int mDevice_hh,mDevice_ww;
static int mShouldHaveFocusAfterBackground,mLoadIssueMessage;
static int mRandomFXCpt,mRandomFXCptRev;
static int infoIsFullscreen=0;
static UIAlertView *alertCrash;
static MPVolumeView *volumeView;

static UIImage *cover_img,*default_cover;
static MPMediaItemArtwork *artwork;

static int txtMenuHandle[16];
static int txtSubMenuHandle[38];

//int texturePiano;

static volatile int mPopupAnimation=0;

//static UIAlertView *alertCannotPlay;
static int alertCannotPlay_displayed;

static int viewTapHelpInfo=0;
static int viewTapHelpShow=0;
static int viewTapHelpShowMode=0;
static int viewTapHelpShow_SubStart=0;
static int viewTapHelpShow_SubNb=0;

static NSString *located_country=nil,*located_city=nil;

static 	UIImage *covers_default; // album covers images

#define TOUCH_KEPT_THRESHOLD 10

#define max2(a,b) (a>b?a:b)
#define max4(a,b,c,d) max2(max2(a,b),max2(c,d))
#define max8(a,b,c,d,e,f,g,h) max2(max4(a,b,c,d),max4(e,f,g,h))

static int display_length_mode=0;

@implementation DetailViewControllerIphone

@synthesize coverflow,lblMainCoverflow,lblSecCoverflow,lblCurrentSongCFlow,lblTimeFCflow;
@synthesize mShuffle,mShouldUpdateInfos;
@synthesize btnPlayCFlow,btnPauseCFlow,btnBackCFlow,btnChangeTime,btnNextCFlow,btnPrevCFlow,btnNextSubCFlow,btnPrevSubCFlow;

@synthesize mOnlyCurrentSubEntry,mOnlyCurrentEntry;

@synthesize mDeviceType,mDeviceIPhoneX;
@synthesize is_macOS;
@synthesize cover_view,cover_viewBG,gifAnimation;
//@synthesize locManager;
@synthesize sc_allowPopup,infoMsgView,infoMsgLbl,infoSecMsgLbl;
@synthesize mIsPlaying,mPaused,mplayer,mPlaylist;
@synthesize labelModuleName,labelModuleLength, labelTime, labelModuleSize,textMessage,labelNumChannels,labelModuleType,labelSeeking,labelLibName;
@synthesize buttonLoopTitleSel,buttonLoopList,buttonLoopListSel,buttonShuffle,buttonShuffleSel,btnLoopInf;
@synthesize repeatingTimer;
@synthesize sliderProgressModule;
@synthesize detailView,commandViewU,volWin,playlistPos;
@synthesize playBar,pauseBar,playBarSub,pauseBarSub;
@synthesize playBarSubRewind,playBarSubFFwd,pauseBarSubRewind,pauseBarSubFFwd;
@synthesize mainView,infoView;
@synthesize mainRating1,mainRating1off,mainRating2,mainRating2off,mainRating3,mainRating3off,mainRating4,mainRating4off,mainRating5,mainRating5off;
@synthesize mShouldHaveFocus,mHasFocus,mScaleFactor;
@synthesize infoButton,backInfo;
@synthesize mPlaylist_pos,mPlaylist_size;

@synthesize oglButton;

@synthesize btnShowSubSong,btnShowArcList;

@synthesize infoZoom,infoUnzoom;
@synthesize mInWasView;
@synthesize mSlowDevice;

-(void)didSelectRowInAlertSubController:(NSInteger)row {
    mPaused=0;
    [self play_curEntry];
    [mplayer playGoToSub:(int)row+mplayer.mod_minsub];
}

-(void) cancelSubSel {
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
}

-(IBAction)showSubSongSelector:(id)sender {
    UIViewController *controller = [[UIViewController alloc]init];
    UITableView *alertTableView;
    CGRect rect,recttv;
    const NSInteger kAlertTableViewTag = 10001;
    
    current_selmode=ARCSUB_MODE_SUB;
    
    float rw,rh,rx,ry;
    if (self.view.traitCollection.horizontalSizeClass==UIUserInterfaceSizeClassCompact) {
        float estimated_height=SELECTOR_TABVIEWCELL_HEIGHT*mplayer.mod_subsongs+32;
        rx=0;
        ry=32;
        rw=self.view.frame.size.width;
        
        if (estimated_height<self.view.frame.size.height-50-ry) rh=estimated_height;
        else rh=self.view.frame.size.height-50-ry;
        rect = CGRectMake(rx, ry,rw,rh+50);
        recttv = CGRectMake(rx, ry,rw,rh);
    } else {
        float estimated_height=SELECTOR_TABVIEWCELL_HEIGHT*mplayer.mod_subsongs+16;
        
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
    [containerView addSubview:alertTableView];
    
    alertTableView.delegate = self;
    alertTableView.dataSource = self;
    alertTableView.tableFooterView = [[UIView alloc]initWithFrame:CGRectZero];
    alertTableView.rowHeight=SELECTOR_TABVIEWCELL_HEIGHT;
    alertTableView.sectionHeaderHeight=32;
    
    [alertTableView setSeparatorStyle:UITableViewCellSeparatorStyleSingleLine];
    [alertTableView setTag:kAlertTableViewTag];
    [controller.view addSubview:containerView];// alertTableView];
    
    
    [controller.view bringSubviewToFront:containerView];//alertTableView];
    [controller.view setUserInteractionEnabled:YES];
    [alertTableView setUserInteractionEnabled:YES];
    [alertTableView setAllowsSelection:YES];
    
    BButton *cancel_btn= [[BButton alloc] initWithFrame:CGRectMake(self.view.frame.size.width/2-100,
                                                                    10,
                                                                    200,
                                                                    
                                                                    30)];
    [cancel_btn setType:BButtonTypePrimary];
    [cancel_btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
    [cancel_btn addTarget:self action:@selector(cancelSubSel) forControlEvents:UIControlEventTouchUpInside];
    [cancel_btn setTitle:NSLocalizedString(@"Cancel", @"Cancel Action") forState:UIControlStateNormal];
    [controller.view addSubview:cancel_btn];
    
    NSDictionary * buttonDic = NSDictionaryOfVariableBindings(cancel_btn);
    cancel_btn.translatesAutoresizingMaskIntoConstraints = NO;
    NSArray * hConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|-50-[cancel_btn]-50-|"
                                                                     options:0
                                                                     metrics:nil
                                                                       views:buttonDic];
    [controller.view addConstraints:hConstraints];
    
    NSArray * vConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[cancel_btn(50)]-16-|"
                                                                     options:0
                                                                     metrics:nil
                                                                       views:buttonDic];
    [controller.view addConstraints:vConstraints];
    
    [self presentViewController:controller animated:YES completion:^{
        NSIndexPath *indexPath = [NSIndexPath indexPathForRow:mplayer.mod_currentsub inSection:0];
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
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
    [self play_loadArchiveModule];
    [self hideWaiting];
    //self.outputLabel.text = [self.data objectAtIndex:row];
}

-(void) cancelArcSel {
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
}

-(IBAction)showArcSelector:(id)sender {
    UIViewController *controller = [[UIViewController alloc]init];
    UITableView *alertTableView;
    CGRect rect,recttv;
    const NSInteger kAlertTableViewTag = 10001;
    
    current_selmode=ARCSUB_MODE_ARC;
    
    float rw,rh,rx,ry;
    if (self.view.traitCollection.horizontalSizeClass==UIUserInterfaceSizeClassCompact) {
        float estimated_height=SELECTOR_TABVIEWCELL_HEIGHT*[mplayer getArcEntriesCnt]+32;
        rx=0;
        ry=32;
        rw=self.view.frame.size.width;
        
        if (estimated_height<self.view.frame.size.height-50-ry) rh=estimated_height;
        else rh=self.view.frame.size.height-50-ry;
        rect = CGRectMake(rx, ry,rw,rh+50);
        recttv = CGRectMake(rx, ry,rw,rh);
        
    } else {
        float estimated_height=SELECTOR_TABVIEWCELL_HEIGHT*[mplayer getArcEntriesCnt]+16;
        
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
    [containerView addSubview:alertTableView];
    
    alertTableView.delegate = self;
    alertTableView.dataSource = self;
    alertTableView.tableFooterView = [[UIView alloc]initWithFrame:CGRectZero];
    alertTableView.rowHeight=SELECTOR_TABVIEWCELL_HEIGHT;
    alertTableView.sectionHeaderHeight=32;
    
    [alertTableView setSeparatorStyle:UITableViewCellSeparatorStyleSingleLine];
    [alertTableView setTag:kAlertTableViewTag];
    [controller.view addSubview:containerView];// alertTableView];
    
    
    [controller.view bringSubviewToFront:containerView];//alertTableView];
    [controller.view setUserInteractionEnabled:YES];
    [alertTableView setUserInteractionEnabled:YES];
    [alertTableView setAllowsSelection:YES];
    
    
    
    BButton *cancel_btn= [[BButton alloc] initWithFrame:CGRectMake(self.view.frame.size.width/2-100,
                                                    10,
                                                    200,
                                                    
                                                    30)];
    [cancel_btn setType:BButtonTypePrimary];
    [cancel_btn removeTarget:self action:NULL forControlEvents:UIControlEventTouchUpInside];
    [cancel_btn addTarget:self action:@selector(cancelArcSel) forControlEvents:UIControlEventTouchUpInside];
    [cancel_btn setTitle:NSLocalizedString(@"Cancel", @"Cancel Action") forState:UIControlStateNormal];
    [controller.view addSubview:cancel_btn];
    
    NSDictionary * buttonDic = NSDictionaryOfVariableBindings(cancel_btn);
    cancel_btn.translatesAutoresizingMaskIntoConstraints = NO;
    NSArray * hConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"H:|-50-[cancel_btn]-50-|"
                                                                     options:0
                                                                     metrics:nil
                                                                       views:buttonDic];
    [controller.view addConstraints:hConstraints];
    
    NSArray * vConstraints = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[cancel_btn(50)]-16-|"
                                                                     options:0
                                                                     metrics:nil
                                                                       views:buttonDic];
    [controller.view addConstraints:vConstraints];
    
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

-(IBAction)pushedLoopInf {
	if (mplayer.mLoopMode==0) {
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
                [self play_curEntry];
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
                [self play_curEntry];
                if (subIdx) [mplayer playGoToSub:subIdx];
            }
        }
	}
}

-(int) computeActiveFX {
    int active_idx=0;
    if (settings[GLOB_FX1].detail.mdz_boolswitch.switch_value) active_idx|=1<<0;
    if (settings[GLOB_FX2].detail.mdz_switch.switch_value) active_idx|=1<<1;
    if (settings[GLOB_FX3].detail.mdz_switch.switch_value) active_idx|=1<<2;
    if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value) active_idx|=1<<3;
    
    if (settings[GLOB_FXOscillo].detail.mdz_switch.switch_value) active_idx|=1<<4;
    if (settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value) active_idx|=1<<5;
    if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value) active_idx|=1<<6;
    if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value) active_idx|=1<<7;
    
    if (settings[GLOB_FX4].detail.mdz_boolswitch.switch_value) active_idx|=1<<8;
    if (settings[GLOB_FX5].detail.mdz_switch.switch_value) active_idx|=1<<9;
    if (settings[GLOB_FXPiano].detail.mdz_switch.switch_value) active_idx|=1<<10;
    if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value) active_idx|=1<<11;
    return active_idx;
}

-(IBAction) oglButtonPushed {
    if (mOglViewIsHidden) {
        mOglViewIsHidden=NO;
    }
    else {
        mOglViewIsHidden=YES;
    }
    [self checkGLViewCanDisplay];
}

-(IBAction) optTIM_Polyphony {
    
}

- (void)showRating:(int)rating {
	mainRating1.hidden=TRUE;mainRating2.hidden=TRUE;mainRating3.hidden=TRUE;mainRating4.hidden=TRUE;mainRating5.hidden=TRUE;
	mainRating1off.hidden=FALSE;mainRating2off.hidden=FALSE;mainRating3off.hidden=FALSE;mainRating4off.hidden=FALSE;mainRating5off.hidden=FALSE;
	
	switch (rating) {
		case 5:
			mainRating5.hidden=FALSE;mainRating5off.hidden=TRUE;
		case 4:
			mainRating4.hidden=FALSE;mainRating4off.hidden=TRUE;
		case 3:
			mainRating3.hidden=FALSE;mainRating3off.hidden=TRUE;
		case 2:
			mainRating2.hidden=FALSE;mainRating2off.hidden=TRUE;
		case 1:
			mainRating1.hidden=FALSE;mainRating1off.hidden=TRUE;
		case 0:
			break;
			
	}
}

-(void) pushedRatingCommon:(signed char)rating{
    signed char tmp_rating;
    short int playcount;
    NSString *filePath,*fileName;
    filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
    fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
    
    //NSLog(@"updating rating (%d): %@\n%@",rating,fileName,filePath);
    
    if ([mplayer isArchive]) {
        /////////////////////////////////////////////////////////////////////////////:
        //Archive
        /////////////////////////////////////////////////////////////////////////////:
        if (mOnlyCurrentEntry) { //Only one entry
            //Update archive entry stats
            DBHelper::getFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],filePath,&playcount,&tmp_rating);
            if (rating==-1) rating=tmp_rating;
            DBHelper::updateFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],filePath,playcount,rating,[mplayer getSongLength],mplayer.numChannels,-1);
        } else { //All entries
            //Update current entry stats
            DBHelper::getFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],&playcount,&tmp_rating);
            if (rating==-1) rating=tmp_rating;
            DBHelper::updateFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],playcount,rating,[mplayer getSongLength],mplayer.numChannels,-1);
            
            //Update Global file stats
            //DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&tmp_rating);
            //DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,tmp_rating,[mplayer getGlobalLength],mplayer.numChannels,-[mplayer getArcEntriesCnt]);
            
            
        }
        
        DBHelper::updateFileStatsAvgRatingDBmod(filePath);
        
    } else if ([mplayer isMultiSongs]) {
        /////////////////////////////////////////////////////////////////////////////:
        //No archive but Multisubsongs
        /////////////////////////////////////////////////////////////////////////////:
        if (mOnlyCurrentSubEntry) { // only one subsong
            //Update subsong stats
            DBHelper::getFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],filePath,&playcount,&tmp_rating);
            if (rating==-1) rating=tmp_rating;
            DBHelper::updateFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],filePath,playcount,rating,[mplayer getSongLength],mplayer.numChannels,-1);
        } else { // all subsongs
            //Update subsong stats
            DBHelper::getFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],[NSString stringWithFormat:@"%@?%d",filePath,mplayer.mod_currentsub],&playcount,&tmp_rating);
            if (rating==-1) rating=tmp_rating;
            DBHelper::updateFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],[NSString stringWithFormat:@"%@?%d",filePath,mplayer.mod_currentsub],playcount,rating,[mplayer getSongLength],mplayer.numChannels,-1);
            
            //Update Global file stats
            //DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&tmp_rating);
            //DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,tmp_rating,[mplayer getGlobalLength],mplayer.numChannels,mplayer.mod_subsongs);
            
        }
        
        DBHelper::updateFileStatsAvgRatingDBmod(filePath);
    } else {
        /////////////////////////////////////////////////////////////////////////////:
        //No archive, no multisongs: simple file
        /////////////////////////////////////////////////////////////////////////////:
        
        //Update file stats
        DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&tmp_rating);
        if (rating==-1) rating=tmp_rating;
        DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,rating,[mplayer getSongLength],mplayer.numChannels,-1);
    }
    
	if (settings[GLOB_StatsUpload].detail.mdz_boolswitch.switch_value) {
		mSendStatTimer=0;
		[GoogleAppHelper SendStatistics:fileName path:filePath rating:mRating playcount:playcount country:located_country city:located_city longitude:located_lon latitude:located_lat];
	}
	mPlaylist[mPlaylist_pos].mPlaylistRating=rating;
	[self showRating:mRating];
}


-(IBAction)pushedRating1{
	if (!mPlaylist_size) return;
	
	if (mRating==1) mRating=0;
	else mRating=1;
	[self pushedRatingCommon:mRating];
}
-(IBAction)pushedRating2{
	if (!mPlaylist_size) return;
	mRating=2;
	[self pushedRatingCommon:mRating];
}
-(IBAction)pushedRating3{
	if (!mPlaylist_size) return;
	mRating=3;
	[self pushedRatingCommon:mRating];
}
-(IBAction)pushedRating4{
	if (!mPlaylist_size) return;
	mRating=4;
	[self pushedRatingCommon:mRating];
}
-(IBAction)pushedRating5{
	if (!mPlaylist_size) return;
	mRating=5;
	[self pushedRatingCommon:mRating];
}

static char note2charA[12]={'C','C','D','D','E','F','F','G','G','A','A','B'};
static char note2charB[12]={'-','#','-','#','-','-','#','-','#','-','#','-'};
static char dec2hex[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
static int currentPattern,currentRow,startChan,visibleChan;
static float oglTapX,oglTapY,movePx,movePy,movePxMOD,movePyMOD,movePxMID,movePyMID,movePxOld,movePyOld;
static float movePxFXPiano,movePyFXPiano,movePx2FXPiano,movePy2FXPiano,movePinchScaleFXPiano;
static float movePx2,movePy2,movePx2Old,movePy2Old;
static float movePinchScale,movePinchScaleOld;



- (void)settingsChanged:(int)scope {
    /////////////////////
    //GLOBAL
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_GLOBAL)) {
        [mplayer optGLOB_Panning:settings[GLOB_Panning].detail.mdz_boolswitch.switch_value];
        [mplayer optGLOB_PanningValue:settings[GLOB_PanningValue].detail.mdz_slider.slider_value];
        switch (settings[GLOB_ForceMono].detail.mdz_boolswitch.switch_value) {
            case 1:mplayer.optForceMono=1;break;
            case 0:mplayer.optForceMono=0;break;
        }
        [mplayer optGEN_DefaultLength:settings[GLOB_DefaultLength].detail.mdz_slider.slider_value];
        
        if ((mPlaylist_pos>=0)&&(mPlaylist_pos<mPlaylist_size)) {
            NSString *fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
            if (settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value) labelModuleName.text=[NSString stringWithString:fileName];
            else labelModuleName.text=[NSString stringWithString:[mplayer getModName]];
            lblCurrentSongCFlow.text=labelModuleName.text;
        }
    }
    
    /////////////////////
    //VISU
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_VISU)) {
        if (settings[GLOB_FXAlpha].detail.mdz_slider.slider_value==1.0f) m_oglView.layer.opaque = YES;
        else m_oglView.layer.opaque = NO;
        
        if (settings[GLOB_FX2].detail.mdz_switch.switch_value&&settings[GLOB_FX3].detail.mdz_switch.switch_value) {
            settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
            settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
            settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
        }
        if (settings[GLOB_FX4].detail.mdz_boolswitch.switch_value) {
            settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
            settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
            settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
        }
        if (settings[GLOB_FX5].detail.mdz_switch.switch_value) {
            settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
            settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
            settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
        }
        int size_chan;
        switch (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value) {
            case 1:
            case 4:
                size_chan=12*6;
                break;
            case 2:
            case 5:
                size_chan=6*6;
                break;
            case 3:
            case 6:
                size_chan=4*6;
                break;
        }
        visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN+size_chan-1)/size_chan;
        if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
        if (startChan<0) startChan=0;
        
        tim_midifx_note_range=88;// //128notes max
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==2) {
            tim_midifx_note_offset=(128-88)/2*m_oglView.frame.size.width/tim_midifx_note_range;
        } else {
            tim_midifx_note_offset=(128-88)/2*m_oglView.frame.size.height/tim_midifx_note_range;
        }
        
        
        
        [self checkGLViewCanDisplay];
    }
	
	
    
    /////////////////////
    //ADPLUG
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_ADPLUG)) {
        [mplayer optADPLUG:settings[ADPLUG_OplType].detail.mdz_switch.switch_value];
    }
    
    /////////////////////
    //AOSDK
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_AOSDK)) {
        [mplayer optAOSDK:settings[AOSDK_Reverb].detail.mdz_boolswitch.switch_value interpol:settings[AOSDK_Interpolation].detail.mdz_switch.switch_value];
        [mplayer optAOSDK_22KHZ:settings[AOSDK_DSF22KHZ].detail.mdz_switch.switch_value];
        [mplayer optAOSDK_DSFDSP:settings[AOSDK_DSFDSP].detail.mdz_boolswitch.switch_value];
        [mplayer optAOSDK_DSFEmuRatio:settings[AOSDK_DSFEmuRatio].detail.mdz_switch.switch_value];
        [mplayer optAOSDK_SSFDSP:settings[AOSDK_SSFDSP].detail.mdz_boolswitch.switch_value];
        [mplayer optAOSDK_SSFEmuRatio:settings[AOSDK_SSFEmuRatio].detail.mdz_switch.switch_value];
    }
    
    /////////////////////
    //SEXYPSF
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_SEXYPSF)) {
        [mplayer optSEXYPSF:settings[SEXYPSF_Reverb].detail.mdz_switch.switch_value interpol:settings[SEXYPSF_Interpolation].detail.mdz_switch.switch_value];
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
        [mplayer optVGMPLAY_MaxLoop:(unsigned int)settings[VGMPLAY_Maxloop].detail.mdz_slider.slider_value];
        [mplayer optVGMPLAY_YM2612emulator:(unsigned char)settings[VGMPLAY_YM2612Emulator].detail.mdz_slider.slider_value];
        [mplayer optVGMPLAY_PreferedJTag:(bool)settings[VGMPLAY_PreferJTAG].detail.mdz_boolswitch.switch_value];
    }
    
    /////////////////////
    //SID
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_SID)) {
        [mplayer optSIDFilter:settings[SID_Filter].detail.mdz_boolswitch.switch_value];
        [mplayer optSIDClock:settings[SID_CLOCK].detail.mdz_boolswitch.switch_value];
        [mplayer optSIDModel:settings[SID_MODEL].detail.mdz_boolswitch.switch_value];                
    }
    
    /////////////////////
    //DUMB
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_DUMB)) {
        [mplayer optDUMB_Resampling:settings[DUMB_Resampling].detail.mdz_switch.switch_value];
        [mplayer optDUMB_MastVol:settings[DUMB_MasterVolume].detail.mdz_slider.slider_value];
    }
    
    /////////////////////
    //GME
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_GME)) {
        [mplayer optGME_Fade:settings[GME_FADEOUT].detail.mdz_slider.slider_value*1000];
        [mplayer optGME_Ratio:settings[GME_RATIO].detail.mdz_slider.slider_value
                    isEnabled:settings[GME_RATIO_ONOFF].detail.mdz_boolswitch.switch_value];
        [mplayer optGME_EQ:settings[GME_EQ_TREBLE].detail.mdz_slider.slider_value bass:settings[GME_EQ_BASS].detail.mdz_slider.slider_value];
        [mplayer optGME_FX:settings[GME_FX_ONOFF].detail.mdz_boolswitch.switch_value
                  surround:settings[GME_FX_SURROUND].detail.mdz_boolswitch.switch_value
                      echo:settings[GME_FX_ECHO].detail.mdz_boolswitch.switch_value
                    stereo:settings[GME_FX_PANNING].detail.mdz_slider.slider_value];
        [mplayer optGME_IgnoreSilence:settings[GME_IGNORESILENCE].detail.mdz_boolswitch.switch_value];
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
    //TIMIDITY
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_TIMIDITY)) {
        [mplayer optTIM_Poly:settings[TIM_Polyphony].detail.mdz_slider.slider_value];
        [mplayer optTIM_Chorus:(int)(settings[TIM_Chorus].detail.mdz_boolswitch.switch_value)];
        [mplayer optTIM_Reverb:(int)(settings[TIM_Reverb].detail.mdz_boolswitch.switch_value)];
        [mplayer optTIM_Resample:(int)(settings[TIM_Resample].detail.mdz_switch.switch_value)];
        [mplayer optTIM_LPFilter:(int)(settings[TIM_LPFilter].detail.mdz_boolswitch.switch_value)];
        [mplayer optTIM_Amplification:(int)(settings[TIM_Amplification].detail.mdz_slider.slider_value)];
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
    //LAZYUSF
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_LAZYUSF)) {
        [mplayer optLAZYUSF_ResampleQuality:(int)(settings[LAZYUSF_ResampleQuality].detail.mdz_switch.switch_value)];
    }
    
    /////////////////////
    //MODPLUG
    /////////////////////
    if ((scope==SETTINGS_ALL)||(scope==SETTINGS_MODPLUG)) {
        ModPlug_Settings *mpsettings;
        mpsettings=[mplayer getMPSettings];
        switch ( settings[MODPLUG_Sampling].detail.mdz_switch.switch_value) {
            case 0:mpsettings->mResamplingMode = MODPLUG_RESAMPLE_NEAREST;break;
            case 1:mpsettings->mResamplingMode = MODPLUG_RESAMPLE_LINEAR;break;
            case 2:mpsettings->mResamplingMode = MODPLUG_RESAMPLE_SPLINE;break;
            case 3:mpsettings->mResamplingMode = MODPLUG_RESAMPLE_FIR;break;
        }
        mpsettings->mFlags|=MODPLUG_ENABLE_OVERSAMPLING|MODPLUG_ENABLE_NOISE_REDUCTION;
       /* switch ( settings[MODPLUG_Reverb].detail.mdz_boolswitch.switch_value) {
            case 1:mpsettings->mFlags|=MODPLUG_ENABLE_REVERB;break;
            case 0:mpsettings->mFlags&=~MODPLUG_ENABLE_REVERB;break;
        }
        switch ( settings[MODPLUG_Megabass].detail.mdz_boolswitch.switch_value) {
            case 1:mpsettings->mFlags|=MODPLUG_ENABLE_MEGABASS;break;
            case 0:mpsettings->mFlags&=~MODPLUG_ENABLE_MEGABASS;break;
        }
        switch ( settings[MODPLUG_Surround].detail.mdz_boolswitch.switch_value) {
            case 1:mpsettings->mFlags|=MODPLUG_ENABLE_SURROUND;break;
            case 0:mpsettings->mFlags&=~MODPLUG_ENABLE_SURROUND;break;
        }
        mpsettings->mReverbDepth=(int)(settings[MODPLUG_ReverbDepth].detail.mdz_slider.slider_value*100.0f);
        mpsettings->mReverbDelay=(int)(settings[MODPLUG_ReverbDelay].detail.mdz_slider.slider_value*160.0f+40.0f);
        mpsettings->mBassAmount=(int)(settings[MODPLUG_BassAmount].detail.mdz_slider.slider_value*100.0f);
        mpsettings->mBassRange=(int)(settings[MODPLUG_BassRange].detail.mdz_slider.slider_value*80.0f+20.0f);
        mpsettings->mSurroundDepth=(int)(settings[MODPLUG_SurroundDepth].detail.mdz_slider.slider_value*100.0f);
        mpsettings->mSurroundDelay=(int)(settings[MODPLUG_SurroundDelay].detail.mdz_slider.slider_value*35.0f+5.0f);*/
        mpsettings->mStereoSeparation=(int)(settings[MODPLUG_StereoSeparation].detail.mdz_slider.slider_value*255.0f+1);
        [mplayer setModPlugMasterVol:settings[MODPLUG_MasterVolume].detail.mdz_slider.slider_value];
        [mplayer updateMPSettings];
    }
	
}

-(void) checkGLViewCanDisplay{
    if (mOglViewIsHidden) {
		m_oglView.hidden=YES;
	} else {
		if ((infoView.hidden==YES)) {
			if (m_oglView.hidden) {
				viewTapHelpInfo=0;//255;
                if ([self computeActiveFX]==0) {viewTapHelpShow=1;viewTapHelpShowMode=1;}
                else viewTapHelpShow=0;
			}
			m_oglView.hidden=NO;
		}
	}
}

- (IBAction)changeLoopMode {
	mLoopMode++;
	if (mLoopMode==3) mLoopMode=0;
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
}

- (IBAction)shuffle {
	if (mShuffle) {
		buttonShuffle.hidden=NO;
		buttonShuffleSel.hidden=YES;
		mShuffle=NO;
	}
	else {
		buttonShuffle.hidden=YES;
		buttonShuffleSel.hidden=NO;
		mShuffle=YES;
	}
}

- (void)oglViewSwitchFS {
	oglViewFullscreen^=1;
	oglViewFullscreenChanged=1;
	[self shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientationHV];
}

- (IBAction)backPushed:(id)sender {
    [[self navigationController] setNavigationBarHidden:NO animated:NO];
    [[self navigationController] popViewControllerAnimated:YES];
}

- (IBAction)playPushed:(id)sender {
	mPaused=0;
	if (mIsPlaying) {
        if (btnPlayCFlow.hidden==NO) {
            btnPlayCFlow.hidden=YES;
            btnPauseCFlow.hidden=NO;
        }
        
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
		[self play_curEntry];
	}
	return;
}
- (IBAction)pausePushed:(id)sender {
	mPaused=1;
	if (mIsPlaying) {
        if (btnPauseCFlow.hidden==NO) {
            btnPauseCFlow.hidden=YES;
            btnPlayCFlow.hidden=NO;
        }
        
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

- (IBAction)sliderProgressModuleTest:(id)sender {
	int slider_time;
	sliderProgressModuleChanged=1;
	sliderProgressModuleEdit=1;
	if ([mplayer getSongLength]>0) slider_time=(int)(sliderProgressModule.value*(float)([mplayer getSongLength]-1));
	
    if (display_length_mode&&([mplayer getSongLength]>0)) {
        labelTime.text=[NSString stringWithFormat:@"-%.2d:%.2d", (([mplayer getSongLength]-slider_time)/1000)/60,(([mplayer getSongLength]-slider_time)/1000)%60];
    } else {
        labelTime.text=[NSString stringWithFormat:@"%.2d:%.2d", (slider_time/1000)/60,(slider_time/1000)%60];
    }
	return;
}

- (IBAction)sliderProgressModuleValueChanged:(id)sender {
	int curTime;
	if ([mplayer getSongLength]>0) curTime=(int)(sliderProgressModule.value*(float)([mplayer getSongLength]-1));
	[mplayer Seek:curTime];
	
	
	if (display_length_mode&&([mplayer getSongLength]>0)) labelTime.text=[NSString stringWithFormat:@"-%.2d:%.2d", (([mplayer getSongLength]-[mplayer getCurrentTime])/1000)/60,(([mplayer getSongLength]-[mplayer getCurrentTime])/1000)%60];
	else labelTime.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getCurrentTime]/1000)/60,([mplayer getCurrentTime]/1000)%60];
	sliderProgressModuleChanged=0;
	sliderProgressModuleEdit=0;
	return;
}

-(IBAction) changeTimeDisplay {
    display_length_mode^=1;
}

//define the targetmethod
-(void) updateInfos: (NSTimer *) theTimer {
    static int updMPNowCnt=0;
	int itime=[mplayer getCurrentTime];
	
	if (mSendStatTimer) {
		mSendStatTimer--;
		if (mSendStatTimer==0) {
			short int playcount;
            signed char tmp_rating;
			DBHelper::getFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,mPlaylist[mPlaylist_pos].mPlaylistFilepath,&playcount,&tmp_rating);
			[GoogleAppHelper SendStatistics:mPlaylist[mPlaylist_pos].mPlaylistFilename path:mPlaylist[mPlaylist_pos].mPlaylistFilepath rating:tmp_rating playcount:playcount country:located_country city:located_city longitude:located_lon latitude:located_lat];
		}
	}
	int mpl_upd=[mplayer shouldUpdateInfos];
	if (mpl_upd||mShouldUpdateInfos) {
        
        /////////////////////////////////////////////////////////////////////////////
        //update rating
        /////////////////////////////////////////////////////////////////////////////
        [self pushedRatingCommon:-1];
        [self showRating:mPlaylist[mPlaylist_pos].mPlaylistRating];
        
        
        if ((mpl_upd!=3)||(mShouldUpdateInfos)) {
            short int playcount=0;
            signed char tmp_rating;
            NSString *fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
            NSString *filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
            
            if ([mplayer isArchive]) {
                /////////////////////////////////////////////////////////////////////////////:
                //Archive
                /////////////////////////////////////////////////////////////////////////////:
                if (mOnlyCurrentEntry) { //Only one entry
                    //Update archive entry stats
                    DBHelper::getFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],filePath,&playcount,&tmp_rating);
                    DBHelper::updateFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],filePath,playcount,tmp_rating,[mplayer getSongLength],mplayer.numChannels,-1);
                    
                } else { //All entries
                    //Update Global file stats
                    DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&tmp_rating);
                    DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,tmp_rating,[mplayer getGlobalLength],mplayer.numChannels,-[mplayer getArcEntriesCnt]);
                    
                    //Update current entry stats
                    DBHelper::getFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],&playcount,&tmp_rating);
                    DBHelper::updateFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],playcount,tmp_rating,[mplayer getSongLength],mplayer.numChannels,-1);
                }
                
            } else if ([mplayer isMultiSongs]) {
                /////////////////////////////////////////////////////////////////////////////:
                //No archive but Multisubsongs
                /////////////////////////////////////////////////////////////////////////////:
                if (mOnlyCurrentSubEntry) { // only one subsong
                    //Update subsong stats
                    DBHelper::getFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],filePath,&playcount,&tmp_rating);
                    DBHelper::updateFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],filePath,playcount,tmp_rating,[mplayer getSongLength],mplayer.numChannels,-1);
                } else { // all subsongs
                    //Update Global file stats
                    DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&tmp_rating);
                    DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,tmp_rating,[mplayer getGlobalLength],mplayer.numChannels,mplayer.mod_subsongs);
                    
                    //Update subsong stats
                    DBHelper::getFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],[NSString stringWithFormat:@"%@?%d",filePath,mplayer.mod_currentsub],&playcount,&tmp_rating);
                    DBHelper::updateFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],[NSString stringWithFormat:@"%@?%d",filePath,mplayer.mod_currentsub],playcount,tmp_rating,[mplayer getSongLength],mplayer.numChannels,-1);
                }
            } else {
                /////////////////////////////////////////////////////////////////////////////:
                //No archive, no multisongs: simple file
                /////////////////////////////////////////////////////////////////////////////:
                
                //Update file stats
                DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&tmp_rating);
                DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,tmp_rating,[mplayer getSongLength],mplayer.numChannels,-1);
            }
            
            
            mShouldUpdateInfos=0;
        }
		if (mpl_upd>=2) {
			if (mpl_upd==2) {
                if (settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value==0) {
                    labelModuleName.text=[NSString stringWithString:[mplayer getModName]];
                    lblCurrentSongCFlow.text=labelModuleName.text;
                }
            }
            if (infoView.hidden==FALSE) textMessage.text=[NSString stringWithFormat:@"%@",[mplayer getModMessage]];
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
                                  mplayer.mod_currentsub,mplayer.mod_minsub,mplayer.mod_maxsub];
            } else {
                playlistPos.text=[NSString stringWithFormat:@"%d of %d/sub %d(%d,%d)",mPlaylist_pos+1,mPlaylist_size,mplayer.mod_currentsub,mplayer.mod_minsub,mplayer.mod_maxsub];
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
		
		if ([mplayer getSongLength]<0) {
            if (display_length_mode) display_length_mode=0;
			sliderProgressModule.enabled=FALSE;
            labelModuleLength.text=@"--:--";
		} else {
            sliderProgressModule.enabled=TRUE;
            labelModuleLength.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getSongLength]/1000)/60,([mplayer getSongLength]/1000)%60];
        }
	}
	
	if (([mplayer getSongLength]>0)&&(itime>[mplayer getSongLength])) // if gone too far, limit
		itime=[mplayer getSongLength];
	
	if (!sliderProgressModuleEdit) {
		labelTime.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getCurrentTime]/1000)/60,([mplayer getCurrentTime]/1000)%60];
        
        if ([mplayer getSongLength]>0) {
            lblTimeFCflow.text=[NSString stringWithFormat:@"%@ | %.2d:%.2d - %.2d:%.2d",playlistPos.text, ([mplayer getCurrentTime]/1000)/60,([mplayer getCurrentTime]/1000)%60,([mplayer getSongLength]/1000)/60,([mplayer getSongLength]/1000)%60];
        } else {
            lblTimeFCflow.text=[NSString stringWithFormat:@"%@ | %.2d:%.2d",playlistPos.text, ([mplayer getCurrentTime]/1000)/60,([mplayer getCurrentTime]/1000)%60];
        }
		if ([mplayer getSongLength]>0) {
			if (display_length_mode) labelTime.text=[NSString stringWithFormat:@"-%.2d:%.2d", (([mplayer getSongLength]-itime)/1000)/60,(([mplayer getSongLength]-itime)/1000)%60];
			sliderProgressModule.value=(float)(itime)/(float)([mplayer getSongLength]);
		}
		if ((mMoveStartChanLeft)&&(startChan>0)) startChan--;
		if ((mMoveStartChanRight)&&(startChan<(mplayer.numChannels-visibleChan))) startChan++;
	}
	
	int seekinprogress=[mplayer isSeeking];
	if (seekinprogress) {
		labelSeeking.hidden=FALSE;
		if (seekinprogress>0) labelSeeking.text=[NSString stringWithFormat:NSLocalizedString(@"Seeking %d%%",@""),seekinprogress];
		else labelSeeking.text=NSLocalizedString(@"Seeking",@"");
	} else labelSeeking.hidden=TRUE;
	
	if (/*(mPaused==0)&&*/(mplayer.bGlobalAudioPause==2)&&[mplayer isEndReached]) {//mod ended
		//have to update the pause button
		mSendStatTimer=0;
		mIsPlaying=FALSE;
		mPaused=1;
		self.pauseBarSub.hidden=YES;
		self.playBarSub.hidden=YES;
		self.pauseBar.hidden=YES;
		self.playBar.hidden=YES;
		if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.playBarSub.hidden=NO;
		else self.playBar.hidden=NO;
		
		[self updateBarPos];
		//and go to next entry if playlist
		if ((mLoopMode==2)||(mplayer.mLoopMode==1))  [self play_curEntry];
		else {
            if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&([mplayer getArcIndex]<[mplayer getArcEntriesCnt]-1)&&(mOnlyCurrentEntry==0)) {
                [mplayer selectNextArcEntry];
                [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
                [self shortWait];
                [self play_loadArchiveModule];
                [self hideWaiting];
            } else [self play_nextEntry];
        }
		
		return;
	} else {
        //		NSLog(@"waiting : %d %d",mplayer.bGlobalAudioPause,[mplayer isEndReached]);
	}
	
	if ( (mPaused==0)&&(mplayer.bGlobalAudioPause==0)&&(settings[GLOB_FXRandom].detail.mdz_boolswitch.switch_value) ) {
		if (mRandomFXCpt) {
			mRandomFXCpt--;
			mRandomFXCptRev++;
			if ((mRandomFXCptRev>ALLOW_CHANGE_ON_BEAT_TIME*5)&&((arc4random()&3)>1) ) {
				for (int i=0;i<SPECTRUM_BANDS;i++) {
					if (real_beatDetectedL[i]) {mRandomFXCpt=0;break;}
					if (real_beatDetectedR[i]) {mRandomFXCpt=0;break;}
				}
			}
		}
		if (mRandomFXCpt==0) {
			mRandomFXCpt=MIN_RANDFX_TIME*5+arc4random()%(5*MAX_RANDFX_TIME); //Min is 10 seconds Max is 60seconds
			mRandomFXCptRev=0;
			settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=0;
			settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=0;
			settings[GLOB_FX1].detail.mdz_boolswitch.switch_value=0;
			settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
			settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
            settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
            settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
			settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=0;
			switch (arc4random()%19) {
				case 0:
					settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					break;
				case 1:
					settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					break;
				case 2:
					settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=1;
					break;
				case 3:
					settings[GLOB_FX1].detail.mdz_boolswitch.switch_value=1;
					break;
				case 4:
					settings[GLOB_FX2].detail.mdz_switch.switch_value=(arc4random()%5)+1;
					break;
				case 5:
					settings[GLOB_FX3].detail.mdz_switch.switch_value=(arc4random()%3)+1;
					break;
				case 6:
					settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=1;
					settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					break;
				case 7:
					settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=1;
					settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					break;
				case 8:
					settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=1;
					settings[GLOB_FX1].detail.mdz_boolswitch.switch_value=1;
					break;
				case 9:
					settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=1;
					settings[GLOB_FX2].detail.mdz_switch.switch_value=(arc4random()%5)+1;
					break;
				case 10:
					settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=1;
					settings[GLOB_FX3].detail.mdz_switch.switch_value=(arc4random()%3)+1;
					break;
				case 11:
					settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					break;
				case 12:
					settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					settings[GLOB_FX2].detail.mdz_switch.switch_value=(arc4random()%5)+1;
					break;
				case 13:
					settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					settings[GLOB_FX3].detail.mdz_switch.switch_value=(arc4random()%3)+1;
					break;
				case 14:
					settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					settings[GLOB_FX1].detail.mdz_boolswitch.switch_value=1;
					break;
				case 15:
					settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=1;
					settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					break;
				case 16:
					settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=1;
					settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					settings[GLOB_FX1].detail.mdz_boolswitch.switch_value=1;
					break;
				case 17:
					settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=1;
					settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					settings[GLOB_FX2].detail.mdz_switch.switch_value=(arc4random()%5)+1;
					break;
				case 18:
					settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=1;
					settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=(arc4random()&1)+1;
					settings[GLOB_FX3].detail.mdz_switch.switch_value=(arc4random()%3)+1;
					break;
                    
			}
		}
		
	}
	
	if (updMPNowCnt==0) {
        MPNowPlayingInfoCenter *infoCenter = [MPNowPlayingInfoCenter defaultCenter];
        
        infoCenter.nowPlayingInfo = [NSDictionary dictionaryWithObjectsAndKeys:[NSString stringWithString:lblCurrentSongCFlow.text],
                                     MPMediaItemPropertyTitle,
                                     [NSString stringWithString:mPlaylist[mPlaylist_pos].mPlaylistFilepath],
                                     MPMediaItemPropertyArtist,
                                     [NSNumber numberWithFloat:(float)([mplayer getSongLength])/1000],
                                     MPMediaItemPropertyPlaybackDuration,
                                     [NSNumber numberWithFloat:(float)([mplayer getCurrentTime])/1000],
                                     MPNowPlayingInfoPropertyElapsedPlaybackTime,
                                     [NSNumber numberWithInt:1],
                                     MPNowPlayingInfoPropertyPlaybackRate,
                                     [NSNumber numberWithInt:mPlaylist_size],
                                     MPNowPlayingInfoPropertyPlaybackQueueCount,
                                     [NSNumber numberWithInt:mPlaylist_pos],
                                     MPNowPlayingInfoPropertyPlaybackQueueIndex,
                                     artwork,
                                     MPMediaItemPropertyArtwork,
                                     nil];
        updMPNowCnt=10;
    } else updMPNowCnt--;
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


- (IBAction)showPlaylist {
    
    temp_playlist=(t_playlist*)malloc(sizeof(t_playlist));
    memset(temp_playlist,0,sizeof(t_playlist));
    
    if (mPlaylist_size) { //display current queue
        for (int i=0;i<mPlaylist_size;i++) {
            temp_playlist->entries[i].label=[[NSString alloc] initWithString:mPlaylist[i].mPlaylistFilename];
            temp_playlist->entries[i].fullpath=[[NSString alloc ] initWithString:mPlaylist[i].mPlaylistFilepath];
            
            temp_playlist->entries[i].ratings=mPlaylist[i].mPlaylistRating;
            temp_playlist->entries[i].playcounts=-1;
        }
        temp_playlist->nb_entries=mPlaylist_size;
        temp_playlist->playlist_name=[[NSString alloc] initWithString:@"Now playing"];
        temp_playlist->playlist_id=nil;
        
        nowplayingPL = [[RootViewControllerPlaylist alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
        //set new title
        nowplayingPL.title = temp_playlist->playlist_name;
        ((RootViewControllerPlaylist*)nowplayingPL)->show_playlist=1;
        
        // Set new directory
        ((RootViewControllerPlaylist*)nowplayingPL)->browse_depth = 1;
        ((RootViewControllerPlaylist*)nowplayingPL)->detailViewController=self;
        ((RootViewControllerPlaylist*)nowplayingPL)->playlist=temp_playlist;
        ((RootViewControllerPlaylist*)nowplayingPL)->mFreePlaylist=1;
        ((RootViewControllerPlaylist*)nowplayingPL)->mDetailPlayerMode=1;
        ((RootViewControllerPlaylist*)nowplayingPL)->integrated_playlist=INTEGRATED_PLAYLIST_NOWPLAYING;
        ((RootViewControllerPlaylist*)nowplayingPL)->currentPlayedEntry=mPlaylist_pos+1;
        
        // And push the window
        [self.navigationController pushViewController:nowplayingPL animated:YES];
        
/*        NSIndexPath *myindex=[[[NSIndexPath alloc] initWithIndex:0] autorelease];
        [((RootViewControllerPlaylist*)childController)->tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos+1] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];*/
        
    }
    
}

- (IBAction)showEQ {
    eqVC = [[EQViewController alloc]  initWithNibName:@"EQViewController" bundle:[NSBundle mainBundle]];
    //set new title
    eqVC.title = @"Equalizer";
    eqVC.detailViewController=self;
    
    // And push the window
    [self.navigationController pushViewController:eqVC animated:YES];
}


- (IBAction)infoFullscreen {
	infoIsFullscreen=1;
	infoZoom.hidden=YES;
	infoUnzoom.hidden=NO;
	[self shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientationHV];
}
- (IBAction)infoNormal {
	infoIsFullscreen=0;
	infoZoom.hidden=NO;
	infoUnzoom.hidden=YES;
	mainView.hidden=NO;
	[self shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientationHV];
}

- (IBAction)showInfo {
	if (infoView.hidden==NO) {
		[self hideInfo];
		return;
	}
    textMessage.text=[NSString stringWithFormat:@"%@",[mplayer getModMessage]];
    
    
    mInWasView=m_oglView;
    mInWasViewHidden=m_oglView.hidden;
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDuration:0.70];
    //	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft  forView:mInWasView cache:YES];
	mInWasView.hidden=YES;
	[UIView commitAnimations];
	if (infoIsFullscreen) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.70];
        //		[UIView setAnimationDelegate:self];
		[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft  forView:mainView cache:YES];
		mainView.hidden=YES;
		[UIView commitAnimations];
	}
	
	infoView.hidden=NO;
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDuration:0.70];
    //	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft  forView:infoView cache:YES];
	[UIView commitAnimations];
	
}

- (IBAction)hideInfo {
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDuration:0.70];
    //	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:infoView cache:YES];
	infoView.hidden=YES;
	[UIView commitAnimations];
	
	if (mInWasView==m_oglView) {  //if ogl view was selected, check if it should be hidden
		
        if (mOglViewIsHidden) mInWasViewHidden=YES;
		else mInWasViewHidden=NO;
	}
	if (!mInWasViewHidden) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.70];
        //		[UIView setAnimationDelegate:self];
		[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:mInWasView cache:YES];
		mInWasView.hidden=NO;
		if (infoIsFullscreen) {
			[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:mainView cache:YES];
			mainView.hidden=NO;
		}
		[UIView commitAnimations];
	} else if (infoIsFullscreen) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.70];
        //		[UIView setAnimationDelegate:self];
		[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:mainView cache:YES];
		mainView.hidden=NO;
		[UIView commitAnimations];
	}
    
	
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDelay:1.00];
	[UIView setAnimationDuration:0.70];
    //	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft  forView:self.navigationItem.rightBarButtonItem.customView cache:YES];
	[UIView commitAnimations];
}


- (IBAction)playPrevSub {
    //if archive and no subsongs => change archive index
    if ([mplayer isArchive]&&(mplayer.mod_subsongs==1)) {
        [mplayer selectPrevArcEntry];
        [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
        [self shortWait];
        [self play_loadArchiveModule];
        [self hideWaiting];
    } else {
        [mplayer playPrevSub];
        if (mPaused) [self playPushed:nil];
    }
}

- (IBAction)playNextSub {
    //if archive and no subsongs => change archive index
    if ([mplayer isArchive]&&(mplayer.mod_subsongs==1)) {
        [mplayer selectNextArcEntry];
        [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
        [self shortWait];
        [self play_loadArchiveModule];
        [self hideWaiting];
    } else {
        [mplayer playNextSub];
        if (mPaused) [self playPushed:nil];
    }
}

-(void) longPressNextSubArc:(UIGestureRecognizer *)gestureRecognizer {
    if ([gestureRecognizer state]==UIGestureRecognizerStateBegan) {
        if ([mplayer isArchive]) {
            [mplayer selectNextArcEntry];
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            [self shortWait];
            [self play_loadArchiveModule];
            [self hideWaiting];
        }
    }
}

-(void) longPressPrevSubArc:(UIGestureRecognizer *)gestureRecognizer {
    if ([gestureRecognizer state]==UIGestureRecognizerStateBegan) {
        if ([mplayer isArchive]) {
            [mplayer selectPrevArcEntry];
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            [self shortWait];
            [self play_loadArchiveModule];
            [self hideWaiting];
        }
    }
}

- (IBAction)playNext {
	[self play_nextEntry];
}

- (IBAction)playPrev {
	if ([mplayer getCurrentTime]>=MIN_DELAY_PREV_ENTRY) {//if more than MIN_DELAY_PREV_ENTRY milliseconds are elapsed, restart current track
		[self play_curEntry];
	} else [self play_prevEntry];
}
-(BOOL)play_curEntry {
	NSString *fileName;
	NSString *filePath;
	mIsPlaying=FALSE;
    
	if (mPlaylist_size==0) {
        [repeatingTimer invalidate];
		repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
		[mplayer Stop];
        mPaused=1;
        if (mHasFocus) [[self navigationController] popViewControllerAnimated:YES];
        return FALSE;
    }
	
	if (mPlaylist_pos>mPlaylist_size-1) mPlaylist_pos=0;
	
	fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
	filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
	mPlaylist[mPlaylist_pos].mPlaylistCount++;
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
    
	if ([self play_module:filePath fname:fileName]==FALSE) {
		[self remove_from_playlist:mPlaylist_pos];
		if ((alertCannotPlay_displayed==0)&&(mLoadIssueMessage)) {
            NSString *alertMsg;
			alertCannotPlay_displayed=1;
            if (mplayer_error_msg[0]) alertMsg=[NSString stringWithFormat:@"%@\n%s", NSLocalizedString(@"File cannot be played. Skipping to next playable file.",@""),mplayer_error_msg];
            else alertMsg=NSLocalizedString(@"File cannot be played. Skipping to next playable file.",@"");
            
            UIAlertView *alertCannotPlay = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"")
                                                                       message:alertMsg delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil];
            if (alertCannotPlay) [alertCannotPlay show];
            
            
			[self play_curEntry];
			
		} else [self play_curEntry];
		//remove
        
        
        [self hideWaiting];
		
		return FALSE;
	}
    
    [self hideWaiting];
    
	return TRUE;
}

-(void)play_prevEntry {
    if (mPlaylist_size==0) {
        [repeatingTimer invalidate];
		repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
		[mplayer Stop];
        mPaused=1;
        if (mHasFocus) [[self navigationController] popViewControllerAnimated:YES];
        return;
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
		[self play_curEntry];
	} else {
		if (mPlaylist_pos>0) mPlaylist_pos--;
		else if (mLoopMode==1) mPlaylist_pos=mPlaylist_size-1;
		[self play_curEntry];
	}
}

-(void)play_nextEntry {
    if (mPlaylist_size==0) {
        [repeatingTimer invalidate];
		repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
		[mplayer Stop];
        mPaused=1;
        if (mHasFocus) [[self navigationController] popViewControllerAnimated:YES];
        return;
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
		[self play_curEntry];
	} else if (mPlaylist_pos<mPlaylist_size-1) {
		mPlaylist_pos++;
		[self play_curEntry];
	} else if (mLoopMode==1) {
		mPlaylist_pos=0;
		[self play_curEntry];
	}
}

-(void)play_randomEntry {
	
}

-(void)play_listmodules:(NSArray *)array start_index:(int)index path:(NSArray *)arrayFilepaths {
	int limitPl=0;
	mRestart=0;
	mRestart_sub=0;
    mRestart_arc=0;
	mPlayingPosRestart=0;
	
	if ([array count]>=MAX_PL_ENTRIES) {
		NSString *msg_str=[NSString stringWithFormat:NSLocalizedString(@"Too much entries! Playlist will be limited to %d first entries.",@""),MAX_PL_ENTRIES];
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:msg_str delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil];
		[alert show];
		limitPl=1;
		//		return;
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
		
		//DBHelper::getFileStatsDBmod(mPlaylist[i].mPlaylistFilename,mPlaylist[i].mPlaylistFilepath,&playcount,&rating);
		mPlaylist[i].mPlaylistRating=-1;//rating;
		mPlaylist[i].mPlaylistCount=0;
        //[self checkAvailableCovers:i];
        mPlaylist[i].cover_flag=-1;
	}
    coverflow_needredraw=1;
	
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
	
	[self play_curEntry];
}

-(void)play_listmodules:(t_playlist*)pl start_index:(int)index {
	int limitPl=0;
	mRestart=0;
	mRestart_sub=0;
    mRestart_arc=0;
	mPlayingPosRestart=0;
    
	if (pl->nb_entries>=MAX_PL_ENTRIES) {
		NSString *msg_str=[NSString stringWithFormat:NSLocalizedString(@"Too much entries! Playlist will be limited to %d first entries.",@""),MAX_PL_ENTRIES];
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:msg_str delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil];
		[alert show];
		limitPl=1;
        //		return;
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
        //        [self checkAvailableCovers:i];
        mPlaylist[i].cover_flag=-1;
	}
    coverflow_needredraw=1;
	
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
    
	[self play_curEntry];
}

-(void)play_restart {
	for (int i=0;i<mPlaylist_size;i++) {
		mPlaylist[i].mPlaylistCount=0;
	}
    //	DBHelper::getFilesStatsDBmod(mPlaylist,mPlaylist_size);
	//[playlistTabView reloadData];
    
	//if (segcont_resumeLaunch.selectedSegmentIndex==0) return;
	mRestart=1;
    
	if ([self play_curEntry]) {
		//	self.tabBarController.selectedViewController = self; //detailViewController;
	}
}

-(int) add_to_playlist:(NSArray *)filePaths fileNames:(NSArray*)fileNames forcenoplay:(int)forcenoplay{
	int added_pos;
	int playLaunched=0;
	int add_entries_nb=[fileNames count];
    
    coverflow_needredraw=1;
    
    //    [self openPopup:@"Playlist updated"];
	if (mPlaylist_size+add_entries_nb>=MAX_PL_ENTRIES) {
		NSString *msg_str=[NSString stringWithFormat:NSLocalizedString(@"Too much entries! Playlist will be limited to %d first entries.",@""),MAX_PL_ENTRIES];
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:msg_str delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil];
		[alert show];
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
                //[self checkAvailableCovers:i];
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
                //[self checkAvailableCovers:i+mPlaylist_pos+1];
                mPlaylist[i+mPlaylist_pos+1].cover_flag=-1;
			}
			added_pos=mPlaylist_pos+1;
		} else { //last
			for (int i=0;i<add_entries_nb;i++) {
				mPlaylist[i+mPlaylist_size].mPlaylistFilename=[[NSString alloc] initWithString:[fileNames objectAtIndex:i]];
				mPlaylist[i+mPlaylist_size].mPlaylistFilepath=[[NSString alloc] initWithString:[filePaths objectAtIndex:i]];
                mPlaylist[i+mPlaylist_size].mPlaylistRating=-1;
                //[self checkAvailableCovers:i+mPlaylist_size];
                mPlaylist[i+mPlaylist_size].cover_flag=-1;
			}
			added_pos=mPlaylist_size;
		}
		mPlaylist_size+=add_entries_nb;
		//TODO To optimize
		for (int i=added_pos;i<add_entries_nb;i++) {
			mPlaylist[i].mPlaylistCount=0; //new entry
		}
		//DBHelper::getFilesStatsDBmod(&(mPlaylist[added_pos]),add_entries_nb);
		
		
		mShouldUpdateInfos=1;
	} else { //create a new playlist with downloaded song
		for (int i=0;i<add_entries_nb;i++) {
			mPlaylist[i].mPlaylistFilename=[[NSString alloc] initWithString:[fileNames objectAtIndex:i]];
			mPlaylist[i].mPlaylistFilepath=[[NSString alloc] initWithString:[filePaths objectAtIndex:i]];
			mPlaylist[i].mPlaylistCount=0;
            mPlaylist[i].mPlaylistRating=-1;
            //[self checkAvailableCovers:i];
            mPlaylist[i].cover_flag=-1;
		}
		mPlaylist_size=add_entries_nb;
		//DBHelper::getFilesStatsDBmod(mPlaylist,mPlaylist_size);
		mPlaylist_pos=0;
		added_pos=0;
		[self play_curEntry];
		playLaunched=1;
	}
	if ((playLaunched==0)&&(!forcenoplay)&&(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==2)) {//Enqueue & play
		mPlaylist_pos=added_pos;
		[self play_curEntry];
		playLaunched=1;
	}
	
    /*	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
     if (mPlaylist_size) [self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
     [myindex autorelease];*/
	return playLaunched;
}


-(int) add_to_playlist:(NSString*)filePath fileName:(NSString*)fileName forcenoplay:(int)forcenoplay{
	int added_pos;
	int playLaunched=0;
	short int playcount=0;
	signed char rating=0;
	if (mPlaylist_size>=MAX_PL_ENTRIES) {
		NSString *msg_str=[NSString stringWithFormat:NSLocalizedString(@"Too much entries! Playlist will be limited to %d first entries.",@""),MAX_PL_ENTRIES];
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:msg_str delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil];
		[alert show];
		return 0;
	}
    coverflow_needredraw=1;
    
	if (mPlaylist_size) { //already in a playlist : append to it
		if (settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value==0) {
			for (int i=mPlaylist_size-1;i>=0;i--) {
				mPlaylist[i+1]=mPlaylist[i];
			}
			mPlaylist[0].mPlaylistFilename=[[NSString alloc] initWithString:fileName];
			mPlaylist[0].mPlaylistFilepath=[[NSString alloc] initWithString:filePath];
            //[self checkAvailableCovers:0];
            mPlaylist[0].cover_flag=-1;
			
			added_pos=0;
			mPlaylist_pos++;
			
		} else if ((settings[GLOB_EnqueueMode].detail.mdz_switch.switch_value==1)&&(mPlaylist_pos<mPlaylist_size-1)) { //after current
			for (int i=mPlaylist_size-1;i>mPlaylist_pos;i--) {
				mPlaylist[i+1]=mPlaylist[i];
			}
			
			mPlaylist[mPlaylist_pos+1].mPlaylistFilename=[[NSString alloc] initWithString:fileName];
			mPlaylist[mPlaylist_pos+1].mPlaylistFilepath=[[NSString alloc] initWithString:filePath];
            //[self checkAvailableCovers:mPlaylist_pos+1];
            mPlaylist[mPlaylist_pos+1].cover_flag=-1;
			added_pos=mPlaylist_pos+1;
		} else { //last
			mPlaylist[mPlaylist_size].mPlaylistFilename=[[NSString alloc] initWithString:fileName];
			mPlaylist[mPlaylist_size].mPlaylistFilepath=[[NSString alloc] initWithString:filePath];
            //            [self checkAvailableCovers:mPlaylist_size];
            mPlaylist[mPlaylist_size].cover_flag=-1;
			added_pos=mPlaylist_size;
		}
		mPlaylist_size++;
		
		//TODO To optimize
		mPlaylist[added_pos].mPlaylistCount=0;
		rating=0;
		DBHelper::getFileStatsDBmod(mPlaylist[added_pos].mPlaylistFilename,mPlaylist[added_pos].mPlaylistFilepath,&playcount,&rating);
		mPlaylist[added_pos].mPlaylistRating=rating;
        
		mShouldUpdateInfos=1;
	} else { //create a new playlist with downloaded song
		mPlaylist[0].mPlaylistFilename=[[NSString alloc] initWithString:fileName];
		mPlaylist[0].mPlaylistFilepath=[[NSString alloc] initWithString:filePath];
        //[self checkAvailableCovers:0];
        mPlaylist[0].cover_flag=-1;
		added_pos=0;
		mPlaylist_size=1;
		mPlaylist[added_pos].mPlaylistCount=0;
		rating=0;
		DBHelper::getFileStatsDBmod(mPlaylist[added_pos].mPlaylistFilename,mPlaylist[added_pos].mPlaylistFilepath,&playcount,&rating);
		mPlaylist[added_pos].mPlaylistRating=rating;
		
		mPlaylist_pos=0;
		
		[self play_curEntry];
		playLaunched=1;
	}
	if ((!forcenoplay)&&(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==2)) {//Enqueue & play
		mPlaylist_pos=added_pos;
		[self play_curEntry];
		playLaunched=1;
	}
	
	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
    /*	if (mPlaylist_size) [self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];*/
	//[myindex autorelease];
	return playLaunched;
}

-(void) remove_from_playlist:(int)index {//remove entry
    coverflow_needredraw=1;
    
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
        /*		NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
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
	int retcode;
	NSString *fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
	NSString *filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
    
    if (!filePath) return FALSE;
	
    
    mOnlyCurrentEntry=0;
    mOnlyCurrentSubEntry=0;
	mSendStatTimer=0;
	
	// if already playing, stop
	if ([mplayer isPlaying]) {
		[repeatingTimer invalidate];
		repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
		[mplayer Stop];
	}
    mShouldUpdateInfos=1;
	
    
    //ensure any settings changes to be taken into account before loading next file
    [self settingsChanged:(int)SETTINGS_ALL];
    
    if (mShuffle) {
        mOnlyCurrentSubEntry=1;
        mOnlyCurrentEntry=1;
    }
    
	// load module

	if ((retcode=[mplayer LoadModule:filePath defaultMODPLAYER:settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value defaultSAPPLAYER:settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value defaultVGMPLAYER:settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value  slowDevice:mSlowDevice archiveMode:1 archiveIndex:-1 singleSubMode:mOnlyCurrentSubEntry  singleArcMode:mOnlyCurrentEntry])) {
		//error while loading
        
        if ( [mplayer isArchive] &&
            ([mplayer getArcIndex]<[mplayer getArcEntriesCnt]-1) &&
            !mOnlyCurrentSubEntry &&
            !mOnlyCurrentEntry ) {
            
            do {
                [mplayer selectNextArcEntry];
                if ([mplayer getArcIndex]>=[mplayer getArcEntriesCnt]) break;
                mRestart_arc=[mplayer getArcIndex];
                retcode=[mplayer LoadModule:filePath defaultMODPLAYER:settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value defaultSAPPLAYER:settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value defaultVGMPLAYER:settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value slowDevice:mSlowDevice archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry];
            } while (retcode);
        }
        
        if (retcode) {
            NSLog(@"Issue in LoadModule(archive) %@",filePath);
            if (retcode==-99) mLoadIssueMessage=0;
            else mLoadIssueMessage=1;
            return FALSE;
        }
	}
    
    if (mShuffle) {
        if ([mplayer isArchive]) {
//            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            //[self shortWait];
            [mplayer Stop]; //deallocate relevant items
            mRestart_arc=arc4random()%[mplayer getArcEntriesCnt];

            if ((retcode=[mplayer LoadModule:filePath defaultMODPLAYER:settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value defaultSAPPLAYER:settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value defaultVGMPLAYER:settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value slowDevice:mSlowDevice archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry  singleArcMode:mOnlyCurrentEntry])) {
                //error while loading
                if ( [mplayer isArchive] &&
                    ([mplayer getArcIndex]<[mplayer getArcEntriesCnt]-1) &&
                    !mOnlyCurrentSubEntry &&
                    !mOnlyCurrentEntry ) {
                    
                    do {
                        [mplayer selectNextArcEntry];
                        if ([mplayer getArcIndex]>=[mplayer getArcEntriesCnt]) break;
                        mRestart_arc=[mplayer getArcIndex];
                        retcode=[mplayer LoadModule:filePath defaultMODPLAYER:settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value defaultSAPPLAYER:settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value defaultVGMPLAYER:settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value slowDevice:mSlowDevice archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry];
                    } while (retcode);
                    
                }
                
                if (retcode) {
                    NSLog(@"Issue in LoadModule(archive) %@",filePath);
                    if (retcode==-99) mLoadIssueMessage=0;
                    else mLoadIssueMessage=2;
                    return FALSE;
                }
            }

//            [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
        }
    }
    
    //fix issue with modplug settings reset after load
    [self settingsChanged:(int)SETTINGS_ALL];
    
    [self checkForCover:filePath];
    
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
	
	if ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0)) btnShowSubSong.hidden=false;
	else btnShowSubSong.hidden=true;
    
    if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0)) btnShowArcList.hidden=false;
	else btnShowArcList.hidden=true;
    
	
	alertCannotPlay_displayed=0;
	//Visiulization stuff
	startChan=0;
    movePx=movePy=movePxOld=movePyOld=0;
    movePx2=movePy2=movePx2Old=movePy2Old=0;
    movePinchScale=movePinchScaleOld=0;
	sliderProgressModuleEdit=0;
	sliderProgressModuleChanged=0;
    
    
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
            mOnlyCurrentSubEntry=1;
            mRestart_sub=arc4random()%(mplayer.mod_subsongs)+mplayer.mod_minsub;
        }
        [mplayer PlaySeek:mPlayingPosRestart subsong:mRestart_sub];
    } else {
        [mplayer PlaySeek:mPlayingPosRestart subsong:0];
    }
    sliderProgressModule.value=0;
    mIsPlaying=YES;
    mPaused=0;
	
	//Update song info if required
	labelModuleName.hidden=NO;
    if (settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value) labelModuleName.text=[NSString stringWithString:fileName];
    else labelModuleName.text=[NSString stringWithString:[mplayer getModName]];
    lblCurrentSongCFlow.text=labelModuleName.text;
	labelModuleName.textColor = [UIColor colorWithRed:0.95 green:0.95 blue:0.99 alpha:1.0];
	labelModuleName.glowColor = [UIColor colorWithRed:0.40 green:0.40 blue:0.99 alpha:1.0];
	labelModuleName.glowOffset = CGSizeMake(0.0, 0.0);
	labelModuleName.glowAmount = 15.0;
	[labelModuleName setNeedsDisplay];
	self.navigationItem.titleView=labelModuleName;
    self.navigationItem.title=labelModuleName.text;
	
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
    DBHelper::getFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],&playcount,&mRating);
    if (!mRestart) playcount++;
    mPlaylist[mPlaylist_pos].mPlaylistRating=mRating;
    DBHelper::updateFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],playcount,mRating,[mplayer getSongLength],mplayer.numChannels,-1);
    
	[self showRating:mRating];
	
	
    
    if (coverflow.hidden==NO) {
        btnPlayCFlow.hidden=YES;
        btnPauseCFlow.hidden=NO;
        btnPrevCFlow.hidden=NO;
        btnNextCFlow.hidden=NO;
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) {
            btnPrevSubCFlow.hidden=NO;
            btnNextSubCFlow.hidden=NO;
        } else {
            btnPrevSubCFlow.hidden=YES;
            btnNextSubCFlow.hidden=YES;
        }
    }
    
	//set volume (if applicable)
	[mplayer setModPlugMasterVol:settings[MODPLUG_MasterVolume].detail.mdz_slider.slider_value];
	
	
	labelTime.text=@"00:00";
	if (mplayer.numChannels) {
		if (mplayer.numChannels==1) labelNumChannels.text=[NSString stringWithFormat:@"1 channel"];
		else labelNumChannels.text=[NSString stringWithFormat:@"%d channels",mplayer.numChannels];
	} else labelNumChannels.text=[NSString stringWithFormat:@""];
	
	labelModuleType.text=[NSString stringWithFormat:@"Format: %@",[mplayer getModType]];
	labelLibName.text=[NSString stringWithFormat:@"Player: %@",[mplayer getPlayerName]];
	textMessage.text=[NSString stringWithFormat:@"\n%@",[mplayer getModMessage]];
	
	[textMessage scrollRangeToVisible:NSMakeRange(0, 1)];
	//Activate timer for play infos
	repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.10f target:self selector:@selector(updateInfos:) userInfo:nil repeats: YES]; //10 times/second
	
    if (settings[GLOB_CoverFlow].detail.mdz_boolswitch.switch_value) {
        if (coverflow.hidden==FALSE) {
            if (coverflow.numberOfCovers!=mPlaylist_size) [coverflow setNumberOfCovers:mPlaylist_size];
            if (coverflow.currentIndex!=mPlaylist_pos) {
                coverflow_pos=mPlaylist_pos;
                [coverflow setCurrentIndex:mPlaylist_pos];
                //[coverflow  bringCoverAtIndexToFront:mPlaylist_pos animated:YES];
            }
        } else coverflow_needredraw=1;
    }
    
    
    if (nowplayingPL) {
        NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
        [nowplayingPL.tableView reloadData];
        nowplayingPL.currentPlayedEntry=mPlaylist_pos;
        [nowplayingPL.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos+1] animated:YES scrollPosition:UITableViewScrollPositionMiddle];
    }
    
    
	return TRUE;
}

- (void)titleTap:(UITapGestureRecognizer *)sender {
    //    NSLog(@"%@",labelModuleName.text);
    [self openPopup:labelModuleName.text secmsg:mPlaylist[mPlaylist_pos].mPlaylistFilepath];
}

-(NSString*) getFullFilePath:(NSString *)_filePath {
    NSString *fullFilePath;
    
    if ([_filePath length]>2) {
        if (([_filePath characterAtIndex:0]=='/')&&([_filePath characterAtIndex:1]=='/')) fullFilePath=[_filePath substringFromIndex:2];
        else fullFilePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
    } else fullFilePath=[NSHomeDirectory() stringByAppendingPathComponent:_filePath];
    return fullFilePath;
}

-(void) checkForCover:(NSString *)filePath {
    NSString *pathFolderImgPNG,*pathFileImgPNG,*pathFolderImgJPG,*pathFolderImgJPEG,*pathFileImgJPG,*pathFileImgJPEG,*pathFolderImgGIF,*pathFileImgGIF;
    
    pathFolderImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.png",[filePath stringByDeletingLastPathComponent]];
    pathFolderImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.jpg",[filePath stringByDeletingLastPathComponent]];
    pathFolderImgJPEG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.jpeg",[filePath stringByDeletingLastPathComponent]];
    pathFolderImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.gif",[filePath stringByDeletingLastPathComponent]];
    pathFileImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.png",[filePath stringByDeletingPathExtension]];
    pathFileImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.jpg",[filePath stringByDeletingPathExtension]];
    pathFileImgJPEG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.jpeg",[filePath stringByDeletingPathExtension]];
    pathFileImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@.gif",[filePath stringByDeletingPathExtension]];
    
    cover_img=nil;
    //    cover_img=[UIImage imageWithData:[NSData dataWithContentsOfFile:pathFolderImgPNG]];
    if (gifAnimation) [gifAnimation removeFromSuperview];
    gifAnimation=nil;
    
    cover_img=[UIImage imageWithContentsOfFile:pathFileImgJPG];
    if (cover_img==nil) cover_img=[UIImage imageWithContentsOfFile:pathFileImgJPEG];
    if (cover_img==nil) cover_img=[UIImage imageWithContentsOfFile:pathFileImgPNG];
    if (cover_img==nil) {
        cover_img=[UIImage imageWithContentsOfFile:pathFileImgGIF];
        if (cover_img) {
            NSURL* firstUrl = [NSURL fileURLWithPath:pathFileImgGIF];
            gifAnimation = [AnimatedGif getAnimationForGifAtUrl: firstUrl];
            
            gifAnimation.frame=CGRectMake(0, 0,cover_view.frame.size.width,cover_view.frame.size.height);
            [gifAnimation layoutSubviews];
            [cover_view addSubview:gifAnimation];
        }
    }
    if (cover_img==nil) cover_img=[UIImage imageWithContentsOfFile:pathFolderImgJPG];
    if (cover_img==nil) cover_img=[UIImage imageWithContentsOfFile:pathFolderImgJPEG];
    if (cover_img==nil) cover_img=[UIImage imageWithContentsOfFile:pathFolderImgPNG];
    if (cover_img==nil) {
        cover_img=[UIImage imageWithContentsOfFile:pathFolderImgGIF];
        if (cover_img) {
            NSURL* firstUrl = [NSURL fileURLWithPath:pathFileImgGIF];
            gifAnimation= [AnimatedGif getAnimationForGifAtUrl: firstUrl];
            
            gifAnimation.frame=CGRectMake(0, 0,cover_view.frame.size.width,cover_view.frame.size.height);
            [gifAnimation layoutSubviews];
            [cover_view addSubview:gifAnimation];
        }
    }
    
    if ((cover_img==nil)&&[mplayer isArchive]) {//archive mode, check tmp folder
        NSError *error;
        NSArray *dirContent;//
        BOOL isDir;
        NSString *file,*cpath;
        NSArray *filetype_ext=[SUPPORTED_FILETYPE_COVER componentsSeparatedByString:@","];
        NSFileManager *fileMngr=[[NSFileManager alloc] init];
        
        cpath=[NSString stringWithFormat:@"%@/tmp/tmpArchive",NSHomeDirectory()];
        dirContent=[fileMngr contentsOfDirectoryAtPath:cpath error:&error];
        for (file in dirContent) {
            [fileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
            if (!isDir) {
                NSString *extension = [[file pathExtension] uppercaseString];
                NSString *file_no_ext = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                if ([filetype_ext indexOfObject:extension]!=NSNotFound) {
                    cover_img=[UIImage imageWithContentsOfFile:[cpath stringByAppendingFormat:@"/%@",file]];
                    break;
                }
                else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) {
                    cover_img=[UIImage imageWithContentsOfFile:[cpath stringByAppendingFormat:@"/%@",file]];
                    break;
                }
                
            }
        }
        //[fileMngr release];
    }
    
    if (cover_img==nil) {    
        cover_img=[self fexGetArchiveCover:[self getFullFilePath:filePath]];
    }
    
    if (cover_img) {
        
        if (mScaleFactor!=1) cover_img = [[UIImage alloc] initWithCGImage:cover_img.CGImage scale:mScaleFactor orientation:UIImageOrientationUp];
        
        cover_view.image=cover_img;
        cover_view.hidden=FALSE;
        
        cover_viewBG.image=[cover_img applyLightEffect];
        cover_viewBG.hidden=FALSE;
    } else {
        cover_view.hidden=TRUE;
        cover_viewBG.hidden=TRUE;
    }
}

-(BOOL)play_module:(NSString *)filePath fname:(NSString *)fileName {
	short int playcount=0;
	int retcode;
    NSString *filePathTmp;
	
	mSendStatTimer=0;
    shouldRestart=0;
    
    if (!filePath) return FALSE;
    if (!fileName) return FALSE;
    
    // if already playing, stop
	if ([mplayer isPlaying]) {
		[repeatingTimer invalidate];
		repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
		[mplayer Stop];
	}
    mShouldUpdateInfos=1;
	// load module
    
    const char *tmp_str=[filePath UTF8String];
    char tmp_str_copy[1024];
    int found_arc=0;
    int arc_index=0;
    int found_sub=0;
    int sub_index=0;
    int i=0;
    mOnlyCurrentEntry=0;
    mOnlyCurrentSubEntry=0;
    while (tmp_str[i]) {
        if (found_arc) {
            arc_index=arc_index*10+tmp_str[i]-'0';
            mOnlyCurrentEntry=1;
        }
        if (found_sub) {
            sub_index=sub_index*10+tmp_str[i]-'0';
            mOnlyCurrentSubEntry=1;
        }
        if (tmp_str[i]=='@') {
            found_arc=1;
            memcpy(tmp_str_copy,tmp_str,i);
            tmp_str_copy[i]=0;
            filePathTmp=[NSString stringWithUTF8String:tmp_str_copy];
        }
        if (tmp_str[i]=='?') {
            found_sub=1;
            memcpy(tmp_str_copy,tmp_str,i);
            tmp_str_copy[i]=0;
            filePathTmp=[NSString stringWithUTF8String:tmp_str_copy];
        }
        i++;
    }
    if ((found_arc==0)&&(found_sub==0)) filePathTmp=[NSString stringWithString:filePath];
    
    if (mRestart) {
        
    } else {
        mRestart_arc=arc_index;
        mRestart_sub=sub_index;
    }
    
    //ensure any settings changes to be taken into account before loading next file
    [self settingsChanged:(int)SETTINGS_ALL];
    
    if (mShuffle) {
        mOnlyCurrentSubEntry=1;
        mOnlyCurrentEntry=1;
    }

	if ((retcode=[mplayer LoadModule:filePathTmp defaultMODPLAYER:settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value defaultSAPPLAYER:settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value defaultVGMPLAYER:settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value slowDevice:mSlowDevice archiveMode:0 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry])) {
		
        //error while loading
        //if it is an archive, try to load next entry until end or valid one reached
        if ( [mplayer isArchive] &&
            ([mplayer getArcIndex]<[mplayer getArcEntriesCnt]-1) &&
            !mOnlyCurrentSubEntry &&
            !mOnlyCurrentEntry ) {
        
            do {
                [mplayer selectNextArcEntry];
                if ([mplayer getArcIndex]>=[mplayer getArcEntriesCnt]) break;
                mRestart_arc=[mplayer getArcIndex];
                retcode=[mplayer LoadModule:filePathTmp defaultMODPLAYER:settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value defaultSAPPLAYER:settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value defaultVGMPLAYER:settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value slowDevice:mSlowDevice archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry];
            } while (retcode);
        }
        
        if (retcode) {
            NSLog(@"Issue in LoadModule %@",filePathTmp);
            mRestart=0;
            mRestart_sub=0;
            mRestart_arc=0;
            if (mplayer_error_msg[0]==0) sprintf(mplayer_error_msg,"%s",[filePathTmp UTF8String]);
            if (retcode==-99) mLoadIssueMessage=0;
            else mLoadIssueMessage=3;
            return FALSE;
        }
	}
    
    if (mShuffle) {
        if ([mplayer isArchive]) {
//            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            [mplayer Stop]; //deallocate relevant items
            mRestart_arc=arc4random()%[mplayer getArcEntriesCnt];

            if ((retcode=[mplayer LoadModule:filePath defaultMODPLAYER:settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value defaultSAPPLAYER:settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value defaultVGMPLAYER:settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value slowDevice:mSlowDevice archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry  singleArcMode:mOnlyCurrentEntry])) {
                //error while loading
                
                do {
                    [mplayer selectNextArcEntry];
                    if ([mplayer getArcIndex]>=[mplayer getArcEntriesCnt]) break;
                    mRestart_arc=[mplayer getArcIndex];
                    retcode=[mplayer LoadModule:filePathTmp defaultMODPLAYER:settings[GLOB_DefaultMODPlayer].detail.mdz_switch.switch_value defaultSAPPLAYER:settings[GLOB_DefaultSAPPlayer].detail.mdz_switch.switch_value defaultVGMPLAYER:settings[GLOB_DefaultVGMPlayer].detail.mdz_switch.switch_value slowDevice:mSlowDevice archiveMode:1 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry];
                } while (retcode);
                
                if (retcode) {
                    NSLog(@"Issue in LoadModule(archive) %@",filePath);
                    if (retcode==-99) mLoadIssueMessage=0;
                    else mLoadIssueMessage=4;
                    return FALSE;
                }
            }
            
//            [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
        }
    }
    
    
    
    //fix issue with modplug settings reset after load
    [self settingsChanged:(int)SETTINGS_ALL];
    
    [self checkForCover:filePathTmp];
    
    //[self checkAvailableCovers:mPlaylist_pos];
    mPlaylist[mPlaylist_pos].cover_flag=-1;
    
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
    
	if ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0)) btnShowSubSong.hidden=false;
	else btnShowSubSong.hidden=true;
    if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0)) btnShowArcList.hidden=false;
	else btnShowArcList.hidden=true;
    
	
	
	alertCannotPlay_displayed=0;
	//Visiulization stuff
	startChan=0;
	movePx=movePy=movePxOld=movePyOld=0;
    movePx2=movePy2=movePx2Old=movePy2Old=0;
    movePinchScale=movePinchScaleOld=0;
	sliderProgressModuleEdit=0;
	sliderProgressModuleChanged=0;
    
    
    
    //Is OGLView visible ?
    [self checkGLViewCanDisplay];
    
    //Restart management
    if (mRestart) {
        mRestart=0;
        self.pauseBarSub.hidden=YES;
        self.playBarSub.hidden=YES;
        self.pauseBar.hidden=YES;
        self.playBar.hidden=YES;
        
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.playBarSub.hidden=NO;
        else self.playBar.hidden=NO;
        
        [self updateBarPos];
        //		NSLog(@"yo %d",mRestart_sub);
        [mplayer PlaySeek:mPlayingPosRestart subsong:mRestart_sub];
        //		NSLog(@"yo1");
        if (mPlayingPosRestart) {
            mPlayingPosRestart=0;
        } else sliderProgressModule.value=0;
        //		NSLog(@"yo2");
        [mplayer Pause:YES];
        //		NSLog(@"yo3");
        mIsPlaying=YES;
        mPaused=1;
    } else {
        //random mode & mutli song ?
        if (mShuffle) {
            /*            if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)) {
             mOnlyCurrentSubEntry=1;
             }*/
            if ([mplayer isMultiSongs]) {
                mOnlyCurrentSubEntry=1;
                mRestart_sub=arc4random()%(mplayer.mod_subsongs)+mplayer.mod_minsub;
            }
        }
        
        self.pauseBarSub.hidden=YES;
        self.playBarSub.hidden=YES;
        self.pauseBar.hidden=YES;
        self.playBar.hidden=YES;
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) self.pauseBarSub.hidden=NO;
        else self.pauseBar.hidden=NO;
        [self updateBarPos];
        mRestart=0;
        [mplayer PlaySeek:mPlayingPosRestart subsong:mRestart_sub];
        if (mPlayingPosRestart) {
            mPlayingPosRestart=0;
        } else sliderProgressModule.value=0;
        mIsPlaying=YES;
        mPaused=0;
        
    }
    
    mRestart_sub=0;
    mRestart_arc=0;
    //set volume (if applicable)
    [mplayer setModPlugMasterVol:settings[MODPLUG_MasterVolume].detail.mdz_slider.slider_value];
    
    
    
    
    
	//Update song info if required
    labelModuleName.hidden=NO;
    if (settings[GLOB_TitleFilename].detail.mdz_boolswitch.switch_value) labelModuleName.text=[NSString stringWithString:fileName];
    else labelModuleName.text=[NSString stringWithString:[mplayer getModName]];
	lblCurrentSongCFlow.text=labelModuleName.text;
    labelModuleName.textColor = [UIColor colorWithRed:0.95 green:0.95 blue:0.99 alpha:1.0];
    labelModuleName.glowColor = [UIColor colorWithRed:0.40 green:0.40 blue:0.99 alpha:1.0];
    labelModuleName.glowOffset = CGSizeMake(0.0, 0.0);
    labelModuleName.glowAmount = 15.0;
    [labelModuleName setNeedsDisplay];
    self.navigationItem.titleView=labelModuleName;
    self.navigationItem.title=labelModuleName.text;
    
    
    
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
	
    if ([mplayer isArchive]) {  //Archive
        if (mOnlyCurrentEntry) { //Only one entry
            //Update rating info for playlist view
            mRating=0;
            DBHelper::getFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],filePath,&playcount,&mRating);
            mPlaylist[mPlaylist_pos].mPlaylistRating=mRating;
            
            //Not restarting app, update playcount
            if (!mRestart) playcount++;
            
            //Update archive entry stats
            DBHelper::updateFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],filePath,playcount,mRating,[mplayer getSongLength],mplayer.numChannels,-1);
            
        } else { //All entries
            //Update rating info for playlist view
            mRating=0;
            DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&mRating);
            
            //Not restarting app, update playcount
            if (!mRestart) playcount++;
            
            //Update Global file stats
            DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,mRating,[mplayer getGlobalLength],mplayer.numChannels,-[mplayer getArcEntriesCnt]);
            
            //Update 1st entry stats
            DBHelper::getFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],&playcount,&mRating);
            if (!mRestart) playcount++;
            DBHelper::updateFileStatsDBmod([mplayer getArcEntryTitle:[mplayer getArcIndex]],[NSString stringWithFormat:@"%@@%d",filePath,[mplayer getArcIndex]],playcount,mRating,[mplayer getSongLength],mplayer.numChannels,-1);
            
            mPlaylist[mPlaylist_pos].mPlaylistRating=mRating;
            
        }
        
    } else if ([mplayer isMultiSongs]) { //Multisubsongs
        if (mOnlyCurrentSubEntry) { // only one subsong
            //Update rating info for playlist view
            mRating=0;
            DBHelper::getFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],filePath,&playcount,&mRating);
            mPlaylist[mPlaylist_pos].mPlaylistRating=mRating;
            
            //Not restarting app, update playcount
            if (!mRestart) playcount++;
            
            //Update subsong stats
            DBHelper::updateFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],filePath,playcount,mRating,[mplayer getSongLength],mplayer.numChannels,-1);
        } else { // all subsongs
            //Update rating info for playlist view
            mRating=0;
            DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&mRating);
            
            //Not restarting app, update playcount
            if (!mRestart) playcount++;
            
            //Update Global file stats
            DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,mRating,[mplayer getGlobalLength],mplayer.numChannels,mplayer.mod_subsongs);
            
            //Update subsong stats
            DBHelper::getFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],[NSString stringWithFormat:@"%@?%d",filePath,mplayer.mod_currentsub],&playcount,&mRating);
            if (!mRestart) playcount++;
            DBHelper::updateFileStatsDBmod([mplayer getSubTitle:mplayer.mod_currentsub],[NSString stringWithFormat:@"%@?%d",filePath,mplayer.mod_currentsub],playcount,mRating,[mplayer getSongLength],mplayer.numChannels,-1);
            
            mPlaylist[mPlaylist_pos].mPlaylistRating=mRating;
            
        }
    } else { //No archive, no multisongs
        //Update rating info for playlist view
        mRating=0;
        DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&mRating);
        mPlaylist[mPlaylist_pos].mPlaylistRating=mRating;
        
        //Not restarting app, update playcount
        if (!mRestart) playcount++;
        
        //Update file stats
        DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,mRating,[mplayer getSongLength],mplayer.numChannels,-1);
        
    }
    
    if (!mRestart) {//UPDATE Google Application
		if (settings[GLOB_StatsUpload].detail.mdz_boolswitch.switch_value) mSendStatTimer=5*10;//Wait 5s
		
	}
	[self showRating:mRating];
	
	
    if (coverflow.hidden==NO) {
        btnPlayCFlow.hidden=YES;
        btnPauseCFlow.hidden=NO;
        btnPrevCFlow.hidden=NO;
        btnNextCFlow.hidden=NO;
        
        if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) {
            btnPrevSubCFlow.hidden=NO;
            btnNextSubCFlow.hidden=NO;
        } else {
            btnPrevSubCFlow.hidden=YES;
            btnNextSubCFlow.hidden=YES;
        }
    }
    
	if (settings[GLOB_CoverFlow].detail.mdz_boolswitch.switch_value) {
        if (coverflow.hidden==FALSE) {
            if (coverflow.numberOfCovers!=mPlaylist_size) [coverflow setNumberOfCovers:mPlaylist_size];
            if (coverflow.currentIndex!=mPlaylist_pos) {
                coverflow_pos=mPlaylist_pos;
                [coverflow setCurrentIndex:mPlaylist_pos];
                //[coverflow  bringCoverAtIndexToFront:mPlaylist_pos animated:YES];
            }
        } else coverflow_needredraw=1;
    }
    
	labelTime.text=@"00:00";
	if (mplayer.numChannels) {
		if (mplayer.numChannels==1) labelNumChannels.text=[NSString stringWithFormat:@"1 channel"];
		else labelNumChannels.text=[NSString stringWithFormat:@"%d channels",mplayer.numChannels];
	} else labelNumChannels.text=[NSString stringWithFormat:@""];
    
	labelModuleType.text=[NSString stringWithFormat:@"Format: %@",[mplayer getModType]];
	labelLibName.text=[NSString stringWithFormat:@"Player: %@",[mplayer getPlayerName]];
	textMessage.text=[NSString stringWithFormat:@"\n%@",[mplayer getModMessage]];
	
	[textMessage scrollRangeToVisible:NSMakeRange(0, 1)];
    
    
    MPNowPlayingInfoCenter *infoCenter = [MPNowPlayingInfoCenter defaultCenter];
    
    
    if (cover_img) artwork=[[MPMediaItemArtwork alloc] initWithImage:cover_img];
    else artwork=[[MPMediaItemArtwork alloc] initWithImage:default_cover];
    
    infoCenter.nowPlayingInfo = [NSDictionary dictionaryWithObjectsAndKeys:[NSString stringWithString:lblCurrentSongCFlow.text],
                                 MPMediaItemPropertyTitle,
                                 [NSString stringWithString:mPlaylist[mPlaylist_pos].mPlaylistFilepath],
                                 MPMediaItemPropertyArtist,
                                 [NSNumber numberWithFloat:(float)([mplayer getSongLength])/1000],
                                 MPMediaItemPropertyPlaybackDuration,
                                 [NSNumber numberWithFloat:(float)([mplayer getCurrentTime])/1000],
                                 MPNowPlayingInfoPropertyElapsedPlaybackTime,
                                 [NSNumber numberWithInt:1],
                                 MPNowPlayingInfoPropertyPlaybackRate,
                                 [NSNumber numberWithInt:mPlaylist_size],
                                 MPNowPlayingInfoPropertyPlaybackQueueCount,
                                 [NSNumber numberWithInt:mPlaylist_pos],
                                 MPNowPlayingInfoPropertyPlaybackQueueIndex,
                                 artwork,
                                 MPMediaItemPropertyArtwork,
                                 nil];

    
	//Activate timer for play infos
	repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.1f target:self selector:@selector(updateInfos:) userInfo:nil repeats: YES]; //10 times/second
    
    if (nowplayingPL) {
        NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
        [nowplayingPL.tableView reloadData];
        nowplayingPL.currentPlayedEntry=mPlaylist_pos;
        [nowplayingPL.tableView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos+1] animated:YES scrollPosition:UITableViewScrollPositionMiddle];
    }
	
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
    //[self shouldAutorotateToInterfaceOrientation:self.interfaceOrientation];
    return YES;
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    
    [super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
    //[self updateLayoutsForCurrentOrientation:toInterfaceOrientation view:self.navigationController.view.superview.superview];
    [self shouldAutorotateToInterfaceOrientation:toInterfaceOrientation];
    
    //NSLog(@"willAnimateRotationToInterfaceOrientation: %d",toInterfaceOrientation);
    
}

// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	orientationHV=interfaceOrientation;
    //NSLog(@"should rotate to : %d",orientationHV);
    
    if (eqVC) [eqVC shouldAutorotateToInterfaceOrientation:interfaceOrientation];
    
	if ((interfaceOrientation==UIInterfaceOrientationPortrait)||(interfaceOrientation==UIInterfaceOrientationPortraitUpsideDown)) {
//        waitingView.transform=CGAffineTransformMakeRotation(interfaceOrientation==UIInterfaceOrientationPortrait?0:M_PI);
        //waitingView.frame=CGRectMake(mDevice_ww/2-60,mDevice_hh/2-60,120,110);
        
        if (coverflow) {
        if (coverflow.hidden==FALSE) {
            
            [UIView beginAnimations:@"cflow_out" context:nil];
            [UIView setAnimationDelay:0.0];
            [UIView setAnimationDuration:0.4];
            [UIView setAnimationDelegate:self];
            coverflow.alpha=0.0;
            lblMainCoverflow.alpha=0;
            lblSecCoverflow.alpha=0;
            lblCurrentSongCFlow.alpha=0;
            lblTimeFCflow.alpha=0;
            btnPlayCFlow.alpha=0;
            btnPauseCFlow.alpha=0;
            btnBackCFlow.alpha=0;
            btnPrevCFlow.alpha=0;
            btnNextCFlow.alpha=0;
            btnPrevSubCFlow.alpha=0;
            btnNextSubCFlow.alpha=0;
            [UIView commitAnimations];
            [[self navigationController] setNavigationBarHidden:NO animated:NO];
        }
        }
		if (oglViewFullscreen) {
            if (mHasFocus) {
                [[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:UIStatusBarAnimationFade];
            }
            [self.navigationController setNavigationBarHidden:YES animated:YES];
			mainView.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh);
			m_oglView.frame = CGRectMake(0.0, 0.0, mDevice_ww, mDevice_hh);
            if (coverflow) coverflow.frame=CGRectMake(0,0,mDevice_hh,mDevice_ww);
			
		} else {
            if (mHasFocus) {
            [[UIApplication sharedApplication] setStatusBarHidden:NO withAnimation:UIStatusBarAnimationFade];
            }
            [self.navigationController setNavigationBarHidden:NO animated:YES];
            
            if (coverflow) coverflow.frame=CGRectMake(0,0,mDevice_hh,mDevice_ww-20);
            
			mainView.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh-20-42);
			m_oglView.frame = CGRectMake(0, 80, mDevice_ww, mDevice_hh-230-(mDeviceIPhoneX?32:0));
            cover_view.frame = CGRectMake(mDevice_ww/20, 80+mDevice_hh/20, mDevice_ww-mDevice_ww/10, mDevice_hh-230-mDevice_hh/10-(mDeviceIPhoneX?32:0));
            cover_viewBG.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh-230+80+44-(mDeviceIPhoneX?32:0));
            
            if (gifAnimation) gifAnimation.frame = CGRectMake(0, 0,cover_view.frame.size.width,cover_view.frame.size.height);
			oglButton.frame = CGRectMake(0, 80, mDevice_ww, mDevice_hh-230-(mDeviceIPhoneX?32:0));
            
            volWin.hidden=NO;
            volWin.frame= CGRectMake(0, mDevice_hh-64-42-(mDeviceIPhoneX?32:0), mDevice_ww, 44);
			volumeView.frame = CGRectMake(volWin.bounds.origin.x+12,volWin.bounds.origin.y+5,
                                          volWin.bounds.size.width-24,volWin.bounds.size.height); //volWin.bounds;
            //			volumeView.center = CGPointMake((mDevice_ww)/2,32);
            //			[volumeView sizeToFit];
			
			
			if (infoIsFullscreen) infoView.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh-20-42);
			else infoView.frame = CGRectMake(0, 80, mDevice_ww, mDevice_hh-230-(mDeviceIPhoneX?32:0));
			
			//commandViewU.frame = CGRectMake(2, 48, mDevice_ww-4, 32);
            commandViewU.frame = CGRectMake(0, 0, mDevice_ww, 32+48);
            
            buttonLoopTitleSel.frame = CGRectMake(10,0+48,32,32);
            buttonLoopList.frame = CGRectMake(10,0+48,32,32);
            buttonLoopListSel.frame = CGRectMake(10,0+48,32,32);
            buttonShuffle.frame = CGRectMake(50,0+48,32,32);
            buttonShuffleSel.frame = CGRectMake(50,0+48,32,32);
            btnLoopInf.frame = CGRectMake(88,-12+48,35,57);
            btnShowSubSong.frame = CGRectMake(mDevice_ww-36,0+48,32,32);
            btnShowArcList.frame = CGRectMake(mDevice_ww-36-36,0+48,32,32);
			
			mainRating1.frame = CGRectMake(130+2,3+48+4,20,20);
			mainRating2.frame = CGRectMake(130+24+2,3+48+4,20,20);
			mainRating3.frame = CGRectMake(130+24*2+2,3+48+4,20,20);
			mainRating4.frame = CGRectMake(130+24*3+2,3+48+4,20,20);
			mainRating5.frame = CGRectMake(130+24*4+2,3+48+4,20,20);
			mainRating1off.frame = CGRectMake(130+2,3+48+4,20,20);
			mainRating2off.frame = CGRectMake(130+24+2,3+48+4,20,20);
			mainRating3off.frame = CGRectMake(130+24*2+2,3+48+4,20,20);
			mainRating4off.frame = CGRectMake(130+24*3+2,3+48+4,20,20);
			mainRating5off.frame = CGRectMake(130+24*4+2,3+48+4,20,20);
			
			infoButton.frame = CGRectMake(mDevice_ww-40,4,36,36);
            eqButton.frame = CGRectMake(mDevice_ww-40-40,4,36,36);
			
			playlistPos.frame = CGRectMake(mDevice_ww/2-90-20,0,180,20);
			labelModuleLength.frame=CGRectMake(2,0,45,20);
			labelTime.frame=CGRectMake(2,24,45,20);
            btnChangeTime.frame=CGRectMake(2,24,45,20);
			sliderProgressModule.frame = CGRectMake(48,23-6,mDevice_ww-48-40-40-4,23);
		}
	} else{
//        waitingView.transform=CGAffineTransformMakeRotation(interfaceOrientation==UIInterfaceOrientationLandscapeLeft?-M_PI_2:M_PI_2);
        //waitingView.frame=CGRectMake(mDevice_hh/2-60,mDevice_ww/2-60,120,110);
        if ((mPlaylist_size>0)&&(settings[GLOB_CoverFlow].detail.mdz_boolswitch.switch_value)) {
            if (mHasFocus) {
                [[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:UIStatusBarAnimationFade];
            }
            [self.navigationController setNavigationBarHidden:YES animated:YES];
            if (coverflow) coverflow.frame=CGRectMake(0,0,mDevice_hh,mDevice_ww);
            
            //[coverflow setNumberOfCovers:mPlaylist_size];
            //coverflow.currentIndex=mPlaylist_pos;
            
            if (coverflow_needredraw||(coverflow_plsize!=mPlaylist_size)) {
                coverflow_plsize=mPlaylist_size;
                coverflow_pos=mPlaylist_pos;
                coverflow_needredraw=0;
                [coverflow setNumberOfCovers:mPlaylist_size pos:coverflow_pos];
            }
            if (coverflow.currentIndex!=mPlaylist_pos) {
                coverflow_pos=mPlaylist_pos;
                //coverflow.currentIndex=mPlaylist_pos;
                [coverflow setCurrentIndex:mPlaylist_pos];
            }
            
            coverflow.alpha=0;
            lblMainCoverflow.alpha=0;
            lblSecCoverflow.alpha=0;
            lblCurrentSongCFlow.alpha=0;
            lblTimeFCflow.alpha=0;
            btnPlayCFlow.alpha=0;
            btnPauseCFlow.alpha=0;
            btnBackCFlow.alpha=0;
            btnPrevCFlow.alpha=0;
            btnNextCFlow.alpha=0;
            btnPrevSubCFlow.alpha=0;
            btnNextSubCFlow.alpha=0;
            
            coverflow.hidden=FALSE;
            lblMainCoverflow.hidden=FALSE;
            lblSecCoverflow.hidden=FALSE;
            lblCurrentSongCFlow.hidden=FALSE;
            lblTimeFCflow.hidden=FALSE;
            btnBackCFlow.hidden=FALSE;
            btnPrevCFlow.hidden=NO;
            btnNextCFlow.hidden=NO;
            
            if (mPaused||(![mplayer isPlaying])) btnPlayCFlow.hidden=FALSE;
            else btnPauseCFlow.hidden=FALSE;
            
            if ( ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0))|| ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0))) {
                btnPrevSubCFlow.hidden=NO;
                btnNextSubCFlow.hidden=NO;
            } else {
                btnPrevSubCFlow.hidden=YES;
                btnNextSubCFlow.hidden=YES;
            }
            
            [[self navigationController] setNavigationBarHidden:YES animated:NO];
            
            [UIView beginAnimations:@"cflow_in" context:nil];
            [UIView setAnimationDelay:0.0];
            [UIView setAnimationDuration:0.40];
            //            [UIView setAnimationDelegate:self];
            coverflow.alpha=1.0;
            lblMainCoverflow.alpha=1.0;
            lblSecCoverflow.alpha=1.0;
            lblCurrentSongCFlow.alpha=1.0;
            lblTimeFCflow.alpha=1.0;
            btnPlayCFlow.alpha=1.0;
            btnPauseCFlow.alpha=1.0;
            btnBackCFlow.alpha=1.0;
            btnPrevCFlow.alpha=1.0;
            btnNextCFlow.alpha=1.0;
            btnPrevSubCFlow.alpha=1.0;
            btnNextSubCFlow.alpha=1.0;
            
            [UIView commitAnimations];
            
            
            
            if (mDeviceType==1) {
                lblMainCoverflow.frame=CGRectMake(0,mDevice_ww-40-64-64,mDevice_hh,40);
                lblSecCoverflow.frame=CGRectMake(40,mDevice_ww-40-24-64,mDevice_hh-80,24);
                
                lblCurrentSongCFlow.frame=CGRectMake(0,0,mDevice_hh*2/3,24);
                lblTimeFCflow.frame=CGRectMake(mDevice_hh*2/3,0,mDevice_hh/3,24);
                
                btnPrevCFlow.frame=CGRectMake((mDevice_hh-32)/2-160,mDevice_ww-22-32-16,32,32);
                btnPrevSubCFlow.frame=CGRectMake((mDevice_hh-32)/2-80,mDevice_ww-22-32-2-16,32,32);
                btnPlayCFlow.frame=CGRectMake((mDevice_hh-32)/2,mDevice_ww-22-32-16,32,32);
                btnPauseCFlow.frame=CGRectMake((mDevice_hh-32)/2,mDevice_ww-22-32-16,32,32);
                btnNextSubCFlow.frame=CGRectMake((mDevice_hh-32)/2+80,mDevice_ww-22-32-2-16,32,32);
                btnNextCFlow.frame=CGRectMake((mDevice_hh-32)/2+160,mDevice_ww-22-32-16,32,32);
                
                
                btnBackCFlow.frame=CGRectMake(8,32,32,32);
            } else {
                lblMainCoverflow.frame=CGRectMake(0,mDevice_ww-40-32-12,mDevice_hh,20);
                lblSecCoverflow.frame=CGRectMake(40,mDevice_ww-40-12-12,mDevice_hh-80,12);
                
                lblCurrentSongCFlow.frame=CGRectMake(0,0,mDevice_hh*2/3,12);
                lblTimeFCflow.frame=CGRectMake(mDevice_hh*2/3,0,mDevice_hh/3,12);
                
                btnPrevCFlow.frame=CGRectMake((mDevice_hh-32)/2-160,mDevice_ww-22-24,32,32);
                btnPrevSubCFlow.frame=CGRectMake((mDevice_hh-32)/2-80,mDevice_ww-22-24-2,32,32);
                btnPlayCFlow.frame=CGRectMake((mDevice_hh-32)/2,mDevice_ww-22-24,32,32);
                btnPauseCFlow.frame=CGRectMake((mDevice_hh-32)/2,mDevice_ww-22-24,32,32);
                btnNextSubCFlow.frame=CGRectMake((mDevice_hh-32)/2+80,mDevice_ww-22-24-2,32,32);
                btnNextCFlow.frame=CGRectMake((mDevice_hh-32)/2+160,mDevice_ww-22-24,32,32);
                
                btnBackCFlow.frame=CGRectMake(8,32,32,32);
            }
            
            lblMainCoverflow.hidden=FALSE;
            lblSecCoverflow.hidden=FALSE;
            lblCurrentSongCFlow.hidden=FALSE;
            lblTimeFCflow.hidden=FALSE;
            btnBackCFlow.hidden=FALSE;
            if (mPaused||(![mplayer isPlaying])) {
                btnPlayCFlow.hidden=FALSE;
                btnPauseCFlow.hidden=TRUE;
            }
            else {
                btnPauseCFlow.hidden=FALSE;
                btnPlayCFlow.hidden=TRUE;
            }
            
            
        } else {
            
            if (settings[GLOB_CoverFlow].detail.mdz_boolswitch.switch_value==FALSE) {
                coverflow.alpha=0;
                lblMainCoverflow.alpha=0;
                lblSecCoverflow.alpha=0;
                lblCurrentSongCFlow.alpha=0;
                lblTimeFCflow.alpha=0;
                btnPlayCFlow.alpha=0;
                btnPauseCFlow.alpha=0;
                btnBackCFlow.alpha=0;
                
                coverflow.hidden=TRUE;
                lblMainCoverflow.hidden=TRUE;
                lblSecCoverflow.hidden=TRUE;
                lblCurrentSongCFlow.hidden=TRUE;
                lblTimeFCflow.hidden=TRUE;
                btnPlayCFlow.hidden=TRUE;
                btnPauseCFlow.hidden=TRUE;
                btnBackCFlow.hidden=TRUE;
            }
            
            
            if (oglViewFullscreen) {
                if (mHasFocus) {
                [[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:UIStatusBarAnimationFade];
                }
                [self.navigationController setNavigationBarHidden:YES animated:YES];
                
                
                mainView.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww);
                m_oglView.frame = CGRectMake(0.0, 0.0, mDevice_hh, mDevice_ww);  //ipad
                if (coverflow) coverflow.frame=CGRectMake(0,0,mDevice_hh,mDevice_ww);
                
            } else {
                if (mHasFocus) {
                    [[UIApplication sharedApplication] setStatusBarHidden:NO withAnimation:UIStatusBarAnimationFade];
                }
                [self.navigationController setNavigationBarHidden:NO animated:YES];
                
                if (coverflow) coverflow.frame=CGRectMake(0,0,mDevice_hh,mDevice_ww-20);
                
                mainView.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww-30);
                m_oglView.frame = CGRectMake(0.0, 82, mDevice_hh, mDevice_ww-82-30);
                cover_view.frame = CGRectMake(0.0+mDevice_hh/20, 82+mDevice_ww/20, mDevice_hh-mDevice_hh/10, mDevice_ww-82-30-mDevice_ww/10);
                cover_viewBG.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww-82-30+82);
                if (gifAnimation) {
                    //NSLog(@"3: %f %f",cover_view.frame.size.width,cover_view.frame.size.height);
                    gifAnimation.frame = CGRectMake(0, 0,cover_view.frame.size.width,cover_view.frame.size.height);
                }

                oglButton.frame = CGRectMake(0.0, 82, mDevice_hh, mDevice_ww-82-30);
                
                //volWin.frame= CGRectMake(200, 40, mDevice_hh-375, 44);
                volWin.hidden=YES;
                
                //volumeView.frame = CGRectMake(volWin.bounds.origin.x+12,volWin.bounds.origin.y,
               //                               volWin.bounds.size.width-24,volWin.bounds.size.height); //volWin.bounds;
                //                volumeView.frame = CGRectMake(10, 0, mDevice_hh-375-10, 44);
                //              volumeView.center = CGPointMake((mDevice_hh-375)/2,32);
                //            [volumeView sizeToFit];
                
                
                if (infoIsFullscreen) infoView.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww-20-30);
                else infoView.frame = CGRectMake(0.0, 82, mDevice_hh, mDevice_ww-82-30);
                
                int xofs=mDevice_hh-(24*5+4+32+4+32+8);
                int yofs=10;
                //commandViewU.frame = CGRectMake(mDevice_hh-72-40-31-20-4, 8, 40+72+31+20, 32+32);
                commandViewU.frame = CGRectMake(0, 0, mDevice_hh, 32+44+8);
                
				buttonLoopTitleSel.frame = CGRectMake(xofs+2,yofs+0,40,32);
				buttonLoopList.frame = CGRectMake(xofs+2,yofs+0,40,32);
				buttonLoopListSel.frame = CGRectMake(xofs+2,yofs+0,40,32);
				buttonShuffle.frame = CGRectMake(xofs+42,yofs+0,40,32);
				buttonShuffleSel.frame = CGRectMake(xofs+42,yofs+0,40,32);
				btnLoopInf.frame = CGRectMake(xofs+80,yofs+-12,35,57);
                
                mainRating1.frame = CGRectMake(xofs+6+2,yofs+42+2,20,20);
                mainRating2.frame = CGRectMake(xofs+6+24+2,yofs+42+2,20,20);
                mainRating3.frame = CGRectMake(xofs+6+24*2+2,yofs+42+2,20,20);
                mainRating4.frame = CGRectMake(xofs+6+24*3+2,yofs+42+2,20,20);
                mainRating5.frame = CGRectMake(xofs+6+24*4+2,yofs+42+2,20,20);
                mainRating1off.frame = CGRectMake(xofs+6+2,yofs+42+2,20,20);
                mainRating2off.frame = CGRectMake(xofs+6+24+2,yofs+42+2,20,20);
                mainRating3off.frame = CGRectMake(xofs+6+24*2+2,yofs+42+2,20,20);
                mainRating4off.frame = CGRectMake(xofs+6+24*3+2,yofs+42+2,20,20);
                mainRating5off.frame = CGRectMake(xofs+6+24*4+2,yofs+42+2,20,20);
                
                
                btnShowSubSong.frame = CGRectMake(xofs+6+24*5+4,yofs+40,32,32);
                btnShowArcList.frame = CGRectMake(xofs+6+24*5+4+32+4,yofs+40,32,32);
                
                
                //infoButton.frame = CGRectMake(mDevice_hh-200-10,1,38,38);
                infoButton.frame = CGRectMake(mDevice_hh-44,4,40,40);
                eqButton.frame = CGRectMake(mDevice_hh-44-44,4,40,40);
                
                playlistPos.frame = CGRectMake((mDevice_hh-200)/2-90,0,180,20);
                
                labelModuleLength.frame=CGRectMake(2,0,45,20);
                labelTime.frame=CGRectMake(2,20,45,20);
                btnChangeTime.frame=CGRectMake(2,17,45,20);
                
                sliderProgressModule.frame = CGRectMake(48,16-3,mDevice_hh-200-60,23);
            }
        }
	}
	[self updateBarPos];
	int size_chan;
	switch (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value) {
        case 1:
        case 4:
            size_chan=12*6;
            break;
        case 2:
        case 5:
            size_chan=6*6;
            break;
        case 3:
        case 6:
            size_chan=4*6;
            break;
    }
	visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN+size_chan-1)/size_chan;
	if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
	if (startChan<0) startChan=0;
    tim_midifx_note_range=88;// //128notes max
    if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==2) {
        tim_midifx_note_offset=(128-88)/2*m_oglView.frame.size.width/tim_midifx_note_range;
    } else {
        tim_midifx_note_offset=(128-88)/2*m_oglView.frame.size.height/tim_midifx_note_range;
    }

	
	return YES;
}

-(void)updateBarPos {
	if ((orientationHV==UIInterfaceOrientationPortrait)||(orientationHV==UIInterfaceOrientationPortraitUpsideDown)) {
		playBar.frame =  CGRectMake(0, mDevice_hh-(playBar.hidden?0:108+42)-(mDeviceIPhoneX?32:0), mDevice_ww, 44);
		pauseBar.frame =  CGRectMake(0, mDevice_hh-(pauseBar.hidden?0:108+42)-(mDeviceIPhoneX?32:0), mDevice_ww, 44);
		playBarSub.frame =  CGRectMake(0, mDevice_hh-(playBarSub.hidden?0:108+42)-(mDeviceIPhoneX?32:0), mDevice_ww, 44);
		pauseBarSub.frame =  CGRectMake(0, mDevice_hh-(pauseBarSub.hidden?0:108+42)-(mDeviceIPhoneX?32:0), mDevice_ww, 44);
	} else {
        int xofs=24*5+32*2+10;
		playBar.frame = CGRectMake(0, 40, mDevice_hh-xofs, 44); //mDevice_hh-(playBar.hidden?0:375)
		pauseBar.frame = CGRectMake(0, 40, mDevice_hh-xofs, 44);
		playBarSub.frame =  CGRectMake(0, 40, mDevice_hh-xofs, 44);
		pauseBarSub.frame =  CGRectMake(0, 40, mDevice_hh-xofs, 44);
	}
}



#pragma mark -
#pragma mark View lifecycle



/**************************************************/
/**************************************************/
/**************************************************/
/* User Defined Variables */
GLfloat angle;           /* Used To Rotate The Helix */
GLfloat vertices[4][3];  /* Holds Float Info For 4 Sets Of Vertices */
GLfloat vertColor[4][4];  /* Holds Float Info For 4 Sets Of Vertices */
GLfloat texcoords[4][2]; /* Holds Float Info For 4 Sets Of Texture coordinates. */
GLfloat normalData[3];       /* An Array To Store The Normal Data */
//GLuint  BlurTexture,FxTexture;     /* An Unsigned Int To Store The Texture Number */

/* Create Storage Space For Texture Data (128x128x4) */


/* Create An Empty Texture */
GLuint EmptyTexture(int ww,int hh)
{
    GLuint txtnumber;           /* Texture ID  */
    unsigned int *dataText;         /* Stored Data */
	
	dataText=(unsigned int*)malloc(ww*hh*4*sizeof(unsigned int));
    
    glGenTextures(1, &txtnumber);               /* Create 1 Texture */
    glBindTexture(GL_TEXTURE_2D, txtnumber);    /* Bind The Texture */
	
    /* Build Texture Using Information In data */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ww, hh,
				 0, GL_RGBA, GL_UNSIGNED_BYTE, dataText);
	
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterx(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	free(dataText);
    /* Return The Texture ID */
    return txtnumber;
}



/* Reduces A Normal Vector (3 Coordinates)       */
/* To A Unit Normal Vector With A Length Of One. */
void ReduceToUnit(GLfloat vector[3])
{
    /* Holds Unit Length */
    GLfloat length;
	
    /* Calculates The Length Of The Vector */
    length=(GLfloat)sqrt((vector[0]*vector[0])+(vector[1]*vector[1])+(vector[2]*vector[2]));
	
    /* Prevents Divide By 0 Error By Providing */
    if (length==0.0f)
    {
        /* An Acceptable Value For Vectors To Close To 0. */
        length=1.0f;
    }
	
    vector[0]/=length;  /* Dividing Each Element By */
    vector[1]/=length;  /* The Length Results In A  */
    vector[2]/=length;  /* Unit Normal Vector.      */
}

/* Calculates Normal For A Quad Using 3 Points */
void calcNormal(GLfloat v[3][3], GLfloat out[3])
{
    /* Vector 1 (x,y,z) & Vector 2 (x,y,z) */
    GLfloat v1[3], v2[3];
    /* Define X Coord */
    static const int x=0;
    /* Define Y Coord */
    static const int y=1;
    /* Define Z Coord */
    static const int z=2;
	
    /* Finds The Vector Between 2 Points By Subtracting */
    /* The x,y,z Coordinates From One Point To Another. */
	
    /* Calculate The Vector From Point 1 To Point 0 */
    v1[x]=v[0][x]-v[1][x];      /* Vector 1.x=Vertex[0].x-Vertex[1].x */
    v1[y]=v[0][y]-v[1][y];      /* Vector 1.y=Vertex[0].y-Vertex[1].y */
    v1[z]=v[0][z]-v[1][z];      /* Vector 1.z=Vertex[0].y-Vertex[1].z */
	
    /* Calculate The Vector From Point 2 To Point 1 */
    v2[x]=v[1][x]-v[2][x];      /* Vector 2.x=Vertex[0].x-Vertex[1].x */
    v2[y]=v[1][y]-v[2][y];      /* Vector 2.y=Vertex[0].y-Vertex[1].y */
    v2[z]=v[1][z]-v[2][z];      /* Vector 2.z=Vertex[0].z-Vertex[1].z */
	
    /* Compute The Cross Product To Give Us A Surface Normal */
    out[x]=v1[y]*v2[z]-v1[z]*v2[y];     /* Cross Product For Y - Z */
    out[y]=v1[z]*v2[x]-v1[x]*v2[z];     /* Cross Product For X - Z */
    out[z]=v1[x]*v2[y]-v1[y]*v2[x];     /* Cross Product For X - Y */
	
    ReduceToUnit(out);          /* Normalize The Vectors */
}


void ProcessSpectrum(int ww,int hh,short int *spectrumDataL,short int *spectrumDataR,int numval)
{
    GLfloat x,x1,x2;
    GLfloat y,y1,y2;
    GLfloat z;
    GLfloat phi;        /* Angle */
    GLfloat v;       /* Angles */
    GLfloat r,cr,cg,cb,rf,cmax;
    
    //////////////////////////////
	glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	const float aspectRatio = (float)ww/(float)hh;
	const float _hw = 0.1f;
	const float _hh = _hw/aspectRatio;
	glFrustumf(-_hw, _hw, -_hh, _hh, 0.4f, 100.0f);
	
    glPushMatrix();                     /* Push The Modelview Matrix */
	
    glTranslatef(0.0, 0.0, -50.0);      /* Translate 50 Units Into The Screen */
    //glRotatef(30.0f*sin(angle*0.11f*3.1459f/180.0f), 1, 0 ,0);
    //glRotatef(20.0f*sin(angle*0.17f*3.1459f/180.0f), 0, 1 ,0);
	glRotatef(angle/20.0f, 0, 0, 1);
	
  	
    /* Begin Drawing Quads, setup vertex array pointer */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
	glColorPointer(4, GL_FLOAT, 0, vertColor);
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
    /* 360 Degrees In Steps Of 20 */
    for (int i=0; i<numval*2; i++)
    {
		if (i<numval) {
			r=(float)spectrumDataL[i]/512.0f;
			cr=(float)i/(float)numval*2+0.2f;
			cg=(float)i/(float)numval;
			cb=(float)i/(float)numval/3;
		}
		else {
			r=(float)spectrumDataR[(2*numval-1-i)]/512.0f;
			cg=(float)(i-numval)/(float)numval*2+0.2f;
			cr=(float)(i-numval)/(float)numval;
			cb=(float)(i-numval)/(float)numval/3;
		}
		cr=cr*r/4.0f;
		cg=cg*r/4.0f;
		cb=cb*r/4.0f;
		cmax=r/4.0f;
		if (cr<0) cr=0; if (cr>1.0f) cr=1.0f;
		if (cg<0) cg=0; if (cg>1.0f) cg=1.0f;
		if (cb<0) cb=0; if (cb>1.0f) cb=1.0f;
		
		
		vertColor[0][3]=vertColor[1][3]=vertColor[2][3]=1.0f;
		
		phi=(float)i*360.0f/(float)numval/2;
		/* Calculate Angle Of First Point (0) */
		v=((phi)/180.0f*3.142f);
		
		rf=1.0f+0.4*sin(angle*0.001f)*cos(v*(8+8*sin(angle*0.0008f))+angle*0.0010f);
		
		/* Calculate x Position (1st Point) */
        x1=(GLfloat)(cos(v)*(r+rf));
		y1=(GLfloat)(sin(v)*(r+rf));
		x2=(GLfloat)(cos(v)*(rf));
		y2=(GLfloat)(sin(v)*(rf));
		z=0.0f;
		
		x=x1+(GLfloat)(sin(v)*0.8f);
		y=y1-(GLfloat)(cos(v)*0.8f);
		vertColor[0][0]=cr;vertColor[0][1]=cg;vertColor[0][2]=cb;
		vertices[0][0]=x;   /* Set x Value Of First Vertex */
        vertices[0][1]=y;   /* Set y Value Of First Vertex */
        vertices[0][2]=z-1.0f;   /* Set z Value Of First Vertex */
		
		x=x2;
		y=y2;
		vertColor[1][0]=vertColor[1][1]=vertColor[1][2]=cmax;
		vertices[1][0]=x;   /* Set x Value Of Second Vertex */
        vertices[1][1]=y;   /* Set y Value Of Second Vertex */
        vertices[1][2]=z;   /* Set z Value Of Second Vertex */
		
		x=x1-(GLfloat)(sin(v)*0.8f);
		y=y1+(GLfloat)(cos(v)*0.8f);
		vertColor[2][0]=cr;vertColor[2][1]=cg;vertColor[2][2]=cb;
		vertices[2][0]=x;   /* Set x Value Of First Vertex */
        vertices[2][1]=y;   /* Set y Value Of First Vertex */
        vertices[2][2]=z-1.0f;   /* Set z Value Of First Vertex */
        calcNormal(vertices, normalData); /* Calculate The Quad Normal */
        /* Set The Normal */
        glNormal3f(normalData[0], normalData[1], normalData[2]);
        
        /* Render The Quad */
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
		
		//same, but smaller
        x1=(GLfloat)(cos(v)*(r*0.9f+rf+0.1f));
		y1=(GLfloat)(sin(v)*(r*0.9f+rf+0.1f));
		x2=(GLfloat)(cos(v)*(rf+0.1f));
		y2=(GLfloat)(sin(v)*(rf+0.1f));
		z=0.0f;
		
		
		x=x1+(GLfloat)(sin(v)*0.6f);
		y=y1-(GLfloat)(cos(v)*0.6f);
		vertColor[0][0]=vertColor[0][1]=vertColor[0][2]=cmax;
		vertices[0][0]=x;   /* Set x Value Of First Vertex */
        vertices[0][1]=y;   /* Set y Value Of First Vertex */
        vertices[0][2]=z-0.9f;   /* Set z Value Of First Vertex */
		
		x=x2;
		y=y2;
		vertColor[1][0]=cr;vertColor[1][1]=cg;vertColor[1][2]=cb;
		vertices[1][0]=x;   /* Set x Value Of Second Vertex */
        vertices[1][1]=y;   /* Set y Value Of Second Vertex */
        vertices[1][2]=z;   /* Set z Value Of Second Vertex */
		
		x=x1-(GLfloat)(sin(v)*0.2f);
		y=y1+(GLfloat)(cos(v)*0.2f);
		vertColor[2][0]=vertColor[2][1]=vertColor[2][2]=cmax;
		vertices[2][0]=x;   /* Set x Value Of First Vertex */
        vertices[2][1]=y;   /* Set y Value Of First Vertex */
        vertices[2][2]=z-0.9f;   /* Set z Value Of First Vertex */
        calcNormal(vertices, normalData); /* Calculate The Quad Normal */
        /* Set The Normal */
        glNormal3f(normalData[0], normalData[1], normalData[2]);
		
		/* Render The Quad */
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
		
	}
	
	
	
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	
    /* Pop The Matrix */
    glPopMatrix();
}


/* Set Up An Ortho View */
void ViewOrtho(GLfloat window_width, GLfloat window_height)
{
    glMatrixMode(GL_PROJECTION);        /* Select Projection */
    glPushMatrix();                     /* Push The Matrix   */
    glLoadIdentity();                   /* Reset The Matrix  */
    /* Select Ortho Mode (window_width x window_height)      */
    glOrthof(0, window_width, window_height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);         /* Select Modelview Matrix */
    glPushMatrix();                     /* Push The Matrix   */
    glLoadIdentity();                   /* Reset The Matrix  */
}

/* Set Up A Perspective View */
void ViewPerspective()
{
    glMatrixMode(GL_PROJECTION);        /* Select Projection */
    glPopMatrix();                      /* Pop The Matrix    */
    glMatrixMode(GL_MODELVIEW);         /* Select Modelview  */
    glPopMatrix();                      /* Pop The Matrix    */
}

void infoMenuShowImages(int window_width,int window_height,int alpha_byte ) {
    glEnable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glDisable(GL_DEPTH_TEST);           /* Disable Depth Testing     */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  /* Set Blending Mode         */
    glEnable(GL_BLEND);                 /* Enable Blending           */
	
    /* Bind To The Blur Texture */
    
	
    /* Switch To An Ortho View   */
    int menu_cell_size;
    if (window_width<window_height) {
        menu_cell_size=window_width;
    } else {
        menu_cell_size=window_height;
    }
    ViewOrtho(window_width, window_height);
	
	
    /* Begin Drawing Quads, setup vertex and texcoord array pointers */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    /* Enable Texture Coordinations Pointer */
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4f(1.0f, 1.0f, 1.0f, alpha_byte*1.0f/255.0f);
	texcoords[0][0]=0.0f; texcoords[0][1]=0.0f;
	texcoords[1][0]=0.0f; texcoords[1][1]=1.0f;
	texcoords[2][0]=1.0f; texcoords[2][1]=0.0f;
	texcoords[3][0]=1.0f; texcoords[3][1]=1.0f;
	
	
	int marg=4;
    for (int i=0;i<4;i++)
        for (int j=0;j<4;j++) {
            if (txtMenuHandle[i*4+j]) {
                glBindTexture(GL_TEXTURE_2D, txtMenuHandle[i*4+j]);
                vertices[0][0]=menu_cell_size*j/4+marg; vertices[0][1]=menu_cell_size*i/4+marg+(window_height-menu_cell_size)/2;
                vertices[0][2]=0.0f;
                vertices[1][0]=menu_cell_size*j/4+marg; vertices[1][1]=menu_cell_size*(i+1)/4-marg+(window_height-menu_cell_size)/2;
                vertices[1][2]=0.0f;
                vertices[2][0]=menu_cell_size*(j+1)/4-marg; vertices[2][1]=menu_cell_size*i/4+marg+(window_height-menu_cell_size)/2;
                vertices[2][2]=0.0f;
                vertices[3][0]=menu_cell_size*(j+1)/4-marg; vertices[3][1]=menu_cell_size*(i+1)/4-marg+(window_height-menu_cell_size)/2;
                vertices[3][2]=0.0f;
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    /* Disable Texture Coordinations Pointer */
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
    /* Switch To A Perspective View */
    ViewPerspective();
	
    /* Enable Depth Testing */
    glEnable(GL_DEPTH_TEST);
    /* Disable 2D Texture Mapping */
    glDisable(GL_TEXTURE_2D);
    /* Disable Blending */
    glDisable(GL_BLEND);
    /* Unbind The Blur Texture */
    glBindTexture(GL_TEXTURE_2D, 0);
}

void infoSubMenuShowImages(int window_width,int window_height,int start_index,int nb,int alpha_byte ) {
    glEnable(GL_TEXTURE_2D);            /* Enable 2D Texture Mapping */
    glDisable(GL_DEPTH_TEST);           /* Disable Depth Testing     */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  /* Set Blending Mode         */
    glEnable(GL_BLEND);                 /* Enable Blending           */
	
    /* Bind To The Blur Texture */
    
	
    /* Switch To An Ortho View   */
    ViewOrtho(window_width, window_height);
    
    int menu_cell_size;
    if (window_width<window_height) {
        menu_cell_size=window_width;
    } else {
        menu_cell_size=window_height;
    }
    
	
	
    /* Begin Drawing Quads, setup vertex and texcoord array pointers */
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texcoords);
    /* Enable Vertex Pointer */
    glEnableClientState(GL_VERTEX_ARRAY);
    /* Enable Texture Coordinations Pointer */
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glColor4f(1.0f, 1.0f, 1.0f, alpha_byte*1.0f/255.0f);
	texcoords[0][0]=0.0f; texcoords[0][1]=0.0f;
	texcoords[1][0]=0.0f; texcoords[1][1]=1.0f;
	texcoords[2][0]=1.0f; texcoords[2][1]=0.0f;
	texcoords[3][0]=1.0f; texcoords[3][1]=1.0f;
	
	
	int marg=4;
    int idx=start_index;
    for (int i=0;(i<4)&&(idx<start_index+nb);i++) {
        for (int j=0;(j<4)&&(idx<start_index+nb);j++) {
            if (txtSubMenuHandle[idx]) {
                glBindTexture(GL_TEXTURE_2D, txtSubMenuHandle[idx]);
                vertices[0][0]=menu_cell_size*j/4+marg; vertices[0][1]=menu_cell_size*i/4+marg+(window_height-menu_cell_size)/2;
                vertices[0][2]=0.0f;
                vertices[1][0]=menu_cell_size*j/4+marg; vertices[1][1]=menu_cell_size*(i+1)/4-marg+(window_height-menu_cell_size)/2;
                vertices[1][2]=0.0f;
                vertices[2][0]=menu_cell_size*(j+1)/4-marg; vertices[2][1]=menu_cell_size*i/4+marg+(window_height-menu_cell_size)/2;
                vertices[2][2]=0.0f;
                vertices[3][0]=menu_cell_size*(j+1)/4-marg; vertices[3][1]=menu_cell_size*(i+1)/4-marg+(window_height-menu_cell_size)/2;
                vertices[3][2]=0.0f;
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            idx++;
        }
    }
    /* Disable Vertex Pointer */
    glDisableClientState(GL_VERTEX_ARRAY);
    /* Disable Texture Coordinations Pointer */
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
    /* Switch To A Perspective View */
    ViewPerspective();
	
    /* Enable Depth Testing */
    glEnable(GL_DEPTH_TEST);
    /* Disable 2D Texture Mapping */
    glDisable(GL_TEXTURE_2D);
    /* Disable Blending */
    glDisable(GL_BLEND);
    /* Unbind The Blur Texture */
    glBindTexture(GL_TEXTURE_2D, 0);
}


void fxRadial(int fxtype,int _ww,int _hh,short int *spectrumDataL,short int *spectrumDataR,int numval,float alphaval) {
	/* Set The Clear Color To Black */
    glClearColor(0.0f, 0.0f, 0.0f, alphaval);
    /* Clear Screen And Depth Buffer */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    /* Reset The View */
    glLoadIdentity();
	
	switch (fxtype) {
		case 0:
			ProcessSpectrum(_ww,_hh,spectrumDataL,spectrumDataR,numval);
			break;
            /*case 1:
             DrawBlur1(0.95f,0.009f, _ww, _hh);
             ProcessSpectrum(_ww,_hh,spectrumDataL,spectrumDataR,numval);
             glBindTexture(GL_TEXTURE_2D, FxTexture);
             glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 512, 512, 0);
             glBindTexture(GL_TEXTURE_2D, 0);
             break;*/
		case 1:
			/*RenderToTexture(_ww, _hh,spectrumDataL,spectrumDataR,numval);
             ProcessSpectrum(_ww,_hh,spectrumDataL,spectrumDataR,numval);
             DrawBlur(8, 0.04f, _ww, _hh);*/
			break;
	}
	
    
    glFlush();
}

/**************************************************/
/**************************************************/
/**************************************************/

-(void)updateFlagOnExit{
	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
	
	valNb=[[NSNumber alloc] initWithInt:0];
	[prefs setObject:valNb forKey:@"ModizerRunning"];
    
    [prefs synchronize];
    //[valNb autorelease];
}

//return 1 if flag is not ok
-(int)checkFlagOnStartup{
	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    int retcode=0;
	
	valNb=[prefs objectForKey:@"ModizerRunning"];if (DEBUG_NO_SETTINGS) valNb=nil;
	if (valNb == nil) retcode=1;
	else if ([valNb intValue]!=0) retcode=1;
	
	valNb=[[NSNumber alloc] initWithInt:1];
	[prefs setObject:valNb forKey:@"ModizerRunning"];
	
	return retcode;
}

-(void) updateAllSettingsAfterChange {
    /////////////////////////////////////
	//update according to settings
	if (oglViewFullscreen) {
		oglViewFullscreen=0;
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
    if (mShuffle) mShuffle=NO;
    else mShuffle=YES;
	[self shuffle];
	
	//update settings according toi what was loaded
    
    [self settingsChanged:(int)SETTINGS_ALL];
    
}

-(void)loadSettings:(int)safe_mode{
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
	int not_expected_version;
    
    not_expected_version=0;
	valNb=[prefs objectForKey:@"VERSION_MAJOR"];if (safe_mode) valNb=nil;
	if (valNb == nil) {
		//should not happen on released version
		not_expected_version=0;
	} else {
		if ([valNb intValue]!=VERSION_MAJOR) {
			not_expected_version=1;
		}
	}
	valNb=[prefs objectForKey:@"VERSION_MINOR"];if (safe_mode) valNb=nil;
	if (valNb == nil) {
		//should not happen on released version
		not_expected_version=0;
	} else {
		if ([valNb intValue]!=VERSION_MINOR) {
			not_expected_version=1;
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
	if (valNb == nil) oglViewFullscreen = 0;
	else oglViewFullscreen = [valNb intValue];
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
	if (mPlaylist_size) {
		NSMutableArray *fileNames,*filePaths;
		
		fileNames=[prefs objectForKey:@"PlaylistEntries_Names"];if (safe_mode) fileNames=nil;
		filePaths=[prefs objectForKey:@"PlaylistEntries_Paths"];if (safe_mode) filePaths=nil;
		
		if (filePaths&&fileNames&&(mPlaylist_size==[filePaths count])&&(mPlaylist_size==[fileNames count])) {
			for (int i=0;i<mPlaylist_size;i++) {
				mPlaylist[i].mPlaylistFilename=[[NSString alloc] initWithString:[fileNames objectAtIndex:i]];
				mPlaylist[i].mPlaylistFilepath=[[NSString alloc] initWithString:[filePaths objectAtIndex:i]];
			}
		}
	}    
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
    
    [self updateAllSettingsAfterChange];
}

-(void)saveSettings{
	NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
    
    valNb=[[NSNumber alloc] initWithInt:VERSION_MAJOR];
	[prefs setObject:valNb forKey:@"VERSION_MAJOR"];
	valNb=[[NSNumber alloc] initWithInt:VERSION_MINOR];
	[prefs setObject:valNb forKey:@"VERSION_MINOR"];
    
    ///////////////////////////////////
    // Internal stuff
    ///////////////////////////////////////
    valNb=[[NSNumber alloc] initWithInt:(int)mOglViewIsHidden];
	[prefs setObject:valNb forKey:@"ViewFX"];
	
    valNb=[[NSNumber alloc] initWithInt:infoIsFullscreen];
	[prefs setObject:valNb forKey:@"InfoFullscreen"];
	valNb=[[NSNumber alloc] initWithInt:oglViewFullscreen];
	[prefs setObject:valNb forKey:@"OGLFullscreen"];
    
	
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
		
		NSMutableArray *fileNames,*filePaths;
		fileNames=[NSMutableArray arrayWithCapacity:mPlaylist_size];
		filePaths=[NSMutableArray arrayWithCapacity:mPlaylist_size];
		for (int i=0;i<mPlaylist_size;i++) {
			[fileNames insertObject:mPlaylist[i].mPlaylistFilename atIndex:i];
			[filePaths insertObject:mPlaylist[i].mPlaylistFilepath atIndex:i];
		}
		
		[prefs setObject:fileNames forKey:@"PlaylistEntries_Names"];
		[prefs setObject:filePaths forKey:@"PlaylistEntries_Paths"];
	}
	
    
	valNb=[[NSNumber alloc] initWithInt:mplayer.mod_subsongs];
	[prefs setObject:valNb forKey:@"Subsongs"];
	valNb=[[NSNumber alloc] initWithInt:mplayer.mod_currentsub];
	[prefs setObject:valNb forKey:@"Cur_subsong"];
    
    valNb=[[NSNumber alloc] initWithInt:[mplayer getArcEntriesCnt]];
	[prefs setObject:valNb forKey:@"ArchiveCnt"];
	valNb=[[NSNumber alloc] initWithInt:[mplayer getArcIndex]];
	[prefs setObject:valNb forKey:@"ArchiveIndex"];
    
	[prefs synchronize];
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

/*- (void)locationManager:(CLLocationManager *)manager didFailWithError:(NSError *)error {
 NSLog(@"Location manager error: %@", [error description]);
 }
 
 - (void)reverseGeocoder:(MKReverseGeocoder *)geocoder didFailWithError:(NSError *)error {
 NSLog(@"Reverse geocoder error: %@", [error description]);
 }
 
 
 - (void)locationManager:(CLLocationManager *)manager didUpdateToLocation:(CLLocation *)newLocation fromLocation:(CLLocation *)oldLocation {
 MKReverseGeocoder *geocoder = [[MKReverseGeocoder alloc] initWithCoordinate:newLocation.coordinate];
 geocoder.delegate = self;
 //	NSLog(@"calling geocoder");
 [geocoder start];
 }
 
 - (void)reverseGeocoder:(MKReverseGeocoder *)geocoder didFindPlacemark:(MKPlacemark *)placemark {
 //	NSLog(@"%@",[placemark.addressDictionary description]);
 located_country=[NSString stringWithString:placemark.country];
 located_city=[NSString stringWithString:placemark.locality];
 located_lon=placemark.coordinate.longitude;
 located_lat=placemark.coordinate.latitude;
 if ([geocoder retainCount]) [geocoder release];
 [self.locManager stopUpdatingLocation];
 locManager_isOn=1;
 }
 */

-(UIImage*) fexGetArchiveCover:(NSString *)filepath {
    UIImage *res_image=nil;
    
    fex_type_t type;
    fex_t* fex;
    const char *path=[filepath UTF8String];
    
    //NSLog(@"%s",path);
    
    /* Determine file's type */
    if (fex_identify_file( &type, path )) {
        //NSLog(@"fex cannot determine type of %s",path);
    }
    /* Only open files that fex can handle */
    if ( type != NULL ) {
        if (fex_open_type( &fex, path, type )) {
            NSLog(@"cannot fex open : %s",path);// / type : %s",path,type.extension);
        } else{
            while ( !fex_done( fex ) ) {
                NSString *strFilename=[NSString stringWithFormat:@"%s",fex_name(fex)];
                //NSLog(@"%@",strFilename);
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
                if ([[strFilename pathExtension] caseInsensitiveCompare:@"GIF"]==NSOrderedSame) {
                    //GIF detected
                    found_img=true;
                }
                
                if (found_img) {
                    char *data_ptr;
                    fex_data(fex,(const void**)&data_ptr);
                    if (data_ptr) {
                        long long size_data=fex_size(fex);
                        //NSLog(@"read img data, size: %d",size_data);
                        res_image=[UIImage imageWithData:[NSData dataWithBytes:data_ptr length:size_data]];
                        break;
                    }
                }

                
                if (fex_next( fex )) {
                    NSLog(@"Error during fex scanning");
                    break;
                }
            }
            fex_close( fex );
        }
        fex=NULL;
    }
    
    //res_image=[UIImage imageWithContentsOfFile:coverFilePath];//covers[index+1];
    
    return res_image;
}

-(int) fexScanArchiveForCover:(const char *)path {
    fex_type_t type;
    fex_t* fex;
    
    //NSLog(@"%s",path);
    
    /* Determine file's type */
    if (fex_identify_file( &type, path )) {
        NSLog(@"fex cannot determine type of %s",path);
    }
    /* Only open files that fex can handle */
    if ( type != NULL ) {
        if (fex_open_type( &fex, path, type )) {
            NSLog(@"cannot fex open : %s",path);// / type : %s",path,type.extension);
        } else{
            while ( !fex_done( fex ) ) {
                NSString *strFilename=[NSString stringWithFormat:@"%s",fex_name(fex)];
                //NSLog(@"%@",strFilename);
                if ([[strFilename pathExtension] caseInsensitiveCompare:@"PNG"]==NSOrderedSame) {
                    //PNG detected
                    fex_close(fex);
                    return 1;
                }
                if ([[strFilename pathExtension] caseInsensitiveCompare:@"JPG"]==NSOrderedSame) {
                    //JPG detected
                    fex_close(fex);
                    return 2;
                }
                if ([[strFilename pathExtension] caseInsensitiveCompare:@"JPEG"]==NSOrderedSame) {
                    //JPEG detected
                    fex_close(fex);
                    return 3;
                }
                if ([[strFilename pathExtension] caseInsensitiveCompare:@"GIF"]==NSOrderedSame) {
                    //GIF detected
                    fex_close(fex);
                    return 4;
                }
                
                if (fex_next( fex )) {
                    NSLog(@"Error during fex scanning");
                    break;
                }
            }
            fex_close( fex );
        }
        fex = NULL;
    } else {
        //NSLog( @"Skipping unsupported archive: %s\n", path );
    }
    return 0;
}


- (void) checkAvailableCovers:(int)index {
    NSString *pathFolderImgPNG,*pathCoverImgPNG,*pathFileImgPNG,*pathFolderImgJPG,*pathCoverImgJPG,*pathFileImgJPG,*pathFolderImgJPEG,*pathCoverImgJPEG,*pathFileImgJPEG,*pathFolderImgGIF,*pathCoverImgGIF,*pathFileImgGIF,*fullFilepath,*filePath,*basePath;
    NSFileManager *fileMngr=[[NSFileManager alloc] init];
    
//    NSLog(@"look for %d",index);
    
    mPlaylist[index].cover_flag=0; //used for cover flag
    filePath=mPlaylist[index].mPlaylistFilepath;
    basePath=[filePath stringByDeletingLastPathComponent];
    
    fullFilepath=[NSHomeDirectory() stringByAppendingFormat:@"/%@",filePath];
    
    pathFolderImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.png",basePath];
    pathFolderImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.jpg",basePath];
    pathFolderImgJPEG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.jpeg",basePath];
    pathFolderImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.gif",basePath];
    pathCoverImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/cover.png",basePath];
    pathCoverImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/cover.jpg",basePath];
    pathCoverImgJPEG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/cover.jpeg",basePath];
    pathCoverImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@/cover.gif",basePath];
    
    basePath=[filePath stringByDeletingPathExtension];
    pathFileImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.png",basePath];
    pathFileImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.jpg",basePath];
    pathFileImgJPEG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.jpeg",basePath];
    pathFileImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@.gif",basePath];
    //isReadableFileAtPath
    if ([fileMngr fileExistsAtPath:pathFileImgJPG]) mPlaylist[index].cover_flag=1;
    else if ([fileMngr fileExistsAtPath:pathFileImgJPEG]) mPlaylist[index].cover_flag=2;
    else if ([fileMngr fileExistsAtPath:pathFileImgPNG]) mPlaylist[index].cover_flag=3;
    else if ([fileMngr fileExistsAtPath:pathFileImgGIF]) mPlaylist[index].cover_flag=4;
    else if ([fileMngr fileExistsAtPath:pathFolderImgJPG]) mPlaylist[index].cover_flag=5;
    else if ([fileMngr fileExistsAtPath:pathFolderImgJPEG]) mPlaylist[index].cover_flag=6;
    else if ([fileMngr fileExistsAtPath:pathFolderImgPNG]) mPlaylist[index].cover_flag=7;
    else if ([fileMngr fileExistsAtPath:pathFolderImgGIF]) mPlaylist[index].cover_flag=8;
    else if ([fileMngr fileExistsAtPath:pathCoverImgJPG]) mPlaylist[index].cover_flag=9;
    else if ([fileMngr fileExistsAtPath:pathCoverImgJPEG]) mPlaylist[index].cover_flag=10;
    else if ([fileMngr fileExistsAtPath:pathCoverImgPNG]) mPlaylist[index].cover_flag=11;
    else if ([fileMngr fileExistsAtPath:pathCoverImgGIF]) mPlaylist[index].cover_flag=12;
    else if ([self fexScanArchiveForCover:[fullFilepath UTF8String] ]) mPlaylist[index].cover_flag=13;
    
    //[fileMngr release];
}

-(void) shortWait {
    [NSThread sleepForTimeInterval:0.1f];
}

-(void)showWaiting{
    waitingView.hidden=FALSE;
}

-(void)hideWaiting{
    waitingView.hidden=TRUE;
}


-(void)orientationDidChange:(NSNotification*)notification
{
    UIDeviceOrientation op=[[UIDevice currentDevice]orientation];
    UIInterfaceOrientation o = [[UIApplication sharedApplication] statusBarOrientation];
    //NSLog(@"change orientation: %d / %d",o,op);
    /*o = [[UIApplication sharedApplication] statusBarOrientation];

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
    */
    switch (op) {
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
    }

    /*if(Orientation==UIDeviceOrientationLandscapeLeft || Orientation==UIDeviceOrientationLandscapeRight)
    {
    }
    else if(Orientation==UIDeviceOrientationPortrait)
    {
    }*/
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
	clock_t start_time,end_time;
	start_time=clock();
    
    [super viewDidLoad];
    
    mLoadIssueMessage=0;
    
    temp_playlist=NULL;
    
    
    default_cover=[UIImage imageNamed:@"AppStore512.png"];
    //[default_cover retain];
    artwork=nil;
    
    cover_view.layer.shadowColor = [UIColor blackColor].CGColor;
    cover_view.layer.shadowOffset = CGSizeMake(1, 2);
    cover_view.layer.shadowOpacity = 1;
    cover_view.layer.shadowRadius = 2.0;
    cover_view.clipsToBounds = NO;
    
    
    //EQ
    eqVC=nil;
  

    [sliderProgressModule setThumbImage:[UIImage imageNamed:@"slider.png" ] forState:UIControlStateNormal];
    
    
    
    shouldRestart=1;
    m_displayLink=nil;
    
    gifAnimation=nil;
    cover_view.contentMode=UIViewContentModeScaleAspectFit;
    cover_viewBG.contentMode=UIViewContentModeScaleToFill;
    
    [UIView setAnimationDelegate:self];
    [UIView setAnimationDidStopSelector:@selector(animationDidStop:finished:context:)];
    
    UILongPressGestureRecognizer *longPressPaPrevSGesture = [[UILongPressGestureRecognizer alloc]
                                                              initWithTarget:self
                                                              action:@selector(longPressPrevSubArc:)];
    UILongPressGestureRecognizer *longPressPaNextSGesture = [[UILongPressGestureRecognizer alloc]
                                                              initWithTarget:self
                                                              action:@selector(longPressNextSubArc:)];
    UILongPressGestureRecognizer *longPressPlPrevSGesture = [[UILongPressGestureRecognizer alloc]
                                                              initWithTarget:self
                                                              action:@selector(longPressPrevSubArc:)];
    UILongPressGestureRecognizer *longPressPlNextSGesture = [[UILongPressGestureRecognizer alloc]
                                                              initWithTarget:self
                                                              action:@selector(longPressNextSubArc:)];
    
    [pauseBarSub layoutIfNeeded];
    [playBarSub layoutIfNeeded];
    
    
    
    /*[[[pauseBarSub subviews] objectAtIndex:2] addGestureRecognizer:longPressPaPrevSGesture];
    [[[pauseBarSub subviews] objectAtIndex:4] addGestureRecognizer:longPressPaNextSGesture];
    [[[playBarSub subviews] objectAtIndex:2] addGestureRecognizer:longPressPlPrevSGesture];
    [[[playBarSub subviews] objectAtIndex:4] addGestureRecognizer:longPressPlNextSGesture];*/
    
    if ([[playBarSubRewind valueForKey:@"view"] respondsToSelector:@selector(addGestureRecognizer:)]) {
        [[playBarSubRewind valueForKey:@"view"] addGestureRecognizer:longPressPlPrevSGesture];
    }
    if ([[playBarSubFFwd valueForKey:@"view"] respondsToSelector:@selector(addGestureRecognizer:)]) {
        [[playBarSubFFwd valueForKey:@"view"] addGestureRecognizer:longPressPlNextSGesture];
    }
    if ([[pauseBarSubRewind valueForKey:@"view"] respondsToSelector:@selector(addGestureRecognizer:)]) {
        [[pauseBarSubRewind valueForKey:@"view"] addGestureRecognizer:longPressPaPrevSGesture];
    }
    if ([[pauseBarSubFFwd valueForKey:@"view"] respondsToSelector:@selector(addGestureRecognizer:)]) {
        [[pauseBarSubFFwd valueForKey:@"view"] addGestureRecognizer:longPressPaNextSGesture];
    }
    
    
    labelModuleName.userInteractionEnabled = YES;
    UITapGestureRecognizer *tapGesture =
    [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(titleTap:)];
    [labelModuleName addGestureRecognizer:tapGesture];
	
	mPlaylist=(t_plPlaylist_entry*)calloc(MAX_PL_ENTRIES,sizeof(t_plPlaylist_entry));
    
	self.navigationItem.title=@"No file selected";
	//	self.navigationItem.backBarButtonItem.title=@"dd";
	
    UIBarButtonItem *bbitem=[[UIBarButtonItem alloc] initWithTitle:@"" style:UIBarButtonItemStylePlain target:self action:@selector(showPlaylist)];
    [bbitem setTitleTextAttributes:[NSDictionary dictionaryWithObjectsAndKeys: [UIFont fontWithName:@"FontAwesome" size:22.0], UITextAttributeFont,nil] forState:UIControlStateNormal];
    unichar tmpChar=0xF0CA;
    [bbitem setTitle:[NSString stringWithCharacters:&tmpChar length:1]];
    [self.navigationItem setRightBarButtonItem:bbitem animated:YES];
    [bbitem setTitlePositionAdjustment:UIOffsetMake(0,1.5) forBarMetrics:UIBarMetricsDefault];
	/*[self.navigationItem setRightBarButtonItem:[[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"bb_list_white.png"]
																				 style:UIBarButtonItemStylePlain
																				target:self
																				action:@selector(showPlaylist)] autorelease]
									  animated:YES];
     */
    mRandomFXCpt=0;
	mRandomFXCptRev=0;
	mHasFocus=0;
	mShouldUpdateInfos=0;
	mPaused=1;
	mScaleFactor=1.0f;
    
    if (@available(iOS 14.0, *)) {
            if ([NSProcessInfo processInfo].isiOSAppOnMac) {
                is_macOS=1;
                mDeviceType=3;
            }else{
                is_macOS=0;
            }
        }
    
    mDeviceIPhoneX=1;
    
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
		if (!is_macOS) mDeviceType=1; //ipad
        UIScreen* mainscr = [UIScreen mainScreen];

        if (mainscr.bounds.size.height>mainscr.bounds.size.width) {
            mDevice_hh=mainscr.bounds.size.height;
            mDevice_ww=mainscr.bounds.size.width;
            orientationHV=UIInterfaceOrientationPortrait; //(int)[[UIDevice currentDevice]orientation];
        } else {
            mDevice_ww=mainscr.bounds.size.height;
            mDevice_hh=mainscr.bounds.size.width;
            orientationHV=UIInterfaceOrientationLandscapeLeft; //(int)[[UIDevice currentDevice]orientation];
        }
        
        if (is_macOS) {
            mDevice_ww=480*1.5f;
            mDevice_hh=480*1.5f;
            /*CGRect frame = [self.view.window frame];
            frame.size.height = mDevice_hh;
            frame.size.width = mDevice_ww;
            [self.view.window setFrame: frame];*/
            
        }
        
        
        mScaleFactor=mainscr.scale;
        
        if (mScaleFactor>=2) mDeviceType=2;
	}
	else {
		
		mDeviceType=0; //iphone
		mDevice_hh=480;
		mDevice_ww=320;
		UIScreen* mainscr = [UIScreen mainScreen];
        
//        NSLog(@"w %f h %f s %f",mainscr.bounds.size.width,mainscr.bounds.size.height,mainscr.scale);
        if (mainscr.bounds.size.height>mainscr.bounds.size.width) {
            mDevice_hh=mainscr.bounds.size.height;
            mDevice_ww=mainscr.bounds.size.width;
            orientationHV=UIInterfaceOrientationPortrait; //(int)[[UIDevice currentDevice]orientation];
        } else {
            mDevice_ww=mainscr.bounds.size.height;
            mDevice_hh=mainscr.bounds.size.width;
            orientationHV=UIInterfaceOrientationLandscapeLeft; //(int)[[UIDevice currentDevice]orientation];
        }
        if (mainscr.bounds.size.height==812) mDeviceIPhoneX=1;
        mScaleFactor=mainscr.scale;
        
        if (mScaleFactor>=2) mDeviceType=2;
        
	}
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(orientationDidChange:) name:UIDeviceOrientationDidChangeNotification object:nil];
    
    
//    NSLog(@"s %f w %d h %d",mScaleFactor,mDevice_ww,mDevice_hh);
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
	mSlowDevice=0;
	NSRange r = [strMachine rangeOfString:@"iPhone1," options:NSCaseInsensitiveSearch];
	if (r.location != NSNotFound) {
		mSlowDevice=1;
	}
	r.location=NSNotFound;
	r = [strMachine rangeOfString:@"iPod1," options:NSCaseInsensitiveSearch];
	if (r.location != NSNotFound) {
		mSlowDevice=1;
	}
	self.hidesBottomBarWhenPushed = YES;
    
    //Coverflow
    coverflow = [[TKCoverflowView alloc] init];
    coverflow.autoresizingMask = 0;//UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    
	coverflow.coverflowDelegate = self;
	coverflow.dataSource = self;
    coverflow.hidden=TRUE;
    
    
    if([UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad){
		coverflow.coverSpacing = 160;
		coverflow.coverSize = CGSizeMake(400, 400);
        coverflow.frame=CGRectMake(0,0,mDevice_hh,mDevice_ww-20);
	} else {
        coverflow.coverSpacing = 80.0;
        coverflow.coverSize = CGSizeMake(200, 200);
        coverflow.frame=CGRectMake(0,0,mDevice_hh,mDevice_ww-20);
    }
    coverflow_plsize=coverflow_pos=coverflow_needredraw=0;
    
    covers_default=[[UIImage imageNamed:@"default_art.png"] scaleToSize:coverflow.coverSize];
    //[covers_default retain];
    [coverflow setNumberOfCovers:0];
    
    //
    lblMainCoverflow=[[UILabel alloc] init];
    lblSecCoverflow=[[UILabel alloc] init];
    
    lblCurrentSongCFlow=[[UILabel alloc] init];
    lblTimeFCflow=[[UILabel alloc] init];
    btnPlayCFlow=[UIButton buttonWithType:UIButtonTypeCustom];
    btnPauseCFlow=[UIButton buttonWithType:UIButtonTypeCustom];
    btnBackCFlow=[UIButton buttonWithType:UIButtonTypeCustom];
    btnPrevCFlow=[UIButton buttonWithType:UIButtonTypeCustom];
    btnNextCFlow=[UIButton buttonWithType:UIButtonTypeCustom];
    btnPrevSubCFlow=[UIButton buttonWithType:UIButtonTypeCustom];
    btnNextSubCFlow=[UIButton buttonWithType:UIButtonTypeCustom];
    
    [btnPlayCFlow setTitleColor:[UIColor clearColor] forState:UIControlStateNormal];
    [btnPauseCFlow setTitleColor:[UIColor clearColor] forState:UIControlStateNormal];
    [btnPrevCFlow setTitleColor:[UIColor clearColor] forState:UIControlStateNormal];
    //[btnPrevSubCFlow setTitleColor:[UIColor clearColor] forState:UIControlStateNormal];
    [btnNextCFlow setTitleColor:[UIColor clearColor] forState:UIControlStateNormal];
    //[btnNextSubCFlow setTitleColor:[UIColor clearColor] forState:UIControlStateNormal];
    
    //    btnNextCFlow.backgroundColor = [UIColor colorWithRed:0.22 green:0.18 blue:0.22 alpha:1.0];
    //    btnNextCFlow.layer.borderColor = [UIColor blackColor].CGColor;
    //    btnNextCFlow.layer.borderWidth = 0.5f;
    //    btnNextCFlow.layer.cornerRadius = 14.0f;
    
    [btnPlayCFlow setImage:[UIImage imageNamed:@"video_play.png"] forState:UIControlStateNormal];
    [btnPlayCFlow setImage:[UIImage imageNamed:@"video_play_h.png"] forState:UIControlStateHighlighted];
    [btnPlayCFlow addTarget: self action: @selector(playPushed:) forControlEvents: UIControlEventTouchUpInside];
    
    [btnPauseCFlow setImage:[UIImage imageNamed:@"video_pause.png"] forState:UIControlStateNormal];
    [btnPauseCFlow setImage:[UIImage imageNamed:@"video_pause_h.png"] forState:UIControlStateHighlighted];
    [btnPauseCFlow addTarget: self action: @selector(pausePushed:) forControlEvents: UIControlEventTouchUpInside];
    
    [btnPrevCFlow setImage:[UIImage imageNamed:@"video_previous.png"] forState:UIControlStateNormal];
    [btnPrevCFlow setImage:[UIImage imageNamed:@"video_previous_h.png"] forState:UIControlStateHighlighted];
    [btnPrevCFlow addTarget: self action: @selector(playPrev) forControlEvents: UIControlEventTouchUpInside];
    
    //[btnPrevSubCFlow setImage:[UIImage imageNamed:@"video_prevsub.png"] forState:UIControlStateNormal];
    //[btnPrevSubCFlow setImage:[UIImage imageNamed:@"video_prevsub_h.png"] forState:UIControlStateHighlighted];
    [btnPrevSubCFlow.titleLabel setFont:[UIFont boldSystemFontOfSize:(mDeviceType==1?32:20)]];
    [btnPrevSubCFlow setTitle:@"<" forState:UIControlStateNormal];
    [btnPrevSubCFlow setTitleColor:[UIColor colorWithRed:0.72f green:0.72f blue:0.72f alpha:1.0f] forState:UIControlStateNormal];
    [btnPrevSubCFlow setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
    [btnPrevSubCFlow addTarget: self action: @selector(playPrevSub) forControlEvents: UIControlEventTouchUpInside];
    
    [btnNextCFlow setImage:[UIImage imageNamed:@"video_next.png"] forState:UIControlStateNormal];
    [btnNextCFlow setImage:[UIImage imageNamed:@"video_next_h.png"] forState:UIControlStateHighlighted];
    [btnNextCFlow addTarget: self action: @selector(playNext) forControlEvents: UIControlEventTouchUpInside];
    
    //[btnNextSubCFlow setImage:[UIImage imageNamed:@"video_nextsub.png"] forState:UIControlStateNormal];
    //[btnNextSubCFlow setImage:[UIImage imageNamed:@"video_nextsub_h.png"] forState:UIControlStateHighlighted];
    [btnNextSubCFlow.titleLabel setFont:[UIFont boldSystemFontOfSize:(mDeviceType==1?32:20)]];
    [btnNextSubCFlow setTitle:@">" forState:UIControlStateNormal];
    [btnNextSubCFlow setTitleColor:[UIColor colorWithRed:0.72f green:0.72f blue:0.72f alpha:1.0f] forState:UIControlStateNormal];
    [btnNextSubCFlow setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
    [btnNextSubCFlow addTarget: self action: @selector(playNextSub) forControlEvents: UIControlEventTouchUpInside];
    
    [btnBackCFlow setImage:[UIImage imageNamed:@"arrow_left.png"] forState:UIControlStateNormal];
    [btnBackCFlow setImage:[UIImage imageNamed:@"arrow_left_h.png"] forState:UIControlStateHighlighted];
    [btnBackCFlow addTarget: self action: @selector(backPushed:) forControlEvents: UIControlEventTouchUpInside];
    
    lblMainCoverflow.hidden=TRUE;
    lblSecCoverflow.hidden=TRUE;
    lblCurrentSongCFlow.hidden=TRUE;
    lblTimeFCflow.hidden=TRUE;
    btnPlayCFlow.hidden=TRUE;
    btnPauseCFlow.hidden=TRUE;
    btnBackCFlow.hidden=TRUE;
    btnPrevCFlow.hidden=TRUE;
    btnPrevSubCFlow.hidden=TRUE;
    btnNextCFlow.hidden=TRUE;
    btnNextSubCFlow.hidden=TRUE;
    
    /* btnPlayCFlow.imageEdgeInsets=UIEdgeInsetsMake(16,8,0,8);
     btnPauseCFlow.imageEdgeInsets=UIEdgeInsetsMake(16,8,0,8);
     btnPrevCFlow.imageEdgeInsets=UIEdgeInsetsMake(16,8,0,8);
     btnNextCFlow.imageEdgeInsets=UIEdgeInsetsMake(16,8,0,8);
     btnPrevSubCFlow.titleEdgeInsets=UIEdgeInsetsMake(16,8,0,8);
     btnNextSubCFlow.titleEdgeInsets =UIEdgeInsetsMake(16,8,0,8);
     */
    lblMainCoverflow.font=[UIFont boldSystemFontOfSize:(mDeviceType==1?32:16)];
    lblSecCoverflow.font=[UIFont systemFontOfSize:(mDeviceType==1?20:10)];
    lblCurrentSongCFlow.font=[UIFont systemFontOfSize:(mDeviceType==1?20:10)];
    lblTimeFCflow.font=[UIFont systemFontOfSize:(mDeviceType==1?20:10)];
    
    lblMainCoverflow.backgroundColor=[UIColor clearColor];
    lblSecCoverflow.backgroundColor=[UIColor clearColor];
    lblCurrentSongCFlow.backgroundColor=[UIColor clearColor];
    lblTimeFCflow.backgroundColor=[UIColor clearColor];
    
    
    lblMainCoverflow.textColor=[UIColor whiteColor];
    lblSecCoverflow.textColor=[UIColor whiteColor];
    lblCurrentSongCFlow.textColor=[UIColor whiteColor];
    lblTimeFCflow.textColor=[UIColor whiteColor];
    
    
    lblMainCoverflow.textAlignment=NSTextAlignmentCenter;
    lblSecCoverflow.textAlignment=NSTextAlignmentCenter;
    lblCurrentSongCFlow.textAlignment=NSTextAlignmentLeft;
    lblTimeFCflow.textAlignment=NSTextAlignmentRight;
    
    lblMainCoverflow.lineBreakMode=NSLineBreakByTruncatingMiddle;
    lblSecCoverflow.lineBreakMode=NSLineBreakByTruncatingMiddle;
    lblCurrentSongCFlow.lineBreakMode=NSLineBreakByTruncatingMiddle;
    lblTimeFCflow.lineBreakMode=NSLineBreakByTruncatingMiddle;
    
    [self.view addSubview:coverflow];
    [self.view addSubview:lblMainCoverflow];
    [self.view addSubview:lblSecCoverflow];
    [self.view addSubview:lblCurrentSongCFlow];
    [self.view addSubview:lblTimeFCflow];
    [self.view addSubview:btnPrevCFlow];
    [self.view addSubview:btnPrevSubCFlow];
    [self.view addSubview:btnPlayCFlow];
    [self.view addSubview:btnPauseCFlow];
    [self.view addSubview:btnNextCFlow];
    [self.view addSubview:btnNextSubCFlow];
    [self.view addSubview:btnBackCFlow];
    
    // Get location
	/*self.locManager = [[[CLLocationManager alloc] init] autorelease];
     if (!self.locManager.locationServicesEnabled) {
     //@"User has opted out of location services"];
     //		NSLog(@"no location available");
     locManager_isOn=0;
     } else {
     self.locManager.delegate = self;
     self.locManager.desiredAccuracy = kCLLocationAccuracyThreeKilometers;
     self.locManager.distanceFilter = 1000.0f; // in meters
     locationLastUpdate=[[NSDate alloc] init];
     locManager_isOn=2;
     [self.locManager startUpdatingLocation];
     }*/
	
//    [[infoButton layer] setCornerRadius:10.0];
	/* Popup stuff */
	[[infoMsgView layer] setCornerRadius:5.0];
	[[infoMsgView layer] setBorderWidth:2.0];
	[[infoMsgView layer] setBorderColor:[[UIColor colorWithRed: 0.95f green: 0.95f blue: 0.95f alpha: 1.0f] CGColor]];   //Adding Border color.
	infoMsgView.hidden=YES;
    
	/**/
	
	ratingImg[0] = @"rating0.png";
	ratingImg[1] = @"rating1.png";
	ratingImg[2] = @"rating2.png";
	ratingImg[3] = @"rating3.png";
	ratingImg[4] = @"rating4.png";
	ratingImg[5] = @"rating5.png";
	
	for (int i=0;i<3;i++) viewTapInfoStr[i]=NULL;
	
	mShouldHaveFocus=0;
    
    mPlaylist_size=0;
	mIsPlaying=FALSE;
	oglViewFullscreenChanged=0;
    mOglViewIsHidden=YES;
	mRating=0;
	
	infoZoom.hidden=NO;
	infoUnzoom.hidden=YES;
	
	volWin.frame= CGRectMake(0, mDevice_hh-64-42-32, mDevice_ww, 44);
    //debug
    /*[volWin setBackgroundColor:[UIColor colorWithRed:66
                                               green:79
                                                blue:91
                                               alpha:1]];
     */
	volumeView = [[MPVolumeView alloc] initWithFrame:CGRectMake(volWin.bounds.origin.x+12,volWin.bounds.origin.y+5,volWin.bounds.size.width-24,volWin.bounds.size.height)/*volWin.bounds*/];
    //	volumeView.center = CGPointMake(mDevice_ww/2,32);
    //  [volumeView setShowsRouteButton:YES];
    //	[volumeView sizeToFit];
	[volWin addSubview:volumeView];
    
	
	mRestart=0;
	mRestart_sub=0;
	
	[sliderProgressModule.layer setCornerRadius:8.0];
	[labelSeeking.layer setCornerRadius:8.0];
	[labelTime.layer setCornerRadius:8.0];
	//[commandViewU.layer setCornerRadius:8.0];
	
	textMessage.font = [UIFont fontWithName:@"Courier-Bold" size:(mDeviceType==1?18:12)];
	
    
    /////////////////////////////////////
    // Waiting view
    /////////////////////////////////////
    waitingView = [[UIView alloc] init];
	waitingView.backgroundColor=[UIColor blackColor];//[UIColor colorWithRed:0 green:0 blue:0 alpha:0.8f];
	waitingView.opaque=YES;
	waitingView.hidden=FALSE;
	waitingView.layer.cornerRadius=20;
	
	UIActivityIndicatorView *indView=[[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(50-20,50-30,40,40)];
	indView.activityIndicatorViewStyle=UIActivityIndicatorViewStyleWhiteLarge;
	[waitingView addSubview:indView];
    UILabel *lblLoading=[[UILabel alloc] initWithFrame:CGRectMake(10,70,80,20)];
    lblLoading.text=@"Loading";
    lblLoading.backgroundColor=[UIColor blackColor];
    lblLoading.opaque=YES;
    lblLoading.textColor=[UIColor whiteColor];
    lblLoading.textAlignment=NSTextAlignmentCenter;
    lblLoading.font=[UIFont italicSystemFontOfSize:16];
    [waitingView addSubview:lblLoading];
    //[lblLoading autorelease];
    
	[indView startAnimating];
	//[indView autorelease];
    
    waitingView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:waitingView];
    
    NSDictionary *views = NSDictionaryOfVariableBindings(waitingView);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingView(100)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingView(100)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    
	//init pattern notes buffer
	for (int i=0;i<128;i++) {
		mText[i]=nil;
	}
	for (int i=0;i<128;i++) {
		mTextLine[i]=nil;
	}
	
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"detail1 : %d",end_time-start_time);
#endif
	
	mHeader=nil;
	mplayer = [[ModizMusicPlayer alloc] initMusicPlayer];
    
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"detail2 : %d",end_time-start_time);
#endif
    //opengl stuff
	m_oglContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
	[EAGLContext setCurrentContext:m_oglContext];
	[m_oglView initialize:m_oglContext scaleFactor:mScaleFactor];
    
    
    // Create gesture recognizer
    UITapGestureRecognizer *glViewOneFingerOneTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(glViewOneFingerOneTap:)];
    // Set required taps and number of touches
    [glViewOneFingerOneTap setNumberOfTapsRequired:1];
    [glViewOneFingerOneTap setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewOneFingerOneTap];
    
    // Create gesture recognizer
    UITapGestureRecognizer *glViewOneFingerTwoTaps = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(glViewOneFingerTwoTaps)];
    // Set required taps and number of touches
    [glViewOneFingerTwoTaps setNumberOfTapsRequired:2];
    [glViewOneFingerTwoTaps setNumberOfTouchesRequired:1];
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewOneFingerTwoTaps];
    
    // Create gesture recognizer
    UIPanGestureRecognizer *glViewPanGesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(glViewPanGesture:)];
    // Set required taps and number of touches
    [glViewPanGesture setMinimumNumberOfTouches:1];
    [glViewPanGesture setMaximumNumberOfTouches:1];
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewPanGesture];

    // Create gesture recognizer
    UIPanGestureRecognizer *glViewPan2Gesture = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(glViewPan2Gesture:)];
    // Set required taps and number of touches
    [glViewPan2Gesture setMinimumNumberOfTouches:2];
    [glViewPan2Gesture setMaximumNumberOfTouches:2];
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewPan2Gesture];
    
    // Create gesture recognizer
    UIPinchGestureRecognizer *glViewPinchGesture = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(glViewPinchGesture:)];
    // Add the gesture to the view
    [m_oglView addGestureRecognizer:glViewPinchGesture];

    //[glViewOneFingerOneTap requireGestureRecognizerToFail : glViewOneFingerTwoTaps];
    
    //FLUID
    if (mDevice_ww==320) initFluid(40,40);
    else initFluid(64,64);
    
    //BButton
    [btnShowArcList setType:BButtonTypeInverse];
    [btnShowSubSong setType:BButtonTypeInverse];
    [btnShowArcList addAwesomeIcon:0x187 beforeTitle:YES font_size:20];
    [btnShowSubSong addAwesomeIcon:0x16c beforeTitle:YES font_size:20];
    
    [infoButton setType:BButtonTypeInverse];
    [infoButton addAwesomeIcon:0x05A beforeTitle:YES font_size:28];
    
    [eqButton setType:BButtonTypeInverse];
    [eqButton setTitleColor:(nvdsp_EQ?[UIColor whiteColor]:[UIColor grayColor]) forState:UIControlStateNormal];
//    [infoButton addAwesomeIcon:0x05A beforeTitle:YES font_size:28];
    
	
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"detail3 : %d",end_time-start_time);
#endif
    //Visualization
	/* Set Starting Angle To Zero */
    angle=0.0f;
    /* Create Our Empty Texture */
    //    BlurTexture=EmptyTexture(128,128);
    //    FxTexture=EmptyTexture(512,512);
	
	
	
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);       /* Black Background        */
    glClearDepthf(1.0f);                        /* Depth Buffer Setup      */
    glDepthFunc(GL_LEQUAL);   /* The Type Of Depth Testing (Less Or Equal) */
    glEnable(GL_DEPTH_TEST);  /* Enable Depth Testing                      */
    /* Set Perspective Calculations To Most Accurate */
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);//GL_NICEST);
    
    memset(txtMenuHandle,0,sizeof(txtMenuHandle));
	txtMenuHandle[0]=TextureUtils::Create([UIImage imageNamed:@"txtMenu1_2x.png"]);
	txtMenuHandle[1]=TextureUtils::Create([UIImage imageNamed:@"txtMenu2a_2x.png"]);
	txtMenuHandle[2]=TextureUtils::Create([UIImage imageNamed:@"txtMenu3a_2x.png"]);
	txtMenuHandle[3]=TextureUtils::Create([UIImage imageNamed:@"txtMenu4a_2x.png"]);
	txtMenuHandle[4]=TextureUtils::Create([UIImage imageNamed:@"txtMenu5a_2x.png"]);
	txtMenuHandle[5]=TextureUtils::Create([UIImage imageNamed:@"txtMenu6_2x.png"]);
    txtMenuHandle[6]=TextureUtils::Create([UIImage imageNamed:@"txtMenu7a_2x.png"]);
    txtMenuHandle[7]=TextureUtils::Create([UIImage imageNamed:@"txtMenu8a_2x.png"]);
    txtMenuHandle[8]=TextureUtils::Create([UIImage imageNamed:@"txtMenu9_2x.png"]);
    txtMenuHandle[9]=TextureUtils::Create([UIImage imageNamed:@"txtMenu10a_2x.png"]);
    txtMenuHandle[10]=TextureUtils::Create([UIImage imageNamed:@"txtMenu11d_2x.png"]);
    txtMenuHandle[11]=TextureUtils::Create([UIImage imageNamed:@"txtMenu12a_2x.png"]);
    txtMenuHandle[12]=TextureUtils::Create([UIImage imageNamed:@"txtMenu0.png"]);
//    texturePiano=TextureUtils::Create([UIImage imageNamed:@"text_wood.png"]);
    
    memset(txtSubMenuHandle,0,sizeof(txtSubMenuHandle));
    
#define SUBMENU0_START 0
#define SUBMENU0_SIZE 6
	txtSubMenuHandle[0]=0;
    txtSubMenuHandle[1]=txtMenuHandle[1];//TextureUtils::Create([UIImage imageNamed:@"txtMenu2a.png"]);
	txtSubMenuHandle[2]=TextureUtils::Create([UIImage imageNamed:@"txtMenu2b_2x.png"]);
	txtSubMenuHandle[3]=TextureUtils::Create([UIImage imageNamed:@"txtMenu2c_2x.png"]);
    txtSubMenuHandle[4]=TextureUtils::Create([UIImage imageNamed:@"txtMenu2d_2x.png"]);
    txtSubMenuHandle[5]=TextureUtils::Create([UIImage imageNamed:@"txtMenu2e_2x.png"]);
    
#define SUBMENU1_START 6
#define SUBMENU1_SIZE 4
    txtSubMenuHandle[6]=0;
	txtSubMenuHandle[7]=txtMenuHandle[2];//TextureUtils::Create([UIImage imageNamed:@"txtMenu3a.png"]);
	txtSubMenuHandle[8]=TextureUtils::Create([UIImage imageNamed:@"txtMenu3b_2x.png"]);
	txtSubMenuHandle[9]=TextureUtils::Create([UIImage imageNamed:@"txtMenu3c_2x.png"]);
    
#define SUBMENU2_START 10
#define SUBMENU2_SIZE 3
    txtSubMenuHandle[10]=0;
	txtSubMenuHandle[11]=txtMenuHandle[3];//TextureUtils::Create([UIImage imageNamed:@"txtMenu4a.png"]);
    txtSubMenuHandle[12]=TextureUtils::Create([UIImage imageNamed:@"txtMenu4b_2x.png"]);
    
#define SUBMENU3_START 13
#define SUBMENU3_SIZE 3
    txtSubMenuHandle[13]=0;
    txtSubMenuHandle[14]=txtMenuHandle[4];//TextureUtils::Create([UIImage imageNamed:@"txtMenu5a.png"]);
    txtSubMenuHandle[15]=TextureUtils::Create([UIImage imageNamed:@"txtMenu5b_2x.png"]);
    
#define SUBMENU4_START 16
#define SUBMENU4_SIZE 7
    txtSubMenuHandle[16]=0;
    txtSubMenuHandle[17]=txtMenuHandle[6];//TextureUtils::Create([UIImage imageNamed:@"txtMenu7a.png"]);
    txtSubMenuHandle[18]=TextureUtils::Create([UIImage imageNamed:@"txtMenu7b_2x.png"]);
    txtSubMenuHandle[19]=TextureUtils::Create([UIImage imageNamed:@"txtMenu7c_2x.png"]);
    txtSubMenuHandle[20]=TextureUtils::Create([UIImage imageNamed:@"txtMenu7d_2x.png"]);
    txtSubMenuHandle[21]=TextureUtils::Create([UIImage imageNamed:@"txtMenu7e_2x.png"]);
    txtSubMenuHandle[22]=TextureUtils::Create([UIImage imageNamed:@"txtMenu7f_2x.png"]);
    
#define SUBMENU5_START 23
#define SUBMENU5_SIZE 3
    txtSubMenuHandle[23]=0;
    txtSubMenuHandle[24]=txtMenuHandle[7];
    txtSubMenuHandle[25]=TextureUtils::Create([UIImage imageNamed:@"txtMenu8b_2x.png"]);
    
#define SUBMENU6_START 26
#define SUBMENU6_SIZE 3
	txtSubMenuHandle[26]=0;
    txtSubMenuHandle[27]=txtMenuHandle[9];
    txtSubMenuHandle[28]=TextureUtils::Create([UIImage imageNamed:@"txtMenu10b_2x.png"]);

#define SUBMENU7_START 29
#define SUBMENU7_SIZE 5
    txtSubMenuHandle[29]=0;
    txtSubMenuHandle[30]=TextureUtils::Create([UIImage imageNamed:@"txtMenu11a_2x.png"]);
    txtSubMenuHandle[31]=TextureUtils::Create([UIImage imageNamed:@"txtMenu11b_2x.png"]);
    txtSubMenuHandle[32]=TextureUtils::Create([UIImage imageNamed:@"txtMenu11c_2x.png"]);
    txtSubMenuHandle[33]=txtMenuHandle[10];

#define SUBMENU8_START 34
#define SUBMENU8_SIZE 4
	txtSubMenuHandle[34]=0;
    txtSubMenuHandle[35]=txtMenuHandle[11];
    txtSubMenuHandle[36]=TextureUtils::Create([UIImage imageNamed:@"txtMenu12b_2x.png"]);
    txtSubMenuHandle[37]=TextureUtils::Create([UIImage imageNamed:@"txtMenu12c_2x.png"]);
    
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"detail4 : %d",end_time-start_time);
#endif
    //init play/pause status for button
	self.pauseBar.hidden=YES;
	self.playBar.hidden=NO;
	self.pauseBarSub.hidden=YES;
	self.playBarSub.hidden=YES;
	[self updateBarPos];
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"detail5 : %d",end_time-start_time);
#endif
	mplayer.bGlobalAudioPause=1;
    //init mod player var
	ModPlug_Settings *mpsettings=[mplayer getMPSettings];
	mpsettings->mResamplingMode = MODPLUG_RESAMPLE_LINEAR; /* RESAMP */
	mpsettings->mChannels = 2;
	mpsettings->mBits = 16;
	mpsettings->mFrequency = 44100;
	/* insert more setting changes here */
	mpsettings->mLoopCount = 0;
	[mplayer updateMPSettings];
    
	
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"detail6 : %d",end_time-start_time);
#endif
	
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

    
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"detail7 : %d",end_time-start_time);
#endif
    
    
    
    
	
	
	int size_chan;
	switch (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value) {
        case 1:
        case 4:
            size_chan=12*6;
            break;
        case 2:
        case 5:
            size_chan=6*6;
            break;
        case 3:
        case 6:
            size_chan=4*6;
            break;
    }
	visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN+size_chan-1)/size_chan;
	mMoveStartChanLeft=mMoveStartChanRight=0;
	
	if ([self checkFlagOnStartup]) {
/*		alertCrash = [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning" ,@"")
                                                 message:NSLocalizedString(@"First launch or issue encountered last time Modizer was running. Apply default settings ?",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		if (alertCrash) [alertCrash show];*/
		[self loadSettings:1];
		mShouldUpdateInfos=1;
	} else [self loadSettings:0];
    
    for (int i=0;i<mPlaylist_size;i++) mPlaylist[i].cover_flag=-1;
    
	
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"detail8 : %d",end_time-start_time);
#endif
    
    [self.view bringSubviewToFront:infoMsgView];
    
	
	
//	[super viewDidLoad];
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"detail : %d",end_time-start_time);
#endif
}

- (void)dealloc {
    [waitingView removeFromSuperview];
    //[waitingView release];
    
    
    //[coverflow release];
    if (covers_default) {
        //[covers_default release];
        covers_default=nil;
    }
    
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
    
    //Fluid
    closeFluid();
    
    //FFT
    delete(fftAccel);
    free(fft_frequency);
    free(fft_frequencyAvg);
    free(fft_freqAvgCount);
    free(fft_time);

    
	//[super dealloc];
}


-(void) enterBackground {
	//if (mHasFocus) [self.navigationController popViewControllerAnimated:YES];
	if (mHasFocus) {
		mShouldHaveFocusAfterBackground=1;
		[self viewWillDisappear:NO];
	} else mShouldHaveFocusAfterBackground=0;
	
}

-(void) enterForeground {
	if (mShouldHaveFocusAfterBackground) {
		[self viewWillAppear:YES];
	}
}


- (void)viewWillAppear:(BOOL)animated {
    alertCannotPlay_displayed=0;
    
    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent animated:YES];
    
    //coverflow
    if (coverflow && (settings[GLOB_CoverFlow].detail.mdz_boolswitch.switch_value==FALSE)) {
        coverflow.alpha=0;
        lblMainCoverflow.alpha=0;
        lblSecCoverflow.alpha=0;
        lblCurrentSongCFlow.alpha=0;
        lblTimeFCflow.alpha=0;
        btnPlayCFlow.alpha=0;
        btnPauseCFlow.alpha=0;
        btnBackCFlow.alpha=0;
        
        coverflow.hidden=TRUE;
        lblMainCoverflow.hidden=TRUE;
        lblSecCoverflow.hidden=TRUE;
        lblCurrentSongCFlow.hidden=TRUE;
        lblTimeFCflow.hidden=TRUE;
        btnPlayCFlow.hidden=TRUE;
        btnPauseCFlow.hidden=TRUE;
        btnBackCFlow.hidden=TRUE;
    }
    
    //eq
    eqVC=nil;
    [eqButton setTitleColor:(nvdsp_EQ?[UIColor whiteColor]:[UIColor grayColor]) forState:UIControlStateNormal];
    [eqButton setTitleColor:(nvdsp_EQ?[UIColor whiteColor]:[UIColor grayColor]) forState:UIControlStateHighlighted];
    
	if (mPlaylist_size) {
		//DBHelper::getFilesStatsDBmod(mPlaylist,mPlaylist_size);
        for (int i=0;i<mPlaylist_size;i++) {  //reset rating to force resynchro (for ex, user delted an entry in 'favorites' list, thus reseting the rating for a given file
            mPlaylist[i].mPlaylistRating=-1;
        }
        
        //update rating (-1 => get current value from DB)
        [self pushedRatingCommon:-1];
        
        //Check rating for current entry
        /*DBHelper::getFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,
                                    mPlaylist[mPlaylist_pos].mPlaylistFilepath,
                                    NULL,
                                    &(mPlaylist[mPlaylist_pos].mPlaylistRating),
                                    NULL,
                                    NULL);*/
		[self showRating:mPlaylist[mPlaylist_pos].mPlaylistRating];
        //update playlist
        /*		NSIndexPath *myindex=[[[NSIndexPath alloc] initWithIndex:0] autorelease];
         [self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:FALSE scrollPosition:UITableViewScrollPositionMiddle];*/
        
        
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
    
    if (shouldRestart) {
        shouldRestart=0;
        [self play_restart];
    }
    
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
	[EAGLContext setCurrentContext:m_oglContext];
    [m_oglView layoutSubviews];
    [m_oglView bind];
    
	
    //	self.navigationController.navigationBar.hidden = YES;
    m_displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(doFrame)];
    m_displayLink.frameInterval = (settings[GLOB_FXFPS].detail.mdz_switch.switch_value?1:2); //30 or 60 fps depending on device speed iPhone
	[m_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
	
	[self updateBarPos];
	//Hack to allow UIToolbar to be set up correctly
	if (((UIInterfaceOrientation)orientationHV==UIInterfaceOrientationPortrait) || ((UIInterfaceOrientation)orientationHV==UIInterfaceOrientationPortraitUpsideDown) ) {
		[self willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)orientationHV duration:0];
    } else {
        if (coverflow.hidden==FALSE) {
            [[self navigationController] setNavigationBarHidden:YES animated:NO];
        }
        [self willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)orientationHV duration:0];
    }
    CATransition *transition=[CATransition animation];
    transition.duration=0.2;
    transition.timingFunction= [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];
    [[[self navigationController] navigationBar].layer addAnimation:transition forKey:nil];
    [[[self navigationController] navigationBar] setBarStyle:UIBarStyleBlack];
    
}


- (void)viewWillDisappear:(BOOL)animated {
    is_macOS=false;
    if (@available(iOS 14.0, *)) {
            if ([NSProcessInfo processInfo].isiOSAppOnMac) {
                is_macOS=true;
            }else{
                is_macOS=false;
            }
        }
    if (!is_macOS) if (m_displayLink) [m_displayLink invalidate];
    self.navigationController.navigationBar.hidden = NO;
    
    CATransition *transition=[CATransition animation];
    transition.duration=0.2;
    transition.timingFunction= [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];
    [[[self navigationController] navigationBar].layer addAnimation:transition forKey:nil];
    
//    [[UIDevice currentDevice] systemVersion]
    
    [[[self navigationController] navigationBar] setBarStyle:UIBarStyleBlack];
    
    
	
	for (int i=0;i<3;i++) if (viewTapInfoStr[i]) {
		delete viewTapInfoStr[i];
		viewTapInfoStr[i]=NULL;
	}
	if (mFont) {
		delete mFont;
		mFont=NULL;
	}
	
}

- (void)viewDidAppear:(BOOL)animated {
	mHasFocus=1;
    
    nowplayingPL=nil;
    
    // if temp_playlist, free it
    if (temp_playlist) {
        
        if (mPlaylist_size) { //display current queue
            for (int i=0;i<mPlaylist_size;i++) {
                temp_playlist->entries[i].label=nil;
                temp_playlist->entries[i].fullpath=nil;
            }
            //[temp_playlist->playlist_name release];
        }
        free(temp_playlist);
        
        temp_playlist=NULL;
    }
    //
    
    [super viewDidAppear:animated];
}
/*
 +(natural_t) get_free_memory {
 mach_port_t host_port;
 mach_msg_type_number_t host_size;
 vm_size_t pagesize;
 host_port = mach_host_self();
 host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
 host_page_size(host_port, &pagesize);
 vm_statistics_data_t vm_stat;
 if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS) {
 NSLog(@"Failed to fetch vm statistics");
 return 0;
 }
 // Stats in bytes
 natural_t mem_free = vm_stat.free_count * pagesize;
 return mem_free;
 }*/

static int mOglView1Tap=0;
static int mOglView2Taps=0;
-(void) glViewOneFingerTwoTaps {
    mOglView2Taps=1;
    mOglView1Tap=0;
}

-(void) glViewOneFingerOneTap:(UITapGestureRecognizer *)gestureRecognizer {
    mOglView1Tap=2;
    CGPoint pt=[gestureRecognizer locationInView:m_oglView];
    oglTapX=pt.x;
    oglTapY=pt.y;
}

-(void) glViewPanGesture:(UIPanGestureRecognizer *)gestureRecognizer {
    CGPoint pt=[gestureRecognizer translationInView:m_oglView];
    movePx=pt.x;
    movePy=pt.y;
    if (gestureRecognizer.state==UIGestureRecognizerStateBegan) {
        movePxOld=movePx;
        movePyOld=movePy;
    }
}

-(void) glViewPan2Gesture:(UIPanGestureRecognizer *)gestureRecognizer {
    CGPoint pt=[gestureRecognizer translationInView:m_oglView];
    movePx2=pt.x;
    movePy2=pt.y;
    if (gestureRecognizer.state==UIGestureRecognizerStateBegan) {
        movePx2Old=movePx2;
        movePy2Old=movePy2;
    }
}

-(void) glViewPinchGesture:(UIPinchGestureRecognizer *)gestureRecognizer {
    CGFloat scale=gestureRecognizer.scale;
    movePinchScale=scale;
    if (gestureRecognizer.state==UIGestureRecognizerStateBegan) {
        movePinchScaleOld=movePinchScale;
    }
}

extern "C" int current_sample;

- (void)doFrame {
    static int framecpt=0;
	uint ww,hh;
	int nb_spectrum_bands;
	uint hasdrawnotes;
	char str_data[200];
	unsigned int cnote,cinst,ceff,cparam,cvol,endChan;
    int numRows,numRowsP,numRowsN;
	int i,j,k,l,note_avail,idx,startRow;
	int linestodraw,midline;
	ModPlugNote *currentNotes,*prevNotes,*nextNotes,*readNote;
	int size_chan;
    int shouldhide=0;
    int playerpos=[mplayer getCurrentPlayedBufferIdx];
    static float piano_posx=0;
    static float piano_posy=0;
    static float piano_posz=0;
    static float piano_rotx=0;
    static float piano_roty=0;
    float fxalpha=settings[GLOB_FXAlpha].detail.mdz_slider.slider_value;
    
	switch (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value) {
        case 1:
        case 4:
            size_chan=12*6;
            break;
        case 2:
        case 5:
            size_chan=6*6;
            break;
        case 3:
        case 6:
            size_chan=4*6;
            break;
    }
    
    framecpt++;  //TODO: check dependency / FPS (30, 60)
    
	
	if (self.mainView.hidden) return;
	if (m_oglView.hidden) return;
    if (coverflow.hidden==FALSE) return;
    
    if (oglViewFullscreen) fxalpha=1.0;
	
	if (!mFont) {
        NSString *fontPath;
        if (mScaleFactor==1) fontPath = [[NSBundle mainBundle] pathForResource:  @"consolas8" ofType: @"fnt"];
        else fontPath = [[NSBundle mainBundle] pathForResource:  @"consolas16" ofType: @"fnt"];
		mFont = new CFont([fontPath cStringUsingEncoding:1]);
	}
	if (!viewTapInfoStr[0]) viewTapInfoStr[0]= new CGLString("Exit Menu", mFont,(mScaleFactor>=2?2:mScaleFactor));
	if (!viewTapInfoStr[1]) viewTapInfoStr[1]= new CGLString("Off", mFont,(mScaleFactor>=2?2:mScaleFactor));
    if (!viewTapInfoStr[2]) viewTapInfoStr[2]= new CGLString("All Off", mFont,(mScaleFactor>=2?2:mScaleFactor));
	
	
    
	//get ogl view dimension
	ww=m_oglView.frame.size.width;
	hh=m_oglView.frame.size.height;
    
    if (mOglView1Tap) mOglView1Tap--;
    
    movePxMOD+=movePx-movePxOld;
    movePyMOD+=movePy-movePyOld;
    movePxMID+=movePx-movePxOld;
    movePyMID+=movePy-movePyOld;
    
    movePxFXPiano+=movePx-movePxOld;
    movePyFXPiano+=movePy-movePyOld;
    movePx2FXPiano+=movePx2-movePx2Old;
    movePy2FXPiano+=movePy2-movePy2Old;
    
    movePinchScaleFXPiano+=movePinchScale-movePinchScaleOld;
    
    movePxOld=movePx;
    movePyOld=movePy;
    movePx2Old=movePx2;
    movePy2Old=movePy2;
    movePinchScaleOld=movePinchScale;
    
    
    if ((mplayer.mPatternDataAvail)&&(settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value)) {//pattern display
        if (visibleChan<mplayer.numChannels) {
            if (movePxMOD>0) movePxMOD=0;
            if (movePxMOD<-(mplayer.numChannels-visibleChan+1)*size_chan) movePxMOD=-(mplayer.numChannels-visibleChan+1)*size_chan;
            startChan=-movePxMOD/size_chan;
            
        } else movePxMOD=0;
        
    }
    
    
    if (((mplayer.mPatternDataAvail)||(mplayer.mPlayType==MMP_TIMIDITY))&&(settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value)) {
        int moveRPx,moveRPy;
        int note_fx_linewidth;
        
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==2) {
            moveRPx=movePyMID;
            moveRPy=-movePxMID;
            note_fx_linewidth=ww/tim_midifx_note_range;
        } else {
            moveRPx=movePxMID;
            moveRPy=movePyMID;
            note_fx_linewidth=hh/tim_midifx_note_range;
        }
        //            if (touch_cpt==1) moveRPx=0; //hack to reset zoom on touch start
        
        if (moveRPx>32) {
            tim_midifx_note_range+=12;
            moveRPx=0;
        }
        if (moveRPx<-32) {
            tim_midifx_note_range-=12;
            moveRPx=0;
        }
        
        if  (tim_midifx_note_range<12*4) {//min is 4 Octaves
            tim_midifx_note_range=12*4;
        }
        if (tim_midifx_note_range>128) tim_midifx_note_range=128; //note is a 8bit, so 256 is a max
        
        if (tim_midifx_note_range<128) {
            tim_midifx_note_offset=((128-tim_midifx_note_range)>>1)*note_fx_linewidth;
        }
        
        if (tim_midifx_note_range<128) {
            int maxofs=(128-tim_midifx_note_range)*note_fx_linewidth;
            tim_midifx_note_offset=moveRPy;
            if (tim_midifx_note_offset<0) tim_midifx_note_offset=0;
            if (tim_midifx_note_offset>maxofs) tim_midifx_note_offset=maxofs;
        } else tim_midifx_note_offset=0;
        moveRPy=tim_midifx_note_offset;
        
        if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value==2) {
            movePxMID=-moveRPy;
            movePyMID=moveRPx;
            note_fx_linewidth=ww/tim_midifx_note_range;
        } else {
            movePxMID=moveRPx;
            movePyMID=moveRPy;
            note_fx_linewidth=hh/tim_midifx_note_range;
        }
    }
		if (mOglView2Taps) { //double tap: fullscreen switch
            mOglView2Taps=0;
			oglViewFullscreen^=1;
			oglViewFullscreenChanged=1;
			[self shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientationHV];
		}

    
	//check for click
	if (mOglView1Tap==1) {
		mOglView1Tap=0;
		
		if (viewTapHelpShow==1) {  //Main Menu
			viewTapHelpShow=0;
            viewTapHelpShowMode=1;
            int menu_cell_size=(ww<hh?ww:hh);
            int tlx=oglTapX;
			int tly=oglTapY-(hh-menu_cell_size)/2;
            if (tly>=0) {
            int touched_cellX=tlx*4/menu_cell_size;
            int touched_cellY=tly*4/menu_cell_size;
            int touched_coord=(touched_cellX<<4)|(touched_cellY);
            
            if (touched_coord==0x00) {
				int val=settings[GLOB_FX1].detail.mdz_boolswitch.switch_value;
				val++;
				if (val>=2) val=0;
				settings[GLOB_FX1].detail.mdz_boolswitch.switch_value=val;
			} else if (touched_coord==0x10) {
                viewTapHelpShow=2;
                viewTapHelpShowMode=2;
                viewTapHelpShow_SubStart=SUBMENU0_START;
                viewTapHelpShow_SubNb=SUBMENU0_SIZE;
			} else if (touched_coord==0x20) {
                viewTapHelpShow=2;
                viewTapHelpShowMode=2;
                viewTapHelpShow_SubStart=SUBMENU1_START;
                viewTapHelpShow_SubNb=SUBMENU1_SIZE;
			} else if (touched_coord==0x30) {
                viewTapHelpShow=2;
                viewTapHelpShowMode=2;
                viewTapHelpShow_SubStart=SUBMENU2_START;
                viewTapHelpShow_SubNb=SUBMENU2_SIZE;
			} else if (touched_coord==0x01) {
				viewTapHelpShow=2;
                viewTapHelpShowMode=2;
                viewTapHelpShow_SubStart=SUBMENU3_START;
                viewTapHelpShow_SubNb=SUBMENU3_SIZE;
			} else if (touched_coord==0x11) {
				int val=settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value;
				val++;
				if (val>=2) val=0;
				settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=val;
			} else if (touched_coord==0x21) {
				viewTapHelpShow=2;
                viewTapHelpShowMode=2;
                viewTapHelpShow_SubStart=SUBMENU4_START;
                viewTapHelpShow_SubNb=SUBMENU4_SIZE;
			} else if (touched_coord==0x31) {
                viewTapHelpShow=2;
                viewTapHelpShowMode=2;
                viewTapHelpShow_SubStart=SUBMENU5_START;
                viewTapHelpShow_SubNb=SUBMENU5_SIZE;
			} else if (touched_coord==0x02) {
				int val=settings[GLOB_FX4].detail.mdz_boolswitch.switch_value;
				val++;
				if (val>=2) val=0;
				settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=val;
                settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
                settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
                settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
			} else if (touched_coord==0x12) {
				viewTapHelpShow=2;
                viewTapHelpShowMode=2;
                viewTapHelpShow_SubStart=SUBMENU6_START;
                viewTapHelpShow_SubNb=SUBMENU6_SIZE;
			} else if (touched_coord==0x22) {
                viewTapHelpShow=2;
                viewTapHelpShowMode=2;
                viewTapHelpShow_SubStart=SUBMENU7_START;
                viewTapHelpShow_SubNb=SUBMENU7_SIZE;
			} else if (touched_coord==0x32) {
                viewTapHelpShow=2;
                viewTapHelpShowMode=2;
                viewTapHelpShow_SubStart=SUBMENU8_START;
                viewTapHelpShow_SubNb=SUBMENU8_SIZE;
			} else if (touched_coord==0x03) {
                shouldhide=1;
			} else if (touched_coord==0x23) {
                settings[GLOB_FX1].detail.mdz_boolswitch.switch_value=0;
                settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
                settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
                settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
                settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value=0;;
                settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=0;
                settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=0;
                settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=0;
                settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=0;
                settings[GLOB_FXPiano].detail.mdz_switch.switch_value=0;
                settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=0;
			}
            }
            
		} else if (viewTapHelpShow==2) { //sub menu
            viewTapHelpShow=0;
            viewTapHelpShowMode=2;
            int menu_cell_size=(ww<hh?ww:hh);
            int tlx=oglTapX;
			int tly=oglTapY-(hh-menu_cell_size)/2;
            if (tly>=0) {
            int touched_cellX=tlx*4/menu_cell_size;
            int touched_cellY=tly*4/menu_cell_size;
            int touched_coord=(touched_cellX<<4)|(touched_cellY);
            if (touched_coord==0x00) {
                switch (viewTapHelpShow_SubStart) {
                    case SUBMENU0_START: //FX2
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU1_START://4: //FX3
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU2_START://8: //Spectrum
                        settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU3_START://11: //Oscillo
                        settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU4_START://14: //MOD Pattern
                        settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=0;
                        movePxMOD=movePyMOD=0;
                        break;
                    case SUBMENU5_START://21: //MIDI Pattern
                        settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=0;
                        movePxMID=movePyMID=0;
                        break;
                    case SUBMENU6_START://24: //3D Sphere/Torus
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU7_START://27: //Piano
                        settings[GLOB_FXPiano].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU8_START://32: //Spectrum3D
                        settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=0;
                        break;
                }
			} else if (touched_coord==0x10) {
                switch (viewTapHelpShow_SubStart) {
                    case SUBMENU0_START://0: //FX2
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=1;
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU1_START://4: //FX3
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=1;
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU2_START://8: //Spectrum
                        settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=1;
                        break;
                    case SUBMENU3_START://11: //Oscillo
                        settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=1;
                        break;
                    case SUBMENU4_START://14: //MOD Pattern
                        settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=1;
                        size_chan=12*6;
                        movePxMOD=movePyMOD=0;
                        visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN+size_chan-1)/size_chan;
                        if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
                        if (startChan<0) startChan=0;
                        break;
                    case SUBMENU5_START://21: //MIDI Pattern
                        settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=1;
                        movePxMID=movePyMID=0;
                        break;
                    case SUBMENU6_START://24: //3D Sphere/Torus
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=1;
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                        break;
                    case SUBMENU7_START://27: //Piano
                        settings[GLOB_FXPiano].detail.mdz_switch.switch_value=1;
                        break;
                    case SUBMENU8_START://32: //Spectrum3D
                        settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=1;
                        break;
                }
            } else if (touched_coord==0x20) {
                switch (viewTapHelpShow_SubStart) {
                    case SUBMENU0_START://0: //FX2
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=2;
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU1_START://4: //FX3
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=2;
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU2_START://8: //Spectrum
                        settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value=2;
                        break;
                    case SUBMENU3_START://11: //Oscillo
                        settings[GLOB_FXOscillo].detail.mdz_switch.switch_value=2;
                        break;
                    case SUBMENU4_START://14: //MOD Pattern
                        settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=2;
                        size_chan=6*6;
                        movePxMOD=movePyMOD=0;
                        visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN+size_chan-1)/size_chan;
                        if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
                        if (startChan<0) startChan=0;
                        break;
                    case SUBMENU5_START://21: //MIDI Pattern
                        settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value=2;
                        movePxMID=movePyMID=0;
                        break;
                    case SUBMENU6_START://24: //3D Sphere/Torus
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=2;
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                        break;
                    case SUBMENU7_START://27: //Piano
                        settings[GLOB_FXPiano].detail.mdz_switch.switch_value=2;
                        break;
                    case SUBMENU8_START://32: //Spectrum3D
                        settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=2;
                        break;
                }
            } else if (touched_coord==0x30) {
                switch (viewTapHelpShow_SubStart) {
                    case SUBMENU0_START://0: //FX2
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=3;
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU1_START://4: //FX3
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=3;
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU4_START://14: //MOD Pattern
                        settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=3;
                        size_chan=4*6;
                        movePxMOD=movePyMOD=0;
                        visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN+size_chan-1)/size_chan;
                        if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
                        if (startChan<0) startChan=0;
                        break;
                        
                    case SUBMENU7_START://27: //Piano
                        settings[GLOB_FXPiano].detail.mdz_switch.switch_value=3;
                        break;
                    case SUBMENU8_START://32: //Spectrum3D
                        settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value=3;
                }
            } else if (touched_coord==0x01) {
                switch (viewTapHelpShow_SubStart) {
                    case SUBMENU0_START://0: //FX2
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=4;
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU4_START://14: //MOD Pattern
                        settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=4;
                        size_chan=12*6;
                        movePxMOD=movePyMOD=0;
                        visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN+size_chan-1)/size_chan;
                        if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
                        if (startChan<0) startChan=0;
                        break;
                    case SUBMENU7_START://27: //Piano
                        settings[GLOB_FXPiano].detail.mdz_switch.switch_value=4;
                        break;
                }
            } else if (touched_coord==0x11) {
                switch (viewTapHelpShow_SubStart) {
                    case SUBMENU0_START://0: //FX2
                        settings[GLOB_FX2].detail.mdz_switch.switch_value=5;
                        settings[GLOB_FX3].detail.mdz_switch.switch_value=0;
                        settings[GLOB_FX4].detail.mdz_boolswitch.switch_value=0;
                        settings[GLOB_FX5].detail.mdz_switch.switch_value=0;
                        break;
                    case SUBMENU4_START://14: //MOD Pattern
                        settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=5;
                        size_chan=6*6;
                        movePxMOD=movePyMOD=0;
                        visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN+size_chan-1)/size_chan;
                        if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
                        if (startChan<0) startChan=0;
                        break;
                }
            } else if (touched_coord==0x21) {
                switch (viewTapHelpShow_SubStart) {
                    case SUBMENU4_START://14: //MOD Pattern
                        settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value=6;
                        size_chan=4*6;
                        movePxMOD=movePyMOD=0;
                        visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN+size_chan-1)/size_chan;
                        if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
                        if (startChan<0) startChan=0;
                        break;
                }
            }
            }
        } else {viewTapHelpShow=1;viewTapHelpShowMode=1;}
	}
    
    if (mOglViewIsHidden) {
        m_oglView.hidden=YES;
        return;
    }
	//check if current mod is ended or not
	/*if (mplayer.bGlobalAudioPause==2) {
     //mod ended => do nothing
     return;
     }
     */
	//get ogl context & bind
	[EAGLContext setCurrentContext:m_oglContext];
	[m_oglView bind];
	
	hasdrawnotes=0;
	
	//update spectrum data
	if ( ((settings[GLOB_FX1].detail.mdz_boolswitch.switch_value)||
		  (settings[GLOB_FX2].detail.mdz_switch.switch_value)||
		  (settings[GLOB_FX3].detail.mdz_switch.switch_value)||
          (settings[GLOB_FX4].detail.mdz_boolswitch.switch_value)||
          (settings[GLOB_FX5].detail.mdz_switch.switch_value)||
		  (settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value)||
		  (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value)||
          (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value))  ) {
		//compute new spectrum data
		if ([mplayer isPlaying]){
            //FFT: build audio buffer
			short int **snd_buffer;
			int cur_pos;
			snd_buffer=[mplayer buffer_ana_cpy];
			cur_pos=[mplayer getCurrentPlayedBufferIdx];
			short int *curBuffer=snd_buffer[cur_pos];
            // COMPUTE FFT
#define SOUND_BUFFER_SIZE_SAMPLE_SPECTRUM 1024
    /////////////////////////////////////////            
            //Number of Samples for input(time domain)/output(frequency domain)
            int numSamples = SOUND_BUFFER_SIZE_SAMPLE_SPECTRUM;
            int idx;
            //Fill Input Array with Left channel
            for (int i=0; i<numSamples; i++) {
                fft_time[i]=(float)curBuffer[i*2]/32768.0f;
            }
            memset(fft_frequencyAvg,0,sizeof(float)*SPECTRUM_BANDS);
            memset(fft_freqAvgCount,0,sizeof(int)*SPECTRUM_BANDS);
            fftAccel->doFFTReal(fft_time, fft_frequency, numSamples);
            
            const float log2FrameSize = log2f(numSamples);
            
            /*for (int i=1; i<numSamples/2; i++) {
                
                //idx=6*(i-1)*2*SPECTRUM_BANDS/numSamples;
                idx=SPECTRUM_BANDS*log2f(i)/log2FrameSize;
                if (idx<SPECTRUM_BANDS) {
                    fft_frequencyAvg[idx]=max(fft_frequencyAvg[idx],fft_frequency[i]);
                    fft_freqAvgCount[idx]++;
                }
            }*/
            int lowfreq,highfreq,tmpfreq;
            float sum;
            
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
                    
                //NSLog(@"/idx %d || %d.%d || %d.%d || %f",i,lowfreq,highfreq,(lowfreq)*44100/(SOUND_BUFFER_SIZE_SAMPLE_SPECTRUM),(highfreq)*44100/(SOUND_BUFFER_SIZE_SAMPLE_SPECTRUM),fft_frequencyAvg[i]);
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
            
            /*for (int i=0;i<SPECTRUM_BANDS;i++) {
                lowfreq=(float)numSamples/2/powf(2.f,log2FrameSize-i*log2FrameSize/SPECTRUM_BANDS)+1;
                highfreq=(float)numSamples/2/powf(2.f,log2FrameSize-(i+1)*log2FrameSize/SPECTRUM_BANDS)+1;
                if (highfreq>=numSamples/2) highfreq=numSamples/2-1;
                if (lowfreq<numSamples/2) {
                    sum=0;
                    for (int k=lowfreq;k<highfreq;k++) {
                        fft_frequencyAvg[i]=max(fft_frequencyAvg[i],fft_frequency[k]);
                        //sum=sum+fft_frequency[k];
                    }
                    //sum=sum/(float)(highfreq-lowfreq+1);
                    //sum*=(float)powf(i,1.5f)+1;
                    //fft_frequencyAvg[i]=sum;
                    fft_frequencyAvg[i]*=(float)powf(i,1.5f)+1;
                }
            }*/
            
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
                
                //NSLog(@"Ridx %d || %d.%d || %d.%d || %f",i,lowfreq,highfreq,(lowfreq)*44100/(SOUND_BUFFER_SIZE_SAMPLE_SPECTRUM),(highfreq)*44100/(SOUND_BUFFER_SIZE_SAMPLE_SPECTRUM),fft_frequencyAvg[i]);
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
                real_beatDetectedL[i]=0;
                real_beatDetectedR[i]=0;
                
                //APPLY THRESHOLDS (MIN VALUE & FACTOR/AVERAGE)
                if ((sumL>BEAT_THRESHOLD_MIN)&&(newSpecL>sumL*BEAT_THRESHOLD_FACTOR)) real_beatDetectedL[i]=1;
                if ((sumR>BEAT_THRESHOLD_MIN)&&(newSpecR>sumR*BEAT_THRESHOLD_FACTOR)) real_beatDetectedR[i]=1;                
            }            
    /////////////////////////////////////////
		}
	}
	
    int detail_lvl=settings[GLOB_FXLOD].detail.mdz_switch.switch_value;
    if (settings[GLOB_FX4].detail.mdz_boolswitch.switch_value) detail_lvl=0; //force low for fx4
    
	switch (detail_lvl) {
		case 2:
			nb_spectrum_bands=SPECTRUM_BANDS;
			for (int i=0;i<SPECTRUM_BANDS;i++) {
				real_spectrumL[i]=oreal_spectrumL[i];
				real_spectrumR[i]=oreal_spectrumR[i];
            }
            break;
		case 1:
			nb_spectrum_bands=SPECTRUM_BANDS/2;
            for (int i=0;i<SPECTRUM_BANDS/2;i++) {
				real_spectrumL[i]=max2(oreal_spectrumL[i*2],oreal_spectrumL[i*2+1]);
				real_spectrumR[i]=max2(oreal_spectrumR[i*2],oreal_spectrumR[i*2+1]);
                real_beatDetectedL[i]=max2(real_beatDetectedL[i*2],real_beatDetectedL[i*2+1]);
				real_beatDetectedR[i]=max2(real_beatDetectedR[i*2],real_beatDetectedR[i*2+1]);
			}
			break;
			
		case 0:
			nb_spectrum_bands=SPECTRUM_BANDS/4;
            for (int i=0;i<SPECTRUM_BANDS/4;i++) {
                real_spectrumL[i]=max4(oreal_spectrumL[i*4],oreal_spectrumL[i*4+1],oreal_spectrumL[i*4+2],oreal_spectrumL[i*4+3]);
                real_spectrumR[i]=max4(oreal_spectrumR[i*4],oreal_spectrumR[i*4+1],oreal_spectrumR[i*4+2],oreal_spectrumR[i*4+3]);
				real_beatDetectedL[i]=max4(real_beatDetectedL[i*4],real_beatDetectedL[i*4+1],real_beatDetectedL[i*4+2],real_beatDetectedL[i*4+3]);
				real_beatDetectedR[i]=max4(real_beatDetectedR[i*4],real_beatDetectedR[i*4+1],real_beatDetectedR[i*4+2],real_beatDetectedR[i*4+3]);
			}
			break;
	}
	
	angle+=(float)4.0f;
	if (settings[GLOB_FX1].detail.mdz_boolswitch.switch_value) {
		/* Update Angle Based On The Clock */
        
		fxRadial(0,ww,hh,real_spectrumL,real_spectrumR,nb_spectrum_bands,fxalpha);
	} else {
		glClearColor(0.0f, 0.0f, 0.0f, fxalpha);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
    
	RenderUtils::SetUpOrtho(0,ww,hh);
    
	if (([mplayer isPlaying])&&
        ((settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value)||
         (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value)||
         (settings[GLOB_FXPiano].detail.mdz_switch.switch_value)) ) {
            
            int display_note_mode=(settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value-1);
            if (display_note_mode>=3) display_note_mode-=3;
            
            if ((mplayer.mPlayType==MMP_TIMIDITY)&&(settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value)) { //Timidity
                playerpos=(playerpos+MIDIFX_OFS)%SOUND_BUFFER_NB;
                RenderUtils::DrawMidiFX(tim_notes_cpy[playerpos],ww,hh,settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value-1,tim_midifx_note_range,tim_midifx_note_offset,MIDIFX_OFS*4,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value);
                
                if (mHeader) delete mHeader;
                mHeader=nil;
                if (DEBUG_INFOS) {
                    sprintf(str_data,"%d/%d",tim_voicenb_cpy[playerpos],(int)(settings[TIM_Polyphony].detail.mdz_slider.slider_value));
                    mHeader= new CGLString(str_data, mFont,(mScaleFactor>=2?2:mScaleFactor));
                    glPushMatrix();
                    glTranslatef(ww-strlen(str_data)*6-2, 5.0f, 0.0f);
                    //glScalef(1.58f, 1.58f, 1.58f);
                    mHeader->Render(0);
                    glPopMatrix();
                }
            } else if (mplayer.mPatternDataAvail) { //Modplug
                if ((settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value)||(settings[GLOB_FXPiano].detail.mdz_switch.switch_value)) {
                    playerpos=(playerpos+MIDIFX_OFS)%SOUND_BUFFER_NB;
                    
                    int *pat,*row;
                    pat=[mplayer playPattern];
                    row=[mplayer playRow];
                    currentPattern=pat[playerpos];
                    currentRow=row[playerpos];
                    
                    currentNotes=ModPlug_GetPattern(mplayer.mp_file,currentPattern,(unsigned int*)(&numRows));
                    idx=startRow*mplayer.numChannels+startChan;
                    
                    for (int i=0;i<mplayer.numChannels;i++) tim_notes_cpy[playerpos][i]=0;
                    
                    if (currentNotes) {
                        idx=currentRow*mplayer.numChannels;
                        for (j=0;j<mplayer.numChannels;j++,idx++)  {
                            if (currentNotes[idx].Note) {
                                cnote=(currentNotes[idx].Note-13)&127;
                                cinst=(currentNotes[idx].Instrument)&0x1F;
                                cvol=(currentNotes[idx].Volume)&0xFF;
                                if (!cvol) cvol=63;
                                
                                tim_notes_cpy[playerpos][j]=cnote|(cinst<<8)|(cvol<<16)|((1<<1)<<24); //VOICE_ON : 1<<1
                            }
                        }
                    }
                    
                    if (settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value) RenderUtils::DrawMidiFX(tim_notes_cpy[playerpos],ww,hh,settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value-1,tim_midifx_note_range,tim_midifx_note_offset,MIDIFX_OFS*4,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value);
                }
                
                if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value) {
                    linestodraw=(hh-NOTES_DISPLAY_TOPMARGIN+11)/12; //+11 => draw even if halfed for last line
                    midline=linestodraw>>1;
                    
                    for (i=0;i<linestodraw;i++) {
                        if (mText[i]) delete mText[i];
                        mText[i]=nil;
                    }
                    for (i=0;i<linestodraw;i++) {
                        if (mTextLine[i]) delete mTextLine[i];
                        mTextLine[i]=nil;
                    }
                    if (mHeader) delete mHeader;
                    mHeader=nil;
                    
                    
                    
                    int *pat,*row;
                    int playerpos;
                    //            int *playerOffset;
                    pat=[mplayer playPattern];
                    row=[mplayer playRow];
                    //            playerOffset=[mplayer playOffset];
                    playerpos=[mplayer getCurrentPlayedBufferIdx];
                    currentPattern=pat[playerpos];
                    currentRow=row[playerpos];
                    
                    endChan=startChan+visibleChan;
                    if (endChan>mplayer.numChannels) endChan=mplayer.numChannels;
                    else if (endChan<mplayer.numChannels) endChan++;
                    startRow=currentRow-midline;
                    
                    int channelVolumeData[SOUND_MAXMOD_CHANNELS];
                    unsigned char *volData=[mplayer playVolData];
                    for (int i=0;i<endChan-startChan;i++) {
                        channelVolumeData[i]=volData[playerpos*SOUND_MAXMOD_CHANNELS+i+startChan];
                    }
                    
                    currentNotes=ModPlug_GetPattern(mplayer.mp_file,currentPattern,(unsigned int*)(&numRows));
                    if (currentPattern>0) prevNotes=ModPlug_GetPattern(mplayer.mp_file,currentPattern-1,(unsigned int*)(&numRowsP));
                    else prevNotes=nil;
                    if (currentPattern<mplayer.numPatterns-1) nextNotes=ModPlug_GetPattern(mplayer.mp_file,currentPattern+1,(unsigned int*)(&numRowsN));
                    else nextNotes=nil;
                    idx=startRow*mplayer.numChannels+startChan;
                    
                    RenderUtils::DrawChanLayout(ww,hh,display_note_mode,endChan-startChan+1,((int)(movePxMOD)%size_chan));
                    
                    if (currentNotes) {
                        hasdrawnotes=1;
                        l=0;
                        for (i=startRow;i<startRow+linestodraw;i++) {
                            note_avail=0;
                            if (i<0) {
                                if ((prevNotes)&&((numRowsP+i)>=0)) {
                                    if (numRowsP+i>=0) {
                                        note_avail=1;
                                        idx=(numRowsP+i)*mplayer.numChannels+startChan;
                                        readNote=prevNotes;
                                    }
                                }
                            } else if (currentNotes&&(i<numRows)) {
                                note_avail=1;
                                idx=i*mplayer.numChannels+startChan;
                                readNote=currentNotes;
                            } else {
                                if ((nextNotes)&&((i-numRows)<numRowsN)) {
                                    note_avail=1;
                                    idx=(i-numRows)*mplayer.numChannels+startChan;
                                    readNote=nextNotes;
                                }
                            }
                            if (note_avail) {
                                k=0;
                                switch (display_note_mode) {
                                    case 0: //all infos
                                        for (j=0;j<endChan-startChan;j++)  {
                                            cnote=currentNotes[idx].Note;
                                            cinst=currentNotes[idx].Instrument;
                                            ceff=currentNotes[idx].Effect;
                                            cparam=currentNotes[idx].Parameter;
                                            cvol=currentNotes[idx].Volume;
                                            if (cnote) {
                                                str_data[k++]=note2charA[(cnote-13)%12];
                                                str_data[k++]=note2charB[(cnote-13)%12];
                                                str_data[k++]=(cnote-13)/12+'0';
                                            } else {
                                                str_data[k++]='.';str_data[k++]='.';str_data[k++]='.';
                                            }
                                            if (cinst) {
                                                str_data[k++]=dec2hex[(cinst>>4)&0xF];
                                                str_data[k++]=dec2hex[cinst&0xF];
                                            } else {
                                                str_data[k++]='.';str_data[k++]='.';
                                            }
                                            if (cvol) {
                                                str_data[k++]=dec2hex[(cvol>>4)&0xF];
                                                str_data[k++]=dec2hex[cvol&0xF];
                                            } else {
                                                str_data[k++]='.';str_data[k++]='.';
                                            }
                                            if (ceff) {
                                                str_data[k++]='A'+ceff;
                                            } else {
                                                str_data[k++]='.';
                                            }
                                            if (cparam) {
                                                str_data[k++]=dec2hex[(cparam>>4)&0xF];
                                                str_data[k++]=dec2hex[cparam&0xF];
                                            } else {
                                                str_data[k++]='.';str_data[k++]='.';
                                            }
                                            str_data[k++]=' ';str_data[k++]=' ';
                                            idx++;
                                        }
                                        break;
                                    case 1: //note + instru
                                        for (j=0;j<endChan-startChan;j++)  {
                                            cnote=currentNotes[idx].Note;
                                            cinst=currentNotes[idx].Instrument;
                                            if (cnote) {
                                                str_data[k++]=note2charA[(cnote-13)%12];
                                                str_data[k++]=note2charB[(cnote-13)%12];
                                                str_data[k++]=(cnote-13)/12+'0';
                                            } else {
                                                str_data[k++]='.';str_data[k++]='.';str_data[k++]='.';
                                            }
                                            if (cinst) {
                                                str_data[k++]=dec2hex[(cinst>>4)&0xF];
                                                str_data[k++]=dec2hex[cinst&0xF];
                                            } else {
                                                str_data[k++]='.';str_data[k++]='.';
                                            }
                                            str_data[k++]=' ';
                                            idx++;
                                        }
                                        break;
                                    case 2: //only note
                                        for (j=0;j<endChan-startChan;j++)  {
                                            cnote=currentNotes[idx].Note;
                                            if (cnote) {
                                                str_data[k++]=note2charA[(cnote-13)%12];
                                                str_data[k++]=note2charB[(cnote-13)%12];
                                                str_data[k++]=(cnote-13)/12+'0';
                                            } else {
                                                str_data[k++]='.';str_data[k++]='.';str_data[k++]='.';
                                            }
                                            str_data[k++]=' ';
                                            idx++;
                                        }
                                        break;
                                }
                                str_data[k]=0;
                                mText[l++] = new CGLString(str_data, mFont,(mScaleFactor>=2?2:mScaleFactor));
                                
                            } else {
                                mText[l++] = NULL;
                                
                            }
                        }
                        str_data[2]=0;
                        for (l=0;l<linestodraw;l++) {
                            i=l+startRow;
                            if (mText[l]) {
                                glPushMatrix();
                                glTranslatef(NOTES_DISPLAY_LEFTMARGIN+((int)(movePxMOD)%size_chan), hh-NOTES_DISPLAY_TOPMARGIN-l*12/*+currentYoffset*/, 0.0f);
                                
                                if ((i<0)||(i>=numRows)) mText[l]->Render(10+display_note_mode);
                                else mText[l]->Render(3+display_note_mode);
                                glPopMatrix();
                            }
                            if ((i<0)&&prevNotes) {
                                str_data[0]=dec2hex[((numRowsP+i)>>4)&0xF];
                                str_data[1]=dec2hex[(numRowsP+i)&0xF];
                            } else if (i<numRows) {
                                str_data[0]=dec2hex[(i>>4)&0xF];
                                str_data[1]=dec2hex[i&0xF];
                            } else if (nextNotes) {
                                str_data[0]=dec2hex[((i-numRows)>>4)&0xF];
                                str_data[1]=dec2hex[(i-numRows)&0xF];
                            }
                            mTextLine[l]= new CGLString(str_data, mFont,(mScaleFactor>=2?2:mScaleFactor));
                            glPushMatrix();
                            glTranslatef(8.0f, hh-NOTES_DISPLAY_TOPMARGIN-l*12/*+currentYoffset*/, 0.0f);
                            mTextLine[l]->Render(1+(l&1));
                            glPopMatrix();
                        }
                        
                        
                    }
                    //draw header + fps
                    memset(str_data,32,171);   //max 171 chars => 1026pix (6pix/char)
                    str_data[12*16]=0;
                    float xofs=0;
                    switch (display_note_mode) {
                        case 0:
                            for (j=startChan;j<endChan;j++) {
                                str_data[(j-startChan)*12+7]='0'+(j+1)/10;
                                str_data[(j-startChan)*12+8]='0'+(j+1)%10;
                            }
                            str_data[(endChan-1-startChan)*12+9]=0;
                            xofs=5.0f;
                            break;
                        case 1:
                            for (j=startChan;j<endChan;j++) {
                                str_data[(j-startChan)*6+5]='0'+(j+1)/10;
                                str_data[(j-startChan)*6+6]='0'+(j+1)%10;
                            }
                            str_data[(endChan-1-startChan)*6+7]=0;
                            xofs=5.0f;
                            break;
                        case 2:
                            for (j=startChan;j<endChan;j++) {
                                str_data[(j-startChan)*4+4]='0'+(j+1)/10;
                                str_data[(j-startChan)*4+5]='0'+(j+1)%10;
                            }
                            str_data[(endChan-1-startChan)*4+6]=0;
                            xofs=8.0f;
                            break;
                    }
                    mHeader= new CGLString(str_data, mFont,(mScaleFactor>=2?2:mScaleFactor));
                    glPushMatrix();
                    glTranslatef(xofs+((int)(movePxMOD)%size_chan), hh-12, 0.0f);
                    //glScalef(1.58f, 1.58f, 1.58f);
                    mHeader->Render(0);
                    glPopMatrix();
                    
                    if (settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value>=3) RenderUtils::DrawChanLayoutAfter(ww,hh,display_note_mode,channelVolumeData,endChan-startChan,((int)(movePxMOD)%size_chan));
                    else RenderUtils::DrawChanLayoutAfter(ww,hh,display_note_mode,NULL,endChan-startChan,((int)(movePxMOD)%size_chan));
                }
            }
        }
	
	if ([mplayer isPlaying]){
		short int **snd_buffer;
		int cur_pos;
		int pos_fx;
		snd_buffer=[mplayer buffer_ana_cpy];
		cur_pos=[mplayer getCurrentPlayedBufferIdx];
		short int *curBuffer=snd_buffer[cur_pos];
		
		pos_fx=0;
		if ((settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value)&&(settings[GLOB_FXOscillo].detail.mdz_switch.switch_value)) pos_fx=1;
		if ((settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value)&&(settings[GLOB_FXOscillo].detail.mdz_switch.switch_value)) pos_fx=1;
		if ((settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value)&&(settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value)) pos_fx=1;
		if (hasdrawnotes) pos_fx=1;
		if (settings[GLOB_FX1].detail.mdz_boolswitch.switch_value) pos_fx=1;
		
		if (settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value) //RenderUtils::DrawSpectrum(real_spectrumL,real_spectrumR,ww,hh,hasdrawnotes,settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value-1,pos_fx,nb_spectrum_bands);
            RenderUtils::DrawSpectrum3DBarFlat(real_spectrumL,real_spectrumR,ww,hh,
                                  settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value,nb_spectrum_bands);
		if (settings[GLOB_FXBeat].detail.mdz_boolswitch.switch_value) RenderUtils::DrawBeat(real_beatDetectedL,real_beatDetectedR,ww,hh,hasdrawnotes,pos_fx,nb_spectrum_bands);
		if (settings[GLOB_FXOscillo].detail.mdz_switch.switch_value) RenderUtils::DrawOscillo(curBuffer,SOUND_BUFFER_SIZE_SAMPLE,ww,hh,hasdrawnotes,settings[GLOB_FXOscillo].detail.mdz_switch.switch_value,pos_fx);
	}
    
	if ([mplayer isPlaying]){
		if (settings[GLOB_FX2].detail.mdz_switch.switch_value) {
            if (settings[GLOB_FX2].detail.mdz_switch.switch_value<4) RenderUtils::DrawSpectrum3D(real_spectrumL,real_spectrumR,ww,hh,angle,settings[GLOB_FX2].detail.mdz_switch.switch_value,nb_spectrum_bands);
            else RenderUtils::DrawSpectrumLandscape3D(real_spectrumL,real_spectrumR,ww,hh,angle,settings[GLOB_FX2].detail.mdz_switch.switch_value-3,nb_spectrum_bands);
        } else if (settings[GLOB_FX3].detail.mdz_switch.switch_value) {
            RenderUtils::DrawSpectrum3DMorph(real_spectrumL,real_spectrumR,ww,hh,angle,settings[GLOB_FX3].detail.mdz_switch.switch_value,nb_spectrum_bands);
        } else if (settings[GLOB_FX4].detail.mdz_boolswitch.switch_value) {
            renderFluid(ww, hh, real_beatDetectedL, real_beatDetectedR, real_spectrumL, real_spectrumR, nb_spectrum_bands, 0, (unsigned char)(fxalpha*255));
        } else if (settings[GLOB_FX5].detail.mdz_switch.switch_value) {
            RenderUtils::DrawSpectrum3DSphere(real_spectrumL,real_spectrumR,ww,hh,angle,settings[GLOB_FX5].detail.mdz_switch.switch_value,nb_spectrum_bands/2);
        }
        
        if (settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value) {
            int mirror=1;
            if (settings[GLOB_FX2].detail.mdz_switch.switch_value) mirror=0;
            if (settings[GLOB_FX3].detail.mdz_switch.switch_value) mirror=0;
            if (settings[GLOB_FX5].detail.mdz_switch.switch_value) mirror=0;
            if (settings[GLOB_FXPiano].detail.mdz_switch.switch_value) mirror=0;
            RenderUtils::DrawSpectrum3DBar(real_spectrumL,real_spectrumR,ww,hh,angle,
                    settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value,nb_spectrum_bands,mirror);
        }
        
        if (settings[GLOB_FXPiano].detail.mdz_switch.switch_value) {
            int playerpos=[mplayer getCurrentPlayedBufferIdx];
            playerpos=(playerpos+MIDIFX_OFS)%SOUND_BUFFER_NB;
            switch (settings[GLOB_FXPiano].detail.mdz_switch.switch_value) {
                case 1:
                    RenderUtils::DrawPiano3D(tim_notes_cpy[playerpos],ww,hh,MIDIFX_OFS*2,1,0,0,0,0,0,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value);
                    break;
                case 2:
                    RenderUtils::DrawPiano3DWithNotesWall(tim_notes_cpy[playerpos],ww,hh,MIDIFX_OFS*4,1,0,0,0,0,0,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value,settings[GLOB_FXLOD].detail.mdz_switch.switch_value);
                    break;
                case 3:
                    if (movePinchScaleFXPiano<-0/4) movePinchScaleFXPiano=-0/4;
                    if (movePinchScaleFXPiano>9.0/4) movePinchScaleFXPiano=9.0/4;
                    piano_rotx=movePyFXPiano;
                    piano_roty=movePxFXPiano;
                    piano_posx=movePx2FXPiano*0.05;
                    piano_posy=-movePy2FXPiano*0.05;
                    piano_posz=movePinchScaleFXPiano*100*4;
                    RenderUtils::DrawPiano3D(tim_notes_cpy[playerpos],ww,hh,MIDIFX_OFS*2,0,piano_posx,piano_posy,piano_posz,piano_rotx,piano_roty,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value);
                    break;
                case 4:
                    if (movePinchScaleFXPiano<-0.8/4) movePinchScaleFXPiano=-0.8/4;
                    if (movePinchScaleFXPiano>14.0/4) movePinchScaleFXPiano=14.0/4;
                    piano_rotx=movePyFXPiano;
                    piano_roty=movePxFXPiano;
                    piano_posx=movePx2FXPiano*0.05;
                    piano_posy=-movePy2FXPiano*0.05;
                    piano_posz=movePinchScaleFXPiano*100*4;
                    RenderUtils::DrawPiano3DWithNotesWall(tim_notes_cpy[playerpos],ww,hh,MIDIFX_OFS*4,0,piano_posx,piano_posy,piano_posz,piano_rotx,piano_roty,settings[GLOB_FXPianoColorMode].detail.mdz_switch.switch_value,settings[GLOB_FXLOD].detail.mdz_switch.switch_value);
                    break;
            }
        }
	}
    if (viewTapHelpShow) {
		if (viewTapHelpInfo<255) {
            viewTapHelpInfo+=256;//48;
            /*			viewTapHelpInfo+=(255-viewTapHelpInfo)/3;*/
			if (viewTapHelpInfo>255) viewTapHelpInfo=255;
		}
	} else {
		if (viewTapHelpInfo>0) {
            viewTapHelpInfo-=256;//48;
            /*			viewTapHelpInfo-=(255+32-viewTapHelpInfo)/3;*/
			if (viewTapHelpInfo<0) viewTapHelpInfo=0;
		}
	}
    
    if (shouldhide) {
        shouldhide=0;
        mOglViewIsHidden=YES;
        viewTapHelpInfo=0;
        if (oglViewFullscreen) {
            oglViewFullscreen=0;
            oglViewFullscreenChanged=1;
            [self shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientationHV];
        }
    }
    
    
	if (viewTapHelpInfo) {
        int fadelev=255*sin(viewTapHelpInfo*3.14159/2/256);
		RenderUtils::SetUpOrtho(0,ww,hh);
        int active_idx=0;
        
		if (viewTapHelpShowMode==1) {
            active_idx=[self computeActiveFX];
            
            //NSLog(@"alpha: %d / %d",fadelev,(int)(fxalpha*255));
            RenderUtils::DrawFXTouchGrid(ww,hh, fadelev,fxalpha*255,active_idx,framecpt);
            infoMenuShowImages(ww,hh,fadelev);
            
            glPushMatrix();
            int menu_cell_size=(ww<hh?ww:hh);
			glTranslatef((menu_cell_size*2/4)+menu_cell_size/8-(strlen(viewTapInfoStr[2]->mText)/2)*6,menu_cell_size/8+(hh-menu_cell_size)/2, 0.0f);
			viewTapInfoStr[2]->Render(128+(fadelev/2));
			glPopMatrix();
        }
        if (viewTapHelpShowMode==2) {
            
            switch (viewTapHelpShow_SubStart) {
                case SUBMENU0_START: //FX2
                    active_idx=1<<settings[GLOB_FX2].detail.mdz_switch.switch_value;
                    break;
                case SUBMENU1_START: //FX3
                    active_idx=1<<settings[GLOB_FX3].detail.mdz_switch.switch_value;
                    break;
                case SUBMENU2_START: //Spectrum
                    active_idx=1<<settings[GLOB_FXSpectrum].detail.mdz_switch.switch_value;
                    break;
                case SUBMENU3_START: //Oscillo
                    active_idx=1<<settings[GLOB_FXOscillo].detail.mdz_switch.switch_value;
                    break;
                case SUBMENU4_START: //MOD Pattern
                    active_idx=1<<settings[GLOB_FXMODPattern].detail.mdz_switch.switch_value;
                    break;
                case SUBMENU5_START: //MIDI Pattern
                    active_idx=1<<settings[GLOB_FXMIDIPattern].detail.mdz_switch.switch_value;
                    break;
                case SUBMENU6_START: //3D Sphere/Torus
                    active_idx=1<<settings[GLOB_FX5].detail.mdz_switch.switch_value;
                    break;
                case SUBMENU7_START: //Piano
                    active_idx=1<<settings[GLOB_FXPiano].detail.mdz_switch.switch_value;
                    break;
                case SUBMENU8_START:
                    active_idx=1<<settings[GLOB_FX3DSpectrum].detail.mdz_switch.switch_value;
                    break;
            }
            if (active_idx==1) active_idx=0;
            
            RenderUtils::DrawFXTouchGrid(ww,hh, fadelev,fxalpha*255,active_idx,framecpt);
            infoSubMenuShowImages(ww,hh,viewTapHelpShow_SubStart,viewTapHelpShow_SubNb,fadelev);
            int menu_cell_size=(ww<hh?ww:hh);
            glPushMatrix();
			glTranslatef(menu_cell_size/8-(strlen(viewTapInfoStr[1]->mText)/2)*6,menu_cell_size*7/8+(hh-menu_cell_size)/2, 0.0f);
			viewTapInfoStr[1]->Render(128+(fadelev/2));
			glPopMatrix();
        }
        int menu_cell_size=(ww<hh?ww:hh);
        glPushMatrix();
		glTranslatef((menu_cell_size*3/4)+menu_cell_size/8-(strlen(viewTapInfoStr[0]->mText)/2)*6,menu_cell_size/8+(hh-menu_cell_size)/2, 0.0f);
		viewTapInfoStr[0]->Render(128+(fadelev/2));
		glPopMatrix();
	}
	
//    [m_oglContext presentRenderbuffer:GL_RENDERBUFFER_OES];
    
    // Apple (and the khronos group) encourages you to discard depth
    // render buffer contents whenever is possible
    
    FrameBufferUtils::SwapBuffer(m_oglView->m_frameBuffer,m_oglContext);
}


- (void)viewDidDisappear:(BOOL)animated {
	mHasFocus=0;
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


/* ALERT VIEW actions*/
- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    /*	if (alertView==alertCannotPlay) {
     [self play_curEntry];
     }*/
	if (alertView==alertCrash) {
		if (buttonIndex==0) [self loadSettings:0];
	}
}

/* POPUP functions */
-(void) hidePopup {
	infoMsgView.hidden=YES;
	mPopupAnimation=0;
}

-(void) openPopup:(NSString *)msg secmsg:(NSString*)secmsg{
	CGRect frame;
	if (mPopupAnimation) return;
    
	mPopupAnimation=1;
    infoMsgLbl.text=[NSString stringWithString:msg];
    infoSecMsgLbl.text=[NSString stringWithString:secmsg];
	frame=infoMsgView.frame;
	frame.origin.y=self.view.frame.size.height;
	infoMsgView.frame=frame;
	infoMsgView.hidden=NO;
	[UIView beginAnimations:@"closePopup" context:nil];
	[UIView setAnimationDelegate:self];
	[UIView setAnimationDelay:0];
	[UIView setAnimationDuration:0.5];
	frame=infoMsgView.frame;
	frame.origin.y=self.view.frame.size.height-144;
	infoMsgView.frame=frame;
	[UIView commitAnimations];
}
-(void) closePopup {
	CGRect frame;
	[UIView beginAnimations:@"hidePopup" context:nil];
    [UIView setAnimationDelegate:self];
	[UIView setAnimationDelay:2.5];
	[UIView setAnimationDuration:1.0];
	frame=infoMsgView.frame;
	frame.origin.y=self.view.frame.size.height;
	infoMsgView.frame=frame;
	[UIView commitAnimations];
}


#pragma mark -
#pragma mark TKCoverflowViewDelegate methods
- (void) coverflowView:(TKCoverflowView*)coverflowView coverAtIndexWasBroughtToFront:(int)index{
	//NSLog(@"Front %d",index);
    if ((index>=0)&&(index<mPlaylist_size)){
        lblMainCoverflow.text=mPlaylist[index].mPlaylistFilename;
        lblSecCoverflow.text=mPlaylist[index].mPlaylistFilepath;
    }
}

- (TKCoverflowCoverView*) coverflowView:(TKCoverflowView*)coverflowView coverAtIndex:(int)index{
	TKCoverflowCoverView *cover = [coverflowView dequeueReusableCoverView];
	if(cover == nil){
		//BOOL phone = [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPhone;
		CGRect rect = CGRectMake(0,0,coverflow.coverSize.width, coverflow.coverSize.height);//phone ? CGRectMake(0, 0, 224,300) : CGRectMake(0, 0, 300, 600);
		cover = [[TKCoverflowCoverView alloc] initWithFrame:rect]; // 224
		cover.baseline = coverflow.coverSize.height;//224;
	}
//    NSLog(@"ask for cov index %d",index);
    if (mPlaylist[index].cover_flag==-1) [self checkAvailableCovers:index];
    if (mPlaylist[index].cover_flag>0) { //A cover should be available
        NSString *filePath,*coverFilePath;
        filePath=mPlaylist[index].mPlaylistFilepath;
        
        UIImage *img=nil;
        
        coverFilePath=nil;
        if (mPlaylist[index].cover_flag==1) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@.jpg",[filePath stringByDeletingPathExtension]];
        else if (mPlaylist[index].cover_flag==2) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@.jpeg",[filePath stringByDeletingPathExtension]];
        else if (mPlaylist[index].cover_flag==3) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@.png",[filePath stringByDeletingPathExtension]];
        else if (mPlaylist[index].cover_flag==4) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@.gif",[filePath stringByDeletingPathExtension]];
        else if (mPlaylist[index].cover_flag==5) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.jpg",[filePath stringByDeletingLastPathComponent]];
        else if (mPlaylist[index].cover_flag==6) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.jpeg",[filePath stringByDeletingLastPathComponent]];
        else if (mPlaylist[index].cover_flag==7) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.png",[filePath stringByDeletingLastPathComponent]];
        else if (mPlaylist[index].cover_flag==8) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.gif",[filePath stringByDeletingLastPathComponent]];
        else if (mPlaylist[index].cover_flag==9) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/cover.jpg",[filePath stringByDeletingLastPathComponent]];
        else if (mPlaylist[index].cover_flag==10) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/cover.jpeg",[filePath stringByDeletingLastPathComponent]];
        else if (mPlaylist[index].cover_flag==11) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/cover.png",[filePath stringByDeletingLastPathComponent]];
        else if (mPlaylist[index].cover_flag==12) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/cover.gif",[filePath stringByDeletingLastPathComponent]];if (mPlaylist[index].cover_flag==13) {
            //NSLog(@"embedded img in archive");
            img=[self fexGetArchiveCover:[NSHomeDirectory() stringByAppendingFormat:@"/%@",filePath]];
        }
                    //NSLog(@"got idx %d %d %@",index,mPlaylist[index].cover_flag,coverFilePath);
        if (coverFilePath) img=[UIImage imageWithContentsOfFile:coverFilePath];//covers[index+1];
        
        if (img==nil) { //file not available anymore
            mPlaylist[index].cover_flag=0;
            cover.image = covers_default;
        } else {
            
            int w=img.size.width;
            int h=img.size.height;
            if (w>h) {
                int csize=coverflowView.coverSize.width;
                if (w>csize) {
                    int new_h=h*csize/w;
                    cover.image = [img scaleToSize:CGSizeMake(csize*mScaleFactor,new_h*mScaleFactor)];
                } else {
                    cover.image = img;
                }
            } else {
                int csize=coverflowView.coverSize.width;
                if (h>csize) {
                    int new_w=w*csize/h;
                    cover.image = [img scaleToSize:CGSizeMake(new_w*mScaleFactor,csize*mScaleFactor)];
                } else {
                    cover.image = img;
                }
            }
        }
        
    } else {  //No cover available, take default one
        //            NSLog(@"using default");
        cover.image = [UIImage imageNamed:@"default_art.png"];//covers[0];
    }
    
    if (mScaleFactor!=1) cover.image = [[UIImage alloc] initWithCGImage:cover.image.CGImage scale:mScaleFactor orientation:UIImageOrientationUp];
     
	return cover;
}

- (void) coverflowView:(TKCoverflowView*)coverflowView coverAtIndexWasDoubleTapped:(int)index{
	TKCoverflowCoverView *cover = [coverflowView coverAtIndex:index];
	if(cover == nil) return;
    /*	[UIView beginAnimations:nil context:nil];
     [UIView setAnimationDuration:1];
     [UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft forView:cover cache:YES];
     [UIView commitAnimations];
     */
}

- (void) coverflowView:(TKCoverflowView*)coverflowView coverAtIndexWasSingleTapped:(int)index{
	TKCoverflowCoverView *cover = [coverflowView coverAtIndex:index];
	if(cover == nil) return;
    
    [UIView beginAnimations:@"selectCov1" context:(void *)cover];
	[UIView setAnimationDuration:0.15f];
    [UIView setAnimationDelegate:self];
    [UIView setAnimationCurve:UIViewAnimationCurveEaseOut];
    cover.frame=CGRectMake(cover.frame.origin.x, cover.frame.origin.y,cover.frame.size.width,cover.frame.size.height);
    cover.transform=CGAffineTransformMakeScale(0.9f,0.9f);
	[UIView commitAnimations];
    
	mPlaylist_pos=index;
	[self play_curEntry];
}

#pragma mark -

- (void)animationDidStop:(NSString *)animationID finished:(NSNumber *)finished context:(void *)context {
    if ([animationID compare:@"closePopup"]==NSOrderedSame) [self closePopup];
    else if ([animationID compare:@"hidePopup"]==NSOrderedSame) [self hidePopup];
    else if ([animationID compare:@"cflow_out"]==NSOrderedSame) {
        coverflow.alpha=0;
        lblMainCoverflow.alpha=0;
        lblSecCoverflow.alpha=0;
        lblCurrentSongCFlow.alpha=0;
        lblTimeFCflow.alpha=0;
        btnPlayCFlow.alpha=0;
        btnPauseCFlow.alpha=0;
        btnBackCFlow.alpha=0;
        
        coverflow.hidden=TRUE;
        lblMainCoverflow.hidden=TRUE;
        lblSecCoverflow.hidden=TRUE;
        lblCurrentSongCFlow.hidden=TRUE;
        lblTimeFCflow.hidden=TRUE;
        btnPlayCFlow.hidden=TRUE;
        btnPauseCFlow.hidden=TRUE;
        btnBackCFlow.hidden=TRUE;
        
        
    } else if ([animationID compare:@"cflow_in"]==NSOrderedSame) {
        coverflow.alpha=1;
        lblMainCoverflow.alpha=1;
        lblSecCoverflow.alpha=1;
        lblCurrentSongCFlow.alpha=1.0;
        lblTimeFCflow.alpha=1.0;
        btnPlayCFlow.alpha=1.0;
        btnPauseCFlow.alpha=1.0;
        btnBackCFlow.alpha=1.0;
        
        coverflow.hidden=FALSE;
        lblMainCoverflow.hidden=FALSE;
        lblSecCoverflow.hidden=FALSE;
        lblCurrentSongCFlow.hidden=FALSE;
        lblTimeFCflow.hidden=FALSE;
        if (mPaused||(![mplayer isPlaying])) btnPlayCFlow.hidden=FALSE;
        else btnPauseCFlow.hidden=FALSE;
        btnPauseCFlow.hidden=FALSE;
        
    } else if ([animationID compare:@"selectCov1"]==NSOrderedSame) {
        [UIView beginAnimations:@"selectCov2" context:context];
        [UIView setAnimationDuration:0.2f];    
        [UIView setAnimationDelegate:self];
        [UIView setAnimationCurve:UIViewAnimationCurveEaseIn];
        TKCoverflowCoverView* cover=((__bridge TKCoverflowCoverView*)context);
        cover.frame=CGRectMake(cover.frame.origin.x, cover.frame.origin.y,cover.frame.size.width,cover.frame.size.height);
        cover.transform=CGAffineTransformMakeScale(1.1f,1.1f);
        [UIView commitAnimations];
    } else if ([animationID compare:@"selectCov2"]==NSOrderedSame) {
        [UIView beginAnimations:@"selectCov3" context:nil];
        [UIView setAnimationDuration:0.2f];    
        [UIView setAnimationCurve:UIViewAnimationCurveEaseOut];
        TKCoverflowCoverView* cover=((__bridge TKCoverflowCoverView*)context);
        cover.frame=CGRectMake(cover.frame.origin.x, cover.frame.origin.y,cover.frame.size.width,cover.frame.size.height);
        cover.transform=CGAffineTransformMakeScale(1.0f,1.0f);
        [UIView commitAnimations];
    }
}

#pragma mark - Table view data source

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    UILabel *myLabel = [[UILabel alloc] init];
    NSString *lbl;
    switch (current_selmode) {
        case ARCSUB_MODE_ARC:
            lbl=NSLocalizedString(@"Choose a song",@"Choose a song");
            break;
        case ARCSUB_MODE_SUB:
            lbl=NSLocalizedString(@"Choose a subsong",@"Choose a subsong");
            break;
    }
    
    
    [myLabel setText:lbl];
    [myLabel setTextAlignment:NSTextAlignmentCenter];
    
    myLabel.backgroundColor = [UIColor blackColor];
    myLabel.textColor = [UIColor whiteColor];
    myLabel.font = [UIFont fontWithName:@"Gotham-Bold" size:17.0f];
    return myLabel;
}

/*- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    switch (current_selmode) {
        case ARCSUB_MODE_ARC:
            return NSLocalizedString(@"Choose a song",@"Choose a song");
        case ARCSUB_MODE_SUB:
            return NSLocalizedString(@"Choose a subsong",@"Choose a subsong");
    }
    return 0;
}*/

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    switch (current_selmode) {
        case ARCSUB_MODE_ARC:
            return [mplayer getArcEntriesCnt];
        case ARCSUB_MODE_SUB:
            return mplayer.mod_subsongs;
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
    
    
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,SELECTOR_TABVIEWCELL_HEIGHT);
        
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
                               tabView.bounds.size.width-8,
                               SELECTOR_TABVIEWCELL_HEIGHT);
    
    
    switch (current_selmode) {
        case ARCSUB_MODE_ARC:
            topLabel.text=[NSString stringWithFormat:@"%@",[mplayer getArcEntryTitle:indexPath.row]];
            break;
        case ARCSUB_MODE_SUB:
            topLabel.text=[NSString stringWithFormat:@"%@",[mplayer getSubTitle:indexPath.row]];
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
        default:
            break;
    }
    current_selmode=ARCSUB_MODE_NONE;
    [self dismissViewControllerAnimated:YES completion:nil];
}


@end
