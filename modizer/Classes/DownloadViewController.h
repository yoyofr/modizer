//
//  DownloadViewController.h
//  modizer4
//
//  Created by yoyofr on 6/20/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "DetailViewControllerIphone.h"
#import "RootViewControllerLocalBrowser.h"
#import "MoreViewController.h"
#import "ASIHTTPRequest.h"
#import "BButton.h"

#define MAX_DOWNLOAD_QUEUE 256

#define STATUS_STOPPED 0
#define STATUS_DOWNLOADING 1

@class OnlineViewController;
@class SearchViewController;
@class MoreViewController;

@interface DownloadViewController : UIViewController <UITableViewDelegate,NSStreamDelegate> {
	IBOutlet DetailViewControllerIphone *detailViewController;
	IBOutlet RootViewControllerLocalBrowser *rootViewController;
    IBOutlet OnlineViewController *onlineVC;
    IBOutlet MoreViewController *moreVC;
	IBOutlet SearchViewController *searchViewController;
	IBOutlet UITableView *downloadTabView;
	IBOutlet BButton *btnCancel,*btnSuspend,*btnResume,*btnClear;
	
	ASIHTTPRequest *mASIrequest;
    
	NSInputStream *networkStream;
	NSOutputStream *fileStream;
	
	NSString *mFilePath[MAX_DOWNLOAD_QUEUE],*mFTPFilename[MAX_DOWNLOAD_QUEUE],*mFTPpath[MAX_DOWNLOAD_QUEUE],*mFTPhost[MAX_DOWNLOAD_QUEUE];
	int mFileSize[MAX_DOWNLOAD_QUEUE],mUsePrimaryAction[MAX_DOWNLOAD_QUEUE];
	int mFTPDownloaded[MAX_DOWNLOAD_QUEUE];
	unsigned char mIsMODLAND[MAX_DOWNLOAD_QUEUE];
    unsigned char mStatus[MAX_DOWNLOAD_QUEUE];
	
	volatile int mFTPAskCancel;
	
	NSString *mURL[MAX_DOWNLOAD_QUEUE];
	NSString *mURLFilename[MAX_DOWNLOAD_QUEUE],*mURLFilePath[MAX_DOWNLOAD_QUEUE];
    unsigned char mURLIsMODLAND[MAX_DOWNLOAD_QUEUE];
	int mURLFilesize[MAX_DOWNLOAD_QUEUE],mURLUsePrimaryAction[MAX_DOWNLOAD_QUEUE];
    unsigned char mURLIsImage[MAX_DOWNLOAD_QUEUE];
    unsigned char mCurrentURLIsImage;
	
	NSString *mCurrentFilePath,*mCurrentFilename,*mCurrentFTPpath,*mCurrentURLFilename;
	int mCurrentFileSize,mCurrentUsePrimaryAction;
	int mCurrentDownloadedBytes;
	
	IBOutlet UILabel *downloadLabelName,*downloadLabelSize;
	IBOutlet UIProgressView *downloadPrgView;
	
	IBOutlet UITabBarItem *barItem;
@public
    int mFTPDownloadQueueDepth;
    int mURLDownloadQueueDepth;
}

/****************/
-(int)addFTPToDownloadList:(NSString *)filePath ftpURL:(NSString *)ftpPath ftpHost:(NSString*)ftpHost filesize:(int)filesize filename:(NSString*)fileName isMODLAND:(int)isMODLAND usePrimaryAction:(int)useDefaultAction;

//-(int)addEntriesFTPToDownloadList:(NSArray *)filePaths ftpURLs:(NSArray *)ftpPaths ftpHost:(NSString*)ftpHost filesizes:(int*)filesizes filenames:(NSArray*)fileName isMODLAND:(int)isMODLAND usePrimaryAction:(int)useDefaultAction;


-(int) addURLToDownloadList:(NSString *)url fileName:(NSString *)fileName filesize:(long long)filesize;
- (int)addURLToDownloadList:(NSString *)url fileName:(NSString *)fileName filePath:(NSString *)filePath filesize:(long long)filesize isMODLAND:(int)isMODLAND usePrimaryAction:(int)useDefaultAction;

-(int) addURLImageToDownloadList:(NSString *)url fileName:(NSString *)fileName filesize:(long long)filesize;
/****************/

-(void) checkNextQueuedItem;
-(void) startReceiveCurrentURLEntry;
-(void) startReceiveCurrentFTPEntry;
-(void) checkIfShouldAddFile:(NSString*)localPath fileName:(NSString*)fileName;
-(int) isAllowedFile:(NSString*)file;

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet RootViewControllerLocalBrowser *rootViewController;
@property (nonatomic, retain) IBOutlet SearchViewController *searchViewController;
@property (nonatomic, retain) IBOutlet OnlineViewController *onlineVC;
@property (nonatomic, retain) IBOutlet MoreViewController *moreVC;
@property (nonatomic, retain) IBOutlet UILabel *downloadLabelName,*downloadLabelSize;
@property (nonatomic, retain) IBOutlet UIProgressView *downloadPrgView;
@property (nonatomic, retain) IBOutlet UITabBarItem *barItem;
@property (nonatomic, retain) IBOutlet UITableView *downloadTabView;
@property (nonatomic, retain) IBOutlet BButton *btnCancel,*btnSuspend,*btnResume,*btnClear;

@property (nonatomic, retain) NSInputStream *networkStream;
@property (nonatomic, retain) NSOutputStream *fileStream;

@property int mFTPDownloadQueueDepth,mURLDownloadQueueDepth;

-(IBAction) goPlayer;
-(IBAction)cancelCurrent;
- (void)_stopReceiveWithStatus:(NSString *)statusString status:(int)status;

//-(void)play_listmodules:(t_playlist*)pl start_index:(int)index;
//-(void)play_listmodules:(NSArray *)array start_index:(int)index path:(NSArray *)arrayFilepaths;

@end
