//
//  RootViewController.m
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

//#define GET_NB_ENTRIES 1
#define NB_MODLAND_ENTRIES 319724
#define NB_HVSC_ENTRIES 43116
#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define ROW_HEIGHT 40

#define LIMITED_LIST_SIZE 1024

#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/xattr.h>

#include "gme.h"

#include "unzip.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;

static int didGoToWorldCharts=0;
static int didGoToUserGuide=0;
//static int shouldFillKeys;
static int local_flag;
static volatile int mUpdateToNewDB;
static volatile int mPopupAnimation=0;


#import "AppDelegate_Phone.h"

#import "RootViewControllerIphone.h"
#import "RootViewControllerLocalBrowser.h"
#import "RootViewControllerMODLAND.h"
#import "RootViewControllerHVSC.h"
#import "RootViewControllerPlaylist.h"


#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "WebBrowser.h"
#import "QuartzCore/CAAnimation.h"

volatile int mDatabaseCreationInProgress;
volatile int db_checked=0;

@implementation RootViewControllerIphone

@synthesize detailViewController;
@synthesize downloadViewController;
@synthesize webBrowser;
@synthesize tableView,sBar;
@synthesize list;
@synthesize keys;
@synthesize currentPath;
@synthesize childController;
@synthesize playerButton;
@synthesize mSearchText;
@synthesize mFileMngr;

#pragma mark -
#pragma mark View lifecycle

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
        [self addSkipBackupAttributeToItemAtPath:pathToOldDB];
	}
	
	[fileManager copyItemAtPath:defaultDBPath toPath:pathToDB error:&error];
    [self addSkipBackupAttributeToItemAtPath:pathToDB];
	
	
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


- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
    if (alertView==alertAlreadyAvail) {
		if (buttonIndex==1) {//force new download
			[self checkCreate:[FTPlocalPath stringByDeletingLastPathComponent]];
			mCurrentWinAskedDownload=1;
			[downloadViewController addFTPToDownloadList:FTPlocalPath ftpURL:FTPftpPath  ftpHost:MODLAND_FTPHOST filesize:FTPfilesize filename:FTPfilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
		} else {
			if (mClickedPrimAction==1) {
				NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
				NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
				[array_label addObject:FTPfilename];
				[array_path addObject:FTPlocalPath];
				[detailViewController play_listmodules:array_label start_index:0 path:array_path];
				//[self goPlayer];
			} else {
				if ([detailViewController add_to_playlist:FTPlocalPath fileName:FTPfilename forcenoplay:1]) {
					if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
					else [[super tableView] reloadData];
				}
			}
		}
		[FTPfilename autorelease];
		[FTPlocalPath autorelease];
		[FTPftpPath autorelease];
		[FTPfilePath autorelease];
	} else {
		if (mDatabaseCreationInProgress) {
			[self recreateDB];
			UIAlertView *alert2 = [[[UIAlertView alloc] initWithTitle:@"Info" message:NSLocalizedString(@"Database created.",@"") delegate:self cancelButtonTitle:@"Close" otherButtonTitles:nil] autorelease];
			[alert2 show];
		}		
	}
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
	NSString *samplesPkgPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"Samples2"];
	
	
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
    BOOL success;
	BOOL wrongversion=FALSE;
	int maj,min;
	mUpdateToNewDB=0;
    NSFileManager *fileManager = [[NSFileManager alloc] init];
    NSError *error;
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *writableDBPath = [documentsDirectory stringByAppendingPathComponent:DATABASENAME_USER];
    success = [fileManager fileExistsAtPath:writableDBPath];
    if (success&&!forceInit) {
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
	if (success&&(!wrongversion)) {//remove existing file
		success = [fileManager removeItemAtPath:writableDBPath error:&error];
	}
	
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
			}
			else  alert1 = [[[UIAlertView alloc] initWithTitle:@"Info" 
                                                       message:NSLocalizedString(@"No database found. Will create a new one. Please validate & wait...",@"") delegate:self cancelButtonTitle:@"Create DB" otherButtonTitles:nil] autorelease];
		}
		[alert1 show];
	}
    [fileManager release];
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
	NSString *machine = [[[NSString alloc] initWithCString:name] autorelease];
	
	// Done with this
	free(name);
	
	return machine;
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
	
	/* Init popup view*/
	/**/
	
	//self.tableView.pagingEnabled;
	self.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
	self.tableView.sectionHeaderHeight = 18;
	self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
	self.tableView.rowHeight = 40;
    //self.tableView.backgroundColor = [UIColor clearColor];
//	self.tableView.backgroundColor = [UIColor blackColor];
	
	shouldFillKeys=1;
	mSearch=0;
	
	search_db=0;  //reset to ensure search_db is not used by default
	search_local=0;
	search_dbHVSC=0;  //reset to ensure search_dbHVSC is not used by default
	
	db_nb_entries=0;
	search_db_nb_entries=0;
	dbHVSC_nb_entries=0;
	search_dbHVSC_nb_entries=0;
	local_nb_entries=0;
	search_local_nb_entries=0;
	
	search_dbHVSC_hasFiles=0;
	dbHVSC_hasFiles=0;
	search_db_hasFiles=0;
	db_hasFiles=0;
	
	mSearchText=nil;
	mCurrentWinAskedDownload=0;
	mClickedPrimAction=0;
	list=nil;
	keys=nil;
	
	if (browse_depth==MENU_COLLECTIONS_ROOTLEVEL) {
		mFiletypeID=mAuthorID=mAlbumID=-1;
#ifdef GET_NB_ENTRIES
		mNbMODLANDFileEntries=DBHelper::getNbMODLANDFilesEntries();
		mNbHVSCFileEntries=DBHelper::getNbHVSCFilesEntries();
#else
		mNbMODLANDFileEntries=NB_MODLAND_ENTRIES;
		mNbHVSCFileEntries=NB_HVSC_ENTRIES;
#endif
	}
	
	if (browse_mode==BROWSE_RATED_MODE) {//favorites
		self.navigationItem.rightBarButtonItem = playerButton; //self.editButtonItem;
	}
	if (browse_mode==BROWSE_MOSTPLAYED_MODE) {//most played
		self.navigationItem.rightBarButtonItem = playerButton; //self.editButtonItem;
	}
	
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
	
	indexTitlesDownload = [[NSMutableArray alloc] init];
	[indexTitlesDownload addObject:@"{search}"];
	[indexTitlesDownload addObject:@" "];
	[indexTitlesDownload addObject:@"#"];
	[indexTitlesDownload addObject:@"A"];
	[indexTitlesDownload addObject:@"B"];
	[indexTitlesDownload addObject:@"C"];
	[indexTitlesDownload addObject:@"D"];
	[indexTitlesDownload addObject:@"E"];
	[indexTitlesDownload addObject:@"F"];
	[indexTitlesDownload addObject:@"G"];
	[indexTitlesDownload addObject:@"H"];	
	[indexTitlesDownload addObject:@"I"];
	[indexTitlesDownload addObject:@"J"];
	[indexTitlesDownload addObject:@"K"];
	[indexTitlesDownload addObject:@"L"];
	[indexTitlesDownload addObject:@"M"];
	[indexTitlesDownload addObject:@"N"];
	[indexTitlesDownload addObject:@"O"];
	[indexTitlesDownload addObject:@"P"];
	[indexTitlesDownload addObject:@"Q"];
	[indexTitlesDownload addObject:@"R"];
	[indexTitlesDownload addObject:@"S"];
	[indexTitlesDownload addObject:@"T"];
	[indexTitlesDownload addObject:@"U"];
	[indexTitlesDownload addObject:@"V"];
	[indexTitlesDownload addObject:@"W"];
	[indexTitlesDownload addObject:@"X"];
	[indexTitlesDownload addObject:@"Y"];
	[indexTitlesDownload addObject:@"Z"];
	
	UIWindow *window=[[UIApplication sharedApplication] keyWindow];		
	
	waitingView = [[UIView alloc] initWithFrame:CGRectMake(window.bounds.size.width/2-40,window.bounds.size.height/2-40,80,80)];
	waitingView.backgroundColor=[UIColor blackColor];//[UIColor colorWithRed:0 green:0 blue:0 alpha:0.8f];
	waitingView.opaque=TRUE;
	waitingView.hidden=TRUE;
	waitingView.layer.cornerRadius=20;
	
	UIActivityIndicatorView *indView=[[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(20,20,37,37)];
	indView.activityIndicatorViewStyle=UIActivityIndicatorViewStyleWhiteLarge;
	[waitingView addSubview:indView];
	[indView startAnimating];		
	[indView autorelease];
	
	[window addSubview:waitingView];
	
	[super viewDidLoad];
	
	end_time=clock();	
#ifdef LOAD_PROFILE
	NSLog(@"rootview : %d",end_time-start_time);
#endif
}

-(void) fillKeys {
	
	if (browse_depth==MENU_ROOTLEVEL) {
		keys = [[NSMutableArray alloc] init];
		list = [[NSMutableArray alloc] init];
		NSMutableArray *modeLabel_entries = [[[NSMutableArray alloc] init] autorelease];
		//Order has to reflet the constant defined in ModizerConstant
		[modeLabel_entries addObject:NSLocalizedString(@"Browser_LocalBrowsing_MainKey", @"")];  //NSLocalizedString(@"WelcomeKey", @"")
		[modeLabel_entries addObject:NSLocalizedString(@"Browser_MODLAND_MainKey", @"")];
		[modeLabel_entries addObject:NSLocalizedString(@"Browser_HVSC_MainKey", @"")];
		[modeLabel_entries addObject:NSLocalizedString(@"Browser_Playlists_MainKey", @"")];
		[modeLabel_entries addObject:NSLocalizedString(@"Browser_Favorites_MainKey", @"")];
		[modeLabel_entries addObject:NSLocalizedString(@"Browser_Most_played_MainKey", @"")];
		[modeLabel_entries addObject:NSLocalizedString(@"Browser_Web_MainKey", @"")];
		[modeLabel_entries addObject:NSLocalizedString(@"Browser_WorldCharts_MainKey", @"")];
        [modeLabel_entries addObject:NSLocalizedString(@"Browser_UserGuide_MainKey", @"")];
		
		NSDictionary *mode_entriesDict = [NSDictionary dictionaryWithObject:modeLabel_entries forKey:@"entries"];
		[keys addObject:mode_entriesDict];
	} else if (browse_mode==BROWSE_RATED_MODE) {//Top Rating
		[self loadFavoritesList];
	} else if (browse_mode==BROWSE_MOSTPLAYED_MODE) {//Top Most played
		[self loadMostPlayedList];
	} else if (browse_mode==BROWSE_WEB_MODE) {//Web browser
	} else if (browse_mode==BROWSE_WORLDCHARTS_MODE) {//Web/World Charts
	}  else if (browse_mode==BROWSE_USERGUIDE_MODE) {//Web/User Guide
	}
}

-(NSString*) getCompletePath:(int)id_mod {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	NSString *fullpath=nil;
	
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;		
		
		sprintf(sqlStatement,"select fullpath from mod_file where id=%d",id_mod);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				fullpath=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);		
	}
	
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return fullpath;
}
-(NSString*) getCompleteLocalPath:(int)id_mod {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	NSString *localpath=nil;
	
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;		
		
		sprintf(sqlStatement,"select localpath from mod_file where id=%d",id_mod);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				localpath=[NSString  stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);		
	}
	
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return localpath;
}
-(int) getFileSize:(NSString*)fileName {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int iFileSize;
	
	pthread_mutex_lock(&db_mutex);
	
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;		
		
		sprintf(sqlStatement,"select filesize from mod_file where filename=\"%s\"",[fileName UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				iFileSize=(int)sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return iFileSize;
}
-(NSString *) getModFilename:(int)idmod {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	NSString *fileName;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;		
		
		sprintf(sqlStatement,"select filename from mod_file where id=%d",idmod);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				fileName=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return fileName;
}

-(void) loadFavoritesList{
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int local_entries_index;	
    
    NSRange r;
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_local=1;
        if (search_local_nb_entries) {
            search_local_nb_entries=0;
            free(search_local_entries_data);
        }
        search_local_entries_data=(t_local_browse_entry*)malloc(local_nb_entries*sizeof(t_local_browse_entry));
        for (int i=0;i<5;i++) {
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
    
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {		
            [local_entries_data[i].label release];
            [local_entries_data[i].fullpath release];
        }
        free(local_entries_data);local_entries_data=NULL;
        local_nb_entries=0;
    }
    for (int i=0;i<27;i++) local_entries_count[i]=0;
    
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        if (mSearch) sprintf(sqlStatement,"SELECT name,rating FROM user_stats WHERE rating>0 AND name like \"%%%s%%\" ORDER BY name,rating DESC",[mSearchText UTF8String]);
        else sprintf(sqlStatement,"SELECT name,rating FROM user_stats WHERE rating>0 ORDER BY rating DESC,name");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {	
                int index=5-sqlite3_column_int(stmt, 1);
                local_entries_count[index]++;
                local_nb_entries++;
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)malloc(local_nb_entries*sizeof(t_local_browse_entry));
            local_entries_index=0;
            for (int i=0;i<5;i++) 
                if (local_entries_count[i]) {
                    local_entries[i]=&(local_entries_data[local_entries_index]);
                    local_entries_index+=local_entries_count[i];
                    local_entries_count[i]=0;
                }
            
            if (mSearch) sprintf(sqlStatement,"SELECT name,fullpath,rating,play_count,length,channels,songs FROM user_stats WHERE rating>0 AND name like \"%%%s%%\" ORDER BY rating DESC,name",[mSearchText UTF8String]);
            else sprintf(sqlStatement,"SELECT name,fullpath,rating,play_count,length,channels,songs FROM user_stats WHERE rating>0 ORDER BY rating DESC,name");
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {	
                    int index=5-sqlite3_column_int(stmt, 2);
                    local_entries[index][local_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
                    local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
                    local_entries[index][local_entries_count[index]].rating=(signed char)sqlite3_column_int(stmt,2);
                    local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt,3);
                    local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt,4);
                    local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt,5);
                    local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt,6);					
                    
                    local_entries[index][local_entries_count[index]].type=1;
                    local_entries_count[index]++;				
                }
                sqlite3_finalize(stmt);
            } else NSLog(@"ErrSQL : %d",err);
        }
        
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
}
-(void) loadMostPlayedList{
    NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
    sqlite3 *db;
    int local_entries_index;
    NSRange r;
    
    // in case of search, do not ask DB again => duplicate already found entries & filter them
    if (mSearch) {
        search_local=1;
        if (search_local_nb_entries) {
            search_local_nb_entries=0;
            free(search_local_entries_data);
        }
        search_local_entries_data=(t_local_browse_entry*)malloc(local_nb_entries*sizeof(t_local_browse_entry));
        for (int i=0;i<1;i++) {
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
    if (local_nb_entries) {
        for (int i=0;i<local_nb_entries;i++) {		
            [local_entries_data[i].label release];
            [local_entries_data[i].fullpath release];
        }
        free(local_entries_data);local_entries_data=NULL;
        local_nb_entries=0;
    }
    for (int i=0;i<27;i++) local_entries_count[i]=0;
    
    pthread_mutex_lock(&db_mutex);
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        if (mSearch) sprintf(sqlStatement,"SELECT name FROM user_stats WHERE play_count>0 AND name like \"%%%s%%\" ORDER BY play_count DESC,name",[mSearchText UTF8String]);
        else sprintf(sqlStatement,"SELECT name FROM user_stats WHERE play_count>0 ORDER BY play_count DESC,name");
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {	
                int index=0;
                //if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                //if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                local_entries_count[index]++;
                local_nb_entries++;					
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
        if (local_nb_entries) {
            //2nd initialize array to receive entries
            local_entries_data=(t_local_browse_entry *)malloc(local_nb_entries*sizeof(t_local_browse_entry));
            local_entries_index=0;
            for (int i=0;i<1;i++) 
                if (local_entries_count[i]) {
                    local_entries[i]=&(local_entries_data[local_entries_index]);
                    local_entries_index+=local_entries_count[i];
                    local_entries_count[i]=0;
                }
            
            if (mSearch) sprintf(sqlStatement,"SELECT name,fullpath,rating,play_count,length,channels,songs FROM user_stats WHERE play_count>0 AND name like \"%%%s%%\" ORDER BY play_count DESC,name",[mSearchText UTF8String]);
            else sprintf(sqlStatement,"SELECT name,fullpath,rating,play_count,length,channels,songs FROM user_stats WHERE play_count>0 ORDER BY play_count DESC,name");
            err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            if (err==SQLITE_OK){
                while (sqlite3_step(stmt) == SQLITE_ROW) {	
                    int index=0;
                    //if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
                    //if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
                    local_entries[index][local_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
                    local_entries[index][local_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
                    local_entries[index][local_entries_count[index]].rating=(signed char)sqlite3_column_int(stmt,2);
                    local_entries[index][local_entries_count[index]].playcount=(short int)sqlite3_column_int(stmt,3);
                    local_entries[index][local_entries_count[index]].song_length=(int)sqlite3_column_int(stmt,4);
                    local_entries[index][local_entries_count[index]].channels_nb=(char)sqlite3_column_int(stmt,5);
                    local_entries[index][local_entries_count[index]].songs=(int)sqlite3_column_int(stmt,6);					
                    
                    
                    local_entries[index][local_entries_count[index]].type=1;
                    local_entries_count[index]++;				
                }
                sqlite3_finalize(stmt);
            } else NSLog(@"ErrSQL : %d",err);
        }
    };
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
}

-(void) viewWillAppear:(BOOL)animated {
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
        if (db_nb_entries) {
            for (int i=0;i<db_nb_entries;i++) {
                db_entries_data[i].rating=-1;
            }            
        }
        if (search_db_nb_entries) {
            for (int i=0;i<search_db_nb_entries;i++) {
                search_db_entries_data[i].rating=-1;
            }            
        }
        if (dbHVSC_nb_entries) {
            for (int i=0;i<dbHVSC_nb_entries;i++) {
                dbHVSC_entries_data[i].rating=-1;
            }            
        }
        if (search_dbHVSC_nb_entries) {
            for (int i=0;i<search_dbHVSC_nb_entries;i++) {
                search_dbHVSC_entries_data[i].rating=-1;
            }            
        }
    /////////////
    
    if (detailViewController.mShouldHaveFocus) {
        detailViewController.mShouldHaveFocus=0;
        [self.navigationController pushViewController:detailViewController animated:(mSlowDevice?NO:YES)];
    } else {				
        if (shouldFillKeys&&(browse_depth>0)&&
            ((browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE))
            ) {
            
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            
            [self fillKeys];
            [[super tableView] reloadData];
            [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
        } else {
            [self fillKeys];
            [[super tableView] reloadData];
        }
    }
    
    
    
    [super viewWillAppear:animated];	
    
}
-(void) refreshMODLANDView {
    /*	if (mCurrentWinAskedDownload) {
     [self fillKeys];
     [[super tableView] reloadData];
     } else */
    if (childController) [(RootViewControllerIphone*)childController refreshMODLANDView];
}

- (void)checkCreate:(NSString *)filePath {
    NSString *completePath=[NSString stringWithFormat:@"%@/%@",NSHomeDirectory(),filePath];
    NSError *err;
    [mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:&err];	
}

- (void)viewDidAppear:(BOOL)animated {        
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];

    [super viewDidAppear:animated];		
}

-(void)hideAllWaitingPopup {
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
    if (childController) {
        [childController hideAllWaitingPopup];
    }
}

- (void)viewDidDisappear:(BOOL)animated {
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
    if (childController) {
        [childController viewDidDisappear:FALSE];
    }
    [super viewDidDisappear:animated];
}

/*- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
    [[super tableView] reloadData];
}*/

- (NSUInteger)supportedInterfaceOrientations {
    return UIInterfaceOrientationMaskAll;
//    return UIInterfaceOrientationMaskAllButUpsideDown;
}

- (BOOL)shouldAutorotate {
    return TRUE;
}

// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
/*- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    [[super tableView] reloadData];
    return YES;
}*/

#pragma mark -
#pragma mark Table view data source

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (browse_depth==0) return nil;
    if (mSearch) return nil;	
    if ((browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)||(browse_mode==BROWSE_WEB_MODE)||(browse_mode==BROWSE_WORLDCHARTS_MODE)||(browse_mode==BROWSE_USERGUIDE_MODE)) return nil;
    if (browse_depth>=2) return [indexTitles objectAtIndex:section];
    return nil;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    local_flag=0;
    
    if (browse_depth==0) return [keys count];
    return 28;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (browse_mode==BROWSE_RATED_MODE) {//favorites / rating
        if (section==0) return 0;
        if (section==1) return 1;        
        return (search_local?search_local_entries_count[section-2]:local_entries_count[section-2]);
    } else if (browse_mode==BROWSE_MOSTPLAYED_MODE) {//favorites / play count
        if (section==0) return 0;
        if (section==1) return 1;
        return (search_local?search_local_entries_count[section-2]:local_entries_count[section-2]);
    } else {
        NSDictionary *dictionary = [keys objectAtIndex:section];
        NSArray *array = [dictionary objectForKey:@"entries"];
        return [array count];
    }
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    if (browse_depth==0) return nil;
    if (mSearch) return nil;	
    if ((browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)||(browse_mode==BROWSE_WEB_MODE)||(browse_mode==BROWSE_WORLDCHARTS_MODE)||(browse_mode==BROWSE_USERGUIDE_MODE)) return nil;
    if (browse_depth>=2) return indexTitles;
    return nil;
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    if (mSearch) return -1;
    if ((browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)||(browse_mode==BROWSE_WEB_MODE)||(browse_mode==BROWSE_WORLDCHARTS_MODE)||(browse_mode==BROWSE_USERGUIDE_MODE)) return nil;
    return -1;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
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
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    NSString *playedXtimes=NSLocalizedString(@"Played %d times.",@"");
    NSString *played1time=NSLocalizedString(@"Played once.",@"");	
    NSString *played0time=NSLocalizedString(@"Never played.",@"");	
    NSString *nbFiles=NSLocalizedString(@"%d files.",@"");	
    NSString *nb1File=NSLocalizedString(@"1 file.",@"");	
    
    
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        
        cell.frame=CGRectMake(0,0,tableView.frame.size.width,40);
        [cell setBackgroundColor:[UIColor clearColor]];
        CAGradientLayer *gradient = [CAGradientLayer layer];
        gradient.frame = cell.bounds;
        gradient.colors = [NSArray arrayWithObjects:
                           (id)[[UIColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:255.0/255.0 green:255.0/255.0 blue:255.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:245.0/255.0 green:245.0/255.0 blue:245.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:240.0/255.0 green:240.0/255.0 blue:240.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:220.0/255.0 green:220.0/255.0 blue:220.0/255.0 alpha:1] CGColor],
                           (id)[[UIColor colorWithRed:220.0/255.0 green:220.0/255.0 blue:220.0/255.0 alpha:1] CGColor],
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
        selgrad.colors = [NSArray arrayWithObjects:
                          (id)[[UIColor colorWithRed:0.9f*220.0/255.0 green:0.99f*220.0/255.0 blue:0.9f*220.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:0.9f*220.0/255.0 green:0.99f*220.0/255.0 blue:0.9f*220.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:0.9f*240.0/255.0 green:0.99f*240.0/255.0 blue:0.9f*240.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:0.9f*245.0/255.0 green:0.99f*245.0/255.0 blue:0.9f*245.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:0.9f*255.0/255.0 green:0.99f*255.0/255.0 blue:0.9f*255.0/255.0 alpha:1] CGColor],
                          (id)[[UIColor colorWithRed:0.9f*255.0/255.0 green:0.99f*255.0/255.0 blue:0.9f*255.0/255.0 alpha:1] CGColor],
                          
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
        topLabel.textColor = [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:1.0];
        topLabel.highlightedTextColor = [UIColor colorWithRed:1.0 green:1.0 blue:0.9 alpha:1.0];
        topLabel.font = [UIFont boldSystemFontOfSize:18];
        topLabel.lineBreakMode=UILineBreakModeMiddleTruncation;
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
        bottomLabel.textColor = [UIColor colorWithRed:0.25 green:0.20 blue:0.20 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.75 green:0.8 blue:0.8 alpha:1.0];
        bottomLabel.font = [UIFont systemFontOfSize:12];
        //bottomLabel.font = [UIFont fontWithName:@"courier" size:12];
        bottomLabel.lineBreakMode=UILineBreakModeMiddleTruncation;
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
        topLabel = (UILabel *)[cell viewWithTag:TOP_LABEL_TAG];
        bottomLabel = (UILabel *)[cell viewWithTag:BOTTOM_LABEL_TAG];
        bottomImageView = (UIImageView *)[cell viewWithTag:BOTTOM_IMAGE_TAG];
        actionView = (UIButton *)[cell viewWithTag:ACT_IMAGE_TAG];
        secActionView = (UIButton *)[cell viewWithTag:SECACT_IMAGE_TAG];
    }
    actionView.hidden=TRUE;
    secActionView.hidden=TRUE;
    
    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                               0,
                               tableView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                               22);
    bottomLabel.frame = CGRectMake(1.0 * cell.indentationWidth,
                                   22,
                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32,
                                   18);
    bottomLabel.text=@""; //default value
    bottomImageView.image=nil;
    
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    // Set up the cell...
    if (browse_depth==0) {
        NSDictionary *dictionary = [keys objectAtIndex:indexPath.section];
        NSArray *array = [dictionary objectForKey:@"entries"];
        cellValue = [array objectAtIndex:indexPath.row];
        
        //topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
        if (indexPath.row==BROWSE_LOCAL_MODE) {
            //topLabel.textColor=[UIColor colorWithRed:0.8f green:0.3f blue:0.3f alpha:1.0f];
            bottomLabel.text=NSLocalizedString(@"Browser_LocalBrowsing_SubKey",@"");// @"Browse, play & delete local files.";
        } else if (indexPath.row==BROWSE_MODLAND_MODE) {
            //topLabel.textColor=[UIColor colorWithRed:0.7f green:0.2f blue:0.4f alpha:1.0f];
            bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"Browser_MODLAND_SubKey",@""),mNbMODLANDFileEntries];
        } else if (indexPath.row==BROWSE_HVSC_MODE) {
            //topLabel.textColor=[UIColor colorWithRed:0.5f green:0.2f blue:0.6f alpha:1.0f];			
            bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"Browser_HVSC_SubKey",@""),mNbHVSCFileEntries];
        } else if (indexPath.row==BROWSE_RATED_MODE) {
            //topLabel.textColor=[UIColor colorWithRed:0.5f green:0.2f blue:0.6f alpha:1.0f];
            bottomLabel.text=NSLocalizedString(@"Browser_Favorites_SubKey",@"");
        } else if (indexPath.row==BROWSE_MOSTPLAYED_MODE) {
            //topLabel.textColor=[UIColor colorWithRed:0.6f green:0.3f blue:0.4f alpha:1.0f];			
            bottomLabel.text=NSLocalizedString(@"Browser_Most_played_SubKey",@"");
        } else if (indexPath.row==BROWSE_WEB_MODE) {
            //topLabel.textColor=[UIColor colorWithRed:0.7f green:0.4f blue:0.3f alpha:1.0f];
            bottomLabel.text=NSLocalizedString(@"Browser_Web_SubKey",@"");
        } else if (indexPath.row==BROWSE_WORLDCHARTS_MODE) {
            //topLabel.textColor=[UIColor colorWithRed:0.7f green:0.4f blue:0.3f alpha:1.0f];
            bottomLabel.text=NSLocalizedString(@"Browser_WorldCharts_SubKey",@"");
        } else if (indexPath.row==BROWSE_USERGUIDE_MODE) {
            //topLabel.textColor=[UIColor colorWithRed:0.7f green:0.4f blue:0.3f alpha:1.0f];
            bottomLabel.text=NSLocalizedString(@"Browser_UserGuide_SubKey",@"");
        }
        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;//UITableViewCellAccessoryDisclosureIndicator;
    }  else if ((browse_mode==BROWSE_LOCAL_MODE)||(browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)) { //local browser & favorites 
        int switch_view_subdir=( (browse_depth>=SHOW_SUDIR_MIN_LEVEL));//&&(browse_mode==BROWSE_LOCAL_MODE)?1:0);
        if (switch_view_subdir&&(indexPath.section==1)){
            cellValue=(mShowSubdir?NSLocalizedString(@"DisplayDir_MainKey",""):NSLocalizedString(@"DisplayAll_MainKey",""));
            bottomLabel.text=[NSString stringWithFormat:@"%@ %d files",(mShowSubdir?NSLocalizedString(@"DisplayDir_SubKey",""):NSLocalizedString(@"DisplayAll_SubKey","")),(search_local?search_local_nb_entries:local_nb_entries)];
            
            bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                           22,
                                           tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                           18);
            
            topLabel.textColor=[UIColor colorWithRed:0.4f green:0.4f blue:0.9f alpha:1.0];			
            
            topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                       0,
                                       tableView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-4-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                       22);
            
            
            
            [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateNormal];
            [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateHighlighted];
            [secActionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            
            [actionView setImage:[UIImage imageNamed:@"play_all.png"] forState:UIControlStateNormal];
            [actionView setImage:[UIImage imageNamed:@"play_all.png"] forState:UIControlStateHighlighted];
            [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
            
            int icon_posx=tableView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE;
            if (browse_mode==BROWSE_LOCAL_MODE) icon_posx-=32;
            actionView.frame = CGRectMake(icon_posx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
            actionView.enabled=YES;
            actionView.hidden=NO;
            secActionView.frame = CGRectMake(icon_posx-PRI_SEC_ACTIONS_IMAGE_SIZE-4,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
            secActionView.enabled=YES;
            secActionView.hidden=NO;
            
        } else {
            int section=indexPath.section-1-switch_view_subdir;
            cellValue=cur_local_entries[section][indexPath.row].label;
            
            
            if (cur_local_entries[section][indexPath.row].type==0) { //directory
                topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
                cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;				
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tableView.bounds.size.width -1.0 * cell.indentationWidth- 32-32,
                                           ROW_HEIGHT);
                
            } else  { //file
                int actionicon_offsetx=0;
                //archive file ?
                if ((cur_local_entries[section][indexPath.row].type==2)||(cur_local_entries[section][indexPath.row].type==3)) {
                    actionicon_offsetx=PRI_SEC_ACTIONS_IMAGE_SIZE;
                    //                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;                    
                    
                    if (browse_mode==BROWSE_LOCAL_MODE) secActionView.frame = CGRectMake(tableView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                    else secActionView.frame = CGRectMake(tableView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                    
                    [secActionView setImage:[UIImage imageNamed:@"arc_details.png"] forState:UIControlStateNormal];
                    [secActionView setImage:[UIImage imageNamed:@"arc_details.png"] forState:UIControlStateHighlighted];
                    [secActionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                    [secActionView addTarget: self action: @selector(accessoryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                    
                    secActionView.enabled=YES;
                    secActionView.hidden=NO;
                    
                }
                
                
                topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
                
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tableView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,
                                           22);
                
                if (browse_mode==BROWSE_LOCAL_MODE) actionView.frame = CGRectMake(tableView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                else actionView.frame = CGRectMake(tableView.bounds.size.width-2-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                
                
                if (detailViewController.sc_DefaultAction.selectedSegmentIndex==0) {
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
                if (isMonoSong) {
                    if (cur_local_entries[section][indexPath.row].song_length>0)
                        bottomStr=[NSString stringWithFormat:@"%02d:%02d",cur_local_entries[section][indexPath.row].song_length/1000/60,(cur_local_entries[section][indexPath.row].song_length/1000)%60];
                    else bottomStr=@"--:--";
                } else bottomStr=@"--:--";
                
                if (isMonoSong) {
                    if (cur_local_entries[section][indexPath.row].channels_nb)
                        bottomStr=[NSString stringWithFormat:@"%@ / %02dch",bottomStr,cur_local_entries[section][indexPath.row].channels_nb];
                    else bottomStr=[NSString stringWithFormat:@"%@ / --ch",bottomStr];
                } else bottomStr=[NSString stringWithFormat:@"%@ / --ch",bottomStr];
                
                if (isMonoSong) {
                    if (cur_local_entries[section][indexPath.row].songs==1) bottomStr=[NSString stringWithFormat:@"%@ / 1 song",bottomStr];
                    else bottomStr=[NSString stringWithFormat:@"%@ / - song",bottomStr];
                } else {
                    if (cur_local_entries[section][indexPath.row].songs>0)
                        bottomStr=[NSString stringWithFormat:@"%@ / %d songs",bottomStr,cur_local_entries[section][indexPath.row].songs];
                    else
                        bottomStr=[NSString stringWithFormat:@"%@ / %d files",bottomStr,-cur_local_entries[section][indexPath.row].songs];
                }                								
                
                
                bottomStr=[NSString stringWithFormat:@"%@ / Pl:%d",bottomStr,cur_local_entries[section][indexPath.row].playcount];
                
                
                /*if (!cur_local_entries[section][indexPath.row].playcount) 
                 bottomStr = [NSString stringWithFormat:@"%@%@",bottomStr,played0time]; 
                 else if (cur_local_entries[section][indexPath.row].playcount==1) 
                 bottomStr = [NSString stringWithFormat:@"%@%@",bottomStr,played1time];
                 else bottomStr = [NSString stringWithFormat:@"%@%@",bottomStr,[NSString stringWithFormat:playedXtimes,cur_local_entries[section][indexPath.row].playcount]];*/
                
                bottomLabel.text=bottomStr;
                
                bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                                               22,
                                               tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60-actionicon_offsetx,
                                               18);
                
                if ((browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)) {
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                                                   22,
                                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60-actionicon_offsetx,
                                                   18);
                    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                               0,
                                               tableView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-actionicon_offsetx,
                                               22);
                    
                }
            }
        }
    }
    
    topLabel.text = cellValue;
    
    return cell;
}

// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        
        //delete entry
        if (browse_mode==BROWSE_RATED_MODE) {
            short int playcount;
            signed char rating;
            int section=indexPath.section-1-1;
            DBHelper::getFileStatsDBmod(cur_local_entries[section][indexPath.row].label,
                                        cur_local_entries[section][indexPath.row].fullpath,
                                        &playcount,&rating);
            rating=0;
            DBHelper::updateFileStatsDBmod(cur_local_entries[section][indexPath.row].label,
                                           cur_local_entries[section][indexPath.row].fullpath,
                                           playcount,rating);
            [self loadFavoritesList];
            [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
        }
        if (browse_mode==BROWSE_MOSTPLAYED_MODE) {
            short int playcount;
            signed char rating;
            int section=indexPath.section-1-1;
            DBHelper::getFileStatsDBmod(cur_local_entries[section][indexPath.row].label,
                                        cur_local_entries[section][indexPath.row].fullpath,
                                        &playcount,&rating);
            playcount=0;
            DBHelper::updateFileStatsDBmod(cur_local_entries[section][indexPath.row].label,
                                           cur_local_entries[section][indexPath.row].fullpath,
                                           playcount,rating);
            [self loadMostPlayedList];
            [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
        }
        
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}
- (NSIndexPath *)tableView:(UITableView *)tableView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath {    
    return proposedDestinationIndexPath;
}
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
}
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return NO;
}
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    if (browse_mode==BROWSE_RATED_MODE) return YES;
    if (browse_mode==BROWSE_MOSTPLAYED_MODE) return YES;
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
    //[[super tableView] reloadData];
    //mSearch=0;
    sBar.showsCancelButton = NO;
}
- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
    if (mSearchText) [mSearchText release];
    
    mSearchText=[[NSString alloc] initWithString:searchText];
    shouldFillKeys=1;
    [self fillKeys];
    [[super tableView] reloadData];
}
- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
    if (mSearchText) [mSearchText release];
    mSearchText=nil;
    sBar.text=nil;
    mSearch=0;
    sBar.showsCancelButton = NO;
    [searchBar resignFirstResponder];
    
    [[super tableView] reloadData];
}
- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar {
    [searchBar resignFirstResponder];
}

-(IBAction)goPlayer {
    //	self.navigationController.navigationBar.hidden = YES;
    //[self performSelectorInBackground:@selector(showWaiting) withObject:nil];    
    [self.navigationController pushViewController:detailViewController animated:(mSlowDevice?NO:YES)];
}

#pragma mark -
#pragma mark Table view delegate
- (void) primaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [[super tableView] indexPathForRowAtPoint:[[[sender superview] superview] center]];
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [[super tableView] selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];                
    
    
    if (browse_depth==0) {
        
    } else {
        if (browse_mode==BROWSE_LOCAL_MODE||(browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)) {//local  browser & favorites
            int switch_view_subdir=((browse_depth>=SHOW_SUDIR_MIN_LEVEL));//&&(browse_mode==BROWSE_LOCAL_MODE)?1:0);
            int section=indexPath.section-1-switch_view_subdir;
            
            if (indexPath.section==1) {
                // launch Play of current list
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
                
                //cur_local_entries[section][indexPath.row].rating=-1;
                if (section<0) pos=-1;
                [detailViewController play_listmodules:array_label start_index:pos path:array_path ratings:tmp_ratings playcounts:tmp_playcounts];
                if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                else [[super tableView] reloadData];				
                
                free(tmp_ratings);
                free(tmp_playcounts);
                
                
            } else {            
                if (cur_local_entries[section][indexPath.row].type&3) {//File selected
                    // launch Play of current dir
                    int pos=0;
                    int total_entries=0;
                    NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                    NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                    /*for (int i=0;i<27;i++) 
                     for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++)
                     if (cur_local_entries[i][j].type==1) {
                     total_entries++;
                     [array_label addObject:cur_local_entries[i][j].label];
                     [array_path addObject:cur_local_entries[i][j].fullpath];
                     if (i<section) pos++;
                     else if (i==(section))
                     if (j<indexPath.row) pos++;
                     }*/
                    [array_label addObject:cur_local_entries[section][indexPath.row].label];
                    [array_path addObject:cur_local_entries[section][indexPath.row].fullpath];
                    total_entries=1;
                    
                    signed char *tmp_ratings;
                    short int *tmp_playcounts;
                    tmp_ratings=(signed char*)malloc(total_entries*sizeof(signed char));
                    tmp_playcounts=(short int*)malloc(total_entries*sizeof(short int));
                    /*total_entries=0;
                     for (int i=0;i<27;i++) 
                     for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++)
                     if (cur_local_entries[i][j].type==1) {
                     tmp_ratings[total_entries]=cur_local_entries[i][j].rating;
                     tmp_playcounts[total_entries++]=cur_local_entries[i][j].playcount;
                     }			
                     */
                    tmp_ratings[0]=cur_local_entries[section][indexPath.row].rating;
                    tmp_playcounts[0]=cur_local_entries[section][indexPath.row].playcount;
                    
                    
                    
                    cur_local_entries[section][indexPath.row].rating=-1;
                    [detailViewController play_listmodules:array_label start_index:pos path:array_path ratings:tmp_ratings playcounts:tmp_playcounts];
                    if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                    else [[super tableView] reloadData];				
                    
                    free(tmp_ratings);
                    free(tmp_playcounts);
                    
                }
            }
        }
    }
    
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
    
    
}
- (void) secondaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [[super tableView] indexPathForRowAtPoint:[[[sender superview] superview] center]];
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [[super tableView] selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];                
    
    
    if (browse_depth==0) {
    } else {
        if (browse_mode==BROWSE_LOCAL_MODE||(browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)) {//local  browser & favorites
            int switch_view_subdir=((browse_depth>=SHOW_SUDIR_MIN_LEVEL));//&&(browse_mode==BROWSE_LOCAL_MODE)?1:0);
            int section=indexPath.section-1-switch_view_subdir;
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
                
                if ([detailViewController add_to_playlist:array_path fileNames:array_label forcenoplay:1]) {
                    if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                    else [[super tableView] reloadData];
                }
                
                free(tmp_ratings);
                free(tmp_playcounts);                                
                
            } else {            
                if (cur_local_entries[section][indexPath.row].type&3) {//File selected
                    cur_local_entries[section][indexPath.row].rating=-1;
                    if ([detailViewController add_to_playlist:cur_local_entries[section][indexPath.row].fullpath fileName:cur_local_entries[section][indexPath.row].label forcenoplay:1]) {
                        if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                        else [[super tableView] reloadData];
                    }
                }
            }
        }
    }
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
}


- (void) accessoryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [[super tableView] indexPathForRowAtPoint:[[[sender superview] superview] center]];
    [[super tableView] selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    mAccessoryButton=1;
    [self tableView:[super tableView] didSelectRowAtIndexPath:indexPath];
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
    [[super tableView] reloadData];
}

-(void) fillKeysWithPopup {
    [self fillKeys];
    [[super tableView] reloadData];
}


- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    // Navigation logic may go here. Create and push another view controller.
    //First get the dictionary object
    NSString *cellValue;
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    if (browse_depth==0) {
        NSDictionary *dictionary = [keys objectAtIndex:indexPath.section];
        NSArray *array = [dictionary objectForKey:@"entries"];
        cellValue = [array objectAtIndex:indexPath.row];
        
        if (indexPath.row==BROWSE_WEB_MODE) { 
            webBrowser.navigationItem.rightBarButtonItem=playerButton;
            if (didGoToWorldCharts) {
                didGoToWorldCharts=0;
                [webBrowser loadLastURL];
            }
            if (didGoToUserGuide) {
                didGoToUserGuide=0;
                [webBrowser loadLastURL];
            }
            [self.navigationController pushViewController:webBrowser animated:YES];	
            [keys release];keys=nil;
            [list release];list=nil;
        } else if (indexPath.row==BROWSE_WORLDCHARTS_MODE) { 
            webBrowser.navigationItem.rightBarButtonItem=playerButton;
            didGoToWorldCharts=1;
            [webBrowser loadWorldCharts];
            [self.navigationController pushViewController:webBrowser animated:YES];	
            [keys release];keys=nil;
            [list release];list=nil;
        } else if (indexPath.row==BROWSE_USERGUIDE_MODE) { 
            webBrowser.navigationItem.rightBarButtonItem=playerButton;
            didGoToUserGuide=1;
            [webBrowser loadUserGuide];
            [self.navigationController pushViewController:webBrowser animated:YES];	
            [keys release];keys=nil;
            [list release];list=nil;
        } else if (indexPath.row==BROWSE_LOCAL_MODE) {
            if (childController == nil) childController = [[RootViewControllerLocalBrowser alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
            else {			// Don't cache childviews
            }
            //set new title
            childController.title = cellValue;
            // Set new directory
            ((RootViewControllerLocalBrowser*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerLocalBrowser*)childController)->detailViewController=detailViewController;
            ((RootViewControllerLocalBrowser*)childController)->playerButton=playerButton;
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];	
            [keys release];keys=nil;
            [list release];list=nil;
        } else if (indexPath.row==BROWSE_MODLAND_MODE) {
            if (childController == nil) childController = [[RootViewControllerMODLAND alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
            else {			// Don't cache childviews
            }
            //set new title
            childController.title = cellValue;
            // Set new directory
            ((RootViewControllerMODLAND*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerMODLAND*)childController)->detailViewController=detailViewController;
            ((RootViewControllerMODLAND*)childController)->playerButton=playerButton;
            ((RootViewControllerMODLAND*)childController)->downloadViewController=downloadViewController;
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];	
            [keys release];keys=nil;
            [list release];list=nil;
        } else if (indexPath.row==BROWSE_HVSC_MODE) {
            if (childController == nil) childController = [[RootViewControllerHVSC alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
            else {			// Don't cache childviews
            }
            //set new title
            childController.title = cellValue;
            // Set new directory
            ((RootViewControllerHVSC*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerHVSC*)childController)->detailViewController=detailViewController;
            ((RootViewControllerHVSC*)childController)->playerButton=playerButton;
            ((RootViewControllerHVSC*)childController)->downloadViewController=downloadViewController;
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];	
            [keys release];keys=nil;
            [list release];list=nil;
        }  else {
            if (childController == nil) childController = [[RootViewControllerIphone alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
            else {			// Don't cache childviews
            }
            //set new title
            childController.title = cellValue;
            // Set new directory
            ((RootViewControllerIphone*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerIphone*)childController)->browse_mode = indexPath.row;
            ((RootViewControllerIphone*)childController)->detailViewController=detailViewController;
            ((RootViewControllerIphone*)childController)->downloadViewController=downloadViewController;
            ((RootViewControllerIphone*)childController)->playerButton=playerButton;
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];	
            [keys release];keys=nil;
            [list release];list=nil;
        }
    } else {
        if ((browse_mode==BROWSE_LOCAL_MODE)||(browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)) {//local  browser & favorites
            int switch_view_subdir=((browse_depth>=SHOW_SUDIR_MIN_LEVEL));//&&(browse_mode==BROWSE_LOCAL_MODE)?1:0);
            int section=indexPath.section-1-switch_view_subdir;
            if ((indexPath.section==1)&&switch_view_subdir) {
                int donothing=0;
                if (mSearch) {
                    if (mSearchText==nil) donothing=1;
                }
                if (!donothing) {
                    mShowSubdir^=1;
                    shouldFillKeys=1;
                    
                    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
                    
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
                    [[super tableView] reloadData];
                    
                    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
                }
            } else {
                cellValue=cur_local_entries[section][indexPath.row].label;
                
                if (cur_local_entries[section][indexPath.row].type==0) { //Directory selected : change current directory
                    
                    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
                    
                    NSString *newPath=[NSString stringWithFormat:@"%@/%@",currentPath,cellValue];
                    [newPath retain];        
                    if (childController == nil) childController = [[RootViewControllerIphone alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
                    else {// Don't cache childviews
                    }
                    //set new title
                    childController.title = cellValue;
                    // Set new depth & new directory
                    ((RootViewControllerIphone*)childController)->currentPath = newPath;				
                    ((RootViewControllerIphone*)childController)->browse_depth = browse_depth+1;
                    ((RootViewControllerIphone*)childController)->browse_mode = browse_mode;
                    ((RootViewControllerIphone*)childController)->detailViewController=detailViewController;
                    ((RootViewControllerIphone*)childController)->downloadViewController=downloadViewController;
                    ((RootViewControllerIphone*)childController)->playerButton=playerButton;
                    // And push the window
                    [self.navigationController pushViewController:childController animated:YES];		
                    
                    
                    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
                    //				[childController autorelease];
                } else if (((cur_local_entries[section][indexPath.row].type==2)||(cur_local_entries[section][indexPath.row].type==3))&&(mAccessoryButton)) { //Archive selected or multisongs: display files inside
                    
                    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
                    
                    NSString *newPath;
//                    NSLog(@"currentPath:%@\ncellValue:%@\nfullpath:%@",currentPath,cellValue,cur_local_entries[section][indexPath.row].fullpath);
                    if (mShowSubdir) newPath=[NSString stringWithString:cur_local_entries[section][indexPath.row].fullpath];
                    else newPath=[NSString stringWithFormat:@"%@/%@",currentPath,cellValue];
                    [newPath retain];        
                    if (childController == nil) childController = [[RootViewControllerIphone alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
                    else {// Don't cache childviews
                    }
                    //set new title
                    childController.title = cellValue;
                    // Set new depth & new directory
                    ((RootViewControllerIphone*)childController)->currentPath = newPath;				
                    ((RootViewControllerIphone*)childController)->browse_depth = browse_depth+1;
                    ((RootViewControllerIphone*)childController)->browse_mode = browse_mode;
                    ((RootViewControllerIphone*)childController)->detailViewController=detailViewController;
                    ((RootViewControllerIphone*)childController)->downloadViewController=downloadViewController;
                    ((RootViewControllerIphone*)childController)->playerButton=playerButton;
                    // And push the window
                    [self.navigationController pushViewController:childController animated:YES];		
                    
                    
                    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
                    //				[childController autorelease];
                } else {  //File selected
                    
                    if (detailViewController.sc_DefaultAction.selectedSegmentIndex==0) {
                        // launch Play of current dir
                        int pos=0;
                        int total_entries=0;
                        NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                        NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                        /*for (int i=0;i<27;i++) 
                         for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++)
                         if (cur_local_entries[i][j].type==1) {
                         total_entries++;
                         [array_label addObject:cur_local_entries[i][j].label];
                         [array_path addObject:cur_local_entries[i][j].fullpath];
                         if (i<section) pos++;
                         else if (i==(section))
                         if (j<indexPath.row) pos++;
                         }
                         */
                        total_entries=1;
                        [array_label addObject:cur_local_entries[section][indexPath.row].label];
                        [array_path addObject:cur_local_entries[section][indexPath.row].fullpath];
                        
                        
                        signed char *tmp_ratings;
                        short int *tmp_playcounts;
                        tmp_ratings=(signed char*)malloc(total_entries*sizeof(signed char));
                        tmp_playcounts=(short int*)malloc(total_entries*sizeof(short int));
                        total_entries=0;
                        /*for (int i=0;i<27;i++) 
                         for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++)
                         if (cur_local_entries[i][j].type==1) {
                         tmp_ratings[total_entries]=cur_local_entries[i][j].rating;
                         tmp_playcounts[total_entries++]=cur_local_entries[i][j].playcount;
                         }			
                         
                         */
                        tmp_ratings[0]=cur_local_entries[section][indexPath.row].rating;
                        tmp_playcounts[0]=cur_local_entries[section][indexPath.row].playcount;
                        
                        [detailViewController play_listmodules:array_label start_index:pos path:array_path ratings:tmp_ratings playcounts:tmp_playcounts];
                        
                        free(tmp_ratings);
                        free(tmp_playcounts);
                        
                        cur_local_entries[section][indexPath.row].rating=-1;
                        if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                        else [tableView reloadData];
                        
                        
                        
                    } else {
                        if ([detailViewController add_to_playlist:cur_local_entries[section][indexPath.row].fullpath fileName:cur_local_entries[section][indexPath.row].label forcenoplay:0]) {
                            if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                            else [tableView reloadData];
                        }
                    }
                    
                    
                }	
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
    
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++)
            [db_entries_data[i].label release];
        free(db_entries_data);
    }
    if (search_db_nb_entries) {
        free(search_db_entries_data);
    }
    
    if (dbHVSC_nb_entries) {
        for (int i=0;i<dbHVSC_nb_entries;i++) {
            if (dbHVSC_entries_data[i].label) [dbHVSC_entries_data[i].label release];
            if (dbHVSC_entries_data[i].fullpath) [dbHVSC_entries_data[i].fullpath release];
            if (dbHVSC_entries_data[i].id_md5) [dbHVSC_entries_data[i].id_md5 release];
            if (dbHVSC_entries_data[i].dir1) [dbHVSC_entries_data[i].dir1 release];
            if (dbHVSC_entries_data[i].dir2) [dbHVSC_entries_data[i].dir2 release];
            if (dbHVSC_entries_data[i].dir3) [dbHVSC_entries_data[i].dir3 release];
            if (dbHVSC_entries_data[i].dir4) [dbHVSC_entries_data[i].dir4 release];
            if (dbHVSC_entries_data[i].dir5) [dbHVSC_entries_data[i].dir5 release];
            
        }
        free(dbHVSC_entries_data);
    }
    if (search_dbHVSC_nb_entries) {
        free(search_dbHVSC_entries_data);
    }
    
    if (indexTitles) {
        [indexTitles release];
        indexTitles=nil;
    }
    if (indexTitlesDownload) {
        [indexTitlesDownload release];
        indexTitlesDownload=nil;
    }
    
    if (mFileMngr) {
        [mFileMngr release];
        mFileMngr=nil;
    }
    
    [super dealloc];
}


@end