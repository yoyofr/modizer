//
//  RootViewControllerLocalBrowser.m
//  modizer1
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

#include <sys/types.h>
#include <sys/sysctl.h>

#include "gme.h"

//FileHelper
#include "ModizFileHelper.h"

//SID2
#include "sidplayfp/SidTune.h"
#include "sidplayfp/SidTuneInfo.h"

#include "unzip.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;
pthread_mutex_t db_mutexHVSCSTIL;
//static int shouldFillKeys;
static int local_flag;
static volatile int mPopupAnimation=0;

#import "RootViewControllerLocalBrowser.h"
#import "AppDelegate_Phone.h"
#import "ModizerWin.h"
#import "DetailViewControllerIphone.h"
#import "QuartzCore/CAAnimation.h"
#import "SettingsGenViewController.h"

extern volatile t_settings settings[MAX_SETTINGS];

#import "TTFadeAnimator.h"


@implementation RootViewControllerLocalBrowser

@synthesize mFileMngr;
@synthesize detailViewController,tableView;
@synthesize sBar;
@synthesize list;
@synthesize keys;
@synthesize currentPath;
@synthesize childController;
@synthesize mSearchText;
@synthesize popTipView;
@synthesize alertRename;
@synthesize waitingView,waitingViewExtract,waitingViewPlayer;

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
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
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
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT major,minor FROM version");
        
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                *major=sqlite3_column_int(stmt, 0);
                *minor=sqlite3_column_int(stmt, 1);
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
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
        NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
        NSString *pathToOldDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_TMP];
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
                    } else NSLog(@"ErrSQL : %d",err);
                    
                    //Migrate DB user data : song length, ratings, playlists, ...
                    sprintf(sqlStatementR,"SELECT name,fullpath,play_count,rating FROM user_stats");
                    err=sqlite3_prepare_v2(dbold, sqlStatementR, -1, &stmt, NULL);
                    if (err==SQLITE_OK){
                        while (sqlite3_step(stmt) == SQLITE_ROW) {
                            
                            sprintf(sqlStatementW,"INSERT INTO user_stats (name,fullpath,play_count,rating) SELECT \"%s\",\"%s\",%d,%d",
                                    (char*)sqlite3_column_text(stmt, 0),
                                    (char*)sqlite3_column_text(stmt, 1),
                                    sqlite3_column_int(stmt, 2),
                                    sqlite3_column_int(stmt, 3));
                            err=sqlite3_exec(db, sqlStatementW, NULL, NULL, NULL);
                            if (err!=SQLITE_OK) NSLog(@"ErrSQL : %d for %s",err,sqlStatementW);
                        }
                        sqlite3_finalize(stmt);
                    } else NSLog(@"ErrSQL : %d",err);
                    
                    /*sprintf(sqlStatementR,"SELECT id_md5,track_nb,song_length FROM songlength");
                     err=sqlite3_prepare_v2(dbold, sqlStatementR, -1, &stmt, NULL);
                     if (err==SQLITE_OK){
                     while (sqlite3_step(stmt) == SQLITE_ROW) {
                     
                     sprintf(sqlStatementW,"INSERT INTO songlength (id_md5,track_nb,song_length) SELECT \"%s\",%d,%d",
                     (char*)sqlite3_column_text(stmt, 0),
                     sqlite3_column_int(stmt, 1),
                     sqlite3_column_int(stmt, 2));
                     err=sqlite3_exec(db, sqlStatementW, NULL, NULL, NULL);
                     if (err!=SQLITE_OK) NSLog(@"ErrSQL : %d for %s",err,sqlStatementW);
                     }
                     sqlite3_finalize(stmt);
                     } else NSLog(@"ErrSQL : %d",err);*/
                    
                    
                    sprintf(sqlStatementR,"SELECT id,name,num_files FROM playlists");
                    err=sqlite3_prepare_v2(dbold, sqlStatementR, -1, &stmt, NULL);
                    if (err==SQLITE_OK){
                        while (sqlite3_step(stmt) == SQLITE_ROW) {
                            int id_playlist;
                            //CREATE NEW PL
                            sprintf(sqlStatementW,"INSERT INTO playlists (name,num_files) SELECT \"%s\",%d",
                                    (char*)sqlite3_column_text(stmt, 1),
                                    sqlite3_column_int(stmt, 2));
                            err=sqlite3_exec(db, sqlStatementW, NULL, NULL, NULL);
                            if (err!=SQLITE_OK) NSLog(@"ErrSQL : %d for %s",err,sqlStatementW);
                            
                            //GET NEW PL ID
                            id_playlist=sqlite3_last_insert_rowid(db);
                            
                            //RECOPY PL ENTRIES
                            sprintf(sqlStatementR2,"SELECT name,fullpath FROM playlists_entries WHERE id_playlist=%d",sqlite3_column_int(stmt, 0));
                            err=sqlite3_prepare_v2(dbold, sqlStatementR2, -1, &stmt2, NULL);
                            if (err==SQLITE_OK){
                                while (sqlite3_step(stmt2) == SQLITE_ROW) {
                                    
                                    sprintf(sqlStatementW,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %d,\"%s\",\"%s\"",
                                            id_playlist,
                                            (char*)sqlite3_column_text(stmt2, 0),
                                            (char*)sqlite3_column_text(stmt2, 1));
                                    err=sqlite3_exec(db, sqlStatementW, NULL, NULL, NULL);
                                    if (err!=SQLITE_OK) NSLog(@"ErrSQL : %d for %s",err,sqlStatementW);
                                }
                                sqlite3_finalize(stmt2);
                            } else NSLog(@"ErrSQL : %d",err);
                            
                        }
                        sqlite3_finalize(stmt);
                    } else NSLog(@"ErrSQL : %d",err);
                    
                    
                    
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
    
    NSString *samplesDocPath=[NSString stringWithFormat:@"%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/Samples"]];
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
        UIAlertView *alert1;
        if (forceInit) alert1 = [[UIAlertView alloc] initWithTitle:@"Info"
                                                           message:NSLocalizedString(@"Database will now be recreated. Please validate & wait.",@"") delegate:self cancelButtonTitle:@"Recreate DB" otherButtonTitles:nil];
        else {
            if (wrongversion) {
                alert1 = [[UIAlertView alloc] initWithTitle:@"Info" message:
                          [NSString stringWithFormat:NSLocalizedString(@"Old database version: %d.%d. Will update to %d.%d. Please validate & wait.",@""),maj,min,VERSION_MAJOR,VERSION_MINOR] delegate:self cancelButtonTitle:@"Update DB" otherButtonTitles:nil];
                [alert1 show];
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

- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    if (mDatabaseCreationInProgress) {
        [self recreateDB];
        UIAlertView *alert2 = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Database created.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
        [alert2 show];
    }
    if (renameFile) {
        renameFile=0;
        if (buttonIndex==1) {
            t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
            UITextField *tf=[alertView textFieldAtIndex:0];
            //NSLog(@"rename %@ to %@",cur_local_entries[renameSec][renameIdx].label,tf.text);
            if (cur_local_entries[renameSec][renameIdx].label) cur_local_entries[renameSec][renameIdx].label=nil;
            
            NSString *curPath,*tgtPath;
            
            curPath=[ModizFileHelper getFullPathForFilePath:cur_local_entries[renameSec][renameIdx].fullpath];
            tgtPath=[ModizFileHelper getFullPathForFilePath:cur_local_entries[renameSec][renameIdx].fullpath];
            
            tgtPath=[[tgtPath stringByDeletingLastPathComponent] stringByAppendingPathComponent:tf.text];
            //NSLog(@"rename %@ to %@",curPath,tgtPath);
            
            NSError *err;
            mFileMngr.delegate=self;
            if ([mFileMngr moveItemAtPath:curPath toPath:tgtPath error:&err]==NO) {
                NSLog(@"Issue %d while renaming file %@",err.code,curPath);
                //UIAlertView *removeAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while trying to renamefile.\n%@",@""),err.code,err.localizedDescription] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                //[removeAlert show];
            } else {
                cur_local_entries[renameSec][renameIdx].label=[[NSString alloc] initWithString:tf.text];
                
                //[cur_local_entries[renameSec][renameIdx].fullpath release];
                cur_local_entries[renameSec][renameIdx].fullpath=[[NSString alloc] initWithString:tgtPath];
                if (mSearch) {
                    mSearch=0;
                    [self listLocalFiles];
                    mSearch=1;
                }
                shouldFillKeys=1;
                [self fillKeys];
                
                [self.tableView reloadData];
            }
            //tf.text=[NSString stringWithString:cur_local_entries[section][indexPath.row].fullpath];
        }
    }
    if (createFolder) {
        createFolder=0;
        if (buttonIndex==1) {
            UITextField *tf=[alertView textFieldAtIndex:0];
            //NSLog(@"rename %@ to %@",cur_local_entries[renameSec][renameIdx].label,tf.text);
            
            NSString *newPath;
            newPath=[[ModizFileHelper getFullPathForFilePath:currentPath] stringByAppendingPathComponent:tf.text];
            
            NSError *err;
            if ([mFileMngr createDirectoryAtPath:newPath withIntermediateDirectories:YES attributes:nil error:&err]==NO) {
                NSLog(@"Issue %d while create folder %@",err.code,newPath);
                UIAlertView *removeAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while creating folder\n%@",@""),err.code,newPath] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                [removeAlert show];
            } else {
                [ModizFileHelper addSkipBackupAttributeToItemAtPath:newPath];
                
                if (mSearch) {
                    mSearch=0;
                    [self listLocalFiles];
                    mSearch=1;
                }
                [self listLocalFiles];
                
                [self.tableView reloadData];
            }
            //tf.text=[NSString stringWithString:cur_local_entries[section][indexPath.row].fullpath];
        }
    }
    
    
}

- (BOOL) alertViewShouldEnableFirstOtherButton:(UIAlertView *)alertView
{
    NSString *inputText = [[alertView textFieldAtIndex:0] text];
    if( [inputText length] >= 1 ) {
        return YES;
    } else {
        return NO;
    }
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

-(void) refreshMiniplayer {
    if ((miniplayerVC==nil)&&([detailViewController mPlaylist_size]>0)) {
        wasMiniPlayerOn=true;
        [self showMiniPlayer];
    }
}

- (void)viewDidLoad {
    clock_t start_time,end_time;
    start_time=clock();
    childController=nil;
    
    cutpaste_initiated=0;
    
    dictActionBtn=[NSMutableDictionary dictionaryWithCapacity:64];
    
    extractProgress = nil;
    
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
    list=nil;
    keys=nil;
    
    if (browse_depth==0) { //Local mode
        currentPath = @"Documents";
        //[currentPath retain];
    }
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[UIBarButtonItem alloc] initWithCustomView: btn];
    self.navigationItem.rightBarButtonItem = item;
    
    
    indexTitles = [[NSMutableArray alloc] init];
    [indexTitles addObject:@"{search}"];
    [indexTitles addObject:@"#"];
    [indexTitles addObject:@"A"];
    [indexTitles addObject:@"B"];
    [indexTitles addObject:@"C"];
    [indexTitles addObject:@"D"];
    [indexTitles addObject:@"E"];
    [indexTitles addObject:@"F"];
    [indexTitles addObject:@"G"];
    [indexTitles addObject:@"H"];
    [indexTitles addObject:@"I"];
    [indexTitles addObject:@"J"];
    [indexTitles addObject:@"K"];
    [indexTitles addObject:@"L"];
    [indexTitles addObject:@"M"];
    [indexTitles addObject:@"N"];
    [indexTitles addObject:@"O"];
    [indexTitles addObject:@"P"];
    [indexTitles addObject:@"Q"];
    [indexTitles addObject:@"R"];
    [indexTitles addObject:@"S"];
    [indexTitles addObject:@"T"];
    [indexTitles addObject:@"U"];
    [indexTitles addObject:@"V"];
    [indexTitles addObject:@"W"];
    [indexTitles addObject:@"X"];
    [indexTitles addObject:@"Y"];
    [indexTitles addObject:@"Z"];
    
    indexTitlesSpace = [[NSMutableArray alloc] init];
    [indexTitlesSpace addObject:@"{search}"];
    [indexTitlesSpace addObject:@" "];
    [indexTitlesSpace addObject:@"#"];
    [indexTitlesSpace addObject:@"A"];
    [indexTitlesSpace addObject:@"B"];
    [indexTitlesSpace addObject:@"C"];
    [indexTitlesSpace addObject:@"D"];
    [indexTitlesSpace addObject:@"E"];
    [indexTitlesSpace addObject:@"F"];
    [indexTitlesSpace addObject:@"G"];
    [indexTitlesSpace addObject:@"H"];
    [indexTitlesSpace addObject:@"I"];
    [indexTitlesSpace addObject:@"J"];
    [indexTitlesSpace addObject:@"K"];
    [indexTitlesSpace addObject:@"L"];
    [indexTitlesSpace addObject:@"M"];
    [indexTitlesSpace addObject:@"N"];
    [indexTitlesSpace addObject:@"O"];
    [indexTitlesSpace addObject:@"P"];
    [indexTitlesSpace addObject:@"Q"];
    [indexTitlesSpace addObject:@"R"];
    [indexTitlesSpace addObject:@"S"];
    [indexTitlesSpace addObject:@"T"];
    [indexTitlesSpace addObject:@"U"];
    [indexTitlesSpace addObject:@"V"];
    [indexTitlesSpace addObject:@"W"];
    [indexTitlesSpace addObject:@"X"];
    [indexTitlesSpace addObject:@"Y"];
    [indexTitlesSpace addObject:@"Z"];
    
    
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
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
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
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewExtract attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewExtract attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
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
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingViewPlayer attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    
    
    
    [super viewDidLoad];
    renameFile=0;
    createFolder=0;
    
    
    // Initialize the refresh control.
    self.tableView.refreshControl = [[UIRefreshControl alloc] init];
    self.tableView.refreshControl.backgroundColor = [UIColor clearColor];
    self.tableView.refreshControl.tintColor = [UIColor whiteColor];
    [self.tableView.refreshControl addTarget:self
                                      action:@selector(refreshViewReloadFiles)
                            forControlEvents:UIControlEventValueChanged];
    
    
    //tableView.refreshControl=refreshControl;
    
    end_time=clock();
#ifdef LOAD_PROFILE
    NSLog(@"rootviewLB : %ul",end_time-start_time);
#endif
    
}

-(void) fillKeys {
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
        } else NSLog(@"ErrSQL : %d",err);
        
        if (!realPath) {
            //try to find realPath with md5
            sprintf(sqlStatement,"SELECT filepath FROM hvsc_path WHERE id_md5=\"%s\"",browser_song_md5);
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    strcpy(tmppath,(const char*)sqlite3_column_text(stmt, 0));
                    realPath=tmppath;
                }
                sqlite3_finalize(stmt);
            } else NSLog(@"ErrSQL : %d",err);
        } else realPath+=5;
        if (realPath) {
            sprintf(sqlStatement,"SELECT stil_info FROM stil WHERE fullpath=\"%s\"",realPath);
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
            } else NSLog(@"ErrSQL : %d",err);
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
    
    //NSLog(@"stil info:\n%s\n",browser_stil_info);
    
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


-(void)listLocalFiles {
    NSString *file,*cpath;
    NSURL *fileURL;
    NSMutableArray *filetype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE];
    NSMutableArray *filetype_extAMIGA=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE_AMIGA];
    NSMutableArray *all_multisongstype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_PLAYABLEFILE_SUBSONGS];
    NSMutableArray *archivetype_ext=[ModizFileHelper buildListSupportFileType:FTYPE_ARCHIVE];
    
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    char sqlStatement[1024];
    sqlite3_stmt *stmt;
    int local_entries_index,local_nb_entries_limit;
    int browseType;
    int shouldStop=0;
    static bool no_reentrant=false;
    NSRange r;
    
    if (no_reentrant) return;
    no_reentrant=true;
    
    // First check count for each section
    cpath=[ModizFileHelper getFullPathForFilePath:currentPath];
    //NSLog(@"%@\n%@",cpath,currentPath);
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
        for (int i=0;i<27;i++) {
            for (int j=0;j<search_local_entries_count[i];j++) {
                search_local_entries[i][j].label=nil;
                search_local_entries[i][j].fullpath=nil;
            }
            search_local_entries[i]=NULL;
        }
        search_local_nb_entries=0;
        free(search_local_entries_data);
    }
    
    if (mSearch) {
        search_local=1;
        
        search_local_entries_data=(t_local_browse_entry*)calloc(local_nb_entries,sizeof(t_local_browse_entry));
        
        for (int i=0;i<27;i++) {
            search_local_entries_count[i]=0;
            if (local_entries_count[i]) search_local_entries[i]=&(search_local_entries_data[search_local_nb_entries]);
            for (int j=0;j<local_entries_count[i];j++)  {
                
                bool found=false;
                if ((browseType==0)&&mShowSubdir) {
                    found=[self searchStringRegExp:mSearchText sourceString:local_entries[i][j].label];
                    if (!found) found=[self searchStringRegExp:mSearchText sourceString:local_entries[i][j].fullpath];
                } else {
                    found=[self searchStringRegExp:mSearchText sourceString:local_entries[i][j].label];
                }
                
                if  (found ||([mSearchText length]==0)) {
                    search_local_entries[i][search_local_entries_count[i]].label=local_entries[i][j].label;
                    search_local_entries[i][search_local_entries_count[i]].fullpath=local_entries[i][j].fullpath;
                    search_local_entries[i][search_local_entries_count[i]].playcount=local_entries[i][j].playcount;
                    search_local_entries[i][search_local_entries_count[i]].rating=local_entries[i][j].rating;
                    search_local_entries[i][search_local_entries_count[i]].type=local_entries[i][j].type;
                    
                    search_local_entries[i][search_local_entries_count[i]].song_length=local_entries[i][j].song_length;
                    search_local_entries[i][search_local_entries_count[i]].songs=local_entries[i][j].songs;
                    search_local_entries[i][search_local_entries_count[i]].channels_nb=local_entries[i][j].channels_nb;
                    
                    search_local_entries_count[i]++;
                    search_local_nb_entries++;
                }
            }
            
            
        }
        no_reentrant=false;
        return;
    }
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) != SQLITE_OK) db=NULL;
    
    err=sqlite3_exec(db, "PRAGMA journal_mode=WAL; PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
    if (err==SQLITE_OK){
    } else NSLog(@"ErrSQL : %d",err);
    
    
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            local_entries_data[i].label=nil;
            local_entries_data[i].fullpath=nil;
        }
        for (int i=0;i<27;i++) {
            for (int j=0;j<local_entries_count[i];j++) {
                local_entries[i][j].label=nil;
                local_entries[i][j].fullpath=nil;
            }
            local_entries[i]=NULL;
        }
        free(local_entries_data);local_entries_data=NULL;
        local_nb_entries=0;
    }
    for (int i=0;i<27;i++) local_entries_count[i]=0;
    
    
    
    if (browseType==3) {//SID
        SidTune *mSidTune=new SidTune([cpath UTF8String],0,true);
        bool sid_ok=true;
        if (mSidTune==NULL) sid_ok=false;
        else if (!(mSidTune->getStatus())) sid_ok=false;
        if (!sid_ok) {
            NSLog(@"SID SidTune init error");
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
                    
                    const char *str=[file UTF8String];
                    int index=0;
                    if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                    if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                    local_entries_count[index]++;
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
                        UIAlertView *memAlert = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                        [memAlert show];
                    } else {
                        //show alert : limited list
                        UIAlertView *memAlert = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                        [memAlert show];
                        local_nb_entries=local_nb_entries_limit;
                    }
                } else local_nb_entries_limit=0;
                if (local_entries_data) {
                    local_entries_index=0;
                    for (int i=0;i<27;i++)
                        if (local_entries_count[i]) {
                            if (local_entries_index+local_entries_count[i]>local_nb_entries) {
                                local_entries_count[i]=local_nb_entries-local_entries_index;
                                local_entries[i]=&(local_entries_data[local_entries_index]);
                                local_entries_index+=local_entries_count[i];
                                local_entries_count[i]=0;
                                for (int j=i+1;j<27;j++) local_entries_count[i]=0;
                            } else {
                                local_entries[i]=&(local_entries_data[local_entries_index]);
                                local_entries_index+=local_entries_count[i];
                                local_entries_count[i]=0;
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
                            
                            const char *str;
                            str=[file UTF8String];
                            int index=0;
                            if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                            if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                            local_entries[index][local_entries_count[index]].type=1;
                            local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                            local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@?%d",currentPath,i];
                            
                            local_entries[index][local_entries_count[index]].rating=0;
                            local_entries[index][local_entries_count[index]].playcount=0;
                            local_entries[index][local_entries_count[index]].song_length=0;
                            local_entries[index][local_entries_count[index]].songs=1;//0;
                            local_entries[index][local_entries_count[index]].channels_nb=0;
                            
                            sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[local_entries[index][local_entries_count[index]].fullpath UTF8String]);
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
                                    local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt, 0);
                                    local_entries[index][local_entries_count[index]].rating=rating;
                                    local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt, 2);
                                    local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                    //local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt, 4);
                                }
                                sqlite3_finalize(stmt);
                            } else NSLog(@"ErrSQL : %d",err);
                            
                            local_entries_count[index]++;
                            
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
            NSLog(@"gme_open_file error: %s",gme_err);
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
                        
                        const char *str=[file UTF8String];
                        int index=0;
                        if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                        if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                        local_entries_count[index]++;
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
                    UIAlertView *memAlert = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                    [memAlert show];
                } else {
                    //show alert : limited list
                    UIAlertView *memAlert = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                    [memAlert show];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                for (int i=0;i<27;i++)
                    if (local_entries_count[i]) {
                        if (local_entries_index+local_entries_count[i]>local_nb_entries) {
                            local_entries_count[i]=local_nb_entries-local_entries_index;
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                            for (int j=i+1;j<27;j++) local_entries_count[i]=0;
                        } else {
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                        }
                    }
                
                gme_err_t gme_err=gme_open_file( [cpath UTF8String], &gme_emu, gme_info_only );
                if (gme_err) {
                    NSLog(@"gme_open_file error: %s",gme_err);
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
                                
                                const char *str;
                                str=[file UTF8String];
                                int index=0;
                                if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                local_entries[index][local_entries_count[index]].type=1;
                                local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@?%d",currentPath,i];
                                
                                local_entries[index][local_entries_count[index]].rating=0;
                                local_entries[index][local_entries_count[index]].playcount=0;
                                local_entries[index][local_entries_count[index]].song_length=0;
                                local_entries[index][local_entries_count[index]].songs=1;//0;
                                local_entries[index][local_entries_count[index]].channels_nb=0;
                                
                                sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[local_entries[index][local_entries_count[index]].fullpath UTF8String]);
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
                                        local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt, 0);
                                        local_entries[index][local_entries_count[index]].rating=rating;
                                        local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt, 2);
                                        local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                        //local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt, 4);
                                    }
                                    sqlite3_finalize(stmt);
                                } else NSLog(@"ErrSQL : %d",err);
                                
                                local_entries_count[index]++;
                                
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
//        struct archive *a;
//        struct archive_entry *entry;
//        int r;
        
        
                
        char **archive_entries;
        int archive_entries_count;
        int found=[ModizFileHelper scanarchive:[cpath UTF8String] filesList_ptr:&archive_entries filesCount_ptr:&archive_entries_count];
        int file_idx=0;
        
        
        
//        a = archive_read_new();
//        archive_read_support_filter_all(a);
//        archive_read_support_format_raw(a);
//        archive_read_support_format_all(a);
//        r = archive_read_open_filename(a, [cpath UTF8String], 16384); // Note 1
//        if (r == ARCHIVE_OK) {
        if (found) {
            
            //sort the file list
            qsort(archive_entries, archive_entries_count, sizeof(char*), &qsort_CompareArcEntries);
            
            
            int is_rsn=0;
            NSString *extension=[[[cpath lastPathComponent] pathExtension] uppercaseString];
            if ([extension caseInsensitiveCompare:@"rsn"]==NSOrderedSame) is_rsn=1;
            
//            while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
            while (file_idx<found) {
                
                
//                file=[ModizFileHelper getCorrectFileName:[cpath UTF8String] archive:a entry:entry];
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
                        const char *str=[[file lastPathComponent] UTF8String];
                        int index=0;
                        if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                        if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                        local_entries_count[index]++;
                        local_nb_entries++;
                    }
                }
                
                //archive_read_data_skip(a);  // Note 2
                file_idx++;
            }
        } else {
            //NSLog( @"Skipping unsupported archive: %s\n", path );
        }
//        r = archive_read_free(a);  // Note 3
        
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
                    UIAlertView *memAlert = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                    [memAlert show];
                } else {
                    //show alert : limited list
                    UIAlertView *memAlert = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                    [memAlert show];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                for (int i=0;i<27;i++)
                    if (local_entries_count[i]) {
                        if (local_entries_index+local_entries_count[i]>local_nb_entries) {
                            local_entries_count[i]=local_nb_entries-local_entries_index;
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                            for (int j=i+1;j<27;j++) local_entries_count[i]=0;
                        } else {
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                        }
                    }
                
                
//                a = archive_read_new();
//                archive_read_support_filter_all(a);
//                archive_read_support_format_raw(a);
//                archive_read_support_format_all(a);
//                r = archive_read_open_filename(a, [cpath UTF8String], 10240); // Note 1
//                if (r == ARCHIVE_OK) {
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
                                int index=0;
                                if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                local_entries[index][local_entries_count[index]].type=1;
                                //NOT Supported: archive or multisongs in an archive
                                //                                //check if Archive file
                                //                                if ([archivetype_ext indexOfObject:extension]!=NSNotFound) local_entries[index][local_entries_count[index]].type=2;
                                //                                else if ([archivetype_ext indexOfObject:file_no_ext]!=NSNotFound) local_entries[index][local_entries_count[index]].type=2;
                                //                                //check if Multisongs file
                                
                                
                                if (other_encoding) {
                                    local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithCString:tmp_str encoding:NSUTF8StringEncoding];
                                    //	free(tmp_convstr);
                                } else local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                
                                local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@@%d",currentPath,arc_counter];
                                
                                local_entries[index][local_entries_count[index]].rating=0;
                                local_entries[index][local_entries_count[index]].playcount=0;
                                local_entries[index][local_entries_count[index]].song_length=0;
                                local_entries[index][local_entries_count[index]].songs=0;
                                local_entries[index][local_entries_count[index]].channels_nb=0;
                                
                                sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s\"",[local_entries[index][local_entries_count[index]].fullpath UTF8String]);
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
                                        local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt, 0);
                                        local_entries[index][local_entries_count[index]].rating=rating;
                                        local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt, 2);
                                        local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                        local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt, 4);
                                    }
                                    sqlite3_finalize(stmt);
                                } else NSLog(@"ErrSQL : %d",err);
                                
                                local_entries_count[index]++;
                                arc_counter++;
                                
                                if (local_nb_entries_limit) {
                                    local_nb_entries_limit--;
                                    if (!local_nb_entries_limit) shouldStop=1;
                                }
                            }
                        }
//                        archive_read_data_skip(a);  // Note 2
                        file_idx++;
                    }
                }
                //r = archive_read_free(a);  // Note 3
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
        //            end_time=clock();
        //            NSLog(@"detail0 : %ul",end_time-start_time);
        //            start_time=end_time;
        
        
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
            //check if dir
            //rdir.location=NSNotFound;
            //rdir = [file rangeOfString:@"." options:NSCaseInsensitiveSearch];
            
            /*file_idx++;
             if ((file_idx&127)==0)
             dispatch_sync(dispatch_get_main_queue(), ^(void){
             //Run UI Updates
             [self updateWaitingDetail:[NSString stringWithFormat:@"%d/%d",file_idx,file_cnt]];
             });*/
            
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
                                const char *str=[file UTF8String];
                                int index=0;
                                if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                local_entries_count[index]++;
                                local_nb_entries++;
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
                            const char *str;
                            if (mShowSubdir==2) str=[file UTF8String];
                            else str=[[file lastPathComponent] UTF8String];
                            int index=0;
                            if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                            if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                            local_entries_count[index]++;
                            local_nb_entries++;
                        }
                    }
                }
            }
        }
        //        end_time=clock();
        //        NSLog(@"detail1 : %ul",end_time-start_time);
        //        start_time=end_time;
        
        
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
                    UIAlertView *memAlert = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                    [memAlert show];
                } else {
                    //show alert : limited list
                    UIAlertView *memAlert = [[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                    [memAlert show];
                    local_nb_entries=local_nb_entries_limit;
                }
            } else local_nb_entries_limit=0;
            if (local_entries_data) {
                local_entries_index=0;
                for (int i=0;i<27;i++)
                    if (local_entries_count[i]) {
                        if (local_entries_index+local_entries_count[i]>local_nb_entries) {
                            local_entries_count[i]=local_nb_entries-local_entries_index;
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                            for (int j=i+1;j<27;j++) local_entries_count[i]=0;
                        } else {
                            local_entries[i]=&(local_entries_data[local_entries_index]);
                            local_entries_index+=local_entries_count[i];
                            local_entries_count[i]=0;
                        }
                    }
                
                //                                end_time=clock();
                //                                NSLog(@"detail2 : %ul",end_time-start_time);
                //                                start_time=end_time;
                // Second check count for each section
                for (fileURL in sortedDirContent) {
                    if (shouldStop) break;
                    //rdir.location=NSNotFound;
                    // rdir = [file rangeOfString:@"." options:NSCaseInsensitiveSearch];
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
                                        const char *str=[file UTF8String];
                                        int index=0;
                                        if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                        if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                        local_entries[index][local_entries_count[index]].type=0;
                                        
                                        local_entries[index][local_entries_count[index]].label=[[NSString alloc] initWithString:file];
                                        
                                        local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@/%@",currentPath,file];
                                        local_entries_count[index]++;
                                        if (local_nb_entries_limit) {
                                            local_nb_entries_limit--;
                                            if (!local_nb_entries_limit) shouldStop=1;
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
                                    const char *str;
                                    char tmp_str[1024];//,*tmp_convstr;
                                    int other_encoding=0;
                                    if (mShowSubdir==2) str=[file UTF8String];
                                    else str=[[file lastPathComponent] UTF8String];
                                    
                                    if ([extension caseInsensitiveCompare:@"mdx"]==NSOrderedSame ) {
                                        [[file lastPathComponent] getFileSystemRepresentation:tmp_str maxLength:1024];
                                        //tmp_convstr=mdx_make_sjis_to_syscharset(tmp_str);
                                        other_encoding=1;
                                    }
                                    int index=0;
                                    if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                    if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                    local_entries[index][local_entries_count[index]].type=1;
                                    //check if Archive file
                                    if ([archivetype_ext indexOfObject:extension]!=NSNotFound) { //check if really an archive
                                        //if ([ModizFileHelper isABrowsableArchive:[cpath stringByAppendingFormat:@"/%@",file]]) local_entries[index][local_entries_count[index]].type=2;
                                        local_entries[index][local_entries_count[index]].type=2|16;  //16 is to flag them as to check before displaying entry in tabiew
                                    } else if ([all_multisongstype_ext indexOfObject:extension]!=NSNotFound) { //check if Multisongs file
                                        local_entries[index][local_entries_count[index]].type=3|16;  //16 is to flag them as to check before displaying entry in tabiew
                                        //if ([ModizFileHelper isGMEFileWithSubsongs:[cpath stringByAppendingFormat:@"/%@",file]]) local_entries[index][local_entries_count[index]].type=3;
                                        //else if ([ModizFileHelper isSidFileWithSubsongs:[cpath stringByAppendingFormat:@"/%@",file]]) local_entries[index][local_entries_count[index]].type=3;
                                        
                                    }
                                    
                                    if (other_encoding) {
                                        local_entries[index][local_entries_count[index]].label=[[NSString alloc] initWithCString:tmp_str encoding:NSUTF8StringEncoding];
                                        //	free(tmp_convstr);
                                    } else local_entries[index][local_entries_count[index]].label=[[NSString alloc] initWithString:[file lastPathComponent]];
                                    
                                    local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@/%@",currentPath,file];
                                    
                                    local_entries[index][local_entries_count[index]].rating=-1;
                                    local_entries[index][local_entries_count[index]].playcount=-1;
                                    local_entries[index][local_entries_count[index]].song_length=-1;
                                    local_entries[index][local_entries_count[index]].songs=-1;
                                    local_entries[index][local_entries_count[index]].channels_nb=-1;
                                    
#if 0
                                    sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs,avg_rating FROM user_stats WHERE fullpath=\"%s/%s\"",[currentPath UTF8String],[file UTF8String]);
                                    err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                    
                                    //printf("%s\n",sqlStatement);
                                    
                                    if (err==SQLITE_OK){
                                        while (sqlite3_step(stmt) == SQLITE_ROW) {
                                            signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                            if (rating<0) rating=0;
                                            if (rating>5) rating=5;
                                            if ((rating==0)&&(sqlite3_column_type(stmt,5)!=SQLITE_NULL)) {
                                                rating=(signed char)sqlite3_column_int(stmt, 5);
                                                if (rating) rating=1;
                                            }
                                            
                                            //printf("ch: %d - sl: %d\n",(int)sqlite3_column_int(stmt, 3),(int)sqlite3_column_int(stmt, 2));
                                            
                                            local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt, 0);
                                            local_entries[index][local_entries_count[index]].rating=rating;
                                            local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt, 2);
                                            local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                            local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt, 4);
                                        }
                                        sqlite3_finalize(stmt);
                                    } else NSLog(@"ErrSQL : %d",err);
#endif
                                    local_entries_count[index]++;
                                    
                                    if (local_nb_entries_limit) {
                                        local_nb_entries_limit--;
                                        if (!local_nb_entries_limit) shouldStop=1;
                                    }
                                }
                            }
                        }
                    }
                }
                //                                end_time=clock();
                //                                NSLog(@"detail3 : %ul",end_time-start_time);
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
        
        [self addRefreshView];
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
    
    if (SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(@"14.0"))
        if (@available(iOS 14.0, *)) {
            if ([NSProcessInfo processInfo].isiOSAppOnMac) {
                
                AppDelegate_Phone *main_delegate=(AppDelegate_Phone*)[[UIApplication sharedApplication] delegate];
                ModizerWin *modizerWin=[main_delegate modizerWin];
                
                //CGRect frame = [modizerWin frame];
                //frame.size.height = MODIZER_MACM1_HEIGHT_MAX;
                //frame.size.width = MODIZER_MACM1_WIDTH_MAX;
                //[modizerWin setFrame: frame];
                //[modizerWin setBounds:frame];
            }
        }
    
    bool oldmode=darkMode;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    if (oldmode!=darkMode) forceReloadCells=true;
    
    if (darkMode) self.tableView.backgroundColor=[UIColor blackColor];
    else self.tableView.backgroundColor=[UIColor whiteColor];
    
    [self.sBar setBarStyle:UIBarStyleDefault];
    
    self.navigationController.delegate = self;
    //[[[self navigationController] navigationBar] setBarStyle:UIBarStyleDefault];
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
    
    if (shouldFillKeys) {
        if (keys) {
            keys=nil;
        }
        if (list) {
            list=nil;
        }
        
        //Reset rating if applicable (ensure updated value)
        if (local_nb_entries) {
            for (int i=0;i<local_nb_entries;i++) {
                local_entries_data[i].rating=-1;
            }
        }
        if (search_local_nb_entries) {
            for (int i=0;i<search_local_nb_entries;i++) {
                search_local_entries_data[i].rating=-1;
            }
        }
    }
    
    //creating view for extending background color
    [self addRefreshView];
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
    
    if (childController) [(RootViewControllerLocalBrowser*) childController refreshViewAfterDownload];
    else {
        
        if (self.tableView.refreshControl.refreshing==false) [self.tableView.refreshControl beginRefreshing];
        
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
                [self hideWaiting];
                [self.tableView reloadData];
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
            dispatch_sync(dispatch_get_main_queue(), ^(void){
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
    
    //check if a pending cut/paste exists
    if (cutpaste_initiated&&(cutpaste_filesrcpath==nil)) {
        //file has been moved, force reload
        shouldFillKeys=1;
        cutpaste_initiated=0;
    }
    
    if (shouldFillKeys) [self refreshViewReloadFiles];
    
    
    
    if ((!wasMiniPlayerOn) && [detailViewController mPlaylist_size]) [self showMiniPlayer];
    
    //[[[self navigationController] navigationBar] setBarStyle:UIBarStyleDefault];
    
    
    if ([detailViewController not_expected_version]==1) {
        NSLog(@"change of version");
        detailViewController.not_expected_version=0;
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


- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
    [tableView reloadData];
}

// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    [tableView reloadData];
    return YES;
}

#pragma mark - Table view data source

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
    return 0;
    if (mSearch) return 0;
    
    if (section==0) return 0;
    if (section==1) return 0;
    if ((search_local?search_local_entries_count[section-2]:local_entries_count[section-2])) {
        return 24;
    } else return 0;
    return 0;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    return nil;
    NSString *lbl=nil;
    
    if (mSearch) lbl=nil;
    else if (section==0) lbl=nil;
    else if (section==1) lbl=@"";
    else if ((search_local?search_local_entries_count[section-2]:local_entries_count[section-2])) lbl=[indexTitlesSpace objectAtIndex:section];
    else lbl=nil;
    
    if (lbl)
    /*if ([lbl compare:@""]!=NSOrderedSame)*/{
        UIView *customView = [[UIView alloc] initWithFrame: CGRectMake(0.0, 0.0, tableView.bounds.size.width, 24.0)];
        if (darkMode) customView.backgroundColor = [UIColor colorWithRed: 0.3f green: 0.3f blue: 0.3f alpha: 1.0f];
        else customView.backgroundColor = [UIColor colorWithRed: 0.7f green: 0.7f blue: 0.7f alpha: 1.0f];
        
        CALayer *layerU = [CALayer layer];
        layerU.frame = CGRectMake(0.0, 0.0, tableView.bounds.size.width, 1.0);
        if (darkMode) layerU.backgroundColor = [[UIColor colorWithRed: 1-183.0f/255.0f green: 1-193.0f/255.0f blue: 1-199.0f/255.0f alpha: 1.00] CGColor];
        else layerU.backgroundColor = [[UIColor colorWithRed: 183.0f/255.0f green: 193.0f/255.0f blue: 199.0f/255.0f alpha: 1.00] CGColor];
        [customView.layer insertSublayer:layerU atIndex:0];
        
        CAGradientLayer *gradient = [CAGradientLayer layer];
        gradient.frame = CGRectMake(0.0, 1.0, tableView.bounds.size.width, 22.0);
        if (darkMode) gradient.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: 1-144.0f/255.0f green: 1-159.0f/255.0f blue: 1-177.0f/255.0f alpha: 1.00] CGColor],
                                         (id)[[UIColor colorWithRed: 1-183.0f/255.0f green: 1-193.0f/255.0f blue: 1-199.0f/255.0f  alpha: 1.00] CGColor], nil];
        else gradient.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: 144.0f/255.0f green: 159.0f/255.0f blue: 177.0f/255.0f alpha: 1.00] CGColor],
                                (id)[[UIColor colorWithRed: 183.0f/255.0f green: 193.0f/255.0f blue: 199.0f/255.0f  alpha: 1.00] CGColor], nil];
        [customView.layer insertSublayer:gradient atIndex:0];
        
        CALayer *layerD = [CALayer layer];
        layerD.frame = CGRectMake(0.0, 23.0, tableView.bounds.size.width, 1.0);
        if (darkMode) layerD.backgroundColor = [[UIColor colorWithRed: 1-144.0f/255.0f green: 1-159.0f/255.0f blue: 1-177.0f/255.0f alpha: 1.00] CGColor];
        else layerD.backgroundColor = [[UIColor colorWithRed: 144.0f/255.0f green: 159.0f/255.0f blue: 177.0f/255.0f alpha: 1.00] CGColor];
        [customView.layer insertSublayer:layerD atIndex:0];
        
        UILabel *topLabel = [[UILabel alloc] init];
        topLabel.frame=CGRectMake(10, 0.0, tableView.bounds.size.width-10*2, 24);
        //
        // Configure the properties for the text that are the same on every row
        //
        topLabel.backgroundColor = [UIColor clearColor];
        topLabel.textColor = [UIColor whiteColor];
        if (darkMode) topLabel.highlightedTextColor = [UIColor colorWithRed:1-0.9 green:1-0.9 blue:1-0.9 alpha:1.0];
        else topLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        topLabel.font = [UIFont boldSystemFontOfSize:20];
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
        topLabel.opaque=TRUE;
        topLabel.text=lbl;
        
        [customView addSubview: topLabel];
        
        return customView;
    }
    
    return nil;
}

/*- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
 if (mSearch) return nil;
 
 if (section==0) return nil;
 if (section==1) return @"";
 if ((search_local?search_local_entries_count[section-2]:local_entries_count[section-2])) {
 return [indexTitlesSpace objectAtIndex:section];
 } else return nil;
 return nil;
 }*/
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    local_flag=0;
    return 28+1;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section==0) return 0;
    if (section==1) {
        if (browse_depth==0) return 2;
        else return 1;
    }
    return (search_local?search_local_entries_count[section-2]:local_entries_count[section-2]);
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    if (mSearch) return nil;
    return indexTitlesSpace;
}

- (NSInteger)tableView:(UITableView *)tabView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    if (mSearch) return -1;
    if (index == 0) {
        [tabView setContentOffset:CGPointZero animated:NO];
        return NSNotFound;
    }
    return index;
}
/**
 Tells the delegate that a button of the left side is triggered.
 
 @param cell The cell informing the delegate of the event.
 @param buttonIndex The index of the button which is triggered.
 */

//*****************************************
//Archive management

- (void)slideTableViewCell:(SESlideTableViewCell*)cell didTriggerLeftButton:(NSInteger)buttonIndex {
    
    if ([cell.reuseIdentifier compare:@"CellH"]==NSOrderedSame) {
        if (cutpaste_filesrcpath) {
            //Paste file or dir
            //NSLog(@"Pasting: %@",cutpaste_filesrcpath);
            
            NSString *sourcePath=[ModizFileHelper getFullPathForFilePath:cutpaste_filesrcpath];
            NSString *destPath=[[ModizFileHelper getFullPathForFilePath:currentPath] stringByAppendingPathComponent:[cutpaste_filesrcpath lastPathComponent]];
            NSError *err;
            
            //NSLog(@"Pasting: %@ to %@",sourcePath,destPath);
            
            mFileMngr.delegate=self;
            if ([mFileMngr moveItemAtPath:sourcePath toPath:destPath error:&err]!=YES) {
                NSLog(@"Issue %d while moving: %@",err.code,cutpaste_filesrcpath);
                UIAlertView *moveAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while moving: %@.\n%@",@""),err.code,cutpaste_filesrcpath] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                [moveAlert show];
            } else {
                //[cutpaste_filesrcpath release];
                cutpaste_filesrcpath=nil;
                if (mSearch) {
                    mSearch=0;
                    [self listLocalFiles];
                    mSearch=1;
                }
                [self listLocalFiles];
            }
        } else {
            //Alert msg => nothing to Paste
            UIAlertView *pasteAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"Nothing to paste",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
            [pasteAlert show];
        }
    } else {
        //File or Directory
        NSIndexPath *indexPath = [self.tableView indexPathForCell:cell];
        
        switch (buttonIndex) {
            case 0: {//rename
                t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
                int section=indexPath.section-2;
                //rename
                renameFile=1;
                renameSec=section;renameIdx=indexPath.row;
                
                if ([cutpaste_filesrcpath compare:cur_local_entries[section][indexPath.row].fullpath]==NSOrderedSame) {
                    //renaming file in cut/paste buffer -> cancel buffer
                    //[cutpaste_filesrcpath release];
                    cutpaste_filesrcpath=nil;
                }
                
                alertRename=[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Enter new name",@"") message:nil delegate:self cancelButtonTitle:NSLocalizedString(@"Cancel",@"") otherButtonTitles:NSLocalizedString(@"Ok",@""),nil];
                [alertRename setAlertViewStyle:UIAlertViewStylePlainTextInput];
                UITextField *tf=[alertRename textFieldAtIndex:0];
                tf.text=[NSString stringWithString:cur_local_entries[section][indexPath.row].label];
                [alertRename show];
                
                break;
            }
            case 1:{//cut
                t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
                int section=indexPath.section-2;
                //cutpaste_filesrcpath=[NSString stringWithString:cur_local_entries[section][indexPath.row].fullpath];
                cutpaste_filesrcpath=[[NSString alloc] initWithString:cur_local_entries[section][indexPath.row].fullpath];
                //NSLog(@"Cut: %@",cutpaste_filesrcpath);
                break;
            }
            case 2:{//extract
                [waitingViewExtract setTitle:NSLocalizedString(@"Extracting",@"")];
                [waitingViewExtract setDetail:@""];
                [waitingViewExtract showCancel];
                [waitingViewExtract showProgress];
                waitingViewExtract.hidden=false;
                //[self showWaiting];
                //[self flushMainLoop];
                t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
                int section=indexPath.section-2;
                
                NSString *filePath=[ModizFileHelper getFullPathForFilePath:cur_local_entries[section][indexPath.row].fullpath];
                NSString *tgtPath;
                tgtPath=[ModizFileHelper getFullPathForFilePath:[cur_local_entries[section][indexPath.row].fullpath stringByDeletingPathExtension]];
                int files_found=[ModizFileHelper scanarchive:[filePath UTF8String] filesList_ptr:nil filesCount_ptr:nil];
                if (files_found) {
                    //NSLog(@"extracting %d files, %@ to %@",files_found,cur_local_entries[section][indexPath.row].fullpath,tgtPath);
                    [self.tableView setUserInteractionEnabled:false];
                    [self.navigationItem setHidesBackButton:YES animated:YES];
                    extractProgress = [NSProgress progressWithTotalUnitCount:1];
                    extractProgress.cancellable = YES;
                    extractProgress.pausable = NO;
                    [ModizFileHelper extractToPath:[filePath UTF8String] path:[tgtPath UTF8String] caller:self progress:extractProgress context:ExtractProgressObserverContext];
                    if (mSearch) {
                        mSearch=0;
                        [self listLocalFiles];
                        mSearch=1;
                    }
                    [self listLocalFiles];
                    
                    //[self.tableView reloadData];
                } else {
                    UIAlertView *cannotExtractAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:NSLocalizedString(@"No file to extract or not supported archive format.\n",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
                    [cannotExtractAlert show];
                }
                //[self hideWaiting];
                break;
            }
        }
    }
    
    [self.tableView reloadData];
}
/**
 Tells the delegate that a button of the right side is triggered.
 
 @param cell The cell informing the delegate of the event.
 @param buttonIndex The index of the button which is triggered.
 */
- (void)slideTableViewCell:(SESlideTableViewCell*)cell didTriggerRightButton:(NSInteger)buttonIndex {
    if ([cell.reuseIdentifier compare:@"CellH"]==NSOrderedSame) {
        //Header => New Folder
        createFolder=1;
        
        alertRename=[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Enter folder name",@"") message:nil delegate:self cancelButtonTitle:NSLocalizedString(@"Cancel",@"") otherButtonTitles:NSLocalizedString(@"Ok",@""),nil];
        [alertRename setAlertViewStyle:UIAlertViewStylePlainTextInput];
        UITextField *tf=[alertRename textFieldAtIndex:0];
        tf.text=[NSString stringWithString:NSLocalizedString(@"New folder",@"")];
        [alertRename show];
    } else {
        //File or Directory => Delete
        
        // Delete button was pressed
        NSIndexPath *indexPath = [self.tableView indexPathForCell:cell];
        
        //delete entry
        t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
        int section=indexPath.section-2;
        NSString *fullpath=[ModizFileHelper getFullPathForFilePath:cur_local_entries[section][indexPath.row].fullpath];
        NSError *err;
        
        if ([cutpaste_filesrcpath compare:cur_local_entries[section][indexPath.row].fullpath]==NSOrderedSame) {
            //deleting file in cut/paste buffer -> cancel buffer
            
            cutpaste_filesrcpath=nil;
        }
        
        if ([mFileMngr removeItemAtPath:fullpath error:&err]!=YES) {
            NSLog(@"Issue %d while removing: %@",err.code,fullpath);
            UIAlertView *removeAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while trying to delete entry.\n%@",@""),err.code,err.localizedDescription] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
            [removeAlert show];
        } else {
            if (cur_local_entries[section][indexPath.row].type==0) { //Dir
                DBHelper::deleteStatsDirDB(fullpath);
            }
            if (cur_local_entries[section][indexPath.row].type&3) { //File
                DBHelper::deleteStatsFileDB(fullpath);
            }
            if (mSearch) {
                mSearch=0;
                [self listLocalFiles]; //force a refresh
                mSearch=1;
            }
            [self listLocalFiles];
            //[self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
            [self.tableView reloadData];
        }
        
        //            [self.tableView deleteRowsAtIndexPaths:@[cellIndexPath]
        //                                  withRowAnimation:UITableViewRowAnimationAutomatic];
    }
    [self.tableView reloadData];
}
/**
 Asks the delegate if the cell can be a slide-state.
 
 The result of this function is not reflected to the slide indicators of the cell.
 You should set "showsLeftSlideIndicator" or "showsRightSlideIndicator" property of SESlideTableViewCell manually.
 
 @return YES if the cell can be the state, otherwise NO.
 @param cell The cell that is making this request.
 @param slideState The state that the cell want to be.
 */
//- (BOOL)slideTableViewCell:(SESlideTableViewCell*)cell canSlideToState:(SESlideTableViewCellSlideState)slideState;
/**
 Tells the delegate that the slide state of the cell will change.
 
 Even when this function is called, the cell's slide state may not be the state which this function tells.
 To know the cell's slide state, use slideTableViewCell:DidSlideToState: instead.
 
 @param cell The cell informing the delegate of the event.
 @param slideState The slide state which the cell may become.
 */
//- (void)slideTableViewCell:(SESlideTableViewCell*)cell willSlideToState:(SESlideTableViewCellSlideState)slideState;
/**
 Tells the delegate that the slide state of the cell did change.
 
 @param cell The cell informing the delegate of the event.
 @param slideState The slide state which the cell became.
 */
//- (void)slideTableViewCell:(SESlideTableViewCell*)cell didSlideToState:(SESlideTableViewCellSlideState)slideState;
/**
 Tells the delegate that the cell will show buttons of the side.
 
 @param cell The cell informing the delegate of the event.
 @param side The side of the buttons which the cell will show.
 */
//- (void)slideTableViewCell:(SESlideTableViewCell *)cell wilShowButtonsOfSide:(SESlideTableViewCellSide)side;

- (UITableViewCell *)tableView:(UITableView *)tabView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *CellIdentifier = @"Cell";
    static NSString *CellIdentifierHeader = @"CellH";
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
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    SESlideTableViewCell *cell;
    
    if (forceReloadCells) {
        while ([tabView dequeueReusableCellWithIdentifier:CellIdentifier]) {}
        while ([tabView dequeueReusableCellWithIdentifier:CellIdentifierHeader]) {}
        forceReloadCells=false;
    }
    
    if (indexPath.section==1) cell = (SESlideTableViewCell *)[tabView dequeueReusableCellWithIdentifier:CellIdentifierHeader];
    else cell = (SESlideTableViewCell *)[tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    
    
    if ((cell == nil)) {
        if (indexPath.section>1) {
            cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:    CellIdentifier];
            cell.delegate = self;
            
            [cell addLeftButtonWithText:NSLocalizedString(@"Rename",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithRed:MDZ_RENAME_COL_R green:MDZ_RENAME_COL_G blue:MDZ_RENAME_COL_B alpha:1.0]];
            [cell addLeftButtonWithText:NSLocalizedString(@"Cut",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithRed:MDZ_CUT_COL_R green:MDZ_CUT_COL_G blue:MDZ_CUT_COL_B alpha:1.0]];
            if (self.tableView.refreshControl.isRefreshing==false) {
                
                if ((cur_local_entries[indexPath.section-2][indexPath.row].type&16)&&((cur_local_entries[indexPath.section-2][indexPath.row].type&15)==2)) { //need to confirm if true archive
                    
                    if ([ModizFileHelper isABrowsableArchive:[ModizFileHelper getFullPathForFilePath:cur_local_entries[indexPath.section-2][indexPath.row].fullpath]]) cur_local_entries[indexPath.section-2][indexPath.row].type=2;
                    else cur_local_entries[indexPath.section-2][indexPath.row].type=1;
                }
                
                if (cur_local_entries[indexPath.section-2][indexPath.row].type==2) {
                    [cell addLeftButtonWithText:NSLocalizedString(@"Extract",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithRed:MDZ_EXTRACT_COL_R green:MDZ_EXTRACT_COL_G blue:MDZ_EXTRACT_COL_B alpha:1.0]];
                }
            }
            [cell addRightButtonWithText:NSLocalizedString(@"Delete",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor redColor]];
        } else {
            cell = [[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:    CellIdentifierHeader];
            cell.delegate = self;
            
            [cell addLeftButtonWithText:NSLocalizedString(@"Paste",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithRed:MDZ_PASTE_COL_R green:MDZ_PASTE_COL_G blue:MDZ_PASTE_COL_B alpha:1]];
            [cell addRightButtonWithText:NSLocalizedString(@"New folder",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithRed:MDZ_NEWFOLDER_COL_R green:MDZ_NEWFOLDER_COL_G blue:MDZ_NEWFOLDER_COL_B alpha:1]];
            
            
        }
        
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,40);
        [cell setBackgroundColor:[UIColor clearColor]];
        
        NSString *imgFile=(darkMode?@"tabview_gradient40Black.png":@"tabview_gradient40.png");
        UIImage *image = [UIImage imageNamed:imgFile];
        
        /*NSURL* imageURL = [[NSBundle mainBundle] URLForResource:@"tabview_gradient40" withExtension:@"png"];
         
         CIImage *img=[CIImage imageWithContentsOfURL:imageURL];
         CIFilter *filterImg=[CIFilter filterWithName:@"CIColorInvert"];
         [filterImg setValue:img forKey:kCIInputImageKey];
         UIImage *image = [UIImage imageWithCIImage:[filterImg outputImage]];*/
        
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
        topLabel.font = [UIFont boldSystemFontOfSize:18];
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
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
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        bottomImageView = (UIImageView *)[cell viewWithTag:BOTTOM_IMAGE_TAG];
        actionView = (UIButton *)[cell viewWithTag:ACT_IMAGE_TAG];
        secActionView = (UIButton *)[cell viewWithTag:SECACT_IMAGE_TAG];
        
        topLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
        
        bottomLabel.lineBreakMode=(settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value?
                                ((settings[GLOB_TruncateNameMode].detail.mdz_switch.switch_value==2) ? NSLineBreakByTruncatingTail:NSLineBreakByTruncatingMiddle):NSLineBreakByTruncatingHead);
        
        [cell removeAllLeftButtons];
        [cell removeAllRightButtons];
        
        if (indexPath.section>1) {
            
            [cell addLeftButtonWithText:NSLocalizedString(@"Rename",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithRed:MDZ_RENAME_COL_R green:MDZ_RENAME_COL_G blue:MDZ_RENAME_COL_B alpha:1.0]];
            [cell addLeftButtonWithText:NSLocalizedString(@"Cut",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithRed:MDZ_CUT_COL_R green:MDZ_CUT_COL_G blue:MDZ_CUT_COL_B alpha:1.0]];
            
            if (self.tableView.refreshControl.isRefreshing==false)
                
                if ((cur_local_entries[indexPath.section-2][indexPath.row].type&16)&&((cur_local_entries[indexPath.section-2][indexPath.row].type&15)==2)) { //need to confirm if true archive
                    if ([ModizFileHelper isABrowsableArchive:[ModizFileHelper getFullPathForFilePath:cur_local_entries[indexPath.section-2][indexPath.row].fullpath]]) cur_local_entries[indexPath.section-2][indexPath.row].type=2;
                    else cur_local_entries[indexPath.section-2][indexPath.row].type=1;
                }
            
            if (cur_local_entries[indexPath.section-2][indexPath.row].type==2) {
                [cell addLeftButtonWithText:NSLocalizedString(@"Extract",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithRed:MDZ_EXTRACT_COL_R green:MDZ_EXTRACT_COL_G blue:MDZ_EXTRACT_COL_B alpha:1.0]];
            }
            
            [cell addRightButtonWithText:NSLocalizedString(@"Delete",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor redColor]];
        } else {
            [cell addLeftButtonWithText:NSLocalizedString(@"Paste",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithRed:MDZ_PASTE_COL_R green:MDZ_PASTE_COL_G blue:MDZ_PASTE_COL_B alpha:1.0]];
            [cell addRightButtonWithText:NSLocalizedString(@"New folder",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor colorWithRed:MDZ_NEWFOLDER_COL_R green:MDZ_NEWFOLDER_COL_G blue:MDZ_NEWFOLDER_COL_B alpha:1]];
            
            
        }
    }
    actionView.hidden=TRUE;
    secActionView.hidden=TRUE;
    
    [cell layoutIfNeeded];
    
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
    
    //if refresh in prg, wait a bit...
    if (self.tableView.refreshControl.isRefreshing) return cell;
    
    // Set up the cell...
    if (indexPath.section==1){
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
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                           18);
            
            if (darkMode) topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED_DARKMODE green:ACTION_COLOR_GREEN_DARKMODE blue:ACTION_COLOR_BLUE_DARKMODE alpha:1.0];
            else topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
            
            topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-4-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       22);
            
            [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateNormal];
            [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateHighlighted];
            [secActionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            //secActionView.tag=indexPath.row*100+indexPath.section;
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[secActionView.description componentsSeparatedByString:@";"] firstObject]];
            
            [actionView setImage:[UIImage imageNamed:@"play_all.png"] forState:UIControlStateNormal];
            [actionView setImage:[UIImage imageNamed:@"play_all.png"] forState:UIControlStateHighlighted];
            [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            //actionView.tag=indexPath.row*100+indexPath.section;
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
            
            int icon_posx=tabView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.right-tabView.safeAreaInsets.left;
            icon_posx-=32;
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
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                           18);
            
            if (darkMode) topLabel.textColor=[UIColor colorWithRed:ICLOUD_COLOR_RED_DARKMODE green:ICLOUD_COLOR_GREEN_DARKMODE blue:ICLOUD_COLOR_BLUE_DARKMODE alpha:1.0];
            else topLabel.textColor=[UIColor colorWithRed:ICLOUD_COLOR_RED green:ICLOUD_COLOR_GREEN blue:ICLOUD_COLOR_BLUE alpha:1.0];
            
            topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-4-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       22);
            actionView.enabled=NO;
            actionView.hidden=YES;
            secActionView.enabled=NO;
            secActionView.hidden=YES;
            /*[secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateNormal];
             [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateHighlighted];
             [secActionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
             
             [actionView setImage:[UIImage imageNamed:@"play_all.png"] forState:UIControlStateNormal];
             [actionView setImage:[UIImage imageNamed:@"play_all.png"] forState:UIControlStateHighlighted];
             [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
             
             int icon_posx=tabView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.right-tabView.safeAreaInsets.left;
             icon_posx-=32;
             actionView.frame = CGRectMake(icon_posx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
             actionView.enabled=YES;
             actionView.hidden=NO;
             secActionView.frame = CGRectMake(icon_posx-PRI_SEC_ACTIONS_IMAGE_SIZE-4,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
             secActionView.enabled=YES;
             secActionView.hidden=NO;*/
        }
    } else {
        int section=indexPath.section-2;
        cellValue=cur_local_entries[section][indexPath.row].label;
        
        
        if (cur_local_entries[section][indexPath.row].type==0) { //directory
            if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:1.0f alpha:1.0f];
            else topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
            //cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            
            topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-32,
                                       40);
            
            int actionicon_offsetx=tabView.safeAreaInsets.right+tabView.safeAreaInsets.left;
            actionicon_offsetx=PRI_SEC_ACTIONS_IMAGE_SIZE+tabView.safeAreaInsets.right+tabView.safeAreaInsets.left;
            //                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            
            secActionView.frame = CGRectMake(tabView.bounds.size.width-2-32-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
            
            [secActionView setImage:[UIImage imageNamed:@"folder.png"] forState:UIControlStateNormal];
            [secActionView setImage:[UIImage imageNamed:@"folder.png"] forState:UIControlStateHighlighted];
            [secActionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
            [secActionView addTarget: self action: @selector(accessoryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[secActionView.description componentsSeparatedByString:@";"] firstObject]];
            
            secActionView.enabled=YES;
            secActionView.hidden=NO;
        } else  { //file
            int actionicon_offsetx=tabView.safeAreaInsets.right+tabView.safeAreaInsets.left;
            //archive file ?
            
            if (cur_local_entries[section][indexPath.row].type&16) { //need to confirm if true archive or multisong
                if ((cur_local_entries[section][indexPath.row].type&15)==2) { //need to confirm if true archive
                    if ([ModizFileHelper isABrowsableArchive:[ModizFileHelper getFullPathForFilePath:cur_local_entries[section][indexPath.row].fullpath]]) cur_local_entries[section][indexPath.row].type=2;
                    else cur_local_entries[section][indexPath.row].type=1;
                } else if ((cur_local_entries[section][indexPath.row].type&15)==3) { //need to confirm if true multisong
                    if ([ModizFileHelper isGMEFileWithSubsongs:[ModizFileHelper getFullPathForFilePath:cur_local_entries[section][indexPath.row].fullpath]]) cur_local_entries[section][indexPath.row].type=3;
                    else if ([ModizFileHelper isSidFileWithSubsongs:[ModizFileHelper getFullPathForFilePath:cur_local_entries[section][indexPath.row].fullpath]]) cur_local_entries[section][indexPath.row].type=3;
                    else cur_local_entries[section][indexPath.row].type=1;
                }
            }
            
            if ((cur_local_entries[section][indexPath.row].type==2)||(cur_local_entries[section][indexPath.row].type==3)) {
                actionicon_offsetx=PRI_SEC_ACTIONS_IMAGE_SIZE+tabView.safeAreaInsets.right+tabView.safeAreaInsets.left;
                //                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                
                secActionView.frame = CGRectMake(tabView.bounds.size.width-2-32-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                
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
            
            topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,
                                       22);
            
            actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
            
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
            
            
            if (cur_local_entries[section][indexPath.row].rating==-1) {
                signed char avg_rating;
                DBHelper::getFileStatsDBmod(cur_local_entries[section][indexPath.row].fullpath,
                                            &cur_local_entries[section][indexPath.row].playcount,
                                            &cur_local_entries[section][indexPath.row].rating,
                                            &avg_rating,
                                            &cur_local_entries[section][indexPath.row].song_length,
                                            &cur_local_entries[section][indexPath.row].channels_nb,
                                            &cur_local_entries[section][indexPath.row].songs);
                if ((cur_local_entries[section][indexPath.row].rating==0)&&(avg_rating>0))
                    cur_local_entries[section][indexPath.row].rating=1;
            }
            if (cur_local_entries[section][indexPath.row].rating>0) bottomImageView.image=[UIImage imageNamed:ratingImg[RATING_IMG(cur_local_entries[section][indexPath.row].rating)]];
            
            NSString *bottomStr;
            int isMonoSong=cur_local_entries[section][indexPath.row].songs==1;
            if (cur_local_entries[section][indexPath.row].song_length>0)
                bottomStr=[NSString stringWithFormat:@"%02d:%02d",cur_local_entries[section][indexPath.row].song_length/1000/60,(cur_local_entries[section][indexPath.row].song_length/1000)%60];
            else bottomStr=@"--:--";
            
            if (cur_local_entries[section][indexPath.row].channels_nb) bottomStr=[NSString stringWithFormat:@"%@|%02dch",bottomStr,cur_local_entries[section][indexPath.row].channels_nb];
            else bottomStr=[NSString stringWithFormat:@"%@|--ch",bottomStr];
            
            if (isMonoSong) {
                bottomStr=[NSString stringWithFormat:@"%@|1 sg",bottomStr];
            } else {
                if (cur_local_entries[section][indexPath.row].songs==0) bottomStr=[NSString stringWithFormat:@"%@",bottomStr,-cur_local_entries[section][indexPath.row].songs];
                else if (cur_local_entries[section][indexPath.row].songs>0) bottomStr=[NSString stringWithFormat:@"%@|%d sg",bottomStr,cur_local_entries[section][indexPath.row].songs];
                else bottomStr=[NSString stringWithFormat:@"%@|%d f",bottomStr,-cur_local_entries[section][indexPath.row].songs];
            }
            
            bottomStr=[NSString stringWithFormat:@"%@|Pl:%d",bottomStr,cur_local_entries[section][indexPath.row].playcount];
            
            if (mShowSubdir==2) {  //if in sort by fullpath mode, indicate path in bottom label
                NSString *strtmp=cur_local_entries[section][indexPath.row].fullpath;
                NSRange rdir=[strtmp rangeOfString:currentPath];
                if (rdir.location!=NSNotFound) {
                    strtmp=[strtmp substringFromIndex:(rdir.location+rdir.length+1)];
                }
                bottomStr=[NSString stringWithFormat:@"%@|%@",bottomStr,[strtmp stringByDeletingLastPathComponent]];
            }
            
            /*if (!cur_local_entries[section][indexPath.row].playcount)
             bottomStr = [NSString stringWithFormat:@"%@%@",bottomStr,played0time];
             else if (cur_local_entries[section][indexPath.row].playcount==1)
             bottomStr = [NSString stringWithFormat:@"%@%@",bottomStr,played1time];
             else bottomStr = [NSString stringWithFormat:@"%@%@",bottomStr,[NSString stringWithFormat:playedXtimes,cur_local_entries[section][indexPath.row].playcount]];*/
            
            bottomLabel.text=bottomStr;
            
            bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+20,
                                           22,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60-actionicon_offsetx,
                                           18);
            
        }
    }
    topLabel.text = cellValue;
    
    return cell;
}

// Override to support editing the table view.
- (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        
        //delete entry
        int section=indexPath.section-2;
        NSString *fullpath=[ModizFileHelper getFullPathForFilePath:cur_local_entries[section][indexPath.row].fullpath];
        NSError *err;
        
        if ([mFileMngr removeItemAtPath:fullpath error:&err]!=YES) {
            NSLog(@"Issue %d while removing: %@",err.code,fullpath);
            UIAlertView *removeAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"") message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while trying to delete entry.\n%@",@""),err.code,err.localizedDescription] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
            [removeAlert show];
        } else {
            if (cur_local_entries[section][indexPath.row].type==0) { //Dir
                DBHelper::deleteStatsDirDB(fullpath);
            }
            if (cur_local_entries[section][indexPath.row].type&3) { //File
                DBHelper::deleteStatsFileDB(fullpath);
            }
            if (mSearch) {
                mSearch=0;
                [self listLocalFiles];
                mSearch=1;
            }
            [self listLocalFiles];
            [tabView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
            [tabView reloadData];
        }
        
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}
/*- (NSIndexPath *)tableView:(UITableView *)tableView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath {
 return proposedDestinationIndexPath;
 }*/
// Override to support rearranging the table view.
/*- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
 
 }*/
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return NO;
}
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return NO;
    
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    int section=indexPath.section-2;
    if (section>=0) {
        NSString *fullpath=[ModizFileHelper getFullPathForFilePath:cur_local_entries[section][indexPath.row].fullpath];
        BOOL res;
        NSFileManager *myMngr=[[NSFileManager alloc] init];
        res=[myMngr isDeletableFileAtPath:fullpath];
        //[myMngr release];
        return res;
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
        if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
        else mSearch=1;
        shouldFillKeys=1;
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
                NSLog(@"Exception: [%@]:%@",[ex  class], ex );
                NSLog(@"ex.name:'%@'", ex.name);
                NSLog(@"ex.reason:'%@'", ex.reason);
                //Full error includes class pointer address so only care if it starts with this error
                NSRange range = [ex.reason rangeOfString:@"Pushing the same view controller instance more than once is not supported"];
                
                if ([ex.name isEqualToString:@"NSInvalidArgumentException"] &&
                    range.location != NSNotFound) {
                    //view controller already exists in the stack - just pop back to it
                    [self.navigationController popToViewController:detailViewController animated:YES];
                } else {
                    NSLog(@"ERROR:UNHANDLED EXCEPTION TYPE:%@", ex);
                }
            } @finally {
                //NSLog(@"finally");
            }
        } else {
            NSLog(@"ERROR:pushViewController: viewController is nil");
        }
    }
    else {
        UIAlertView *nofileplaying=[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Warning",@"")
                                                              message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil];
        [nofileplaying show];
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
    
    if (tableView.refreshControl.isRefreshing) return;
    
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self updateWaitingTitle:@""];
    [self updateWaitingDetail:@""];
    [self hideWaitingCancel];
    [self hideWaitingProgress];
    [self showWaiting];
    [self flushMainLoop];
    
    int section=indexPath.section-2;
    
    if (indexPath.section==1) {
        // launch Play of current list
        int pos=0;
        
        t_playlist *pl;
        pl=(t_playlist *)calloc(1,sizeof(t_playlist));
        pl->nb_entries=0;
        for (int i=0;i<27;i++) {
            for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++) {
                if (cur_local_entries[i][j].type&3) {
                    
                    pl->entries[pl->nb_entries].label=cur_local_entries[i][j].label;
                    pl->entries[pl->nb_entries].fullpath=cur_local_entries[i][j].fullpath;
                    pl->entries[pl->nb_entries].ratings=cur_local_entries[i][j].rating;
                    pl->entries[pl->nb_entries].playcounts=cur_local_entries[i][j].playcount;
                    pl->nb_entries++;
                    if (pl->nb_entries>=MAX_PL_ENTRIES) {
                        NSLog(@"max entries reached (%d)",MAX_PL_ENTRIES);
                        break;
                    }
                    
                    if (i<section) pos++;
                    else if ((i==(section))&&(j<indexPath.row)) pos++;
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
        if (cur_local_entries[section][indexPath.row].type&3) {//File selected
            // launch Play
            t_playlist *pl;
            pl=(t_playlist*)calloc(1,sizeof(t_playlist));
            pl->nb_entries=1;
            pl->entries[0].label=cur_local_entries[section][indexPath.row].label;
            pl->entries[0].fullpath=cur_local_entries[section][indexPath.row].fullpath;
            pl->entries[0].ratings=cur_local_entries[section][indexPath.row].rating;
            pl->entries[0].playcounts=cur_local_entries[section][indexPath.row].playcount;
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
    cpath=[NSHomeDirectory() stringByAppendingPathComponent:@"Documents"];
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
    
    if (tableView.refreshControl.isRefreshing) return;
    
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self updateWaitingTitle:@""];
    [self updateWaitingDetail:@""];
    [self hideWaitingCancel];
    [self hideWaitingProgress];
    [self showWaiting];
    [self flushMainLoop];
    
    
    //local  browser & favorites
    int section=indexPath.section-2;
    
    if (indexPath.section==1) {
        // enqueue current dir
        
        
        int pos=0;
        int total_entries=0;
        NSMutableArray *array_label = [[NSMutableArray alloc] init];
        NSMutableArray *array_path = [[NSMutableArray alloc] init];
        for (int i=0;i<27;i++)
            for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++)
                if (cur_local_entries[i][j].type&3) {
                    total_entries++;
                    [array_label addObject:cur_local_entries[i][j].label];
                    [array_path addObject:cur_local_entries[i][j].fullpath];
                    if (i<section) pos++;
                    else if (i==(section))
                        if (j<indexPath.row) pos++;
                }
        
        //add to playlist
        if (total_entries) [self addMultipleToPlaylistSelView:array_path label:array_label showNowListening:true];
        
    } else {
        if (cur_local_entries[section][indexPath.row].type&3) {//File selected
            
            //add to playlist
            [self addToPlaylistSelView:cur_local_entries[section][indexPath.row].fullpath label:cur_local_entries[section][indexPath.row].label showNowListening:true];
        }
    }
    [self hideWaiting];
}

- (void) accessoryActionTapped: (UIButton*) sender {
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    if (tableView.refreshControl.isRefreshing) return;
    
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
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    if (tabView.refreshControl.isRefreshing) return;
    
    int section=indexPath.section-2;
    
    
    if (indexPath.section==1) {
        int donothing=0;
        if (indexPath.row==0) {
            if (mSearch) {
                if (mSearchText==nil) donothing=1;
            }
            if (!donothing) {
                mShowSubdir=(mShowSubdir+1)%3;
                
                [self refreshViewReloadFiles];
                
                /*[self hideWaitingCancel];
                 [self updateWaitingTitle:@""];
                 [self updateWaitingDetail:@""];
                 
                 [self showWaiting];
                 [self flushMainLoop];
                 
                 
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
                 [tabView reloadData];
                 
                 [self hideWaiting];*/
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
                
                childController.view.frame=self.view.frame;
                // And push the window
                [self.navigationController pushViewController:childController animated:YES];
                
                
                [self hideWaiting];
            } else {
                //iCloud not available
                NSLog(@"icloud not available");
            }
        }
    } else {
        cellValue=cur_local_entries[section][indexPath.row].label;
        
        if (cur_local_entries[section][indexPath.row].type==0) { //Directory selected : change current directory
            
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
            childController.view.frame=self.view.frame;
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
            
            
            [self hideWaiting];
            //				[childController autorelease];
        } else if (((cur_local_entries[section][indexPath.row].type==2)||(cur_local_entries[section][indexPath.row].type==3))&&(mAccessoryButton||settings[GLOB_ArcMultiDefaultAction].detail.mdz_switch.switch_value)) { //Archive selected or multisongs: display files inside
            
            
            [self updateWaitingTitle:@""];
            [self updateWaitingDetail:@""];
            [self hideWaitingCancel];
            [self hideWaitingProgress];
            [self showWaiting];
            [self flushMainLoop];
            
            
            NSString *newPath;
            //                    NSLog(@"currentPath:%@\ncellValue:%@\nfullpath:%@",currentPath,cellValue,cur_local_entries[section][indexPath.row].fullpath);
            if (mShowSubdir) newPath=[NSString stringWithString:cur_local_entries[section][indexPath.row].fullpath];
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
            childController.view.frame=self.view.frame;
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
                pl->entries[0].label=cur_local_entries[section][indexPath.row].label;
                pl->entries[0].fullpath=cur_local_entries[section][indexPath.row].fullpath;
                pl->entries[0].ratings=cur_local_entries[section][indexPath.row].rating;
                pl->entries[0].playcounts=cur_local_entries[section][indexPath.row].playcount;
                [detailViewController play_listmodules:pl start_index:0];
                
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                
                
                free(pl);
                
            } else {
                if ([detailViewController add_to_playlist:cur_local_entries[section][indexPath.row].fullpath fileName:cur_local_entries[section][indexPath.row].label forcenoplay:0]) {
                                    
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
            //                    NSLog(@"long press on table view at %d/%d", indexPath.section,indexPath.row);
            int crow=indexPath.row;
            int csection=indexPath.section-2;
            if (csection>=0) {
                //display popup
                t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
                
                //get file info
                NSError *err;
                NSDictionary *dict;
                NSFileManager *fileManager=[[NSFileManager alloc] init];
                
                NSString *str=[NSString stringWithFormat:@"%@\n%@",cur_local_entries[csection][crow].label,cur_local_entries[csection][crow].fullpath];
                dict=[fileManager attributesOfItemAtPath:[ModizFileHelper getFullPathForFilePath:cur_local_entries[csection][crow].fullpath] error:&err];
                if (dict) {
                    str=[str stringByAppendingFormat:@"\n%lluKo",[dict fileSize]/1024];
                }
                
                //[fileManager release];
                
                
                if (self.popTipView == nil) {
                    self.popTipView = [[CMPopTipView alloc] initWithMessage:str];
                    self.popTipView.delegate = self;
                    self.popTipView.backgroundColor = [UIColor lightGrayColor];
                    self.popTipView.textColor = [UIColor darkTextColor];
                    
                    [self.popTipView presentPointingAtView:[self.tableView cellForRowAtIndexPath:indexPath] inView:self.view animated:YES];
                    popTipViewRow=crow;
                    popTipViewSection=csection;
                } else {
                    if ((popTipViewRow!=crow)||(popTipViewSection!=csection)||([str compare:self.popTipView.message]!=NSOrderedSame)) {
                        self.popTipView.message=str;
                        [self.popTipView presentPointingAtView:[self.tableView cellForRowAtIndexPath:indexPath] inView:self.view animated:YES];
                        popTipViewRow=crow;
                        popTipViewSection=csection;
                    }
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
    
    /*float red_col[3]={235/255.0f,235/255.0f,110/255.0f};
     float green_col[3]={80/255.0f,80/255.0f,40/255.0f};
     float blue_col[3]={193/255.0f,193/255.0f,110/255.0f};
     
     CAGradientLayer *gradient = [CAGradientLayer layer];
     gradient.frame = CGRectMake(0.0, 0.0, tableView.bounds.size.width, 6.0);
     gradient.colors=@[(id)[UIColor colorWithRed:red_col[0] green:green_col[0] blue:blue_col[0] alpha:1].CGColor,
     (id)[UIColor colorWithRed:red_col[1] green:green_col[1] blue:blue_col[1] alpha:1].CGColor,                                                   ];
     
     [refreshBackgroundView.layer insertSublayer:gradient atIndex:0];
     
     gradient = [CAGradientLayer layer];
     gradient.frame = CGRectMake(0.0, 6.0, tableView.bounds.size.width, 88.0);
     gradient.colors=@[(id)[UIColor colorWithRed:red_col[1] green:green_col[1] blue:blue_col[1] alpha:1].CGColor,                                                   (id)[UIColor colorWithRed:red_col[2] green:green_col[2] blue:blue_col[2] alpha:1].CGColor];
     
     [refreshBackgroundView.layer insertSublayer:gradient atIndex:0];
     
     gradient = [CAGradientLayer layer];
     gradient.frame = CGRectMake(0.0, 94.0, tableView.bounds.size.width, 6.0);
     float base_col;
     if (darkMode) base_col=0;
     else base_col=1;
     gradient.colors=@[(id)[UIColor colorWithRed:red_col[2] green:green_col[2] blue:blue_col[2] alpha:1].CGColor,
     (id)[UIColor colorWithRed:base_col green:base_col blue:base_col alpha:1].CGColor];
     
     [refreshBackgroundView.layer insertSublayer:gradient atIndex:0];*/
    
    
    [self.tableView insertSubview:refreshBackgroundView atIndex:0];
}

- (void)viewDidUnload {
    // Relinquish ownership of anything that can be recreated in viewDidLoad or on demand.
    // For example: self.myOutlet = nil;;
    NSLog(@"unload");
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
    if (keys) {
        //[keys release];
        keys=nil;
    }
    if (list) {
        //[list release];
        list=nil;
    }
    
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            local_entries_data[i].label=nil;
            local_entries_data[i].fullpath=nil;
        }
        for (int i=0;i<27;i++) {
            for (int j=0;j<local_entries_count[i];j++) {
                local_entries[i][j].label=nil;
                local_entries[i][j].fullpath=nil;
            }
            local_entries[i]=NULL;
        }
        free(local_entries_data);
    }
    if (search_local_nb_entries) {
        
        for (int i=0;i<27;i++) {
            for (int j=0;j<search_local_entries_count[i];j++) {
                search_local_entries[i][j].label=nil;
                search_local_entries[i][j].fullpath=nil;
            }
            search_local_entries[i]=NULL;
        }
        
        search_local_nb_entries=0;
        free(search_local_entries_data);
    }
    
    
    if (indexTitles) {
        //[indexTitles release];
        indexTitles=nil;
    }
    if (indexTitlesSpace) {
        //[indexTitlesSpace release];
        indexTitlesSpace=nil;
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
/*- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
 static int flag=0;
 if (scrollView.contentOffset.y>=0) flag=0;
 if ((scrollView.contentOffset.y<-80.0)&&(!refreshControl.refreshing)&&(flag==0)) {
 [refreshControl beginRefreshing];
 flag=1;
 [refreshControl sendActionsForControlEvents:UIControlEventValueChanged];
 }
 }*/

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

-(void) updateLoadingInfos: (NSTimer *) theTimer {
    [waitingViewPlayer.progressView setProgress:detailViewController.waitingView.progressView.progress animated:YES];
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
        
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

@end
