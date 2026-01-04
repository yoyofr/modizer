//
//  RootViewController.h
//  modizer
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

#import "Notifications.h"

@class DetailViewControllerIphone;

@interface RootViewControllerPlaylist : UIViewController <UINavigationControllerDelegate,UISearchBarDelegate,UIActionSheetDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
	NSString *ratingImg[3];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    
	CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
	
	WaitingView *waitingView,*waitingViewExtract,*waitingViewPlayer;
    NSTimer *repeatingTimer;
    
    NSFileManager *mFileMngr;
	
    bool darkMode;
    bool forceReloadCells;

	IBOutlet UISearchBar *sBar;	
	
    NSMutableArray *list;
    NSMutableArray *keys;
    
    NSMutableDictionary *dictActionBtn;
	
	int mShowSubdir,shouldFillKeys;
    
    int mAccessoryButton;

	t_local_browse_entry *local_entries_data;
	int local_entries_count;
    t_local_browse_entry *local_entries;
	int local_nb_entries;
	
	t_local_browse_entry *search_local_entries_data;
	int search_local_entries_count;
    t_local_browse_entry *search_local_entries;
	int search_local_nb_entries;
	
	int search_local;
	
	
	int mRenamePlaylist;
	
	NSString *currentPath;
	int mSearch;
	NSString *mSearchText;
	
	UIViewController *childController;
	
	int mClickedPrimAction;
    
    NSProgress *extractProgress;
    int is_rsn;
    
    NSTimer *repeatTimer;
    int activeKey;
@public
    IBOutlet UITableView *tableView;

    t_playlist *playlist;
	int mFreePlaylist;
    int show_playlist;
    int browse_depth;
    int integrated_playlist;
    int mDetailPlayerMode;
    DetailViewControllerIphone *detailViewController;
    int currentPlayedEntry;
}

@property (nonatomic, retain) NSFileManager *mFileMngr;
@property (nonatomic, retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UIViewController *childController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;

@property (nonatomic, retain) NSProgress *extractProgress;

@property (nonatomic, retain) NSArray *list;
@property (nonatomic, retain) NSArray *keys;
@property (nonatomic, retain) NSString *currentPath,*mSearchText;

@property (nonatomic, retain) CMPopTipView *popTipView;
@property (nonatomic, retain) WaitingView *waitingView,*waitingViewExtract,*waitingViewPlayer;

@property (nonatomic, retain) NSTimer *repeatTimer;
@property (nonatomic, assign) int activeKey;

@property (nonatomic, strong) id mdzChangeObserverToken;

@property int mDetailPlayerMode,currentPlayedEntry;

-(IBAction)goPlayer;

- (void)freePlaylist;
-(void) refreshViewAfterDownload;
- (void)listLocalFiles;
- (void)checkCreate:(NSString *)filePath;


-(bool) removeFromPlaylistDB:(NSString*)id_playlist fullPath:(NSString*)fullpath;

-(void) loadFavoritesList:(t_playlist*)playlist;
-(void) loadMostPlayedList:(t_playlist*)playlist;
-(void) reloadNowPlaying;

-(int) loadLocalFilesRandomPL:(NSMutableArray*)labels fullpaths:(NSMutableArray*)fullpaths;


-(bool) addToPlaylistDB:(NSString*)id_playlist label:(NSString *)label fullPath:(NSString *)fullPath;
-(bool) addListToPlaylistDB;
-(NSString *) minitNewPlaylistDB:(NSString *)listName;

-(bool) addListToPlaylistDB:(NSString*)id_playlist entries:(t_plPlaylist_entry*)pl_entries nb_entries:(int)nb_entries;

-(void)updateMiniPlayer;

@end
