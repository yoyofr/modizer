//
//  RootViewController.m
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#define NB_HVSC_ENTRIES 40400


#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define ROW_HEIGHT 40

#define LIMITED_LIST_SIZE 1024

#include <sys/types.h>
#include <sys/sysctl.h>

#include "gme.h"

#include "unzip.h"

#include <pthread.h>
extern pthread_mutex_t db_mutex;
//static int shouldFillKeys;
static int local_flag;
static volatile int mPopupAnimation=0;

static NSFileManager *mFileMngr;

#import "AppDelegate_Phone.h"
#import "RootViewControllerHVSC.h"
#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "WebBrowser.h"
#import "QuartzCore/CAAnimation.h"

@implementation RootViewControllerHVSC

@synthesize detailViewController;
@synthesize downloadViewController;
@synthesize tabView,sBar;
@synthesize list;
@synthesize keys;
@synthesize currentPath;
@synthesize childController;
@synthesize playerButton;
@synthesize mSearchText;

#pragma mark -
#pragma mark View lifecycle

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
	
	search_dbHVSC=0;  //reset to ensure search_dbHVSC is not used by default
	
	dbHVSC_nb_entries=0;
	search_dbHVSC_nb_entries=0;
	
	search_dbHVSC_hasFiles=0;
	dbHVSC_hasFiles=0;
	
	mSearchText=nil;
	mCurrentWinAskedDownload=0;
	mClickedPrimAction=0;
	list=nil;
	keys=nil;
	
	if (browse_depth==MENU_COLLECTIONS_ROOTLEVEL) {
#ifdef GET_NB_ENTRIES
		mNbHVSCFileEntries=DBHelper::getNbHVSCFilesEntries();
#else
		mNbHVSCFileEntries=NB_HVSC_ENTRIES;
#endif
	}
	
	self.navigationItem.rightBarButtonItem = playerButton; //self.editButtonItem;
	
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
        if (shouldFillKeys&&(browse_depth>0)) {
            
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
    if (childController) [(RootViewControllerHVSC*)childController refreshMODLANDView];
    else {
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

#pragma mark -
#pragma mark Table view data source

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    if (browse_depth==0) return nil;
    if (mSearch) return nil;	
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
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    local_flag=0;
    
    if (browse_depth==0) return [keys count];
        //Check if "Get all entries" has to be displayed
        if (search_dbHVSC) {
            if (search_dbHVSC_hasFiles) return 28+1;
            return 28;
        } else {
            if (dbHVSC_hasFiles) return 28+1;
            return 28;
        }
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
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
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    if (browse_depth==0) return nil;
    if (mSearch) return nil;	
        if (search_dbHVSC) {
            if (search_dbHVSC_hasFiles) return indexTitlesDownload;
        } else {
            if (dbHVSC_hasFiles) return indexTitlesDownload;
        }
        return indexTitles;
}

- (NSInteger)tableView:(UITableView *)tableView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    if (mSearch) return -1;
        if (index == 0) {
            [tableView setContentOffset:CGPointZero animated:NO];
            return NSNotFound;
        }
        return index;
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
    } else { //HVSC
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
    }
    
    topLabel.text = cellValue;
    
    return cell;
}

// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        
        //delete entry
        
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
        
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
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
    
    [[super tableView] selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];                
    
    
    if (browse_depth==0) {
        
    } else {
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
        
    }
    
    [self performSelectorInBackground:@selector(hideWaiting) withObject:nil];
    
    
}
- (void) secondaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [[super tableView] indexPathForRowAtPoint:[[[sender superview] superview] center]];
    
    [[super tableView] selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];                
    
    
    if (browse_depth==0) {
    } else {
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
    
    if (browse_depth==0) {        
    } else {
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
                
                int *cur_db_entries_count=(search_dbHVSC?search_dbHVSC_entries_count:dbHVSC_entries_count);
                
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
                    
                    if (childController == nil) childController = [[RootViewControllerHVSC alloc]  initWithNibName:@"RootViewController" bundle:[NSBundle mainBundle]];
                    else {// Don't cache childviews
                    }
                    
                    childController.title = cur_db_entries[section][indexPath.row].label;
                    // Set new depth
                    ((RootViewControllerHVSC*)childController)->browse_depth = browse_depth+1;
                    ((RootViewControllerHVSC*)childController)->playerButton=playerButton;
                    ((RootViewControllerHVSC*)childController)->detailViewController=detailViewController;
                    ((RootViewControllerHVSC*)childController)->downloadViewController=downloadViewController;
                    
                    ((RootViewControllerHVSC*)childController)->mDir1 = mDir1;
                    ((RootViewControllerHVSC*)childController)->mDir2 = mDir2;
                    ((RootViewControllerHVSC*)childController)->mDir3 = mDir3;
                    ((RootViewControllerHVSC*)childController)->mDir4 = mDir4;
                    ((RootViewControllerHVSC*)childController)->mDir5 = mDir5;
                    
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
    
    [super dealloc];
}


@end