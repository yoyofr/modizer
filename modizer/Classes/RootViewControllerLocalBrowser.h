//
//  RootViewControllerLocalBrowser.h
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


@interface RootViewControllerLocalBrowser : UITableViewController <UISearchBarDelegate> {
	NSString *ratingImg[6];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
	
	
	UIView *waitingView;
	
    IBOutlet UITableView *tabView;

	IBOutlet UISearchBar *sBar;
    
    NSMutableArray *list;
    NSMutableArray *keys;
	NSMutableArray *indexTitles,*indexTitlesSpace;
		
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
		
	int mSlowDevice;
	
	NSString *currentPath;
	int mSearch;
	NSString *mSearchText;
	
	UIViewController *childController;
	

	int mClickedPrimAction;
	int mCurrentWinAskedDownload;
@public    
    int browse_depth;
    IBOutlet DetailViewControllerIphone *detailViewController;	
	IBOutlet UIBarButtonItem *playerButton;    	
}

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UIViewController *childController;
@property (nonatomic, retain) IBOutlet UIBarButtonItem *playerButton;
@property (nonatomic, retain) IBOutlet UITableView *tabView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;

@property (nonatomic, retain) NSArray *list;
@property (nonatomic, retain) NSArray *keys;
@property (nonatomic, retain) NSString *currentPath,*mSearchText;

-(IBAction)validatePlaylistName;
-(IBAction)cancelPlaylistName;
-(IBAction)goPlayer;

- (void)freePlaylist;
-(void) refreshMODLANDView;
-(int) deleteStatsDirDB:(NSString*)fullpath;
-(int) deleteStatsFileDB:(NSString*)fullpath;
- (void)listLocalFiles;
- (void)checkCreate:(NSString *)filePath;

-(NSString*) getCompletePath:(int)id_mod;
- (void)createEditableCopyOfDatabaseIfNeeded:(bool)forceInit quiet:(int)quiet;

-(bool) removeFromPlaylistDB:(NSString*)id_playlist fullPath:(NSString*)fullpath;

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
-(void) loadPlayListsListFromDB:(NSMutableArray*)entries list_id:(NSMutableArray*)list_id;

-(void) fillKeysWithHVSCDB_Dir1;
-(void) fillKeysWithHVSCDB_Dir2:(NSString*)dir1;
-(void) fillKeysWithHVSCDB_Dir3:(NSString*)dir1 dir2:(NSString*)dir2;
-(void) fillKeysWithHVSCDB_Dir4:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3;
-(void) fillKeysWithHVSCDB_Dir5:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4;
-(void) fillKeysWithHVSCDB_AllDirs:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4 dir5:(NSString*)dir5;

-(bool) addToPlaylistDB:(NSString*)id_playlist label:(NSString *)label fullPath:(NSString *)fullPath;
-(bool) addListToPlaylistDB;
-(NSString *) initNewPlaylistDB:(NSString *)listName;
-(void) getFileStatsDB:(NSString *)name fullpath:(NSString *)fullpath playcount:(short int*)playcount rating:(signed char*)rating;

-(bool) addListToPlaylistDB:(NSString*)id_playlist entries:(t_plPlaylist_entry*)pl_entries nb_entries:(int)nb_entries;

- (void)createSamplesFromPackage:(BOOL)forceCreate;
-(NSString*) getCompleteLocalPath:(int)id_mod;
-(void)hideAllWaitingPopup;


@end
