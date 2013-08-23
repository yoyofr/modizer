//
//  RootViewControllerLocalBrowser.m
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define LIMITED_LIST_SIZE 1024

#include <sys/types.h>
#include <sys/sysctl.h>

#include "gme.h"

//SID2
#import "SidTune.h"


#include "unzip.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;
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


#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/xattr.h>

static volatile int mUpdateToNewDB;
static volatile int mDatabaseCreationInProgress;
static volatile int db_checked=0;


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
    
    UIButton *btn = [[UIButton alloc] initWithFrame: CGRectMake(0, 0, 61, 31)];
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
    
    
    waitingView = [[UIView alloc] initWithFrame:CGRectMake(self.view.bounds.size.width/2-40,self.view.bounds.size.height/2-40,80,80)];
	waitingView.backgroundColor=[UIColor blackColor];//[UIColor colorWithRed:0 green:0 blue:0 alpha:0.8f];
	waitingView.opaque=TRUE;
	waitingView.hidden=TRUE;
	waitingView.layer.cornerRadius=20;
	UIActivityIndicatorView *indView=[[UIActivityIndicatorView alloc] initWithFrame:CGRectMake(20,20,37,37)];
	indView.activityIndicatorViewStyle=UIActivityIndicatorViewStyleWhiteLarge;
	[waitingView addSubview:indView];
	[indView startAnimating];
	[indView autorelease];
	[self.view addSubview:waitingView];
	
	[super viewDidLoad];
	
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
            
            for (int i=0;i<sidtune_info.songs;i++){
                SidTuneInfo s_info;
                file=nil;
                mSidTune->selectSong(i);
                s_info=mSidTune->getInfo();
                
                if (s_info.infoString[0][0]) {
                    file=[NSString stringWithFormat:@"%.3d-%s",i,s_info.infoString[0]];
                } else {
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
                        
                        if (s_info.infoString[0][0]) {
                            file=[NSString stringWithFormat:@"%.3d-%s",i,s_info.infoString[0]];
                        } else {
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
                    }
                }
            }
            if (mSidTune) {delete mSidTune;mSidTune=NULL;}
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
                if (fex_open_type( &fex, path, ftype )) {
                    NSLog(@"cannot fex open : %s",path);
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
    /////////////
    if (detailViewController.mShouldHaveFocus) {
        detailViewController.mShouldHaveFocus=0;
        [self.navigationController pushViewController:detailViewController animated:(mSlowDevice?NO:YES)];
    } else {
        [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
        [self fillKeys];
        [tableView reloadData];
        [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
    }
    [super viewWillAppear:animated];	
    
}
-(void) refreshMODLANDView {
}

- (void)viewDidAppear:(BOOL)animated {
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
    [super viewDidAppear:animated];		
}

- (void)viewDidDisappear:(BOOL)animated {
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
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

#pragma mark -
#pragma mark Table view data source

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (mSearch) return nil;
    
    if (section==0) return nil;
    if (section==1) return @"";
    if ((search_local?search_local_entries_count[section-2]:local_entries_count[section-2])) {
        return [indexTitlesSpace objectAtIndex:section];
    } else return nil;
    return nil;
}
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
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        
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
        bottomLabel.textColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:1.0];
        bottomLabel.highlightedTextColor = [UIColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1.0];
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
    [self.navigationController pushViewController:detailViewController animated:(mSlowDevice?NO:YES)];
}

#pragma mark -
#pragma mark Table view delegate
- (void) primaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[[[sender superview] superview] center]];
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];                
    
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
    
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
    
    
}
- (void) secondaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[[[sender superview] superview] center]];
    t_local_browse_entry **cur_local_entries=(search_local?search_local_entries:local_entries);
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];                
    
    
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
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
}


- (void) accessoryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[[[sender superview] superview] center]];
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
            [tabView reloadData];
            
            [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
        }
    } else {
        cellValue=cur_local_entries[section][indexPath.row].label;
        
        if (cur_local_entries[section][indexPath.row].type==0) { //Directory selected : change current directory
            
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            
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
            
            
            [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
            //				[childController autorelease];
        } else if (((cur_local_entries[section][indexPath.row].type==2)||(cur_local_entries[section][indexPath.row].type==3))&&(mAccessoryButton)) { //Archive selected or multisongs: display files inside
            
            [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
            
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
            
            
            [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
            //				[childController autorelease];
        } else {  //File selected
            
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