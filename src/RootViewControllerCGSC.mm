//
//  RootViewControllerCGSC.mm
//  modizer
//
//  Created by Yohann Magnien on 07/05/21.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//


#define RATING_IMG(a) ( (a==5?2:(a?1:0)) )


#define GET_NB_ENTRIES 1

#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define ROW_HEIGHT 40

#define LIMITED_LIST_SIZE 1024

#include <sys/types.h>
#include <sys/sysctl.h>

#include "gme.h"

#include "unzip.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;
static volatile int mPopupAnimation=0;

#import "AppDelegate_Phone.h"
#import "RootViewControllerCGSC.h"
#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "WebBrowser.h"
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];
#import "QuartzCore/CAAnimation.h"

#import "TTFadeAnimator.h"
#import "ModizFileHelper.h"

@implementation RootViewControllerCGSC

@synthesize mFileMngr;
@synthesize detailViewController;
@synthesize downloadViewController;
@synthesize tableView,sBar;
@synthesize currentPath;
@synthesize childController;
@synthesize mSearchText;
@synthesize popTipView;
@synthesize forceReloadCells;
@synthesize waitingView,waitingViewPlayer;
@synthesize repeatTimer,activeKey;

#pragma mark -
#pragma mark Search functions
#include "SearchCommonFunctions.h"

#pragma mark -
#pragma mark Miniplayer functions
#include "MiniPlayerImplementTableView.h"


/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////
#define HAS_DETAILVIEW_CONT
#include "PlaylistCommonFunctions.h"



-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer {
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    
    NSIndexPath *indexPath = [self.tableView indexPathForRowAtPoint:p];
    if (browse_depth>1) {
        if (indexPath != nil) {
            if ((gestureRecognizer.state==UIGestureRecognizerStateBegan)||(gestureRecognizer.state==UIGestureRecognizerStateChanged)) {
                int crow=indexPath.row;
                
                int download_all=0;
                if (search_dbCGSC) {
                    if (search_dbCGSC_hasFiles) download_all=1;
                } else {
                    if (dbCGSC_hasFiles) download_all=1;
                }
                
                crow-=download_all;
                
                    //display popup
                    t_dbHVSC_browse_entry *cur_db_entries;
                    cur_db_entries=(search_dbCGSC?search_dbCGSC_entries:dbCGSC_entries);
                    
                    NSString *str=cur_db_entries[crow].fullpath;
                    if (self.popTipView == nil) {
                        self.popTipView = [[CMPopTipView alloc] initWithMessage:str];
                        self.popTipView.delegate = self;
                        self.popTipView.backgroundColor = [UIColor lightGrayColor];
                        self.popTipView.textColor = [UIColor darkTextColor];
                        
                        [self.popTipView presentPointingAtView:[self.tableView cellForRowAtIndexPath:indexPath] inView:self.tableView animated:YES];
                        popTipViewRow=crow;
                        popTipViewSection=0;
                    } else {
                        if ((popTipViewRow!=crow)||(popTipViewSection!=0)||([str compare:self.popTipView.message]!=NSOrderedSame)) {
                            self.popTipView.message=str;
                            [self.popTipView presentPointingAtView:[self.tableView cellForRowAtIndexPath:indexPath] inView:self.tableView animated:YES];
                            popTipViewRow=crow;
                            popTipViewSection=0;
                        }
                    }
            } else {
                //hide popup
                if (popTipView!=nil) {
                    [self.popTipView dismissAnimated:YES];
                    popTipView=nil;
                }
            }
        }
    }
}

#pragma mark CMPopTipViewDelegate methods
- (void)popTipViewWasDismissedByUser:(CMPopTipView *)_popTipView {
    // User can tap CMPopTipView to dismiss it
    self.popTipView = nil;
}

- (id)findChildOfClass:(Class)cls inTabBarController:(UITabBarController *)tbc {
    for (UIViewController *vc in tbc.viewControllers) {
        // Unwrap nav controllers if present
        UIViewController *candidate = vc;
        if ([vc isKindOfClass:[UINavigationController class]]) {
            candidate = ((UINavigationController *)vc).viewControllers.firstObject;
        }
        if ([candidate isKindOfClass:cls]) {
            return candidate;
        }
    }
    return nil;
}



-(void) loadControllers {
    // With automatic storyboard loading, the window and root VC are created by UIKit.
    UIWindow *window=[UIApplication sharedApplication].windows.firstObject;
    if (!window) {
        // Fallback to keyWindow if needed
        window = [UIApplication sharedApplication].keyWindow;
    }
    UITabBarController *tbc = (UITabBarController *)window.rootViewController;
    if (![tbc isKindOfClass:[UITabBarController class]]) {
        MDZELog("[SceneDelegate] Unexpected root VC: %@", NSStringFromClass([window.rootViewController class]));
        return;
    }
    // Resolve specific child controllers
    if (!self.downloadViewController) self.downloadViewController = [self findChildOfClass:[DownloadViewController class] inTabBarController:tbc];
    if (!self.detailViewController) self.detailViewController = [self findChildOfClass:[DetailViewControllerIphone class] inTabBarController:tbc];
}


- (void)viewDidLoad {
    START_PROFILE
    childController=NULL;
    
    [self loadControllers];
    
    dictActionBtn=[NSMutableDictionary dictionaryWithCapacity:64];
    
    self.navigationController.delegate = self;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    mFileMngr=[[NSFileManager alloc] init];
    
    ratingImg[0] = @"heart-empty.png";
    ratingImg[1] = @"heart-half-filled.png";
    ratingImg[2] = @"heart-filled.png";
    
    /* Init popup view*/
    /**/
    
    //self.tableView.pagingEnabled;
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.tableView.sectionHeaderHeight = 18;
    //self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
    self.tableView.rowHeight = 40;
    //self.tableView.backgroundColor = [UIColor clearColor];
    //	self.tableView.backgroundColor = [UIColor blackColor];
    
    popTipViewRow=-1;
    popTipViewSection=-1;
    UILongPressGestureRecognizer *lpgr = [[UILongPressGestureRecognizer alloc]
                                          initWithTarget:self action:@selector(handleLongPress:)];
    lpgr.minimumPressDuration = 1.0; //seconds
    lpgr.delegate = self;
    [self.tableView addGestureRecognizer:lpgr];
    //[lpgr release];
    
    shouldFillKeys=1;
    mSearch=0;
    
    search_dbCGSC=0;  //reset to ensure search_dbCGSC is not used by default
    
    dbCGSC_entries=NULL;
    search_dbCGSC_entries=NULL;
    
    dbCGSC_nb_entries=0;
    search_dbCGSC_nb_entries=0;
    
    search_dbCGSC_hasFiles=0;
    dbCGSC_hasFiles=0;
    
    mSearchText=nil;
    mCurrentWinAskedDownload=0;
    mClickedPrimAction=0;
    
    if (browse_depth==0) {
#ifdef GET_NB_ENTRIES
        mNbCGSCFileEntries=DBHelper::getNbCGSCFilesEntries();
#else
        mNbCGSCFileEntries=NB_CGSC_ENTRIES;
#endif
    }
    
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithImage:[UIImage systemImageNamed:NOW_PLAYING_ICON] style:UIBarButtonItemStylePlain target:self action:@selector(goPlayer)];
    self.navigationItem.rightBarButtonItem = item;
    
    /////////////////////////////////////
    // Waiting view
    /////////////////////////////////////
    waitingView = [[WaitingView alloc] init];
    waitingView.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingView];
    waitingView.hidden=TRUE;
    
    NSDictionary *views = NSDictionaryOfVariableBindings(waitingView);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingView(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingView(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    waitingViewPlayer = [[WaitingView alloc] init];
    waitingViewPlayer.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingViewPlayer];
    waitingViewPlayer.hidden=TRUE;
    
    views = NSDictionaryOfVariableBindings(waitingViewPlayer);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingViewPlayer(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    [super viewDidLoad];
    
END_PROFILE
}

-(void) fillKeys {
    
    
    if (shouldFillKeys) {
        shouldFillKeys=0;
        if (browse_depth==1) [self fillKeysWithCGSCDB_Dir1];
        else if (browse_depth==2) [self fillKeysWithCGSCDB_Dir2:mDir1];
    } else { //reset downloaded, rating & playcount flags
        for (int i=0;i<dbCGSC_nb_entries;i++) {
            dbCGSC_entries_data[i].downloaded=-1;
            dbCGSC_entries_data[i].rating=-1;
            dbCGSC_entries_data[i].playcount=-1;
        }
    }
}

-(void) fillKeysWithCGSCDB_Dir1 {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int dbCGSC_entries_index;
    
    if (search_dbCGSC_nb_entries) {
            for (int j=0;j<search_dbCGSC_entries_count;j++) {
                search_dbCGSC_entries[j].label=nil;
                search_dbCGSC_entries[j].fullpath=nil;
                search_dbCGSC_entries[j].id_md5=nil;
                search_dbCGSC_entries[j].dir1=nil;
                search_dbCGSC_entries[j].dir2=nil;
            }
            search_dbCGSC_entries=NULL;
        search_dbCGSC_nb_entries=0;
        free(search_dbCGSC_entries_data);
    }
    
    
    dbCGSC_hasFiles=search_dbCGSC_hasFiles=0;
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_dbCGSC=1;
        
        search_dbCGSC_entries_data=(t_dbHVSC_browse_entry*)calloc(1,dbCGSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
        
            search_dbCGSC_entries_count=0;
            if (dbCGSC_entries_count) search_dbCGSC_entries=search_dbCGSC_entries_data;
            for (int j=0;j<dbCGSC_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:dbCGSC_entries[j].label]) {
                    search_dbCGSC_entries[search_dbCGSC_entries_count].label=dbCGSC_entries[j].label;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].downloaded=dbCGSC_entries[j].downloaded;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].rating=dbCGSC_entries[j].rating;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].playcount=dbCGSC_entries[j].playcount;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].filesize=dbCGSC_entries[j].filesize;
                    
                    search_dbCGSC_entries[search_dbCGSC_entries_count].id_md5=dbCGSC_entries[j].id_md5;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].fullpath=dbCGSC_entries[j].fullpath;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].dir1=dbCGSC_entries[j].dir1;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].dir2=dbCGSC_entries[j].dir2;
                    
                    search_dbCGSC_entries_count++;
                    search_dbCGSC_nb_entries++;
            }
        }
        return;
    }
    pthread_mutex_lock(&db_mutex);
    if (dbCGSC_nb_entries) {
        for (int i=0;i<dbCGSC_nb_entries;i++) {
            dbCGSC_entries_data[i].label=nil;
            dbCGSC_entries_data[i].fullpath=nil;
            dbCGSC_entries_data[i].id_md5=nil;
            dbCGSC_entries_data[i].dir1=nil;
            dbCGSC_entries_data[i].dir2=nil;
        }
        free(dbCGSC_entries_data);dbCGSC_entries_data=NULL;
        dbCGSC_nb_entries=0;
    }
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //1st : count how many entries we'll have
        if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir1) FROM cgsc_file WHERE dir1 LIKE \"%%%s%%\"",[mSearchText UTF8String]);
        else snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir1) FROM cgsc_file");
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                dbCGSC_nb_entries=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        if (dbCGSC_nb_entries) {
            //2nd initialize array to receive entries
            dbCGSC_entries_data=(t_dbHVSC_browse_entry *)calloc(1,dbCGSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
            memset(dbCGSC_entries_data,0,dbCGSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
            dbCGSC_entries_index=0;
                dbCGSC_entries_count=0;
                dbCGSC_entries=dbCGSC_entries_data;
            //3rd get the entries
            if (mSearch) snprintf(sqlStatement,1024,"SELECT dir1,COUNT(1) FROM cgsc_file WHERE dir1 LIKE \"%%%s%%\" GROUP BY dir1 ORDER BY dir1 COLLATE NOCASE",[mSearchText UTF8String]);
            else snprintf(sqlStatement,1024,"SELECT dir1,COUNT(1) FROM cgsc_file GROUP BY dir1 ORDER BY dir1 COLLATE NOCASE");
            
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    char *str=(char*)sqlite3_column_text(stmt, 0);
                    dbCGSC_entries[dbCGSC_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                    dbCGSC_entries[dbCGSC_entries_count].dir1=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                    dbCGSC_entries[dbCGSC_entries_count].filesize=sqlite3_column_int(stmt, 1);
                    
                    dbCGSC_entries[dbCGSC_entries_count].downloaded=-1;
                    dbCGSC_entries[dbCGSC_entries_count].rating=-1;
                    dbCGSC_entries[dbCGSC_entries_count].playcount=-1;
                    dbCGSC_entries_count++;
                    dbCGSC_entries_index++;
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
}
-(void) fillKeysWithCGSCDB_Dir2:(NSString*)dir1 {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int dbCGSC_entries_index;
    
    if (search_dbCGSC_nb_entries) {
            for (int j=0;j<search_dbCGSC_entries_count;j++) {
                search_dbCGSC_entries[j].label=nil;
                search_dbCGSC_entries[j].fullpath=nil;
                search_dbCGSC_entries[j].id_md5=nil;
                search_dbCGSC_entries[j].dir1=nil;
                search_dbCGSC_entries[j].dir2=nil;
            }
            search_dbCGSC_entries=NULL;
        search_dbCGSC_nb_entries=0;
        free(search_dbCGSC_entries_data);
    }
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_dbCGSC=1;
        
        search_dbCGSC_entries_data=(t_dbHVSC_browse_entry*)calloc(1,dbCGSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
        
        search_dbCGSC_hasFiles=0;
            search_dbCGSC_entries_count=0;
            if (dbCGSC_entries_count) search_dbCGSC_entries=search_dbCGSC_entries_data;
            for (int j=0;j<dbCGSC_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:dbCGSC_entries[j].label]) {
                    search_dbCGSC_entries[search_dbCGSC_entries_count].label=dbCGSC_entries[j].label;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].downloaded=dbCGSC_entries[j].downloaded;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].rating=dbCGSC_entries[j].rating;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].playcount=dbCGSC_entries[j].playcount;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].filesize=dbCGSC_entries[j].filesize;
                    
                    search_dbCGSC_entries[search_dbCGSC_entries_count].id_md5=dbCGSC_entries[j].id_md5;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].fullpath=dbCGSC_entries[j].fullpath;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].dir1=dbCGSC_entries[j].dir1;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].dir2=dbCGSC_entries[j].dir2;
                    
                    if (dbCGSC_entries[j].id_md5) search_dbCGSC_hasFiles++;
                    
                    search_dbCGSC_entries_count++;
                    search_dbCGSC_nb_entries++;
                }
            }
        return;
    }
    pthread_mutex_lock(&db_mutex);
    if (dbCGSC_nb_entries) {
        for (int i=0;i<dbCGSC_nb_entries;i++) {
            dbCGSC_entries_data[i].label=nil;
            dbCGSC_entries_data[i].fullpath=nil;
            dbCGSC_entries_data[i].id_md5=nil;
            dbCGSC_entries_data[i].dir1=nil;
            dbCGSC_entries_data[i].dir2=nil;
        }
        free(dbCGSC_entries_data);dbCGSC_entries_data=NULL;
        dbCGSC_nb_entries=0;
    }
    dbCGSC_hasFiles=0;
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //1st : count how many entries we'll have
        if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir2) FROM cgsc_file WHERE dir1=\"%s\" AND dir2 is not null AND dir2 LIKE \"%%%s%%\"\
                             UNION SELECT COUNT(filename) FROM cgsc_file WHERE dir1=\"%s\" AND dir2 is null AND filename LIKE \"%%%s%%\"",[dir1 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[mSearchText UTF8String]);
        else snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir2) FROM cgsc_file WHERE dir1=\"%s\" AND dir2 is not null\
                     UNION SELECT COUNT(filename) FROM cgsc_file WHERE dir1=\"%s\" AND dir2 is null",[dir1 UTF8String],[dir1 UTF8String]);
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                dbCGSC_nb_entries+=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        
        
        if (dbCGSC_nb_entries) {
            //2nd initialize array to receive entries
            dbCGSC_entries_data=(t_dbHVSC_browse_entry *)calloc(1,dbCGSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
            dbCGSC_entries_index=0;
                dbCGSC_entries_count=0;
                dbCGSC_entries=dbCGSC_entries_data;
            //3rd get the entries
            if (mSearch) snprintf(sqlStatement,1024,"SELECT dir2,NULL,NULL,COUNT(1),0 FROM cgsc_file WHERE dir1=\"%s\" AND dir2 is not null AND dir2 LIKE \"%%%s%%\" GROUP BY dir2 \
                                 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM cgsc_file WHERE dir1=\"%s\" AND dir2 is null AND filename LIKE \"%%%s%%\" \
                                 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[mSearchText UTF8String]);
            else snprintf(sqlStatement,1024,"SELECT dir2,NULL,NULL,COUNT(1),0 FROM cgsc_file WHERE dir1=\"%s\" AND dir2 is not null GROUP BY dir2\
                         UNION SELECT filename,fullpath,id_md5,NULL,1 FROM cgsc_file WHERE dir1=\"%s\" AND dir2 is null \
                         ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir1 UTF8String]);
            
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    char *str=(char*)sqlite3_column_text(stmt, 0);
                    dbCGSC_entries[dbCGSC_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                    dbCGSC_entries[dbCGSC_entries_count].dir1=[[NSString alloc] initWithString:dir1];
                    
                    if (sqlite3_column_int(stmt, 4)==0) {
                        dbCGSC_entries[dbCGSC_entries_count].dir2=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                        dbCGSC_entries[dbCGSC_entries_count].filesize=sqlite3_column_int(stmt, 3);
                    } else {
                        dbCGSC_entries[dbCGSC_entries_count].id_md5=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 2)];
                        dbCGSC_entries[dbCGSC_entries_count].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
                        dbCGSC_hasFiles++;
                    }
                    
                    dbCGSC_entries[dbCGSC_entries_count].downloaded=-1;
                    dbCGSC_entries[dbCGSC_entries_count].rating=-1;
                    dbCGSC_entries[dbCGSC_entries_count].playcount=-1;
                    dbCGSC_entries_count++;
                    dbCGSC_entries_index++;
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
            
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithCGSCDB_AllDirs:(NSString*)dir1 dir2:(NSString*)dir2 {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int dbCGSC_entries_index;
    
    if (search_dbCGSC_nb_entries) {
            for (int j=0;j<search_dbCGSC_entries_count;j++) {
                search_dbCGSC_entries[j].label=nil;
                search_dbCGSC_entries[j].fullpath=nil;
                search_dbCGSC_entries[j].id_md5=nil;
                search_dbCGSC_entries[j].dir1=nil;
                search_dbCGSC_entries[j].dir2=nil;
            }
            search_dbCGSC_entries=NULL;
        search_dbCGSC_nb_entries=0;
        free(search_dbCGSC_entries_data);
    }
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_dbCGSC=1;
        
        search_dbCGSC_entries_data=(t_dbHVSC_browse_entry*)calloc(1,dbCGSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
        search_dbCGSC_hasFiles=0;
            search_dbCGSC_entries_count=0;
            if (dbCGSC_entries_count) search_dbCGSC_entries=search_dbCGSC_entries_data;
            for (int j=0;j<dbCGSC_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:dbCGSC_entries[j].label]) {
                    search_dbCGSC_entries[search_dbCGSC_entries_count].label=dbCGSC_entries[j].label;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].downloaded=dbCGSC_entries[j].downloaded;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].rating=dbCGSC_entries[j].rating;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].playcount=dbCGSC_entries[j].playcount;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].filesize=dbCGSC_entries[j].filesize;
                    
                    search_dbCGSC_entries[search_dbCGSC_entries_count].id_md5=dbCGSC_entries[j].id_md5;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].fullpath=dbCGSC_entries[j].fullpath;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].dir1=dbCGSC_entries[j].dir1;
                    search_dbCGSC_entries[search_dbCGSC_entries_count].dir2=dbCGSC_entries[j].dir2;
                    
                    if (dbCGSC_entries[j].id_md5) search_dbCGSC_hasFiles++;
                    
                    search_dbCGSC_entries_count++;
                    search_dbCGSC_nb_entries++;
                }
        }
        return;
    }
    pthread_mutex_lock(&db_mutex);
    if (dbCGSC_nb_entries) {
        for (int i=0;i<dbCGSC_nb_entries;i++) {
            dbCGSC_entries_data[i].label=nil;
            dbCGSC_entries_data[i].fullpath=nil;
            dbCGSC_entries_data[i].id_md5=nil;
            dbCGSC_entries_data[i].dir1=nil;
            dbCGSC_entries_data[i].dir2=nil;
        }
        free(dbCGSC_entries_data);dbCGSC_entries_data=NULL;
        dbCGSC_nb_entries=0;
    }
    dbCGSC_hasFiles=0;
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        //1st : count how many entries we'll have
        if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(filename) FROM cgsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND filename LIKE \"%%%s%%\"",[dir1 UTF8String],[dir2 UTF8String],[mSearchText UTF8String]);
        else snprintf(sqlStatement,1024,"SELECT COUNT(filename) FROM cgsc_file WHERE dir1=\"%s\" AND dir2=\"%s\"",[dir1 UTF8String],[dir2 UTF8String]);
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                dbCGSC_nb_entries+=sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        
        
        if (dbCGSC_nb_entries) {
            //2nd initialize array to receive entries
            dbCGSC_entries_data=(t_dbHVSC_browse_entry *)calloc(1,dbCGSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
            
            dbCGSC_entries_index=0;
                dbCGSC_entries_count=0;
                dbCGSC_entries=dbCGSC_entries_data;
            //3rd get the entries
            if (mSearch) snprintf(sqlStatement,1024,"SELECT filename,fullpath,id_md5,1 FROM cgsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND filename LIKE \"%%%s%%\" \
                                 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[mSearchText UTF8String]);
            else snprintf(sqlStatement,1024,"SELECT filename,fullpath,id_md5,1 FROM cgsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" \
                         ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String]);
            
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    char *str=(char*)sqlite3_column_text(stmt, 0);
                    dbCGSC_entries[dbCGSC_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                    dbCGSC_entries[dbCGSC_entries_count].dir1=[[NSString alloc] initWithString:dir1];
                    dbCGSC_entries[dbCGSC_entries_count].dir2=[[NSString alloc] initWithString:dir2];
                    
                    dbCGSC_entries[dbCGSC_entries_count].id_md5=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 2)];
                    dbCGSC_entries[dbCGSC_entries_count].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
                    dbCGSC_hasFiles++;
                    
                    dbCGSC_entries[dbCGSC_entries_count].downloaded=-1;
                    dbCGSC_entries[dbCGSC_entries_count].rating=-1;
                    dbCGSC_entries[dbCGSC_entries_count].playcount=-1;
                    dbCGSC_entries_count++;
                    dbCGSC_entries_index++;
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
            
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
}

-(void) traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    [self.tableView reloadData];
}

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleDefault;
}

-(void) viewWillAppear:(BOOL)animated {
//    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    [self.sBar setBarStyle:UIBarStyleDefault];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    
    self.navigationController.delegate = self;
    
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    
    if ([detailViewController mPlaylist_size]>0) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    } else {
        wasMiniPlayerOn=false;
        [self hideMiniPlayer];
    }
    
    [waitingViewPlayer resetCancelStatus];
    waitingViewPlayer.hidden=detailViewController.waitingView.hidden;
    waitingViewPlayer.btnStopCurrentAction.hidden=detailViewController.waitingView.btnStopCurrentAction.hidden;
    waitingViewPlayer.progressView.progress=detailViewController.waitingView.progressView.progress;
    waitingViewPlayer.progressView.hidden=detailViewController.waitingView.progressView.hidden;
    waitingViewPlayer.lblTitle.text=[NSString stringWithString:detailViewController.waitingView.lblTitle.text];
    waitingViewPlayer.lblDetail.text=[NSString stringWithString:detailViewController.waitingView.lblDetail.text];
    
    //    [waitingViewPlayer.progressView setObservedProgress:detailViewController.mplayer.extractProgress];
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView addObserver:self
                                       forKeyPath:observedSelector
                                          options:NSKeyValueObservingOptionInitial
                                          context:LoadingProgressObserverContext];
    
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.20f target:self selector:@selector(updateLoadingInfos:) userInfo:nil repeats: YES]; //5 times/second
    
    if (childController) {
        //[childController release];
        childController = NULL;
    }
    
    //Reset rating if applicable (ensure updated value)
    if (dbCGSC_nb_entries) {
        for (int i=0;i<dbCGSC_nb_entries;i++) {
            dbCGSC_entries_data[i].rating=-1;
        }
    }
    if (search_dbCGSC_nb_entries) {
        for (int i=0;i<search_dbCGSC_nb_entries;i++) {
            search_dbCGSC_entries_data[i].rating=-1;
        }
    }
    /////////////
    
    if (shouldFillKeys&&(browse_depth>0)) {
        
        [self showWaiting];
        [self flushMainLoop];
        
        [self fillKeys];
        [tableView reloadData];
        [self hideWaiting];
    } else {
        [self fillKeys];
        [tableView reloadData];
    }
    
    self.mdzChangeObserverToken =
        [[NSNotificationCenter defaultCenter] addObserverForName:MDZFileStatsChangedNotification
                                                          object:nil
                                                           queue:[NSOperationQueue mainQueue]
                                                      usingBlock:^(NSNotification * _Nonnull note) {
        // Consommer la notification
        NSDictionary *info = note.userInfo;
//        NSString *fileName = info[@"fileName"];
//        NSString *filePath = info[@"filePath"];
        // Mets à jour l’UI / ton modèle
        //self.forceReloadCells=true;
        [self fillKeys];
        [self.tableView reloadData];
    }];
    
    [super viewWillAppear:animated];
    
}
-(void) refreshViewAfterDownload {
    if (childController) [(RootViewControllerCGSC*)childController refreshViewAfterDownload];
    else {
        shouldFillKeys=1;
        [self fillKeys];
        [tableView reloadData];
    }
}

- (void)checkCreate:(NSString *)filePath {
    NSString *completePath=[NSString stringWithFormat:@"%@/%@",[ModizFileHelper getAppHomeDirectory],filePath];
    NSError *err;
    [mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:&err];
}

- (void)viewDidAppear:(BOOL)animated {
    [self hideWaiting];
    
    [super viewDidAppear:animated];
    
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}

- (void)viewDidDisappear:(BOOL)animated {
    [self hideWaiting];
    
    [repeatingTimer invalidate];
    repeatingTimer = nil;
    
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView removeObserver:self
                                          forKeyPath:observedSelector
                                             context:LoadingProgressObserverContext];
    
    if (self.mdzChangeObserverToken) {
        [[NSNotificationCenter defaultCenter] removeObserver:self.mdzChangeObserverToken];
        self.mdzChangeObserverToken = nil;
    }
    
    [super viewDidDisappear:animated];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    [self.tableView reloadData];
    [miniplayerVC viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}


- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
    [tableView reloadData];
}

// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    [tableView reloadData];
    return YES;
}

#pragma mark -
#pragma mark Table view data source

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return nil;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (search_dbCGSC) {
        return search_dbCGSC_entries_count+(search_dbCGSC_hasFiles?1:0);
    } else {
        return dbCGSC_entries_count+(dbCGSC_hasFiles?1:0);
    }
    return 0;
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    return nil;
}

- (NSInteger)tableView:(UITableView *)tabView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    return -1;
}

- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    const NSInteger BOTTOM_IMAGE_TAG = 1003;
    const NSInteger ACT_IMAGE_TAG = 1004;
    const NSInteger SECACT_IMAGE_TAG = 1005;
    UILabel *topLabel;
    UILabel *bottomLabel;
    UIImageView *bottomImageView;
    UIButton *actionView,*secActionView;
    NSString *nbFiles=NSLocalizedString(@"%d files.",@"");
    NSString *nb1File=NSLocalizedString(@"1 file.",@"");
    
    if (forceReloadCells) {
        while ([tableView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        forceReloadCells=false;
    }
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
        
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,40);
        [cell setBackgroundColor:[UIColor clearColor]];
        
        /*CAGradientLayer *gradient = [CAGradientLayer layer];
         gradient.frame = cell.bounds;
         gradient.colors = [NSArray arrayWithObjects:
         (id)[[UIColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1] CGColor],
         (id)[[UIColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1] CGColor],
         (id)[[UIColor colorWithRed:235.0/255.0 green:235.0/255.0 blue:235.0/255.0 alpha:1] CGColor],
         (id)[[UIColor colorWithRed:240.0/255.0 green:240.0/255.0 blue:240.0/255.0 alpha:1] CGColor],
         (id)[[UIColor colorWithRed:200.0/255.0 green:200.0/255.0 blue:200.0/255.0 alpha:1] CGColor],
         (id)[[UIColor colorWithRed:200.0/255.0 green:200.0/255.0 blue:200.0/255.0 alpha:1] CGColor],
         nil];
         gradient.locations = [NSArray arrayWithObjects:
         (id)[NSNumber numberWithFloat:0.00f],
         (id)[NSNumber numberWithFloat:0.03f],
         (id)[NSNumber numberWithFloat:0.03f],
         (id)[NSNumber numberWithFloat:0.97f],
         (id)[NSNumber numberWithFloat:0.97f],
         (id)[NSNumber numberWithFloat:1.00f],
         nil];
         [cell setBackgroundView:[[UIView alloc] init]];
         [cell.backgroundView.layer insertSublayer:gradient atIndex:0];
         
         CAGradientLayer *selgrad = [CAGradientLayer layer];
         selgrad.frame = cell.bounds;
         float rev_col_adj=1.2f;
         selgrad.colors = [NSArray arrayWithObjects:
         (id)[[UIColor colorWithRed:rev_col_adj-255.0/255.0 green:rev_col_adj-255.0/255.0 blue:rev_col_adj-255.0/255.0 alpha:1] CGColor],
         (id)[[UIColor colorWithRed:rev_col_adj-255.0/255.0 green:rev_col_adj-255.0/255.0 blue:rev_col_adj-255.0/255.0 alpha:1] CGColor],
         (id)[[UIColor colorWithRed:rev_col_adj-235.0/255.0 green:rev_col_adj-235.0/255.0 blue:rev_col_adj-235.0/255.0 alpha:1] CGColor],
         (id)[[UIColor colorWithRed:rev_col_adj-240.0/255.0 green:rev_col_adj-240.0/255.0 blue:rev_col_adj-240.0/255.0 alpha:1] CGColor],
         (id)[[UIColor colorWithRed:rev_col_adj-200.0/255.0 green:rev_col_adj-200.0/255.0 blue:rev_col_adj-200.0/255.0 alpha:1] CGColor],
         (id)[[UIColor colorWithRed:rev_col_adj-200.0/255.0 green:rev_col_adj-200.0/255.0 blue:rev_col_adj-200.0/255.0 alpha:1] CGColor],
         nil];
         selgrad.locations = [NSArray arrayWithObjects:
         (id)[NSNumber numberWithFloat:0.00f],
         (id)[NSNumber numberWithFloat:0.03f],
         (id)[NSNumber numberWithFloat:0.03f],
         (id)[NSNumber numberWithFloat:0.97f],
         (id)[NSNumber numberWithFloat:0.97f],
         (id)[NSNumber numberWithFloat:1.00f],
         nil];
         
         [cell setSelectedBackgroundView:[[UIView alloc] init]];
         [cell.selectedBackgroundView.layer insertSublayer:selgrad atIndex:0];
         */
        NSString *imgFile=(darkMode?@"tabview_gradient40Black.png":@"tabview_gradient40.png");
        UIImage *image = [UIImage imageNamed:imgFile];
        
        UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;
        //[imageView release];
        
        //
        // Create the label for the top row of text
        //
        topLabel = [[UILabel alloc] init];
        [cell.contentView addSubview:topLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        topLabel.tag = TOP_LABEL_TAG;
        topLabel.backgroundColor = [UIColor clearColor];
        topLabel.font = [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;;
        topLabel.opaque=TRUE;
        
        //
        // Create the label for the top row of text
        //
        bottomLabel = [[UILabel alloc] init];
        [cell.contentView addSubview:bottomLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        bottomLabel.tag = BOTTOM_LABEL_TAG;
        bottomLabel.backgroundColor = [UIColor clearColor];
        bottomLabel.font = [UIFont systemFontOfSize:12];
        //bottomLabel.font = [UIFont fontWithName:@"courier" size:12];
        bottomLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                   ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);;;
        bottomLabel.opaque=TRUE;
        
        bottomImageView = [[UIImageView alloc] initWithImage:nil];
        bottomImageView.frame = CGRectMake(1.0*cell.indentationWidth,
                                           22,
                                           14,14);
        bottomImageView.tag = BOTTOM_IMAGE_TAG;
        bottomImageView.opaque=TRUE;
        [cell.contentView addSubview:bottomImageView];
        
        actionView                = [UIButton buttonWithType: UIButtonTypeCustom];
        [cell.contentView addSubview:actionView];
        actionView.tag = ACT_IMAGE_TAG;
        
        secActionView                = [UIButton buttonWithType: UIButtonTypeCustom];
        [cell.contentView addSubview:secActionView];
        secActionView.tag = SECACT_IMAGE_TAG;
        
        cell.accessoryView=nil;
        //cell.selectionStyle=UITableViewCellSelectionStyleGray;
    } else {
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        bottomImageView = (UIImageView *)[cell viewWithTag:BOTTOM_IMAGE_TAG];
        actionView = (UIButton *)[cell viewWithTag:ACT_IMAGE_TAG];
        secActionView = (UIButton *)[cell viewWithTag:SECACT_IMAGE_TAG];
        
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
        
        bottomLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
    }
    actionView.hidden=TRUE;
    secActionView.hidden=TRUE;
    
    if (darkMode) {
        topLabel.textColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:1 green:1 blue:1 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.6 green:0.6 blue:0.6 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0];
    }
    
    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                               0,
                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                               22);
    bottomLabel.frame = CGRectMake(1.0 * cell.indentationWidth,
                                   22,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32,
                                   18);
    bottomLabel.text=@""; //default value
    bottomImageView.image=nil;
    
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    // Set up the cell...
    if (browse_depth==0) {
    } else { //CGSC
        t_dbHVSC_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbCGSC?search_dbCGSC_entries:dbCGSC_entries);
        int section = indexPath.section-1;
        int download_all=0;
        if (search_dbCGSC) {
            if (search_dbCGSC_hasFiles) download_all=1;
        } else {
            if (dbCGSC_hasFiles) download_all=1;
        }
        
        int crow=indexPath.row;
        if (download_all) crow--;
        
        if (download_all&&(crow==-1)) {
            cellValue=NSLocalizedString(@"GetAllEntries_MainKey","");;
            topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
            bottomLabel.text=NSLocalizedString(@"GetAllEntries_SubKey","");
        } else {
            cellValue=cur_db_entries[crow].label;
            int colFactor;
            //update downloaded if needed
            if(cur_db_entries[crow].downloaded==-1) {
                NSString *pathToCheck=nil;
                
                if (cur_db_entries[crow].fullpath)
                    pathToCheck=[NSString stringWithFormat:@"%@/Documents/%@%@",[ModizFileHelper getAppHomeDirectory],CGSC_BASEDIR,cur_db_entries[crow].fullpath];
                if (pathToCheck) {
                    if ([mFileMngr fileExistsAtPath:pathToCheck]) cur_db_entries[crow].downloaded=1;
                    else cur_db_entries[crow].downloaded=0;
                } else cur_db_entries[crow].downloaded=0;
            }
            
            if(cur_db_entries[crow].downloaded==1) {
                colFactor=1;
            } else colFactor=0;
            
            if (cur_db_entries[crow].id_md5) { //FILE
                if (colFactor==0) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0];
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                           22);
                if (cur_db_entries[crow].downloaded==1) {
                    if (cur_db_entries[crow].rating==-1) {
                        DBHelper::getFileStatsDBmod([NSString stringWithFormat:@"Documents/%@%@",CGSC_BASEDIR,cur_db_entries[crow].fullpath],
                                                    &cur_db_entries[crow].playcount,
                                                    &cur_db_entries[crow].rating,
                                                    NULL,
                                                    &cur_db_entries[crow].song_length,
                                                    &cur_db_entries[crow].channels_nb,
                                                    &cur_db_entries[crow].songs);
                    }
                    if (cur_db_entries[crow].rating>0) bottomImageView.image=[UIImage imageNamed:ratingImg[RATING_IMG(cur_db_entries[crow].rating)]];
                    
                    NSString *bottomStr;
                    if (cur_db_entries[crow].song_length>0)
                        bottomStr=[NSString stringWithFormat:@"%02d:%02d",cur_db_entries[crow].song_length/1000/60,(cur_db_entries[crow].song_length/1000)%60];
                    else bottomStr=@"--:--";
                    if (cur_db_entries[crow].channels_nb)
                        bottomStr=[NSString stringWithFormat:@"%@|%02dch",bottomStr,cur_db_entries[crow].channels_nb];
                    else bottomStr=[NSString stringWithFormat:@"%@|--ch",bottomStr];
                    if (cur_db_entries[crow].songs) {
                        if (cur_db_entries[crow].songs==1) bottomStr=[NSString stringWithFormat:@"%@|1 song",bottomStr];
                        else bottomStr=[NSString stringWithFormat:@"%@|%d songs",bottomStr,cur_db_entries[crow].songs];
                    }
                    else bottomStr=[NSString stringWithFormat:@"%@|- song",bottomStr];
                    bottomStr=[NSString stringWithFormat:@"%@|Pl:%d",bottomStr,cur_db_entries[crow].playcount];
                    
                    bottomLabel.text=bottomStr;
                    
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+20,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-20,
                                                   18);
                } else {
                }
                
                if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                    [actionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateNormal];
                    [actionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateHighlighted];
                    [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                    [actionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                    [dictActionBtn setObject:[NSNumber numberWithInteger:crow*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
                } else {
                    [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateNormal];
                    [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateHighlighted];
                    [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                    [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                    [dictActionBtn setObject:[NSNumber numberWithInteger:crow*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
                }
                actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                actionView.enabled=YES;
                actionView.hidden=NO;
                
            } else { // DIR
                bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                               22,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                               18);
                bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[crow].filesize>1?nbFiles:nb1File),cur_db_entries[crow].filesize];
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                                           22);
                if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:1.0f alpha:1.0f];
                else topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
                
                cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            }
        }
    }
    topLabel.text = cellValue;
    
    return cell;
}

- (UISwipeActionsConfiguration *)tableView:(UITableView *)tableView
trailingSwipeActionsConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath {
    // DELETE ACTION
    UIContextualAction *deleteAction =
    [UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive
                                            title:NSLocalizedString(@"Delete", @"")
                                          handler:^(UIContextualAction *action,
                                                    UIView *sourceView,
                                                    void (^completionHandler)(BOOL)) {

        t_dbHVSC_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbCGSC?search_dbCGSC_entries:dbCGSC_entries);
        
        int crow=indexPath.row;
        if (search_dbCGSC) {
            if (search_dbCGSC_hasFiles) crow--;
        } else {
            if (dbCGSC_hasFiles) crow--;
        }

        //delete file
        NSString *fullpath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents/%@%@",CGSC_BASEDIR,cur_db_entries[crow].fullpath]];
        NSError *err;
        DBHelper::deleteStatsFileDB(fullpath);
        cur_db_entries[crow].downloaded=0;

        //delete local file
        [mFileMngr removeItemAtPath:fullpath error:&err];

        // Reload the cell to show the file is no longer downloaded
        [tableView reloadRowsAtIndexPaths:@[indexPath]
                         withRowAnimation:UITableViewRowAnimationAutomatic];

        completionHandler(YES);
    }];
    deleteAction.backgroundColor = [UIColor redColor];

    // Return multiple actions - they appear from right to left
    return [UISwipeActionsConfiguration configurationWithActions:@[deleteAction]];
}


- (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    t_dbHVSC_browse_entry *cur_db_entries=(search_dbCGSC?search_dbCGSC_entries:dbCGSC_entries);
    int crow=indexPath.row;
    if (search_dbCGSC) {
        if (search_dbCGSC_hasFiles) crow--;
    } else {
        if (dbCGSC_hasFiles) crow--;
    }
    if (cur_db_entries&&(crow>=0)) {
        if (cur_db_entries[crow].downloaded==1) return YES;
    }
    return NO;
}

#pragma mark UISearchBarDelegate
- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar {
    // only show the status bar’s cancel button while in edit mode
    sBar.showsCancelButton = YES;
    sBar.autocorrectionType = UITextAutocorrectionTypeNo;
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    
    // flush the previous search content
    //[tableData removeAllObjects];
}
- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar {
    //[self fillKeys];
    //[tableView reloadData];
    //mSearch=0;
    sBar.showsCancelButton = NO;
}
- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
    //if (mSearchText) [mSearchText release];
    
    mSearchText=[[NSString alloc] initWithString:searchText];
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    shouldFillKeys=1;
    search_dbCGSC=0;
    [self fillKeys];
    [tableView reloadData];
}
- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
    //if (mSearchText) [mSearchText release];
    mSearchText=nil;
    sBar.text=nil;
    mSearch=0;
    sBar.showsCancelButton = NO;
    [searchBar resignFirstResponder];
    shouldFillKeys=1;
    search_dbCGSC=0;
    [self fillKeys];
    
    [tableView reloadData];
}
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar {
    [searchBar resignFirstResponder];
}

- (void)moveCursorOnce {
    UITextField *tf = self.sBar.searchTextField;
    UITextPosition *pos = tf.selectedTextRange.start;

    NSInteger offset = (self.activeKey == UIKeyboardHIDUsageKeyboardLeftArrow) ? -1 : 1;
    UITextPosition *newPos = [tf positionFromPosition:pos offset:offset];

    if (newPos) {
        tf.selectedTextRange = [tf textRangeFromPosition:newPos toPosition:newPos];
    }
}


- (void)startRepeating {
    [self.repeatTimer invalidate];
    self.repeatTimer = [NSTimer scheduledTimerWithTimeInterval:0.05
                                                        target:self
                                                      selector:@selector(moveCursorOnce)
                                                      userInfo:nil
                                                       repeats:YES];
}


- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    for (UIPress *press in presses) {
        UIKey *key = press.key;
        if (!key) continue;

        if (key.keyCode == UIKeyboardHIDUsageKeyboardLeftArrow ||
            key.keyCode == UIKeyboardHIDUsageKeyboardRightArrow) {

            self.activeKey = key.keyCode;
            [self moveCursorOnce];

            // délai initial macOS (~0.45s)
            self.repeatTimer = [NSTimer scheduledTimerWithTimeInterval:0.45
                                                                target:self
                                                              selector:@selector(startRepeating)
                                                              userInfo:nil
                                                               repeats:NO];
            return;
        }
    }
    [super pressesBegan:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    [self.repeatTimer invalidate];
    self.repeatTimer = nil;
    self.activeKey = 0;
    [super pressesEnded:presses withEvent:event];
}

- (void)pressesCancelled:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event {
    [self.repeatTimer invalidate];
    self.repeatTimer = nil;
    self.activeKey = 0;
    [super pressesCancelled:presses withEvent:event];
}


-(IBAction)goPlayer {
    if (detailViewController.mPlaylist_size) {
        if (detailViewController) {
            @try {
                [self.navigationController pushViewController:detailViewController animated:YES];
            } @catch (NSException * ex) {
                //“Pushing the same view controller instance more than once is not supported”
                //NSInvalidArgumentException
                MDZELog("Exception: [%@]:%@",[ex  class], ex );
                MDZELog("ex.name:'%@'", ex.name);
                MDZELog("ex.reason:'%@'", ex.reason);
                //Full error includes class pointer address so only care if it starts with this error
                NSRange range = [ex.reason rangeOfString:@"Pushing the same view controller instance more than once is not supported"];
                
                if ([ex.name isEqualToString:@"NSInvalidArgumentException"] &&
                    range.location != NSNotFound) {
                    //view controller already exists in the stack - just pop back to it
                    [self.navigationController popToViewController:detailViewController animated:YES];
                } else {
                    MDZELog("ERROR:UNHANDLED EXCEPTION TYPE:%@", ex);
                }
            } @finally {
            }
        } else {
            MDZELog("ERROR:pushViewController: viewController is nil");
        }
    }
    else {
        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"")];
    }
}

#pragma mark -
#pragma mark Table view delegate
- (void) primaryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    
    if (browse_depth==0) {
        
    } else {
        t_dbHVSC_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbCGSC?search_dbCGSC_entries:dbCGSC_entries);
        int download_all=0;
        if (search_dbCGSC) {
            if (search_dbCGSC_hasFiles) download_all=1;
        } else {
            if (dbCGSC_hasFiles) download_all=1;
        }
        
        int crow=indexPath.row;
        //if (download_all) crow--;
        
        if (crow>=0) {
            if (cur_db_entries[crow].id_md5) { //FILE
                //File selected, start download is needed
                NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[crow].label];
                NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[crow].fullpath];
                NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",CGSC_BASEDIR,cur_db_entries[crow].fullpath];
                
                if (cur_db_entries[crow].downloaded==1) {
                    NSMutableArray *array_label = [[NSMutableArray alloc] init];
                    NSMutableArray *array_path = [[NSMutableArray alloc] init];
                    [array_label addObject:sidFilename];
                    [array_path addObject:localPath];
                    cur_db_entries[crow].rating=-1;
                    [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                    if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                    
                    [tableView reloadData];
                } else {
                    [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                    mCurrentWinAskedDownload=1;
                    
                    NSString *cgsc_url=[NSString stringWithFormat:@"%s",settings[ONLINE_CGSC_CURRENT_URL].detail.mdz_msgbox.text];
                    NSRange nsr=[cgsc_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                    if (nsr.location==NSNotFound) {
                        //HTTP
                        [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",cgsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:1];
                    } else {
                        //FTP
                        [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[cgsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:1];
                    }
                }
            }
        }
        
    }
    
    [self hideWaiting];
    
    
}
- (void) secondaryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    
    if (browse_depth==0) {
    } else {
        t_dbHVSC_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbCGSC?search_dbCGSC_entries:dbCGSC_entries);
        int download_all=0;
        if (search_dbCGSC) {
            if (search_dbCGSC_hasFiles) download_all=1;
        } else {
            if (dbCGSC_hasFiles) download_all=1;
        }
        
        int crow=indexPath.row;
        //if (download_all) crow--;

        if (crow>=0) {
            if (cur_db_entries[crow].id_md5) { //FILE
                //File selected, start download is needed
                NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[crow].label];
                NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[crow].fullpath];
                NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",CGSC_BASEDIR,cur_db_entries[crow].fullpath];
                mClickedPrimAction=2;
                
                if (cur_db_entries[crow].downloaded==1) {
                    //add to playlist
                    [self addToPlaylistSelView:localPath label:sidFilename showNowListening:true];
                    
                    cur_db_entries[crow].rating=-1;
                    [tableView reloadData];
                } else {
                    [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                    mCurrentWinAskedDownload=1;
                    
                    NSString *cgsc_url=[NSString stringWithFormat:@"%s",settings[ONLINE_CGSC_CURRENT_URL].detail.mdz_msgbox.text];
                    NSRange nsr=[cgsc_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                    if (nsr.location==NSNotFound) {
                        //HTTP
                        [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",cgsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                    } else {
                        //FTP
                        [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[cgsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                    }
                }
            }
        }
    }
    [self hideWaiting];
}


- (void) accessoryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    mAccessoryButton=1;
    [self tableView:tableView didSelectRowAtIndexPath:indexPath];
}


-(void) fillKeysSearchWithPopup {
    int old_mSearch=mSearch;
    NSString *old_mSearchText=mSearchText;
    mSearch=0;
    mSearchText=nil;
    [self fillKeys];   //1st load eveything
    mSearch=old_mSearch;
    mSearchText=old_mSearchText;
    if (mSearch) {
        shouldFillKeys=1;
        [self fillKeys];   //2nd filter for drawing
    }
    [tableView reloadData];
}

-(void) fillKeysWithPopup {
    [self fillKeys];
    [tableView reloadData];
}


- (void)tableView:(UITableView *)tabView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    // Navigation logic may go here. Create and push another view controller.
    //First get the dictionary object
    
    if (browse_depth==0) {
    } else {
        t_dbHVSC_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbCGSC?search_dbCGSC_entries:dbCGSC_entries);
        int download_all=0;
        if (search_dbCGSC) {
            if (search_dbCGSC_hasFiles) download_all=1;
        } else {
            if (dbCGSC_hasFiles) download_all=1;
        }
        int crow=indexPath.row;
        if (download_all) crow--;
        
        
        if (download_all && (crow==-1)) {
            //download all dir
            NSString *sidFilename;
            NSString *ftpPath;
            NSString *localPath;
            int first=0; //1;  Do not play even first file => TODO : add a setting for this
            int existing;
            int tooMuch=0;
            if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==2) first=0;//enqueue only
            
            int cur_db_entries_count=(search_dbCGSC?search_dbCGSC_entries_count:dbCGSC_entries_count);
            
                for (int j=0;j<cur_db_entries_count;j++) {
                    if (cur_db_entries[j].id_md5) {//mod found
                        
                        existing=cur_db_entries[j].downloaded;
                        if (existing==-1) {
                            NSString *pathToCheck=nil;
                            
                            if (cur_db_entries[j].fullpath)
                                pathToCheck=[NSString stringWithFormat:@"%@/Documents/%@%@",[ModizFileHelper getAppHomeDirectory],CGSC_BASEDIR,cur_db_entries[j].fullpath];
                            if (pathToCheck) {
                                if ([mFileMngr fileExistsAtPath:pathToCheck]) cur_db_entries[j].downloaded=1;
                                else existing=cur_db_entries[j].downloaded=0;
                            } else existing=cur_db_entries[j].downloaded=0;
                        }
                        if (existing==0) {
                            
                            //File selected, start download is needed
                            sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[j].label];
                            ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[j].fullpath];
                            localPath=[NSString stringWithFormat:@"Documents/%@%@",CGSC_BASEDIR,cur_db_entries[j].fullpath];
                            mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
                            
                            [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                            mCurrentWinAskedDownload=1;
                            
                            NSString *cgsc_url=[NSString stringWithFormat:@"%s",settings[ONLINE_CGSC_CURRENT_URL].detail.mdz_msgbox.text];
                            NSRange nsr=[cgsc_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                            
                            NSMutableArray *addDataSIDFname=[NSMutableArray array];
                            NSMutableArray *addDataFTPPath=[NSMutableArray array];
                            NSMutableArray *addDataLocalPath=[NSMutableArray array];
                            
                            [addDataSIDFname addObject:[[sidFilename stringByDeletingPathExtension] stringByAppendingString:@".str"]];
                            [addDataFTPPath addObject:[[ftpPath stringByDeletingPathExtension] stringByAppendingString:@".str"]];
                            [addDataLocalPath addObject:[[localPath stringByDeletingPathExtension] stringByAppendingString:@".str"]];
                            [addDataSIDFname addObject:[[sidFilename stringByDeletingPathExtension] stringByAppendingString:@".wds"]];
                            [addDataFTPPath addObject:[[ftpPath stringByDeletingPathExtension] stringByAppendingString:@".wds"]];
                            [addDataLocalPath addObject:[[localPath stringByDeletingPathExtension] stringByAppendingString:@".wds"]];
                            [addDataSIDFname addObject:[[sidFilename stringByDeletingPathExtension] stringByAppendingString:@".pic"]];
                            [addDataFTPPath addObject:[[ftpPath stringByDeletingPathExtension] stringByAppendingString:@".pic"]];
                            [addDataLocalPath addObject:[[localPath stringByDeletingPathExtension] stringByAppendingString:@".pic"]];
                            [addDataSIDFname addObject:[[sidFilename stringByDeletingPathExtension] stringByAppendingString:@".pgg"]];
                            [addDataFTPPath addObject:[[ftpPath stringByDeletingPathExtension] stringByAppendingString:@".pgg"]];
                            [addDataLocalPath addObject:[[localPath stringByDeletingPathExtension] stringByAppendingString:@".pgg"]];
                            [addDataSIDFname addObject:[[sidFilename stringByDeletingPathExtension] stringByAppendingString:@".pjj"]];
                            [addDataFTPPath addObject:[[ftpPath stringByDeletingPathExtension] stringByAppendingString:@".pjj"]];
                            [addDataLocalPath addObject:[[localPath stringByDeletingPathExtension] stringByAppendingString:@".pjj"]];
                            
                            if (first) {
                                if (nsr.location==NSNotFound) {
                                    //HTTP
                                    //Additional optional files, try to download
                                    for (int i=0;i<[addDataSIDFname count];i++) {
                                        if ([downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",cgsc_url,[addDataFTPPath objectAtIndex:i]] fileName:[addDataSIDFname objectAtIndex:i] filePath:[addDataLocalPath objectAtIndex:i] filesize:-1 isMODLAND:2 usePrimaryAction:0]
                                            ) {
                                            tooMuch=1;
                                            break;
                                        }
                                    }
                                    
                                    if ([downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",cgsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:1]
                                        ) {
                                        tooMuch=1;
                                        break;
                                    }
                                    
                                } else {
                                    //FTP
                                    if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[cgsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:1]) {
                                        tooMuch=1;
                                        break;
                                    }
                                }
                                first=0;
                            } else {
                                
                                if (nsr.location==NSNotFound) {
                                    //HTTP
                                    //Additional optional files, try to download
                                    for (int i=0;i<[addDataSIDFname count];i++) {
                                        if ([downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",cgsc_url,[addDataFTPPath objectAtIndex:i]] fileName:[addDataSIDFname objectAtIndex:i] filePath:[addDataLocalPath objectAtIndex:i] filesize:-1 isMODLAND:2 usePrimaryAction:0]
                                            ) {
                                            tooMuch=1;
                                            break;
                                        }
                                    }
                                    
                                    if ([downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",cgsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:2] ) {
                                        tooMuch=1;
                                        break;
                                    }
                                } else {
                                    //FTP
                                    if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[cgsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:2]) {
                                        tooMuch=1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
        } else {
            
            if (cur_db_entries[crow].id_md5) { //FILE
                //File selected, start download is needed
                NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[crow].label];
                NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[crow].fullpath];
                NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",CGSC_BASEDIR,cur_db_entries[crow].fullpath];
                mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
                
                if (cur_db_entries[crow].downloaded==1) {
                    if (mClickedPrimAction) {
                        NSMutableArray *array_label = [[NSMutableArray alloc] init];
                        NSMutableArray *array_path = [[NSMutableArray alloc] init];
                        [array_label addObject:sidFilename];
                        [array_path addObject:localPath];
                        cur_db_entries[crow].rating=-1;
                        [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                        if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                        
                        [tabView reloadData];
                    } else {
                        if ([detailViewController add_to_playlist:localPath fileName:sidFilename forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)]) {
                            if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                            
                            cur_db_entries[crow].rating=-1;
                            [tabView reloadData];
                        }
                    }
                } else {
                    [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                    mCurrentWinAskedDownload=1;
                    
                    NSString *cgsc_url=[NSString stringWithFormat:@"%s",settings[ONLINE_CGSC_CURRENT_URL].detail.mdz_msgbox.text];
                    NSRange nsr=[cgsc_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                    if (nsr.location==NSNotFound) {
                        //HTTP
                        NSMutableArray *addDataSIDFname=[NSMutableArray array];
                        NSMutableArray *addDataFTPPath=[NSMutableArray array];
                        NSMutableArray *addDataLocalPath=[NSMutableArray array];
                        
                        [addDataSIDFname addObject:[[sidFilename stringByDeletingPathExtension] stringByAppendingString:@".str"]];
                        [addDataFTPPath addObject:[[ftpPath stringByDeletingPathExtension] stringByAppendingString:@".str"]];
                        [addDataLocalPath addObject:[[localPath stringByDeletingPathExtension] stringByAppendingString:@".str"]];
                        [addDataSIDFname addObject:[[sidFilename stringByDeletingPathExtension] stringByAppendingString:@".wds"]];
                        [addDataFTPPath addObject:[[ftpPath stringByDeletingPathExtension] stringByAppendingString:@".wds"]];
                        [addDataLocalPath addObject:[[localPath stringByDeletingPathExtension] stringByAppendingString:@".wds"]];
                        [addDataSIDFname addObject:[[sidFilename stringByDeletingPathExtension] stringByAppendingString:@".pic"]];
                        [addDataFTPPath addObject:[[ftpPath stringByDeletingPathExtension] stringByAppendingString:@".pic"]];
                        [addDataLocalPath addObject:[[localPath stringByDeletingPathExtension] stringByAppendingString:@".pic"]];
                        [addDataSIDFname addObject:[[sidFilename stringByDeletingPathExtension] stringByAppendingString:@".pgg"]];
                        [addDataFTPPath addObject:[[ftpPath stringByDeletingPathExtension] stringByAppendingString:@".pgg"]];
                        [addDataLocalPath addObject:[[localPath stringByDeletingPathExtension] stringByAppendingString:@".pgg"]];
                        [addDataSIDFname addObject:[[sidFilename stringByDeletingPathExtension] stringByAppendingString:@".pjj"]];
                        [addDataFTPPath addObject:[[ftpPath stringByDeletingPathExtension] stringByAppendingString:@".pjj"]];
                        [addDataLocalPath addObject:[[localPath stringByDeletingPathExtension] stringByAppendingString:@".pjj"]];
                        
                        for (int i=0;i<[addDataSIDFname count];i++) {
                            if ([downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",cgsc_url,[addDataFTPPath objectAtIndex:i]] fileName:[addDataSIDFname objectAtIndex:i] filePath:[addDataLocalPath objectAtIndex:i] filesize:-1 isMODLAND:2 usePrimaryAction:0]
                                ) {
                            }
                        }
                        
                        [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",cgsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                    } else {
                        //FTP
                        [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[cgsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                    }
                    
                }
            } else { //DIR
                if (browse_depth==1) {//DIR1
                    mDir1=cur_db_entries[crow].dir1;
                } else if (browse_depth==2) {//DIR2
                    mDir2=cur_db_entries[crow].dir2;
                }
                
                if (childController == nil) childController = [[RootViewControllerCGSC alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                else {// Don't cache childviews
                }
                
                childController.title = cur_db_entries[crow].label;
                // Set new depth
                ((RootViewControllerCGSC*)childController)->browse_depth = browse_depth+1;
                ((RootViewControllerCGSC*)childController)->detailViewController=detailViewController;
                ((RootViewControllerCGSC*)childController)->downloadViewController=downloadViewController;
                
                ((RootViewControllerCGSC*)childController)->mDir1 = mDir1;
                ((RootViewControllerCGSC*)childController)->mDir2 = mDir2;
//                childController.view.frame=self.view.frame;
                // Ensure proper layout under navigation/tab bars
                if ([childController respondsToSelector:@selector(setEdgesForExtendedLayout:)]) {
                    childController.edgesForExtendedLayout = UIRectEdgeNone;
                    childController.extendedLayoutIncludesOpaqueBars = NO;
                }
                if ([childController isKindOfClass:[UITableViewController class]]) {
                    ((UITableViewController *)childController).tableView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentAutomatic;
                } else if ([childController.view isKindOfClass:[UIScrollView class]]) {
                    ((UIScrollView *)childController.view).contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentAutomatic;
                }
                // And push the window
                [self.navigationController pushViewController:childController animated:YES];
            }
        }
    }
    mAccessoryButton=0;
}


/* POPUP functions */
-(void) hidePopup {
    infoMsgView.hidden=YES;
    mPopupAnimation=0;
}

-(void) openPopup:(NSString *)msg {
    CGRect frame;
    if (mPopupAnimation) return;
    mPopupAnimation=1;
    infoMsgView.layer.zPosition=MAXFLOAT;
    frame=infoMsgView.frame;
    frame.origin.y=self.view.frame.size.height;
    infoMsgView.frame=frame;
    infoMsgView.hidden=NO;
    infoMsgLbl.text=[NSString stringWithString:msg];
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDelay:0];
    [UIView setAnimationDuration:0.5];
    [UIView setAnimationDelegate:self];
    frame=infoMsgView.frame;
    frame.origin.y=self.view.frame.size.height-64-32;
    infoMsgView.frame=frame;
    [UIView setAnimationDidStopSelector:@selector(closePopup)];
    [UIView commitAnimations];
}
-(void) closePopup {
    CGRect frame;
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDelay:1.0];
    [UIView setAnimationDuration:0.5];
    [UIView setAnimationDelegate:self];
    frame=infoMsgView.frame;
    frame.origin.y=self.view.frame.size.height;
    infoMsgView.frame=frame;
    [UIView setAnimationDidStopSelector:@selector(hidePopup)];
    [UIView commitAnimations];
}


#pragma mark -
#pragma mark Memory management

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Relinquish ownership any cached data, images, etc. that aren't in use.
}
- (void)viewDidUnload {
    // Relinquish ownership of anything that can be recreated in viewDidLoad or on demand.
    // For example: self.myOutlet = nil;;
}
- (void)dealloc {
    [waitingView removeFromSuperview];
    waitingView=nil;
    //[waitingView release];
    
    //[currentPath release];
    if (mSearchText) {
        //[mSearchText release];
        mSearchText=nil;
    }
    
    if (dbCGSC_nb_entries) {
        for (int i=0;i<dbCGSC_nb_entries;i++) {
            dbCGSC_entries_data[i].label=nil;
            dbCGSC_entries_data[i].fullpath=nil;
            dbCGSC_entries_data[i].id_md5=nil;
            dbCGSC_entries_data[i].dir1=nil;
            dbCGSC_entries_data[i].dir2=nil;
            
        }
        free(dbCGSC_entries_data);
    }
    if (search_dbCGSC_nb_entries) {
            for (int j=0;j<search_dbCGSC_entries_count;j++) {
                search_dbCGSC_entries[j].label=nil;
                search_dbCGSC_entries[j].fullpath=nil;
                search_dbCGSC_entries[j].id_md5=nil;
                search_dbCGSC_entries[j].dir1=nil;
                search_dbCGSC_entries[j].dir2=nil;
            }
            search_dbCGSC_entries=NULL;
        search_dbCGSC_nb_entries=0;
        free(search_dbCGSC_entries_data);
    }
    
    if (mFileMngr) {
        //[mFileMngr release];
        mFileMngr=nil;
    }
    
    if (self.mdzChangeObserverToken) {
        [[NSNotificationCenter defaultCenter] removeObserver:self.mdzChangeObserverToken];
        self.mdzChangeObserverToken = nil;
    }
    
    //[super dealloc];
}

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
        [self updateMiniPlayer];
    }
}


#pragma mark - UINavigationControllerDelegate

- (id <UIViewControllerAnimatedTransitioning>)navigationController:(UINavigationController *)navigationController
                                   animationControllerForOperation:(UINavigationControllerOperation)operation
                                                fromViewController:(UIViewController *)fromVC
                                                  toViewController:(UIViewController *)toVC
{
    return [[TTFadeAnimator alloc] init];
}

#pragma mark - LoadingView related stuff

- (void) cancelPushed {
    detailViewController.mplayer.extractPendingCancel=true;
    [detailViewController setCancelStatus:true];
    [detailViewController hideWaitingCancel];
    [detailViewController hideWaitingProgress];
    [detailViewController updateWaitingDetail:NSLocalizedString(@"Cancelling...",@"")];
        
    [self hideWaitingCancel];
    [self hideWaitingProgress];
    [self updateWaitingDetail:NSLocalizedString(@"Cancelling...",@"")];
}

-(void) updateLoadingInfos: (NSTimer *) theTimer {
    [waitingViewPlayer.progressView setProgress:detailViewController.waitingView.progressView.progress animated:YES];
}


- (void) observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                       context:(void *)context
{
    if (context==LoadingProgressObserverContext){
        WaitingView *wv = object;
        if (wv.hidden==false) {
            [waitingViewPlayer resetCancelStatus];
            waitingViewPlayer.hidden=detailViewController.waitingView.hidden;
            waitingViewPlayer.btnStopCurrentAction.hidden=detailViewController.waitingView.btnStopCurrentAction.hidden;
            waitingViewPlayer.progressView.progress=detailViewController.waitingView.progressView.progress;
            waitingViewPlayer.progressView.hidden=detailViewController.waitingView.progressView.hidden;
            waitingViewPlayer.lblTitle.text=[NSString stringWithString:detailViewController.waitingView.lblTitle.text];
            waitingViewPlayer.lblDetail.text=[NSString stringWithString:detailViewController.waitingView.lblDetail.text];
        }
        waitingViewPlayer.hidden=wv.hidden;
        
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}


@end
