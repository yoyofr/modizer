//
//  RootViewControllerXPWebParser.h
//  modizer1
//
//  Created by Yohann Magnien on 10/03/24.
//  Copyright __YoyoFR / Yohann Magnien__ 2024. All rights reserved.
//

#define RATING_IMG(a) ( (a==5?2:(a?1:0)) )
#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define ROW_HEIGHT 40

#include <sys/types.h>
#include <sys/sysctl.h>

#include <pthread.h>
extern pthread_mutex_t db_mutex;

#include <stdlib.h>

#import "AppDelegate_Phone.h"

#import <UIKit/UIKit.h>
#import <sqlite3.h>
#import "DBHelper.h"
#import "RootViewControllerStruct.h"

#import "CMPopTipView.h"
#import "MiniPlayerVC.h"
#import "WaitingView.h"
#import "ImagesCache.h"
#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "WebBrowser.h"
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];
#import "TFHpple.h"
#import "CBAutoScrollLabel.h"
#import "QuartzCore/CAAnimation.h"
#import "TTFadeAnimator.h"
#import "AFNetworking.h"
#import "AFHTTPSessionManager.h"
#import "AFURLSessionManager.h"

@class DetailViewControllerIphone;
@class DownloadViewController;


@interface RootViewControllerXPWebParser : UIViewController <UINavigationControllerDelegate,UISearchBarDelegate,UIGestureRecognizerDelegate,CMPopTipViewDelegate> {
    NSString *ratingImg[3];
    UIView *infoMsgView;
    UILabel *infoMsgLbl;
    UILabel *navbarTitle;
    
    volatile int mPopupAnimation;


    
    MiniPlayerVC *miniplayerVC;
    bool wasMiniPlayerOn;
    
    ImagesCache *imagesCache;
    
    CMPopTipView *popTipView;
    int popTipViewRow,popTipViewSection;
    
    WaitingView *waitingView;
    IBOutlet UITableView *tableView;
    
    IBOutlet UISearchBar *sBar;
    
    NSFileManager *mFileMngr;
    
    NSMutableArray *indexTitles;
    
    NSMutableDictionary *dictActionBtn;
    
    int sort_mode;
    int browse_mode;
    int shouldFillKeys;
    
    int indexTitleMode;
    
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
    char category;
    
}

@property (nonatomic, retain) NSFileManager *mFileMngr;

@property (nonatomic, retain) IBOutlet DetailViewControllerIphone *detailViewController;
@property (nonatomic, retain) IBOutlet DownloadViewController *downloadViewController;
@property (nonatomic, retain) IBOutlet UIViewController *childController;
@property (nonatomic, retain) IBOutlet UITableView *tableView;
@property (nonatomic, retain) IBOutlet UISearchBar *sBar;

@property (nonatomic, retain) NSString *mSearchText,*mWebBaseURL,*rootDir;
@property (nonatomic, retain) CMPopTipView *popTipView;

-(IBAction)goPlayer;

-(void) refreshViewAfterDownload;

-(void) checkCreate:(NSString *)filePath;

-(void)updateMiniPlayer;

-(bool) searchStringRegExp:(NSString*)searchPattern sourceString:(NSString*)sourceString;
- (void)showMiniPlayer;
-(void) flushMainLoop;
-(void) updateWaitingDetail:(NSString *)text;
-(void) showAlertMsg:(NSString*)title message:(NSString*)message;
-(void) showWaiting;
-(void) hideWaiting;
- (void) addToPlaylistSelView:(NSString*)fullPath label:(NSString*)label showNowListening:(bool)showNL;
@end
