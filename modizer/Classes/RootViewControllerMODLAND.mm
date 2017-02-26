//
//  RootViewControllerMODLAND.mm
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

extern BOOL is_ios7;

#define GET_NB_ENTRIES 1
#define NB_MODLAND_ENTRIES 319746


#define PRI_SEC_ACTIONS_IMAGE_SIZE 40
#define ROW_HEIGHT 40

#define LIMITED_LIST_SIZE 1024

#include <sys/types.h>
#include <sys/sysctl.h>

#include <pthread.h>
extern pthread_mutex_t db_mutex;
//static int shouldFillKeys;
static int local_flag;
static volatile int mPopupAnimation=0;

#import "AppDelegate_Phone.h"
#import "RootViewControllerMODLAND.h"
#import "DetailViewControllerIphone.h"
#import "DownloadViewController.h"
#import "QuartzCore/CAAnimation.h"
#import "SettingsGenViewController.h"
extern volatile t_settings settings[MAX_SETTINGS];


@implementation RootViewControllerMODLAND

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

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex {
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
            
            int file_or_album=0;
            if (search_db) {
                if (search_db_hasFiles) file_or_album=1;
            } else {
                if (db_hasFiles) file_or_album=1;
            }
            csection=indexPath.section-1-file_or_album;
            
            if (csection>=0) {
                //display popup
                t_db_browse_entry **cur_db_entries;
                cur_db_entries=(search_db?search_db_entries:db_entries);
                
                NSString *str=[self getCompletePath:cur_db_entries[csection][crow].id_mod];
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
	search_db=0;  //reset to ensure search_db is not used by default
	
	db_nb_entries=0;
	search_db_nb_entries=0;
	
	search_db_hasFiles=0;
	db_hasFiles=0;
	
	mSearchText=nil;
	mCurrentWinAskedDownload=0;
	mClickedPrimAction=0;
	list=nil;
	keys=nil;
		
		if (browse_depth==1) { //browse mode menu
			sBar.frame=CGRectMake(0,0,0,0);
			sBar.hidden=TRUE;
			//get stats on nb of entries
			mNbFormatEntries=DBHelper::getNbFormatEntries();
			mNbAuthorEntries=DBHelper::getNbAuthorEntries();
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
			switch (modland_browse_mode) {
				case 0://Formats/Authors/Files
					if (browse_depth==2) [self fillKeysWithDB_fileType];
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
							 UNION SELECT count(1),1 FROM mod_author_album m,mod_album a WHERE m.id_author=%d AND m.id_album=a.id AND a.album like \"%%%s%%\"  AND m.id_author=a.id_author",authorID,[mSearchText UTF8String],authorID,[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT count(1),0 FROM mod_file WHERE id_author=%d AND id_album is null \
					 UNION SELECT count(1),1 FROM mod_author_album m,mod_album a WHERE m.id_author=%d AND m.id_album=a.id AND m.id_author=a.id_author",authorID,authorID);
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
								 WHERE m.id_author=%d AND m.id_album=a.id AND a.album like \"%%%s%%\"  AND m.id_author=a.id_author\
								 ORDER BY 1  COLLATE NOCASE",authorID,[mSearchText UTF8String],authorID,[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT filename,filesize,id,0 FROM mod_file \
						 WHERE id_author=%d AND id_album is null \
						 UNION SELECT a.album,a.num_files,a.id,1 FROM mod_author_album m,mod_album a \
						 WHERE m.id_author=%d AND m.id_album=a.id AND m.id_author=a.id_author\
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
							 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id AND a.album like \"%%%s%%\" AND m.id_author=a.id_author",authorID,filetypeID,[mSearchText UTF8String],authorID,filetypeID,[mSearchText UTF8String]);
		else sprintf(sqlStatement,"SELECT count(1),0 FROM mod_file \
					 WHERE id_author=%d AND id_type=%d AND id_album is null \
					 UNION SELECT count(1),1 FROM mod_type_author_album m,mod_album a \
					 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id AND m.id_author=a.id_author",authorID,filetypeID,authorID,filetypeID);
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
								 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id AND a.album like \"%%%s%%\"  AND m.id_author=a.id_author\
								 ORDER BY 1  COLLATE NOCASE",authorID,filetypeID,[mSearchText UTF8String],authorID,filetypeID,[mSearchText UTF8String]);
			else sprintf(sqlStatement,"SELECT filename,filesize,id,0 FROM mod_file \
						 WHERE id_author=%d AND id_type=%d AND id_album is null \
						 UNION SELECT a.album,m.num_files,a.id,1 FROM mod_type_author_album m,mod_album a \
						 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id  AND m.id_author=a.id_author\
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
    if (childController) [(RootViewControllerMODLAND*)childController refreshViewAfterDownload];
    else  {
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
    if (browse_depth>=2) return [indexTitles objectAtIndex:section];
    return nil;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    local_flag=0;
    
    if (browse_depth==0) return [keys count];
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
    return 28;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (browse_depth>=1) {//modland DB
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
    } else {
        NSDictionary *dictionary = [keys objectAtIndex:section];
        NSArray *array = [dictionary objectForKey:@"entries"];
        return [array count];
    }
}
- (NSArray *)sectionIndexTitlesForTableView:(UITableView *)tableView {
    if (browse_depth==0) return nil;
    if (mSearch) return nil;	
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
    if (browse_depth>=2) return indexTitles;
    return nil;
}

- (NSInteger)tableView:(UITableView *)tabView sectionForSectionIndexTitle:(NSString *)title atIndex:(NSInteger)index {
    if (mSearch) return -1;
    if (browse_depth==1) return -1; //Modland browse mode chooser
        if (index == 0) {
            [tabView setContentOffset:CGPointZero animated:NO];
            return NSNotFound;
        }
        return index;
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
    
    
    UITableViewCell *cell = [tabView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier] autorelease];
        
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
    } else {//modland
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
                topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
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
                    if (colFactor==0) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0f];
                    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                               0,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                               22);
                    
                    if (cur_db_entries[section][indexPath.row].downloaded==1) {
                        if (cur_db_entries[section][indexPath.row].rating==-1) {
                            DBHelper::getFileStatsDBmod(
                                                        cur_db_entries[section][indexPath.row].label,
                                                        [NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,
                                                         [self getCompleteLocalPath:cur_db_entries[section][indexPath.row].id_mod]],
                                                        &cur_db_entries[section][indexPath.row].playcount,
                                                        &cur_db_entries[section][indexPath.row].rating,
                                                        &cur_db_entries[section][indexPath.row].song_length,
                                                        &cur_db_entries[section][indexPath.row].channels_nb,
                                                        &cur_db_entries[section][indexPath.row].songs);
                            
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
                        bottomLabel.text=[NSString stringWithFormat:@"%dKB",cur_db_entries[section][indexPath.row].filesize/1024];
                        bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                                                       22,
                                                       tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                                       18);
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
                    
                } else if (cur_db_entries[section][indexPath.row].id_album>=0) {// ALBUM ?
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                                   18);
                    topLabel.textColor=[UIColor colorWithRed:0.8f green:0.6f blue:0.0f alpha:1.0f];
                    bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[section][indexPath.row].filesize>1?nbFiles:nb1File),cur_db_entries[section][indexPath.row].filesize];
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                } else if (cur_db_entries[section][indexPath.row].id_author>=0) {// AUTHOR ?
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                                   18);
                    topLabel.textColor=[UIColor colorWithRed:1.0f green:0.0f blue:0.8f alpha:1.0f];
                    bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[section][indexPath.row].filesize>1?nbFiles:nb1File),cur_db_entries[section][indexPath.row].filesize];
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;				
                } else  {
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
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
- (void)tableView:(UITableView *)tabView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath {
    
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        
        //delete entry
        
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
            DBHelper::deleteStatsFileDB(localpath);
            cur_db_entries[section][indexPath.row].downloaded=0;
            //delete local file
            [mFileMngr removeItemAtPath:localpath error:&err];
            //ask for a reload/redraw
            [tabView reloadData];
        
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }
}
- (NSIndexPath *)tableView:(UITableView *)tabView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath {
    
    return proposedDestinationIndexPath;
}
// Override to support rearranging the table view.
/*- (void)tableView:(UITableView *)tabView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath {
}*/

// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tabView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return NO;
}
- (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
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
                [detailViewController play_listmodules:(NSArray*)array_label start_index:(int)0 path:(NSArray*)array_path];
                if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                else [tableView reloadData];
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                mCurrentWinAskedDownload=1;
                
                NSString *modland_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
                NSRange nsr=[modland_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                if (nsr.location==NSNotFound) {
                    //HTTP
                    [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:cur_db_entries[section][indexPath.row].filesize isMODLAND:1 usePrimaryAction:1];
                } else {
                    //FTP
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:cur_db_entries[section][indexPath.row].filesize filename:modFilename isMODLAND:1 usePrimaryAction:1];
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
                    if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                    else [tableView reloadData];
                }
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                mCurrentWinAskedDownload=1;
                
                NSString *modland_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
                NSRange nsr=[modland_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                if (nsr.location==NSNotFound) {
                    //HTTP
                    [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:cur_db_entries[section][indexPath.row].filesize isMODLAND:1 usePrimaryAction:2];
                    
                } else {
                    //FTP
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:cur_db_entries[section][indexPath.row].filesize filename:modFilename isMODLAND:1 usePrimaryAction:2];
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
    
    if (browse_depth==0) {
        NSDictionary *dictionary = [keys objectAtIndex:indexPath.section];
        NSArray *array = [dictionary objectForKey:@"entries"];
        cellValue = [array objectAtIndex:indexPath.row];
        
            if (childController == nil) childController = [[RootViewControllerMODLAND alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
            else {			// Don't cache childviews
            }
            //set new title
            childController.title = cellValue;
            // Set new directory
            ((RootViewControllerMODLAND*)childController)->browse_depth = browse_depth+1;
            ((RootViewControllerMODLAND*)childController)->detailViewController=detailViewController;
            ((RootViewControllerMODLAND*)childController)->downloadViewController=downloadViewController;
            // And push the window
            [self.navigationController pushViewController:childController animated:YES];	
            [keys release];keys=nil;
            [list release];list=nil;
    } else {
         
            if (browse_depth==1) {
                if (childController == nil) childController = [[RootViewControllerMODLAND alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
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
                ((RootViewControllerMODLAND*)childController)->browse_depth = browse_depth+1;
                ((RootViewControllerMODLAND*)childController)->modland_browse_mode=modland_browse_mode;
                ((RootViewControllerMODLAND*)childController)->detailViewController=detailViewController;
                ((RootViewControllerMODLAND*)childController)->downloadViewController=downloadViewController;
                
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
                    
                    if (childController == nil) childController = [[RootViewControllerMODLAND alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                    else {// Don't cache childviews
                    }
                    //set new title
                    childController.title = cellValue;
                    // Set new depth
                    ((RootViewControllerMODLAND*)childController)->browse_depth = browse_depth+1;
                    ((RootViewControllerMODLAND*)childController)->modland_browse_mode=modland_browse_mode;
                    
                    switch (modland_browse_mode) {
                        case 0:		//Formats/Authors/Files
                            //Filetype selected
                            if (browse_depth==2) {
                                ((RootViewControllerMODLAND*)childController)->mFiletypeID = cur_db_entries[section][indexPath.row].id_type;
                            } else ((RootViewControllerMODLAND*)childController)->mFiletypeID = mFiletypeID;
                            //Author selected
                            if (browse_depth==3) {
                                ((RootViewControllerMODLAND*)childController)->mAuthorID = cur_db_entries[section][indexPath.row].id_author;
                            } else ((RootViewControllerMODLAND*)childController)->mAuthorID = mAuthorID;
                            if (browse_depth==4) {
                                ((RootViewControllerMODLAND*)childController)->mAlbumID = cur_db_entries[section][indexPath.row].id_album;
                            } else ((RootViewControllerMODLAND*)childController)->mAlbumID = mAlbumID;
                            break;
                        case 1:		//Authors/Formats/Files
                            //Author selected
                            if (browse_depth==2) {
                                ((RootViewControllerMODLAND*)childController)->mAuthorID = cur_db_entries[section][indexPath.row].id_author;
                            } else ((RootViewControllerMODLAND*)childController)->mAuthorID = mAuthorID;
                            //Filetype selected
                            if (browse_depth==3) {
                                ((RootViewControllerMODLAND*)childController)->mFiletypeID = cur_db_entries[section][indexPath.row].id_type;
                            } else ((RootViewControllerMODLAND*)childController)->mFiletypeID = mFiletypeID;
                            if (browse_depth==4) {
                                ((RootViewControllerMODLAND*)childController)->mAlbumID = cur_db_entries[section][indexPath.row].id_album;
                            } else ((RootViewControllerMODLAND*)childController)->mAlbumID = mAlbumID;
                            break;
                        case 2:		//Authors/Files
                            //Author selected
                            if (browse_depth==2) {
                                ((RootViewControllerMODLAND*)childController)->mAuthorID = cur_db_entries[section][indexPath.row].id_author;
                            } else ((RootViewControllerMODLAND*)childController)->mAuthorID = mAuthorID;
                            if (browse_depth==3) {
                                ((RootViewControllerMODLAND*)childController)->mAlbumID = cur_db_entries[section][indexPath.row].id_album;
                            } else ((RootViewControllerMODLAND*)childController)->mAlbumID = mAlbumID;
                            break;
                    }
                    
                    ((RootViewControllerMODLAND*)childController)->detailViewController=detailViewController;
                    ((RootViewControllerMODLAND*)childController)->downloadViewController=downloadViewController;
                    
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
                        if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==2) first=0;//enqueue only
                        
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
                                        
                                        
                                        NSString *modland_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
                                        NSRange nsr=[modland_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                                        if (first) {
                                            if (nsr.location==NSNotFound) {
                                                //HTTP
                                                [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:cur_db_entries[i][j].filesize isMODLAND:1 usePrimaryAction:1];
                                                
                                            } else {
                                                //FTP
                                                if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:cur_db_entries[i][j].filesize filename:modFilename isMODLAND:1 usePrimaryAction:1]) {
                                                    tooMuch=1;
                                                    break;
                                                }
                                            }
                                            first=0;
                                        } else {
                                            if (nsr.location==NSNotFound) {
                                                //HTTP
                                                [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:cur_db_entries[i][j].filesize isMODLAND:1 usePrimaryAction:2];
                                                
                                            } else {
                                                //FTP
                                                if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:cur_db_entries[i][j].filesize filename:modFilename isMODLAND:1 usePrimaryAction:2]) {
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
                        cellValue=cur_db_entries[section][indexPath.row].label;
                        
                        //Check if an album was selected
                        if (cur_db_entries[section][indexPath.row].id_mod==-1) {//no mod : Album selcted
                            if (childController == nil) childController = [[RootViewControllerMODLAND alloc]  initWithNibName:@"PlaylistViewController" bundle:[NSBundle mainBundle]];
                            else {// Don't cache childviews
                            }
                            //set new title
                            childController.title = cellValue;	
                            // Set new depth
                            ((RootViewControllerMODLAND*)childController)->browse_depth = browse_depth+1;
                            ((RootViewControllerMODLAND*)childController)->modland_browse_mode=modland_browse_mode;
                            //Filetype & Author selected
                            if (mFiletypeID>=0) ((RootViewControllerMODLAND*)childController)->mFiletypeID = mFiletypeID;
                            if (mAuthorID>=0) ((RootViewControllerMODLAND*)childController)->mAuthorID = mAuthorID;
                            //Album selected
                            ((RootViewControllerMODLAND*)childController)->mAlbumID = cur_db_entries[section][indexPath.row].id_album;
                            
                            ((RootViewControllerMODLAND*)childController)->detailViewController=detailViewController;
                            ((RootViewControllerMODLAND*)childController)->downloadViewController=downloadViewController;
                            // And push the window
                            [self.navigationController pushViewController:childController animated:YES];
                            
                        } else { //File selected, start download is needed
                            NSString *filePath=[self getCompletePath:cur_db_entries[section][indexPath.row].id_mod];
                            NSString *modFilename=[self getModFilename:cur_db_entries[section][indexPath.row].id_mod];
                            NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
                            NSString *localPath=[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[section][indexPath.row].id_mod]];
                            mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
                            
                            if (cur_db_entries[section][indexPath.row].downloaded==1) {
                                    if (mClickedPrimAction) {
                                        NSMutableArray *array_label = [[[NSMutableArray alloc] init] autorelease];
                                        NSMutableArray *array_path = [[[NSMutableArray alloc] init] autorelease];
                                        [array_label addObject:modFilename];
                                        [array_path addObject:localPath];
                                        [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                                        cur_db_entries[section][indexPath.row].rating=-1;
                                        if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                                        else [tabView reloadData];
                                    } else {
                                        if ([detailViewController add_to_playlist:localPath fileName:modFilename forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)]) {
                                            cur_db_entries[section][indexPath.row].rating=-1;
                                            if (settings[GLOB_PlayerViewOnPlay].detail.mdz_boolswitch.switch_value) [self goPlayer];
                                            else [tabView reloadData];
                                        }
                                    }                                
                            } else {
                                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                                mCurrentWinAskedDownload=1;
                                
                                NSString *modland_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
                                NSRange nsr=[modland_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                                if (nsr.location==NSNotFound) {
                                    //HTTP
                                    [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:cur_db_entries[section][indexPath.row].filesize isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                                    
                                } else {
                                    //FTP
                                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:cur_db_entries[section][indexPath.row].filesize filename:modFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
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
    
    
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++)
            [db_entries_data[i].label release];
        free(db_entries_data);
    }
    if (search_db_nb_entries) {
        free(search_db_entries_data);
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
