//
//  RootViewControllerMODLAND.h
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


@interface RootViewControllerMODLAND : UITableViewController <UISearchBarDelegate> {
	NSString *ratingImg[6];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
	
	
	UIView *waitingView;
	
    IBOutlet UITableView *tabView;

	IBOutlet UISearchBar *sBar;
	
    NSFileManager *mFileMngr;

    
    NSMutableArray *list;
    NSMutableArray *keys;
	NSMutableArray *indexTitles,*indexTitlesDownload;
	
	int mNbFormatEntries,mNbAuthorEntries,mNbMODLANDFileEntries;
	
	int shouldFillKeys;
    
    int mAccessoryButton;

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
@public    
    int browse_depth;
    IBOutlet DetailViewControllerIphone *detailViewController;	
	IBOutlet UIBarButtonItem *playerButton;    	
    IBOutlet DownloadViewController *downloadViewController;    
}

@property (nonatomic, retain) NSFileManager *mFileMngr;

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet UIViewController *childController;
@property (nonatomic, retain) IBOutlet UIBarButtonItem *playerButton;
@property (nonatomic, retain) IBOutlet UITableView *tabView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;

@property (nonatomic, retain) NSArray *list;
@property (nonatomic, retain) NSArray *keys;
@property (nonatomic, retain) NSString *currentPath,*mSearchText;

-(IBAction)goPlayer;

-(void) refreshMODLANDView;
-(int) deleteStatsDirDB:(NSString*)fullpath;
-(int) deleteStatsFileDB:(NSString*)fullpath;
-(void) listLocalFiles;
-(void) checkCreate:(NSString *)filePath;

-(NSString*) getCompletePath:(int)id_mod;


-(void) fillKeysWithDB_fileType:(int)authorID;
-(void) fillKeysWithDB_fileType;
-(void) fillKeysWithDB_fileAuthor;
-(void) fillKeysWithDB_fileAuthor:(int)filetypeID;
-(void) fillKeysWithDB_albumORfilename:(int)authorID;
-(void) fillKeysWithDB_albumORfilename:(int)filetypeID fileAuthorID:(int)authorID;
-(void) fillKeysWithDB_filename:(int)authorID fileAlbumID:(int)albumID;
-(void) fillKeysWithDB_filename:(int)filetypeID fileAuthorID:(int)authorID fileAlbumID:(int)albumID;

-(void) getFileStatsDB:(NSString *)name fullpath:(NSString *)fullpath playcount:(short int*)playcount rating:(signed char*)rating;

-(NSString*) getCompleteLocalPath:(int)id_mod;
-(void)hideAllWaitingPopup;


@end
