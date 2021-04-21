//
//  RootViewControllerASMA.h
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <sqlite3.h>
#import "DBHelper.h"
#import "RootViewControllerStruct.h"
#import "fex.h"
#import "CMPopTipView.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"

@class DetailViewControllerIphone;
@class DownloadViewController;


@interface RootViewControllerASMA : UIViewController <UINavigationControllerDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
	NSString *ratingImg[2];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
	
    CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
	
	WaitingView *waitingView;
    IBOutlet UITableView *tableView;

	IBOutlet UISearchBar *sBar;
	
    NSFileManager *mFileMngr;
    
    NSMutableArray *list;
    NSMutableArray *keys;
	NSMutableArray *indexTitles,*indexTitlesDownload;
	
	int mNbASMAFileEntries;
	
	int shouldFillKeys;
    
    int mAccessoryButton;

	t_dbHVSC_browse_entry *dbASMA_entries_data;
	int dbASMA_entries_count[27];
	t_dbHVSC_browse_entry *dbASMA_entries[27];
	int dbASMA_nb_entries,dbASMA_hasFiles;
	t_dbHVSC_browse_entry *search_dbASMA_entries_data;
	int search_dbASMA_entries_count[27];
	t_dbHVSC_browse_entry *search_dbASMA_entries[27];
	int search_dbASMA;
	int search_dbASMA_nb_entries,search_dbASMA_hasFiles;
		
	NSString *mDir1,*mDir2,*mDir3;
	
	NSString *currentPath;
	int mSearch;
	NSString *mSearchText;
	
	UIViewController *childController;
	
	NSString *FTPlocalPath,*FTPftpPath,*FTPfilename,*FTPfilePath;
	int FTPfilesize;
    
    bool darkMode;
    bool forceReloadCells;
	
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

-(IBAction)goPlayer;

-(void) refreshViewAfterDownload;
-(void)checkCreate:(NSString *)filePath;

-(NSString*) getCompletePath:(int)id_mod;

-(void) fillKeysWithASMADB_Dir1;
-(void) fillKeysWithASMADB_Dir2:(NSString*)dir1;
-(void) fillKeysWithASMADB_Dir3:(NSString*)dir1 dir2:(NSString*)dir2;
-(void) fillKeysWithASMADB_AllDirs:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3;

-(NSString*) getCompleteLocalPath:(int)id_mod;

-(void)updateMiniPlayer;

@end
