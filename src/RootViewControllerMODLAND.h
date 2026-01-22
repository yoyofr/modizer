//
//  RootViewControllerMODLAND.h
//  modizer
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <sqlite3.h>
#import "DBHelper.h"
#import "RootViewControllerStruct.h"

#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "CMPopTipView.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"

#import "Notifications.h"

@interface RootViewControllerMODLAND : UIViewController <UINavigationControllerDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
	NSString *ratingImg[3];
	UIView *infoMsgView;
	UILabel *infoMsgLbl;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
	
	WaitingView *waitingView,*waitingViewPlayer;
    NSTimer *repeatingTimer,*updRSTimer;
	
    IBOutlet UITableView *tableView;

	IBOutlet UISearchBar *sBar;
    IBOutlet BButton *radioButton;
	
    NSFileManager *mFileMngr;

    
    NSMutableDictionary *dictActionBtn;
	
	int mNbFormatEntries,mNbAuthorEntries,mNbMODLANDFileEntries;
	
	int shouldFillKeys;
    
    int mAccessoryButton;

	t_db_browse_entry *db_entries_data;
	int db_entries_count;
	t_db_browse_entry *db_entries;
	int db_nb_entries,db_hasFiles;
	
	t_db_browse_entry *search_db_entries_data;
	int search_db_entries_count;
	t_db_browse_entry *search_db_entries;	
	int search_db;
	int search_db_nb_entries,search_db_hasFiles;

	NSString *currentPath;
	int modland_browse_mode;
	int mSearch;
	NSString *mSearchText;
    
    bool darkMode;
    bool forceReloadCells;
	
	int mFiletypeID,mAuthorID,mAlbumID;
	
	UIViewController *childController;
	
	NSString *FTPlocalPath,*FTPftpPath,*FTPfilename,*FTPfilePath;
	int FTPfilesize;
	
	int mClickedPrimAction;
	int mCurrentWinAskedDownload;
    NSTimer *repeatTimer;
    int activeKey;
@public
    int browse_depth;
    DetailViewControllerIphone *detailViewController;
    DownloadViewController *downloadViewController;
}

@property (nonatomic, retain) NSFileManager *mFileMngr;

@property (nonatomic, retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet UIViewController *childController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;
@property (nonatomic, retain) WaitingView *waitingView,*waitingViewPlayer;

@property (nonatomic, retain) NSString *currentPath,*mSearchText;
@property (nonatomic, retain) CMPopTipView *popTipView;

@property (nonatomic, retain) NSTimer *repeatTimer,*updRSTimer;
@property (nonatomic, assign) int activeKey;

@property (nonatomic, strong) id mdzChangeObserverToken;

@property (weak, nonatomic) IBOutlet NSLayoutConstraint *radioButtonWidthConstraint;


-(IBAction)goPlayer;

-(void) refreshViewAfterDownload;
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

-(NSString*) getCompleteLocalPath:(int)id_mod;

-(void)updateMiniPlayer;

@end
