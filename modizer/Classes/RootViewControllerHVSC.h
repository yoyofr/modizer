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
#import "fex.h"

@class DetailViewControllerIphone;
@class DownloadViewController;


@interface RootViewControllerHVSC : UIViewController <UISearchBarDelegate> {
	NSString *ratingImg[6];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
	
	
	UIView *waitingView;
    IBOutlet UITableView *tableView;

	IBOutlet UISearchBar *sBar;
	
    NSFileManager *mFileMngr;
    
    NSMutableArray *list;
    NSMutableArray *keys;
	NSMutableArray *indexTitles,*indexTitlesDownload;
	
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
	
	int mSlowDevice;
		
	NSString *currentPath;
	int mSearch;
	NSString *mSearchText;
	
	UIViewController *childController;
	
	UIAlertView *alertAlreadyAvail;

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

-(IBAction)goPlayer;

-(void) refreshMODLANDView;
-(int) deleteStatsDirDB:(NSString*)fullpath;
-(int) deleteStatsFileDB:(NSString*)fullpath;
- (void)listLocalFiles;
- (void)checkCreate:(NSString *)filePath;

-(NSString*) getCompletePath:(int)id_mod;
- (void)createEditableCopyOfDatabaseIfNeeded:(bool)forceInit quiet:(int)quiet;

-(bool) removeFromPlaylistDB:(NSString*)id_playlist fullPath:(NSString*)fullpath;

-(void) fillKeysWithHVSCDB_Dir1;
-(void) fillKeysWithHVSCDB_Dir2:(NSString*)dir1;
-(void) fillKeysWithHVSCDB_Dir3:(NSString*)dir1 dir2:(NSString*)dir2;
-(void) fillKeysWithHVSCDB_Dir4:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3;
-(void) fillKeysWithHVSCDB_Dir5:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4;
-(void) fillKeysWithHVSCDB_AllDirs:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4 dir5:(NSString*)dir5;

-(void) getFileStatsDB:(NSString *)name fullpath:(NSString *)fullpath playcount:(short int*)playcount rating:(signed char*)rating;

-(NSString*) getCompleteLocalPath:(int)id_mod;

@end
