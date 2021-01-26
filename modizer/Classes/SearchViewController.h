//
//  SearchViewController.h
//  modizer4
//
//  Created by yoyofr on 7/16/10.
//  Copyright 2010 __YoyoFR / Yohann Magnien__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <sqlite3.h>

#import "AppDelegate_Phone.h"
#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "RootViewControllerLocalBrowser.h"
#import "CMPopTipView.h"

@class DetailViewControllerIphone;
@class DownloadViewController;
@class RootViewControllerLocalBrowser;

typedef struct {
	NSString *label;
	NSString *fullpath;
	int type; //0:dir, 1:file
} t_local_browse_entryS;

//Max MAX_PL_ENTRIES entry/playlist
typedef struct {
	NSString *label[MAX_PL_ENTRIES];
	NSString *fullpath[MAX_PL_ENTRIES];
	int nb_entries;
	NSString *playlist_name,*playlist_id;
} t_playlistS;

typedef struct {
	NSString *filename;
	NSString *fullpath;
	NSString *playlist_name,*playlist_id;
} t_playlist_entryS;

typedef struct {
	NSString *label;
	NSString *fullpath;
	int id_mod;
	int filesize;
	signed char  downloaded;
} t_db_browse_entryS;

typedef struct {
	NSString *label;
	NSString *dir1,*dir2,*dir3,*dir4,*dir5;
	NSString *fullpath;
	NSString *id_md5;
	signed char downloaded;
} t_dbHVSC_browse_entryS;

typedef struct {
	NSString *label;
	NSString *dir1,*dir2,*dir3,*dir4;
	NSString *fullpath;
	NSString *id_md5;
	signed char downloaded;
} t_dbASMA_browse_entryS;

@interface SearchViewController : UIViewController <UITableViewDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate>  {
	IBOutlet DetailViewControllerIphone *detailViewController;
	IBOutlet DownloadViewController *downloadViewController;
	IBOutlet RootViewControllerLocalBrowser *rootViewControllerIphone;
	IBOutlet UITableView *searchResultTabView;
	IBOutlet UISearchBar *sBar;
	IBOutlet UIView *searchPrgView;
	IBOutlet UILabel *searchLabel;
	IBOutlet UIProgressView *prgView;
    
    CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
	
	UIAlertView *alertDownload;
	NSString *mSearchText;
	int	mSearch;
	
	t_db_browse_entryS *db_entries;
	int db_entries_count;
	int modland_expanded,modland_searchOn;
	
	t_dbHVSC_browse_entryS *dbHVSC_entries;
	int dbHVSC_entries_count;
	int HVSC_expanded,HVSC_searchOn;
    
    t_dbASMA_browse_entryS *dbASMA_entries;
	int dbASMA_entries_count;
	int ASMA_expanded,ASMA_searchOn;
	
	t_local_browse_entryS *local_entries;
	int local_entries_count;
	int local_expanded,local_searchOn;
	
	t_playlist_entryS *playlist_entries;
	t_playlistS *playlist;
	int playlist_entries_count;
	int playlist_expanded,playlist_searchOn;
	
	
	int tooMuchDB,tooMuchPL,tooMuchLO,tooMuchDBHVSC,tooMuchDBASMA;

}

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet RootViewControllerLocalBrowser *rootViewControllerIphone;
@property (nonatomic, retain) IBOutlet UITableView *searchResultTabView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;
@property (nonatomic, retain) IBOutlet UIView *searchPrgView;
@property (nonatomic, retain) IBOutlet UILabel *searchLabel;
@property (nonatomic, retain) IBOutlet UIProgressView *prgView;
@property (nonatomic, retain) CMPopTipView *popTipView;

-(IBAction)goPlayer;

-(void) refreshViewAfterDownload;
-(void) doPrimAction:(NSIndexPath *)indexPath;
-(void) doSecAction:(NSIndexPath *)indexPath;

@end
