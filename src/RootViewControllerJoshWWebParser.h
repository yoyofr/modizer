//
//  RootViewControllerJoshWWebParser.h
//  modizer1
//
//  Created by Yohann Magnien on 07/05/21.
//  Copyright __YoyoFR / Yohann Magnien__ 2021. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <sqlite3.h>
#import "DBHelper.h"
#import "RootViewControllerStruct.h"
#import "fex.h"
#import "CMPopTipView.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"

@class DetailViewControllerIphone;
@class DownloadViewController;


@interface RootViewControllerJoshWWebParser : UIViewController <UINavigationControllerDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
    NSString *ratingImg[3];
    UIView *infoMsgView;
    UILabel *infoMsgLbl;
    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
    
    WaitingView *waitingView;
    IBOutlet UITableView *tableView;
    
    IBOutlet UISearchBar *sBar;
    
    NSFileManager *mFileMngr;
    
    NSMutableArray *indexTitles;
    
    int shouldFillKeys;
    
    int mAccessoryButton;
    
    t_WEB_browse_entry *dbWEB_entries_data;
    int dbWEB_entries_count[27];
    t_WEB_browse_entry *dbWEB_entries[27];
    int dbWEB_nb_entries,dbWEB_hasFiles;
    t_WEB_browse_entry *search_dbWEB_entries_data;
    int search_dbWEB_entries_count[27];
    t_WEB_browse_entry *search_dbWEB_entries[27];
    int search_dbWEB;
    int search_dbWEB_nb_entries,search_dbWEB_hasFiles;
    
    NSString *rootDir;
    int mSearch;
    NSString *mSearchText;
    
    UIViewController *childController;
    
    NSString *FTPlocalPath,*FTPftpPath,*FTPfilename,*FTPfilePath;
    int FTPfilesize;
    
    bool darkMode;
    bool forceReloadCells;
    
    int mClickedPrimAction;    
@public
    int browse_depth;
    IBOutlet DetailViewControllerIphone *detailViewController;
    IBOutlet DownloadViewController *downloadViewController;
    NSString *mWebBaseURL;
    NSString *mWebBaseDir;
    char category;
    
}

@property (nonatomic, retain) NSFileManager *mFileMngr;

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet UIViewController *childController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;

@property (nonatomic, retain) NSString *mSearchText,*mWebBaseURL,*mWebBaseDir,*rootDir;
@property (nonatomic, retain) CMPopTipView *popTipView;

-(IBAction)goPlayer;

-(void) refreshViewAfterDownload;

-(void) checkCreate:(NSString *)filePath;

-(void) fillKeysWithRepoList;

-(void) fillKeysWithWEBSource;

-(void)updateMiniPlayer;

@end
