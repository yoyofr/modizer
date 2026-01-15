//
//  RootViewControllerCGSC.h
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
@class DownloadViewController;


@interface RootViewControllerCGSC : UIViewController <UINavigationControllerDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
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
    
    int mNbCGSCFileEntries;
    
    int shouldFillKeys;
    
    int mAccessoryButton;
    
    t_dbHVSC_browse_entry *dbCGSC_entries_data;
    int dbCGSC_entries_count;
    t_dbHVSC_browse_entry *dbCGSC_entries;
    int dbCGSC_nb_entries,dbCGSC_hasFiles;
    t_dbHVSC_browse_entry *search_dbCGSC_entries_data;
    int search_dbCGSC_entries_count;
    t_dbHVSC_browse_entry *search_dbCGSC_entries;
    int search_dbCGSC;
    int search_dbCGSC_nb_entries,search_dbCGSC_hasFiles;
    
    NSString *mDir1,*mDir2;
    
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
    NSTimer *repeatTimer;
    int activeKey;
@public
    int browse_depth;
    DetailViewControllerIphone *detailViewController;
    DownloadViewController *downloadViewController;
    
}

@property (nonatomic, strong) id mdzChangeObserverToken;

@property (nonatomic, retain) NSFileManager *mFileMngr;

@property (nonatomic, retain) DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet UIViewController *childController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;

@property (nonatomic, retain) NSString *currentPath,*mSearchText;
@property (nonatomic, retain) CMPopTipView *popTipView;
@property (nonatomic, retain) WaitingView *waitingView,*waitingViewPlayer;
@property (nonatomic, retain) NSTimer *repeatTimer,*updRSTimer;
@property (nonatomic, assign) int activeKey;
@property (nonatomic, assign) bool forceReloadCells;


-(IBAction)goPlayer;

-(void) refreshViewAfterDownload;
-(void)checkCreate:(NSString *)filePath;

-(void) fillKeysWithCGSCDB_Dir1;
-(void) fillKeysWithCGSCDB_Dir2:(NSString*)dir1;
-(void) fillKeysWithCGSCDB_AllDirs:(NSString*)dir1 dir2:(NSString*)dir2;

-(void)updateMiniPlayer;

@end
