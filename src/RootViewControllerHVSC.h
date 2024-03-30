//
//  RootViewControllerHVSC.h
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <sqlite3.h>
#import "DBHelper.h"
#import "RootViewControllerStruct.h"

#import "CMPopTipView.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"

@class DetailViewControllerIphone;
@class DownloadViewController;


@interface RootViewControllerHVSC : UIViewController <UINavigationControllerDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
	NSString *ratingImg[3];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
	
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
	
    WaitingView *waitingView,*waitingViewPlayer;
    NSTimer *repeatingTimer;
    
    IBOutlet UITableView *tableView;

	IBOutlet UISearchBar *sBar;
	
    NSFileManager *mFileMngr;
    
    NSMutableArray *list;
    NSMutableArray *keys;
	NSMutableArray *indexTitles,*indexTitlesDownload;
    
    NSMutableDictionary *dictActionBtn;
	
	int mNbHVSCFileEntries;
	
	int shouldFillKeys;
    
    int mAccessoryButton;

	t_dbHVSC_browse_entry *dbHVSC_entries_data;
	int dbHVSC_entries_count[27];
	t_dbHVSC_browse_entry *dbHVSC_entries[27];
	int dbHVSC_nb_entries,dbHVSC_hasFiles;
	t_dbHVSC_browse_entry *search_dbHVSC_entries_data;
	int search_dbHVSC_entries_count[27];
	t_dbHVSC_browse_entry *search_dbHVSC_entries[27];	
	int search_dbHVSC;
	int search_dbHVSC_nb_entries,search_dbHVSC_hasFiles;
		
	NSString *mDir1,*mDir2,*mDir3,*mDir4,*mDir5;
	
	NSString *currentPath;
	int mSearch;
	NSString *mSearchText;
	
	UIViewController *childController;
    
    bool darkMode;
    bool forceReloadCells;
	
	NSString *FTPlocalPath,*FTPftpPath,*FTPfilename,*FTPfilePath;
	int FTPfilesize;
	
	int mClickedPrimAction;
	int mCurrentWinAskedDownload;
@public    
    int browse_depth;
    IBOutlet DetailViewControllerIphone *detailViewController;	
    IBOutlet DownloadViewController *downloadViewController;

}

@property (nonatomic, retain) NSFileManager *mFileMngr;

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet UIViewController *childController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;

@property (nonatomic, retain) NSArray *list;
@property (nonatomic, retain) NSArray *keys;
@property (nonatomic, retain) NSString *currentPath,*mSearchText;
@property (nonatomic, retain) CMPopTipView *popTipView;
@property (nonatomic, retain) WaitingView *waitingView,*waitingViewPlayer;

-(IBAction)goPlayer;

-(void) refreshViewAfterDownload;
- (void)checkCreate:(NSString *)filePath;

-(NSString*) getCompletePath:(int)id_mod;

-(void) fillKeysWithHVSCDB_Dir1;
-(void) fillKeysWithHVSCDB_Dir2:(NSString*)dir1;
-(void) fillKeysWithHVSCDB_Dir3:(NSString*)dir1 dir2:(NSString*)dir2;
-(void) fillKeysWithHVSCDB_Dir4:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3;
-(void) fillKeysWithHVSCDB_Dir5:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4;
-(void) fillKeysWithHVSCDB_AllDirs:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4 dir5:(NSString*)dir5;

-(NSString*) getCompleteLocalPath:(int)id_mod;

-(void)updateMiniPlayer;

@end
