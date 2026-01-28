//
//  RootViewControllerHVSC.mm
//  modizer
//
//  Created by Yohann Magnien on 04/06/10.
//  Copyright __YoyoFR / Yohann Magnien__ 2010. All rights reserved.
//


#define RATING_IMG(a) ( (a==5?2:(a?1:0)) )

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

#import "TTFadeAnimator.h"
#import "ModizFileHelper.h"
#import "RadioSource.h"


@implementation RootViewControllerHVSC

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


/////////////////////////////////////////////////////////////////////////////////////////////
// WaitingView methods
/////////////////////////////////////////////////////////////////////////////////////////////
#include "WaitingViewCommonMethods.h"
/////////////////////////////////////////////////////////////////////////////////////////////
#define HAS_DETAILVIEW_CONT
#include "PlaylistCommonFunctions.h"

-(void) pushRadioButton {
    if ([detailViewController.radioSource isActive]&&(detailViewController.radioSource.mRadioSource==RS_COLLECTION_HVSC)) {
        [detailViewController stop];
        [detailViewController clearQueue];
        [detailViewController.radioSource stop];
        [radioButton setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    } else {
        [detailViewController.radioSource stop];
        [detailViewController clearQueue];
        detailViewController.radioSource.mRadioSource=RS_COLLECTION_HVSC;
        
        t_dbHVSC_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
        int nb_entries=(search_dbHVSC?search_dbHVSC_nb_entries:dbHVSC_nb_entries);
        
        switch (browse_depth) {
            case 1:
                detailViewController.radioSource.mRadioSource_mode=1;
                [detailViewController.radioSource.mSourceData removeAllObjects];
                for (int i=0;i<nb_entries;i++) {
                    [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"d:%@",cur_db_entries[i].dir1]];
                }
                break;
            case 2:
                detailViewController.radioSource.mRadioSource_mode=2;
                [detailViewController.radioSource.mSourceData removeAllObjects];
                for (int i=0;i<nb_entries;i++) {
                    if (cur_db_entries[i].dir2) {
                        //entry is a dir
                        [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"d:%@/%@",cur_db_entries[i].dir1,cur_db_entries[i].dir2]];
                    } else {
                        //entry is a file
                        [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"f:%@",cur_db_entries[i].fullpath]];
                    }
                }
                break;
            case 3:
                detailViewController.radioSource.mRadioSource_mode=3;
                [detailViewController.radioSource.mSourceData removeAllObjects];
                for (int i=0;i<nb_entries;i++) {
                    if (cur_db_entries[i].dir3) {
                        //entry is a dir
                        [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"d:%@/%@/%@",cur_db_entries[i].dir1,cur_db_entries[i].dir2,cur_db_entries[i].dir3]];
                    } else {
                        //entry is a file
                        [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"f:%@",cur_db_entries[i].fullpath]];
                    }
                }
                break;
            case 4:
                detailViewController.radioSource.mRadioSource_mode=4;
                [detailViewController.radioSource.mSourceData removeAllObjects];
                for (int i=0;i<nb_entries;i++) {
                    if (cur_db_entries[i].dir4) {
                        //entry is a dir
                        [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"d:%@/%@/%@/%@",cur_db_entries[i].dir1,cur_db_entries[i].dir2,cur_db_entries[i].dir3,cur_db_entries[i].dir4]];
                    } else {
                        //entry is a file
                        [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"f:%@",cur_db_entries[i].fullpath]];
                    }
                }
                break;
            case 5:
                detailViewController.radioSource.mRadioSource_mode=5;
                [detailViewController.radioSource.mSourceData removeAllObjects];
                for (int i=0;i<nb_entries;i++) {
                    if (cur_db_entries[i].dir5) {
                        //entry is a dir
                        [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"d:%@/%@/%@/%@/%@",cur_db_entries[i].dir1,cur_db_entries[i].dir2,cur_db_entries[i].dir3,cur_db_entries[i].dir4,cur_db_entries[i].dir5]];
                    } else {
                        //entry is a file
                        [detailViewController.radioSource.mSourceData addObject:[NSString stringWithFormat:@"f:%@",cur_db_entries[i].fullpath]];
                    }
                }
                break;
            default:
                detailViewController.radioSource.mRadioSource_mode=0;
                break;
        }
        [detailViewController.radioSource activate];
        [radioButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    }
}

-(void)handleLongPress:(UILongPressGestureRecognizer *)gestureRecognizer {
    CGPoint p = [gestureRecognizer locationInView:self.tableView];
    
    NSIndexPath *indexPath = [self.tableView indexPathForRowAtPoint:p];
    if (browse_depth>1) {
        if (indexPath != nil) {
            if ((gestureRecognizer.state==UIGestureRecognizerStateBegan)||(gestureRecognizer.state==UIGestureRecognizerStateChanged)) {
                int crow=indexPath.row;
                
                int download_all=0;
                if (search_dbHVSC) {
                    if (search_dbHVSC_hasFiles) download_all=1;
                } else {
                    if (dbHVSC_hasFiles) download_all=1;
                }
                crow-=download_all;
                
                if (crow>=0) {
                    //display popup
                    t_dbHVSC_browse_entry *cur_db_entries;
                    cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
                    
                    NSString *str=cur_db_entries[crow].fullpath;
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
    
    mFileMngr=[[NSFileManager alloc] init];
    
    forceReloadCells=false;
    darkMode=false;
    if (self.traitCollection.userInterfaceStyle==UIUserInterfaceStyleDark) darkMode=true;
    
	
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
	
	search_dbHVSC=0;  //reset to ensure search_dbHVSC is not used by default
	
    dbHVSC_entries=NULL;
    search_dbHVSC_entries=NULL;
    
	dbHVSC_nb_entries=0;
	search_dbHVSC_nb_entries=0;
	
	search_dbHVSC_hasFiles=0;
	dbHVSC_hasFiles=0;
	
	mSearchText=nil;
	mCurrentWinAskedDownload=0;
	mClickedPrimAction=0;
    
    [radioButton setStyle:BButtonStyleBootstrapV2];
    [radioButton setType:BButtonTypeInverse];
    [radioButton addAwesomeIcon:FAIconRss beforeTitle:YES];
    [radioButton setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    [radioButton addTarget:self action:@selector(pushRadioButton) forControlEvents:UIControlEventTouchUpInside];
	
	if (browse_depth==0) {
#ifdef GET_NB_ENTRIES
		mNbHVSCFileEntries=DBHelper::getNbHVSCFilesEntries();
#else
		mNbHVSCFileEntries=NB_HVSC_ENTRIES;
#endif
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
    
    if ((mSearchText==nil)||([mSearchText length]==0)) mSearch=0;
    else mSearch=1;
    shouldFillKeys=1;
    search_dbHVSC=0;
    
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
    
    if (search_dbHVSC_nb_entries) {
            for (int j=0;j<search_dbHVSC_entries_count;j++) {
                search_dbHVSC_entries[j].label=nil;
                search_dbHVSC_entries[j].fullpath=nil;
                search_dbHVSC_entries[j].id_md5=nil;
                search_dbHVSC_entries[j].dir1=nil;
                search_dbHVSC_entries[j].dir2=nil;
                search_dbHVSC_entries[j].dir3=nil;
                search_dbHVSC_entries[j].dir4=nil;
                search_dbHVSC_entries[j].dir5=nil;
            }
            search_dbHVSC_entries=NULL;
        search_dbHVSC_nb_entries=0;
        free(search_dbHVSC_entries_data);
    }
	
	dbHVSC_hasFiles=search_dbHVSC_hasFiles=0;
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		
			search_dbHVSC_entries_count=0;
			if (dbHVSC_entries_count) search_dbHVSC_entries=search_dbHVSC_entries_data;
			for (int j=0;j<dbHVSC_entries_count;j++)  {
                if ([self searchStringRegExp:mSearchText sourceString:dbHVSC_entries[j].label]) {
					search_dbHVSC_entries[search_dbHVSC_entries_count].label=dbHVSC_entries[j].label;
					search_dbHVSC_entries[search_dbHVSC_entries_count].downloaded=dbHVSC_entries[j].downloaded;
					search_dbHVSC_entries[search_dbHVSC_entries_count].rating=dbHVSC_entries[j].rating;
					search_dbHVSC_entries[search_dbHVSC_entries_count].playcount=dbHVSC_entries[j].playcount;
					search_dbHVSC_entries[search_dbHVSC_entries_count].filesize=dbHVSC_entries[j].filesize;
					
					search_dbHVSC_entries[search_dbHVSC_entries_count].id_md5=dbHVSC_entries[j].id_md5;
					search_dbHVSC_entries[search_dbHVSC_entries_count].fullpath=dbHVSC_entries[j].fullpath;
					search_dbHVSC_entries[search_dbHVSC_entries_count].dir1=dbHVSC_entries[j].dir1;
					search_dbHVSC_entries[search_dbHVSC_entries_count].dir2=dbHVSC_entries[j].dir2;
					search_dbHVSC_entries[search_dbHVSC_entries_count].dir3=dbHVSC_entries[j].dir3;
					search_dbHVSC_entries[search_dbHVSC_entries_count].dir4=dbHVSC_entries[j].dir4;
					search_dbHVSC_entries[search_dbHVSC_entries_count].dir5=dbHVSC_entries[j].dir5;
					
					search_dbHVSC_entries_count++;
					search_dbHVSC_nb_entries++;
				}
			}
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (dbHVSC_nb_entries) {
		for (int i=0;i<dbHVSC_nb_entries;i++) {
            dbHVSC_entries_data[i].label=nil;
            dbHVSC_entries_data[i].fullpath=nil;
            dbHVSC_entries_data[i].id_md5=nil;
            dbHVSC_entries_data[i].dir1=nil;
            dbHVSC_entries_data[i].dir2=nil;
            dbHVSC_entries_data[i].dir3=nil;
            dbHVSC_entries_data[i].dir4=nil;
            dbHVSC_entries_data[i].dir5=nil;
		}
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir1) FROM hvsc_file WHERE dir1 LIKE \"%%%s%%\"",[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir1) FROM hvsc_file");
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			
			dbHVSC_entries_index=0;
				dbHVSC_entries_count=0;
				dbHVSC_entries=dbHVSC_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT dir1,COUNT(1) FROM hvsc_file WHERE dir1 LIKE \"%%%s%%\" GROUP BY dir1 ORDER BY dir1 COLLATE NOCASE",[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT dir1,COUNT(1) FROM hvsc_file GROUP BY dir1 ORDER BY dir1 COLLATE NOCASE");
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					dbHVSC_entries[dbHVSC_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[dbHVSC_entries_count].dir1=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[dbHVSC_entries_count].filesize=sqlite3_column_int(stmt, 1);
					
					dbHVSC_entries[dbHVSC_entries_count].downloaded=-1;
					dbHVSC_entries[dbHVSC_entries_count].rating=-1;
					dbHVSC_entries[dbHVSC_entries_count].playcount=-1;
					dbHVSC_entries_count++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
	
}
-(void) fillKeysWithHVSCDB_Dir2:(NSString*)dir1 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
    
    if (search_dbHVSC_nb_entries) {
        for (int j=0;j<search_dbHVSC_entries_count;j++) {
            search_dbHVSC_entries[j].label=nil;
            search_dbHVSC_entries[j].fullpath=nil;
            search_dbHVSC_entries[j].id_md5=nil;
            search_dbHVSC_entries[j].dir1=nil;
            search_dbHVSC_entries[j].dir2=nil;
            search_dbHVSC_entries[j].dir3=nil;
            search_dbHVSC_entries[j].dir4=nil;
            search_dbHVSC_entries[j].dir5=nil;
        }
        search_dbHVSC_entries=NULL;
        search_dbHVSC_nb_entries=0;
        free(search_dbHVSC_entries_data);
    }

	
	dbHVSC_hasFiles=search_dbHVSC_hasFiles=0;
	
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		
        search_dbHVSC_entries_count=0;
        if (dbHVSC_entries_count) search_dbHVSC_entries=search_dbHVSC_entries_data;
        for (int j=0;j<dbHVSC_entries_count;j++)  {
            if ([self searchStringRegExp:mSearchText sourceString:dbHVSC_entries[j].label]) {
                search_dbHVSC_entries[search_dbHVSC_entries_count].label=dbHVSC_entries[j].label;
                search_dbHVSC_entries[search_dbHVSC_entries_count].downloaded=dbHVSC_entries[j].downloaded;
                search_dbHVSC_entries[search_dbHVSC_entries_count].rating=dbHVSC_entries[j].rating;
                search_dbHVSC_entries[search_dbHVSC_entries_count].playcount=dbHVSC_entries[j].playcount;
                search_dbHVSC_entries[search_dbHVSC_entries_count].filesize=dbHVSC_entries[j].filesize;
                
                search_dbHVSC_entries[search_dbHVSC_entries_count].id_md5=dbHVSC_entries[j].id_md5;
                search_dbHVSC_entries[search_dbHVSC_entries_count].fullpath=dbHVSC_entries[j].fullpath;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir1=dbHVSC_entries[j].dir1;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir2=dbHVSC_entries[j].dir2;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir3=dbHVSC_entries[j].dir3;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir4=dbHVSC_entries[j].dir4;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir5=dbHVSC_entries[j].dir5;
                
                search_dbHVSC_entries_count++;
                search_dbHVSC_nb_entries++;
            }
        }
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (dbHVSC_nb_entries) {
		for (int i=0;i<dbHVSC_nb_entries;i++) {
            dbHVSC_entries_data[i].label=nil;
            dbHVSC_entries_data[i].fullpath=nil;
            dbHVSC_entries_data[i].id_md5=nil;
            dbHVSC_entries_data[i].dir1=nil;
            dbHVSC_entries_data[i].dir2=nil;
            dbHVSC_entries_data[i].dir3=nil;
            dbHVSC_entries_data[i].dir4=nil;
            dbHVSC_entries_data[i].dir5=nil;
		}
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir2) FROM hvsc_file WHERE dir1=\"%s\" AND dir2 LIKE \"%%%s%%\"",[dir1 UTF8String],[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir2) FROM hvsc_file WHERE dir1=\"%s\"",[dir1 UTF8String]);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			
			dbHVSC_entries_index=0;
				dbHVSC_entries_count=0;
				dbHVSC_entries=dbHVSC_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT dir2,COUNT(1) FROM hvsc_file WHERE dir1=\"%s\" AND dir2 LIKE \"%%%s%%\" GROUP BY dir2 ORDER BY dir2 COLLATE NOCASE",[dir1 UTF8String],[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT dir2,COUNT(1) FROM hvsc_file WHERE dir1=\"%s\" GROUP BY dir2 ORDER BY dir2 COLLATE NOCASE",[dir1 UTF8String]);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					dbHVSC_entries[dbHVSC_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[dbHVSC_entries_count].dir1=[[NSString alloc] initWithString:dir1];
					dbHVSC_entries[dbHVSC_entries_count].dir2=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[dbHVSC_entries_count].filesize=sqlite3_column_int(stmt, 1);
					dbHVSC_entries[dbHVSC_entries_count].downloaded=-1;
					dbHVSC_entries[dbHVSC_entries_count].rating=-1;
					dbHVSC_entries[dbHVSC_entries_count].playcount=-1;
					dbHVSC_entries_count++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithHVSCDB_Dir3:(NSString*)dir1 dir2:(NSString*)dir2 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
    
    if (search_dbHVSC_nb_entries) {
        for (int j=0;j<search_dbHVSC_entries_count;j++) {
            search_dbHVSC_entries[j].label=nil;
            search_dbHVSC_entries[j].fullpath=nil;
            search_dbHVSC_entries[j].id_md5=nil;
            search_dbHVSC_entries[j].dir1=nil;
            search_dbHVSC_entries[j].dir2=nil;
            search_dbHVSC_entries[j].dir3=nil;
            search_dbHVSC_entries[j].dir4=nil;
            search_dbHVSC_entries[j].dir5=nil;
        }
        search_dbHVSC_entries=NULL;
        search_dbHVSC_nb_entries=0;
        free(search_dbHVSC_entries_data);
    }
    
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		search_dbHVSC_hasFiles=0;
        search_dbHVSC_entries_count=0;
        if (dbHVSC_entries_count) search_dbHVSC_entries=search_dbHVSC_entries_data;
        for (int j=0;j<dbHVSC_entries_count;j++)  {
            if ([self searchStringRegExp:mSearchText sourceString:dbHVSC_entries[j].label]) {
                search_dbHVSC_entries[search_dbHVSC_entries_count].label=dbHVSC_entries[j].label;
                search_dbHVSC_entries[search_dbHVSC_entries_count].downloaded=dbHVSC_entries[j].downloaded;
                search_dbHVSC_entries[search_dbHVSC_entries_count].rating=dbHVSC_entries[j].rating;
                search_dbHVSC_entries[search_dbHVSC_entries_count].playcount=dbHVSC_entries[j].playcount;
                search_dbHVSC_entries[search_dbHVSC_entries_count].filesize=dbHVSC_entries[j].filesize;
                
                search_dbHVSC_entries[search_dbHVSC_entries_count].id_md5=dbHVSC_entries[j].id_md5;
                search_dbHVSC_entries[search_dbHVSC_entries_count].fullpath=dbHVSC_entries[j].fullpath;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir1=dbHVSC_entries[j].dir1;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir2=dbHVSC_entries[j].dir2;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir3=dbHVSC_entries[j].dir3;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir4=dbHVSC_entries[j].dir4;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir5=dbHVSC_entries[j].dir5;
                
                search_dbHVSC_entries_count++;
                search_dbHVSC_nb_entries++;
            }
        }
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (dbHVSC_nb_entries) {
		for (int i=0;i<dbHVSC_nb_entries;i++) {
            dbHVSC_entries_data[i].label=nil;
            dbHVSC_entries_data[i].fullpath=nil;
            dbHVSC_entries_data[i].id_md5=nil;
            dbHVSC_entries_data[i].dir1=nil;
            dbHVSC_entries_data[i].dir2=nil;
            dbHVSC_entries_data[i].dir3=nil;
            dbHVSC_entries_data[i].dir4=nil;
            dbHVSC_entries_data[i].dir5=nil;
		}
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	dbHVSC_hasFiles=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir3) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is not null AND dir3 LIKE \"%%%s%%\"\
							 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is null AND filename LIKE \"%%%s%%\"",[dir1 UTF8String],[dir2 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir3) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is not null\
					 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is null",[dir1 UTF8String],[dir2 UTF8String],[dir1 UTF8String],[dir2 UTF8String]);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		
		
		
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			
			dbHVSC_entries_index=0;
				dbHVSC_entries_count=0;
				dbHVSC_entries=dbHVSC_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT dir3,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is not null AND dir3 LIKE \"%%%s%%\" GROUP BY dir3 \
								 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is null AND filename LIKE \"%%%s%%\" \
								 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT dir3,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is not null GROUP BY dir3\
						 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3 is null \
						 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir1 UTF8String],[dir2 UTF8String]);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					dbHVSC_entries[dbHVSC_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[dbHVSC_entries_count].dir1=[[NSString alloc] initWithString:dir1];
					dbHVSC_entries[dbHVSC_entries_count].dir2=[[NSString alloc] initWithString:dir2];
					
					if (sqlite3_column_int(stmt, 4)==0) {
						dbHVSC_entries[dbHVSC_entries_count].dir3=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						dbHVSC_entries[dbHVSC_entries_count].filesize=sqlite3_column_int(stmt, 3);
					} else {
						dbHVSC_entries[dbHVSC_entries_count].id_md5=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 2)];
						dbHVSC_entries[dbHVSC_entries_count].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
						dbHVSC_hasFiles++;
					}
					
					dbHVSC_entries[dbHVSC_entries_count].downloaded=-1;
					dbHVSC_entries[dbHVSC_entries_count].rating=-1;
					dbHVSC_entries[dbHVSC_entries_count].playcount=-1;
					dbHVSC_entries_count++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithHVSCDB_Dir4:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
    
    if (search_dbHVSC_nb_entries) {
        for (int j=0;j<search_dbHVSC_entries_count;j++) {
            search_dbHVSC_entries[j].label=nil;
            search_dbHVSC_entries[j].fullpath=nil;
            search_dbHVSC_entries[j].id_md5=nil;
            search_dbHVSC_entries[j].dir1=nil;
            search_dbHVSC_entries[j].dir2=nil;
            search_dbHVSC_entries[j].dir3=nil;
            search_dbHVSC_entries[j].dir4=nil;
            search_dbHVSC_entries[j].dir5=nil;
        }
        search_dbHVSC_entries=NULL;
        search_dbHVSC_nb_entries=0;
        free(search_dbHVSC_entries_data);
    }
    
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		search_dbHVSC_hasFiles=0;
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		
        search_dbHVSC_entries_count=0;
        if (dbHVSC_entries_count) search_dbHVSC_entries=search_dbHVSC_entries_data;
        for (int j=0;j<dbHVSC_entries_count;j++)  {
            if ([self searchStringRegExp:mSearchText sourceString:dbHVSC_entries[j].label]) {
                search_dbHVSC_entries[search_dbHVSC_entries_count].label=dbHVSC_entries[j].label;
                search_dbHVSC_entries[search_dbHVSC_entries_count].downloaded=dbHVSC_entries[j].downloaded;
                search_dbHVSC_entries[search_dbHVSC_entries_count].rating=dbHVSC_entries[j].rating;
                search_dbHVSC_entries[search_dbHVSC_entries_count].playcount=dbHVSC_entries[j].playcount;
                search_dbHVSC_entries[search_dbHVSC_entries_count].filesize=dbHVSC_entries[j].filesize;
                
                search_dbHVSC_entries[search_dbHVSC_entries_count].id_md5=dbHVSC_entries[j].id_md5;
                search_dbHVSC_entries[search_dbHVSC_entries_count].fullpath=dbHVSC_entries[j].fullpath;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir1=dbHVSC_entries[j].dir1;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir2=dbHVSC_entries[j].dir2;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir3=dbHVSC_entries[j].dir3;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir4=dbHVSC_entries[j].dir4;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir5=dbHVSC_entries[j].dir5;
                
                search_dbHVSC_entries_count++;
                search_dbHVSC_nb_entries++;
            }
        }
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (dbHVSC_nb_entries) {
		for (int i=0;i<dbHVSC_nb_entries;i++) {
            dbHVSC_entries_data[i].label=nil;
            dbHVSC_entries_data[i].fullpath=nil;
            dbHVSC_entries_data[i].id_md5=nil;
            dbHVSC_entries_data[i].dir1=nil;
            dbHVSC_entries_data[i].dir2=nil;
            dbHVSC_entries_data[i].dir3=nil;
            dbHVSC_entries_data[i].dir4=nil;
            dbHVSC_entries_data[i].dir5=nil;
		}
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	dbHVSC_hasFiles=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir4) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is not null AND dir4 LIKE \"%%%s%%\"\
							 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is null AND filename LIKE \"%%%s%%\"",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir4) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is not null\
					 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is null",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String]);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		
		
		
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			
			dbHVSC_entries_index=0;
				dbHVSC_entries_count=0;
				dbHVSC_entries=dbHVSC_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT dir4,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is not null AND dir3 LIKE \"%%%s%%\" GROUP BY dir4 \
								 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is null AND filename LIKE \"%%%s%%\" \
								 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT dir4,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is not null GROUP BY dir4 \
						 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4 is null \
						 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String]);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					dbHVSC_entries[dbHVSC_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[dbHVSC_entries_count].dir1=[[NSString alloc] initWithString:dir1];
					dbHVSC_entries[dbHVSC_entries_count].dir2=[[NSString alloc] initWithString:dir2];
					dbHVSC_entries[dbHVSC_entries_count].dir3=[[NSString alloc] initWithString:dir3];
					if (sqlite3_column_int(stmt, 4)==0) {
						dbHVSC_entries[dbHVSC_entries_count].dir4=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						dbHVSC_entries[dbHVSC_entries_count].filesize=sqlite3_column_int(stmt, 3);
					} else {
						dbHVSC_hasFiles++;
						dbHVSC_entries[dbHVSC_entries_count].id_md5=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 2)];
						dbHVSC_entries[dbHVSC_entries_count].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
					}
					
					dbHVSC_entries[dbHVSC_entries_count].downloaded=-1;					
					dbHVSC_entries[dbHVSC_entries_count].rating=-1;
					dbHVSC_entries[dbHVSC_entries_count].playcount=-1;
					dbHVSC_entries_count++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithHVSCDB_Dir5:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
    
    if (search_dbHVSC_nb_entries) {
        for (int j=0;j<search_dbHVSC_entries_count;j++) {
            search_dbHVSC_entries[j].label=nil;
            search_dbHVSC_entries[j].fullpath=nil;
            search_dbHVSC_entries[j].id_md5=nil;
            search_dbHVSC_entries[j].dir1=nil;
            search_dbHVSC_entries[j].dir2=nil;
            search_dbHVSC_entries[j].dir3=nil;
            search_dbHVSC_entries[j].dir4=nil;
            search_dbHVSC_entries[j].dir5=nil;
        }
        search_dbHVSC_entries=NULL;
        search_dbHVSC_nb_entries=0;
        free(search_dbHVSC_entries_data);
    }
    
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		search_dbHVSC_hasFiles=0;
        search_dbHVSC_entries_count=0;
        if (dbHVSC_entries_count) search_dbHVSC_entries=search_dbHVSC_entries_data;
        for (int j=0;j<dbHVSC_entries_count;j++)  {
            if ([self searchStringRegExp:mSearchText sourceString:dbHVSC_entries[j].label]) {
                search_dbHVSC_entries[search_dbHVSC_entries_count].label=dbHVSC_entries[j].label;
                search_dbHVSC_entries[search_dbHVSC_entries_count].downloaded=dbHVSC_entries[j].downloaded;
                search_dbHVSC_entries[search_dbHVSC_entries_count].rating=dbHVSC_entries[j].rating;
                search_dbHVSC_entries[search_dbHVSC_entries_count].playcount=dbHVSC_entries[j].playcount;
                search_dbHVSC_entries[search_dbHVSC_entries_count].filesize=dbHVSC_entries[j].filesize;
                
                search_dbHVSC_entries[search_dbHVSC_entries_count].id_md5=dbHVSC_entries[j].id_md5;
                search_dbHVSC_entries[search_dbHVSC_entries_count].fullpath=dbHVSC_entries[j].fullpath;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir1=dbHVSC_entries[j].dir1;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir2=dbHVSC_entries[j].dir2;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir3=dbHVSC_entries[j].dir3;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir4=dbHVSC_entries[j].dir4;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir5=dbHVSC_entries[j].dir5;
                
                search_dbHVSC_entries_count++;
                search_dbHVSC_nb_entries++;
            }
        }
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (dbHVSC_nb_entries) {
		for (int i=0;i<dbHVSC_nb_entries;i++) {
            dbHVSC_entries_data[i].label=nil;
            dbHVSC_entries_data[i].fullpath=nil;
            dbHVSC_entries_data[i].id_md5=nil;
            dbHVSC_entries_data[i].dir1=nil;
            dbHVSC_entries_data[i].dir2=nil;
            dbHVSC_entries_data[i].dir3=nil;
            dbHVSC_entries_data[i].dir4=nil;
            dbHVSC_entries_data[i].dir5=nil;
		}
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	dbHVSC_hasFiles=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir5) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is not null AND dir5 LIKE \"%%%s%%\"\
							 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is null AND filename LIKE \"%%%s%%\"",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT COUNT(DISTINCT dir5) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is not null\
					 UNION SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is null",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String]);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		
		
		
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			
			dbHVSC_entries_index=0;
				dbHVSC_entries_count=0;
				dbHVSC_entries=dbHVSC_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT dir5,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is not null AND dir5 LIKE \"%%%s%%\" GROUP BY dir5 \
								 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is null AND filename LIKE \"%%%s%%\" \
								 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[mSearchText UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT dir5,NULL,NULL,COUNT(1),0 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is not null GROUP BY dir5 \
						 UNION SELECT filename,fullpath,id_md5,NULL,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5 is null \
						 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String]);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					dbHVSC_entries[dbHVSC_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[dbHVSC_entries_count].dir1=[[NSString alloc] initWithString:dir1];
					dbHVSC_entries[dbHVSC_entries_count].dir2=[[NSString alloc] initWithString:dir2];
					dbHVSC_entries[dbHVSC_entries_count].dir3=[[NSString alloc] initWithString:dir3];
					dbHVSC_entries[dbHVSC_entries_count].dir4=[[NSString alloc] initWithString:dir4];
					if (sqlite3_column_int(stmt, 4)==0) {
						dbHVSC_entries[dbHVSC_entries_count].dir5=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
						dbHVSC_entries[dbHVSC_entries_count].filesize=sqlite3_column_int(stmt, 3);
					} else {
						dbHVSC_entries[dbHVSC_entries_count].id_md5=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 2)];
						dbHVSC_entries[dbHVSC_entries_count].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
						dbHVSC_hasFiles++;
					}
					
					dbHVSC_entries[dbHVSC_entries_count].downloaded=-1;
					dbHVSC_entries[dbHVSC_entries_count].rating=-1;
					dbHVSC_entries[dbHVSC_entries_count].playcount=-1;
					dbHVSC_entries_count++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
}
-(void) fillKeysWithHVSCDB_AllDirs:(NSString*)dir1 dir2:(NSString*)dir2 dir3:(NSString*)dir3 dir4:(NSString*)dir4 dir5:(NSString*)dir5 {
	NSString *pathToDB=[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:DATABASENAME_MAIN];
	sqlite3 *db;
	int dbHVSC_entries_index;
    
    if (search_dbHVSC_nb_entries) {
        for (int j=0;j<search_dbHVSC_entries_count;j++) {
            search_dbHVSC_entries[j].label=nil;
            search_dbHVSC_entries[j].fullpath=nil;
            search_dbHVSC_entries[j].id_md5=nil;
            search_dbHVSC_entries[j].dir1=nil;
            search_dbHVSC_entries[j].dir2=nil;
            search_dbHVSC_entries[j].dir3=nil;
            search_dbHVSC_entries[j].dir4=nil;
            search_dbHVSC_entries[j].dir5=nil;
        }
        search_dbHVSC_entries=NULL;
        search_dbHVSC_nb_entries=0;
        free(search_dbHVSC_entries_data);
    }
    
	// in case of search, do not ask DB again => duplicate already found entries & filter them
	if (mSearch) {
		search_dbHVSC=1;
		
		search_dbHVSC_entries_data=(t_dbHVSC_browse_entry*)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
		search_dbHVSC_hasFiles=0;
        search_dbHVSC_entries_count=0;
        if (dbHVSC_entries_count) search_dbHVSC_entries=search_dbHVSC_entries_data;
        for (int j=0;j<dbHVSC_entries_count;j++)  {
            if ([self searchStringRegExp:mSearchText sourceString:dbHVSC_entries[j].label]) {
                search_dbHVSC_entries[search_dbHVSC_entries_count].label=dbHVSC_entries[j].label;
                search_dbHVSC_entries[search_dbHVSC_entries_count].downloaded=dbHVSC_entries[j].downloaded;
                search_dbHVSC_entries[search_dbHVSC_entries_count].rating=dbHVSC_entries[j].rating;
                search_dbHVSC_entries[search_dbHVSC_entries_count].playcount=dbHVSC_entries[j].playcount;
                search_dbHVSC_entries[search_dbHVSC_entries_count].filesize=dbHVSC_entries[j].filesize;
                
                search_dbHVSC_entries[search_dbHVSC_entries_count].id_md5=dbHVSC_entries[j].id_md5;
                search_dbHVSC_entries[search_dbHVSC_entries_count].fullpath=dbHVSC_entries[j].fullpath;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir1=dbHVSC_entries[j].dir1;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir2=dbHVSC_entries[j].dir2;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir3=dbHVSC_entries[j].dir3;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir4=dbHVSC_entries[j].dir4;
                search_dbHVSC_entries[search_dbHVSC_entries_count].dir5=dbHVSC_entries[j].dir5;
                
                search_dbHVSC_entries_count++;
                search_dbHVSC_nb_entries++;
            }
        }
		return;
	}
	pthread_mutex_lock(&db_mutex);
	if (dbHVSC_nb_entries) {
		for (int i=0;i<dbHVSC_nb_entries;i++) {
            dbHVSC_entries_data[i].label=nil;
            dbHVSC_entries_data[i].fullpath=nil;
            dbHVSC_entries_data[i].id_md5=nil;
            dbHVSC_entries_data[i].dir1=nil;
            dbHVSC_entries_data[i].dir2=nil;
            dbHVSC_entries_data[i].dir3=nil;
            dbHVSC_entries_data[i].dir4=nil;
            dbHVSC_entries_data[i].dir5=nil;
		}
		free(dbHVSC_entries_data);dbHVSC_entries_data=NULL;
		dbHVSC_nb_entries=0;
	}
	dbHVSC_hasFiles=0;
	if (sqlite3_open([pathToDB UTF8String], &db) == SQLITE_OK){
		char sqlStatement[1024];
		sqlite3_stmt *stmt;
		int err;
        
        err=sqlite3_exec(db, "PRAGMA cache_size = 1;PRAGMA synchronous = 1;PRAGMA locking_mode = EXCLUSIVE;", 0, 0, 0);
        if (err==SQLITE_OK){
        } else MDZELog("ErrSQL : %d",err);
        
		//1st : count how many entries we'll have		
		if (mSearch) snprintf(sqlStatement,1024,"SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5=\"%s\" AND filename LIKE \"%%%s%%\"",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir5 UTF8String],[mSearchText UTF8String]);
		else snprintf(sqlStatement,1024,"SELECT COUNT(filename) FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5=\"%s\"",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir5 UTF8String]);
		
		err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
		if (err==SQLITE_OK){
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				dbHVSC_nb_entries+=sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		} else MDZELog("ErrSQL : %d",err);
		
		
		
		if (dbHVSC_nb_entries) {
			//2nd initialize array to receive entries
			dbHVSC_entries_data=(t_dbHVSC_browse_entry *)calloc(1,dbHVSC_nb_entries*sizeof(t_dbHVSC_browse_entry));
			
			dbHVSC_entries_index=0;
				dbHVSC_entries_count=0;
				dbHVSC_entries=dbHVSC_entries_data;
			//3rd get the entries
			if (mSearch) snprintf(sqlStatement,1024,"SELECT filename,fullpath,id_md5,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5=\"%s\" AND filename LIKE \"%%%s%%\" \
								 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir5 UTF8String],[mSearchText UTF8String]);
			else snprintf(sqlStatement,1024,"SELECT filename,fullpath,id_md5,1 FROM hvsc_file WHERE dir1=\"%s\" AND dir2=\"%s\" AND dir3=\"%s\" AND dir4=\"%s\" AND dir5=\"%s\"\
						 ORDER BY 1 COLLATE NOCASE",[dir1 UTF8String],[dir2 UTF8String],[dir3 UTF8String],[dir4 UTF8String],[dir5 UTF8String]);
			
			err=sqlite3_prepare_v2(db, sqlStatement, -1, &stmt, NULL);
			if (err==SQLITE_OK){
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					char *str=(char*)sqlite3_column_text(stmt, 0);
					dbHVSC_entries[dbHVSC_entries_count].label=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 0)];
					dbHVSC_entries[dbHVSC_entries_count].dir1=[[NSString alloc] initWithString:dir1];
					dbHVSC_entries[dbHVSC_entries_count].dir2=[[NSString alloc] initWithString:dir2];
					dbHVSC_entries[dbHVSC_entries_count].dir3=[[NSString alloc] initWithString:dir3];
					dbHVSC_entries[dbHVSC_entries_count].dir4=[[NSString alloc] initWithString:dir4];
					dbHVSC_entries[dbHVSC_entries_count].dir5=[[NSString alloc] initWithString:dir5];
					
					dbHVSC_entries[dbHVSC_entries_count].id_md5=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 2)];
					dbHVSC_entries[dbHVSC_entries_count].fullpath=[[NSString alloc] initWithUTF8String:(const char*)sqlite3_column_text(stmt, 1)];
					dbHVSC_hasFiles++;
					
					dbHVSC_entries[dbHVSC_entries_count].downloaded=-1;					
					dbHVSC_entries[dbHVSC_entries_count].rating=-1;
					dbHVSC_entries[dbHVSC_entries_count].playcount=-1;
					dbHVSC_entries_count++;
					dbHVSC_entries_index++;
				}
				sqlite3_finalize(stmt);
			} else MDZELog("ErrSQL : %d",err);
			
		}
	};
	sqlite3_close(db);
	pthread_mutex_unlock(&db_mutex);
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

-(void) updRadioButton {
}

-(void) updRadioStatus {
    if ([detailViewController.radioSource isActive]&&(detailViewController.radioSource.mRadioSource==RS_COLLECTION_HVSC)) {
        [radioButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    } else {
        [radioButton setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    }
    [self updRadioButton];
}


-(void) viewWillAppear:(BOOL)animated {
//    [self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    [self.sBar setBarStyle:UIBarStyleDefault];
    //[[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleDefault animated:YES];
    
    self.navigationController.delegate = self;
    
    [self.updRSTimer invalidate];
    self.updRSTimer = nil;
    self.updRSTimer = [NSTimer scheduledTimerWithTimeInterval:0.3
                                                                 target:self
                                                               selector:@selector(updRadioStatus)
                                                               userInfo:nil
                                                                repeats:YES];
    if ([detailViewController.radioSource isActive]&&(detailViewController.radioSource.mRadioSource==RS_COLLECTION_HVSC)) {
        [radioButton setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    } else {
        [radioButton setTitleColor:[UIColor grayColor] forState:UIControlStateNormal];
    }
    
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
    
    [self updRadioButton];
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

#pragma mark -
#pragma mark Table view data source

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return nil;
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (search_dbHVSC) {
        return search_dbHVSC_entries_count+(search_dbHVSC_hasFiles?1:0);
    } else {
        return dbHVSC_entries_count+(dbHVSC_hasFiles?1:0);
    }
    return 0;
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
        
        UIBackgroundConfiguration *backgroundConfig = [UIBackgroundConfiguration listGroupedCellConfiguration];
        backgroundConfig.backgroundColor = [UIColor systemGroupedBackgroundColor];
        cell.backgroundConfiguration = backgroundConfig;
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
        topLabel.font = [UIFont systemFontOfSize:17 weight:MDZ_UIFONT_WEIGHT];
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
    } else { //HVSC
        t_dbHVSC_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
        int download_all=0;
        if (search_dbHVSC) {
            if (search_dbHVSC_hasFiles) download_all=1;
        } else {
            if (dbHVSC_hasFiles) download_all=1;
        }
        int crow=indexPath.row;
        crow-=download_all;
        
        if (download_all&&(crow==-1)) {
            cellValue=NSLocalizedString(@"GetAllEntries_MainKey","");
            if (darkMode) topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED_DARKMODE green:ACTION_COLOR_GREEN_DARKMODE blue:ACTION_COLOR_BLUE_DARKMODE alpha:1.0];
            else topLabel.textColor=[UIColor colorWithRed:ACTION_COLOR_RED green:ACTION_COLOR_GREEN blue:ACTION_COLOR_BLUE alpha:1.0];
            bottomLabel.text=NSLocalizedString(@"GetAllEntries_SubKey","");
        } else {
            cellValue=cur_db_entries[crow].label;
            int colFactor;
            //update downloaded if needed
            if(cur_db_entries[crow].downloaded==-1) {
                NSString *pathToCheck=nil;
                
                if (cur_db_entries[crow].fullpath)
                    pathToCheck=[NSString stringWithFormat:@"%@/Documents/%@%@",[ModizFileHelper getAppHomeDirectory],HVSC_BASEDIR,cur_db_entries[crow].fullpath];
                if (pathToCheck) {
                    if ([mFileMngr fileExistsAtPath:pathToCheck]) cur_db_entries[crow].downloaded=1;
                    else cur_db_entries[crow].downloaded=0;
                } else cur_db_entries[crow].downloaded=0;
            }
            
            if(cur_db_entries[crow].downloaded==1) {
                colFactor=1;
            } else colFactor=0;
            
            if (cur_db_entries[crow].id_md5) { //FILE
                if (colFactor==0) topLabel.textColor=[UIColor colorWithRed:0.5f green:0.5f blue:0.5f alpha:1.0];
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth- 32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                           22);
                if (cur_db_entries[crow].downloaded==1) {
                    if (cur_db_entries[crow].rating==-1) {
                        DBHelper::getFileStatsDBmod([NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[crow].fullpath],
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
                    
                    if (cur_db_entries[crow].songs>0) {
                        if (cur_db_entries[crow].songs==1) bottomStr=[NSString stringWithFormat:@"%@|1 song",bottomStr];
                        else bottomStr=[NSString stringWithFormat:@"%@|%d songs",bottomStr,cur_db_entries[crow].songs];
                    } else bottomStr=[NSString stringWithFormat:@"%@|- song",bottomStr];
                    bottomStr=[NSString stringWithFormat:@"%@|Pl:%d",bottomStr,cur_db_entries[crow].playcount];
                    
                    bottomLabel.text=bottomStr;
                    
                    bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth+60,
                                                   22,
                                                   tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE-60,
                                                   18);
                } else {
                }
                if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0) {
                    [actionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateNormal];
                    [actionView setImage:[UIImage imageNamed:@"playlist_add.png"] forState:UIControlStateHighlighted];
                    [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                    [actionView addTarget: self action: @selector(secondaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                    [dictActionBtn setObject:[NSNumber numberWithInteger:crow*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
                    [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[[actionView.description componentsSeparatedByString:@";"] firstObject] stringByAppendingString:@"realIndexP"]];
                } else {
                    [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateNormal];
                    [actionView setImage:[UIImage imageNamed:@"play.png"] forState:UIControlStateHighlighted];
                    [actionView removeTarget: self action:NULL forControlEvents: UIControlEventTouchUpInside];
                    [actionView addTarget: self action: @selector(primaryActionTapped:) forControlEvents: UIControlEventTouchUpInside];
                    [dictActionBtn setObject:[NSNumber numberWithInteger:crow*100+indexPath.section] forKey:[[actionView.description componentsSeparatedByString:@";"] firstObject]];
                    [dictActionBtn setObject:[NSNumber numberWithInteger:indexPath.row*100+indexPath.section] forKey:[[[actionView.description componentsSeparatedByString:@";"] firstObject] stringByAppendingString:@"realIndexP"]];
                }
                actionView.frame = CGRectMake(tabView.bounds.size.width-2-32-PRI_SEC_ACTIONS_IMAGE_SIZE-tabView.safeAreaInsets.left-tabView.safeAreaInsets.right,0,PRI_SEC_ACTIONS_IMAGE_SIZE,PRI_SEC_ACTIONS_IMAGE_SIZE);
                actionView.enabled=YES;
                actionView.hidden=NO;
                
            } else { // DIR
                bottomLabel.frame = CGRectMake( 1.0 * cell.indentationWidth,
                                               22,
                                               tabView.bounds.size.width -1.0 * cell.indentationWidth-32-PRI_SEC_ACTIONS_IMAGE_SIZE,
                                               18);
                bottomLabel.text=[NSString stringWithFormat:(cur_db_entries[crow].filesize>1?nbFiles:nb1File),cur_db_entries[crow].filesize];
                topLabel.frame= CGRectMake(1.0 * cell.indentationWidth,
                                           0,
                                           tabView.bounds.size.width -1.0 * cell.indentationWidth- 32,
                                           22);
                if (darkMode) topLabel.textColor=[UIColor colorWithRed:MDZ_FOLDER_DARK_R green:MDZ_FOLDER_DARK_G blue:MDZ_FOLDER_DARK_B alpha:1.0f];
                else topLabel.textColor=[UIColor colorWithRed:MDZ_FOLDER_LIGHT_R green:MDZ_FOLDER_LIGHT_G blue:MDZ_FOLDER_LIGHT_B alpha:1.0f];
                cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
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

        t_dbHVSC_browse_entry *cur_db_entries;
        cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
        
        int crow=indexPath.row;
        if (search_dbHVSC) {
            if (search_dbHVSC_hasFiles) crow--;
        } else {
            if (dbHVSC_hasFiles) crow--;
        }

        //delete file
        NSString *fullpath=[[ModizFileHelper getAppHomeDirectory] stringByAppendingPathComponent:[NSString stringWithFormat:@"/Documents/%@%@",HVSC_BASEDIR,cur_db_entries[crow].fullpath]];
        NSError *err;
        DBHelper::deleteStatsFileDB(fullpath);
        cur_db_entries[crow].downloaded=0;

        //delete local file
        [mFileMngr removeItemAtPath:fullpath error:&err];

        // Reload the cell to show the file is no longer downloaded
        [tableView reloadRowsAtIndexPaths:@[indexPath]
                         withRowAnimation:UITableViewRowAnimationAutomatic];

        completionHandler(YES);
    }];
    deleteAction.backgroundColor = [UIColor redColor];

    // Return multiple actions - they appear from right to left
    return [UISwipeActionsConfiguration configurationWithActions:@[deleteAction]];
}


- (BOOL)tableView:(UITableView *)tabView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
    t_dbHVSC_browse_entry *cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
    int crow=indexPath.row;
    if (search_dbHVSC) {
        if (search_dbHVSC_hasFiles) crow--;
    } else {
        if (dbHVSC_hasFiles) crow--;
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
    //if (mSearchText) [mSearchText release];
    
    mSearchText=[[NSString alloc] initWithString:searchText];
    
    [self fillKeys];
    [tableView reloadData];
}
- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
    //if (mSearchText) [mSearchText release];
    mSearchText=nil;
    sBar.text=nil;
    sBar.showsCancelButton = NO;
    [searchBar resignFirstResponder];
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
    
    value=(NSNumber*)[dictActionBtn objectForKey:[[[sender.description componentsSeparatedByString:@";"] firstObject] stringByAppendingFormat:@"realIndexP"] ];
    if (value==NULL) return;
    NSIndexPath *realIndexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    [tableView selectRowAtIndexPath:realIndexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    
    if (browse_depth==0) {
        
    } else {
            t_dbHVSC_browse_entry *cur_db_entries;
            cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
        int crow = indexPath.row;
            int download_all=0;
            if (search_dbHVSC) {
                if (search_dbHVSC_hasFiles) download_all=1;
            } else {
                if (dbHVSC_hasFiles) download_all=1;
            }
            //crow-=download_all;
            
            if (cur_db_entries[crow].id_md5) { //FILE
                //File selected, start download is needed
                NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[crow].label];
                NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[crow].fullpath];
                NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[crow].fullpath];
                
                if (cur_db_entries[crow].downloaded==1) {
                    NSMutableArray *array_label = [[NSMutableArray alloc] init];
                    NSMutableArray *array_path = [[NSMutableArray alloc] init];
                    [array_label addObject:sidFilename];
                    [array_path addObject:localPath];
                    cur_db_entries[crow].rating=-1;
                    [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                    if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                    
                    [tableView reloadData];
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
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    value=(NSNumber*)[dictActionBtn objectForKey:[[[sender.description componentsSeparatedByString:@";"] firstObject] stringByAppendingFormat:@"realIndexP"] ];
    if (value==NULL) return;
    NSIndexPath *realIndexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    [tableView selectRowAtIndexPath:realIndexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
    [self showWaiting];
    [self flushMainLoop];
    
    
    if (browse_depth==0) {
    } else {
            t_dbHVSC_browse_entry *cur_db_entries;
            cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
            int download_all=0;
            int crow=indexPath.row;
            if (search_dbHVSC) {
                if (search_dbHVSC_hasFiles) download_all=1;
            } else {
                if (dbHVSC_hasFiles) download_all=1;
            }
            //crow-=download_all;
            
            if (cur_db_entries[crow].id_md5) { //FILE
                //File selected, start download is needed
                NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[crow].label];
                NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[crow].fullpath];
                NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[crow].fullpath];
                mClickedPrimAction=2;
                
                if (cur_db_entries[crow].downloaded==1) {
                    //add to playlist
                    [self addToPlaylistSelView:localPath label:sidFilename showNowListening:true];
                    
                    cur_db_entries[crow].rating=-1;
                    [tableView reloadData];
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
    //NSIndexPath *indexPath = [tableView indexPathForRowAtPoint:[sender convertPoint:CGPointZero toView:self.tableView]];
    NSNumber *value=(NSNumber*)[dictActionBtn objectForKey:[[sender.description componentsSeparatedByString:@";"] firstObject] ];
    if (value==NULL) return;
    NSIndexPath *indexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    
    value=(NSNumber*)[dictActionBtn objectForKey:[[[sender.description componentsSeparatedByString:@";"] firstObject] stringByAppendingFormat:@"realIndexP"] ];
    if (value==NULL) return;
    NSIndexPath *realIndexPath=[NSIndexPath indexPathForRow:(value.longValue/100) inSection:(value.longValue%100)];
    [tableView selectRowAtIndexPath:realIndexPath animated:FALSE scrollPosition:UITableViewScrollPositionNone];
    
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
            t_dbHVSC_browse_entry *cur_db_entries;
            cur_db_entries=(search_dbHVSC?search_dbHVSC_entries:dbHVSC_entries);
            int crow =(int)( indexPath.row);
            int download_all=0;
            if (search_dbHVSC) {
                if (search_dbHVSC_hasFiles) download_all=1;
            } else {
                if (dbHVSC_hasFiles) download_all=1;
            }
            crow-=download_all;
            
            if (download_all&& (crow==-1)) {
                //download all dir
                NSString *sidFilename;
                NSString *ftpPath;
                NSString *localPath;
                int first=0; //1;  Do not play even first file => TODO : add a setting for this
                int existing;
                int tooMuch=0;
                if (settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==2) first=0;//enqueue only
                
                int cur_db_entries_count=(search_dbHVSC?search_dbHVSC_entries_count:dbHVSC_entries_count);
                
                    for (int j=0;j<cur_db_entries_count;j++) {
                        if (cur_db_entries[j].id_md5) {//mod found
                            
                            existing=cur_db_entries[j].downloaded;
                            if (existing==-1) {
                                NSString *pathToCheck=nil;
                                
                                if (cur_db_entries[j].fullpath)
                                    pathToCheck=[NSString stringWithFormat:@"%@/Documents/%@%@",[ModizFileHelper getAppHomeDirectory],HVSC_BASEDIR,cur_db_entries[j].fullpath];
                                if (pathToCheck) {
                                    if ([mFileMngr fileExistsAtPath:pathToCheck]) cur_db_entries[j].downloaded=1;
                                    else existing=cur_db_entries[j].downloaded=0;
                                } else existing=cur_db_entries[j].downloaded=0;
                            }
                            if (existing==0) {
                                
                                //File selected, start download is needed
                                sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[j].label];
                                ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[j].fullpath];
                                localPath=[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[j].fullpath];
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
            } else {
                
                if (cur_db_entries[crow].id_md5) { //FILE
                    //File selected, start download is needed
                    NSString *sidFilename=[NSString stringWithFormat:@"%@",cur_db_entries[crow].label];
                    NSString *ftpPath=[NSString stringWithFormat:@"%@",cur_db_entries[crow].fullpath];
                    NSString *localPath=[NSString stringWithFormat:@"Documents/%@%@",HVSC_BASEDIR,cur_db_entries[crow].fullpath];
                    mClickedPrimAction=(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==0);
                    
                    if (cur_db_entries[crow].downloaded==1) {
                        if (mClickedPrimAction) {
                            NSMutableArray *array_label = [[NSMutableArray alloc] init];
                            NSMutableArray *array_path = [[NSMutableArray alloc] init];
                            [array_label addObject:sidFilename];
                            [array_path addObject:localPath];
                            cur_db_entries[crow].rating=-1;
                            [detailViewController play_listmodules:array_label start_index:0 path:array_path];
                            if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                            
                            [tabView reloadData];
                        } else {
                            if ([detailViewController add_to_playlist:localPath fileName:sidFilename forcenoplay:(settings[GLOB_PlayEnqueueAction].detail.mdz_switch.switch_value==1)]) {
                                if ([detailViewController.mplayer isPlaying]) [self showMiniPlayer];
                                
                                cur_db_entries[crow].rating=-1;
                                [tabView reloadData];
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
                        mDir1=cur_db_entries[crow].dir1;
                    } else if (browse_depth==2) {//DIR2
                        mDir2=cur_db_entries[crow].dir2;
                    } else if (browse_depth==3) {//DIR3
                        mDir3=cur_db_entries[crow].dir3;
                    } else if (browse_depth==4) {//DIR4
                        mDir4=cur_db_entries[crow].dir4;
                    } else if (browse_depth==5) {//DIR5
                        mDir5=cur_db_entries[crow].dir5;
                    }
                    
                    if (childController == nil) childController = [[RootViewControllerHVSC alloc]  initWithNibName:@"CollectionViewController" bundle:[NSBundle mainBundle]];
                    else {// Don't cache childviews
                    }
                    
                    childController.title = cur_db_entries[crow].label;
                    // Set new depth
                    ((RootViewControllerHVSC*)childController)->browse_depth = browse_depth+1;
                    ((RootViewControllerHVSC*)childController)->detailViewController=detailViewController;
                    ((RootViewControllerHVSC*)childController)->downloadViewController=downloadViewController;
                    
                    ((RootViewControllerHVSC*)childController)->mDir1 = mDir1;
                    ((RootViewControllerHVSC*)childController)->mDir2 = mDir2;
                    ((RootViewControllerHVSC*)childController)->mDir3 = mDir3;
                    ((RootViewControllerHVSC*)childController)->mDir4 = mDir4;
                    ((RootViewControllerHVSC*)childController)->mDir5 = mDir5;
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
    
    if (dbHVSC_nb_entries) {
        for (int i=0;i<dbHVSC_nb_entries;i++) {
            dbHVSC_entries_data[i].label=nil;
            dbHVSC_entries_data[i].fullpath=nil;
            dbHVSC_entries_data[i].id_md5=nil;
            dbHVSC_entries_data[i].dir1=nil;
            dbHVSC_entries_data[i].dir2=nil;
            dbHVSC_entries_data[i].dir3=nil;
            dbHVSC_entries_data[i].dir4=nil;
            dbHVSC_entries_data[i].dir5=nil;
            
        }
        free(dbHVSC_entries_data);
    }
    if (search_dbHVSC_nb_entries) {
        for (int j=0;j<search_dbHVSC_entries_count;j++) {
            search_dbHVSC_entries[j].label=nil;
            search_dbHVSC_entries[j].fullpath=nil;
            search_dbHVSC_entries[j].id_md5=nil;
            search_dbHVSC_entries[j].dir1=nil;
            search_dbHVSC_entries[j].dir2=nil;
            search_dbHVSC_entries[j].dir3=nil;
            search_dbHVSC_entries[j].dir4=nil;
            search_dbHVSC_entries[j].dir5=nil;
        }
        search_dbHVSC_entries=NULL;
        search_dbHVSC_nb_entries=0;
        free(search_dbHVSC_entries_data);
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
