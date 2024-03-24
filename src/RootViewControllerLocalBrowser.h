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

#import "archive.h"
#import "archive_entry.h"

#import "CMPopTipView.h"

#import "MiniPlayerVC.h"

#import "SESlideTableViewCell.h"

#import "WaitingView.h"

@class DetailViewControllerIphone;

@interface RootViewControllerLocalBrowser : UIViewController <UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate,SESlideTableViewCellDelegate,UINavigationControllerDelegate,UIScrollViewDelegate,NSFileManagerDelegate> {
	NSString *ratingImg[3];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
    NSFileManager *mFileMngr;
    
    //UIRefreshControl *refreshControl;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
    
	WaitingView *waitingView;
    
    NSProgress *extractProgress;
    
    UIAlertView *alertRename;
    int renameFile,renameSec,renameIdx;
    int createFolder;

	
	IBOutlet UISearchBar *sBar;
    IBOutlet UITableView *tableView;
    
    NSMutableArray *list;
    NSMutableArray *keys;
	NSMutableArray *indexTitles,*indexTitlesSpace;
    
    NSMutableDictionary *dictActionBtn;
		
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
		
	NSString *currentPath;
    bool icloud_folder_mode;
    int mSearch;
	NSString *mSearchText;
	
	UIViewController *childController;
	
    bool darkMode;
    bool forceReloadCells;
    
	int mClickedPrimAction;
	int mCurrentWinAskedDownload;
@public    
    int browse_depth;
    IBOutlet DetailViewControllerIphone *detailViewController;	
}

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) UIViewController *childController;
//@property (nonatomic, retain) UIRefreshControl *refreshControl;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;

@property (nonatomic, retain) NSProgress *extractProgress;

@property (nonatomic, retain) WaitingView *waitingView;

@property (nonatomic, retain) NSArray *list;
@property (nonatomic, retain) NSArray *keys;
@property (nonatomic, retain) NSString *currentPath,*mSearchText;

@property (nonatomic, retain) NSFileManager *mFileMngr;
@property (nonatomic, retain) CMPopTipView *popTipView;

@property (nonatomic, retain) UIAlertView *alertRename;

-(IBAction)goPlayer;

-(void) refreshViewReloadFiles;
-(void) refreshViewAfterDownload;

-(void)listLocalFiles;
-(void)createEditableCopyOfDatabaseIfNeeded:(bool)forceInit quiet:(int)quiet;
-(void)createSamplesFromPackage:(BOOL)forceCreate;

-(void)modizerIsLaunched;

-(void)updateMiniPlayer;

-(int) loadPlayListsListFromDB:(t_playlist_DB**)plList;
-(int) getFavoritesCountFromDB;
-(int) getMostPlayedCountFromDB;
-(int) loadFavoritesList:(NSMutableArray*)labels fullpaths:(NSMutableArray*)fullpaths;
-(int) loadMostPlayedList:(NSMutableArray*)labels fullpaths:(NSMutableArray*)fullpaths;
-(int) loadUserList:(int)pl_id  labels:(NSMutableArray*)labels fullpaths:(NSMutableArray*)fullpaths;
-(int) getLocalFilesCount;
-(int) loadLocalFilesRandomPL:(NSMutableArray*)labels fullpaths:(NSMutableArray*)fullpaths;

- (void) cancelPushed;

@end
