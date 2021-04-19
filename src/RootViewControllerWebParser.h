//
//  RootViewControllerWebParser.h
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


@interface RootViewControllerWebParser : UIViewController <UINavigationControllerDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
    
    NSString *downloadbaseDir;
    
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
	
	int mNbFileEntries;
	
	int shouldFillKeys;
    
    int mAccessoryButton;

    t_dbWEB_browse_entry *db_entries_data;
	int db_entries_count[27];
    t_dbWEB_browse_entry *db_entries[27];
	int db_nb_entries,db_hasFiles;
    t_dbWEB_browse_entry *search_db_entries_data;
	int search_db_entries_count[27];
    t_dbWEB_browse_entry *search_db_entries[27];
	int search_db;
	int search_db_nb_entries,search_db_hasFiles;
			
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

-(void) fillKeysWithDB;

-(NSString*) getCompleteLocalPath:(int)id_mod;

-(void)updateMiniPlayer;

@end
