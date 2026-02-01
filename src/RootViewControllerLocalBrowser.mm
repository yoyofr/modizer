//
//  RootViewControllerLocalBrowser.mm
//  modizer
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//
#define RATING_IMG(a) ( (a==5?2:(a?1:0)) )

#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define LIMITED_LIST_SIZE 1024

extern NSURL *icloudURL;
extern bool icloud_available;

NSString *cutpaste_filesrcpath=nil;

#import <UserNotifications/UserNotifications.h>

#include <sys/types.h>
#include <sys/sysctl.h>

#include "gme.h"

//SPC parser
#include "SPCTagParser.h"


//FileHelper
#include "ModizFileHelper.h"

//SID2
#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidTuneInfo.h"

#include "unzip.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;
pthread_mutex_t db_mutexHVSCSTIL;
static volatile int mPopupAnimation=0;

#import "RootViewControllerLocalBrowser.h"
#import "AppDelegate_Phone.h"
#import "DetailViewControllerIphone.h"
#import "QuartzCore/CAAnimation.h"
#import "SettingsGenViewController.h"

extern volatile t_settings settings[MAX_SETTINGS];

#import "TTFadeAnimator.h"


@implementation RootViewControllerLocalBrowser

@synthesize mFileMngr;
@synthesize detailViewController,tableView;
@synthesize sBar;
@synthesize currentPath;
@synthesize childController;
@synthesize mSearchText;
@synthesize popTipView;
@synthesize waitingView,waitingViewExtract,waitingViewPlayer;
@synthesize repeatTimer,activeKey;

#pragma mark -
#pragma mark Search functiçns
#import "SearchCommonFunctions.h"


#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/xattr.h>

static volatile int mUpdateToNewDB;
static volatile int mDatabaseCreationInProgress;
static volatile int db_checked=0;

extern "C" {
#include "common/md5.h"
}


static char browser_song_md5[33];
static char *browser_stil_info;//[MAX_STIL_DATA_LENGTH];
static char **browser_sidtune_title,**browser_sidtune_name;

#include "MiniPlayerImplementTableView.h"

#include "AlertsCommonFunctions.h"



-(void) getDBVersion:(int*)major minor:(int*)minor {
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    pthread_mutex_lock(&db_mutex);
    
    *major=0;
    *minor=0;
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT major,minor FROM version");
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                *major=sqlite3_column_int(stmt, 0);
                *minor=sqlite3_column_int(stmt, 1);
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        sqlite3_close(db);
    }
    
    pthread_mutex_unlock(&db_mutex);
}

void mymkdir(char *filePath);
int do_extract_currentfile(unzFile uf,char *pathToExtract,NSString *pathBase);
int do_extract(unzFile uf,char *pathToExtract,NSString *pathBase);

-(void)recreateDB {
    //NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    @autoreleasepool {
        
        NSString *defaultDBPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_USER];
        NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
        NSString *pathToOldDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_TMP];
        NSError *error;
        NSFileManager *fileManager=[[NSFileManager alloc] init];
        
        if (mUpdateToNewDB) {
            //1st clean obsolete data
            DBHelper::cleanDB();
        }
        
        pthread_mutex_lock(&db_mutex);
        if (mUpdateToNewDB) {
            //rename to old DB
            [fileManager moveItemAtPath:pathToDB toPath:pathToOldDB error:&error];
            //        [self addSkipBackupAttributeToItemAtPath:pathToOldDB];
        }
        
        [fileManager copyItemAtPath:defaultDBPath toPath:pathToDB error:&error];
        //    [self addSkipBackupAttributeToItemAtPath:pathToDB];
        
        
        if (mUpdateToNewDB) {
            sqlite3 *db,*dbold;
            
            if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
                if (sqlite3_open([pathToOldDB UTF8String], &dbold) == SQLITE_OK){
                    char sqlStatementR[1024];
                    char sqlStatementR2[1024];
                    char sqlStatementW[1024];
                    sqlite3_stmt *stmt,*stmt2;
                    int err;
                    
                    err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
                    if (err==SQLITE_OK){
                    } else MDZELog("ErrSQL : %d",err);
                    
                    //Migrate DB user data : song length, ratings, playlists, ...
                    snprintf(sqlStatementR,1024,"SELECT name,fullpath,play_count,rating FROM user_stats");
                    err=sqlite3_prepare_v2(dbold, sqlStatementR, -1, &stmt, NULL);
                    if (err==SQLITE_OK){
                        while (sqlite3_step(stmt) == SQLITE_ROW) {
                            
                            snprintf(sqlStatementW,1024,"INSERT INTO user_stats (name,fullpath,play_count,rating) SELECT \"%s\",\"%s\",%d,%d",
                                    (char*)sqlite3_column_text(stmt, 0),
                                    (char*)sqlite3_column_text(stmt, 1),
                                    sqlite3_column_int(stmt, 2),
                                    sqlite3_column_int(stmt, 3));
                            err=sqlite3_exec(db, sqlStatementW, NULL, NULL, NULL);
                            if (err!=SQLITE_OK) MDZELog("ErrSQL : %d for %s",err,sqlStatementW);
                        }
                        sqlite3_finalize(stmt);
                    } else MDZELog("ErrSQL : %d",err);
                    
                    snprintf(sqlStatementR,1024,"SELECT id,name,num_files FROM playlists");
                    err=sqlite3_prepare_v2(dbold, sqlStatementR, -1, &stmt, NULL);
                    if (err==SQLITE_OK){
                        while (sqlite3_step(stmt) == SQLITE_ROW) {
                            int id_playlist;
                            //CREATE NEW PL
                            snprintf(sqlStatementW,1024,"INSERT INTO playlists (name,num_files) SELECT \"%s\",%d",
                                    (char*)sqlite3_column_text(stmt, 1),
                                    sqlite3_column_int(stmt, 2));
                            err=sqlite3_exec(db, sqlStatementW, NULL, NULL, NULL);
                            if (err!=SQLITE_OK) MDZELog("ErrSQL : %d for %s",err,sqlStatementW);
                            
                            //GET NEW PL ID
                            id_playlist=sqlite3_last_insert_rowid(db);
                            
                            //RECOPY PL ENTRIES
                            snprintf(sqlStatementR2,1024,"SELECT name,fullpath FROM playlists_entries WHERE id_playlist=%d",sqlite3_column_int(stmt, 0));
                            err=sqlite3_prepare_v2(dbold, sqlStatementR2, -1, &stmt2, NULL);
                            if (err==SQLITE_OK){
                                while (sqlite3_step(stmt2) == SQLITE_ROW) {
                                    
                                    snprintf(sqlStatementW,104,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %d,\"%s\",\"%s\"",
                                            id_playlist,
                                            [[[NSString stringWithUTF8String:(char*)sqlite3_column_text(stmt2, 0)] lastPathComponent] UTF8String],
                                            (char*)sqlite3_column_text(stmt2, 1));
                                    err=sqlite3_exec(db, sqlStatementW, NULL, NULL, NULL);
                                    if (err!=SQLITE_OK) MDZELog("ErrSQL : %d for %s",err,sqlStatementW);
                                }
                                sqlite3_finalize(stmt2);
                            } else MDZELog("ErrSQL : %d",err);
                            
                        }
                        sqlite3_finalize(stmt);
                    } else MDZELog("ErrSQL : %d",err);
                    
                    
                    
                    sqlite3_close(dbold);
                    
                    //remove old DB
                    [fileManager removeItemAtPath:pathToOldDB error:&error];
                }
                sqlite3_close(db);
            };
        }
        
        /*	rar_main(argc,argv);
         free(argv_buffer);
         free(argv);*/
        pthread_mutex_unlock(&db_mutex);
        //	[self recreateDBIndexes];
        
        //[unrarPath release];
        db_checked=1;
        mDatabaseCreationInProgress=0;
        
        fileManager=nil;
    }
}

// Creates a writable copy of the bundled default database in the application Documents directory.
- (void) createSamplesFromPackage:(BOOL)forceCreate {
    BOOL success;
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    NSError *error;
    
    NSString *samplesDocPath=[NSString stringWithFormat:@"%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents/Samples"]];
    NSString *samplesPkgPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Samples"];
    
    
    success = [fileManager fileExistsAtPath:samplesDocPath];
    if (success&&(forceCreate==FALSE)) {
        //[fileManager release];
        fileManager=nil;
        return;
    }
    [fileManager removeItemAtPath:samplesDocPath error:&error];
    [fileManager copyItemAtPath:samplesPkgPath toPath:samplesDocPath error:&error];
    //update 'Do Not Backup' flag for directory & content (not sure it is required for the latest,...)
    [ModizFileHelper addSkipBackupAttributeToItemAtPath:samplesDocPath];
    [ModizFileHelper updateFilesDoNotBackupAttributes];
    
    //[fileManager release];
    fileManager=nil;
}

// Creates a writable copy of the bundled default database in the application Documents directory.
- (void) createEditableCopyOfDatabaseIfNeeded:(bool)forceInit quiet:(int)quiet {
    // First, test for existence.
    BOOL success=FALSE;
    BOOL wrongversion=FALSE;
    int maj,min;
    mUpdateToNewDB=0;
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    NSError *error;
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *writableDBPath = [documentsDirectory stringByAppendingPathComponent:DATABASENAME_USER];
    
    success = [fileManager fileExistsAtPath:writableDBPath];
    if (success && (!forceInit)) {
        maj=min=0;
        [self getDBVersion:&maj minor:&min];
        if ((maj==VERSION_MAJOR)&&(min==VERSION_MINOR)) {
            db_checked=1;
            fileManager=nil;
            //check if Samples folder has to be recreated
            if (settings[GLOB_RecreateSamplesFolder].detail.mdz_boolswitch.switch_value==1) [self createSamplesFromPackage:FALSE];
            return;
        } else {
            mUpdateToNewDB=1;
            wrongversion=TRUE;
        }
    }
    // The writable database does not exist, so copy the default to the appropriate location.
    
    if (forceInit||(success&&(!wrongversion))) {//remove existing file
        success = [fileManager removeItemAtPath:writableDBPath error:&error];
    }
    fileManager=nil;
    
    mDatabaseCreationInProgress=1;
    
    if (settings[GLOB_RecreateSamplesFolder].detail.mdz_boolswitch.switch_value==1) {
        /*if (mUpdateToNewDB) */[self createSamplesFromPackage:TRUE];
    } else [self createSamplesFromPackage:FALSE];
    
    if (quiet) [self recreateDB];
    else {
        if (forceInit) {
            [self showAlertMsg:NSLocalizedString(@"Info",@"") message:NSLocalizedString(@"Database will now be recreated. Please validate & wait.",@"")];
            [self recreateDB];
        }
        else {
            if (wrongversion) {
                [self showAlertMsg:NSLocalizedString(@"Info",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Old database version: %d.%d. Will update to %d.%d. Please validate & wait.",@""),maj,min,VERSION_MAJOR,VERSION_MINOR]];
                [self recreateDB];
            }
            else  {
                //USer database missing, create it
                [self recreateDB];
            }
        }
    }
}


#pragma mark -
#pragma mark View lifecycle

- (id) initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (NSString *)machine {
    size_t size;
    
    // Set 'oldp' parameter to NULL to get the size of the data
    // returned so we can allocate appropriate amount of space
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    
    // Allocate the space to store name
    char *name = (char*)malloc(size);
    
    // Get the platform name
    sysctlbyname("hw.machine", name, &size, NULL, 0);
    
    // Place name into a string
    NSString *machine = [[NSString alloc] initWithFormat:@"%s",name];
    
    // Done with this
    free(name);
    
    return machine;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////

-(void) searchbarMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    }
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
    if (!self.detailViewController) self.detailViewController = [self findChildOfClass:[DetailViewControllerIphone class] inTabBarController:tbc];
}

- (void)viewDidLoad {
    START_PROFILE
    childController=nil;
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad || [NSProcessInfo processInfo].isiOSAppOnMac) {
        self.hidesBottomBarWhenPushed = YES;
    }

    [self loadControllers];
    
    cutpaste_initiated=0;
    
    dictActionBtn=[NSMutableDictionary dictionaryWithCapacity:64];
    
    extractProgress = nil;
    
    tabViewRefresh=0;
    
    is_rsn=0;
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    self.navigationController.delegate = self;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
    mFileMngr=[[NSFileManager alloc] init];
    
    mShowSubdir=0;
    
    ratingImg[0] = @"heart-empty.png";
    ratingImg[1] = @"heart-half-filled.png";
    ratingImg[2] = @"heart-filled.png";
    
    //self.tableView.pagingEnabled;
    self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.tableView.sectionHeaderHeight = 18;
    self.tableView.rowHeight = 40;
    self.tableView.separatorInset = UIEdgeInsetsZero;
    
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
    
    search_local=0;
    
    local_nb_entries=0;
    search_local_nb_entries=0;
    
    mSearchText=nil;
    mCurrentWinAskedDownload=0;
    mClickedPrimAction=0;
    
    if (browse_depth==0) { //Local mode
        currentPath = @"Documents";
        //[currentPath retain];
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
    
    waitingViewExtract = [[WaitingView alloc] init];
    waitingViewExtract.layer.zPosition=MAXFLOAT;
    [self.view addSubview:waitingViewExtract];
    waitingViewExtract.hidden=TRUE;
    
    views = NSDictionaryOfVariableBindings(waitingViewExtract);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingViewExtract(150)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingViewExtract(150)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewExtract attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view.safeAreaLayoutGuide attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewExtract attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
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
    
    // Initialize the refresh control.
    self.tableView.refreshControl = [[UIRefreshControl alloc] init];
    self.tableView.refreshControl.backgroundColor = [UIColor clearColor];
    self.tableView.refreshControl.tintColor = [UIColor whiteColor];
    [self.tableView.refreshControl addTarget:self
                                      action:@selector(refreshViewReloadFiles)
                            forControlEvents:UIControlEventValueChanged];
    
    
    //tableView.refreshControl=refreshControl;
    
    END_PROFILE
}

-(void) fillKeys {
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    shouldFillKeys=1;
    
    if (shouldFillKeys) {
        shouldFillKeys=0;
        [self listLocalFiles];
    }
}


-(void) getStilInfo:(char*)fullPath {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    int err;
    
    
    strcpy(browser_stil_info,"");
    pthread_mutex_lock(&db_mutexHVSCSTIL);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        char tmppath[256];
        sqlite3_stmt *stmt;
        char *realPath=strstr(fullPath,"/HVSC");
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        if (!realPath) {
            //try to find realPath with md5
            snprintf(sqlStatement,1024,"SELECT filepath FROM hvsc_path WHERE id_md5=\"%s\"",browser_song_md5);
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    strcpy(tmppath,(const char*)sqlite3_column_text(stmt, 0));
                    realPath=tmppath;
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
        } else realPath+=5;
        if (realPath) {
            snprintf(sqlStatement,1024,"SELECT stil_info FROM stil WHERE fullpath=\"%s\"",realPath);
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    strcpy(browser_stil_info,(const char*)sqlite3_column_text(stmt, 0));
                    while ((realPath=strstr(browser_stil_info,"\\n"))) {
                        *realPath='\n';
                        realPath++;
                        memmove(realPath,realPath+1,strlen(realPath));
                    }
                }
                sqlite3_finalize(stmt);
            } else MDZELog("ErrSQL : %d",err);
        }
    };
    sqlite3_close(db);
    
    pthread_mutex_unlock(&db_mutexHVSCSTIL);
}


-(void) sid_parseStilInfo:(int)subsongs_nb {
    
    int idx=0;
    int parser_status=0;
    int parser_track_nb=0;
    int stil_info_len=strlen(browser_stil_info);
    char tmp_str[1024];
    int tmp_str_idx;
    
    browser_sidtune_name=(char**)calloc(subsongs_nb,sizeof(char*));
    
    browser_sidtune_title=(char**)calloc(subsongs_nb,sizeof(char*));
    
    while (browser_stil_info[idx]) {
        if ((browser_stil_info[idx]=='(')&&(browser_stil_info[idx+1]=='#')) {
            parser_status=1;
            parser_track_nb=0;
        }
        else {
            switch (parser_status) {
                case 1: // got a "(" before
                    if (browser_stil_info[idx]=='#') parser_status=2;
                    else parser_status=0;
                    break;
                case 2: // got a "(#" before
                    if ((browser_stil_info[idx]>='0')&&(browser_stil_info[idx]<='9')) {
                        parser_track_nb=parser_track_nb*10+(browser_stil_info[idx]-'0');
                        parser_status=2;
                    } else if (browser_stil_info[idx]==')') {
                        parser_status=3;
                    }
                    break;
                case 3: // got a "(#<track_nb>)" before
                    if (strncmp(browser_stil_info+idx,"NAME: ",strlen("NAME: "))==0) {
                        parser_status=4;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                        idx+=strlen("NAME: ")-1;
                    } else if (strncmp(browser_stil_info+idx,"TITLE: ",strlen("TITLE: "))==0) {
                        parser_status=5;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                        idx+=strlen("TITLE: ")-1;
                    } else
                        break;
                case 4: // "NAME: "
                    if (browser_stil_info[idx]==0x0A) {
                        parser_status=0;
                        if (parser_track_nb<=subsongs_nb) {
                            browser_sidtune_name[parser_track_nb-1]=(char*)malloc(tmp_str_idx+1);
                            strcpy(browser_sidtune_name[parser_track_nb-1],tmp_str);
                        }
                    } else {
                        tmp_str[tmp_str_idx]=browser_stil_info[idx];
                        if (tmp_str_idx<1024) tmp_str_idx++;
                        tmp_str[tmp_str_idx]=0;
                    }
                    break;
                case 5: // "TITLE: "
                    if (browser_stil_info[idx]==0x0A) {
                        parser_status=0;
                        if (parser_track_nb<=subsongs_nb) {
                            browser_sidtune_title[parser_track_nb-1]=(char*)malloc(tmp_str_idx+1);
                            strcpy(browser_sidtune_title[parser_track_nb-1],tmp_str);
                        }
                    } else {
                        tmp_str[tmp_str_idx]=browser_stil_info[idx];
                        if (tmp_str_idx<1024) tmp_str_idx++;
                        tmp_str[tmp_str_idx]=0;
                    }
                    break;
                default:
                    break;
            }
        }
        idx++;
        
    }
}

static void writeLEword(unsigned char ptr[2], int someWord) {
    ptr[0] = (someWord & 0xFF);
    ptr[1] = (someWord >> 8);
}



static void md5_from_buffer(char *dest, size_t destlen,char * buf, size_t bufsize)
{
    uint8_t md5[16];
    int ret;
    MD5_CTX ctx;
    MD5Init(&ctx);
    MD5Update(&ctx, (const unsigned char*)buf, bufsize);
    MD5Final(md5, &ctx);
    ret =
    snprintf(dest, destlen,
             "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
             md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6],
             md5[7], md5[8], md5[9], md5[10], md5[11], md5[12], md5[13],
             md5[14], md5[15]);
    if (ret >= destlen || ret != 32) {
        fprintf(stderr, "md5 buffer error (%d/%zd)\n", ret, destlen);
        exit(1);
    }
}

static int qsort_CompareArcEntries(const void *entryA, const void *entryB) {
    char *strA,*strB;
    int res;
    strA=*((char**)entryA);
    strB=*((char**)entryB);
    res=strcmp(strA,strB);
    if (res<0) return -1;
    if (res==0) return 0;
    return 1;
}


-(void) listLocalFiles {
    NSString *file,*cpath;
    NSURL *fileURL;
    NSMutableArray *filetype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE];
    NSMutableArray *filetype_extAMIGA=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE_AMIGA];
    NSMutableArray *all_multisongstype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE_SUBSONGS];
    NSMutableArray *archivetype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_ARCHIVE];
    
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    char sqlStatement[1024];
    sqlite3_stmt *stmt;
    int local_entries_index,local_nb_entries_limit;
    int browseType;
    int shouldStop=0;
    static bool no_reentrant=false;
    
    if (no_reentrant) return;
    no_reentrant=true;
    
    // First check count for each section
    cpath=[ModizFileHelper getFullPathForFilePath:currentPath];
    //Check if it is a directory or an archive
    BOOL isDirectory;
    browseType=0;
    if ([mFileMngr fileExistsAtPath:cpath isDirectory:&isDirectory]) {
        if (!isDirectory) {
            //file:check if archive or multisongs
            NSString *extension=[[[cpath lastPathComponent] pathExtension] uppercaseString];
            if ([archivetype_ext indexOfObject:extension]!=NSNotFound) {
                //check if really an archive
                if ([ModizFileHelper isABrowsableArchive:cpath]) browseType=1;
            }
            //check if Multisongs file
            else if ([all_multisongstype_ext indexOfObject:extension]!=NSNotFound) {
                //check if really a gme file
                if ([ModizFileHelper isGMEFileWithSubsongs:cpath]) browseType=2;
                else if ([ModizFileHelper isSidFileWithSubsongs:cpath]) browseType=3;
            }
        }
    }
    
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    search_local=0;
    
    if (search_local_nb_entries) {
        for (int j=0;j<search_local_entries_count;j++) {
            search_local_entries[j].label=nil;
            search_local_entries[j].altlabel=nil;
            search_local_entries[j].fullpath=nil;
        }
        search_local_entries=NULL;
        search_local_nb_entries=0;
        free(search_local_entries_data);
    }
    
    if (mSearch) {
        search_local=1;
        
        search_local_entries_data=(t_local_browse_entry*)calloc(local_nb_entries,sizeof(t_local_browse_entry));
        
        search_local_entries_count=0;
        if (local_entries_count) search_local_entries=search_local_entries_data;
        for (int j=0;j<local_entries_count;j++)  {
            
            bool found=false;
            if ((browseType==0)&&mShowSubdir) {
                found=[self searchStringRegExp:mSearchText sourceString:local_entries[j].label];
                if (!found) found=[self searchStringRegExp:mSearchText sourceString:local_entries[j].fullpath];
            } else {
                found=[self searchStringRegExp:mSearchText sourceString:local_entries[j].label];
            }
            
            if  (found ||([mSearchText length]==0)) {
                search_local_entries[search_local_entries_count].label=local_entries[j].label;
                search_local_entries[search_local_entries_count].altlabel=local_entries[j].altlabel;
                search_local_entries[search_local_entries_count].fullpath=local_entries[j].fullpath;
                search_local_entries[search_local_entries_count].playcount=local_entries[j].playcount;
                search_local_entries[search_local_entries_count].rating=local_entries[j].rating;
                search_local_entries[search_local_entries_count].type=local_entries[j].type;
                
                search_local_entries[search_local_entries_count].song_length=local_entries[j].song_length;
                search_local_entries[search_local_entries_count].songs=local_entries[j].songs;
                search_local_entries[search_local_entries_count].channels_nb=local_entries[j].channels_nb;
                
                search_local_entries_count++;
                search_local_nb_entries++;
            }
        }
        no_reentrant=false;
        return;
    }
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) != SQLITE_OK) db=NULL;
    
    err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
    if (err==SQLITE_OK){
    } else MDZELog("ErrSQL : %d",err);
    
    
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            local_entries_data[i].label=nil;
            local_entries_data[i].altlabel=nil;
            local_entries_data[i].fullpath=nil;
        }
        for (int j=0;j<local_entries_count;j++) {
            local_entries[j].label=nil;
            local_entries[j].altlabel=nil;
            local_entries[j].fullpath=nil;
        }
        local_entries=NULL;
        free(local_entries_data);local_entries_data=NULL;
        local_nb_entries=0;
    }
    local_entries_count=0;
    
    
    
    if (browseType==3) {//SID
        SidTune *mSidTune=new SidTune([cpath UTF8String],0,true);
        bool sid_ok=true;
        if (mSidTune==NULL) sid_ok=false;
        else if (!(mSidTune->getStatus())) sid_ok=false;
        if (!sid_ok) {
            MDZELog("SID SidTune init error");
            if (mSidTune) {delete mSidTune;mSidTune=NULL;}
        } else {
            const SidTuneInfo *sidtune_info;
            sidtune_info=mSidTune->getInfo();
            
            
            
            //Compute MD5
            memset(browser_song_md5,0,33);
            
            mSidTune->createMD5New(browser_song_md5);
            browser_song_md5[32]=0;
            
            
            //Get STIL info
            browser_stil_info=(char*)calloc(1,MAX_STIL_DATA_LENGTH);
            [self getStilInfo:(char*)[cpath UTF8String]];
            
            [self sid_parseStilInfo:sidtune_info->songs()];
            mdz_safe_free(browser_stil_info)
            
            for (int i=0;i<sidtune_info->songs();i++){
                const SidTuneInfo *s_info;
                file=nil;
                mSidTune->selectSong(i);
                s_info=mSidTune->getInfo();
                
                if (browser_sidtune_name[i]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,browser_sidtune_name[i]];
                else if (browser_sidtune_title[i]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,browser_sidtune_title[i]];
                else if (s_info->infoString(0)[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,s_info->infoString(0)];
                else file=[NSString stringWithFormat:@"%.3d-%@",i+1,[cpath lastPathComponent]];
                
                int filtered=0;
                if ((mSearch)&&([mSearchText length]>0)) {
                    filtered=1;
                    //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                    //if (r.location != NSNotFound) {
                    if ([self searchStringRegExp:mSearchText sourceString:file]) {
                        /*if(r.location== 0)*/ filtered=0;
                    }
                }
                if (!filtered) {
                    local_entries_count++;
                    local_nb_entries++;
                }
            }
            
            if (local_nb_entries) {
                //2nd initialize array to receive entries
                local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries,sizeof(t_local_browse_entry));
                if (!local_entries_data) {
                    //Not enough memory
                    //try to allocate less entries
                    local_nb_entries_limit=LIMITED_LIST_SIZE;
                    if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                    local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries_limit,sizeof(t_local_browse_entry));
                    if (local_entries_data==NULL) {
                        //show alert : cannot list
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem.",@"")];
                    } else {
                        //show alert : limited list
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                        local_nb_entries=local_nb_entries_limit;
                    }
                } else local_nb_entries_limit=0;
                if (local_entries_data) {
                    local_entries_index=0;
                    if (local_entries_count) {
                        if (local_entries_index+local_entries_count>local_nb_entries) {
                            local_entries_count=local_nb_entries-local_entries_index;
                            local_entries=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count;
                            local_entries_count=0;
                        } else {
                            local_entries=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count;
                            local_entries_count=0;
                        }
                    }
                    
                    for (int i=0;i<sidtune_info->songs();i++){
                        const SidTuneInfo *s_info;
                        file=nil;
                        mSidTune->selectSong(i);
                        s_info=mSidTune->getInfo();
                        
                        if (browser_sidtune_name[i]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,browser_sidtune_name[i]];
                        else if (browser_sidtune_title[i]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,browser_sidtune_title[i]];
                        else if (s_info->infoString(0)[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,s_info->infoString(0)];
                        else file=[NSString stringWithFormat:@"%.3d-%@",i+1,[cpath lastPathComponent]];
                        
                        int filtered=0;
                        if ((mSearch)&&([mSearchText length]>0)) {
                            filtered=1;
                            //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                            //if (r.location != NSNotFound) {
                            if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                /*if(r.location== 0)*/ filtered=0;
                            }
                        }
                        
                        if (!filtered) {
                            
                            local_entries[local_entries_count].type=1;
                            local_entries[local_entries_count].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                            local_entries[local_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@?%d",currentPath,i];
                            
                            local_entries[local_entries_count].rating=0;
                            local_entries[local_entries_count].playcount=0;
                            local_entries[local_entries_count].song_length=0;
                            local_entries[local_entries_count].songs=1;//0;
                            local_entries[local_entries_count].channels_nb=0;
                            
                            snprintf(sqlStatement,1024,"SELECT play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[local_entries[local_entries_count].fullpath UTF8String]);
                            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                            if (err==SQLITE_OK){
                                while (sqlite3_step(stmt) == SQLITE_ROW) {
                                    signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                    if (rating<0) rating=0;
                                    if (rating>5) rating=5;
                                    if ((rating==0)&&(sqlite3_column_type(stmt,5)!=SQLITE_NULL)) {
                                        rating=(signed char)sqlite3_column_int(stmt, 5);
                                        if (rating) rating=1;
                                    }
                                    local_entries[local_entries_count].playcount=(short int)sqlite3_column_int(stmt, 0);
                                    local_entries[local_entries_count].rating=rating;
                                    local_entries[local_entries_count].song_length=(int)sqlite3_column_int(stmt, 2);
                                    local_entries[local_entries_count].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                }
                                sqlite3_finalize(stmt);
                            } else MDZELog("ErrSQL : %d",err);
                            
                            local_entries_count++;
                            
                            if (local_nb_entries_limit) {
                                local_nb_entries_limit--;
                                if (!local_nb_entries_limit) shouldStop=1;
                            }
                            
                        }
                    }
                }
            }
            if (browser_sidtune_title) {
                for (int i=0;i<sidtune_info->songs();i++)
                    if (browser_sidtune_title[i]) free(browser_sidtune_title[i]);
                free(browser_sidtune_title);
                browser_sidtune_title=NULL;
            }
            if (browser_sidtune_name) {
                for (int i=0;i<sidtune_info->songs();i++)
                    if (browser_sidtune_name[i]) free(browser_sidtune_name[i]);
                free(browser_sidtune_name);
                browser_sidtune_name=NULL;
            }
            if (mSidTune) {delete mSidTune;mSidTune=NULL;}
        }
    } else if (browseType==2) { //GME Multisongs
        // Open music file in new emulator
        Music_Emu* gme_emu;
        gme_emu=NULL;
        gme_err_t gme_err=gme_open_file( [cpath UTF8String], &gme_emu, gme_info_only );
        if (gme_err) {
            MDZELog("gme_open_file error: %s",gme_err);
        } else {
            gme_info_t *gme_info;
            
            //is a m3u available ?
            NSString *tmpStr=[NSString stringWithFormat:@"%@.m3u",[cpath stringByDeletingPathExtension]];
            gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
            if (gme_err) {
                NSString *tmpStr=[NSString stringWithFormat:@"%@.M3U",[cpath stringByDeletingPathExtension]];
                gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
            }
            
            
            int total_trackNb=gme_track_count( gme_emu );
            for (int i=0;i<total_trackNb;i++) {
                //err=gme_start_track( gme_emu, i );
                //if (!err) {
                if (gme_track_info( gme_emu, &gme_info, i )==0) {
                    file=nil;
                    if (gme_info->song) {
                        if (gme_info->song[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,gme_info->song];
                    }
                    if (!file) {
                        if (gme_info->game) {
                            if (gme_info->game[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,gme_info->game];
                        }
                    }
                    if (!file) {
                        file=[NSString stringWithFormat:@"%.3d-%@",i+1,[cpath lastPathComponent]];
                    }
                    
                    int filtered=0;
                    if ((mSearch)&&([mSearchText length]>0)) {
                        filtered=1;
                        //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        //if (r.location != NSNotFound) {
                        if ([self searchStringRegExp:mSearchText sourceString:file]) {
                            /*if(r.location== 0)*/ filtered=0;
                        }
                    }
                    if (!filtered) {
                        local_entries_count++;
                        local_nb_entries++;
                    }
                    gme_free_info(gme_info);
                }
                //}
            }
            gme_delete(gme_emu);
        }
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries,sizeof(t_local_browse_entry));
            if (!local_entries_data) {
                //Not enough memory
                //try to allocate less entries
                local_nb_entries_limit=LIMITED_LIST_SIZE;
                if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries_limit,sizeof(t_local_browse_entry));
                if (local_entries_data==NULL) {
                    //show alert : cannot list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem.",@"")];
                } else {
                    //show alert : limited list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                if (local_entries_count) {
                    if (local_entries_index+local_entries_count>local_nb_entries) {
                        local_entries_count=local_nb_entries-local_entries_index;
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    } else {
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    }
                }
                
                gme_err_t gme_err=gme_open_file( [cpath UTF8String], &gme_emu, gme_info_only );
                if (gme_err) {
                    MDZELog("gme_open_file error: %s",gme_err);
                } else {
                    gme_info_t *gme_info;
                    
                    //is a m3u available ?
                    NSString *tmpStr=[NSString stringWithFormat:@"%@.m3u",[cpath stringByDeletingPathExtension]];
                    gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
                    if (gme_err) {
                        NSString *tmpStr=[NSString stringWithFormat:@"%@.M3U",[cpath stringByDeletingPathExtension]];
                        gme_err=gme_load_m3u(gme_emu,[tmpStr UTF8String] );
                    }
                    
                    for (int i=0;i<gme_track_count( gme_emu );i++) {
                        if (gme_track_info( gme_emu, &gme_info, i )==0) {
                            file=nil;
                            if (gme_info->song) {
                                if (gme_info->song[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,gme_info->song];
                            }
                            if (!file) {
                                if (gme_info->game) {
                                    if (gme_info->game[0]) file=[NSString stringWithFormat:@"%.3d-%s",i+1,gme_info->game];
                                }
                            }
                            if (!file) {
                                file=[NSString stringWithFormat:@"%.3d-%@",i+1,[cpath lastPathComponent]];
                            }
                            
                            int filtered=0;
                            if ((mSearch)&&([mSearchText length]>0)) {
                                filtered=1;
                                //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                    //if (r.location != NSNotFound) {
                                    /*if(r.location== 0)*/ filtered=0;
                                }
                            }
                            if (!filtered) {
                                
                                local_entries[local_entries_count].type=1;
                                local_entries[local_entries_count].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                local_entries[local_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@?%d",currentPath,i];
                                
                                local_entries[local_entries_count].rating=0;
                                local_entries[local_entries_count].playcount=0;
                                local_entries[local_entries_count].song_length=0;
                                local_entries[local_entries_count].songs=1;//0;
                                local_entries[local_entries_count].channels_nb=0;
                                
                                snprintf(sqlStatement,1024,"SELECT play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[local_entries[local_entries_count].fullpath UTF8String]);
                                err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                if (err==SQLITE_OK){
                                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                                        signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                        if (rating<0) rating=0;
                                        if (rating>5) rating=5;
                                        if ((rating==0)&&(sqlite3_column_type(stmt,5)!=SQLITE_NULL)) {
                                            rating=(signed char)sqlite3_column_int(stmt, 5);
                                            if (rating) rating=1;
                                        }
                                        local_entries[local_entries_count].playcount=(short int)sqlite3_column_int(stmt, 0);
                                        local_entries[local_entries_count].rating=rating;
                                        local_entries[local_entries_count].song_length=(int)sqlite3_column_int(stmt, 2);
                                        local_entries[local_entries_count].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                    }
                                    sqlite3_finalize(stmt);
                                } else MDZELog("ErrSQL : %d",err);
                                
                                local_entries_count++;
                                
                                if (local_nb_entries_limit) {
                                    local_nb_entries_limit--;
                                    if (!local_nb_entries_limit) shouldStop=1;
                                }
                                
                            }
                            gme_free_info(gme_info);
                        }
                    }
                    gme_delete(gme_emu);
                }
            }
        }
    } else if (browseType==1) { //Archive
        char **archive_entries;
        int archive_entries_count;
        int found=[ModizFileHelper scanarchive:[cpath UTF8String] filesList_ptr:&archive_entries filesCount_ptr:&archive_entries_count];
        int file_idx=0;
        
        if (found) {
            
            //sort the file list
            qsort(archive_entries, archive_entries_count, sizeof(char*), &qsort_CompareArcEntries);
            
            
            NSString *extension=[[[cpath lastPathComponent] pathExtension] uppercaseString];
            if ( !is_rsn && ([extension caseInsensitiveCompare:@"rsn"]==NSOrderedSame) ) {
                is_rsn=1;
                
                dispatch_async(dispatch_get_main_queue(), ^(void){
                    //Run UI Updates
                    [self.tableView setUserInteractionEnabled:false];
                    [self.navigationItem setHidesBackButton:YES animated:YES];
                });
                
                extractProgress = [NSProgress progressWithTotalUnitCount:1];
                extractProgress.cancellable = YES;
                extractProgress.pausable = NO;
                NSString *tmpPath=[NSString stringWithFormat:@"%@/tmpArchiveBrowser",NSTemporaryDirectory()];
                [mFileMngr removeItemAtPath:tmpPath error:NULL];
                [ModizFileHelper extractToPath:[cpath UTF8String] path:[tmpPath UTF8String] caller:self progress:extractProgress context:ExtractBrowserListProgressObserverContext completion:^(BOOL success,NSError *err) {
                    
                    dispatch_async(dispatch_get_main_queue(), ^(void){
                        [self.tableView reloadData];
                        [self.tableView layoutIfNeeded];
                    });
                }];
            }
            
            while (file_idx<found) {
                
                file=[NSString stringWithUTF8String:archive_entries[file_idx]];
                
                NSString *extension;
                NSString *file_no_ext;
                
                NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                extension = (NSString *)[temparray_filepath lastObject];
                file_no_ext=[temparray_filepath firstObject];
                
                int filtered=0;
                if ((mSearch)&&([mSearchText length]>0)) {
                    filtered=1;
                    if ([self searchStringRegExp:mSearchText sourceString:file]) {
                        filtered=0;
                    }
                }
                if (!filtered) {
                    int found=0;
                    
                    if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                    else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                    
                    if (found)  {
                        local_entries_count++;
                        local_nb_entries++;
                    }
                }
                file_idx++;
            }
        } else {
        }
        
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries,sizeof(t_local_browse_entry));
            if (!local_entries_data) {
                //Not enough memory
                //try to allocate less entries
                local_nb_entries_limit=LIMITED_LIST_SIZE;
                if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries_limit,sizeof(t_local_browse_entry));
                if (local_entries_data==NULL) {
                    //show alert : cannot list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem.",@"")];
                } else {
                    //show alert : limited list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                if (local_entries_count) {
                    if (local_entries_index+local_entries_count>local_nb_entries) {
                        local_entries_count=local_nb_entries-local_entries_index;
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    } else {
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    }
                }
                
                file_idx=0;
                if (found) {
                    int arc_counter=0;
                    //while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
                    while (file_idx<found) {
                        //file=[ModizFileHelper getCorrectFileName:[cpath UTF8String] archive:a entry:entry];
                        file=[NSString stringWithUTF8String:archive_entries[file_idx]];
                        
                        NSString *extension;// = [[file pathExtension] uppercaseString];
                        NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                        
                        NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                        extension = (NSString *)[temparray_filepath lastObject];
                        file_no_ext=[temparray_filepath firstObject];
                        
                        
                        int filtered=0;
                        if ((mSearch)&&([mSearchText length]>0)) {
                            filtered=1;
                            if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                filtered=0;
                            }
                        }
                        if (!filtered) {
                            int found=0;
                            
                            if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                            else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                            
                            if (found)  {
                                const char *str;
                                char tmp_str[1024];//,*tmp_convstr;
                                int other_encoding=0;
                                str=[[file lastPathComponent] UTF8String];
                                if ([extension caseInsensitiveCompare:@"mdx"]==NSOrderedSame ) {
                                    [[file lastPathComponent] getFileSystemRepresentation:tmp_str maxLength:1024];
                                    //tmp_convstr=mdx_make_sjis_to_syscharset(tmp_str);
                                    other_encoding=1;
                                }
                                local_entries[local_entries_count].type=1;
                                //NOT Supported: archive or multisongs in an archive
                                
                                if (other_encoding) {
                                    local_entries[local_entries_count].label=[[NSString alloc ] initWithCString:tmp_str encoding:NSUTF8StringEncoding];
                                    //	free(tmp_convstr);
                                } else local_entries[local_entries_count].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                
                                local_entries[local_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@@%d",currentPath,arc_counter];
                                
                                local_entries[local_entries_count].rating=0;
                                local_entries[local_entries_count].playcount=0;
                                local_entries[local_entries_count].song_length=0;
                                local_entries[local_entries_count].songs=0;
                                local_entries[local_entries_count].channels_nb=0;
                                
                                snprintf(sqlStatement,1024,"SELECT play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[local_entries[local_entries_count].fullpath UTF8String]);
                                err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                if (err==SQLITE_OK){
                                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                                        signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                        if (rating<0) rating=0;
                                        if (rating>5) rating=5;
                                        if ((rating==0)&&(sqlite3_column_type(stmt,5)!=SQLITE_NULL)) {
                                            rating=(signed char)sqlite3_column_int(stmt, 5);
                                            if (rating) rating=1;
                                        }
                                        local_entries[local_entries_count].playcount=(short int)sqlite3_column_int(stmt, 0);
                                        local_entries[local_entries_count].rating=rating;
                                        local_entries[local_entries_count].song_length=(int)sqlite3_column_int(stmt, 2);
                                        local_entries[local_entries_count].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                        local_entries[local_entries_count].songs=(int)sqlite3_column_int(stmt, 4);
                                    }
                                    sqlite3_finalize(stmt);
                                } else MDZELog("ErrSQL : %d",err);
                                
                                local_entries_count++;
                                arc_counter++;
                                
                                if (local_nb_entries_limit) {
                                    local_nb_entries_limit--;
                                    if (!local_nb_entries_limit) shouldStop=1;
                                }
                            }
                        }
                        file_idx++;
                    }
                }
            }
            
        }
    } else {
        clock_t start_time,end_time;
        start_time=clock();
        NSError *error;
        NSRange rdir;
        NSArray *dirContent;//=[NSMutableArray array];//
        BOOL isDir;
        
        //List all entries
        NSURL *directoryURL = [NSURL fileURLWithPath:cpath];
        NSDirectoryEnumerator *directoryEnumerator =
        [mFileMngr enumeratorAtURL:directoryURL
        includingPropertiesForKeys:@[NSURLPathKey, NSURLNameKey, NSURLIsDirectoryKey]
                           options:NSDirectoryEnumerationSkipsHiddenFiles|(mShowSubdir?0:NSDirectoryEnumerationSkipsSubdirectoryDescendants)
                      errorHandler:nil];
        
        /*for (NSURL *fileURL in directoryEnumerator) {
         [dirContent addObject:fileURL];
         }*/
        dirContent=[directoryEnumerator allObjects];
        
        //if (mShowSubdir) dirContent=[mFileMngr subpathsOfDirectoryAtPath:cpath error:&error];
        //else dirContent=[mFileMngr contentsOfDirectoryAtPath:cpath error:&error];
        
        //NSArray *sortedDirContent = [dirContent sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
        NSArray *sortedDirContent = [dirContent sortedArrayUsingComparator:^(id obj1, id obj2) {
            
            NSString *str1;//[(NSString *)obj1 lastPathComponent];
            NSString *str2;//[(NSString *)obj2 lastPathComponent];
            
            if (mShowSubdir==2) { //use path
                [(NSURL*)obj1 getResourceValue:&str1 forKey:NSURLPathKey error:nil];
                [(NSURL*)obj2 getResourceValue:&str2 forKey:NSURLPathKey error:nil];
            } else { //use filename
                [(NSURL*)obj1 getResourceValue:&str1 forKey:NSURLNameKey error:nil];
                [(NSURL*)obj2 getResourceValue:&str2 forKey:NSURLNameKey error:nil];
            }
            return [str1 caseInsensitiveCompare:str2];
        }];
        
        int file_idx=0;
        int file_cnt=[sortedDirContent count];
        for (fileURL in sortedDirContent) {
            //[mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
            NSNumber *isDirectory = nil;
            [fileURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
            [fileURL getResourceValue:&file forKey:NSURLPathKey error:nil];
            
            
            rdir=[file rangeOfString:cpath];
            if (rdir.location!=NSNotFound) {
                file=[file substringFromIndex:(rdir.location+rdir.length+1)];
            }
            
            isDir=[isDirectory boolValue];
            
            if (isDir) { //rdir.location == NSNotFound) {  //assume it is a dir if no "." in file name
                if (![file isEqualToString:@"ProjectM"]) {
                    rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                    if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                        if (1/*[file compare:@"tmpArchive"]!=NSOrderedSame*/) {
                            //do not display dir if subdir mode is on
                            int filtered=mShowSubdir;
                            if (!filtered) {
                                if ((mSearch)&&([mSearchText length]>0)) {
                                    filtered=1;
                                    //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                    //if (r.location != NSNotFound) {
                                    if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                        /*if(r.location== 0)*/ filtered=0;
                                    }
                                }
                                if (!filtered) {
                                    local_entries_count++;
                                    local_nb_entries++;
                                }
                            }
                        }
                    }
                }
            } else {
                rdir.location=NSNotFound;
                rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                    NSString *extension;// = [[file pathExtension] uppercaseString];
                    NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                    extension = (NSString *)[temparray_filepath lastObject];
                    //[temparray_filepath removeLastObject];
                    file_no_ext=[temparray_filepath firstObject];
                    
                    
                    int filtered=0;
                    if ((mSearch)&&([mSearchText length]>0)) {
                        filtered=1;
                        //NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        //if (r.location != NSNotFound) {
                        if ([self searchStringRegExp:mSearchText sourceString:file]) {
                            /*if(r.location== 0)*/ filtered=0;
                        }
                    }
                    if (!filtered) {
                        int found=0;
                        if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                        else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                        else if ([archivetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                        
                        if (found)  {
                            local_entries_count++;
                            local_nb_entries++;
                        }
                    }
                }
            }
        }
        
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries,sizeof(t_local_browse_entry));
            
            if (!local_entries_data) {
                //Not enough memory
                //try to allocate less entries
                local_nb_entries_limit=LIMITED_LIST_SIZE;
                if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                local_entries_data=(t_local_browse_entry *)calloc(local_nb_entries_limit,sizeof(t_local_browse_entry));
                if (local_entries_data==NULL) {
                    //show alert : cannot list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem.",@"")];
                } else {
                    //show alert : limited list
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Browser not enough mem. Limited.",@"")];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                if (local_entries_count) {
                    if (local_entries_index+local_entries_count>local_nb_entries) {
                        local_entries_count=local_nb_entries-local_entries_index;
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    } else {
                        local_entries=&(local_entries_data[local_entries_index]);
                        local_entries_index+=local_entries_count;
                        local_entries_count=0;
                    }
                }
                
                // Second check count for each section
                for (fileURL in sortedDirContent) {
                    if (shouldStop) break;
                    NSNumber *isDirectory = nil;
                    [fileURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
                    [fileURL getResourceValue:&file forKey:NSURLPathKey error:nil];
                    rdir=[file rangeOfString:cpath];
                    if (rdir.location!=NSNotFound) {
                        file=[file substringFromIndex:(rdir.location+rdir.length+1)];
                    }
                    isDir=[isDirectory boolValue];
                    
                    
                    if (isDir) { //rdir.location == NSNotFound) {  //assume it is a dir if no "." in file name
                        if (![file isEqualToString:@"ProjectM"]) {
                            rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                            if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                                if (1/*[file compare:@"tmpArchive"]!=NSOrderedSame*/) {
                                    //do not display dir if subdir mode is on
                                    int filtered=mShowSubdir;
                                    if (!filtered) {
                                        if ((mSearch)&&([mSearchText length]>0)) {
                                            filtered=1;
                                            //NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                            //if (r.location != NSNotFound) {
                                            if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                                /*if(r.location== 0)*/ filtered=0;
                                            }
                                        }
                                        if (!filtered) {
                                            local_entries[local_entries_count].type=0;
                                            
                                            local_entries[local_entries_count].label=[[NSString alloc] initWithString:file];
                                            
                                            local_entries[local_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@/%@",currentPath,file];
                                            
                                            //check for cover
                                            NSString *fpath=[ModizFileHelper getFullPathForFilePath:local_entries[local_entries_count].fullpath];
                                            NSString *imgPath=[fpath stringByAppendingString:@"/folder.png"];
                                            bool imgExist=false;
                                            if (![mFileMngr fileExistsAtPath:imgPath]) {
                                                imgPath=[fpath stringByAppendingString:@"/folder.webp"];
                                                if (![mFileMngr fileExistsAtPath:imgPath]) {
                                                    imgPath=[fpath stringByAppendingString:@"/folder.jpg"];
                                                    if (![mFileMngr fileExistsAtPath:imgPath]) {
                                                        imgPath=[fpath stringByAppendingString:@"/folder.jpeg"];
                                                        if (![mFileMngr fileExistsAtPath:imgPath]) {
                                                            imgPath=[fpath stringByAppendingString:@"/folder.gif"];
                                                            if ([mFileMngr fileExistsAtPath:imgPath]) imgExist=true;
                                                        } else imgExist=true;
                                                    } else imgExist=true;
                                                } else imgExist=true;
                                            } else imgExist=true;
                                            if (imgExist) local_entries[local_entries_count].imgpath=[NSString stringWithString:imgPath];
                                            else local_entries[local_entries_count].imgpath=nil;
                                            
                                            local_entries_count++;
                                            if (local_nb_entries_limit) {
                                                local_nb_entries_limit--;
                                                if (!local_nb_entries_limit) shouldStop=1;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        rdir.location=NSNotFound;
                        rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                        if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                            NSString *extension;// = [[file pathExtension] uppercaseString];
                            NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                            
                            NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                            extension = (NSString *)[temparray_filepath lastObject];
                            //[temparray_filepath removeLastObject];
                            file_no_ext=[temparray_filepath firstObject];
                            
                            int filtered=0;
                            if ((mSearch)&&([mSearchText length]>0)) {
                                filtered=1;
                                //NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                //if (r.location != NSNotFound) {
                                if ([self searchStringRegExp:mSearchText sourceString:file]) {
                                    /*if(r.location== 0)*/ filtered=0;
                                }
                            }
                            if (!filtered) {
                                int found=0;
                                
                                if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                                else if ([filetype_extAMIGA indexOfObject:file_no_ext]!=NSNotFound) found=1;
                                else if ([archivetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                                
                                
                                if (found)  {
                                    char tmp_str[1024];//,*tmp_convstr;
                                    int other_encoding=0;
                                    
                                    if ([extension caseInsensitiveCompare:@"mdx"]==NSOrderedSame ) {
                                        [[file lastPathComponent] getFileSystemRepresentation:tmp_str maxLength:1024];
                                        //tmp_convstr=mdx_make_sjis_to_syscharset(tmp_str);
                                        other_encoding=1;
                                    }
                                    local_entries[local_entries_count].type=1;
                                    //check if Archive file
                                    if ([archivetype_ext indexOfObject:extension]!=NSNotFound) { //check if really an archive
                                        local_entries[local_entries_count].type=2|16;  //16 is to flag them as to check before displaying entry in tabiew
                                    } else if ([all_multisongstype_ext indexOfObject:extension]!=NSNotFound) { //check if Multisongs file
                                        local_entries[local_entries_count].type=3|16;  //16 is to flag them as to check before displaying entry in tabiew
                                        
                                    }
                                    
                                    if (other_encoding) {
                                        local_entries[local_entries_count].label=[[NSString alloc] initWithCString:tmp_str encoding:NSUTF8StringEncoding];
                                        //	free(tmp_convstr);
                                    } else local_entries[local_entries_count].label=[[NSString alloc] initWithString:[file lastPathComponent]];
                                    
                                    local_entries[local_entries_count].fullpath=[[NSString alloc] initWithFormat:@"%@/%@",currentPath,file];
                                    
                                    local_entries[local_entries_count].rating=-1;
                                    local_entries[local_entries_count].playcount=-1;
                                    local_entries[local_entries_count].song_length=-1;
                                    local_entries[local_entries_count].songs=-1;
                                    local_entries[local_entries_count].channels_nb=-1;
                                    
                                    //check for cover
                                    NSString *fpath=[ModizFileHelper getFullPathForFilePath:local_entries[local_entries_count].fullpath];
                                    NSString *imgPath=[[fpath stringByDeletingPathExtension] stringByAppendingString:@".png"];
                                    bool imgExist=false;
                                    if (![mFileMngr fileExistsAtPath:imgPath]) {
                                        imgPath=[[fpath stringByDeletingPathExtension] stringByAppendingString:@".webp"];
                                        if (![mFileMngr fileExistsAtPath:imgPath]) {
                                            imgPath=[[fpath stringByDeletingPathExtension] stringByAppendingString:@".jpg"];
                                            if (![mFileMngr fileExistsAtPath:imgPath]) {
                                                imgPath=[[fpath stringByDeletingPathExtension] stringByAppendingString:@".jpeg"];
                                                if (![mFileMngr fileExistsAtPath:imgPath]) {
                                                    imgPath=[[fpath stringByDeletingPathExtension] stringByAppendingString:@".gif"];
                                                    if ([mFileMngr fileExistsAtPath:imgPath]) imgExist=true;
                                                } else imgExist=true;
                                            } else imgExist=true;
                                        } else imgExist=true;
                                    } else imgExist=true;
                                    if (imgExist) local_entries[local_entries_count].imgpath=[NSString stringWithString:imgPath];
                                    else local_entries[local_entries_count].imgpath=nil;
                                    
                                    local_entries_count++;
                                    
                                    if (local_nb_entries_limit) {
                                        local_nb_entries_limit--;
                                        if (!local_nb_entries_limit) shouldStop=1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    if (db) {
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
    }
    
    no_reentrant=false;
    return;
}

-(void) traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) {
        forceReloadCells=true;
        
        //[self addRefreshView];
    }
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    [self.tableView reloadData];
    //[[[self navigationController] navigationBar] setBarStyle:UIBarStyleDefault];
}

static int shouldRestart=1;

- (UIStatusBarStyle)preferredStatusBarStyle {
    return UIStatusBarStyleDefault;
}

-(void) viewWillAppear:(BOOL)animated {
    //static int firstcall=0;
    [super viewWillAppear:animated];

    shouldFillKeys=1;
    
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    
    [self.sBar setBarStyle:UIBarStyleDefault];
    
    self.navigationController.delegate = self;
    [[self navigationController] setNavigationBarHidden:NO animated:YES];
    [self.navigationController setNeedsStatusBarAppearanceUpdate];
    
    
    if ([detailViewController mPlaylist_size]>0) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    } else {
        wasMiniPlayerOn=false;
        [self hideMiniPlayer];
    }
    
    /////////////
    //shouldFillKeys=1;
    
    [waitingViewPlayer resetCancelStatus];
    waitingViewPlayer.hidden=detailViewController.waitingView.hidden;
    waitingViewPlayer.btnStopCurrentAction.hidden=detailViewController.waitingView.btnStopCurrentAction.hidden;
    waitingViewPlayer.progressView.progress=detailViewController.waitingView.progressView.progress;
    waitingViewPlayer.progressView.hidden=detailViewController.waitingView.progressView.hidden;
    waitingViewPlayer.lblTitle.text=[NSString stringWithString:detailViewController.waitingView.lblTitle.text];
    waitingViewPlayer.lblDetail.text=[NSString stringWithString:detailViewController.waitingView.lblDetail.text];
        
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView addObserver:self
                                       forKeyPath:observedSelector
                                          options:NSKeyValueObservingOptionInitial
                                          context:LoadingProgressObserverContext];
    
    repeatingTimer = [NSTimer scheduledTimerWithTimeInterval: 0.20f target:self selector:@selector(updateLoadingInfos:) userInfo:nil repeats: YES]; //5 times/second
    
    if (childController) {
        childController = nil;
    }
    
    if (1/*shouldFillKeys*/) {
        //Reset rating if applicable (ensure updated value)
        if (local_nb_entries) {
            for (int i=0;i<local_nb_entries;i++) {
                local_entries_data[i].rating=-1;
                local_entries_data[i].playcount=-1;
                local_entries_data[i].song_length=-1;
                local_entries_data[i].songs=-1;
                local_entries_data[i].channels_nb=-1;
            }
        }
        if (search_local_nb_entries) {
            for (int i=0;i<search_local_nb_entries;i++) {
                search_local_entries_data[i].rating=-1;
                search_local_entries_data[i].playcount=-1;
                search_local_entries_data[i].song_length=-1;
                search_local_entries_data[i].songs=-1;
                search_local_entries_data[i].channels_nb=-1;
            }
        }
    }
    
    //creating view for extending background color
    //[self addRefreshView];
    
    
    //check if a pending cut/paste exists
    if (cutpaste_initiated&&(cutpaste_filesrcpath==nil)) {
        //file has been moved, force reload
        shouldFillKeys=1;
        cutpaste_initiated=0;
    }
    
    if (shouldFillKeys) [self refreshViewReloadFiles];
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
    
}

-(void) resetCachedStats {
    for (int i=0;i<local_nb_entries;i++) {
        local_entries_data[i].rating=-1;
    }
    for (int i=0;i<search_local_nb_entries;i++) {
        search_local_entries_data[i].rating=-1;
    }
}

-(void) refreshViewReloadFiles {
    
    //if (self.tableView.refreshControl.refreshing) return;
    if (tabViewRefresh) return;
    
    if (childController) [(RootViewControllerLocalBrowser*) childController refreshViewAfterDownload];
    else {
        //if (self.tableView.refreshControl.refreshing==false) {
        if (!tabViewRefresh) {
            // Sauvegarder la position AVANT beginRefreshing
            tabViewRefresh=1;
//            CGPoint savedOffset = self.tableView.contentOffset;
//
//            [self.tableView.refreshControl beginRefreshing];
//
//            // Restaurer APRÈS dans le prochain runloop
//            dispatch_async(dispatch_get_main_queue(), ^{
//                [self.tableView setContentOffset:savedOffset animated:NO];
//            });
        }
        
        //self.sBar.enabled=false;//disable search bar
        
        [self hideWaitingCancel];
        [self hideWaitingProgress];
        [self updateWaitingTitle:@""];
        [self updateWaitingDetail:@""];
        
        [self showWaiting];
        
        //        [self flushMainLoop];
        //        [NSThread sleepForTimeInterval:0.1f];
        //        [self flushMainLoop];
        
        shouldFillKeys=1;
        
        dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
            //Background Thread
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
            
            
            dispatch_async(dispatch_get_main_queue(), ^(void){
                //Run UI Updates
                [self.tableView.refreshControl endRefreshing];
                tabViewRefresh=0;
                [self hideWaiting];
                [self.tableView reloadData];
                
                //self.sBar.enabled=true;//enable search bar
            });
        });
    }
}

-(void) refreshViewAfterDownload {
    
    //do not force refresh when download complete, can block UI
    //[self refreshViewReloadFiles];
}

-(void)modizerIsLaunched {
    if (shouldRestart) {
        
        self.view.userInteractionEnabled = NO;
        //self.view.alpha=0.5f;
        
        [self hideWaitingCancel];
        [self hideWaitingProgress];
        [self updateWaitingTitle:NSLocalizedString(@"Loading",@"")];
        [self updateWaitingDetail:NSLocalizedString(@"Resuming last\nplayed file",@"")];
        [self showWaiting];
        shouldRestart=0;
        
        
        dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void){
            //Background Thread
            dispatch_async(dispatch_get_main_queue(), ^(void){
                //Run UI Updates
                [detailViewController play_restart];
            });
            
            dispatch_async(dispatch_get_main_queue(), ^(void){
                //Run UI Updates
                [self hideWaiting];
                self.view.userInteractionEnabled = YES;
            });
        });
        
        //        detailViewController.no_hud_mode=true;
        //        [detailViewController play_restart];
        //        [detailViewController play_restart];
        //        detailViewController.no_hud_mode=false;
        
        //        [self hideWaiting];
        //        self.view.userInteractionEnabled = YES;
    }
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    forceReloadCells=false;
    
    
    
    if ([detailViewController not_expected_version]==1) {
        MDZILog("change of version");
        //detailViewController.not_expected_version=0;
#if 0
        UIAlertController *alertC = [UIAlertController alertControllerWithTitle:[NSString stringWithFormat:NSLocalizedString(@"Modizer v%d.%d",@""),VERSION_MAJOR,VERSION_MINOR]
                                                                        message:NSLocalizedString(@"\
Due to internal changes, settings will be reseted to default and database will be cleaned.\n\
As a consequence, some entries might disappear from existing playlist.\n\
\n\
",@"")
                                                                 preferredStyle:UIAlertControllerStyleAlert];
        UIAlertAction* closeAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Ok",@"") style:UIAlertActionStyleCancel
                                                            handler:^(UIAlertAction * action) {
            
//            [self updateWaitingTitle:NSLocalizedString(@"Cleaning DB & \nreseting settings",@"")];
            [self updateWaitingDetail:NSLocalizedString(@"please wait",@"")];
            
            [self showWaiting];
            [self flushMainLoop];
            
            DBHelper::cleanDB();
            
#if 0
            //remove settings from userpref
            NSString *appDomain = [[NSBundle mainBundle] bundleIdentifier];
            [[NSUserDefaults standardUserDefaults] removePersistentDomainForName:appDomain];
            //
            //load default
            [SettingsGenViewController applyDefaultSettings];
            [detailViewController settingsChanged:(int)SETTINGS_ALL];
#endif
            detailViewController.not_expected_version=0;
            
            [self hideWaiting];
            
        }];
        [alertC addAction:closeAction];
        
        if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone) { //if iPhone
            [self presentViewController:alertC animated:YES completion:nil];
        } else { //if iPad
            alertC.modalPresentationStyle = UIModalPresentationPopover;
            alertC.popoverPresentationController.sourceView = self.view;
            alertC.popoverPresentationController.sourceRect = CGRectMake(self.view.frame.size.width/3, self.view.frame.size.height/2, 0, 0);
            alertC.popoverPresentationController.permittedArrowDirections=0;
            [self presentViewController:alertC animated:YES completion:nil];
        }
#endif
    }
    
    
    //Faster loading of player view for debug
#ifdef DEBUG_MODIZER
    [self modizerIsLaunched];
    //[self goPlayer];
#else
#endif

}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated {
    //[self hideWaiting];
    [self hideMiniPlayer];
    /*if (childController) {
     [childController viewDidDisappear:FALSE];
     }*/
    
    [repeatingTimer invalidate];
    repeatingTimer = nil;
    
    NSString *observedSelector = NSStringFromSelector(@selector(hidden));
    [detailViewController.waitingView removeObserver:self
                                          forKeyPath:observedSelector
                                             context:LoadingProgressObserverContext];
    
    [super viewDidDisappear:animated];
}

- (void)viewWillTransitionToSize:(CGSize)size withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator {
    
    [tableView reloadData];
    [miniplayerVC viewWillTransitionToSize:size withTransitionCoordinator:coordinator];
}


//- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
//    [tableView reloadData];
//}
//
//// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
//- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
//    [tableView reloadData];
//    return YES;
//}

#pragma mark - Table view data source

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
    return 0;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    return nil;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 2;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section==0) {
        if (browse_depth==0) return 2;
        else return 1;
    }
    return (search_local?search_local_entries_count:local_entries_count);
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    return nil;
}

- (NSInteger)tableView:(UITableView *)tabView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    return -1;
}

- (UITableViewCell *) tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
    static NSString *CellIdentifierHeader = @"CellH";
    NSString *cellValue;
    const NSInteger TOP_LABEL_TAG = 1001;
    const NSInteger BOTTOM_LABEL_TAG = 1002;
    const NSInteger BOTTOM_IMAGE_TAG = 1003;
    const NSInteger ACT_IMAGE_TAG = 1004;
    const NSInteger SECACT_IMAGE_TAG = 1005;
    const NSInteger COVER_IMAGE_TAG = 1006;
    //UILabel *topLabel;
    CBAutoScrollLabel *topLabel;
    UILabel *bottomLabel;
    UIImageView *bottomImageView,*coverImgView;
    UIButton *actionView,*secActionView;
    
    
    t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
    
    UITableViewCell *cell;
    
    if (forceReloadCells) {
        while ([tabView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        while ([tabView dequeueReusableCellWithIdentifier:CellIdentifierHeader]) {}
        forceReloadCells=false;
    }
    
    if (indexPath.section==0) cell = (UITableViewCell *)[tabView dequeueReusableCellWithIdentifier:CellIdentifierHeader];
    else cell = (UITableViewCell *)[tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    if (cell == nil) {
        if (indexPath.section>1) {
            cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:    CellIdentifier];
            
            //if (self.tableView.refreshControl.isRefreshing==false) {
            if (!tabViewRefresh) {
                
                if ((cur_local_entries[indexPath.row].type&16)&&((cur_local_entries[indexPath.row].type&15)==2)) { //need to confirm if true archive
                    
                    if ([ModizFileHelper isABrowsableArchive:[ModizFileHelper getFullPathForFilePath:cur_local_entries[indexPath.row].fullpath]]) cur_local_entries[indexPath.row].type=2;
                    else cur_local_entries[indexPath.row].type=1;
                }
            }
        } else {
            cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:    CellIdentifierHeader];
        }
        
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,40);
        [cell setBackgroundColor:[UIColor clearColor]];
        
        UIBackgroundConfiguration *backgroundConfig = [UIBackgroundConfiguration listGroupedCellConfiguration];
        backgroundConfig.backgroundColor = [UIColor systemGroupedBackgroundColor];
        cell.backgroundConfiguration = backgroundConfig;
        
        //
        // Create the label for the top row of text
        //
        //topLabel = [[UILabel alloc] init];
        topLabel = [[CBAutoScrollLabel alloc] init];
        topLabel.labelSpacing = 35; // distance between start and end labels
        topLabel.pauseInterval = 3.7; // seconds of pause before scrolling starts again
        topLabel.scrollSpeed = 30; // pixels per second
        topLabel.fadeLength = 12.f; // length of the left and right edge fade, 0 to disable
        
        [cell.contentView addSubview:topLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        topLabel.tag = TOP_LABEL_TAG;
        topLabel.backgroundColor = [UIColor clearColor];

        topLabel.font = [UIFont systemFontOfSize:17 weight:MDZ_UIFONT_WEIGHT];
//        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
//                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
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
                                   ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
        bottomLabel.opaque=TRUE;
        
        bottomImageView = [[UIImageView alloc] initWithImage:nil];
        bottomImageView.frame = CGRectMake(1.0*cell.indentationWidth,
                                           22,
                                           14,14);
        bottomImageView.tag = BOTTOM_IMAGE_TAG;
        bottomImageView.opaque=TRUE;
        [cell.contentView addSubview:bottomImageView];
        
        coverImgView=[[UIImageView alloc] initWithImage:nil];
        coverImgView.frame= CGRectMake(0,1,34,34);
        coverImgView.contentMode=UIViewContentModeScaleAspectFit;
        coverImgView.tag = COVER_IMAGE_TAG;
        coverImgView.opaque=TRUE;
        [cell.contentView addSubview:coverImgView];
        
        actionView                = [UIButton buttonWithType: UIButtonTypeCustom];
        [cell.contentView addSubview:actionView];
        actionView.tag = ACT_IMAGE_TAG;
        
        secActionView                = [UIButton buttonWithType: UIButtonTypeCustom];
        [cell.contentView addSubview:secActionView];
        secActionView.tag = SECACT_IMAGE_TAG;
        
        cell.accessoryView=nil;
        //        cell.selectionStyle=UITableViewCellSelectionStyleGray;
    } else {
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,40);
        //topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        topLabel = (CBAutoScrollLabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        bottomImageView = (UIImageView *)[cell viewWithTag:BOTTOM_IMAGE_TAG];
        coverImgView = (UIImageView *)[cell viewWithTag:COVER_IMAGE_TAG];
        actionView = (UIButton *)[cell viewWithTag:ACT_IMAGE_TAG];
        secActionView = (UIButton *)[cell viewWithTag:SECACT_IMAGE_TAG];
        
//        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
//                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
        
        bottomLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
        
        if (indexPath.section>1) {
            
            //if (self.tableView.refreshControl.isRefreshing==false) {
            if (!tabViewRefresh) {
                if ((cur_local_entries[indexPath.row].type&16)&&((cur_local_entries[indexPath.row].type&15)==2)) { //need to confirm if true archive
                    if ([ModizFileHelper isABrowsableArchive:[ModizFileHelper getFullPathForFilePath:cur_local_entries[indexPath.row].fullpath]]) cur_local_entries[indexPath.row].type=2;
                    else cur_local_entries[indexPath.row].type=1;
                }
                
            }
        } else {
        }
    }
    float margin=MDZ_TABVIEW_SEPARATOR_MARGIN;
    cell.layoutMargins = UIEdgeInsetsMake(0, margin, 0, margin);
    cell.separatorInset = UIEdgeInsetsMake(0, margin, 0, margin);
    
    actionView.hidden=TRUE;
    secActionView.hidden=TRUE;
    
    [cell layoutIfNeeded];
    
    if (darkMode) {
        topLabel.textColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
//        topLabel.highlightedTextColor = [UIColor colorWithRed:1 green:1 blue:1 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.6 green:0.6 blue:0.6 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
    } else {
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
//        topLabel.highlightedTextColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.0];
    }
    
    
    
    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                               0,
                               tabView.bounds.size.width -1.0 * cell.indentationWidth/*- 32*/,
                               22);
    bottomLabel.frame = CGRectMake(1.0 * cell.indentationWidth,
                                   22,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth/*-32*/,
                                   18);
    bottomLabel.text=@""; //default value
    bottomImageView.image=nil;
    coverImgView.image=nil;
    
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    //if refresh in prg, wait a bit...
    //if (self.tableView.refreshControl.isRefreshing) return cell;
//    if (tabViewRefresh) return cell;
    
    // Set up the cell...
    if (indexPath.section==0){
        if (indexPath.row==0) {
            switch (mShowSubdir) {
                case 0:
                    cellValue=NSLocalizedString(@"DisplayAll_MainKey","");
                    bottomLabel.text=[NSString stringWithFormat:@"%@ %d entries",NSLocalizedString(@"DisplayAll_SubKey",""),(search_local?search_local_nb_entries:local_nb_entries)];
                    break;
                case 1:
                    cellValue=NSLocalizedString(@"DisplayAllPath_MainKey","");
                    bottomLabel.text=[NSString stringWithFormat:@"%@ %d entries",NSLocalizedString(@"DisplayAllPath_SubKey",""),(search_local?search_local_nb_entries:local_nb_entries)];
                    break;
                case 2:
                    cellValue=NSLocalizedString(@"DisplayDir_MainKey","");
                    bottomLabel.text=[NSString stringWithFormat:@"%@ %d entries",NSLocalizedString(@"DisplayDir_SubKey",""),(search_local?search_local_nb_entries:local_nb_entries)];
                    break;
            }
            
            
            
            bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                           22,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth/*-32*/-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                           18);
            
            if (darkMode) topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED_DARKMODE green:ACTION_COLOR_GREEN_DARKMODE blue:ACTION_COLOR_BLUE_DARKMODE alpha:1.0];
            else topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
            
            topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth/*- 32*/-PRI_SEC_ACTIONS_IMAGE_SIZE-4-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       22);
            
            [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateNormal];
            [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateHighlighted];
            [secActionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
            [secActionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            //secActionView.tag=indexPath.row*100+indexPath.section;
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[secActionView.description componentsSeparatedByString:@";"] firstObject]];
            
            [actionView setImage:[UIImage imageNamed:@"play_all.png"] forState:UIControlStateNormal];
            [actionView setImage:[UIImage imageNamed:@"play_all.png"] forState:UIControlStateHighlighted];
            [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
            [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            //actionView.tag=indexPath.row*100+indexPath.section;
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
            
            int icon_posx=tabView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.right-tabView.safeAreaInsets.left;
            //icon_posx-=32;
            actionView.frame = CGRectMake(icon_posx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
            actionView.enabled=YES;
            actionView.hidden=NO;
            secActionView.frame = CGRectMake(icon_posx-PRI_SEC_ACTIONS_IMAGE_SIZE-4,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
            secActionView.enabled=YES;
            secActionView.hidden=NO;
        } else {
            cellValue=NSLocalizedString(@"iCloud","");
            if (icloud_available) bottomLabel.text=NSLocalizedString(@"available",@"");
            else bottomLabel.text=NSLocalizedString(@"unavailable",@"");
            
            bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                           22,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth/*-32*/-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                           18);
            
            if (darkMode) topLabel.textColor=[UIColor colorWithRed:ICLOUD_COLOR_RED_DARKMODE green:ICLOUD_COLOR_GREEN_DARKMODE blue:ICLOUD_COLOR_BLUE_DARKMODE alpha:1.0];
            else topLabel.textColor=[UIColor colorWithRed:ICLOUD_COLOR_RED green:ICLOUD_COLOR_GREEN blue:ICLOUD_COLOR_BLUE alpha:1.0];
            
            topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth/*- 32*/-PRI_SEC_ACTIONS_IMAGE_SIZE-4-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       22);
            actionView.enabled=NO;
            actionView.hidden=YES;
            secActionView.enabled=NO;
            secActionView.hidden=YES;
        }
    } else {
        
        if (tabViewRefresh) return cell;
        
        cellValue=cur_local_entries[indexPath.row].label;
        
        if (is_rsn&& (cur_local_entries[indexPath.row].altlabel==nil)) {
            NSString *tmpFile=[NSString stringWithFormat:@"%@/tmpArchiveBrowser/%@",NSTemporaryDirectory(),cur_local_entries[indexPath.row].label];
            SPCTag tag;
            if ([SPCTagParser parseTagsFromFile:tmpFile tag:&tag]) {
                cur_local_entries[indexPath.row].altlabel=[NSString stringWithFormat:@"%.3d-%s",indexPath.row,tag.songName];
                [SPCTagParser freeTag:&tag]; // Libérer la mémoire
            }
        }
        
        if (cur_local_entries[indexPath.row].altlabel) cellValue=cur_local_entries[indexPath.row].altlabel;
        
        
        if (cur_local_entries[indexPath.row].type==0) { //directory
            if (darkMode) topLabel.textColor=[UIColor colorWithRed:MDZ_FOLDER_DARK_R green:MDZ_FOLDER_DARK_G blue:MDZ_FOLDER_DARK_B alpha:1.0f];
            else topLabel.textColor=[UIColor colorWithRed:MDZ_FOLDER_LIGHT_R green:MDZ_FOLDER_LIGHT_G blue:MDZ_FOLDER_LIGHT_B alpha:1.0f];
            //cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            
            bool hasImg=false;
            if (cur_local_entries[indexPath.row].imgpath) {
                coverImgView.image=[UIImage imageWithContentsOfFile:cur_local_entries[indexPath.row].imgpath];
                hasImg=true;
            }
            
            topLabel.frame= CGRectMake((hasImg?35:0)+1.0 * cell.indentationWidth,
                                       0,
                                       -(hasImg?35:0)+tabView.bounds.size.width -1.0 * cell.indentationWidth/*- 32*/-32,
                                       40);
            
            int actionicon_offsetx=tabView.safeAreaInsets.right+tabView.safeAreaInsets.left;
            actionicon_offsetx=PRI_SEC_ACTIONS_IMAGE_SIZE+tabView.safeAreaInsets.right+tabView.safeAreaInsets.left;
            //                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            
            secActionView.frame = CGRectMake(tabView.bounds.size.width-2/*-32*/-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
            
            [secActionView setImage:[UIImage imageNamed:@"folder.png"] forState:UIControlStateNormal];
            [secActionView setImage:[UIImage imageNamed:@"folder.png"] forState:UIControlStateHighlighted];
            [secActionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
            [secActionView addTarget: self action: @selector(accessoryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[secActionView.description componentsSeparatedByString:@";"] firstObject]];
            
            secActionView.enabled=YES;
            secActionView.hidden=NO;
        } else  { //file
            int actionicon_offsetx=tabView.safeAreaInsets.right+tabView.safeAreaInsets.left;
            
            bool hasImg=false;
            if (cur_local_entries[indexPath.row].imgpath) {
                coverImgView.image=[UIImage imageWithContentsOfFile:cur_local_entries[indexPath.row].imgpath];
                hasImg=true;
            }
            
            
            //archive file ?
            if (cur_local_entries[indexPath.row].type&16) { //need to confirm if true archive or multisong
                if ((cur_local_entries[indexPath.row].type&15)==2) { //need to confirm if true archive
                    if ([ModizFileHelper isABrowsableArchive:[ModizFileHelper getFullPathForFilePath:cur_local_entries[indexPath.row].fullpath]]) cur_local_entries[indexPath.row].type=2;
                    else cur_local_entries[indexPath.row].type=1;
                } else if ((cur_local_entries[indexPath.row].type&15)==3) { //need to confirm if true multisong
                    if ([ModizFileHelper isGMEFileWithSubsongs:[ModizFileHelper getFullPathForFilePath:cur_local_entries[indexPath.row].fullpath]]) cur_local_entries[indexPath.row].type=3;
                    else if ([ModizFileHelper isSidFileWithSubsongs:[ModizFileHelper getFullPathForFilePath:cur_local_entries[indexPath.row].fullpath]]) cur_local_entries[indexPath.row].type=3;
                    else cur_local_entries[indexPath.row].type=1;
                }
            }
            
            if ((cur_local_entries[indexPath.row].type==2)||(cur_local_entries[indexPath.row].type==3)) {
                actionicon_offsetx=PRI_SEC_ACTIONS_IMAGE_SIZE+tabView.safeAreaInsets.right+tabView.safeAreaInsets.left;
                //                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                
                secActionView.frame = CGRectMake(tabView.bounds.size.width-2/*-32*/-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                
                if (settings[GLOB_ArcMultiDefaultAction].detail.mdz_switch.switch_value) {
                    if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                        [secActionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateNormal];
                        [secActionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateHighlighted];
                        [secActionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                        [secActionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                    } else {
                        [secActionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateNormal];
                        [secActionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateHighlighted];
                        [secActionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                        [secActionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                    }
                } else {
                    [secActionView setImage:[UIImage imageNamed:@"arc_details.png"] forState:UIControlStateNormal];
                    [secActionView setImage:[UIImage imageNamed:@"arc_details.png"] forState:UIControlStateHighlighted];
                    [secActionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                    [secActionView addTarget: self action: @selector(accessoryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                }
                [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[secActionView.description componentsSeparatedByString:@";"] firstObject]];
                
                secActionView.enabled=YES;
                secActionView.hidden=NO;
                
            }
            
            topLabel.frame= CGRectMake((hasImg?35:0)+1.0 * cell.indentationWidth,
                                       0,
                                       -(hasImg?35:0)+tabView.bounds.size.width -1.0 * cell.indentationWidth/*- 32*/-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,
                                       22);
            
            actionView.frame = CGRectMake(tabView.bounds.size.width-2/*-32*/-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
            
            if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                [actionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateNormal];
                [actionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateHighlighted];
                [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                [actionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
            } else {
                [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateNormal];
                [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateHighlighted];
                [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
            }
            actionView.enabled=YES;
            actionView.hidden=NO;
            
            
            if (cur_local_entries[indexPath.row].rating==-1) {
                signed char avg_rating;
                DBHelper::getFileStatsDBmod(cur_local_entries[indexPath.row].fullpath,
                                            &cur_local_entries[indexPath.row].playcount,
                                            &cur_local_entries[indexPath.row].rating,
                                            &avg_rating,
                                            &cur_local_entries[indexPath.row].song_length,
                                            &cur_local_entries[indexPath.row].channels_nb,
                                            &cur_local_entries[indexPath.row].songs);
                if ((cur_local_entries[indexPath.row].rating==0)&&(avg_rating>0))
                    cur_local_entries[indexPath.row].rating=1;
            }
            if (cur_local_entries[indexPath.row].rating>0) bottomImageView.image=[UIImage imageNamed:ratingImg[RATING_IMG(cur_local_entries[indexPath.row].rating)]];
            
            NSString *bottomStr;
            int isMonoSong=cur_local_entries[indexPath.row].songs==1;
            if (cur_local_entries[indexPath.row].song_length>0)
                bottomStr=[NSString stringWithFormat:@"%02d:%02d",cur_local_entries[indexPath.row].song_length/1000/60,(cur_local_entries[indexPath.row].song_length/1000)%60];
            else bottomStr=@"--:--";
            
            if (cur_local_entries[indexPath.row].channels_nb) bottomStr=[NSString stringWithFormat:@"%@|%02dch",bottomStr,cur_local_entries[indexPath.row].channels_nb];
            else bottomStr=[NSString stringWithFormat:@"%@|--ch",bottomStr];
            
            if (isMonoSong) {
                bottomStr=[NSString stringWithFormat:@"%@|1 sg",bottomStr];
            } else {
                if (cur_local_entries[indexPath.row].songs==0) bottomStr=[NSString stringWithFormat:@"%@",bottomStr,-cur_local_entries[indexPath.row].songs];
                else if (cur_local_entries[indexPath.row].songs>0) bottomStr=[NSString stringWithFormat:@"%@|%d sg",bottomStr,cur_local_entries[indexPath.row].songs];
                else bottomStr=[NSString stringWithFormat:@"%@|%d f",bottomStr,-cur_local_entries[indexPath.row].songs];
            }
            
            bottomStr=[NSString stringWithFormat:@"%@|Pl:%d",bottomStr,cur_local_entries[indexPath.row].playcount];
            
            if (mShowSubdir==2) {  //if in sort by fullpath mode, indicate path in bottom label
                NSString *strtmp=cur_local_entries[indexPath.row].fullpath;
                NSRange rdir=[strtmp rangeOfString:currentPath];
                if (rdir.location!=NSNotFound) {
                    strtmp=[strtmp substringFromIndex:(rdir.location+rdir.length+1)];
                }
                bottomStr=[NSString stringWithFormat:@"%@|%@",bottomStr,[strtmp stringByDeletingLastPathComponent]];
            }
            
            bottomLabel.text=bottomStr;
            
            bottomImageView.frame = CGRectMake((hasImg?35:0) +1.0*cell.indentationWidth,
                                               24,
                                               14,14);
            
            
            bottomLabel.frame = CGRectMake((bottomImageView.image?16:0)+(hasImg?35:0)+ 1.0 * cell.indentationWidth,
                                           22,
                                           -(bottomImageView.image?16:0)-(hasImg?35:0)+tabView.bounds.size.width -1.0 * cell.indentationWidth/*-32*/-PRI_SEC_ACTIONS_IMAGE_SIZE-40-actionicon_offsetx,
                                           18);
            
        }
    }
    topLabel.text = cellValue;
    
    return cell;
}

// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return NO;
}

- (UISwipeActionsConfiguration *)tableView:(UITableView *)tableView
trailingSwipeActionsConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath {
    
    t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
    
    if (indexPath.section==0) {
        if (indexPath.row==0) {
            // NEW FOLDER ACTION
            UIContextualAction *newFolderAction =
            [UIContextualAction contextualActionWithStyle:UIContextualActionStyleNormal
                                                    title:NSLocalizedString(@"New folder", @"")
                                                  handler:^(UIContextualAction *action,
                                                            UIView *sourceView,
                                                            void (^completionHandler)(BOOL)) {
                
                UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Enter folder name",@"")
                                                                                message:nil
                                                                         preferredStyle:UIAlertControllerStyleAlert];
                __weak UIAlertController *weakAlert = alertC;
                [alertC addTextFieldWithConfigurationHandler:^(UITextField *textField) {
                    textField.placeholder = [NSString stringWithString:NSLocalizedString(@"New folder",@"")];;
                }];
                
                UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
                    
                }];
                [alertC addAction:cancelAction];
                
                UIAlertAction *saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Create",@"") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                    UITextField *tf = weakAlert.textFields.firstObject;
                    
                    NSString *newPath;
                    newPath=[[ModizFileHelper getFullPathForFilePath:currentPath] stringByAppendingPathComponent:tf.text];
                    
                    NSError *err;
                    if ([mFileMngr createDirectoryAtPath:newPath withIntermediateDirectories:YES attributes:nil error:&err]==NO) {
                        MDZELog("Issue %d while create folder %@",(int)(err.code),newPath);
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while creating folder\n%@",@""),err.code,newPath]];
                        completionHandler(NO);
                    } else {
                        [ModizFileHelper addSkipBackupAttributeToItemAtPath:newPath];
                        
                        if (mSearch) {
                            mSearch=0;
                            [self listLocalFiles];
                            mSearch=1;
                        }
                        [self listLocalFiles];
                        
                        // Reload the cell to show the file is no longer downloaded
                        //[tableView reloadRowsAtIndexPaths:@[indexPath]
                        //                 withRowAnimation:UITableViewRowAnimationAutomatic];
                        [tableView reloadData];
                        
                        completionHandler(YES);
                    }
                }];
                [alertC addAction:saveAction];
                
                [self showAlert:alertC];
            }];
            newFolderAction.backgroundColor = [UIColor colorWithRed:MDZ_NEWFOLDER_COL_R green:MDZ_NEWFOLDER_COL_G blue:MDZ_NEWFOLDER_COL_B alpha:1];
            
            // Return multiple actions - they appear from right to left
            return [UISwipeActionsConfiguration configurationWithActions:@[newFolderAction]];
        }
    } else {
        // DELETE ACTION
        UIContextualAction *deleteAction =
        [UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive
                                                title:NSLocalizedString(@"Delete", @"")
                                              handler:^(UIContextualAction *action,
                                                        UIView *sourceView,
                                                        void (^completionHandler)(BOOL)) {
            
            //File or Directory => Delete
            
            //delete entry
            NSString *fullpath=[ModizFileHelper getFullPathForFilePath:cur_local_entries[indexPath.row].fullpath];
            NSError *err;
            
            if ([cutpaste_filesrcpath compare:cur_local_entries[indexPath.row].fullpath]==NSOrderedSame) {
                //deleting file in cut/paste buffer -> cancel buffer
                cutpaste_filesrcpath=nil;
            }
            
            if ([mFileMngr removeItemAtPath:fullpath error:&err]!=YES) {
                MDZELog("Issue %d while removing: %@",(int)(err.code),fullpath);
                [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while trying to delete entry.\n%@",@""),err.code,err.localizedDescription]];
                completionHandler(NO);
            } else {
                if (cur_local_entries[indexPath.row].type==0) { //Dir
                    DBHelper::deleteStatsDirDB(fullpath);
                    [self.detailViewController cleanPlaylistAfterDelDir:fullpath];
                }
                if (cur_local_entries[indexPath.row].type&3) { //File
                    [ModizFileHelper cleanAllCoversForFile:fullpath];
                    DBHelper::deleteStatsFileDB(fullpath);
                    [self.detailViewController cleanPlaylistAfterDelFile:fullpath];
                }
                if (mSearch) {
                    mSearch=0;
                    [self listLocalFiles]; //force a refresh
                    mSearch=1;
                }
                [self listLocalFiles];
                //[self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
                [tableView reloadData];
                completionHandler(YES);
            }
        }];
        deleteAction.backgroundColor = [UIColor redColor];
        
        
        // DELETE COVER ACTION
        UIContextualAction *deleteImageAction = nil;
        
        if (cur_local_entries[indexPath.row].imgpath) {
            deleteImageAction=[UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive
                                                    title:NSLocalizedString(@"Delete Cover", @"")
                                                  handler:^(UIContextualAction *action,
                                                            UIView *sourceView,
                                                            void (^completionHandler)(BOOL)) {
                //File or Directory => Delete
                //delete entry
                NSString *fullpath=cur_local_entries[indexPath.row].imgpath;
                NSError *err;
                
                if ([mFileMngr removeItemAtPath:fullpath error:&err]!=YES) {
                    MDZELog("Issue %d while removing: %@",(int)(err.code),fullpath);
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while trying to delete entry.\n%@",@""),err.code,err.localizedDescription]];
                    completionHandler(NO);
                } else {
                    //                    if (mSearch) {
                    //                        mSearch=0;
                    //                        [self listLocalFiles]; //force a refresh
                    //                        mSearch=1;
                    //                    }
                    //                    [self listLocalFiles];
                    cur_local_entries[indexPath.row].imgpath=nil;
                    
                    [tableView reloadData];
                    completionHandler(YES);
                }
            }];
            deleteImageAction.backgroundColor = [UIColor redColor];
            return [UISwipeActionsConfiguration configurationWithActions:@[deleteImageAction,deleteAction]];
        }
        
        // Return multiple actions - they appear from right to left
        return [UISwipeActionsConfiguration configurationWithActions:@[deleteAction]];
    }
    return nil;
}

- (UISwipeActionsConfiguration *)tableView:(UITableView *)tableView
leadingSwipeActionsConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section==0) {
        if (indexPath.row==0) {
            // PASTE ACTION
            UIContextualAction *pasteAction =
            [UIContextualAction contextualActionWithStyle:UIContextualActionStyleNormal
                                                    title:NSLocalizedString(@"Paste", @"")
                                                  handler:^(UIContextualAction *action,
                                                            UIView *sourceView,
                                                            void (^completionHandler)(BOOL)) {
                
                if (cutpaste_filesrcpath) {
                    //Paste file or dir
                    NSString *sourcePath=[ModizFileHelper getFullPathForFilePath:cutpaste_filesrcpath];
                    NSString *destPath=[[ModizFileHelper getFullPathForFilePath:currentPath] stringByAppendingPathComponent:[cutpaste_filesrcpath lastPathComponent]];
                    NSError *err;
                    
                    mFileMngr.delegate=self;
                    if ([mFileMngr moveItemAtPath:sourcePath toPath:destPath error:&err]!=YES) {
                        MDZELog("Issue %d while moving: %@",(int)(err.code),cutpaste_filesrcpath);
                        [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while moving: %@.\n%@",@""),err.code,cutpaste_filesrcpath]];
                    } else {
                        //[cutpaste_filesrcpath release];
                        cutpaste_filesrcpath=nil;
                        if (mSearch) {
                            mSearch=0;
                            [self listLocalFiles];
                            mSearch=1;
                        }
                        [self listLocalFiles];
                        [self.tableView reloadData];
                    }
                    completionHandler(YES);
                } else {
                    //Alert msg => nothing to Paste
                    [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Nothing to paste",@"")];
                    completionHandler(NO);
                }
            }];
            pasteAction.backgroundColor = [UIColor colorWithRed:MDZ_PASTE_COL_R green:MDZ_PASTE_COL_G blue:MDZ_PASTE_COL_B alpha:1];
            
            // Return multiple actions - they appear from right to left
            return [UISwipeActionsConfiguration configurationWithActions:@[pasteAction]];
        }
    } else {
        // RENAME ACTION
        UIContextualAction *renameAction =
        [UIContextualAction contextualActionWithStyle:UIContextualActionStyleNormal
                                                title:NSLocalizedString(@"Rename", @"")
                                              handler:^(UIContextualAction *action,
                                                        UIView *sourceView,
                                                        void (^completionHandler)(BOOL)) {
            //File or Directory
            //rename
            t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
            //rename
            renameIdx=indexPath.row;
            
            if ([cutpaste_filesrcpath compare:cur_local_entries[indexPath.row].fullpath]==NSOrderedSame) {
                //renaming file in cut/paste buffer -> cancel buffer
                //[cutpaste_filesrcpath release];
                cutpaste_filesrcpath=nil;
            }
            
            UIAlertController *alertC = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Enter new name",@"")
                                                                            message:nil
                                                                     preferredStyle:UIAlertControllerStyleAlert];
            __weak UIAlertController *weakAlert = alertC;
            [alertC addTextFieldWithConfigurationHandler:^(UITextField *textField) {
                textField.placeholder = [NSString stringWithString:cur_local_entries[indexPath.row].label];
            }];
            
            UIAlertAction *cancelAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel",@"") style:UIAlertActionStyleCancel handler:^(UIAlertAction * _Nonnull action) {
                
            }];
            [alertC addAction:cancelAction];
            
            UIAlertAction *saveAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Rename",@"") style:UIAlertActionStyleDefault handler:^(UIAlertAction * _Nonnull action) {
                UITextField *tf = weakAlert.textFields.firstObject;
                t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
                if (cur_local_entries[renameIdx].label) cur_local_entries[renameIdx].label=nil;
                
                NSString *curPath,*tgtPath;
                
                curPath=[ModizFileHelper getFullPathForFilePath:cur_local_entries[renameIdx].fullpath];
                tgtPath=[ModizFileHelper getFullPathForFilePath:cur_local_entries[renameIdx].fullpath];
                
                tgtPath=[[tgtPath stringByDeletingLastPathComponent] stringByAppendingPathComponent:tf.text];
                
                NSError *err;
                mFileMngr.delegate=self;
                if ([mFileMngr moveItemAtPath:curPath toPath:tgtPath error:&err]==NO) {
                    MDZELog("Issue %d while renaming file %@",(int)(err.code),curPath);
                } else {
                    cur_local_entries[renameIdx].label=[[NSString alloc] initWithString:tf.text];
                    
                    cur_local_entries[renameIdx].fullpath=[[NSString alloc] initWithString:tgtPath];
                    if (mSearch) {
                        mSearch=0;
                        [self listLocalFiles];
                        mSearch=1;
                    }
                    shouldFillKeys=1;
                    [self fillKeys];
                    
                    [self.tableView reloadData];
                }
            }];
            [alertC addAction:saveAction];
            
            [self showAlert:alertC];
            
            
            completionHandler(YES);
        }];
        renameAction.backgroundColor = [UIColor colorWithRed:MDZ_RENAME_COL_R green:MDZ_RENAME_COL_G blue:MDZ_RENAME_COL_B alpha:1];
        
        // CUT ACTION
        UIContextualAction *cutAction =
        [UIContextualAction contextualActionWithStyle:UIContextualActionStyleNormal
                                                title:NSLocalizedString(@"Cut", @"")
                                              handler:^(UIContextualAction *action,
                                                        UIView *sourceView,
                                                        void (^completionHandler)(BOOL)) {
            //cut
            t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
            cutpaste_filesrcpath=[[NSString alloc] initWithString:cur_local_entries[indexPath.row].fullpath];
            completionHandler(YES);
        }];
        cutAction.backgroundColor = [UIColor colorWithRed:MDZ_CUT_COL_R green:MDZ_CUT_COL_G blue:MDZ_CUT_COL_B alpha:1];
        
        // EXTRACT ACTION
        UIContextualAction *extractAction =
        [UIContextualAction contextualActionWithStyle:UIContextualActionStyleNormal
                                                title:NSLocalizedString(@"Extract", @"")
                                              handler:^(UIContextualAction *action,
                                                        UIView *sourceView,
                                                        void (^completionHandler)(BOOL)) {
            //extract
            [waitingViewExtract setTitle:NSLocalizedString(@"Extracting",@"")];
            [waitingViewExtract setDetail:@""];
            [waitingViewExtract showCancel];
            [waitingViewExtract showProgress];
            waitingViewExtract.hidden=false;
            //[self showWaiting];
            //[self flushMainLoop];
            t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
            int section=indexPath.section-2;
            
            NSString *filePath=[ModizFileHelper getFullPathForFilePath:cur_local_entries[indexPath.row].fullpath];
            NSString *tgtPath;
            tgtPath=[ModizFileHelper getFullPathForFilePath:[cur_local_entries[indexPath.row].fullpath stringByDeletingPathExtension]];
            int files_found=[ModizFileHelper scanarchive:[filePath UTF8String] filesList_ptr:nil filesCount_ptr:nil];
            if (files_found) {
                [self.tableView setUserInteractionEnabled:false];
                [self.navigationItem setHidesBackButton:YES animated:YES];
                extractProgress = [NSProgress progressWithTotalUnitCount:1];
                extractProgress.cancellable = YES;
                extractProgress.pausable = NO;
                [ModizFileHelper extractToPath:[filePath UTF8String] path:[tgtPath UTF8String] caller:self progress:extractProgress context:ExtractProgressObserverContext completion:nil];
//                if (mSearch) {
//                    mSearch=0;
//                    [self listLocalFiles];
//                    mSearch=1;
//                }
//                [self listLocalFiles];
                
                //[self.tableView reloadData];
            } else {
                [self showAlertMsg:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"No file to extract or not supported archive format.\n",@"")];
                self.waitingViewExtract.hidden=TRUE;
                [self.waitingViewExtract hideProgress];
            }
            //[self hideWaiting];
            completionHandler(YES);
        }];
        extractAction.backgroundColor = [UIColor colorWithRed:MDZ_EXTRACT_COL_R green:MDZ_EXTRACT_COL_G blue:MDZ_EXTRACT_COL_B alpha:1];
        
        // Return multiple actions - they appear from right to left
        t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
        if (cur_local_entries[indexPath.row].type==2) return [UISwipeActionsConfiguration configurationWithActions:@[cutAction,renameAction,extractAction]];
        else return [UISwipeActionsConfiguration configurationWithActions:@[cutAction,renameAction]];
    }
    return nil;
}


- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section==0) {
        if (indexPath.row==0) return YES;
    } else return YES;
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
        if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
        else mSearch=1;
        
        //    shouldFillKeys=1;
        //    [self fillKeys];
        //    [tableView reloadData];
        [self refreshViewReloadFiles];
    
    sBar.showsCancelButton = NO;
}
- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
    //if (mSearchText) [mSearchText release];
    
    mSearchText=[[NSString alloc] initWithString:searchText];
    //dont auto validate if too many entries
    if (local_nb_entries<MAX_AUTOSEARCH_ENTRIES_NB) {
        
        [self fillKeys];
        [tableView reloadData];
    }
}
- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
    //if (mSearchText) [mSearchText release];
    mSearchText=nil;
    sBar.text=nil;
    mSearch=0;
    sBar.showsCancelButton = NO;
    [searchBar resignFirstResponder];
    shouldFillKeys=1;
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
    //    [self updateWaitingTitle:@""];
    //    [self updateWaitingDetail:@""];
    //    [self hideWaitingCancel];
    //    [self hideWaitingProgress];
    //    [self showWaiting];
    
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
    //    [self hideWaiting];
}

#pragma mark -
#pragma mark Table view delegate
- (void) primaryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    //if (tableView.refreshControl.isRefreshing) return;
    if (tabViewRefresh) return;
    
    t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self updateWaitingTitle:@""];
    [self updateWaitingDetail:@""];
    [self hideWaitingCancel];
    [self hideWaitingProgress];
    [self showWaiting];
    [self flushMainLoop];
    
    if (indexPath.section==0) {
        // launch Play of current list
        t_playlist *pl;
        pl=(t_playlist *)calloc(1,sizeof(t_playlist));
        pl->nb_entries=0;
            for (int j=0;j<(search_local?search_local_entries_count:local_entries_count);j++) {
                if (cur_local_entries[j].type&3) {
                    
                    pl->entries[pl->nb_entries].label=cur_local_entries[j].label;
                    pl->entries[pl->nb_entries].fullpath=cur_local_entries[j].fullpath;
                    pl->entries[pl->nb_entries].ratings=cur_local_entries[j].rating;
                    pl->entries[pl->nb_entries].playcounts=cur_local_entries[j].playcount;
                    pl->nb_entries++;
                    if (pl->nb_entries>=MAX_PL_ENTRIES) {
                        MDZELog("max entries reached (%d)",MAX_PL_ENTRIES);
                        break;
                    }
                    
                }
            }
        if (pl->nb_entries) {
            [detailViewController play_listmodules:pl start_index:-1];
            
            if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
            
            [tableView reloadData];
        }
        free(pl);
    } else {
        if (cur_local_entries[indexPath.row].type&3) {//File selected
            // launch Play
            t_playlist *pl;
            pl=(t_playlist*)calloc(1,sizeof(t_playlist));
            pl->nb_entries=1;
            pl->entries[0].label=cur_local_entries[indexPath.row].label;
            pl->entries[0].fullpath=cur_local_entries[indexPath.row].fullpath;
            pl->entries[0].ratings=cur_local_entries[indexPath.row].rating;
            pl->entries[0].playcounts=cur_local_entries[indexPath.row].playcount;
            [detailViewController play_listmodules:pl start_index:0];
                        
            [tableView reloadData];
            
            free(pl);
        }
        
    }
    [self hideWaiting];
    
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Playlist common functions
/////////////////////////////////////////////////////////////////////////////////////////////
#define HAS_DETAILVIEW_CONT
#import "PlaylistCommonFunctions.h"
/////////////////////////////////////////////////////////////////////////////////////////////

-(int) getLocalFilesCount {
    int pl_entries=0;
    NSString *file,*cpath;
    NSMutableArray *filetype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE];;
    
    // First check count for each section
    cpath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:@"Documents"];
    NSError *error;
    NSArray *dirContent;//
    BOOL isDir;
    
    dirContent=[mFileMngr subpathsOfDirectoryAtPath:cpath error:&error];
    
    NSArray *sortedDirContent = [dirContent sortedArrayUsingComparator:^(id obj1, id obj2) {
        NSString *str1=[(NSString *)obj1 lastPathComponent];
        NSString *str2=[(NSString *)obj2 lastPathComponent];
        return [str1 caseInsensitiveCompare:str2];
    }];
    
    for (file in sortedDirContent) {
        //check if dir
        [mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
        if (isDir) {
        } else {
            
            NSString *extension;// = [[file pathExtension] uppercaseString];
            NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
            NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
            extension = (NSString *)[temparray_filepath lastObject];
            //[temparray_filepath removeLastObject];
            file_no_ext=[temparray_filepath firstObject];
            
            int found=0;
            
            if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
            else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
            
            if (found)  {
                pl_entries++;
            }
        }
    }
    return pl_entries;
}


- (void) secondaryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    //if (tableView.refreshControl.isRefreshing) return;
    if (tabViewRefresh) return;
    
    t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
    
    
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self updateWaitingTitle:@""];
    [self updateWaitingDetail:@""];
    [self hideWaitingCancel];
    [self hideWaitingProgress];
    [self showWaiting];
    [self flushMainLoop];
    
    
    //local  browser & favorites
    
    if (indexPath.section==0) {
        // enqueue current dir
        
        
        int total_entries=0;
        NSMutableArray *array_label = [[NSMutableArray alloc] init];
        NSMutableArray *array_path = [[NSMutableArray alloc] init];
            for (int j=0;j<(search_local?search_local_entries_count:local_entries_count);j++)
                if (cur_local_entries[j].type&3) {
                    total_entries++;
                    [array_label addObject:cur_local_entries[j].label];
                    [array_path addObject:cur_local_entries[j].fullpath];
                }
        
        //add to playlist
        if (total_entries) [self addMultipleToPlaylistSelView:array_path label:array_label showNowListening:true];
        
    } else {
        if (cur_local_entries[indexPath.row].type&3) {//File selected
            
            //add to playlist
            [self addToPlaylistSelView:cur_local_entries[indexPath.row].fullpath label:cur_local_entries[indexPath.row].label showNowListening:true];
        }
    }
    [self hideWaiting];
}

- (void) accessoryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    //if (tableView.refreshControl.isRefreshing) return;
    if (tabViewRefresh) return;
    
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
    NSString *cellValue;
    t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
    
    //if (tabView.refreshControl.isRefreshing) return;
    if (tabViewRefresh) return;
    
    int section=indexPath.section-2;
    
    
    if (indexPath.section==0) {
        int donothing=0;
        if (indexPath.row==0) {
            if (mSearch) {
                if (mSearchText==nil) donothing=1;
            }
            if (!donothing) {
                mShowSubdir=(mShowSubdir+1)%3;
                
                [self refreshViewReloadFiles];
                
                
            }
        }else {
            if (icloud_available) {
                [self updateWaitingTitle:@""];
                [self updateWaitingDetail:@""];
                [self hideWaitingCancel];
                [self hideWaitingProgress];
                [self showWaiting];
                [self flushMainLoop];
                
                
                NSString *newPath=[NSString stringWithFormat:@"%@/%@",currentPath,cellValue];
                //[newPath retain];
                if (childController == nil) childController = [[RootViewControllerLocalBrowser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                else {// Don't cache childviews
                }
                //set new title
                childController.title = @"iCloud";
                // Set new depth & new directory
                ((RootViewControllerLocalBrowser*)childController)->icloud_folder_mode=true;
                ((RootViewControllerLocalBrowser*)childController)->currentPath = [icloudURL path];
                ((RootViewControllerLocalBrowser*)childController)->browse_depth = browse_depth+1;
                ((RootViewControllerLocalBrowser*)childController)->detailViewController=detailViewController;
                
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
                
                
                [self hideWaiting];
            } else {
                //iCloud not available
                MDZILog("icloud not available");
            }
        }
    } else {
        cellValue=cur_local_entries[indexPath.row].label;
        
        if (cur_local_entries[indexPath.row].type==0) { //Directory selected : change current directory
            
            [self updateWaitingTitle:@""];
            [self updateWaitingDetail:@""];
            [self hideWaitingCancel];
            [self hideWaitingProgress];
            [self showWaiting];
            [self flushMainLoop];
            
            
            NSString *newPath=[NSString stringWithFormat:@"%@/%@",currentPath,cellValue];
            //[newPath retain];
            if (childController == nil) childController = [[RootViewControllerLocalBrowser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            else {// Don't cache childviews
            }
            //set new title
            childController.title = cellValue;
            // Set new depth & new directory
            ((RootViewControllerLocalBrowser*)childController)->currentPath = newPath;
            ((RootViewControllerLocalBrowser*)childController)->icloud_folder_mode=icloud_folder_mode;
            ((RootViewControllerLocalBrowser*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerLocalBrowser*)childController)->detailViewController=detailViewController;
            //childController.view.frame=self.view.frame;
            
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
            
            
            [self hideWaiting];
            //				[childController autorelease];
        } else if (((cur_local_entries[indexPath.row].type==2)||(cur_local_entries[indexPath.row].type==3))&&(mAccessoryButton||settings[GLOB_ArcMultiDefaultAction].detail.mdz_switch.switch_value)) { //Archive selected or multisongs: display files inside
            
            
            [self updateWaitingTitle:@""];
            [self updateWaitingDetail:@""];
            [self hideWaitingCancel];
            [self hideWaitingProgress];
            [self showWaiting];
            [self flushMainLoop];
            
            
            NSString *newPath;
            if (mShowSubdir) newPath=[NSString stringWithString:cur_local_entries[indexPath.row].fullpath];
            else newPath=[NSString stringWithFormat:@"%@/%@",currentPath,cellValue];
            //[newPath retain];
            if (childController == nil) childController = [[RootViewControllerLocalBrowser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            else {// Don't cache childviews
            }
            //set new title
            childController.title = cellValue;
            // Set new depth & new directory
            ((RootViewControllerLocalBrowser*)childController)->currentPath = newPath;
            ((RootViewControllerLocalBrowser*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerLocalBrowser*)childController)->detailViewController=detailViewController;
//            childController.view.frame=self.view.frame;
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
            
            
            [self hideWaiting];
            //				[childController autorelease];
        } else {  //File selected
            
            //            [self updateWaitingTitle:@""];
            //            [self updateWaitingDetail:@""];
            //            [self showWaitingCancel];
            //            [self showWaitingProgress];
            //            [self showWaiting];
            
            
            if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                // launch Play
                t_playlist *pl;
                pl=(t_playlist *)calloc(1,sizeof(t_playlist));
                pl->nb_entries=1;
                pl->entries[0].label=cur_local_entries[indexPath.row].label;
                pl->entries[0].fullpath=cur_local_entries[indexPath.row].fullpath;
                pl->entries[0].ratings=cur_local_entries[indexPath.row].rating;
                pl->entries[0].playcounts=cur_local_entries[indexPath.row].playcount;
                [detailViewController play_listmodules:pl start_index:0];
                
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                free(pl);
                
            } else {
                if ([detailViewController add_to_playlist:cur_local_entries[indexPath.row].fullpath fileName:cur_local_entries[indexPath.row].label forcenoplay:0]) {
                                    
                }
            }
            
            //[self hideWaiting];
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

-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer {
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    
    NSIndexPath *indexPath = [self.tableView indexPathForRowAtPoint:p];
    if (indexPath != nil) {
        if ((gestureRecognizer.state==UIGestureRecognizerStateBegan)||(gestureRecognizer.state==UIGestureRecognizerStateChanged)) {
            int crow=indexPath.row;
            int csection=indexPath.section;
            if (csection>=1) {
                //display popup
                t_local_browse_entry *cur_local_entries=(search_local?search_local_entries:local_entries);
                
                //get file info
                NSError *err;
                NSDictionary *dict;
                NSFileManager *fileManager=[[NSFileManager alloc] init];
                
                NSString *str=[NSString stringWithFormat:@"%@\n%@",cur_local_entries[crow].label,cur_local_entries[crow].fullpath];
                dict=[fileManager attributesOfItemAtPath:[ModizFileHelper getFullPathForFilePath:cur_local_entries[crow].fullpath] error:&err];
                if (dict) {
                    str=[str stringByAppendingFormat:@"\n%lluKo",[dict fileSize]/1024];
                }
                
                //[fileManager release];
                
                MDZILog("got: %@",str);
                
                if (self.popTipView == nil) {
                    self.popTipView = [[CMPopTipView alloc] initWithMessage:str];
                    self.popTipView.delegate = self;
                    self.popTipView.backgroundColor = [UIColor lightGrayColor];
                    self.popTipView.textColor = [UIColor darkTextColor];
                    
                    [self.popTipView presentPointingAtView:[self.tableView cellForRowAtIndexPath:indexPath] inView:self.tableView animated:YES];
                    popTipViewRow=crow;
                    popTipViewSection=csection;
                } else {
                    if ((popTipViewRow!=crow)||(popTipViewSection!=csection)||([str compare:self.popTipView.message]!=NSOrderedSame)) {
                        self.popTipView.message=str;
                        [self.popTipView presentPointingAtView:[self.tableView cellForRowAtIndexPath:indexPath] inView:self.tableView animated:YES];
                        popTipViewRow=crow;
                        popTipViewSection=csection;
                    }
                }
            }
        } else if ((gestureRecognizer.state==UIGestureRecognizerStateEnded)||(gestureRecognizer.state==UIGestureRecognizerStateCancelled)) {
            //hide popup
            if (popTipView!=nil) {
                [self.popTipView dismissAnimated:YES];
                popTipView=nil;
            }
        }
    }
}

#pragma mark UIGestureRecognizerDelegate
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    // Allow swipe actions to work simultaneously with long press
    return YES;
}

#pragma mark CMPopTipViewDelegate methods
- (void)popTipViewWasDismissedByUser:(CMPopTipView *)_popTipView {
    // User can tap CMPopTipView to dismiss it
    self.popTipView = nil;
}

#pragma mark -
#pragma mark Memory management

- (void)didReceiveMemoryWarning {
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Relinquish ownership any cached data, images, etc. that aren't in use.
}

-(void) addRefreshView {
    return;
    const int TAG_REFRESHVIEW=100;
    
    UIView *refreshBackgroundView=[self.tableView viewWithTag:TAG_REFRESHVIEW];
    if (refreshBackgroundView) {
        [refreshBackgroundView removeFromSuperview];
    }
    
    CGRect frame = CGRectMake(0,0,self.tableView.bounds.size.width,200);
    frame.origin.y = -frame.size.height;
    refreshBackgroundView = [[UIView alloc]initWithFrame:frame];
    refreshBackgroundView.tag=TAG_REFRESHVIEW;
    refreshBackgroundView.autoresizingMask = UIViewAutoresizingFlexibleWidth;// | UIViewAutoresizingFlexibleHeight;
    
    if (darkMode) {
        refreshBackgroundView.backgroundColor=[UIColor blackColor];
        self.tableView.refreshControl.tintColor = [UIColor whiteColor];
    } else {
        refreshBackgroundView.backgroundColor=[UIColor whiteColor];
        self.tableView.refreshControl.tintColor = [UIColor blackColor];
    }
    
    
    [self.tableView insertSubview:refreshBackgroundView atIndex:0];
}

- (void)viewDidUnload {
    // Relinquish ownership of anything that can be recreated in viewDidLoad or on demand.
    // For example: self.myOutlet = nil;;
}
- (void)dealloc {
    [waitingView removeFromSuperview];
    waitingView=nil;
    [waitingViewExtract removeFromSuperview];
    waitingViewExtract=nil;
    
    //[currentPath release];
    if (mSearchText) {
        //[mSearchText release];
        mSearchText=nil;
    }
    
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            local_entries_data[i].label=nil;
            local_entries_data[i].fullpath=nil;
        }
            for (int j=0;j<local_entries_count;j++) {
                local_entries[j].label=nil;
                local_entries[j].fullpath=nil;
            }
            local_entries=NULL;
        free(local_entries_data);
    }
    if (search_local_nb_entries) {
        
            for (int j=0;j<search_local_entries_count;j++) {
                search_local_entries[j].label=nil;
                search_local_entries[j].fullpath=nil;
            }
            search_local_entries=NULL;
        
        search_local_nb_entries=0;
        free(search_local_entries_data);
    }
    
    if (mFileMngr) {
        //[mFileMngr release];
        mFileMngr=nil;
    }

    //[super dealloc];
}

#pragma mark - UINavigationControllerDelegate

- (id <UIViewControllerAnimatedTransitioning>)navigationController:(UINavigationController *)navigationController
                                   animationControllerForOperation:(UINavigationControllerOperation)operation
                                                fromViewController:(UIViewController *)fromVC
                                                  toViewController:(UIViewController *)toVC
{
    return [[TTFadeAnimator alloc] init];
}

#pragma mark - UIScrollViewDelegate
- (BOOL)fileManager:(NSFileManager *)fileManager shouldMoveItemAtPath:(NSString *)srcPath toPath:(NSString *)dstPath {
    return YES;
}

- (BOOL)fileManager:(NSFileManager *)fileManager shouldProceedAfterError:(NSError *)error movingItemAtPath:(NSString *)srcPath toPath:(NSString *)dstPath {
    return YES;
}

#pragma mark - LoadingView related stuff

- (void) cancelPushed {
    if (extractProgress) {
        [extractProgress cancel];
        [waitingViewExtract hideCancel];
        [waitingViewExtract hideProgress];
        [waitingViewExtract setDetail:NSLocalizedString(@"Cancelling...",@"")];
    } else {
        [waitingViewPlayer hideCancel];
        [waitingViewPlayer hideProgress];
        [waitingViewPlayer setDetail:NSLocalizedString(@"Cancelling...",@"")];
        
        detailViewController.mplayer.extractPendingCancel=true;
        [detailViewController setCancelStatus:true];
        [detailViewController hideWaitingCancel];
        [detailViewController hideWaitingProgress];
        [detailViewController updateWaitingDetail:NSLocalizedString(@"Cancelling...",@"")];
         
        
    }
}

- (void) updateLoadingInfos:(NSTimer *) theTimer {
    [waitingViewPlayer.progressView setProgress:detailViewController.waitingView.progressView.progress animated:YES];
    
    if ([detailViewController.mplayer isPlaying]&& (miniplayerVC==nil) ) [self showMiniPlayer];
}

- (void) observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                       context:(void *)context
{
    if (context == ExtractProgressObserverContext) {
        NSProgress *progress = object;
        
        if ([progress isCancelled]) {
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                [self.waitingViewExtract resetCancelStatus];
                self.waitingViewExtract.hidden=TRUE;
                [self.waitingViewExtract hideProgress];
                [self.tableView setUserInteractionEnabled:true];
                [self.navigationItem setHidesBackButton:NO animated:YES];
                [self refreshViewReloadFiles];
            }];
        }
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            [self.waitingViewExtract setProgress:progress.fractionCompleted];
            if (progress.fractionCompleted>=1.0f) {
                self.waitingViewExtract.hidden=TRUE;
                [self.waitingViewExtract hideProgress];
                [self.tableView setUserInteractionEnabled:true];
                [self.navigationItem setHidesBackButton:NO animated:YES];
                [self refreshViewReloadFiles];
            }
        }];
    } else if (context==LoadingProgressObserverContext){
        WaitingView *wv = object;
        if (wv.hidden==false) {
            [waitingViewPlayer resetCancelStatus];
            waitingViewPlayer.hidden=detailViewController.waitingView.hidden;
            waitingViewPlayer.btnStopCurrentAction.hidden=detailViewController.waitingView.btnStopCurrentAction.hidden;
            waitingViewPlayer.progressView.progress=detailViewController.waitingView.progressView.progress;
            waitingViewPlayer.progressView.hidden=detailViewController.waitingView.progressView.hidden;
            waitingViewPlayer.lblTitle.text=[NSString stringWithString:detailViewController.waitingView.lblTitle.text];
            waitingViewPlayer.lblDetail.text=[NSString stringWithString:detailViewController.waitingView.lblDetail.text];
            
            
        } else {
            if (waitingViewPlayer.hidden==false) {
                [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                    [self resetCachedStats];
                    [self.tableView reloadData];
                }];
            }
        }
        waitingViewPlayer.hidden=wv.hidden;
        
    } else if (context == ExtractBrowserListProgressObserverContext) {
        NSProgress *progress = object;
        
        if ([progress isCancelled]) {
            [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                [self.waitingViewExtract resetCancelStatus];
                self.waitingViewExtract.hidden=TRUE;
                [self.waitingViewExtract hideProgress];
                [self.tableView setUserInteractionEnabled:true];
                [self.navigationItem setHidesBackButton:NO animated:YES];
            }];
        }
        [[NSOperationQueue mainQueue] addOperationWithBlock:^{
            [self.waitingViewExtract setProgress:progress.fractionCompleted];
            if (progress.fractionCompleted>=1.0f) {
                self.waitingViewExtract.hidden=TRUE;
                [self.waitingViewExtract hideProgress];
                [self.tableView setUserInteractionEnabled:true];
                [self.navigationItem setHidesBackButton:NO animated:YES];
            }
        }];
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

@end
