//
//  RootViewController.h
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




@class DetailViewControllerIphone;

@interface RootViewControllerPlaylist : UIViewController <UISearchBarDelegate,UIActionSheetDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
	NSString *ratingImg[6];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
    
    
	CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
	
	UIView *waitingView;
    
    NSFileManager *mFileMngr;
	

	IBOutlet UISearchBar *sBar;	
	
    NSMutableArray *list;
    NSMutableArray *keys;
	NSMutableArray *indexTitles,*indexTitlesDownload;
	
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
	
	
	int mRenamePlaylist;
	
	int mSlowDevice;
	

	NSString *currentPath;
	int mSearch;
	NSString *mSearchText;
	
	UIViewController *childController;
	
	int mClickedPrimAction;
@public
    IBOutlet UITableView *tableView;

    t_playlist *playlist;
	int mFreePlaylist;
    int show_playlist;
    int browse_depth;
    int integrated_playlist;
    int mDetailPlayerMode;
    IBOutlet DetailViewControllerIphone *detailViewController;
    int currentPlayedEntry;
}

@property (nonatomic, retain) NSFileManager *mFileMngr;
@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet UIViewController *childController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;

@property (nonatomic, retain) NSArray *list;
@property (nonatomic, retain) NSArray *keys;
@property (nonatomic, retain) NSString *currentPath,*mSearchText;

@property (nonatomic, retain) CMPopTipView *popTipView;

@property int mDetailPlayerMode,currentPlayedEntry;

-(IBAction)goPlayer;

- (void)freePlaylist;
-(void) refreshViewAfterDownload;
- (void)listLocalFiles;
- (void)checkCreate:(NSString *)filePath;


-(bool) removeFromPlaylistDB:(NSString*)id_playlist fullPath:(NSString*)fullpath;

-(void) loadFavoritesList;
-(void) loadMostPlayedList;


-(bool) addToPlaylistDB:(NSString*)id_playlist label:(NSString *)label fullPath:(NSString *)fullPath;
-(bool) addListToPlaylistDB;
-(NSString *) initNewPlaylistDB:(NSString *)listName;

-(bool) addListToPlaylistDB:(NSString*)id_playlist entries:(t_plPlaylist_entry*)pl_entries nb_entries:(int)nb_entries;


@end
