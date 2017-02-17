//
//  DownloadViewController.m
//  modizer4
//
//  Created by yoyofr on 6/20/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

extern BOOL is_ios7;

#define FTP_BUFFER_SIZE 256*1024
#define TMP_FILE_NAME @"Documents/tmp.tmpfile"

#import "DownloadViewController.h"
#import "SettingsGenViewController.h"
#import "OnlineViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];
#import <CFNetwork/CFNetwork.h>
#include <sys/xattr.h>


int lCancelURL;
extern pthread_mutex_t download_mutex;
static volatile int mGetURLInProgress;
static volatile int mGetFTPInProgress,mConnectionIssue,mSuspended;

static NSFileManager *mFileMngr;

#import "MDZUIImageView.h"

@implementation DownloadViewController

@synthesize networkStream,fileStream,downloadLabelSize,downloadLabelName,downloadTabView,downloadPrgView,detailViewController,barItem,rootViewController,onlineVC;
@synthesize searchViewController,btnCancel,btnSuspend,btnResume,btnClear;
@synthesize mFTPDownloadQueueDepth,mURLDownloadQueueDepth,moreVC;

/* NOT Implemented yet
- (void)backupDownloadList {
    
}

- (void)restoreDownloadList {
    NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
    NSString *str,*valStr;
    
	for (int i=0;i<MAX_DOWNLOAD_QUEUE;i++) {
        str=[NSString stringWithFormat:@"DownSlot#%d",i];
        valStr=[prefs objectForKey:str];
        if (valStr!=nil) {
            
        }
    }

}
*/
- (BOOL)addSkipBackupAttributeToItemAtPath:(NSString* )path
{
    const char* filePath = [path fileSystemRepresentation];
    
    const char* attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    
    int result = setxattr(filePath, attrName, &attrValue, sizeof(attrValue), 0, 0);
    return result == 0;
}

-(IBAction) goPlayer {
    if (detailViewController.mPlaylist_size) [self.navigationController pushViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];
    else {
        UIAlertView *nofileplaying=[[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
        [nofileplaying show];
    }
}

-(IBAction)cancelCurrent {
	if (mGetFTPInProgress) {//FTP
		mFTPAskCancel=1;
		[self _stopReceiveWithStatus:@"Download cancelled" status:-1];
	} else if (mGetURLInProgress) {//HTTP
		lCancelURL=1;
		[mASIrequest cancel];
	}
}

-(IBAction) clearAll {
    pthread_mutex_lock(&download_mutex);
	
    //FTP
    for (int i=mGetFTPInProgress;i<mFTPDownloadQueueDepth;i++) {
		if (mFilePath[i]) {[mFilePath[i] release];mFilePath[i]=nil;}
		if (mFTPpath[i])  {[mFTPpath[i] release];mFTPpath[i]=nil;}
		if (mFTPhost[i])  {[mFTPhost[i] release];mFTPhost[i]=nil;}
		if (mFTPFilename[i]) {[mFTPFilename[i] release];mFTPFilename[i]=nil;}
	}
    mFTPDownloadQueueDepth=mGetFTPInProgress;
    
    //HTTP URL
    for (int i=mGetURLInProgress;i<mURLDownloadQueueDepth;i++) {
        if (mURL[i]) {[mURL[i] release];mURL[i]=nil;}
        if (mURLFilename[i])  {[mURLFilename[i] release];mURLFilename[i]=nil;}
        if (mURLFilePath[i])  {[mURLFilePath[i] release];mURLFilePath[i]=nil;}
        mURL[i]=nil;
        mURLFilename[i]=nil;
        mURLFilePath[i]=nil;
    }
    mURLDownloadQueueDepth=mGetURLInProgress;
    
	barItem.badgeValue=nil;
	
	[downloadTabView reloadData];
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
		[mASIrequest cancel];
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

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
	clock_t start_time,end_time;
	start_time=clock();
    
    mFileMngr=[[NSFileManager alloc] init];
    
    
    [btnCancel setType:BButtonTypeGray];
    [btnClear setType:BButtonTypeGray];
    [btnSuspend setType:BButtonTypeGray];
    [btnResume setType:BButtonTypeGray];
    
    [btnCancel setShouldShowDisabled:YES];
    [btnClear setShouldShowDisabled:YES];
	
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
	
	downloadTabView.rowHeight=40;
	
	for (int i=0;i<MAX_DOWNLOAD_QUEUE;i++) {
		mFTPDownloaded[i]=0;
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
        mStatus[i]=0;
	}
    
    UIButton *btn = [[[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)] autorelease];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[[UIBarButtonItem alloc] initWithCustomView: btn] autorelease];
    self.navigationItem.rightBarButtonItem = item;
    
    self.downloadTabView.editing=TRUE;
    
    [super viewDidLoad];
	end_time=clock();
#ifdef LOAD_PROFILE
	NSLog(@"download : %d",end_time-start_time);
#endif
}

- (void)viewWillAppear:(BOOL)animated {
    if (!is_ios7) {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleBlack];
    } else {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    }
    [super viewWillAppear:animated];
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
	for (int i=0;i<MAX_DOWNLOAD_QUEUE;i++) {
		if (mFilePath[i]) {[mFilePath[i] release];mFilePath[i]=nil;}
		if (mFTPpath[i])  {[mFTPpath[i] release];mFTPpath[i]=nil;}
		if (mFTPhost[i])  {[mFTPhost[i] release];mFTPhost[i]=nil;}
		if (mFTPFilename[i]) {[mFTPFilename[i] release];mFTPFilename[i]=nil;}
		
		if (mURL[i]) {[mURL[i] release];mURL[i]=nil;}
		if (mURLFilename[i]) {[mURLFilename[i] release];mURLFilename[i]=nil;}
        if (mURLFilePath[i]) {[mURLFilePath[i] release];mURLFilePath[i]=nil;}
	}
    
    [mFileMngr release];
    
    [super dealloc];
}


- (void)_receiveDidStart {
}

- (void)_updateStatus:(NSString *)statusString {
	if (mCurrentFileSize) {
		downloadLabelSize.text=[NSString stringWithFormat:@"%dKB/%dKB",mCurrentDownloadedBytes/1024,mCurrentFileSize/1024];
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
			if ([self isAllowedFile:mCurrentFilename]) {
                if ((mCurrentUsePrimaryAction==1)&&(mIsMODLAND[0]==1)) {
                    NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                    NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                    [array_label addObject:mCurrentFilename];
                    [array_path addObject:mCurrentFilePath];
                    [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                    //[self goPlayer];
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
			UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:@"Warning" message:statusString delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
			[alert show];
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
        if (mFilePath[0]) {[mFilePath[0] release];mFilePath[0]=nil;}
        if (mFTPpath[0])  {[mFTPpath[0] release];mFTPpath[0]=nil;}
        if (mFTPhost[0])  {[mFTPhost[0] release];mFTPhost[0]=nil;}
        if (mFTPFilename[0]) {[mFTPFilename[0] release];mFTPFilename[0]=nil;}
        
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
            uint8_t         buffer[FTP_BUFFER_SIZE];
			NSString *msg;
			
            
            // Pull some data off the network.
            
            bytesRead = [self.networkStream read:buffer maxLength:sizeof(buffer)];
			
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
        } break;
        case NSStreamEventHasSpaceAvailable: {
            assert(NO);     // should never happen for the output stream
        } break;
        case NSStreamEventErrorOccurred: {
            //connection error
            [self suspend];
			//[self _stopReceiveWithStatus:@"Connection error." status:3];
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
				r.location=NSNotFound;
				r = [fileName rangeOfString:@".mdat" options:NSCaseInsensitiveSearch];
				if (r.location != NSNotFound) {
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

-(void)checkNextQueuedItem {
	
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
	[downloadTabView reloadData];
	
}

-(void) addDownloadedURLtoPlayer:(NSString*)_filename filepath:(NSString*)_filepath forcenoplay:(int)fnp {
	switch ((int)(settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value)) {
		case 0://do nothing
			break;
		case 1://Enqueue
			[detailViewController add_to_playlist:_filepath fileName:_filename forcenoplay:fnp];
			break;
		case 2://Play
			NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
			NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
			[array_label addObject:_filename];
			[array_path addObject:_filepath];
			[detailViewController play_listmodules:array_label start_index:0 path:array_path];
			//[self goPlayer];
			break;
	}
}

-(int) isAllowedFile:(NSString*)file {
	NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
	NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
	NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
	NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
	NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
	NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_MODPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extDUMB=[SUPPORTED_FILETYPE_DUMB componentsSeparatedByString:@","];
	NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
	NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
	NSArray *filetype_extSEXYPSF=[SUPPORTED_FILETYPE_SEXYPSF componentsSeparatedByString:@","];
	NSArray *filetype_extAOSDK=[SUPPORTED_FILETYPE_AOSDK componentsSeparatedByString:@","];
	NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
	NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
	NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
	NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
    NSArray *filetype_extLAZYUSF=[SUPPORTED_FILETYPE_LAZYUSF componentsSeparatedByString:@","];
    NSArray *filetype_extXSF=[SUPPORTED_FILETYPE_XSF componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
	NSString *extension = [file pathExtension];
	NSString *file_no_ext = [[file lastPathComponent] stringByDeletingPathExtension];
	
	int found=0;
	for (int i=0;i<[filetype_extUADE count];i++) {
		if ([extension caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		if ([file_no_ext caseInsensitiveCompare:[filetype_extUADE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
	}
	if (!found)
		for (int i=0;i<[filetype_extMDX count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extMDX objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extSID count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSID objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extSTSOUND count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSTSOUND objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extSC68 count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSC68 objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
	if (!found)
        for (int i=0;i<[filetype_extMODPLUG count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extMODPLUG objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extXMP count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extXMP objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extDUMB count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extDUMB objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
	if (!found)
		for (int i=0;i<[filetype_extGME count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extGME objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extADPLUG count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extADPLUG objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extSEXYPSF count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extSEXYPSF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extSEXYPSF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
    if (!found)
        for (int i=0;i<[filetype_extLAZYUSF count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extLAZYUSF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extLAZYUSF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extXSF count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extXSF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extXSF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
    if (!found)
        for (int i=0;i<[filetype_extVGMSTREAM count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extVGMSTREAM objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
	if (!found)
		for (int i=0;i<[filetype_extAOSDK count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extAOSDK objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extAOSDK objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extHVL count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extHVL objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extGSF count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extGSF objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
	if (!found)
		for (int i=0;i<[filetype_extASAP count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extASAP objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
    if (!found)
        for (int i=0;i<[filetype_extVGM count];i++) {
            if ([extension caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
            if ([file_no_ext caseInsensitiveCompare:[filetype_extVGM objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
        }
	if (!found)
		for (int i=0;i<[filetype_extWMIDI count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extWMIDI objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
    if (!found)
		for (int i=0;i<[filetype_extARCHIVE count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extARCHIVE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extARCHIVE objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
    if (!found)
		for (int i=0;i<[filetype_extPMD count];i++) {
			if ([extension caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
			if ([file_no_ext caseInsensitiveCompare:[filetype_extPMD objectAtIndex:i]]==NSOrderedSame) {found=1;break;}
		}
    
	if (found) return 1;
	return 0;
}

-(void) addDownloadedDIRtoPlayer:(NSString*)_path shortPath:(NSString*)_shortPath{
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
			if ([self isAllowedFile:file]) {
				nb_added++;
				//[self addDownloadedURLtoPlayer:file filepath:[NSString stringWithFormat:@"%@%@",_shortPath,file] forcenoplay:fnp];
				[filePaths addObject:[NSString stringWithFormat:@"%@%@",_shortPath,file]];
				[fileNames addObject:file];
			}
			
			[dirEnum skipDescendents];
		}
	}
	
	if (nb_added==0) {
		UIAlertView *alert = [[[UIAlertView alloc] initWithTitle:@"Info"
                                                         message:[NSString stringWithString:NSLocalizedString(@"Could not add files. Please check manually with file browser (Local Browsing/Downloads).",@"")] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
		[alert show];
	} else {
		switch ((int)(settings[GLOB_AfterDownloadAction].detail.mdz_switch.switch_value)) {
			case 0://do nothing
				break;
			case 1://Enqueue
				[detailViewController add_to_playlist:filePaths fileNames:fileNames forcenoplay:1];
				break;
			case 2://Play
				[detailViewController play_listmodules:fileNames start_index:0 path:filePaths];
				//[self goPlayer];
				break;
		}
	}
	[filePaths autorelease];
	[fileNames autorelease];
}

- (void)updateToNextURL{
	pthread_mutex_lock(&download_mutex);
	if (mURL[0]) {[mURL[0] release];mURL[0]=nil;}
	if (mURLFilename[0]) {[mURLFilename[0] release];mURLFilename[0]=nil;}
    if (mURLFilePath[0]) {[mURLFilePath[0] release];mURLFilePath[0]=nil;}
	
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
    if ([self isAllowedFile:fileName]) {
        [self addDownloadedURLtoPlayer:fileName filepath:[NSString stringWithFormat:@"Documents/Downloads/%@",fileName] forcenoplay:1];
    }
}

- (void)requestFinished:(ASIHTTPRequest *)request {
	NSData *fileData;
	NSString *fileName;
	NSError *err;
	char _strFilename[1024];
	
	fileName=mCurrentURLFilename;
	if (fileName==nil) {
		//Determine filename
		NSDictionary *dict=[request responseHeaders];
		
		
		
		NSString *contentDispo = [dict objectForKey:@"Content-Disposition"];
		if (contentDispo) {
			//With content-disposition header field
			_strFilename[0]=0;
			sscanf([contentDispo UTF8String],"attachment;\nfilename=%s",_strFilename);
			if (_strFilename[0]) {
				if (_strFilename[strlen(_strFilename)-1]==';') _strFilename[strlen(_strFilename)-1]=0;
				if (_strFilename[strlen(_strFilename)-1]=='"') _strFilename[strlen(_strFilename)-1]=0;
				if (_strFilename[0]=='"') for (int i=0;i<strlen(_strFilename);i++) _strFilename[i]=_strFilename[i+1];
			} else {
				//cannot determine a filename
				[self updateToNextURL];
				mGetURLInProgress=0;
				[self checkNextQueuedItem];
				NSLog(@"no filename");
				return;
			}
		} else {
			//no content-dispo
			[self updateToNextURL];
			mGetURLInProgress=0;
			[self checkNextQueuedItem];
			NSLog(@"no cont-disp");
			return;
		}
		fileName=[NSString stringWithFormat:@"%s",_strFilename];
	}
	sprintf(_strFilename,"%s",[fileName UTF8String]);
	
	fileData = [request responseData];
	NSString *localPath;
	
    if (mCurrentURLIsImage) localPath=[[[NSString alloc] initWithFormat:@"%@/%s",NSHomeDirectory(),_strFilename] autorelease];
    else {
        if (mURLIsMODLAND[0]) {
            localPath=[[[NSString alloc] initWithFormat:@"%@/%@",NSHomeDirectory(),mURLFilePath[0]] autorelease];
        } else {
            localPath=[[[NSString alloc] initWithFormat:@"%@/%s",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/Downloads"],_strFilename] autorelease];
        }
    }
	[mFileMngr createDirectoryAtPath:[localPath stringByDeletingLastPathComponent] withIntermediateDirectories:TRUE attributes:nil error:&err];
	[fileData writeToFile:localPath atomically:NO];
	
    [self addSkipBackupAttributeToItemAtPath:localPath];
	
    if (mURLIsMODLAND[0]) {
        if ([self isAllowedFile:mCurrentURLFilename]) {
            if ((mURLUsePrimaryAction[0]==1)&&(mURLIsMODLAND[0]==1)) {
                NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                [array_label addObject:mCurrentURLFilename];
                [array_path addObject:mURLFilePath[0]];
                [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                //[self goPlayer];
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
    } else [self checkIfShouldAddFile:localPath fileName:fileName];
	//Remove file if it is not part of accepted one
	
	[self updateToNextURL];
	mGetURLInProgress=0;
	[self checkNextQueuedItem];
}

- (void)requestFailed:(ASIHTTPRequest *)request {
	UIAlertView *alert;
	if (lCancelURL) {
		lCancelURL=0;
		//alert = [[[UIAlertView alloc] initWithTitle:@"Error" message:[NSString stringWithFormat:@"Cannot download from this URL."] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
	} else {
		alert = [[[UIAlertView alloc] initWithTitle:@"Error" message:[NSString stringWithString:NSLocalizedString(@"Cannot download from this URL.",@"")] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
		[alert show];
	}
	[self updateToNextURL];
	mGetURLInProgress=0;
	[self checkNextQueuedItem];
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
	mASIrequest = [ASIHTTPRequest requestWithURL:[NSURL URLWithString:mURL[0]]];
	[mASIrequest setDownloadProgressDelegate:downloadPrgView];
	[mASIrequest setDelegate:self];
	[mASIrequest startAsynchronous];
}

- (void)startReceiveCurrentFTPEntry {
    // Starts a connection to download the current URL.
    NSURL *             url;
    CFReadStreamRef     ftpStream;
	UIAlertView *alert;
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
	mCurrentDownloadedBytes=0;
	mCurrentFilename=[[NSString stringWithString:mFTPFilename[0]] stringByReplacingPercentEscapesUsingEncoding:NSASCIIStringEncoding];
	downloadLabelName.text=[NSString stringWithFormat:NSLocalizedString(@"Downloading %@",@""),mFTPFilename[0]];
	if (mCurrentFileSize>0) downloadLabelSize.text=[NSString stringWithFormat:@"%dKB/%dKB",mCurrentDownloadedBytes/1024,mCurrentFileSize/1024];
	else downloadLabelSize.text=[NSString stringWithFormat:@"%dKB",mCurrentDownloadedBytes/1024];
	downloadPrgView.progress=0.0f;//(float)mCurrentDownloadedBytes/(float)mFileSize;
	// Open a CFFTPStream for the URL.
    
	//url=[[[NSURL alloc] initWithScheme:@"ftp" host:mFTPhost[0] path:[[NSString stringWithString:mFTPpath[0]] stringByAddingPercentEscapesUsingEncoding:NSISOLatin1StringEncoding]] autorelease];
    
	url=[NSURL URLWithString:[NSString stringWithFormat:@"ftp://%@%@",mFTPhost[0],[[NSString stringWithString:mFTPpath[0]] stringByAddingPercentEscapesUsingEncoding:NSISOLatin1StringEncoding]]];
    ftpStream = CFReadStreamCreateWithFTPURL(NULL, (CFURLRef) url);
    if (ftpStream == NULL) {
		alert = [[[UIAlertView alloc] initWithTitle:@"Error" message:[NSString stringWithFormat:@"Cannot connect to FTP."] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
		[alert show];
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
    self.networkStream = (NSInputStream *) ftpStream;
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

- (void) cancelTapped: (UIButton*) sender {
	pthread_mutex_lock(&download_mutex);
	NSIndexPath *indexPath = [downloadTabView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.downloadTabView]];
	int pos=indexPath.row;
	if (indexPath.section==0) {//FTP
        if (mGetFTPInProgress) pos++;
		if (mFilePath[pos]) {[mFilePath[pos] release];mFilePath[pos]=nil;}
		if (mFTPpath[pos])  {[mFTPpath[pos] release];mFTPpath[pos]=nil;}
		if (mFTPhost[pos])  {[mFTPhost[pos] release];mFTPhost[pos]=nil;}
		if (mFTPFilename[pos]) {[mFTPFilename[pos] release];mFTPFilename[pos]=nil;}
		
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
		if (mURL[pos]) {[mURL[pos] release];mURL[pos]=nil;}
		if (mURLFilename[pos]) {[mURLFilename[pos] release];mURLFilename[pos]=nil;}
        
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
	
	[downloadTabView reloadData];
	pthread_mutex_unlock(&download_mutex);
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *CellIdentifier = @"Cell";
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    UILabel *topLabel;
    UILabel *bottomLabel;
    
//	BButton *lCancelButton;
	
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        
        cell.frame=CGRectMake(0,0,tableView.frame.size.width,40);
        [cell setBackgroundColor:[UIColor clearColor]];
        
        UIImage *image = [UIImage imageNamed:@"tabview_gradient40.png"];
        MDZUIImageView *imageView = [[MDZUIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;
        [imageView release];
        
        cell.contentView.backgroundColor = [UIColor clearColor];
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
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        topLabel.font = [UIFont boldSystemFontOfSize:18];
        topLabel.lineBreakMode=(NSLineBreakMode)UILineBreakModeMiddleTruncation;
        topLabel.opaque=TRUE;
        topLabel.autoresizesSubviews=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleRightMargin;
        
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
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
        bottomLabel.font = [UIFont systemFontOfSize:12];
        //bottomLabel.font = [UIFont fontWithName:@"courier" size:12];
        bottomLabel.lineBreakMode=(NSLineBreakMode)UILineBreakModeMiddleTruncation;
        bottomLabel.opaque=TRUE;
        bottomLabel.autoresizesSubviews=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleRightMargin;
        
        
/*        lCancelButton = [[[BButton alloc] initWithFrame:CGRectMake(tableView.bounds.size.width-2-32,4,32,32) type:BButtonTypeDanger] autorelease];
        [lCancelButton addTarget: self action: @selector(cancelTapped:) forControlEvents: UIControlEventTouchUpInside];
		cell.accessoryView=lCancelButton;
        [lCancelButton addAwesomeIcon:0x057 beforeTitle:YES font_size:20];*/
		[cell layoutSubviews];
	} else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
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
        
        tmpI=mFTPDownloaded[fromIndexPath.row];
        mFTPDownloaded[fromIndexPath.row]=mFTPDownloaded[toIndexPath.row];
        mFTPDownloaded[toIndexPath.row]=tmpI;
        
        unsigned char tmpC;
        tmpC=mIsMODLAND[fromIndexPath.row];
        mIsMODLAND[fromIndexPath.row]=mIsMODLAND[toIndexPath.row];
        mIsMODLAND[toIndexPath.row]=tmpC;
        
        tmpC=mStatus[fromIndexPath.row];
        mStatus[fromIndexPath.row]=mStatus[toIndexPath.row];
        mStatus[toIndexPath.row]=tmpC;
        
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
		if (mFilePath[pos]) {[mFilePath[pos] release];mFilePath[pos]=nil;}
		if (mFTPpath[pos])  {[mFTPpath[pos] release];mFTPpath[pos]=nil;}
		if (mFTPhost[pos])  {[mFTPhost[pos] release];mFTPhost[pos]=nil;}
		if (mFTPFilename[pos]) {[mFTPFilename[pos] release];mFTPFilename[pos]=nil;}
		
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
		if (mURL[pos]) {[mURL[pos] release];mURL[pos]=nil;}
		if (mURLFilename[pos]) {[mURLFilename[pos] release];mURLFilename[pos]=nil;}
        
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
	
	[downloadTabView reloadData];
	pthread_mutex_unlock(&download_mutex);
    }
}

@end
