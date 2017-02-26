//
//  RootViewControllerLocalBrowser.m
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define LIMITED_LIST_SIZE 1024

extern BOOL is_ios7;

NSString *cutpaste_filesrcpath=nil;

#include <sys/types.h>
#include <sys/sysctl.h>

#include "gme.h"

//SID2
#import "SidTune.h"

#include "unzip.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;
pthread_mutex_t db_mutexHVSCSTIL;
//static int shouldFillKeys;
static int local_flag;
static volatile int mPopupAnimation=0;

#import "RootViewControllerLocalBrowser.h"
#import "AppDelegate_Phone.h"
#import "DetailViewControllerIphone.h"
#import "QuartzCore/CAAnimation.h"
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

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

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/xattr.h>

static volatile int mUpdateToNewDB;
static volatile int mDatabaseCreationInProgress;
static volatile int db_checked=0;

extern "C" {
#include "md5.h"
}

static char browser_stil_info[MAX_STIL_DATA_LENGTH];
static char browser_song_md5[33];
static char **browser_sidtune_title,**browser_sidtune_name;


- (BOOL)addSkipBackupAttributeToItemAtPath:(NSString*)path
{
    //    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    //  NSString *documentsDirectory = [paths objectAtIndex:0];
    const char* filePath = [path fileSystemRepresentation];
    
    const char* attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    
    int result = setxattr(filePath, attrName, &attrValue, sizeof(attrValue), 0, 0);
    return result == 0;
}

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
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    NSString *defaultDBPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_USER];
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    NSString *pathToOldDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_TMP];
    NSError *error;
    NSFileManager *fileManager=[[NSFileManager alloc] init];
    
    pthread_mutex_lock(&db_mutex);
    
    if (mUpdateToNewDB) {
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
    [pool release];
    [fileManager release];
}

-(void) updateFilesDoNotBackupAttributes {
    NSError *error;
    NSArray *dirContent;
    int result;
    //BOOL isDir;
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    NSString *cpath=[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/Samples"];
    NSString *file;
    const char* attrName = "com.apple.MobileBackup";
    u_int8_t attrValue = 1;
    
    dirContent=[fileManager subpathsOfDirectoryAtPath:cpath error:&error];
    for (file in dirContent) {
        //NSLog(@"%@",file);
        //        [mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
        result = setxattr([[cpath stringByAppendingFormat:@"/%@",file] fileSystemRepresentation], attrName, &attrValue, sizeof(attrValue), 0, 0);
        if (result) NSLog(@"Issue %d when settings nobackup flag on %@",result,[cpath stringByAppendingFormat:@"/%@",file]);
    }
    [fileManager release];
}
// Creates a writable copy of the bundled default database in the application Documents directory.
- (void)createSamplesFromPackage:(BOOL)forceCreate {
    BOOL success;
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    NSError *error;
    
    NSString *samplesDocPath=[NSString stringWithFormat:@"%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents/Samples"]];
    NSString *samplesPkgPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Samples"];
    
    
    success = [fileManager fileExistsAtPath:samplesDocPath];
    if (success&&(forceCreate==FALSE)) {
        [fileManager release];
        return;
    }
    [fileManager removeItemAtPath:samplesDocPath error:&error];
    [fileManager copyItemAtPath:samplesPkgPath toPath:samplesDocPath error:&error];
    //update 'Do Not Backup' flag for directory & content (not sure it is required for the latest,...)
    [self addSkipBackupAttributeToItemAtPath:samplesDocPath];
    [self updateFilesDoNotBackupAttributes];
    
    [fileManager release];
}

// Creates a writable copy of the bundled default database in the application Documents directory.
- (void)createEditableCopyOfDatabaseIfNeeded:(bool)forceInit quiet:(int)quiet {
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
            [fileManager release];
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
    [fileManager release];
    
    mDatabaseCreationInProgress=1;
    
    if (mUpdateToNewDB) [self createSamplesFromPackage:TRUE];  //If upgrade to new version, recreate Samples dir
    else [self createSamplesFromPackage:FALSE];
    
    if (quiet) [self recreateDB];
    else {
        UIAlertView *alert1;
        if (forceInit) alert1 = [[[UIAlertView alloc] initWithTitle:@"Info"
                                                            message:NSLocalizedString(@"Database will now be recreated. Please validate & wait.",@"") delegate:self cancelButtonTitle:@"Recreate DB" otherButtonTitles:nil] autorelease];
        else {
            if (wrongversion) {
                alert1 = [[[UIAlertView alloc] initWithTitle:@"Info" message:
                           [NSString stringWithFormat:NSLocalizedString(@"Wrong database version: %d.%d. Will update to %d.%d. Please validate & wait.",@""),maj,min,VERSION_MAJOR,VERSION_MINOR] delegate:self cancelButtonTitle:@"Update DB" otherButtonTitles:nil] autorelease];
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

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    if (mDatabaseCreationInProgress) {
        [self recreateDB];
        UIAlertView *alert2 = [[[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Database created.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
        [alert2 show];
    }
    if (renameFile) {
        renameFile=0;
        if (buttonIndex==1) {
            t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
            UITextField *tf=[alertView textFieldAtIndex:0];
            //NSLog(@"rename %@ to %@",cur_local_entries[renameSec][renameIdx].label,tf.text);
            if (cur_local_entries[renameSec][renameIdx].label) [cur_local_entries[renameSec][renameIdx].label release];
            
            NSString *curPath,*tgtPath;
            curPath=[NSHomeDirectory() stringByAppendingPathComponent:cur_local_entries[renameSec][renameIdx].fullpath];
            tgtPath=[NSHomeDirectory() stringByAppendingPathComponent:cur_local_entries[renameSec][renameIdx].fullpath];
            
            tgtPath=[[tgtPath stringByDeletingLastPathComponent] stringByAppendingPathComponent:tf.text];
            //NSLog(@"rename %@ to %@",curPath,tgtPath);
            
            NSError *err;
            if ([mFileMngr moveItemAtPath:curPath toPath:tgtPath error:&err]==NO) {
                NSLog(@"Issue %d while renaming file %@",err.code,curPath);
                UIAlertView *removeAlert = [[[UIAlertView alloc] initWithTitle:@"Warning" message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while trying to renamefile.\n%@",@""),err.code,err.localizedDescription] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                [removeAlert show];
            } else {
                cur_local_entries[renameSec][renameIdx].label=[[NSString alloc] initWithString:tf.text];
                
                [cur_local_entries[renameSec][renameIdx].fullpath release];
                cur_local_entries[renameSec][renameIdx].fullpath=[[NSString alloc] initWithString:tgtPath];
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
            newPath=[[NSHomeDirectory() stringByAppendingPathComponent:currentPath] stringByAppendingPathComponent:tf.text];
            
            NSError *err;
            if ([mFileMngr createDirectoryAtPath:newPath withIntermediateDirectories:YES attributes:nil error:&err]==NO) {
                NSLog(@"Issue %d while create folder %@",err.code,newPath);
                UIAlertView *removeAlert = [[[UIAlertView alloc] initWithTitle:@"Warning" message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while creating folder\n%@",@""),err.code,newPath] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                [removeAlert show];
            } else {
                [self addSkipBackupAttributeToItemAtPath:newPath];
                
                [self listLocalFiles];
                
                [self.tableView reloadData];
            }
            //tf.text=[NSString stringWithString:cur_local_entries[section][indexPath.row].fullpath];
        }
    }
    
    
}

- (BOOL)alertViewShouldEnableFirstOtherButton:(UIAlertView *)alertView
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
    NSString *machine = [[[NSString alloc] initWithFormat:@"%s",name] autorelease];
    
    // Done with this
    free(name);
    
    return machine;
}

-(void) shortWait {
    [NSThread sleepForTimeInterval:0.1f];
}

-(void)showWaiting{
    waitingView.hidden=FALSE;
}

-(void)hideWaiting{
    waitingView.hidden=TRUE;
}

- (void)viewDidLoad {
    clock_t start_time,end_time;
    start_time=clock();
    childController=NULL;
    
    mFileMngr=[[NSFileManager alloc] init];
    
    NSString *strMachine=[self machine];
    mSlowDevice=0;
    NSRange r = [strMachine rangeOfString:@"iPhone1," options:NSCaseInsensitiveSearch];
    if (r.location != NSNotFound) {
        mSlowDevice=1;
    }
    r.location=NSNotFound;
    r = [strMachine rangeOfString:@"iPod1," options:NSCaseInsensitiveSearch];
    if (r.location != NSNotFound) {
        mSlowDevice=1;
    }
    
    mShowSubdir=0;
    
    ratingImg[0] = @"rating0.png";
    ratingImg[1] = @"rating1.png";
    ratingImg[2] = @"rating2.png";
    ratingImg[3] = @"rating3.png";
    ratingImg[4] = @"rating4.png";
    ratingImg[5] = @"rating5.png";
    
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
    [lpgr release];
    
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
        [currentPath retain];
    }
    
    UIButton *btn = [[[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)] autorelease];
    [btn setBackgroundImage:[UIImage imageNamed:@"nowplaying_fwd.png"] forState:UIControlStateNormal];
    btn.adjustsImageWhenHighlighted = YES;
    [btn addTarget:self action:@selector(goPlayer) forControlEvents:UIControlEventTouchUpInside];
    UIBarButtonItem *item = [[[UIBarButtonItem alloc] initWithCustomView: btn] autorelease];
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
    waitingView = [[UIView alloc] init];
    waitingView.backgroundColor=[UIColor blackColor];//[UIColor colorWithRed:0 green:0 blue:0 alpha:0.8f];
    waitingView.opaque=YES;
    waitingView.hidden=FALSE;
    waitingView.layer.cornerRadius=20;
    
    UIActivityIndicatorView *indView=[[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(50-20,50-20,40,40)];
    indView.activityIndicatorViewStyle=UIActivityIndicatorViewStyleWhiteLarge;
    [waitingView addSubview:indView];
    
    [indView startAnimating];
    [indView autorelease];
    
    waitingView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:waitingView];
    
    NSDictionary *views = NSDictionaryOfVariableBindings(waitingView);
    // width constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"H:[waitingView(100)]" options:0 metrics:nil views:views]];
    // height constraint
    [self.view addConstraints:[NSLayoutConstraint constraintsWithVisualFormat:@"V:[waitingView(100)]" options:0 metrics:nil views:views]];
    // center align
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterX relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterX multiplier:1.0 constant:0]];
    [self.view addConstraint:[NSLayoutConstraint constraintWithItem:self.view attribute:NSLayoutAttributeCenterY relatedBy:NSLayoutRelationEqual toItem:waitingView attribute:NSLayoutAttributeCenterY multiplier:1.0 constant:0]];
    /////////////////////////////////////////
    
    
    [super viewDidLoad];
    renameFile=0;
    createFolder=0;
    
    end_time=clock();
#ifdef LOAD_PROFILE
    NSLog(@"rootviewLB : %d",end_time-start_time);
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
    
    browser_sidtune_name=(char**)malloc(subsongs_nb*sizeof(char*));
    memset(browser_sidtune_name,0,sizeof(char*)*subsongs_nb);
    browser_sidtune_title=(char**)malloc(subsongs_nb*sizeof(char*));
    memset(browser_sidtune_title,0,sizeof(char*)*subsongs_nb);
    
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
                    if (browser_stil_info[idx]=='N') parser_status=4;
                    if (browser_stil_info[idx]=='T') parser_status=10;
                    break;
                    ///////////////////////////////////////////////
                    //"NAME: "
                    ///////////////////////////////////////////////
                case 4: // "N"
                    if (browser_stil_info[idx]=='A') parser_status=5;
                    break;
                case 5: // "NA"
                    if (browser_stil_info[idx]=='M') parser_status=6;
                    break;
                case 6: // "NAM"
                    if (browser_stil_info[idx]=='E') parser_status=7;
                    break;
                case 7: // "NAME"
                    if (browser_stil_info[idx]==':') parser_status=8;
                    break;
                case 8: // "NAME:"
                    if (browser_stil_info[idx]==' ') {
                        parser_status=9;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                    }
                    break;
                case 9: // "NAME: "
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
                    ///////////////////////////////////////////////
                    //"TITLE: "
                    ///////////////////////////////////////////////
                case 10: // "T"
                    if (browser_stil_info[idx]=='I') parser_status=11;
                    break;
                case 11: // "TI"
                    if (browser_stil_info[idx]=='T') parser_status=12;
                    break;
                case 12: // "TIT"
                    if (browser_stil_info[idx]=='L') parser_status=13;
                    break;
                case 13: // "TITL"
                    if (browser_stil_info[idx]=='E') parser_status=14;
                    break;
                case 14: // "TITLE"
                    if (browser_stil_info[idx]==':') parser_status=15;
                    break;
                case 15: // "TITLE:"
                    if (browser_stil_info[idx]==' ') {
                        parser_status=16;
                        tmp_str[0]=0;
                        tmp_str_idx=0;
                    }
                    break;
                case 16: // "TITLE: "
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



-(void)listLocalFiles {
    NSString *file,*cpath;
    NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
    NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
    NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
    NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
    NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
    NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
    NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_MODPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extXMP=[SUPPORTED_FILETYPE_XMP componentsSeparatedByString:@","];
    NSArray *filetype_extDUMB=[SUPPORTED_FILETYPE_DUMB componentsSeparatedByString:@","];
    NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
    NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extSEXYPSF=[SUPPORTED_FILETYPE_SEXYPSF componentsSeparatedByString:@","];
    NSArray *filetype_extLAZYUSF=[SUPPORTED_FILETYPE_LAZYUSF componentsSeparatedByString:@","];
    NSArray *filetype_extXSF=[SUPPORTED_FILETYPE_XSF componentsSeparatedByString:@","];
    NSArray *filetype_extVGMSTREAM=[SUPPORTED_FILETYPE_VGMSTREAM componentsSeparatedByString:@","];
    NSArray *filetype_extAOSDK=[SUPPORTED_FILETYPE_AOSDK componentsSeparatedByString:@","];
    NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
    NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
    NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
    NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];
    NSArray *filetype_extVGM=[SUPPORTED_FILETYPE_VGM componentsSeparatedByString:@","];
    NSMutableArray *filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extMDX count]+[filetype_extPMD count]+[filetype_extSID count]+[filetype_extSTSOUND count]+
                                  [filetype_extSC68 count]+[filetype_extARCHIVE count]+[filetype_extUADE count]+[filetype_extMODPLUG count]+[filetype_extXMP count]+[filetype_extDUMB count]+
                                  [filetype_extGME count]+[filetype_extADPLUG count]+[filetype_extSEXYPSF count]+[filetype_extLAZYUSF count]+[filetype_extXSF count]+[filetype_extVGMSTREAM count]+
                                  [filetype_extAOSDK count]+[filetype_extHVL count]+[filetype_extGSF count]+
                                  [filetype_extASAP count]+[filetype_extWMIDI count]+[filetype_extVGM count]];
    NSArray *filetype_extARCHIVEFILE=[SUPPORTED_FILETYPE_ARCFILE componentsSeparatedByString:@","];
    NSMutableArray *archivetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extARCHIVEFILE count]];
    NSArray *filetype_extGME_MULTISONGSFILE=[SUPPORTED_FILETYPE_GME_MULTISONGS componentsSeparatedByString:@","];
    NSMutableArray *gme_multisongstype_ext=[NSMutableArray arrayWithCapacity:[filetype_extGME_MULTISONGSFILE count]];
    NSArray *filetype_extSID_MULTISONGSFILE=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
    NSMutableArray *sid_multisongstype_ext=[NSMutableArray arrayWithCapacity:[filetype_extSID_MULTISONGSFILE count]];
    
    NSMutableArray *all_multisongstype_ext=[NSMutableArray arrayWithCapacity:[filetype_extGME_MULTISONGSFILE count]+[filetype_extSID_MULTISONGSFILE count]];
    
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int err;
    char sqlStatement[1024];
    sqlite3_stmt *stmt;
    int local_entries_index,local_nb_entries_limit;
    int browseType;
    int shouldStop=0;
    
    NSRange r;
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    search_local=0;
    if (mSearch) {
        search_local=1;
        
        if (search_local_nb_entries) {
            search_local_nb_entries=0;
            free(search_local_entries_data);
        }
        search_local_entries_data=(t_local_browse_entry*)malloc(local_nb_entries*sizeof(t_local_browse_entry));
        
        for (int i=0;i<27;i++) {
            search_local_entries_count[i]=0;
            if (local_entries_count[i]) search_local_entries[i]=&(search_local_entries_data[search_local_nb_entries]);
            for (int j=0;j<local_entries_count[i];j++)  {
                r.location=NSNotFound;
                r = [local_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
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
        return;
    }
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) != SQLITE_OK) db=NULL;
    
    [filetype_ext addObjectsFromArray:filetype_extMDX];
    [filetype_ext addObjectsFromArray:filetype_extPMD];
    [filetype_ext addObjectsFromArray:filetype_extSID];
    [filetype_ext addObjectsFromArray:filetype_extSTSOUND];
    [filetype_ext addObjectsFromArray:filetype_extSC68];
    [filetype_ext addObjectsFromArray:filetype_extARCHIVE];
    [filetype_ext addObjectsFromArray:filetype_extUADE];
    [filetype_ext addObjectsFromArray:filetype_extMODPLUG];
    [filetype_ext addObjectsFromArray:filetype_extXMP];
    [filetype_ext addObjectsFromArray:filetype_extDUMB];
    [filetype_ext addObjectsFromArray:filetype_extGME];
    [filetype_ext addObjectsFromArray:filetype_extADPLUG];
    [filetype_ext addObjectsFromArray:filetype_extSEXYPSF];
    [filetype_ext addObjectsFromArray:filetype_extLAZYUSF];
    [filetype_ext addObjectsFromArray:filetype_extXSF];
    [filetype_ext addObjectsFromArray:filetype_extVGMSTREAM];
    [filetype_ext addObjectsFromArray:filetype_extAOSDK];
    [filetype_ext addObjectsFromArray:filetype_extHVL];
    [filetype_ext addObjectsFromArray:filetype_extGSF];
    [filetype_ext addObjectsFromArray:filetype_extASAP];
    [filetype_ext addObjectsFromArray:filetype_extWMIDI];
    [filetype_ext addObjectsFromArray:filetype_extVGM];
    
    [archivetype_ext addObjectsFromArray:filetype_extARCHIVEFILE];
    [gme_multisongstype_ext addObjectsFromArray:filetype_extGME_MULTISONGSFILE];
    [sid_multisongstype_ext addObjectsFromArray:filetype_extSID_MULTISONGSFILE];
    
    [all_multisongstype_ext addObjectsFromArray:filetype_extGME_MULTISONGSFILE];
    [all_multisongstype_ext addObjectsFromArray:filetype_extSID_MULTISONGSFILE];
    
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            [local_entries_data[i].label release];
            [local_entries_data[i].fullpath release];
        }
        free(local_entries_data);local_entries_data=NULL;
        local_nb_entries=0;
    }
    for (int i=0;i<27;i++) local_entries_count[i]=0;
    
    // First check count for each section
    cpath=[NSHomeDirectory() stringByAppendingPathComponent:  currentPath];
    //NSLog(@"%@\n%@",cpath,currentPath);
    //Check if it is a directory or an archive
    BOOL isDirectory;
    browseType=0;
    if ([mFileMngr fileExistsAtPath:cpath isDirectory:&isDirectory]) {
        if (!isDirectory) {
            //file:check if archive or multisongs
            NSString *extension=[[[cpath lastPathComponent] pathExtension] uppercaseString];
            if ([archivetype_ext indexOfObject:extension]!=NSNotFound) browseType=1;
            //check if Multisongs file
            else if ([gme_multisongstype_ext indexOfObject:extension]!=NSNotFound) browseType=2;
            else if ([sid_multisongstype_ext indexOfObject:extension]!=NSNotFound) browseType=3;
        }
    }
    
    if (browseType==3) {//SID
        SidTune *mSidTune=new SidTune([cpath UTF8String],0,true);
        
        if ((mSidTune==NULL)||(mSidTune->cache.get()==0)) {
            NSLog(@"SID SidTune init error");
            if (mSidTune) {delete mSidTune;mSidTune=NULL;}
        } else {
            SidTuneInfo sidtune_info;
            sidtune_info=mSidTune->getInfo();
            
            
            
            //Compute MD5
            memset(browser_song_md5,0,33);
            int tmp_md5_data_size=sidtune_info.c64dataLen+2*3+sizeof(sidtune_info.songSpeed)*sidtune_info.songs;
            char *tmp_md5_data=(char*)malloc(tmp_md5_data_size);
            memset(tmp_md5_data,0,tmp_md5_data_size);
            int ofs_md5_data=0;
            unsigned char tmp[2];
            memcpy(tmp_md5_data,mSidTune->cache.get()+mSidTune->fileOffset,sidtune_info.c64dataLen);
            ofs_md5_data+=sidtune_info.c64dataLen;
            // Include INIT and PLAY address.
            writeLEword(tmp,sidtune_info.initAddr);
            memcpy(tmp_md5_data+ofs_md5_data,tmp,2);
            ofs_md5_data+=2;
            writeLEword(tmp,sidtune_info.playAddr);
            memcpy(tmp_md5_data+ofs_md5_data,tmp,2);
            ofs_md5_data+=2;
            // Include number of songs.
            writeLEword(tmp,sidtune_info.songs);
            memcpy(tmp_md5_data+ofs_md5_data,tmp,2);
            ofs_md5_data+=2;
            
            // Include song speed for each song.
            for (unsigned int s = 1; s <= sidtune_info.songs; s++)
            {
                mSidTune->selectSong(s);
                memcpy(tmp_md5_data+ofs_md5_data,&mSidTune->info.songSpeed,sizeof(mSidTune->info.songSpeed));
                //NSLog(@"sp : %d %d %d",s,mSidTune->info.songSpeed,sizeof(mSidTune->info.songSpeed));
                ofs_md5_data+=sizeof(mSidTune->info.songSpeed);
            }
            // Deal with PSID v2NG clock speed flags: Let only NTSC
            // clock speed change the MD5 fingerprint. That way the
            // fingerprint of a PAL-speed sidtune in PSID v1, v2, and
            // PSID v2NG format is the same.
            if ( mSidTune->info.clockSpeed == SIDTUNE_CLOCK_NTSC ) {
                memcpy(tmp_md5_data+ofs_md5_data,&mSidTune->info.clockSpeed,sizeof(mSidTune->info.clockSpeed));
                ofs_md5_data+=sizeof(mSidTune->info.clockSpeed);
                //myMD5.append(&info.clockSpeed,sizeof(info.clockSpeed));
            }
            md5_from_buffer(browser_song_md5,33,tmp_md5_data,tmp_md5_data_size);
            browser_song_md5[32]=0;
            free(tmp_md5_data);
            
            //Get STIL info
            browser_stil_info[0]=0;
            [self getStilInfo:(char*)[cpath UTF8String]];
            
            [self sid_parseStilInfo:sidtune_info.songs];
            
            for (int i=0;i<sidtune_info.songs;i++){
                SidTuneInfo s_info;
                file=nil;
                mSidTune->selectSong(i);
                s_info=mSidTune->getInfo();
                
                if (browser_sidtune_name[i]) file=[NSString stringWithFormat:@"%.3d-%s",i,browser_sidtune_name[i]];
                else if (browser_sidtune_title[i]) file=[NSString stringWithFormat:@"%.3d-%s",i,browser_sidtune_title[i]];
                else if (s_info.infoString[0][0]) file=[NSString stringWithFormat:@"%.3d-%s",i,s_info.infoString[0]];
                else file=[NSString stringWithFormat:@"%.3d-%@",i,[cpath lastPathComponent]];
                
                int filtered=0;
                if ((mSearch)&&([mSearchText length]>0)) {
                    filtered=1;
                    NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                    if (r.location != NSNotFound) {
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
                local_entries_data=(t_local_browse_entry *)malloc(local_nb_entries*sizeof(t_local_browse_entry));
                if (!local_entries_data) {
                    //Not enough memory
                    //try to allocate less entries
                    local_nb_entries_limit=LIMITED_LIST_SIZE;
                    if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                    local_entries_data=(t_local_browse_entry *)malloc(local_nb_entries_limit*sizeof(t_local_browse_entry));
                    if (local_entries_data==NULL) {
                        //show alert : cannot list
                        UIAlertView *memAlert = [[[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                        [memAlert show];
                    } else {
                        //show alert : limited list
                        UIAlertView *memAlert = [[[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
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
                    
                    for (int i=0;i<sidtune_info.songs;i++){
                        SidTuneInfo s_info;
                        file=nil;
                        mSidTune->selectSong(i);
                        s_info=mSidTune->getInfo();
                        
                        if (browser_sidtune_name[i]) file=[NSString stringWithFormat:@"%.3d-%s",i,browser_sidtune_name[i]];
                        else if (browser_sidtune_title[i]) file=[NSString stringWithFormat:@"%.3d-%s",i,browser_sidtune_title[i]];
                        else if (s_info.infoString[0][0]) file=[NSString stringWithFormat:@"%.3d-%s",i,s_info.infoString[0]];
                        else file=[NSString stringWithFormat:@"%.3d-%@",i,[cpath lastPathComponent]];
                        
                        /*if (s_info.infoString[0][0]) {
                            file=[NSString stringWithFormat:@"%.3d-%s",i,s_info.infoString[0]];
                        } else {
                            file=[NSString stringWithFormat:@"%.3d-%@",i,[cpath lastPathComponent]];
                        }*/
                        
                        int filtered=0;
                        if ((mSearch)&&([mSearchText length]>0)) {
                            filtered=1;
                            NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                            if (r.location != NSNotFound) {
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
                            
                            sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[[file lastPathComponent] UTF8String],[local_entries[index][local_entries_count[index]].fullpath UTF8String]);
                            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                            if (err==SQLITE_OK){
                                while (sqlite3_step(stmt) == SQLITE_ROW) {
                                    signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                    if (rating<0) rating=0;
                                    if (rating>5) rating=5;
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
            if (mSidTune) {delete mSidTune;mSidTune=NULL;}
            if (browser_sidtune_title) {
                for (int i=0;i<sidtune_info.songs;i++)
                    if (browser_sidtune_title[i]) free(browser_sidtune_title[i]);
                free(browser_sidtune_title);
                browser_sidtune_title=NULL;
            }
            if (browser_sidtune_name) {
                for (int i=0;i<sidtune_info.songs;i++)
                    if (browser_sidtune_name[i]) free(browser_sidtune_name[i]);
                free(browser_sidtune_name);
                browser_sidtune_name=NULL;
            }
        }
    } else if (browseType==2) { //GME Multisongs
        // Open music file in new emulator
        Music_Emu* gme_emu;
        
        gme_err_t gme_err=gme_open_file( [cpath UTF8String], &gme_emu, gme_info_only );
        if (gme_err) {
            NSLog(@"gme_open_file error: %s",gme_err);
        } else {
            gme_info_t *gme_info;
            for (int i=0;i<gme_track_count( gme_emu );i++) {
                if (gme_track_info( gme_emu, &gme_info, i )==0) {
                    file=nil;
                    if (gme_info->song) {
                        if (gme_info->song[0]) file=[NSString stringWithFormat:@"%.3d-%s",i,gme_info->song];
                    }
                    if (!file) {
                        if (gme_info->game) {
                            if (gme_info->game[0]) file=[NSString stringWithFormat:@"%.3d-%s",i,gme_info->game];
                        }
                    }
                    if (!file) {
                        file=[NSString stringWithFormat:@"%.3d-%@",i,[cpath lastPathComponent]];
                    }
                    
                    int filtered=0;
                    if ((mSearch)&&([mSearchText length]>0)) {
                        filtered=1;
                        NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        if (r.location != NSNotFound) {
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
            }
            gme_delete(gme_emu);
        }
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)malloc(local_nb_entries*sizeof(t_local_browse_entry));
            if (!local_entries_data) {
                //Not enough memory
                //try to allocate less entries
                local_nb_entries_limit=LIMITED_LIST_SIZE;
                if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                local_entries_data=(t_local_browse_entry *)malloc(local_nb_entries_limit*sizeof(t_local_browse_entry));
                if (local_entries_data==NULL) {
                    //show alert : cannot list
                    UIAlertView *memAlert = [[[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                    [memAlert show];
                } else {
                    //show alert : limited list
                    UIAlertView *memAlert = [[[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
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
                    
                    for (int i=0;i<gme_track_count( gme_emu );i++) {
                        if (gme_track_info( gme_emu, &gme_info, i )==0) {
                            file=nil;
                            if (gme_info->song) {
                                if (gme_info->song[0]) file=[NSString stringWithFormat:@"%.3d-%s",i,gme_info->song];
                            }
                            if (!file) {
                                if (gme_info->game) {
                                    if (gme_info->game[0]) file=[NSString stringWithFormat:@"%.3d-%s",i,gme_info->game];
                                }
                            }
                            if (!file) {
                                file=[NSString stringWithFormat:@"%.3d-%@",i,[cpath lastPathComponent]];
                            }
                            
                            int filtered=0;
                            if ((mSearch)&&([mSearchText length]>0)) {
                                filtered=1;
                                NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                if (r.location != NSNotFound) {
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
                                
                                sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[[file lastPathComponent] UTF8String],[local_entries[index][local_entries_count[index]].fullpath UTF8String]);
                                err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                if (err==SQLITE_OK){
                                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                                        signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                        if (rating<0) rating=0;
                                        if (rating>5) rating=5;
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
    } else if (browseType==1) { //FEX Archive (zip,7z,rar,rsn)
        fex_type_t ftype;
        fex_t* fex;
        const char *path=[cpath UTF8String];
        /* Determine file's type */
        if (fex_identify_file( &ftype, path)) {
            NSLog(@"fex cannot determine type of %s",path);
        }
        /* Only open files that fex can handle */
        if ( ftype != NULL ) {
            if (fex_open_type( &fex, path, ftype )) {
                NSLog(@"cannot fex open : %s",path);
            } else {
                int is_rsn=0;
                NSString *extension=[[[cpath lastPathComponent] pathExtension] uppercaseString];
                if ([extension caseInsensitiveCompare:@"rsn"]==NSOrderedSame) is_rsn=1;
                
                
                while ( !fex_done( fex ) ) {
                    file=[NSString stringWithFormat:@"%s",fex_name(fex)];
                    NSString *extension;// = [[file pathExtension] uppercaseString];
                    NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                    
                    NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                    extension = (NSString *)[temparray_filepath lastObject];
                    [temparray_filepath removeLastObject];
                    file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
                    
                    
                    
                    int filtered=0;
                    if ((mSearch)&&([mSearchText length]>0)) {
                        filtered=1;
                        NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        if (r.location != NSNotFound) {
                            /*if(r.location== 0)*/ filtered=0;
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
                    
                    if (fex_next( fex )) {
                        NSLog(@"Error during fex scanning");
                        break;
                    }
                }
                fex_close( fex );
            }
            fex = NULL;
        } else {
            //NSLog( @"Skipping unsupported archive: %s\n", path );
        }
        
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)malloc(local_nb_entries*sizeof(t_local_browse_entry));
            if (!local_entries_data) {
                //Not enough memory
                //try to allocate less entries
                local_nb_entries_limit=LIMITED_LIST_SIZE;
                if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                local_entries_data=(t_local_browse_entry *)malloc(local_nb_entries_limit*sizeof(t_local_browse_entry));
                if (local_entries_data==NULL) {
                    //show alert : cannot list
                    UIAlertView *memAlert = [[[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                    [memAlert show];
                } else {
                    //show alert : limited list
                    UIAlertView *memAlert = [[[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
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
                if (fex_open_type( &fex, path, ftype )) {
                    NSLog(@"cannot fex open : %s",path);
                } else {
                    int arc_counter=0;
                    while ( !fex_done( fex ) ) {
                        file=[NSString stringWithFormat:@"%s",fex_name(fex)];
                        NSString *extension;// = [[file pathExtension] uppercaseString];
                        NSString *file_no_ext;// = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                        
                        NSMutableArray *temparray_filepath=[NSMutableArray arrayWithArray:[[[file lastPathComponent] uppercaseString] componentsSeparatedByString:@"."]];
                        extension = (NSString *)[temparray_filepath lastObject];
                        [temparray_filepath removeLastObject];
                        file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
                        
                        
                        int filtered=0;
                        if ((mSearch)&&([mSearchText length]>0)) {
                            filtered=1;
                            NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                            if (r.location != NSNotFound) {
                                /*if(r.location== 0)*/ filtered=0;
                            }
                        }
                        if (!filtered) {
                            int found=0;
                            
                            if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                            else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                            
                            if (found)  {
                                const char *str;
                                char tmp_str[1024];//,*tmp_convstr;
                                int toto=0;
                                str=[[file lastPathComponent] UTF8String];
                                if ([extension caseInsensitiveCompare:@"mdx"]==NSOrderedSame ) {
                                    [[file lastPathComponent] getFileSystemRepresentation:tmp_str maxLength:1024];
                                    //tmp_convstr=mdx_make_sjis_to_syscharset(tmp_str);
                                    toto=1;
                                }
                                int index=0;
                                if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                local_entries[index][local_entries_count[index]].type=1;
                                //check if Archive file
                                if ([archivetype_ext indexOfObject:extension]!=NSNotFound) local_entries[index][local_entries_count[index]].type=2;
                                else if ([archivetype_ext indexOfObject:file_no_ext]!=NSNotFound) local_entries[index][local_entries_count[index]].type=2;
                                //check if Multisongs file
                                if (toto) {
                                    local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithCString:tmp_str encoding:NSUTF8StringEncoding];
                                    //	free(tmp_convstr);
                                } else local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                
                                local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@@%d",currentPath,arc_counter];
                                
                                local_entries[index][local_entries_count[index]].rating=0;
                                local_entries[index][local_entries_count[index]].playcount=0;
                                local_entries[index][local_entries_count[index]].song_length=0;
                                local_entries[index][local_entries_count[index]].songs=0;
                                local_entries[index][local_entries_count[index]].channels_nb=0;
                                
                                sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[[file lastPathComponent] UTF8String],[local_entries[index][local_entries_count[index]].fullpath UTF8String]);
                                err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                if (err==SQLITE_OK){
                                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                                        signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                        if (rating<0) rating=0;
                                        if (rating>5) rating=5;
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
                        if (fex_next( fex )) {
                            NSLog(@"Error during fex scanning");
                            break;
                        }
                    }
                    fex_close( fex );
                }
                fex = NULL;
            }
            
        }
    } else {
        //        clock_t start_time,end_time;
        //        start_time=clock();
        NSError *error;
        NSRange rdir;
        NSArray *dirContent;//
        BOOL isDir;
        if (mShowSubdir) dirContent=[mFileMngr subpathsOfDirectoryAtPath:cpath error:&error];
        else dirContent=[mFileMngr contentsOfDirectoryAtPath:cpath error:&error];
        for (file in dirContent) {
            //NSLog(@"%@",file);
            //check if dir
            //rdir.location=NSNotFound;
            //rdir = [file rangeOfString:@"." options:NSCaseInsensitiveSearch];
            [mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
            if (isDir) { //rdir.location == NSNotFound) {  //assume it is a dir if no "." in file name
                rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                    if (1/*[file compare:@"tmpArchive"]!=NSOrderedSame*/) {
                        //do not display dir if subdir mode is on
                        int filtered=mShowSubdir;
                        if (!filtered) {
                            if ((mSearch)&&([mSearchText length]>0)) {
                                filtered=1;
                                NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                if (r.location != NSNotFound) {
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
                    [temparray_filepath removeLastObject];
                    file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
                    
                    
                    int filtered=0;
                    if ((mSearch)&&([mSearchText length]>0)) {
                        filtered=1;
                        NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        if (r.location != NSNotFound) {
                            /*if(r.location== 0)*/ filtered=0;
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
                }
            }
        }
        //        end_time=clock();
        //        NSLog(@"detail1 : %d",end_time-start_time);
        //        start_time=end_time;
        
        
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)malloc(local_nb_entries*sizeof(t_local_browse_entry));
            if (!local_entries_data) {
                //Not enough memory
                //try to allocate less entries
                local_nb_entries_limit=LIMITED_LIST_SIZE;
                if (local_nb_entries_limit>local_nb_entries) local_nb_entries_limit=local_nb_entries;
                local_entries_data=(t_local_browse_entry *)malloc(local_nb_entries_limit*sizeof(t_local_browse_entry));
                if (local_entries_data==NULL) {
                    //show alert : cannot list
                    UIAlertView *memAlert = [[[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                    [memAlert show];
                } else {
                    //show alert : limited list
                    UIAlertView *memAlert = [[[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Browser not enough mem. Limited.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
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
                
                //                end_time=clock();
                //                NSLog(@"detail2 : %d",end_time-start_time);
                //                start_time=end_time;
                // Second check count for each section
                for (file in dirContent) {
                    if (shouldStop) break;
                    //rdir.location=NSNotFound;
                    // rdir = [file rangeOfString:@"." options:NSCaseInsensitiveSearch];
                    [mFileMngr fileExistsAtPath:[cpath stringByAppendingFormat:@"/%@",file] isDirectory:&isDir];
                    if (isDir) { //rdir.location == NSNotFound) {  //assume it is a dir if no "." in file name
                        rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                        if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                            if (1/*[file compare:@"tmpArchive"]!=NSOrderedSame*/) {
                                //do not display dir if subdir mode is on
                                int filtered=mShowSubdir;
                                if (!filtered) {
                                    if ((mSearch)&&([mSearchText length]>0)) {
                                        filtered=1;
                                        NSRange r = [file rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                        if (r.location != NSNotFound) {
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
                            [temparray_filepath removeLastObject];
                            file_no_ext=[temparray_filepath componentsJoinedByString:@"."];
                            
                            
                            int filtered=0;
                            if ((mSearch)&&([mSearchText length]>0)) {
                                filtered=1;
                                NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                if (r.location != NSNotFound) {
                                    /*if(r.location== 0)*/ filtered=0;
                                }
                            }
                            if (!filtered) {
                                int found=0;
                                
                                if ([filetype_ext indexOfObject:extension]!=NSNotFound) found=1;
                                else if ([filetype_ext indexOfObject:file_no_ext]!=NSNotFound) found=1;
                                
                                
                                if (found)  {
                                    const char *str;
                                    char tmp_str[1024];//,*tmp_convstr;
                                    int toto=0;
                                    str=[[file lastPathComponent] UTF8String];
                                    if ([extension caseInsensitiveCompare:@"mdx"]==NSOrderedSame ) {
                                        [[file lastPathComponent] getFileSystemRepresentation:tmp_str maxLength:1024];
                                        //tmp_convstr=mdx_make_sjis_to_syscharset(tmp_str);
                                        toto=1;
                                    }
                                    int index=0;
                                    if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                                    if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                                    local_entries[index][local_entries_count[index]].type=1;
                                    //check if Archive file
                                    if ([archivetype_ext indexOfObject:extension]!=NSNotFound) local_entries[index][local_entries_count[index]].type=2;
                                    else if ([archivetype_ext indexOfObject:file_no_ext]!=NSNotFound) local_entries[index][local_entries_count[index]].type=2;
                                    //check if Multisongs file
                                    else if ([all_multisongstype_ext indexOfObject:extension]!=NSNotFound) local_entries[index][local_entries_count[index]].type=3;
                                    else if ([all_multisongstype_ext indexOfObject:file_no_ext]!=NSNotFound) local_entries[index][local_entries_count[index]].type=3;
                                    if (toto) {
                                        local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithCString:tmp_str encoding:NSUTF8StringEncoding];
                                        //	free(tmp_convstr);
                                    } else local_entries[index][local_entries_count[index]].label=[[NSString alloc ] initWithString:[file lastPathComponent]];
                                    
                                    local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%@/%@",currentPath,file];
                                    
                                    local_entries[index][local_entries_count[index]].rating=0;
                                    local_entries[index][local_entries_count[index]].playcount=0;
                                    local_entries[index][local_entries_count[index]].song_length=0;
                                    local_entries[index][local_entries_count[index]].songs=0;
                                    local_entries[index][local_entries_count[index]].channels_nb=0;
                                    
                                    sprintf(sqlStatement,"SELECT play_count,rating,length,channels,songs FROM user_stats WHERE name=\"%s\" and fullpath=\"%s/%s\"",[[file lastPathComponent] UTF8String],[currentPath UTF8String],[file UTF8String]);
                                    err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
                                    
                                    //printf("%s\n",sqlStatement);
                                    
                                    if (err==SQLITE_OK){
                                        while (sqlite3_step(stmt) == SQLITE_ROW) {
                                            signed char rating=(signed char)sqlite3_column_int(stmt, 1);
                                            if (rating<0) rating=0;
                                            if (rating>5) rating=5;
                                            
                                            //printf("ch: %d - sl: %d\n",(int)sqlite3_column_int(stmt, 3),(int)sqlite3_column_int(stmt, 2));
                                            
                                            local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt, 0);
                                            local_entries[index][local_entries_count[index]].rating=rating;
                                            local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt, 2);
                                            local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt, 3);
                                            local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt, 4);
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
                }
                //                end_time=clock();
                //                NSLog(@"detail1 : %d",end_time-start_time);
            }
        }
    }
    
    if (db) {
        sqlite3_close(db);
        pthread_mutex_unlock(&db_mutex);
    }
    
    
    return;
}

-(void) viewWillAppear:(BOOL)animated {
    if (!is_ios7) {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleBlack];
        [self.sBar setBarStyle:UIBarStyleBlack];
    } else {
        [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
        [self.sBar setBarStyle:UIBarStyleDefault];
        [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    }
    
    if (keys) {
        [keys release];
        keys=nil;
    }
    if (list) {
        [list release];
        list=nil;
    }
    if (childController) {
        [childController release];
        childController = NULL;
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
    /////////////
    if (detailViewController.mShouldHaveFocus) {
        detailViewController.mShouldHaveFocus=0;
        [self.navigationController pushViewController:detailViewController animated:(mSlowDevice?NO:YES)];
    } else {
        if (mShowSubdir==0) shouldFillKeys=1; //performance limit-> no update if listing all files
        
        [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
        [self shortWait];
        [self fillKeys];
        [tableView reloadData];
        [self hideWaiting];
    }
    
    [super viewWillAppear:animated];
    [self hideWaiting];
    
    
    //[tableView reloadData];
    //[self.view setNeedsLayout];
    //[self.view layoutIfNeeded];
    //[tableView reloadData];
    //[self shortWait];
}

-(void) refreshViewAfterDownload {
    //    if (mShowSubdir==0) {
    
    if (childController) [childController refreshViewAfterDownload];
    else {
        
        shouldFillKeys=1;
        [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
        [self shortWait];
        
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
        
        [self hideWaiting];
    }
    //}
}

- (void)viewDidAppear:(BOOL)animated {
    [self hideWaiting];
    
    
    /*[tableView setNeedsLayout];
     [tableView layoutSubviews];
     [tableView layoutIfNeeded];
     [tableView reloadData];*/
    
    [super viewDidAppear:animated];
    //[tableView reloadData];
}

- (void)viewDidDisappear:(BOOL)animated {
    [self hideWaiting];
    if (childController) {
        [childController viewDidDisappear:FALSE];
    }
    [super viewDidDisappear:animated];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
    [tableView reloadData];
}

// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    [tableView reloadData];
    return YES;
}

#pragma mark - SWTableViewDelegate
/*
- (void)swipeableTableViewCell:(SWTableViewCell *)cell didTriggerLeftUtilityButtonWithIndex:(NSInteger)index {
    NSIndexPath *indexPath = [self.tableView indexPathForCell:cell];
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    int section=indexPath.section-2;
    
    switch (index) {
        case 0: {
            //rename
            renameFile=1;
            renameSec=section;renameIdx=indexPath.row;
            
            alertRename=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Enter new name",@"") message:nil delegate:self cancelButtonTitle:NSLocalizedString(@"Cancel",@"") otherButtonTitles:NSLocalizedString(@"Ok",@""),nil] autorelease];
            [alertRename setAlertViewStyle:UIAlertViewStylePlainTextInput];
            UITextField *tf=[alertRename textFieldAtIndex:0];
            tf.text=[NSString stringWithString:cur_local_entries[section][indexPath.row].label];
            [alertRename show];
            
            break;
        }
        default:
            break;
    }
}

- (void)swipeableTableViewCell:(SWTableViewCell *)cell didTriggerRightUtilityButtonWithIndex:(NSInteger)index {
    switch (index) {
        case 0:
        {
            // Delete button was pressed
            NSIndexPath *indexPath = [self.tableView indexPathForCell:cell];
            
            //delete entry
            t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
            int section=indexPath.section-2;
            NSString *fullpath=[NSHomeDirectory() stringByAppendingPathComponent:cur_local_entries[section][indexPath.row].fullpath];
            NSError *err;
            
            if ([mFileMngr removeItemAtPath:fullpath error:&err]!=YES) {
                NSLog(@"Issue %d while removing: %@",err.code,fullpath);
                UIAlertView *removeAlert = [[[UIAlertView alloc] initWithTitle:@"Warning" message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while trying to delete entry.\n%@",@""),err.code,err.localizedDescription] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                [removeAlert show];
            } else {
                if (cur_local_entries[section][indexPath.row].type==0) { //Dir
                    DBHelper::deleteStatsDirDB(fullpath);
                }
                if (cur_local_entries[section][indexPath.row].type&3) { //File
                    DBHelper::deleteStatsFileDB(fullpath);
                }
                
                [self listLocalFiles];
                [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
                [self.tableView reloadData];
            }
            
            //            [self.tableView deleteRowsAtIndexPaths:@[cellIndexPath]
            //                                  withRowAnimation:UITableViewRowAnimationAutomatic];
            break;
        }
        default:
            break;
    }
}

- (BOOL)swipeableTableViewCellShouldHideUtilityButtonsOnSwipe:(SWTableViewCell *)cell {
    return YES;
}
*/
#pragma mark - Table view data source

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
    if (mSearch) return 0;
    
    if (section==0) return 0;
    if (section==1) return 0;
    if ((search_local?search_local_entries_count[section-2]:local_entries_count[section-2])) {
        return 24;
    } else return 0;
    return 0;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    NSString *lbl=nil;
    
    if (mSearch) lbl=nil;
    else if (section==0) lbl=nil;
    else if (section==1) lbl=@"";
    else if ((search_local?search_local_entries_count[section-2]:local_entries_count[section-2])) lbl=[indexTitlesSpace objectAtIndex:section];
    else lbl=nil;
    
    
    if (lbl)
        /*if ([lbl compare:@""]!=NSOrderedSame)*/{
    UIView *customView = [[[UIView alloc] initWithFrame: CGRectMake(0.0, 0.0, tableView.bounds.size.width, 24.0)] autorelease];
    customView.backgroundColor = [UIColor colorWithRed: 0.7f green: 0.7f blue: 0.7f alpha: 1.0f];
    
    CALayer *layerU = [CALayer layer];
    layerU.frame = CGRectMake(0.0, 0.0, tableView.bounds.size.width, 1.0);
    layerU.backgroundColor = [[UIColor colorWithRed: 183.0f/255.0f green: 193.0f/255.0f blue: 199.0f/255.0f alpha: 1.00] CGColor];
    [customView.layer insertSublayer:layerU atIndex:0];
    
    CAGradientLayer *gradient = [CAGradientLayer layer];
    gradient.frame = CGRectMake(0.0, 1.0, tableView.bounds.size.width, 22.0);
    gradient.colors = [NSArray arrayWithObjects:(id)[[UIColor colorWithRed: 144.0f/255.0f green: 159.0f/255.0f blue: 177.0f/255.0f alpha: 1.00] CGColor],
                       (id)[[UIColor colorWithRed: 183.0f/255.0f green: 193.0f/255.0f blue: 199.0f/255.0f  alpha: 1.00] CGColor], nil];
    [customView.layer insertSublayer:gradient atIndex:0];
    
    CALayer *layerD = [CALayer layer];
    layerD.frame = CGRectMake(0.0, 23.0, tableView.bounds.size.width, 1.0);
    layerD.backgroundColor = [[UIColor colorWithRed: 144.0f/255.0f green: 159.0f/255.0f blue: 177.0f/255.0f alpha: 1.00] CGColor];
    [customView.layer insertSublayer:layerD atIndex:0];
    
    UILabel *topLabel = [[[UILabel alloc] init] autorelease];
            topLabel.frame=CGRectMake(10, 0.0, tableView.bounds.size.width-10*2, 24);
    //
    // Configure the properties for the text that are the same on every row
    //
    topLabel.backgroundColor = [UIColor clearColor];
    topLabel.textColor = [UIColor whiteColor];
    topLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
    topLabel.font = [UIFont boldSystemFontOfSize:20];
    topLabel.lineBreakMode=NSLineBreakByTruncatingMiddle;
    topLabel.opaque=TRUE;
    topLabel.text=lbl;
            
    [customView addSubview: topLabel];
            
    /*UIButton *buttonLabel                  = [UIButton buttonWithType: UIButtonTypeCustom];
    buttonLabel.titleLabel.font            = [UIFont boldSystemFontOfSize: 16];
    buttonLabel.titleLabel.shadowOffset    = CGSizeMake (0.0, 1.0);
    buttonLabel.titleLabel.lineBreakMode   = (NSLineBreakMode)UILineBreakModeTailTruncation;
    //	buttonLabel.titleLabel.shadowOffset    = CGSizeMake (1.0, 0.0);
    buttonLabel.frame=CGRectMake(32, 0.0, tableView.bounds.size.width-32*2, 24);
        
        [buttonLabel setTitle:lbl forState:UIControlStateNormal];
        [customView addSubview: buttonLabel];*/
        
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
    if (section==1) return 1;
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
- (void)slideTableViewCell:(SESlideTableViewCell*)cell didTriggerLeftButton:(NSInteger)buttonIndex {
    
    if ([cell.reuseIdentifier compare:@"CellH"]==NSOrderedSame) {
        if (cutpaste_filesrcpath) {
        //Paste file or dir
            //NSLog(@"Pasting: %@",cutpaste_filesrcpath);
        
            NSString *sourcePath=[NSHomeDirectory() stringByAppendingPathComponent:cutpaste_filesrcpath];
            NSString *destPath=[[NSHomeDirectory() stringByAppendingPathComponent:currentPath] stringByAppendingPathComponent:[cutpaste_filesrcpath lastPathComponent]];
            NSError *err;
        
            //NSLog(@"Pasting: %@ to %@",sourcePath,destPath);
            
            
            if ([mFileMngr moveItemAtPath:sourcePath toPath:destPath error:&err]!=YES) {
                NSLog(@"Issue %d while moving: %@",err.code,cutpaste_filesrcpath);
                UIAlertView *moveAlert = [[[UIAlertView alloc] initWithTitle:@"Warning" message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while moving: %@.\n%@",@""),err.code,cutpaste_filesrcpath] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                [moveAlert show];
            } else {
                [cutpaste_filesrcpath release];
                cutpaste_filesrcpath=nil;
                [self listLocalFiles];
            }
    } else {
            //Alert msg => nothing to Paste
            UIAlertView *pasteAlert = [[[UIAlertView alloc] initWithTitle:@"Warning" message:NSLocalizedString(@"Nothing to paste",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
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
                    [cutpaste_filesrcpath release];
                    cutpaste_filesrcpath=nil;
                }
                        
                        alertRename=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Enter new name",@"") message:nil delegate:self cancelButtonTitle:NSLocalizedString(@"Cancel",@"") otherButtonTitles:NSLocalizedString(@"Ok",@""),nil] autorelease];
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
        
        alertRename=[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Enter folder name",@"") message:nil delegate:self cancelButtonTitle:NSLocalizedString(@"Cancel",@"") otherButtonTitles:NSLocalizedString(@"Ok",@""),nil] autorelease];
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
            NSString *fullpath=[NSHomeDirectory() stringByAppendingPathComponent:cur_local_entries[section][indexPath.row].fullpath];
            NSError *err;
        
        if ([cutpaste_filesrcpath compare:cur_local_entries[section][indexPath.row].fullpath]==NSOrderedSame) {
            //deleting file in cut/paste buffer -> cancel buffer
            [cutpaste_filesrcpath release];
            cutpaste_filesrcpath=nil;
        }
        
            if ([mFileMngr removeItemAtPath:fullpath error:&err]!=YES) {
                NSLog(@"Issue %d while removing: %@",err.code,fullpath);
                UIAlertView *removeAlert = [[[UIAlertView alloc] initWithTitle:@"Warning" message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while trying to delete entry.\n%@",@""),err.code,err.localizedDescription] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
                [removeAlert show];
            } else {
                if (cur_local_entries[section][indexPath.row].type==0) { //Dir
                    DBHelper::deleteStatsDirDB(fullpath);
                }
                if (cur_local_entries[section][indexPath.row].type&3) { //File
                    DBHelper::deleteStatsFileDB(fullpath);
                }
                
                [self listLocalFiles];
                [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
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
    
    if (indexPath.section==1) cell = (SESlideTableViewCell *)[tabView dequeueReusableCellWithIdentifier:CellIdentifierHeader];
    else cell = (SESlideTableViewCell *)[tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    
    
    if (cell == nil) {
        if (is_ios7) {
            if (indexPath.section>1) {
                cell = [[[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:    CellIdentifier] autorelease];
                cell.delegate = self;
                
            [cell addLeftButtonWithText:NSLocalizedString(@"Rename",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor blueColor]];
            [cell addLeftButtonWithText:NSLocalizedString(@"Cut",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor purpleColor]];
            [cell addRightButtonWithText:NSLocalizedString(@"Delete",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor redColor]];
            } else {
                cell = [[[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:    CellIdentifierHeader] autorelease];
                cell.delegate = self;
                
                [cell addLeftButtonWithText:NSLocalizedString(@"Paste",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor purpleColor]];
                [cell addRightButtonWithText:NSLocalizedString(@"New folder",@"") textColor:[UIColor whiteColor] backgroundColor:[UIColor blueColor]];
                
                
            }
        } else {
            cell = (SESlideTableViewCell*)[[[SESlideTableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:CellIdentifier] autorelease];
        }
        cell.frame=CGRectMake(0,0,tabView.frame.size.width,40);
        [cell setBackgroundColor:[UIColor clearColor]];
        
        UIImage *image = [UIImage imageNamed:@"tabview_gradient40.png"];
        UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
        imageView.contentMode = UIViewContentModeScaleToFill;
        cell.backgroundView = imageView;
        [imageView release];
        
        //
        // Create the label for the top row of text
        //
        topLabel = [[[UILabel alloc] init] autorelease];
        [cell.contentView addSubview:topLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        topLabel.tag = TOP_LABEL_TAG;
        topLabel.backgroundColor = [UIColor clearColor];
        topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
        topLabel.font = [UIFont boldSystemFontOfSize:18];
        topLabel.lineBreakMode=NSLineBreakByTruncatingMiddle;
        topLabel.opaque=TRUE;
        
        //
        // Create the label for the top row of text
        //
        bottomLabel = [[[UILabel alloc] init] autorelease];
        [cell.contentView addSubview:bottomLabel];
        //
        // Configure the properties for the text that are the same on every row
        //
        bottomLabel.tag = BOTTOM_LABEL_TAG;
        bottomLabel.backgroundColor = [UIColor clearColor];
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
        bottomLabel.font = [UIFont systemFontOfSize:12];
        //bottomLabel.font = [UIFont fontWithName:@"courier" size:12];
        bottomLabel.lineBreakMode=NSLineBreakByTruncatingMiddle;
        bottomLabel.opaque=TRUE;
        
        bottomImageView = [[[UIImageView alloc] initWithImage:nil]  autorelease];
        bottomImageView.frame = CGRectMake(1.0*cell.indentationWidth,
                                           26,
                                           50,9);
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
    }
    actionView.hidden=TRUE;
    secActionView.hidden=TRUE;
    
    [cell layoutIfNeeded];
    
    topLabel.textColor = [UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:1.0];
    topLabel.highlightedTextColor = [UIColor colorWithRed:0.9 green:0.9 blue:0.9 alpha:1.0];
    bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
    bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
    
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
    if (indexPath.section==1){
        cellValue=(mShowSubdir?NSLocalizedString(@"DisplayDir_MainKey",""):NSLocalizedString(@"DisplayAll_MainKey",""));
        bottomLabel.text=[NSString stringWithFormat:@"%@ %d entries",(mShowSubdir?NSLocalizedString(@"DisplayDir_SubKey",""):NSLocalizedString(@"DisplayAll_SubKey","")),(search_local?search_local_nb_entries:local_nb_entries)];
        
        bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                       22,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                       18);
        
        topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
        
        topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                   0,
                                   tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-4-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                   22);
        
        [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateNormal];
        [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateHighlighted];
        [secActionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
        
        [actionView setImage:[UIImage imageNamed:@"play_all.png"] forState:UIControlStateNormal];
        [actionView setImage:[UIImage imageNamed:@"play_all.png"] forState:UIControlStateHighlighted];
        [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
        
        int icon_posx=tabView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE;
        icon_posx-=32;
        actionView.frame = CGRectMake(icon_posx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
        actionView.enabled=YES;
        actionView.hidden=NO;
        secActionView.frame = CGRectMake(icon_posx-PRI_SEC_ACTIONS_IMAGE_SIZE-4,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
        secActionView.enabled=YES;
        secActionView.hidden=NO;
        
    } else {
        int section=indexPath.section-2;
        cellValue=cur_local_entries[section][indexPath.row].label;
        
        
        if (cur_local_entries[section][indexPath.row].type==0) { //directory
            topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                       0,
                                       tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-32,
                                       40);
            
        } else  { //file
            int actionicon_offsetx=0;
            //archive file ?
            if ((cur_local_entries[section][indexPath.row].type==2)||(cur_local_entries[section][indexPath.row].type==3)) {
                actionicon_offsetx=PRI_SEC_ACTIONS_IMAGE_SIZE;
                //                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                
                secActionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                
                [secActionView setImage:[UIImage imageNamed:@"arc_details.png"] forState:UIControlStateNormal];
                [secActionView setImage:[UIImage imageNamed:@"arc_details.png"] forState:UIControlStateHighlighted];
                [secActionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                [secActionView addTarget: self action: @selector(accessoryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                
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
            } else {
                [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateNormal];
                [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateHighlighted];
                [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            }
            actionView.enabled=YES;
            actionView.hidden=NO;
            
            
            if (cur_local_entries[section][indexPath.row].rating==-1) {
                DBHelper::getFileStatsDBmod(
                                            cur_local_entries[section][indexPath.row].label,
                                            cur_local_entries[section][indexPath.row].fullpath,
                                            &cur_local_entries[section][indexPath.row].playcount,
                                            &cur_local_entries[section][indexPath.row].rating,
                                            &cur_local_entries[section][indexPath.row].song_length,
                                            &cur_local_entries[section][indexPath.row].channels_nb,
                                            &cur_local_entries[section][indexPath.row].songs);
            }
            if (cur_local_entries[section][indexPath.row].rating>=0) bottomImageView.image=[UIImage imageNamed:ratingImg[cur_local_entries[section][indexPath.row].rating]];
            
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
            
            
            /*if (!cur_local_entries[section][indexPath.row].playcount)
             bottomStr = [NSString stringWithFormat:@"%@%@",bottomStr,played0time];
             else if (cur_local_entries[section][indexPath.row].playcount==1)
             bottomStr = [NSString stringWithFormat:@"%@%@",bottomStr,played1time];
             else bottomStr = [NSString stringWithFormat:@"%@%@",bottomStr,[NSString stringWithFormat:playedXtimes,cur_local_entries[section][indexPath.row].playcount]];*/
            
            bottomLabel.text=bottomStr;
            
            bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
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
        NSString *fullpath=[NSHomeDirectory() stringByAppendingPathComponent:cur_local_entries[section][indexPath.row].fullpath];
        NSError *err;
        
        if ([mFileMngr removeItemAtPath:fullpath error:&err]!=YES) {
            NSLog(@"Issue %d while removing: %@",err.code,fullpath);
            UIAlertView *removeAlert = [[[UIAlertView alloc] initWithTitle:@"Warning" message:[NSString stringWithFormat:NSLocalizedString(@"Issue %d while trying to delete entry.\n%@",@""),err.code,err.localizedDescription] delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
            [removeAlert show];
        } else {
            if (cur_local_entries[section][indexPath.row].type==0) { //Dir
                DBHelper::deleteStatsDirDB(fullpath);
            }
            if (cur_local_entries[section][indexPath.row].type&3) { //File
                DBHelper::deleteStatsFileDB(fullpath);
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
    if (is_ios7) return NO;
    
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    int section=indexPath.section-2;
    if (section>=0) {
        NSString *fullpath=[NSHomeDirectory() stringByAppendingPathComponent:cur_local_entries[section][indexPath.row].fullpath];
        BOOL res;
        NSFileManager *myMngr=[[NSFileManager alloc] init];
        res=[myMngr isDeletableFileAtPath:fullpath];
        [myMngr release];
        return res;
    }
    return NO;
}

#pragma mark UISearchBarDelegate
- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar {
    // only show the status bar’s cancel button while in edit mode
    sBar.showsCancelButton = YES;
    sBar.autocorrectionType = UITextAutocorrectionTypeNo;
    mSearch=1;
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
    if (mSearchText) [mSearchText release];
    
    mSearchText=[[NSString alloc] initWithString:searchText];
    shouldFillKeys=1;
    [self fillKeys];
    [tableView reloadData];
}
- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
    if (mSearchText) [mSearchText release];
    mSearchText=nil;
    sBar.text=nil;
    mSearch=0;
    sBar.showsCancelButton = NO;
    [searchBar resignFirstResponder];
    
    [tableView reloadData];
}
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar {
    [searchBar resignFirstResponder];
}

-(IBAction)goPlayer {
    if (detailViewController.mPlaylist_size) {
        if (detailViewController) {
            @try {
                [self.navigationController pushViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];
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
                    [self.navigationController popToViewController:detailViewController animated:(detailViewController.mSlowDevice?NO:YES)];
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
        UIAlertView *nofileplaying=[[[UIAlertView alloc] initWithTitle:@"Warning"
                                                               message:NSLocalizedString(@"Nothing currently playing. Please select a file.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
        [nofileplaying show];
    }
}

#pragma mark -
#pragma mark Table view delegate
- (void) primaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
    
    int section=indexPath.section-2;
    
    if (indexPath.section==1) {
        // launch Play of current list
        int pos=0;
        
        t_playlist pl;
        pl.nb_entries=0;
        for (int i=0;i<27;i++) {
            for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++) {
                if (cur_local_entries[i][j].type&3) {
                    
                    pl.entries[pl.nb_entries].label=cur_local_entries[i][j].label;
                    pl.entries[pl.nb_entries].fullpath=cur_local_entries[i][j].fullpath;
                    pl.entries[pl.nb_entries].ratings=cur_local_entries[i][j].rating;
                    pl.entries[pl.nb_entries].playcounts=cur_local_entries[i][j].playcount;
                    pl.nb_entries++;
                    
                    if (i<section) pos++;
                    else if ((i==(section))&&(j<indexPath.row)) pos++;
                }
            }
        }
        if (pl.nb_entries) {
            [detailViewController play_listmodules:&pl start_index:-1];
            if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
            else [tableView reloadData];
        }
    } else {
        if (cur_local_entries[section][indexPath.row].type&3) {//File selected
            // launch Play
            t_playlist pl;
            pl.nb_entries=1;
            pl.entries[0].label=cur_local_entries[section][indexPath.row].label;
            pl.entries[0].fullpath=cur_local_entries[section][indexPath.row].fullpath;
            pl.entries[0].ratings=cur_local_entries[section][indexPath.row].rating;
            pl.entries[0].playcounts=cur_local_entries[section][indexPath.row].playcount;
            [detailViewController play_listmodules:&pl start_index:0];
            
            cur_local_entries[section][indexPath.row].rating=-1;
            [detailViewController play_listmodules:&pl start_index:0];
            if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
            else [tableView reloadData];
            
        }
    }
    
    [self hideWaiting];
    
    
}

- (void) secondaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
    
    
    //local  browser & favorites
    int section=indexPath.section-2;
    
    if (indexPath.section==1) {
        // launch Play of current dir
        int pos=0;
        int total_entries=0;
        NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
        NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
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
        
        signed char *tmp_ratings;
        short int *tmp_playcounts;
        tmp_ratings=(signed char*)malloc(total_entries*sizeof(signed char));
        tmp_playcounts=(short int*)malloc(total_entries*sizeof(short int));
        total_entries=0;
        for (int i=0;i<27;i++)
            for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++)
                if (cur_local_entries[i][j].type&3) {
                    tmp_ratings[total_entries]=cur_local_entries[i][j].rating;
                    tmp_playcounts[total_entries++]=cur_local_entries[i][j].playcount;
                }
        
        if (total_entries) {
            if ([detailViewController add_to_playlist:array_path fileNames:array_label forcenoplay:1]) {
                if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                else [tableView reloadData];
            }
        }
        
        free(tmp_ratings);
        free(tmp_playcounts);
        
    } else {
        if (cur_local_entries[section][indexPath.row].type&3) {//File selected
            cur_local_entries[section][indexPath.row].rating=-1;
            if ([detailViewController add_to_playlist:cur_local_entries[section][indexPath.row].fullpath fileName:cur_local_entries[section][indexPath.row].label forcenoplay:1]) {
                if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                else [tableView reloadData];
            }
        }
    }
    [self hideWaiting];
}

- (void) accessoryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
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
    
    int section=indexPath.section-2;
    
    
    if (indexPath.section==1) {
        int donothing=0;
        if (mSearch) {
            if (mSearchText==nil) donothing=1;
        }
        if (!donothing) {
            mShowSubdir^=1;
            shouldFillKeys=1;
            
            //[self showWaiting];
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            [self shortWait];
            
            
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
            
            [self hideWaiting];
        }
    } else {
        cellValue=cur_local_entries[section][indexPath.row].label;
        
        if (cur_local_entries[section][indexPath.row].type==0) { //Directory selected : change current directory
            
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            [self shortWait];
            
            
            NSString *newPath=[NSString stringWithFormat:@"%@/%@",currentPath,cellValue];
            [newPath retain];
            if (childController == nil) childController = [[RootViewControllerLocalBrowser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            else {// Don't cache childviews
            }
            //set new title
            childController.title = cellValue;
            // Set new depth & new directory
            ((RootViewControllerLocalBrowser*)childController)->currentPath = newPath;
            ((RootViewControllerLocalBrowser*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerLocalBrowser*)childController)->detailViewController=detailViewController;
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
            
            
            [self hideWaiting];
            //				[childController autorelease];
        } else if (((cur_local_entries[section][indexPath.row].type==2)||(cur_local_entries[section][indexPath.row].type==3))&&(mAccessoryButton)) { //Archive selected or multisongs: display files inside
            
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            [self shortWait];
            
            
            NSString *newPath;
            //                    NSLog(@"currentPath:%@\ncellValue:%@\nfullpath:%@",currentPath,cellValue,cur_local_entries[section][indexPath.row].fullpath);
            if (mShowSubdir) newPath=[NSString stringWithString:cur_local_entries[section][indexPath.row].fullpath];
            else newPath=[NSString stringWithFormat:@"%@/%@",currentPath,cellValue];
            [newPath retain];
            if (childController == nil) childController = [[RootViewControllerLocalBrowser alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            else {// Don't cache childviews
            }
            //set new title
            childController.title = cellValue;
            // Set new depth & new directory
            ((RootViewControllerLocalBrowser*)childController)->currentPath = newPath;
            ((RootViewControllerLocalBrowser*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerLocalBrowser*)childController)->detailViewController=detailViewController;
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
            
            
            [self hideWaiting];
            //				[childController autorelease];
        } else {  //File selected
            
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            [self shortWait];
            
            
            if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                // launch Play
                t_playlist pl;
                pl.nb_entries=1;
                pl.entries[0].label=cur_local_entries[section][indexPath.row].label;
                pl.entries[0].fullpath=cur_local_entries[section][indexPath.row].fullpath;
                pl.entries[0].ratings=cur_local_entries[section][indexPath.row].rating;
                pl.entries[0].playcounts=cur_local_entries[section][indexPath.row].playcount;
                [detailViewController play_listmodules:&pl start_index:0];
                
                cur_local_entries[section][indexPath.row].rating=-1;
                if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                else [tabView reloadData];
                
                
                
            } else {
                if ([detailViewController add_to_playlist:cur_local_entries[section][indexPath.row].fullpath fileName:cur_local_entries[section][indexPath.row].label forcenoplay:0]) {
                    if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                    else [tabView reloadData];
                }
            }
            
            [self hideWaiting];
            
            
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
    frame.origin.y=self.view.frame.size.height-64;
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
                dict=[fileManager attributesOfItemAtPath:[NSHomeDirectory() stringByAppendingPathComponent:cur_local_entries[csection][crow].fullpath] error:&err];
                if (dict) {
                    str=[str stringByAppendingFormat:@"\n%lluKo",[dict fileSize]/1024];
                }
                
                [fileManager release];
                
                
                if (self.popTipView == nil) {
                    self.popTipView = [[[CMPopTipView alloc] initWithMessage:str] autorelease];
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
- (void)viewDidUnload {
    // Relinquish ownership of anything that can be recreated in viewDidLoad or on demand.
    // For example: self.myOutlet = nil;;
}
- (void)dealloc {
    [waitingView removeFromSuperview];
    [waitingView release];
    
    [currentPath release];
    if (mSearchText) {
        [mSearchText release];
        mSearchText=nil;
    }
    if (keys) {
        [keys release];
        keys=nil;
    }
    if (list) {
        [list release];
        list=nil;
    }	
    
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {
            [local_entries_data[i].label release];
            [local_entries_data[i].fullpath release];
        }
        free(local_entries_data);
    }
    if (search_local_nb_entries) {
        free(search_local_entries_data);
    }
    
    
    if (indexTitles) {
        [indexTitles release];
        indexTitles=nil;        
    }
    if (indexTitlesSpace) {
        [indexTitlesSpace release];
        indexTitlesSpace=nil;        
    }
    
    if (mFileMngr) {
        [mFileMngr release];
        mFileMngr=nil;
    }
    
    [super dealloc];
}


@end
