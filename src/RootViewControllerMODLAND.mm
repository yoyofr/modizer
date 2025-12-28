//
//  RootViewControllerMODLAND.mm
//  modizer1
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//

#define RATING_IMG(a) ( (a==5?2:(a?1:0)) )

#define GET_NB_ENTRIES 1

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

#import "TTFadeAnimator.h"
#import "ModizFileHelper.h"


@implementation RootViewControllerMODLAND

@synthesize mFileMngr;
@synthesize detailViewController;
@synthesize downloadViewController;
@synthesize tableView,sBar;
@synthesize currentPath;
@synthesize childController;
@synthesize mSearchText;
@synthesize popTipView;
@synthesize repeatTimer,activeKey;

#pragma mark -
#pragma mark Search functions
#include "SearchCommonFunctions.h"

#pragma mark -
#pragma mark Miniplayer functions
#include "MiniPlayerImplementTableView.h"

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
	NSString *machine = [[NSString alloc] initWithUTF8String:(const char*)name];
	
	// Done with this
	free(name);
	
	return machine;
}

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
            
            int file_or_album=0;
            if (search_db) {
                if (search_db_hasFiles) file_or_album=1;
            } else {
                if (db_hasFiles) file_or_album=1;
            }
            crow-=file_or_album;
            
            if (crow>=0) {
                //display popup
                t_db_browse_entry *cur_db_entries;
                cur_db_entries=(search_db?search_db_entries:db_entries);
                
                NSString *str=[self getCompletePath:cur_db_entries[crow].id_mod];
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
    
    wasMiniPlayerOn=([detailViewController mPlaylist_size]>0?true:false);
    miniplayerVC=nil;
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
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
	search_db=0;  //reset to ensure search_db is not used by default
	
    db_entries=NULL;
    search_db_entries=NULL;
    
	db_nb_entries=0;
	search_db_nb_entries=0;
	
	search_db_hasFiles=0;
	db_hasFiles=0;
	
	mSearchText=nil;
	mCurrentWinAskedDownload=0;
	mClickedPrimAction=0;
		
		if (browse_depth==1) { //browse mode menu
			sBar.frame=CGRectMake(0,0,0,0);
			sBar.hidden=TRUE;
			//get stats on nb of entries
			mNbFormatEntries=DBHelper::getNbFormatEntries();
			mNbAuthorEntries=DBHelper::getNbAuthorEntries();
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
    
    if (search_db_nb_entries) {
        for (int j=0;j<search_db_entries_count;j++) {
            search_db_entries[j].label=nil;
        }
            search_db_entries=NULL;
        search_db_nb_entries=0;
        free(search_db_entries_data);
    }
    
	db_hasFiles=search_db_hasFiles=0;
	
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
        if (search_db_nb_entries) {
            for (int j=0;j<search_db_entries_count;j++) {
                search_db_entries[j].label=nil;
            }
                search_db_entries=NULL;
            
            search_db_nb_entries=0;
            free(search_db_entries_data);
        }
        
		search_db_entries_data=(t_db_browse_entry*)calloc(db_nb_entries,sizeof(t_db_browse_entry));
		
			search_db_entries_count=0;
			if (db_entries_count) search_db_entries=search_db_entries_data;
			for (int j=0;j<db_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:db_entries[j].label]) {
					search_db_entries[search_db_entries_count].label=db_entries[j].label;
					search_db_entries[search_db_entries_count].downloaded=db_entries[j].downloaded;
					search_db_entries[search_db_entries_count].rating=db_entries[j].rating;
					search_db_entries[search_db_entries_count].playcount=db_entries[j].playcount;
					search_db_entries[search_db_entries_count].id_type=db_entries[j].id_type;
					
					search_db_entries[search_db_entries_count].id_author=search_db_entries[search_db_entries_count].id_album=search_db_entries[search_db_entries_count].id_mod=-1;
					search_db_entries[search_db_entries_count].filesize=db_entries[j].filesize;
					search_db_entries_count++;
					search_db_nb_entries++;
				}
			}
		return;
	}
	
	pthread_mutex_lock(&db_mutex);
	
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].label=nil;
        }
        for (int j=0;j<db_entries_count;j++) {
                db_entries[j].label=nil;
            }
            db_entries=NULL;
        free(db_entries_data);
    }
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have
		if (mSearch) snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_type t,mod_type_author ta\
							 WHERE ta.id_author=%d AND ta.id_type=t.id AND t.filetype like \"%%%s%%\"",authorID,[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_type t,mod_type_author ta\
					 WHERE ta.id_author=%d AND ta.id_type=t.id",authorID);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)calloc(db_nb_entries,sizeof(t_db_browse_entry));

			db_entries_index=0;
				db_entries_count=0;
				db_entries=db_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT t.filetype,ta.num_files,t.id FROM mod_type t,mod_type_author ta\
								 WHERE ta.id_author=%d AND ta.id_type=t.id AND t.filetype like \"%%%s%%\" ORDER BY t.filetype COLLATE NOCASE",authorID,[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT t.filetype,ta.num_files,t.id FROM mod_type t,mod_type_author ta\
						 WHERE ta.id_author=%d AND ta.id_type=t.id ORDER BY t.filetype COLLATE NOCASE",authorID);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[db_entries_count].filesize=sqlite3_column_int(stmt, 1);
					db_entries[db_entries_count].id_type=sqlite3_column_int(stmt, 2);
					db_entries[db_entries_count].id_author=authorID;
					db_entries[db_entries_count].id_album=db_entries[db_entries_count].id_mod=-1;
					db_entries[db_entries_count].downloaded=-1;
					db_entries[db_entries_count].rating=-1;
					db_entries[db_entries_count].playcount=-1;
					db_entries_count++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_fileType{
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
    
    if (search_db_nb_entries) {
        for (int j=0;j<search_db_entries_count;j++) {
            search_db_entries[j].label=nil;
        }
            search_db_entries=NULL;
        search_db_nb_entries=0;
        free(search_db_entries_data);
    }
    
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
				
		search_db_entries_data=(t_db_browse_entry*)calloc(db_nb_entries,sizeof(t_db_browse_entry));
		
			search_db_entries_count=0;
			if (db_entries_count) search_db_entries=search_db_entries_data;
			for (int j=0;j<db_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:db_entries[j].label]) {
					search_db_entries[search_db_entries_count].label=db_entries[j].label;
					search_db_entries[search_db_entries_count].downloaded=db_entries[j].downloaded;
					search_db_entries[search_db_entries_count].rating=db_entries[j].rating;
					search_db_entries[search_db_entries_count].playcount=db_entries[j].playcount;
					search_db_entries[search_db_entries_count].id_type=db_entries[j].id_type;
					
					search_db_entries[search_db_entries_count].id_author=search_db_entries[search_db_entries_count].id_album=search_db_entries[search_db_entries_count].id_mod=-1;
					search_db_entries[search_db_entries_count].filesize=db_entries[j].filesize;
					search_db_entries_count++;
					search_db_nb_entries++;
				}
			}
		return;
	}
	pthread_mutex_lock(&db_mutex);
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].label=nil;
        }
        for (int j=0;j<db_entries_count;j++) {
            db_entries[j].label=nil;
        }
            db_entries=NULL;
        free(db_entries_data);
    }
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(1) FROM mod_type WHERE filetype LIKE \"%%%s%%\"",[mSearchText UTF8String]);
		else  snprintf(sqlStatement,1024,"SELECT COUNT(1) FROM mod_type");
		
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)calloc(db_nb_entries,sizeof(t_db_browse_entry));
			db_entries_index=0;
				db_entries_count=0;
				db_entries=db_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT filetype,num_files,id FROM mod_type WHERE filetype LIKE \"%%%s%%\" ORDER BY filetype COLLATE NOCASE",[mSearchText UTF8String]);
			else  snprintf(sqlStatement,1024,"SELECT filetype,num_files,id FROM mod_type ORDER BY filetype COLLATE NOCASE");
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
                    
                    db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[db_entries_count].filesize=sqlite3_column_int(stmt, 1);
					db_entries[db_entries_count].id_type=sqlite3_column_int(stmt, 2);
					db_entries[db_entries_count].id_author=db_entries[db_entries_count].id_album=db_entries[db_entries_count].id_mod=-1;
					db_entries[db_entries_count].downloaded=-1;		
					db_entries[db_entries_count].rating=-1;
					db_entries[db_entries_count].playcount=-1;
					db_entries_count++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_fileAuthor{
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
    
    if (search_db_nb_entries) {
        for (int j=0;j<search_db_entries_count;j++) {
            search_db_entries[j].label=nil;
        }
            search_db_entries=NULL;
        search_db_nb_entries=0;
        free(search_db_entries_data);
    }
    
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		search_db_entries_data=(t_db_browse_entry*)calloc(db_nb_entries,sizeof(t_db_browse_entry));
		
			search_db_entries_count=0;
			if (db_entries_count) search_db_entries=search_db_entries_data;
			for (int j=0;j<db_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:db_entries[j].label]) {
					search_db_entries[search_db_entries_count].label=db_entries[j].label;
					search_db_entries[search_db_entries_count].id_author=db_entries[j].id_author;
					search_db_entries[search_db_entries_count].downloaded=db_entries[j].downloaded;
					search_db_entries[search_db_entries_count].rating=db_entries[j].rating;
					search_db_entries[search_db_entries_count].playcount=db_entries[j].playcount;
					search_db_entries[search_db_entries_count].id_type=search_db_entries[search_db_entries_count].id_album=search_db_entries[search_db_entries_count].id_mod=-1;
					search_db_entries[search_db_entries_count].filesize=db_entries[j].filesize;
					search_db_entries_count++;
					search_db_nb_entries++;
				}
			}
		return;
	}
	pthread_mutex_lock(&db_mutex);
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].label=nil;
        }
            for (int j=0;j<db_entries_count;j++) {
                db_entries[j].label=nil;
            }
            db_entries=NULL;
        free(db_entries_data);
    }
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_author WHERE author LIKE \"%%%s%%\"",[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_author");
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)calloc(db_nb_entries,sizeof(t_db_browse_entry));
            
			db_entries_index=0;
				db_entries_count=0;
				db_entries=db_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT author,num_files,id FROM mod_author WHERE author LIKE \"%%%s%%\" ORDER BY author COLLATE NOCASE",[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT author,num_files,id FROM mod_author ORDER BY author COLLATE NOCASE");
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					if (db_entries[db_entries_count].label==nil) db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[db_entries_count].filesize=sqlite3_column_int(stmt, 1);
					db_entries[db_entries_count].id_author=sqlite3_column_int(stmt, 2);
					db_entries[db_entries_count].id_type=db_entries[db_entries_count].id_album=db_entries[db_entries_count].id_mod=-1;
					db_entries[db_entries_count].downloaded=-1;
					db_entries[db_entries_count].rating=-1;
					db_entries[db_entries_count].playcount=-1;
					db_entries_count++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_fileAuthor:(int)filetypeID{
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
    
    if (search_db_nb_entries) {
            for (int j=0;j<search_db_entries_count;j++) {
                search_db_entries[j].label=nil;
            }
            search_db_entries=NULL;
        search_db_nb_entries=0;
        free(search_db_entries_data);
    }
    
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		search_db_entries_data=(t_db_browse_entry*)calloc(db_nb_entries,sizeof(t_db_browse_entry));
		
			search_db_entries_count=0;
			if (db_entries_count) search_db_entries=search_db_entries_data;
			for (int j=0;j<db_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:db_entries[j].label]) {
					search_db_entries[search_db_entries_count].label=db_entries[j].label;
					
					search_db_entries[search_db_entries_count].id_author=db_entries[j].id_author;
					search_db_entries[search_db_entries_count].downloaded=db_entries[j].downloaded;
					search_db_entries[search_db_entries_count].rating=db_entries[j].rating;
					search_db_entries[search_db_entries_count].playcount=db_entries[j].playcount;
					
					search_db_entries[search_db_entries_count].id_type=filetypeID;
					search_db_entries[search_db_entries_count].id_album=search_db_entries[search_db_entries_count].id_mod=-1;
					search_db_entries[search_db_entries_count].filesize=db_entries[j].filesize;
					search_db_entries_count++;
					search_db_nb_entries++;
				}
			}
		return;
	}
	pthread_mutex_lock(&db_mutex);
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].label=nil;
        }
            for (int j=0;j<db_entries_count;j++) {
                db_entries[j].label=nil;
            }
            db_entries=NULL;
        free(db_entries_data);
    }
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(1) FROM mod_author a,mod_type_author m WHERE m.id_type=%d AND m.id_author=a.id AND a.author LIKE \"%%%s%%\"",filetypeID,[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT COUNT(1) FROM mod_type_author m WHERE m.id_type=%d",filetypeID);
		
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)calloc(db_nb_entries,sizeof(t_db_browse_entry));
			db_entries_index=0;
				db_entries_count=0;
				db_entries=db_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT a.author,m.num_files,a.id FROM mod_author a,mod_type_author m WHERE m.id_type=%d AND m.id_author=a.id AND a.author LIKE \"%%%s%%\" ORDER BY a.author COLLATE NOCASE",filetypeID,[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT a.author,m.num_files,a.id FROM mod_type_author m,mod_author a WHERE m.id_type=%d AND m.id_author=a.id ORDER BY a.author COLLATE NOCASE",filetypeID);
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
            
            
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					
					db_entries[db_entries_count].filesize=sqlite3_column_int(stmt, 1);
					db_entries[db_entries_count].id_author=sqlite3_column_int(stmt, 2);
					db_entries[db_entries_count].id_type=filetypeID;
					db_entries[db_entries_count].id_album=db_entries[db_entries_count].id_mod=-1;
					db_entries[db_entries_count].downloaded=-1;		
					db_entries[db_entries_count].rating=-1;
					db_entries[db_entries_count].playcount=-1;
					db_entries_count++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);		
}
-(void) fillKeysWithDB_albumORfilename:(int)authorID {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
    
    if (search_db_nb_entries) {
            for (int j=0;j<search_db_entries_count;j++) {
                search_db_entries[j].label=nil;
            }
            search_db_entries=NULL;
        search_db_nb_entries=0;
        free(search_db_entries_data);
    }
    
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		search_db_entries_data=(t_db_browse_entry*)calloc(db_nb_entries,sizeof(t_db_browse_entry));
		
			search_db_entries_count=0;
			if (db_entries_count) search_db_entries=search_db_entries_data;
			for (int j=0;j<db_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:db_entries[j].label]) {
					search_db_entries[search_db_entries_count].label=db_entries[j].label;
					search_db_entries[search_db_entries_count].id_author=authorID;
					search_db_entries[search_db_entries_count].downloaded=db_entries[j].downloaded;
					search_db_entries[search_db_entries_count].rating=db_entries[j].rating;
					search_db_entries[search_db_entries_count].playcount=db_entries[j].playcount;
					
					search_db_entries[search_db_entries_count].id_type=-1;
					search_db_entries[search_db_entries_count].id_album=db_entries[j].id_album;
					search_db_entries[search_db_entries_count].id_mod=db_entries[j].id_mod;
					search_db_entries[search_db_entries_count].filesize=db_entries[j].filesize;
					
					if (db_entries[j].id_mod>0) search_db_hasFiles++;
					
					search_db_entries_count++;
					search_db_nb_entries++;
				}
			}
		return;
	}
	pthread_mutex_lock(&db_mutex);
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].label=nil;
        }
            for (int j=0;j<db_entries_count;j++) {
                db_entries[j].label=nil;
            }
            db_entries=NULL;
        free(db_entries_data);
    }
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT count(1),0 FROM mod_file WHERE id_author=%d AND id_album is null AND filename like \"%%%s%%\" \
							 UNION SELECT count(1),1 FROM mod_author_album m,mod_album a WHERE m.id_author=%d AND m.id_album=a.id AND a.album like \"%%%s%%\"  AND m.id_author=a.id_author",authorID,[mSearchText UTF8String],authorID,[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT count(1),0 FROM mod_file WHERE id_author=%d AND id_album is null \
					 UNION SELECT count(1),1 FROM mod_author_album m,mod_album a WHERE m.id_author=%d AND m.id_album=a.id AND m.id_author=a.id_author",authorID,authorID);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		db_nb_entries=0;
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)calloc(db_nb_entries,sizeof(t_db_browse_entry));
			db_entries_index=0;
				db_entries_count=0;
				db_entries=db_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT filename,filesize,id,0 FROM mod_file \
								 WHERE id_author=%d AND id_album is null AND filename like \"%%%s%%\" \
								 UNION SELECT a.album,a.num_files,a.id,1 FROM mod_author_album m,mod_album a \
								 WHERE m.id_author=%d AND m.id_album=a.id AND a.album like \"%%%s%%\"  AND m.id_author=a.id_author\
								 ORDER BY 1  COLLATE NOCASE",authorID,[mSearchText UTF8String],authorID,[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT filename,filesize,id,0 FROM mod_file \
						 WHERE id_author=%d AND id_album is null \
						 UNION SELECT a.album,a.num_files,a.id,1 FROM mod_author_album m,mod_album a \
						 WHERE m.id_author=%d AND m.id_album=a.id AND m.id_author=a.id_author\
						 ORDER BY 1 COLLATE NOCASE",authorID,authorID);
			
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					int is_album=sqlite3_column_int(stmt, 3);
					db_entries[db_entries_count].id_author=authorID;
					db_entries[db_entries_count].id_type=-1;
					if (is_album) {
						db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						db_entries[db_entries_count].id_album=sqlite3_column_int(stmt, 2);
						db_entries[db_entries_count].id_mod=-1;
					} else {
						db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						db_entries[db_entries_count].id_album=-1;
						db_entries[db_entries_count].id_mod=sqlite3_column_int(stmt, 2);						
						db_hasFiles++;
					}
					db_entries[db_entries_count].filesize=sqlite3_column_int(stmt, 1);
					
					db_entries[db_entries_count].downloaded=-1;
					db_entries[db_entries_count].rating=-1;
					db_entries[db_entries_count].playcount=-1;
					db_entries_count++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_albumORfilename:(int)filetypeID fileAuthorID:(int)authorID {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
    
    if (search_db_nb_entries) {
            for (int j=0;j<search_db_entries_count;j++) {
                search_db_entries[j].label=nil;
            }
            search_db_entries=NULL;
        search_db_nb_entries=0;
        free(search_db_entries_data);
    }
    
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		search_db_entries_data=(t_db_browse_entry*)calloc(db_nb_entries,sizeof(t_db_browse_entry));
		
			search_db_entries_count=0;
			if (db_entries_count) search_db_entries=search_db_entries_data;
			for (int j=0;j<db_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:db_entries[j].label]) {
					search_db_entries[search_db_entries_count].label=db_entries[j].label;
					search_db_entries[search_db_entries_count].id_author=authorID;
					search_db_entries[search_db_entries_count].downloaded=db_entries[j].downloaded;
					search_db_entries[search_db_entries_count].rating=db_entries[j].rating;
					search_db_entries[search_db_entries_count].playcount=db_entries[j].playcount;
					
					search_db_entries[search_db_entries_count].id_type=filetypeID;
					search_db_entries[search_db_entries_count].id_album=db_entries[j].id_album;
					search_db_entries[search_db_entries_count].id_mod=db_entries[j].id_mod;
					search_db_entries[search_db_entries_count].filesize=db_entries[j].filesize;
					if (db_entries[j].id_mod>0) search_db_hasFiles++;
					search_db_entries_count++;
					search_db_nb_entries++;
				}
			}
		return;
	}
	pthread_mutex_lock(&db_mutex);
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].label=nil;
        }
            for (int j=0;j<db_entries_count;j++) {
                db_entries[j].label=nil;
            }
            db_entries=NULL;
        free(db_entries_data);
    }
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT count(1),0 FROM mod_file \
							 WHERE id_author=%d AND id_type=%d id_album is null AND filename like \"%%%s%%\" \
							 UNION SELECT count(1),1 FROM mod_type_author_album m,mod_album a \
							 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id AND a.album like \"%%%s%%\" AND m.id_author=a.id_author",authorID,filetypeID,[mSearchText UTF8String],authorID,filetypeID,[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT count(1),0 FROM mod_file \
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
		} else MDZELog("ErrSQL : %d",err);
		
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)calloc(db_nb_entries,sizeof(t_db_browse_entry));
			db_entries_index=0;
				db_entries_count=0;
				db_entries=db_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT filename,filesize,id,0 FROM mod_file \
								 WHERE id_author=%d AND id_type=%d AND id_album is null AND filename like \"%%%s%%\" \
								 UNION SELECT a.album,m.num_files,a.id,1 FROM mod_type_author_album m,mod_album a \
								 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id AND a.album like \"%%%s%%\"  AND m.id_author=a.id_author\
								 ORDER BY 1  COLLATE NOCASE",authorID,filetypeID,[mSearchText UTF8String],authorID,filetypeID,[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT filename,filesize,id,0 FROM mod_file \
						 WHERE id_author=%d AND id_type=%d AND id_album is null \
						 UNION SELECT a.album,m.num_files,a.id,1 FROM mod_type_author_album m,mod_album a \
						 WHERE m.id_author=%d AND m.id_type=%d AND m.id_album=a.id  AND m.id_author=a.id_author\
						 ORDER BY 1 COLLATE NOCASE",authorID,filetypeID,authorID,filetypeID);
            
            
            
            
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					int is_album=sqlite3_column_int(stmt, 3);
					db_entries[db_entries_count].id_author=authorID;
					db_entries[db_entries_count].id_type=filetypeID;
					if (is_album) {
						db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						db_entries[db_entries_count].id_album=sqlite3_column_int(stmt, 2);
						db_entries[db_entries_count].id_mod=-1;
					} else {
						db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						db_entries[db_entries_count].id_album=-1;
						db_entries[db_entries_count].id_mod=sqlite3_column_int(stmt, 2);
						db_hasFiles++;
					}
					db_entries[db_entries_count].filesize=sqlite3_column_int(stmt, 1);
					db_entries[db_entries_count].downloaded=-1;
					db_entries[db_entries_count].rating=-1;
					db_entries[db_entries_count].playcount=-1;
					db_entries_count++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_filename:(int)authorID fileAlbumID:(int)albumID {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
    
    if (search_db_nb_entries) {
            for (int j=0;j<search_db_entries_count;j++) {
                search_db_entries[j].label=nil;
            }
            search_db_entries=NULL;
        search_db_nb_entries=0;
        free(search_db_entries_data);
    }
	
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		search_db_entries_data=(t_db_browse_entry*)calloc(db_nb_entries,sizeof(t_db_browse_entry));
		
			search_db_entries_count=0;
			if (db_entries_count) search_db_entries=search_db_entries_data;
			for (int j=0;j<db_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:db_entries[j].label]) {
					search_db_entries[search_db_entries_count].label=db_entries[j].label;
					search_db_entries[search_db_entries_count].id_author=authorID;
					search_db_entries[search_db_entries_count].downloaded=db_entries[j].downloaded;
					search_db_entries[search_db_entries_count].rating=db_entries[j].rating;
					search_db_entries[search_db_entries_count].playcount=db_entries[j].playcount;
					
					search_db_entries[search_db_entries_count].id_type=-1;
					search_db_entries[search_db_entries_count].id_album=albumID;
					search_db_entries[search_db_entries_count].id_mod=db_entries[j].id_mod;
					search_db_entries[search_db_entries_count].filesize=db_entries[j].filesize;
					if (db_entries[j].id_mod>0) search_db_hasFiles++;
					search_db_entries_count++;
					search_db_nb_entries++;
				}
			}
		return;
	}
	pthread_mutex_lock(&db_mutex);
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].label=nil;
        }
            for (int j=0;j<db_entries_count;j++) {
                db_entries[j].label=nil;
            }
            db_entries=NULL;
        free(db_entries_data);
    }
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_file WHERE id_author=%d AND id_album=%d AND filename LIKE \"%%%s%%\" ORDER BY filename",authorID,albumID,[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_file WHERE id_author=%d AND id_album=%d ORDER BY filename",authorID,albumID);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)calloc(db_nb_entries,sizeof(t_db_browse_entry));
			db_entries_index=0;
				db_entries_count=0;
				db_entries=db_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT filename,filesize,id FROM mod_file WHERE id_author=%d AND id_album=%d AND filename LIKE \"%%%s%%\" ORDER BY filename COLLATE NOCASE",authorID,albumID,[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT filename,filesize,id FROM mod_file WHERE id_author=%d AND id_album=%d ORDER BY filename COLLATE NOCASE",authorID,albumID);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[db_entries_count].id_author=authorID;
					db_entries[db_entries_count].id_type=-1;
					db_entries[db_entries_count].id_album=albumID;
					db_entries[db_entries_count].id_mod=sqlite3_column_int(stmt,2);
					db_entries[db_entries_count].filesize=sqlite3_column_int(stmt,1);
					db_entries[db_entries_count].downloaded=-1;					
					db_entries[db_entries_count].rating=-1;
					db_entries[db_entries_count].playcount=-1;
					db_hasFiles++;
					db_entries_count++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithDB_filename:(int)filetypeID fileAuthorID:(int)authorID fileAlbumID:(int)albumID {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int db_entries_index;
    
    if (search_db_nb_entries) {
            for (int j=0;j<search_db_entries_count;j++) {
                search_db_entries[j].label=nil;
            }
            search_db_entries=NULL;
        search_db_nb_entries=0;
        free(search_db_entries_data);
    }
	
	db_hasFiles=search_db_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_db=1;
		
		search_db_entries_data=(t_db_browse_entry*)calloc(db_nb_entries,sizeof(t_db_browse_entry));
		
			search_db_entries_count=0;
			if (db_entries_count) search_db_entries=search_db_entries_data;
			for (int j=0;j<db_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:db_entries[j].label]) {
					search_db_entries[search_db_entries_count].label=db_entries[j].label;
					search_db_entries[search_db_entries_count].id_author=authorID;
					search_db_entries[search_db_entries_count].downloaded=db_entries[j].downloaded;
					search_db_entries[search_db_entries_count].rating=db_entries[j].rating;
					search_db_entries[search_db_entries_count].playcount=db_entries[j].playcount;
					
					search_db_entries[search_db_entries_count].id_type=filetypeID;
					search_db_entries[search_db_entries_count].id_album=albumID;
					search_db_entries[search_db_entries_count].id_mod=db_entries[j].id_mod;
					search_db_entries[search_db_entries_count].filesize=db_entries[j].filesize;
					if (db_entries[j].id_mod>0) search_db_hasFiles++;
					search_db_entries_count++;
					search_db_nb_entries++;
				}
			}
		return;
	}
	pthread_mutex_lock(&db_mutex);
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].label=nil;
        }
            for (int j=0;j<db_entries_count;j++) {
                db_entries[j].label=nil;
            }
            db_entries=NULL;
        free(db_entries_data);
    }
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_file WHERE id_type=%d AND id_author=%d AND id_album=%d AND filename like \"%%%s%%\"",filetypeID,authorID,albumID,[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT count(1) FROM mod_file WHERE id_type=%d AND id_author=%d AND id_album=%d",filetypeID,authorID,albumID);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		db_nb_entries=0;
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				db_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		
		if (db_nb_entries) {
			//2nd initialize array to receive entries
			db_entries_data=(t_db_browse_entry *)calloc(db_nb_entries,sizeof(t_db_browse_entry));
			db_entries_index=0;
				db_entries_count=0;
				db_entries=db_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT filename,filesize,id FROM mod_file \
								 WHERE id_type=%d AND id_author=%d AND id_album=%d AND filename like \"%%%s%%\" ORDER BY 1 COLLATE NOCASE",filetypeID,authorID,albumID,[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT filename,filesize,id FROM mod_file \
						 WHERE id_type=%d AND id_author=%d AND id_album=%d ORDER BY 1 COLLATE NOCASE",filetypeID,authorID,albumID);
			
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					db_entries[db_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					db_entries[db_entries_count].id_author=authorID;
					db_entries[db_entries_count].id_type=filetypeID;
					db_entries[db_entries_count].id_album=albumID;
					db_entries[db_entries_count].id_mod=sqlite3_column_int(stmt, 2);
					db_entries[db_entries_count].filesize=sqlite3_column_int(stmt, 1);
					db_entries[db_entries_count].downloaded=-1;
					db_entries[db_entries_count].rating=-1;
					db_entries[db_entries_count].playcount=-1;
					db_hasFiles++;
					db_entries_count++;
					db_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
			
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		
        snprintf(sqlStatement,1024,"select fullpath from mod_file where id=%d",id_mod);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				fullpath=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);		
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		
        snprintf(sqlStatement,1024,""
                 "SELECT a.author||'/'||t.filetype||'/'||l.album||'/'||f.filename "
                 "FROM mod_author a,mod_type t,mod_album l, mod_file f "
                 "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album=l.id AND f.id=%d "
                 "UNION "
                 "SELECT a.author||'/'||t.filetype||'/'||f.filename "
                 "FROM mod_author a,mod_type t, mod_file f "
                 "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album IS NULL AND f.id=%d;"
                 "",id_mod,id_mod);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				localpath=[NSString  stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);		
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
		
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"select filesize from mod_file where filename=\"%s\"",[fileName UTF8String]);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				iFileSize=(int)sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
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
		
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"select filename from mod_file where id=%d",idmod);
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				fileName=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	return fileName;
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
    if (childController) [(RootViewControllerMODLAND*)childController refreshViewAfterDownload];
    else  {
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,""
                 "SELECT a.author||'/'||t.filetype||'/'||l.album||'/'||f.filename "
                 "FROM mod_author a,mod_type t,mod_album l, mod_file f "
                 "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album=l.id AND f.id=%d "
                 "UNION "
                 "SELECT a.author||'/'||t.filetype||'/'||f.filename "
                 "FROM mod_author a,mod_type t, mod_file f "
                 "WHERE f.id_author=a.id AND f.id_type=t.id AND f.id_album IS NULL AND f.id=%d;"
                 "",id_mod,id_mod);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strFullPath=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT author FROM mod_author WHERE id=%d",id_author);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAuthor=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT author FROM mod_author WHERE id=%d",id_author);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAuthor=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        snprintf(sqlStatement,1024,"SELECT filetype FROM mod_type WHERE id=%d",id_type);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strType=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT author FROM mod_author WHERE id=%d",id_author);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAuthor=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        snprintf(sqlStatement,1024,"SELECT filetype FROM mod_type WHERE id=%d",id_type);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strType=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        snprintf(sqlStatement,1024,"SELECT album FROM mod_album WHERE id=%d",id_album);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAlbum=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT filetype FROM mod_type WHERE id=%d",id_type);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strType=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT a.author FROM mod_type_author m,mod_author a WHERE m.id_type=%d AND m.id_author=a.id",id_type);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAuthor=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                checkPath = [documentsDirectory stringByAppendingPathComponent: [NSString stringWithFormat:@"%@/%@/%@",MODLAND_BASEDIR,strAuthor,strType]];
                success = [fileManager fileExistsAtPath:checkPath];
                if (success) break;
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
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
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
        snprintf(sqlStatement,1024,"SELECT filetype FROM mod_type WHERE id=%d",id_type);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strType=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        snprintf(sqlStatement,1024,"SELECT album FROM mod_album WHERE id=%d",id_album);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAlbum=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
        
        snprintf(sqlStatement,1024,"SELECT a.author FROM mod_type_author_album m,mod_author a WHERE m.id_type=%d AND m.id_album=%d m.id_author=a.id",id_type,id_album);
        err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
        if (err==SQLITE_OK){
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                strAuthor=[NSString stringWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
                checkPath = [documentsDirectory stringByAppendingPathComponent: [NSString stringWithFormat:@"%@/%@/%@/%@",MODLAND_BASEDIR,strAuthor,strType,strAlbum]];
                success = [fileManager fileExistsAtPath:checkPath];
                if (success) break;
            }
            sqlite3_finalize(stmt);
        } else MDZELog("ErrSQL : %d",err);
        
    }
    sqlite3_close(db);
    pthread_mutex_unlock(&db_mutex);
    
    if (success) return 1;
    return 0;
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
    if (browse_depth==1) return 3; //Modland browse mode chooser
        int file_or_album=0;
        if (search_db) {
            if (search_db_hasFiles) file_or_album=1;
        } else {
            if (db_hasFiles) file_or_album=1;
        }
        
        return (search_db?search_db_entries_count+file_or_album:db_entries_count+file_or_album);
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
        topLabel.font = [UIFont systemFontOfSize:17];
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
            t_db_browse_entry *cur_db_entries;
            cur_db_entries=(search_db?search_db_entries:db_entries);
            int file_or_album=0;
            if (search_db) {
                if (search_db_hasFiles) file_or_album=1;
            } else {
                if (db_hasFiles) file_or_album=1;
            }
            int crow=indexPath.row-file_or_album;
            if (file_or_album&&(crow==-1)){
                cellValue=NSLocalizedString(@"GetAllEntries_MainKey","");
                topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
                bottomLabel.text=NSLocalizedString(@"GetAllEntries_SubKey","");;
                //cell.accessoryType = UITableViewCellAccessoryDetailDisclosureButton;
            } else {
                cellValue=cur_db_entries[crow].label;
                int colFactor;
                //update downloaded if needed
                if(cur_db_entries[crow].downloaded==-1) {
                    if (cur_db_entries[crow].id_mod>=0) {  //id_mod is known
                        cur_db_entries[crow].downloaded=[self checkIsDownloadedMod:cur_db_entries[crow].id_mod];
                    } else {
                        cur_db_entries[crow].downloaded=1;
                    }
                }
                
                if(cur_db_entries[crow].downloaded==1) {
                    colFactor=1;
                } else colFactor=0;
                
                if (cur_db_entries[crow].id_mod>=0) { //MOD ?
                    if (colFactor==0) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0f];
                    topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                               0,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,
                                               22);
                    
                    if (cur_db_entries[crow].downloaded==1) {
                        if (cur_db_entries[crow].rating==-1) {
                            DBHelper::getFileStatsDBmod([NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,
                                                         [self getCompleteLocalPath:cur_db_entries[crow].id_mod]],
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
                        bottomLabel.text=[NSString stringWithFormat:@"%dKB",cur_db_entries[crow].filesize/1024];
                        bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+20,
                                                       22,
                                                       tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-20,
                                                       18);
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
                    
                } else if (cur_db_entries[crow].id_album>=0) {// ALBUM ?
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                                   18);
                    topLabel.textColor=[UIColor colorWithRed:0.8f green:0.6f blue:0.0f alpha:1.0f];
                    bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[crow].filesize>1?nbFiles:nb1File),cur_db_entries[crow].filesize];
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                } else if (cur_db_entries[crow].id_author>=0) {// AUTHOR ?
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                                   18);
                    topLabel.textColor=[UIColor colorWithRed:1.0f green:0.0f blue:0.8f alpha:1.0f];
                    bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[crow].filesize>1?nbFiles:nb1File),cur_db_entries[crow].filesize];
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                } else  {
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                                   18);
                    if (darkMode) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:1.0f alpha:1.0f];
                    else topLabel.textColor=[UIColor colorWithRed:0.0f green:0.0f blue:1.0f alpha:1.0f];                    
                    bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[crow].filesize>1?nbFiles:nb1File),cur_db_entries[crow].filesize];
                    cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
                }		
                
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

        //delete entry
        
            t_db_browse_entry *cur_db_entries;
            cur_db_entries=(search_db?search_db_entries:db_entries);
            int crow = indexPath.row;
            int download_all=0;
            if (search_db) {
                if (search_db_hasFiles) download_all=1;
            } else {
                if (db_hasFiles) download_all=1;
            }
            crow-=download_all;
            //delete file
            NSString *localpath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[crow].id_mod]]];
            NSError *err;
            DBHelper::deleteStatsFileDB(localpath);
            cur_db_entries[crow].downloaded=0;
            //delete local file
            [mFileMngr removeItemAtPath:localpath error:&err];

        // Reload the cell to show the file is no longer downloaded
        [tableView reloadRowsAtIndexPaths:@[indexPath]
                         withRowAnimation:UITableViewRowAnimationAutomatic];

        completionHandler(YES);
    }];
    deleteAction.backgroundColor = [UIColor redColor];

    // Return multiple actions - they appear from right to left
    return [UISwipeActionsConfiguration configurationWithActions:@[deleteAction]];
}


- (NSIndexPath *)tableView:(UITableView *)tabView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)sourceIndexPath toProposedIndexPath:(NSIndexPath *)proposedDestinationIndexPath {
    
    return proposedDestinationIndexPath;
}

// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tabView canMoveRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
    return NO;
}
- (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    // Return NO if you do not want the item to be re-orderable.
        t_db_browse_entry *cur_db_entries=(search_db?search_db_entries:db_entries);
        int crow=indexPath.row;
        if (search_db) {
            if (search_db_hasFiles) crow--;
        } else {
            if (db_hasFiles) crow--;
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
    if (mSearchText&&([mSearchText length]>0)) mSearch=1;
    else mSearch=0;
    
    shouldFillKeys=1;
    search_db=0;
    [self fillKeys];
    
    [tableView reloadData];
    // flush the previous search content
    //[tableData removeAllObjects];
}
- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar {
    //[self fillKeys];
    //[tableView reloadData];
    //mSearch=0;
    sBar.showsCancelButton = NO;
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    
    shouldFillKeys=1;
    search_db=0;
    [self fillKeys];
    
    [tableView reloadData];
}
- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
    //if (mSearchText) [mSearchText release];
    mSearchText=nil;
    
    mSearchText=[[NSString alloc] initWithString:searchText];
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    
    shouldFillKeys=1;
    search_db=0;
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
    search_db=0;
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
            NSString *filePath;
            NSString *modFilename;
            t_db_browse_entry *cur_db_entries;
            cur_db_entries=(search_db?search_db_entries:db_entries);
            int download_all=0;
            int crow=indexPath.row;
            if (search_db) {
                if (search_db_hasFiles) download_all=1;
            } else {
                if (db_hasFiles) download_all=1;
            }
            //crow-=download_all;
        if (crow>=0) {
            filePath=[self getCompletePath:cur_db_entries[crow].id_mod];
            modFilename=[self getModFilename:cur_db_entries[crow].id_mod];
            
            NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
            NSString *localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[crow].id_mod]];
            mClickedPrimAction=1;
            
            if (cur_db_entries[crow].downloaded==1) {
                NSMutableArray *array_label = [[NSMutableArray alloc] init];
                NSMutableArray *array_path = [[NSMutableArray alloc] init];
                [array_label addObject:modFilename];
                [array_path addObject:localPath];
                cur_db_entries[crow].rating=-1;
                [detailViewController play_listmodules:(NSArray*)array_label start_index:(int)0 path:(NSArray*)array_path];
                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                                
                [tableView reloadData];
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                mCurrentWinAskedDownload=1;
                
                NSString *modland_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
                NSRange nsr=[modland_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                if (nsr.location==NSNotFound) {
                    //HTTP
                    [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:cur_db_entries[crow].filesize isMODLAND:1 usePrimaryAction:1];
                } else {
                    //FTP
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:cur_db_entries[crow].filesize filename:modFilename isMODLAND:1 usePrimaryAction:1];
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
            t_db_browse_entry *cur_db_entries;
            cur_db_entries=(search_db?search_db_entries:db_entries);
            int download_all=0;
            int crow=indexPath.row;
            if (search_db) {
                if (search_db_hasFiles) download_all=1;
            } else {
                if (db_hasFiles) download_all=1;
            }
            //crow-=download_all;
        if (crow>=0) {
            NSString *filePath=[self getCompletePath:cur_db_entries[crow].id_mod];
            NSString *modFilename=[self getModFilename:cur_db_entries[crow].id_mod];
            NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
            NSString *localPath=[[NSString alloc] initWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[crow].id_mod]];
            mClickedPrimAction=2;
            
            if (cur_db_entries[crow].downloaded==1) {
                
                if ([detailViewController add_to_playlist:localPath fileName:modFilename forcenoplay:1]) {
                    if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                    
                    cur_db_entries[crow].rating=-1;
                    
                    [tableView reloadData];
                }
            } else {
                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                mCurrentWinAskedDownload=1;
                
                NSString *modland_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
                NSRange nsr=[modland_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                if (nsr.location==NSNotFound) {
                    //HTTP
                    [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:cur_db_entries[crow].filesize isMODLAND:1 usePrimaryAction:2];
                    
                } else {
                    //FTP
                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:cur_db_entries[crow].filesize filename:modFilename isMODLAND:1 usePrimaryAction:2];
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
    NSString *cellValue;
    
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
                
            } else {
                int file_or_album=0;
                t_db_browse_entry *cur_db_entries;
                cur_db_entries=(search_db?search_db_entries:db_entries);
                
                if (search_db) {
                    if (search_db_hasFiles) file_or_album=1;
                } else {
                    if (db_hasFiles) file_or_album=1;
                }
                int crow=indexPath.row-file_or_album;
                if (!file_or_album) {
                    cellValue=cur_db_entries[crow].label;
                    
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
                                ((RootViewControllerMODLAND*)childController)->mFiletypeID = cur_db_entries[crow].id_type;
                            } else ((RootViewControllerMODLAND*)childController)->mFiletypeID = mFiletypeID;
                            //Author selected
                            if (browse_depth==3) {
                                ((RootViewControllerMODLAND*)childController)->mAuthorID = cur_db_entries[crow].id_author;
                            } else ((RootViewControllerMODLAND*)childController)->mAuthorID = mAuthorID;
                            if (browse_depth==4) {
                                ((RootViewControllerMODLAND*)childController)->mAlbumID = cur_db_entries[crow].id_album;
                            } else ((RootViewControllerMODLAND*)childController)->mAlbumID = mAlbumID;
                            break;
                        case 1:		//Authors/Formats/Files
                            //Author selected
                            if (browse_depth==2) {
                                ((RootViewControllerMODLAND*)childController)->mAuthorID = cur_db_entries[crow].id_author;
                            } else ((RootViewControllerMODLAND*)childController)->mAuthorID = mAuthorID;
                            //Filetype selected
                            if (browse_depth==3) {
                                ((RootViewControllerMODLAND*)childController)->mFiletypeID = cur_db_entries[crow].id_type;
                            } else ((RootViewControllerMODLAND*)childController)->mFiletypeID = mFiletypeID;
                            if (browse_depth==4) {
                                ((RootViewControllerMODLAND*)childController)->mAlbumID = cur_db_entries[crow].id_album;
                            } else ((RootViewControllerMODLAND*)childController)->mAlbumID = mAlbumID;
                            break;
                        case 2:		//Authors/Files
                            //Author selected
                            if (browse_depth==2) {
                                ((RootViewControllerMODLAND*)childController)->mAuthorID = cur_db_entries[crow].id_author;
                            } else ((RootViewControllerMODLAND*)childController)->mAuthorID = mAuthorID;
                            if (browse_depth==3) {
                                ((RootViewControllerMODLAND*)childController)->mAlbumID = cur_db_entries[crow].id_album;
                            } else ((RootViewControllerMODLAND*)childController)->mAlbumID = mAlbumID;
                            break;
                    }
                    
                    ((RootViewControllerMODLAND*)childController)->detailViewController=detailViewController;
                    ((RootViewControllerMODLAND*)childController)->downloadViewController=downloadViewController;
//                    childController.view.frame=self.view.frame;
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
                    
                    
                    //				[childController autorelease];
                } else { 
                    if (crow==-1) {//download all dir
                        NSString *filePath;
                        NSString *modFilename;
                        NSString *ftpPath;
                        NSString *localPath;
                        int first=0; //1;  Do not play even first file => TODO : add a setting for this
                        int existing;
                        int tooMuch=0;
                        if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==2) first=0;//enqueue only
                        
                        int cur_db_entries_count=(search_db?search_db_entries_count:db_entries_count);
                        
                            for (int j=0;j<cur_db_entries_count;j++) {
                                if (cur_db_entries[j].id_mod!=-1) {//mod found
                                    
                                    existing=cur_db_entries[j].downloaded;
                                    if (existing==-1) {
                                        existing=cur_db_entries[j].downloaded=[self checkIsDownloadedMod:cur_db_entries[j].id_mod];
                                    }
                                    
                                    if (existing==0) {
                                        filePath=[self getCompletePath:cur_db_entries[j].id_mod];
                                        modFilename=[self getModFilename:cur_db_entries[j].id_mod];
                                        ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
                                        localPath=[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[j].id_mod]];
                                        mCurrentWinAskedDownload=1;
                                        [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                                        
                                        
                                        NSString *modland_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
                                        NSRange nsr=[modland_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                                        if (first) {
                                            if (nsr.location==NSNotFound) {
                                                //HTTP
                                                [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:cur_db_entries[j].filesize isMODLAND:1 usePrimaryAction:1];
                                                
                                            } else {
                                                //FTP
                                                if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:cur_db_entries[j].filesize filename:modFilename isMODLAND:1 usePrimaryAction:1]) {
                                                    tooMuch=1;
                                                    break;
                                                }
                                            }
                                            first=0;
                                        } else {
                                            if (nsr.location==NSNotFound) {
                                                //HTTP
                                                [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:cur_db_entries[j].filesize isMODLAND:1 usePrimaryAction:2];
                                                
                                            } else {
                                                //FTP
                                                if ([downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:cur_db_entries[j].filesize filename:modFilename isMODLAND:1 usePrimaryAction:2]) {
                                                    tooMuch=1;
                                                    break;
                                                }
                                            }
                                            
                                            
                                        }
                                    }
                                }
                            }
                        
                    } else {
                        cellValue=cur_db_entries[crow].label;
                        
                        //Check if an album was selected
                        if (cur_db_entries[crow].id_mod==-1) {//no mod : Album selcted
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
                            ((RootViewControllerMODLAND*)childController)->mAlbumID = cur_db_entries[crow].id_album;
                            
                            ((RootViewControllerMODLAND*)childController)->detailViewController=detailViewController;
                            ((RootViewControllerMODLAND*)childController)->downloadViewController=downloadViewController;
//                            childController.view.frame=self.view.frame;
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
                            
                        } else { //File selected, start download is needed
                            NSString *filePath=[self getCompletePath:cur_db_entries[crow].id_mod];
                            NSString *modFilename=[self getModFilename:cur_db_entries[crow].id_mod];
                            NSString *ftpPath=[NSString stringWithFormat:@"/pub/modules/%@",filePath];
                            NSString *localPath=[NSString stringWithFormat:@"Documents/%@/%@",MODLAND_BASEDIR,[self getCompleteLocalPath:cur_db_entries[crow].id_mod]];
                            mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
                            
                            if (cur_db_entries[crow].downloaded==1) {
                                    if (mClickedPrimAction) {
                                        NSMutableArray *array_label = [[NSMutableArray alloc] init];
                                        NSMutableArray *array_path = [[NSMutableArray alloc] init];
                                        [array_label addObject:modFilename];
                                        [array_path addObject:localPath];
                                        [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                                        if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                                        
                                        cur_db_entries[crow].rating=-1;
                                        
                                        [tabView reloadData];
                                    } else {
                                        if ([detailViewController add_to_playlist:localPath fileName:modFilename forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)]) {
                                            if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                                            
                                            cur_db_entries[crow].rating=-1;
                                            
                                            [tabView reloadData];
                                        }
                                    }                                
                            } else {
                                [self checkCreate:[localPath stringByDeletingLastPathComponent]];
                                mCurrentWinAskedDownload=1;
                                
                                NSString *modland_url=[NSString stringWithFormat:@"%s",settings[ONLINE_MODLAND_CURRENT_URL].detail.mdz_msgbox.text];
                                NSRange nsr=[modland_url rangeOfString:@"ftp://" options:NSCaseInsensitiveSearch];
                                if (nsr.location==NSNotFound) {
                                    //HTTP
                                    [downloadViewController addURLToDownloadList:[NSString stringWithFormat:@"%@%@",modland_url,ftpPath] fileName:modFilename filePath:localPath filesize:cur_db_entries[crow].filesize isMODLAND:1 usePrimaryAction:mClickedPrimAction];
                                    
                                } else {
                                    //FTP
                                    [downloadViewController addFTPToDownloadList:localPath ftpURL:ftpPath ftpHost:[modland_url substringFromIndex:6] filesize:cur_db_entries[crow].filesize filename:modFilename isMODLAND:1 usePrimaryAction:mClickedPrimAction];
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
    
    if (db_nb_entries) {
        for (int i=0;i<db_nb_entries;i++) {
            db_entries_data[i].label=nil;
        }
            for (int j=0;j<db_entries_count;j++) {
                db_entries[j].label=nil;
            }
            db_entries=NULL;
        free(db_entries_data);
    }
    
    if (search_db_nb_entries) {
            for (int j=0;j<search_db_entries_count;j++) {
                search_db_entries[j].label=nil;
            }
            search_db_entries=NULL;
        search_db_nb_entries=0;
        free(search_db_entries_data);
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
