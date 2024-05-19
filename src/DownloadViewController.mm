//
//  DownloadViewController.m
//  modizer4
//
//  Created by yoyofr on 6/20/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//


#define FTP_BUFFER_SIZE 1*1024*1024
#define TMP_FILE_NAME @"Documents/tmp.tmpfile"

#import "DownloadViewController.h"
#import "SettingsGenViewController.h"
#import "OnlineViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];
#import <CFNetwork/CFNetwork.h>
#include <sys/xattr.h>
#import "DBHelper.h"

#import "ModizFileHelper.h"


int lCancelURL;
extern pthread_mutex_t download_mutex;
static volatile int mGetURLInProgress;
static volatile int mGetFTPInProgress,mConnectionIssue,mSuspended;

static NSFileManager *mFileMngr;

#import "MDZUIImageView.h"

@implementation DownloadViewController

@synthesize networkStream,fileStream,downloadLabelSize,downloadLabelName,tableView,downloadPrgView,detailViewController,barItem,rootViewController,onlineVC;
@synthesize searchViewController,btnCancel,btnSuspend,btnResume,btnClear;
@synthesize mFTPDownloadQueueDepth,mURLDownloadQueueDepth,moreVC;

#define DOWNLOAD_BKP_WRITE_STR(a) \
if (a) str=[a UTF8String]; \
else str=""; \
str_len=(int)(strlen(str)+1); \
gzwrite(f,&str_len,sizeof(str_len));gzwrite(f,str,str_len);

- (void)backupDownloadList {
    [self suspend]; //1st, suspend any ongoing download
    gzFile f;
    f=gzopen([[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/modizer.downloadqueue"] UTF8String],"wb");
    if (f) {
        gzwrite(f,&mFTPDownloadQueueDepth,sizeof(mFTPDownloadQueueDepth));
        for (int i=0;i<mFTPDownloadQueueDepth;i++) {
            const char *str;
            int str_len;
            
            DOWNLOAD_BKP_WRITE_STR(mFilePath[i])
            DOWNLOAD_BKP_WRITE_STR(mFTPFilename[i])
            DOWNLOAD_BKP_WRITE_STR(mFTPpath[i])
            DOWNLOAD_BKP_WRITE_STR(mFTPhost[i])
            
            gzwrite(f,&mFileSize[i],sizeof(mFileSize[i]));
            gzwrite(f,&mUsePrimaryAction[i],sizeof(mUsePrimaryAction[i]));
            gzwrite(f,&mIsMODLAND[i],sizeof(mIsMODLAND[i]));
        }
        
        gzwrite(f,&mURLDownloadQueueDepth,sizeof(mURLDownloadQueueDepth));
        for (int i=0;i<mURLDownloadQueueDepth;i++) {
            const char *str;
            int str_len;
            
            DOWNLOAD_BKP_WRITE_STR(mURL[i])
            DOWNLOAD_BKP_WRITE_STR(mURLFilename[i])
            DOWNLOAD_BKP_WRITE_STR(mURLFilePath[i])
            
            gzwrite(f,&mURLIsMODLAND[i],sizeof(mURLIsMODLAND[i]));
            gzwrite(f,&mURLFilesize[i],sizeof(mURLFilesize[i]));
            gzwrite(f,&mURLUsePrimaryAction[i],sizeof(mURLUsePrimaryAction[i]));
            gzwrite(f,&mURLIsImage[i],sizeof(mURLIsImage[i]));
        }
        gzclose(f);
    }
}

#define DOWNLOAD_BKP_READ_VAL(a) \
        if (gzread(f,&a,sizeof(a))!=sizeof(a)) {\
            NSLog(@"gzread error val");\
            gzclose(f);\
            return -5;\
        }
#define DOWNLOAD_BKP_READ_STR(a) \
            if (gzread(f,&str_len,sizeof(str_len))!=sizeof(str_len)) { \
                NSLog(@"gzread error FTP entry %d",i); \
                gzclose(f); \
                return -2; \
            } \
            if (str_len>=sizeof(str)) { \
                NSLog(@"gzread error too long str (%d) for entry %d",str_len,i); \
                gzclose(f); \
                return -3; \
            } \
            if (gzread(f,str,str_len)!=str_len) { \
                NSLog(@"gzread error str for FTP entry %d",i); \
                gzclose(f); \
                return -4; \
            } \
            a=[NSString stringWithUTF8String:str];


- (int)restoreDownloadList {
    gzFile f;
    f=gzopen([[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/modizer.downloadqueue"] UTF8String],"rb");
    if (f) {
        char str[4096];
        int str_len;
        
        DOWNLOAD_BKP_READ_VAL(mFTPDownloadQueueDepth)
        if (mFTPDownloadQueueDepth>MAX_DOWNLOAD_QUEUE) {
            NSLog(@"Error FTP queue too high %d/%d",mFTPDownloadQueueDepth,MAX_DOWNLOAD_QUEUE);
            gzclose(f);
            return -1;
        }
        for (int i=0;i<mFTPDownloadQueueDepth;i++) {
            
            DOWNLOAD_BKP_READ_STR(mFilePath[i])
            DOWNLOAD_BKP_READ_STR(mFTPFilename[i])
            DOWNLOAD_BKP_READ_STR(mFTPpath[i])
            DOWNLOAD_BKP_READ_STR(mFTPhost[i])
        
            DOWNLOAD_BKP_READ_VAL(mFileSize[i])
            DOWNLOAD_BKP_READ_VAL(mUsePrimaryAction[i])
            DOWNLOAD_BKP_READ_VAL(mIsMODLAND[i])
            
        }
        
        DOWNLOAD_BKP_READ_VAL(mURLDownloadQueueDepth)
        if (mURLDownloadQueueDepth>MAX_DOWNLOAD_QUEUE) {
            NSLog(@"Error URL queue too high %d/%d",mURLDownloadQueueDepth,MAX_DOWNLOAD_QUEUE);
            gzclose(f);
            return -1;
        }
        for (int i=0;i<mURLDownloadQueueDepth;i++) {
            DOWNLOAD_BKP_READ_STR(mURL[i])
            DOWNLOAD_BKP_READ_STR(mURLFilename[i])
            DOWNLOAD_BKP_READ_STR(mURLFilePath[i])
            
            DOWNLOAD_BKP_READ_VAL(mURLIsMODLAND[i])
            DOWNLOAD_BKP_READ_VAL(mURLFilesize[i])
            DOWNLOAD_BKP_READ_VAL(mURLUsePrimaryAction[i])
            DOWNLOAD_BKP_READ_VAL(mURLIsImage[i])
        }
        
        gzclose(f);
    }
    
    [self resume];
    return 0;
}

#include "MiniPlayerImplementTableView.h"

- (BOOL)addSkipBackupAttributeToItemAtPath:(NSString* )path
{
    const char* filePath = [path fileSystemRepresentation];
    
    const char* attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    
    int result = setxattr(filePath, attrName, &attrValue, sizeof(attrValue), 0, 0);
    return result == 0;
}

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:YES];
    else {
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"")];
    }
}

-(IBAction)cancelCurrent {
	if (mGetFTPInProgress) {//FTP
		mFTPAskCancel=1;
		[self _stopReceiveWithStatus:@"Download cancelled" status:-1];
	} else if (mGetURLInProgress) {//HTTP
		lCancelURL=1;
        [AFmanager invalidateSessionCancelingTasks:YES resetSession:NO];
	}
}

-(IBAction) clearAll {
    pthread_mutex_lock(&download_mutex);
	
    //FTP
    for (int i=mGetFTPInProgress;i<mFTPDownloadQueueDepth;i++) {
		if (mFilePath[i]) {mFilePath[i]=nil;}
		if (mFTPpath[i])  {mFTPpath[i]=nil;}
		if (mFTPhost[i])  {mFTPhost[i]=nil;}
		if (mFTPFilename[i]) {mFTPFilename[i]=nil;}
	}
    mFTPDownloadQueueDepth=mGetFTPInProgress;
    
    //HTTP URL
    for (int i=mGetURLInProgress;i<mURLDownloadQueueDepth;i++) {
        if (mURL[i]) {mURL[i]=nil;}
        if (mURLFilename[i])  {mURLFilename[i]=nil;}
        if (mURLFilePath[i])  {mURLFilePath[i]=nil;}
        mURL[i]=nil;
        mURLFilename[i]=nil;
        mURLFilePath[i]=nil;
    }
    mURLDownloadQueueDepth=mGetURLInProgress;
    
	barItem.badgeValue=nil;
	
	[tableView reloadData];
	pthread_mutex_unlock(&download_mutex);
    
    if (mGetFTPInProgress||mGetURLInProgress) [self cancelCurrent];
}

-(IBAction) suspend {
    btnResume.hidden=NO;
    btnSuspend.hidden=YES;
    mSuspended=1;
    if (mGetFTPInProgress) {//FTP
		mFTPAskCancel=2;
		[self _stopReceiveWithStatus:@"Download suspended" status:-2];
	} else if (mGetURLInProgress) {//HTTP
		lCancelURL=2;
        [AFmanager invalidateSessionCancelingTasks:YES resetSession:NO];
	}
	downloadLabelName.text=NSLocalizedString(@"No download in progress",@"");
	downloadLabelSize.text=@"";
}

-(IBAction) resume {
    btnResume.hidden=YES;
    btnSuspend.hidden=NO;
    mSuspended=0;
    
    [self checkNextQueuedItem];
}


/*
 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
 - (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
 if ((self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil])) {
 // Custom initialization
 }
 return self;
 }
 */

/*
 // Implement loadView to create a view hierarchy programmatically, without using a nib.
 - (void)loadView {
 }
 */

#include "AlertsCommonFunctions.h"


/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////

-(void)viewDidDisappear:(BOOL)animated {
    [repeatingTimer invalidate];
    repeatingTimer = nil;
    
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView removeObserver:self
                                          forKeyPath:observedSelector
                                             context:LoadingProgressObserverContext];
    [super viewDidDisappear:animated];
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
	clock_t start_time,end_time;
	start_time=clock();
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
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
    
    mFileMngr=[[NSFileManager alloc] init];
    
    
    [btnCancel setType:BButtonTypeGray];
    [btnClear setType:BButtonTypeGray];
    [btnSuspend setType:BButtonTypeGray];
    [btnResume setType:BButtonTypeGray];
    
    //[btnCancel setShouldShowDisabled:YES];
    //[btnClear setShouldShowDisabled:YES];
	
	btnCancel.enabled=NO;
    btnClear.enabled=NO;
    
    btnResume.hidden=YES;
    mSuspended=0;
	mConnectionIssue=0;
	mGetURLInProgress=0;
    
	mFTPDownloadQueueDepth=0;
	mURLDownloadQueueDepth=0;
	mCurrentDownloadedBytes=0;
    mCurrentURLIsImage=0;
	
	downloadLabelName.text=NSLocalizedString(@"No download in progress",@"");
	downloadLabelSize.text=@"";
	downloadPrgView.progress=0.0f;
	
	tableView.rowHeight=40;
	
	for (int i=0;i<MAX_DOWNLOAD_QUEUE;i++) {
		mFilePath[i]=nil;
		mFTPFilename[i]=nil;
		mURLFilename[i]=nil;
        mURLFilePath[i]=nil;
		mFTPpath[i]=nil;
		mFTPhost[i]=nil;
		mURL[i]=nil;
		mFileSize[i]=0;
		mURLFilesize[i]=0;
        mURLIsMODLAND[i]=0;
        mURLUsePrimaryAction[i]=0;
		mIsMODLAND[i]=0;
	}
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
    self.navigationItem.rightBarButtonItem = item;
    
    self.tableView.editing=TRUE;
    
    [self restoreDownloadList];
    barItem.badgeValue=[NSString stringWithFormat:@"%d",(mFTPDownloadQueueDepth+mURLDownloadQueueDepth)];
    [self checkNextQueuedItem];
    
    [super viewDidLoad];
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"download : %d",end_time-start_time);
#endif
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

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}


- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleDefault;
}

- (void)viewWillAppear:(BOOL)animated {
    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    
    [self hideWaiting];
    
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    
    [super viewWillAppear:animated];
    
    if ([detailViewController mPlaylist_size]>0) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    } else {
        wasMiniPlayerOn=false;
        [self hideMiniPlayer];
    }
    
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
}


// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return YES;
}


- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}


- (void)dealloc {
    [waitingView removeFromSuperview];
    [waitingViewPlayer removeFromSuperview];
    waitingView=nil;
    waitingViewPlayer=nil;
    
	for (int i=0;i<MAX_DOWNLOAD_QUEUE;i++) {
		if (mFilePath[i]) {mFilePath[i]=nil;}
		if (mFTPpath[i])  {mFTPpath[i]=nil;}
		if (mFTPhost[i])  {mFTPhost[i]=nil;}
		if (mFTPFilename[i]) {mFTPFilename[i]=nil;}
		
		if (mURL[i]) {mURL[i]=nil;}
		if (mURLFilename[i]) {mURLFilename[i]=nil;}
        if (mURLFilePath[i]) {mURLFilePath[i]=nil;}
	}
    
    //[mFileMngr release];
    
    ////[super dealloc];
}


- (void)_receiveDidStart {
}

- (void)_updateStatus:(NSString *)statusString {
    static NSDate *lastDate=nil;
    static int64_t last_progress=0;
    NSDate *currentDate;
    int64_t current_progress;
    static double speed=0;
    
    if (mCurrentFileSize) {
        
        currentDate=[NSDate date];
        current_progress=mCurrentDownloadedBytes;
        // check if new file, if so reset
        if (last_progress>=current_progress) {
            lastDate=nil;
            speed=0;
        }
        
        if (lastDate==nil) {
            //starting point
            lastDate=currentDate;
            last_progress=current_progress;
        } else {
            double time_passed=[currentDate timeIntervalSinceDate:lastDate];
            if (time_passed>=0.5f) {
                speed=(double)(current_progress-last_progress)/(double)time_passed/(1024*1024);
                lastDate=currentDate;
                last_progress=current_progress;
            }
        }
        
        if (mCurrentFileSize>1*1024*1024) {
            if (speed>0) downloadLabelSize.text=[NSString stringWithFormat:@"%.1lfMB / %.1lfMB - %.3f Mo/s",(double)mCurrentDownloadedBytes/(1024*1024),(double)mCurrentFileSize/(1024*1024),speed];
            else downloadLabelSize.text=[NSString stringWithFormat:@"%.1lfMB / %.1lfMB - n/a MB/s",(double)mCurrentDownloadedBytes/(1024*1024),(double)mCurrentFileSize/(1024*1024)];
        } else {
            if (speed>0) downloadLabelSize.text=[NSString stringWithFormat:@"%dKB / %dKB - %.3f Mo/s",mCurrentDownloadedBytes>>10,mCurrentFileSize>>10,speed];
            else downloadLabelSize.text=[NSString stringWithFormat:@"%dKB / %dKB - n/a Mo/s",mCurrentDownloadedBytes>>10,mCurrentFileSize>>10];
        }
        
        downloadPrgView.progress=(float)mCurrentDownloadedBytes/(float)mCurrentFileSize;
    } else downloadLabelSize.text=[NSString stringWithFormat:@"%dKB",mCurrentDownloadedBytes/1024];
    
	[downloadLabelSize setNeedsDisplay];
	[downloadPrgView setNeedsDisplay];
}

- (void)_receiveDidStopWithStatus:(NSString *)statusString {
    if (statusString == nil) {
		//        assert(self.filePath != nil);
        statusString = @"GET succeeded";
		downloadLabelName.text=[NSString stringWithFormat:NSLocalizedString(@"Downloaded %@",@""),mCurrentFilename];
		downloadLabelSize.text=[NSString stringWithFormat:@"%dKB",mCurrentDownloadedBytes/1024];
		downloadPrgView.progress=1.0f;
		
		
		//Rename file to good name
		NSError *err;
		
		[mFileMngr createDirectoryAtPath:[[NSHomeDirectory() stringByAppendingPathComponent:mCurrentFilePath] stringByDeletingLastPathComponent]
             withIntermediateDirectories:TRUE attributes:nil error:&err];
		
		[mFileMngr moveItemAtPath:[NSHomeDirectory() stringByAppendingPathComponent:TMP_FILE_NAME]
                           toPath:[NSHomeDirectory() stringByAppendingPathComponent:mCurrentFilePath] error:&err];
		
        [self addSkipBackupAttributeToItemAtPath:[NSHomeDirectory() stringByAppendingPathComponent:mCurrentFilePath]];
        
		if (mIsMODLAND[0]==0) [self checkIfShouldAddFile:[NSHomeDirectory() stringByAppendingPathComponent: mCurrentFilePath] fileName:mCurrentFilename ];
		else {  //MODLAND
			if ([ModizFileHelper isPlayableFile:mCurrentFilename]) {
                if ((mCurrentUsePrimaryAction==1)&&(mIsMODLAND[0]==1)) {
                    NSMutableArray *array_label = [[NSMutableArray alloc] init];
                    NSMutableArray *array_path = [[NSMutableArray alloc] init];
                    [array_label addObject:mCurrentFilename];
                    [array_path addObject:mCurrentFilePath];
                    [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                    
                } else {
                    if (mIsMODLAND[0]==1) {
                        [detailViewController add_to_playlist:mCurrentFilePath fileName:mCurrentFilename forcenoplay:(mCurrentUsePrimaryAction==2)];
                    }
                }
			}
		}
		//refresh view which potentially list the file as not downloaded
		[onlineVC refreshViewAfterDownload];
		[searchViewController refreshViewAfterDownload];
        [moreVC refreshViewAfterDownload];
		[rootViewController refreshViewAfterDownload];
        //TODO: playlist
    } else if (mFTPAskCancel==0) {
		if (mConnectionIssue==0) {
			mConnectionIssue=1;
			
            [self showAlertMsg:NSLocalizedString(@"Warning", @"") message:statusString];
		}
	}
}

- (BOOL)isReceiving {
    return (self.networkStream != nil);
}

- (void)_stopReceiveWithStatus:(NSString *)statusString status:(int)status{
    // Shuts down the connection and displays the result (statusString == nil)
    // or the error status (otherwise).
    //    NSLog(@"stop, reason: %@",statusString);
    
	NSError *err;
    if (self.networkStream != nil) {
        [self.networkStream removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        self.networkStream.delegate = nil;
        [self.networkStream close];
        self.networkStream = nil;
    }
    if (self.fileStream != nil) {
        [self.fileStream close];
        self.fileStream = nil;
		//if an error occured : remove file
		if (status) [mFileMngr removeItemAtPath:[NSHomeDirectory() stringByAppendingPathComponent:TMP_FILE_NAME] error:&err];
    }
    [self _receiveDidStopWithStatus:statusString];
	
    if (status==-2) {//Suspended
    } else { //move to next entry
        pthread_mutex_lock(&download_mutex);
        if (mFilePath[0]) {mFilePath[0]=nil;}
        if (mFTPpath[0])  {mFTPpath[0]=nil;}
        if (mFTPhost[0])  {mFTPhost[0]=nil;}
        if (mFTPFilename[0]) {mFTPFilename[0]=nil;}
        
        for (int i=1;i<mFTPDownloadQueueDepth;i++) {
            mFilePath[i-1]=mFilePath[i];
            mFTPpath[i-1]=mFTPpath[i];
            mFTPhost[i-1]=mFTPhost[i];
            mFTPFilename[i-1]=mFTPFilename[i];
            mUsePrimaryAction[i-1]=mUsePrimaryAction[i];
            mFileSize[i-1]=mFileSize[i];
            mIsMODLAND[i-1]=mIsMODLAND[i];
        }
        if (mFTPDownloadQueueDepth) {
            mFilePath[mFTPDownloadQueueDepth-1]=nil;
            mFTPpath[mFTPDownloadQueueDepth-1]=nil;
            mFTPhost[mFTPDownloadQueueDepth-1]=nil;
            mFTPFilename[mFTPDownloadQueueDepth-1]=nil;
            mFTPDownloadQueueDepth--;
        }
        pthread_mutex_unlock(&download_mutex);
    }
	
	mGetFTPInProgress=0;
	[self checkNextQueuedItem];
	//    self.filePath = nil;
}

- (void)stream:(NSStream *)aStream handleEvent:(NSStreamEvent)eventCode {
    // An NSStream delegate callback that's called when events happen on our
    // network stream.
#pragma unused(aStream)
    assert(aStream == self.networkStream);
	
	if (mFTPAskCancel==1) {
		[self _stopReceiveWithStatus:@"Download cancelled" status:-1];
		mConnectionIssue=0;
		return;
	}
    if (mFTPAskCancel==2) {
		[self _stopReceiveWithStatus:@"Download suspended" status:-2];
		mConnectionIssue=0;
		return;
	}
	
    switch (eventCode) {
        case NSStreamEventOpenCompleted: {
			mConnectionIssue=0;
            [self _updateStatus:@"Opened connection"];
        } break;
        case NSStreamEventHasBytesAvailable: {
            NSInteger       bytesRead;
            uint8_t         *buffer;
			NSString *msg;
			
            buffer=(uint8_t*)malloc(FTP_BUFFER_SIZE);
            // Pull some data off the network.
            
            bytesRead = [self.networkStream read:buffer maxLength:FTP_BUFFER_SIZE];
			
			mCurrentDownloadedBytes+=bytesRead;
			
			msg=[NSString stringWithFormat:@"Receiving : %d",bytesRead];
            [self _updateStatus:msg];
			
            if (bytesRead == -1) {
                [self _stopReceiveWithStatus:@"Network read error" status:1];
            } else if (bytesRead == 0) {
                [self _stopReceiveWithStatus:nil status:0];
            } else {
                NSInteger   bytesWritten;
                NSInteger   bytesWrittenSoFar;
                
                // Write to the file.
                
                bytesWrittenSoFar = 0;
                do {
                    bytesWritten = [self.fileStream write:&buffer[bytesWrittenSoFar] maxLength:bytesRead - bytesWrittenSoFar];
                    assert(bytesWritten != 0);
                    if (bytesWritten == -1) {
                        [self _stopReceiveWithStatus:@"File write error" status:2];
                        break;
                    } else {
                        bytesWrittenSoFar += bytesWritten;
                    }
                } while (bytesWrittenSoFar != bytesRead);
            }
            free(buffer);
        } break;
        case NSStreamEventHasSpaceAvailable: {
            assert(NO);     // should never happen for the output stream
        } break;
        case NSStreamEventErrorOccurred: {
            //connection error
            if (mCurrentIsMODLAND!=2) [self suspend];
			else [self _stopReceiveWithStatus:@"Connection error." status:3];
        } break;
        case NSStreamEventEndEncountered: {
            // ignore
        } break;
        default: {
            assert(NO);
        } break;
    }
}

- (int)addFTPToDownloadList:(NSString *)filePath ftpURL:(NSString *)ftpPath ftpHost:(NSString*)ftpHost filesize:(int)filesize filename:(NSString*)fileName isMODLAND:(int)isMODLAND usePrimaryAction:(int)useDefaultAction {
	if (mFTPDownloadQueueDepth==0) mConnectionIssue=0;
	
	if (mFTPDownloadQueueDepth<MAX_DOWNLOAD_QUEUE) {
		//check it is not already in the list
		pthread_mutex_lock(&download_mutex);
		int duplicated=0;
		
		for (int i=0;i<=mFTPDownloadQueueDepth;i++) {
			if (mFilePath[i])
				if ([mFilePath[i] compare:filePath]==NSOrderedSame) {duplicated=1; break;}
		}
		if (!duplicated) {
            
			if (isMODLAND) { //check if secondary files are required
				//1/ TFMX => if mdat, smpl should be downloaded too
				NSRange r;
                
                //if TFMX-ST, change name from mdat to mdst
				r.location=NSNotFound;
				r = [fileName rangeOfString:@".mdat" options:NSCaseInsensitiveSearch];
				if (r.location != NSNotFound) {
                    r.location=NSNotFound;
                    r = [ftpPath rangeOfString:@"TFMX ST" options:NSCaseInsensitiveSearch];
                    if (r.location != NSNotFound) {
                        //if TFMX st, no smpl to load
                    } else {
                        char *tmp_str_ptr;
                        char tmp_str[1024];
                        strcpy(tmp_str,[filePath UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                        mFilePath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.smpl",tmp_str];
                        
                        strcpy(tmp_str,[ftpPath UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                        mFTPpath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.smpl",tmp_str];
                        
                        mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
                        
                        strcpy(tmp_str,[fileName UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                        mFTPFilename[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.smpl",tmp_str];
                        
                        mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
                        mFileSize[mFTPDownloadQueueDepth]=-1;
                        mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
                        
                        mFTPDownloadQueueDepth++;
                    }
				}
                r = [fileName rangeOfString:@"mdat." options:NSCaseInsensitiveSearch];
                if (r.location != NSNotFound) {
                    
                    
                    r.location=NSNotFound;
                    r = [ftpPath rangeOfString:@"TFMX ST" options:NSCaseInsensitiveSearch];
                    if (r.location != NSNotFound) {
                        //if TFMX st, no smpl to load
                    } else {
                        char *tmp_str_ptr;
                        char tmp_str[1024];
                        
                        strcpy(tmp_str,[filePath UTF8String]);
                        tmp_str_ptr=strrchr(tmp_str,'/');tmp_str_ptr++;
                        *tmp_str_ptr++='s';*tmp_str_ptr++='m';*tmp_str_ptr++='p';*tmp_str_ptr++='l';
                        mFilePath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s",tmp_str];
                        
                        strcpy(tmp_str,[ftpPath UTF8String]);
                        tmp_str_ptr=strrchr(tmp_str,'/');tmp_str_ptr++;
                        *tmp_str_ptr++='s';*tmp_str_ptr++='m';*tmp_str_ptr++='p';*tmp_str_ptr++='l';
                        mFTPpath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s",tmp_str];
                        
                        mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
                        
                        strcpy(tmp_str,[fileName UTF8String]);
                        tmp_str_ptr=strrchr(tmp_str,'/');
                        if (!tmp_str_ptr) tmp_str_ptr=tmp_str;
                        else tmp_str_ptr++;
                        *tmp_str_ptr++='s';*tmp_str_ptr++='m';*tmp_str_ptr++='p';*tmp_str_ptr++='l';
                        mFTPFilename[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s",tmp_str];
                        
                        mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
                        mFileSize[mFTPDownloadQueueDepth]=-1;
                        mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
                        
                        mFTPDownloadQueueDepth++;
                    }
                }
                //2/SCI => if .sci the .003 patch file should be downloaded as well
                r.location=NSNotFound;
				r = [fileName rangeOfString:@".sci" options:NSCaseInsensitiveSearch];
				if (r.location != NSNotFound) {
					char *tmp_str_ptr;
					char tmp_str[1024];
                    NSString *newPath,*newName;
                    
                    //                    NSLog(@"FILENAME: %@",filePath);
                    //                    NSLog(@"FILENAME: %@",[filePath stringByDeletingLastPathComponent]);
                    
                    newPath=[NSString stringWithFormat:@"%@/%@patch.003",[filePath stringByDeletingLastPathComponent],[fileName substringToIndex:3]];
					mFilePath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:newPath];
					
                    newPath=[NSString stringWithFormat:@"%@/%@patch.003",[ftpPath stringByDeletingLastPathComponent],[fileName substringToIndex:3]];
					mFTPpath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:newPath];
					
					mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
					
					mFTPFilename[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%@patch.003",[fileName substringToIndex:3]];
					
					mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
					mFileSize[mFTPDownloadQueueDepth]=-1;
					mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
					
					mFTPDownloadQueueDepth++;
					
				}
                //3/KH => if .kh the songplay file should be downloaded as well
                r.location=NSNotFound;
                r = [fileName rangeOfString:@".kh" options:NSCaseInsensitiveSearch];
                if (r.location != NSNotFound) {
                    char *tmp_str_ptr;
                    char tmp_str[1024];
                    NSString *newPath,*newName;
                    
                    //                    NSLog(@"FILENAME: %@",filePath);
                    //                    NSLog(@"FILENAME: %@",[filePath stringByDeletingLastPathComponent]);
                    
                    newPath=[NSString stringWithFormat:@"%@/songplay",[filePath stringByDeletingLastPathComponent]];
                    mFilePath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:newPath];
                    
                    newPath=[NSString stringWithFormat:@"%@/songplay",[ftpPath stringByDeletingLastPathComponent]];
                    mFTPpath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:newPath];
                    
                    mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
                    
                    mFTPFilename[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:@"songplay"];
                    
                    mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
                    mFileSize[mFTPDownloadQueueDepth]=-1;
                    mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
                    
                    mFTPDownloadQueueDepth++;
                    
                }
                //4/ Adlib tracker => if sng, ins should be downloaded too
                r.location=NSNotFound;
                r = [fileName rangeOfString:@".sng" options:NSCaseInsensitiveSearch];
                if (r.location != NSNotFound) {
                    char *tmp_str_ptr;
                    char tmp_str[1024];
                    strcpy(tmp_str,[filePath UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                    mFilePath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.ins",tmp_str];
                    
                    strcpy(tmp_str,[ftpPath UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                    mFTPpath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.ins",tmp_str];
                    mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
                    
                    strcpy(tmp_str,[fileName UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                    mFTPFilename[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.ins",tmp_str];
                    
                    mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
                    mFileSize[mFTPDownloadQueueDepth]=-1;
                    mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
                    
                    mFTPDownloadQueueDepth++;
                    
                }
                //5/ Stereo sid file -> .mus and .str should go be paired
                r.location=NSNotFound;
                r = [fileName rangeOfString:@".mus" options:NSCaseInsensitiveSearch];
                if (r.location != NSNotFound) {
                    char *tmp_str_ptr;
                    char tmp_str[1024];
                    strcpy(tmp_str,[filePath UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                    mFilePath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.str",tmp_str];
                    
                    strcpy(tmp_str,[ftpPath UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                    mFTPpath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.str",tmp_str];
                    
                    mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
                    
                    strcpy(tmp_str,[fileName UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                    mFTPFilename[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.str",tmp_str];
                    
                    mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
                    mFileSize[mFTPDownloadQueueDepth]=-1;
                    mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
                    
                    mFTPDownloadQueueDepth++;
                }
                r.location=NSNotFound;
                r = [fileName rangeOfString:@".str" options:NSCaseInsensitiveSearch];
                if (r.location != NSNotFound) {
                    char *tmp_str_ptr;
                    char tmp_str[1024];
                    strcpy(tmp_str,[filePath UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                    mFilePath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.mus",tmp_str];
                    
                    strcpy(tmp_str,[ftpPath UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                    mFTPpath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.mus",tmp_str];
                
                    mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
                    
                    strcpy(tmp_str,[fileName UTF8String]);tmp_str_ptr=strrchr(tmp_str,'.');*tmp_str_ptr=0;
                    mFTPFilename[mFTPDownloadQueueDepth]=[[NSString alloc] initWithFormat:@"%s.mus",tmp_str];
                    
                    mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
                    mFileSize[mFTPDownloadQueueDepth]=-1;
                    mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
                    
                    mFTPDownloadQueueDepth++;
                }
                r.location=NSNotFound;
                r = [fileName rangeOfString:@".mini" options:NSCaseInsensitiveSearch];
                if (r.location != NSNotFound) {  //trying to download
                    char *tmp_str_ptr;
                    char tmp_str[1024];
                    NSMutableArray *libsList=DBHelper::getMissingPartsNameFromFilePath(filePath,@"lib");
                    for (int i=0;i<[libsList count]/2;i++) {
                        NSString *fullP=(NSString *)[libsList objectAtIndex:i*2];
                        NSString *localP=(NSString *)[libsList objectAtIndex:i*2+1];
                        
                        NSString *filePath=fullP;
                        NSString *modFilename=[fullP lastPathComponent];
                        NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
                        NSString *localPath=[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,localP];
                        
                        //add only if file isn't existing already
                        if ([mFileMngr fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent:localPath]]==false) {
                            
                            bool already_in=false;
                            for (int j=0;j<mFTPDownloadQueueDepth;j++) {
                                if ([mFTPpath[mFTPDownloadQueueDepth] isEqualToString:ftpPath]) {
                                    already_in=true;
                                    break;
                                }
                            }
                            if (!already_in) {
                                mFilePath[mFTPDownloadQueueDepth]=localPath;
                                mFTPpath[mFTPDownloadQueueDepth]=ftpPath;
                                mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
                                mFTPFilename[mFTPDownloadQueueDepth]=modFilename;
                                
                                mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
                                mFileSize[mFTPDownloadQueueDepth]=-1;
                                mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
                                
                                mFTPDownloadQueueDepth++;
                            }
                        }
                    }
                    
                }
                r.location=NSNotFound;
                r = [fileName rangeOfString:@".mdx" options:NSCaseInsensitiveSearch];
                if (r.location != NSNotFound) {  //trying to download
                    char *tmp_str_ptr;
                    char tmp_str[1024];
                    NSMutableArray *libsList=DBHelper::getMissingPartsNameFromFilePath(filePath,@"pdx");
                    for (int i=0;i<[libsList count]/2;i++) {
                        NSString *fullP=(NSString *)[libsList objectAtIndex:i*2];
                        NSString *localP=(NSString *)[libsList objectAtIndex:i*2+1];
                        
                        NSString *filePath=fullP;
                        NSString *modFilename=[fullP lastPathComponent];
                        NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
                        NSString *localPath=[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,localP];
                        
                        //add only if file isn't existing already
                        if ([mFileMngr fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent:localPath]]==false) {
                            
                            bool already_in=false;
                            for (int j=0;j<mFTPDownloadQueueDepth;j++) {
                                if ([mFTPpath[mFTPDownloadQueueDepth] isEqualToString:ftpPath]) {
                                    already_in=true;
                                    break;
                                }
                            }
                            if (!already_in) {
                                
                                mFilePath[mFTPDownloadQueueDepth]=localPath;
                                mFTPpath[mFTPDownloadQueueDepth]=ftpPath;
                                mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
                                mFTPFilename[mFTPDownloadQueueDepth]=modFilename;
                                
                                mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
                                mFileSize[mFTPDownloadQueueDepth]=-1;
                                mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
                                
                                mFTPDownloadQueueDepth++;
                            }
                        }
                    }
                }
                r.location=NSNotFound;
                r = [fileName rangeOfString:@".eup" options:NSCaseInsensitiveSearch];
                if (r.location != NSNotFound) {  //trying to download
                    char *tmp_str_ptr;
                    char tmp_str[1024];
                    NSMutableArray *libsList=DBHelper::getMissingPartsNameFromFilePath(filePath,@"fmb");
                    for (int i=0;i<[libsList count]/2;i++) {
                        NSString *fullP=(NSString *)[libsList objectAtIndex:i*2];
                        NSString *localP=(NSString *)[libsList objectAtIndex:i*2+1];
                        
                        NSString *filePath=fullP;
                        NSString *modFilename=[fullP lastPathComponent];
                        NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
                        NSString *localPath=[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,localP];
                        
                        //add only if file isn't existing already
                        if ([mFileMngr fileExistsAtPath:[NSHomeDirectory() stringByAppendingPathComponent:localPath]]==false) {
                            
                            bool already_in=false;
                            for (int j=0;j<mFTPDownloadQueueDepth;j++) {
                                if ([mFTPpath[mFTPDownloadQueueDepth] isEqualToString:ftpPath]) {
                                    already_in=true;
                                    break;
                                }
                            }
                            if (!already_in) {
                                
                                mFilePath[mFTPDownloadQueueDepth]=localPath;
                                mFTPpath[mFTPDownloadQueueDepth]=ftpPath;
                                mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
                                mFTPFilename[mFTPDownloadQueueDepth]=modFilename;
                                
                                mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
                                mFileSize[mFTPDownloadQueueDepth]=-1;
                                mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
                                
                                mFTPDownloadQueueDepth++;
                            }
                        }
                    }
                    
                    libsList=DBHelper::getMissingPartsNameFromFilePath(filePath,@"pmb");
                    for (int i=0;i<[libsList count]/2;i++) {
                        NSString *fullP=(NSString *)[libsList objectAtIndex:i*2];
                        NSString *localP=(NSString *)[libsList objectAtIndex:i*2+1];
                        
                        NSString *filePath=fullP;
                        NSString *modFilename=[fullP lastPathComponent];
                        NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
                        NSString *localPath=[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,localP];
                        
                        
                        mFilePath[mFTPDownloadQueueDepth]=localPath;
                        mFTPpath[mFTPDownloadQueueDepth]=ftpPath;
                        mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
                        mFTPFilename[mFTPDownloadQueueDepth]=modFilename;
                        
                        mIsMODLAND[mFTPDownloadQueueDepth]=2; //will be treated as modland but not played
                        mFileSize[mFTPDownloadQueueDepth]=-1;
                        mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
                        
                        mFTPDownloadQueueDepth++;
                    }
                }
			}
			
			mFilePath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:filePath];
			mFTPpath[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpPath];
			mFTPhost[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:ftpHost];
			mFTPFilename[mFTPDownloadQueueDepth]=[[NSString alloc] initWithString:fileName];
			mIsMODLAND[mFTPDownloadQueueDepth]=isMODLAND;
			mFileSize[mFTPDownloadQueueDepth]=filesize;
			mUsePrimaryAction[mFTPDownloadQueueDepth]=useDefaultAction;
			mFTPDownloadQueueDepth++;
			[self checkNextQueuedItem];
		}
		pthread_mutex_unlock(&download_mutex);
		return 0;
	} else return -1;
}

- (int)addURLToDownloadList:(NSString *)url fileName:(NSString *)fileName filePath:(NSString *)filePath filesize:(long long)filesize isMODLAND:(int)isMODLAND usePrimaryAction:(int)useDefaultAction {
    if (mURLDownloadQueueDepth<MAX_DOWNLOAD_QUEUE) {
        
        //check it is not already in the list
        pthread_mutex_lock(&download_mutex);
        int duplicated=0;
        for (int i=0;i<=mURLDownloadQueueDepth;i++) {
            if (mURLFilename[i])
                if ([mURLFilename[i] compare:fileName]==NSOrderedSame) {duplicated=1; break;}
        }
        if (!duplicated) {
            if (fileName) mURLFilename[mURLDownloadQueueDepth]=[[NSString alloc] initWithString:fileName];
            else mURLFilename[mURLDownloadQueueDepth]=nil;
            mURLFilesize[mURLDownloadQueueDepth]=filesize;
            mURL[mURLDownloadQueueDepth]=[[NSString alloc] initWithString:url];
            mURLIsImage[mURLDownloadQueueDepth]=0;
            
            mURLIsMODLAND[mURLDownloadQueueDepth]=isMODLAND;
            mURLFilePath[mURLDownloadQueueDepth]=[[NSString alloc] initWithString:filePath];
            mURLUsePrimaryAction[mURLDownloadQueueDepth]=useDefaultAction;
            
            mURLDownloadQueueDepth++;
            [self checkNextQueuedItem];
        }
        
        pthread_mutex_unlock(&download_mutex);
        return 0;
    } else return -1;
}


- (int)addURLToDownloadList:(NSString *)url fileName:(NSString *)fileName  filesize:(long long)filesize{
	if (mURLDownloadQueueDepth<MAX_DOWNLOAD_QUEUE) {
		
		//check it is not already in the list
		pthread_mutex_lock(&download_mutex);
		int duplicated=0;
		for (int i=0;i<=mURLDownloadQueueDepth;i++) {
			if (mURLFilename[i])
				if ([mURLFilename[i] compare:fileName]==NSOrderedSame) {duplicated=1; break;}
		}
		if (!duplicated) {
			if (fileName) mURLFilename[mURLDownloadQueueDepth]=[[NSString alloc] initWithString:fileName];
			else mURLFilename[mURLDownloadQueueDepth]=nil;
			mURLFilesize[mURLDownloadQueueDepth]=filesize;
			mURL[mURLDownloadQueueDepth]=[[NSString alloc] initWithString:url];
            mURLIsImage[mURLDownloadQueueDepth]=0;
            
            mURLIsMODLAND[mURLDownloadQueueDepth]=0;
            mURLFilePath[mURLDownloadQueueDepth]=nil;
            mURLUsePrimaryAction[mURLDownloadQueueDepth]=0;
            
			mURLDownloadQueueDepth++;
			[self checkNextQueuedItem];
		}
		
		pthread_mutex_unlock(&download_mutex);
		return 0;
	} else return -1;
}
- (int)addURLImageToDownloadList:(NSString *)url fileName:(NSString *)fileName  filesize:(long long)filesize{
	if (mURLDownloadQueueDepth<MAX_DOWNLOAD_QUEUE) {
		
		//check it is not already in the list
		pthread_mutex_lock(&download_mutex);
		int duplicated=0;
		for (int i=0;i<=mURLDownloadQueueDepth;i++) {
			if (mURLFilename[i])
				if ([mURLFilename[i] compare:fileName]==NSOrderedSame) {duplicated=1; break;}
		}
		if (!duplicated) {
			if (fileName) mURLFilename[mURLDownloadQueueDepth]=[[NSString alloc] initWithString:fileName];
			else mURLFilename[mURLDownloadQueueDepth]=nil;
			mURLFilesize[mURLDownloadQueueDepth]=filesize;
			mURL[mURLDownloadQueueDepth]=[[NSString alloc] initWithString:url];
            mURLIsImage[mURLDownloadQueueDepth]=1;
			mURLDownloadQueueDepth++;
			[self checkNextQueuedItem];
		}
		
		pthread_mutex_unlock(&download_mutex);
		return 0;
	} else return -1;
}

- (void)checkNextQueuedItem {
	
	if (mFTPDownloadQueueDepth+mURLDownloadQueueDepth) {
		btnCancel.enabled=YES;
        btnClear.enabled=YES;
		barItem.badgeValue=[NSString stringWithFormat:@"%d",(mFTPDownloadQueueDepth+mURLDownloadQueueDepth)];
		if (mFTPDownloadQueueDepth&& (!mGetFTPInProgress)&& (!mGetURLInProgress)) [self startReceiveCurrentFTPEntry];
		else if (mURLDownloadQueueDepth&& (!mGetFTPInProgress)&& (!mGetURLInProgress)) [self startReceiveCurrentURLEntry];
	} else {
		barItem.badgeValue=nil;
		downloadLabelName.text=NSLocalizedString(@"No download in progress",@"");
		downloadLabelSize.text=@"";
		downloadPrgView.progress=0.0f;
		btnCancel.enabled=NO;
        btnClear.enabled=NO;
	}
	[tableView reloadData];
	
}

- (void) addDownloadedURLtoPlayer:(NSString*)_filename filepath:(NSString*)_filepath forcenoplay:(int)fnp {
	switch ((int)(settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value)) {
		case 0://do nothing
			break;
		case 1://Enqueue
			[detailViewController add_to_playlist:_filepath fileName:_filename forcenoplay:fnp];
			break;
		case 2://Play
			NSMutableArray *array_label = [[NSMutableArray alloc] init];
			NSMutableArray *array_path = [[NSMutableArray alloc] init];
			[array_label addObject:_filename];
			[array_path addObject:_filepath];
			[detailViewController play_listmodules:array_label start_index:0 path:array_path];
			
			break;
	}
}

- (void) addDownloadedDIRtoPlayer:(NSString*)_path shortPath:(NSString*)_shortPath{
	NSDirectoryEnumerator *dirEnum;
	NSDictionary *fileAttributes;
	NSString *file;
	NSMutableArray *filePaths,*fileNames;
	int nb_added=0;
    
	filePaths=[[NSMutableArray alloc] init];
	fileNames=[[NSMutableArray alloc] init];
	
	dirEnum = [mFileMngr enumeratorAtPath:_path];
	while (file = [dirEnum nextObject]) {
		fileAttributes=[dirEnum fileAttributes];
		if ([fileAttributes objectForKey:NSFileType]==NSFileTypeDirectory) {
			[dirEnum skipDescendents];
		}
		if ([fileAttributes objectForKey:NSFileType]==NSFileTypeRegular) {
			if ([ModizFileHelper isPlayableFile:file]) {
				nb_added++;
				//[self addDownloadedURLtoPlayer:file filepath:[NSString stringWithFormat:@"%@%@",_shortPath,file] forcenoplay:fnp];
				[filePaths addObject:[NSString stringWithFormat:@"%@%@",_shortPath,file]];
				[fileNames addObject:file];
			}
			
			[dirEnum skipDescendents];
		}
	}
	
	if (nb_added==0) {
        [self showAlertMsg:NSLocalizedString(@"Info", @"") message:NSLocalizedString(@"Could not add files. Please check manually with file browser (Local Browsing/Downloads).",@"")];
	} else {
		switch ((int)(settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value)) {
			case 0://do nothing
				break;
			case 1://Enqueue
				[detailViewController add_to_playlist:filePaths fileNames:fileNames forcenoplay:1];
				break;
			case 2://Play
				[detailViewController play_listmodules:fileNames start_index:0 path:filePaths];
				
				break;
		}
	}
	
}

- (void)updateToNextURL{
	pthread_mutex_lock(&download_mutex);
	if (mURL[0]) {mURL[0]=nil;}
	if (mURLFilename[0]) {mURLFilename[0]=nil;}
    if (mURLFilePath[0]) {mURLFilePath[0]=nil;}
	
	for (int i=1;i<mURLDownloadQueueDepth;i++) {
		mURL[i-1]=mURL[i];
		mURLFilename[i-1]=mURLFilename[i];
		mURLFilesize[i-1]=mURLFilesize[i];
        mURLIsImage[i-1]=mURLIsImage[i];
        mURLFilePath[i-1]=mURLFilePath[i];
        mURLIsMODLAND[i-1]=mURLIsMODLAND[i];
        mURLUsePrimaryAction[i-1]=mURLUsePrimaryAction[i];
	}
	if (mURLDownloadQueueDepth) {
		mURL[mURLDownloadQueueDepth-1]=nil;
		mURLFilename[mURLDownloadQueueDepth-1]=nil;
        mURLIsImage[mURLDownloadQueueDepth-1]=0;
        mURLFilePath[mURLDownloadQueueDepth-1]=nil;
        mURLIsMODLAND[mURLDownloadQueueDepth-1]=0;
        mURLUsePrimaryAction[mURLDownloadQueueDepth-1]=0;
		mURLDownloadQueueDepth--;
	}
	pthread_mutex_unlock(&download_mutex);
}

- (void)checkIfShouldAddFile:(NSString*)localPath fileName:(NSString*)fileName {
    if ([ModizFileHelper isPlayableFile:fileName]) {
        [self addDownloadedURLtoPlayer:fileName filepath:[NSString stringWithFormat:@"Documents/Downloads/%@",fileName] forcenoplay:1];
    }
}

- (void)requestFailed {
    bool moveToNext=true;
	if (lCancelURL) {
        if (lCancelURL==2) moveToNext=false; //Suspended
		lCancelURL=0;
	} else {
        [self showAlertMsg:NSLocalizedString(@"Error", @"") message:NSLocalizedString(@"Cannot download from this URL.",@"")];
	}
    if (moveToNext) [self updateToNextURL];
	mGetURLInProgress=0;
	[self checkNextQueuedItem];
}

-(void)updateHTTPProgress:(NSProgress*)downloadProgress{
    static NSDate *lastDate=nil;
    static int64_t last_progress=0;
    NSDate *currentDate;
    int64_t current_progress;
    static double speed=0;
    
    currentDate=[NSDate date];
    current_progress=[downloadProgress completedUnitCount];
    // check if new file, if so reset
    if (last_progress>=current_progress) {
        lastDate=nil;
        speed=0;
    }
    
    if (lastDate==nil) {
        //starting point
        lastDate=currentDate;
        last_progress=current_progress;
    } else {
        double time_passed=[currentDate timeIntervalSinceDate:lastDate];
        if (time_passed>=0.5f) {
            speed=(double)(current_progress-last_progress)/(double)time_passed/(1024*1024);
            last_progress=current_progress;
            lastDate=currentDate;
        }
    }
    
    if ([downloadProgress completedUnitCount]>1*1024*1024) {
        if (speed>0) downloadLabelSize.text=[NSString stringWithFormat:@"%.1lfMB / %.1lfMB - %.3f Mo/s",(double)[downloadProgress completedUnitCount]/(1024*1024),(double)[downloadProgress totalUnitCount]/(1024*1024),speed];
        else downloadLabelSize.text=[NSString stringWithFormat:@"%.1lfMB / %.1lfMB - n/a MB/s",(double)[downloadProgress completedUnitCount]/(1024*1024),(double)[downloadProgress totalUnitCount]/(1024*1024)];
    } else {
        if (speed>0) downloadLabelSize.text=[NSString stringWithFormat:@"%dKB / %dKB - %.3f Mo/s",[downloadProgress completedUnitCount]>>10,[downloadProgress totalUnitCount]>>10,speed];
        else downloadLabelSize.text=[NSString stringWithFormat:@"%dKB / %dKB - n/a Mo/s",[downloadProgress completedUnitCount]>>10,[downloadProgress totalUnitCount]>>10];
    }
    downloadPrgView.progress=[downloadProgress fractionCompleted];
}

-(void) startReceiveCurrentURLEntry{
	if (mGetURLInProgress) return;
    
    if (mSuspended) return;
    
    //Check download dir exist & create if needed
    NSError *err;
    [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"]  withIntermediateDirectories:TRUE attributes:nil error:&err];
    
	downloadLabelName.text=[[NSString stringWithFormat:NSLocalizedString(@"Downloading %@",@""),mURL[0]] stringByReplacingPercentEscapesUsingEncoding:NSASCIIStringEncoding];
	downloadLabelSize.text=[NSString stringWithFormat:@"%dKB",mURLFilesize[0]/1024];
	
	mGetURLInProgress=1;
	
	mCurrentURLFilename=mURLFilename[0];
    mCurrentURLIsImage=mURLIsImage[0];
    
    NSURLSessionConfiguration *configuration = [NSURLSessionConfiguration defaultSessionConfiguration];
        
    AFmanager = [[AFURLSessionManager alloc] initWithSessionConfiguration:configuration];
    
    AFmanager.responseSerializer = [AFHTTPResponseSerializer serializer];
    
    NSURL *URL = [NSURL URLWithString:mURL[0]];
    
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:URL];
    
    //set url host as referer as some downloads won't work without it (ex: sndh website)
    NSString *baseURL;
    if ([[[URL absoluteString] lowercaseString] containsString:@"https"]) baseURL=[NSString stringWithFormat:@"https://%@",URL.host];
    else baseURL=[NSString stringWithFormat:@"http://%@",URL.host];
    //NSLog(@"baseurl: %@",baseURL);
    
    [request setValue:baseURL forHTTPHeaderField:@"referer"];
    
    
    NSURLSessionDownloadTask *downloadTask = [AFmanager downloadTaskWithRequest:request
                                            progress:^(NSProgress * _Nonnull downloadProgress) {
        [self performSelectorOnMainThread:@selector(updateHTTPProgress:) withObject:downloadProgress waitUntilDone:YES];
        
    } 
                                        destination:^NSURL *(NSURL *targetPath, NSURLResponse *response) {
        
        NSString *fileName=mCurrentURLFilename;
        if (fileName==nil) fileName=[NSString stringWithString:[response suggestedFilename]];
        NSString *localPath;
            
        if (mCurrentURLIsImage) localPath=[[NSString alloc] initWithFormat:@"%@/%@",NSHomeDirectory(),fileName];
        else {
            if (mURLIsMODLAND[0]) {
                localPath=[[NSString alloc] initWithFormat:@"%@/%@",NSHomeDirectory(),mURLFilePath[0]];
            } else {
                localPath=[[NSString alloc] initWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/Downloads"],fileName];
            }
        }
        return [NSURL fileURLWithPath:localPath];
    } completionHandler:^(NSURLResponse *response, NSURL *filePath, NSError *error) {
        if (error) {
            NSLog(@"URL file download error for %@, error: %d %@", filePath,error.code,error.localizedDescription);
            [self requestFailed];
        } else {
            NSLog(@"File downloaded to: %@", filePath);
            
            [self addSkipBackupAttributeToItemAtPath:[filePath path]];
            
            if (mURLIsMODLAND[0]) {
                if ([ModizFileHelper isPlayableFile:mCurrentURLFilename]) {
                    if ((mURLUsePrimaryAction[0]==1)&&(mURLIsMODLAND[0]==1)) {
                        NSMutableArray *array_label = [[NSMutableArray alloc] init];
                        NSMutableArray *array_path = [[NSMutableArray alloc] init];
                        [array_label addObject:mCurrentURLFilename];
                        [array_path addObject:mURLFilePath[0]];
                        [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                        
                    } else {
                        if (mURLIsMODLAND[0]==1) {
                            [detailViewController add_to_playlist:mURLFilePath[0] fileName:mCurrentURLFilename forcenoplay:(mURLUsePrimaryAction[0]==2)];
                        }
                    }
                }
                //refresh view which potentially list the file as not downloaded
                [onlineVC refreshViewAfterDownload];
                [searchViewController refreshViewAfterDownload];
                [moreVC refreshViewAfterDownload];
                [rootViewController refreshViewAfterDownload];
                //TODO: playlist
            } else [self checkIfShouldAddFile:[filePath path] fileName:[[filePath path] lastPathComponent]];
            //Remove file if it is not part of accepted one
            
            [AFmanager invalidateSessionCancelingTasks:NO resetSession:NO];
            
            [self updateToNextURL];
            mGetURLInProgress=0;
            [self checkNextQueuedItem];
        }
    }];
    [downloadTask resume];
}

- (void)startReceiveCurrentFTPEntry {
    // Starts a connection to download the current URL.
    NSURL *             url;
    CFReadStreamRef     ftpStream;
	NSError *err;
    
    if (mSuspended) return;
	
	if (self.networkStream!=nil) {
		//already downloading
		return;
	}
    
    //Check download dir exist & create if needed
    [mFileMngr createDirectoryAtPath:[NSHomeDirectory() stringByAppendingPathComponent:@"Documents/Downloads"]  withIntermediateDirectories:TRUE attributes:nil error:&err];
    
    
    
	mGetFTPInProgress=1;
	mFTPAskCancel=0;
	
	mCurrentFilePath=mFilePath[0];
	mCurrentFileSize=mFileSize[0];
	mCurrentUsePrimaryAction=mUsePrimaryAction[0];
    mCurrentIsMODLAND=mIsMODLAND[0];
	mCurrentDownloadedBytes=0;
	mCurrentFilename=[[NSString stringWithString:mFTPFilename[0]] stringByReplacingPercentEscapesUsingEncoding:NSASCIIStringEncoding];
	downloadLabelName.text=[NSString stringWithFormat:NSLocalizedString(@"Downloading %@",@""),mFTPFilename[0]];
	if (mCurrentFileSize>0) downloadLabelSize.text=[NSString stringWithFormat:@"%dKB/%dKB",mCurrentDownloadedBytes/1024,mCurrentFileSize/1024];
	else downloadLabelSize.text=[NSString stringWithFormat:@"%dKB",mCurrentDownloadedBytes/1024];
	downloadPrgView.progress=0.0f;//(float)mCurrentDownloadedBytes/(float)mFileSize;
	// Open a CFFTPStream for the URL.
    
	//url=[[[NSURL alloc] initWithScheme:@"ftp" host:mFTPhost[0] path:[[NSString stringWithString:mFTPpath[0]] stringByAddingPercentEscapesUsingEncoding:NSISOLatin1StringEncoding]] autorelease];
    
	url=[NSURL URLWithString:[NSString stringWithFormat:@"ftp://%@%@",mFTPhost[0],[[NSString stringWithString:mFTPpath[0]] stringByAddingPercentEscapesUsingEncoding:NSISOLatin1StringEncoding]]];
    ftpStream = CFReadStreamCreateWithFTPURL(NULL, (__bridge CFURLRef) url);
    if (ftpStream == NULL) {
        [self showAlertMsg:NSLocalizedString(@"Error", @"") message:NSLocalizedString(@"Cannot connect to FTP.", @"")];
		return;
	}
	// Open a local stream for the file we're going to receive into.
	[mFileMngr removeItemAtPath:[NSHomeDirectory() stringByAppendingPathComponent:TMP_FILE_NAME] error:&err];
    self.fileStream = [NSOutputStream outputStreamToFileAtPath:[NSHomeDirectory() stringByAppendingPathComponent:TMP_FILE_NAME] append:NO];
    assert(self.fileStream != nil);
    [self.fileStream setDelegate:self];
    [self.fileStream open];
    
    [self addSkipBackupAttributeToItemAtPath:[NSHomeDirectory() stringByAppendingPathComponent:TMP_FILE_NAME]];
	
    // Open a FTP stream for the file to download
    self.networkStream = (__bridge NSInputStream *) ftpStream;
    [self.networkStream setDelegate:self];
    [self.networkStream scheduleInRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [self.networkStream open];
	
	// Have to release ftpStream to balance out the create.  self.networkStream
	// has retained this for our persistent use.
    CFRelease(ftpStream);
    // Tell the UI we're receiving.
	
	[self _receiveDidStart];
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	if (section==0) return @"FTP";
	if (section==1) return @"HTTP";
	return @"";
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (section==0) {
		return mFTPDownloadQueueDepth-mGetFTPInProgress;
	}
	if (section==1) {
		return mURLDownloadQueueDepth-mGetURLInProgress;
	}
	return 0;
}
/*
 - (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
 return nil;
 }*/
/*
 - (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
 else return -1;
 }*/


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *CellIdentifier = @"Cell";
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    UILabel *topLabel;
    UILabel *bottomLabel;
    
//	BButton *lCancelButton;
    
    if (forceReloadCells) {
        while ([tableView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        forceReloadCells=false;
    }
	
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,tableView.frame.size.width,40);
        [cell setBackgroundColor:[UIColor clearColor]];
        
        NSString *imgFilename=(darkMode?@"tabview_gradient40Black.png":@"tabview_gradient40.png");
        UIImage *image = [UIImage imageNamed:imgFilename];
        MDZUIImageView *imageView = [[MDZUIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;
        //[imageView release];
        
        cell.contentView.backgroundColor = [UIColor clearColor];
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
        topLabel.font = [UIFont boldSystemFontOfSize:18];
        topLabel.lineBreakMode=(NSLineBreakMode)UILineBreakModeMiddleTruncation;
        topLabel.opaque=TRUE;
        topLabel.autoresizesSubviews=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleRightMargin;
        
        //
        // Create the label for the top row of text
        //
        bottomLabel = [[UILabel alloc] init];
        [cell.contentView addSubview:bottomLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        bottomLabel.tag = BOTTOM_LABEL_TAG;
        bottomLabel.backgroundColor = [UIColor clearColor];
        bottomLabel.font = [UIFont systemFontOfSize:12];
        //bottomLabel.font = [UIFont fontWithName:@"courier" size:12];
        bottomLabel.lineBreakMode=(NSLineBreakMode)UILineBreakModeMiddleTruncation;
        bottomLabel.opaque=TRUE;
        bottomLabel.autoresizesSubviews=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleRightMargin;
        
        
		[cell layoutSubviews];
	} else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
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
    
    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                               0,
                               tableView.bounds.size.width -1.0 * cell.indentationWidth- 32-(cell.editingStyle==UITableViewCellEditingStyleDelete?32:0),
                               22);
    bottomLabel.frame = CGRectMake(1.0 * cell.indentationWidth,
                                   22,
                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32-(cell.editingStyle==UITableViewCellEditingStyleDelete?32:0),
                                   18);
    bottomLabel.text=@""; //default value
    
    
	
	if (indexPath.section==0) { //FTP
		if (indexPath.row>=mFTPDownloadQueueDepth) return cell;
		// Set up the cell...
		topLabel.text = mFTPFilename[indexPath.row+mGetFTPInProgress];
		//	cell.textLabel.font = [UIFont boldSystemFontOfSize:14];
		//	cell.textLabel.textColor=[UIColor whiteColor];
		//cell.accessoryType = UITableViewCellAccessoryNone;
		
	}
	if (indexPath.section==1) { //URL
		if (indexPath.row>=mURLDownloadQueueDepth) return cell;
		// Set up the cell...
		topLabel.text = mURL[indexPath.row+mGetURLInProgress];
		//	cell.textLabel.font = [UIFont boldSystemFontOfSize:14];
		//	cell.textLabel.textColor=[UIColor whiteColor];
		//cell.accessoryType = UITableViewCellAccessoryNone;
	}
	return cell;
}

// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tabView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    return YES;
}
- (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    return YES;
}

- (void)tableView:(UITableView *)tabView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
    pthread_mutex_lock(&download_mutex);
    if (fromIndexPath.section==0) { //FTP
        NSString *tmpS;
        tmpS=mFilePath[fromIndexPath.row];
        mFilePath[fromIndexPath.row]=mFilePath[toIndexPath.row];
        mFilePath[toIndexPath.row]=tmpS;
        
        tmpS=mFTPFilename[fromIndexPath.row];
        mFTPFilename[fromIndexPath.row]=mFTPFilename[toIndexPath.row];
        mFTPFilename[toIndexPath.row]=tmpS;
        
        tmpS=mFTPpath[fromIndexPath.row];
        mFTPpath[fromIndexPath.row]=mFTPpath[toIndexPath.row];
        mFTPpath[toIndexPath.row]=tmpS;
        
        tmpS=mFTPhost[fromIndexPath.row];
        mFTPhost[fromIndexPath.row]=mFTPhost[toIndexPath.row];
        mFTPhost[toIndexPath.row]=tmpS;
        
        int tmpI;
        tmpI=mFileSize[fromIndexPath.row];
        mFileSize[fromIndexPath.row]=mFileSize[toIndexPath.row];
        mFileSize[toIndexPath.row]=tmpI;
        
        tmpI=mUsePrimaryAction[fromIndexPath.row];
        mUsePrimaryAction[fromIndexPath.row]=mUsePrimaryAction[toIndexPath.row];
        mUsePrimaryAction[toIndexPath.row]=tmpI;
        
        unsigned char tmpC;
        tmpC=mIsMODLAND[fromIndexPath.row];
        mIsMODLAND[fromIndexPath.row]=mIsMODLAND[toIndexPath.row];
        mIsMODLAND[toIndexPath.row]=tmpC;                        
    } else { //HTTP
        NSString *tmpS;
        tmpS=mURL[fromIndexPath.row];
        mURL[fromIndexPath.row]=mURL[toIndexPath.row];
        mURL[toIndexPath.row]=tmpS;
        
        tmpS=mURLFilename[fromIndexPath.row];
        mURLFilename[fromIndexPath.row]=mURLFilename[toIndexPath.row];
        mURLFilename[toIndexPath.row]=tmpS;
        
        int tmpI;
        tmpI=mURLFilesize[fromIndexPath.row];
        mURLFilesize[fromIndexPath.row]=mURLFilesize[toIndexPath.row];
        mURLFilesize[toIndexPath.row]=tmpI;
        
        unsigned char tmpC;
        tmpC=mURLIsImage[fromIndexPath.row];
        mURLIsImage[fromIndexPath.row]=mURLIsImage[toIndexPath.row];
        mURLIsImage[toIndexPath.row]=tmpC;
        
    }
    pthread_mutex_unlock(&download_mutex);
}

- (NSIndexPath *)tableView:(UITableView *)tabView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath {
        //NSIndexPath *newIndexPath=[[[NSIndexPath alloc] initWithIndex:0] autorelease];
    if (proposedDestinationIndexPath.section>sourceIndexPath.section) {
        return [NSIndexPath indexPathForRow:[tabView numberOfRowsInSection:sourceIndexPath.section]-1 inSection:sourceIndexPath.section];
    } else if (proposedDestinationIndexPath.section<sourceIndexPath.section) {
        return [NSIndexPath indexPathForRow:0 inSection:sourceIndexPath.section];
    }
    return proposedDestinationIndexPath;
}

// Override to support editing the table view.
- (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (editingStyle == UITableViewCellEditingStyleDelete) {
    pthread_mutex_lock(&download_mutex);
	int pos=indexPath.row;
	if (indexPath.section==0) {//FTP
        if (mGetFTPInProgress) pos++;
		if (mFilePath[pos]) {mFilePath[pos]=nil;}
		if (mFTPpath[pos])  {mFTPpath[pos]=nil;}
		if (mFTPhost[pos])  {mFTPhost[pos]=nil;}
		if (mFTPFilename[pos]) {mFTPFilename[pos]=nil;}
		
		for (int i=pos;i<mFTPDownloadQueueDepth-1;i++) {
			mFilePath[i]=mFilePath[i+1];
			mFTPpath[i]=mFTPpath[i+1];
			mFTPhost[i]=mFTPhost[i+1];
			mFTPFilename[i]=mFTPFilename[i+1];
			mUsePrimaryAction[i]=mUsePrimaryAction[i+1];
			mFileSize[i]=mFileSize[i+1];
			mIsMODLAND[i]=mIsMODLAND[i+1];
		}
		if (mFTPDownloadQueueDepth) {
			mFilePath[mFTPDownloadQueueDepth-1]=nil;
			mFTPpath[mFTPDownloadQueueDepth-1]=nil;
			mFTPhost[mFTPDownloadQueueDepth-1]=nil;
			mFTPFilename[mFTPDownloadQueueDepth-1]=nil;
			mFTPDownloadQueueDepth--;
		}
		
	}
	if (indexPath.section==1) {//URL
		if (mGetURLInProgress) pos++;
		if (mURL[pos]) {mURL[pos]=nil;}
		if (mURLFilename[pos]) {mURLFilename[pos]=nil;}
        
		for (int i=pos;i<mURLDownloadQueueDepth-1;i++) {
			mURL[i]=mURL[i+1];
			mURLFilename[i]=mURLFilename[i+1];
		}
		if (mURLDownloadQueueDepth) {
			mURL[mURLDownloadQueueDepth-1]=nil;
			mURLFilename[mURLDownloadQueueDepth-1]=nil;
			mURLDownloadQueueDepth--;
		}
	}
	
	if (mFTPDownloadQueueDepth+mURLDownloadQueueDepth) barItem.badgeValue=[NSString stringWithFormat:@"%d",(mFTPDownloadQueueDepth+mURLDownloadQueueDepth)];
    else barItem.badgeValue=nil;
	
	[tableView reloadData];
	pthread_mutex_unlock(&download_mutex);
    }
}

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    }
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
