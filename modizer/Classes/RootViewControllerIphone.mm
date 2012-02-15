//
//  RootViewController.m
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

//#define GET_NB_ENTRIES 1
#define NB_MODLAND_ENTRIES 297759
#define NB_HVSC_ENTRIES 41250
#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define ROW_HEIGHT 40

#define LIMITED_LIST_SIZE 1024

#include <sys/types.h>
#include <sys/sysctl.h>

#include "gme.h"

#include "unzip.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;
static 	int	mValidatePlName,newPlaylist;
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
@synthesize tabView,sBar;
@synthesize textInputView;
@synthesize list;
@synthesize keys;
@synthesize currentPath;
@synthesize inputText;
@synthesize childController;
@synthesize playerButton;
@synthesize mSearchText;
@synthesize mFileMngr;

#pragma mark -
#pragma mark View lifecycle

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
	}
	
	[fileManager copyItemAtPath:defaultDBPath toPath:pathToDB error:&error];
	
	
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
	//self.tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
	self.tableView.rowHeight = 50;
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
	mRenamePlaylist=0;
	mCurrentWinAskedDownload=0;
	mClickedPrimAction=0;
	list=nil;
	keys=nil;
	mFreePlaylist=0;
	
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
	
	if ((browse_depth==MENU_BROWSER_LOCAL_ROOTLEVEL)&&(browse_mode==BROWSE_LOCAL_MODE)) { //Local mode
		currentPath = @"Documents";
		[currentPath retain];
	}
	if ((browse_depth==MENU_BROWSER_PLAYLIST_ROOTLEVEL)&&(browse_mode==BROWSE_PLAYLIST_MODE)) { //Playlist/Local mode
		currentPath = @"Documents";
		[currentPath retain];
	}
	if (browse_mode==BROWSE_PLAYLIST_MODE) {//playlist
		if (show_playlist) {
			self.navigationItem.rightBarButtonItem = self.editButtonItem;
			sBar.frame=CGRectMake(0,0,0,0);
			sBar.hidden=TRUE;
		} else self.navigationItem.rightBarButtonItem = playerButton;
		if (browse_depth==MENU_PLAYLIST_ROOTLEVEL) {
			//	self.navigationItem.rightBarButtonItem = self.editButtonItem;
			sBar.frame=CGRectMake(0,0,0,0);
			sBar.hidden=TRUE;
		}
		
	}
	if (browse_mode==BROWSE_LOCAL_MODE) {//local
		self.navigationItem.rightBarButtonItem = playerButton; //self.editButtonItem;
	}
	if (browse_mode==BROWSE_MODLAND_MODE) {//modland
		if (browse_depth==MENU_COLLECTION_MODLAND_ROOTLEVEL) { //browse mode menu
			sBar.frame=CGRectMake(0,0,0,0);
			sBar.hidden=TRUE;
			//get stats on nb of entries
			mNbFormatEntries=DBHelper::getNbFormatEntries();
			mNbAuthorEntries=DBHelper::getNbAuthorEntries();
		}
		self.navigationItem.rightBarButtonItem = playerButton; //self.editButtonItem;
	}
	if (browse_mode==BROWSE_HVSC_MODE) {//modland
		self.navigationItem.rightBarButtonItem = playerButton; //self.editButtonItem;
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
	} else if (browse_mode==BROWSE_PLAYLIST_MODE) {//Playlist
		if (browse_depth==MENU_PLAYLIST_ROOTLEVEL) {
			keys = [[NSMutableArray alloc] init];
			list = [[NSMutableArray alloc] init];
			NSMutableArray *mode_entries = [[[NSMutableArray alloc] init] autorelease];
			[mode_entries addObject:NSLocalizedString(@"New playlist",@"")];
			[mode_entries addObject:NSLocalizedString(@"Save current one",@"")];
			[self loadPlayListsListFromDB:mode_entries list_id:list];
			NSDictionary *mode_entriesDict = [NSDictionary dictionaryWithObject:mode_entries forKey:@"entries"];
			[keys addObject:mode_entriesDict];
		} else if (show_playlist==0) {
			[self listLocalFiles];
		}
	} else if (browse_mode==BROWSE_LOCAL_MODE) {//local browser		
		if (shouldFillKeys) {
			shouldFillKeys=0;						
			[self listLocalFiles];
		}
	} else if (browse_mode==BROWSE_MODLAND_MODE) {//Modland DB
		if (shouldFillKeys) {
			shouldFillKeys=0;
			switch (modland_browse_mode) {
				case 0://Formats/Authors/Files
					if (browse_depth==MENU_COLLECTIONS_MODLAND_FAF_1) [self fillKeysWithDB_fileType];
					if (browse_depth==3) [self fillKeysWithDB_fileAuthor:mFiletypeID];
					if (browse_depth==4) [self fillKeysWithDB_albumORfilename:mFiletypeID fileAuthorID:mAuthorID];
					if (browse_depth==5) [self fillKeysWithDB_filename:mFiletypeID fileAuthorID:mAuthorID fileAlbumID:mAlbumID];
					break;
				case 1://Authors/Formats/Files
					if (browse_depth==2) [self fillKeysWithDB_fileAuthor];
					if (browse_depth==3) [self fillKeysWithDB_fileType:mAuthorID];
					if (browse_depth==4) [self fillKeysWithDB_albumORfilename:mFiletypeID fileAuthorID:mAuthorID];
					if (browse_depth==5) [self fillKeysWithDB_filename:mFiletypeID fileAuthorID:mAuthorID fileAlbumID:mAlbumID];
					break;
				case 2://Authors/Files
					if (browse_depth==2) [self fillKeysWithDB_fileAuthor];
					if (browse_depth==3) [self fillKeysWithDB_albumORfilename:mAuthorID];
					if (browse_depth==4) [self fillKeysWithDB_filename:mAuthorID fileAlbumID:mAlbumID];
					break;
			}
		} else {//reset downloaded, rating & playcount flags
			for (int i=0;i<db_nb_entries;i++) {
				db_entries_data[i].downloaded=-1;
				db_entries_data[i].rating=-1;
				db_entries_data[i].playcount=-1;
			}
		}
	}  else if (browse_mode==BROWSE_HVSC_MODE) {//HVSC
		if (shouldFillKeys) {
			shouldFillKeys=0;
			if (browse_depth==1) [self fillKeysWithHVSCDB_Dir1];
			else if (browse_depth==2) [self fillKeysWithHVSCDB_Dir2:mDir1];
			else if (browse_depth==3) [self fillKeysWithHVSCDB_Dir3:mDir1 dir2:mDir2];
			else if (browse_depth==4) [self fillKeysWithHVSCDB_Dir4:mDir1 dir2:mDir2 dir3:mDir3];
			else if (browse_depth==5) [self fillKeysWithHVSCDB_Dir5:mDir1 dir2:mDir2 dir3:mDir3 dir4:mDir4];
			else if (browse_depth==6) [self fillKeysWithHVSCDB_AllDirs:mDir1 dir2:mDir2 dir3:mDir3 dir4:mDir4 dir5:mDir5];
		} else { //reset downloaded, rating & playcount flags
			for (int i=0;i<dbHVSC_nb_entries;i++) {
				dbHVSC_entries_data[i].downloaded=-1;
				dbHVSC_entries_data[i].rating=-1;
				dbHVSC_entries_data[i].playcount=-1;
			}
		}
	} else if (browse_mode==BROWSE_RATED_MODE) {//Top Rating
		[self loadFavoritesList];
	} else if (browse_mode==BROWSE_MOSTPLAYED_MODE) {//Top Most played
		[self loadMostPlayedList];
	} else if (browse_mode==BROWSE_WEB_MODE) {//Web browser
	} else if (browse_mode==BROWSE_WORLDCHARTS_MODE) {//Web/World Charts
	}  else if (browse_mode==BROWSE_USERGUIDE_MODE) {//Web/User Guide
	}
}

-(void) fillKeysWithHVSCDB_Dir1 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
	int index,previndex;
	
	NSRange r;
	
	dbHVSC_hasFiles=search_dbHVSC_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		if (search_dbHVSC_nb_entries) {
			search_dbHVSC_nb_entries=0;
			free(search_dbHVSC_entries_data);
		}
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_dbHVSC_entries_count[i]=0;
			if (dbHVSC_entries_count[i]) search_dbHVSC_entries[i]=&(search_dbHVSC_entries_data[search_dbHVSC_nb_entries]);
			for (int j=0;j<dbHVSC_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [dbHVSC_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].label=dbHVSC_entries[i][j].label;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].downloaded=dbHVSC_entries[i][j].downloaded;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].rating=dbHVSC_entries[i][j].rating;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].playcount=dbHVSC_entries[i][j].playcount;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].filesize=dbHVSC_entries[i][j].filesize;
					
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].id_md5=dbHVSC_entries[i][j].id_md5;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].fullpath=dbHVSC_entries[i][j].fullpath;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir1=dbHVSC_entries[i][j].dir1;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir2=dbHVSC_entries[i][j].dir2;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir3=dbHVSC_entries[i][j].dir3;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir4=dbHVSC_entries[i][j].dir4;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir5=dbHVSC_entries[i][j].dir5;
					
					search_dbHVSC_entries_count[i]++;
					search_dbHVSC_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
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
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT COUNT(DISTINCT dir1) FROM hvsc_file WHERE dir1 LIKE \"%%%s%%\"",[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT COUNT(DISTINCT dir1) FROM hvsc_file");
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			memset(dbHVSC_entries_data,0,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			dbHVSC_entries_index=0;
			for (int i=0;i<27;i++) {
				dbHVSC_entries_count[i]=0;
				dbHVSC_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT dir1,COUNT(1) FROM hvsc_file WHERE dir1 LIKE \"%%%s%%\" GROUP BY dir1 ORDER BY dir1 COLLATE NOCASE",[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT dir1,COUNT(1) FROM hvsc_file GROUP BY dir1 ORDER BY dir1 COLLATE NOCASE");
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else dbHVSC_entries[index]=&(dbHVSC_entries_data[dbHVSC_entries_index]);
					}
					dbHVSC_entries[index][dbHVSC_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir1=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].filesize=sqlite3_column_int(stmt, 1);
					
					dbHVSC_entries[index][dbHVSC_entries_count[index]].downloaded=-1;
					dbHVSC_entries[index][dbHVSC_entries_count[index]].rating=-1;
					dbHVSC_entries[index][dbHVSC_entries_count[index]].playcount=-1;
					dbHVSC_entries_count[index]++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	
}
-(void) fillKeysWithHVSCDB_Dir2:(NSString*)dir1 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
	int index,previndex;
	
	dbHVSC_hasFiles=search_dbHVSC_hasFiles=0;
	
	NSRange r;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		if (search_dbHVSC_nb_entries) {
			search_dbHVSC_nb_entries=0;
			free(search_dbHVSC_entries_data);
		}
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_dbHVSC_entries_count[i]=0;
			if (dbHVSC_entries_count[i]) search_dbHVSC_entries[i]=&(search_dbHVSC_entries_data[search_dbHVSC_nb_entries]);
			for (int j=0;j<dbHVSC_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [dbHVSC_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].label=dbHVSC_entries[i][j].label;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].downloaded=dbHVSC_entries[i][j].downloaded;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].rating=dbHVSC_entries[i][j].rating;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].playcount=dbHVSC_entries[i][j].playcount;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].filesize=dbHVSC_entries[i][j].filesize;
					
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].id_md5=dbHVSC_entries[i][j].id_md5;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].fullpath=dbHVSC_entries[i][j].fullpath;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir1=dbHVSC_entries[i][j].dir1;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir2=dbHVSC_entries[i][j].dir2;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir3=dbHVSC_entries[i][j].dir3;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir4=dbHVSC_entries[i][j].dir4;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir5=dbHVSC_entries[i][j].dir5;
					
					search_dbHVSC_entries_count[i]++;
					search_dbHVSC_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
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
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT COUNT(DISTINCT dir2) FROM hvsc_file WHERE dir1=\"%s\" AND dir2 LIKE \"%%%s%%\"",[dir1 UTF8String],[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT COUNT(DISTINCT dir2) FROM hvsc_file WHERE dir1=\"%s\"",[dir1 UTF8String]);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			memset(dbHVSC_entries_data,0,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			dbHVSC_entries_index=0;
			for (int i=0;i<27;i++) {
				dbHVSC_entries_count[i]=0;
				dbHVSC_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT dir2,COUNT(1) FROM hvsc_file WHERE dir1=\"%s\" AND dir2 LIKE \"%%%s%%\" GROUP BY dir2 ORDER BY dir2 COLLATE NOCASE",[dir1 UTF8String],[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT dir2,COUNT(1) FROM hvsc_file WHERE dir1=\"%s\" GROUP BY dir2 ORDER BY dir2 COLLATE NOCASE",[dir1 UTF8String]);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else dbHVSC_entries[index]=&(dbHVSC_entries_data[dbHVSC_entries_index]);
					}
					dbHVSC_entries[index][dbHVSC_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir1=[[NSString alloc] initWithString:dir1];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir2=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].filesize=sqlite3_column_int(stmt, 1);
					dbHVSC_entries[index][dbHVSC_entries_count[index]].downloaded=-1;
					dbHVSC_entries[index][dbHVSC_entries_count[index]].rating=-1;
					dbHVSC_entries[index][dbHVSC_entries_count[index]].playcount=-1;
					dbHVSC_entries_count[index]++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithHVSCDB_Dir3:(NSString*)dir1 dir2:(NSString*)dir2 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
	int index,previndex;
	
	NSRange r;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		if (search_dbHVSC_nb_entries) {
			search_dbHVSC_nb_entries=0;
			free(search_dbHVSC_entries_data);
		}
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		search_dbHVSC_hasFiles=0;
		for (int i=0;i<27;i++) {
			search_dbHVSC_entries_count[i]=0;
			if (dbHVSC_entries_count[i]) search_dbHVSC_entries[i]=&(search_dbHVSC_entries_data[search_dbHVSC_nb_entries]);
			for (int j=0;j<dbHVSC_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [dbHVSC_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].label=dbHVSC_entries[i][j].label;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].downloaded=dbHVSC_entries[i][j].downloaded;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].rating=dbHVSC_entries[i][j].rating;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].playcount=dbHVSC_entries[i][j].playcount;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].filesize=dbHVSC_entries[i][j].filesize;
					
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].id_md5=dbHVSC_entries[i][j].id_md5;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].fullpath=dbHVSC_entries[i][j].fullpath;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir1=dbHVSC_entries[i][j].dir1;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir2=dbHVSC_entries[i][j].dir2;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir3=dbHVSC_entries[i][j].dir3;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir4=dbHVSC_entries[i][j].dir4;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir5=dbHVSC_entries[i][j].dir5;
					
					if (dbHVSC_entries[i][j].id_md5) search_dbHVSC_hasFiles++;
					
					search_dbHVSC_entries_count[i]++;
					search_dbHVSC_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
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
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	dbHVSC_hasFiles=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT COUNT(DISTINCT dir3) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is not null AND dir3 LIKE \"%%%s%%\"\
							 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is null AND filename LIKE \"%%%s%%\"",[dir1 UTF8String],[dir2 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT COUNT(DISTINCT dir3) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is not null\
					 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is null",[dir1 UTF8String],[dir2 UTF8String],[dir1 UTF8String],[dir2 UTF8String]);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		
		
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			memset(dbHVSC_entries_data,0,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			dbHVSC_entries_index=0;
			for (int i=0;i<27;i++) {
				dbHVSC_entries_count[i]=0;
				dbHVSC_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT dir3,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is not null AND dir3 LIKE \"%%%s%%\" GROUP BY dir3 \
								 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is null AND filename LIKE \"%%%s%%\" \
								 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT dir3,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is not null GROUP BY dir3\
						 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is null \
						 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir1 UTF8String],[dir2 UTF8String]);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else dbHVSC_entries[index]=&(dbHVSC_entries_data[dbHVSC_entries_index]);
					}
					dbHVSC_entries[index][dbHVSC_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir1=[[NSString alloc] initWithString:dir1];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir2=[[NSString alloc] initWithString:dir2];
					
					if (sqlite3_column_int(stmt, 4)==0) {
						dbHVSC_entries[index][dbHVSC_entries_count[index]].dir3=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
						dbHVSC_entries[index][dbHVSC_entries_count[index]].filesize=sqlite3_column_int(stmt, 3);
					} else {
						dbHVSC_entries[index][dbHVSC_entries_count[index]].id_md5=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 2)];
						dbHVSC_entries[index][dbHVSC_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
						dbHVSC_hasFiles++;
					}
					
					dbHVSC_entries[index][dbHVSC_entries_count[index]].downloaded=-1;
					dbHVSC_entries[index][dbHVSC_entries_count[index]].rating=-1;
					dbHVSC_entries[index][dbHVSC_entries_count[index]].playcount=-1;
					dbHVSC_entries_count[index]++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithHVSCDB_Dir4:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
	int index,previndex;
	
	NSRange r;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		if (search_dbHVSC_nb_entries) {
			search_dbHVSC_nb_entries=0;
			free(search_dbHVSC_entries_data);
		}
		search_dbHVSC_hasFiles=0;
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_dbHVSC_entries_count[i]=0;
			if (dbHVSC_entries_count[i]) search_dbHVSC_entries[i]=&(search_dbHVSC_entries_data[search_dbHVSC_nb_entries]);
			for (int j=0;j<dbHVSC_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [dbHVSC_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].label=dbHVSC_entries[i][j].label;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].downloaded=dbHVSC_entries[i][j].downloaded;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].rating=dbHVSC_entries[i][j].rating;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].playcount=dbHVSC_entries[i][j].playcount;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].filesize=dbHVSC_entries[i][j].filesize;
					
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].id_md5=dbHVSC_entries[i][j].id_md5;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].fullpath=dbHVSC_entries[i][j].fullpath;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir1=dbHVSC_entries[i][j].dir1;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir2=dbHVSC_entries[i][j].dir2;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir3=dbHVSC_entries[i][j].dir3;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir4=dbHVSC_entries[i][j].dir4;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir5=dbHVSC_entries[i][j].dir5;
					
					if (dbHVSC_entries[i][j].id_md5) search_dbHVSC_hasFiles++;
					
					search_dbHVSC_entries_count[i]++;
					search_dbHVSC_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
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
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	dbHVSC_hasFiles=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT COUNT(DISTINCT dir4) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is not null AND dir4 LIKE \"%%%s%%\"\
							 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is null AND filename LIKE \"%%%s%%\"",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT COUNT(DISTINCT dir4) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is not null\
					 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is null",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String]);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		
		
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			memset(dbHVSC_entries_data,0,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			dbHVSC_entries_index=0;
			for (int i=0;i<27;i++) {
				dbHVSC_entries_count[i]=0;
				dbHVSC_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT dir4,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is not null AND dir3 LIKE \"%%%s%%\" GROUP BY dir4 \
								 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is null AND filename LIKE \"%%%s%%\" \
								 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT dir4,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is not null GROUP BY dir4 \
						 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is null \
						 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String]);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else dbHVSC_entries[index]=&(dbHVSC_entries_data[dbHVSC_entries_index]);
					}
					dbHVSC_entries[index][dbHVSC_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir1=[[NSString alloc] initWithString:dir1];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir2=[[NSString alloc] initWithString:dir2];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir3=[[NSString alloc] initWithString:dir3];
					if (sqlite3_column_int(stmt, 4)==0) {
						dbHVSC_entries[index][dbHVSC_entries_count[index]].dir4=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
						dbHVSC_entries[index][dbHVSC_entries_count[index]].filesize=sqlite3_column_int(stmt, 3);
					} else {
						dbHVSC_hasFiles++;
						dbHVSC_entries[index][dbHVSC_entries_count[index]].id_md5=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 2)];
						dbHVSC_entries[index][dbHVSC_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
					}
					
					dbHVSC_entries[index][dbHVSC_entries_count[index]].downloaded=-1;					
					dbHVSC_entries[index][dbHVSC_entries_count[index]].rating=-1;
					dbHVSC_entries[index][dbHVSC_entries_count[index]].playcount=-1;
					dbHVSC_entries_count[index]++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithHVSCDB_Dir5:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
	int index,previndex;
	
	NSRange r;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		if (search_dbHVSC_nb_entries) {
			search_dbHVSC_nb_entries=0;
			free(search_dbHVSC_entries_data);
		}
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		search_dbHVSC_hasFiles=0;
		for (int i=0;i<27;i++) {
			search_dbHVSC_entries_count[i]=0;
			if (dbHVSC_entries_count[i]) search_dbHVSC_entries[i]=&(search_dbHVSC_entries_data[search_dbHVSC_nb_entries]);
			for (int j=0;j<dbHVSC_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [dbHVSC_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].label=dbHVSC_entries[i][j].label;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].downloaded=dbHVSC_entries[i][j].downloaded;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].rating=dbHVSC_entries[i][j].rating;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].playcount=dbHVSC_entries[i][j].playcount;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].filesize=dbHVSC_entries[i][j].filesize;
					
					
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].id_md5=dbHVSC_entries[i][j].id_md5;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].fullpath=dbHVSC_entries[i][j].fullpath;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir1=dbHVSC_entries[i][j].dir1;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir2=dbHVSC_entries[i][j].dir2;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir3=dbHVSC_entries[i][j].dir3;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir4=dbHVSC_entries[i][j].dir4;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir5=dbHVSC_entries[i][j].dir5;
					
					if (dbHVSC_entries[i][j].id_md5) search_dbHVSC_hasFiles++;
					
					search_dbHVSC_entries_count[i]++;
					search_dbHVSC_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
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
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	dbHVSC_hasFiles=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT COUNT(DISTINCT dir5) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is not null AND dir5 LIKE \"%%%s%%\"\
							 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is null AND filename LIKE \"%%%s%%\"",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT COUNT(DISTINCT dir5) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is not null\
					 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is null",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String]);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		
		
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			memset(dbHVSC_entries_data,0,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			dbHVSC_entries_index=0;
			for (int i=0;i<27;i++) {
				dbHVSC_entries_count[i]=0;
				dbHVSC_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT dir5,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is not null AND dir5 LIKE \"%%%s%%\" GROUP BY dir5 \
								 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is null AND filename LIKE \"%%%s%%\" \
								 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT dir5,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is not null GROUP BY dir5 \
						 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is null \
						 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String]);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else dbHVSC_entries[index]=&(dbHVSC_entries_data[dbHVSC_entries_index]);
					}
					dbHVSC_entries[index][dbHVSC_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir1=[[NSString alloc] initWithString:dir1];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir2=[[NSString alloc] initWithString:dir2];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir3=[[NSString alloc] initWithString:dir3];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir4=[[NSString alloc] initWithString:dir4];
					if (sqlite3_column_int(stmt, 4)==0) {
						dbHVSC_entries[index][dbHVSC_entries_count[index]].dir5=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
						dbHVSC_entries[index][dbHVSC_entries_count[index]].filesize=sqlite3_column_int(stmt, 3);
					} else {
						dbHVSC_entries[index][dbHVSC_entries_count[index]].id_md5=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 2)];
						dbHVSC_entries[index][dbHVSC_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
						dbHVSC_hasFiles++;
					}
					
					dbHVSC_entries[index][dbHVSC_entries_count[index]].downloaded=-1;
					dbHVSC_entries[index][dbHVSC_entries_count[index]].rating=-1;
					dbHVSC_entries[index][dbHVSC_entries_count[index]].playcount=-1;
					dbHVSC_entries_count[index]++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithHVSCDB_AllDirs:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4 dir5:(NSString*)dir5 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
	int index,previndex;
	
	NSRange r;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		if (search_dbHVSC_nb_entries) {
			search_dbHVSC_nb_entries=0;
			free(search_dbHVSC_entries_data);
		}
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		search_dbHVSC_hasFiles=0;
		for (int i=0;i<27;i++) {
			search_dbHVSC_entries_count[i]=0;
			if (dbHVSC_entries_count[i]) search_dbHVSC_entries[i]=&(search_dbHVSC_entries_data[search_dbHVSC_nb_entries]);
			for (int j=0;j<dbHVSC_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [dbHVSC_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].label=dbHVSC_entries[i][j].label;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].downloaded=dbHVSC_entries[i][j].downloaded;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].rating=dbHVSC_entries[i][j].rating;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].playcount=dbHVSC_entries[i][j].playcount;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].filesize=dbHVSC_entries[i][j].filesize;
					
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].id_md5=dbHVSC_entries[i][j].id_md5;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].fullpath=dbHVSC_entries[i][j].fullpath;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir1=dbHVSC_entries[i][j].dir1;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir2=dbHVSC_entries[i][j].dir2;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir3=dbHVSC_entries[i][j].dir3;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir4=dbHVSC_entries[i][j].dir4;
					search_dbHVSC_entries[i][search_dbHVSC_entries_count[i]].dir5=dbHVSC_entries[i][j].dir5;
					
					if (dbHVSC_entries[i][j].id_md5) search_db_hasFiles++;
					
					search_dbHVSC_entries_count[i]++;
					search_dbHVSC_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
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
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	dbHVSC_hasFiles=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5=\"%s\" AND filename LIKE \"%%%s%%\"",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir5 UTF8String],[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5=\"%s\"",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir5 UTF8String]);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		
		
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)malloc(dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			memset(dbHVSC_entries_data,0,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			dbHVSC_entries_index=0;
			for (int i=0;i<27;i++) {
				dbHVSC_entries_count[i]=0;
				dbHVSC_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT filename,fullpath,id_md5,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5=\"%s\" AND filename LIKE \"%%%s%%\" \
								 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir5 UTF8String],[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT filename,fullpath,id_md5,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5=\"%s\"\
						 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir5 UTF8String]);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else dbHVSC_entries[index]=&(dbHVSC_entries_data[dbHVSC_entries_index]);
					}
					dbHVSC_entries[index][dbHVSC_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir1=[[NSString alloc] initWithString:dir1];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir2=[[NSString alloc] initWithString:dir2];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir3=[[NSString alloc] initWithString:dir3];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir4=[[NSString alloc] initWithString:dir4];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].dir5=[[NSString alloc] initWithString:dir5];
					
					dbHVSC_entries[index][dbHVSC_entries_count[index]].id_md5=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 2)];
					dbHVSC_entries[index][dbHVSC_entries_count[index]].fullpath=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
					dbHVSC_hasFiles++;
					
					dbHVSC_entries[index][dbHVSC_entries_count[index]].downloaded=-1;					
					dbHVSC_entries[index][dbHVSC_entries_count[index]].rating=-1;
					dbHVSC_entries[index][dbHVSC_entries_count[index]].playcount=-1;
					dbHVSC_entries_count[index]++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}

-(void) fillKeysWithDB_fileType:(int)authorID{
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
	int index,previndex;
	NSRange r;
	db_hasFiles=search_db_hasFiles=0;
	
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		if (search_db_nb_entries) {
			search_db_nb_entries=0;
			free(search_db_entries_data);
		}
		search_db_entries_data=(t_db_browse_entry*)malloc(db_nb_entries*sizeof(t_db_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_db_entries_count[i]=0;
			if (db_entries_count[i]) search_db_entries[i]=&(search_db_entries_data[search_db_nb_entries]);
			for (int j=0;j<db_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [db_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_db_entries[i][search_db_entries_count[i]].label=db_entries[i][j].label;
					search_db_entries[i][search_db_entries_count[i]].downloaded=db_entries[i][j].downloaded;
					search_db_entries[i][search_db_entries_count[i]].rating=db_entries[i][j].rating;
					search_db_entries[i][search_db_entries_count[i]].playcount=db_entries[i][j].playcount;
					search_db_entries[i][search_db_entries_count[i]].id_type=db_entries[i][j].id_type;
					
					search_db_entries[i][search_db_entries_count[i]].id_author=search_db_entries[i][search_db_entries_count[i]].id_album=search_db_entries[i][search_db_entries_count[i]].id_mod=-1;
					search_db_entries[i][search_db_entries_count[i]].filesize=db_entries[i][j].filesize;										
					search_db_entries_count[i]++;
					search_db_nb_entries++;
				}
			}
		}
		return;
	}
	
	pthread_mutex_lock(&db_mutex);
	
	if (db_nb_entries) {
		for (int i=0;i<db_nb_entries;i++) {
			[db_entries_data[i].label release];
		}
		free(db_entries_data);db_entries_data=NULL;
		db_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have
		if (mSearch) sprintf(sqlStatement,"SELECT count(1) FROM mod_type t,mod_type_author ta\
							 WHERE ta.id_author=%d AND ta.id_type=t.id AND t.filetype like \"%%%s%%\"",authorID,[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT count(1) FROM mod_type t,mod_type_author ta\
					 WHERE ta.id_author=%d AND ta.id_type=t.id",authorID);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)malloc(db_nb_entries*sizeof(t_db_browse_entry));
			db_entries_index=0;
			for (int i=0;i<27;i++) {
				db_entries_count[i]=0;
				db_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT t.filetype,ta.num_files,t.id FROM mod_type t,mod_type_author ta\
								 WHERE ta.id_author=%d AND ta.id_type=t.id AND t.filetype like \"%%%s%%\" ORDER BY t.filetype COLLATE NOCASE",authorID,[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT t.filetype,ta.num_files,t.id FROM mod_type t,mod_type_author ta\
						 WHERE ta.id_author=%d AND ta.id_type=t.id ORDER BY t.filetype COLLATE NOCASE",authorID);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else db_entries[index]=&(db_entries_data[db_entries_index]);
					}
					db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[index][db_entries_count[index]].filesize=sqlite3_column_int(stmt, 1);
					db_entries[index][db_entries_count[index]].id_type=sqlite3_column_int(stmt, 2);
					db_entries[index][db_entries_count[index]].id_author=authorID;
					db_entries[index][db_entries_count[index]].id_album=db_entries[index][db_entries_count[index]].id_mod=-1;
					db_entries[index][db_entries_count[index]].downloaded=-1;
					db_entries[index][db_entries_count[index]].rating=-1;
					db_entries[index][db_entries_count[index]].playcount=-1;
					db_entries_count[index]++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_fileType{
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
	int index,previndex;
	NSRange r;
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		if (search_db_nb_entries) {
			search_db_nb_entries=0;
			free(search_db_entries_data);
		}
		search_db_entries_data=(t_db_browse_entry*)malloc(db_nb_entries*sizeof(t_db_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_db_entries_count[i]=0;
			if (db_entries_count[i]) search_db_entries[i]=&(search_db_entries_data[search_db_nb_entries]);
			for (int j=0;j<db_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [db_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_db_entries[i][search_db_entries_count[i]].label=db_entries[i][j].label;
					search_db_entries[i][search_db_entries_count[i]].downloaded=db_entries[i][j].downloaded;
					search_db_entries[i][search_db_entries_count[i]].rating=db_entries[i][j].rating;
					search_db_entries[i][search_db_entries_count[i]].playcount=db_entries[i][j].playcount;
					search_db_entries[i][search_db_entries_count[i]].id_type=db_entries[i][j].id_type;
					
					search_db_entries[i][search_db_entries_count[i]].id_author=search_db_entries[i][search_db_entries_count[i]].id_album=search_db_entries[i][search_db_entries_count[i]].id_mod=-1;
					search_db_entries[i][search_db_entries_count[i]].filesize=db_entries[i][j].filesize;
					search_db_entries_count[i]++;
					search_db_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (db_nb_entries) {
		for (int i=0;i<db_nb_entries;i++) {
			[db_entries_data[i].label release];
		}
		free(db_entries_data);db_entries_data=NULL;
		db_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT COUNT(1) FROM mod_type WHERE filetype LIKE \"%%%s%%\"",[mSearchText UTF8String]);
		else  sprintf(sqlStatement,"SELECT COUNT(1) FROM mod_type");
		
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)malloc(db_nb_entries*sizeof(t_db_browse_entry));
			db_entries_index=0;
			for (int i=0;i<27;i++) {
				db_entries_count[i]=0;
				db_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT filetype,num_files,id FROM mod_type WHERE filetype LIKE \"%%%s%%\" ORDER BY filetype COLLATE NOCASE",[mSearchText UTF8String]);
			else  sprintf(sqlStatement,"SELECT filetype,num_files,id FROM mod_type ORDER BY filetype COLLATE NOCASE");
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else db_entries[index]=&(db_entries_data[db_entries_index]);
					}
					db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[index][db_entries_count[index]].filesize=sqlite3_column_int(stmt, 1);
					db_entries[index][db_entries_count[index]].id_type=sqlite3_column_int(stmt, 2);
					db_entries[index][db_entries_count[index]].id_author=db_entries[index][db_entries_count[index]].id_album=db_entries[index][db_entries_count[index]].id_mod=-1;
					db_entries[index][db_entries_count[index]].downloaded=-1;		
					db_entries[index][db_entries_count[index]].rating=-1;
					db_entries[index][db_entries_count[index]].playcount=-1;
					db_entries_count[index]++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_fileAuthor{
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
	int index,previndex;
	NSRange r;
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		if (search_db_nb_entries) {
			search_db_nb_entries=0;
			free(search_db_entries_data);
		}
		search_db_entries_data=(t_db_browse_entry*)malloc(db_nb_entries*sizeof(t_db_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_db_entries_count[i]=0;
			if (db_entries_count[i]) search_db_entries[i]=&(search_db_entries_data[search_db_nb_entries]);
			for (int j=0;j<db_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [db_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_db_entries[i][search_db_entries_count[i]].label=db_entries[i][j].label;
					search_db_entries[i][search_db_entries_count[i]].id_author=db_entries[i][j].id_author;
					search_db_entries[i][search_db_entries_count[i]].downloaded=db_entries[i][j].downloaded;
					search_db_entries[i][search_db_entries_count[i]].rating=db_entries[i][j].rating;
					search_db_entries[i][search_db_entries_count[i]].playcount=db_entries[i][j].playcount;
					search_db_entries[i][search_db_entries_count[i]].id_type=search_db_entries[i][search_db_entries_count[i]].id_album=search_db_entries[i][search_db_entries_count[i]].id_mod=-1;
					search_db_entries[i][search_db_entries_count[i]].filesize=db_entries[i][j].filesize;
					search_db_entries_count[i]++;
					search_db_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (db_nb_entries) {
		for (int i=0;i<db_nb_entries;i++) {
			[db_entries_data[i].label release];
		}
		free(db_entries_data);db_entries_data=NULL;
		db_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT count(1) FROM mod_author WHERE author LIKE \"%%%s%%\"",[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT count(1) FROM mod_author");
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)malloc(db_nb_entries*sizeof(t_db_browse_entry));
			db_entries_index=0;
			for (int i=0;i<27;i++) {
				db_entries_count[i]=0;
				db_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT author,num_files,id FROM mod_author WHERE author LIKE \"%%%s%%\" ORDER BY author COLLATE NOCASE",[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT author,num_files,id FROM mod_author ORDER BY author COLLATE NOCASE");
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else db_entries[index]=&(db_entries_data[db_entries_index]);
					}
					
					
					//if (sqlite3_column_int(stmt, 1)==37) NSLog(@"%s",(const char*)sqlite3_column_text(stmt, 0));
					
					db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					if (db_entries[index][db_entries_count[index]].label==nil) db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithFormat:@"%s",(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[index][db_entries_count[index]].filesize=sqlite3_column_int(stmt, 1);
					db_entries[index][db_entries_count[index]].id_author=sqlite3_column_int(stmt, 2);
					db_entries[index][db_entries_count[index]].id_type=db_entries[index][db_entries_count[index]].id_album=db_entries[index][db_entries_count[index]].id_mod=-1;
					db_entries[index][db_entries_count[index]].downloaded=-1;
					db_entries[index][db_entries_count[index]].rating=-1;
					db_entries[index][db_entries_count[index]].playcount=-1;
					db_entries_count[index]++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_fileAuthor:(int)filetypeID{
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
	int index,previndex;
	NSRange r;
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		if (search_db_nb_entries) {
			search_db_nb_entries=0;
			free(search_db_entries_data);
		}
		search_db_entries_data=(t_db_browse_entry*)malloc(db_nb_entries*sizeof(t_db_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_db_entries_count[i]=0;
			if (db_entries_count[i]) search_db_entries[i]=&(search_db_entries_data[search_db_nb_entries]);
			for (int j=0;j<db_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [db_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_db_entries[i][search_db_entries_count[i]].label=db_entries[i][j].label;
					
					search_db_entries[i][search_db_entries_count[i]].id_author=db_entries[i][j].id_author;
					search_db_entries[i][search_db_entries_count[i]].downloaded=db_entries[i][j].downloaded;
					search_db_entries[i][search_db_entries_count[i]].rating=db_entries[i][j].rating;
					search_db_entries[i][search_db_entries_count[i]].playcount=db_entries[i][j].playcount;
					
					search_db_entries[i][search_db_entries_count[i]].id_type=filetypeID;
					search_db_entries[i][search_db_entries_count[i]].id_album=search_db_entries[i][search_db_entries_count[i]].id_mod=-1;
					search_db_entries[i][search_db_entries_count[i]].filesize=db_entries[i][j].filesize;
					search_db_entries_count[i]++;
					search_db_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (db_nb_entries) {
		for (int i=0;i<db_nb_entries;i++) {
			[db_entries_data[i].label release];
		}
		free(db_entries_data);db_entries_data=NULL;
		db_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT COUNT(1) FROM mod_author a,mod_type_author m WHERE m.id_type=%d AND m.id_author=a.id AND a.author LIKE \"%%%s%%\"",filetypeID,[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT COUNT(1) FROM mod_type_author m WHERE m.id_type=%d",filetypeID);
		
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)malloc(db_nb_entries*sizeof(t_db_browse_entry));
			db_entries_index=0;
			for (int i=0;i<27;i++) {
				db_entries_count[i]=0;
				db_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT a.author,m.num_files,a.id FROM mod_author a,mod_type_author m WHERE m.id_type=%d AND m.id_author=a.id AND a.author LIKE \"%%%s%%\" ORDER BY a.author COLLATE NOCASE",filetypeID,[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT a.author,m.num_files,a.id FROM mod_type_author m,mod_author a WHERE m.id_type=%d AND m.id_author=a.id ORDER BY a.author COLLATE NOCASE",filetypeID);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            
            
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else db_entries[index]=&(db_entries_data[db_entries_index]);
					}
					db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					
					db_entries[index][db_entries_count[index]].filesize=sqlite3_column_int(stmt, 1);
					db_entries[index][db_entries_count[index]].id_author=sqlite3_column_int(stmt, 2);
					db_entries[index][db_entries_count[index]].id_type=filetypeID;
					db_entries[index][db_entries_count[index]].id_album=db_entries[index][db_entries_count[index]].id_mod=-1;
					db_entries[index][db_entries_count[index]].downloaded=-1;		
					db_entries[index][db_entries_count[index]].rating=-1;
					db_entries[index][db_entries_count[index]].playcount=-1;
					db_entries_count[index]++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);		
}
-(void) fillKeysWithDB_albumORfilename:(int)authorID {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
	int index,previndex;
	NSRange r;
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		if (search_db_nb_entries) {
			search_db_nb_entries=0;
			free(search_db_entries_data);
		}
		search_db_entries_data=(t_db_browse_entry*)malloc(db_nb_entries*sizeof(t_db_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_db_entries_count[i]=0;
			if (db_entries_count[i]) search_db_entries[i]=&(search_db_entries_data[search_db_nb_entries]);
			for (int j=0;j<db_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [db_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_db_entries[i][search_db_entries_count[i]].label=db_entries[i][j].label;
					search_db_entries[i][search_db_entries_count[i]].id_author=authorID;
					search_db_entries[i][search_db_entries_count[i]].downloaded=db_entries[i][j].downloaded;
					search_db_entries[i][search_db_entries_count[i]].rating=db_entries[i][j].rating;
					search_db_entries[i][search_db_entries_count[i]].playcount=db_entries[i][j].playcount;
					
					search_db_entries[i][search_db_entries_count[i]].id_type=-1;
					search_db_entries[i][search_db_entries_count[i]].id_album=db_entries[i][j].id_album;
					search_db_entries[i][search_db_entries_count[i]].id_mod=db_entries[i][j].id_mod;
					search_db_entries[i][search_db_entries_count[i]].filesize=db_entries[i][j].filesize;
					
					if (db_entries[i][j].id_mod>0) search_db_hasFiles++;
					
					search_db_entries_count[i]++;
					search_db_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (db_nb_entries) {
		for (int i=0;i<db_nb_entries;i++) {
			[db_entries_data[i].label release];
		}
		free(db_entries_data);db_entries_data=NULL;
		db_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT count(1),0 FROM mod_file WHERE id_author=%d AND id_album is null AND filename like \"%%%s%%\" \
							 UNION SELECT count(1),1 FROM mod_author_album m,mod_album a WHERE m.id_author=%d AND m.id_album=a.id AND a.album like \"%%%s%%\"",authorID,[mSearchText UTF8String],authorID,[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT count(1),0 FROM mod_file WHERE id_author=%d AND id_album is null \
					 UNION SELECT count(1),1 FROM mod_author_album m,mod_album a WHERE m.id_author=%d AND m.id_album=a.id",authorID,authorID);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		db_nb_entries=0;
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)malloc(db_nb_entries*sizeof(t_db_browse_entry));
			db_entries_index=0;
			for (int i=0;i<27;i++) {
				db_entries_count[i]=0;
				db_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT filename,filesize,id,0 FROM mod_file \
								 WHERE id_author=%d AND id_album is null AND filename like \"%%%s%%\" \
								 UNION SELECT a.album,a.num_files,a.id,1 FROM mod_author_album m,mod_album a \
								 WHERE m.id_author=%d AND m.id_album=a.id AND a.album like \"%%%s%%\" \
								 ORDER BY 1  COLLATE NOCASE",authorID,[mSearchText UTF8String],authorID,[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT filename,filesize,id,0 FROM mod_file \
						 WHERE id_author=%d AND id_album is null \
						 UNION SELECT a.album,a.num_files,a.id,1 FROM mod_author_album m,mod_album a \
						 WHERE m.id_author=%d AND m.id_album=a.id \
						 ORDER BY 1 COLLATE NOCASE",authorID,authorID);
			
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					int is_album=sqlite3_column_int(stmt, 3);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else db_entries[index]=&(db_entries_data[db_entries_index]);
					}					
					db_entries[index][db_entries_count[index]].id_author=authorID;
					db_entries[index][db_entries_count[index]].id_type=-1;
					if (is_album) {
						db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						db_entries[index][db_entries_count[index]].id_album=sqlite3_column_int(stmt, 2);
						db_entries[index][db_entries_count[index]].id_mod=-1;
					} else {
						db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						db_entries[index][db_entries_count[index]].id_album=-1;
						db_entries[index][db_entries_count[index]].id_mod=sqlite3_column_int(stmt, 2);						
						db_hasFiles++;
					}
					db_entries[index][db_entries_count[index]].filesize=sqlite3_column_int(stmt, 1);
					
					db_entries[index][db_entries_count[index]].downloaded=-1;
					db_entries[index][db_entries_count[index]].rating=-1;
					db_entries[index][db_entries_count[index]].playcount=-1;
					db_entries_count[index]++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_albumORfilename:(int)filetypeID fileAuthorID:(int)authorID {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
	int index,previndex;
	NSRange r;
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		if (search_db_nb_entries) {
			search_db_nb_entries=0;
			free(search_db_entries_data);
		}
		search_db_entries_data=(t_db_browse_entry*)malloc(db_nb_entries*sizeof(t_db_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_db_entries_count[i]=0;
			if (db_entries_count[i]) search_db_entries[i]=&(search_db_entries_data[search_db_nb_entries]);
			for (int j=0;j<db_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [db_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_db_entries[i][search_db_entries_count[i]].label=db_entries[i][j].label;
					search_db_entries[i][search_db_entries_count[i]].id_author=authorID;
					search_db_entries[i][search_db_entries_count[i]].downloaded=db_entries[i][j].downloaded;
					search_db_entries[i][search_db_entries_count[i]].rating=db_entries[i][j].rating;
					search_db_entries[i][search_db_entries_count[i]].playcount=db_entries[i][j].playcount;
					
					search_db_entries[i][search_db_entries_count[i]].id_type=filetypeID;
					search_db_entries[i][search_db_entries_count[i]].id_album=db_entries[i][j].id_album;
					search_db_entries[i][search_db_entries_count[i]].id_mod=db_entries[i][j].id_mod;
					search_db_entries[i][search_db_entries_count[i]].filesize=db_entries[i][j].filesize;
					if (db_entries[i][j].id_mod>0) search_db_hasFiles++;
					search_db_entries_count[i]++;
					search_db_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (db_nb_entries) {
		for (int i=0;i<db_nb_entries;i++) {
			[db_entries_data[i].label release];
		}
		free(db_entries_data);db_entries_data=NULL;
		db_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT count(1),0 FROM mod_file \
							 WHERE id_author=%d AND id_type=%d id_album is null AND filename like \"%%%s%%\" \
							 UNION SELECT count(1),1 FROM mod_type_author_album m,mod_album a \
							 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id AND a.album like \"%%%s%%\"",authorID,filetypeID,[mSearchText UTF8String],authorID,filetypeID,[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT count(1),0 FROM mod_file \
					 WHERE id_author=%d AND id_type=%d AND id_album is null \
					 UNION SELECT count(1),1 FROM mod_type_author_album m,mod_album a \
					 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id",authorID,filetypeID,authorID,filetypeID);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		db_nb_entries=0;
        
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)malloc(db_nb_entries*sizeof(t_db_browse_entry));
			db_entries_index=0;
			for (int i=0;i<27;i++) {
				db_entries_count[i]=0;
				db_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT filename,filesize,id,0 FROM mod_file \
								 WHERE id_author=%d AND id_type=%d AND id_album is null AND filename like \"%%%s%%\" \
								 UNION SELECT a.album,m.num_files,a.id,1 FROM mod_type_author_album m,mod_album a \
								 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id AND a.album like \"%%%s%%\" \
								 ORDER BY 1  COLLATE NOCASE",authorID,filetypeID,[mSearchText UTF8String],authorID,filetypeID,[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT filename,filesize,id,0 FROM mod_file \
						 WHERE id_author=%d AND id_type=%d AND id_album is null \
						 UNION SELECT a.album,m.num_files,a.id,1 FROM mod_type_author_album m,mod_album a \
						 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id \
						 ORDER BY 1 COLLATE NOCASE",authorID,filetypeID,authorID,filetypeID);
            
            
            
            
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					int is_album=sqlite3_column_int(stmt, 3);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
							index=previndex;
						} else db_entries[index]=&(db_entries_data[db_entries_index]);
					}
					db_entries[index][db_entries_count[index]].id_author=authorID;
					db_entries[index][db_entries_count[index]].id_type=filetypeID;
					if (is_album) {
						db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						db_entries[index][db_entries_count[index]].id_album=sqlite3_column_int(stmt, 2);
						db_entries[index][db_entries_count[index]].id_mod=-1;
					} else {
						db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						db_entries[index][db_entries_count[index]].id_album=-1;
						db_entries[index][db_entries_count[index]].id_mod=sqlite3_column_int(stmt, 2);
						db_hasFiles++;
					}
					db_entries[index][db_entries_count[index]].filesize=sqlite3_column_int(stmt, 1);
					db_entries[index][db_entries_count[index]].downloaded=-1;
					db_entries[index][db_entries_count[index]].rating=-1;
					db_entries[index][db_entries_count[index]].playcount=-1;
					db_entries_count[index]++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_filename:(int)authorID fileAlbumID:(int)albumID {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
	int index,previndex;
	NSRange r;
	
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		if (search_db_nb_entries) {
			search_db_nb_entries=0;
			free(search_db_entries_data);
		}
		search_db_entries_data=(t_db_browse_entry*)malloc(db_nb_entries*sizeof(t_db_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_db_entries_count[i]=0;
			if (db_entries_count[i]) search_db_entries[i]=&(search_db_entries_data[search_db_nb_entries]);
			for (int j=0;j<db_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [db_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_db_entries[i][search_db_entries_count[i]].label=db_entries[i][j].label;
					search_db_entries[i][search_db_entries_count[i]].id_author=authorID;
					search_db_entries[i][search_db_entries_count[i]].downloaded=db_entries[i][j].downloaded;
					search_db_entries[i][search_db_entries_count[i]].rating=db_entries[i][j].rating;
					search_db_entries[i][search_db_entries_count[i]].playcount=db_entries[i][j].playcount;
					
					search_db_entries[i][search_db_entries_count[i]].id_type=-1;
					search_db_entries[i][search_db_entries_count[i]].id_album=albumID;
					search_db_entries[i][search_db_entries_count[i]].id_mod=db_entries[i][j].id_mod;
					search_db_entries[i][search_db_entries_count[i]].filesize=db_entries[i][j].filesize;
					if (db_entries[i][j].id_mod>0) search_db_hasFiles++;
					search_db_entries_count[i]++;
					search_db_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (db_nb_entries) {
		for (int i=0;i<db_nb_entries;i++) {
			[db_entries_data[i].label release];
		}
		free(db_entries_data);db_entries_data=NULL;
		db_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT count(1) FROM mod_file WHERE id_author=%d AND id_album=%d AND filename LIKE \"%%%s%%\" ORDER BY filename",authorID,albumID,[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT count(1) FROM mod_file WHERE id_author=%d AND id_album=%d ORDER BY filename",authorID,albumID);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)malloc(db_nb_entries*sizeof(t_db_browse_entry));
			db_entries_index=0;
			for (int i=0;i<27;i++) {
				db_entries_count[i]=0;
				db_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT filename,filesize,id FROM mod_file WHERE id_author=%d AND id_album=%d AND filename LIKE \"%%%s%%\" ORDER BY filename COLLATE NOCASE",authorID,albumID,[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT filename,filesize,id FROM mod_file WHERE id_author=%d AND id_album=%d ORDER BY filename COLLATE NOCASE",authorID,albumID);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else db_entries[index]=&(db_entries_data[db_entries_index]);
					}
					db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[index][db_entries_count[index]].id_author=authorID;
					db_entries[index][db_entries_count[index]].id_type=-1;
					db_entries[index][db_entries_count[index]].id_album=albumID;
					db_entries[index][db_entries_count[index]].id_mod=sqlite3_column_int(stmt,2);
					db_entries[index][db_entries_count[index]].filesize=sqlite3_column_int(stmt,1);
					db_entries[index][db_entries_count[index]].downloaded=-1;					
					db_entries[index][db_entries_count[index]].rating=-1;
					db_entries[index][db_entries_count[index]].playcount=-1;
					db_hasFiles++;
					db_entries_count[index]++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_filename:(int)filetypeID fileAuthorID:(int)authorID fileAlbumID:(int)albumID {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
	int index,previndex;
	NSRange r;
	
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		if (search_db_nb_entries) {
			search_db_nb_entries=0;
			free(search_db_entries_data);
		}
		search_db_entries_data=(t_db_browse_entry*)malloc(db_nb_entries*sizeof(t_db_browse_entry));
		
		for (int i=0;i<27;i++) {
			search_db_entries_count[i]=0;
			if (db_entries_count[i]) search_db_entries[i]=&(search_db_entries_data[search_db_nb_entries]);
			for (int j=0;j<db_entries_count[i];j++)  {
				r.location=NSNotFound;
				r = [db_entries[i][j].label rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
				if  ((r.location!=NSNotFound)||([mSearchText length]==0)) {
					search_db_entries[i][search_db_entries_count[i]].label=db_entries[i][j].label;
					search_db_entries[i][search_db_entries_count[i]].id_author=authorID;
					search_db_entries[i][search_db_entries_count[i]].downloaded=db_entries[i][j].downloaded;
					search_db_entries[i][search_db_entries_count[i]].rating=db_entries[i][j].rating;
					search_db_entries[i][search_db_entries_count[i]].playcount=db_entries[i][j].playcount;
					
					search_db_entries[i][search_db_entries_count[i]].id_type=filetypeID;
					search_db_entries[i][search_db_entries_count[i]].id_album=albumID;
					search_db_entries[i][search_db_entries_count[i]].id_mod=db_entries[i][j].id_mod;
					search_db_entries[i][search_db_entries_count[i]].filesize=db_entries[i][j].filesize;
					if (db_entries[i][j].id_mod>0) search_db_hasFiles++;
					search_db_entries_count[i]++;
					search_db_nb_entries++;
				}
			}
		}
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (db_nb_entries) {
		for (int i=0;i<db_nb_entries;i++) {
			[db_entries_data[i].label release];
		}
		free(db_entries_data);db_entries_data=NULL;
		db_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		//1st : count how many entries we'll have		
		if (mSearch) sprintf(sqlStatement,"SELECT count(1) FROM mod_file WHERE id_type=%d AND id_author=%d AND id_album=%d AND filename like \"%%%s%%\"",filetypeID,authorID,albumID,[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT count(1) FROM mod_file WHERE id_type=%d AND id_author=%d AND id_album=%d",filetypeID,authorID,albumID);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		db_nb_entries=0;
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)malloc(db_nb_entries*sizeof(t_db_browse_entry));
			db_entries_index=0;
			for (int i=0;i<27;i++) {
				db_entries_count[i]=0;
				db_entries[i]=NULL;
			}
			//3rd get the entries
			if (mSearch) sprintf(sqlStatement,"SELECT filename,filesize,id FROM mod_file \
								 WHERE id_type=%d AND id_author=%d AND id_album=%d AND filename like \"%%%s%%\" ORDER BY 1 COLLATE NOCASE",filetypeID,authorID,albumID,[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT filename,filesize,id FROM mod_file \
						 WHERE id_type=%d AND id_author=%d AND id_album=%d ORDER BY 1 COLLATE NOCASE",filetypeID,authorID,albumID);
			
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				index=-1;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					previndex=index;
					index=0;
					if ((str[0]>='A')&&(str[0]<='Z') ) index=(str[0]-'A'+1);
					if ((str[0]>='a')&&(str[0]<='z') ) index=(str[0]-'a'+1);
					
					//sections are determined 'on the fly' since result set is already sorted
					if (previndex!=index) {
						if (previndex>index) {
							NSLog(@"********* %s",str);
						} else db_entries[index]=&(db_entries_data[db_entries_index]);
					}
					db_entries[index][db_entries_count[index]].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[index][db_entries_count[index]].id_author=authorID;
					db_entries[index][db_entries_count[index]].id_type=filetypeID;
					db_entries[index][db_entries_count[index]].id_album=albumID;
					db_entries[index][db_entries_count[index]].id_mod=sqlite3_column_int(stmt, 2);
					db_entries[index][db_entries_count[index]].filesize=sqlite3_column_int(stmt, 1);
					db_entries[index][db_entries_count[index]].downloaded=-1;
					db_entries[index][db_entries_count[index]].rating=-1;
					db_entries[index][db_entries_count[index]].playcount=-1;
					db_hasFiles++;
					db_entries_count[index]++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else NSLog(@"ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	
	
	
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

-(void) loadPlayListsListFromDB:(NSMutableArray*)entries list_id:(NSMutableArray*)list_id {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;		
		
		sprintf(sqlStatement,"SELECT id,name,num_files FROM playlists ORDER BY name");
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				[entries addObject:[NSString stringWithFormat:@"%s (%d)",sqlite3_column_text(stmt, 1),sqlite3_column_int(stmt, 2)]];
				[list_id addObject:[NSString stringWithFormat:@"%d",sqlite3_column_int(stmt, 0)]];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) loadPlayListsFromDB:(NSString *)_id_playlist intoPlaylist:(t_playlist *)_playlist  {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	
	pthread_mutex_lock(&db_mutex);
	_playlist->nb_entries=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
		
		//Get playlist name
		sprintf(sqlStatement,"SELECT id,name,num_files FROM playlists WHERE id=%s",[_id_playlist UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				_playlist->playlist_id=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
				_playlist->playlist_name=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
		sprintf(sqlStatement,"SELECT p.name,p.fullpath,s.rating,s.play_count FROM playlists_entries p \
				LEFT OUTER JOIN user_stats s ON p.fullpath=s.fullpath \
				WHERE id_playlist=%s",[_id_playlist UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				_playlist->label[_playlist->nb_entries]=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
				_playlist->fullpath[_playlist->nb_entries]=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 1)];
				signed char tmpsc=(signed char)sqlite3_column_int(stmt, 2);
				if (tmpsc<0) tmpsc=0;
				if (tmpsc>5) tmpsc=5;
				_playlist->ratings[_playlist->nb_entries]=tmpsc;
				_playlist->playcounts[_playlist->nb_entries]=(short int)sqlite3_column_int(stmt, 3);
				_playlist->nb_entries++;
				if (_playlist->nb_entries==MAX_PL_ENTRIES) break;
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(NSString *) initNewPlaylistDB:(NSString *)listName {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	NSString *id_playlist;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		int err;		
		
		sprintf(sqlStatement,"INSERT INTO playlists (name,num_files) SELECT \"%s\",0",[listName UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
		
		//Get id
		id_playlist=[[NSString alloc] initWithFormat:@"%d",sqlite3_last_insert_rowid(db) ];
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return id_playlist;
}
-(NSString *) getPlaylistNameDB:(NSString*)id_playlist {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	NSString *listName;
	sqlite3 *db;
	int err;	
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		
		
		//Get playlist name
		sprintf(sqlStatement,"SELECT name FROM playlists WHERE id=%s",[id_playlist UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				listName=[[NSString alloc] initWithFormat:@"%s",sqlite3_column_text(stmt, 0)];
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return listName;
}
-(bool) addToPlaylistDB:(NSString*)id_playlist label:(NSString *)label fullPath:(NSString *)fullPath {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	bool result;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
		
		sprintf(sqlStatement,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
				[id_playlist UTF8String],[label UTF8String],[fullPath UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
			result=TRUE;
		} else {
			result=FALSE;
			NSLog(@"ErrSQL : %d",err);
		}
		if (result) {
			sprintf(sqlStatement,"UPDATE playlists SET num_files=\
					(SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
					WHERE id=%s",
					[id_playlist UTF8String],[id_playlist UTF8String]);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
				result=TRUE;
			} else {
				result=FALSE;
				NSLog(@"ErrSQL : %d",err);
			}
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return result;
}
-(bool) addListToPlaylistDB {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	bool result;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
		for (int i=0;i<playlist->nb_entries;i++) {
			sprintf(sqlStatement,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
					[playlist->playlist_id UTF8String],[playlist->label[i] UTF8String],[playlist->fullpath[i] UTF8String]);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
				result=TRUE;
			} else {
				result=FALSE;
				NSLog(@"ErrSQL : %d",err);
				break;
			}
		}
		if (result) {
			sprintf(sqlStatement,"UPDATE playlists SET num_files=\
					(SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
					WHERE id=%s",
					[playlist->playlist_id UTF8String],[playlist->playlist_id UTF8String]);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
				result=TRUE;
			} else {
				result=FALSE;
				NSLog(@"ErrSQL : %d",err);
			}
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return result;
}
-(bool) addListToPlaylistDB:(NSString*)id_playlist entries:(t_plPlaylist_entry*)pl_entries nb_entries:(int)nb_entries  {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	bool result;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[4096];
        char sqlTmp[1024];
		int cur_sqllen;
        sqlStatement[0]=0;
        cur_sqllen=0;
		for (int i=0;i<nb_entries;i++) {
			sprintf(sqlTmp,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
					[id_playlist UTF8String],[pl_entries[i].mPlaylistFilename UTF8String],[pl_entries[i].mPlaylistFilepath UTF8String]);
            if (strlen(sqlTmp)+cur_sqllen+1<4096) {
                if (cur_sqllen==0) strcpy(sqlStatement,sqlTmp);
                else sprintf(sqlStatement,"%s;%s",sqlStatement,sqlTmp);
                cur_sqllen+=strlen(sqlTmp)+1;
            } else {
                err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
                if (err==SQLITE_OK){
                    result=TRUE;
                } else {
                    result=FALSE;
                    NSLog(@"ErrSQL : %d",err);
                    break;
                }
                strcpy(sqlStatement,sqlTmp);
                cur_sqllen=strlen(sqlTmp);
            }
		}        
		if (result) {
            if (sqlStatement[0]) {
                err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
                if (err==SQLITE_OK){
                    result=TRUE;
                } else {
                    result=FALSE;
                    NSLog(@"ErrSQL : %d",err);                
                }
            }
            
            if (result) {
            
			sprintf(sqlStatement,"UPDATE playlists SET num_files=\
					(SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
					WHERE id=%s",
					[id_playlist UTF8String],[id_playlist UTF8String]);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
				result=TRUE;
			} else {
				result=FALSE;
				NSLog(@"ErrSQL : %d",err);
			}
            }
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return result;
}
-(bool) removeFromPlaylistDB:(NSString*)id_playlist fullPath:(NSString*)fullpath {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;
	bool result;
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
		
		sprintf(sqlStatement,"DELETE FROM playlists_entries WHERE id_playlist=\"%s\" AND fullpath=\"%s\"",
				[id_playlist UTF8String],[fullpath UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
			result=TRUE;
		} else {
			NSLog(@"ErrSQL : %d",err);
			result=FALSE;
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return result;
}
-(bool) replacePlaylistDBwithCurrent {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;	
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
		sprintf(sqlStatement,"DELETE FROM playlists_entries WHERE id_playlist=%s",
				[playlist->playlist_id UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
		
		
		for (int i=0;i<playlist->nb_entries;i++) {
			sprintf(sqlStatement,"INSERT INTO playlists_entries (id_playlist,name,fullpath) SELECT %s,\"%s\",\"%s\"",
					[playlist->playlist_id UTF8String],[playlist->label[i] UTF8String],[playlist->fullpath[i] UTF8String]);
			err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
			if (err==SQLITE_OK){
			} else NSLog(@"ErrSQL : %d",err);
		}
		
		sprintf(sqlStatement,"UPDATE playlists SET num_files=\
				(SELECT COUNT(1) FROM playlists_entries e WHERE playlists.id=e.id_playlist AND playlists.id=%s)\
				WHERE id=%s",
				[playlist->playlist_id UTF8String],[playlist->playlist_id UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return TRUE;
}
-(void) updatePlaylistNameDB:(NSString*)id_playlist playlist_name:(NSString *)playlist_name {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;	
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
		sprintf(sqlStatement,"UPDATE playlists SET name=\"%s\" WHERE id=%s",[playlist_name UTF8String],[id_playlist UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else NSLog(@"ErrSQL : %d",err);
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) getFileStatsDB:(NSString *)name fullpath:(NSString *)fullpath playcount:(short int*)playcount rating:(signed char*)rating{
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;	
	
	if (playcount) *playcount=0;
	if (rating) *rating=0;
	
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		
		
		//Get playlist name
		sprintf(sqlStatement,"SELECT play_count,rating FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[name UTF8String],[fullpath UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				if (playcount) *playcount=(short int)sqlite3_column_int(stmt, 0);
				if (rating) {
					*rating=(signed char)sqlite3_column_int(stmt, 1);
					if (*rating<0) *rating=0;
					if (*rating>5) *rating=5;
				}
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) getFileStatsDB:(NSString *)name fullpath:(NSString *)fullpath playcount:(short int*)playcount rating:(signed char*)rating song_length:(int*)song_length songs:(int*)songs channels_nb:(char*)channels_nb {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err;	
	
	if (playcount) *playcount=0;
	if (rating) *rating=0;
	if (song_length) *song_length=0;
	if (songs) *songs=0;
	if (channels_nb) *channels_nb=0;
	
	pthread_mutex_lock(&db_mutex);
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		
		
		//Get playlist name
		sprintf(sqlStatement,"SELECT play_count,rating,length,songs,channels FROM user_stats WHERE name=\"%s\" and fullpath=\"%s\"",[name UTF8String],[fullpath UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				if (playcount) *playcount=(short int)sqlite3_column_int(stmt, 0);
				if (rating) {
					*rating=(signed char)sqlite3_column_int(stmt, 1);
					if (*rating<0) *rating=0;
					if (*rating>5) *rating=5;
				}
				if (song_length) *song_length=(int)sqlite3_column_int(stmt, 2);				
				if (songs) *songs=(int)sqlite3_column_int(stmt, 3);
				if (channels_nb) *channels_nb=(char)sqlite3_column_int(stmt, 4);
			}
			sqlite3_finalize(stmt);
		} else NSLog(@"ErrSQL : %d",err);
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(int) deletePlaylistDB:(NSString*)id_playlist {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err,ret;	
	pthread_mutex_lock(&db_mutex);
	ret=1;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
		sprintf(sqlStatement,"DELETE FROM playlists_entries WHERE id_playlist=%s",[id_playlist UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else {ret=0;NSLog(@"ErrSQL : %d",err);}
		
		sprintf(sqlStatement,"DELETE FROM playlists WHERE id=%s",[id_playlist UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else {ret=0;NSLog(@"ErrSQL : %d",err);}
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return ret;
}

-(int) deleteStatsFileDB:(NSString*)fullpath {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err,ret;	
	pthread_mutex_lock(&db_mutex);
	ret=1;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
		sprintf(sqlStatement,"DELETE FROM user_stats WHERE fullpath=\"%s\"",[fullpath UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else {ret=0;NSLog(@"ErrSQL : %d",err);}
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return ret;
}
-(int) deleteStatsDirDB:(NSString*)fullpath {
	NSString *pathToDB=[NSString stringWithFormat:@"%@/%@",[NSHomeDirectory() stringByAppendingPathComponent:  @"Documents"],DATABASENAME_USER];
	sqlite3 *db;
	int err,ret;	
	pthread_mutex_lock(&db_mutex);
	ret=1;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		
		sprintf(sqlStatement,"DELETE FROM user_stats WHERE fullpath like \"%s%%\"",[fullpath UTF8String]);
		err=sqlite3_exec(db, sqlStatement, NULL, NULL, NULL);
		if (err==SQLITE_OK){
		} else {ret=0;NSLog(@"ErrSQL : %d",err);}
		
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return ret;
}

//extern "C" char *mdx_make_sjis_to_syscharset(char* str);

-(void)listLocalFiles {
	NSString *file,*cpath;
	NSDirectoryEnumerator *dirEnum,*dirEnum2;
	NSDictionary *fileAttributes;
	NSArray *filetype_extMDX=[SUPPORTED_FILETYPE_MDX componentsSeparatedByString:@","];
    NSArray *filetype_extPMD=[SUPPORTED_FILETYPE_PMD componentsSeparatedByString:@","];
	NSArray *filetype_extSID=[SUPPORTED_FILETYPE_SID componentsSeparatedByString:@","];
	NSArray *filetype_extSTSOUND=[SUPPORTED_FILETYPE_STSOUND componentsSeparatedByString:@","];
	NSArray *filetype_extSC68=[SUPPORTED_FILETYPE_SC68 componentsSeparatedByString:@","];
	NSArray *filetype_extARCHIVE=[SUPPORTED_FILETYPE_ARCHIVE componentsSeparatedByString:@","];
	NSArray *filetype_extUADE=[SUPPORTED_FILETYPE_UADE componentsSeparatedByString:@","];
	NSArray *filetype_extMODPLUG=[SUPPORTED_FILETYPE_MODPLUG componentsSeparatedByString:@","];
    NSArray *filetype_extDUMB=[SUPPORTED_FILETYPE_DUMB componentsSeparatedByString:@","];
	NSArray *filetype_extGME=[SUPPORTED_FILETYPE_GME componentsSeparatedByString:@","];
	NSArray *filetype_extADPLUG=[SUPPORTED_FILETYPE_ADPLUG componentsSeparatedByString:@","];
	NSArray *filetype_extSEXYPSF=[SUPPORTED_FILETYPE_SEXYPSF componentsSeparatedByString:@","];
	NSArray *filetype_extAOSDK=[SUPPORTED_FILETYPE_AOSDK componentsSeparatedByString:@","];
	NSArray *filetype_extHVL=[SUPPORTED_FILETYPE_HVL componentsSeparatedByString:@","];
	NSArray *filetype_extGSF=[SUPPORTED_FILETYPE_GSF componentsSeparatedByString:@","];
	NSArray *filetype_extASAP=[SUPPORTED_FILETYPE_ASAP componentsSeparatedByString:@","];
	NSArray *filetype_extWMIDI=[SUPPORTED_FILETYPE_WMIDI componentsSeparatedByString:@","];    
	NSMutableArray *filetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extMDX count]+[filetype_extPMD count]+[filetype_extSID count]+[filetype_extSTSOUND count]+
								  [filetype_extSC68 count]+[filetype_extARCHIVE count]+[filetype_extUADE count]+[filetype_extMODPLUG count]+[filetype_extDUMB count]+
								  [filetype_extGME count]+[filetype_extADPLUG count]+[filetype_extSEXYPSF count]+
								  [filetype_extAOSDK count]+[filetype_extHVL count]+[filetype_extGSF count]+
								  [filetype_extASAP count]+[filetype_extWMIDI count]];
    NSArray *filetype_extARCHIVEFILE=[SUPPORTED_FILETYPE_ARCFILE componentsSeparatedByString:@","];
	NSMutableArray *archivetype_ext=[NSMutableArray arrayWithCapacity:[filetype_extARCHIVEFILE count]];
	NSArray *filetype_extGME_MULTISONGSFILE=[SUPPORTED_FILETYPE_GME_MULTISONGS componentsSeparatedByString:@","];
	NSMutableArray *gme_multisongstype_ext=[NSMutableArray arrayWithCapacity:[filetype_extGME_MULTISONGSFILE count]];
	
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
    [filetype_ext addObjectsFromArray:filetype_extDUMB];
	[filetype_ext addObjectsFromArray:filetype_extGME];
	[filetype_ext addObjectsFromArray:filetype_extADPLUG];
	[filetype_ext addObjectsFromArray:filetype_extSEXYPSF];
	[filetype_ext addObjectsFromArray:filetype_extAOSDK];
	[filetype_ext addObjectsFromArray:filetype_extHVL];
	[filetype_ext addObjectsFromArray:filetype_extGSF];
	[filetype_ext addObjectsFromArray:filetype_extASAP];
	[filetype_ext addObjectsFromArray:filetype_extWMIDI];
    
    [archivetype_ext addObjectsFromArray:filetype_extARCHIVEFILE];
    [gme_multisongstype_ext addObjectsFromArray:filetype_extGME_MULTISONGSFILE];
	
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
        }
    }
    
    if (browseType==2) { //GME Multisongs
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
                        if (gme_info->song[0]) file=[NSString stringWithFormat:@"%s",gme_info->song];
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
                        NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                        if (r.location != NSNotFound) {
                            /*if(r.location== 0)*/ filtered=0;
                        }
                    }
                    if (!filtered) {
                        
                        const char *str=[[file lastPathComponent] UTF8String];
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
                                if (gme_info->song[0]) file=[NSString stringWithFormat:@"%s",gme_info->song];
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
                                NSRange r = [[file lastPathComponent] rangeOfString:mSearchText options:NSCaseInsensitiveSearch];
                                if (r.location != NSNotFound) {
                                    /*if(r.location== 0)*/ filtered=0;
                                }
                            }
                            if (!filtered) {
                                
                                const char *str;
                                char tmp_str[1024];//,*tmp_convstr;
                                str=[[file lastPathComponent] UTF8String];
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
        fex_type_t type;
        fex_t* fex;
        const char *path=[cpath UTF8String];
        /* Determine file's type */
        if (fex_identify_file( &type, path)) {
            NSLog(@"fex cannot determine type of %s",path);
        }
        /* Only open files that fex can handle */
        if ( type != NULL ) {
            if (fex_open_type( &fex, path, type )) {
                NSLog(@"cannot fex open : %s / type : %d",path,type);
            } else {
                while ( !fex_done( fex ) ) {
                    file=[NSString stringWithFormat:@"%s",fex_name(fex)]; 
                    NSString *extension = [[file pathExtension] uppercaseString];
                    NSString *file_no_ext = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                    
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
                if (fex_open_type( &fex, path, type )) {
                    NSLog(@"cannot fex open : %s / type : %d",path,type);
                } else {
                    int arc_counter=0;
                    while ( !fex_done( fex ) ) {
                        file=[NSString stringWithFormat:@"%s",fex_name(fex)]; 
                        NSString *extension = [[file pathExtension] uppercaseString];
                        NSString *file_no_ext = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                        
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
        if (mShowSubdir) dirContent=[mFileMngr subpathsOfDirectoryAtPath:cpath error:&error];
        else dirContent=[mFileMngr contentsOfDirectoryAtPath:cpath error:&error];
        for (file in dirContent) {
            //check if dir
            rdir.location=NSNotFound;
            rdir = [file rangeOfString:@"." options:NSCaseInsensitiveSearch];
            if (rdir.location == NSNotFound) {  //assume it is a dir if no "." in file name
                rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                    if ([file compare:@"tmpArchive"]!=NSOrderedSame) {
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
                    NSString *extension = [[file pathExtension] uppercaseString];
                    NSString *file_no_ext = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                    
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
                    rdir.location=NSNotFound;
                    rdir = [file rangeOfString:@"." options:NSCaseInsensitiveSearch];
                    if (rdir.location == NSNotFound) {  //assume it is a dir if no "." in file name                    
                        rdir = [file rangeOfString:@"/" options:NSCaseInsensitiveSearch];
                        if ((rdir.location==NSNotFound)||(mShowSubdir)) {
                            if ([file compare:@"tmpArchive"]!=NSOrderedSame) {
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
                            NSString *extension = [[file pathExtension] uppercaseString];
                            NSString *file_no_ext = [[[file lastPathComponent] stringByDeletingPathExtension] uppercaseString];
                            
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
                                    else if ([gme_multisongstype_ext indexOfObject:extension]!=NSNotFound) local_entries[index][local_entries_count[index]].type=3;
                                    else if ([gme_multisongstype_ext indexOfObject:file_no_ext]!=NSNotFound) local_entries[index][local_entries_count[index]].type=3;
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
    
    if (newPlaylist) {//get name from modal view
        if (mValidatePlName) {
            mValidatePlName=0;
            
            if (childController == nil) childController = [[RootViewControllerIphone alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
            
            if (newPlaylist==1) {  //new blank playlist
                if (playlist->playlist_id) [playlist->playlist_id release];
                if (playlist->playlist_name) [playlist->playlist_name release];
                playlist->playlist_name=[[NSString alloc] initWithString:self.inputText.text];
                playlist->playlist_id=[self initNewPlaylistDB:playlist->playlist_name];
                self.navigationItem.title=playlist->playlist_name;
                
                ((RootViewControllerIphone*)childController)->show_playlist=0;
            }
            if (newPlaylist==2) {  //new playlist from current played list
                if (playlist->playlist_id) [playlist->playlist_id release];
                if (playlist->playlist_name) [playlist->playlist_name release];
                playlist->playlist_name=[[NSString alloc] initWithString:self.inputText.text];
                playlist->playlist_id=[self initNewPlaylistDB:playlist->playlist_name];
                t_plPlaylist_entry *pl_entries=(t_plPlaylist_entry*)(detailViewController.mPlaylist);
                playlist->nb_entries=0;
                for (int i=0;i<detailViewController.mPlaylist_size;i++) {
                    playlist->nb_entries++;
                    playlist->label[playlist->nb_entries-1]=[[NSString alloc] initWithFormat:@"%@",pl_entries[i].mPlaylistFilename];
                    playlist->fullpath[playlist->nb_entries-1]=[[NSString alloc] initWithFormat:@"%@",pl_entries[i].mPlaylistFilepath];
                }
                [self addListToPlaylistDB];
                
                ((RootViewControllerIphone*)childController)->show_playlist=1;
            }
            //set new title
            childController.title = playlist->playlist_name;
            
            // Set new directory
            ((RootViewControllerIphone*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerIphone*)childController)->browse_mode = BROWSE_PLAYLIST_MODE;
            ((RootViewControllerIphone*)childController)->detailViewController=detailViewController;
            ((RootViewControllerIphone*)childController)->downloadViewController=downloadViewController;
            ((RootViewControllerIphone*)childController)->playlist=playlist;
            
            ((RootViewControllerIphone*)childController)->textInputView=textInputView;
            ((RootViewControllerIphone*)childController)->inputText=inputText;
            ((RootViewControllerIphone*)childController)->playerButton=playerButton;
            [keys release];keys=nil;
            [list release];list=nil;
            mFreePlaylist=1;
            
            
            newPlaylist=0;		
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];
            mFreePlaylist=1;
        } else {  //cancel => no playlist created
            [self freePlaylist];
            mFreePlaylist=0;
        }
    }
    
    if (mValidatePlName&&mRenamePlaylist) {
        mRenamePlaylist=0;
        if (playlist->playlist_name) [playlist->playlist_name release];
        playlist->playlist_name=[[NSString alloc] initWithString:self.inputText.text];
        [self updatePlaylistNameDB:playlist->playlist_id playlist_name:playlist->playlist_name];
        self.navigationItem.title=[NSString stringWithFormat:@"%@ (%d)",playlist->playlist_name,playlist->nb_entries];
    }
    if (show_playlist) self.navigationItem.title=[NSString stringWithFormat:@"%@ (%d)",playlist->playlist_name,playlist->nb_entries];
    
    if (detailViewController.mShouldHaveFocus) {
        detailViewController.mShouldHaveFocus=0;
        [self.navigationController pushViewController:detailViewController animated:(mSlowDevice?NO:YES)];
    } else {				
        if (shouldFillKeys&&(browse_depth>0)&&
            ((browse_mode==BROWSE_LOCAL_MODE)||(browse_mode==BROWSE_MODLAND_MODE)||(browse_mode==BROWSE_HVSC_MODE)||
             (browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE))
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
    else if ( (browse_mode==BROWSE_MODLAND_MODE)||((browse_mode==BROWSE_HVSC_MODE)) ) {
        shouldFillKeys=1;
        [self fillKeys];
        [[super tableView] reloadData];
    }
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

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration {
    [[super tableView] reloadData];
}

// Ensure that the view controller supports rotation and that the split view can therefore show in both portrait and landscape.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    [[super tableView] reloadData];
    return YES;
}

-(int) isLocalEntryInPlaylist:(NSString*)filepath {
    int nb_occur=0;
    for (int i=0;i<playlist->nb_entries;i++) 
        if ([filepath compare:playlist->fullpath[i]]== NSOrderedSame ) nb_occur++;
    
    return nb_occur;
}

-(int) checkIsDownloadedMod:(int)id_mod {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSFileManager *fileManager = mFileMngr;
    BOOL success;
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *checkPath,*strFullPath;
    
    pthread_mutex_lock(&db_mutex);
    strFullPath=nil;
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        sprintf(sqlStatement,"SELECT localpath FROM mod_file WHERE id=%d",id_mod);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strFullPath=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    if (strFullPath) {
        checkPath = [documentsDirectory stringByAppendingPathComponent: [NSString stringWithFormat:@"%@/%@",MODLAND_BASEDIR,strFullPath]];
        success = [fileManager fileExistsAtPath:checkPath];
        if (success) return 1;
    }
    return 0;
}
-(int) checkIsDownloaded:(int)id_author {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSFileManager *fileManager = mFileMngr;
    BOOL success;
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *checkPath,*strAuthor;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        sprintf(sqlStatement,"SELECT author FROM mod_author WHERE id=%d",id_author);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAuthor=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    checkPath = [documentsDirectory stringByAppendingPathComponent: [NSString stringWithFormat:@"%@/%@",MODLAND_BASEDIR,strAuthor]];
    success = [fileManager fileExistsAtPath:checkPath];
    if (success) return 1;
    return 0;
}
-(int) checkIsDownloaded:(int)id_author id_type:(int)id_type{
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSFileManager *fileManager = mFileMngr;
    BOOL success;
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *checkPath,*strType,*strAuthor;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        sprintf(sqlStatement,"SELECT author FROM mod_author WHERE id=%d",id_author);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAuthor=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        sprintf(sqlStatement,"SELECT filetype FROM mod_type WHERE id=%d",id_type);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strType=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    checkPath = [documentsDirectory stringByAppendingPathComponent: [NSString stringWithFormat:@"%@/%@/%@",MODLAND_BASEDIR,strAuthor,strType]];
    success = [fileManager fileExistsAtPath:checkPath];
    if (success) return 1;
    return 0;
}
-(int) checkIsDownloaded:(int)id_author id_type:(int)id_type id_album:(int)id_album {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSFileManager *fileManager = mFileMngr;
    BOOL success;
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *checkPath,*strType,*strAuthor,*strAlbum;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        sprintf(sqlStatement,"SELECT author FROM mod_author WHERE id=%d",id_author);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAuthor=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        sprintf(sqlStatement,"SELECT filetype FROM mod_type WHERE id=%d",id_type);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strType=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        sprintf(sqlStatement,"SELECT album FROM mod_album WHERE id=%d",id_album);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAlbum=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    checkPath = [documentsDirectory stringByAppendingPathComponent: [NSString stringWithFormat:@"%@/%@/%@/%@",MODLAND_BASEDIR,strAuthor,strType,strAlbum]];
    success = [fileManager fileExistsAtPath:checkPath];
    if (success) return 1;
    return 0;
}
-(int) checkIsDownloadedNoAuthor:(int)id_type{
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSFileManager *fileManager = mFileMngr;
    BOOL success;
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *checkPath,*strType,*strAuthor;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        sprintf(sqlStatement,"SELECT filetype FROM mod_type WHERE id=%d",id_type);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strType=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
        sprintf(sqlStatement,"SELECT a.author FROM mod_type_author m,mod_author a WHERE m.id_type=%d AND m.id_author=a.id",id_type);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAuthor=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
                checkPath = [documentsDirectory stringByAppendingPathComponent: [NSString stringWithFormat:@"%@/%@/%@",MODLAND_BASEDIR,strAuthor,strType]];
                success = [fileManager fileExistsAtPath:checkPath];
                if (success) break;
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    if (success) return 1;
    return 0;
}
-(int) checkIsDownloadedNoAuthor:(int)id_type id_album:(int)id_album {
    NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
    sqlite3 *db;
    NSFileManager *fileManager = mFileMngr;
    BOOL success;
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *checkPath,*strType,*strAuthor,*strAlbum;
    
    pthread_mutex_lock(&db_mutex);
    
    if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
        char sqlStatement[1024];
        sqlite3_stmt *stmt;
        int err;
        
        sprintf(sqlStatement,"SELECT filetype FROM mod_type WHERE id=%d",id_type);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strType=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        sprintf(sqlStatement,"SELECT album FROM mod_album WHERE id=%d",id_album);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAlbum=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
        
        sprintf(sqlStatement,"SELECT a.author FROM mod_type_author_album m,mod_author a WHERE m.id_type=%d AND m.id_album=%d m.id_author=a.id",id_type,id_album);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAuthor=[NSString stringWithFormat:@"%s",(char*)sqlite3_column_text(stmt, 0)];
                checkPath = [documentsDirectory stringByAppendingPathComponent: [NSString stringWithFormat:@"%@/%@/%@/%@",MODLAND_BASEDIR,strAuthor,strType,strAlbum]];
                success = [fileManager fileExistsAtPath:checkPath];
                if (success) break;
            }
            sqlite3_finalize(stmt);
        } else NSLog(@"ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    if (success) return 1;
    return 0;
}


#pragma mark -
#pragma mark Table view data source

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (browse_depth==0) return nil;
    if (mSearch) return nil;	
    if (show_playlist) return nil;
    if ((browse_mode==BROWSE_PLAYLIST_MODE)&&(browse_depth>=2)&&(show_playlist==0)) {
        if (browse_depth<=1) return nil;
        if (show_playlist) return nil;
        if (section==0) return nil;
        if (section==1) return @"";
        if ((search_local?search_local_entries_count[section-2]:local_entries_count[section-2])) return [indexTitlesDownload objectAtIndex:section];
        return nil;
    }
    if ((browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)||(browse_mode==BROWSE_WEB_MODE)||(browse_mode==BROWSE_WORLDCHARTS_MODE)||(browse_mode==BROWSE_USERGUIDE_MODE)) return nil;
    if (browse_mode==BROWSE_MODLAND_MODE) {
        if (browse_depth==1) return nil; //Modland browse mode chooser
        if (section==0) return nil;
        else {
            int file_or_album=0;
            if (search_db) {
                if (search_db_hasFiles) file_or_album=1;
            } else {
                if (db_hasFiles) file_or_album=1;
            }
            
            if (file_or_album&&(section==1)) return @"";
            if ((search_db?search_db_entries_count[section-1-file_or_album]:db_entries_count[section-1-file_or_album])) {
                if (file_or_album) return [indexTitlesDownload objectAtIndex:section];
                else return [indexTitles objectAtIndex:section];
            }
            return nil;
        }
    }
    if (browse_mode==BROWSE_LOCAL_MODE) {
        int switch_view_subdir=(browse_depth>=SHOW_SUDIR_MIN_LEVEL?1:0);		
        
        if (section==0) return nil;
        if ((section==1)&&switch_view_subdir) return @"";
        if ((search_local?search_local_entries_count[section-1-switch_view_subdir]:local_entries_count[section-1-switch_view_subdir])) {
            if (switch_view_subdir) return [indexTitlesDownload objectAtIndex:section];	
            return [indexTitles objectAtIndex:section];
        } else return nil;
    }
    if (browse_mode==BROWSE_HVSC_MODE) {
        if (section==0) return 0;
        //Check if "Get all entries" has to be displayed
        if (search_dbHVSC) {
            if (search_dbHVSC_hasFiles) {
                if (section==1) return @"";
                if (search_dbHVSC_entries_count[section-2]) return [indexTitlesDownload objectAtIndex:section];
                return nil;
            }
            if (search_dbHVSC_entries_count[section-1]) return [indexTitles objectAtIndex:section];
            return nil;
        } else {
            if (dbHVSC_hasFiles) {
                if (section==1) return @"";
                if (dbHVSC_entries_count[section-2]) return [indexTitlesDownload objectAtIndex:section];
                return nil;
            }
            if (dbHVSC_entries_count[section-1]) return [indexTitles objectAtIndex:section];
            return nil;
        }
        return nil;
    }
    if (browse_depth>=2) return [indexTitles objectAtIndex:section];
    return nil;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    local_flag=0;
    
    if (browse_depth==0) return [keys count];
    if (browse_mode==BROWSE_PLAYLIST_MODE) {
        if (browse_depth==1) return [keys count];
        if (show_playlist) return 1;
        if ((show_playlist==0)&&(browse_depth>1)) return 28+1;
    }
    if (browse_mode==BROWSE_MODLAND_MODE) {
        if (browse_depth==1) return 1; //Modland browse mode chooser
        if (browse_depth>1) {
            int file_or_album=0;
            if (search_db) {
                if (search_db_hasFiles) file_or_album=1;
            } else {
                if (db_hasFiles) file_or_album=1;
            }
            return 28+file_or_album;
        }
    }
    if (browse_mode==BROWSE_HVSC_MODE) {
        //Check if "Get all entries" has to be displayed
        if (search_dbHVSC) {
            if (search_dbHVSC_hasFiles) return 28+1;
            return 28;
        } else {
            if (dbHVSC_hasFiles) return 28+1;
            return 28;
        }
    }
    if (browse_mode==BROWSE_LOCAL_MODE) {
        int switch_view_subdir=(browse_depth>=SHOW_SUDIR_MIN_LEVEL?1:0);
        if (switch_view_subdir) return 28+1;
    }
    return 28;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (browse_mode==BROWSE_PLAYLIST_MODE) { //playlist
        if (browse_depth<=1) {
            NSDictionary *dictionary = [keys objectAtIndex:section];
            NSArray *array = [dictionary objectForKey:@"entries"];
            return [array count];
        }
        if (show_playlist) return (playlist->nb_entries+2);
        if (section==0) return 0;
        if (section==1) return 1;
        return (search_local?search_local_entries_count[section-2]:local_entries_count[section-2]);
    } else if ((browse_mode==BROWSE_LOCAL_MODE)&&(browse_depth>=1)) {//local browser
        int switch_view_subdir=(browse_depth>=SHOW_SUDIR_MIN_LEVEL?1:0);
        if (section==0) return 0;
        if ((section==1)&&switch_view_subdir) return 1;
        return (search_local?search_local_entries_count[section-1-switch_view_subdir]:local_entries_count[section-1-switch_view_subdir]);
    } else if ((browse_mode==BROWSE_MODLAND_MODE)&&(browse_depth>=1)) {//modland DB
        if (browse_depth==1) return 3; //Modland browse mode chooser
        if (section==0) return 0;
        else {
            int file_or_album=0;
            if (search_db) {
                if (search_db_hasFiles) file_or_album=1;
            } else {
                if (db_hasFiles) file_or_album=1;
            }
            
            if (file_or_album&&(section==1)) return 1;
            return (search_db?search_db_entries_count[section-1-file_or_album]:db_entries_count[section-1-file_or_album]);
        }
    } else if (browse_mode==BROWSE_RATED_MODE) {//favorites / rating
        if (section==0) return 0;
        if (section==1) return 1;        
        return (search_local?search_local_entries_count[section-2]:local_entries_count[section-2]);
    } else if (browse_mode==BROWSE_MOSTPLAYED_MODE) {//favorites / play count
        if (section==0) return 0;
        if (section==1) return 1;
        return (search_local?search_local_entries_count[section-2]:local_entries_count[section-2]);
    }  else if (browse_mode==BROWSE_HVSC_MODE) {//HVSC
        if (section==0) return 0;
        //Check if "Get all entries" has to be displayed
        if (search_dbHVSC) {
            if (search_dbHVSC_hasFiles) {
                if (section==1) return 1;
                return search_dbHVSC_entries_count[section-2];
            }
            return search_dbHVSC_entries_count[section-1];
        } else {
            if (dbHVSC_hasFiles) {
                if (section==1) return 1;
                return dbHVSC_entries_count[section-2];
            }
            return dbHVSC_entries_count[section-1];
        }
    } else {
        NSDictionary *dictionary = [keys objectAtIndex:section];
        NSArray *array = [dictionary objectForKey:@"entries"];
        return [array count];
    }
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    if (browse_depth==0) return nil;
    if (mSearch) return nil;	
    if (show_playlist) return nil;
    if ((browse_mode==BROWSE_PLAYLIST_MODE)&&(browse_depth>=2)&&(show_playlist==0)) return indexTitlesDownload;
    if ((browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)||(browse_mode==BROWSE_WEB_MODE)||(browse_mode==BROWSE_WORLDCHARTS_MODE)||(browse_mode==BROWSE_USERGUIDE_MODE)) return nil;
    if (browse_mode==BROWSE_MODLAND_MODE) {
        if (browse_depth==1) return nil; //Modland browse mode chooser
        if (browse_depth>1) {
            int file_or_album=0;
            if (search_db) {
                if (search_db_hasFiles) file_or_album=1;
            } else {
                if (db_hasFiles) file_or_album=1;
            }
            
            if (file_or_album) return indexTitlesDownload;
        }
        return indexTitles;
    }
    if (browse_mode==BROWSE_LOCAL_MODE) {
        int switch_view_subdir=(browse_depth>=SHOW_SUDIR_MIN_LEVEL?1:0);
        if (switch_view_subdir) return indexTitlesDownload;
        return indexTitles;
    }
    if (browse_mode==BROWSE_HVSC_MODE) {
        if (search_dbHVSC) {
            if (search_dbHVSC_hasFiles) return indexTitlesDownload;
        } else {
            if (dbHVSC_hasFiles) return indexTitlesDownload;
        }
        return indexTitles;
    }
    if (browse_depth>=2) return indexTitles;
    return nil;
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    if (mSearch) return -1;
    if (show_playlist) return -1;
    if ((browse_mode==BROWSE_MODLAND_MODE)&&(browse_depth==1)) return -1; //Modland browse mode chooser
    if ((browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)||(browse_mode==BROWSE_WEB_MODE)||(browse_mode==BROWSE_WORLDCHARTS_MODE)||(browse_mode==BROWSE_USERGUIDE_MODE)) return nil;
    if ((browse_mode==BROWSE_LOCAL_MODE)||(browse_mode==BROWSE_MODLAND_MODE)||(browse_depth>=2)) {
        if (index == 0) {
            [tableView setContentOffset:CGPointZero animated:NO];
            return NSNotFound;
        }
        return index;
    }
    if (browse_mode==BROWSE_HVSC_MODE) {
        if (index == 0) {
            [tableView setContentOffset:CGPointZero animated:NO];
            return NSNotFound;
        }
        return index;
    }
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
        cell = [[[UITableViewCell alloc] initWithFrame:CGRectZero reuseIdentifier:CellIdentifier] autorelease];
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
        topLabel.font = [UIFont boldSystemFontOfSize:20];
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
        cell.selectionStyle=UITableViewCellSelectionStyleGray;
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
        } else if (indexPath.row==BROWSE_PLAYLIST_MODE) {
            //topLabel.textColor=[UIColor colorWithRed:0.2f green:0.1f blue:0.7f alpha:1.0f];
            bottomLabel.text=NSLocalizedString(@"Browser_Playlists_SubKey",@"");
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
    } else if (browse_mode==BROWSE_PLAYLIST_MODE) { //playlist
        if (browse_depth==MENU_BROWSE_PLAYLIST_ROOTLEVEL) {
            NSDictionary *dictionary = [keys objectAtIndex:indexPath.section];
            NSArray *array = [dictionary objectForKey:@"entries"];
            cellValue = [array objectAtIndex:indexPath.row];
            
            if (indexPath.row<2) {
                cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
                
                if (indexPath.row==0) bottomLabel.text = NSLocalizedString(@"Create a new empty playlist.",@"");
                else if (indexPath.row==1) bottomLabel.text = NSLocalizedString(@"Create a playlist from currently played files.",@"");
                
            } else {			
                topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
                actionView.enabled=YES;
                actionView.hidden=NO;
                actionView.frame = CGRectMake(tableView.bounds.size.width-2-32-34,0,34,34);
                [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateNormal];
                [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateHighlighted];
                
                [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                
                cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tableView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                                           ROW_HEIGHT);
            }
        } else {
            if (show_playlist) {
                if (indexPath.row==0) {  //playlist/file browser 
                    cellValue=NSLocalizedString(@"Browse files",@"");
                    bottomLabel.text = NSLocalizedString(@"Add or remove entries from browser.",@"");
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                    topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
                }
                else if (indexPath.row==1) {  //playlist/rename
                    cellValue=NSLocalizedString(@"Rename playlist",@"");
                    bottomLabel.text = NSLocalizedString(@"Choose a new name.",@"");
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                    topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
                } else {  //playlist entries
                    cellValue=playlist->label[indexPath.row-2];
                    cell.accessoryType = UITableViewCellAccessoryNone;				
                    topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
                    bottomImageView.image=[UIImage imageNamed:ratingImg[playlist->ratings[indexPath.row-2]]];
                    if (!playlist->playcounts[indexPath.row-2]) bottomLabel.text = [NSString stringWithString:played0time];  //Not possible ?
                    else if (playlist->playcounts[indexPath.row-2]==1) bottomLabel.text = [NSString stringWithString:played1time];
                    else bottomLabel.text = [NSString stringWithFormat:playedXtimes,playlist->playcounts[indexPath.row-2]];
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                                                   22,
                                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32-60,
                                                   18);
                    
                }
            } else {				
                if (indexPath.section==1) {
                    cellValue=(mShowSubdir?NSLocalizedString(@"DisplayDir_MainKey",""):NSLocalizedString(@"DisplayAll_MainKey",""));
                    topLabel.textColor=[UIColor colorWithRed:0.4f green:0.4f blue:0.9f alpha:1.0];
                    bottomLabel.text=(mShowSubdir?NSLocalizedString(@"DisplayDir_SubKey",""):NSLocalizedString(@"DisplayAll_SubKey",""));
                    bottomLabel.text=[NSString stringWithFormat:@"%@ %d files",(mShowSubdir?NSLocalizedString(@"DisplayDir_SubKey",""):NSLocalizedString(@"DisplayAll_SubKey","")),(search_local?search_local_nb_entries:local_nb_entries)];
                    
                    
                    
                    
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                                   22,
                                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                                   18);
                    
                    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                               0,
                                               tableView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-4-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                               22);
                    
                    
                    
                    [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateNormal];
                    [secActionView setImage:[UIImage imageNamed:@"playlist_add_all.png"] forState:UIControlStateHighlighted];
                    [secActionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                    
                    [actionView setImage:[UIImage imageNamed:@"playlist_del_all.png"] forState:UIControlStateNormal];
                    [actionView setImage:[UIImage imageNamed:@"playlist_del_all.png"] forState:UIControlStateHighlighted];
                    [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                    
                    actionView.frame = CGRectMake(tableView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                    actionView.enabled=YES;
                    actionView.hidden=NO;
                    secActionView.frame = CGRectMake(tableView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-PRI_SEC_ACTIONS_IMAGE_SIZE-4,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                    secActionView.enabled=YES;
                    secActionView.hidden=NO;
                    
                } else {
                    
                    if (cur_local_entries[indexPath.section-2][indexPath.row].type==0) { //directory
                        cellValue=cur_local_entries[indexPath.section-2][indexPath.row].label;
                        topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
                        cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;				
                        topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                                   0,
                                                   tableView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                                                   34);
                        
                    } else  { //file
                        int nb_occur;
                        NSString *tmp_str;
                        cellValue=cur_local_entries[indexPath.section-2][indexPath.row].label;
                        cell.accessoryType = UITableViewCellAccessoryNone;
                        
                        topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                                   0,
                                                   tableView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                                   22);
                        
                        
                        if (cur_local_entries[indexPath.section-2][indexPath.row].rating==-1) {
                            [self getFileStatsDB:cur_local_entries[indexPath.section-2][indexPath.row].label
                                        fullpath:cur_local_entries[indexPath.section-2][indexPath.row].fullpath
                                       playcount:&cur_local_entries[indexPath.section-2][indexPath.row].playcount
                                          rating:&cur_local_entries[indexPath.section-2][indexPath.row].rating];
                        }
                        if (cur_local_entries[indexPath.section-2][indexPath.row].rating>=0) bottomImageView.image=[UIImage imageNamed:ratingImg[cur_local_entries[indexPath.section-2][indexPath.row].rating]];
                        if (!cur_local_entries[indexPath.section-2][indexPath.row].playcount) tmp_str = [NSString stringWithString:played0time]; 
                        else if (cur_local_entries[indexPath.section-2][indexPath.row].playcount==1) tmp_str = [NSString stringWithString:played1time];
                        else tmp_str = [NSString stringWithFormat:playedXtimes,cur_local_entries[indexPath.section-2][indexPath.row].playcount];				
                        
                        bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                                                       22,
                                                       tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                                       18);
                        if ((nb_occur=[self isLocalEntryInPlaylist:cur_local_entries[indexPath.section-2][indexPath.row].fullpath])) {
                            
                            [actionView setImage:[UIImage imageNamed:@"playlist_del.png"] forState:UIControlStateNormal];
                            [actionView setImage:[UIImage imageNamed:@"playlist_del.png"] forState:UIControlStateHighlighted];
                            [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                            [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                            actionView.frame = CGRectMake(tableView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                            actionView.enabled=YES;
                            actionView.hidden=NO;
                            
                            if (nb_occur==1) bottomLabel.text=[NSString stringWithFormat:@"Added 1 time. %@",tmp_str];
                            else bottomLabel.text=[NSString stringWithFormat:@"Added %d times. %@",nb_occur,tmp_str];									
                            topLabel.textColor=[UIColor colorWithRed:0.4f green:0.4f blue:0.4f alpha:1.0f];
                        } else {
                            topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
                            bottomLabel.text=[NSString stringWithFormat:@"Not in playlist. %@",tmp_str];
                        }
                    }
                }
            }
        }
    } else if ((browse_mode==BROWSE_LOCAL_MODE)||(browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)) { //local browser & favorites 
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
                    [self getFileStatsDB:cur_local_entries[section][indexPath.row].label
                                fullpath:cur_local_entries[section][indexPath.row].fullpath
                               playcount:&cur_local_entries[section][indexPath.row].playcount
                                  rating:&cur_local_entries[section][indexPath.row].rating
                             song_length:&cur_local_entries[section][indexPath.row].song_length									 
                                   songs:&cur_local_entries[section][indexPath.row].songs
                             channels_nb:&cur_local_entries[section][indexPath.row].channels_nb];
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
    } else if (browse_mode==BROWSE_HVSC_MODE) { //HVSC
        t_dbHVSC_browse_entry **cur_db_entries;
        cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
        int section = indexPath.section-1;
        int download_all=0;
        if (search_dbHVSC) {
            if (search_dbHVSC_hasFiles) download_all=1;
        } else {
            if (dbHVSC_hasFiles) download_all=1;
        }
        section-=download_all;
        if (download_all&&(indexPath.section==1)) {
            cellValue=NSLocalizedString(@"GetAllEntries_MainKey","");;
            topLabel.textColor=[UIColor colorWithRed:0.4f green:0.4f blue:0.8f alpha:1.0];
            bottomLabel.text=NSLocalizedString(@"GetAllEntries_SubKey","");
        } else {
            cellValue=cur_db_entries[section][indexPath.row].label;
            int colFactor;
            //update downloaded if needed
            if(cur_db_entries[section][indexPath.row].downloaded==-1) {
                NSString *pathToCheck=nil;
                
                if (cur_db_entries[section][indexPath.row].fullpath)
                    pathToCheck=[NSString stringWithFormat:@"%@/Documents/%@%@",NSHomeDirectory(),HVSC_BASEDIR,cur_db_entries[section][indexPath.row].fullpath];
                if (pathToCheck) {
                    if ([mFileMngr fileExistsAtPath:pathToCheck]) cur_db_entries[section][indexPath.row].downloaded=1;
                    else cur_db_entries[section][indexPath.row].downloaded=0;
                } else cur_db_entries[section][indexPath.row].downloaded=1;
            }
            
            if(cur_db_entries[section][indexPath.row].downloaded==1) {
                colFactor=1;
            } else colFactor=0;
            
            if (cur_db_entries[section][indexPath.row].id_md5) { //FILE
                if (colFactor) topLabel.textColor=[UIColor colorWithRed:0 green:0 blue:0 alpha:1.0];
                else topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0];
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tableView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                           22);
                if (cur_db_entries[section][indexPath.row].downloaded==1) {
                    if (cur_db_entries[section][indexPath.row].rating==-1) {
                        [self getFileStatsDB:cur_db_entries[section][indexPath.row].label
                                    fullpath:[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[section][indexPath.row].fullpath]
                                   playcount:&cur_db_entries[section][indexPath.row].playcount
                                      rating:&cur_db_entries[section][indexPath.row].rating
                                 song_length:&cur_db_entries[section][indexPath.row].song_length									 
                                       songs:&cur_db_entries[section][indexPath.row].songs
                                 channels_nb:&cur_db_entries[section][indexPath.row].channels_nb];
                    }
                    if (cur_db_entries[section][indexPath.row].rating>=0) bottomImageView.image=[UIImage imageNamed:ratingImg[cur_db_entries[section][indexPath.row].rating]];
                    
                    /*if (!cur_db_entries[section][indexPath.row].playcount) bottomLabel.text = [NSString stringWithString:played0time]; 
                     else if (cur_db_entries[section][indexPath.row].playcount==1) bottomLabel.text = [NSString stringWithString:played1time];
                     else bottomLabel.text = [NSString stringWithFormat:playedXtimes,cur_db_entries[section][indexPath.row].playcount];*/
                    
                    NSString *bottomStr;
                    if (cur_db_entries[section][indexPath.row].song_length>0)
                        bottomStr=[NSString stringWithFormat:@"%02d:%02d",cur_db_entries[section][indexPath.row].song_length/1000/60,(cur_db_entries[section][indexPath.row].song_length/1000)%60];
                    else bottomStr=@"--:--";						
                    if (cur_db_entries[section][indexPath.row].channels_nb)
                        bottomStr=[NSString stringWithFormat:@"%@ / %02dch",bottomStr,cur_db_entries[section][indexPath.row].channels_nb];
                    else bottomStr=[NSString stringWithFormat:@"%@ / --ch",bottomStr];						
                    if (cur_db_entries[section][indexPath.row].songs) {
                        if (cur_db_entries[section][indexPath.row].songs==1) bottomStr=[NSString stringWithFormat:@"%@ / 1 song",bottomStr];
                        else bottomStr=[NSString stringWithFormat:@"%@ / %d songs",bottomStr,cur_db_entries[section][indexPath.row].songs];
                    }
                    else bottomStr=[NSString stringWithFormat:@"%@ / - song",bottomStr];		   						
                    bottomStr=[NSString stringWithFormat:@"%@ / Pl:%d",bottomStr,cur_db_entries[section][indexPath.row].playcount];
                    
                    bottomLabel.text=bottomStr;
                    
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                                                   22,
                                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                                   18);
                } else {
                    /*bottomLabel.text=[NSString stringWithFormat:@"%dKB",cur_db_entries[indexPath.section-1][indexPath.row].filesize/1024];
                     bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                     22,
                     tableView.bounds.size.width -1.0 * cell.indentationWidth-32-34-34-60,
                     18);*/
                }
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
                actionView.frame = CGRectMake(tableView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                actionView.enabled=YES;
                actionView.hidden=NO;
                
            } else { // DIR
                bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                               22,
                                               tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                               18);
                bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[section][indexPath.row].filesize>1?nbFiles:nb1File),cur_db_entries[section][indexPath.row].filesize];
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tableView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                                           22);
                topLabel.textColor=[UIColor colorWithRed:0.3f green:0.3f blue:0.8f alpha:1.0f];
                cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
            }		
        }
    } else if (browse_mode==BROWSE_MODLAND_MODE) {//modland
        if (browse_depth==1) {//choose browse mode
            if (indexPath.row==0) {
                cellValue=NSLocalizedString(@"Formats/Artists/Files",@"");
                //topLabel.textColor=[UIColor colorWithRed:0.1f green:0.4f blue:0.8f alpha:1.0f];
                bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"Formats: %d entries.",@""),mNbFormatEntries];
            } else if (indexPath.row==1) {
                cellValue=NSLocalizedString(@"Artists/Formats/Files",@"");
                //topLabel.textColor=[UIColor colorWithRed:0.3f green:0.3f blue:0.8f alpha:1.0f];
                bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"Artists: %d entries.",@""),mNbAuthorEntries];
            } else if (indexPath.row==2) {
                cellValue=NSLocalizedString(@"Artists/Files",@"");
                //topLabel.textColor=[UIColor colorWithRed:0.6f green:0.2f blue:0.7f alpha:1.0f];
                bottomLabel.text=[NSString stringWithFormat:NSLocalizedString(@"Artists: %d entries.",@""),mNbAuthorEntries];
            }
            cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        } else {
            t_db_browse_entry **cur_db_entries;
            cur_db_entries=(search_db?search_db_entries:db_entries);
            int file_or_album=0;
            if (search_db) {
                if (search_db_hasFiles) file_or_album=1;
            } else {
                if (db_hasFiles) file_or_album=1;
            }
            int section=indexPath.section-1-file_or_album;
            if (file_or_album&&(indexPath.section==1)){
                cellValue=NSLocalizedString(@"GetAllEntries_MainKey","");
                topLabel.textColor=[UIColor colorWithRed:0.4f green:0.4f blue:0.8f alpha:1.0];
                bottomLabel.text=NSLocalizedString(@"GetAllEntries_SubKey","");;
                //cell.accessoryType = UITableViewCellAccessoryDetailDisclosureButton;
            } else {
                cellValue=cur_db_entries[section][indexPath.row].label;
                int colFactor;
                //update downloaded if needed
                if(cur_db_entries[section][indexPath.row].downloaded==-1) {
                    if (cur_db_entries[section][indexPath.row].id_mod>=0) {  //id_mod is known
                        cur_db_entries[section][indexPath.row].downloaded=[self checkIsDownloadedMod:cur_db_entries[section][indexPath.row].id_mod];
                    } else {
                        cur_db_entries[section][indexPath.row].downloaded=1;
                    }
                }
                
                if(cur_db_entries[section][indexPath.row].downloaded==1) {
                    colFactor=1;
                } else colFactor=0;
                
                if (cur_db_entries[section][indexPath.row].id_mod>=0) { //MOD ?
                    if (colFactor) topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f];
                    else topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0f];
                    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                               0,
                                               tableView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                               22);
                    
                    if (cur_db_entries[section][indexPath.row].downloaded==1) {
                        if (cur_db_entries[section][indexPath.row].rating==-1) {
                            [self getFileStatsDB:cur_db_entries[section][indexPath.row].label
                                        fullpath:[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,
                                                  [self getCompleteLocalPath:cur_db_entries[section][indexPath.row].id_mod]]
                                       playcount:&cur_db_entries[section][indexPath.row].playcount
                                          rating:&cur_db_entries[section][indexPath.row].rating
                                     song_length:&cur_db_entries[section][indexPath.row].song_length									 
                                           songs:&cur_db_entries[section][indexPath.row].songs
                                     channels_nb:&cur_db_entries[section][indexPath.row].channels_nb];
                        }
                        if (cur_db_entries[section][indexPath.row].rating>=0) bottomImageView.image=[UIImage imageNamed:ratingImg[cur_db_entries[section][indexPath.row].rating]];
                        
                        /*if (!cur_db_entries[section][indexPath.row].playcount) bottomLabel.text = [NSString stringWithString:played0time]; 
                         else if (cur_db_entries[section][indexPath.row].playcount==1) bottomLabel.text = [NSString stringWithString:played1time];
                         else bottomLabel.text = [NSString stringWithFormat:playedXtimes,cur_db_entries[section][indexPath.row].playcount];				
                         */
                        NSString *bottomStr;
                        if (cur_db_entries[section][indexPath.row].song_length>0)
                            bottomStr=[NSString stringWithFormat:@"%02d:%02d",cur_db_entries[section][indexPath.row].song_length/1000/60,(cur_db_entries[section][indexPath.row].song_length/1000)%60];
                        else bottomStr=@"--:--";						
                        if (cur_db_entries[section][indexPath.row].channels_nb)
                            bottomStr=[NSString stringWithFormat:@"%@ / %02dch",bottomStr,cur_db_entries[section][indexPath.row].channels_nb];
                        else bottomStr=[NSString stringWithFormat:@"%@ / --ch",bottomStr];						
                        if (cur_db_entries[section][indexPath.row].songs) {
                            if (cur_db_entries[section][indexPath.row].songs==1) bottomStr=[NSString stringWithFormat:@"%@ / 1 song",bottomStr];
                            else bottomStr=[NSString stringWithFormat:@"%@ / %d songs",bottomStr,cur_db_entries[section][indexPath.row].songs];
                        }
                        else bottomStr=[NSString stringWithFormat:@"%@ / - song",bottomStr];		   						
                        bottomStr=[NSString stringWithFormat:@"%@ / Pl:%d",bottomStr,cur_db_entries[section][indexPath.row].playcount];
                        
                        bottomLabel.text=bottomStr;
                        
                        bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                                                       22,
                                                       tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                                       18);
                    } else {
                        bottomLabel.text=[NSString stringWithFormat:@"%dKB",cur_db_entries[section][indexPath.row].filesize/1024];
                        bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                                                       22,
                                                       tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                                       18);
                    }
                    
                    
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
                    actionView.frame = CGRectMake(tableView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                    actionView.enabled=YES;
                    actionView.hidden=NO;
                    
                } else if (cur_db_entries[section][indexPath.row].id_album>=0) {// ALBUM ?
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                                   22,
                                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                                   18);
                    topLabel.textColor=[UIColor colorWithRed:0.8f green:0.6f blue:0.0f alpha:1.0f];
                    bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[section][indexPath.row].filesize>1?nbFiles:nb1File),cur_db_entries[section][indexPath.row].filesize];
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                } else if (cur_db_entries[section][indexPath.row].id_author>=0) {// AUTHOR ?
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                                   22,
                                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                                   18);
                    topLabel.textColor=[UIColor colorWithRed:1.0f green:0.0f blue:0.8f alpha:1.0f];
                    bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[section][indexPath.row].filesize>1?nbFiles:nb1File),cur_db_entries[section][indexPath.row].filesize];
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;				
                } else  {
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                                   22,
                                                   tableView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                                   18);
                    topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];
                    bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[section][indexPath.row].filesize>1?nbFiles:nb1File),cur_db_entries[section][indexPath.row].filesize];
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;				
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
        
        if (show_playlist&&(indexPath.row>=2)) { //delete playlist entry			
            [playlist->label[indexPath.row-2] release];
            [playlist->fullpath[indexPath.row-2] release];
            for (int i=indexPath.row-1;i<playlist->nb_entries;i++) {
                playlist->label[i-1]=playlist->label[i];
                playlist->fullpath[i-1]=playlist->fullpath[i];
                playlist->ratings[i-1]=playlist->ratings[i];
                playlist->playcounts[i-1]=playlist->playcounts[i];
            }
            playlist->nb_entries--;
            [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
            [self replacePlaylistDBwithCurrent];
        }
        if ((browse_mode==BROWSE_PLAYLIST_MODE)&&(browse_depth==1)&&(indexPath.row>=2)) {  //delete a playlist
            if ([self deletePlaylistDB:[list objectAtIndex:indexPath.row-2]]) {
                
                [keys release];keys=nil;
                [list release];list=nil;
                [self fillKeys];
                [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
                
                
            }
        }
        if ((browse_mode==BROWSE_LOCAL_MODE)&&(browse_depth>=1)) {  //Local browse mode ?
            int switch_view_subdir=(browse_depth>=SHOW_SUDIR_MIN_LEVEL?1:0);
            int section=indexPath.section-1-switch_view_subdir;
            NSString *fullpath=[NSHomeDirectory() stringByAppendingPathComponent:cur_local_entries[section][indexPath.row].fullpath];
            NSError *err;
            
            if (cur_local_entries[section][indexPath.row].type==0) { //Dir
                [self deleteStatsDirDB:fullpath];
            }
            if (cur_local_entries[section][indexPath.row].type&3) { //File
                [self deleteStatsFileDB:fullpath];
            }
            
            [mFileMngr removeItemAtPath:fullpath error:&err];
            
            [self listLocalFiles];						
            [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
        }
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
        if (browse_mode==BROWSE_MODLAND_MODE) {
            t_db_browse_entry **cur_db_entries;
            cur_db_entries=(search_db?search_db_entries:db_entries);
            int section = indexPath.section-1;
            int download_all=0;
            if (search_db) {
                if (search_db_hasFiles) download_all=1;
            } else {
                if (db_hasFiles) download_all=1;
            }
            section-=download_all;
            //delete file
            NSString *localpath=[NSHomeDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[section][indexPath.row].id_mod]]];
            NSError *err;
            //			NSLog(@"%@",localpath);
            [self deleteStatsFileDB:localpath];
            cur_db_entries[section][indexPath.row].downloaded=0;
            //delete local file
            [mFileMngr removeItemAtPath:localpath error:&err];
            //ask for a reload/redraw
            [tableView reloadData];
        }
        if (browse_mode==BROWSE_HVSC_MODE) {
            t_dbHVSC_browse_entry **cur_db_entries;
            cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
            int section = indexPath.section-1;
            int download_all=0;
            if (search_dbHVSC) {
                if (search_dbHVSC_hasFiles) download_all=1;
            } else {
                if (dbHVSC_hasFiles) download_all=1;
            }
            section-=download_all;
            //delete file
            NSString *fullpath=[NSHomeDirectory() stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents/%@%@",HVSC_BASEDIR,cur_db_entries[section][indexPath.row].fullpath]];
            NSError *err;
            //			NSLog(@"%@",fullpath);
            [self deleteStatsFileDB:fullpath];
            cur_db_entries[section][indexPath.row].downloaded=0;
            //delete local file
            [mFileMngr removeItemAtPath:fullpath error:&err];
            //ask for a reload/redraw
            [tableView reloadData];
        }
        
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}
- (NSIndexPath *)tableView:(UITableView *)tableView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath {
    
    if (show_playlist) {
        if (proposedDestinationIndexPath.row<2) {
            NSIndexPath *newIndexPath=[[[NSIndexPath alloc] initWithIndex:0] autorelease];
            return [newIndexPath indexPathByAddingIndex:2];
        }
    }
    if ((browse_mode==BROWSE_PLAYLIST_MODE)&&(browse_depth==1)) {
        if (proposedDestinationIndexPath.row<2) {
            NSIndexPath *newIndexPath=[[[NSIndexPath alloc] initWithIndex:0] autorelease];
            return [newIndexPath indexPathByAddingIndex:2];
        }
    }
    return proposedDestinationIndexPath;
}
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
    if (show_playlist&&(fromIndexPath.row&&toIndexPath.row>=2)) {
        signed char tmpR=playlist->ratings[fromIndexPath.row-2];
        short int tmpC=playlist->playcounts[fromIndexPath.row-2];
        NSString *tmpF=playlist->fullpath[fromIndexPath.row-2];
        NSString *tmpL=playlist->label[fromIndexPath.row-2];
        if (toIndexPath.row<fromIndexPath.row) {
            for (int i=fromIndexPath.row-2;i>toIndexPath.row-2;i--) {
                playlist->label[i]=playlist->label[i-1];
                playlist->fullpath[i]=playlist->fullpath[i-1];
                playlist->ratings[i]=playlist->ratings[i-1];
                playlist->playcounts[i]=playlist->playcounts[i-1];
            }
            playlist->label[toIndexPath.row-2]=tmpL;
            playlist->fullpath[toIndexPath.row-2]=tmpF;
            playlist->ratings[toIndexPath.row-2]=tmpR;
            playlist->playcounts[toIndexPath.row-2]=tmpC;
        } else {
            for (int i=fromIndexPath.row-2;i<toIndexPath.row-2;i++) {
                playlist->label[i]=playlist->label[i+1];
                playlist->fullpath[i]=playlist->fullpath[i+1];
                playlist->ratings[i]=playlist->ratings[i+1];
                playlist->playcounts[i]=playlist->playcounts[i+1];
            }
            playlist->label[toIndexPath.row-2]=tmpL;
            playlist->fullpath[toIndexPath.row-2]=tmpF;
            playlist->ratings[toIndexPath.row-2]=tmpR;
            playlist->playcounts[toIndexPath.row-2]=tmpC;
        }
        
        [self replacePlaylistDBwithCurrent];
    } 
}
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    if (show_playlist&&(indexPath.row>=2)) return YES;
    return NO;
}
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    if (show_playlist&&(indexPath.row>=2)) return YES;
    if ((browse_mode==BROWSE_LOCAL_MODE)&&(browse_depth>=1)) return YES;
    if ((browse_mode==BROWSE_PLAYLIST_MODE)&&(browse_depth==1)&&(indexPath.row>=2)) return YES;
    if (browse_mode==BROWSE_RATED_MODE) return YES;
    if (browse_mode==BROWSE_MOSTPLAYED_MODE) return YES;
    if (browse_mode==BROWSE_HVSC_MODE) {
        t_dbHVSC_browse_entry **cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
        int section =indexPath.section-1;
        if (search_dbHVSC) {
            if (search_dbHVSC_hasFiles) section--;
        } else {
            if (dbHVSC_hasFiles) section--;
        }
        if (section>=0) {
            if (cur_db_entries[section][indexPath.row].downloaded==1) return YES;
        }
    }
    if (browse_mode==BROWSE_MODLAND_MODE) {
        t_db_browse_entry **cur_db_entries=(search_db?search_db_entries:db_entries);
        int section =indexPath.section-1;
        if (search_db) {
            if (search_db_hasFiles) section--;
        } else {
            if (db_hasFiles) section--;
        }
        if (section>=0) {
            if (cur_db_entries[section][indexPath.row].downloaded==1) return YES;
        }
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

-(IBAction)cancelPlaylistName{
    if (childController) [childController cancelPlaylistName];   
    [self dismissModalViewControllerAnimated:YES];
}
-(IBAction)validatePlaylistName{
    if (childController) [childController validatePlaylistName];   
    [self dismissModalViewControllerAnimated:YES];
}

- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    [theTextField resignFirstResponder];
    if (childController) [childController validatePlaylistName];
    return YES;
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
        if (browse_mode==BROWSE_PLAYLIST_MODE) {//playlist
            if (browse_depth==1) {
                if (indexPath.row>=2) { //start selected playlist
                    [self freePlaylist];
                    playlist=(t_playlist*)malloc(sizeof(t_playlist));
                    memset(playlist,0,sizeof(t_playlist));
                    [self loadPlayListsFromDB:[list objectAtIndex:(indexPath.row-2)] intoPlaylist:playlist];
                    if (playlist->nb_entries) {
                        //						self.tabBarController.selectedViewController = detailViewController;
                        //						self.navigationController.navigationBar.hidden = YES;
                        if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                        else [[super tableView] reloadData];
                        
                        NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                        NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                        for (int j=0;j<playlist->nb_entries;j++) {
                            [array_label addObject:playlist->label[j]];
                            [array_path addObject:playlist->fullpath[j]];
                        }
                        [detailViewController play_listmodules:array_label start_index:-1 path:array_path ratings:playlist->ratings playcounts:playlist->playcounts];
                        if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) {
                            [keys release];keys=nil;
                            [list release];list=nil;
                        }
                    }
                    [self freePlaylist];
                }
            } else {
                if (show_playlist) {
                } else { //browsing for playlist, remove selected file from playlist
                    if (indexPath.section==1) {
                        //remove all
                        for (int i=0;i<27;i++) 
                            for (int j=0;j<(search_local?search_local_entries_count[i]:local_entries_count[i]);j++)
                                if (cur_local_entries[i][j].type&3) {
                                    int found=-1;
                                    for (int ii=0;ii<playlist->nb_entries;ii++) {
                                        if ([playlist->fullpath[ii] compare:cur_local_entries[i][j].fullpath]==NSOrderedSame) found=ii;
                                    }
                                    if (found>=0) {
                                        [playlist->label[found] release];
                                        [playlist->fullpath[found] release];
                                        for (int ii=found;ii<playlist->nb_entries-1;ii++) {
                                            playlist->label[ii]=playlist->label[ii+1];
                                            playlist->fullpath[ii]=playlist->fullpath[ii+1];
                                            playlist->ratings[ii]=playlist->ratings[ii+1];
                                            playlist->playcounts[ii]=playlist->playcounts[ii+1];
                                        }
                                        playlist->nb_entries--;
                                        
                                        
                                    }             
                                }                    
                        [self replacePlaylistDBwithCurrent];
                        [[super tableView] reloadData];
                        
                    } else {
                        
                        int found=-1;
                        for (int i=0;i<playlist->nb_entries;i++) {
                            if ([playlist->fullpath[i] compare:cur_local_entries[indexPath.section-2][indexPath.row].fullpath]==NSOrderedSame) found=i;
                        }
                        if (found>=0) {
                            [playlist->label[found] release];
                            [playlist->fullpath[found] release];
                            for (int i=found;i<playlist->nb_entries-1;i++) {
                                playlist->label[i]=playlist->label[i+1];
                                playlist->fullpath[i]=playlist->fullpath[i+1];
                                playlist->ratings[i]=playlist->ratings[i+1];
                                playlist->playcounts[i]=playlist->playcounts[i+1];
                            }
                            playlist->nb_entries--;
                            
                            [self replacePlaylistDBwithCurrent];
                            [[super tableView] reloadData];
                        }
                    }
                    
                }
            }
        } else if (browse_mode==BROWSE_LOCAL_MODE||(browse_mode==BROWSE_RATED_MODE)||(browse_mode==BROWSE_MOSTPLAYED_MODE)) {//local  browser & favorites
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
        } else if (browse_mode==BROWSE_HVSC_MODE) {  //HVSC DB
            t_dbHVSC_browse_entry **cur_db_entries;
            cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
            int section = indexPath.section-1;
            int download_all=0;
            if (search_dbHVSC) {
                if (search_dbHVSC_hasFiles) download_all=1;
            } else {
                if (dbHVSC_hasFiles) download_all=1;
            }
            section-=download_all;
            
            if (cur_db_entries[section][indexPath.row].id_md5) { //FILE
                //File selected, start download is needed
                NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].label];
                NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].fullpath];				
                NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[section][indexPath.row].fullpath];
                
                if (cur_db_entries[section][indexPath.row].downloaded==1) {
                    NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                    NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                    [array_label addObject:sidFilename];
                    [array_path addObject:localPath];
                    cur_db_entries[section][indexPath.row].rating=-1;
                    [detailViewController play_listmodules:array_label start_index:0 path:array_path ratings:&(cur_db_entries[section][indexPath.row].rating) playcounts:&(cur_db_entries[section][indexPath.row].playcount)];
                    if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                    else [[super tableView] reloadData];
                } else {
                    [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                    mCurrentWinAskedDownload=1;
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:HVSC_FTPHOST filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                }
            }
        } else if (browse_mode==BROWSE_MODLAND_MODE) {  //modland DB
            NSString *filePath;
            NSString *modFilename;
            t_db_browse_entry **cur_db_entries;
            cur_db_entries=(search_db?search_db_entries:db_entries);
            int download_all=0;
            int section=indexPath.section-1;
            if (search_db) {
                if (search_db_hasFiles) download_all=1;
            } else {
                if (db_hasFiles) download_all=1;
            }
            section-=download_all;
            
            filePath=[self getCompletePath:cur_db_entries[section][indexPath.row].id_mod];
            modFilename=[self getModFilename:cur_db_entries[section][indexPath.row].id_mod];
            
            NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
            NSString *localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[section][indexPath.row].id_mod]];
            mClickedPrimAction=1;
            
            if (cur_db_entries[section][indexPath.row].downloaded==1) {
                NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                [array_label addObject:modFilename];
                [array_path addObject:localPath];
                cur_db_entries[section][indexPath.row].rating=-1;
                [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                else [[super tableView] reloadData];
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                mCurrentWinAskedDownload=1;
                [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:MODLAND_FTPHOST filesize:cur_db_entries[section][indexPath.row].filesize filename:modFilename isMODLAND:1 usePrimaryAction:1];
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
        }  else if (browse_mode==BROWSE_HVSC_MODE) {  //HVSC DB
            t_dbHVSC_browse_entry **cur_db_entries;
            cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
            int download_all=0;
            int section=indexPath.section-1;
            if (search_dbHVSC) {
                if (search_dbHVSC_hasFiles) download_all=1;
            } else {
                if (dbHVSC_hasFiles) download_all=1;
            }
            section-=download_all;
            
            if (cur_db_entries[section][indexPath.row].id_md5) { //FILE
                //File selected, start download is needed
                NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].label];
                NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].fullpath];				
                NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[section][indexPath.row].fullpath];
                mClickedPrimAction=2;
                
                if (cur_db_entries[section][indexPath.row].downloaded==1) {
                    NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                    NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                    [array_label addObject:sidFilename];
                    [array_path addObject:localPath];
                    [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                    cur_db_entries[section][indexPath.row].rating=-1;
                    if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                    else [[super tableView] reloadData];					
                } else {
                    [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                    mCurrentWinAskedDownload=1;
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:HVSC_FTPHOST filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                }
            }
        } else if (browse_mode==BROWSE_MODLAND_MODE) {  //modland DB
            t_db_browse_entry **cur_db_entries;
            cur_db_entries=(search_db?search_db_entries:db_entries);
            int download_all=0;
            int section=indexPath.section-1;
            if (search_db) {
                if (search_db_hasFiles) download_all=1;
            } else {
                if (db_hasFiles) download_all=1;
            }
            section-=download_all;
            
            NSString *filePath=[self getCompletePath:cur_db_entries[section][indexPath.row].id_mod];
            NSString *modFilename=[self getModFilename:cur_db_entries[section][indexPath.row].id_mod];
            NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
            NSString *localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[section][indexPath.row].id_mod]];
            mClickedPrimAction=2;
            
            if (cur_db_entries[section][indexPath.row].downloaded==1) {
                if ([detailViewController add_to_playlist:localPath fileName:modFilename forcenoplay:1]) {
                    cur_db_entries[section][indexPath.row].rating=-1;
                    if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                    else [[super tableView] reloadData];
                }
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                mCurrentWinAskedDownload=1;
                [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:MODLAND_FTPHOST filesize:cur_db_entries[section][indexPath.row].filesize filename:modFilename isMODLAND:1 usePrimaryAction:2];
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
        }  else if (indexPath.row==BROWSE_PLAYLIST_MODE) {
            if (childController == nil) childController = [[RootViewControllerPlaylist alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
            else {			// Don't cache childviews
            }
            //set new title
            childController.title = cellValue;
            // Set new directory
            ((RootViewControllerPlaylist*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerPlaylist*)childController)->detailViewController=detailViewController;
            ((RootViewControllerPlaylist*)childController)->playerButton=playerButton;
            ((RootViewControllerPlaylist*)childController)->textInputView=textInputView;
            ((RootViewControllerPlaylist*)childController)->inputText=inputText;
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];	
            [keys release];keys=nil;
            [list release];list=nil;
        }else {
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
            ((RootViewControllerIphone*)childController)->textInputView=textInputView;
            ((RootViewControllerIphone*)childController)->inputText=inputText;
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
        } else if (browse_mode==BROWSE_HVSC_MODE) {  //HVSC DB
            t_dbHVSC_browse_entry **cur_db_entries;
            cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
            int section = indexPath.section-1;
            int download_all=0;
            if (search_dbHVSC) {
                if (search_dbHVSC_hasFiles) download_all=1;
            } else {
                if (dbHVSC_hasFiles) download_all=1;
            }
            section-=download_all;
            
            if ((indexPath.section==1)&&(download_all)) {
                //download all dir
                NSString *sidFilename;
                NSString *ftpPath;
                NSString *localPath;
                int first=0; //1;  Do not play even first file => TODO : add a setting for this
                int existing;
                int tooMuch=0;
                if (detailViewController.sc_DefaultAction.selectedSegmentIndex==2) first=0;//enqueue only
                
                int *cur_db_entries_count=(search_db?search_dbHVSC_entries_count:dbHVSC_entries_count);
                
                for (int i=0;i<27;i++) {
                    for (int j=0;j<cur_db_entries_count[i];j++) {
                        if (cur_db_entries[i][j].id_md5) {//mod found
                            
                            existing=cur_db_entries[i][j].downloaded;
                            if (existing==-1) {
                                NSString *pathToCheck=nil;
                                
                                if (cur_db_entries[i][j].fullpath)
                                    pathToCheck=[NSString stringWithFormat:@"%@/Documents/%@%@",NSHomeDirectory(),HVSC_BASEDIR,cur_db_entries[i][j].fullpath];
                                if (pathToCheck) {
                                    if ([mFileMngr fileExistsAtPath:pathToCheck]) cur_db_entries[i][j].downloaded=1;
                                    else existing=cur_db_entries[i][j].downloaded=0;
                                } else existing=cur_db_entries[i][j].downloaded=1;								
                            }
                            if (existing==0) {
                                
                                //File selected, start download is needed
                                sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[i][j].label];
                                ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[i][j].fullpath];				
                                localPath=[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[i][j].fullpath];
                                mClickedPrimAction=(detailViewController.sc_DefaultAction.selectedSegmentIndex==0);
                                
                                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                                mCurrentWinAskedDownload=1;
                                if (first) {
                                    if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:HVSC_FTPHOST filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:1]) {
                                        tooMuch=1;
                                        break;
                                    }
                                    first=0;
                                } else {
                                    if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:HVSC_FTPHOST filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:2]) {
                                        tooMuch=1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (tooMuch) break;
                }
            } else {
                
                if (cur_db_entries[section][indexPath.row].id_md5) { //FILE
                    //File selected, start download is needed
                    NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].label];
                    NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[section][indexPath.row].fullpath];				
                    NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[section][indexPath.row].fullpath];
                    mClickedPrimAction=(detailViewController.sc_DefaultAction.selectedSegmentIndex==0);
                    
                    if (cur_db_entries[section][indexPath.row].downloaded==1) {
                        if (mClickedPrimAction) {
                            NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                            NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                            [array_label addObject:sidFilename];
                            [array_path addObject:localPath];
                            cur_db_entries[section][indexPath.row].rating=-1;
                            [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                            if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                            else [tableView reloadData];
                        } else {
                            if ([detailViewController add_to_playlist:localPath fileName:sidFilename forcenoplay:(detailViewController.sc_DefaultAction.selectedSegmentIndex==1)]) {
                                cur_db_entries[section][indexPath.row].rating=-1;
                                if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                                else [tableView reloadData];
                            }
                        }
                    } else {
                        [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                        mCurrentWinAskedDownload=1;
                        [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:HVSC_FTPHOST filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                    }
                } else { //DIR
                    if (browse_depth==1) {//DIR1
                        mDir1=cur_db_entries[section][indexPath.row].dir1;
                    } else if (browse_depth==2) {//DIR2
                        mDir2=cur_db_entries[section][indexPath.row].dir2;
                    } else if (browse_depth==3) {//DIR3
                        mDir3=cur_db_entries[section][indexPath.row].dir3;
                    } else if (browse_depth==4) {//DIR4
                        mDir4=cur_db_entries[section][indexPath.row].dir4;
                    } else if (browse_depth==5) {//DIR5
                        mDir5=cur_db_entries[section][indexPath.row].dir5;
                    }
                    
                    if (childController == nil) childController = [[RootViewControllerIphone alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
                    else {// Don't cache childviews
                    }
                    
                    childController.title = cur_db_entries[section][indexPath.row].label;
                    // Set new depth
                    ((RootViewControllerIphone*)childController)->browse_depth = browse_depth+1;
                    ((RootViewControllerIphone*)childController)->browse_mode = browse_mode;
                    ((RootViewControllerIphone*)childController)->playerButton=playerButton;
                    ((RootViewControllerIphone*)childController)->detailViewController=detailViewController;
                    ((RootViewControllerIphone*)childController)->downloadViewController=downloadViewController;
                    
                    ((RootViewControllerIphone*)childController)->mDir1 = mDir1;
                    ((RootViewControllerIphone*)childController)->mDir2 = mDir2;
                    ((RootViewControllerIphone*)childController)->mDir3 = mDir3;
                    ((RootViewControllerIphone*)childController)->mDir4 = mDir4;
                    ((RootViewControllerIphone*)childController)->mDir5 = mDir5;
                    
                    // And push the window
                    [self.navigationController pushViewController:childController animated:YES];
                }
            }
        } else if (browse_mode==BROWSE_MODLAND_MODE) {  //modland DB			
            if (browse_depth==1) {
                if (childController == nil) childController = [[RootViewControllerIphone alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
                else {// Don't cache childviews
                }
                
                if (indexPath.row==0) { //Formats/Authors/Files
                    modland_browse_mode=0;
                    childController.title = @"Formats";
                    
                } else if (indexPath.row==1) {  //Authors/Formats/Files
                    modland_browse_mode=1;
                    childController.title = @"Artists";
                    
                } else if (indexPath.row==2) {  //Authors/Files
                    modland_browse_mode=2;
                    childController.title = @"Artists";
                }
                // Set new depth
                ((RootViewControllerIphone*)childController)->browse_depth = browse_depth+1;
                ((RootViewControllerIphone*)childController)->browse_mode = browse_mode;
                ((RootViewControllerIphone*)childController)->modland_browse_mode=modland_browse_mode;
                ((RootViewControllerIphone*)childController)->playerButton=playerButton;
                ((RootViewControllerIphone*)childController)->detailViewController=detailViewController;
                ((RootViewControllerIphone*)childController)->downloadViewController=downloadViewController;
                
                // And push the window
                [self.navigationController pushViewController:childController animated:YES];
                
            } else {
                int file_or_album=0;
                t_db_browse_entry **cur_db_entries;
                cur_db_entries=(search_db?search_db_entries:db_entries);
                
                if (search_db) {
                    if (search_db_hasFiles) file_or_album=1;
                } else {
                    if (db_hasFiles) file_or_album=1;
                }
                int section=indexPath.section-1-file_or_album;
                if (!file_or_album) {
                    cellValue=cur_db_entries[section][indexPath.row].label;
                    
                    if (childController == nil) childController = [[RootViewControllerIphone alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
                    else {// Don't cache childviews
                    }
                    //set new title
                    childController.title = cellValue;
                    // Set new depth
                    ((RootViewControllerIphone*)childController)->browse_depth = browse_depth+1;
                    ((RootViewControllerIphone*)childController)->browse_mode = browse_mode;
                    ((RootViewControllerIphone*)childController)->modland_browse_mode=modland_browse_mode;
                    ((RootViewControllerIphone*)childController)->playerButton=playerButton;
                    
                    switch (modland_browse_mode) {
                        case 0:		//Formats/Authors/Files
                            //Filetype selected
                            if (browse_depth==2) {
                                ((RootViewControllerIphone*)childController)->mFiletypeID = cur_db_entries[section][indexPath.row].id_type;
                            } else ((RootViewControllerIphone*)childController)->mFiletypeID = mFiletypeID;
                            //Author selected
                            if (browse_depth==3) {
                                ((RootViewControllerIphone*)childController)->mAuthorID = cur_db_entries[section][indexPath.row].id_author;
                            } else ((RootViewControllerIphone*)childController)->mAuthorID = mAuthorID;
                            if (browse_depth==4) {
                                ((RootViewControllerIphone*)childController)->mAlbumID = cur_db_entries[section][indexPath.row].id_album;
                            } else ((RootViewControllerIphone*)childController)->mAlbumID = mAlbumID;
                            break;
                        case 1:		//Authors/Formats/Files
                            //Author selected
                            if (browse_depth==2) {
                                ((RootViewControllerIphone*)childController)->mAuthorID = cur_db_entries[section][indexPath.row].id_author;
                            } else ((RootViewControllerIphone*)childController)->mAuthorID = mAuthorID;
                            //Filetype selected
                            if (browse_depth==3) {
                                ((RootViewControllerIphone*)childController)->mFiletypeID = cur_db_entries[section][indexPath.row].id_type;
                            } else ((RootViewControllerIphone*)childController)->mFiletypeID = mFiletypeID;
                            if (browse_depth==4) {
                                ((RootViewControllerIphone*)childController)->mAlbumID = cur_db_entries[section][indexPath.row].id_album;
                            } else ((RootViewControllerIphone*)childController)->mAlbumID = mAlbumID;
                            break;
                        case 2:		//Authors/Files
                            //Author selected
                            if (browse_depth==2) {
                                ((RootViewControllerIphone*)childController)->mAuthorID = cur_db_entries[section][indexPath.row].id_author;
                            } else ((RootViewControllerIphone*)childController)->mAuthorID = mAuthorID;
                            if (browse_depth==3) {
                                ((RootViewControllerIphone*)childController)->mAlbumID = cur_db_entries[section][indexPath.row].id_album;
                            } else ((RootViewControllerIphone*)childController)->mAlbumID = mAlbumID;
                            break;
                    }
                    
                    ((RootViewControllerIphone*)childController)->detailViewController=detailViewController;
                    ((RootViewControllerIphone*)childController)->downloadViewController=downloadViewController;
                    
                    // And push the window
                    [self.navigationController pushViewController:childController animated:YES];
                    
                    
                    //				[childController autorelease];
                } else { 
                    if (indexPath.section==1) {//download all dir
                        NSString *filePath;
                        NSString *modFilename;
                        NSString *ftpPath;
                        NSString *localPath;
                        int first=0; //1;  Do not play even first file => TODO : add a setting for this
                        int existing;
                        int tooMuch=0;
                        if (detailViewController.sc_DefaultAction.selectedSegmentIndex==2) first=0;//enqueue only
                        
                        int *cur_db_entries_count=(search_db?search_db_entries_count:db_entries_count);
                        
                        for (int i=0;i<27;i++) {
                            for (int j=0;j<cur_db_entries_count[i];j++) {
                                if (cur_db_entries[i][j].id_mod!=-1) {//mod found
                                    
                                    existing=cur_db_entries[i][j].downloaded;
                                    if (existing==-1) {
                                        existing=cur_db_entries[i][j].downloaded=[self checkIsDownloadedMod:cur_db_entries[i][j].id_mod];
                                    }
                                    
                                    if (existing==0) {
                                        filePath=[self getCompletePath:cur_db_entries[i][j].id_mod];
                                        modFilename=[self getModFilename:cur_db_entries[i][j].id_mod];
                                        ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
                                        localPath=[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[i][j].id_mod]];
                                        mCurrentWinAskedDownload=1;
                                        [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                                        
                                        if (first) {
                                            if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:MODLAND_FTPHOST filesize:cur_db_entries[i][j].filesize filename:modFilename isMODLAND:1 usePrimaryAction:1]) {
                                                tooMuch=1;
                                                break;
                                            }
                                            first=0;
                                        } else {
                                            if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:MODLAND_FTPHOST filesize:cur_db_entries[i][j].filesize filename:modFilename isMODLAND:1 usePrimaryAction:2]) {
                                                tooMuch=1;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            if (tooMuch) break;
                        }
                        
                    } else {
                        cellValue=cur_db_entries[section][indexPath.row].label;
                        
                        //Check if an album was selected
                        if (cur_db_entries[section][indexPath.row].id_mod==-1) {//no mod : Album selcted
                            if (childController == nil) childController = [[RootViewControllerIphone alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
                            else {// Don't cache childviews
                            }
                            //set new title
                            childController.title = cellValue;	
                            // Set new depth
                            ((RootViewControllerIphone*)childController)->browse_depth = browse_depth+1;
                            ((RootViewControllerIphone*)childController)->browse_mode = browse_mode;
                            ((RootViewControllerIphone*)childController)->modland_browse_mode=modland_browse_mode;
                            ((RootViewControllerIphone*)childController)->playerButton=playerButton;
                            //Filetype & Author selected
                            if (mFiletypeID>=0) ((RootViewControllerIphone*)childController)->mFiletypeID = mFiletypeID;
                            if (mAuthorID>=0) ((RootViewControllerIphone*)childController)->mAuthorID = mAuthorID;
                            //Album selected
                            ((RootViewControllerIphone*)childController)->mAlbumID = cur_db_entries[section][indexPath.row].id_album;
                            
                            ((RootViewControllerIphone*)childController)->detailViewController=detailViewController;
                            ((RootViewControllerIphone*)childController)->downloadViewController=downloadViewController;
                            // And push the window
                            [self.navigationController pushViewController:childController animated:YES];
                            
                        } else { //File selected, start download is needed
                            NSString *filePath=[self getCompletePath:cur_db_entries[section][indexPath.row].id_mod];
                            NSString *modFilename=[self getModFilename:cur_db_entries[section][indexPath.row].id_mod];
                            NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
                            NSString *localPath=[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[section][indexPath.row].id_mod]];
                            mClickedPrimAction=(detailViewController.sc_DefaultAction.selectedSegmentIndex==0);
                            
                            if (cur_db_entries[section][indexPath.row].downloaded==1) {
                                if (detailViewController.sc_checkBeforeRedownload.selectedSegmentIndex==1) {
                                    FTPfilePath=[[NSString alloc] initWithString:filePath];
                                    FTPlocalPath=[[NSString alloc] initWithString:localPath];
                                    FTPftpPath=[[NSString alloc] initWithString:ftpPath];
                                    FTPfilename=[[NSString alloc] initWithString:modFilename];
                                    FTPfilesize=cur_db_entries[section][indexPath.row].filesize;
                                    alertAlreadyAvail = [[[UIAlertView alloc] initWithTitle:@"Warning" 
                                                                                    message:NSLocalizedString(@"File already available locally. Do you want to download it again?",@"") delegate:self cancelButtonTitle:NSLocalizedString(@"No",@"") otherButtonTitles:NSLocalizedString(@"Yes",@""),nil] autorelease];
                                    [alertAlreadyAvail show];
                                } else {
                                    if (mClickedPrimAction) {
                                        NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                                        NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                                        [array_label addObject:modFilename];
                                        [array_path addObject:localPath];
                                        [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                                        cur_db_entries[section][indexPath.row].rating=-1;
                                        if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                                        else [tableView reloadData];
                                    } else {
                                        if ([detailViewController add_to_playlist:localPath fileName:modFilename forcenoplay:(detailViewController.sc_DefaultAction.selectedSegmentIndex==1)]) {
                                            cur_db_entries[section][indexPath.row].rating=-1;
                                            if (detailViewController.sc_PlayerViewOnPlay.selectedSegmentIndex) [self goPlayer];
                                            else [tableView reloadData];
                                        }
                                    }
                                }
                            } else {
                                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                                mCurrentWinAskedDownload=1;
                                [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:MODLAND_FTPHOST filesize:cur_db_entries[section][indexPath.row].filesize filename:modFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                            }
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
- (void)freePlaylist {
    if (playlist) {
        for (int i=0;i<playlist->nb_entries;i++) {
            [playlist->label[i] release];
            [playlist->fullpath[i] release];
        }
        if (playlist->playlist_name) {
            [playlist->playlist_name release];
            playlist->playlist_name=nil;
        }
        if (playlist->playlist_id) {
            [playlist->playlist_id release];
            playlist->playlist_id=nil;
        }
        free(playlist);
        playlist=NULL;
    }
}
- (void)dealloc {
    [waitingView removeFromSuperview];
    [waitingView release];
    
    [currentPath release];
    if (mSearchText) {
        [mSearchText release];
        mSearchText=nil;
    }
    if (mFreePlaylist) [self freePlaylist];
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
    
    for (int i=0;i<search_dbHVSC_nb_entries;i++) {
        
    }
    
    if (indexTitles) [indexTitles release];
    if (indexTitlesDownload) [indexTitlesDownload release];
    
    if (mFileMngr) {
        [mFileMngr release];
        mFileMngr=nil;
    }
    
    [super dealloc];
}


@end