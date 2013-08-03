//
//  RootViewController.h
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#define SHOW_SUDIR_MIN_LEVEL 1

#define MENU_ROOTLEVEL 0
#define MENU_COLLECTIONS_ROOTLEVEL 0
#define MENU_BROWSER_LOCAL_ROOTLEVEL 1
#define MENU_COLLECTION_MODLAND_ROOTLEVEL 1
#define MENU_COLLECTIONS_MODLAND_FAF_1 2
#define MENU_COLLECTIONS_MODLAND_FAF_2 3

#import <UIKit/UIKit.h>
#import <sqlite3.h>
#import "DBHelper.h"
#import "RootViewControllerStruct.h"
#import "RootViewControllerLocalBrowser.h"
#import "fex.h"

@class EAGLContext;
@class DetailViewControllerIphone;
@class DownloadViewController;
@class WebBrowser;


@interface RootViewControllerIphone : UITableViewController <UISearchBarDelegate> {
	NSString *ratingImg[6];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
	
	
	UIView *waitingView;
	
    IBOutlet DetailViewControllerIphone *detailViewController;
    IBOutlet DownloadViewController *downloadViewController;
    IBOutlet WebBrowser *webBrowser;
    IBOutlet UITableView *tableView;

	IBOutlet UISearchBar *sBar;
	
	IBOutlet UIBarButtonItem *playerButton;
    
    NSFileManager *mFileMngr;

    
    NSMutableArray *list;
    NSMutableArray *keys;
	NSMutableArray *indexTitles,*indexTitlesDownload;
	
	int mNbFormatEntries,mNbAuthorEntries,mNbMODLANDFileEntries,mNbHVSCFileEntries;
	
	int mShowSubdir,shouldFillKeys;
    
    int mAccessoryButton;

	t_local_browse_entry *local_entries_data;
	int local_entries_count[27];
    t_local_browse_entry *local_entries[27];
	int local_nb_entries;
	
	t_local_browse_entry *search_local_entries_data;
	int search_local_entries_count[27];
    t_local_browse_entry *search_local_entries[27];
	int search_local_nb_entries;
	
	int search_local;
	
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
	
	t_db_browse_entry *db_entries_data;
	int db_entries_count[27];
	t_db_browse_entry *db_entries[27];
	int db_nb_entries,db_hasFiles;
	
	t_db_browse_entry *search_db_entries_data;
	int search_db_entries_count[27];
	t_db_browse_entry *search_db_entries[27];	
	int search_db;
	int search_db_nb_entries,search_db_hasFiles;

	NSString *currentPath;
	int browse_depth;
	int browse_mode; //1:local, 2:modland
	int modland_browse_mode;
	int mSearch;
	NSString *mSearchText;
	
	int mFiletypeID,mAuthorID,mAlbumID;
	
	UIViewController *childController;
	
	UIAlertView *alertAlreadyAvail;

	NSString *FTPlocalPath,*FTPftpPath,*FTPfilename,*FTPfilePath;
	int FTPfilesize;
	
	int mClickedPrimAction;
	int mCurrentWinAskedDownload;
}

@property (nonatomic, retain) NSFileManager *mFileMngr;

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet WebBrowser *webBrowser;
@property (nonatomic, retain) IBOutlet UIBarButtonItem *playerButton;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;
@property (nonatomic, retain) UIViewController *childController;


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

-(void) fillKeysWithDB_fileType:(int)authorID;
-(void) fillKeysWithDB_fileType;
-(void) fillKeysWithDB_fileAuthor;
-(void) fillKeysWithDB_fileAuthor:(int)filetypeID;
-(void) fillKeysWithDB_albumORfilename:(int)authorID;
-(void) fillKeysWithDB_albumORfilename:(int)filetypeID fileAuthorID:(int)authorID;
-(void) fillKeysWithDB_filename:(int)authorID fileAlbumID:(int)albumID;
-(void) fillKeysWithDB_filename:(int)filetypeID fileAuthorID:(int)authorID fileAlbumID:(int)albumID;
-(void) loadFavoritesList;
-(void) loadMostPlayedList;

-(void) fillKeysWithHVSCDB_Dir1;
-(void) fillKeysWithHVSCDB_Dir2:(NSString*)dir1;
-(void) fillKeysWithHVSCDB_Dir3:(NSString*)dir1 dir2:(NSString*)dir2;
-(void) fillKeysWithHVSCDB_Dir4:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3;
-(void) fillKeysWithHVSCDB_Dir5:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4;
-(void) fillKeysWithHVSCDB_AllDirs:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4 dir5:(NSString*)dir5;

-(void) getFileStatsDB:(NSString *)name fullpath:(NSString *)fullpath playcount:(short int*)playcount rating:(signed char*)rating;


- (void)createSamplesFromPackage:(BOOL)forceCreate;
-(NSString*) getCompleteLocalPath:(int)id_mod;
-(void)hideAllWaitingPopup;


@end
