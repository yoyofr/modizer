//
//  RootViewControllerHVSC.mm
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

extern BOOL is_ios7;

#define GET_NB_ENTRIES 1
#define NB_HVSC_ENTRIES 43856


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

#import "AppDelegate_Phone.h"
#import "RootViewControllerHVSC.h"
#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "WebBrowser.h"
#import "QuartzCore/CAAnimation.h"
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];

@implementation RootViewControllerHVSC

@synthesize mFileMngr;
@synthesize detailViewController;
@synthesize downloadViewController;
@synthesize tableView,sBar;
@synthesize list;
@synthesize keys;
@synthesize currentPath;
@synthesize childController;
@synthesize mSearchText;
@synthesize popTipView;

#pragma mark -
#pragma mark View lifecycle

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
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

-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer {
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    
    NSIndexPath *indexPath = [self.tableView indexPathForRowAtPoint:p];
    if (browse_depth>1) {
        if (indexPath != nil) {
            if ((gestureRecognizer.state==UIGestureRecognizerStateBegan)||(gestureRecognizer.state==UIGestureRecognizerStateChanged)) {
                //                    NSLog(@"long press on table view at %d/%d", indexPath.section,indexPath.row);
                int crow=indexPath.row;
                int csection;
                
                int download_all=0;
                if (search_dbHVSC) {
                    if (search_dbHVSC_hasFiles) download_all=1;
                } else {
                    if (dbHVSC_hasFiles) download_all=1;
                }
                csection=indexPath.section-1-download_all;
                
                if (csection>=0) {
                    //display popup
                    t_dbHVSC_browse_entry **cur_db_entries;
                    cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
                    
                    NSString *str=cur_db_entries[csection][crow].fullpath;
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
}

#pragma mark CMPopTipViewDelegate methods
- (void)popTipViewWasDismissedByUser:(CMPopTipView *)_popTipView {
    // User can tap CMPopTipView to dismiss it
    self.popTipView = nil;
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
    [lpgr release];
    
	
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
	
	if (browse_depth==0) {
#ifdef GET_NB_ENTRIES
		mNbHVSCFileEntries=DBHelper::getNbHVSCFilesEntries();
#else
		mNbHVSCFileEntries=NB_HVSC_ENTRIES;
#endif
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
            [self shortWait];
            
            [self fillKeys];
            [tableView reloadData];
            [self hideWaiting];
        } else {
            [self fillKeys];
            [tableView reloadData];
        }
    }
    [super viewWillAppear:animated];	
    
}

-(void) refreshViewAfterDownload {
    if (childController) [(RootViewControllerHVSC*)childController refreshViewAfterDownload];
    else {
        shouldFillKeys=1;
        [self fillKeys];
        [tableView reloadData];
    }
}

- (void)checkCreate:(NSString *)filePath {
    NSString *completePath=[NSString stringWithFormat:@"%@/%@",NSHomeDirectory(),filePath];
    NSError *err;
    [mFileMngr createDirectoryAtPath:completePath withIntermediateDirectories:TRUE attributes:nil error:&err];	
}

- (void)viewDidAppear:(BOOL)animated {        
    [self hideWaiting];

    [super viewDidAppear:animated];		
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
    NSString *nbFiles=NSLocalizedString(@"%d files.",@"");	
    NSString *nb1File=NSLocalizedString(@"1 file.",@"");	
    
    
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
        //cell.selectionStyle=UITableViewCellSelectionStyleGray;
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
            topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
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
                if (colFactor==0) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0];
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                           22);
                if (cur_db_entries[section][indexPath.row].downloaded==1) {
                    if (cur_db_entries[section][indexPath.row].rating==-1) {
                        DBHelper::getFileStatsDBmod(
                                                    cur_db_entries[section][indexPath.row].label,
                                                    [NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[section][indexPath.row].fullpath],
                                                    &cur_db_entries[section][indexPath.row].playcount,
                                                    &cur_db_entries[section][indexPath.row].rating,
                                                    &cur_db_entries[section][indexPath.row].song_length,
                                                    &cur_db_entries[section][indexPath.row].channels_nb,
                                                    &cur_db_entries[section][indexPath.row].songs);
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
                        bottomStr=[NSString stringWithFormat:@"%@|%02dch",bottomStr,cur_db_entries[section][indexPath.row].channels_nb];
                    else bottomStr=[NSString stringWithFormat:@"%@|--ch",bottomStr];						
                    if (cur_db_entries[section][indexPath.row].songs) {
                        if (cur_db_entries[section][indexPath.row].songs==1) bottomStr=[NSString stringWithFormat:@"%@|1 song",bottomStr];
                        else bottomStr=[NSString stringWithFormat:@"%@|%d songs",bottomStr,cur_db_entries[section][indexPath.row].songs];
                    }
                    else bottomStr=[NSString stringWithFormat:@"%@|- song",bottomStr];		   						
                    bottomStr=[NSString stringWithFormat:@"%@|Pl:%d",bottomStr,cur_db_entries[section][indexPath.row].playcount];
                    
                    bottomLabel.text=bottomStr;
                    
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                                   18);
                } else {
                    /*bottomLabel.text=[NSString stringWithFormat:@"%dKB",cur_db_entries[indexPath.section-1][indexPath.row].filesize/1024];
                     bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                     22,
                     tabView.bounds.size.width -1.0 * cell.indentationWidth-32-34-34-60,
                     18);*/
                }
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
                actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                actionView.enabled=YES;
                actionView.hidden=NO;
                
            } else { // DIR
                bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                               22,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                               18);
                bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[section][indexPath.row].filesize>1?nbFiles:nb1File),cur_db_entries[section][indexPath.row].filesize];
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth- 32,
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
- (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    
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
        DBHelper::deleteStatsFileDB(fullpath);
            cur_db_entries[section][indexPath.row].downloaded=0;
            //delete local file
            [mFileMngr removeItemAtPath:fullpath error:&err];
            //ask for a reload/redraw
            [tabView reloadData];
        
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}

- (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
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
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
    
    
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
                    [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                    if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                    else [tableView reloadData];
                } else {
                    [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                    mCurrentWinAskedDownload=1;
                    
                    NSString *hvsc_url=[NSString stringWithFormat:@"%s",settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text];
                    NSRange nsr=[hvsc_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                    if (nsr.location==NSNotFound) {
                        //HTTP
                        [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",hvsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:1];
                    } else {
                        //FTP
                        [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[hvsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:1];
                    }
                }
            }
        
    }
    
    [self hideWaiting];
    
    
}
- (void) secondaryActionTapped: (UIButton*) sender {
    NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    
    [tableView selectRowAtIndexPath:indexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self performSelectorInBackground:@selector(showWaiting) withObject:nil];
    [self shortWait];
    
    
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
                    if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                    else [tableView reloadData];
                } else {
                    [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                    mCurrentWinAskedDownload=1;
                    
                    NSString *hvsc_url=[NSString stringWithFormat:@"%s",settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text];
                    NSRange nsr=[hvsc_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                    if (nsr.location==NSNotFound) {
                        //HTTP
                        [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",hvsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                    } else {
                        //FTP
                        [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[hvsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                    }
                    
                    //[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:HVSC_FTPHOST filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                    //[downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",HVSC_HTTPHOST,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
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
                if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==2) first=0;//enqueue only
                
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
                                mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
                                
                                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                                mCurrentWinAskedDownload=1;
                                
                                NSString *hvsc_url=[NSString stringWithFormat:@"%s",settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text];
                                NSRange nsr=[hvsc_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                                
                                if (first) {
                                    if (nsr.location==NSNotFound) {
                                        //HTTP
                                        if (
                                         [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",hvsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:1]
                                            ) {
                                            tooMuch=1;
                                            break;
                                        }
                                    } else {
                                        //FTP
                                        if (
                                            [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[hvsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:1]
                                            ) {
                                            tooMuch=1;
                                            break;
                                        }
                                    }
                                    
                                    first=0;
                                } else {
                                    if (nsr.location==NSNotFound) {
                                        //HTTP
                                        if (
                                            [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",hvsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:2]
                                            ) {
                                            tooMuch=1;
                                            break;
                                        }
                                    } else {
                                        //FTP
                                        if (
                                            [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[hvsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:2]

                                            ) {
                                            tooMuch=1;
                                            break;
                                        }
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
                    mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
                    
                    if (cur_db_entries[section][indexPath.row].downloaded==1) {
                        if (mClickedPrimAction) {
                            NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                            NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                            [array_label addObject:sidFilename];
                            [array_path addObject:localPath];
                            cur_db_entries[section][indexPath.row].rating=-1;
                            [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                            if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                            else [tabView reloadData];
                        } else {
                            if ([detailViewController add_to_playlist:localPath fileName:sidFilename forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)]) {
                                cur_db_entries[section][indexPath.row].rating=-1;
                                if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                                else [tabView reloadData];
                            }
                        }
                    } else {
                        [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                        mCurrentWinAskedDownload=1;
                        
                        
                        NSString *hvsc_url=[NSString stringWithFormat:@"%s",settings[ONLINE_HVSC_CURRENT_URL].detail.mdz_msgbox.text];
                        NSRange nsr=[hvsc_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                        if (nsr.location==NSNotFound) {
                            //HTTP
                            [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",hvsc_url,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                        } else {
                            //FTP
                            [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[hvsc_url substringFromIndex:6] filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                        }
                        //[downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:HVSC_FTPHOST filesize:-1 filename:sidFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                        //[downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",HVSC_HTTPHOST,ftpPath] fileName:sidFilename filePath:localPath filesize:-1 isMODLAND:1 usePrimaryAction:mClickedPrimAction];
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
                    
                    if (childController == nil) childController = [[RootViewControllerHVSC alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    else {// Don't cache childviews
                    }
                    
                    childController.title = cur_db_entries[section][indexPath.row].label;
                    // Set new depth
                    ((RootViewControllerHVSC*)childController)->browse_depth = browse_depth+1;
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
