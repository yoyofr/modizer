//
//  DetailViewController.m
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#import <mach/mach.h>
#import <mach/mach_host.h>



#define MIDIFX_OFS 10

#define LOCATION_UPDATE_TIMING 1800 //in second : 30minutes
#define NOTES_DISPLAY_LEFTMARGIN 30
#define NOTES_DISPLAY_TOPMARGIN 30


#include <sys/types.h>
#include <sys/sysctl.h>


#include "TextureUtils.h"
#include "DBHelper.h"

#define DEBUG_NO_SETTINGS 0

#include "OGLView.h"
#include "RenderUtils.h"
#include "FXFluid.h"

#include <QuartzCore/CADisplayLink.h>
#import <OpenGLES/EAGLDrawable.h>
#include <OpenGLES/ES1/glext.h>
#import <QuartzCore/QuartzCore.h>

#import "UIImageResize.h"

#import "DetailViewControllerIphone.h"
#import "RootViewControllerIphone.h"
#import <MediaPlayer/MediaPlayer.h>

#import "modplug.h"
#import "gme.h"

#import "math.h"

#import "Font.h"
#import "GLString.h"

#import "timidity.h"

#import "AnimatedGif.h"

extern "C" {
int fix_fftr(short int f[], int m, int inverse);
int fix_fft(short int  fr[], short int  fi[], short int  m, short int  inverse);
}

extern int tim_notes_cpy[SOUND_BUFFER_NB][DEFAULT_VOICES];
extern unsigned char tim_voicenb_cpy[SOUND_BUFFER_NB];
extern char mplayer_error_msg[1024];
int tim_midifx_note_range,tim_midifx_note_offset;
static int mOnlyCurrentEntry;
static int mOnlyCurrentSubEntry;

extern volatile int db_checked;

static int shouldRestart;

//static volatile int locManager_isOn;
static int coverflow_plsize,coverflow_pos,coverflow_needredraw;
static BOOL mOglViewIsHidden;
static volatile int mSendStatTimer;
static NSDate *locationLastUpdate=nil;
static double located_lat=999,located_lon=999;
int mDevice_hh,mDevice_ww;
static int mShouldHaveFocusAfterBackground,mShouldUpdateInfos,mLoadIssueMessage;
static int mRandomFXCpt,mRandomFXCptRev;
static int infoIsFullscreen=0;
static int plIsFullscreen=0;
static UIAlertView *alertCrash;
static MPVolumeView *volumeView;

static int txtMenuHandle[16];

static int mValidatePlName=0;
static int mSavePlaylist=0;

static volatile int mPopupAnimation=0;

//static UIAlertView *alertCannotPlay;
static int alertCannotPlay_displayed;

static uint touch_cpt=0; 
static int viewTapHelpInfo=0;
static int viewTapHelpShow=0;

static NSString *located_country=nil,*located_city=nil;

static 	UIImage *covers_default; // album covers images

//#define TOUCH_CPT_INIT 5  //20 is 1s  (20frames)
#define TOUCH_KEPT_THRESHOLD 10

#define max2(a,b) (a>b?a:b)
#define max4(a,b,c,d) max2(max2(a,b),max2(c,d))
#define max8(a,b,c,d,e,f,g,h) max2(max4(a,b,c,d),max4(e,f,g,h))

static int display_length_mode=0;

@implementation DetailViewControllerIphone

@synthesize coverflow,lblMainCoverflow,lblSecCoverflow,lblCurrentSongCFlow,lblTimeFCflow;
@synthesize btnPlayCFlow,btnPauseCFlow,btnBackCFlow,btnChangeTime;
@synthesize sld_DefaultLength,labelDefaultLength;

@synthesize mDeviceType;
@synthesize cover_view,gifAnimation;
//@synthesize locManager;
@synthesize sc_allowPopup,infoMsgView,infoMsgLbl,infoSecMsgLbl,sc_titleFilename;
@synthesize mIsPlaying,mPaused,mplayer,mPlaylist;
@synthesize detailItem, detailDescriptionLabel;
@synthesize labelModuleName,labelModuleLength, labelTime, labelModuleSize,textMessage,labelNumChannels,labelModuleType,labelSeeking,labelLibName;
@synthesize buttonLoopTitleSel,buttonLoopList,buttonLoopListSel,buttonShuffle,buttonShuffleSel,btnLoopInf;
@synthesize repeatingTimer;
@synthesize sliderProgressModule;
@synthesize playlistView,playlistTabView,detailView,commandViewU,volWin,playlistPos;
@synthesize surDepSld,surDelSld,revDepSld,revDelSld,bassAmoSld,bassRanSld,mastVolSld,mpPanningSld,sldFxAlpha;
@synthesize playBar,pauseBar,playBarSub,pauseBarSub;
@synthesize mainView,infoView;
@synthesize rating1,rating1off,rating2,rating2off,rating3,rating3off,rating4,rating4off,rating5,rating5off;
@synthesize mainRating1,mainRating1off,mainRating2,mainRating2off,mainRating3,mainRating3off,mainRating4,mainRating4off,mainRating5,mainRating5off;
@synthesize mShouldHaveFocus,mHasFocus,mScaleFactor;
@synthesize mParentViewController;
@synthesize infoButton,backInfo,backPlaylist;
@synthesize mPlaylist_pos,mPlaylist_size;
//,mPlaylistFilenames,mPlaylistFilepaths;

@synthesize segcont_oscillo,segcont_forceMono,sc_checkBeforeRedownload,sc_bgPlay,sc_StatsUpload,sc_SpokenTitle,sc_showDebug;
//segcont_resumeLaunch
@synthesize segcont_spectrum,segcont_shownote,segcont_mpSampling;
@synthesize segcont_mpMB,segcont_mpReverb,segcont_mpSUR,segcont_fx1,segcont_fx2,segcont_fx3,segcont_fx4,segcont_fx5,segcont_FxBeat,sc_FXDetail,sc_cflow,sc_AOSDKDSFDSP,sc_AOSDKDSFEmuRatio,sc_AOSDKSSFDSP,sc_AOSDKSSFEmuRatio,sc_AOSDKDSF22KHZ;
@synthesize sc_SEXYPSF_Reverb,sc_SEXYPSF_Interpol;
@synthesize sc_AOSDK_Reverb,sc_AOSDK_Interpol;
@synthesize sc_ADPLUG_opltype;
@synthesize labelTimPolyphony,sld_TIMPoly;
@synthesize oglButton;
@synthesize sldDUMBMastVol,labelDUMBMastVol,sc_DUMBResampling;

@synthesize pvSubSongSel,pvSubSongLabel,pvSubSongValidate,btnShowSubSong;
@synthesize pvArcSel,pvArcLabel,pvArcValidate,btnShowArcList;


@synthesize sc_AfterDownload,sc_EnqueueMode,sc_DefaultAction,segcont_randFx,sc_defaultMODplayer,sc_PlayerViewOnPlay;
@synthesize sc_UADE_Led,sc_UADE_Norm,sc_UADE_PostFX,sc_UADE_Pan,sc_UADE_Head,sc_UADE_Gain,sc_SID_Optim,sc_SID_LibVersion,sc_SID_Filter;
@synthesize sld_UADEpan,sld_UADEgain;
@synthesize sc_TIMreverb,sc_TIMfilter,sc_TIMresample,sc_TIMchorus;
@synthesize sldPanning,sc_Panning,lblPanningValue;

@synthesize btnSortPlAZ,btnSortPlZA,btnPlShuffle,btnPlClear,btnPlEdit,btnPlOk;

@synthesize infoZoom,infoUnzoom,plZoom,plUnzoom;
@synthesize mPlWasView,mInWasView;
@synthesize inputText,textInputView,rootViewControllerIphone,mSlowDevice;

-(IBAction)showSubSongSelector {
	if (pvSubSongSel.hidden) {
		pvSubSongSel.hidden=false;
		pvSubSongLabel.hidden=false;
		pvSubSongValidate.hidden=false;
		[pvSubSongSel selectRow:mplayer.mod_currentsub-mplayer.mod_minsub inComponent:0 animated:TRUE];
		
        /*		[UIView beginAnimations:nil context:nil];
         [UIView setAnimationDelay:0.2];
         [UIView setAnimationDuration:0.70];
         [UIView setAnimationDelegate:self];
         [UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.pvSubSongValidate cache:YES];
         [UIView commitAnimations];*/
	}
	else {
		pvSubSongSel.hidden=true;
		pvSubSongLabel.hidden=true;
		pvSubSongValidate.hidden=true;
	}
}

-(IBAction)showArcSelector {
	if (pvArcSel.hidden) {
		pvArcSel.hidden=false;
		pvArcLabel.hidden=false;
		pvArcValidate.hidden=false;
		[pvArcSel selectRow:[mplayer getArcIndex] inComponent:0 animated:TRUE];
		
        /*		[UIView beginAnimations:nil context:nil];
         [UIView setAnimationDelay:0.2];
         [UIView setAnimationDuration:0.70];
         [UIView setAnimationDelegate:self];
         [UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.pvArcValidate cache:YES];
         [UIView commitAnimations];*/
	}
	else {
		pvArcSel.hidden=true;
		pvArcLabel.hidden=true;
		pvArcValidate.hidden=true;
	}
}

-(IBAction)optGENTitleFilename {
    if (mPlaylist_pos>=mPlaylist_size) return;
    NSString *fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;    
    if (sc_titleFilename.selectedSegmentIndex) labelModuleName.text=[NSString stringWithString:fileName];
    else labelModuleName.text=[NSString stringWithString:[mplayer getModName]];
    
    lblCurrentSongCFlow.text=labelModuleName.text;
}


-(IBAction)pushedLoopInf {
	if (mplayer.mLoopMode==0) {
		[mplayer setLoopInf:1];
		[btnLoopInf setTitleColor:[UIColor colorWithRed:0.3f green:0.5f blue:1.0f alpha:1.0f] forState:UIControlStateNormal];
		if (mPaused==0) [self play_curEntry];
	} else  {
		[mplayer setLoopInf:0];
		[btnLoopInf setTitleColor:[UIColor colorWithRed:0.3f green:0.3f blue:0.3f alpha:1.0f] forState:UIControlStateNormal];
		if (mPaused==0) [self play_curEntry];
	}
}

-(IBAction) oglButtonPushed {
    if (mOglViewIsHidden) mOglViewIsHidden=NO;
    else mOglViewIsHidden=YES;
    [self checkGLViewCanDisplay];
}

-(IBAction)ADPLUG_OptChanged {
	[mplayer optADPLUG:sc_ADPLUG_opltype.selectedSegmentIndex];
}


-(IBAction)AOSDK_OptChanged {
	[mplayer optAOSDK:sc_AOSDK_Reverb.selectedSegmentIndex interpol:sc_AOSDK_Interpol.selectedSegmentIndex];
}
-(IBAction)SEXYPSF_OptChanged {
	[mplayer optSEXYPSF:sc_SEXYPSF_Reverb.selectedSegmentIndex interpol:sc_SEXYPSF_Interpol.selectedSegmentIndex];
}

-(IBAction)optSID_Optim {
	mplayer.optSIDoptim=(int)(sc_SID_Optim.selectedSegmentIndex);
}

-(IBAction)optSID_Filter {
    [mplayer optSIDFilter:sc_SID_Filter.selectedSegmentIndex];
}


-(IBAction)optSID_LibVersion {
	mplayer.mAskedSidEngineType =(int)(sc_SID_LibVersion.selectedSegmentIndex)+1;
}

-(IBAction) optUADE_Led {
	[mplayer optUADE_Led:sc_UADE_Led.selectedSegmentIndex];
}
-(IBAction) optUADE_PostFX {	
	[mplayer optUADE_PostFX:sc_UADE_PostFX.selectedSegmentIndex];
}
-(IBAction) optUADE_Norm {
	if ((sc_UADE_Norm.selectedSegmentIndex==1)&&(sc_UADE_PostFX.selectedSegmentIndex==0)) {
		sc_UADE_PostFX.selectedSegmentIndex=1;
		[mplayer optUADE_PostFX:sc_UADE_PostFX.selectedSegmentIndex];
	}
	[mplayer optUADE_Norm:sc_UADE_Norm.selectedSegmentIndex];
}

-(IBAction) optUADE_Pan {
	if ((sc_UADE_Pan.selectedSegmentIndex==1)&&(sc_UADE_PostFX.selectedSegmentIndex==0)) {
		sc_UADE_PostFX.selectedSegmentIndex=1;
		[mplayer optUADE_PostFX:sc_UADE_PostFX.selectedSegmentIndex];
	}
	[mplayer optUADE_Pan:sc_UADE_Pan.selectedSegmentIndex];
}
-(IBAction) optUADE_Head {
	if ((sc_UADE_Head.selectedSegmentIndex==1)&&(sc_UADE_PostFX.selectedSegmentIndex==0)) {
		sc_UADE_PostFX.selectedSegmentIndex=1;
		[mplayer optUADE_PostFX:sc_UADE_PostFX.selectedSegmentIndex];
	}
	[mplayer optUADE_Head:sc_UADE_Head.selectedSegmentIndex];
}
-(IBAction) optUADE_Gain {
	if ((sc_UADE_Gain.selectedSegmentIndex==1)&&(sc_UADE_PostFX.selectedSegmentIndex==0)) {
		sc_UADE_PostFX.selectedSegmentIndex=1;
		[mplayer optUADE_PostFX:sc_UADE_PostFX.selectedSegmentIndex];
	}
	[mplayer optUADE_Gain:sc_UADE_Gain.selectedSegmentIndex];
}
-(IBAction) optUADE_PanValue {
	[mplayer optUADE_PanValue:sld_UADEpan.value];
}
-(IBAction) optUADE_GainValue {
	[mplayer optUADE_GainValue:sld_UADEgain.value];
}

-(IBAction) optGEN_DefaultLength {
	[mplayer optGEN_DefaultLength:sld_DefaultLength.value];
	labelDefaultLength.text=[NSString stringWithFormat:NSLocalizedString(@"Default length %d:%.2d", @""),(int)(sld_DefaultLength.value)/60,(int)(sld_DefaultLength.value)%60];
}


-(IBAction) optFX_Alpha {
//    m_oglView.alpha=;
    if (sldFxAlpha.value==1.0f) m_oglView.layer.opaque = YES;
    else m_oglView.layer.opaque = NO;
}

-(IBAction) optGLOB_Panning {
    [mplayer optGLOB_Panning:sc_Panning.selectedSegmentIndex];
}

-(IBAction) optGLOB_PanningValue {
    [mplayer optGLOB_PanningValue:sldPanning.value];
    lblPanningValue.text=[NSString stringWithFormat:@"Panning value: %.2f",sldPanning.value];
}

-(IBAction) optDUMBMastVol {
    [mplayer optDUMB_MastVol:sldDUMBMastVol.value];
    labelDUMBMastVol.text=[NSString stringWithFormat:@"%.2f",sldDUMBMastVol.value];
}
-(IBAction) optDUMBResampling {
    [mplayer optDUMB_Resampling:sc_DUMBResampling.selectedSegmentIndex];
}

-(IBAction) optAOSDK_DSF22KHZ {
    [mplayer optAOSDK_22KHZ:sc_AOSDKDSF22KHZ.selectedSegmentIndex];
}
-(IBAction) optAOSDK_DSFDSP {
    [mplayer optAOSDK_DSFDSP:sc_AOSDKDSFDSP.selectedSegmentIndex];
}
-(IBAction) optAOSDK_DSFEmuRatio {
    [mplayer optAOSDK_DSFEmuRatio:sc_AOSDKDSFEmuRatio.selectedSegmentIndex];
}
-(IBAction) optAOSDK_SSFDSP {
    [mplayer optAOSDK_SSFDSP:sc_AOSDKSSFDSP.selectedSegmentIndex];   
}
-(IBAction) optAOSDK_SSFEmuRatio {
    [mplayer optAOSDK_SSFEmuRatio:sc_AOSDKSSFEmuRatio.selectedSegmentIndex];
}


-(IBAction) optTIM_Polyphony {
    [mplayer optTIM_Poly:(int)(sld_TIMPoly.value)];
    labelTimPolyphony.text=[NSString stringWithFormat:@"%d voices",(int)(sld_TIMPoly.value)];
}

- (void)showRating:(int)rating {
	rating1.hidden=TRUE;rating2.hidden=TRUE;rating3.hidden=TRUE;rating4.hidden=TRUE;rating5.hidden=TRUE;
	rating1off.hidden=FALSE;rating2off.hidden=FALSE;rating3off.hidden=FALSE;rating4off.hidden=FALSE;rating5off.hidden=FALSE;
	mainRating1.hidden=TRUE;mainRating2.hidden=TRUE;mainRating3.hidden=TRUE;mainRating4.hidden=TRUE;mainRating5.hidden=TRUE;
	mainRating1off.hidden=FALSE;mainRating2off.hidden=FALSE;mainRating3off.hidden=FALSE;mainRating4off.hidden=FALSE;mainRating5off.hidden=FALSE;
	
	switch (rating) {
		case 5:
			rating5.hidden=FALSE;rating5off.hidden=TRUE;
			mainRating5.hidden=FALSE;mainRating5off.hidden=TRUE;
		case 4:
			rating4.hidden=FALSE;rating4off.hidden=TRUE;
			mainRating4.hidden=FALSE;mainRating4off.hidden=TRUE;
		case 3:
			rating3.hidden=FALSE;rating3off.hidden=TRUE;
			mainRating3.hidden=FALSE;mainRating3off.hidden=TRUE;
		case 2:
			rating2.hidden=FALSE;rating2off.hidden=TRUE;
			mainRating2.hidden=FALSE;mainRating2off.hidden=TRUE;
		case 1:
			rating1.hidden=FALSE;rating1off.hidden=TRUE;
			mainRating1.hidden=FALSE;mainRating1off.hidden=TRUE;
		case 0:
			break;
			
	}
}

-(void) pushedRatingCommon:(short int)playcount{
	if ([mplayer getSongLength]>0) {
        DBHelper::updateFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,mPlaylist[mPlaylist_pos].mPlaylistFilepath,playcount,mRating,[mplayer getSongLength],mplayer.numChannels,(mOnlyCurrentEntry==0?([mplayer isArchive]?-[mplayer getArcEntriesCnt]:mplayer.mod_subsongs):-1) );
    }
	else DBHelper::updateFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,mPlaylist[mPlaylist_pos].mPlaylistFilepath,playcount,mRating);
	if (sc_StatsUpload.selectedSegmentIndex) {
		mSendStatTimer=0;
		[GoogleAppHelper SendStatistics:mPlaylist[mPlaylist_pos].mPlaylistFilename path:mPlaylist[mPlaylist_pos].mPlaylistFilepath rating:mRating playcount:playcount country:located_country city:located_city longitude:located_lon latitude:located_lat];	
	}
	mPlaylist[mPlaylist_pos].mPlaylistRating=mRating;
	[self showRating:mRating];
	[playlistTabView reloadData];
	NSIndexPath *myindex=[[[NSIndexPath alloc] initWithIndex:0] autorelease];
	[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];	
}


-(IBAction)pushedRating1{
	short int playcount=0;
	if (!mPlaylist_size) return;
	
	DBHelper::getFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,mPlaylist[mPlaylist_pos].mPlaylistFilepath,&playcount,&mRating);
	if (mRating==1) mRating=0;
	else mRating=1;
	[self pushedRatingCommon:playcount];
}
-(IBAction)pushedRating2{
	short int playcount=0;
	if (!mPlaylist_size) return;
	DBHelper::getFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,mPlaylist[mPlaylist_pos].mPlaylistFilepath,&playcount,&mRating);
	mRating=2;
	[self pushedRatingCommon:playcount];
}
-(IBAction)pushedRating3{
	short int playcount=0;
	if (!mPlaylist_size) return;
	DBHelper::getFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,mPlaylist[mPlaylist_pos].mPlaylistFilepath,&playcount,&mRating);
	mRating=3;
	[self pushedRatingCommon:playcount];
}
-(IBAction)pushedRating4{
	short int playcount=0;
	if (!mPlaylist_size) return;
	DBHelper::getFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,mPlaylist[mPlaylist_pos].mPlaylistFilepath,&playcount,&mRating);
	mRating=4;
	[self pushedRatingCommon:playcount];
}
-(IBAction)pushedRating5{
	short int playcount=0;
	if (!mPlaylist_size) return;
	DBHelper::getFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,mPlaylist[mPlaylist_pos].mPlaylistFilepath,&playcount,&mRating);
	mRating=5;
	[self pushedRatingCommon:playcount];
}

static char note2charA[12]={'C','C','D','D','E','F','F','G','G','A','A','B'};
static char note2charB[12]={'-','#','-','#','-','-','#','-','#','-','#','-'};
static char dec2hex[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
static int currentPattern,currentRow,startChan,visibleChan,movePx,movePy;

- (IBAction)settingsChanged:(id)sender {
	ModPlug_Settings *mpsettings;
	
	mpsettings=[mplayer getMPSettings];
	
	switch ( segcont_mpSampling.selectedSegmentIndex) {
		case 0:mpsettings->mResamplingMode = MODPLUG_RESAMPLE_LINEAR;break;
		case 1:mpsettings->mResamplingMode = MODPLUG_RESAMPLE_SPLINE;break;
		case 2:mpsettings->mResamplingMode = MODPLUG_RESAMPLE_FIR;break;
		case 3:mpsettings->mResamplingMode = MODPLUG_RESAMPLE_NEAREST;break;
	}
	mpsettings->mFlags|=MODPLUG_ENABLE_OVERSAMPLING|MODPLUG_ENABLE_NOISE_REDUCTION;
	switch ( segcont_mpReverb.selectedSegmentIndex) {
		case 1:mpsettings->mFlags|=MODPLUG_ENABLE_REVERB;break;
		case 0:mpsettings->mFlags&=~MODPLUG_ENABLE_REVERB;break;
	}
	switch ( segcont_mpMB.selectedSegmentIndex) {
		case 1:mpsettings->mFlags|=MODPLUG_ENABLE_MEGABASS;break;
		case 0:mpsettings->mFlags&=~MODPLUG_ENABLE_MEGABASS;break;
	}
	switch ( segcont_mpSUR.selectedSegmentIndex) {
		case 1:mpsettings->mFlags|=MODPLUG_ENABLE_SURROUND;break;
		case 0:mpsettings->mFlags&=~MODPLUG_ENABLE_SURROUND;break;
	}
	
    
	switch ( segcont_forceMono.selectedSegmentIndex) {
		case 1:mplayer.optForceMono=1;break;
		case 0:mplayer.optForceMono=0;break;
	}
	if (segcont_fx2.selectedSegmentIndex&&segcont_fx3.selectedSegmentIndex) {
		if (sender==segcont_fx2) segcont_fx3.selectedSegmentIndex=0;
		else segcont_fx2.selectedSegmentIndex=0;
        segcont_fx4.selectedSegmentIndex=0;
        segcont_fx5.selectedSegmentIndex=0;
	}
    if (segcont_fx4.selectedSegmentIndex) {
        if (sender==segcont_fx4) {
            //segcont_fx1.selectedSegmentIndex=0;
            segcont_fx2.selectedSegmentIndex=0;
            segcont_fx3.selectedSegmentIndex=0;
            segcont_fx5.selectedSegmentIndex=0;
        }
    }
	if (segcont_fx5.selectedSegmentIndex) {
        if (sender==segcont_fx5) {
            //segcont_fx1.selectedSegmentIndex=0;
            segcont_fx2.selectedSegmentIndex=0;
            segcont_fx3.selectedSegmentIndex=0;
            segcont_fx4.selectedSegmentIndex=0;
        }
    }
	int size_chan=12*6;
	if (segcont_shownote.selectedSegmentIndex==2) size_chan=6*6;
	visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN)/size_chan;
	if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
	if (startChan<0) startChan=0;
    
    tim_midifx_note_range=88; //88notes on a Piano
    tim_midifx_note_offset=0;
    
    [mplayer optTIM_Chorus:(int)(sc_TIMchorus.selectedSegmentIndex)];
    [mplayer optTIM_Reverb:(int)(sc_TIMreverb.selectedSegmentIndex)];
    [mplayer optTIM_Resample:(int)(sc_TIMresample.selectedSegmentIndex)];
    [mplayer optTIM_LPFilter:(int)(sc_TIMfilter.selectedSegmentIndex)];
	
	
	mpsettings->mReverbDepth=(int)(revDepSld.value*100.0f);	
	mpsettings->mReverbDelay=(int)(revDelSld.value*160.0f+40.0f);
	mpsettings->mBassAmount=(int)(bassAmoSld.value*100.0f);	
	mpsettings->mBassRange=(int)(bassRanSld.value*80.0f+20.0f);	
	
	
	mpsettings->mStereoSeparation=(int)(mpPanningSld.value*255.0f+1);
	
	mpsettings->mSurroundDepth=(int)(surDepSld.value*100.0f);	
	mpsettings->mSurroundDelay=(int)(surDelSld.value*35.0f+5.0f);
	
	[mplayer setModPlugMasterVol:mastVolSld.value];
	[mplayer updateMPSettings];
	
	[self checkGLViewCanDisplay];
}

-(void) checkGLViewCanDisplay{
	/*if ((segcont_fx1.selectedSegmentIndex==0)&&
		(segcont_fx2.selectedSegmentIndex==0)&&
		(segcont_fx3.selectedSegmentIndex==0)&&
		(segcont_FxBeat.selectedSegmentIndex==0)&&
		(segcont_spectrum.selectedSegmentIndex==0)&&
		(segcont_oscillo.selectedSegmentIndex==0)&&
		((segcont_shownote.selectedSegmentIndex==0)||([mplayer isPlayingTrackedMusic]==0)) ) {*/
    if (mOglViewIsHidden) {
		m_oglView.hidden=YES;
	} else {
		if ((infoView.hidden==YES)&&(playlistView.hidden==YES)) {
			if (m_oglView.hidden) {
				viewTapHelpInfo=0;//255;
				viewTapHelpShow=0;//1; //display info panel upon activation
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
	int itime=[mplayer getCurrentTime];
	
	if (mSendStatTimer) {
		mSendStatTimer--;
		if (mSendStatTimer==0) {
			short int playcount;
			DBHelper::getFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,mPlaylist[mPlaylist_pos].mPlaylistFilepath,&playcount,&mRating);
			[GoogleAppHelper SendStatistics:mPlaylist[mPlaylist_pos].mPlaylistFilename path:mPlaylist[mPlaylist_pos].mPlaylistFilepath rating:mRating playcount:playcount country:located_country city:located_city longitude:located_lon latitude:located_lat];
		}
	}
	int mpl_upd=[mplayer shouldUpdateInfos];
	if (mpl_upd||mShouldUpdateInfos) {
        if ((mpl_upd!=3)||(mShouldUpdateInfos)) {
            short int playcount=0;
            NSString *fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
            NSString *filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
            
            DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&mRating);
            DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,mRating,[mplayer getSongLength],mplayer.numChannels,(mOnlyCurrentEntry==0?([mplayer isArchive]?-[mplayer getArcEntriesCnt]:mplayer.mod_subsongs):-1));
            mShouldUpdateInfos=0;
        }
		if (mpl_upd>=2) {			
			if (mpl_upd==2) {
                if (sc_titleFilename.selectedSegmentIndex==0) {
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
		if ((mplayer.mod_subsongs>1)&&(mOnlyCurrentSubEntry==0)) {
            int mpl_arcCnt=[mplayer getArcEntriesCnt];
            if (mpl_arcCnt&&(mOnlyCurrentEntry==0)) {            
                playlistPos.text=[NSString stringWithFormat:@"%d of %d/arc %d of %d/sub %d(%d,%d)",mPlaylist_pos+1,mPlaylist_size,
                                  [mplayer getArcIndex]+1,mpl_arcCnt,
                                  mplayer.mod_currentsub,mplayer.mod_minsub,mplayer.mod_maxsub];
            } else {
                playlistPos.text=[NSString stringWithFormat:@"%d of %d/sub %d(%d,%d)",mPlaylist_pos+1,mPlaylist_size,mplayer.mod_currentsub,mplayer.mod_minsub,mplayer.mod_maxsub];
            }
			[pvSubSongSel reloadAllComponents];
			
			if (btnShowSubSong.hidden==true) {
				btnShowSubSong.hidden=false;
				[UIView beginAnimations:nil context:nil];
				[UIView setAnimationDelay:0.0];
				[UIView setAnimationDuration:0.70];
//				[UIView setAnimationDelegate:self];
				[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.btnShowSubSong cache:YES];
				[UIView commitAnimations];
			}
			
			
		} else {
            int mpl_arcCnt=[mplayer getArcEntriesCnt];
            if (mpl_arcCnt&&(mOnlyCurrentEntry==0)) {            
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
            lblTimeFCflow.text=[NSString stringWithFormat:@"%.2d:%.2d - %.2d:%.2d", ([mplayer getCurrentTime]/1000)/60,([mplayer getCurrentTime]/1000)%60,([mplayer getSongLength]/1000)/60,([mplayer getSongLength]/1000)%60];
        } else {
            lblTimeFCflow.text=[NSString stringWithFormat:@"%.2d:%.2d", ([mplayer getCurrentTime]/1000)/60,([mplayer getCurrentTime]/1000)%60];
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
	
	if ((mPaused==0)&&(mplayer.bGlobalAudioPause==2)&&[mplayer isEndReached]) {//mod ended
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
                [self play_loadArchiveModule];
                [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
            } else [self play_nextEntry];   
        }
		
		return;
	} else {
        //		NSLog(@"waiting : %d %d",mplayer.bGlobalAudioPause,[mplayer isEndReached]);
	}
	
	if ( (mPaused==0)&&(mplayer.bGlobalAudioPause==0)&&(segcont_randFx.selectedSegmentIndex==1) ) {
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
			segcont_spectrum.selectedSegmentIndex=0;
			segcont_oscillo.selectedSegmentIndex=0;
			segcont_fx1.selectedSegmentIndex=0;
			segcont_fx2.selectedSegmentIndex=0;
			segcont_fx3.selectedSegmentIndex=0;
            segcont_fx4.selectedSegmentIndex=0;
            segcont_fx5.selectedSegmentIndex=0;
			segcont_FxBeat.selectedSegmentIndex=0;
			switch (arc4random()%19) {
				case 0:
					segcont_spectrum.selectedSegmentIndex=(arc4random()&1)+1;
					break;
				case 1:
					segcont_oscillo.selectedSegmentIndex=(arc4random()&1)+1;
					break;
				case 2:
					segcont_FxBeat.selectedSegmentIndex=1;
					break;
				case 3:
					segcont_fx1.selectedSegmentIndex=1;
					break;
				case 4:
					segcont_fx2.selectedSegmentIndex=(arc4random()%3)+1;
					break;
				case 5:
					segcont_fx3.selectedSegmentIndex=(arc4random()%3)+1;
					break;
				case 6:
					segcont_FxBeat.selectedSegmentIndex=1;
					segcont_oscillo.selectedSegmentIndex=(arc4random()&1)+1;
					break;
				case 7:
					segcont_FxBeat.selectedSegmentIndex=1;
					segcont_spectrum.selectedSegmentIndex=(arc4random()&1)+1;
					break;
				case 8:
					segcont_FxBeat.selectedSegmentIndex=1;
					segcont_fx1.selectedSegmentIndex=1;
					break;
				case 9:
					segcont_FxBeat.selectedSegmentIndex=1;
					segcont_fx2.selectedSegmentIndex=(arc4random()%3)+1;
					break;
				case 10:
					segcont_FxBeat.selectedSegmentIndex=1;
					segcont_fx3.selectedSegmentIndex=(arc4random()%3)+1;
					break;
				case 11:
					segcont_oscillo.selectedSegmentIndex=(arc4random()&1)+1;
					segcont_spectrum.selectedSegmentIndex=(arc4random()&1)+1;
					break;
				case 12:
					segcont_oscillo.selectedSegmentIndex=(arc4random()&1)+1;
					segcont_fx2.selectedSegmentIndex=(arc4random()%3)+1;
					break;
				case 13:
					segcont_oscillo.selectedSegmentIndex=(arc4random()&1)+1;
					segcont_fx3.selectedSegmentIndex=(arc4random()%3)+1;
					break;
				case 14:
					segcont_oscillo.selectedSegmentIndex=(arc4random()&1)+1;
					segcont_fx1.selectedSegmentIndex=1;
					break;
				case 15:
					segcont_FxBeat.selectedSegmentIndex=1;
					segcont_oscillo.selectedSegmentIndex=(arc4random()&1)+1;
					segcont_spectrum.selectedSegmentIndex=(arc4random()&1)+1;
					break;
				case 16:
					segcont_FxBeat.selectedSegmentIndex=1;
					segcont_oscillo.selectedSegmentIndex=(arc4random()&1)+1;
					segcont_fx1.selectedSegmentIndex=1;
					break;
				case 17:
					segcont_FxBeat.selectedSegmentIndex=1;
					segcont_oscillo.selectedSegmentIndex=(arc4random()&1)+1;
					segcont_fx2.selectedSegmentIndex=(arc4random()%3)+1;
					break;
				case 18:
					segcont_FxBeat.selectedSegmentIndex=1;
					segcont_oscillo.selectedSegmentIndex=(arc4random()&1)+1;
					segcont_fx3.selectedSegmentIndex=(arc4random()%3)+1;
					break;
                    
			}
		}
		
	}
	
	
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

-(IBAction) plSortAZ {
	btnSortPlAZ.hidden=YES;
	btnSortPlZA.hidden=NO;
    coverflow_needredraw=1;
    
	NSString *curEntryPath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;	
	qsort(mPlaylist,mPlaylist_size,sizeof(t_plPlaylist_entry),qsort_ComparePlEntries);
	mPlaylist_pos=0;
	while (mPlaylist[mPlaylist_pos].mPlaylistFilepath!=curEntryPath) {
		mPlaylist_pos++;
		if (mPlaylist_pos>=mPlaylist_size) {mPlaylist_pos=0; break;}
	}
	
	[playlistTabView reloadData];
    
	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
	[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
	[myindex autorelease];
	mShouldUpdateInfos=1;
}


-(IBAction) plSortZA {
	btnSortPlAZ.hidden=NO;
	btnSortPlZA.hidden=YES;
    coverflow_needredraw=1;
	
	NSString *curEntryPath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
	qsort(mPlaylist,mPlaylist_size,sizeof(t_plPlaylist_entry),qsort_ComparePlEntriesRev);
	mPlaylist_pos=0;
	while (mPlaylist[mPlaylist_pos].mPlaylistFilepath!=curEntryPath) {
		mPlaylist_pos++;
		if (mPlaylist_pos>=mPlaylist_size) {mPlaylist_pos=0; break;}
	}
	[playlistTabView reloadData];
	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
	[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
	[myindex autorelease];
	mShouldUpdateInfos=1;
}

-(IBAction) plShuffle {
	int fromr,tor;
	t_plPlaylist_entry tmp;
	if (mPlaylist_size<2) return;
    coverflow_needredraw=1;
	for (int i=0;i<mPlaylist_size*2;i++) {
		fromr=arc4random()%(mPlaylist_size);
		do {
			tor=arc4random()%(mPlaylist_size);
		} while (tor==fromr);
		
		tmp=mPlaylist[fromr];
		mPlaylist[fromr]=mPlaylist[tor];
		mPlaylist[tor]=tmp;
		
		
		if (mPlaylist_pos==fromr) mPlaylist_pos=tor;
		else if (mPlaylist_pos==tor) mPlaylist_pos=fromr;
	}
	[playlistTabView reloadData];
	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
	[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
	[myindex autorelease];
	mShouldUpdateInfos=1;
}

-(IBAction) plClear {
    coverflow_needredraw=1;
	if (mPlaylist_size) { //ensure playlist is not empty
		for (int i=mPlaylist_size-1;i>=0;i--) if (i!=mPlaylist_pos) {
			[mPlaylist[i].mPlaylistFilename autorelease];
			[mPlaylist[i].mPlaylistFilepath autorelease];
		}
		mPlaylist[0]=mPlaylist[mPlaylist_pos];
		
		mPlaylist_size=1;
		mPlaylist_pos=0;
		
		mShouldUpdateInfos=1;
	}
	[playlistTabView reloadData];
	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
	[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
	[myindex autorelease];
	
}

-(IBAction) plEdit {
	[playlistTabView setEditing:YES];
	btnPlEdit.hidden=YES;
	btnPlOk.hidden=NO;
    coverflow_needredraw=1;
}

-(IBAction) plDone {
	[playlistTabView setEditing:NO];
	btnPlEdit.hidden=NO;
	btnPlOk.hidden=YES;
	
	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
	[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
	[myindex autorelease];
	
}

- (IBAction)showPlaylist {
	if (!mPlaylist_size) return;
	if (playlistView.hidden==NO) {
		[self hidePlaylist];
		return;
	}
	
	if (infoView.hidden==NO) {
		mPlWasView=infoView;
		mPlWasViewHidden=infoView.hidden;
	} else {
		mPlWasView=m_oglView;
		mPlWasViewHidden=m_oglView.hidden;
	}
    
	btnPlOk.hidden=YES;
	btnPlEdit.hidden=NO;
	btnSortPlAZ.hidden=NO;
	btnSortPlZA.hidden=YES;
	
	
	
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDuration:0.70];
//	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:mPlWasView cache:YES];
	mPlWasView.hidden=YES;
	[UIView commitAnimations];
	
	if (plIsFullscreen) {
		mainView.hidden=YES;
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.70];
//		[UIView setAnimationDelegate:self];
		[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:mainView cache:YES];
		[UIView commitAnimations];
	}
	
	self.playlistView.hidden=NO;
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDuration:0.70];
//	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.playlistView cache:YES];
	[UIView commitAnimations];	
	
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDelay:0.70];
	[UIView setAnimationDuration:0.70];
//	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.backPlaylist cache:YES];
	[UIView commitAnimations];		
	
	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
	[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
	[myindex autorelease];
	
    
}

- (IBAction)hidePlaylist {
	if ((mPlWasView==infoView)&&(mInWasView==playlistView)) mPlWasView=m_oglView;	//avoid cycling between view
	
	if (mPlWasView==m_oglView) {  //if ogl view was selected, check if it should be hidden
		
		/*if ((segcont_fx1.selectedSegmentIndex)||
			(segcont_fx2.selectedSegmentIndex)||
			(segcont_fx3.selectedSegmentIndex)||
			(segcont_FxBeat.selectedSegmentIndex)||
			(segcont_spectrum.selectedSegmentIndex)||
			(segcont_oscillo.selectedSegmentIndex)||
			((segcont_shownote.selectedSegmentIndex)&&([mplayer isPlayingTrackedMusic])) ) {
			mPlWasViewHidden=NO;
		} else mPlWasViewHidden=YES;*/
        if (mOglViewIsHidden) mPlWasViewHidden=YES;
		else mPlWasViewHidden=NO;
	}
	
	if (btnPlEdit.hidden) [self plDone];
	
	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
	[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
	[myindex autorelease];
	
	
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDuration:0.70];
//	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft  forView:self.playlistView cache:YES];
	self.playlistView.hidden=YES;
	[UIView commitAnimations];
	
	if (!mPlWasViewHidden) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.70];
//		[UIView setAnimationDelegate:self];
		[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft  forView:mPlWasView cache:YES];	
		mPlWasView.hidden=NO;
		if (plIsFullscreen) {
			[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft  forView:mainView cache:YES];	
			mainView.hidden=NO;
		}
		[UIView commitAnimations];
	} else if (plIsFullscreen) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.70];
//		[UIView setAnimationDelegate:self];
		[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft  forView:mainView cache:YES];	
		mainView.hidden=NO;
		[UIView commitAnimations];
	}
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDelay:0.70];
	[UIView setAnimationDuration:0.70];
//	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.infoButton cache:YES];
	[UIView commitAnimations];	
	
	if (btnShowSubSong.hidden==false) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDelay:0.75];
		[UIView setAnimationDuration:0.70];
//		[UIView setAnimationDelegate:self];
		[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.btnShowSubSong cache:YES];
		[UIView commitAnimations];
	}
    if (btnShowArcList.hidden==false) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDelay:0.75];
		[UIView setAnimationDuration:0.70];
//		[UIView setAnimationDelegate:self];
		[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.btnShowArcList cache:YES];
		[UIView commitAnimations];
	}
	
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDelay:1.00];	
	[UIView setAnimationDuration:0.70];
//	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft  forView:self.navigationItem.rightBarButtonItem.customView cache:YES];
	[UIView commitAnimations];
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
- (IBAction)plFullscreen {
	plIsFullscreen=1;
	plZoom.hidden=YES;
	plUnzoom.hidden=NO;
	[self shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientationHV];
}
- (IBAction)plNormal {
	plIsFullscreen=0;
	plZoom.hidden=NO;
	plUnzoom.hidden=YES;
	mainView.hidden=NO;
	[self shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientationHV];
}

- (IBAction)showInfo {
	if (infoView.hidden==NO) {
		[self hideInfo];
		return;
	}
    textMessage.text=[NSString stringWithFormat:@"%@",[mplayer getModMessage]];
    
    
	if (playlistView.hidden==NO) {
		mInWasView=playlistView;
		mInWasViewHidden=playlistView.hidden;
	} else {
		mInWasView=m_oglView;
		mInWasViewHidden=m_oglView.hidden;
	}
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
	
	[UIView beginAnimations:nil context:nil];
	[UIView setAnimationDelay:0.70];
	[UIView setAnimationDuration:0.70];
//	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:backInfo cache:YES];
	[UIView commitAnimations];		
	
}

- (IBAction)hideInfo {
	if ((mPlWasView==infoView)&&(mInWasView==playlistView)) mInWasView=m_oglView;	//avoid cycling between view
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
	[UIView setAnimationDelay:0.70];
	[UIView setAnimationDuration:0.70];
//	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:infoButton cache:YES];
	[UIView commitAnimations];	
	
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
        [self play_loadArchiveModule];
        [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
    } else [mplayer playPrevSub];
}
- (IBAction)playNextSub {
    //if archive and no subsongs => change archive index
    if ([mplayer isArchive]&&(mplayer.mod_subsongs==1)) {
        [mplayer selectNextArcEntry];
        [self performSelectorInBackground:@selector(showWaiting) withObject:nil];                    
        [self play_loadArchiveModule];
        [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
    } else [mplayer playNextSub];
}

-(void) longPressNextSubArc:(UIGestureRecognizer *)gestureRecognizer {
    if ([gestureRecognizer state]==UIGestureRecognizerStateBegan) {
        if ([mplayer isArchive]) {
            [mplayer selectNextArcEntry];
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];                    
            [self play_loadArchiveModule];
            [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
        }
    }
}

-(void) longPressPrevSubArc:(UIGestureRecognizer *)gestureRecognizer {
    if ([gestureRecognizer state]==UIGestureRecognizerStateBegan) {
        if ([mplayer isArchive]) {
            [mplayer selectPrevArcEntry];
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];                    
            [self play_loadArchiveModule];
            [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
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
	
	if (!mPlaylist_size) return FALSE;
	
	if (mPlaylist_pos>mPlaylist_size-1) mPlaylist_pos=0;
	
	fileName=mPlaylist[mPlaylist_pos].mPlaylistFilename;
	filePath=mPlaylist[mPlaylist_pos].mPlaylistFilepath;
	mPlaylist[mPlaylist_pos].mPlaylistCount++;
	
	if (self.playlistView.hidden==FALSE) {
		NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
		[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
		[myindex autorelease];
	}
	
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];    
    
	if ([self play_module:filePath fname:fileName]==FALSE) {
		[self remove_from_playlist:mPlaylist_pos];
		if ((alertCannotPlay_displayed==0)&&(mLoadIssueMessage)) {
            NSString *alertMsg;
			alertCannotPlay_displayed=1;
			
            if (mplayer_error_msg[0]) alertMsg=[NSString stringWithFormat:@"%@\n%s", NSLocalizedString(@"File cannot be played. Skipping to next playable file.",@""),mplayer_error_msg];
            else alertMsg=NSLocalizedString(@"File cannot be played. Skipping to next playable file.",@"");
            
            UIAlertView *alertCannotPlay = [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") 
                                                          message:alertMsg delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil] autorelease];
            if (alertCannotPlay) [alertCannotPlay show];

            
			[self play_curEntry];
			
		} else [self play_curEntry];
		//remove
        
        
        [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
		
		return FALSE;
	}
    
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];    
    
	return TRUE;
}

-(void)play_prevEntry {
	if (mShuffle) {
		/*int i;
		int minval;
		minval=mPlaylist[0].mPlaylistCount;
		for (i=0;i<mPlaylist_size;i++) if (mPlaylist[i].mPlaylistCount<minval) minval=mPlaylist[i].mPlaylistCount;
		*/
		mPlaylist_pos=arc4random()%(mPlaylist_size);
		/*i=0;
		while ((i<mPlaylist_size)&&(mPlaylist[mPlaylist_pos].mPlaylistCount>minval)) {
			i++;
			mPlaylist_pos++; if (mPlaylist_pos>=mPlaylist_size) mPlaylist_pos=0;
		}*/
		[self play_curEntry];
	} else {
		if (mPlaylist_pos>0) mPlaylist_pos--;
		else if (mLoopMode==1) mPlaylist_pos=mPlaylist_size-1;
		[self play_curEntry];
	}
}

-(void)play_nextEntry {
	if (mShuffle) {
		/*int i;
		int minval;
		minval=mPlaylist[0].mPlaylistCount;
		for (i=0;i<mPlaylist_size;i++) if (mPlaylist[i].mPlaylistCount<minval) minval=mPlaylist[i].mPlaylistCount;*/
		
		mPlaylist_pos=arc4random()%(mPlaylist_size);		
		/*i=0;
		while ((i<mPlaylist_size)&&(mPlaylist[mPlaylist_pos].mPlaylistCount>minval)) {
			i++;
			mPlaylist_pos++; if (mPlaylist_pos>=mPlaylist_size) mPlaylist_pos=0;
		}*/
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
		UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:msg_str delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil] autorelease];
		[alert show];
		limitPl=1;
		//		return;
	}
	
	if (mPlaylist_size) {
		for (int i=0;i<mPlaylist_size;i++) {
			[mPlaylist[i].mPlaylistFilename autorelease];
			[mPlaylist[i].mPlaylistFilepath autorelease];
		}
	}
	
	mPlaylist_size=(limitPl?MAX_PL_ENTRIES:[array count]);	
	for (int i=0;i<mPlaylist_size;i++) {
		mPlaylist[i].mPlaylistFilename=[[NSString alloc] initWithString:[array objectAtIndex:i]];
		mPlaylist[i].mPlaylistFilepath=[[NSString alloc] initWithString:[arrayFilepaths objectAtIndex:i]];
		
		short int playcount=0;
		signed char rating=0;
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
	
	[playlistTabView reloadData];
	
	[self play_curEntry];
}

-(void)play_listmodules:(NSArray *)array start_index:(int)index path:(NSArray *)arrayFilepaths ratings:(signed char*)ratings playcounts:(short int*)playcounts {
	int limitPl=0;
	mRestart=0;
	mRestart_sub=0;
    mRestart_arc=0;
	mPlayingPosRestart=0;
	
	if ([array count]>=MAX_PL_ENTRIES) {
		NSString *msg_str=[NSString stringWithFormat:NSLocalizedString(@"Too much entries! Playlist will be limited to %d first entries.",@""),MAX_PL_ENTRIES];
		UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:msg_str delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil] autorelease];
		[alert show];
		limitPl=1;
        //		return;
	}
    
	if (mPlaylist_size) {
		for (int i=0;i<mPlaylist_size;i++) {
			[mPlaylist[i].mPlaylistFilename autorelease];
			[mPlaylist[i].mPlaylistFilepath autorelease];
		}
	}
	
	mPlaylist_size=(limitPl?MAX_PL_ENTRIES:[array count]);	
	for (int i=0;i<mPlaylist_size;i++) {
		mPlaylist[i].mPlaylistFilename=[[NSString alloc] initWithString:[array objectAtIndex:i]];
		mPlaylist[i].mPlaylistFilepath=[[NSString alloc] initWithString:[arrayFilepaths objectAtIndex:i]];
		
		mPlaylist[i].mPlaylistRating=ratings[i];
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
    [playlistTabView reloadData];
	
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
		UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:msg_str delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil] autorelease];
		[alert show];
		return 0;
	}
	
	if (mPlaylist_size) { //already in a playlist : append to it
		if (sc_EnqueueMode.selectedSegmentIndex==0) {
            
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
		} else if ((sc_EnqueueMode.selectedSegmentIndex==1)&&(mPlaylist_pos<mPlaylist_size-1)) { //after current
			
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
		
		
		[playlistTabView reloadData];
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
		[playlistTabView reloadData];
		[self play_curEntry];
		playLaunched=1;
	}
	if ((playLaunched==0)&&(!forcenoplay)&&(sc_DefaultAction.selectedSegmentIndex==2)) {//Enqueue & play
		mPlaylist_pos=added_pos;
		[self play_curEntry];
		playLaunched=1;
	}
	
	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
	if (mPlaylist_size) [self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
	[myindex autorelease];
	return playLaunched;
}


-(int) add_to_playlist:(NSString*)filePath fileName:(NSString*)fileName forcenoplay:(int)forcenoplay{
	int added_pos;
	int playLaunched=0;
	short int playcount=0;
	signed char rating=0;
	if (mPlaylist_size>=MAX_PL_ENTRIES) {
		NSString *msg_str=[NSString stringWithFormat:NSLocalizedString(@"Too much entries! Playlist will be limited to %d first entries.",@""),MAX_PL_ENTRIES];
		UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:msg_str delegate:self cancelButtonTitle:NSLocalizedString(@"Close",@"") otherButtonTitles:nil] autorelease];
		[alert show];
		return 0;
	}
        coverflow_needredraw=1;
    
	if (mPlaylist_size) { //already in a playlist : append to it
		if (sc_EnqueueMode.selectedSegmentIndex==0) {
			for (int i=mPlaylist_size-1;i>=0;i--) {
				mPlaylist[i+1]=mPlaylist[i];
			}
			mPlaylist[0].mPlaylistFilename=[[NSString alloc] initWithString:fileName];
			mPlaylist[0].mPlaylistFilepath=[[NSString alloc] initWithString:filePath];
            //[self checkAvailableCovers:0];
            mPlaylist[0].cover_flag=-1;
			
			added_pos=0;
			mPlaylist_pos++;
			
		} else if ((sc_EnqueueMode.selectedSegmentIndex==1)&&(mPlaylist_pos<mPlaylist_size-1)) { //after current
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
        
		[playlistTabView reloadData];
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
		
		[playlistTabView reloadData];
		[self play_curEntry];
		playLaunched=1;
	}
	if ((!forcenoplay)&&(sc_DefaultAction.selectedSegmentIndex==2)) {//Enqueue & play
		mPlaylist_pos=added_pos;
		[self play_curEntry];
		playLaunched=1;
	}
	
	NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
	if (mPlaylist_size) [self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
	[myindex autorelease];
	return playLaunched;
}

-(void) remove_from_playlist:(int)index {//remove entry
    coverflow_needredraw=1;    
    
	if (mPlaylist_size) { //ensure playlist is not empty
		[mPlaylist[index].mPlaylistFilename autorelease];
		[mPlaylist[index].mPlaylistFilepath autorelease];
		
		for (int i=index;i<mPlaylist_size-1;i++) {
			mPlaylist[i]=mPlaylist[i+1];            
		}
        
		mPlaylist_size--;
		if ((index<mPlaylist_pos)||(mPlaylist_pos==mPlaylist_size)) mPlaylist_pos--;
		
		[playlistTabView reloadData];
		//playlistPos.text=[NSString stringWithFormat:@"%d on %d",mPlaylist_pos,mPlaylist_size];
		mShouldUpdateInfos=1;
	}
	
	if (mPlaylist_size) {
		NSIndexPath *myindex=[[NSIndexPath alloc] initWithIndex:0];
		[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:TRUE scrollPosition:UITableViewScrollPositionMiddle];
		[myindex autorelease];
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
	
    
    
	// load module
	if (retcode=[mplayer LoadModule:filePath defaultMODPLAYER:sc_defaultMODplayer.selectedSegmentIndex slowDevice:mSlowDevice archiveMode:1 archiveIndex:-1 singleSubMode:mOnlyCurrentSubEntry  singleArcMode:mOnlyCurrentEntry]) {
		//error while loading
		NSLog(@"Issue in LoadModule(archive) %@",filePath);
		if (retcode==-99) mLoadIssueMessage=0;
		else mLoadIssueMessage=1;
		return FALSE;
	}
    //fix issue with modplug settings reset after load
    [self settingsChanged:nil];
    
    
	
	pvSubSongSel.hidden=true;
	pvSubSongLabel.hidden=true;
	pvSubSongValidate.hidden=true;	
	[pvSubSongSel reloadAllComponents];
	if ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0)) btnShowSubSong.hidden=false;
	else btnShowSubSong.hidden=true;
    
    pvArcSel.hidden=true,
    pvArcLabel.hidden=true;
    pvArcValidate.hidden=true;
    [pvArcSel reloadAllComponents];
    if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0)) btnShowArcList.hidden=false;
	else btnShowArcList.hidden=true;
    
	
	alertCannotPlay_displayed=0;
	//Visiulization stuff
	startChan=0;
    movePx=movePy=0;
	sliderProgressModuleEdit=0;
	sliderProgressModuleChanged=0;
	
	//Update song info if required
	labelModuleName.hidden=NO;
    if (sc_titleFilename.selectedSegmentIndex) labelModuleName.text=[NSString stringWithString:fileName];
    else labelModuleName.text=[NSString stringWithString:[mplayer getModName]];
    lblCurrentSongCFlow.text=labelModuleName.text;
	labelModuleName.textColor = [UIColor colorWithRed:0.95 green:0.95 blue:0.99 alpha:1.0];
	labelModuleName.glowColor = [UIColor colorWithRed:0.40 green:0.40 blue:0.99 alpha:1.0];
	labelModuleName.glowOffset = CGSizeMake(0.0, 0.0);
	labelModuleName.glowAmount = 15.0;
	[labelModuleName setNeedsDisplay];
	self.navigationItem.titleView=labelModuleName;
	
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
	//Update rating info for playlist view
	mRating=0;
	DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&mRating);
    mPlaylist[mPlaylist_pos].mPlaylistRating=mRating;
    
	if (!mRestart) { //if not resuming, update playcount
		playcount++;
        DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,mRating,[mplayer getSongLength],mplayer.numChannels,(mOnlyCurrentEntry==0?([mplayer isArchive]?-[mplayer getArcEntriesCnt]:mplayer.mod_subsongs):-1));
		
		
		//UPDATE Google Application
		if (sc_StatsUpload.selectedSegmentIndex) mSendStatTimer=15;//Wait 15 updateInfos call => 15*0.33 => 15seconds
		
	}
	[self showRating:mRating];
	
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
	[mplayer PlaySeek:mPlayingPosRestart subsong:0];
	sliderProgressModule.value=0;
	mIsPlaying=YES;
	mPaused=0;	
    
    if (coverflow.hidden==NO) {
        btnPlayCFlow.hidden=YES;
        btnPauseCFlow.hidden=NO;
    }
    
	//		[self openPopup:fileName];
    //    if (sc_SpokenTitle.selectedSegmentIndex==1) [fliteTTS speakText:[mplayer getModName]];
    
	//set volume (if applicable)
	[mplayer setModPlugMasterVol:mastVolSld.value];
	
	
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
	repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.33 target:self selector:@selector(updateInfos:) userInfo:nil repeats: YES]; //3 times/second
	
    if (sc_cflow.selectedSegmentIndex) {
        if (coverflow.currentIndex!=mPlaylist_pos) {
            coverflow_pos=mPlaylist_pos;
            [coverflow setCurrentIndex:mPlaylist_pos];
            //[coverflow  bringCoverAtIndexToFront:mPlaylist_pos animated:YES];
        }
    }
    
	return TRUE;
}

- (void)titleTap:(UITapGestureRecognizer *)sender {
//    NSLog(@"%@",labelModuleName.text);
    [self openPopup:labelModuleName.text secmsg:mPlaylist[mPlaylist_pos].mPlaylistFilepath];
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
	//Update position
    /*	if (locManager_isOn==1) {		
     if (locationLastUpdate) {
     int i=[locationLastUpdate timeIntervalSinceNow];
     if (i<-LOCATION_UPDATE_TIMING) {
     locManager_isOn=2;
     [self.locManager startUpdatingLocation];
     [locationLastUpdate release];
     locationLastUpdate=[[NSDate alloc] init];
     }
     }
     }*/
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
            filePathTmp=[NSString stringWithFormat:@"%s",tmp_str_copy];            
        }
        if (tmp_str[i]=='?') {
            found_sub=1;
            memcpy(tmp_str_copy,tmp_str,i);
            tmp_str_copy[i]=0;
            filePathTmp=[NSString stringWithFormat:@"%s",tmp_str_copy];            
        }
        i++;
    }
    if ((found_arc==0)&&(found_sub==0)) filePathTmp=[NSString stringWithString:filePath];            
    if (mRestart) {
        
    } else {
        mRestart_arc=arc_index;
        mRestart_sub=sub_index;
    }
    
	if (retcode=[mplayer LoadModule:filePathTmp defaultMODPLAYER:sc_defaultMODplayer.selectedSegmentIndex slowDevice:mSlowDevice archiveMode:0 archiveIndex:mRestart_arc singleSubMode:mOnlyCurrentSubEntry singleArcMode:mOnlyCurrentEntry]) {
		//error while loading
		NSLog(@"Issue in LoadModule %@",filePathTmp);
		mRestart=0;
		mRestart_sub=0;
        mRestart_arc=0;
		if (retcode==-99) mLoadIssueMessage=0;
		else mLoadIssueMessage=1;
		return FALSE;
	}
    //fix issue with modplug settings reset after load
    [self settingsChanged:nil];
    
    NSString *pathFolderImgPNG,*pathFileImgPNG,*pathFolderImgJPG,*pathFileImgJPG,*pathFolderImgGIF,*pathFileImgGIF;
    pathFolderImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.png",[filePathTmp stringByDeletingLastPathComponent]];
    pathFolderImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.jpg",[filePathTmp stringByDeletingLastPathComponent]];
    pathFolderImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.gif",[filePathTmp stringByDeletingLastPathComponent]];
    pathFileImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.png",[filePathTmp stringByDeletingPathExtension]];
    pathFileImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.jpg",[filePathTmp stringByDeletingPathExtension]];
    pathFileImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@.gif",[filePathTmp stringByDeletingPathExtension]];
    
    UIImage *cover_img=nil;
    //    cover_img=[UIImage imageWithData:[NSData dataWithContentsOfFile:pathFolderImgPNG]];
    if (gifAnimation) [gifAnimation removeFromSuperview];
    gifAnimation=nil;
    
    cover_img=[UIImage imageWithContentsOfFile:pathFileImgJPG];
    if (cover_img==nil) cover_img=[UIImage imageWithContentsOfFile:pathFileImgPNG];
    if (cover_img==nil) {
        cover_img=[UIImage imageWithContentsOfFile:pathFileImgGIF];
        if (cover_img) {
            NSURL* firstUrl = [NSURL fileURLWithPath:pathFileImgGIF];
            gifAnimation = [AnimatedGif getAnimationForGifAtUrl: firstUrl];
            
            gifAnimation.frame=CGRectMake(cover_view.frame.origin.x, cover_view.frame.origin.y,                                          cover_view.frame.size.width,cover_view.frame.size.height);
            [cover_view addSubview:gifAnimation];
        }
    }
    if (cover_img==nil) cover_img=[UIImage imageWithContentsOfFile:pathFolderImgJPG];
    if (cover_img==nil) cover_img=[UIImage imageWithContentsOfFile:pathFolderImgPNG];
    if (cover_img==nil) {
        cover_img=[UIImage imageWithContentsOfFile:pathFolderImgGIF];
        if (cover_img) {
            NSURL* firstUrl = [NSURL fileURLWithPath:pathFileImgGIF];
            gifAnimation= [AnimatedGif getAnimationForGifAtUrl: firstUrl];
            
            gifAnimation.frame=CGRectMake(0, 0, mDevice_ww, mDevice_hh-234+82);
            [gifAnimation layoutSubviews];
            [cover_view addSubview:gifAnimation];
        }
    }
    if (cover_img) {
        cover_view.image=cover_img;
        cover_view.hidden=FALSE;        
    } else cover_view.hidden=TRUE;
    
    //[self checkAvailableCovers:mPlaylist_pos];        
    mPlaylist[mPlaylist_pos].cover_flag=-1;
    
	pvSubSongSel.hidden=true;
	pvSubSongLabel.hidden=true;
	pvSubSongValidate.hidden=true;	
	[pvSubSongSel reloadAllComponents];
	if ([mplayer isMultiSongs]&&(mOnlyCurrentSubEntry==0)) btnShowSubSong.hidden=false;
	else btnShowSubSong.hidden=true;
    
    
    pvArcSel.hidden=true,
    pvArcLabel.hidden=true;
    pvArcValidate.hidden=true;
    [pvArcSel reloadAllComponents];
    if ([mplayer isArchive]&&([mplayer getArcEntriesCnt]>1)&&(mOnlyCurrentEntry==0)) btnShowArcList.hidden=false;
	else btnShowArcList.hidden=true;
    
	
	
	alertCannotPlay_displayed=0;
	//Visiulization stuff
	startChan=0;
	movePx=movePy=0;
	sliderProgressModuleEdit=0;
	sliderProgressModuleChanged=0;
    
	//Update song info if required
    labelModuleName.hidden=NO;
    if (sc_titleFilename.selectedSegmentIndex) labelModuleName.text=[NSString stringWithString:fileName];
    else labelModuleName.text=[NSString stringWithString:[mplayer getModName]];
	lblCurrentSongCFlow.text=labelModuleName.text;
    labelModuleName.textColor = [UIColor colorWithRed:0.95 green:0.95 blue:0.99 alpha:1.0];
    labelModuleName.glowColor = [UIColor colorWithRed:0.40 green:0.40 blue:0.99 alpha:1.0];
    labelModuleName.glowOffset = CGSizeMake(0.0, 0.0);
    labelModuleName.glowAmount = 15.0;
    [labelModuleName setNeedsDisplay];
    self.navigationItem.titleView=labelModuleName;
    
    
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
	//Update rating info for playlist view
	mRating=0;
	DBHelper::getFileStatsDBmod(fileName,filePath,&playcount,&mRating);
    mPlaylist[mPlaylist_pos].mPlaylistRating=mRating;
    
	if (!mRestart) { //if not resuming, update playcount
		playcount++;
		DBHelper::updateFileStatsDBmod(fileName,filePath,playcount,mRating,[mplayer getSongLength],mplayer.numChannels,(mOnlyCurrentEntry==0?([mplayer isArchive]?-[mplayer getArcEntriesCnt]:mplayer.mod_subsongs):-1));
		
		//UPDATE Google Application
		if (sc_StatsUpload.selectedSegmentIndex) mSendStatTimer=15;//Wait 15 updateInfos call => 15*0.33 => 15seconds
		
	}
	[self showRating:mRating];
	
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
		
        //		[self openPopup:fileName];
        //		if (sc_SpokenTitle.selectedSegmentIndex==1) [fliteTTS speakText:[mplayer getModName]];
		
	}
	mRestart_sub=0;
    mRestart_arc=0;
	//set volume (if applicable)
	[mplayer setModPlugMasterVol:mastVolSld.value];
    
    if (coverflow.hidden==NO) {
        btnPlayCFlow.hidden=YES;
        btnPauseCFlow.hidden=NO;
    }

	if (sc_cflow.selectedSegmentIndex) {
        if (coverflow.currentIndex!=mPlaylist_pos) {
            coverflow_pos=mPlaylist_pos;
            [coverflow setCurrentIndex:mPlaylist_pos];
            //[coverflow  bringCoverAtIndexToFront:mPlaylist_pos animated:YES];
        }
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
	//Activate timer for play infos
	repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.33 target:self selector:@selector(updateInfos:) userInfo:nil repeats: YES]; //3 times/second
	
	return TRUE;
}



#pragma mark -
#pragma mark Managing the detail item

/*
 When setting the detail item, update the view and dismiss the popover controller if it's showing.
 */
/*
 - (void)setDetailItem:(id)newDetailItem {
 if (detailItem != newDetailItem) {
 [detailItem release];
 detailItem = [newDetailItem retain];
 
 // Update the view.
 [self configureView];
 }
 }
 
 
 - (void)configureView {
 // Update the user interface for the detail item.
 detailDescriptionLabel.text = [detailItem description];   
 }
 */


#pragma mark -
#pragma mark Rotation support

//- (void)willAnimateSecondHalfOfRotationFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation duration:(NSTimeInterval)duration {
//- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {	
/*- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
    if ((fromInterfaceOrientation==UIInterfaceOrientationPortrait)||(fromInterfaceOrientation==UIInterfaceOrientationPortraitUpsideDown)) {
    } else {
    
    }
}*/

// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	orientationHV=interfaceOrientation;
	if ((interfaceOrientation==UIInterfaceOrientationPortrait)||(interfaceOrientation==UIInterfaceOrientationPortraitUpsideDown)) {
        waitingView.transform=CGAffineTransformMakeRotation(interfaceOrientation==UIInterfaceOrientationPortrait?0:M_PI);
        
        
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
            [UIView commitAnimations];
            [[self navigationController] setNavigationBarHidden:NO animated:NO];
        }
        
		if (oglViewFullscreen) {
			//self.navigationController.navigationBar.hidden = YES;
			mainView.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh-20-42);
			m_oglView.frame = CGRectMake(0.0, 0.0, mDevice_ww, mDevice_hh-20-42);
			
		} else {
			//self.navigationController.navigationBar.hidden = NO;
			mainView.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh-20-42);
			m_oglView.frame = CGRectMake(0, 80, mDevice_ww, mDevice_hh-234+4);
            cover_view.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh-234+82);
            if (gifAnimation) gifAnimation.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh-234+82);
			oglButton.frame = CGRectMake(0, 82, mDevice_ww, mDevice_hh-234);
			volWin.frame= CGRectMake(0, mDevice_hh-64-42, mDevice_ww, 44);
			volumeView.frame = volWin.bounds;
//			volumeView.center = CGPointMake((mDevice_ww)/2,32);
//			[volumeView sizeToFit];
			
			
			if (infoIsFullscreen) infoView.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh-20-42);
			else infoView.frame = CGRectMake(0, 82, mDevice_ww, mDevice_hh-234);
			if (plIsFullscreen) playlistView.frame = CGRectMake(0, 0, mDevice_ww, mDevice_hh-20-42);
			else playlistView.frame = CGRectMake(0, 82, mDevice_ww, mDevice_hh-234);
			
			commandViewU.frame = CGRectMake(2, 48, mDevice_ww-4, 32);
            buttonLoopTitleSel.frame = CGRectMake(10,0,32,32);
            buttonLoopList.frame = CGRectMake(10,0,32,32);
            buttonLoopListSel.frame = CGRectMake(10,0,32,32);
            buttonShuffle.frame = CGRectMake(50,0,32,32);
            buttonShuffleSel.frame = CGRectMake(50,0,32,32);
            btnLoopInf.frame = CGRectMake(88,-12,35,57);
            btnShowSubSong.frame = CGRectMake(mDevice_ww-36,0,32,32);
            btnShowArcList.frame = CGRectMake(mDevice_ww-36-36,0,32,32);
			
			infoButton.frame = CGRectMake(mDevice_ww-44,4,40,40);
			
			mainRating1.frame = CGRectMake(130,3,24,24);
			mainRating2.frame = CGRectMake(130+24,3,24,24);
			mainRating3.frame = CGRectMake(130+24*2,3,24,24);
			mainRating4.frame = CGRectMake(130+24*3,3,24,24);
			mainRating5.frame = CGRectMake(130+24*4,3,24,24);
			mainRating1off.frame = CGRectMake(130,3,24,24);
			mainRating2off.frame = CGRectMake(130+24,3,24,24);
			mainRating3off.frame = CGRectMake(130+24*2,3,24,24);
			mainRating4off.frame = CGRectMake(130+24*3,3,24,24);
			mainRating5off.frame = CGRectMake(130+24*4,3,24,24);
			
			
			playlistPos.frame = CGRectMake(mDevice_ww/2-90,0,180,20);
			labelModuleLength.frame=CGRectMake(2,0,45,20);
			labelTime.frame=CGRectMake(2,24,45,20);            
            btnChangeTime.frame=CGRectMake(2,24,45,20);            
			sliderProgressModule.frame = CGRectMake(48,23,mDevice_ww-48*2,23);
		}
	} else{            
                waitingView.transform=CGAffineTransformMakeRotation(interfaceOrientation==UIInterfaceOrientationLandscapeLeft?-M_PI_2:M_PI_2);
        if ((mPlaylist_size>0)&&(sc_cflow.selectedSegmentIndex)) {
            
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
            
            coverflow.hidden=FALSE;
            lblMainCoverflow.hidden=FALSE;
            lblSecCoverflow.hidden=FALSE;
            lblCurrentSongCFlow.hidden=FALSE;
            lblTimeFCflow.hidden=FALSE;
            btnBackCFlow.hidden=FALSE;
            
            if (mPaused||(![mplayer isPlaying])) btnPlayCFlow.hidden=FALSE;
            else btnPauseCFlow.hidden=FALSE;
            
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
            
            [UIView commitAnimations];
            
            
            
            if (mDeviceType==1) {
                lblMainCoverflow.frame=CGRectMake(0,mDevice_ww-40-64,mDevice_hh,40);
                lblSecCoverflow.frame=CGRectMake(40,mDevice_ww-40-24,mDevice_hh-80,24);
                
                lblCurrentSongCFlow.frame=CGRectMake(0,0,mDevice_hh*2/3,24);
                lblTimeFCflow.frame=CGRectMake(mDevice_hh*2/3,0,mDevice_hh/3,24);
                btnPlayCFlow.frame=CGRectMake(8,mDevice_ww-40-32,32,32);
                btnPauseCFlow.frame=CGRectMake(8,mDevice_ww-40-32,32,32);
                btnBackCFlow.frame=CGRectMake(8,32,32,32);
            } else {
                lblMainCoverflow.frame=CGRectMake(0,mDevice_ww-40-32,mDevice_hh,20);
                lblSecCoverflow.frame=CGRectMake(40,mDevice_ww-40-12,mDevice_hh-80,12);
                
                lblCurrentSongCFlow.frame=CGRectMake(0,0,mDevice_hh*2/3,12);
                lblTimeFCflow.frame=CGRectMake(mDevice_hh*2/3,0,mDevice_hh/3,12);
                btnPlayCFlow.frame=CGRectMake(4,mDevice_ww-32-32,32,32);
                btnPauseCFlow.frame=CGRectMake(4,mDevice_ww-32-32,32,32);
                btnBackCFlow.frame=CGRectMake(4,16,32,32);
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
            
            
            if (oglViewFullscreen) {
                //self.navigationController.navigationBar.hidden = YES;
                mainView.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww-20-30);
                m_oglView.frame = CGRectMake(0.0, 0.0, mDevice_hh, mDevice_ww-20-30);  //ipad
                
            } else {
                //self.navigationController.navigationBar.hidden = NO;
                mainView.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww-20-30);
                m_oglView.frame = CGRectMake(0.0, 82, mDevice_hh, mDevice_ww-104-30);
                cover_view.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww-104-30+82);
                if (gifAnimation) gifAnimation.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww-104-30+82);
                oglButton.frame = CGRectMake(0.0, 82, mDevice_hh, mDevice_ww-104-30);
                
                volWin.frame= CGRectMake(200, 41, mDevice_hh-375, 44);
                volumeView.frame = volWin.bounds;
//                volumeView.frame = CGRectMake(10, 0, mDevice_hh-375-10, 44);
  //              volumeView.center = CGPointMake((mDevice_hh-375)/2,32);
    //            [volumeView sizeToFit];
                
                
                if (infoIsFullscreen) infoView.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww-20-30);
                else infoView.frame = CGRectMake(0.0, 82, mDevice_hh, mDevice_ww-104-30);
                if (plIsFullscreen) playlistView.frame = CGRectMake(0.0, 0, mDevice_hh, mDevice_ww-20-30);
                else playlistView.frame = CGRectMake(0.0, 82, mDevice_hh, mDevice_ww-104-30);
                
                commandViewU.frame = CGRectMake(mDevice_hh-72-40-31-20-4, 8, 40+72+31+20, 32+32);
				buttonLoopTitleSel.frame = CGRectMake(2,0,40,32);
				buttonLoopList.frame = CGRectMake(2,0,40,32);
				buttonLoopListSel.frame = CGRectMake(2,0,40,32);
				buttonShuffle.frame = CGRectMake(42,0,40,32);
				buttonShuffleSel.frame = CGRectMake(42,0,40,32);
				btnLoopInf.frame = CGRectMake(80,-12,35,57);
                
                mainRating1.frame = CGRectMake(6,36,24,24);
                mainRating2.frame = CGRectMake(6+24,36,24,24);
                mainRating3.frame = CGRectMake(6+24*2,36,24,24);
                mainRating4.frame = CGRectMake(6+24*3,36,24,24);
                mainRating5.frame = CGRectMake(6+24*4,36,24,24);
                mainRating1off.frame = CGRectMake(6,36,24,24);
                mainRating2off.frame = CGRectMake(6+24,36,24,24);
                mainRating3off.frame = CGRectMake(6+24*2,36,24,24);
                mainRating4off.frame = CGRectMake(6+24*3,36,24,24);
                mainRating5off.frame = CGRectMake(6+24*4,36,24,24);
                
                
                btnShowSubSong.frame = CGRectMake(124+7,0,32,32);
                btnShowArcList.frame = CGRectMake(124+7,32,32,32);
                infoButton.frame = CGRectMake(mDevice_hh-200-10,1,38,38);				
                
                playlistPos.frame = CGRectMake((mDevice_hh-200)/2-90,0,180,20);
                
                labelModuleLength.frame=CGRectMake(2,0,45,20);
                labelTime.frame=CGRectMake(2,20,45,20);
                btnChangeTime.frame=CGRectMake(2,17,45,20);
                
                sliderProgressModule.frame = CGRectMake(48,16,mDevice_hh-200-60,23);
            }
        }
	}
	[self updateBarPos];
	int size_chan=12*6;
	if (segcont_shownote.selectedSegmentIndex==2) size_chan=6*6;
	visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN)/size_chan;
	if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
	if (startChan<0) startChan=0;
    tim_midifx_note_range=88; //88notes on a Piano
    tim_midifx_note_offset=0;
	
	return YES;
}

-(void)updateBarPos {
	if ((orientationHV==UIInterfaceOrientationPortrait)||(orientationHV==UIInterfaceOrientationPortraitUpsideDown)) {
		playBar.frame =  CGRectMake(0, mDevice_hh-(playBar.hidden?0:108+42), mDevice_ww, 44);
		pauseBar.frame =  CGRectMake(0, mDevice_hh-(pauseBar.hidden?0:108+42), mDevice_ww, 44);
		playBarSub.frame =  CGRectMake(0, mDevice_hh-(playBarSub.hidden?0:108+42), mDevice_ww, 44);
		pauseBarSub.frame =  CGRectMake(0, mDevice_hh-(pauseBarSub.hidden?0:108+42), mDevice_ww, 44);
	} else {
		playBar.frame = CGRectMake(0, 40, 200, 44); //mDevice_hh-(playBar.hidden?0:375)
		pauseBar.frame = CGRectMake(0, 40, 200, 44);
		playBarSub.frame =  CGRectMake(0, 40, 200, 44);
		pauseBarSub.frame =  CGRectMake(0, 40, 200, 44);
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  /* Set Blending Mode         */
    glEnable(GL_BLEND);                 /* Enable Blending           */
	
    /* Bind To The Blur Texture */
    
	
    /* Switch To An Ortho View   */
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
            vertices[0][0]=window_width*j/4+marg; vertices[0][1]=window_height*i/4+marg;
            vertices[0][2]=0.0f;		
            vertices[1][0]=window_width*j/4+marg; vertices[1][1]=window_height*(i+1)/4-marg;
            vertices[1][2]=0.0f;	
            vertices[2][0]=window_width*(j+1)/4-marg; vertices[2][1]=window_height*i/4+marg;
            vertices[2][2]=0.0f;	
            vertices[3][0]=window_width*(j+1)/4-marg; vertices[3][1]=window_height*(i+1)/4-marg;
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

void fxRadialBlur(int fxtype,int _ww,int _hh,short int *spectrumDataL,short int *spectrumDataR,int numval,float alphaval) {	
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
	[prefs setObject:valNb forKey:@"ModizerRunning"];[valNb autorelease];
    
    [prefs synchronize];
    [valNb autorelease];
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
	[prefs setObject:valNb forKey:@"ModizerRunning"];[valNb autorelease];
	
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
    if (plIsFullscreen) {
        plZoom.hidden=YES;
        plUnzoom.hidden=NO;
    } else {
        plZoom.hidden=NO;
        plUnzoom.hidden=YES;
    }
    
	mLoopMode--;
	[self changeLoopMode];
    [mplayer setLoopInf:mplayer.mLoopMode^1];
    [self pushedLoopInf];
	mShuffle^=1;	
	[self shuffle];
	
	//update settings according toi what was loaded
	[self optUADE_Led];
	[self optUADE_Norm];
	[self optUADE_PostFX];
	[self optUADE_Pan];
	[self optUADE_Head];
	[self optUADE_Gain];
	[self optUADE_PanValue];
	[self optUADE_GainValue];
    [self optGEN_DefaultLength];
    [self optTIM_Polyphony];
    [self optFX_Alpha];
	[self optSID_Optim];
	[self optSID_LibVersion];
    [self optSID_Filter];
    
    [self optDUMBMastVol];
    [self optDUMBResampling];
    
    [self optAOSDK_DSFEmuRatio];
    [self optAOSDK_DSFDSP];
    [self optAOSDK_DSF22KHZ];
    [self optAOSDK_SSFEmuRatio];
    [self optAOSDK_SSFDSP];
    
    [self optGLOB_Panning];
    [self optGLOB_PanningValue];
    
    [self settingsChanged:nil];

}

-(void) loadDefaultSettings {
    ///////////////////////////////////
    // General
    ///////////////////////////////////////	
    sld_DefaultLength.value = SONG_DEFAULT_LENGTH/1000;
    sc_titleFilename.selectedSegmentIndex = 0;
    sc_StatsUpload.selectedSegmentIndex = 1;
	sc_SpokenTitle.selectedSegmentIndex = 1;
	sc_bgPlay.selectedSegmentIndex = 1;
	sc_showDebug.selectedSegmentIndex = 0;
	sc_cflow.selectedSegmentIndex = 1;
	sldFxAlpha.value = (mSlowDevice?1.0f:0.5f);
//	segcont_resumeLaunch.selectedSegmentIndex = 0;
	segcont_oscillo.selectedSegmentIndex = 0;
	segcont_spectrum.selectedSegmentIndex = 0;
	segcont_fx1.selectedSegmentIndex = 0;
	segcont_fx2.selectedSegmentIndex = 0;
	segcont_fx3.selectedSegmentIndex = 0;
    segcont_fx4.selectedSegmentIndex = 0;
    segcont_fx5.selectedSegmentIndex = 0;
	segcont_FxBeat.selectedSegmentIndex = 0;
	segcont_randFx.selectedSegmentIndex = 0;
	sc_FXDetail.selectedSegmentIndex = 0;
	segcont_forceMono.selectedSegmentIndex = 0;
	sc_checkBeforeRedownload.selectedSegmentIndex = 0;
	sc_AfterDownload.selectedSegmentIndex = 1;
	sc_EnqueueMode.selectedSegmentIndex = 2;
	sc_DefaultAction.selectedSegmentIndex = 0;
	sc_defaultMODplayer.selectedSegmentIndex = 0;
	sc_PlayerViewOnPlay.selectedSegmentIndex = 0;
	
    ///////////////////////////////////
    // UADE
    ///////////////////////////////////////
	sc_UADE_Led.selectedSegmentIndex = 0;
	sc_UADE_Norm.selectedSegmentIndex = 0;
	sc_UADE_PostFX.selectedSegmentIndex = 1;
	sc_UADE_Head.selectedSegmentIndex = 0;
	sc_UADE_Pan.selectedSegmentIndex = 1;
	sc_UADE_Gain.selectedSegmentIndex = 0;
	sld_UADEpan.value = 0.7f;
	sld_UADEgain.value = 0.5f;
    ////////////////////////////////////
    // SID
    ///////////////////////////////////////
	sc_SID_Optim.selectedSegmentIndex = 1;
	sc_SID_LibVersion.selectedSegmentIndex = (mSlowDevice?0:1);
    sc_SID_Filter.selectedSegmentIndex = (mSlowDevice?0:1);
    ////////////////////////////////////
    // SexyPSF
    ///////////////////////////////////////
	sc_SEXYPSF_Reverb.selectedSegmentIndex = 2;
	sc_SEXYPSF_Interpol.selectedSegmentIndex = 2;
    ////////////////////////////////////
    // AOSDK
    ///////////////////////////////////////
	sc_AOSDK_Reverb.selectedSegmentIndex = 1;
	sc_AOSDK_Interpol.selectedSegmentIndex = 2;
	sc_AOSDKDSFDSP.selectedSegmentIndex = 0;
    sc_AOSDKDSF22KHZ.selectedSegmentIndex = 1;
	sc_AOSDKDSFEmuRatio.selectedSegmentIndex = 2;
	sc_AOSDKSSFDSP.selectedSegmentIndex = 0;
	sc_AOSDKSSFEmuRatio.selectedSegmentIndex = 2;
    ////////////////////////////////////
    // ADPLUG
    ///////////////////////////////////////
	sc_ADPLUG_opltype.selectedSegmentIndex = 1;
    ////////////////////////////////////
    // GME
    ///////////////////////////////////////
    
    ////////////////////////////////////
    // TIMIDITY
    ///////////////////////////////////////    
	sc_TIMchorus.selectedSegmentIndex = (mSlowDevice? 0:1);
	sc_TIMreverb.selectedSegmentIndex = (mSlowDevice? 0:1);
	sc_TIMfilter.selectedSegmentIndex = (mSlowDevice? 0:1);
	sc_TIMresample.selectedSegmentIndex = (mSlowDevice? 0:1);
	sld_TIMPoly.value = (mSlowDevice? 64:128);
	
    ////////////////////////////////////
    // MODPLUG
    ///////////////////////////////////////
	segcont_shownote.selectedSegmentIndex = 0;
	mastVolSld.value = 0.5f;
	segcont_mpSampling.selectedSegmentIndex = 0;
	segcont_mpMB.selectedSegmentIndex = 0;
	bassAmoSld.value = 0.7f;
	bassRanSld.value = 0.3f;
	segcont_mpSUR.selectedSegmentIndex = 0;
	surDepSld.value = 0.9f;
	surDelSld.value = 0.8f;
	segcont_mpReverb.selectedSegmentIndex = 0;
	revDepSld.value = 0.8f;
	revDelSld.value = 0.7f;
	mpPanningSld.value = 0.5f;
    
    [self updateAllSettingsAfterChange];
}
-(void) loadHighSettings {
    ///////////////////////////////////
    // General
    ///////////////////////////////////////	
    sc_titleFilename.selectedSegmentIndex = 0;
    sld_DefaultLength.value = SONG_DEFAULT_LENGTH/1000;
    sc_StatsUpload.selectedSegmentIndex = 1;
	sc_SpokenTitle.selectedSegmentIndex = 1;
	sc_bgPlay.selectedSegmentIndex = 2;
	sc_showDebug.selectedSegmentIndex = 0;
	sc_cflow.selectedSegmentIndex = 1;
	sldFxAlpha.value = 0.5f;
    sc_Panning.selectedSegmentIndex=0;
	sldPanning.value = 0.7f;
	
//	segcont_resumeLaunch.selectedSegmentIndex = 0;
	segcont_oscillo.selectedSegmentIndex = 2;
	segcont_spectrum.selectedSegmentIndex = 1;
	segcont_fx1.selectedSegmentIndex = 0;
	segcont_fx2.selectedSegmentIndex = 0;
	segcont_fx3.selectedSegmentIndex = 0;
    segcont_fx4.selectedSegmentIndex=0;
    segcont_fx5.selectedSegmentIndex = 0;
	segcont_FxBeat.selectedSegmentIndex = 0;
	segcont_randFx.selectedSegmentIndex = 0;
	sc_FXDetail.selectedSegmentIndex = 2;
	segcont_forceMono.selectedSegmentIndex = 0;
	sc_checkBeforeRedownload.selectedSegmentIndex = 0;
	sc_AfterDownload.selectedSegmentIndex = 1;
	sc_EnqueueMode.selectedSegmentIndex = 2;
	sc_DefaultAction.selectedSegmentIndex = 0;
	sc_defaultMODplayer.selectedSegmentIndex = 1;
	sc_PlayerViewOnPlay.selectedSegmentIndex = 1;
    ///////////////////////////////////
    // UADE
    ///////////////////////////////////////
	sc_UADE_Led.selectedSegmentIndex = 0;
	sc_UADE_Norm.selectedSegmentIndex = 0;
	sc_UADE_PostFX.selectedSegmentIndex = 1;
	sc_UADE_Head.selectedSegmentIndex = 0;
	sc_UADE_Pan.selectedSegmentIndex = 1;
	sc_UADE_Gain.selectedSegmentIndex = 0;
	sld_UADEpan.value = 0.7f;
	sld_UADEgain.value = 0.5f;
    ////////////////////////////////////
    // SID
    ///////////////////////////////////////
	sc_SID_Optim.selectedSegmentIndex = 1;
	sc_SID_LibVersion.selectedSegmentIndex = 1;
    sc_SID_Filter.selectedSegmentIndex = 1;
    ////////////////////////////////////
    // SexyPSF
    ///////////////////////////////////////
	sc_SEXYPSF_Reverb.selectedSegmentIndex = 2;
	sc_SEXYPSF_Interpol.selectedSegmentIndex = 2;
    ////////////////////////////////////
    // AOSDK
    ///////////////////////////////////////
	sc_AOSDK_Reverb.selectedSegmentIndex = 1;
	sc_AOSDK_Interpol.selectedSegmentIndex = 2;
    sc_AOSDKDSF22KHZ.selectedSegmentIndex = 0;
	sc_AOSDKDSFDSP.selectedSegmentIndex = 1;
	sc_AOSDKDSFEmuRatio.selectedSegmentIndex = 0;
	sc_AOSDKSSFDSP.selectedSegmentIndex = 1;
	sc_AOSDKSSFEmuRatio.selectedSegmentIndex = 0;
    ////////////////////////////////////
    // ADPLUG
    ///////////////////////////////////////
	sc_ADPLUG_opltype.selectedSegmentIndex = 1;
    ////////////////////////////////////
    // GME
    ///////////////////////////////////////
    ////////////////////////////////////
    // DUMB
    ///////////////////////////////////////    
    sc_DUMBResampling.selectedSegmentIndex = 2;
	sldDUMBMastVol.value = 0.5f;

    ////////////////////////////////////
    // TIMIDITY
    ///////////////////////////////////////    
	sc_TIMchorus.selectedSegmentIndex = 1;
	sc_TIMreverb.selectedSegmentIndex = 1;
	sc_TIMfilter.selectedSegmentIndex = 1;
	sc_TIMresample.selectedSegmentIndex = 3;
	sld_TIMPoly.value = 256;
	
    ////////////////////////////////////
    // MODPLUG
    ///////////////////////////////////////
	segcont_shownote.selectedSegmentIndex = 1;
	mastVolSld.value = 0.5f;
	segcont_mpSampling.selectedSegmentIndex = 2;
	segcont_mpMB.selectedSegmentIndex = 0;
	bassAmoSld.value = 0.7f;
	bassRanSld.value = 0.3f;
	segcont_mpSUR.selectedSegmentIndex = 0;
	surDepSld.value = 0.9f;
	surDelSld.value = 0.8f;
	segcont_mpReverb.selectedSegmentIndex = 0;
	revDepSld.value = 0.8f;
	revDelSld.value = 0.7f;
	mpPanningSld.value = 0.5f;
    
    [self updateAllSettingsAfterChange];    
}
-(void) loadMedSettings {
    ///////////////////////////////////
    // General
    ///////////////////////////////////////	
    sld_DefaultLength.value = SONG_DEFAULT_LENGTH/1000;
    sc_titleFilename.selectedSegmentIndex = 0;
    sc_StatsUpload.selectedSegmentIndex = 1;
	sc_SpokenTitle.selectedSegmentIndex = 0;
	sc_bgPlay.selectedSegmentIndex = 1;
	sc_showDebug.selectedSegmentIndex = 0;
	sc_cflow.selectedSegmentIndex = 1;
	sldFxAlpha.value = 0.5f;
    sc_Panning.selectedSegmentIndex=0;
	sldPanning.value = 0.7f;
	
//	segcont_resumeLaunch.selectedSegmentIndex = 0;
	segcont_oscillo.selectedSegmentIndex = 0;
	segcont_spectrum.selectedSegmentIndex = 0;
	segcont_fx1.selectedSegmentIndex = 0;
	segcont_fx2.selectedSegmentIndex = 0;
	segcont_fx3.selectedSegmentIndex = 0;
    segcont_fx4.selectedSegmentIndex=0;
    segcont_fx5.selectedSegmentIndex = 0;
	segcont_FxBeat.selectedSegmentIndex = 0;
	segcont_randFx.selectedSegmentIndex = 0;
	sc_FXDetail.selectedSegmentIndex = 1;
	segcont_forceMono.selectedSegmentIndex = 0;
	sc_checkBeforeRedownload.selectedSegmentIndex = 0;
	sc_AfterDownload.selectedSegmentIndex = 1;
	sc_EnqueueMode.selectedSegmentIndex = 2;
	sc_DefaultAction.selectedSegmentIndex = 0;
	sc_defaultMODplayer.selectedSegmentIndex = 0;
	sc_PlayerViewOnPlay.selectedSegmentIndex = 1;
    ///////////////////////////////////
    // UADE
    ///////////////////////////////////////
	sc_UADE_Led.selectedSegmentIndex = 0;
	sc_UADE_Norm.selectedSegmentIndex = 0;
	sc_UADE_PostFX.selectedSegmentIndex = 1;
	sc_UADE_Head.selectedSegmentIndex = 0;
	sc_UADE_Pan.selectedSegmentIndex = 1;
	sc_UADE_Gain.selectedSegmentIndex = 0;
	sld_UADEpan.value = 0.7f;
	sld_UADEgain.value = 0.5f;
    ////////////////////////////////////
    // SID
    ///////////////////////////////////////
	sc_SID_Optim.selectedSegmentIndex = 1;
	sc_SID_LibVersion.selectedSegmentIndex = 1;
    sc_SID_Filter.selectedSegmentIndex = 1;
    ////////////////////////////////////
    // SexyPSF
    ///////////////////////////////////////
	sc_SEXYPSF_Reverb.selectedSegmentIndex = 2;
	sc_SEXYPSF_Interpol.selectedSegmentIndex = 2;
    ////////////////////////////////////
    // AOSDK
    ///////////////////////////////////////
	sc_AOSDK_Reverb.selectedSegmentIndex = 1;
	sc_AOSDK_Interpol.selectedSegmentIndex = 1;
    sc_AOSDKDSF22KHZ.selectedSegmentIndex = 1;
	sc_AOSDKDSFDSP.selectedSegmentIndex = 0;
	sc_AOSDKDSFEmuRatio.selectedSegmentIndex = 2;
	sc_AOSDKSSFDSP.selectedSegmentIndex = 0;
	sc_AOSDKSSFEmuRatio.selectedSegmentIndex = 2;
    ////////////////////////////////////
    // ADPLUG
    ///////////////////////////////////////
	sc_ADPLUG_opltype.selectedSegmentIndex = 1;
    ////////////////////////////////////
    // GME
    ///////////////////////////////////////
    ////////////////////////////////////
    // DUMB
    ///////////////////////////////////////    
    sc_DUMBResampling.selectedSegmentIndex = 1;
	sldDUMBMastVol.value = 0.5f;

    ////////////////////////////////////
    // TIMIDITY
    ///////////////////////////////////////    
	sc_TIMchorus.selectedSegmentIndex = 1;
	sc_TIMreverb.selectedSegmentIndex = 1;
	sc_TIMfilter.selectedSegmentIndex = 1;
	sc_TIMresample.selectedSegmentIndex = 3;
	sld_TIMPoly.value = 128;
	
    ////////////////////////////////////
    // MODPLUG
    ///////////////////////////////////////
	segcont_shownote.selectedSegmentIndex = 1;
	mastVolSld.value = 0.5f;
	segcont_mpSampling.selectedSegmentIndex = 0;
	segcont_mpMB.selectedSegmentIndex = 0;
	bassAmoSld.value = 0.7f;
	bassRanSld.value = 0.3f;
	segcont_mpSUR.selectedSegmentIndex = 0;
	surDepSld.value = 0.9f;
	surDelSld.value = 0.8f;
	segcont_mpReverb.selectedSegmentIndex = 0;
	revDepSld.value = 0.8f;
	revDelSld.value = 0.7f;
	mpPanningSld.value = 0.5f;
    
    [self updateAllSettingsAfterChange];        
}
-(void) loadLowSettings {
    ///////////////////////////////////
    // General
    ///////////////////////////////////////	
    sld_DefaultLength.value = SONG_DEFAULT_LENGTH/1000;
    sc_titleFilename.selectedSegmentIndex = 0;
    sc_StatsUpload.selectedSegmentIndex = 1;
	sc_SpokenTitle.selectedSegmentIndex = 0;
	sc_bgPlay.selectedSegmentIndex = 0;
	sc_showDebug.selectedSegmentIndex = 0;
	sc_cflow.selectedSegmentIndex = 1;
	sldFxAlpha.value = 1.0f;
	sc_Panning.selectedSegmentIndex=0;
	sldPanning.value = 0.7f;
	
//	segcont_resumeLaunch.selectedSegmentIndex = 0;
	segcont_oscillo.selectedSegmentIndex = 0;
	segcont_spectrum.selectedSegmentIndex = 0;
	segcont_fx1.selectedSegmentIndex = 0;
	segcont_fx2.selectedSegmentIndex = 0;
	segcont_fx3.selectedSegmentIndex = 0;
    segcont_fx4.selectedSegmentIndex=0;
    segcont_fx5.selectedSegmentIndex = 0;
	segcont_FxBeat.selectedSegmentIndex = 0;
	segcont_randFx.selectedSegmentIndex = 0;
	sc_FXDetail.selectedSegmentIndex = 0;
	segcont_forceMono.selectedSegmentIndex = 0;
	sc_checkBeforeRedownload.selectedSegmentIndex = 0;
	sc_AfterDownload.selectedSegmentIndex = 1;
	sc_EnqueueMode.selectedSegmentIndex = 2;
	sc_DefaultAction.selectedSegmentIndex = 0;
	sc_defaultMODplayer.selectedSegmentIndex = 0;
	sc_PlayerViewOnPlay.selectedSegmentIndex = 0;
    ///////////////////////////////////
    // UADE
    ///////////////////////////////////////
	sc_UADE_Led.selectedSegmentIndex = 0;
	sc_UADE_Norm.selectedSegmentIndex = 0;
	sc_UADE_PostFX.selectedSegmentIndex = 0;
	sc_UADE_Head.selectedSegmentIndex = 0;
	sc_UADE_Pan.selectedSegmentIndex = 0;
	sc_UADE_Gain.selectedSegmentIndex = 0;
	sld_UADEpan.value = 0.7f;
	sld_UADEgain.value = 0.5f;
    ////////////////////////////////////
    // SID
    ///////////////////////////////////////
	sc_SID_Optim.selectedSegmentIndex = 2;
	sc_SID_LibVersion.selectedSegmentIndex = 0;
    sc_SID_Filter.selectedSegmentIndex = 0;
    ////////////////////////////////////
    // SexyPSF
    ///////////////////////////////////////
	sc_SEXYPSF_Reverb.selectedSegmentIndex = 0;
	sc_SEXYPSF_Interpol.selectedSegmentIndex = 0;
    ////////////////////////////////////
    // AOSDK
    ///////////////////////////////////////
	sc_AOSDK_Reverb.selectedSegmentIndex = 0;
	sc_AOSDK_Interpol.selectedSegmentIndex = 0;
    sc_AOSDKDSF22KHZ.selectedSegmentIndex = 1;
	sc_AOSDKDSFDSP.selectedSegmentIndex = 0;
	sc_AOSDKDSFEmuRatio.selectedSegmentIndex = 3;
	sc_AOSDKSSFDSP.selectedSegmentIndex = 0;
	sc_AOSDKSSFEmuRatio.selectedSegmentIndex = 3;
    ////////////////////////////////////
    // ADPLUG
    ///////////////////////////////////////
	sc_ADPLUG_opltype.selectedSegmentIndex = 0;
    ////////////////////////////////////
    // GME
    ///////////////////////////////////////
    ////////////////////////////////////
    // DUMB
    ///////////////////////////////////////    
    sc_DUMBResampling.selectedSegmentIndex = 0;
	sldDUMBMastVol.value = 0.5f;
	
    ////////////////////////////////////
    // TIMIDITY
    ///////////////////////////////////////    
	sc_TIMchorus.selectedSegmentIndex = 0;
	sc_TIMreverb.selectedSegmentIndex = 0;
	sc_TIMfilter.selectedSegmentIndex = 0;
	sc_TIMresample.selectedSegmentIndex = 0;
	sld_TIMPoly.value = 64;
	
    ////////////////////////////////////
    // MODPLUG
    ///////////////////////////////////////
	segcont_shownote.selectedSegmentIndex = 0;
	mastVolSld.value = 0.5f;
	segcont_mpSampling.selectedSegmentIndex = 3;
	segcont_mpMB.selectedSegmentIndex = 0;
	bassAmoSld.value = 0.7f;
	bassRanSld.value = 0.3f;
	segcont_mpSUR.selectedSegmentIndex = 0;
	surDepSld.value = 0.9f;
	surDelSld.value = 0.8f;
	segcont_mpReverb.selectedSegmentIndex = 0;
	revDepSld.value = 0.8f;
	revDelSld.value = 0.7f;
	mpPanningSld.value = 0.5f;
    
    [self updateAllSettingsAfterChange];    
}


-(void)loadSettings:(int)safe_mode{
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
	NSNumber *valNb;
	int not_expected_version;
	
    ///////////////////////////////////
    // General
    ///////////////////////////////////////	
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
	valNb=[prefs objectForKey:@"StatsUpload"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_StatsUpload.selectedSegmentIndex = 1;
	else sc_StatsUpload.selectedSegmentIndex = [valNb intValue];		
    valNb=[prefs objectForKey:@"SpokenTitle"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_SpokenTitle.selectedSegmentIndex = 1;
	else sc_SpokenTitle.selectedSegmentIndex = [valNb intValue];		
	valNb=[prefs objectForKey:@"BGPlayback"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_bgPlay.selectedSegmentIndex = 1;
	else sc_bgPlay.selectedSegmentIndex = [valNb intValue];    
    valNb=[prefs objectForKey:@"TitleFilename"];if (safe_mode) valNb=nil;
    if (valNb == nil) sc_titleFilename.selectedSegmentIndex = 0;
	else sc_titleFilename.selectedSegmentIndex = [valNb intValue];
    
    valNb=[prefs objectForKey:@"ShowDebugInfos"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_showDebug.selectedSegmentIndex = 0;
	else sc_showDebug.selectedSegmentIndex = [valNb intValue];
    valNb=[prefs objectForKey:@"Coverflow"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_cflow.selectedSegmentIndex = 1;
	else sc_cflow.selectedSegmentIndex = [valNb intValue];
    valNb=[prefs objectForKey:@"FXAlpha"];if (safe_mode) valNb=nil;
	if (valNb == nil) sldFxAlpha.value = (mSlowDevice?1.0f:0.5f);
	else sldFxAlpha.value = [valNb floatValue];	
    
    valNb=[prefs objectForKey:@"GLOBPanning"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_Panning.selectedSegmentIndex=0;
	else sc_Panning.selectedSegmentIndex = [valNb intValue];	
    valNb=[prefs objectForKey:@"GLOBPanningValue"];if (safe_mode) valNb=nil;
	if (valNb == nil) sldPanning.value = 0.7f;
	else sldPanning.value = [valNb floatValue];	
	
//	valNb=[prefs objectForKey:@"ResumeOnLaunch"];if (safe_mode) valNb=nil;
//	if (valNb == nil) segcont_resumeLaunch.selectedSegmentIndex = 0;
//	else segcont_resumeLaunch.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"Oscillo"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_oscillo.selectedSegmentIndex = 0;
	else segcont_oscillo.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"2DSpectrum"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_spectrum.selectedSegmentIndex = 0;
	else segcont_spectrum.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"3DFX1"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_fx1.selectedSegmentIndex = 0;
	else segcont_fx1.selectedSegmentIndex = [valNb intValue];	
	valNb=[prefs objectForKey:@"3DFX2"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_fx2.selectedSegmentIndex = 0;
	else segcont_fx2.selectedSegmentIndex = [valNb intValue];	
	valNb=[prefs objectForKey:@"3DFX3"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_fx3.selectedSegmentIndex = 0;
	else segcont_fx3.selectedSegmentIndex = [valNb intValue];
    valNb=[prefs objectForKey:@"FX4"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_fx4.selectedSegmentIndex = 0;
	else segcont_fx4.selectedSegmentIndex = [valNb intValue];
    valNb=[prefs objectForKey:@"FX5"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_fx5.selectedSegmentIndex = 0;
	else segcont_fx5.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"FXBEAT"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_FxBeat.selectedSegmentIndex = 0;
	else segcont_FxBeat.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"RandomFX"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_randFx.selectedSegmentIndex = 0;
	else segcont_randFx.selectedSegmentIndex = [valNb intValue];	
	valNb=[prefs objectForKey:@"FXDetail"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_FXDetail.selectedSegmentIndex = 0;
	else sc_FXDetail.selectedSegmentIndex = [valNb intValue];	
	valNb=[prefs objectForKey:@"ForceMono"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_forceMono.selectedSegmentIndex = 0;
	else segcont_forceMono.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"AllowRedownload"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_checkBeforeRedownload.selectedSegmentIndex = 0;
	else sc_checkBeforeRedownload.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"AfterDownload"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_AfterDownload.selectedSegmentIndex = 1;
	else sc_AfterDownload.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"EnqueueMode"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_EnqueueMode.selectedSegmentIndex = 2;
	else sc_EnqueueMode.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"DefaultAction"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_DefaultAction.selectedSegmentIndex = 0;
	else sc_DefaultAction.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"DefaultMODplayer"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_defaultMODplayer.selectedSegmentIndex = 0;
	else sc_defaultMODplayer.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"PlayerViewOnPlay"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_PlayerViewOnPlay.selectedSegmentIndex = 0;
	else sc_PlayerViewOnPlay.selectedSegmentIndex = [valNb intValue];
	
    valNb=[prefs objectForKey:@"DefaultLength"];if (safe_mode) valNb=nil;
	if (valNb == nil) sld_DefaultLength.value = SONG_DEFAULT_LENGTH/1000;
	else sld_DefaultLength.value = [valNb floatValue];	
    ///////////////////////////////////
    // UADE
    ///////////////////////////////////////
	valNb=[prefs objectForKey:@"UADE_LED"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_UADE_Led.selectedSegmentIndex = 0;
	else sc_UADE_Led.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"UADE_NORM"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_UADE_Norm.selectedSegmentIndex = 0;
	else sc_UADE_Norm.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"UADE_POSTFX"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_UADE_PostFX.selectedSegmentIndex = 1;
	else sc_UADE_PostFX.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"UADE_HEAD"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_UADE_Head.selectedSegmentIndex = 0;
	else sc_UADE_Head.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"UADE_PAN"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_UADE_Pan.selectedSegmentIndex = 1;
	else sc_UADE_Pan.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"UADE_GAIN"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_UADE_Gain.selectedSegmentIndex = 0;
	else sc_UADE_Gain.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"UADE_PANVAL"];if (safe_mode) valNb=nil;
	if (valNb == nil) sld_UADEpan.value = 0.7f;
	else sld_UADEpan.value = [valNb floatValue];	
	valNb=[prefs objectForKey:@"UADE_GAINVAL"];if (safe_mode) valNb=nil;
	if (valNb == nil) sld_UADEgain.value = 0.5f;
	else sld_UADEgain.value = [valNb floatValue];	
    ////////////////////////////////////
    // SID
    ///////////////////////////////////////
	valNb=[prefs objectForKey:@"SIDOptim"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_SID_Optim.selectedSegmentIndex = 1;
	else sc_SID_Optim.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"SIDLibVersion"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_SID_LibVersion.selectedSegmentIndex = (mSlowDevice?0:1);
	else sc_SID_LibVersion.selectedSegmentIndex = [valNb intValue];
    valNb=[prefs objectForKey:@"SIDFilter"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_SID_Filter.selectedSegmentIndex = (mSlowDevice?0:1);
	else sc_SID_Filter.selectedSegmentIndex = [valNb intValue];
    ////////////////////////////////////
    // SexyPSF
    ///////////////////////////////////////
	valNb=[prefs objectForKey:@"SexyPSFReverb"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_SEXYPSF_Reverb.selectedSegmentIndex = 2;
	else sc_SEXYPSF_Reverb.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"SexyPSFInterp"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_SEXYPSF_Interpol.selectedSegmentIndex = 2;
	else sc_SEXYPSF_Interpol.selectedSegmentIndex = [valNb intValue];
    ////////////////////////////////////
    // AOSDK
    ///////////////////////////////////////
	valNb=[prefs objectForKey:@"AOSDKReverb"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_AOSDK_Reverb.selectedSegmentIndex = 1;
	else sc_AOSDK_Reverb.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"AOSDKDSF22KHZ"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_AOSDKDSF22KHZ.selectedSegmentIndex = 1;
	else sc_AOSDKDSF22KHZ.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"AOSDKInterp"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_AOSDK_Interpol.selectedSegmentIndex = 2;
	else sc_AOSDK_Interpol.selectedSegmentIndex = [valNb intValue];
    valNb=[prefs objectForKey:@"AOSDKDSFDSP"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_AOSDKDSFDSP.selectedSegmentIndex = 0;
	else sc_AOSDKDSFDSP.selectedSegmentIndex = [valNb intValue];
    valNb=[prefs objectForKey:@"AOSDKDSFEmuRatio"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_AOSDKDSFEmuRatio.selectedSegmentIndex = 2;
	else sc_AOSDKDSFEmuRatio.selectedSegmentIndex = [valNb intValue];
    valNb=[prefs objectForKey:@"AOSDKSSFDSP"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_AOSDKSSFDSP.selectedSegmentIndex = 0;
	else sc_AOSDKSSFDSP.selectedSegmentIndex = [valNb intValue];
    valNb=[prefs objectForKey:@"AOSDKSSFEmuRatio"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_AOSDKSSFEmuRatio.selectedSegmentIndex = 2;
	else sc_AOSDKSSFEmuRatio.selectedSegmentIndex = [valNb intValue];
    ////////////////////////////////////
    // ADPLUG
    ///////////////////////////////////////
	valNb=[prefs objectForKey:@"ADPLUGopltype"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_ADPLUG_opltype.selectedSegmentIndex = 1;
	else sc_ADPLUG_opltype.selectedSegmentIndex = [valNb intValue];        
    ////////////////////////////////////
    // GME
    ///////////////////////////////////////
    
    ////////////////////////////////////
    // DUMB
    ///////////////////////////////////////    
    valNb=[prefs objectForKey:@"DUMBResampling"];if (safe_mode) valNb=nil;
    if (valNb == nil) sc_DUMBResampling.selectedSegmentIndex = (mSlowDevice? 0:2);
	else sc_DUMBResampling.selectedSegmentIndex = [valNb intValue];    
	valNb=[prefs objectForKey:@"DUMBMastVol"];if (safe_mode) valNb=nil;
	if (valNb == nil) sldDUMBMastVol.value = 0.5f;
	else sldDUMBMastVol.value = [valNb floatValue];
    
    ////////////////////////////////////
    // TIMIDITY
    ///////////////////////////////////////    
    valNb=[prefs objectForKey:@"TIMChorus"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_TIMchorus.selectedSegmentIndex = (mSlowDevice? 0:1);
	else sc_TIMchorus.selectedSegmentIndex = [valNb intValue];    
	valNb=[prefs objectForKey:@"TIMReverb"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_TIMreverb.selectedSegmentIndex = (mSlowDevice? 0:1);
	else sc_TIMreverb.selectedSegmentIndex = [valNb intValue];    
	valNb=[prefs objectForKey:@"TIMFilter"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_TIMfilter.selectedSegmentIndex = (mSlowDevice? 0:1);
	else sc_TIMfilter.selectedSegmentIndex = [valNb intValue];    
	valNb=[prefs objectForKey:@"TIMResample"];if (safe_mode) valNb=nil;
	if (valNb == nil) sc_TIMresample.selectedSegmentIndex = (mSlowDevice? 0:1);
	else sc_TIMresample.selectedSegmentIndex = [valNb intValue];    
	valNb=[prefs objectForKey:@"TIMPoly"];if (safe_mode) valNb=nil;
	if (valNb == nil) sld_TIMPoly.value = (mSlowDevice? 64:128);
	else sld_TIMPoly.value = [valNb intValue];
	
    ////////////////////////////////////
    // MODPLUG
    ///////////////////////////////////////
	valNb=[prefs objectForKey:@"ShowNotes"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_shownote.selectedSegmentIndex = 0;
	else segcont_shownote.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"MasterVol"];if (safe_mode) valNb=nil;
	if (valNb == nil) mastVolSld.value = 0.5f;
	else mastVolSld.value = [valNb floatValue];
	valNb=[prefs objectForKey:@"Resampling"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_mpSampling.selectedSegmentIndex = 0;
	else segcont_mpSampling.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"Megabass"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_mpMB.selectedSegmentIndex = 0;
	else segcont_mpMB.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"MBAmount"];if (safe_mode) valNb=nil;
	if (valNb == nil) bassAmoSld.value = 0.7f;
	else bassAmoSld.value = [valNb floatValue];
	valNb=[prefs objectForKey:@"MBRange"];if (safe_mode) valNb=nil;
	if (valNb == nil) bassRanSld.value = 0.3f;
	else bassRanSld.value = [valNb floatValue];
	valNb=[prefs objectForKey:@"Surround"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_mpSUR.selectedSegmentIndex = 0;
	else segcont_mpSUR.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"SURDepth"];if (safe_mode) valNb=nil;
	if (valNb == nil) surDepSld.value = 0.9f;
	else surDepSld.value = [valNb floatValue];
	valNb=[prefs objectForKey:@"SURDelay"];if (safe_mode) valNb=nil;
	if (valNb == nil) surDelSld.value = 0.8f;
	else surDelSld.value = [valNb floatValue];
	valNb=[prefs objectForKey:@"Reverb"];if (safe_mode) valNb=nil;
	if (valNb == nil) segcont_mpReverb.selectedSegmentIndex = 0;
	else segcont_mpReverb.selectedSegmentIndex = [valNb intValue];
	valNb=[prefs objectForKey:@"REVDepth"];if (safe_mode) valNb=nil;
	if (valNb == nil) revDepSld.value = 0.8f;
	else revDepSld.value = [valNb floatValue];
	valNb=[prefs objectForKey:@"REVDelay"];if (safe_mode) valNb=nil;
	if (valNb == nil) revDelSld.value = 0.7f;
	else revDelSld.value = [valNb floatValue];
	
	valNb=[prefs objectForKey:@"MPPanning"];if (safe_mode) valNb=nil;
	if (valNb == nil) mpPanningSld.value = 0.5f;
	else mpPanningSld.value = [valNb floatValue];
	
	
    ////////////////////////////////////
    // Internal stuff
    /////////////////////////
    valNb=[prefs objectForKey:@"InfoFullscreen"];if (safe_mode) valNb=nil;
	if (valNb == nil) infoIsFullscreen = 0;
	else infoIsFullscreen = [valNb intValue];	
    valNb=[prefs objectForKey:@"PlFullscreen"];if (safe_mode) valNb=nil;
	if (valNb == nil) plIsFullscreen = 0;
	else plIsFullscreen = [valNb intValue];
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
	
    ///////////////////////////////////
    // General
    ///////////////////////////////////////
//	valNb=[[NSNumber alloc] initWithInt:0];
//	[prefs setObject:valNb forKey:@"ModizerRunning"];[valNb autorelease];
	
    valNb=[[NSNumber alloc] initWithInt:(int)mOglViewIsHidden];
	[prefs setObject:valNb forKey:@"ViewFX"];[valNb autorelease];
	
	valNb=[[NSNumber alloc] initWithInt:sc_StatsUpload.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"StatsUpload"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:sc_SpokenTitle.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"SpokenTitle"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_bgPlay.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"BGPlayback"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:sc_titleFilename.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"TitleFilename"];[valNb autorelease];
    
    
    valNb=[[NSNumber alloc] initWithInt:sc_showDebug.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"ShowDebugInfos"];[valNb autorelease];    
    valNb=[[NSNumber alloc] initWithInt:sc_cflow.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"Coverflow"];[valNb autorelease];    
    valNb=[[NSNumber alloc] initWithFloat:sldFxAlpha.value];
	[prefs setObject:valNb forKey:@"FXAlpha"];[valNb autorelease];
    
    valNb=[[NSNumber alloc] initWithInt:sc_Panning.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"GLOBPanning"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithFloat:sldPanning.value];
	[prefs setObject:valNb forKey:@"GLOBPanningValue"];[valNb autorelease];
    
    
	valNb=[[NSNumber alloc] initWithInt:segcont_oscillo.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"Oscillo"];[valNb autorelease];	
	valNb=[[NSNumber alloc] initWithInt:segcont_fx1.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"3DFX1"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:segcont_fx2.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"3DFX2"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:segcont_fx3.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"3DFX3"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:segcont_fx4.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"FX4"];[valNb autorelease];	
    valNb=[[NSNumber alloc] initWithInt:segcont_fx5.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"FX5"];[valNb autorelease];	
	valNb=[[NSNumber alloc] initWithInt:segcont_FxBeat.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"FXBEAT"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:segcont_randFx.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"RandomFX"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:segcont_spectrum.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"2DSpectrum"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_FXDetail.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"FXDetail"];[valNb autorelease];
//	valNb=[[NSNumber alloc] initWithInt:segcont_resumeLaunch.selectedSegmentIndex];
//	[prefs setObject:valNb forKey:@"ResumeOnLaunch"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:segcont_forceMono.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"ForceMono"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_checkBeforeRedownload.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"AllowRedownload"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_AfterDownload.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"AfterDownload"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_EnqueueMode.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"EnqueueMode"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_DefaultAction.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"DefaultAction"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_defaultMODplayer.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"DefaultMODplayer"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_PlayerViewOnPlay.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"PlayerViewOnPlay"];[valNb autorelease];
    
	valNb=[[NSNumber alloc] initWithInt:VERSION_MAJOR];
	[prefs setObject:valNb forKey:@"VERSION_MAJOR"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:VERSION_MINOR];
	[prefs setObject:valNb forKey:@"VERSION_MINOR"];[valNb autorelease];
    
    valNb=[[NSNumber alloc] initWithFloat:sld_DefaultLength.value];
	[prefs setObject:valNb forKey:@"DefaultLength"];[valNb autorelease];
    ///////////////////////////////////
    // UADE
    ///////////////////////////////////////
	valNb=[[NSNumber alloc] initWithInt:sc_UADE_Led.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"UADE_LED"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_UADE_Norm.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"UADE_NORM"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_UADE_PostFX.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"UADE_POSTFX"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_UADE_Pan.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"UADE_PAN"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_UADE_Head.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"UADE_HEAD"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_UADE_Gain.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"UADE_GAIN"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithFloat:sld_UADEpan.value];
	[prefs setObject:valNb forKey:@"UADE_PANVAL"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithFloat:sld_UADEgain.value];
	[prefs setObject:valNb forKey:@"UADE_GAINVAL"];[valNb autorelease];
    ///////////////////////////////////
    // SID
    ///////////////////////////////////////
	valNb=[[NSNumber alloc] initWithInt:sc_SID_Optim.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"SIDOptim"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_SID_LibVersion.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"SIDLibVersion"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:sc_SID_Filter.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"SIDFilter"];[valNb autorelease];
    ////////////////////////////////////
    // SexyPSF
    ///////////////////////////////////////
	valNb=[[NSNumber alloc] initWithInt:sc_SEXYPSF_Reverb.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"SexyPSFReverb"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_SEXYPSF_Interpol.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"SexyPSFInterp"];[valNb autorelease];
    ////////////////////////////////////
    // AOSDK
    ///////////////////////////////////////
	valNb=[[NSNumber alloc] initWithInt:sc_AOSDK_Reverb.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"AOSDKReverb"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:sc_AOSDK_Interpol.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"AOSDKInterp"];[valNb autorelease];
    
    valNb=[[NSNumber alloc] initWithInt:sc_AOSDKDSF22KHZ.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"AOSDKDSF22KHZ"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:sc_AOSDKDSFDSP.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"AOSDKDSFDSP"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:sc_AOSDKDSFEmuRatio.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"AOSDKDSFEmuRatio"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:sc_AOSDKSSFDSP.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"AOSDKSSFDSP"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:sc_AOSDKSSFEmuRatio.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"AOSDKSSFEmuRatio"];[valNb autorelease];
    
    ////////////////////////////////////
    // ADPLUG
    ///////////////////////////////////////
    valNb=[[NSNumber alloc] initWithInt:sc_ADPLUG_opltype.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"ADPLUGopltype"];[valNb autorelease];
    ///////////////////////////////////
    // GME
    ///////////////////////////////////////
    ////////////////////////////////////
    // DUMB
    ///////////////////////////////////////    
    valNb=[[NSNumber alloc] initWithInt:sc_DUMBResampling.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"DUMBResampling"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithFloat:sldDUMBMastVol.value];
	[prefs setObject:valNb forKey:@"DUMBMastVol"];[valNb autorelease];
    
    ////////////////////////////////////
    // TIMIDITY
    ///////////////////////////////////////    
    
    valNb=[[NSNumber alloc] initWithInt:sc_TIMchorus.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"TIMChorus"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:sc_TIMreverb.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"TIMReverb"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:sc_TIMfilter.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"TIMFilter"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:sc_TIMresample.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"TIMResample"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:(int)(sld_TIMPoly.value)];
	[prefs setObject:valNb forKey:@"TIMPoly"];[valNb autorelease];
	
    ///////////////////////////////////
    // MODPLUG
    ///////////////////////////////////////
 	valNb=[[NSNumber alloc] initWithFloat:mastVolSld.value];
	[prefs setObject:valNb forKey:@"MasterVol"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:segcont_shownote.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"ShowNotes"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:segcont_mpSampling.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"Resampling"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:segcont_mpMB.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"Megabass"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithFloat:bassAmoSld.value];
	[prefs setObject:valNb forKey:@"MBAmount"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithFloat:bassRanSld.value];
	[prefs setObject:valNb forKey:@"MBRange"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:segcont_mpSUR.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"Surround"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithFloat:surDepSld.value];
	[prefs setObject:valNb forKey:@"SURDepth"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithFloat:surDelSld.value];
	[prefs setObject:valNb forKey:@"SURDelay"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:segcont_mpReverb.selectedSegmentIndex];
	[prefs setObject:valNb forKey:@"Reverb"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithFloat:revDepSld.value];
	[prefs setObject:valNb forKey:@"REVDepth"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithFloat:revDelSld.value];
	[prefs setObject:valNb forKey:@"REVDelay"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithFloat:mpPanningSld.value];
	[prefs setObject:valNb forKey:@"MPPanning"];[valNb autorelease];
    ///////////////////////////////////
    // Internal stuff
    ///////////////////////////////////////
    valNb=[[NSNumber alloc] initWithInt:infoIsFullscreen];
	[prefs setObject:valNb forKey:@"InfoFullscreen"];[valNb autorelease];
    valNb=[[NSNumber alloc] initWithInt:plIsFullscreen];
	[prefs setObject:valNb forKey:@"PlFullscreen"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:oglViewFullscreen];
	[prefs setObject:valNb forKey:@"OGLFullscreen"];[valNb autorelease];
    
	
	valNb=[[NSNumber alloc] initWithInt:mLoopMode];
	[prefs setObject:valNb forKey:@"LoopMode"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:mplayer.mLoopMode];
	[prefs setObject:valNb forKey:@"LoopModeInf"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:mShuffle];
	[prefs setObject:valNb forKey:@"Shuffle"];[valNb autorelease];
    
	
	valNb=[[NSNumber alloc] initWithBool:mIsPlaying];
	[prefs setObject:valNb forKey:@"IsPlaying"];[valNb autorelease];
	
	valNb=[[NSNumber alloc] initWithInt:mPlaylist_size];
	[prefs setObject:valNb forKey:@"PlaylistSize"];[valNb autorelease];
	
	valNb=[[NSNumber alloc] initWithInt:mPlaylist_pos];
	[prefs setObject:valNb forKey:@"PlaylistPos"];[valNb autorelease];
	
	valNb=[[NSNumber alloc] initWithInt:[mplayer getCurrentTime]];
	[prefs setObject:valNb forKey:@"PlayingPos"];[valNb autorelease];
    
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
	[prefs setObject:valNb forKey:@"Subsongs"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:mplayer.mod_currentsub];
	[prefs setObject:valNb forKey:@"Cur_subsong"];[valNb autorelease];
    
    valNb=[[NSNumber alloc] initWithInt:[mplayer getArcEntriesCnt]];
	[prefs setObject:valNb forKey:@"ArchiveCnt"];[valNb autorelease];
	valNb=[[NSNumber alloc] initWithInt:[mplayer getArcIndex]];
	[prefs setObject:valNb forKey:@"ArchiveIndex"];[valNb autorelease];
    
	
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
	NSString *machine = [[[NSString alloc] initWithCString:name] autorelease];
	
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

- (void) checkAvailableCovers {
    NSString *pathFolderImgPNG,*pathFileImgPNG,*pathFolderImgJPG,*pathFileImgJPG,*pathFolderImgGIF,*pathFileImgGIF,*filePath,*basePath;
    NSFileManager *fileMngr=[[NSFileManager alloc] init];
    for (int i=0;i<mPlaylist_size;i++) {
        mPlaylist[i].cover_flag=0; //used for cover flag
        filePath=mPlaylist[i].mPlaylistFilepath;
        basePath=[filePath stringByDeletingLastPathComponent];        
        pathFolderImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.png",basePath];
        pathFolderImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.jpg",basePath];
        pathFolderImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.gif",basePath];
        basePath=[filePath stringByDeletingPathExtension];
        pathFileImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.png",basePath];
        pathFileImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.jpg",basePath];
        pathFileImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@.gif",basePath];
        if ([fileMngr isReadableFileAtPath:pathFileImgJPG]) mPlaylist[i].cover_flag=1;
        else if ([fileMngr isReadableFileAtPath:pathFileImgPNG]) mPlaylist[i].cover_flag=2;
        else if ([fileMngr isReadableFileAtPath:pathFileImgGIF]) mPlaylist[i].cover_flag=4;
        else if ([fileMngr isReadableFileAtPath:pathFolderImgJPG]) mPlaylist[i].cover_flag=8;
        else if ([fileMngr isReadableFileAtPath:pathFolderImgPNG]) mPlaylist[i].cover_flag=16;
        else if ([fileMngr isReadableFileAtPath:pathFolderImgGIF]) mPlaylist[i].cover_flag=32;
	}
    [fileMngr release];
}

- (void) checkAvailableCovers:(int)index {
    NSString *pathFolderImgPNG,*pathFileImgPNG,*pathFolderImgJPG,*pathFileImgJPG,*pathFolderImgGIF,*pathFileImgGIF,*filePath,*basePath;
    NSFileManager *fileMngr=[[NSFileManager alloc] init];
    
    
    mPlaylist[index].cover_flag=0; //used for cover flag
    filePath=mPlaylist[index].mPlaylistFilepath;
    basePath=[filePath stringByDeletingLastPathComponent];        
    pathFolderImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.png",basePath];
    pathFolderImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.jpg",basePath];
    pathFolderImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.gif",basePath];
    basePath=[filePath stringByDeletingPathExtension];
    pathFileImgPNG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.png",basePath];
    pathFileImgJPG=[NSHomeDirectory() stringByAppendingFormat:@"/%@.jpg",basePath];
    pathFileImgGIF=[NSHomeDirectory() stringByAppendingFormat:@"/%@.gif",basePath];
    if ([fileMngr isReadableFileAtPath:pathFileImgJPG]) mPlaylist[index].cover_flag=1;
    else if ([fileMngr isReadableFileAtPath:pathFileImgPNG]) mPlaylist[index].cover_flag=2;
    else if ([fileMngr isReadableFileAtPath:pathFileImgGIF]) mPlaylist[index].cover_flag=4;
    else if ([fileMngr isReadableFileAtPath:pathFolderImgJPG]) mPlaylist[index].cover_flag=8;
    else if ([fileMngr isReadableFileAtPath:pathFolderImgPNG]) mPlaylist[index].cover_flag=16;
    else if ([fileMngr isReadableFileAtPath:pathFolderImgGIF]) mPlaylist[index].cover_flag=32;
    [fileMngr release];
}

-(void)showWaiting{
//	waitingView.hidden=FALSE;
    UIWindow *window=[[UIApplication sharedApplication] keyWindow];		
	[window addSubview:waitingView];
}

-(void)hideWaiting{
//	waitingView.hidden=TRUE;
    [waitingView removeFromSuperview];    
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
	clock_t start_time,end_time;	
	start_time=clock();	
    
    shouldRestart=1;
    m_displayLink=nil;
    
    gifAnimation=nil;
    cover_view.contentMode=UIViewContentModeScaleAspectFill;
    
    [UIView setAnimationDelegate:self];
    [UIView setAnimationDidStopSelector:@selector(animationDidStop:)];
    
    UILongPressGestureRecognizer *longPressPaPrevSGesture = [[[UILongPressGestureRecognizer alloc] 
                                                             initWithTarget:self 
                                                             action:@selector(longPressPrevSubArc:)] autorelease];
    UILongPressGestureRecognizer *longPressPaNextSGesture = [[[UILongPressGestureRecognizer alloc] 
                                                             initWithTarget:self 
                                                             action:@selector(longPressNextSubArc:)] autorelease];
    UILongPressGestureRecognizer *longPressPlPrevSGesture = [[[UILongPressGestureRecognizer alloc] 
                                                             initWithTarget:self 
                                                             action:@selector(longPressPrevSubArc:)] autorelease];
    UILongPressGestureRecognizer *longPressPlNextSGesture = [[[UILongPressGestureRecognizer alloc] 
                                                             initWithTarget:self 
                                                             action:@selector(longPressNextSubArc:)] autorelease];
    
    
    [[[pauseBarSub subviews] objectAtIndex:2] addGestureRecognizer:longPressPaPrevSGesture];
    [[[pauseBarSub subviews] objectAtIndex:4] addGestureRecognizer:longPressPaNextSGesture];
    [[[playBarSub subviews] objectAtIndex:2] addGestureRecognizer:longPressPlPrevSGesture];
    [[[playBarSub subviews] objectAtIndex:4] addGestureRecognizer:longPressPlNextSGesture];
    
/*    for (int i=0;i<[[pauseBarSub subviews] count];i++) {
        UIView *v=[[pauseBarSub subviews] objectAtIndex:i];
        NSLog(@"idx:%d - x:%f",i,v.frame.origin.x);
    }*/        
    
    labelModuleName.userInteractionEnabled = YES;
    UITapGestureRecognizer *tapGesture =
    [[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(titleTap:)] autorelease];
    [labelModuleName addGestureRecognizer:tapGesture];
	
	mPlaylist=(t_plPlaylist_entry*)malloc(MAX_PL_ENTRIES*sizeof(t_plPlaylist_entry));
    
	self.navigationItem.title=@"Modizer";
	//	self.navigationItem.backBarButtonItem.title=@"dd";
	
    /*	self.navigationItem.rightBarButtonItem=[[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"playlist.png"]
     style:UIBarButtonItemStylePlain 
     target:self
     action:@selector(showPlaylist)] autorelease];
     */
	[self.navigationItem setRightBarButtonItem:[[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@"playlist.png"]
																				 style:UIBarButtonItemStylePlain 
																				target:self
																				action:@selector(showPlaylist)] autorelease] 
									  animated:YES];
    mRandomFXCpt=0;
	mRandomFXCptRev=0;
	mHasFocus=0;
	mShouldUpdateInfos=0;
	mPaused=1;
	mScaleFactor=1.0f;
	if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad) {
		mDeviceType=1; //ipad
		mDevice_hh=1024;
		mDevice_ww=768;
	}
	else {
		
		mDeviceType=0; //iphone   (iphone 4 res currently not handled)
		mDevice_hh=480;
		mDevice_ww=320;
		UIScreen* mainscr = [UIScreen mainScreen];
		if ([mainscr respondsToSelector:@selector(currentMode)]) {
			if (mainscr.currentMode.size.width>480) {  //iphone 4
				mDeviceType=2;
				mScaleFactor=(float)mainscr.currentMode.size.width/480.0f;
				// mDevice_ww = mainscr.currentMode.size.width;
				// mDevice_hh = mainscr.currentMode.size.height;
			}
		}
		
	}
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
    [covers_default retain];
    [coverflow setNumberOfCovers:0];
    
    //
    lblMainCoverflow=[[UILabel alloc] init];
    lblSecCoverflow=[[UILabel alloc] init];
    
    lblCurrentSongCFlow=[[UILabel alloc] init];
    lblTimeFCflow=[[UILabel alloc] init];
    btnPlayCFlow=[UIButton buttonWithType:UIButtonTypeCustom];
    btnPauseCFlow=[UIButton buttonWithType:UIButtonTypeCustom];
    btnBackCFlow=[UIButton buttonWithType:UIButtonTypeCustom];
    
    [btnPlayCFlow setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    btnPlayCFlow.backgroundColor = [UIColor colorWithRed:0.22 green:0.18 blue:0.22 alpha:1.0];
    btnPlayCFlow.layer.borderColor = [UIColor blackColor].CGColor;
    btnPlayCFlow.layer.borderWidth = 0.5f;
    btnPlayCFlow.layer.cornerRadius = 14.0f;
    
    [btnPauseCFlow setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    btnPauseCFlow.backgroundColor = [UIColor colorWithRed:0.2 green:0.15 blue:0.2 alpha:1.0];
    btnPauseCFlow.layer.borderColor = [UIColor blackColor].CGColor;
    btnPauseCFlow.layer.borderWidth = 0.5f;
    btnPauseCFlow.layer.cornerRadius = 14.0f;
    
    [btnPlayCFlow setImage:[UIImage imageNamed:@"btnplay.png"] forState:UIControlStateNormal];
    [btnPlayCFlow setImage:[UIImage imageNamed:@"btnplay.png"] forState:UIControlStateHighlighted];    
    [btnPlayCFlow addTarget: self action: @selector(playPushed:) forControlEvents: UIControlEventTouchUpInside];
    [btnPauseCFlow setImage:[UIImage imageNamed:@"btnpause.png"] forState:UIControlStateNormal];
    [btnPauseCFlow setImage:[UIImage imageNamed:@"btnpause.png"] forState:UIControlStateHighlighted];    
    [btnPauseCFlow addTarget: self action: @selector(pausePushed:) forControlEvents: UIControlEventTouchUpInside];
    [btnBackCFlow setImage:[UIImage imageNamed:@"btnback.png"] forState:UIControlStateNormal];
    [btnBackCFlow setImage:[UIImage imageNamed:@"btnback.png"] forState:UIControlStateHighlighted];    
    [btnBackCFlow addTarget: self action: @selector(backPushed:) forControlEvents: UIControlEventTouchUpInside];
    
    lblMainCoverflow.hidden=TRUE;
    lblSecCoverflow.hidden=TRUE;
    lblCurrentSongCFlow.hidden=TRUE;
    lblTimeFCflow.hidden=TRUE;
    btnPlayCFlow.hidden=TRUE;
    btnPauseCFlow.hidden=TRUE;
    btnBackCFlow.hidden=TRUE;
    
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
    
    
    lblMainCoverflow.textAlignment=UITextAlignmentCenter;
    lblSecCoverflow.textAlignment=UITextAlignmentCenter;
    lblCurrentSongCFlow.textAlignment=UITextAlignmentLeft;
    lblTimeFCflow.textAlignment=UITextAlignmentRight;
    
    lblMainCoverflow.lineBreakMode=UILineBreakModeMiddleTruncation;
    lblSecCoverflow.lineBreakMode=UILineBreakModeMiddleTruncation;
    lblCurrentSongCFlow.lineBreakMode=UILineBreakModeMiddleTruncation;
    lblTimeFCflow.lineBreakMode=UILineBreakModeMiddleTruncation;
    
    [self.view addSubview:coverflow];
    [self.view addSubview:lblMainCoverflow];
    [self.view addSubview:lblSecCoverflow];
    [self.view addSubview:lblCurrentSongCFlow];
    [self.view addSubview:lblTimeFCflow];
    [self.view addSubview:btnPlayCFlow];
    [self.view addSubview:btnPauseCFlow];
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
	
	/* Popup stuff */
    [[infoButton layer] setCornerRadius:10.0];	
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
	
	for (int i=0;i<2;i++) viewTapInfoStr[i]=NULL;
	
	mShouldHaveFocus=0;
    
    //Init pickerview linebreakmode
    //    pvArcSel.inputView.
    //    .lineBreakMode

    
	
	orientationHV=(int)UIInterfaceOrientationPortrait;
	mPlaylist_size=0;
	mIsPlaying=FALSE;
	oglViewFullscreenChanged=0;
    mOglViewIsHidden=YES;
	mRating=0;
	
	infoZoom.hidden=NO;
	plZoom.hidden=NO;
	infoUnzoom.hidden=YES;
	plUnzoom.hidden=YES;
	
	volWin.frame= CGRectMake(0, mDevice_hh-64-42, mDevice_ww, 44);
	volumeView = [[[MPVolumeView alloc] initWithFrame:volWin.bounds] autorelease];
//	volumeView.center = CGPointMake(mDevice_ww/2,32);
//  [volumeView setShowsRouteButton:YES];
//	[volumeView sizeToFit];
	[volWin addSubview:volumeView];

	
	mRestart=0;
	mRestart_sub=0;
	
	[sliderProgressModule.layer setCornerRadius:8.0];
	[labelSeeking.layer setCornerRadius:8.0];
	[labelTime.layer setCornerRadius:8.0];
	[commandViewU.layer setCornerRadius:8.0];
	
	textMessage.font = [UIFont fontWithName:@"Courier-Bold" size:(mDeviceType==1?18:12)];
	
	playlistTabView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	playlistTabView.separatorStyle = UITableViewCellSeparatorStyleNone;
    playlistTabView.rowHeight = (mDeviceType==1?34:24);
    playlistTabView.backgroundColor = [UIColor clearColor];
	playlistView.hidden=YES;

/////////////////////////////////////    
// Waiting view
/////////////////////////////////////
    waitingView = [[UIView alloc] initWithFrame:CGRectMake(mDevice_ww/2-60,mDevice_hh/2-60,120,110)];
	waitingView.backgroundColor=[UIColor blackColor];//[UIColor colorWithRed:0 green:0 blue:0 alpha:0.8f];
	waitingView.opaque=YES;
	waitingView.hidden=FALSE;
	waitingView.layer.cornerRadius=20;
	
	UIActivityIndicatorView *indView=[[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(40,20,37,37)];
	indView.activityIndicatorViewStyle=UIActivityIndicatorViewStyleWhiteLarge;
	[waitingView addSubview:indView];
    UILabel *lblLoading=[[UILabel alloc] initWithFrame:CGRectMake(10,70,100,20)];
    lblLoading.text=@"Loading";
    lblLoading.backgroundColor=[UIColor blackColor];
    lblLoading.opaque=YES;
    lblLoading.textColor=[UIColor whiteColor];
    lblLoading.textAlignment=UITextAlignmentCenter;
    lblLoading.font=[UIFont italicSystemFontOfSize:18];
    [waitingView addSubview:lblLoading];
    [lblLoading autorelease];
    
	[indView startAnimating];		
	[indView autorelease];

		
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
    
    //FLUID
    if (mDevice_ww==320) initFluid(40,40);
    else initFluid(64,64);
	
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
	txtMenuHandle[0]=TextureUtils::Create([UIImage imageNamed:@"txtMenu1.png"]);
	txtMenuHandle[1]=TextureUtils::Create([UIImage imageNamed:@"txtMenu2.png"]);
	txtMenuHandle[2]=TextureUtils::Create([UIImage imageNamed:@"txtMenu3.png"]);
	txtMenuHandle[3]=TextureUtils::Create([UIImage imageNamed:@"txtMenu4.png"]);
	txtMenuHandle[4]=TextureUtils::Create([UIImage imageNamed:@"txtMenu5.png"]);
	txtMenuHandle[5]=TextureUtils::Create([UIImage imageNamed:@"txtMenu6.png"]);
	txtMenuHandle[6]=TextureUtils::Create([UIImage imageNamed:@"txtMenu7.png"]);
    txtMenuHandle[7]=TextureUtils::Create([UIImage imageNamed:@"txtMenu8.png"]);
    txtMenuHandle[8]=TextureUtils::Create([UIImage imageNamed:@"txtMenu9.png"]);
    txtMenuHandle[9]=TextureUtils::Create([UIImage imageNamed:@"txtMenu10.png"]);
    txtMenuHandle[10]=TextureUtils::Create([UIImage imageNamed:@"txtMenu11.png"]);
    txtMenuHandle[14]=TextureUtils::Create([UIImage imageNamed:@"txtMenu0.png"]);
		
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
	
    //	fliteTTS=[[FliteTTS alloc] init];
	
    //low level audio stuff
    
    //	textMessage.layer.cornerRadius=10;
	
    //FFT	
    for (int i=0;i<SPECTRUM_BANDS;i++)
        for (int j=0;j<8;j++) {
            real_spectrumSumL[i][j]=real_spectrumSumR[i][j]=0;
        }
    
	end_time=clock();	
#ifdef LOAD_PROFILE	
	NSLog(@"detail7 : %d",end_time-start_time);
#endif	
    
    
    
    
	
	
	int size_chan=12*6;
	if (segcont_shownote.selectedSegmentIndex==2) size_chan=6*6;
	visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN)/size_chan;
	mMoveStartChanLeft=mMoveStartChanRight=0;
	
	if ([self checkFlagOnStartup]) {
		alertCrash = [[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning" ,@"")
                                                 message:NSLocalizedString(@"First launch or issue encountered last time Modizer was running. Apply default settings ?",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
		if (alertCrash) [alertCrash show];
		[self loadSettings:1];
		mShouldUpdateInfos=1;
	} else [self loadSettings:0];
    
    /*update covers*/
//    [self checkAvailableCovers];
    for (int i=0;i<mPlaylist_size;i++) mPlaylist[i].cover_flag=-1;

	
	end_time=clock();	
#ifdef LOAD_PROFILE	
	NSLog(@"detail8 : %d",end_time-start_time);
#endif	
 
    [self.view bringSubviewToFront:infoMsgView];

	
	
	[super viewDidLoad];
	end_time=clock();	
#ifdef LOAD_PROFILE	
	NSLog(@"detail : %d",end_time-start_time);
#endif
}

- (void)dealloc {
    [waitingView removeFromSuperview];
    [waitingView release];
    
    
    [coverflow release];
    if (covers_default) {
        [covers_default release];
        covers_default=nil;
    }
    
	if (locationLastUpdate) [locationLastUpdate release];
	
	[repeatingTimer invalidate];
	repeatingTimer = nil; // ensures we never invalidate an already invalid Timer
	
	[mplayer Stop];
	[mplayer release];
    
    [detailItem release];
    [detailDescriptionLabel release];
	
	for (int i=0;i<mPlaylist_size;i++) {
		[mPlaylist[i].mPlaylistFilename autorelease];
		[mPlaylist[i].mPlaylistFilepath autorelease];
	}
	free(mPlaylist);
	mPlaylist_size=0;
    
    //Fluid
    closeFluid();
    
	[super dealloc];
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
    
	if (mPlaylist_size) {
		//DBHelper::getFilesStatsDBmod(mPlaylist,mPlaylist_size);
        for (int i=0;i<mPlaylist_size;i++) {  //reset rating to force resynchro (for ex, user delted an entry in 'favorites' list, thus reseting the rating for a given file
            mPlaylist[i].mPlaylistRating=-1;
        }
        //Check rating for current entry
        DBHelper::getFileStatsDBmod(mPlaylist[mPlaylist_pos].mPlaylistFilename,
                                    mPlaylist[mPlaylist_pos].mPlaylistFilepath,
                                    NULL,
                                    &(mPlaylist[mPlaylist_pos].mPlaylistRating),
                                    NULL,
                                    NULL);
		[self showRating:mPlaylist[mPlaylist_pos].mPlaylistRating];
        //update playlist
		[playlistTabView reloadData];
		NSIndexPath *myindex=[[[NSIndexPath alloc] initWithIndex:0] autorelease];
		[self.playlistTabView selectRowAtIndexPath:[myindex indexPathByAddingIndex:mPlaylist_pos] animated:FALSE scrollPosition:UITableViewScrollPositionMiddle];
	}
    
    if (shouldRestart) {
        shouldRestart=0;
        //[self checkAvailableCovers];
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
    [m_oglView bind];
	
    //	self.navigationController.navigationBar.hidden = YES;
    m_displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(doFrame)];
    m_displayLink.frameInterval = (mSlowDevice?4:2); //15 or 30 fps depending on device speed iPhone
	[m_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
	
	[self updateBarPos];	
	//Hack to allow UIToolbar to be set up correctly
	if (((UIInterfaceOrientation)orientationHV==UIInterfaceOrientationPortrait) || ((UIInterfaceOrientation)orientationHV==UIInterfaceOrientationPortraitUpsideDown) ) {
		[self willAnimateRotationToInterfaceOrientation:UIInterfaceOrientationLandscapeLeft duration:0];
    } else {
        if (coverflow.hidden==FALSE) {
            [[self navigationController] setNavigationBarHidden:YES animated:NO];
        }
        [self willAnimateRotationToInterfaceOrientation:UIInterfaceOrientationPortrait duration:0];        
    }
    
	if (mSavePlaylist) {
		mSavePlaylist=0;
		if (mValidatePlName) {
			mValidatePlName=0;
            
			[rootViewControllerIphone addListToPlaylistDB:[rootViewControllerIphone initNewPlaylistDB:self.inputText.text]
                                                  entries:mPlaylist nb_entries:mPlaylist_size ];
		}
	}
	
}


- (void)viewWillDisappear:(BOOL)animated {
    if (m_displayLink) [m_displayLink invalidate];
	self.navigationController.navigationBar.hidden = NO;
	
	for (int i=0;i<2;i++) if (viewTapInfoStr[i]) {
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
	[UIView beginAnimations:@"player_appear1" context:nil];
	[UIView setAnimationDelay:0.3];
	[UIView setAnimationDuration:0.70];
	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.navigationItem.rightBarButtonItem.customView cache:YES];
	[UIView commitAnimations];
	
	[UIView beginAnimations:@"player_appear2" context:nil];
	[UIView setAnimationDelay:0.5];
	[UIView setAnimationDuration:0.70];
	[UIView setAnimationDelegate:self];
	[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.infoButton cache:YES];
	[UIView commitAnimations];
    
	if (btnShowSubSong.hidden==false) {
		[UIView beginAnimations:@"player_appear3" context:nil];
		[UIView setAnimationDelay:0.75];
		[UIView setAnimationDuration:0.70];
		[UIView setAnimationDelegate:self];
		[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.btnShowSubSong cache:YES];
		[UIView commitAnimations];
	}
    if (btnShowArcList.hidden==false) {
		[UIView beginAnimations:@"player_appear4" context:nil];
		[UIView setAnimationDelay:0.75];
		[UIView setAnimationDuration:0.70];
		[UIView setAnimationDelegate:self];
		[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight  forView:self.btnShowArcList cache:YES];
		[UIView commitAnimations];
	}
    
    if (sc_cflow.selectedSegmentIndex) {
        if (coverflow_needredraw||(coverflow_plsize!=mPlaylist_size)) {
            coverflow_plsize=mPlaylist_size;
            coverflow_pos=mPlaylist_pos;    
            coverflow_needredraw=0;
            [coverflow setNumberOfCovers:mPlaylist_size pos:coverflow_pos];
        }
        if (coverflow_pos!=mPlaylist_pos) {
            coverflow_pos=mPlaylist_pos;
            coverflow.currentIndex=mPlaylist_pos;
        }
    }
    [rootViewControllerIphone hideAllWaitingPopup];
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

- (void)doFrame {
	uint ww,hh;
	int nb_spectrum_bands;
	uint hasdrawnotes;
	char str_data[200];
	unsigned int numRows,numRowsP,numRowsN,cnote,cinst,ceff,cparam,cvol,endChan;
	int i,j,k,l,note_avail,idx,startRow;
	int linestodraw,midline;
	ModPlugNote *currentNotes,*prevNotes,*nextNotes,*readNote;
	int size_chan=12*6;
	if (segcont_shownote.selectedSegmentIndex==2) size_chan=6*6;
    
	
	if (self.mainView.hidden) return;
	if (m_oglView.hidden) return;
    if (coverflow.hidden==FALSE) return;
	
	if (!mFont) {
		NSString *fontPath = [[NSBundle mainBundle] pathForResource:  @"consolas8" ofType: @"fnt"];	
		mFont = new CFont([fontPath cStringUsingEncoding:1]);
	}
	if (!viewTapInfoStr[0]) viewTapInfoStr[0]= new CGLString("Exit", mFont);
//	if (!viewTapInfoStr[1]) viewTapInfoStr[1]= new CGLString("Off", mFont);
	
	
	//get ogl view dimension
	ww=m_oglView.frame.size.width;
	hh=m_oglView.frame.size.height;
    
    //force midifx if in pattern mode & midi being played
    if ((mplayer.mPlayType==15)&&(segcont_shownote.selectedSegmentIndex)&&(segcont_shownote.selectedSegmentIndex<3)) segcont_shownote.selectedSegmentIndex=3;
	
	if (m_oglView->m_touchcount==1) { 
        touch_cpt++;
		movePx+=m_oglView->currentMove.x;
        movePy+=m_oglView->currentMove.y;
        
		m_oglView->previousTouchLocation.x=m_oglView->previousTouchLocation.x+m_oglView->currentMove.x;
        m_oglView->previousTouchLocation.y=m_oglView->previousTouchLocation.y+m_oglView->currentMove.y;
		m_oglView->currentMove.x=0;
        m_oglView->currentMove.y=0;
        if ((mplayer.mPatternDataAvail)&&(segcont_shownote.selectedSegmentIndex<3)) {//pattern display
            if (visibleChan<mplayer.numChannels) {
                if (movePx>0) movePx=0;
                if (movePx<-(mplayer.numChannels-visibleChan)*size_chan) movePx=-(mplayer.numChannels-visibleChan)*size_chan;
                startChan=-movePx/size_chan;
                
            } else movePx=0;
            
        } else if (((mplayer.mPatternDataAvail)||(mplayer.mPlayType==15))&&(segcont_shownote.selectedSegmentIndex>2)) {
            int moveRPx,moveRPy;
            int note_fx_linewidth;
            
            
            if (segcont_shownote.selectedSegmentIndex==4) {
                moveRPx=movePy;
                moveRPy=-movePx;
                note_fx_linewidth=ww/tim_midifx_note_range;
            } else {                
                moveRPx=movePx;
                moveRPy=movePy;
                note_fx_linewidth=hh/tim_midifx_note_range;
            }
            if (touch_cpt==1) moveRPx=0; //hack to reset zoom on touch start
            
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
            
            if (tim_midifx_note_range<88) {
                tim_midifx_note_offset=((88-tim_midifx_note_range)>>1)*note_fx_linewidth;
            }
            
            if (tim_midifx_note_range<128) {
                int maxofs=(128-tim_midifx_note_range)*note_fx_linewidth;
                tim_midifx_note_offset=moveRPy;
                if (tim_midifx_note_offset<0) tim_midifx_note_offset=0;
                if (tim_midifx_note_offset>maxofs) tim_midifx_note_offset=maxofs;                
            } else tim_midifx_note_offset=0;
            moveRPy=tim_midifx_note_offset;
            
            if (segcont_shownote.selectedSegmentIndex==4) {
                movePx=-moveRPy;
                movePy=moveRPx;
                note_fx_linewidth=ww/tim_midifx_note_range;
            } else {                
                movePx=moveRPx;
                movePy=moveRPy;
                note_fx_linewidth=hh/tim_midifx_note_range;
            }
        }
	} else {
		touch_cpt=0;
		if ((m_oglView->m_touchcount==2)&&(m_oglView->m_poptrigger==FALSE)) { //2 fingers : fullscreen switch
			m_oglView->m_poptrigger=TRUE;
			oglViewFullscreen^=1;
			oglViewFullscreenChanged=1;
			[self shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)orientationHV];
		}
	}
    
	//check for click
	if (m_oglView->m_1clicked) {
		m_oglView->m_1clicked=FALSE;
		m_oglView->m_poptrigger=TRUE;
		
		if (viewTapHelpShow==1) {
			viewTapHelpShow=0;
			int tlx=m_oglView->previousTouchLocation.x;
			int tly=m_oglView->previousTouchLocation.y;
            int touched_cellX=tlx*4/ww;
            int touched_cellY=tly*4/hh;
            int touched_coord=(touched_cellX<<4)|(touched_cellY);
            
            if (touched_coord==0x00) {
				int val=segcont_fx1.selectedSegmentIndex;
				val++;
				if (val>=2) val=0;
				segcont_fx1.selectedSegmentIndex=val;
			} else if (touched_coord==0x10) {
				int val=segcont_fx2.selectedSegmentIndex;
				val++;
				if (val>=4) val=0;
				segcont_fx2.selectedSegmentIndex=val;
                segcont_fx3.selectedSegmentIndex=0;
                segcont_fx4.selectedSegmentIndex=0;
                segcont_fx5.selectedSegmentIndex=0;
			} else if (touched_coord==0x20) {
				int val=segcont_fx3.selectedSegmentIndex;
				val++;
				if (val>=4) val=0;
				segcont_fx3.selectedSegmentIndex=val;
                segcont_fx2.selectedSegmentIndex=0;
                segcont_fx4.selectedSegmentIndex=0;
                segcont_fx5.selectedSegmentIndex=0;
			} else if (touched_coord==0x30) {
				int val=segcont_spectrum.selectedSegmentIndex;
				val++;
				if (val>=3) val=0;
				segcont_spectrum.selectedSegmentIndex=val;
			} else if (touched_coord==0x01) {
				int val=segcont_oscillo.selectedSegmentIndex;
				val++;
				if (val>=3) val=0;
				segcont_oscillo.selectedSegmentIndex=val;
			} else if (touched_coord==0x11) {
				int val=segcont_FxBeat.selectedSegmentIndex;
				val++;
				if (val>=2) val=0;
				segcont_FxBeat.selectedSegmentIndex=val;
			} else if (touched_coord==0x21) {
				int val=segcont_shownote.selectedSegmentIndex;
                if (val>=3) val=0; //if bar mode was active, reset
				val++;
				if (val>=3) val=0;
				segcont_shownote.selectedSegmentIndex=val;
                
                size_chan=12*6;
                if (segcont_shownote.selectedSegmentIndex==2) size_chan=6*6;
                visibleChan=(m_oglView.frame.size.width-NOTES_DISPLAY_LEFTMARGIN)/size_chan;
                if (startChan>mplayer.numChannels-visibleChan) startChan=mplayer.numChannels-visibleChan;
                if (startChan<0) startChan=0;
                
                
			} else if (touched_coord==0x31) {
                int val=segcont_shownote.selectedSegmentIndex;
                if (val<3) val=3-1; //if note mode was active, reset
				val++;
				if (val>=5) val=0;
				segcont_shownote.selectedSegmentIndex=val;
			} else if (touched_coord==0x02) {
				int val=segcont_fx4.selectedSegmentIndex;
				val++;
				if (val>=2) val=0;
				segcont_fx4.selectedSegmentIndex=val;
                segcont_fx2.selectedSegmentIndex=0;
                segcont_fx3.selectedSegmentIndex=0;
                segcont_fx5.selectedSegmentIndex=0;
			} else if (touched_coord==0x12) {
				int val=segcont_fx5.selectedSegmentIndex;
				if (val!=1) val=1;
                else val=0;
				segcont_fx5.selectedSegmentIndex=val;
                segcont_fx2.selectedSegmentIndex=0;
                segcont_fx3.selectedSegmentIndex=0;
                segcont_fx4.selectedSegmentIndex=0;
			} else if (touched_coord==0x22) {
				int val=segcont_fx5.selectedSegmentIndex;
				if (val!=2) val=2;
                else val=0;
				segcont_fx5.selectedSegmentIndex=val;
                segcont_fx2.selectedSegmentIndex=0;
                segcont_fx3.selectedSegmentIndex=0;
                segcont_fx4.selectedSegmentIndex=0;
			} else if (touched_coord==0x23) {
                mOglViewIsHidden=YES;
			}

		} else viewTapHelpShow=1;
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
	if ( ((segcont_fx1.selectedSegmentIndex)||
		  (segcont_fx2.selectedSegmentIndex)||
		  (segcont_fx3.selectedSegmentIndex)||
          (segcont_fx4.selectedSegmentIndex)||
          (segcont_fx5.selectedSegmentIndex)||
		  (segcont_FxBeat.selectedSegmentIndex)||
		  (segcont_spectrum.selectedSegmentIndex)) /*&& (mplayer.bGlobalAudioPause==0)*/ ) {
		//compute new spectrum data
		if ([mplayer isPlaying]){
            //FFT: build audio buffer
			short int **snd_buffer;
			int cur_pos;
			snd_buffer=[mplayer buffer_ana_cpy];
			cur_pos=[mplayer getCurrentPlayedBufferIdx];
			short int *curBuffer=snd_buffer[cur_pos];
			int k=0;
			int j=256*SOUND_BUFFER_SIZE_SAMPLE/(SPECTRUM_BANDS*2);
            if (mplayer.bGlobalAudioPause==0) {
			for (int i=0;i<SPECTRUM_BANDS*2;i++) {
                real_spectrumL[i]=(curBuffer[(k>>8)*2]);
                real_spectrumR[i]=(curBuffer[(k>>8)*2+1]);
                img_spectrumL[i]=0;
                img_spectrumR[i]=0;
				k+=j;
			}				
            } else {
                memset(real_spectrumL,0,SPECTRUM_BANDS*2*2);
                memset(real_spectrumR,0,SPECTRUM_BANDS*2*2);
                memset(img_spectrumL,0,SPECTRUM_BANDS*2*2);
                memset(img_spectrumR,0,SPECTRUM_BANDS*2*2);
            }
            // COMPUTE FFT
            int scaleL=fix_fft(real_spectrumL,img_spectrumL, LOG2_SPECTRUM_BANDS+1, 0);
            int scaleR=fix_fft(real_spectrumR,img_spectrumR, LOG2_SPECTRUM_BANDS+1, 0);
            
            // COMPUTE MODULUS
            for (int i=0;i<SPECTRUM_BANDS;i++) {
                real_spectrumIL[i]=(int)((int)real_spectrumL[i]*(int)real_spectrumL[i]+(int)img_spectrumL[i]*(int)img_spectrumL[i]);
                real_spectrumIR[i]=(int)((int)real_spectrumR[i]*(int)real_spectrumR[i]+(int)img_spectrumR[i]*(int)img_spectrumR[i]);
            }
            //oreal_spectrumL[0]=oreal_spectrumL[1];
            //oreal_spectrumR[0]=oreal_spectrumR[1];
            
            // COMPUTE FINAL FFT & BEAT DETECTION
            int newSpecL,newSpecR,sumL,sumR;
            for (int i=0;i<SPECTRUM_BANDS/2;i++) {
                newSpecL=(((int)sqrt(max2(real_spectrumIL[i*2+0],real_spectrumIL[i*2+1])))<<scaleL);
                newSpecR=(((int)sqrt(max2(real_spectrumIR[i*2+0],real_spectrumIR[i*2+1])))<<scaleR);
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
                
                //SMOOTH FOR SPECTRUM
                if (newSpecL<((int)oreal_spectrumL[i]*13)>>4) newSpecL=((int)oreal_spectrumL[i]*13)>>4;
                if (newSpecR<((int)oreal_spectrumR[i]*13)>>4) newSpecR=((int)oreal_spectrumR[i]*13)>>4;
                oreal_spectrumL[i]=newSpecL;
                oreal_spectrumR[i]=newSpecR;
            }
		}
	}
	
    int detail_lvl=sc_FXDetail.selectedSegmentIndex;
    if (segcont_fx4.selectedSegmentIndex) detail_lvl=0; //force low for fx4
    
	switch (detail_lvl) {
		case 2:
			nb_spectrum_bands=SPECTRUM_BANDS/2;
			for (int i=0;i<SPECTRUM_BANDS/2;i++) {
				real_spectrumL[i]=oreal_spectrumL[i];
				real_spectrumR[i]=oreal_spectrumR[i];
            }
            break;
		case 1:
			nb_spectrum_bands=SPECTRUM_BANDS/4;
            for (int i=0;i<SPECTRUM_BANDS/4;i++) {
				real_spectrumL[i]=max2(oreal_spectrumL[i*2],oreal_spectrumL[i*2+1]);
				real_spectrumR[i]=max2(oreal_spectrumR[i*2],oreal_spectrumR[i*2+1]);
                real_beatDetectedL[i]=max2(real_beatDetectedL[i*2],real_beatDetectedL[i*2+1]);
				real_beatDetectedR[i]=max2(real_beatDetectedR[i*2],real_beatDetectedR[i*2+1]);
			}
			break;
			
		case 0:
			nb_spectrum_bands=SPECTRUM_BANDS/8;
            for (int i=0;i<SPECTRUM_BANDS/8;i++) {
                real_spectrumL[i]=max4(oreal_spectrumL[i*4],oreal_spectrumL[i*4+1],oreal_spectrumL[i*4+2],oreal_spectrumL[i*4+3]);
                real_spectrumR[i]=max4(oreal_spectrumR[i*4],oreal_spectrumR[i*4+1],oreal_spectrumR[i*4+2],oreal_spectrumR[i*4+3]);
				real_beatDetectedL[i]=max4(real_beatDetectedL[i*4],real_beatDetectedL[i*4+1],real_beatDetectedL[i*4+2],real_beatDetectedL[i*4+3]);
				real_beatDetectedR[i]=max4(real_beatDetectedR[i*4],real_beatDetectedR[i*4+1],real_beatDetectedR[i*4+2],real_beatDetectedR[i*4+3]);
			}
			break;
	}
	
	angle+=(float)4.0f;	
	if (segcont_fx1.selectedSegmentIndex) {
		/* Update Angle Based On The Clock */
        
		fxRadialBlur(0,ww,hh,real_spectrumL,real_spectrumR,nb_spectrum_bands,sldFxAlpha.value);
	} else {
		glClearColor(0.0f, 0.0f, 0.0f, sldFxAlpha.value);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
    
	RenderUtils::SetUpOrtho(0,ww,hh);
    
	if (([mplayer isPlaying])&&(segcont_shownote.selectedSegmentIndex)) {
        
		int display_note_mode=segcont_shownote.selectedSegmentIndex-1;
		
        if ((mplayer.mPlayType==15)&&(segcont_shownote.selectedSegmentIndex>2)) { //Timidity
            int playerpos=[mplayer getCurrentPlayedBufferIdx];
            playerpos=(playerpos+MIDIFX_OFS)%SOUND_BUFFER_NB;
            RenderUtils::DrawMidiFX(tim_notes_cpy[playerpos],ww,hh,mDeviceType==1,segcont_shownote.selectedSegmentIndex-3,tim_midifx_note_range,tim_midifx_note_offset,60/(3-sc_FXDetail.selectedSegmentIndex));
            
            if (mHeader) delete mHeader;
            mHeader=nil;
            if (sc_showDebug.selectedSegmentIndex) {
                sprintf(str_data,"%d/%d",tim_voicenb_cpy[playerpos],(int)(sld_TIMPoly.value));
                mHeader= new CGLString(str_data, mFont);
                glPushMatrix();
                glTranslatef(ww-strlen(str_data)*6-2, 5.0f, 0.0f);
                //glScalef(1.58f, 1.58f, 1.58f);
                mHeader->Render(0);
                glPopMatrix();
            }            
        } else if (mplayer.mPatternDataAvail) { //Modplug
            if (segcont_shownote.selectedSegmentIndex>2) {
                int playerpos=[mplayer getCurrentPlayedBufferIdx];
                playerpos=(playerpos+MIDIFX_OFS)%SOUND_BUFFER_NB;
                
                int *pat,*row;
                pat=[mplayer playPattern];
                row=[mplayer playRow];
                currentPattern=pat[playerpos];
                currentRow=row[playerpos];
                
                currentNotes=ModPlug_GetPattern(mplayer.mp_file,currentPattern,&numRows);
                idx=startRow*mplayer.numChannels+startChan;
                
                if (currentNotes) {
                    idx=currentRow*mplayer.numChannels;
                    for (j=0;j<mplayer.numChannels;j++,idx++)  {
                        cnote=currentNotes[idx].Note;
                        cinst=(currentNotes[idx].Instrument)&0x1F;
                        cvol=(currentNotes[idx].Volume)&0xFF;
                        if (!cvol) cvol=63;
                        
                        tim_notes_cpy[playerpos][j]=cnote|(cinst<<8)|(cvol<<16)|((1<<1)<<24); //VOICE_ON : 1<<1
                    }
                    memset(&(tim_notes_cpy[playerpos][mplayer.numChannels]),0,(256-mplayer.numChannels)*4);
                }
                
                RenderUtils::DrawMidiFX(tim_notes_cpy[playerpos],ww,hh,mDeviceType==1,segcont_shownote.selectedSegmentIndex-3,tim_midifx_note_range,tim_midifx_note_offset,60/(3-sc_FXDetail.selectedSegmentIndex));
                
            } else {
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
                
                //            int currentYoffset=playerOffset[playerpos]*12/1000;
                
                endChan=startChan+visibleChan;
                if (endChan>mplayer.numChannels) endChan=mplayer.numChannels;
                else if (endChan<mplayer.numChannels) endChan++;
                startRow=currentRow-midline;
                
                
                
                currentNotes=ModPlug_GetPattern(mplayer.mp_file,currentPattern,&numRows);
                if (currentPattern>0) prevNotes=ModPlug_GetPattern(mplayer.mp_file,currentPattern-1,&numRowsP);
                else prevNotes=nil;
                if (currentPattern<mplayer.numPatterns-1) nextNotes=ModPlug_GetPattern(mplayer.mp_file,currentPattern+1,&numRowsN);
                else nextNotes=nil;
                idx=startRow*mplayer.numChannels+startChan;
                
                RenderUtils::DrawChanLayout(ww,hh,display_note_mode,endChan-startChan+1,(movePx%size_chan));
                
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
                                case 0:
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
                                case 1:
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
                            }
                            str_data[k]=0;
                            mText[l++] = new CGLString(str_data, mFont);
                            
                        } else {
                            mText[l++] = NULL;
                            
                        }
                    }
                    str_data[2]=0;
                    for (l=0;l<linestodraw;l++) {
                        i=l+startRow;
                        if (mText[l]) {
                            glPushMatrix();
                            glTranslatef(NOTES_DISPLAY_LEFTMARGIN+(movePx%size_chan), hh-NOTES_DISPLAY_TOPMARGIN-l*12/*+currentYoffset*/, 0.0f);
                            
                            if ((i<0)||(i>=numRows)) mText[l]->Render(4+display_note_mode*2);
                            else mText[l]->Render(3+display_note_mode*2);
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
                        mTextLine[l]= new CGLString(str_data, mFont);
                        glPushMatrix();
                        glTranslatef(8.0f, hh-NOTES_DISPLAY_TOPMARGIN-l*12/*+currentYoffset*/, 0.0f);
                        mTextLine[l]->Render(1+(l&1));
                        glPopMatrix();
                    }
                    
                    
                }
                //draw header + fps
                memset(str_data,32,171);   //max 171 chars => 1026pix (6pix/char)
                str_data[12*16]=0;
                switch (display_note_mode) {
                    case 0:
                        for (j=startChan;j<endChan;j++) {
                            str_data[(j-startChan)*12+7]='0'+(j+1)/10;
                            str_data[(j-startChan)*12+8]='0'+(j+1)%10;
                        }
                        str_data[(endChan-1-startChan)*12+9]=0;
                        break;
                    case 1:
                        for (j=startChan;j<endChan;j++) {
                            str_data[(j-startChan)*6+5]='0'+(j+1)/10;
                            str_data[(j-startChan)*6+6]='0'+(j+1)%10;
                        }
                        str_data[(endChan-1-startChan)*6+7]=0;
                        break;
                }
                
                /*		if (DEBUG_SHOW_FPS){
                 j=0;
                 for (i=0; i<m_dtHistory.GetCount(); i++) j+=(int)(1.0f/m_dtHistory[i]);
                 j=j/i;
                 str_data[0]='0'+(j/100)%10;
                 str_data[1]='0'+(j/10)%10;
                 str_data[2]='0'+(j%10);
                 }*/
                mHeader= new CGLString(str_data, mFont);
                glPushMatrix();
                glTranslatef(5.0f+(movePx%size_chan), hh-12, 0.0f);
                //glScalef(1.58f, 1.58f, 1.58f);
                mHeader->Render(0);
                glPopMatrix();
                
                
                RenderUtils::DrawChanLayoutAfter(ww,hh,display_note_mode);
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
		if ((segcont_spectrum.selectedSegmentIndex)&&(segcont_oscillo.selectedSegmentIndex)) pos_fx=1;
		if ((segcont_FxBeat.selectedSegmentIndex)&&(segcont_oscillo.selectedSegmentIndex)) pos_fx=1;
		if ((segcont_FxBeat.selectedSegmentIndex)&&(segcont_spectrum.selectedSegmentIndex)) pos_fx=1;
		if (hasdrawnotes) pos_fx=1;
		if (segcont_fx1.selectedSegmentIndex) pos_fx=1;
		
		if (segcont_spectrum.selectedSegmentIndex) RenderUtils::DrawSpectrum(real_spectrumL,real_spectrumR,ww,hh,hasdrawnotes,segcont_spectrum.selectedSegmentIndex-1,pos_fx,mDeviceType==1,nb_spectrum_bands);
		if (segcont_FxBeat.selectedSegmentIndex) RenderUtils::DrawBeat(real_beatDetectedL,real_beatDetectedR,ww,hh,hasdrawnotes,pos_fx,mDeviceType==1,nb_spectrum_bands);
		if (segcont_oscillo.selectedSegmentIndex) RenderUtils::DrawOscillo(curBuffer,SOUND_BUFFER_SIZE_SAMPLE,ww,hh,hasdrawnotes,segcont_oscillo.selectedSegmentIndex,pos_fx,mDeviceType==1);
	}
    
	if ([mplayer isPlaying]){
		if (segcont_fx2.selectedSegmentIndex) {
            RenderUtils::DrawSpectrum3D(real_spectrumL,real_spectrumR,ww,hh,angle,segcont_fx2.selectedSegmentIndex,mDeviceType==1,nb_spectrum_bands);
        } else if (segcont_fx3.selectedSegmentIndex) {
            RenderUtils::DrawSpectrum3DMorph(real_spectrumL,real_spectrumR,ww,hh,angle,segcont_fx3.selectedSegmentIndex,mDeviceType==1,nb_spectrum_bands);
        } else if (segcont_fx4.selectedSegmentIndex) {
            renderFluid(ww, hh, real_beatDetectedL, real_beatDetectedR, real_spectrumL, real_spectrumR, nb_spectrum_bands, 0, (unsigned char)(sldFxAlpha.value*255));
        } else if (segcont_fx5.selectedSegmentIndex) {
            RenderUtils::DrawSpectrum3DSphere(real_spectrumL,real_spectrumR,ww,hh,angle,segcont_fx5.selectedSegmentIndex,mDeviceType==1,nb_spectrum_bands);
        }
	}
	
	if (viewTapHelpInfo) {
		RenderUtils::SetUpOrtho(0,ww,hh);
		RenderUtils::DrawFXTouchGrid(ww,hh,viewTapHelpInfo);
        
		infoMenuShowImages(ww,hh,viewTapHelpInfo);
        
		for (int i=0;i<1;i++) {
			glPushMatrix();
			glTranslatef(i*ww/4+(ww*3/4)+ww/8-(strlen(viewTapInfoStr[i]->mText)/2)*6,hh/8, 0.0f);
			viewTapInfoStr[i]->Render(128+(viewTapHelpInfo/2));
			glPopMatrix();
		}
	}
	if (viewTapHelpShow) {
		if (viewTapHelpInfo<255) {
			viewTapHelpInfo+=(255-viewTapHelpInfo)/3;
			if (viewTapHelpInfo>250) viewTapHelpInfo=255;
		}
	} else {
		if (viewTapHelpInfo>0) {
			viewTapHelpInfo-=(255+32-viewTapHelpInfo)/3;
			if (viewTapHelpInfo<0) viewTapHelpInfo=0;
		}
	}
	
/*    if (sc_showDebug.selectedSegmentIndex) {
        sprintf(str_data,"free mem: %dMB",[DetailViewControllerIphone get_free_memory]/1024/1024);
        CGLString *minfo= new CGLString(str_data, mFont);
        glPushMatrix();
        glTranslatef(0, 5.0f, 0.0f);
        //glScalef(1.58f, 1.58f, 1.58f);
        minfo->Render(0);
        glPopMatrix();
        delete minfo;
        minfo=nil;        
    }*/
	
	
    [m_oglContext presentRenderbuffer:GL_RENDERBUFFER_OES];
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



/*
 - (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
 return @"Playlist";
 }*/

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return mPlaylist_size;
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
	const NSInteger TOP_LABEL_TAG = 1001;
	const NSInteger BOTTOM_LABEL_TAG = 1002;
	UILabel *topLabel;
	UILabel *bottomLabel;
	
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:CellIdentifier] autorelease];
		cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:CellIdentifier] autorelease];
		//
		// Create the label for the top row of text
		//
		topLabel = [[[UILabel alloc] init] autorelease];
		[cell.contentView addSubview:topLabel];
		
		//
		// Configure the properties for the text that are the same on every row
		//
		topLabel.tag = TOP_LABEL_TAG;
		topLabel.backgroundColor = [UIColor clearColor];
		topLabel.textColor = [UIColor colorWithRed:1.0 green:1.0 blue:1.0 alpha:1.0];
		topLabel.highlightedTextColor = [UIColor colorWithRed:1.0 green:1.0 blue:1.0 alpha:1.0];
		topLabel.font = [UIFont boldSystemFontOfSize:(mDeviceType==1?16:12)];
		
		//
		// Create the label for the top row of text
		//
		bottomLabel = [[[UILabel alloc] init] autorelease];
		[cell.contentView addSubview:bottomLabel];
		//
		// Configure the properties for the text that are the same on every row
		//
		bottomLabel.tag = BOTTOM_LABEL_TAG;
		bottomLabel.backgroundColor = [UIColor clearColor];
		bottomLabel.textColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.8 alpha:1.0];
		bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.8 blue:0.8 alpha:1.0];
		bottomLabel.font = [UIFont systemFontOfSize:(mDeviceType==1?12:9)];
    } else {
		topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
		bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
	}
	bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
								   (mDeviceType==1?18:14),
								   tableView.bounds.size.width - 1.0 * cell.indentationWidth-50,
								   (mDeviceType==1?14:10));
	topLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
								0,
								tableView.bounds.size.width - 1.0 * cell.indentationWidth-50,
								(mDeviceType==1?18:14));
	
	
	// Set up the cell...
	NSArray *filename_parts=[mPlaylist[indexPath.row].mPlaylistFilepath componentsSeparatedByString:@"/"];
	topLabel.text = mPlaylist[indexPath.row].mPlaylistFilename;//[NSString stringWithFormat:@"%@",[filename_parts objectAtIndex:[filename_parts count]-1]];
	
	if ([filename_parts count]>=3) {
		if ([(NSString*)[filename_parts objectAtIndex:[filename_parts count]-3] compare:@"Documents"]!=NSOrderedSame) {
			bottomLabel.text = [NSString stringWithFormat:@"%@/%@",[filename_parts objectAtIndex:[filename_parts count]-3],[filename_parts objectAtIndex:[filename_parts count]-2]];
		} else bottomLabel.text = [NSString stringWithFormat:@"%@",[filename_parts objectAtIndex:[filename_parts count]-2]];
	} else if ([filename_parts count]>=2) {
		if ([(NSString*)[filename_parts objectAtIndex:[filename_parts count]-2] compare:@"Documents"]!=NSOrderedSame) {
			bottomLabel.text = [NSString stringWithFormat:@"%@",[filename_parts objectAtIndex:[filename_parts count]-2]];
		} else bottomLabel.text = @"";
	}
    
    
    NSArray *filename_IsArc=[mPlaylist[indexPath.row].mPlaylistFilepath componentsSeparatedByString:@"@"];
    NSArray *filename_IsSub=[mPlaylist[indexPath.row].mPlaylistFilepath componentsSeparatedByString:@"?"];
    if ([filename_IsArc count]>1) {//this is an archive entry
        bottomLabel.text=[bottomLabel.text stringByAppendingFormat:@"/%@",[filename_parts objectAtIndex:[filename_parts count]-1]];
    } else if ([filename_IsSub count]>1) {//this is a subsong entry
        bottomLabel.text=[bottomLabel.text stringByAppendingFormat:@"/%@",[filename_parts objectAtIndex:[filename_parts count]-1]];
    }
    
	cell.accessoryType = UITableViewCellAccessoryNone;
    
    //TODO
    //WARNING: might lead to some desync issue if rating is reseted in browser or setting screen
    //To check => reset ratign to -1 on viewwillappear ?
    if (mPlaylist[indexPath.row].mPlaylistRating==-1) { 
        DBHelper::getFileStatsDBmod(mPlaylist[indexPath.row].mPlaylistFilename,
                                mPlaylist[indexPath.row].mPlaylistFilepath,
                                NULL,
                                &(mPlaylist[indexPath.row].mPlaylistRating),
                                NULL,
                                NULL);
    }
    
	if (mPlaylist[indexPath.row].mPlaylistRating>0) {
		int rating;
        
        
		rating=mPlaylist[indexPath.row].mPlaylistRating;
        //		NSLog(@"%@ %d",mPlaylist[indexPath.row].mPlaylistFilepath,rating);
		if (rating<0) rating=0;
		if (rating>5) rating=5;
		UIImage *accessoryImage = [UIImage imageNamed:ratingImg[rating]];
		UIImageView *accImageView = [[[UIImageView alloc] initWithImage:accessoryImage] autorelease];
		[accImageView setFrame:CGRectMake(0, 0, 50.0, 9.0)];
		cell.accessoryView = accImageView;
	} else cell.accessoryView=nil;
    
	return cell;
}


// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
		//delete entry
		if (mPlaylist_size) { //ensure playlist is not empty
			[mPlaylist[indexPath.row].mPlaylistFilename autorelease];
			[mPlaylist[indexPath.row].mPlaylistFilepath autorelease];
			if (mPlaylist_size>1) {
				for (int i=indexPath.row;i<mPlaylist_size-1;i++) {
					mPlaylist[i]=mPlaylist[i+1];
				}
				if (indexPath.row<mPlaylist_pos) mPlaylist_pos--;				
				mPlaylist_size--;
			} else {
				mPlaylist_size=mPlaylist_pos=0;
			}
			//playlistPos.text=[NSString stringWithFormat:@"%d on %d",mPlaylist_pos+1,mPlaylist_size];
			mShouldUpdateInfos=1;
		}
		[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
    }   
    else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}



/*- (NSIndexPath *)tableView:(UITableView *)tableView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath {
 }
 */
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
	t_plPlaylist_entry tmpF;
	tmpF=mPlaylist[fromIndexPath.row];
	if (toIndexPath.row<fromIndexPath.row) {
		for (int i=fromIndexPath.row;i>toIndexPath.row;i--) {
			mPlaylist[i]=mPlaylist[i-1];
		}
		mPlaylist[toIndexPath.row]=tmpF;
	} else {
		for (int i=fromIndexPath.row;i<toIndexPath.row;i++) {
			mPlaylist[i]=mPlaylist[i+1];
		}
		mPlaylist[toIndexPath.row]=tmpF;
	}
	if ((fromIndexPath.row>mPlaylist_pos)&&(toIndexPath.row<=mPlaylist_pos)) mPlaylist_pos++;
	if ((fromIndexPath.row<mPlaylist_pos)&&(toIndexPath.row>=mPlaylist_pos)) mPlaylist_pos--;
}

// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
	if (indexPath.row==mPlaylist_pos) return NO;
	return YES;
}
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    //	if (indexPath.row==mPlaylist_pos) return NO;
    //Allow edit (delete in this case) only if more than 1 file
    if (mPlaylist_size>1) return YES;
    else return NO;
}


- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	mPlaylist_pos=indexPath.row;
	[self play_curEntry];
    
}

-(IBAction)savePlaylist {
	mSavePlaylist=1;
	[self presentModalViewController:textInputView animated:YES];
}

- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
	[theTextField resignFirstResponder];
	[self validatePlaylistName];
	return YES;
}

-(IBAction)validatePlaylistName {
	mValidatePlName=1;
	[self dismissModalViewControllerAnimated:YES];
}
-(IBAction)cancelPlaylistName {
	mValidatePlName=0;
	[self dismissModalViewControllerAnimated:YES];
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
#pragma mark UIPickerViewDataSource methods

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView {
    return 1;   
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component {
	if (mplayer==nil) return 0;
	else {
        if (pickerView==pvSubSongSel) return (mplayer.mod_subsongs);
        if (pickerView==pvArcSel) return [mplayer getArcEntriesCnt];
    }
    return 0;
}

/*- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component {	
    if (pickerView==pvSubSongSel) return [mplayer getSubTitle:row+mplayer.mod_minsub];
    if (pickerView==pvArcSel) return [mplayer getArcEntryTitle:row];
    return nil;
}*/
- (UIView *)pickerView:(UIPickerView *)pickerView
            viewForRow:(NSInteger)row
          forComponent:(NSInteger)component
           reusingView:(UIView *)view {
    
    UILabel *pickerLabel = (UILabel *)view;
    
    if (pickerLabel == nil) {
        CGRect frame = CGRectMake(0.0, 0.0, pickerView.frame.size.width-32, 32);
        pickerLabel = [[[UILabel alloc] initWithFrame:frame] autorelease];
        pickerLabel.lineBreakMode=UILineBreakModeMiddleTruncation;
        [pickerLabel setTextAlignment:UITextAlignmentLeft];
        [pickerLabel setBackgroundColor:[UIColor clearColor]];
        [pickerLabel setFont:[UIFont boldSystemFontOfSize:15]];
    }
    
    if (pickerView==pvSubSongSel) [pickerLabel setText:[mplayer getSubTitle:row+mplayer.mod_minsub]];
    if (pickerView==pvArcSel) [pickerLabel setText:[mplayer getArcEntryTitle:row]];

    
    
    
    return pickerLabel;
    
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component {
}

-(IBAction)playSelectedSubSong{
	[mplayer playGoToSub:[pvSubSongSel selectedRowInComponent:0]+mplayer.mod_minsub];
	[self showSubSongSelector];
}

-(IBAction)playSelectedArc{    
	[mplayer selectArcEntry:[pvArcSel selectedRowInComponent:0]];
	[self showArcSelector];
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];                    
    [self play_loadArchiveModule];
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
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
		cover = [[[TKCoverflowCoverView alloc] initWithFrame:rect] autorelease]; // 224
		cover.baseline = coverflow.coverSize.height;//224;
	}
    //    NSLog(@"ask for cov index %d",index);
        //        NSLog(@"look for it");
    if (mPlaylist[index].cover_flag==-1) [self checkAvailableCovers:index];
        if (mPlaylist[index].cover_flag>0) { //A cover should be available
            NSString *filePath,*coverFilePath;
            filePath=mPlaylist[index].mPlaylistFilepath;
            
            
            
            if (mPlaylist[index].cover_flag&1) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@.jpg",[filePath stringByDeletingPathExtension]];
            else if (mPlaylist[index].cover_flag&2) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@.png",[filePath stringByDeletingPathExtension]];
            else if (mPlaylist[index].cover_flag&4) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@.gif",[filePath stringByDeletingPathExtension]];
            else if (mPlaylist[index].cover_flag&8) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.jpg",[filePath stringByDeletingLastPathComponent]];
            else if (mPlaylist[index].cover_flag&16) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.png",[filePath stringByDeletingLastPathComponent]];
            else if (mPlaylist[index].cover_flag&32) coverFilePath=[NSHomeDirectory() stringByAppendingFormat:@"/%@/folder.gif",[filePath stringByDeletingLastPathComponent]];
            
//            NSLog(@"got %d %@",mPlaylist[index].cover_flag,coverFilePath);
            
            UIImage *img=[UIImage imageWithContentsOfFile:coverFilePath];//covers[index+1];
            
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
                    cover.image = [img scaleToSize:CGSizeMake(csize,new_h)];
                } else {
                    cover.image = img;
                }
            } else {
                int csize=coverflowView.coverSize.width;
                if (h>csize) {
                    int new_w=w*csize/h;
                    cover.image = [img scaleToSize:CGSizeMake(new_w,csize)];
                } else {
                    cover.image = img;
                }
            }
            }
            
        } else {  //No cover available, take default one
//            NSLog(@"using default");
            cover.image = [UIImage imageNamed:@"default_art.png"];//covers[0];
            
        }
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
    
    [UIView beginAnimations:@"selectCov1" context:cover];
	[UIView setAnimationDuration:0.15f];
    [UIView setAnimationDelegate:self];
    [UIView setAnimationCurve:UIViewAnimationCurveEaseOut];
    cover.frame=CGRectMake(cover.frame.origin.x, cover.frame.origin.y,cover.frame.size.width,cover.frame.size.height);
    cover.transform=CGAffineTransformMakeScale(0.9f,0.9f);
	[UIView commitAnimations];
    
	mPlaylist_pos=index;
	[self play_curEntry];
}

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
        TKCoverflowCoverView* cover=((TKCoverflowCoverView*)context);
        cover.frame=CGRectMake(cover.frame.origin.x, cover.frame.origin.y,cover.frame.size.width,cover.frame.size.height);
        cover.transform=CGAffineTransformMakeScale(1.1f,1.1f);
        [UIView commitAnimations];
    } else if ([animationID compare:@"selectCov2"]==NSOrderedSame) {
        [UIView beginAnimations:@"selectCov3" context:nil];
        [UIView setAnimationDuration:0.2f];    
        [UIView setAnimationCurve:UIViewAnimationCurveEaseOut];
        TKCoverflowCoverView* cover=((TKCoverflowCoverView*)context);
        cover.frame=CGRectMake(cover.frame.origin.x, cover.frame.origin.y,cover.frame.size.width,cover.frame.size.height);
        cover.transform=CGAffineTransformMakeScale(1.0f,1.0f);
        [UIView commitAnimations];
    }
}



@end
